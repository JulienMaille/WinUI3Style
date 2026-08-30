#include "gallerywindow.h"
#include "ui_gallerywindow.h"

#include <QApplication>
#include <QColorDialog>
#include <QCompleter>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QGuiApplication>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QScreen>
#include <QScopeGuard>
#include <QStyle>
#include <QTableWidgetItem>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

namespace {
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
    buttons->button(QDialogButtonBox::Ok)->setDefault(true);
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
} // namespace

GalleryWindow::GalleryWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::GalleryWindow)
{
    ui->setupUi(this);
    configureGallery();
    populateCollections();
    configurePaletteLab();
}

GalleryWindow::~GalleryWindow() { delete ui; }

void GalleryWindow::configureGallery()
{
    setProperty("winuiBackdrop", QStringLiteral("mica"));
    for (QLabel *heading : {ui->controlsHeading, ui->collectionsHeading,
                            ui->settingsHeading, ui->dialogsHeading,
                            ui->paletteHeading})
        setHeadingFont(heading);
    ui->themeCombo->setMinimumWidth(ui->themeCombo->sizeHint().width());
    ui->densityCombo->setMinimumWidth(ui->densityCombo->sizeHint().width());
    ui->autoSuggestEdit->setCompleter(new QCompleter(
        {tr("Alpha"), tr("Beta"), tr("Gamma")}, ui->autoSuggestEdit));
    ui->selectedEdit->selectAll();
    ui->advancedDetails->setVisible(ui->advancedCard->isChecked());
    ui->collectionsTabs->setTabEnabled(ui->collectionsTabs->indexOf(ui->disabledTab), false);

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
    const QList<QStyle::StandardPixmap> navigationIcons = {
        QStyle::SP_DesktopIcon, QStyle::SP_DirIcon, QStyle::SP_FileDialogContentsView,
        QStyle::SP_MessageBoxInformation, QStyle::SP_FileDialogDetailedView};
    for (int row = 0; row < ui->navigationList->count(); ++row)
        ui->navigationList->item(row)->setIcon(icon(navigationIcons.at(row)));

    connect(ui->themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GalleryWindow::setTheme);
    connect(ui->densityCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [](int index) {
        if (qApp->style())
            qApp->style()->setProperty("densityMode", index);
    });
    connect(ui->advancedCard, &QGroupBox::toggled,
            ui->advancedDetails, &QWidget::setVisible);
    connect(ui->searchSettings, &QLineEdit::textChanged, this,
            [this](const QString &text) {
        for (int row = 0; row < ui->navigationList->count(); ++row) {
            QListWidgetItem *item = ui->navigationList->item(row);
            item->setHidden(!item->text().contains(text, Qt::CaseInsensitive));
        }
    });
    connect(ui->messageDialogButton, &QPushButton::clicked, this, [this] {
        QMessageBox::information(this, tr("WinUI 3 Style"),
            tr("This is a native Qt message box rendered by the style."));
    });
    connect(ui->contentDialogButton, &QPushButton::clicked, this, [this] {
        QDialog dialog(this);
        configureContentDialog(&dialog);
        dialog.exec();
    });
}

void GalleryWindow::populateCollections()
{
    const QIcon folder = style()->standardIcon(QStyle::SP_DirIcon, nullptr, this);
    const QIcon file = style()->standardIcon(QStyle::SP_FileIcon, nullptr, this);
    for (const QString &text : {tr("Documents"), tr("Pictures"), tr("Downloads")})
        new QListWidgetItem(folder, text, ui->listViewTab);
    new QListWidgetItem(file, tr("Readme.txt"), ui->listViewTab);
    auto *root = new QTreeWidgetItem(ui->treeViewTab,
                                     {tr("Example album — Selected tracks")});
    root->setIcon(0, folder);
    root->setFlags(root->flags() | Qt::ItemIsUserCheckable
                   | Qt::ItemIsAutoTristate);
    root->setCheckState(0, Qt::PartiallyChecked);
    const QStringList tracks{tr("01 — First track"), tr("02 — Second track"),
                             tr("03 — Third track"), tr("04 — Fourth track")};
    for (int index = 0; index < tracks.size(); ++index) {
        auto *track = new QTreeWidgetItem(root, {tracks.at(index)});
        track->setFlags(track->flags() | Qt::ItemIsUserCheckable);
        track->setCheckState(0, index == 2 ? Qt::Unchecked : Qt::Checked);
    }
    root->setExpanded(true);
    const QString rows[4][3] = {
        {tr("Button"), tr("Ready"), tr("Complete")},
        {tr("ComboBox"), tr("Interactive"), tr("Complete")},
        {tr("TreeView"), tr("Expanded"), tr("Complete")},
        {tr("TableView"), tr("Editable"), tr("Complete")}};
    for (int row = 0; row < 4; ++row)
        for (int column = 0; column < 3; ++column)
            ui->tableViewTab->setItem(row, column, new QTableWidgetItem(rows[row][column]));
    ui->tableViewTab->horizontalHeader()->setStretchLastSection(true);
}

void GalleryWindow::configurePaletteLab()
{
    struct RoleRow { QPalette::ColorRole role; const char *label; };
    static const RoleRow roles[] = {
        {QPalette::Window, QT_TR_NOOP("Window (paper)")},
        {QPalette::WindowText, QT_TR_NOOP("WindowText/Text (ink)")},
        {QPalette::Base, QT_TR_NOOP("Base (layer)")},
        {QPalette::Button, QT_TR_NOOP("Button (control fill)")},
        {QPalette::Highlight, QT_TR_NOOP("Highlight (selection)")}};
    struct PaletteState { QPalette working; QList<QLabel *> swatches; };
    auto *state = new PaletteState{ui->palettePreview->palette(), {}};
    connect(ui->palettePreview, &QObject::destroyed, [state] { delete state; });
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
        connect(pick, &QPushButton::clicked, this,
                [this, state, role = roles[i].role, refresh] {
            const QColor color = QColorDialog::getColor(state->working.color(role),
                this, tr("Choose color"), QColorDialog::ShowAlphaChannel);
            if (!color.isValid()) return;
            for (QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive}) {
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
        if (color.isValid()) qApp->style()->setProperty("accentColor", color);
    });
    ui->paletteRolesLayout->addWidget(accent, 2, 6, 1, 2);
    refresh();
}

void GalleryWindow::setTheme(int index)
{
    qApp->style()->setProperty("themeMode", index);
    setProperty("winuiBackdrop", QStringLiteral("mica"));
}

bool GalleryWindow::saveSnapshots(const QString &directory)
{
    QDir output;
    if (!output.mkpath(directory)) return false;
    output.setPath(directory);
    QStyle *activeStyle = qApp->style();
    if (!activeStyle->property("themeMode").isValid()) return false;
    const QVariant previousTheme = activeStyle->property("themeMode");
    const QVariant previousDensity = activeStyle->property("densityMode");
    const QVariant previousAccent = activeStyle->property("accentColor");
    const QVariant previousBackdrop = property("winuiBackdrop");
    const QPalette previousPalette = palette();
    const bool previousAutoFill = autoFillBackground();
    const bool animationsWereDisabled = qEnvironmentVariableIsSet("WINUI3STYLE_DISABLE_ANIMATIONS");
    const QByteArray previousAnimationSetting = qgetenv("WINUI3STYLE_DISABLE_ANIMATIONS");
    qputenv("WINUI3STYLE_DISABLE_ANIMATIONS", "1");
    setProperty("winuiBackdrop", QStringLiteral("none"));
    setPalette(QPalette());
    setAutoFillBackground(true);
    activeStyle->setProperty("accentColor", QColor(0, 120, 212));
    activeStyle->setProperty("densityMode", 0);
    const bool mouseEventsWereTransparent = testAttribute(Qt::WA_TransparentForMouseEvents);
    const QPoint previousCursorPosition = QCursor::pos();
    const auto restore = qScopeGuard([&] {
        activeStyle->setProperty("themeMode", previousTheme);
        activeStyle->setProperty("densityMode", previousDensity);
        activeStyle->setProperty("accentColor", previousAccent);
        setProperty("winuiBackdrop", previousBackdrop);
        if (animationsWereDisabled) qputenv("WINUI3STYLE_DISABLE_ANIMATIONS", previousAnimationSetting);
        else qunsetenv("WINUI3STYLE_DISABLE_ANIMATIONS");
        setPalette(previousPalette);
        setAutoFillBackground(previousAutoFill);
        setAttribute(Qt::WA_TransparentForMouseEvents, mouseEventsWereTransparent);
        QCursor::setPos(previousCursorPosition);
    });
    if (QScreen *screen = QGuiApplication::primaryScreen())
        QCursor::setPos(screen->availableGeometry().bottomRight());
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    const auto settle = [this] {
        if (QWidget *focused = qApp->focusWidget()) focused->clearFocus();
        for (QWidget *widget : findChildren<QWidget *>()) {
            QEvent leave(QEvent::Leave);
            QCoreApplication::sendEvent(widget, &leave);
        }
        qApp->processEvents();
    };
    bool success = true;
    for (int mode : {1, 2}) {
        activeStyle->setProperty("themeMode", mode);
        const QString theme = mode == 1 ? QStringLiteral("light") : QStringLiteral("dark");
        ui->fileMenu->popup(ui->menuBar->mapToGlobal(QPoint(
            ui->menuBar->actionGeometry(ui->menuBar->actions().first()).left(), ui->menuBar->height())));
        qApp->processEvents();
        success = ui->fileMenu->grab().save(output.filePath(theme + "-menu.png"), "PNG") && success;
        if (QAction *firstAction = ui->fileMenu->actions().value(0)) {
            ui->fileMenu->setActiveAction(firstAction);
            qApp->processEvents();
            success = ui->fileMenu->grab().save(
                output.filePath(theme + "-state-menu-hover.png"), "PNG") && success;
            const QPoint local = ui->fileMenu->actionGeometry(firstAction).center();
            QMouseEvent press(QEvent::MouseButtonPress, QPointF(local),
                              QPointF(ui->fileMenu->mapToGlobal(local)),
                              Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QCoreApplication::sendEvent(ui->fileMenu, &press);
            qApp->processEvents();
            success = ui->fileMenu->grab().save(
                output.filePath(theme + "-state-menu-pressed.png"), "PNG") && success;
        }
        ui->fileMenu->hide();
        for (int page = 0; page < ui->pages->count(); ++page) {
            if (ui->pages->widget(page) == ui->paletteLabPage) continue;
            ui->pages->setCurrentIndex(page);
            qApp->processEvents();
            settle();
            success = grab().save(output.filePath(QStringLiteral("%1-page-%2.png").arg(theme).arg(page)), "PNG") && success;
            auto *area = qobject_cast<QScrollArea *>(ui->pages->widget(page));
            if (area && area->verticalScrollBar()->maximum() > 0) {
                area->verticalScrollBar()->setValue(area->verticalScrollBar()->maximum());
                qApp->processEvents();
                success = grab().save(output.filePath(QStringLiteral("%1-page-%2-scrolled.png").arg(theme).arg(page)), "PNG") && success;
                area->verticalScrollBar()->setValue(0);
            }
            if (area && area->widget()) {
                if (auto *tabs = area->widget()->findChild<QTabWidget *>()) {
                    const int oldTab = tabs->currentIndex();
                    for (int tab = 0; tab < tabs->count(); ++tab) {
                        if (!tabs->isTabEnabled(tab)) continue;
                        tabs->setCurrentIndex(tab);
                        qApp->processEvents();
                        success = grab().save(output.filePath(QStringLiteral("%1-page-%2-tab-%3.png").arg(theme).arg(page).arg(tab)), "PNG") && success;
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
            success = popup->grab().save(output.filePath(theme + "-combo-popup.png"), "PNG") && success;
        ui->galleryComboBox->hidePopup();
        const auto saveControl = [&](QWidget *control, const QString &state) {
            qApp->processEvents();
            return control && control->grab().save(output.filePath(theme + "-state-" + state + ".png"), "PNG");
        };
        const auto sendPointerState = [](QWidget *control, bool hovered, bool pressed) {
            if (!control)
                return;
            QEvent boundary(hovered ? QEvent::Enter : QEvent::Leave);
            QCoreApplication::sendEvent(control, &boundary);
            const QPoint local = control->rect().center();
            QMouseEvent mouse(pressed ? QEvent::MouseButtonPress
                                      : QEvent::MouseButtonRelease,
                              QPointF(local), QPointF(control->mapToGlobal(local)),
                              Qt::LeftButton,
                              pressed ? Qt::LeftButton : Qt::NoButton,
                              Qt::NoModifier);
            QCoreApplication::sendEvent(control, &mouse);
            qApp->processEvents();
        };
        success = saveControl(ui->galleryStandardButton, "button-rest") && success;
        sendPointerState(ui->galleryStandardButton, true, false);
        success = saveControl(ui->galleryStandardButton, "button-hover") && success;
        sendPointerState(ui->galleryStandardButton, true, true);
        success = saveControl(ui->galleryStandardButton, "button-pressed") && success;
        sendPointerState(ui->galleryStandardButton, false, false);
        ui->galleryLineEdit->setFocus(Qt::TabFocusReason);
        success = saveControl(ui->galleryLineEdit, "textbox-keyboard-focus") && success;
        ui->galleryLineEdit->clearFocus();
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
            && ui->galleryToggleSwitchDisabled->property("winuiToggleSwitch").toBool() && success;
        QDialog contentDialog(this);
        configureContentDialog(&contentDialog);
        contentDialog.show();
        qApp->processEvents();
        success = contentDialog.grab().save(output.filePath(theme + "-content-dialog.png"), "PNG") && success;
        contentDialog.close();

        QMessageBox messageBox(QMessageBox::Information, tr("WinUI 3 Style"),
            tr("This is a native Qt message box rendered by the style."),
            QMessageBox::Ok, this);
        messageBox.show();
        qApp->processEvents();
        success = messageBox.grab().save(
            output.filePath(theme + "-message-box.png"), "PNG") && success;
        messageBox.close();

        // Keep all historical Standard captures byte-comparable, then add a
        // small Compact oracle for the controls covered by WinUI's Compact
        // Sizing resource rather than blindly doubling the whole matrix.
        activeStyle->setProperty("densityMode", 1);
        ui->densityCombo->setCurrentIndex(1);
        for (int page : {0, 1}) {
            ui->pages->setCurrentIndex(page);
            qApp->processEvents();
            settle();
            success = grab().save(output.filePath(
                QStringLiteral("%1-density-compact-page-%2.png")
                    .arg(theme).arg(page)), "PNG") && success;
        }
        ui->pages->setCurrentIndex(0);
        ui->galleryComboBox->showPopup();
        qApp->processEvents();
        if (QWidget *popup = ui->galleryComboBox->view()->window()) {
            success = popup->grab().save(output.filePath(
                theme + "-density-compact-combo-popup.png"), "PNG") && success;
        }
        ui->galleryComboBox->hidePopup();
        ui->densityCombo->setCurrentIndex(0);
        activeStyle->setProperty("densityMode", 0);
    }
    return success;
}
