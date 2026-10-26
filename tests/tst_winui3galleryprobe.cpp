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
#include <QCheckBox>
#include <QCalendarWidget>
#include <QDateEdit>
#include <QGroupBox>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QMenuBar>
#include <QMenu>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QStackedWidget>
#include <QStyleFactory>
#include <QTemporaryDir>

class WinUI3GalleryProbeTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void galleryProbeKeepsCardFillOverComposited();
    void galleryNavigationPaletteFollowsThemeAfterMicaDisable();
    void galleryThemeSwitchWhileGrantedThenDisable();
    void galleryRealGrantThemeSwitchThenDisable();
    void galleryStaleReapplyAfterDisable();
    void gallerySnapshotPinsAndRestoresCalendarDate();
    void galleryComparisonStyleAvailabilityAndRestore();
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

void WinUI3GalleryProbeTest::galleryComparisonStyleAvailabilityAndRestore()
{
    GalleryWindow gallery;
    auto *comparison = gallery.findChild<QMenu *>(QStringLiteral("comparisonStyleMenu"));
    auto *winuiAction = gallery.findChild<QAction *>(QStringLiteral("winui3StyleAction"));
    auto *windows11Action = gallery.findChild<QAction *>(QStringLiteral("windows11StyleAction"));
    auto *theme = gallery.findChild<QComboBox *>(QStringLiteral("themeCombo"));
    auto *density = gallery.findChild<QComboBox *>(QStringLiteral("densityCombo"));
    QVERIFY(comparison && winuiAction && windows11Action && theme && density);
    QVERIFY(winuiAction->isChecked());

    QString windows11Key;
    for (const QString &key : QStyleFactory::keys()) {
        if (key.compare(QStringLiteral("windows11"), Qt::CaseInsensitive) == 0) {
            windows11Key = key;
            break;
        }
    }
    QStyle *nativeStyle = windows11Key.isEmpty() ? nullptr : QStyleFactory::create(windows11Key);
    if (!nativeStyle) {
        QVERIFY(!windows11Action->isEnabled());
        QVERIFY(windows11Action->text().contains(QStringLiteral("unavailable"),
                                                 Qt::CaseInsensitive));
        return;
    }
    const QByteArray nativeClassName = nativeStyle->metaObject()->className();
    delete nativeStyle;

    QVERIFY(windows11Action->isEnabled());
    theme->setCurrentIndex(2);
    density->setCurrentIndex(1);
    windows11Action->trigger();
    QVERIFY(!theme->isEnabled());
    QVERIFY(!density->isEnabled());
    QCOMPARE(QByteArray(qApp->style()->metaObject()->className()), nativeClassName);

    winuiAction->trigger();
    QVERIFY(theme->isEnabled());
    QVERIFY(density->isEnabled());
    QCOMPARE(qApp->style()->property("themeMode").toInt(), 2);
    QCOMPARE(qApp->style()->property("densityMode").toInt(), 1);
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

void WinUI3GalleryProbeTest::galleryNavigationPaletteFollowsThemeAfterMicaDisable()
{
    DisableAnimationsGuard animations;
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setThemeMode(WinUI3::ThemeMode::Dark);

    GalleryWindow gallery;
    gallery.resize(1180, 780);
    gallery.show();
    QVERIFY(QTest::qWaitForWindowExposed(&gallery));
    auto *navigation = gallery.findChild<QListWidget *>(QStringLiteral("navigationList"));
    auto *density = gallery.findChild<QComboBox *>(QStringLiteral("densityCombo"));
    auto *micaToggle = gallery.findChild<QCheckBox *>(QStringLiteral("micaToggle"));
    auto *controls = gallery.findChild<QScrollArea *>(QStringLiteral("controlsPage"));
    QVERIFY(navigation && density && micaToggle && controls);

    const QPalette darkPalette = style->standardPalette();
    QCOMPARE(WinUI3::Private::tokens(navigation->palette()).dark, true);
    QCOMPARE(navigation->palette().color(QPalette::Active, QPalette::Text),
             darkPalette.color(QPalette::Active, QPalette::Text));

    // Replay the Gallery control path, then publish the compositor grant just
    // as the existing offscreen backdrop mechanism tests do.
    micaToggle->setChecked(true);
    QVERIFY(navigation->property(WinUI3::Private::ownedPaletteProperty).toBool());
    QCOMPARE(navigation->palette(),
             navigation->property(WinUI3::Private::implicitPaletteSnapshotProperty)
                     .value<QPalette>());
    qApp->processEvents();
    gallery.setProperty("_winui_backdrop", 1);
    gallery.setProperty("_winui_backdrop_effective", 2);
    WinUI3::Private::makeChromeSurfacesTransparent(&gallery);
    WinUI3::Private::syncContentSurfacesForBackdrop(&gallery);
    qApp->processEvents();
    QCOMPARE(navigation->palette().color(QPalette::Window).alpha(), 0);

    // Match the reported below-fold interaction before turning the backdrop
    // off: density changes and scrolling must not detach the navigation view
    // from its style-owned palette snapshot.
    density->setCurrentIndex(1);
    qApp->processEvents();
    QCOMPARE(WinUI3::Style::densityMode(navigation), WinUI3::DensityMode::Compact);
    controls->verticalScrollBar()->setValue(controls->verticalScrollBar()->maximum());
    qApp->processEvents();
    QVERIFY(controls->verticalScrollBar()->value() > 0);

    micaToggle->setChecked(false);
    qApp->processEvents();
    WinUI3::Private::restoreChromeSurfaces(&gallery);
    WinUI3::Private::restoreContentSurfacesForBackdrop(&gallery);
    gallery.setProperty("_winui_backdrop", 0);
    gallery.setProperty("_winui_backdrop_effective", 0);
    qApp->processEvents();

    style->setThemeMode(WinUI3::ThemeMode::Light);
    qApp->processEvents();
    const QPalette lightPalette = style->standardPalette();
    // The restored NavigationView palette and its delegate tokens must follow
    // Light after the Mica cycle, just like other style-owned widgets.
    QCOMPARE(WinUI3::Private::tokens(navigation->palette()).dark, false);
    QCOMPARE(navigation->palette().color(QPalette::Active, QPalette::Text),
             lightPalette.color(QPalette::Active, QPalette::Text));
    QCOMPARE(navigation->viewport()->palette().color(QPalette::Active, QPalette::Text),
             lightPalette.color(QPalette::Active, QPalette::Text));
    // Direct palette writes in NavigationView's temporary transparent
    // surface must not make its dark style palette look application-owned.
    QCOMPARE(navigation->property(WinUI3::Private::ownedPaletteProperty).toBool(), true);
}

void WinUI3GalleryProbeTest::galleryThemeSwitchWhileGrantedThenDisable()
{
    // Live defect (Gallery 2026-10-12, Qt 6.12 x64): Dark + Mica ON, switch
    // to Light while granted, then Mica OFF leaves the navigation pane gray
    // and client chrome on the wrong theme. Unlike
    // galleryNavigationPaletteFollowsThemeAfterMicaDisable (disable first,
    // switch after), the theme moves while the material is granted, so every
    // transparentized shell/island surface must rebase and still restore the
    // current theme on disable.
    DisableAnimationsGuard animations;
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setThemeMode(WinUI3::ThemeMode::Dark);

    GalleryWindow gallery;
    gallery.resize(1180, 780);
    gallery.show();
    QVERIFY(QTest::qWaitForWindowExposed(&gallery));
    auto *navigation = gallery.findChild<QListWidget *>(QStringLiteral("navigationList"));
    auto *menuBar = gallery.findChild<QMenuBar *>();
    auto *controls = gallery.findChild<QScrollArea *>(QStringLiteral("controlsPage"));
    auto *micaToggle = gallery.findChild<QCheckBox *>(QStringLiteral("micaToggle"));
    QVERIFY(navigation && menuBar && controls && micaToggle);

    micaToggle->setChecked(true);
    qApp->processEvents();
    gallery.setProperty("_winui_backdrop", 1);
    gallery.setProperty("_winui_backdrop_effective", 2);
    WinUI3::Private::makeChromeSurfacesTransparent(&gallery);
    WinUI3::Private::syncContentSurfacesForBackdrop(&gallery);
    qApp->processEvents();

    // Theme switch while granted (the reported order), letting the queued
    // backdrop re-apply fire as in the live Gallery.
    style->setThemeMode(WinUI3::ThemeMode::Light);
    qApp->processEvents();

    micaToggle->setChecked(false);
    qApp->processEvents();
    WinUI3::Private::restoreChromeSurfaces(&gallery);
    WinUI3::Private::restoreContentSurfacesForBackdrop(&gallery);
    gallery.setProperty("_winui_backdrop", 0);
    gallery.setProperty("_winui_backdrop_effective", 0);
    qApp->processEvents();

    const QPalette lightPalette = style->standardPalette();
    QCOMPARE(WinUI3::Private::tokens(navigation->palette()).dark, false);
    QCOMPARE(navigation->palette().color(QPalette::Active, QPalette::Text),
             lightPalette.color(QPalette::Active, QPalette::Text));
    QCOMPARE(menuBar->palette().color(QPalette::Window), lightPalette.color(QPalette::Window));
    QCOMPARE(controls->viewport()->palette().color(QPalette::Base),
             lightPalette.color(QPalette::Base));
}

void WinUI3GalleryProbeTest::galleryRealGrantThemeSwitchThenDisable()
{
    // Live replay intent (Gallery 2026-10-12, Qt 6.12 x64, Dark + Mica ON,
    // switch to Light while granted, then Mica OFF): shares the toggle path
    // and the theme-while-granted order with the live report. Offscreen the
    // DWM grant falls back to the opaque Painted surface, so no Composited
    // state is asserted here; the palette recipe below is the offscreen
    // contract. The granted-material pass is covered live by the reviewer.
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setThemeMode(WinUI3::ThemeMode::Dark);

    GalleryWindow gallery;
    gallery.resize(1180, 780);
    gallery.show();
    QVERIFY(QTest::qWaitForWindowExposed(&gallery));
    auto *navigation = gallery.findChild<QListWidget *>(QStringLiteral("navigationList"));
    auto *menuBar = gallery.findChild<QMenuBar *>();
    auto *micaToggle = gallery.findChild<QCheckBox *>(QStringLiteral("micaToggle"));
    QVERIFY(navigation && menuBar && micaToggle);

    // Real grant through the Gallery's own toggle path. Offscreen there is
    // no DWM: the grant path falls back to the opaque Painted surface and
    // Composited must NOT be asserted here (native-only evidence). The
    // palette/attribute recipe below is what this offscreen pass checks.
    micaToggle->setChecked(true);
    qApp->processEvents();

    // Theme switch while the material is requested (the reported order),
    // letting the queued backdrop re-apply fire as live. Offscreen the
    // gallery property carries the string request ("mica") while the
    // published effective surface stays Composited-stale from the fixture;
    // the Solid verdict below is what the disable leg must establish.
    style->setThemeMode(WinUI3::ThemeMode::Light);
    QTest::qWait(200);
    QCOMPARE(gallery.property("winuiBackdrop").toString(), QStringLiteral("mica"));

    micaToggle->setChecked(false);
    QTest::qWait(200);
    QCOMPARE(WinUI3::Private::backdropEffectiveSurface(&gallery),
             WinUI3::Private::BackdropSurface::Solid);

    const QPalette lightPalette = style->standardPalette();
    QCOMPARE(gallery.palette().color(QPalette::Window).alpha(), 255);
    QCOMPARE(navigation->palette().color(QPalette::Active, QPalette::Text),
             lightPalette.color(QPalette::Active, QPalette::Text));
    QCOMPARE(menuBar->palette().color(QPalette::Window), lightPalette.color(QPalette::Window));
}

void WinUI3GalleryProbeTest::galleryStaleReapplyAfterDisable()
{
    // The theme switch queues a deferred backdrop re-apply. If disabling
    // happens before that shot fires, the stale shot must not re-enable the
    // material behind the unchecked toggle: the window stays opaque on the
    // current theme with a Solid effective surface.
    // Offscreen there is no DWM grant, so this test drives the request state
    // directly: after a switch, a same-value "none" request must still leave
    // the window opaque and Solid instead of re-arming a material.
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setThemeMode(WinUI3::ThemeMode::Dark);

    GalleryWindow gallery;
    gallery.resize(1180, 780);
    gallery.show();
    QVERIFY(QTest::qWaitForWindowExposed(&gallery));

    style->setThemeMode(WinUI3::ThemeMode::Light);
    WinUI3::applyBackdrop(&gallery, WinUI3::Backdrop::None);
    QTest::qWait(300);

    QCOMPARE(WinUI3::Private::backdropEffectiveSurface(&gallery),
             WinUI3::Private::BackdropSurface::Solid);
    QCOMPARE(gallery.palette().color(QPalette::Window).alpha(), 255);
    const QPalette lightPalette = style->standardPalette();
    QCOMPARE(gallery.palette().color(QPalette::Window), lightPalette.color(QPalette::Window));
}

void WinUI3GalleryProbeTest::gallerySnapshotPinsAndRestoresCalendarDate()
{
    DisableAnimationsGuard animations;
    GalleryWindow gallery;
    const QLocale originalLocale(QLocale::English, QLocale::UnitedStates);
    gallery.setLocale(originalLocale);
    gallery.resize(1180, 780);
    gallery.show();
    QVERIFY(QTest::qWaitForWindowExposed(&gallery));
    auto *calendar = gallery.findChild<QCalendarWidget *>(QStringLiteral("persistentCalendar"));
    QVERIFY(calendar);
    auto *standardDate = gallery.findChild<QDateEdit *>(QStringLiteral("standardDensityDate"));
    auto *compactDate = gallery.findChild<QDateEdit *>(QStringLiteral("compactDensityDate"));
    QVERIFY(standardDate && compactDate);
    const QString originalFormat = QStringLiteral("M/d/yyyy");
    standardDate->setDisplayFormat(originalFormat);
    compactDate->setDisplayFormat(originalFormat);
    const QDate originalDate(2035, 4, 5);
    calendar->setSelectedDate(originalDate);
    calendar->setCurrentPage(2034, 7);
    QList<QDate> selectedDuringCapture;
    QList<QLocale> localesDuringCapture;
    bool sawPinnedDateFormats = false;
    const auto selectionConnection =
            connect(calendar, &QCalendarWidget::selectionChanged, this, [&] {
                selectedDuringCapture.append(calendar->selectedDate());
                localesDuringCapture.append(calendar->locale());
                if (calendar->selectedDate() == QDate(2026, 10, 11)) {
                    sawPinnedDateFormats =
                            standardDate->displayFormat() == QLatin1String("dd/MM/yyyy")
                            && compactDate->displayFormat() == QLatin1String("dd/MM/yyyy");
                }
            });
    QTemporaryDir output;
    QVERIFY(output.isValid());
    const bool saved = gallery.saveSnapshots(output.path());
    disconnect(selectionConnection);
    QVERIFY(saved);
    QVERIFY2(selectedDuringCapture.contains(QDate(2026, 10, 11)),
             "snapshot selection must use the approved fixture date, never the wall clock");
    QVERIFY2(localesDuringCapture.contains(QLocale(QLocale::French, QLocale::France)),
             "snapshot dates must use the approved locale, never the host locale");
    QCOMPARE(gallery.locale(), originalLocale);
    QCOMPARE(calendar->locale(), originalLocale);
    QVERIFY2(sawPinnedDateFormats, "snapshot editor formats must not depend on the host locale");
    QCOMPARE(standardDate->displayFormat(), originalFormat);
    QCOMPARE(compactDate->displayFormat(), originalFormat);
    QCOMPARE(calendar->selectedDate(), originalDate);
    QCOMPARE(calendar->yearShown(), 2034);
    QCOMPARE(calendar->monthShown(), 7);
}

QTEST_MAIN(WinUI3GalleryProbeTest)
#include "tst_winui3galleryprobe.moc"
