#include <winui3style/winui3style.h>

#define WINUI3STYLE_NATIVE_CAPTURE
#include "winui3testhelpers.h"
#include "winui3tokens_p.h"

#include <QComboBox>
#include <QAbstractItemView>
#include <QAbstractAnimation>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QElapsedTimer>
#include <QGroupBox>
#include <QImage>
#include <QLabel>
#include <QMainWindow>
#include <QMenuBar>
#include <QMouseEvent>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QStackedWidget>
#include <QParallelAnimationGroup>
#include <QSignalSpy>
#include <QSlider>
#include <QStyleOptionSlider>
#include <QStyleFactory>
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
    void tooltipLifecycleAndReuse();
    void comboPopupSurface();
    void dialogThemeUpdate();
    void captionThemeAfterBackdropDisable_data();
    void captionThemeAfterBackdropDisable();
    void backdropDisableRestoresOpaqueContent();
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

void WinUI3StyleNativeTest::tooltipLifecycleAndReuse()
{
    QWidget window;
    window.setWindowTitle(QStringLiteral("Native tooltip lifecycle"));
    auto *button = new QPushButton(QStringLiteral("Hover target"), &window);
    button->setToolTip(QStringLiteral("WinUI tooltip surface"));
    button->move(24, 24);
    window.resize(320, 160);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    const auto visibleTip = []() -> QLabel * {
        for (QWidget *candidate : qApp->topLevelWidgets()) {
            if (candidate->windowType() == Qt::ToolTip && candidate->isVisible())
                return qobject_cast<QLabel *>(candidate);
        }
        return nullptr;
    };

    // Hover delivery needs a real pointer over the window, which a test session
    // without a foreground desktop cannot guarantee, so the harness drives
    // QToolTip directly. From there the style owns everything the platform
    // hands it: window shape, layout and typography.
    const QPoint anchor = button->mapToGlobal(button->rect().center());
    QToolTip::showText(anchor, QStringLiteral("WinUI tooltip surface"), button);
    QTRY_VERIFY_WITH_TIMEOUT(visibleTip() != nullptr, 2000);
    QLabel *tip = visibleTip();
    const WId firstHandle = tip->winId();
    const QSize shortSize = tip->size();
    QVERIFY(tip->height() >= 30);
    QCOMPARE(tip->contentsMargins(), WinUI3::Private::tooltipContentMargins());
    QCOMPARE(tip->font().pixelSize(), WinUI3::Private::TooltipContentFontSize);

#ifdef Q_OS_WIN
    const auto regionType = [](WId handle) {
        const HRGN region = CreateRectRgn(0, 0, 0, 0);
        const int type = GetWindowRgn(reinterpret_cast<HWND>(handle), region);
        DeleteObject(region);
        return type;
    };
    QCOMPARE(regionType(firstHandle), 0); // no hard window region
    constexpr DWORD cornerAttribute = 33; // DWMWA_WINDOW_CORNER_PREFERENCE
    DWORD corner = 0;
    QCOMPARE(DwmGetWindowAttribute(reinterpret_cast<HWND>(firstHandle), cornerAttribute, &corner,
                                   sizeof(corner)),
             S_OK);
    QCOMPARE(corner, DWORD(3)); // DWMWCP_ROUNDSMALL: the 4 px ToolTip radius
#endif

    // A longer tip reuses the same label and HWND: Qt only resizes it, so the
    // style's Resize path must re-apply the same surface contract. This is the
    // regression the leftover `applyWindowRoundedRegion` on resize produced -
    // the reused tip came back with a hard, non-antialiased region.
    const QString longText = QStringLiteral(
            "WinUI tooltips wrap at the template maximum width of 320 pixels and keep the same "
            "window handle when the text changes under the pointer.");
    QToolTip::showText(button->mapToGlobal(button->rect().center()), longText, button);
    QTRY_COMPARE(tip->text(), longText);
    QCOMPARE(tip->winId(), firstHandle);
    QCOMPARE(tip->width(), WinUI3::Private::TooltipMaximumWidth);
    QCOMPARE(tip->size() == shortSize, false);
    QCOMPARE(tip->contentsMargins(), WinUI3::Private::tooltipContentMargins());
#ifdef Q_OS_WIN
    QCOMPARE(regionType(firstHandle), 0);
#endif

    // Qt's tooltip label is an application-wide event filter, so the Leave that
    // ends a hover hides the tip through its 300 ms grace timer. Reopening the
    // same widget then gives a fresh tip.
    QEvent leave(QEvent::Leave);
    QCoreApplication::sendEvent(button, &leave);
    QTRY_VERIFY_WITH_TIMEOUT(visibleTip() == nullptr, 1500);
    QToolTip::showText(anchor, QStringLiteral("WinUI tooltip surface"), button);
    QTRY_VERIFY_WITH_TIMEOUT(visibleTip() != nullptr, 2000);

    // A click anywhere dismisses the visible tip immediately (the same filter
    // stops on application mouse presses).
    QTest::mouseClick(button, Qt::LeftButton, Qt::NoModifier, button->rect().center());
    QTRY_VERIFY_WITH_TIMEOUT(visibleTip() == nullptr, 1500);

    // Display time: an explicit timeout closes the tip on its own.
    QToolTip::showText(button->mapToGlobal(button->rect().center()),
                       QStringLiteral("WinUI tooltip surface"), button, QRect(), 400);
    QTRY_VERIFY_WITH_TIMEOUT(visibleTip() != nullptr, 1500);
    QTRY_VERIFY_WITH_TIMEOUT(visibleTip() == nullptr, 3000);

    // Screen clamp: a tip placed at the bottom-right corner still fits inside
    // the screen's available geometry after the template re-layout.
    const QScreen *screen = testScreen(&window);
    QVERIFY(screen);
    const QRect available = screen->availableGeometry();
    QToolTip::showText(available.bottomRight() - QPoint(2, 2), longText, button);
    QTRY_VERIFY_WITH_TIMEOUT(visibleTip() != nullptr, 1500);
    tip = visibleTip();
    QVERIFY2(available.contains(tip->frameGeometry()),
             qPrintable(QStringLiteral("tooltip %1,%2 %3x%4 escapes %5,%6 %7x%8")
                                .arg(tip->frameGeometry().x())
                                .arg(tip->frameGeometry().y())
                                .arg(tip->width())
                                .arg(tip->height())
                                .arg(available.x())
                                .arg(available.y())
                                .arg(available.width())
                                .arg(available.height())));
    QToolTip::hideText();
    QTRY_VERIFY_WITH_TIMEOUT(visibleTip() == nullptr, 1500);
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
    // Reopen stability (bottom-margin regression): the cached popup keeps
    // the (0,4,0,4) inset, so the second open must reuse the settled height
    // (rows + 8) with no viewport tail from a repeated +8 compensation.
    const int settledRowHeight = combo->view()->visualRect(combo->model()->index(0, 0)).height();
    QVERIFY(settledRowHeight > 0);
    QCOMPARE(popup->contentsMargins(), QMargins(0, 4, 0, 4));
    QCOMPARE(settledGeometry.height(), combo->count() * settledRowHeight + 8);
    QTest::keyClick(combo->view(), Qt::Key_Escape);
    QTRY_VERIFY(!combo->view()->isVisible());
    combo->showPopup();
    QTRY_VERIFY(combo->view()->isVisible());
    QVERIFY(settleMenuSweep(popup));
    QTest::qWait(60);
    QCOMPARE(popup->height(), settledGeometry.height());
    // Outer geometry, not just row sizing: late margins used to reserve a
    // spurious 10 px scroller and clip the last row of this three-item flyout.
    const QRect viewport(combo->view()->viewport()->mapTo(popup, QPoint()),
                         combo->view()->viewport()->size());
    QCOMPARE(popup->height(), 3 * 40 + 8);
    QCOMPARE(viewport.top(), 4);
    QCOMPARE(viewport.height(), 3 * 40);
    QCOMPARE(popup->height() - viewport.bottom() - 1, 4);
    QCOMPARE(combo->view()->verticalScrollBar()->maximum(), 0);
    const QString capture = qEnvironmentVariable("WINUI3STYLE_COMBO_CAPTURE_FILE");
    if (!capture.isEmpty()) {
        const QImage frame = nativeWindowFrame(popup->winId());
        QCOMPARE(popup->height(), 128);
        QVERIFY(!frame.isNull());
        QVERIFY(frame.save(capture));
    }
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

void WinUI3StyleNativeTest::backdropDisableRestoresOpaqueContent()
{
    QStyle *pluginStyle = QStyleFactory::create(QStringLiteral("winui3"));
    // The style plugin is optional and its availability is an environment
    // property, not a style property: a run without QT_PLUGIN_PATH pointing at
    // the build's plugins directory (a direct binary run, or a host whose Qt
    // plugin loader refuses it) has no "winui3" key at all. Skip instead of
    // failing - the assertion below still runs whenever the plugin loads, which
    // is the case under ctest and CI.
    if (!pluginStyle)
        QSKIP("The optional WinUI3 style plugin is not available");
    qApp->setStyle(pluginStyle);
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setThemeMode(WinUI3::ThemeMode::Dark);

    QMainWindow window;
    // This test samples the desktop, not just the backing store. Unrelated
    // foreground windows and their DWM shadows must not cover its probe.
    window.setWindowFlag(Qt::WindowStaysOnTopHint);
    window.setGeometry(80, 80, 600, 400);
    auto *shell = new QWidget(&window);
    shell->setObjectName(QStringLiteral("centralWidget"));
    auto *layout = new QVBoxLayout(shell);
    layout->setContentsMargins(0, 0, 0, 0);
    auto *navigation = new QWidget(shell);
    navigation->setObjectName(QStringLiteral("navigationPanel"));
    layout->addWidget(navigation);
    auto *pages = new QStackedWidget(shell);
    pages->setProperty(WinUI3::Style::SurfaceProperty, QStringLiteral("content"));
    auto *area = new QScrollArea(pages);
    area->setFrameShape(QFrame::NoFrame);
    area->setWidgetResizable(true);
    auto *body = new QWidget;
    body->setMinimumHeight(700);
    auto *bodyLayout = new QVBoxLayout(body);
    bodyLayout->addWidget(new QLabel(QStringLiteral("Controls"), body));
    auto *card = new QGroupBox(QStringLiteral("Buttons"), body);
    auto *cardLayout = new QVBoxLayout(card);
    auto *standard = new QPushButton(QStringLiteral("Standard"), card);
    cardLayout->addWidget(standard);
    bodyLayout->addWidget(card);
    bodyLayout->addStretch();
    area->setWidget(body);
    pages->addWidget(area);
    layout->addWidget(pages);
    window.setCentralWidget(shell);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    const QColor expected = style->standardPalette().color(QPalette::Window);
    // Probe the margin beside the cards: the body does not cover missing
    // viewport/page erasure there, unlike a card-center sample.
    const QPoint probe = area->viewport()->mapToGlobal(QPoint(3, area->viewport()->height() / 2));
    const QSize contentSize = pages->size();
    const bool windowPaletteExplicit = window.testAttribute(Qt::WA_SetPalette);
    const bool pageAutoFill = pages->autoFillBackground();
    const bool bodyAutoFill = body->autoFillBackground();
    const bool viewportAutoFill = area->viewport()->autoFillBackground();
    const auto presented = [&] {
        QCoreApplication::processEvents();
        flushNativeCompositor();
        return DesktopTestFrame::capture(testScreen(&window)).colorAt(probe);
    };
    // Desktop capture is evidence only when it can see this window at all. A
    // GDI screen grab misses a DXGI-composed window on some hosts (remote
    // sessions, hardware overlay planes) and then every sample below is the
    // desktop wallpaper: the opacity contract would be measuring the
    // wallpaper, not the presented content. stableOpaqueDesktopBaseline gates
    // the menu probes the same way. The positive control is the "Standard"
    // button: the window always paints it opaque, so a match proves the grab
    // sees this window while saying nothing about the page margin under test.
    const QPoint control = standard->mapToGlobal(
            QPoint(standard->rect().width() / 4, standard->rect().height() / 2));
    const QImage selfFrame = window.grab().toImage();
    const qreal selfDpr = selfFrame.devicePixelRatio() > 0 ? selfFrame.devicePixelRatio() : 1.0;
    const QPoint controlInWindow = standard->mapTo(
            &window, QPoint(standard->rect().width() / 4, standard->rect().height() / 2));
    const QColor controlExpected = selfFrame.pixelColor(
            QPoint(qRound(controlInWindow.x() * selfDpr), qRound(controlInWindow.y() * selfDpr)));
    bool captureSeesWindow = false;
    for (int attempt = 0; attempt < 20 && !captureSeesWindow; ++attempt) {
        QCoreApplication::processEvents();
        flushNativeCompositor();
        captureSeesWindow =
                colorDistance(DesktopTestFrame::capture(testScreen(&window)).colorAt(control),
                              controlExpected)
                <= 2;
        if (!captureSeesWindow)
            QTest::qWait(25);
    }
    qWarning().noquote() << "presented capture"
                         << (captureSeesWindow ? "ready" : "BLOCKED/unverifiable")
                         << "control=" << control << "expected=" << controlExpected;
    const auto expectPresented = [&](const QColor &wanted) {
        if (!captureSeesWindow) {
            qWarning().noquote() << "presented-content sample BLOCKED/unverifiable: presented="
                                 << presented() << "expected=" << wanted;
            return;
        }
        QTRY_VERIFY2(colorDistance(presented(), wanted) <= 2,
                     qPrintable(QStringLiteral("presented=%1 expected=%2 probe=%3,%4")
                                        .arg(presented().name(QColor::HexArgb),
                                             wanted.name(QColor::HexArgb))
                                        .arg(probe.x())
                                        .arg(probe.y())));
    };
    QCOMPARE(window.palette().color(QPalette::Window), expected);
    QCOMPARE(window.testAttribute(Qt::WA_SetPalette), windowPaletteExplicit);
    expectPresented(expected);
    QVERIFY(pageAutoFill);
    QVERIFY(area->property("_winui_theme_owned_palette").toBool());
    QVERIFY(navigation->property("_winui_theme_owned_palette").toBool());

    window.setProperty(WinUI3::Style::BackdropProperty, QStringLiteral("mica"));
    QTRY_COMPARE(window.property("_winui_backdrop_effective").toInt(), 2);
    QVERIFY(window.property("_winui_theme_owned_palette").toBool());
    QVERIFY(area->property("_winui_theme_owned_palette").toBool());
    QVERIFY(navigation->property("_winui_theme_owned_palette").toBool());
    QCOMPARE(pages->palette().color(QPalette::Window).alpha(), 0);
    QVERIFY(!pages->autoFillBackground());
    area->verticalScrollBar()->setValue(100);
    QCOMPARE(area->verticalScrollBar()->value(), 100);
    window.setProperty(WinUI3::Style::BackdropProperty, QStringLiteral("none"));
    QCOMPARE(window.property("_winui_backdrop_effective").toInt(), 0);
    QCOMPARE(window.palette().color(QPalette::Window), expected);
    QCOMPARE(window.testAttribute(Qt::WA_SetPalette), windowPaletteExplicit);
    QVERIFY(area->property("_winui_theme_owned_palette").toBool());
    QVERIFY(navigation->property("_winui_theme_owned_palette").toBool());
    QCOMPARE(pages->size(), contentSize);
    QCOMPARE(window.testAttribute(Qt::WA_TranslucentBackground), false);
    QCOMPARE(pages->autoFillBackground(), pageAutoFill);
    QCOMPARE(body->autoFillBackground(), bodyAutoFill);
    QCOMPARE(area->viewport()->autoFillBackground(), viewportAutoFill);
    QCOMPARE(area->verticalScrollBar()->value(), 100);
    QTest::qWait(200);
    expectPresented(expected);

    window.setProperty(WinUI3::Style::BackdropProperty, QStringLiteral("mica"));
    QTRY_COMPARE(window.property("_winui_backdrop_effective").toInt(), 2);
    window.setProperty(WinUI3::Style::BackdropProperty, QStringLiteral("none"));
    QCOMPARE(pages->autoFillBackground(), pageAutoFill);
    QCOMPARE(window.testAttribute(Qt::WA_SetPalette), windowPaletteExplicit);
    expectPresented(expected);

    style->setThemeMode(WinUI3::ThemeMode::Light);
    const QColor lightSurface = style->standardPalette().color(QPalette::Window);
    QCOMPARE(qApp->palette().color(QPalette::Window), lightSurface);
    QCOMPARE(window.palette().color(QPalette::Window), lightSurface);
    QCOMPARE(shell->palette().color(QPalette::Window), lightSurface);
    QCOMPARE(pages->palette().color(QPalette::Window), lightSurface);
    QCOMPARE(area->palette().color(QPalette::Window), lightSurface);
    QCOMPARE(area->viewport()->palette().color(QPalette::Window), lightSurface);
    QCOMPARE(body->palette().color(QPalette::Window), lightSurface);
    QCOMPARE(card->palette().color(QPalette::Window), lightSurface);
    QCOMPARE(navigation->palette().color(QPalette::Window), lightSurface);
    QCOMPARE(pages->autoFillBackground(), pageAutoFill);
    expectPresented(lightSurface);
    area->verticalScrollBar()->setValue(200);
    QCOMPARE(area->verticalScrollBar()->value(), 200);
    expectPresented(lightSurface);
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
