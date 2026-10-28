// SPDX-License-Identifier: LGPL-2.1-or-later
// System-mode adaptation contract. The style replaces the application palette
// in Style::polish(QApplication *), so after startup the palette only reports
// the theme the style last applied. A desktop Light -> Dark switch must
// therefore be read from the platform color scheme, not from that style-owned
// palette. CMake forces a platform theme on non-Windows so the offscreen test
// process still publishes a real scheme; a platform that reports Unknown skips
// the contract instead of guessing.
#include <winui3style/winui3style.h>

#include <QApplication>
#include <QPalette>
#include <QStyleHints>
#include <QtTest>

class WinUI3SystemThemeTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void systemModeFollowsPlatformSchemeOverStyleOwnedPalette();

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

QTEST_MAIN(WinUI3SystemThemeTest)
#include "tst_winui3systemtheme.moc"
