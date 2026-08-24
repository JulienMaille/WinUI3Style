#include <winui3style/winui3style.h>
#include <winui3style/winui3backdrop.h>

#include <QComboBox>
#include <QAbstractItemView>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QElapsedTimer>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollBar>
#include <QSignalSpy>
#include <QStyleOptionSlider>
#include <QTest>
#include <QToolTip>
#include <QVBoxLayout>

class NativePopupProbe final : public QObject
{
public:
    QComboBox *combo = nullptr;
    bool visible = false;
    bool painted = false;
    int movesAfterPaint = 0;
    int resizesAfterPaint = 0;
    QRect firstPaintGeometry;
    QPoint selectedCenterAtFirstPaint;
    int scrollAtFirstPaint = -1;

    bool eventFilter(QObject *object, QEvent *event) override
    {
        auto *popup = qobject_cast<QWidget *>(object);
        if (event->type() == QEvent::Show)
            visible = true;
        else if (event->type() == QEvent::Hide)
            visible = false;
        else if (visible && event->type() == QEvent::Paint && !painted
                 && combo && popup) {
            painted = true;
            firstPaintGeometry = popup->geometry();
            const QModelIndex selected = combo->model()->index(
                combo->currentIndex(), combo->modelColumn(), combo->rootModelIndex());
            selectedCenterAtFirstPaint = combo->view()->viewport()->mapToGlobal(
                combo->view()->visualRect(selected).center());
            scrollAtFirstPaint = combo->view()->verticalScrollBar()->value();
        } else if (painted && visible && event->type() == QEvent::Move)
            ++movesAfterPaint;
        else if (painted && visible && event->type() == QEvent::Resize)
            ++resizesAfterPaint;
        return false;
    }
};

class WinUI3StyleNativeTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();
    void tooltipSurface();
    void menuSurface();
    void comboPopupSurface();
    void backdropStateRestoration();
    void dialogThemeUpdate();
    void dockFloatingFocusCleanup();
    void scrollBarHoverTiming();
};

void WinUI3StyleNativeTest::initTestCase()
{
    qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light));
}

void WinUI3StyleNativeTest::cleanup()
{
    for (QWidget *widget : qApp->topLevelWidgets()) {
        if (widget->windowType() == Qt::Popup
            || widget->windowType() == Qt::ToolTip
            || qobject_cast<QDialog *>(widget)) {
            widget->close();
            widget->hide();
        }
    }
    if (QWidget *focus = qApp->focusWidget())
        focus->clearFocus();
    qApp->processEvents();
}

void WinUI3StyleNativeTest::tooltipSurface()
{
    QWidget window;
    window.setWindowTitle(QStringLiteral("Native tooltip"));
    auto *button = new QPushButton(QStringLiteral("Hover target"), &window);
    button->setToolTip(QStringLiteral("WinUI tooltip surface"));
    window.resize(300, 100);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QTest::mouseMove(button, button->rect().center());
    // Keep the test meaningful on desktop policies that disable delayed
    // tooltip tracking while still exercising the real Qt tooltip window.
    QToolTip::showText(button->mapToGlobal(button->rect().center()),
                       button->toolTip(), button);
    QTRY_VERIFY_WITH_TIMEOUT([&] {
        for (QWidget *candidate : qApp->topLevelWidgets()) {
            if (candidate->windowType() == Qt::ToolTip && candidate->isVisible())
                return true;
        }
        return false;
    }(), 1800);
}

void WinUI3StyleNativeTest::menuSurface()
{
    QWidget window;
    window.resize(360, 120);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QMenu menu;
    QAction *action = menu.addAction(QStringLiteral("Native menu action\tCtrl+N"));
    action->setCheckable(true);
    menu.addSeparator();
    QMenu *submenu = menu.addMenu(QStringLiteral("More options"));
    submenu->addAction(QStringLiteral("Child"));
    menu.popup(window.mapToGlobal(QPoint(20, 20)));
    QTRY_VERIFY(menu.isVisible());
    const QRect actionRect = menu.actionGeometry(action);
    QTest::mouseMove(&menu, actionRect.center());
    QVERIFY(actionRect.isValid());
    QTest::qWait(50);
    QTest::mouseClick(&menu, Qt::LeftButton, Qt::NoModifier, actionRect.center());
    QVERIFY(action->isChecked());
}

void WinUI3StyleNativeTest::comboPopupSurface()
{
    QWidget window;
    auto *combo = new QComboBox(&window);
    combo->addItems({QStringLiteral("First"), QStringLiteral("Second"),
                     QStringLiteral("Third")});
    combo->setCurrentIndex(1);
    combo->move(20, 20);
    window.resize(320, 120);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    NativePopupProbe probe;
    probe.combo = combo;
    QWidget *popup = combo->view()->window();
    popup->installEventFilter(&probe);
    QSignalSpy scrollChanges(combo->view()->verticalScrollBar(),
                             &QScrollBar::valueChanged);
    combo->showPopup();
    QTRY_VERIFY(combo->view()->isVisible());
    QVERIFY(popup->isVisible());
    QTRY_VERIFY(probe.painted);
    const QModelIndex selected = combo->model()->index(1, 0);
    QVERIFY(combo->view()->visualRect(selected).isValid());
    const QPoint comboCenter = combo->mapToGlobal(combo->rect().center());
    QVERIFY(qAbs(probe.selectedCenterAtFirstPaint.y() - comboCenter.y()) <= 4);
    QCOMPARE(combo->view()->verticalScrollBar()->value(), probe.scrollAtFirstPaint);
    QCOMPARE(scrollChanges.count(), 0);
    QTest::qWait(60);
    QCOMPARE(popup->geometry(), probe.firstPaintGeometry);
    QCOMPARE(probe.movesAfterPaint, 0);
    QCOMPARE(probe.resizesAfterPaint, 0);
    QTest::keyClick(combo->view(), Qt::Key_Escape);
    QTRY_VERIFY(!combo->view()->isVisible());
}

void WinUI3StyleNativeTest::backdropStateRestoration()
{
    QWidget window;
    QPalette explicitPalette = window.palette();
    explicitPalette.setColor(QPalette::Window, QColor(31, 73, 119));
    window.setPalette(explicitPalette);
    window.setAutoFillBackground(false);
    window.setAttribute(Qt::WA_TranslucentBackground, false);
    window.setAttribute(Qt::WA_NoSystemBackground, false);
    window.setAttribute(Qt::WA_OpaquePaintEvent, true);

    WinUI3::applyBackdrop(&window, WinUI3::Backdrop::Mica);
    QVERIFY(window.property("_winui_backdrop").isValid());
    WinUI3::applyBackdrop(&window, WinUI3::Backdrop::None);
    QVERIFY(!window.property("_winui_backdrop").isValid());
    QCOMPARE(window.palette(), explicitPalette);
    QVERIFY(window.testAttribute(Qt::WA_SetPalette));
    QVERIFY(!window.autoFillBackground());
    QVERIFY(!window.testAttribute(Qt::WA_TranslucentBackground));
    QVERIFY(!window.testAttribute(Qt::WA_NoSystemBackground));
    QVERIFY(window.testAttribute(Qt::WA_OpaquePaintEvent));

    QWidget inheritedPaletteWindow;
    inheritedPaletteWindow.setAutoFillBackground(false);
    QVERIFY(!inheritedPaletteWindow.testAttribute(Qt::WA_SetPalette));
    WinUI3::applyBackdrop(&inheritedPaletteWindow, WinUI3::Backdrop::Acrylic);
    WinUI3::applyBackdrop(&inheritedPaletteWindow, WinUI3::Backdrop::None);
    QVERIFY(!inheritedPaletteWindow.testAttribute(Qt::WA_SetPalette));
    QVERIFY(!inheritedPaletteWindow.autoFillBackground());

    // Disabling a backdrop that was never installed must also be a no-op on
    // application-owned widget state.
    inheritedPaletteWindow.setAttribute(Qt::WA_OpaquePaintEvent, true);
    WinUI3::applyBackdrop(&inheritedPaletteWindow, WinUI3::Backdrop::None);
    QVERIFY(!inheritedPaletteWindow.testAttribute(Qt::WA_SetPalette));
    QVERIFY(!inheritedPaletteWindow.autoFillBackground());
    QVERIFY(inheritedPaletteWindow.testAttribute(Qt::WA_OpaquePaintEvent));
}

void WinUI3StyleNativeTest::dialogThemeUpdate()
{
    QDialog dialog;
    WinUI3::Style::setContentDialog(&dialog);
    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(new QLabel(QStringLiteral("Native content dialog")));
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, &dialog);
    layout->addWidget(buttons);
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setThemeMode(WinUI3::ThemeMode::Dark);
    QTRY_VERIFY(dialog.palette().color(QPalette::Window).lightness() < 128);
    style->setAccentColor(QColor(220, 40, 80));
    QCOMPARE(style->standardPalette().color(QPalette::Highlight), QColor(220, 40, 80));
    style->setThemeMode(WinUI3::ThemeMode::Light);
    QTRY_VERIFY(dialog.palette().color(QPalette::Window).lightness() > 128);
}

void WinUI3StyleNativeTest::dockFloatingFocusCleanup()
{
    QMainWindow window;
    auto *dock = new QDockWidget(QStringLiteral("Inspector"), &window);
    dock->setWidget(new QLabel(QStringLiteral("Dock content")));
    dock->setFeatures(QDockWidget::DockWidgetClosable
                      | QDockWidget::DockWidgetFloatable
                      | QDockWidget::DockWidgetMovable);
    window.addDockWidget(Qt::RightDockWidgetArea, dock);
    window.resize(640, 360);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    dock->setFloating(true);
    QTRY_VERIFY(dock->isFloating());
    dock->activateWindow();
    dock->widget()->setFocusPolicy(Qt::StrongFocus);
    dock->widget()->setFocus(Qt::TabFocusReason);
    if (!dock->widget()->hasFocus() && !dock->isActiveWindow())
        QSKIP("The current Windows session did not grant focus to the floating dock");
    dock->close();
    QVERIFY(!dock->isVisible());
    window.close();
}

void WinUI3StyleNativeTest::scrollBarHoverTiming()
{
    const bool hadAnimationOverride = qEnvironmentVariableIsSet(
        "WINUI3STYLE_DISABLE_ANIMATIONS");
    const QByteArray animationOverride =
        qgetenv("WINUI3STYLE_DISABLE_ANIMATIONS");
    qunsetenv("WINUI3STYLE_DISABLE_ANIMATIONS");

    QWidget host;
    host.resize(80, 300);
    QScrollBar bar(Qt::Vertical, &host);
    bar.setRange(0, 100);
    bar.setPageStep(20);
    bar.setValue(30);
    bar.setGeometry(0, 0, 12, 300);
    host.show();
    const bool exposed = QTest::qWaitForWindowExposed(&host);
    host.activateWindow();
    const bool active = QTest::qWaitForWindowActive(&host);
    Q_UNUSED(active);
    bar.setProperty("_winui_hover_progress", 0.0);

    QTest::mouseMove(&host, QPoint(60, 150));
    QTest::qWait(30);
    bar.setProperty("_winui_hover_progress", 0.0);

    QStyleOptionSlider option;
    option.initFrom(&bar);
    option.orientation = bar.orientation();
    option.minimum = bar.minimum();
    option.maximum = bar.maximum();
    option.sliderPosition = bar.sliderPosition();
    option.sliderValue = bar.value();
    option.pageStep = bar.pageStep();
    option.upsideDown = false;
    const QRect thumb = bar.style()->subControlRect(
        QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarSlider, &bar);

    QElapsedTimer timer;
    timer.start();
    QTest::mouseMove(&bar, bar.rect().center());
    // QTest's Windows cursor injection does not synthesize QEvent::Enter for
    // a 12 px child consistently on every desktop session. Keep the native
    // QScrollBar and QTest path, then deliver the same Qt enter event that
    // QWidget receives when the platform does synthesize it.
    QEnterEvent enter(bar.rect().center(), bar.rect().center(),
                      bar.mapToGlobal(bar.rect().center()));
    QCoreApplication::sendEvent(&bar, &enter);
    QTest::qWait(350);
    const bool stayedCollapsed =
        bar.property("_winui_hover_progress").toReal() <= 0.01;

    int revealAt = -1;
    while (timer.elapsed() < 650) {
        if (bar.property("_winui_hover_progress").toReal() > 0.01) {
            revealAt = timer.elapsed();
            break;
        }
        QTest::qWait(5);
    }
    int settledAt = -1;
    while (timer.elapsed() < 850) {
        if (bar.property("_winui_hover_progress").toReal() > 0.99) {
            settledAt = timer.elapsed();
            break;
        }
        QTest::qWait(5);
    }

    if (hadAnimationOverride)
        qputenv("WINUI3STYLE_DISABLE_ANIMATIONS", animationOverride);
    else
        qunsetenv("WINUI3STYLE_DISABLE_ANIMATIONS");

    QVERIFY(exposed);
    QCOMPARE(thumb.width(), 12);
    QVERIFY(thumb.height() >= 30);
    QVERIFY(stayedCollapsed);
    QVERIFY(revealAt >= 380);
    QVERIFY(revealAt <= 650);
    QVERIFY(settledAt >= 520);
    QVERIFY(settledAt <= 850);
}

QTEST_MAIN(WinUI3StyleNativeTest)
#include "tst_winui3style_native.moc"
