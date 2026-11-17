// SPDX-License-Identifier: LGPL-2.1-or-later
// System-mode adaptation contract. The style replaces the application palette
// in Style::polish(QApplication *), so after startup the palette only reports
// the theme the style last applied. A desktop Light -> Dark switch must
// therefore be read from the platform color scheme, not from that style-owned
// palette. CMake forces a platform theme on non-Windows so the offscreen test
// process still publishes a real scheme; a platform that reports Unknown skips
// the contract instead of guessing.
#include <winui3style/winui3style.h>

#include "../src/lib/winui3theme_p.h"

#include <QApplication>
#include <QPalette>
#include <QScopeGuard>
#include <QStyleHints>
#include <QtTest>

#ifdef Q_OS_WIN
#  define WIN32_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#endif

class WinUI3SystemThemeTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void systemModeFollowsPlatformSchemeOverStyleOwnedPalette();
    void animationsAllowedFollowsSystemPreferenceAndOverride();

private:
    WinUI3::Style *m_style = nullptr;
};

void WinUI3SystemThemeTest::initTestCase()
{
    m_style = new WinUI3::Style(WinUI3::ThemeMode::Light);
    qApp->setStyle(m_style);
}

void WinUI3SystemThemeTest::cleanupTestCase()
{
    if (auto *style = qobject_cast<WinUI3::Style *>(qApp->style()))
        style->setThemeMode(WinUI3::ThemeMode::Light);
}

void WinUI3SystemThemeTest::systemModeFollowsPlatformSchemeOverStyleOwnedPalette()
{
#ifdef Q_OS_WIN
    QSKIP("Windows resolves the shell theme from the registry");
#else
    const Qt::ColorScheme scheme = QGuiApplication::styleHints()->colorScheme();
    if (scheme == Qt::ColorScheme::Unknown)
        QSKIP("platform does not publish a color scheme");

    // Explicit Light leaves the style in exactly the state a running Light
    // desktop produces: the style owns a light application palette.
    m_style->setThemeMode(WinUI3::ThemeMode::Light);
    QCOMPARE(qGray(qApp->palette().color(QPalette::Window).rgb()) > 128, true);

    // System must follow the platform scheme, not the style-owned palette.
    m_style->setThemeMode(WinUI3::ThemeMode::System);
    const bool expectDark = scheme == Qt::ColorScheme::Dark;
    const int windowLuma = qGray(qApp->palette().color(QPalette::Window).rgb());
    QCOMPARE(windowLuma < 128, expectDark);
    // The style still owns the palette, so it must be the palette it published.
    QCOMPARE(m_style->standardPalette().color(QPalette::Window),
             qApp->palette().color(QPalette::Window));

    // Explicit Light keeps overriding the platform scheme.
    m_style->setThemeMode(WinUI3::ThemeMode::Light);
    QCOMPARE(qGray(qApp->palette().color(QPalette::Window).rgb()) > 128, true);
#endif
}

void WinUI3SystemThemeTest::animationsAllowedFollowsSystemPreferenceAndOverride()
{
    WinUI3::Private::invalidateSystemAppearanceCache();
    const bool envWasSet = qEnvironmentVariableIsSet("WINUI3STYLE_DISABLE_ANIMATIONS");
    const QByteArray savedEnv = qgetenv("WINUI3STYLE_DISABLE_ANIMATIONS");
    const auto restore = qScopeGuard([&] {
        if (envWasSet)
            qputenv("WINUI3STYLE_DISABLE_ANIMATIONS", savedEnv);
        else
            qunsetenv("WINUI3STYLE_DISABLE_ANIMATIONS");
        WinUI3::Private::invalidateSystemAppearanceCache();
    });

    qunsetenv("WINUI3STYLE_DISABLE_ANIMATIONS");
    WinUI3::Private::invalidateSystemAppearanceCache();

#ifdef Q_OS_WIN
    constexpr UINT spiGetClientAreaAnimation = 0x1042;
    BOOL native = FALSE;
    QVERIFY(SystemParametersInfoW(spiGetClientAreaAnimation, 0, &native, 0));
    const bool expected = native != FALSE;
    QCOMPARE(WinUI3::Private::systemAnimationsAllowed(), expected);
#else
    const bool expected = true;
    QVERIFY(WinUI3::Private::systemAnimationsAllowed());
#endif
    QCOMPARE(WinUI3::Style::animationsAllowed(), expected);

    // Explicit opt-out wins over the system preference, for any env value.
    qputenv("WINUI3STYLE_DISABLE_ANIMATIONS", "1");
    QVERIFY(!WinUI3::Style::animationsAllowed());
    qputenv("WINUI3STYLE_DISABLE_ANIMATIONS", "0");
    QVERIFY(!WinUI3::Style::animationsAllowed());
}

QTEST_MAIN(WinUI3SystemThemeTest)
#include "tst_winui3systemtheme.moc"
