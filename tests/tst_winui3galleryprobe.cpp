// SPDX-License-Identifier: LGPL-2.1-or-later
// Exact-construction live repro: the REAL GalleryWindow class (the same
// src/gallery/gallerywindow.cpp/.h/.ui sources the reviewer runs live) driven
// through the live Mica-toggle property path ("winuiBackdrop").
// Offscreen-capable: the DWM grant is simulated the same way the micacard
// mechanism tests do (Composited publish + chrome/content syncs in live
// order). Controls page scrolled to the below-fold Density rows 7-9
// (Navigation/MenuBar/Password) + Palette Lab page visit; exact widget
// rects probed against a NONE twin. Every pixel assert pairs with a
// token/geometry assert on the same state.
#include "../src/gallery/gallerywindow.h"
#include "winui3testhelpers.h"

#include "../src/lib/winui3surfaces_p.h"
#include "../src/lib/winui3paint_p.h"
#include "../src/lib/winui3tokens_p.h"

#include <QComboBox>
#include <QCheckBox>
#include <QCommandLinkButton>
#include <QCalendarWidget>
#include <QDateEdit>
#include <QFrame>
#include <QGroupBox>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QMenuBar>
#include <QMenu>
#include <QMouseEvent>
#include <QPushButton>
#include <QRubberBand>
#include <QScrollArea>
#include <QScrollBar>
#include <QStackedWidget>
#include <QStyleFactory>
#include <QTabWidget>
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
    void gallerySettingsExpanderAnimates();
    void galleryRubberBandReachable();
    void settingsIconAlignment_data();
    void settingsIconAlignment();
    void settingsExpansionMargins_data();
    void settingsExpansionMargins();
    void settingsExpansionSeparator_data();
    void settingsExpansionSeparator();
    void settingsChevronForeground_data();
    void settingsChevronForeground();
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
        // Palette Lab page: the editor keeps its own fill, while the preview
        // panel stays on the bare material (uniform-mica direction).
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
        // Dialogs page: Qt translates a command link's painted contents by
        // PM_ButtonShiftHorizontal/Vertical on press, so the real Gallery
        // instance must report 0 on both axes (the pixel proof is
        // commandLinkPressKeepsContentPosition; this is the same mechanism on
        // the widget the reviewer actually sees).
        auto *dialogsPage = mica.findChild<QScrollArea *>(QStringLiteral("dialogsPage"));
        auto *commandLink =
                mica.findChild<QCommandLinkButton *>(QStringLiteral("commandLinkButton"));
        auto *disabledLink =
                mica.findChild<QCommandLinkButton *>(QStringLiteral("disabledCommandLink"));
        QVERIFY(dialogsPage && commandLink && disabledLink);
        QCOMPARE(commandLink->isEnabled(), true);
        QCOMPARE(disabledLink->isEnabled(), false);
        QCOMPARE(commandLink->style()->pixelMetric(QStyle::PM_ButtonShiftHorizontal, nullptr,
                                                   commandLink),
                 0);
        QCOMPARE(commandLink->style()->pixelMetric(QStyle::PM_ButtonShiftVertical, nullptr,
                                                   commandLink),
                 0);
        QCOMPARE(disabledLink->style()->pixelMetric(QStyle::PM_ButtonShiftHorizontal, nullptr,
                                                    disabledLink),
                 0);
        QCOMPARE(disabledLink->style()->pixelMetric(QStyle::PM_ButtonShiftVertical, nullptr,
                                                    disabledLink),
                 0);
        pages->setCurrentWidget(dialogsPage);
        dialogsPage->ensureWidgetVisible(commandLink);
        qApp->processEvents();
        const QRect linkRect = commandLink->rect();
        const QImage linkRest = commandLink->grab().toImage();
        QTest::mousePress(commandLink, Qt::LeftButton, Qt::NoModifier, linkRect.center());
        QVERIFY(commandLink->isDown());
        const QImage linkPressed = commandLink->grab().toImage();
        QTest::mouseRelease(commandLink, Qt::LeftButton, Qt::NoModifier, linkRect.center());
        QVERIFY(!commandLink->isDown());
        // The press surface is real, and the widget never re-lays out.
        QVERIFY(linkPressed != linkRest);
        QCOMPARE(commandLink->rect(), linkRect);
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

void WinUI3GalleryProbeTest::gallerySettingsExpanderAnimates()
{
    // The Designer composition must animate, not jump: click the Advanced
    // header in the real gallery and sample the expansion mid-flight.
    QVERIFY(!qEnvironmentVariableIsSet("WINUI3STYLE_DISABLE_ANIMATIONS"));
    GalleryWindow window;
    window.resize(1180, 780);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto *pages = window.findChild<QStackedWidget *>(QStringLiteral("pages"));
    auto *settingsPage = window.findChild<QScrollArea *>(QStringLiteral("settingsPage"));
    auto *card = window.findChild<QFrame *>(QStringLiteral("advancedCard"));
    auto *header = window.findChild<QWidget *>(QStringLiteral("advancedHeader"));
    auto *title = window.findChild<QLabel *>(QStringLiteral("advancedTitle"));
    auto *details = window.findChild<QWidget *>(QStringLiteral("advancedDetailsHost"));
    QVERIFY(pages && settingsPage && card && header && title && details);
    pages->setCurrentWidget(settingsPage);
    qApp->processEvents();

    QCOMPARE(card->property(WinUI3::Style::SettingsCardProperty).toBool(), true);
    QCOMPARE(card->property(WinUI3::Style::SettingsCardExpandedProperty).toBool(), false);
    QCOMPARE(details->maximumHeight(), 0);
    const int contentHeight = details->sizeHint().height();
    const int collapsedHeight = card->height();
    QVERIFY(contentHeight > 0);

    // Click the header label, not the frame: this is the live path where Qt
    // propagates an ignored press up through the nested header widget.
    QTest::mouseClick(title, Qt::LeftButton, Qt::NoModifier, title->rect().center());
    QTest::qWait(80);
    const qreal mid = frameReal(card, "_winui_card_expansion_progress");
    QVERIFY2(mid > 0.0 && mid < 1.0, qPrintable(QString::number(mid)));
    QVERIFY2(details->maximumHeight() > 0 && details->maximumHeight() < contentHeight,
             qPrintable(QString::number(details->maximumHeight())));
    QVERIFY2(card->height() > collapsedHeight
                     && card->height() < collapsedHeight + details->sizeHint().height(),
             qPrintable(QStringLiteral("%1 -> %2").arg(collapsedHeight).arg(card->height())));
    QTRY_COMPARE_WITH_TIMEOUT(frameReal(card, "_winui_card_expansion_progress"), 1.0, 1500);
    QCOMPARE(details->maximumHeight(), QWIDGETSIZE_MAX);
    // The wrapped QLabel's cached sizeHint can disagree with the height the
    // layout actually needs; the contract is that the expanded host is at
    // least its minimum (nothing clipped) and the card grew by that height.
    QVERIFY2(details->height() >= details->minimumSizeHint().height(),
             qPrintable(QStringLiteral("%1 < %2")
                                .arg(details->height())
                                .arg(details->minimumSizeHint().height())));
    QCOMPARE(card->height(), details->y() + details->height());
}

void WinUI3GalleryProbeTest::galleryRubberBandReachable()
{
    GalleryWindow window;
    window.resize(1180, 780);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto *pages = window.findChild<QStackedWidget *>(QStringLiteral("pages"));
    auto *collections = window.findChild<QScrollArea *>(QStringLiteral("collectionsPage"));
    auto *tabs = window.findChild<QTabWidget *>(QStringLiteral("collectionsTabs"));
    auto *list = window.findChild<QListWidget *>(QStringLiteral("listViewTab"));
    QVERIFY(pages && collections && tabs && list);
    pages->setCurrentWidget(collections);
    tabs->setCurrentWidget(list);
    qApp->processEvents();
    QCOMPARE(list->selectionMode(), QAbstractItemView::ExtendedSelection);
    // QListView only rubbers up in IconMode unless this is set explicitly.
    QCOMPARE(list->isSelectionRectVisible(), true);
    QVERIFY(list->viewport()->height() > 160);

    const QPoint start(24, list->viewport()->height() - 24);
    const QPoint end(340, list->viewport()->height() - 110);
    QTest::mousePress(list->viewport(), Qt::LeftButton, Qt::NoModifier, start);
    QMouseEvent move(QEvent::MouseMove, QPointF(end), QPointF(list->viewport()->mapToGlobal(end)),
                     Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(list->viewport(), &move);
    QTest::qWait(40);
    // Qt 6 does not create a QRubberBand widget for item views: QListView
    // paints CE_RubberBand itself while it is in DragSelectingState.
    const QImage image = list->viewport()->grab().toImage();
    const auto tokens = WinUI3::Private::tokens(list->palette());
    QRect painted;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (colorDistance(image.pixelColor(x, y), tokens.selectionAccent) <= 8)
                painted |= QRect(x, y, 1, 1);
        }
    }
    QTest::mouseRelease(list->viewport(), Qt::LeftButton, Qt::NoModifier, end);
    // The accent outline follows the dragged rectangle (Qt insets the
    // rectangle by 1 px before stroking the top and bottom edges).
    QVERIFY2(painted.isValid(), "no accent pixels: the marquee is invisible");
    const QRect expected = QRect(start, end).normalized();
    QCOMPARE(painted.left(), expected.left());
    QCOMPARE(painted.right(), expected.right());
    QVERIFY(qAbs(painted.top() - expected.top()) <= 1);
    QVERIFY(qAbs(painted.bottom() - expected.bottom()) <= 1);
}

// Issue #56: exercise the actual Designer composition, not a reconstructed card.
namespace {
void settingsVisualRows()
{
    QTest::addColumn<bool>("dark");
    QTest::addColumn<bool>("compact");
    QTest::addColumn<bool>("rtl");
    for (bool dark : { false, true })
        for (bool compact : { false, true })
            for (bool rtl : { false, true })
                QTest::newRow(
                        qPrintable(QStringLiteral("%1-%2-%3").arg(dark).arg(compact).arg(rtl)))
                        << dark << compact << rtl;
}
void showSettings(GalleryWindow &window, bool dark, bool compact, bool rtl)
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    style->setThemeMode(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
    style->setDensityMode(compact ? WinUI3::DensityMode::Compact : WinUI3::DensityMode::Standard);
    window.setLayoutDirection(rtl ? Qt::RightToLeft : Qt::LeftToRight);
    window.resize(1180, 780);
    window.show();
    window.findChild<QStackedWidget *>()->setCurrentWidget(
            window.findChild<QWidget *>("settingsPage"));
    QCoreApplication::processEvents();
}
} // namespace
void WinUI3GalleryProbeTest::settingsIconAlignment_data()
{
    settingsVisualRows();
}
void WinUI3GalleryProbeTest::settingsIconAlignment()
{
    QFETCH(bool, dark);
    QFETCH(bool, compact);
    QFETCH(bool, rtl);
    GalleryWindow window;
    showSettings(window, dark, compact, rtl);
    for (const auto &names : { qMakePair("notificationsCard", "notificationsCardIcon"),
                               qMakePair("updatesCard", "updatesCardIcon"),
                               qMakePair("advancedCard", "advancedIcon") }) {
        auto *card = window.findChild<QFrame *>(names.first);
        auto *icon = window.findChild<QLabel *>(names.second);
        const int x = icon->mapTo(card, QPoint()).x();
        QCOMPARE(icon->size(), QSize(20, 20));
        // SettingsCardborder 1 + Padding.Left 16 + HeaderIconMargin.Left 2.
        QCOMPARE(rtl ? card->width() - x - icon->width() : x, 19);
        const QImage image = card->grab().toImage();
        const QRect bounds(icon->mapTo(card, QPoint()), icon->size());
        const QColor surface = image.pixelColor(bounds.left() - 1, bounds.center().y());
        int pixels = 0;
        for (int y = bounds.top(); y <= bounds.bottom(); ++y)
            for (int x = bounds.left(); x <= bounds.right(); ++x)
                pixels += colorDistance(image.pixelColor(x, y), surface) > 8;
        QVERIFY(pixels > 6);
    }
}
void WinUI3GalleryProbeTest::settingsExpansionMargins_data()
{
    settingsVisualRows();
}
void WinUI3GalleryProbeTest::settingsExpansionMargins()
{
    QFETCH(bool, dark);
    QFETCH(bool, compact);
    QFETCH(bool, rtl);
    GalleryWindow window;
    showSettings(window, dark, compact, rtl);
    auto *card = window.findChild<QFrame *>("advancedCard");
    auto *host = window.findChild<QWidget *>("advancedDetailsHost");
    card->setProperty("winuiSettingsCardExpanded", true);
    QTRY_VERIFY(host->maximumHeight() == QWIDGETSIZE_MAX);
    QTRY_VERIFY(host->height() >= 51);
    // Item padding + content border + the template's -2 presenter overlap.
    QCOMPARE(host->layout()->contentsMargins(),
             rtl ? QMargins(45, 7, 59, 9) : QMargins(59, 7, 45, 9));
    auto *label = window.findChild<QLabel *>("advancedDetails");
    QCOMPARE(label->geometry().center().y(), (host->height() - 9 + 7 - 1) / 2);
    QCOMPARE(label->mapTo(host, QPoint()).x(), rtl ? 45 : 59);
    auto *title = window.findChild<QLabel *>("advancedTitle");
    const int contentLeading = label->mapTo(card, QPoint()).x() + (rtl ? label->width() : 0);
    const int titleLeading = title->mapTo(card, QPoint()).x() + (rtl ? title->width() : 0);
    QCOMPARE(contentLeading, titleLeading);
    QCOMPARE(host->minimumSizeHint().height(), 51);
    QVERIFY(host->height() >= 51);
    QVERIFY(window.findChild<QWidget *>("advancedHeader")->height() >= 68);
}
void WinUI3GalleryProbeTest::settingsExpansionSeparator_data()
{
    settingsVisualRows();
}
void WinUI3GalleryProbeTest::settingsExpansionSeparator()
{
    QFETCH(bool, dark);
    QFETCH(bool, compact);
    QFETCH(bool, rtl);
    GalleryWindow window;
    showSettings(window, dark, compact, rtl);
    auto *card = window.findChild<QFrame *>("advancedCard");
    auto *host = window.findChild<QWidget *>("advancedDetailsHost");
    card->setProperty("winuiSettingsCardExpanded", true);
    QTRY_VERIFY(host->height() > 0);
    const int seam = host->y() - 1;
    QCOMPARE(seam, window.findChild<QWidget *>("advancedHeader")->height() - 1);
    const QImage image = card->grab().toImage();
    const int x = card->width() / 2;
    QImage expected(1, 1, QImage::Format_ARGB32_Premultiplied);
    expected.fill(image.pixelColor(x, seam - 2));
    QPainter painter(&expected);
    painter.fillRect(expected.rect(), QColor(0, 0, 0, dark ? 25 : 15));
    painter.end();
    QCOMPARE(image.pixelColor(x, seam), expected.pixelColor(0, 0));
    // The hairline is exactly one row and does not cover content.
    QCOMPARE(image.pixelColor(x, seam - 1), image.pixelColor(x, seam - 2));
}
void WinUI3GalleryProbeTest::settingsChevronForeground_data()
{
    settingsVisualRows();
}
void WinUI3GalleryProbeTest::settingsChevronForeground()
{
    QFETCH(bool, dark);
    QFETCH(bool, compact);
    QFETCH(bool, rtl);
    GalleryWindow window;
    showSettings(window, dark, compact, rtl);
    auto *card = window.findChild<QFrame *>("advancedCard");
    auto *header = window.findChild<QWidget *>("advancedHeader");
    for (bool enabled : { true, false }) {
        card->setEnabled(enabled);
        QCoreApplication::processEvents();
        const auto t = WinUI3::Private::tokens(card->palette());
        const QColor ink = enabled ? t.textPrimary : t.textDisabled;
        QCOMPARE(ink.alpha(), enabled ? (dark ? 255 : 228) : (dark ? 93 : 92));
        const QRect logical(card->width() - 9 - 32, (header->height() - 32) / 2, 32, 32);
        const QRect box = QStyle::visualRect(card->layoutDirection(), header->rect(), logical);
        const QImage actual = card->grab().toImage();
        QImage expected(box.size(), QImage::Format_ARGB32_Premultiplied);
        expected.fill(actual.pixelColor(box.left() - 2, box.center().y()));
        QPainter painter(&expected);
        WinUI3::PaintPrivate::paintRotatedIcon(
                &painter, WinUI3::icon(WinUI3::Icon::ChevronDown, ink),
                QRectF(box.center().x() - 8 - box.left(), box.center().y() - 8 - box.top(), 16, 16),
                0.0, ink, enabled ? QIcon::Normal : QIcon::Disabled);
        painter.end();
        QCOMPARE(actual.copy(box), expected);
    }
}

QTEST_MAIN(WinUI3GalleryProbeTest)
#include "tst_winui3galleryprobe.moc"
