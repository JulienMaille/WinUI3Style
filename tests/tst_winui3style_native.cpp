#include <winui3style/winui3style.h>

#define WINUI3STYLE_NATIVE_CAPTURE
#include "winui3testhelpers.h"

#include <QComboBox>
#include <QAbstractItemView>
#include <QAbstractAnimation>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QElapsedTimer>
#include <QImage>
#include <QLabel>
#include <QMainWindow>
#include <QMenuBar>
#include <QMouseEvent>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>
#include <QScrollBar>
#include <QParallelAnimationGroup>
#include <QSignalSpy>
#include <QSlider>
#include <QStyleOptionSlider>
#include <QTest>
#include <QTimer>
#include <QToolTip>
#include <QVBoxLayout>
#include <QWindow>

#if defined(Q_OS_WIN)
#  define WIN32_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#  include <dwmapi.h>
#  pragma comment(lib, "dwmapi.lib")
#endif

#include <algorithm>

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
        QWidget *popup = combo ? combo->view()->window() : qobject_cast<QWidget *>(object);
        if (event->type() == QEvent::Show)
            visible = true;
        else if (event->type() == QEvent::Hide)
            visible = false;
        else if (visible && event->type() == QEvent::Paint && !painted && combo && popup
                 && popup->windowOpacity() > 0.0) {
            painted = true;
            firstPaintGeometry = popup->geometry();
            const QModelIndex selected = combo->model()->index(
                    combo->currentIndex(), combo->modelColumn(), combo->rootModelIndex());
            selectedCenterAtFirstPaint = combo->view()->viewport()->mapToGlobal(
                    combo->view()->visualRect(selected).center());
            scrollAtFirstPaint = combo->view()->verticalScrollBar()->value();
        } else if (object == popup && painted && visible && event->type() == QEvent::Move)
            ++movesAfterPaint;
        else if (object == popup && painted && visible && event->type() == QEvent::Resize)
            ++resizesAfterPaint;
        return false;
    }
};

class NativeScrollBarInputProbe final : public QObject
{
public:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        if (object != bar)
            return false;
        if (event->type() == QEvent::Enter)
            ++enters;
        else if (event->type() == QEvent::Leave)
            ++leaves;
        return false;
    }

    QScrollBar *bar = nullptr;
    int enters = 0;
    int leaves = 0;
};

class WinUI3StyleNativeTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();
    void tooltipSurface();
    void comboPopupSurface();
    void dialogThemeUpdate();
    void captionThemeAfterBackdropDisable_data();
    void captionThemeAfterBackdropDisable();
    void dockFloatingFocusCleanup();
    void scrollBarNativeInputDiagnostic();
    void sliderToolTipDebounceSurface();
};

void WinUI3StyleNativeTest::initTestCase()
{
    qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light));
}

void WinUI3StyleNativeTest::cleanup()
{
    for (QWidget *widget : qApp->topLevelWidgets()) {
        if (widget->windowType() == Qt::Popup || widget->windowType() == Qt::ToolTip
            || qobject_cast<QDialog *>(widget)) {
            widget->close();
            widget->hide();
        }
    }
    if (QWidget *focus = qApp->focusWidget())
        focus->clearFocus();
    qApp->processEvents();
    drainDeferredDeletion();
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
    QToolTip::showText(button->mapToGlobal(button->rect().center()), button->toolTip(), button);
    QTRY_VERIFY_WITH_TIMEOUT(
            [&] {
                for (QWidget *candidate : qApp->topLevelWidgets()) {
                    if (candidate->windowType() == Qt::ToolTip && candidate->isVisible())
                        return true;
                }
                return false;
            }(),
            1800);
}

void WinUI3StyleNativeTest::sliderToolTipDebounceSurface()
{
    QSlider slider(Qt::Horizontal);
    slider.setRange(0, 100);
    slider.resize(320, 40);
    slider.show();
    QVERIFY(QTest::qWaitForWindowExposed(&slider));

    QTest::mousePress(&slider, Qt::LeftButton, Qt::NoModifier, slider.rect().center());
    QTRY_VERIFY_WITH_TIMEOUT(WinUI3::Private::framePropertyRegistry()
                                     .value(&slider, "_winui_slider_tooltip_visible")
                                     .toBool(),
                             500);

    // The first tooltip display is immediate. Once visible, a burst of
    // pointer updates must use one trailing surface timer instead of moving
    // and repainting the native popup for every mouse event.
    for (int i = 0; i < 200; ++i) {
        slider.setValue(i % 100);
        QMouseEvent move(QEvent::MouseMove, QPointF(slider.rect().center()),
                         QPointF(slider.rect().center()), Qt::NoButton, Qt::LeftButton,
                         Qt::NoModifier);
        QCoreApplication::sendEvent(&slider, &move);
    }
    QCoreApplication::processEvents();
    auto *timer = slider.findChild<QTimer *>(QStringLiteral("_winui_slider_tooltip_debounce_timer"),
                                             Qt::FindDirectChildrenOnly);
    QVERIFY(timer);
    QVERIFY(timer->isSingleShot());
    QTRY_VERIFY_WITH_TIMEOUT(!timer->isActive(), 500);

    QSignalSpy callbacks(timer, &QTimer::timeout);
    for (int i = 0; i < 200; ++i) {
        slider.setValue((i + 37) % 100);
        QMouseEvent move(QEvent::MouseMove, QPointF(slider.rect().center()),
                         QPointF(slider.rect().center()), Qt::NoButton, Qt::LeftButton,
                         Qt::NoModifier);
        QCoreApplication::sendEvent(&slider, &move);
    }
    QCoreApplication::processEvents();
    QTRY_COMPARE_WITH_TIMEOUT(callbacks.count(), 1, 500);

    QTest::mouseRelease(&slider, Qt::LeftButton, Qt::NoModifier, slider.rect().center());
    QVERIFY(!timer->isActive());
    QVERIFY(!WinUI3::Private::framePropertyRegistry()
                     .value(&slider, "_winui_slider_tooltip_visible")
                     .toBool());

    // Disabling during the trailing debounce must clear both the inspectable
    // state and the already-created native popup, rather than leaving the last
    // value visible until another pointer event arrives.
    slider.setEnabled(true);
    QTest::mousePress(&slider, Qt::LeftButton, Qt::NoModifier, slider.rect().center());
    QTRY_VERIFY_WITH_TIMEOUT(WinUI3::Private::framePropertyRegistry()
                                     .value(&slider, "_winui_slider_tooltip_visible")
                                     .toBool(),
                             500);
    QMouseEvent move(QEvent::MouseMove, QPointF(slider.rect().center()),
                     QPointF(slider.rect().center()), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&slider, &move);
    QCoreApplication::processEvents();
    QVERIFY(timer->isActive());
    slider.setEnabled(false);
    QTRY_VERIFY_WITH_TIMEOUT(!timer->isActive(), 500);
    QVERIFY(!WinUI3::Private::framePropertyRegistry()
                     .value(&slider, "_winui_slider_tooltip_visible")
                     .toBool());
    QVERIFY(!WinUI3::Private::framePropertyRegistry()
                     .value(&slider, "_winui_slider_tooltip_value")
                     .isValid());
    if (auto *tip = slider.findChild<QWidget *>(QStringLiteral("_winui_slider_value_tip"),
                                                Qt::FindDirectChildrenOnly)) {
        QVERIFY(!tip->isVisible());
    }
}

void WinUI3StyleNativeTest::comboPopupSurface()
{
    QWidget window;
    auto *combo = new QComboBox(&window);
    combo->addItems({ QStringLiteral("First"), QStringLiteral("Second"), QStringLiteral("Third") });
    combo->setCurrentIndex(1);
    combo->move(20, 20);
    window.resize(320, 120);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    NativePopupProbe probe;
    probe.combo = combo;
    QWidget *popup = combo->view()->window();
    popup->installEventFilter(&probe);
    combo->view()->viewport()->installEventFilter(&probe);
    QSignalSpy scrollChanges(combo->view()->verticalScrollBar(), &QScrollBar::valueChanged);
    combo->showPopup();
    QTRY_VERIFY(combo->view()->isVisible());
    QVERIFY(popup->isVisible());
    const int popupWindowAlpha = popup->palette().color(QPalette::Window).alpha();
    if (popup->testAttribute(Qt::WA_TranslucentBackground)) {
        QCOMPARE(popupWindowAlpha, 242);
        QCOMPARE(combo->view()->viewport()->palette().color(QPalette::Base).alpha(), 242);
        QVERIFY(!popup->autoFillBackground());
        QVERIFY(!combo->view()->viewport()->autoFillBackground());
    } else {
        QCOMPARE(popupWindowAlpha, 255);
        QCOMPARE(combo->view()->viewport()->palette().color(QPalette::Base).alpha(), 255);
        QVERIFY(popup->autoFillBackground());
        QVERIFY(combo->view()->viewport()->autoFillBackground());
    }
    QTRY_VERIFY(probe.painted);
    const QModelIndex selected = combo->model()->index(1, 0);
    QVERIFY(combo->view()->visualRect(selected).isValid());
    const QPoint comboCenter = combo->mapToGlobal(combo->rect().center());
    QVERIFY(qAbs(probe.selectedCenterAtFirstPaint.y() - comboCenter.y()) <= 12);
    QCOMPARE(combo->view()->verticalScrollBar()->value(), probe.scrollAtFirstPaint);
    QCOMPARE(scrollChanges.count(), 0);
    const auto rowCenter = [&] {
        return combo->view()->viewport()->mapToGlobal(combo->view()->visualRect(selected).center());
    };
    QTRY_VERIFY_WITH_TIMEOUT(qAbs(rowCenter().y() - comboCenter.y()) <= 4, 1000);
    QVERIFY(settleMenuSweep(popup));
    const QPoint settledCenter = rowCenter();
    const QRect settledGeometry = popup->geometry();
    QCOMPARE(probe.selectedCenterAtFirstPaint.y() - settledCenter.y(),
             probe.firstPaintGeometry.y() - settledGeometry.y());
    probe.movesAfterPaint = 0;
    QTest::qWait(60);
    QCOMPARE(popup->geometry(), settledGeometry);
    QCOMPARE(probe.movesAfterPaint, 0);
    QCOMPARE(probe.resizesAfterPaint, 0);
    QTest::keyClick(combo->view(), Qt::Key_Escape);
    QTRY_VERIFY(!combo->view()->isVisible());
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

void WinUI3StyleNativeTest::captionThemeAfterBackdropDisable_data()
{
    QTest::addColumn<bool>("startDark");
    QTest::addColumn<bool>("changeWhileEnabled");
    QTest::newRow("dark-disable-light") << true << false;
    QTest::newRow("light-disable-dark") << false << false;
    QTest::newRow("dark-light-disable-dark") << true << true;
    QTest::newRow("light-dark-disable-light") << false << true;
}

void WinUI3StyleNativeTest::captionThemeAfterBackdropDisable()
{
    QFETCH(bool, startDark);
    QFETCH(bool, changeWhileEnabled);
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    const auto initial = startDark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light;
    const auto opposite = startDark ? WinUI3::ThemeMode::Light : WinUI3::ThemeMode::Dark;
    style->setThemeMode(initial);
    QMainWindow window;
    window.resize(400, 240);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    const auto attribute = [&window](int id) {
        DWORD value = 0;
        const HRESULT result =
                DwmGetWindowAttribute(reinterpret_cast<HWND>(window.windowHandle()->winId()),
                                      DWORD(id), &value, sizeof(value));
        if (FAILED(result))
            QTest::qFail(qPrintable(QStringLiteral("Cannot read DWM attribute %1: %2")
                                            .arg(id)
                                            .arg(quint32(result), 0, 16)),
                         __FILE__, __LINE__);
        return int(value);
    };
    window.setProperty(WinUI3::Style::BackdropProperty, QStringLiteral("mica"));
    QTRY_COMPARE(attribute(20), int(startDark));
    if (changeWhileEnabled) {
        style->setThemeMode(opposite);
        QTRY_COMPARE(attribute(20), int(!startDark));
    }
    window.setProperty(WinUI3::Style::BackdropProperty, QStringLiteral("none"));
    QCOMPARE(window.property("_winui_backdrop_effective").toInt(), 0);
    QVERIFY(!window.property("_winui_backdrop").isValid());
    // Caption/text colors are set-only DWM attributes (Get returns E_INVALIDARG).
    // Assert the native theme mechanism and sample the actual non-client fill.
    const auto caption = [&window] {
        const QImage frame = nativeWindowFrame(window.windowHandle()->winId());
        const int titleHeight = qRound((window.geometry().top() - window.frameGeometry().top())
                                       * window.devicePixelRatioF());
        return frame.isNull() ? QColor() : frame.pixelColor(frame.width() / 2, titleHeight / 2);
    };
    QCOMPARE(attribute(20), int(changeWhileEnabled ? !startDark : startDark));
    QTRY_COMPARE(caption(), style->standardPalette().color(QPalette::Window));
    // No material is requested now, but the native caption is still owned
    // by the style. It must continue following subsequent theme changes.
    style->setThemeMode(changeWhileEnabled ? initial : opposite);
    QTRY_COMPARE(attribute(20), int(changeWhileEnabled ? startDark : !startDark));
    QTRY_COMPARE(caption(), style->standardPalette().color(QPalette::Window));
}

void WinUI3StyleNativeTest::dockFloatingFocusCleanup()
{
    QMainWindow window;
    auto *dock = new QDockWidget(QStringLiteral("Inspector"), &window);
    dock->setWidget(new QLabel(QStringLiteral("Dock content")));
    dock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable
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

void WinUI3StyleNativeTest::scrollBarNativeInputDiagnostic()
{
    // This is an opt-in diagnostic only. The normal contracts use synthetic
    // Qt events; this test never injects QEnterEvent and never claims that
    // QTest reaches a physical Win32 cursor on every desktop session.
    if (qEnvironmentVariableIntValue("WINUI3STYLE_RUN_NATIVE_INPUT_DIAGNOSTIC") != 1)
        QSKIP("Native cursor diagnostic disabled; use synthetic contracts in the unit test");

    QWidget host;
    host.resize(80, 300);
    QScrollBar bar(Qt::Vertical, &host);
    bar.setRange(0, 100);
    bar.setPageStep(20);
    bar.setValue(30);
    bar.setGeometry(0, 0, 12, 300);
    host.show();
    QVERIFY(QTest::qWaitForWindowExposed(&host));
    NativeScrollBarInputProbe probe;
    probe.bar = &bar;
    bar.installEventFilter(&probe);

    QTest::mouseMove(&host, QPoint(60, 150));
    QCoreApplication::processEvents();
    QTest::mouseMove(&bar, bar.rect().center());
    QCoreApplication::processEvents();
    if (probe.enters == 0)
        QSKIP("The current Windows session did not deliver a native Enter event");
    QCOMPARE(probe.enters, 1);

    QTest::mouseMove(&host, QPoint(60, 150));
    QCoreApplication::processEvents();
    if (probe.leaves == 0)
        QSKIP("The current Windows session did not deliver a native Leave event");
    QCOMPARE(probe.leaves, 1);
}

QTEST_MAIN(WinUI3StyleNativeTest)
#include "tst_winui3style_native.moc"
