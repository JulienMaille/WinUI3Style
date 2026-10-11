// SPDX-License-Identifier: LGPL-2.1-or-later
// Exact-construction live repro: the REAL GalleryWindow class (the same
// demo/gallerywindow.cpp/.h/.ui sources the reviewer runs live) driven
// through the live Mica-toggle property path ("winuiBackdrop").
// Offscreen-capable: the DWM grant is simulated the same way the micacard
// mechanism tests do (Composited publish + chrome/content syncs in live
// order). Controls page scrolled to the below-fold Density rows 7-9
// (Navigation/MenuBar/Password) + Palette Lab page visit; exact widget
// rects probed against a NONE twin. Every pixel assert pairs with a
// token/geometry assert on the same state.
#include "../demo/gallerywindow.h"
#include "winui3testhelpers.h"

#include "../src/winui3surfaces_p.h"
#include "../src/winui3tokens_p.h"

#include <QComboBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QListWidget>
#include <QMenuBar>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>

class WinUI3GalleryProbeTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void galleryProbeKeepsCardFillOverComposited();
};

void WinUI3GalleryProbeTest::initTestCase()
{
    qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light));
}

void WinUI3GalleryProbeTest::init()
{
    if (auto *style = qobject_cast<WinUI3::Style *>(qApp->style())) {
        style->setThemeMode(WinUI3::ThemeMode::Light);
        style->setAccentColor({});
    }
}

void WinUI3GalleryProbeTest::cleanup()
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

void WinUI3GalleryProbeTest::galleryProbeKeepsCardFillOverComposited()
{
    // Dark/light; every pixel assert pairs with a token/geometry assert.
    DisableAnimationsGuard animations;
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    for (const bool dark : { false, true }) {
        style->setThemeMode(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
        GalleryWindow mica;
        mica.resize(1180, 780);
        mica.show();
        QVERIFY(QTest::qWaitForWindowExposed(&mica));
        // The live toggle path: dynamic property consumed by the style's
        // event filter (offscreen applyBackdrop publishes Painted and rebinds
        // the window Window role; the grant below publishes Composited).
        mica.setProperty("winuiBackdrop", QStringLiteral("mica"));
        mica.setProperty("_winui_backdrop", 1);
        mica.setProperty("_winui_backdrop_effective", 2); // Composited
        WinUI3::Private::makeChromeSurfacesTransparent(&mica);
        WinUI3::Private::syncContentSurfacesForBackdrop(&mica);
        qApp->processEvents();
        auto *nav = mica.findChild<QListWidget *>(QStringLiteral("standardDensityNav"));
        auto *menu = mica.findChild<QMenuBar *>(QStringLiteral("standardDensityMenu"));
        auto *password = mica.findChild<QLineEdit *>(QStringLiteral("standardDensityPassword"));
        auto *compactNav = mica.findChild<QListWidget *>(QStringLiteral("compactDensityNav"));
        auto *compactMenu = mica.findChild<QMenuBar *>(QStringLiteral("compactDensityMenu"));
        auto *compactPassword =
                mica.findChild<QLineEdit *>(QStringLiteral("compactDensityPassword"));
        auto *group = mica.findChild<QGroupBox *>(QStringLiteral("densityComparisonGroup"));
        auto *search = mica.findChild<QLineEdit *>(QStringLiteral("searchSettings"));
        auto *preview = mica.findChild<QWidget *>(QStringLiteral("palettePreview"));
        auto *previewEdit = mica.findChild<QLineEdit *>(QStringLiteral("previewEdit"));
        auto *pages = mica.findChild<QStackedWidget *>(QStringLiteral("pages"));
        auto *controlsPage = mica.findChild<QScrollArea *>(QStringLiteral("controlsPage"));
        auto *palettePage = mica.findChild<QScrollArea *>(QStringLiteral("paletteLabPage"));
        QVERIFY(nav && menu && password && group && search && preview && previewEdit && pages
                && controlsPage && palettePage);
        QVERIFY(compactNav && compactMenu && compactPassword);
        QVERIFY(WinUI3::Private::insideOpaqueCard(nav));
        QVERIFY(WinUI3::Private::insideOpaqueCard(menu));
        QVERIFY(WinUI3::Private::insideOpaqueCard(password));
        QVERIFY(WinUI3::Private::insideOpaqueCard(compactNav));
        QVERIFY(WinUI3::Private::insideOpaqueCard(compactMenu));
        QVERIFY(WinUI3::Private::insideOpaqueCard(compactPassword));
        // palettePreview is deliberately NOT a card (uniform-mica design):
        // no fill of its own, uniform with the material page.
        QVERIFY(!WinUI3::Private::insideOpaqueCard(previewEdit));
        // Token pair: the group card tone is the min-clamp veil of the Base
        // layer (dedicated CC_GroupBox painter, reverted 178 experiment).
        const QColor cardTone =
                WinUI3::Private::veiledCard(WinUI3::Private::tokens(group->palette()).layer, true);
        QCOMPARE(cardTone.alpha(), group->palette().color(QPalette::Base).alpha());
        // NONE twin: same construction, no backdrop path at all.
        GalleryWindow none;
        none.resize(1180, 780);
        none.show();
        QVERIFY(QTest::qWaitForWindowExposed(&none));
        auto *noneNav = none.findChild<QListWidget *>(QStringLiteral("standardDensityNav"));
        auto *noneMenu = none.findChild<QMenuBar *>(QStringLiteral("standardDensityMenu"));
        auto *nonePassword = none.findChild<QLineEdit *>(QStringLiteral("standardDensityPassword"));
        auto *noneCompactNav = none.findChild<QListWidget *>(QStringLiteral("compactDensityNav"));
        auto *noneCompactMenu = none.findChild<QMenuBar *>(QStringLiteral("compactDensityMenu"));
        auto *noneCompactPassword =
                none.findChild<QLineEdit *>(QStringLiteral("compactDensityPassword"));
        auto *noneSearch = none.findChild<QLineEdit *>(QStringLiteral("searchSettings"));
        auto *nonePreview = none.findChild<QWidget *>(QStringLiteral("palettePreview"));
        QVERIFY(noneNav && noneMenu && nonePassword && noneSearch && nonePreview);
        QVERIFY(noneCompactNav && noneCompactMenu && noneCompactPassword);
        // Geometries identical NONE vs Mica: no shifts, no eaten borders.
        QCOMPARE(nav->geometry(), noneNav->geometry());
        QCOMPARE(menu->geometry(), noneMenu->geometry());
        QCOMPARE(password->geometry(), nonePassword->geometry());
        QCOMPARE(compactNav->geometry(), noneCompactNav->geometry());
        QCOMPARE(compactMenu->geometry(), noneCompactMenu->geometry());
        QCOMPARE(compactPassword->geometry(), noneCompactPassword->geometry());
        QCOMPARE(search->geometry(), noneSearch->geometry());
        QCOMPARE(preview->geometry(), nonePreview->geometry());
        // Scroll the live page to the below-fold rows 7-9, like the
        // reviewer. Each target is revealed with ensureWidgetVisible and the
        // visibility guard itself is asserted, so a probe can never pass by
        // skipping (vacuous green).
        pages->setCurrentIndex(0);
        const auto renderWindow = [&] {
            QImage frame(mica.size(), QImage::Format_ARGB32_Premultiplied);
            frame.fill(Qt::transparent);
            mica.render(&frame);
            return frame;
        };
        const auto probeInWindow = [&](const QWidget *viewport, const QPoint &local) {
            const QPoint mapped = viewport->mapTo(&mica, local);
            // Asserted via fatal, not assumed: the target must be on-screen
            // (QTest macros return void and cannot type a lambda result).
            if (!QRect(QPoint(0, 0), mica.size()).contains(mapped)
                || !viewport->rect().contains(local))
                qFatal("probe offscreen for %s — refusing vacuous green",
                       qPrintable(viewport->objectName()));
            return mapped;
        };
        // Row 7 sample: rest row keeps the card composite (RED: alpha 0).
        const QRect rowRect = nav->visualItemRect(nav->item(0));
        QVERIFY(!rowRect.isEmpty());
        controlsPage->ensureWidgetVisible(nav);
        qApp->processEvents();
        QImage grab = renderWindow();
        QCOMPARE(grab.size(), mica.size());
        QVERIFY(grab.pixelColor(probeInWindow(nav->viewport(), QPoint(4, rowRect.center().y())))
                        .alpha()
                != 0);
        // Row 8 sample: menu bar keeps its fill over the card.
        QAction *fileAction = menu->actions().value(0);
        QVERIFY(fileAction);
        const QRect actionRect = menu->actionGeometry(fileAction);
        QVERIFY(!actionRect.isEmpty());
        controlsPage->ensureWidgetVisible(menu);
        qApp->processEvents();
        grab = renderWindow();
        QVERIFY(grab.pixelColor(probeInWindow(menu, actionRect.center())).alpha() != 0);
        // Row 9 sample: password editor keeps fill + border.
        const QRect passwordRect = password->rect();
        controlsPage->ensureWidgetVisible(password);
        qApp->processEvents();
        grab = renderWindow();
        QVERIFY(grab.pixelColor(probeInWindow(password, passwordRect.center())).alpha() != 0);
        QVERIFY(grab.pixelColor(probeInWindow(password, QPoint(0, passwordRect.height() / 2)))
                        .alpha()
                != 0);
        // Compact twins (same densityComparisonGroup card): rest row, menu
        // action, and password editor keep the card composite in Compact
        // metrics too. Same pairings as the standard rows above — geometry
        // QCOMPAREs (mica vs none, asserted at lookup) plus these pixels.
        const QRect compactRowRect = compactNav->visualItemRect(compactNav->item(0));
        QVERIFY(!compactRowRect.isEmpty());
        controlsPage->ensureWidgetVisible(compactNav);
        qApp->processEvents();
        grab = renderWindow();
        QVERIFY(grab.pixelColor(probeInWindow(compactNav->viewport(),
                                              QPoint(4, compactRowRect.center().y())))
                        .alpha()
                != 0);
        QAction *compactFileAction = compactMenu->actions().value(0);
        QVERIFY(compactFileAction);
        const QRect compactActionRect = compactMenu->actionGeometry(compactFileAction);
        QVERIFY(!compactActionRect.isEmpty());
        controlsPage->ensureWidgetVisible(compactMenu);
        qApp->processEvents();
        grab = renderWindow();
        QVERIFY(grab.pixelColor(probeInWindow(compactMenu, compactActionRect.center())).alpha()
                != 0);
        const QRect compactPasswordRect = compactPassword->rect();
        controlsPage->ensureWidgetVisible(compactPassword);
        qApp->processEvents();
        grab = renderWindow();
        QVERIFY(grab.pixelColor(probeInWindow(compactPassword, compactPasswordRect.center()))
                        .alpha()
                != 0);
        QVERIFY(grab.pixelColor(probeInWindow(compactPassword,
                                              QPoint(0, compactPasswordRect.height() / 2)))
                        .alpha()
                != 0);
        // Shell search box: border + fill present under Mica (never scrolled).
        grab = renderWindow();
        QVERIFY(grab.pixelColor(probeInWindow(search, QPoint(0, search->rect().height() / 2)))
                        .alpha()
                != 0);
        QVERIFY(grab.pixelColor(probeInWindow(search, search->rect().center())).alpha() != 0);
        // Live interaction states take different paint branches (hover
        // fills). Hover a nav row with real events, then re-probe: any
        // in-card branch that erases without the card refill shows here.
        controlsPage->ensureWidgetVisible(nav);
        QTest::mouseMove(nav->viewport(), nav->visualItemRect(nav->item(0)).center());
        qApp->processEvents();
        QImage liveGrab(mica.size(), QImage::Format_ARGB32_Premultiplied);
        liveGrab.fill(Qt::transparent);
        mica.render(&liveGrab);
        QVERIFY(liveGrab.pixelColor(probeInWindow(nav->viewport(), QPoint(4, rowRect.center().y())))
                        .alpha()
                != 0);
        // Palette Lab page: the preview card + editor keep the card tone.
        QStackedWidget *nonePages = none.findChild<QStackedWidget *>(QStringLiteral("pages"));
        QScrollArea *nonePalettePage =
                none.findChild<QScrollArea *>(QStringLiteral("paletteLabPage"));
        QVERIFY(nonePages && nonePalettePage);
        pages->setCurrentWidget(palettePage);
        nonePages->setCurrentWidget(nonePalettePage);
        qApp->processEvents();
        auto *nonePreviewEdit = none.findChild<QLineEdit *>(QStringLiteral("previewEdit"));
        QVERIFY(nonePreviewEdit);
        QCOMPARE(previewEdit->geometry(), nonePreviewEdit->geometry());
        QImage labGrab(mica.size(), QImage::Format_ARGB32_Premultiplied);
        labGrab.fill(Qt::transparent);
        mica.render(&labGrab);
        const QPoint editCenter = previewEdit->mapTo(&mica, previewEdit->rect().center());
        QVERIFY(QRect(QPoint(0, 0), mica.size()).contains(editCenter));
        QVERIFY(labGrab.pixelColor(editCenter).alpha() != 0);
        const QPoint bareCard =
                preview->mapTo(&mica, QPoint(preview->width() / 2, preview->height() - 8));
        QVERIFY(QRect(QPoint(0, 0), mica.size()).contains(bareCard));
        // Bare preview strip: uniform material, never a veil.
        QCOMPARE(labGrab.pixelColor(bareCard).alpha(), 0);
        // Focused editor on the live page: focused fill + accent underline
        // repaint over the material; the rounded corner stays cleared.
        previewEdit->setFocus(Qt::MouseFocusReason);
        qApp->processEvents();
        QImage focusGrab(mica.size(), QImage::Format_ARGB32_Premultiplied);
        focusGrab.fill(Qt::transparent);
        mica.render(&focusGrab);
        QVERIFY(focusGrab.pixelColor(probeInWindow(previewEdit, previewEdit->rect().center()))
                       .alpha()
                != 0);
        QCOMPARE(focusGrab.pixelColor(probeInWindow(previewEdit, QPoint(0, 0))).alpha(), 0);
    }
    style->setThemeMode(WinUI3::ThemeMode::Light);
}

QTEST_MAIN(WinUI3GalleryProbeTest)
#include "tst_winui3galleryprobe.moc"
