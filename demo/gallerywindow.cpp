#include "gallerywindow.h"

#include <winui3style/navigationview.h>
#include <winui3style/animatedstack.h>
#include <winui3style/settingscard.h>
#include <winui3style/winui3icons.h>
#include <winui3style/winui3style.h>

#include <QAction>
#include <QAbstractItemView>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDockWidget>
#include <QEvent>
#include <QCursor>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QKeySequence>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QProgressBar>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QScreen>
#include <QScopeGuard>
#include <QSlider>
#include <QSplitter>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace {

QWidget *scrollingPage(const QString &title, QLayout *content)
{
    auto *body = new QWidget;
    auto *layout = new QVBoxLayout(body);
    layout->setContentsMargins(28, 20, 28, 28);
    layout->setSpacing(16);
    auto *heading = new QLabel(title);
    QFont font = heading->font();
    font.setPixelSize(28);
    font.setWeight(QFont::DemiBold);
    heading->setFont(font);
    layout->addWidget(heading);
    layout->addLayout(content);
    layout->addStretch();
    auto *area = new QScrollArea;
    area->setFrameShape(QFrame::NoFrame);
    area->setWidgetResizable(true);
    area->setWidget(body);
    return area;
}

QGroupBox *section(const QString &title, QLayout *layout)
{
    auto *box = new QGroupBox(title);
    box->setLayout(layout);
    return box;
}

void configureContentDialog(QDialog *dialog)
{
    WinUI3::Style::setContentDialog(dialog);
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
        "Every control remains a native QWidget, with WinUI spacing and interaction states."));
    description->setWordWrap(true);
    layout->addWidget(description);
    layout->addStretch();
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok
                                         | QDialogButtonBox::Cancel);
    if (auto *primary = buttons->button(QDialogButtonBox::Ok)) {
        primary->setDefault(true);
        primary->setMinimumWidth(120);
    }
    if (auto *close = buttons->button(QDialogButtonBox::Cancel))
        close->setMinimumWidth(120);
    QObject::connect(buttons, &QDialogButtonBox::accepted,
                     dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected,
                     dialog, &QDialog::reject);
    layout->addWidget(buttons);
}

} // namespace

GalleryWindow::GalleryWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_navigation(new WinUI3::NavigationView(this))
{
    setWindowTitle(tr("WinUI 3 for Qt Widgets"));
    auto *fileMenu = menuBar()->addMenu(tr("File"));
    auto *newAction = fileMenu->addAction(WinUI3::icon(WinUI3::Icon::Add),
                                          tr("New project"));
    newAction->setShortcut(QKeySequence::New);
    auto *openAction = fileMenu->addAction(WinUI3::icon(WinUI3::Icon::Folder),
        tr("Open a recent project with a deliberately long name"));
    openAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O));
    auto *autoSave = fileMenu->addAction(tr("Save changes automatically"));
    autoSave->setCheckable(true);
    autoSave->setChecked(true);
    auto *exportMenu = fileMenu->addMenu(tr("Export"));
    exportMenu->addAction(tr("Portable document"));
    exportMenu->addAction(tr("Image"));
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Exit"), this, &QWidget::close);
    auto *viewMenu = menuBar()->addMenu(tr("View"));
    viewMenu->addAction(tr("Navigation pane"))->setCheckable(true);

    auto *toolbar = addToolBar(tr("Command bar"));
    toolbar->setMovable(false);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbar->addAction(WinUI3::icon(WinUI3::Icon::Add), tr("New"));
    toolbar->addAction(WinUI3::icon(WinUI3::Icon::Save), tr("Save"));
    toolbar->addSeparator();
    auto *play = toolbar->addAction(WinUI3::icon(WinUI3::Icon::Play), tr("Preview"));
    play->setCheckable(true);
    toolbar->addAction(WinUI3::icon(WinUI3::Icon::More), tr("More"));
    auto *textTool = new QToolButton(toolbar);
    textTool->setText(tr("Text tool"));
    textTool->setIcon(WinUI3::icon(WinUI3::Icon::Edit));
    textTool->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbar->addWidget(textTool);
    auto *checkTool = new QToolButton(toolbar);
    checkTool->setText(tr("Pin"));
    checkTool->setIcon(WinUI3::icon(WinUI3::Icon::Save));
    checkTool->setCheckable(true);
    checkTool->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbar->addWidget(checkTool);

    auto *theme = new QComboBox(toolbar);
    theme->addItems({tr("System theme"), tr("Light"), tr("Dark")});
    theme->setAccessibleName(tr("Theme"));
    toolbar->addWidget(theme);
    connect(theme, &QComboBox::currentIndexChanged, this, &GalleryWindow::setTheme);

    m_navigation->addPage(controlsPage(), WinUI3::icon(WinUI3::Icon::Home), tr("Controls"));
    m_navigation->addPage(collectionsPage(), WinUI3::icon(WinUI3::Icon::Folder), tr("Collections"));
    m_navigation->addPage(settingsPage(), WinUI3::icon(WinUI3::Icon::Settings), tr("Settings"));
    m_navigation->addPage(dialogsPage(), WinUI3::icon(WinUI3::Icon::More), tr("Dialogs & states"));
    setCentralWidget(m_navigation);
}

bool GalleryWindow::saveSnapshots(const QString &directory)
{
    QDir output;
    if (!output.mkpath(directory)) return false;
    output.setPath(directory);
    auto *winuiStyle = qobject_cast<WinUI3::Style *>(qApp->style());
    if (!winuiStyle) return false;

    const int previousDuration = m_navigation->stack()->duration();
    const auto previousTheme = winuiStyle->themeMode();
    const QColor previousAccent = winuiStyle->accentColor();
    const QPalette previousPalette = palette();
    const bool previousAutoFill = autoFillBackground();
    m_navigation->stack()->setDuration(0);
    const bool animationsWereDisabled = qEnvironmentVariableIsSet(
        "WINUI3STYLE_DISABLE_ANIMATIONS");
    const QByteArray previousAnimationSetting = qgetenv(
        "WINUI3STYLE_DISABLE_ANIMATIONS");
    qputenv("WINUI3STYLE_DISABLE_ANIMATIONS", "1");
    // Snapshots must not depend on the host's current Windows accent. Keep the
    // interactive gallery system-aware, but use Fluent's reference blue for
    // deterministic visual baselines across machines.
    winuiStyle->setAccentColor(QColor(0, 120, 212));
    setPalette(QPalette());
    setAutoFillBackground(true);
    const bool mouseEventsWereTransparent =
        testAttribute(Qt::WA_TransparentForMouseEvents);
    const QPoint previousCursorPosition = QCursor::pos();
    const auto restoreCaptureState = qScopeGuard([&] {
        m_navigation->stack()->setDuration(previousDuration);
        winuiStyle->setThemeMode(previousTheme);
        winuiStyle->setAccentColor(previousAccent);
        if (animationsWereDisabled)
            qputenv("WINUI3STYLE_DISABLE_ANIMATIONS", previousAnimationSetting);
        else
            qunsetenv("WINUI3STYLE_DISABLE_ANIMATIONS");
        setPalette(previousPalette);
        setAutoFillBackground(previousAutoFill);
        setAttribute(Qt::WA_TransparentForMouseEvents, mouseEventsWereTransparent);
        QCursor::setPos(previousCursorPosition);
        for (QWidget *topLevel : qApp->topLevelWidgets()) {
            if (topLevel->windowType() == Qt::Popup
                || topLevel->windowType() == Qt::ToolTip)
                topLevel->hide();
        }
    });
    if (QScreen *screen = QGuiApplication::primaryScreen())
        QCursor::setPos(screen->availableGeometry().bottomRight());
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    const auto settlePointerStates = [this] {
        if (QWidget *focused = qApp->focusWidget())
            focused->clearFocus();
        QEvent leave(QEvent::Leave);
        QCoreApplication::sendEvent(this, &leave);
        const auto descendants = findChildren<QWidget *>();
        for (QWidget *descendant : descendants) {
            QEvent descendantLeave(QEvent::Leave);
            QCoreApplication::sendEvent(descendant, &descendantLeave);
        }
        qApp->processEvents();
    };
    settlePointerStates();
    qApp->processEvents();
    bool success = true;
    for (const auto mode : {WinUI3::ThemeMode::Light, WinUI3::ThemeMode::Dark}) {
        winuiStyle->setThemeMode(mode);
        const QString theme = mode == WinUI3::ThemeMode::Light
            ? QStringLiteral("light") : QStringLiteral("dark");

        if (auto *fileMenu = menuBar()->actions().value(0)->menu()) {
            fileMenu->popup(menuBar()->mapToGlobal(
                QPoint(menuBar()->actionGeometry(menuBar()->actions().value(0)).left(),
                       menuBar()->height())));
            qApp->processEvents();
            success = fileMenu->grab().save(
                output.filePath(theme + QStringLiteral("-menu.png")), "PNG") && success;
            if (QAction *firstAction = fileMenu->actions().value(0)) {
                fileMenu->setActiveAction(firstAction);
                qApp->processEvents();
                success = fileMenu->grab().save(output.filePath(
                    theme + QStringLiteral("-state-menu-hover.png")), "PNG")
                    && success;
                const QPoint local = fileMenu->actionGeometry(firstAction).center();
                QMouseEvent press(QEvent::MouseButtonPress, QPointF(local),
                                  QPointF(fileMenu->mapToGlobal(local)),
                                  Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                QCoreApplication::sendEvent(fileMenu, &press);
                qApp->processEvents();
                success = fileMenu->grab().save(output.filePath(
                    theme + QStringLiteral("-state-menu-pressed.png")), "PNG")
                    && success;
            }
            fileMenu->hide();
        }
        for (int page = 0; page < m_navigation->count(); ++page) {
            m_navigation->setCurrentIndex(page);
            qApp->processEvents();
            settlePointerStates();
            const QString name = QStringLiteral("%1-page-%2.png").arg(theme).arg(page);
            success = grab().save(output.filePath(name), "PNG") && success;
            if (auto *area = qobject_cast<QScrollArea *>(m_navigation->widget(page));
                area && area->verticalScrollBar()->maximum() > 0) {
                area->verticalScrollBar()->setValue(area->verticalScrollBar()->maximum());
                qApp->processEvents();
                settlePointerStates();
                const QString scrolledName = QStringLiteral("%1-page-%2-scrolled.png")
                    .arg(theme).arg(page);
                success = grab().save(output.filePath(scrolledName), "PNG") && success;
                area->verticalScrollBar()->setValue(0);
            }
            if (auto *area = qobject_cast<QScrollArea *>(m_navigation->widget(page));
                area && area->widget()) {
                if (auto *tabs = area->widget()->findChild<QTabWidget *>()) {
                    const int previousTab = tabs->currentIndex();
                    for (int tab = 0; tab < tabs->count(); ++tab) {
                        if (!tabs->isTabEnabled(tab))
                            continue;
                        tabs->setCurrentIndex(tab);
                        qApp->processEvents();
                        const QString tabName = QStringLiteral("%1-page-%2-tab-%3.png")
                            .arg(theme).arg(page).arg(tab);
                        success = grab().save(output.filePath(tabName), "PNG")
                            && success;
                    }
                    tabs->setCurrentIndex(previousTab);
                }
            }
        }

        m_navigation->setCurrentIndex(0);
        qApp->processEvents();
        if (auto *area = qobject_cast<QScrollArea *>(m_navigation->widget(0));
            area && area->widget()) {
            if (auto *combo = area->widget()->findChild<QComboBox *>()) {
                combo->showPopup();
                qApp->processEvents();
                if (QWidget *popup = combo->view()->window())
                    success = popup->grab().save(output.filePath(
                        theme + QStringLiteral("-combo-popup.png")), "PNG") && success;
                combo->hidePopup();
            }

            const auto saveControl = [&](QWidget *control,
                                         const QString &state) {
                if (!control)
                    return false;
                qApp->processEvents();
                return control->grab().save(output.filePath(
                    theme + QStringLiteral("-state-") + state
                    + QStringLiteral(".png")), "PNG");
            };
            const auto sendPointerState = [&](QWidget *control,
                                              bool hovered, bool pressed) {
                if (!control)
                    return;
                QEvent boundary(hovered ? QEvent::Enter : QEvent::Leave);
                QCoreApplication::sendEvent(control, &boundary);
                const QPointF local(control->rect().center());
                const QPointF global(control->mapToGlobal(
                    control->rect().center()));
                QMouseEvent mouse(pressed ? QEvent::MouseButtonPress
                                          : QEvent::MouseButtonRelease,
                                  local, global, Qt::LeftButton,
                                  pressed ? Qt::LeftButton : Qt::NoButton,
                                  Qt::NoModifier);
                QCoreApplication::sendEvent(control, &mouse);
                qApp->processEvents();
            };

            QWidget *body = area->widget();
            auto *button = body->findChild<QPushButton *>(
                QStringLiteral("galleryStandardButton"));
            auto *lineEdit = body->findChild<QLineEdit *>(
                QStringLiteral("galleryLineEdit"));
            auto *stateCombo = body->findChild<QComboBox *>(
                QStringLiteral("galleryComboBox"));
            auto *checkBox = body->findChild<QCheckBox *>(
                QStringLiteral("galleryCheckBox"));
            auto *radio = body->findChild<QRadioButton *>(
                QStringLiteral("galleryRadioButton"));
            auto *disabledRadio = body->findChild<QRadioButton *>(
                QStringLiteral("galleryRadioButtonDisabled"));
            auto *disabledToggle = body->findChild<QCheckBox *>(
                QStringLiteral("galleryToggleSwitchDisabled"));
            auto *slider = body->findChild<QSlider *>(
                QStringLiteral("gallerySlider"));

            success = disabledRadio && !disabledRadio->isEnabled() && success;
            success = disabledToggle && !disabledToggle->isEnabled()
                && WinUI3::Style::isToggleSwitch(disabledToggle) && success;

            success = saveControl(button, QStringLiteral("button-rest")) && success;
            sendPointerState(button, true, false);
            success = saveControl(button, QStringLiteral("button-hover")) && success;
            sendPointerState(button, true, true);
            success = saveControl(button, QStringLiteral("button-pressed")) && success;
            sendPointerState(button, false, false);

            if (lineEdit) {
                lineEdit->setFocus(Qt::TabFocusReason);
                success = saveControl(lineEdit,
                    QStringLiteral("textbox-keyboard-focus")) && success;
                lineEdit->clearFocus();
            }
            if (stateCombo) {
                sendPointerState(stateCombo, true, true);
                success = saveControl(stateCombo,
                    QStringLiteral("combobox-pressed")) && success;
                sendPointerState(stateCombo, false, false);
                stateCombo->hidePopup();
            }
            if (checkBox) {
                checkBox->setChecked(true);
                success = saveControl(checkBox,
                    QStringLiteral("checkbox-checked")) && success;
                checkBox->setChecked(false);
                success = saveControl(checkBox,
                    QStringLiteral("checkbox-unchecked")) && success;
            }
            if (radio) {
                radio->setChecked(true);
                success = saveControl(radio,
                    QStringLiteral("radio-checked")) && success;
            }
            if (slider) {
                sendPointerState(slider, true, false);
                success = saveControl(slider,
                    QStringLiteral("slider-hover")) && success;
                sendPointerState(slider, false, false);
            }
        }

        QDialog contentDialog(this);
        configureContentDialog(&contentDialog);
        contentDialog.show();
        qApp->processEvents();
        success = contentDialog.grab().save(output.filePath(
            theme + QStringLiteral("-content-dialog.png")), "PNG") && success;
        contentDialog.close();

        QMessageBox messageBox(QMessageBox::Information,
            tr("WinUI 3 Style"),
            tr("This is a native Qt message box rendered by the style."),
            QMessageBox::Ok, this);
        messageBox.show();
        qApp->processEvents();
        success = messageBox.grab().save(output.filePath(
            theme + QStringLiteral("-message-box.png")), "PNG") && success;
        messageBox.close();
    }
    return success;
}

QWidget *GalleryWindow::controlsPage()
{
    auto *content = new QVBoxLayout;
    content->setSpacing(16);

    auto *buttons = new QHBoxLayout;
    auto *standard = new QPushButton(tr("Standard"));
    standard->setObjectName(QStringLiteral("galleryStandardButton"));
    auto *accent = new QPushButton(tr("Accent"));
    WinUI3::Style::setControlRole(accent, WinUI3::ControlRole::Accent);
    auto *subtle = new QPushButton(tr("Subtle"));
    WinUI3::Style::setControlRole(subtle, WinUI3::ControlRole::Subtle);
    auto *destructive = new QPushButton(tr("Delete"));
    WinUI3::Style::setControlRole(destructive, WinUI3::ControlRole::Destructive);
    auto *disabled = new QPushButton(tr("Disabled"));
    disabled->setEnabled(false);
    buttons->addWidget(standard);
    buttons->addWidget(accent);
    buttons->addWidget(subtle);
    buttons->addWidget(destructive);
    buttons->addWidget(disabled);
    buttons->addStretch();
    content->addWidget(section(tr("Buttons"), buttons));

    auto *selection = new QGridLayout;
    auto *check = new QCheckBox(tr("Check box"));
    check->setObjectName(QStringLiteral("galleryCheckBox"));
    check->setChecked(true);
    auto *tri = new QCheckBox(tr("Indeterminate"));
    tri->setTristate(true);
    tri->setCheckState(Qt::PartiallyChecked);
    auto *radioA = new QRadioButton(tr("Option A"));
    radioA->setObjectName(QStringLiteral("galleryRadioButton"));
    auto *radioB = new QRadioButton(tr("Option B"));
    auto *radioDisabled = new QRadioButton(tr("Disabled option"));
    radioDisabled->setObjectName(QStringLiteral("galleryRadioButtonDisabled"));
    radioDisabled->setEnabled(false);
    radioA->setChecked(true);
    auto *toggle = new QCheckBox;
    auto *toggleOff = new QCheckBox;
    auto *toggleDisabled = new QCheckBox;
    toggleDisabled->setObjectName(QStringLiteral("galleryToggleSwitchDisabled"));
    WinUI3::Style::setToggleSwitch(toggle);
    WinUI3::Style::setToggleSwitch(toggleOff);
    WinUI3::Style::setToggleSwitch(toggleDisabled);
    WinUI3::Style::setToggleSwitchText(toggle, tr("On"), tr("Off"));
    WinUI3::Style::setToggleSwitchText(toggleOff, tr("On"), tr("Off"));
    WinUI3::Style::setToggleSwitchText(toggleDisabled, tr("On"), tr("Off"));
    toggle->setChecked(true);
    toggleDisabled->setEnabled(false);
    selection->addWidget(check, 0, 0);
    selection->addWidget(tri, 0, 1);
    selection->addWidget(radioA, 1, 0);
    selection->addWidget(radioB, 1, 1);
    selection->addWidget(radioDisabled, 3, 0);
    selection->addWidget(toggle, 2, 0);
    selection->addWidget(toggleOff, 2, 1);
    selection->addWidget(toggleDisabled, 3, 1);
    content->addWidget(section(tr("Selection controls"), selection));

    auto *groupStates = new QHBoxLayout;
    auto *plainGroup = new QGroupBox(tr("Standard group"));
    auto *plainLayout = new QVBoxLayout(plainGroup);
    plainLayout->addWidget(new QLabel(tr("A WinUI-consistent card for grouped Qt content.")));
    auto *checkedGroup = new QGroupBox(tr("Enable diagnostics"));
    checkedGroup->setCheckable(true);
    checkedGroup->setChecked(true);
    auto *checkedLayout = new QVBoxLayout(checkedGroup);
    checkedLayout->addWidget(new QCheckBox(tr("Include detailed logs")));
    auto *uncheckedGroup = new QGroupBox(tr("Use proxy server"));
    uncheckedGroup->setCheckable(true);
    uncheckedGroup->setChecked(false);
    auto *uncheckedLayout = new QVBoxLayout(uncheckedGroup);
    uncheckedLayout->addWidget(new QLineEdit(tr("proxy.example")));
    groupStates->addWidget(plainGroup);
    groupStates->addWidget(checkedGroup);
    groupStates->addWidget(uncheckedGroup);
    content->addWidget(section(tr("Group boxes"), groupStates));

    auto *inputs = new QFormLayout;
    inputs->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
    auto *line = new QLineEdit;
    line->setObjectName(QStringLiteral("galleryLineEdit"));
    line->setText(tr("Text to clear"));
    line->setPlaceholderText(tr("Placeholder text"));
    line->setClearButtonEnabled(true);
    line->setFixedWidth(300);
    auto *password = new QLineEdit;
    password->setEchoMode(QLineEdit::Password);
    password->setText(QStringLiteral("password"));
    password->setFixedWidth(300);
    auto *readOnlyLine = new QLineEdit(tr("Read-only text can still be selected"));
    readOnlyLine->setReadOnly(true);
    readOnlyLine->setClearButtonEnabled(true);
    readOnlyLine->setFixedWidth(300);
    auto *selectedLine = new QLineEdit(tr("Selected text"));
    selectedLine->setFixedWidth(300);
    selectedLine->selectAll();
    auto *disabledLine = new QLineEdit(tr("Disabled text"));
    disabledLine->setPlaceholderText(tr("Disabled placeholder"));
    disabledLine->setEnabled(false);
    disabledLine->setFixedWidth(300);
    auto *combo = new QComboBox;
    combo->setObjectName(QStringLiteral("galleryComboBox"));
    combo->addItems({tr("First item"), tr("Second item"), tr("Third item")});
    combo->setCurrentIndex(1);
    combo->setFixedWidth(200);
    auto *spin = new QSpinBox;
    spin->setRange(0, 100);
    spin->setValue(42);
    spin->setFixedWidth(150);
    auto *verticalSpin = new QSpinBox;
    verticalSpin->setRange(0, 100);
    verticalSpin->setValue(42);
    verticalSpin->setFixedWidth(150);
    WinUI3::Style::setVerticalSpinButtons(verticalSpin);
    auto *disabledSpin = new QSpinBox;
    disabledSpin->setObjectName(QStringLiteral("galleryDisabledSpinBox"));
    disabledSpin->setRange(0, 100);
    disabledSpin->setValue(42);
    disabledSpin->setFixedWidth(150);
    disabledSpin->setEnabled(false);
    inputs->addRow(tr("Text box"), line);
    inputs->addRow(tr("Password"), password);
    inputs->addRow(tr("Read-only"), readOnlyLine);
    inputs->addRow(tr("Selection"), selectedLine);
    inputs->addRow(tr("Disabled"), disabledLine);
    inputs->addRow(tr("Combo box"), combo);
    inputs->addRow(tr("Number box (WinUI)"), spin);
    inputs->addRow(tr("Number box (vertical)"), verticalSpin);
    inputs->addRow(tr("Number box (disabled)"), disabledSpin);
    content->addWidget(section(tr("Input"), inputs));

    auto *values = new QVBoxLayout;
    auto *progress = new QProgressBar;
    progress->setValue(64);
    auto *indeterminate = new QProgressBar;
    indeterminate->setRange(0, 0);
    auto *slider = new QSlider(Qt::Horizontal);
    slider->setObjectName(QStringLiteral("gallerySlider"));
    slider->setRange(0, 100);
    slider->setValue(42);
    slider->setTickPosition(QSlider::TicksBelow);
    slider->setTickInterval(10);
    auto *invertedSlider = new QSlider(Qt::Horizontal);
    invertedSlider->setRange(0, 100);
    invertedSlider->setValue(42);
    invertedSlider->setInvertedAppearance(true);
    auto *disabledSlider = new QSlider(Qt::Horizontal);
    disabledSlider->setRange(0, 100);
    disabledSlider->setValue(42);
    disabledSlider->setEnabled(false);
    auto *verticalSlider = new QSlider(Qt::Vertical);
    verticalSlider->setRange(0, 100);
    verticalSlider->setValue(42);
    verticalSlider->setFixedHeight(112);
    auto *horizontalScrollBar = new QScrollBar(Qt::Horizontal);
    horizontalScrollBar->setRange(0, 100);
    horizontalScrollBar->setPageStep(24);
    horizontalScrollBar->setValue(35);
    auto *verticalScrollBar = new QScrollBar(Qt::Vertical);
    verticalScrollBar->setRange(0, 100);
    verticalScrollBar->setPageStep(24);
    verticalScrollBar->setValue(35);
    verticalScrollBar->setFixedHeight(112);
    auto *disabledScrollBar = new QScrollBar(Qt::Horizontal);
    disabledScrollBar->setRange(0, 100);
    disabledScrollBar->setPageStep(24);
    disabledScrollBar->setValue(35);
    disabledScrollBar->setEnabled(false);
    auto *orientationRow = new QHBoxLayout;
    orientationRow->addWidget(verticalSlider);
    orientationRow->addSpacing(20);
    orientationRow->addWidget(verticalScrollBar);
    orientationRow->addStretch();
    values->addWidget(progress);
    values->addWidget(indeterminate);
    values->addWidget(new QLabel(tr("Slider: ticks, inverted, disabled")));
    values->addWidget(slider);
    values->addWidget(invertedSlider);
    values->addWidget(disabledSlider);
    values->addWidget(new QLabel(tr("ScrollBar: horizontal, vertical, disabled")));
    values->addWidget(horizontalScrollBar);
    values->addWidget(disabledScrollBar);
    values->addLayout(orientationRow);
    content->addWidget(section(tr("Progress and values"), values));
    return scrollingPage(tr("Controls"), content);
}

QWidget *GalleryWindow::collectionsPage()
{
    auto *content = new QVBoxLayout;
    auto *tabs = new QTabWidget;
    tabs->setTabsClosable(true);
    tabs->setMovable(true);
    auto *list = new QListWidget;
    const QList<QPair<WinUI3::Icon, QString>> listEntries{
        {WinUI3::Icon::Folder, tr("Documents")},
        {WinUI3::Icon::More, tr("Pictures")},
        {WinUI3::Icon::Play, tr("Music")},
        {WinUI3::Icon::Settings, tr("Videos")}
    };
    for (const auto &[glyph, text] : listEntries) {
        auto *item = new QListWidgetItem(WinUI3::icon(glyph), text, list);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
    }
    list->item(3)->setFlags(list->item(3)->flags() & ~Qt::ItemIsEnabled);
    list->setEditTriggers(QAbstractItemView::DoubleClicked
                          | QAbstractItemView::EditKeyPressed);
    list->setCurrentRow(0);
    auto *tree = new QTreeWidget;
    tree->setHeaderHidden(true);
    auto *root = new QTreeWidgetItem(tree, {tr("Workspace")});
    root->setIcon(0, WinUI3::icon(WinUI3::Icon::Folder));
    auto *source = new QTreeWidgetItem(root, {tr("src")});
    source->setIcon(0, WinUI3::icon(WinUI3::Icon::Folder));
    auto *tests = new QTreeWidgetItem(root, {tr("tests")});
    tests->setIcon(0, WinUI3::icon(WinUI3::Icon::Folder));
    new QTreeWidgetItem(source, {tr("winui3style.cpp")});
    new QTreeWidgetItem(tests, {tr("tst_winui3style.cpp")});
    tree->expandAll();
    tree->setCurrentItem(root);
    auto *table = new QTableWidget(4, 3);
    table->setHorizontalHeaderLabels({tr("Control"), tr("State"), tr("Coverage")});
    const QStringList names{tr("Button"), tr("Toggle"), tr("Navigation"), tr("Menu")};
    for (int row = 0; row < names.size(); ++row) {
        table->setItem(row, 0, new QTableWidgetItem(names.at(row)));
        table->setItem(row, 1, new QTableWidgetItem(tr("Implemented")));
        table->setItem(row, 2, new QTableWidgetItem(QString::number(95 - row) + QLatin1Char('%')));
    }
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setCurrentCell(0, 0);
    table->setSortingEnabled(true);
    table->horizontalHeader()->setStretchLastSection(true);
    tabs->addTab(list, WinUI3::icon(WinUI3::Icon::More), tr("List view"));
    tabs->addTab(tree, WinUI3::icon(WinUI3::Icon::Folder), tr("Tree view"));
    tabs->addTab(table, WinUI3::icon(WinUI3::Icon::Settings), tr("Data grid"));
    const int disabledTab = tabs->addTab(new QWidget, tr("Disabled"));
    tabs->setTabEnabled(disabledTab, false);
    content->addWidget(tabs);
    return scrollingPage(tr("Collections"), content);
}

QWidget *GalleryWindow::settingsPage()
{
    auto *content = new QVBoxLayout;
    auto card = [](const QString &title, const QString &description,
                   WinUI3::Icon glyph, QWidget *trailing) {
        auto *item = new WinUI3::SettingsCard;
        item->setTitle(title);
        item->setDescription(description);
        item->setIcon(WinUI3::icon(glyph));
        item->setTrailingWidget(trailing);
        return item;
    };
    auto *notifications = new QCheckBox;
    WinUI3::Style::setToggleSwitch(notifications);
    notifications->setChecked(true);
    content->addWidget(card(tr("Notifications"), tr("Show alerts and status messages"),
                            WinUI3::Icon::Settings, notifications));
    auto *updates = new QComboBox;
    updates->addItems({tr("Automatic"), tr("Notify me"), tr("Manual")});
    content->addWidget(card(tr("Updates"), tr("Choose how updates are installed"),
                            WinUI3::Icon::Refresh, updates));
    auto *advanced = card(tr("Advanced options"), tr("Developer and diagnostic settings"),
                          WinUI3::Icon::More, nullptr);
    auto *details = new QTextEdit;
    details->setPlainText(tr("Expanded settings content. This area uses the WinUI expand/collapse motion token."));
    details->setMaximumHeight(100);
    advanced->setExpandableWidget(details);
    content->addWidget(advanced);
    return scrollingPage(tr("Settings"), content);
}

QWidget *GalleryWindow::dialogsPage()
{
    auto *content = new QVBoxLayout;
    auto *row = new QHBoxLayout;
    auto *message = new QPushButton(tr("Message dialog"));
    auto *modal = new QPushButton(tr("Content dialog"));
    row->addWidget(message);
    row->addWidget(modal);
    row->addStretch();
    connect(message, &QPushButton::clicked, this, [this] {
        QMessageBox::information(this, tr("WinUI 3 Style"),
                                 tr("This is a native Qt message box rendered by the style."));
    });
    connect(modal, &QPushButton::clicked, this, [this] {
        QDialog dialog(this);
        configureContentDialog(&dialog);
        dialog.exec();
    });
    content->addWidget(section(tr("Dialogs"), row));

    auto *states = new QGridLayout;
    for (int i = 0; i < 8; ++i) {
        auto *button = new QPushButton(i & 1 ? tr("Toggle button") : tr("Command"));
        button->setCheckable(i & 1);
        button->setChecked(i == 3);
        button->setEnabled(i < 6);
        states->addWidget(button, i / 4, i % 4);
    }
    content->addWidget(section(tr("Persistent states"), states));

    auto *splitter = new QSplitter(Qt::Horizontal);
    splitter->setChildrenCollapsible(false);
    auto *leftPane = new QTextEdit;
    leftPane->setPlainText(tr("Drag the fluent splitter handle to resize this pane."));
    auto *rightPane = new QTextEdit;
    rightPane->setPlainText(tr("The neutral one-pixel divider reveals on hover and uses the accent while dragged."));
    splitter->addWidget(leftPane);
    splitter->addWidget(rightPane);
    splitter->setSizes({320, 320});
    splitter->setMinimumHeight(130);
    auto *splitterLayout = new QVBoxLayout;
    splitterLayout->addWidget(splitter);
    auto *verticalSplitter = new QSplitter(Qt::Vertical);
    auto *plainPane = new QPlainTextEdit;
    plainPane->setPlainText(tr("QPlainTextEdit uses the same RichEditBox frame and focus contract."));
    auto *verticalLabel = new QLabel(tr("Both splitter orientations use the same fluent handle."));
    verticalLabel->setAlignment(Qt::AlignCenter);
    verticalSplitter->addWidget(plainPane);
    verticalSplitter->addWidget(verticalLabel);
    verticalSplitter->setSizes({90, 40});
    verticalSplitter->setMinimumHeight(140);
    splitterLayout->addWidget(verticalSplitter);
    content->addWidget(section(tr("Splitter and resize handle"), splitterLayout));

    auto *dockHost = new QMainWindow;
    dockHost->setWindowFlags(Qt::Widget);
    dockHost->setMinimumHeight(220);
    auto *workspace = new QLabel(tr("Docking workspace"));
    workspace->setAlignment(Qt::AlignCenter);
    dockHost->setCentralWidget(workspace);
    auto *inspector = new QDockWidget(tr("Inspector"), dockHost);
    inspector->setObjectName(QStringLiteral("embeddedInspector"));
    inspector->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    inspector->setFeatures(QDockWidget::DockWidgetClosable
                           | QDockWidget::DockWidgetFloatable
                           | QDockWidget::DockWidgetMovable);
    auto *inspectorBody = new QWidget(inspector);
    auto *inspectorLayout = new QFormLayout(inspectorBody);
    inspectorLayout->addRow(tr("Name"), new QLineEdit(tr("Selected control")));
    auto *visible = new QCheckBox(tr("Visible"));
    visible->setChecked(true);
    inspectorLayout->addRow(visible);
    inspector->setWidget(inspectorBody);
    inspector->setMinimumWidth(240);
    dockHost->addDockWidget(Qt::RightDockWidgetArea, inspector);
    auto *dockLayout = new QVBoxLayout;
    dockLayout->addWidget(dockHost);
    content->addWidget(section(tr("Dock widget and dock separator"), dockLayout));
    return scrollingPage(tr("Dialogs & states"), content);
}

void GalleryWindow::setTheme(int index)
{
    auto *winuiStyle = qobject_cast<WinUI3::Style *>(qApp->style());
    if (!winuiStyle) return;
    const auto mode = index == 1 ? WinUI3::ThemeMode::Light
        : index == 2 ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::System;
    winuiStyle->setThemeMode(mode);
}
