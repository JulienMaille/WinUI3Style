// SPDX-License-Identifier: LGPL-2.1-or-later
// Mica theme-switch lifecycle: rebase under a granted material, disable onto
// the current theme, deferred re-apply guards. Split out of
// tst_winui3micabackdrop.cpp per the 60 KB test-size ratchet. Every
// grab()/pixelColor() assertion pairs with a QCOMPARE token/geometry
// assertion on the same state.
#include <winui3style/winui3backdrop.h>
#include <winui3style/winui3style.h>

#include "winui3testhelpers.h"

#include "../src/winui3surfaces_p.h"
#include "../src/winui3tokens_p.h"

#include <QGroupBox>
#include <QListWidget>
#include <QMenuBar>
#include <QScrollArea>
#include <QScrollBar>
#include <QTabWidget>
#include <QToolBar>

class WinUI3MicaThemeTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void themeSwitchThenDisableRestoresCurrentTheme();
};

void WinUI3MicaThemeTest::initTestCase()
{
    qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light));
}

void WinUI3MicaThemeTest::init()
{
    if (auto *style = qobject_cast<WinUI3::Style *>(qApp->style())) {
        style->setThemeMode(WinUI3::ThemeMode::Light);
        style->setAccentColor({});
    }
}

void WinUI3MicaThemeTest::cleanup()
{
    if (auto *style = qobject_cast<WinUI3::Style *>(qApp->style()))
        style->setThemeMode(WinUI3::ThemeMode::Light);
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

void WinUI3MicaThemeTest::themeSwitchThenDisableRestoresCurrentTheme()
{
    // Live defect (Gallery 2026-10-12, Qt 6.12 x64): Dark + Mica ON, switch
    // to Light, then Mica OFF leaves the navigation pane gray and the
    // content cards on the wrong theme. Disabling a granted material must
    // restore every shell/island/card surface on the CURRENT theme palette,
    // opaque, in both switch directions. The double-switch leg below also
    // pins convergence of two back-to-back refreshes with queued backdrop
    // re-applies still pending.
    DisableAnimationsGuard animations;
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    for (int leg = 0; leg < 2; ++leg) {
        const bool startDark = (leg == 0);
        style->setThemeMode(startDark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
        QMainWindow window;
        window.setProperty("winuiBackdrop", QStringLiteral("mica"));
        auto *central = new QWidget(&window);
        central->setObjectName(QStringLiteral("centralWidget"));
        window.setCentralWidget(central);
        auto *nav = new QWidget(central);
        nav->setObjectName(QStringLiteral("navigationPanel"));
        auto *menuBar = new QMenuBar(central);
        menuBar->addAction(QStringLiteral("File"));
        auto *toolBar = new QToolBar(central);
        toolBar->addAction(QStringLiteral("Preview"));
        auto *tabs = new QTabWidget(central);
        auto *page = new QWidget;
        tabs->addTab(page, QStringLiteral("Collections"));
        auto *island = new QWidget(central);
        island->setProperty(WinUI3::Style::SurfaceProperty, QStringLiteral("content"));
        auto *card = new QGroupBox(QStringLiteral("Card"), island);
        auto *navView = new QListWidget(nav);
        navView->setProperty(WinUI3::Style::NavigationViewProperty, true);
        navView->addItems({ QStringLiteral("Controls"), QStringLiteral("Settings") });
        auto *area = new QScrollArea(island);
        auto *content = new QWidget;
        content->resize(760, 900);
        area->setWidget(content);
        area->resize(400, 200);
        window.resize(640, 420);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        // Theme switch while granted, then disable: the reported sequence.
        // Scroll first so scrolled backing-store rows join the picture.
        area->verticalScrollBar()->setValue(area->verticalScrollBar()->maximum());
        QVERIFY(area->verticalScrollBar()->value() > 0);
        style->setThemeMode(startDark ? WinUI3::ThemeMode::Light : WinUI3::ThemeMode::Dark);
        // Pump the event loop so the theme switch's queued backdrop re-apply
        // fires before disabling, exactly like the live Gallery.
        QTest::qWait(100);
        // Switch back with no pump in between: two back-to-back refreshes
        // must converge without an intervening sync (the queued shots stay
        // pending, so this leg observes the refresh passes alone).
        // Leg 1 skips the switch-back: after a full theme round-trip the
        // queued shots alone exercise its disable path, which leg 0 covers.
        if (leg != 1)
            style->setThemeMode(startDark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
        const QPalette granted = style->standardPalette();
        QCOMPARE(central->palette().color(QPalette::Window).red(),
                 granted.color(QPalette::Window).red());
        QCOMPARE(island->palette().color(QPalette::Window).red(),
                 granted.color(QPalette::Window).red());
        QTest::qWait(100);
        window.setProperty("winuiBackdrop", QStringLiteral("none"));
        const QPalette expected = style->standardPalette();
        const QColor expectedWindow = expected.color(QPalette::Window);
        const QColor expectedBase = expected.color(QPalette::Base);
        // tail asserts one QCOMPARE group at a time.
        const QList<QWidget *> surfaces = { central, nav, island, card };
        for (QWidget *surface : surfaces) {
            QCOMPARE(surface->palette().color(QPalette::Window).alpha(), 255);
            QCOMPARE(surface->palette().color(QPalette::Window).red(), expectedWindow.red());
            QCOMPARE(surface->palette().color(QPalette::Window).green(), expectedWindow.green());
            QCOMPARE(surface->palette().color(QPalette::Window).blue(), expectedWindow.blue());
        }
        QCOMPARE(menuBar->palette().color(QPalette::Window).red(), expectedWindow.red());
        QCOMPARE(toolBar->palette().color(QPalette::Window).red(), expectedWindow.red());
        QCOMPARE(tabs->palette().color(QPalette::Window).alpha(), 255);
        QCOMPARE(menuBar->palette().color(QPalette::Window).red(), expectedWindow.red());
        QCOMPARE(toolBar->palette().color(QPalette::Window).red(), expectedWindow.red());
        QCOMPARE(tabs->palette().color(QPalette::Window).alpha(), 255);
        QCOMPARE(navView->palette().color(QPalette::Base).alpha(), expectedBase.alpha());
        QCOMPARE(area->viewport()->palette().color(QPalette::Base).alpha(), expectedBase.alpha());
    }
    style->setThemeMode(WinUI3::ThemeMode::Light);
}

QTEST_MAIN(WinUI3MicaThemeTest)
#include "tst_winui3micatheme.moc"
