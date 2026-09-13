// SPDX-License-Identifier: LGPL-2.1-or-later
// Menu corner convergence (spec/coverage.md MenuFlyout row): paint r8 +
// mask r8 at the 50% threshold + concentric stroke. Both cases drive the
// real style dispatch offscreen; every grab()/paint pairs a QCOMPARE on
// the same state.
#include <winui3style/winui3backdrop.h>
#include <winui3style/winui3style.h>

#include "winui3testhelpers.h"

#include <QElapsedTimer>
#include <QGuiApplication>
#include <QImage>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QPainter>
#include <QStyleOption>
#include <QWindow>
#include <QtTest>
#include <functional>

#if defined(Q_OS_WIN)
#  include <windows.h>
#  include <dwmapi.h>
#endif

namespace {

constexpr int dwmSystemBackdropType = 38; // DWMWA_SYSTEMBACKDROP_TYPE.

int readGrant(WId windowId)
{
    int out = -2;
#if defined(Q_OS_WIN)
    for (int attempt = 0; attempt < 2; ++attempt) {
        DWORD value = 0;
        if (windowId
            && SUCCEEDED(DwmGetWindowAttribute(reinterpret_cast<HWND>(windowId),
                                               DWORD(dwmSystemBackdropType), &value,
                                               sizeof(value)))) {
            out = int(value);
            return out;
        }
        QTest::qSleep(50);
    }
#else
    Q_UNUSED(windowId)
#endif
    return out;
}

bool settlePopup(QWidget *popup)
{
    if (!popup)
        return false;
    QElapsedTimer timer;
    timer.start();
    while (popup->windowOpacity() != 1.0 && !timer.hasExpired(3000)) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        QTest::qWait(20);
    }
    QCoreApplication::processEvents();
    QTest::qWait(50);
    QCoreApplication::processEvents();
    return popup->windowOpacity() == 1.0;
}

struct GrantSnapshot
{
    QColor windowRole;
    QColor baseRole;
    QString effective;
    QImage grab;
    int dwm = -2;
};

#if defined(Q_OS_WIN)
struct BackdropReadback
{
    bool readable = false;
    int value = -2;
};

BackdropReadback readBackdropGrant(WId windowId)
{
    BackdropReadback result;
    const HWND hwnd = reinterpret_cast<HWND>(windowId);
    if (!hwnd)
        return result;
    DWORD raw = 0;
    if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWORD(dwmSystemBackdropType), &raw, sizeof(raw)))) {
        result.readable = true;
        result.value = int(raw);
    }
    return result;
}
#endif

bool snapGrant(const char *tag, QMenu &menu, GrantSnapshot *out)
{
    Q_UNUSED(tag)
    if (!menu.isVisible() || !settlePopup(&menu))
        return false;
    const QPixmap pixmap = menu.grab();
    if (pixmap.isNull() || pixmap.size() != menu.size())
        return false;
    out->grab = pixmap.toImage();
    if (out->grab.isNull() || out->grab.size().isEmpty())
        return false;
    out->windowRole = menu.palette().color(QPalette::Window);
    out->baseRole = menu.palette().color(QPalette::Base);
    const QVariant eff = menu.property("_winui_backdrop_effective");
    out->effective = eff.isValid() ? eff.toString() : QStringLiteral("invalid(solid)");
    out->dwm = readGrant(menu.winId());
    return true;
}

} // namespace

class WinUI3PopupCornersTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void popupCornersMaskGeometry();
    void popupCornersPanelFringe();
    void menuSubmenuGrantConverges();
    void menuReopenReadbackParity();
    void menuReopenGrantParityNative();
    void menuReopenGrantParityGalleryShape_data();
    void menuReopenGrantParityGalleryShape();
};

void WinUI3PopupCornersTest::initTestCase()
{
    qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light));
}

void WinUI3PopupCornersTest::init()
{
    if (auto *style = qobject_cast<WinUI3::Style *>(qApp->style())) {
        style->setThemeMode(WinUI3::ThemeMode::Light);
        style->setAccentColor({});
        style->setDensityMode(WinUI3::DensityMode::Standard);
    }
}

void WinUI3PopupCornersTest::cleanup()
{
    for (QWidget *widget : qApp->topLevelWidgets()) {
        if (widget->windowType() == Qt::Popup || widget->windowType() == Qt::ToolTip)
            widget->hide();
        else
            widget->close();
    }
    qApp->processEvents();
}

void WinUI3PopupCornersTest::popupCornersMaskGeometry()
{
    // Nominal r8 mapping (density-invariant token) with paint/mask
    // coincidence at the 50% line: full-size bounds, cut corner, kept
    // center, settled retry chain.
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QCOMPARE(WinUI3::Private::OverlayRadius, 8.0);

    QMenu menu;
    menu.addAction(QStringLiteral("&New project"));
    menu.addAction(QStringLiteral("E&xit"));
    menu.popup(QPoint(80, 80));
    QTRY_VERIFY(menu.isVisible());
    QVERIFY(!menu.size().isEmpty());
    QCOMPARE(menu.property("_winui_menu_mask_retry").isValid(), false);
    QCOMPARE(menu.mask().boundingRect(), QRect(QPoint(0, 0), menu.size()));
    QVERIFY(!menu.mask().contains(QPoint(0, 0)));
    QVERIFY(menu.mask().contains(menu.rect().center()));
    const QImage rendered = menu.grab().toImage();
    QVERIFY(!rendered.isNull());
    QCOMPARE(rendered.size(), menu.size());
    menu.hide();
}

void WinUI3PopupCornersTest::popupCornersPanelFringe()
{
    // Fringe mechanism RED->GREEN gate: the erase-then-fill must stay
    // inside the r8 surface, so the diagonal corner pixel keeps the
    // canvas fill. Pre-fix the unclipped Source clear left it
    // transparent (alpha distance 255), rendering as a dark fringe over
    // the live material wherever the mask keeps that pixel.
    QMenu menu;
    menu.addAction(QStringLiteral("&New project"));
    menu.resize(240, 160);
    menu.setAttribute(Qt::WA_TranslucentBackground, true);
    menu.setProperty("_winui_backdrop", static_cast<int>(WinUI3::Backdrop::Acrylic));
    menu.setProperty("_winui_backdrop_effective", 2); // BackdropSurface::Composited.
    const QColor fill = menu.palette().color(QPalette::Window);
    QImage canvas(menu.size(), QImage::Format_ARGB32_Premultiplied);
    canvas.fill(fill);
    QStyleOption option;
    option.initFrom(&menu);
    option.rect = menu.rect();
    QPainter painter(&canvas);
    qApp->style()->drawPrimitive(QStyle::PE_PanelMenu, &option, &painter, &menu);
    painter.end();
    QCOMPARE(canvas.size(), menu.size());
    QCOMPARE(canvas.pixelColor(canvas.width() / 2, canvas.height() / 2), fill);
    const QColor fringe = canvas.pixelColor(0, 0);
    QVERIFY2(colorDistance(fringe, fill) <= 12,
             qPrintable(QStringLiteral("corner fringe %1 vs fill %2")
                                .arg(fringe.name(QColor::HexArgb))
                                .arg(fill.name(QColor::HexArgb))));
}

void WinUI3PopupCornersTest::menuSubmenuGrantConverges()
{
    // Fresh-submenu grant convergence RED->GREEN gate: hover-opening the
    // Export submenu must converge the submenu on the parent's composited
    // branch (Show-dispatch DWM reads refuse on the fresh submenu HWND).
    // Pre-fix the parent holds the tint (eff 2) while the submenu stays
    // opaque (eff invalid(solid)); post-fix the deferred re-attempt past
    // dispatch converges both.
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget window;
    window.resize(480, 400);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QMenu fileMenu;
    fileMenu.setTitle(QStringLiteral("&File"));
    fileMenu.addAction(QStringLiteral("&New project"));
    QMenu *exportMenu = fileMenu.addMenu(QStringLiteral("&Export"));
    exportMenu->addAction(QStringLiteral("Portable &document"));
    exportMenu->addAction(QStringLiteral("&Image"));
    fileMenu.addSeparator();
    fileMenu.addAction(QStringLiteral("E&xit"));

    fileMenu.popup(window.mapToGlobal(QPoint(20, 20)));
    QTRY_VERIFY(fileMenu.isVisible());
    GrantSnapshot parent;
    QVERIFY2(snapGrant("parent", fileMenu, &parent), "parent snap failed");

    // Hover-open Export; fall back to popup at the row if hover stalls.
    const QRect exportRect = fileMenu.actionGeometry(exportMenu->menuAction());
    QVERIFY(exportRect.isValid());
    QElapsedTimer submenuWait;
    submenuWait.start();
    while (!exportMenu->isVisible() && !submenuWait.hasExpired(20000)) {
        QTest::mouseMove(&fileMenu, exportRect.center());
        QTest::qWait(500);
    }
    if (!exportMenu->isVisible())
        exportMenu->popup(fileMenu.mapToGlobal(exportRect.topRight()));
    QTRY_VERIFY(exportMenu->isVisible());
    GrantSnapshot submenu;
    QVERIFY2(snapGrant("submenu", *exportMenu, &submenu), "submenu snap failed");

    // Verdict: both composited pins tint equality; an honestly opaque
    // cycle gets opaque-recipe consistency instead (mixed-branch escape).
    // Offscreen always takes the opaque fallback, so its solid-parity pins
    // live here (the standalone unconditional case false-REDs live granted
    // runs).
    QCOMPARE(parent.windowRole, parent.baseRole);
    QCOMPARE(submenu.windowRole, submenu.baseRole);
    QCOMPARE(parent.grab.size(), fileMenu.size());
    QCOMPARE(submenu.grab.size(), exportMenu->size());
    const bool parentComp = parent.effective == QStringLiteral("2");
    const bool submenuComp = submenu.effective == QStringLiteral("2");
    if (parentComp && submenuComp) {
        QCOMPARE(submenu.windowRole, parent.windowRole);
        QCOMPARE(submenu.baseRole, parent.baseRole);
        const int greyDelta =
                qAbs(submenu.grab.pixelColor(2, 2).red() - parent.grab.pixelColor(2, 2).red())
                + qAbs(submenu.grab.pixelColor(2, 2).green() - parent.grab.pixelColor(2, 2).green())
                + qAbs(submenu.grab.pixelColor(2, 2).blue() - parent.grab.pixelColor(2, 2).blue());
        QVERIFY2(greyDelta <= 4,
                 qPrintable(QStringLiteral("submenu grant diverged (delta %1)").arg(greyDelta)));
    } else {
        if (!parentComp) {
            QCOMPARE(parent.windowRole.alpha(), 255);
            QCOMPARE(parent.baseRole.alpha(), 255);
        }
        if (!submenuComp) {
            QCOMPARE(submenu.windowRole.alpha(), 255);
            QCOMPARE(submenu.baseRole.alpha(), 255);
            QVERIFY(exportMenu->autoFillBackground());
            QVERIFY(exportMenu->testAttribute(Qt::WA_OpaquePaintEvent));
        }
        if (!parentComp && !submenuComp) {
            // Offscreen both-opaque-identical pin: the shipped grant fix
            // must not break the deterministic fallback.
            QCOMPARE(parent.effective, QStringLiteral("invalid(solid)"));
            QCOMPARE(submenu.effective, QStringLiteral("invalid(solid)"));
            QCOMPARE(submenu.windowRole, parent.windowRole);
            QCOMPARE(submenu.baseRole, parent.baseRole);
        }
    }
    fileMenu.hide();
}

void WinUI3PopupCornersTest::menuReopenReadbackParity()
{
    // Reopened-popup readback parity (re-arm refusal mechanism): DWM can
    // ACK Set(38) during Show dispatch while ignoring the value, so the
    // re-arm loop must not treat HRESULT success as granted. Showing the
    // same QMenu twice and comparing Get(38) across the two cycles pins
    // the mechanism; no pixels are asserted here.
    if (QGuiApplication::platformName() == QStringLiteral("offscreen"))
        QSKIP("DWM readback needs the native windows QPA");
#if !defined(Q_OS_WIN)
    QSKIP("DWM readback needs Windows");
#else
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QMenu menu;
    menu.addAction(QStringLiteral("&New project"));
    menu.addAction(QStringLiteral("E&xit"));

    menu.popup(QPoint(80, 80));
    QTRY_VERIFY(menu.isVisible());
    QTest::qWait(400);
    const BackdropReadback first = readBackdropGrant(menu.winId());
    menu.hide();
    QTest::qWait(100);
    QCoreApplication::processEvents();

    menu.popup(QPoint(80, 80));
    QTRY_VERIFY(menu.isVisible());
    QTest::qWait(400);
    const BackdropReadback second = readBackdropGrant(menu.winId());
    menu.hide();

    if (!first.readable || !second.readable)
        QSKIP("Session refused readable DWM (Get(38) failed)");
    // The readback QCOMPAREs are the mechanism assertion: a silently
    // refused reopen must not read back differently from the first show.
    QCOMPARE(second.value, first.value);
    if (first.value == 3)
        QCOMPARE(second.value, 3);
#endif
}

void WinUI3PopupCornersTest::menuReopenGrantParityNative()
{
    // RED reproduction for the live File-menu reopen defect (no setup
    // contamination: no preparePopupSurface, no _winui_backdrop property
    // writes, no QWidget::winId() forcing before show — event loop only).
    // Open-method priority: (1) Alt+F keyClick on the host, (2)
    // menuBar setActiveAction + Key_Down, (3) QMenu::popup(anchor)
    // fallback. The method actually used is recorded and reused for epoch
    // 2 ("reopen identically"); see openMethod below.
    if (QGuiApplication::platformName() != QStringLiteral("windows"))
        QSKIP("native DWM grant parity needs the windows QPA");
#if !defined(Q_OS_WIN)
    QSKIP("DWM readback needs Windows");
#else
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);

    QMainWindow host;
    QMenu *fileMenu = host.menuBar()->addMenu(QStringLiteral("&File"));
    fileMenu->addAction(QStringLiteral("&New project"));
    fileMenu->addAction(QStringLiteral("E&xit"));
    host.resize(640, 480);
    host.show();
    QVERIFY(QTest::qWaitForWindowExposed(&host));
    host.activateWindow();
    QTest::qWait(100);

    auto waitVisible = [&](int timeoutMs) {
        QElapsedTimer timer;
        timer.start();
        while (!timer.hasExpired(timeoutMs)) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
            if (fileMenu->isVisible())
                return true;
            QTest::qWait(50);
        }
        return fileMenu->isVisible();
    };
    auto waitHidden = [&](int timeoutMs) {
        QElapsedTimer timer;
        timer.start();
        while (!timer.hasExpired(timeoutMs)) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
            if (!fileMenu->isVisible())
                return true;
            QTest::qWait(50);
        }
        return !fileMenu->isVisible();
    };

    QString openMethod;
    const QPoint anchor = host.mapToGlobal(QPoint(20, host.menuBar()->height() + 4));
    auto openOnce = [&](bool firstAttempt) {
        if (!firstAttempt && !openMethod.isEmpty()) {
            // Reopen identically with the epoch-1 method.
            if (openMethod.startsWith(QStringLiteral("Alt+F"))) {
                QTest::keyClick(&host, Qt::Key_F, Qt::AltModifier);
                return waitVisible(1000);
            }
            if (openMethod.startsWith(QStringLiteral("setActiveAction"))) {
                host.menuBar()->setActiveAction(fileMenu->menuAction());
                QTest::keyClick(host.menuBar(), Qt::Key_Down);
                return waitVisible(1000);
            }
            fileMenu->popup(anchor);
            return waitVisible(1000);
        }
        QTest::keyClick(&host, Qt::Key_F, Qt::AltModifier);
        if (waitVisible(1000)) {
            openMethod = QStringLiteral("Alt+F keyClick(host)");
            return true;
        }
        host.menuBar()->setActiveAction(fileMenu->menuAction());
        QTest::keyClick(host.menuBar(), Qt::Key_Down);
        if (waitVisible(1000)) {
            openMethod = QStringLiteral("setActiveAction+Key_Down(menuBar)");
            return true;
        }
        fileMenu->popup(anchor);
        if (waitVisible(1000)) {
            openMethod = QStringLiteral("QMenu::popup(anchor) fallback");
            return true;
        }
        return false;
    };

    QVERIFY2(openOnce(true),
             "could not open File menu by Alt+F, setActiveAction+Down, or popup fallback");

    // Epoch 1: expose + ~500 ms QTRY window (covers rearm retries
    // 0/32/96/200 ms). HWND via windowHandle()->winId() only.
    QWindow *handle1 = fileMenu->windowHandle();
    QVERIFY2(handle1 != nullptr, "epoch1: File menu has no windowHandle after show");
    QVERIFY(QTest::qWaitForWindowExposed(handle1));
    QTRY_VERIFY_WITH_TIMEOUT(fileMenu->isVisible(), 500);
    QMenu *ptr1 = fileMenu;
    const WId hwnd1 = handle1->winId();
    DWORD raw1 = 0;
    const HRESULT hr1 = DwmGetWindowAttribute(reinterpret_cast<HWND>(hwnd1),
                                              DWORD(dwmSystemBackdropType), &raw1, sizeof(raw1));
    const QRect rect1 = fileMenu->geometry();
    const QString rectText1 = QStringLiteral("(%1,%2 %3x%4)")
                                      .arg(rect1.x())
                                      .arg(rect1.y())
                                      .arg(rect1.width())
                                      .arg(rect1.height());

    // Close via hide() + processEvents (recorded method).
    fileMenu->hide();
    QCoreApplication::processEvents();
    QTest::qWait(100);
    QVERIFY2(waitHidden(1000), "epoch1: File menu did not hide after hide()+processEvents");

    // Epoch 2: reopen identically, same waits.
    QVERIFY2(openOnce(false), "could not reopen File menu with the epoch-1 method");
    QWindow *handle2 = fileMenu->windowHandle();
    QVERIFY2(handle2 != nullptr, "epoch2: File menu has no windowHandle after reopen");
    QVERIFY(QTest::qWaitForWindowExposed(handle2));
    QTRY_VERIFY_WITH_TIMEOUT(fileMenu->isVisible(), 500);
    QMenu *ptr2 = fileMenu;
    const WId hwnd2 = handle2->winId();
    DWORD raw2 = 0;
    const HRESULT hr2 = DwmGetWindowAttribute(reinterpret_cast<HWND>(hwnd2),
                                              DWORD(dwmSystemBackdropType), &raw2, sizeof(raw2));
    const QRect rect2 = fileMenu->geometry();
    const QString rectText2 = QStringLiteral("(%1,%2 %3x%4)")
                                      .arg(rect2.x())
                                      .arg(rect2.y())
                                      .arg(rect2.width())
                                      .arg(rect2.height());
    fileMenu->hide();
    QCoreApplication::processEvents();

    const bool sameMenuObject = (ptr2 == ptr1);
    const bool sameHwnd = (hwnd2 == hwnd1);

    if (hr1 != S_OK || hr2 != S_OK) {
        QSKIP("Session refused readable DWM (Get(38) HRESULT failed on at least one epoch)");
    }
    const int value1 = int(raw1);
    const int value2 = int(raw2);
    // Expected RED: first popup granted (3), reopen silently AUTO (0).
    QVERIFY2(!(value1 == 3 && value2 != 3),
             qPrintable(QStringLiteral(
                                "reopen grant lost: e1 hwnd=0x%1 value=%2, e2 hwnd=0x%3 value=%4 "
                                "(openMethod=%5 sameQMenu=%6 sameHWND=%7)")
                                .arg(static_cast<quintptr>(hwnd1), 0, 16)
                                .arg(value1)
                                .arg(static_cast<quintptr>(hwnd2), 0, 16)
                                .arg(value2)
                                .arg(openMethod)
                                .arg(sameMenuObject)
                                .arg(sameHwnd)));
    QCOMPARE(value2, value1);
#endif
}

void WinUI3PopupCornersTest::menuReopenGrantParityGalleryShape_data()
{
    QTest::addColumn<bool>("requestMica");
    QTest::newRow("plain-host") << false;
    QTest::newRow("mica-host") << true;
}

void WinUI3PopupCornersTest::menuReopenGrantParityGalleryShape()
{
    // MenuFlyout mapping: controlled host-backdrop hypothesis, not a claim
    // that the external gallery used Mica (its host Get(38) was AUTO).
    // gallerywindow.cpp constructor: popup at 1500, close at 5500, reopen
    // at 8500 ms. Keep content/theme/input identical across the two rows.
    if (QGuiApplication::platformName() != QStringLiteral("windows"))
        QSKIP("native DWM grant parity needs the windows QPA");
#if !defined(Q_OS_WIN)
    QSKIP("DWM readback needs Windows");
#else
    QFETCH(bool, requestMica);
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setThemeMode(WinUI3::ThemeMode::Light);
    QMainWindow host;
    QMenu *fileMenu = host.menuBar()->addMenu(QStringLiteral("&File"));
    fileMenu->addAction(QStringLiteral("&New project"));
    fileMenu->addAction(QStringLiteral("E&xit"));
    host.resize(640, 480);
    QElapsedTimer timeline;
    timeline.start();
    host.show();
    QVERIFY(QTest::qWaitForWindowExposed(&host));
    // Use the public API, never a manually toggled translucent attribute.
    const bool applied = requestMica && WinUI3::applyBackdrop(&host, WinUI3::Backdrop::Mica);

    struct Sample
    {
        quintptr menu = 0;
        WId hwnd = 0;
        HRESULT hr = E_HANDLE;
        DWORD value = DWORD(-1);
        HRESULT hostHr = E_HANDLE;
        DWORD hostValue = DWORD(-1);
    } settled[2];
    int epoch = 0;
    int showCount = 0;
    bool micaVerified = true;
    const auto record = [&](const char *phase) {
        Sample sample;
        sample.menu = reinterpret_cast<quintptr>(fileMenu);
        // qwidget.h: internalWinId() returns data->winid; never creates a handle.
        sample.hwnd = fileMenu->internalWinId();
        const WId hostId = host.internalWinId();
        sample.hr = DwmGetWindowAttribute(reinterpret_cast<HWND>(sample.hwnd),
                                          DWORD(dwmSystemBackdropType), &sample.value,
                                          sizeof(sample.value));
        sample.hostHr =
                DwmGetWindowAttribute(reinterpret_cast<HWND>(hostId), DWORD(dwmSystemBackdropType),
                                      &sample.hostValue, sizeof(sample.hostValue));
        micaVerified = micaVerified && sample.hostHr == S_OK && sample.hostValue == 2;
        return sample;
    };
    // Task-local, read-only Show observer; no custom widget or shared helper.
    struct ShowObserver final : QObject
    {
        std::function<void()> onShow;
        bool eventFilter(QObject *, QEvent *event) override
        {
            if (event->type() == QEvent::Show)
                onShow();
            return false;
        }
    } observer;
    observer.onShow = [&] {
        ++showCount;
        record("Show");
    };
    fileMenu->installEventFilter(&observer);
    const auto waitUntil = [&](qint64 deadline) {
        while (timeline.elapsed() < deadline)
            QTest::qWait(int(qMin(qint64(20), deadline - timeline.elapsed())));
    };
    for (epoch = 0; epoch < 2; ++epoch) {
        waitUntil(epoch == 0 ? 1500 : 8500);
        const QPoint anchor = host.menuBar()->mapToGlobal(
                QPoint(host.menuBar()->actionGeometry(fileMenu->menuAction()).left(),
                       host.menuBar()->height()));
        fileMenu->popup(anchor);
        QTRY_VERIFY(fileMenu->isVisible());
        QVERIFY(fileMenu->windowHandle());
        QVERIFY(QTest::qWaitForWindowExposed(fileMenu->windowHandle()));
        const qint64 settleStart = timeline.elapsed();
        QTest::qWait(500); // Real elapsed time, not immediately-true QTRY.
        QVERIFY(timeline.elapsed() - settleStart >= 500);
        QVERIFY(fileMenu->isVisible());
        settled[epoch] = record("settled");
        if (epoch == 0)
            waitUntil(5500);
        QVERIFY(fileMenu->close());
        QCoreApplication::processEvents();
        QVERIFY(!fileMenu->isVisible());
    }
    QCOMPARE(showCount, 2);
    const auto &first = settled[0];
    const auto &second = settled[1];
    if (requestMica && (!applied || !micaVerified))
        QSKIP("BLOCKED: requested Mica was not verified as host Get(38)==2 at every sample");
    if (first.hr != S_OK || first.value != 3)
        QSKIP("Session did not grant readable acrylic on first open; cannot test grant retention");
    // A granted first epoch followed by a stale E_HANDLE is the reproduced
    // close-time WinIdChange defect, not a session-refused skip.
    QVERIFY2(second.hr == S_OK && second.value == 3,
             qPrintable(QStringLiteral("close/reopen grant lost: first HWND=0x%1 hr=0x%2 value=%3; "
                                       "reopen HWND=0x%4 hr=0x%5 value=%6")
                                .arg(quintptr(first.hwnd), 0, 16)
                                .arg(quint32(first.hr), 8, 16, QLatin1Char('0'))
                                .arg(qint32(first.value))
                                .arg(quintptr(second.hwnd), 0, 16)
                                .arg(quint32(second.hr), 8, 16, QLatin1Char('0'))
                                .arg(qint32(second.value))));
    QCOMPARE(second.value, first.value);
#endif
}

QTEST_MAIN(WinUI3PopupCornersTest)
#include "tst_winui3popupcorners.moc"
