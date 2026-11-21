#include "gallerywindow.h"
#include "ui_gallerywindow.h"

#include <memory>

#include <QApplication>
#include <QActionGroup>
#include <QCalendarWidget>
#include <QColorDialog>
#include <QCompleter>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDockWidget>
#include <QEvent>
#include <QEventLoop>
#include <QFont>
#include <winui3style/winui3icons.h>
#include <QGuiApplication>
#include <QHeaderView>
#include <QKeySequence>
#include <QLabel>
#include <QLocale>
#include <QMessageBox>
#include <QMenu>
#include <QMetaEnum>
#include <QMouseEvent>
#include <QPainter>
#include <QPlainTextEdit>
#include <QScreen>
#include <QScopeGuard>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QStyle>
#include <QStyleFactory>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QToolBox>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWindow>
#include <QWizard>
#include <QWizardPage>

namespace {
// Matches codepointer's CommitForm PathElideDelegate: the file path is elided
// on the left before the stock item view item is painted, so the available
// width is the column width and not the model's full path string.
class PathElideDelegate final : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        QStyleOptionViewItem opt(option);
        initStyleOption(&opt, index);
        opt.text = opt.fontMetrics.elidedText(opt.text, Qt::ElideLeft, opt.rect.width() - 10);
        opt.textElideMode = Qt::ElideNone;
        QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
        style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);
    }
};

void configureContentDialog(QDialog *dialog)
{
    dialog->setProperty("winuiContentDialog", true);
    dialog->setWindowTitle(QObject::tr("Content dialog"));
    dialog->resize(420, 220);
    auto *layout = new QVBoxLayout(dialog);
    auto *title = new QLabel(QObject::tr("Content dialog"));
    QFont titleFont = title->font();
    titleFont.setPixelSize(20);
    titleFont.setWeight(QFont::DemiBold);
    title->setFont(titleFont);
    layout->addWidget(title);
    auto *description = new QLabel(QObject::tr(
            "Every control remains a native QWidget configured through Qt properties."));
    description->setWordWrap(true);
    layout->addWidget(description);
    layout->addStretch();
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    if (auto *primary = buttons->button(QDialogButtonBox::Ok)) {
        primary->setDefault(true);
        primary->setMinimumWidth(120);
    }
    if (auto *close = buttons->button(QDialogButtonBox::Cancel))
        close->setMinimumWidth(120);
    QObject::connect(buttons, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    layout->addWidget(buttons);
}

void setHeadingFont(QLabel *heading)
{
    QFont font = heading->font();
    font.setPixelSize(28);
    font.setWeight(QFont::DemiBold);
    heading->setFont(font);
}

QCompleter *makeAutoSuggestCompleter(const QStringList &words, QWidget *parent)
{
    auto *completer = new QCompleter(words, parent);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    return completer;
}
} // namespace

GalleryWindow::GalleryWindow(QWidget *parent, bool captureMode)
    : QMainWindow(parent), ui(new Ui::GalleryWindow)
{
    ui->setupUi(this);
    configureGallery();
    populateCollections();
    populateIconSheet();
    configurePaletteLab(captureMode);
    // Opt-in live diagnostic: exercises popup open/close/reopen with real
    // windows QPA so DWM grant behavior on a reused HWND can be observed.
    // Driven by WINUI3STYLE_LIVE_MENUS=first|double.
    const QString liveMode = qEnvironmentVariable("WINUI3STYLE_LIVE_MENUS");
    if (!liveMode.isEmpty()) {
        const int delay = 1500;
        QTimer *firstTimer = new QTimer(this);
        firstTimer->setSingleShot(true);
        QObject::connect(firstTimer, &QTimer::timeout, this, [this] {
            ui->fileMenu->popup(ui->menuBar->mapToGlobal(
                    QPoint(ui->menuBar->actionGeometry(ui->menuBar->actions().first()).left(),
                           ui->menuBar->height())));
        });
        firstTimer->start(delay);
        if (liveMode == QLatin1String("double")) {
            QTimer *closeTimer = new QTimer(this);
            closeTimer->setSingleShot(true);
            QObject::connect(closeTimer, &QTimer::timeout, this, [this] { ui->fileMenu->close(); });
            QTimer *secondTimer = new QTimer(this);
            secondTimer->setSingleShot(true);
            QObject::connect(secondTimer, &QTimer::timeout, this, [this] {
                ui->fileMenu->popup(ui->menuBar->mapToGlobal(
                        QPoint(ui->menuBar->actionGeometry(ui->menuBar->actions().first()).left(),
                               ui->menuBar->height())));
            });
            closeTimer->start(delay + 4000);
            secondTimer->start(delay + 7000);
        }
    }
}

GalleryWindow::~GalleryWindow()
{
    delete ui;
}

void GalleryWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);
    // A style swap changes which standard pixmaps the style answers with its
    // own glyphs, so the Icons page is rebuilt, not just re-rendered. Compare
    // the style instance: a child widget of the visible page sends StyleChange
    // for its own first polish while the application style has not moved.
    // Rebuilding inline would delete children while QApplication is still
    // broadcasting that style change, so the rebuild waits for the event loop.
    if (event->type() == QEvent::StyleChange && ui->iconsPage && style() != m_iconSheetStyle
        && !m_iconSheetRebuildPending) {
        m_iconSheetRebuildPending = true;
        QTimer::singleShot(0, this, [this] {
            m_iconSheetRebuildPending = false;
            populateIconSheet();
        });
    }
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::LayoutDirectionChange) {
        refreshCardIcons();
        refreshIconSheet();
    }
}

void GalleryWindow::refreshCardIcons()
{
    const bool rtl = layoutDirection() == Qt::RightToLeft;
    // Designer owns the LTR composition. Qt layout margins are physical, so
    // orient each asymmetric pair without copying its values into C++.
    const auto orientMargins = [rtl](QLayout *layout, bool leadingIsLarger) {
        const QMargins margins = layout->contentsMargins();
        const int smaller = qMin(margins.left(), margins.right());
        const int larger = qMax(margins.left(), margins.right());
        const int leading = leadingIsLarger ? larger : smaller;
        const int trailing = leadingIsLarger ? smaller : larger;
        layout->setContentsMargins(rtl ? trailing : leading, margins.top(),
                                   rtl ? leading : trailing, margins.bottom());
    };
    for (QLayout *layout : { static_cast<QLayout *>(ui->notificationsCardIconLayout),
                             static_cast<QLayout *>(ui->updatesCardIconLayout),
                             static_cast<QLayout *>(ui->advancedIconLayout),
                             static_cast<QLayout *>(ui->advancedHeaderLayout),
                             static_cast<QLayout *>(ui->advancedTextLayout) })
        orientMargins(layout, false);
    orientMargins(ui->advancedDetailsLayout, true);

    // SettingsCard header icon: SettingsCardHeaderIconMaxSize 20, drawn from
    // the style's themed standard icons. A QLabel stores a plain pixmap, so a
    // theme switch must re-render it or the old ink stays on the new surface.
    const auto setCardIcon = [this](QLabel *label, QStyle::StandardPixmap pixmap) {
        label->setPixmap(style()->standardIcon(pixmap, nullptr, this).pixmap(QSize(20, 20)));
    };
    setCardIcon(ui->notificationsCardIcon, QStyle::SP_MessageBoxInformation);
    setCardIcon(ui->updatesCardIcon, QStyle::SP_BrowserReload);
    setCardIcon(ui->advancedIcon, QStyle::SP_FileDialogDetailedView);
    // SettingsCardDescriptionFontSize is 12 px in SettingsCard.xaml. uic has
    // no pixel-size font serialization, so the stock labels get it here.
    for (QLabel *description : { ui->notificationsCardDescription, ui->updatesCardDescription,
                                 ui->advancedDescription }) {
        QFont font = description->font();
        font.setPixelSize(12);
        description->setFont(font);
    }
}

void GalleryWindow::configureGallery()
{
    // Let the scroll area honor the embedded QMainWindow/QDockWidget minimum
    // height instead of compressing the Designer-owned dock host to zero.
    ui->dialogsLayout->setSizeConstraint(QLayout::SetMinimumSize);
    // QMainWindow's default window type remains top-level even when uic gives
    // it a parent. The old imperative gallery explicitly embedded this host;
    // preserve that lifecycle while keeping its contents Designer-owned.
    ui->dockHost->setWindowFlags(Qt::Widget);
    ui->dockHost->show();
    for (QLabel *heading : { ui->controlsHeading, ui->collectionsHeading, ui->settingsHeading,
                             ui->dialogsHeading, ui->paletteHeading, ui->iconsHeading })
        setHeadingFont(heading);
    ui->themeCombo->setMinimumWidth(ui->themeCombo->sizeHint().width());
    ui->densityCombo->setMinimumWidth(ui->densityCombo->sizeHint().width());
    m_themeIndex = qApp->style()->property("themeMode").toInt();
    m_densityIndex = qApp->style()->property("densityMode").toInt();
    auto *styleMenu = ui->viewMenu->addMenu(tr("Style"));
    styleMenu->setObjectName(QStringLiteral("comparisonStyleMenu"));
    auto *styleGroup = new QActionGroup(this);
    styleGroup->setExclusive(true);
    auto *winuiAction = styleGroup->addAction(tr("WinUI 3 style"));
    winuiAction->setObjectName(QStringLiteral("winui3StyleAction"));
    winuiAction->setCheckable(true);
    winuiAction->setChecked(true);
    styleMenu->addAction(winuiAction);
    QString windows11Key;
    for (const QString &key : QStyleFactory::keys()) {
        if (key.compare(QStringLiteral("windows11"), Qt::CaseInsensitive) == 0) {
            windows11Key = key;
            break;
        }
    }
    QStyle *nativeProbe = windows11Key.isEmpty() ? nullptr : QStyleFactory::create(windows11Key);
    const bool nativeStyleAvailable = nativeProbe != nullptr;
    auto *windows11Action = styleGroup->addAction(nativeProbe ? tr("Windows 11 style")
                                                              : tr("Windows 11 unavailable"));
    delete nativeProbe;
    windows11Action->setObjectName(QStringLiteral("windows11StyleAction"));
    windows11Action->setCheckable(true);
    windows11Action->setEnabled(nativeStyleAvailable);
    if (!nativeStyleAvailable) {
        windows11Action->setToolTip(tr("This Qt build has no Windows 11 QStyleFactory plugin."));
    }
    styleMenu->addAction(windows11Action);
    connect(winuiAction, &QAction::triggered, this,
            [this] { setComparisonStyle(QStringLiteral("winui3")); });
    if (nativeStyleAvailable) {
        connect(windows11Action, &QAction::triggered, this,
                [this, windows11Key] { setComparisonStyle(windows11Key); });
    }
    ui->themeCombo->setCurrentIndex(m_themeIndex);
    ui->densityCombo->setCurrentIndex(m_densityIndex);
    const QStringList autoSuggestWords = { tr("Alpha"), tr("Beta"),     tr("Gamma"),
                                           tr("Delta"), tr("Settings"), tr("Controls") };
    ui->autoSuggestEdit->setCompleter(
            makeAutoSuggestCompleter(autoSuggestWords, ui->autoSuggestEdit));
    ui->standardDensityAutoSuggest->setCompleter(
            makeAutoSuggestCompleter(autoSuggestWords, ui->standardDensityAutoSuggest));
    ui->compactDensityAutoSuggest->setCompleter(
            makeAutoSuggestCompleter(autoSuggestWords, ui->compactDensityAutoSuggest));

    // QKeySequenceEdit has no WinUI counterpart. It stays a stock widget: the
    // style recognizes the recorder's private editor and paints the TextBox
    // field plus KeyTip-derived keycaps, so the Gallery adds no code at all.
    // The presets keep the rest states legible; clicking and typing records.
    ui->shortcutEdit->setKeySequence(QKeySequence(QStringLiteral("Ctrl+K")));
    ui->modifierShortcutEdit->setKeySequence(QKeySequence(QStringLiteral("Meta+Shift+K")));

    // QToolBox -> WinUI Expander accordion: three pages so the live pass can
    // compare open/closed headers, hover/press on the chevron, and the shared
    // card surface Qt builds from each page button's own palette role.
    const QStringList expanderPages = { tr("Expander one"), tr("Expander two"),
                                        tr("Expander three") };
    for (const QString &title : expanderPages) {
        auto *body = new QLabel(tr("ExpandDown body (Qt owns this page surface)"), this);
        body->setContentsMargins(16, 8, 16, 8);
        ui->expandToolBox->addItem(body, title);
    }
    ui->expandToolBox->setCurrentIndex(0);

    const auto icon = [this](QStyle::StandardPixmap pixmap) {
        return style()->standardIcon(pixmap, nullptr, this);
    };
    ui->newProjectAction->setIcon(icon(QStyle::SP_FileIcon));
    ui->openProjectAction->setIcon(icon(QStyle::SP_DirOpenIcon));
    ui->newAction->setIcon(icon(QStyle::SP_FileIcon));
    ui->saveAction->setIcon(icon(QStyle::SP_DialogSaveButton));
    ui->previewAction->setIcon(icon(QStyle::SP_MediaPlay));
    ui->moreAction->setIcon(icon(QStyle::SP_TitleBarMenuButton));
    ui->textToolAction->setIcon(icon(QStyle::SP_FileDialogDetailedView));
    ui->pinAction->setIcon(icon(QStyle::SP_DialogApplyButton));
    ui->standaloneToolButton->setIcon(icon(QStyle::SP_FileDialogDetailedView));
    ui->splitToolButton->setIcon(icon(QStyle::SP_DirOpenIcon));

    // Visible toolbar switch between this style and the native comparison.
    // Windows 11 style ships only with Qt's Windows plugin, so Linux builds
    // compare against the always-available Fusion style instead.
    if (QAction *styleToggle = findChild<QAction *>(QStringLiteral("comparisonToggleAction"))) {
        styleToggle->setIcon(icon(QStyle::SP_DirHomeIcon));
        const QString comparisonKey =
                nativeStyleAvailable ? windows11Key : QStringLiteral("Fusion");
        connect(styleToggle, &QAction::triggered, this, [this, comparisonKey](bool checked) {
            setComparisonStyle(checked ? QStringLiteral("winui3") : comparisonKey);
        });
    }
    refreshCardIcons();
    ui->iconCombo->setItemIcon(0, icon(QStyle::SP_FileIcon));
    ui->iconCombo->setItemIcon(1, icon(QStyle::SP_DirIcon));
    auto *buttonMenu = new QMenu(ui->menuButton);
    buttonMenu->addAction(icon(QStyle::SP_FileIcon), tr("Create document"));
    buttonMenu->addAction(icon(QStyle::SP_DirOpenIcon), tr("Open document"));
    ui->menuButton->setMenu(buttonMenu);
    auto *splitMenu = new QMenu(ui->splitToolButton);
    splitMenu->addAction(tr("Run normally"));
    splitMenu->addAction(tr("Run with options"));
    ui->splitToolButton->setMenu(splitMenu);
    ui->statusBar->showMessage(tr("Ready (QSizeGrip enabled)"));
    // Real hover targets for the WinUI ToolTip surface: the coverage evidence
    // needs a live tooltip in the product demo, not only a synthetic probe.
    ui->themeCombo->setToolTip(tr("Choose the light, dark or system theme."));
    ui->densityCombo->setToolTip(tr("Standard controls are 32 px high; Compact fits more."));
    ui->floatDockButton->setToolTip(tr("Float or redock the inspector dock."));
    ui->autoSuggestEdit->setToolTip(tr("Type to see completion suggestions."));
    connect(ui->floatDockButton, &QPushButton::clicked, ui->embeddedInspector,
            [this] { ui->embeddedInspector->setFloating(!ui->embeddedInspector->isFloating()); });
    const QList<QStyle::StandardPixmap> navigationIcons = {
        QStyle::SP_DesktopIcon,
        QStyle::SP_DirIcon,
        QStyle::SP_FileDialogContentsView,
        QStyle::SP_MessageBoxInformation,
        QStyle::SP_FileDialogDetailedView,
        QStyle::SP_ToolBarHorizontalExtensionButton,
        QStyle::SP_FileDialogListView
    };
    for (int row = 0; row < ui->navigationList->count(); ++row)
        ui->navigationList->item(row)->setIcon(icon(navigationIcons.at(row)));
    // Use the style's Fluent icon engine (the same QIcon path QMenu uses) so
    // Qt centres these decorations like any other item-view icon.
    ui->navigationList->item(0)->setIcon(WinUI3::icon(WinUI3::Icon::Monitor));
    ui->navigationList->item(2)->setIcon(WinUI3::icon(WinUI3::Icon::Settings));
    ui->navigationList->item(4)->setIcon(WinUI3::icon(WinUI3::Icon::Color));
    ui->navigationList->item(5)->setIcon(WinUI3::icon(WinUI3::Icon::EmojiTabSymbols));
    connect(ui->themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &GalleryWindow::setTheme);
    connect(ui->densityCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index) {
                m_densityIndex = index;
                if (qApp->style())
                    qApp->style()->setProperty("densityMode", index);
            });
    connect(ui->micaToggle, &QCheckBox::toggled, this, [this](bool checked) {
        // The gallery stays a plain Qt client: toggling the "winuiBackdrop"
        // dynamic property is the documented, dependency-free way to ask the
        // style for a Mica surface.
        setProperty("winuiBackdrop", checked ? QStringLiteral("mica") : QStringLiteral("none"));
    });
    connect(ui->searchSettings, &QLineEdit::textChanged, this, [this](const QString &text) {
        for (int row = 0; row < ui->navigationList->count(); ++row) {
            QListWidgetItem *item = ui->navigationList->item(row);
            item->setHidden(!item->text().contains(text, Qt::CaseInsensitive));
        }
    });
    connect(ui->messageDialogButton, &QPushButton::clicked, this, [this] {
        // Issue #51 reproducer: same separate rich-text title and informative
        // text labels used by QSimpleUpdater's QMessageBox prompt.
        QMessageBox dialog(QMessageBox::Information, tr("WinUI 3 Style"), {},
                           QMessageBox::No | QMessageBox::Yes, this);
        dialog.setTextFormat(Qt::RichText);
        dialog.setText(tr("<h3>Version 0.1.0 of WinUI 3 Style has been released!</h3>"));
        dialog.setInformativeText(
                tr("Would you like to download the update now?<br/><br/>"
                   "<strong>Change log:</strong><br/>"
                   "Improved rich-text message layout, content spacing, and footer alignment."));
        dialog.setDefaultButton(QMessageBox::Yes);
        dialog.exec();
    });
    connect(ui->contentDialogButton, &QPushButton::clicked, this, [this] {
        QDialog dialog(this);
        configureContentDialog(&dialog);
        dialog.exec();
    });
    connect(ui->wizardButton, &QPushButton::clicked, this, [this] {
        QWizard wizard(this);
        wizard.setWizardStyle(QWizard::ModernStyle);
        wizard.setWindowTitle(tr("Application setup"));
        auto *welcome = new QWizardPage;
        welcome->setTitle(tr("Welcome"));
        welcome->setSubTitle(tr("Standard QWizard rendered by the style."));
        auto *welcomeLayout = new QVBoxLayout(welcome);
        welcomeLayout->addWidget(
                new QLabel(tr("Content and command areas use separate surfaces."), welcome));
        wizard.addPage(welcome);
        auto *finish = new QWizardPage;
        finish->setTitle(tr("Ready to continue"));
        finish->setSubTitle(tr("Navigation buttons and resize grip."));
        auto *finishLayout = new QVBoxLayout(finish);
        finishLayout->addWidget(new QLabel(tr("Setup complete."), finish));
        wizard.addPage(finish);
        wizard.exec();
    });

    auto *dockWidgetsAction = new QAction(tr("Dock widgets"), this);
    dockWidgetsAction->setObjectName(QStringLiteral("dockWidgetsAction"));
    dockWidgetsAction->setCheckable(true);
    dockWidgetsAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_D));
    ui->viewMenu->addAction(dockWidgetsAction);
    connect(dockWidgetsAction, &QAction::toggled, this, [this](bool on) {
        if (!on) {
            for (QDockWidget *dock : m_demoDocks) {
                removeDockWidget(dock);
                dock->deleteLater();
            }
            m_demoDocks.clear();
            return;
        }
        const QList<QPair<QString, QString>> changedFiles = {
            { tr("src/plugins/help/help_plg.cpp"), tr("Modified") },
            { tr(".qtcreator/"), tr("Untracked") },
            { tr("CMakePresets.json"), tr("Untracked") },
            { tr("CMakeUserPresets.json"), tr("Untracked") },
            { tr("codepointer.json"), tr("Untracked") },
            { tr("conan-create-all.sh"), tr("Untracked") },
            { tr("recipes/"), tr("Untracked") },
            { tr("src/widgets/ResponsiveHBoxLayout.cpp"), tr("Untracked") }
        };
        auto *projectDock = new QDockWidget(tr("Project"), this);
        auto *projectTree = new QTreeWidget(projectDock);
        projectTree->setHeaderLabels({ tr("File"), tr("Status") });
        auto *projectRoot = new QTreeWidgetItem(projectTree, { tr("codepointer") });
        for (const auto &file : changedFiles)
            new QTreeWidgetItem(projectRoot, { file.first, file.second });
        projectRoot->setExpanded(true);
        projectTree->header()->setStretchLastSection(true);
        projectDock->setWidget(projectTree);
        auto *filesDock = new QDockWidget(tr("Files"), this);
        auto *filesTable = new QTableWidget(changedFiles.size(), 3, filesDock);
        filesTable->setHorizontalHeaderLabels({ tr("Commit"), tr("Status"), tr("File") });
        filesTable->verticalHeader()->hide();
        filesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        filesTable->horizontalHeader()->setStretchLastSection(true);
        filesTable->setColumnWidth(0, 64);
        filesTable->setColumnWidth(1, 90);
        for (int row = 0; row < changedFiles.size(); ++row) {
            auto *check = new QTableWidgetItem();
            check->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            check->setCheckState(Qt::Unchecked);
            filesTable->setItem(row, 0, check);
            filesTable->setItem(row, 1, new QTableWidgetItem(changedFiles.at(row).second));
            filesTable->setItem(row, 2, new QTableWidgetItem(changedFiles.at(row).first));
        }
        filesDock->setWidget(filesTable);
        auto *commitDock = new QDockWidget(tr("Commit message"), this);
        commitDock->setWidget(new QPlainTextEdit(commitDock));
        auto *outputDock = new QDockWidget(tr("Output"), this);
        auto *outputPane = new QPlainTextEdit(outputDock);
        outputPane->setReadOnly(true);
        outputPane->setPlainText(tr("LspClientImpl: connected to clangd\n"
                                    "CodeAction failed: trying to get AST for non-added document\n"
                                    "CodeAction failed: trying to get AST for non-added document\n"
                                    "CodeAction failed: trying to get AST for non-added document"));
        outputDock->setWidget(outputPane);
        for (QDockWidget *dock : { projectDock, filesDock, commitDock, outputDock }) {
            dock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable
                              | QDockWidget::DockWidgetFloatable);
            dock->setAllowedAreas(Qt::AllDockWidgetAreas);
            m_demoDocks.append(dock);
        }
        addDockWidget(Qt::LeftDockWidgetArea, projectDock);
        addDockWidget(Qt::RightDockWidgetArea, filesDock);
        tabifyDockWidget(filesDock, commitDock);
        addDockWidget(Qt::BottomDockWidgetArea, outputDock);
        resizeDocks({ projectDock }, { 260 }, Qt::Horizontal);
        resizeDocks({ filesDock }, { 420 }, Qt::Horizontal);
        resizeDocks({ outputDock }, { 140 }, Qt::Vertical);
    });
}

void GalleryWindow::setComparisonStyle(const QString &key)
{
    QStyle *comparisonStyle = QStyleFactory::create(key);
    if (!comparisonStyle) {
        ui->statusBar->showMessage(tr("The selected comparison style is unavailable."));
        if (QAction *winuiAction = findChild<QAction *>(QStringLiteral("winui3StyleAction")))
            winuiAction->setChecked(true);
        if (QAction *styleToggle = findChild<QAction *>(QStringLiteral("comparisonToggleAction")))
            styleToggle->setChecked(true);
        return;
    }
    comparisonStyle->setProperty("themeMode", m_themeIndex);
    comparisonStyle->setProperty("densityMode", m_densityIndex);
    qApp->setStyle(comparisonStyle);

    const bool winuiStyle = key == QLatin1String("winui3");
    if (QAction *action = findChild<QAction *>(winuiStyle ? QStringLiteral("winui3StyleAction")
                                                          : QStringLiteral("windows11StyleAction")))
        action->setChecked(true);
    // Keep the toolbar switch in sync when the View > Style menu is used.
    // setChecked() emits toggled(), not triggered(), so this cannot recurse.
    if (QAction *styleToggle = findChild<QAction *>(QStringLiteral("comparisonToggleAction")))
        styleToggle->setChecked(winuiStyle);
    ui->themeCombo->setEnabled(winuiStyle);
    ui->densityCombo->setEnabled(winuiStyle);
    ui->micaToggle->setEnabled(winuiStyle);
    ui->statusBar->showMessage(winuiStyle ? tr("WinUI 3 style")
                                          : tr("Comparison style: %1").arg(key));
}

void GalleryWindow::populateCollections()
{
    const QIcon folder = style()->standardIcon(QStyle::SP_DirIcon, nullptr, this);
    const QIcon file = style()->standardIcon(QStyle::SP_FileIcon, nullptr, this);
    for (const QString &text : { tr("Documents"), tr("Pictures"), tr("Downloads") })
        new QListWidgetItem(folder, text, ui->listViewTab);
    new QListWidgetItem(file, tr("Readme.txt"), ui->listViewTab);
    for (QListWidget *nav : { ui->standardDensityNav, ui->compactDensityNav }) {
        for (const QString &text : { tr("Home"), tr("Collections") })
            new QListWidgetItem(text, nav);
    }
    for (const QString &text : { tr("Documents"), tr("Pictures"), tr("Downloads") })
        new QListWidgetItem(folder, text, ui->compactListView);
    new QListWidgetItem(file, tr("Readme.txt"), ui->compactListView);
    auto *compactRoot = new QTreeWidgetItem(ui->compactTreeView, { tr("Example album") });
    compactRoot->setIcon(0, folder);
    for (const QString &text : { tr("01: First track"), tr("02: Second track") })
        new QTreeWidgetItem(compactRoot, { text });
    compactRoot->setExpanded(true);
    for (const QString &text : { tr("Disabled document"), tr("Disabled folder") })
        new QListWidgetItem(text, ui->disabledListView);
    auto *rtlRoot = new QTreeWidgetItem(ui->rtlTreeView, { tr("RTL collection") });
    new QTreeWidgetItem(rtlRoot, { tr("First child") });
    new QTreeWidgetItem(rtlRoot, { tr("Second child") });
    rtlRoot->setExpanded(true);
    auto *root = new QTreeWidgetItem(ui->treeViewTab, { tr("Example album: Selected tracks") });
    root->setIcon(0, folder);
    root->setFlags(root->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsAutoTristate);
    root->setCheckState(0, Qt::PartiallyChecked);
    const QStringList tracks{ tr("01: First track"), tr("02: Second track"), tr("03: Third track"),
                              tr("04: Fourth track") };
    for (int index = 0; index < tracks.size(); ++index) {
        auto *track = new QTreeWidgetItem(root, { tracks.at(index) });
        track->setFlags(track->flags() | Qt::ItemIsUserCheckable);
        track->setCheckState(0, index == 2 ? Qt::Unchecked : Qt::Checked);
    }
    root->setExpanded(true);
    // Populate the Designer-owned plain TreeView so its hierarchy and
    // disclosure affordance can be inspected independently of checkboxes.
    auto *plainRoot =
            new QTreeWidgetItem(ui->plainTreeViewTab, { tr("Example album"), tr("Expanded") });
    plainRoot->setIcon(0, folder);
    for (const QString &track : tracks)
        new QTreeWidgetItem(plainRoot, { track, tr("Ready") });
    plainRoot->setExpanded(true);
    ui->plainTreeViewTab->setCurrentItem(plainRoot);
    ui->plainTreeViewTab->setColumnWidth(0, 360);
    ui->plainTreeViewTab->setSortingEnabled(true);
    ui->plainTreeViewTab->header()->setSortIndicator(0, Qt::AscendingOrder);

    const QString rows[4][3] = { { tr("Button"), tr("Ready"), tr("Complete") },
                                 { tr("ComboBox"), tr("Interactive"), tr("Complete") },
                                 { tr("TreeView"), tr("Expanded"), tr("Complete") },
                                 { tr("TableView"), tr("Editable"), tr("Complete") } };
    for (int row = 0; row < 4; ++row)
        for (int column = 0; column < 3; ++column)
            ui->tableViewTab->setItem(row, column, new QTableWidgetItem(rows[row][column]));
    ui->tableViewTab->setAlternatingRowColors(true);
    ui->tableViewTab->horizontalHeader()->setStretchLastSection(true);
    // Keep one live sort on the Data grid so the WinUI DataGrid sort glyph
    // (SortIconAscending E74A / Descending E74B in the trailing 32 px column)
    // is a real, captured part of the Gallery surface; clicking any header
    // flips the indicator through the stock QTableWidget behaviour.
    ui->tableViewTab->setSortingEnabled(true);
    ui->tableViewTab->horizontalHeader()->setSortIndicator(0, Qt::AscendingOrder);

    // Live todo-list reproducer: a checkable Task column, a Status column,
    // and a left-elided File path. The header modes are the same ones the
    // CommitForm defect exposed (0/1 ResizeToContents, 2 Stretch). These modes
    // exercise both sizing contracts: the cell's 12 px inset must be part of
    // the item view hint, and CT_HeaderSection must measure the title itself.
    // Keep them so this page remains a regression fixture.
    auto *tableViewModel = new QStandardItemModel(ui->tableView);
    tableViewModel->setHorizontalHeaderLabels({ tr("Task"), tr("Status"), tr("File") });
    struct TodoRow
    {
        const char *task;
        const char *status;
        const char *file;
        bool checked;
    };
    const TodoRow todoRows[] = {
        TodoRow{ "Write release notes", "Done", "docs/release-notes.md", true },
        TodoRow{ "Review header sizing", "In progress", "src/lib/winui3style_contracts_p.cpp",
                 false },
        TodoRow{ "Update coverage", "To do", "docs/coverage.md", false },
        TodoRow{ "Run gallery capture", "To do", "tests/tst_winui3tableheader.cpp", false },
        TodoRow{ "Fix clipped table text", "To do", "docs/evidence/datagrid-header.png", false },
    };
    for (const TodoRow &row : todoRows) {
        auto *task = new QStandardItem(tr(row.task));
        task->setCheckable(true);
        task->setCheckState(row.checked ? Qt::Checked : Qt::Unchecked);
        task->setEditable(false);
        auto *status = new QStandardItem(tr(row.status));
        status->setEditable(false);
        auto *fileItem = new QStandardItem(tr(row.file));
        fileItem->setEditable(false);
        tableViewModel->appendRow({ task, status, fileItem });
    }
    ui->tableView->setModel(tableViewModel);
    ui->tableView->setItemDelegateForColumn(2, new PathElideDelegate(ui->tableView));
    auto *tableHeader = ui->tableView->horizontalHeader();
    tableHeader->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    tableHeader->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    tableHeader->setSectionResizeMode(2, QHeaderView::Stretch);
    // Row heights stay non-interactive; only the column modes above matter.
    ui->tableView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
}

namespace {
// The Icons page shows one cell per QStyle::StandardPixmap at 24 logical
// pixels. QIcon::pixmap(size, devicePixelRatio) hands back whatever raster
// the icon engine offers at that logical size: Qt 6.12 scales base-style
// icons up to the request, while Qt 6.8 returns the engine's own native
// raster instead, and Qt 5.12 has no devicePixelRatio overload at all.
// Re-rasterizing into an explicitly sized grid keeps every preview cell on
// the same pixel contract across those behaviours, which is what the
// galleryprobe contract asserts.
constexpr int kIconPreviewPixelSize = 24;

QPixmap standardIconPixmap(QStyle *style, QStyle::StandardPixmap standard, const QWidget *widget,
                           qreal devicePixelRatio)
{
    const QIcon glyph = style->standardIcon(standard, nullptr, widget);
    if (glyph.isNull())
        return {};
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // Qt 5 has no QIcon::pixmap(QSize, qreal) overload on the oldest supported
    // release (5.12): request the logical size and attach the ratio here; the
    // normalization below keeps the preview grid identical to Qt 6's.
    QPixmap pixmap = glyph.pixmap(QSize(kIconPreviewPixelSize, kIconPreviewPixelSize));
    if (pixmap.isNull())
        return {};
    pixmap.setDevicePixelRatio(devicePixelRatio);
#else
    QPixmap pixmap =
            glyph.pixmap(QSize(kIconPreviewPixelSize, kIconPreviewPixelSize), devicePixelRatio);
    if (pixmap.isNull())
        return {};
#endif
    const int deviceSide = qRound(kIconPreviewPixelSize * devicePixelRatio);
    const QSize targetSize(deviceSide, deviceSide);
    if (pixmap.size() == targetSize && pixmap.devicePixelRatioF() == devicePixelRatio)
        return pixmap;
    // Re-rasterize the engine's native pixels onto the preview grid: the
    // device pixels are scaled into the box and the ratio rides along, so
    // pixmap().size() equals 24 * devicePixelRatio for every engine answer.
    QPixmap result(targetSize);
    result.setDevicePixelRatio(devicePixelRatio);
    result.fill(Qt::transparent);
    QPainter painter(&result);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.drawPixmap(result.rect(), pixmap, QRectF(pixmap.rect()));
    return result;
}
} // namespace

void GalleryWindow::populateIconSheet()
{
    const auto clearGrid = [](QGridLayout *grid) {
        while (QLayoutItem *item = grid->takeAt(0)) {
            delete item->widget();
            delete item;
        }
    };
    clearGrid(ui->fluentIconsGrid);
    clearGrid(ui->baseIconsGrid);
    clearGrid(ui->missingIconsGrid);
    m_iconPreviews.clear();

    // One side-by-side reference style for the whole sheet. On Windows this is
    // Qt's official Windows 11 plugin; elsewhere Fusion is Qt's own default
    // style and gives the same plain-Qt comparison. The gallery stays a plain
    // Qt client and never links the style library.
    if (!m_iconReferenceStyle) {
        m_iconReferenceStyle = QStyleFactory::create(QStringLiteral("windows11"));
        m_iconReferenceStyleName = QStringLiteral("Windows 11");
        if (!m_iconReferenceStyle) {
            m_iconReferenceStyle = QStyleFactory::create(QStringLiteral("Fusion"));
            m_iconReferenceStyleName = QStringLiteral("Fusion");
        }
        if (m_iconReferenceStyle)
            m_iconReferenceStyle->setParent(this);
    }

    // QStyle tags the icons it answers itself with a reserved engine name; an
    // icon the style does not map keeps an empty name and comes from the base
    // style, which has no artwork for some ancestor-dependent pixmaps. Reading
    // the tag keeps this page a plain Qt client: the gallery never links the
    // style library.
    const QString fluentPrefix = QStringLiteral("winui3-fluent-icon:");
    const QMetaEnum standardPixmaps = QMetaEnum::fromType<QStyle::StandardPixmap>();
    const int columns = 6;
    const int captionWidth = 132;
    int fluentCount = 0;
    int baseCount = 0;
    int missingCount = 0;
    for (int index = 0; index < standardPixmaps.keyCount(); ++index) {
        const auto standard = static_cast<QStyle::StandardPixmap>(standardPixmaps.value(index));
        const QString name = QString::fromLatin1(standardPixmaps.key(index));
        const QIcon glyph = style()->standardIcon(standard, nullptr, this);
        const bool fluent = glyph.name().startsWith(fluentPrefix);
        const bool missing =
                standardIconPixmap(style(), standard, this, devicePixelRatioF()).isNull();
        QGridLayout *grid = ui->baseIconsGrid;
        if (fluent)
            grid = ui->fluentIconsGrid;
        else if (missing)
            grid = ui->missingIconsGrid;
        const int position = fluent ? fluentCount++ : (missing ? missingCount++ : baseCount++);

        auto *cell = new QWidget;
        auto *cellLayout = new QVBoxLayout(cell);
        cellLayout->setContentsMargins(4, 4, 4, 4);
        cellLayout->setSpacing(4);
        auto *preview = new QLabel(cell);
        preview->setAlignment(Qt::AlignCenter);
        preview->setFixedHeight(28);
        // Property, not pixmap state: a pixmap the base style cannot provide
        // leaves the cell showing a placeholder, and the probe still counts it.
        preview->setProperty("standardPixmapValue", standardPixmaps.value(index));
        preview->setToolTip(fluent ? tr("Answered by the style with %1").arg(glyph.name())
                                    : missing ? tr("No glyph: neither the style nor its base "
                                                   "style provides this pixmap.")
                                              : tr("Not mapped by the style; drawn by the base "
                                                   "style."));
        if (missing)
            preview->setText(QStringLiteral("\u2205"));

        // Windows 11 reference on the left, active style on the right.
        auto *compareRow = new QHBoxLayout;
        compareRow->setContentsMargins(0, 0, 0, 0);
        compareRow->setSpacing(3);
        QLabel *referenceLabel = nullptr;
        if (m_iconReferenceStyle) {
            referenceLabel = new QLabel(cell);
            referenceLabel->setAlignment(Qt::AlignCenter);
            referenceLabel->setFixedHeight(28);
            referenceLabel->setProperty("standardPixmapReference", standardPixmaps.value(index));
            referenceLabel->setToolTip(tr("Qt %1 style").arg(m_iconReferenceStyleName));
            compareRow->addWidget(referenceLabel);
            auto *arrow = new QLabel(QStringLiteral("\u2192"), cell);
            arrow->setToolTip(tr("Qt %1 \u2192 active style").arg(m_iconReferenceStyleName));
            compareRow->addWidget(arrow);
        }
        compareRow->addWidget(preview);
        cellLayout->addLayout(compareRow);

        auto *caption = new QLabel(name, cell);
        caption->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
        caption->setWordWrap(true);
        caption->setFixedWidth(captionWidth);
        QFont captionFont = caption->font();
        captionFont.setPixelSize(11);
        caption->setFont(captionFont);
        cellLayout->addWidget(caption);
        grid->addWidget(cell, position / columns, position % columns);
        m_iconPreviews.append({ preview, referenceLabel, standardPixmaps.value(index) });
    }
    for (int column = 0; column < columns; ++column) {
        ui->fluentIconsGrid->setColumnStretch(column, 1);
        ui->baseIconsGrid->setColumnStretch(column, 1);
        ui->missingIconsGrid->setColumnStretch(column, 1);
    }

    ui->fluentIconsGroup->setTitle(tr("Fluent glyphs (%1)").arg(fluentCount));
    ui->baseIconsGroup->setTitle(tr("Base style fallback (%1)").arg(baseCount));
    ui->missingIconsGroup->setTitle(tr("No glyph (%1)").arg(missingCount));
    ui->iconsNote->setText(
            tr("%1 of the %2 QStyle::StandardPixmap values are answered with one of the "
               "style's own Fluent glyphs, %3 come from the base style, and %4 have no glyph at "
               "all. Each cell places Qt's %5 glyph on the left and the active "
               "style's glyph on the right. Captions show the QStyle enumerator.")
                    .arg(fluentCount)
                    .arg(fluentCount + baseCount + missingCount)
                    .arg(baseCount)
                    .arg(missingCount)
                    .arg(m_iconReferenceStyleName));
    m_iconSheetStyle = style();
    refreshIconSheet();
}

void GalleryWindow::refreshIconSheet()
{
    for (const IconPreview &preview : m_iconPreviews) {
        if (!preview.label)
            continue;
        if (preview.referenceLabel) {
            const QPixmap reference =
                    standardIconPixmap(m_iconReferenceStyle,
                                       static_cast<QStyle::StandardPixmap>(preview.standardPixmap),
                                       this, devicePixelRatioF());
            if (reference.isNull()) {
                preview.referenceLabel->setPixmap(QPixmap());
                preview.referenceLabel->setText(QStringLiteral("\u2205"));
            } else {
                preview.referenceLabel->setText(QString());
                preview.referenceLabel->setPixmap(reference);
            }
        }
        const QPixmap pixmap = standardIconPixmap(
                style(), static_cast<QStyle::StandardPixmap>(preview.standardPixmap), this,
                devicePixelRatioF());
        if (pixmap.isNull()) {
            // Neither the style nor its base style has artwork for this
            // ancestor-dependent pixmap: an empty cell would read as a broken
            // label, so the sheet shows an explicit no-glyph mark instead.
            preview.label->setPixmap(QPixmap());
            preview.label->setText(QStringLiteral("\u2205"));
            continue;
        }
        // Clear any stale placeholder before attaching the raster so a cell
        // that gained artwork does not keep the no-glyph mark around.
        preview.label->setText(QString());
        preview.label->setPixmap(pixmap);
    }
}

void GalleryWindow::configurePaletteLab(bool captureMode)
{
    // Keep the palette content anchored at the top when the page is taller than
    // its controls, matching the other vertically stacked gallery pages.
    ui->paletteLayout->addStretch(1);
    struct RoleRow
    {
        QPalette::ColorRole role;
        const char *label;
    };
    static const RoleRow roles[] = { { QPalette::Window, QT_TR_NOOP("Window (paper)") },
                                     { QPalette::WindowText, QT_TR_NOOP("WindowText/Text (ink)") },
                                     { QPalette::Base, QT_TR_NOOP("Base (layer)") },
                                     { QPalette::Button, QT_TR_NOOP("Button (control fill)") },
                                     { QPalette::Highlight, QT_TR_NOOP("Highlight (selection)") } };
    struct PaletteState
    {
        QPalette working;
        QList<QLabel *> swatches;
    };
    QPalette capturePalette = ui->palettePreview->palette();
    if (captureMode) {
        // The swatches are sampled at Gallery construction. Use an isolated
        // factory-created style for the approved fixture; this instance is
        // never installed as QApplication's active style.
        const std::unique_ptr<QStyle> captureStyle(QStyleFactory::create(QStringLiteral("winui3")));
        if (captureStyle) {
            captureStyle->setProperty("themeMode", 2);
            captureStyle->setProperty("accentColor", QColor(0, 120, 212));
            capturePalette = captureStyle->standardPalette();
        }
    }
    // Shared ownership: every capture below holds its own reference, so the
    // state outlives the last pick button / refresh closure. palettePreview
    // destruction needs no explicit cleanup (no raw new / destroyed->delete).
    auto state = std::make_shared<PaletteState>(PaletteState{ capturePalette, {} });
    const auto refresh = [state] {
        for (qsizetype i = 0; i < state->swatches.size(); ++i) {
            QPixmap pixmap(48, 20);
            pixmap.fill(state->working.color(roles[i].role));
            state->swatches[i]->setPixmap(pixmap);
        }
    };
    for (int i = 0; i < int(std::size(roles)); ++i) {
        auto *label = new QLabel(tr(roles[i].label));
        auto *swatch = new QLabel;
        swatch->setFixedSize(48, 20);
        state->swatches.append(swatch);
        auto *pick = new QPushButton(tr("Edit..."));
        connect(pick, &QPushButton::clicked, this, [this, state, role = roles[i].role, refresh] {
            const QColor color =
                    QColorDialog::getColor(state->working.color(role), this, tr("Choose color"),
                                           QColorDialog::ShowAlphaChannel);
            if (!color.isValid())
                return;
            for (QPalette::ColorGroup group : { QPalette::Active, QPalette::Inactive }) {
                state->working.setColor(group, role, color);
                if (role == QPalette::WindowText) {
                    state->working.setColor(group, QPalette::Text, color);
                    state->working.setColor(group, QPalette::ButtonText, color);
                }
            }
            ui->palettePreview->setPalette(state->working);
            refresh();
        });
        const int row = i / 3;
        const int column = (i % 3) * 3;
        ui->paletteRolesLayout->addWidget(label, row, column);
        ui->paletteRolesLayout->addWidget(swatch, row, column + 1);
        ui->paletteRolesLayout->addWidget(pick, row, column + 2);
    }
    auto *accent = new QPushButton(tr("Edit application accent..."));
    connect(accent, &QPushButton::clicked, this, [this] {
        const QColor current = qApp->style()->property("accentColor").value<QColor>();
        const QColor color = QColorDialog::getColor(current, this, tr("Choose accent"));
        if (color.isValid())
            qApp->style()->setProperty("accentColor", color);
    });
    ui->paletteRolesLayout->addWidget(accent, 2, 6, 1, 2);
    refresh();
}

void GalleryWindow::setTheme(int index)
{
    // Theme selection must not also change the composition pipeline. System
    // and an equivalent explicit Light/Dark mode use the same palette and
    // therefore must render identically. Applications can opt into Mica
    // independently through the winuiBackdrop top-level property.
    m_themeIndex = index;
    qApp->style()->setProperty("themeMode", index);
}

bool GalleryWindow::saveSnapshots(const QString &directory, const QString &backdrop)
{
    QDir output;
    if (!output.mkpath(directory))
        return false;
    output.setPath(directory);
    QStyle *activeStyle = qApp->style();
    if (!activeStyle->property("themeMode").isValid())
        return false;
    const QVariant previousTheme = activeStyle->property("themeMode");
    const QVariant previousDensity = activeStyle->property("densityMode");
    const QVariant previousAccent = activeStyle->property("accentColor");
    const QVariant previousBackdrop = property("winuiBackdrop");
    const QPalette previousPalette = palette();
    const bool previousAutoFill = autoFillBackground();
    const QLocale previousLocale = locale();
    const bool previousExplicitLocale = testAttribute(Qt::WA_SetLocale);
    const QString previousStandardDateFormat = ui->standardDensityDate->displayFormat();
    const QString previousCompactDateFormat = ui->compactDensityDate->displayFormat();
    const QDate previousCalendarDate = ui->persistentCalendar->selectedDate();
    const int previousCalendarYear = ui->persistentCalendar->yearShown();
    const int previousCalendarMonth = ui->persistentCalendar->monthShown();
    const bool animationsWereDisabled = qEnvironmentVariableIsSet("WINUI3STYLE_DISABLE_ANIMATIONS");
    const QByteArray previousAnimationSetting = qgetenv("WINUI3STYLE_DISABLE_ANIMATIONS");
    qputenv("WINUI3STYLE_DISABLE_ANIMATIONS", "1");
    setProperty("winuiBackdrop", backdrop);
    if (backdrop == QLatin1String("none")) {
        setPalette(QPalette());
        setAutoFillBackground(true);
    }
    activeStyle->setProperty("accentColor", QColor(0, 120, 212));
    activeStyle->setProperty("densityMode", 0);
    const bool mouseEventsWereTransparent = testAttribute(Qt::WA_TransparentForMouseEvents);
    const QPoint previousCursorPosition = QCursor::pos();
    const auto restore = qScopeGuard([&] {
        activeStyle->setProperty("themeMode", previousTheme);
        activeStyle->setProperty("densityMode", previousDensity);
        activeStyle->setProperty("accentColor", previousAccent);
        setProperty("winuiBackdrop", previousBackdrop);
        if (animationsWereDisabled)
            qputenv("WINUI3STYLE_DISABLE_ANIMATIONS", previousAnimationSetting);
        else
            qunsetenv("WINUI3STYLE_DISABLE_ANIMATIONS");
        setPalette(previousPalette);
        setAutoFillBackground(previousAutoFill);
        if (previousExplicitLocale)
            setLocale(previousLocale);
        else
            unsetLocale();
        ui->standardDensityDate->setDisplayFormat(previousStandardDateFormat);
        ui->compactDensityDate->setDisplayFormat(previousCompactDateFormat);
        setAttribute(Qt::WA_TransparentForMouseEvents, mouseEventsWereTransparent);
        ui->persistentCalendar->setSelectedDate(previousCalendarDate);
        ui->persistentCalendar->setCurrentPage(previousCalendarYear, previousCalendarMonth);
        QCursor::setPos(previousCursorPosition);
    });
    // The reviewed fixtures use French date/month formatting, independently
    // of the Windows account locale used by a hosted runner.
    setLocale(QLocale(QLocale::French, QLocale::France));
    ui->standardDensityDate->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    ui->compactDensityDate->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    // Preserve the reviewed calendar fixture across midnight/day changes.
    // This is capture state only; the live Gallery retains its own date/page.
    const QDate calendarFixture(2026, 10, 11);
    ui->persistentCalendar->setSelectedDate(calendarFixture);
    ui->persistentCalendar->setCurrentPage(calendarFixture.year(), calendarFixture.month());
    if (QScreen *screen = QGuiApplication::primaryScreen())
        QCursor::setPos(screen->availableGeometry().bottomRight());
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    const auto settle = [this] {
        if (QWidget *focused = qApp->focusWidget())
            focused->clearFocus();
        for (QWidget *widget : findChildren<QWidget *>()) {
            QEvent leave(QEvent::Leave);
            QCoreApplication::sendEvent(widget, &leave);
        }
        qApp->processEvents();
        // A page change can enqueue another layout/polish pass. Capturing the
        // first exposed frame made the first light-theme page transition look
        // shifted even though the settled widget was correct. Give Qt one
        // compositor frame, then force the final backing-store repaint.
        QEventLoop frame;
        QTimer::singleShot(20, &frame, &QEventLoop::quit);
        frame.exec(QEventLoop::ExcludeUserInputEvents);
        repaint();
        for (QWidget *widget : findChildren<QWidget *>()) {
            if (widget->isVisible()) {
                if (widget->layout())
                    widget->layout()->activate();
                widget->repaint();
            }
        }
        qApp->processEvents();
    };
    bool success = true;
    for (int mode : { 1, 2 }) {
        activeStyle->setProperty("themeMode", mode);
        const QString theme = mode == 1 ? QStringLiteral("light") : QStringLiteral("dark");
        ui->fileMenu->popup(ui->menuBar->mapToGlobal(
                QPoint(ui->menuBar->actionGeometry(ui->menuBar->actions().first()).left(),
                       ui->menuBar->height())));
        qApp->processEvents();
        success = ui->fileMenu->grab().save(output.filePath(theme + "-menu.png"), "PNG") && success;
        if (QAction *firstAction = ui->fileMenu->actions().value(0)) {
            ui->fileMenu->setActiveAction(firstAction);
            qApp->processEvents();
            success = ui->fileMenu->grab().save(output.filePath(theme + "-state-menu-hover.png"),
                                                "PNG")
                    && success;
            const QPoint local = ui->fileMenu->actionGeometry(firstAction).center();
            QMouseEvent press(QEvent::MouseButtonPress, QPointF(local),
                              QPointF(ui->fileMenu->mapToGlobal(local)), Qt::LeftButton,
                              Qt::LeftButton, Qt::NoModifier);
            QCoreApplication::sendEvent(ui->fileMenu, &press);
            qApp->processEvents();
            success = ui->fileMenu->grab().save(output.filePath(theme + "-state-menu-pressed.png"),
                                                "PNG")
                    && success;
        }
        ui->fileMenu->hide();
        for (int page = 0; page < ui->pages->count(); ++page) {
            ui->pages->setCurrentIndex(page);
            qApp->processEvents();
            settle();
            // A page never visited in-capture before can still be mid-layout
            // or mid-polish after one pass (palette lab: missing headings,
            // shifted content, light-painted editors in dark mode). A second
            // settle round lets nested layouts converge and queued refreshes
            // fire before the grab.
            settle();
            success = grab().save(output.filePath(
                                          QStringLiteral("%1-page-%2.png").arg(theme).arg(page)),
                                  "PNG")
                    && success;
            auto *area = qobject_cast<QScrollArea *>(ui->pages->widget(page));
            if (area && area->verticalScrollBar()->maximum() > 0) {
                area->verticalScrollBar()->setValue(area->verticalScrollBar()->maximum());
                qApp->processEvents();
                success = grab().save(output.filePath(QStringLiteral("%1-page-%2-scrolled.png")
                                                              .arg(theme)
                                                              .arg(page)),
                                      "PNG")
                        && success;
                area->verticalScrollBar()->setValue(0);
            }
            if (area && area->widget()) {
                if (auto *tabs = area->widget()->findChild<QTabWidget *>()) {
                    const int oldTab = tabs->currentIndex();
                    for (int tab = 0; tab < tabs->count(); ++tab) {
                        if (!tabs->isTabEnabled(tab))
                            continue;
                        tabs->setCurrentIndex(tab);
                        qApp->processEvents();
                        settle();
                        success =
                                grab().save(output.filePath(QStringLiteral("%1-page-%2-tab-%3.png")
                                                                    .arg(theme)
                                                                    .arg(page)
                                                                    .arg(tab)),
                                            "PNG")
                                && success;
                    }
                    tabs->setCurrentIndex(oldTab);
                }
            }
        }
        ui->pages->setCurrentIndex(0);
        qApp->processEvents();
        ui->galleryComboBox->showPopup();
        qApp->processEvents();
        if (QWidget *popup = ui->galleryComboBox->view()->window())
            success = popup->grab().save(output.filePath(theme + "-combo-popup.png"), "PNG")
                    && success;
        ui->galleryComboBox->hidePopup();
        const auto saveControl = [&](QWidget *control, const QString &state) {
            qApp->processEvents();
            return control
                    && control->grab().save(output.filePath(theme + "-state-" + state + ".png"),
                                            "PNG");
        };
        const auto sendPointerState = [](QWidget *control, bool hovered, bool pressed) {
            if (!control)
                return;
            QEvent boundary(hovered ? QEvent::Enter : QEvent::Leave);
            QCoreApplication::sendEvent(control, &boundary);
            const QPoint local = control->rect().center();
            QMouseEvent mouse(pressed ? QEvent::MouseButtonPress : QEvent::MouseButtonRelease,
                              QPointF(local), QPointF(control->mapToGlobal(local)), Qt::LeftButton,
                              pressed ? Qt::LeftButton : Qt::NoButton, Qt::NoModifier);
            QCoreApplication::sendEvent(control, &mouse);
            qApp->processEvents();
        };
        success = saveControl(ui->galleryStandardButton, "button-rest") && success;
        sendPointerState(ui->galleryStandardButton, true, false);
        success = saveControl(ui->galleryStandardButton, "button-hover") && success;
        sendPointerState(ui->galleryStandardButton, true, true);
        success = saveControl(ui->galleryStandardButton, "button-pressed") && success;
        sendPointerState(ui->galleryStandardButton, false, false);
        // No `textbox-keyboard-focus` state capture: the text caret is one real
        // column of that raster and its ink is not stable across the Qt matrix
        // (6.8 paints the bare palette text colour, 6.12 composites its alpha),
        // and the matrix compares pixel-exact with no mask. The keyboard ring is
        // covered by `textBoxStateMatrix` instead.
        sendPointerState(ui->galleryComboBox, true, true);
        success = saveControl(ui->galleryComboBox, "combobox-pressed") && success;
        sendPointerState(ui->galleryComboBox, false, false);
        ui->galleryComboBox->hidePopup();
        ui->galleryCheckBox->setChecked(true);
        success = saveControl(ui->galleryCheckBox, "checkbox-checked") && success;
        ui->galleryCheckBox->setChecked(false);
        success = saveControl(ui->galleryCheckBox, "checkbox-unchecked") && success;
        ui->galleryRadioButton->setChecked(true);
        success = saveControl(ui->galleryRadioButton, "radio-checked") && success;
        sendPointerState(ui->gallerySlider, true, false);
        success = saveControl(ui->gallerySlider, "slider-hover") && success;
        sendPointerState(ui->gallerySlider, false, false);
        success = !ui->galleryRadioButtonDisabled->isEnabled() && success;
        success = !ui->galleryToggleSwitchDisabled->isEnabled()
                && ui->galleryToggleSwitchDisabled->property("winuiToggleSwitch").toBool()
                && success;
        QDialog contentDialog(this);
        configureContentDialog(&contentDialog);
        contentDialog.show();
        qApp->processEvents();
        success = contentDialog.grab().save(output.filePath(theme + "-content-dialog.png"), "PNG")
                && success;
        contentDialog.close();

        QMessageBox messageBox(QMessageBox::Information, tr("WinUI 3 Style"),
                               tr("Native Qt message box rendered by the style."), QMessageBox::Ok,
                               this);
        messageBox.show();
        qApp->processEvents();
        success = messageBox.grab().save(output.filePath(theme + "-message-box.png"), "PNG")
                && success;
        messageBox.close();
        QWizard wizardSnap;
        wizardSnap.setWizardStyle(QWizard::ModernStyle);
        wizardSnap.setWindowTitle(tr("Application setup"));
        auto *wizardWelcome = new QWizardPage;
        wizardWelcome->setTitle(tr("Welcome"));
        wizardWelcome->setSubTitle(tr("Standard QWizard rendered by the style."));
        auto *wizardWelcomeLayout = new QVBoxLayout(wizardWelcome);
        wizardWelcomeLayout->addWidget(
                new QLabel(tr("Content and command areas use separate surfaces."), wizardWelcome));
        wizardSnap.addPage(wizardWelcome);
        wizardSnap.resize(520, 340);
        wizardSnap.show();
        for (int spin = 0;
             spin < 50 && (!wizardSnap.windowHandle() || !wizardSnap.windowHandle()->isExposed());
             ++spin) {
            qApp->processEvents();
            QEventLoop pause;
            QTimer::singleShot(20, &pause, &QEventLoop::quit);
            pause.exec(QEventLoop::ExcludeUserInputEvents);
        }
        // Showing can deliver a pre-layout first frame; resize again once
        // exposed so the header/page/footer settle before the grab.
        wizardSnap.resize(521, 341);
        wizardSnap.resize(520, 340);
        if (QWidget *wizardFooter = wizardSnap.findChild<QWidget *>(
                    QStringLiteral("_winui_wizard_footer_surface"))) {
            for (int spin = 0; spin < 50 && !wizardFooter->isVisible(); ++spin) {
                qApp->processEvents();
                QEventLoop pause;
                QTimer::singleShot(20, &pause, &QEventLoop::quit);
                pause.exec(QEventLoop::ExcludeUserInputEvents);
            }
        }
        if (QLayout *wizardLayout = wizardSnap.layout())
            wizardLayout->activate();
        qApp->processEvents();
        wizardSnap.repaint();
        qApp->processEvents();
        success = wizardSnap.grab().save(output.filePath(theme + "-wizard.png"), "PNG") && success;
        wizardSnap.close();

        // Keep all historical Standard captures byte-comparable, then add a
        // small Compact oracle for the controls covered by WinUI's Compact
        // Sizing resource rather than blindly doubling the whole matrix.
        activeStyle->setProperty("densityMode", 1);
        ui->densityCombo->setCurrentIndex(1);
        for (int page = 0; page < ui->pages->count(); ++page) {
            ui->pages->setCurrentIndex(page);
            qApp->processEvents();
            settle();
            // A page never visited in-capture before can still be mid-layout
            // or mid-polish after one pass (palette lab: missing headings,
            // shifted content, light-painted editors in dark mode). A second
            // settle round lets nested layouts converge and queued refreshes
            // fire before the grab.
            settle();
            success = grab().save(output.filePath(QStringLiteral("%1-density-compact-page-%2.png")
                                                          .arg(theme)
                                                          .arg(page)),
                                  "PNG")
                    && success;
        }
        ui->pages->setCurrentIndex(1);
        const int previousCollectionTab = ui->collectionsTabs->currentIndex();
        ui->collectionsTabs->setCurrentWidget(ui->plainTreeViewTab);
        settle();
        success = ui->plainTreeViewTab->grab().save(
                          output.filePath(theme + "-density-compact-tree.png"), "PNG")
                && success;
        ui->collectionsTabs->setCurrentWidget(ui->tableViewTab);
        settle();
        success = ui->tableViewTab->grab().save(
                          output.filePath(theme + "-density-compact-table.png"), "PNG")
                && success;
        ui->collectionsTabs->setCurrentIndex(previousCollectionTab);
        ui->pages->setCurrentIndex(0);
        ui->galleryComboBox->showPopup();
        qApp->processEvents();
        if (QWidget *popup = ui->galleryComboBox->view()->window()) {
            success = popup->grab().save(
                              output.filePath(theme + "-density-compact-combo-popup.png"), "PNG")
                    && success;
        }
        ui->galleryComboBox->hidePopup();
        ui->densityCombo->setCurrentIndex(0);
        activeStyle->setProperty("densityMode", 0);
    }
    return success;
}
