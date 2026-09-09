// SPDX-License-Identifier: LGPL-2.1-or-later
// Domain split of tst_winui3style.cpp (plan step 8): test-function names
// kept identical so the DPI reruns in tests/CMakeLists.txt keep working.
#include <winui3style/animatedstack.h>
#include <winui3style/navigationview.h>
#include <winui3style/settingscard.h>
#include <winui3style/toggleswitch.h>
#include <winui3style/winui3backdrop.h>
#include <winui3style/winui3style.h>
#include <winui3style/winui3icons.h>

#include "../src/winui3frameproperties_p.h"
#include "../src/winui3helpers_p.h"
#include "../src/winui3tokens_p.h"

#include <QLabel>
#include <QListWidget>
#include <QListView>
#include <QLineEdit>
#include <QAction>
#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAccessible>
#include <QCheckBox>
#include <QComboBox>
#include <QCommandLinkButton>
#include <QDialog>
#include <QDialogButtonBox>
#include <QEvent>
#include <QDockWidget>
#include <QFocusEvent>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QGroupBox>
#include <QHeaderView>
#include <QImage>
#include <QKeySequence>
#include <QKeyEvent>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QParallelAnimationGroup>
#include <QPointer>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollBar>
#include <QScrollArea>
#include <QSplitter>
#include <QSignalSpy>
#include <QSizeGrip>
#include <QSpinBox>
#include <QStatusBar>
#include <QStyleFactory>
#include <QStyleOptionGroupBox>
#include <QStyleOptionButton>
#include <QStyleOptionComboBox>
#include <QStyleOptionHeader>
#include <QStyleOptionMenuItem>
#include <QStyleOptionSlider>
#include <QStyleOptionSpinBox>
#include <QStyleOptionTab>
#include <QStyledItemDelegate>
#include <QStyleOptionToolButton>
#include <QSlider>
#include <QTabBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QToolButton>
#include <QToolBar>
#include <QTimer>
#include <QTest>
#include <QTextEdit>
#include <QTreeWidget>
#include <QVariantAnimation>
#include <QVBoxLayout>
#include <QWizard>
#include <QWizardPage>
#include <QtMath>
#include <QStandardItemModel>

#include <cmath>
#include <limits>

#include "winui3testhelpers.h"

class WinUI3SurfacesTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void backdropLifecycleContract();
    void backdropButtonRepaintDoesNotAccumulate();
    void backdropComboRepaintDoesNotAccumulate();
    void contentDialogContract();
    void messageBoxContentDialogContract();
    void wizardSurfaceContract();
    void wizardUsesModernStyleHint();
    void wizardOpenThemeSwitchLifecycle();
    void contentDialogScrimLifecycle();
    void progressAnimationAndOrientations();
    void progressTextAndDisabledPaletteContract();
    void progressTimerScalingAndLifecycle();
};

void WinUI3SurfacesTest::initTestCase()
{
    qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light));
}

void WinUI3SurfacesTest::init()
{
    if (auto *style = qobject_cast<WinUI3::Style *>(qApp->style())) {
        style->setThemeMode(WinUI3::ThemeMode::Light);
        style->setAccentColor({});
    }
}

void WinUI3SurfacesTest::cleanup()
{
    for (QWidget *widget : qApp->topLevelWidgets()) {
        if (widget->windowType() == Qt::Popup || widget->windowType() == Qt::ToolTip)
            widget->hide();
        else
            widget->close();
    }
    if (QWidget *focus = qApp->focusWidget())
        focus->clearFocus();
    qApp->processEvents();
}

void WinUI3SurfacesTest::backdropLifecycleContract()
{
#ifndef Q_OS_WIN
    QWidget window;
    QVERIFY(!WinUI3::applyBackdrop(&window, WinUI3::Backdrop::Mica));
    return;
#else
    // DWM is deliberately not part of this unit test. The offscreen Windows
    // platform still exercises the palette/attribute/property lifecycle
    // deterministically; native composition is covered by the native test
    // target and by the gallery.
    if (QGuiApplication::platformName() != QStringLiteral("offscreen"))
        QSKIP("DWM-backed lifecycle belongs to the native test target");

    QWidget window;
    window.setAttribute(Qt::WA_TranslucentBackground, false);
    window.setAttribute(Qt::WA_NoSystemBackground, false);
    window.setAttribute(Qt::WA_OpaquePaintEvent, true);
    window.setAutoFillBackground(true);
    QPalette custom = window.palette();
    custom.setColor(QPalette::Window, QColor(19, 37, 53));
    window.setPalette(custom);

    const QPalette originalPalette = window.palette();
    const bool originalTranslucent = window.testAttribute(Qt::WA_TranslucentBackground);
    const bool originalNoSystemBackground = window.testAttribute(Qt::WA_NoSystemBackground);
    const bool originalOpaquePaint = window.testAttribute(Qt::WA_OpaquePaintEvent);
    const bool originalAutoFill = window.autoFillBackground();

    // Offscreen has no HWND/DWM: prepareBackdropSurface skips attribute changes
    // there to keep fallback PNGs deterministic. Attributes stay as set above.
    QVERIFY(!window.testAttribute(Qt::WA_NoSystemBackground));
    QVERIFY(window.testAttribute(Qt::WA_OpaquePaintEvent));
    QVERIFY(window.autoFillBackground());
    QCOMPARE(window.palette().color(QPalette::Window).alpha(), 255);

    QVERIFY(WinUI3::applyBackdrop(&window, WinUI3::Backdrop::MicaAlt));
    QCOMPARE(window.property("_winui_backdrop").toInt(), int(WinUI3::Backdrop::MicaAlt));
    QVERIFY(WinUI3::applyBackdrop(&window, WinUI3::Backdrop::None));
    QVERIFY(!window.property("_winui_backdrop").isValid());
    QCOMPARE(window.testAttribute(Qt::WA_TranslucentBackground), originalTranslucent);
    QCOMPARE(window.testAttribute(Qt::WA_NoSystemBackground), originalNoSystemBackground);
    QCOMPARE(window.testAttribute(Qt::WA_OpaquePaintEvent), originalOpaquePaint);
    QCOMPARE(window.autoFillBackground(), originalAutoFill);
    QCOMPARE(window.palette(), originalPalette);

    // Clearing an already-cleared backdrop must remain a no-op. In
    // particular it must not create a native handle just to reset DWM state.
    QVERIFY(WinUI3::applyBackdrop(&window, WinUI3::Backdrop::None));
    QVERIFY(!window.property("_winui_backdrop").isValid());
    QCOMPARE(window.palette(), originalPalette);
#endif
}

void WinUI3SurfacesTest::backdropButtonRepaintDoesNotAccumulate()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget window;
    window.setProperty("_winui_backdrop", 1);
    QPushButton button(QStringLiteral("Mica"), &window);
    button.resize(96, 32);

    QStyleOptionButton option;
    option.initFrom(&button);
    option.rect = button.rect();
    const auto paintFrame = [&](QImage &image, QStyle::State state) {
        option.state = state;
        QPainter painter(&image);
        style->drawPrimitive(QStyle::PE_PanelButtonCommand, &option, &painter, &button);
    };
    QImage normal(button.size(), QImage::Format_ARGB32_Premultiplied);
    normal.fill(Qt::transparent);
    paintFrame(normal, QStyle::State_Enabled);

    QImage hoverThenNormal(button.size(), QImage::Format_ARGB32_Premultiplied);
    hoverThenNormal.fill(Qt::transparent);
    setFrame(&button, "_winui_hover_progress", 1.0);
    paintFrame(hoverThenNormal, QStyle::State_Enabled | QStyle::State_MouseOver);
    setFrame(&button, "_winui_hover_progress", 0.0);
    paintFrame(hoverThenNormal, QStyle::State_Enabled);
    QCOMPARE(hoverThenNormal, normal);

    QWidget opaqueLayer(&window);
    opaqueLayer.setProperty(WinUI3::Style::SurfaceProperty, QStringLiteral("content"));
    QPushButton layeredButton(QStringLiteral("Layered"), &opaqueLayer);
    QVERIFY(!WinUI3::Private::paintsDirectlyOnBackdrop(&layeredButton));
}

void WinUI3SurfacesTest::backdropComboRepaintDoesNotAccumulate()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget window;
    window.setProperty("_winui_backdrop", 1);
    QComboBox combo(&window);
    combo.addItem(QStringLiteral("Theme"));
    combo.resize(160, 32);

    QStyleOptionComboBox option;
    option.initFrom(&combo);
    option.rect = combo.rect();
    option.subControls = QStyle::SC_ComboBoxFrame | QStyle::SC_ComboBoxArrow;
    const auto paintFrame = [&](QImage &image, QStyle::State state) {
        option.state = state;
        QPainter painter(&image);
        style->drawComplexControl(QStyle::CC_ComboBox, &option, &painter, &combo);
    };
    QImage normal(combo.size(), QImage::Format_ARGB32_Premultiplied);
    normal.fill(Qt::transparent);
    setFrame(&combo, "_winui_hover_progress", 0.0);
    paintFrame(normal, QStyle::State_Enabled);

    QImage hoverThenNormal(combo.size(), QImage::Format_ARGB32_Premultiplied);
    hoverThenNormal.fill(Qt::transparent);
    setFrame(&combo, "_winui_hover_progress", 1.0);
    paintFrame(hoverThenNormal, QStyle::State_Enabled | QStyle::State_MouseOver);
    setFrame(&combo, "_winui_hover_progress", 0.0);
    paintFrame(hoverThenNormal, QStyle::State_Enabled);
    QCOMPARE(hoverThenNormal, normal);
}

void WinUI3SurfacesTest::contentDialogContract()
{
    QDialog dialog;
    WinUI3::Style::setContentDialog(&dialog);
    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(new QLabel(QStringLiteral("Dialog content")));
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);
    if (auto *primary = buttons->button(QDialogButtonBox::Ok))
        primary->setDefault(true);
    dialog.show();
    QTRY_VERIFY(dialog.isVisible());
    QVERIFY(dialog.minimumWidth() >= 320);
    QVERIFY(dialog.minimumHeight() >= 184);
    QCOMPARE(layout->contentsMargins(), QMargins(24, 24, 24, 24));
    QCOMPARE(layout->spacing(), 12);
    QCOMPARE(WinUI3::Style::controlRole(buttons->button(QDialogButtonBox::Ok)),
             WinUI3::ControlRole::Accent);
    QVERIFY(!buttons->autoFillBackground());
    QCOMPARE(dialog.palette().color(QPalette::Window), buttons->palette().color(QPalette::Window));
    QTRY_VERIFY(dialog.findChild<QWidget *>(QStringLiteral("_winui_content_dialog_footer_surface"),
                                            Qt::FindDirectChildrenOnly));
    QWidget *footer = dialog.findChild<QWidget *>(
            QStringLiteral("_winui_content_dialog_footer_surface"), Qt::FindDirectChildrenOnly);
    QTRY_COMPARE(footer->width(), dialog.width());
    QCOMPARE(footer->x(), 0);
    QCOMPARE(footer->geometry().bottom(), dialog.rect().bottom());
    QVERIFY(footer->height() > buttons->height());
    const int buttonsTop = buttons->mapTo(&dialog, QPoint(0, 0)).y();
    const int buttonsBottom = buttonsTop + buttons->height() - 1;
    const int upperInset = buttonsTop - footer->geometry().top();
    const int lowerInset = footer->geometry().bottom() - buttonsBottom;
    QVERIFY(qAbs(upperInset - lowerInset) <= 1);
    const QImage renderedDialog = dialog.grab().toImage();
    const QColor contentPixel = renderedDialog.pixelColor(5, 5);
    const QColor footerPixel = renderedDialog.pixelColor(5, renderedDialog.height() - 5);
    QVERIFY(colorDistance(contentPixel, footerPixel) > 4);
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setThemeMode(WinUI3::ThemeMode::Dark);
    QTRY_COMPARE(dialog.palette().color(QPalette::Window), QColor(0x2C, 0x2C, 0x2C));
    const QImage darkDialog = dialog.grab().toImage();
    QCOMPARE(darkDialog.pixelColor(5, 5), QColor(0x2C, 0x2C, 0x2C));
    QCOMPARE(darkDialog.pixelColor(5, darkDialog.height() - 5), QColor(0x20, 0x20, 0x20));
    QVERIFY(!dialog.style()->standardIcon(QStyle::SP_MessageBoxInformation).isNull());
    QVERIFY(!dialog.style()->standardIcon(QStyle::SP_MessageBoxWarning).isNull());
    QVERIFY(!dialog.style()->standardIcon(QStyle::SP_MessageBoxCritical).isNull());
    QVERIFY(!dialog.style()->standardIcon(QStyle::SP_MessageBoxQuestion).isNull());
}

void WinUI3SurfacesTest::messageBoxContentDialogContract()
{
    QMessageBox dialog(QMessageBox::Information, QStringLiteral("WinUI 3 Style"),
                       QStringLiteral("This is a native Qt message box rendered by the style."),
                       QMessageBox::Ok);
    dialog.show();
    QTRY_VERIFY(dialog.isVisible());
    QTRY_VERIFY(dialog.height() >= 184);
    QCOMPARE(dialog.layout()->contentsMargins(), QMargins(24, 24, 24, 24));
    QCOMPARE(dialog.layout()->sizeConstraint(), QLayout::SetMinimumSize);

    auto *buttons = dialog.findChild<QDialogButtonBox *>();
    QVERIFY(buttons);
    QWidget *footer = dialog.findChild<QWidget *>(
            QStringLiteral("_winui_content_dialog_footer_surface"), Qt::FindDirectChildrenOnly);
    QVERIFY(footer);
    QTRY_COMPARE(footer->width(), dialog.width());
    const int buttonsTop = buttons->mapTo(&dialog, QPoint(0, 0)).y();
    const int buttonsBottom = buttonsTop + buttons->height() - 1;
    QCOMPARE(buttonsTop - footer->geometry().top(), footer->geometry().bottom() - buttonsBottom);

    auto *label = dialog.findChild<QLabel *>(QStringLiteral("qt_msgbox_label"));
    QVERIFY(label);
    QCOMPARE(label->focusPolicy(), Qt::NoFocus);
    QVERIFY(dialog.button(QMessageBox::Ok)->hasFocus());
}

void WinUI3SurfacesTest::wizardSurfaceContract()
{
    QWizard wizard;
    wizard.resize(520, 340);
    auto *first = new QWizardPage;
    first->setTitle(QStringLiteral("Welcome"));
    first->setSubTitle(QStringLiteral("A standard Qt wizard page."));
    auto *firstLayout = new QVBoxLayout(first);
    firstLayout->addWidget(new QLabel(QStringLiteral("Wizard content")));
    wizard.addPage(first);
    auto *second = new QWizardPage;
    second->setTitle(QStringLiteral("Finish"));
    wizard.addPage(second);
    wizard.show();
    QVERIFY(QTest::qWaitForWindowExposed(&wizard));

    QVERIFY(wizard.autoFillBackground());
    QVERIFY(first->autoFillBackground());
    QCOMPARE(WinUI3::Style::controlRole(wizard.button(QWizard::NextButton)),
             WinUI3::ControlRole::Accent);
    QCOMPARE(WinUI3::Style::controlRole(wizard.button(QWizard::FinishButton)),
             WinUI3::ControlRole::Accent);

    const QImage image = wizard.grab().toImage();
    const QColor content = first->palette().color(QPalette::Window);
    const QColor commands = wizard.palette().color(QPalette::Window);
    const auto containsColor = [](const QImage &source, const QColor &target) {
        for (int y = 0; y < source.height(); ++y)
            for (int x = 0; x < source.width(); ++x)
                if (colorDistance(source.pixelColor(x, y), target) <= 8)
                    return true;
        return false;
    };
    QVERIFY(containsColor(image, content));
    QVERIFY(containsColor(image, commands));

    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setThemeMode(WinUI3::ThemeMode::Dark);
    QWizard darkWizard;
    darkWizard.resize(520, 340);
    auto *darkPage = new QWizardPage;
    darkPage->setTitle(QStringLiteral("Welcome"));
    darkPage->setSubTitle(QStringLiteral("A standard Qt wizard page."));
    auto *darkLayout = new QVBoxLayout(darkPage);
    darkLayout->addWidget(new QLabel(QStringLiteral("Wizard content")));
    darkWizard.addPage(darkPage);
    darkWizard.show();
    QVERIFY(QTest::qWaitForWindowExposed(&darkWizard));
    QCOMPARE(darkWizard.palette().color(QPalette::Window), QColor(44, 44, 44));
    QCOMPARE(darkPage->palette().color(QPalette::Window), QColor(44, 44, 44));
    QCOMPARE(darkWizard.button(QWizard::FinishButton)->palette().color(QPalette::Window),
             QColor(32, 32, 32));
    QCOMPARE(WinUI3::Style::controlRole(darkWizard.button(QWizard::FinishButton)),
             WinUI3::ControlRole::Accent);
    auto *darkFooter = darkWizard.findChild<QWidget *>(
            QStringLiteral("_winui_wizard_footer_surface"), Qt::FindDirectChildrenOnly);
    QVERIFY(darkFooter);
    QTRY_VERIFY(darkFooter->isVisible());
    const QImage darkImage = darkWizard.grab().toImage();
    const QPoint darkBodyPixel = darkPage->mapTo(&darkWizard, QPoint(2, darkPage->height() / 2));
    QVERIFY(darkWizard.rect().contains(darkBodyPixel));
    QCOMPARE(darkImage.pixelColor(darkBodyPixel), QColor(44, 44, 44));
    QCOMPARE(darkImage.pixelColor(darkWizard.width() / 2, darkFooter->geometry().top() + 2),
             QColor(32, 32, 32));

    // Gallery path: the app starts in System mode and the combo sets the
    // raw index (0=System, 1=Light, 2=Dark). The style must answer
    // ModernStyle even before any wizard exists (live defect: blue
    // Classic header + white footer; QWizardPrivate caches wizStyle at
    // construction from the app style).
    style->setThemeMode(WinUI3::ThemeMode::System);
    QCOMPARE(qApp->style()->styleHint(QStyle::SH_WizardStyle), int(QWizard::ModernStyle));

    style->setThemeMode(WinUI3::ThemeMode::Light);
    QWizard runtimeWizard;
    runtimeWizard.resize(520, 340);
    auto *runtimePage = new QWizardPage;
    runtimePage->setTitle(QStringLiteral("Runtime theme"));
    runtimeWizard.addPage(runtimePage);
    runtimeWizard.show();
    QVERIFY(QTest::qWaitForWindowExposed(&runtimeWizard));
    QCOMPARE(runtimePage->palette().color(QPalette::Window), QColor(252, 252, 252));
    style->setThemeMode(WinUI3::ThemeMode::Dark);
    QTRY_COMPARE(runtimePage->palette().color(QPalette::Window), QColor(44, 44, 44));
    QTRY_COMPARE(runtimeWizard.button(QWizard::NextButton)->palette().color(QPalette::Window),
                 QColor(32, 32, 32));
    auto *runtimeFooter = runtimeWizard.findChild<QWidget *>(
            QStringLiteral("_winui_wizard_footer_surface"), Qt::FindDirectChildrenOnly);
    QVERIFY(runtimeFooter);
    QTRY_VERIFY(runtimeFooter->isVisible());
}

void WinUI3SurfacesTest::wizardUsesModernStyleHint()
{
    // Live defect 2026-09-09: ClassicStyle draws a blue banner pixmap, a
    // hardcoded #003399 title and a white page; AeroStyle forces its own
    // white fills. ModernStyle takes the style palette in both themes.
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWizard wizard;
    QCOMPARE(style->styleHint(QStyle::SH_WizardStyle, nullptr, &wizard),
             int(QWizard::ModernStyle));
    QCOMPARE(wizard.wizardStyle(), QWizard::ModernStyle);
}

void WinUI3SurfacesTest::wizardOpenThemeSwitchLifecycle()
{
    // Live defect 2026-09-09: an open wizard kept a stale command-area
    // fill across Light/Dark switches (white footer in Dark). Switching
    // the theme with the wizard open must re-resolve the footer fill,
    // the page palette and the command-button palette, with no stale
    // theme pixels left behind.
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setThemeMode(WinUI3::ThemeMode::Dark);
    QWizard wizard;
    wizard.resize(520, 340);
    auto *page = new QWizardPage;
    page->setTitle(QStringLiteral("Welcome"));
    wizard.addPage(page);
    wizard.show();
    QVERIFY(QTest::qWaitForWindowExposed(&wizard));
    auto *footer = wizard.findChild<QWidget *>(QStringLiteral("_winui_wizard_footer_surface"),
                                               Qt::FindDirectChildrenOnly);
    QVERIFY(footer);
    QTRY_VERIFY(footer->isVisible());
    QAbstractButton *next = wizard.button(QWizard::NextButton);
    QVERIFY(next);
    // QWizard's internal title/description labels must inherit the content
    // ink: a stale blue Link-role title on a dark page is unreadable (live
    // defect 2026-09-09).
    const auto titleLabels = wizard.findChildren<QLabel *>();
    QVERIFY(!titleLabels.isEmpty());
    const QColor darkContent(44, 44, 44);
    const QColor darkCommand(32, 32, 32);
    const QColor lightContent(252, 252, 252);
    const QColor lightCommand(243, 243, 243);
    QTRY_COMPARE(page->palette().color(QPalette::Window), darkContent);
    QTRY_COMPARE(next->palette().color(QPalette::Window), darkCommand);
    for (QLabel *label : titleLabels)
        QTRY_COMPARE(label->palette().color(label->foregroundRole()), QColor(255, 255, 255));
    QCOMPARE(wizard.grab().toImage().pixelColor(
                     wizard.width() / 2, footer->geometry().top() + 2),
             darkCommand);
    style->setThemeMode(WinUI3::ThemeMode::Light);
    QTRY_COMPARE(page->palette().color(QPalette::Window), lightContent);
    QTRY_COMPARE(next->palette().color(QPalette::Window), lightCommand);
    for (QLabel *label : titleLabels)
        QTRY_COMPARE(label->palette().color(label->foregroundRole()), QColor(0, 0, 0, 228));
    style->setThemeMode(WinUI3::ThemeMode::Dark);
    QTRY_COMPARE(page->palette().color(QPalette::Window), darkContent);
    QTRY_COMPARE(next->palette().color(QPalette::Window), darkCommand);
    for (QLabel *label : titleLabels)
        QTRY_COMPARE(label->palette().color(label->foregroundRole()), QColor(255, 255, 255));
    QTRY_COMPARE(wizard.grab().toImage().pixelColor(
                         wizard.width() / 2, footer->geometry().top() + 2),
                 darkCommand);
}


void WinUI3SurfacesTest::contentDialogScrimLifecycle()
{
    QWidget parent;
    parent.resize(640, 480);
    QDialog dialog(&parent);
    WinUI3::Style::setContentDialog(&dialog);
    parent.show();
    dialog.show();
    QTRY_VERIFY(dialog.isVisible());
    const auto scrims =
            parent.findChildren<QWidget *>(QStringLiteral("_winui_content_dialog_scrim"));
    QCOMPARE(scrims.size(), 1);
    QVERIFY(scrims.first()->isVisible());
    QCOMPARE(scrims.first()->geometry(), parent.rect());
    parent.resize(700, 500);
    qApp->processEvents();
    QCOMPARE(scrims.first()->geometry(), parent.rect());
    dialog.hide();
    qApp->processEvents();
    qApp->sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(parent.findChildren<QWidget *>(QStringLiteral("_winui_content_dialog_scrim"))
                    .isEmpty());
}

void WinUI3SurfacesTest::progressAnimationAndOrientations()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QProgressBar bar;
    bar.setRange(0, 0);
    bar.resize(240, 24);
    bar.show();
    QImage first = bar.grab().toImage();
    QTest::qWait(40);
    QImage second = bar.grab().toImage();
    QVERIFY(first != second);
    auto *timer = style->findChild<QTimer *>(QStringLiteral("_winui_progress_timer"),
                                             Qt::FindDirectChildrenOnly);
    QVERIFY(timer);
    QVERIFY(timer->isActive());
    QVERIFY(!bar.findChild<QTimer *>(QStringLiteral("_winui_progress_timer"),
                                     Qt::FindDirectChildrenOnly));

    bar.hide();
    QTRY_VERIFY(!timer->isActive());
    bar.show();
    QTRY_VERIFY(timer->isActive());

    bar.setRange(0, 100);
    bar.setValue(40);
    bar.grab();
    QCoreApplication::processEvents();
    QVERIFY(!timer->isActive());
    bar.setRange(0, 0);
    bar.grab();
    QCoreApplication::processEvents();
    QVERIFY(timer->isActive());

    bar.setOrientation(Qt::Vertical);
    bar.resize(24, 180);
    bar.setInvertedAppearance(true);
    bar.setEnabled(false);
    QVERIFY(!bar.grab().isNull());

    qputenv("WINUI3STYLE_DISABLE_ANIMATIONS", "1");
    QTRY_VERIFY(!timer->isActive());
    const QImage frozenFirst = bar.grab().toImage();
    QTest::qWait(40);
    const QImage frozenSecond = bar.grab().toImage();
    QCOMPARE(frozenFirst, frozenSecond);
    qunsetenv("WINUI3STYLE_DISABLE_ANIMATIONS");

    style->unpolish(&bar);
    QVERIFY(!timer->isActive());
}

void WinUI3SurfacesTest::progressTextAndDisabledPaletteContract()
{
    QProgressBar bar;
    bar.setRange(0, 100);
    bar.setValue(42);
    bar.setFormat(QStringLiteral("Transferred %p%"));

    bar.setTextVisible(false);
    const QSize bareHint = bar.sizeHint();
    bar.setTextVisible(true);
    const QSize textHint = bar.sizeHint();
    QVERIFY(textHint.height() >= bareHint.height() + 6);

    QPalette palette = bar.palette();
    const QColor activeText(210, 20, 10);
    const QColor disabledText(10, 20, 210);
    palette.setColor(QPalette::Active, QPalette::WindowText, activeText);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, disabledText);
    bar.setPalette(palette);
    bar.setEnabled(false);

    QStyleOptionProgressBar option;
    option.initFrom(&bar);
    option.rect = QRect(QPoint(0, 0), QSize(220, textHint.height()));
    option.minimum = bar.minimum();
    option.maximum = bar.maximum();
    option.progress = bar.value();
    option.text = bar.text();
    option.textVisible = true;

    QImage image(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    bar.style()->drawControl(QStyle::CE_ProgressBarLabel, &option, &painter, &bar);
    painter.end();

    bool foundDisabledText = false;
    bool foundActiveText = false;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor pixel = image.pixelColor(x, y);
            if (pixel.alpha() == 0)
                continue;
            foundDisabledText |= pixel.blue() > pixel.red() + 40;
            foundActiveText |= pixel.red() > pixel.blue() + 40;
        }
    }
    QVERIFY(foundDisabledText);
    QVERIFY(!foundActiveText);
}

void WinUI3SurfacesTest::progressTimerScalingAndLifecycle()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget host;
    host.resize(420, 420);
    QList<QProgressBar *> bars;
    for (int i = 0; i < 100; ++i) {
        auto *bar = new QProgressBar(&host);
        bar->setRange(0, 0);
        bar->setGeometry((i % 10) * 42, (i / 10) * 42, 40, 24);
        bars.append(bar);
    }
    host.show();
    QCoreApplication::processEvents();

    const auto timers = style->findChildren<QTimer *>(QStringLiteral("_winui_progress_timer"),
                                                      Qt::FindDirectChildrenOnly);
    QCOMPARE(timers.size(), 1);
    auto *timer = timers.constFirst();
    QVERIFY(timer->isActive());
    for (QProgressBar *bar : bars) {
        QVERIFY(!bar->findChild<QTimer *>(QStringLiteral("_winui_progress_timer"),
                                          Qt::FindDirectChildrenOnly));
    }

    host.hide();
    QCoreApplication::processEvents();
    QVERIFY(!timer->isActive());
    host.show();
    QCoreApplication::processEvents();
    QVERIFY(timer->isActive());

    delete bars.takeLast();
    QVERIFY(timer->isActive());
    qDeleteAll(bars);
    bars.clear();
    QCoreApplication::processEvents();
    QVERIFY(!timer->isActive());
}

QTEST_MAIN(WinUI3SurfacesTest)
#include "tst_winui3surfaces.moc"
