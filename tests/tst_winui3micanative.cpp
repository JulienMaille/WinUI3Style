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

class WinUI3MicaNativeTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void liveThemeSwitchWhileGrantedThenDisable();
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

QTEST_MAIN(WinUI3MicaNativeTest)
#include "tst_winui3micanative.moc"
