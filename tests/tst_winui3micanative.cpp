// SPDX-License-Identifier: LGPL-2.1-or-later
// Native live repro of the reported Gallery regression (2026-10-12):
// Dark + Mica ON, switch to Light while granted, then Mica OFF must leave
// the navigation pane and client chrome on the Light theme, opaque. Drives
// the REAL GalleryWindow toggle path and the REAL DWM grant on the windows
// platform (skipped honestly offscreen). Companion to the offscreen recipe
// tests; the verdict here is presented pixels, not palette roles.
#include "../demo/gallerywindow.h"

#include "winui3testhelpers.h"

#include "../src/winui3backdrop_p.h"
#include "../src/winui3surfaces_p.h"
#include "../src/winui3tokens_p.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGuiApplication>
#include <QListWidget>
#include <QMenuBar>
#include <QScrollArea>

#ifdef Q_OS_WIN
#  include <windows.h>
#endif
#include <QLibrary>
#include <QWindow>

class WinUI3MicaNativeTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void liveThemeSwitchWhileGrantedThenDisable();
    void popupGrantAliveRejectsRevokedEffect();
};

void WinUI3MicaNativeTest::initTestCase()
{
    qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Dark));
}

void WinUI3MicaNativeTest::liveThemeSwitchWhileGrantedThenDisable()
{
    if (QGuiApplication::platformName() != QStringLiteral("windows"))
        QSKIP("requires the live DWM compositor");
    DisableAnimationsGuard animations;
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setThemeMode(WinUI3::ThemeMode::Dark);

    GalleryWindow gallery;
    gallery.resize(1180, 780);
    gallery.show();
    QVERIFY(QTest::qWaitForWindowExposed(&gallery));
    auto *navigation = gallery.findChild<QListWidget *>(QStringLiteral("navigationList"));
    auto *micaToggle = gallery.findChild<QCheckBox *>(QStringLiteral("micaToggle"));
    auto *themeCombo = gallery.findChild<QComboBox *>(QStringLiteral("themeCombo"));
    QVERIFY(navigation && micaToggle);
    // The Gallery theme switcher may be absent under some .ui revisions;
    // fall back to the style API (same property the combo writes).
    if (!themeCombo)
        qWarning() << "themeCombo not found, driving themeMode directly";

    // Dark + Mica ON through the Gallery's own toggle path.
    micaToggle->setChecked(true);
    QTRY_VERIFY_WITH_TIMEOUT(WinUI3::Private::backdropEffectiveSurface(&gallery)
                                     == WinUI3::Private::BackdropSurface::Composited,
                             5000);
    QCOMPARE(gallery.palette().color(QPalette::Window).alpha(), 0);

    // Switch to Light while granted, exactly like the live report.
    if (themeCombo)
        themeCombo->setCurrentIndex(themeCombo->findText(QStringLiteral("Light")));
    else
        style->setThemeMode(WinUI3::ThemeMode::Light);
    QTest::qWait(500);

    // Mica OFF through the Gallery's own toggle path.
    micaToggle->setChecked(false);
    QTest::qWait(500);
    QVERIFY(WinUI3::Private::backdropEffectiveSurface(&gallery)
            != WinUI3::Private::BackdropSurface::Composited);

    // Verdict on presented pixels: nav ink readable on a Light pane,
    // window opaque on the Light theme.
    const QImage frame = gallery.grab().toImage();
    QVERIFY(!frame.isNull());
    const QColor windowInk = frame.pixelColor(frame.width() / 2, frame.height() / 2);
    QCOMPARE(windowInk.alpha(), 255);
    const QPalette lightPalette = style->standardPalette();
    const QColor expectedWindow = lightPalette.color(QPalette::Window);
    QVERIFY(qAbs(windowInk.red() - expectedWindow.red()) <= 8);
    QVERIFY(qAbs(windowInk.green() - expectedWindow.green()) <= 8);
    QVERIFY(qAbs(windowInk.blue() - expectedWindow.blue()) <= 8);
    QCOMPARE(navigation->palette().color(QPalette::Active, QPalette::Text),
             lightPalette.color(QPalette::Active, QPalette::Text));
}

void WinUI3MicaNativeTest::popupGrantAliveRejectsRevokedEffect()
{
    // Combo popup light bands (user 2026-10-16): a silently revoked DWM
    // grant left the cached Composited property lying, so the re-show kept
    // the translucent tint over a white backing store. The keep-probe must
    // consult the live DWM readback, not just the property.
    if (QGuiApplication::platformName() != QStringLiteral("windows"))
        QSKIP("requires the live DWM compositor");
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setThemeMode(WinUI3::ThemeMode::Dark);

    QWidget host;
    host.resize(560, 440);
    QComboBox combo(&host);
    combo.addItems({ QStringLiteral("Standard density"), QStringLiteral("Compact density") });
    combo.resize(200, 32);
    host.show();
    QVERIFY(QTest::qWaitForWindowExposed(&host));
    combo.showPopup();
    QVERIFY(QTest::qWaitForWindowExposed(combo.view()));
    QWidget *popup = combo.view()->window();
    QVERIFY(popup && popup->windowHandle());
    combo.hidePopup();

    // Force the incoherent state: published Composited, DWM effect revoked.
    // Resolved dynamically so the test links with no extra dependency.
    QLibrary dwm(QStringLiteral("dwmapi"));
    using SetAttribute = HRESULT(WINAPI *)(HWND, DWORD, LPCVOID, DWORD);
    using GetAttribute = HRESULT(WINAPI *)(HWND, DWORD, PVOID, DWORD);
    auto setAttribute = reinterpret_cast<SetAttribute>(dwm.resolve("DwmSetWindowAttribute"));
    auto getAttribute = reinterpret_cast<GetAttribute>(dwm.resolve("DwmGetWindowAttribute"));
    QVERIFY(setAttribute && getAttribute);
    HWND hwnd = reinterpret_cast<HWND>(popup->windowHandle()->winId());
    QVERIFY(IsWindow(hwnd));
    DWORD autoValue = 0; // DWMSBT_AUTO: no acrylic.
    QVERIFY(SUCCEEDED(setAttribute(hwnd, 38, &autoValue, sizeof(autoValue))));
    DWORD readback = 0;
    if (FAILED(getAttribute(hwnd, 38, &readback, sizeof(readback))))
        QSKIP("DWM readback unreadable here");
    if (readback != 0)
        QSKIP("DWM ignores the revoke here");
    popup->setProperty("_winui_backdrop_effective", 2); // published Composited
    // The live readback (refused) overrules the lying property.
    QVERIFY(!WinUI3::Private::popupBackdropGrantAlive(popup));
    // Geometry pairing on the same state: the probe changed no geometry.
    QCOMPARE(popup->contentsMargins().top(), 4);
    QCOMPARE(popup->contentsMargins().bottom(), 4);
    style->setThemeMode(WinUI3::ThemeMode::Dark);
}

QTEST_MAIN(WinUI3MicaNativeTest)
#include "tst_winui3micanative.moc"
