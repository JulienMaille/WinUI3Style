// SPDX-License-Identifier: LGPL-2.1-or-later
// Mica backdrop binary: shell scroll-chain, chrome loop, tab-pane card veil,
// group-card child fill, alternate-row and
// selected-tab erase-before-fill, plus rich-text frame, alternate-row,
// tab-pressed and scrollbar-track veils. Split out of tst_winui3surfaces.cpp /
// tst_winui3views.cpp per the 60 KB test-size ratchet. Theme-switch lifecycle
// (rebase, disable, re-apply) lives in tst_winui3micatheme.cpp so this file
// stays under the ratchet. Every
// grab()/pixelColor() assertion pairs with a QCOMPARE token/geometry
// assertion on the same state.
#include <winui3style/winui3backdrop.h>
#include <winui3style/winui3style.h>

#include "winui3testhelpers.h"

#include "../src/winui3surfaces_p.h"
#include "../src/winui3tokens_p.h"

#include <QStyleOptionDockWidget>

class WinUI3MicaBackdropTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void shellScrollChainSharesMaterial();
    void chromeLoopCoversAllMenuBars();
    void tabPaneCardLinksStayOpaqueWhileIslandSharesMaterial();
    void tabPaneCardVeilsOverComposited();
    void groupCardChildKeepsCardFillOverComposited();
    void comboPopupViewportRebuildsPopupTint();
    void standardButtonInsideCardKeepsCardFillOverComposited();
    void alternateRowErasesBeforeFill();
    void selectedTabErasesBeforeOpaqueFill();
    void settingsCardVeilsOverComposited();
    void dockTitleVeilsOverComposited();
    void headerRestVeilsOverComposited();
    void tabHoverPillVeilsOverComposited();
    void richTextFrameVeilsOverComposited();
    void alternateRowVeilsOverComposited();
    void tabPressedPillVeilsOverComposited();
    void scrollBarHoverTrackVeilsOverComposited();
    void bareStyledPanelSyncSharesMaterial();
    void lateScrollChainArmedAtPolish();
    void reparentedChainReconvergesWithoutSync();
};

void WinUI3MicaBackdropTest::initTestCase()
{
    qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light));
}

void WinUI3MicaBackdropTest::init()
{
    if (auto *style = qobject_cast<WinUI3::Style *>(qApp->style())) {
        style->setThemeMode(WinUI3::ThemeMode::Light);
        style->setAccentColor({});
    }
}

void WinUI3MicaBackdropTest::cleanup()
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

void WinUI3MicaBackdropTest::shellScrollChainSharesMaterial()
{
    // Bare QListWidget under the centralWidget/navigationPanel shell: the
    // chain arrives armed from polish (area, viewport, both bars share the
    // material with the no-fill recipe); the explicit sync below is
    // idempotent and toggle-off restore returns every link.
    DisableAnimationsGuard animations;
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QMainWindow window;
    window.setProperty("_winui_backdrop", 1);
    window.setProperty("_winui_backdrop_effective", 2); // Composited
    auto *central = new QWidget(&window);
    central->setObjectName(QStringLiteral("centralWidget"));
    window.setCentralWidget(central);
    auto *nav = new QWidget(central);
    nav->setObjectName(QStringLiteral("navigationPanel"));
    auto *list = new QListWidget(central);
    for (int row = 0; row < 30; ++row)
        list->addItem(QStringLiteral("Row %1").arg(row));
    list->resize(200, 140);
    window.resize(480, 320);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QCOMPARE(list->palette().color(QPalette::Window).alpha(), 0);
    WinUI3::Private::syncContentSurfacesForBackdrop(&window);
    QCOMPARE(central->palette().color(QPalette::Window).alpha(), 0);
    QCOMPARE(nav->palette().color(QPalette::Window).alpha(), 0);
    QCOMPARE(list->palette().color(QPalette::Window).alpha(), 0);
    QCOMPARE(list->viewport()->palette().color(QPalette::Window).alpha(), 0);
    QCOMPARE(list->verticalScrollBar()->palette().color(QPalette::Window).alpha(), 0);
    QCOMPARE(list->horizontalScrollBar()->palette().color(QPalette::Window).alpha(), 0);
    QCOMPARE(list->autoFillBackground(), false);
    QCOMPARE(list->viewport()->autoFillBackground(), false);
    QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(list->viewport()));
    const QImage frame = window.grab().toImage();
    QVERIFY(!frame.isNull());
    QCOMPARE(frame.size(), window.size());
    WinUI3::Private::restoreContentSurfacesForBackdrop(&window);
    QCOMPARE(list->palette().color(QPalette::Window).alpha(), 255);
    QCOMPARE(list->viewport()->palette().color(QPalette::Window).alpha(), 255);
    QCOMPARE(list->verticalScrollBar()->palette().color(QPalette::Window).alpha(), 255);
    QCOMPARE(list->horizontalScrollBar()->palette().color(QPalette::Window).alpha(), 255);
}

void WinUI3MicaBackdropTest::chromeLoopCoversAllMenuBars()
{
    // Chrome loop (menu bars, tool bars, status bar) visits EVERY bar, not
    // just the first: the main-window bar and a nested bar arrive armed from
    // polish and toggle-off restore returns every bar opaque. Driven through
    // the public theme refresh (which must preserve the armed recipe) and
    // the public applyBackdrop(None) teardown; same state, token asserts.
    DisableAnimationsGuard animations;
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QMainWindow window;
    window.setProperty("_winui_backdrop", 1);
    window.setProperty("_winui_backdrop_effective", 2); // Composited
    QMenuBar *menuBar = window.menuBar();
    auto *nestedHost = new QWidget(&window);
    auto *nestedBar = new QMenuBar(nestedHost);
    QToolBar *toolBar = window.addToolBar(QStringLiteral("Tools"));
    QStatusBar *statusBar = window.statusBar();
    window.resize(480, 320);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QCOMPARE(menuBar->palette().color(QPalette::Window).alpha(), 0);
    QCOMPARE(nestedBar->palette().color(QPalette::Window).alpha(), 0);
    style->setThemeMode(WinUI3::ThemeMode::Dark);
    const QList<QMenuBar *> bars = window.findChildren<QMenuBar *>();
    QCOMPARE(bars.size(), 2);
    for (QMenuBar *bar : bars)
        QCOMPARE(bar->palette().color(QPalette::Window).alpha(), 0);
    QCOMPARE(toolBar->palette().color(QPalette::Window).alpha(), 0);
    QCOMPARE(statusBar->palette().color(QPalette::Window).alpha(), 0);
    const QImage chromeGrab = menuBar->grab().toImage();
    QVERIFY(!chromeGrab.isNull());
    QCOMPARE(chromeGrab.width(), menuBar->width());
    QVERIFY(WinUI3::applyBackdrop(&window, WinUI3::Backdrop::None));
    for (QMenuBar *bar : window.findChildren<QMenuBar *>())
        QCOMPARE(bar->palette().color(QPalette::Window).alpha(), 255);
    QCOMPARE(toolBar->palette().color(QPalette::Window).alpha(), 255);
    QCOMPARE(statusBar->palette().color(QPalette::Window).alpha(), 255);
    style->setThemeMode(WinUI3::ThemeMode::Light);
}

void WinUI3MicaBackdropTest::tabPaneCardLinksStayOpaqueWhileIslandSharesMaterial()
{
    // Generic container links under a content island: the QTabWidget pane is
    // a card surface, so neither the tab widget nor its page is ever
    // transparentized (no StyledBackground write) while the island itself
    // still shares the material; the sibling group-box card keeps its veiled
    // fill by design (card keep-opaque gate).
    DisableAnimationsGuard animations;
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget window;
    window.setProperty("_winui_backdrop", 1);
    window.setProperty("_winui_backdrop_effective", 2); // Composited
    auto *island = new QWidget(&window);
    island->setProperty(WinUI3::Style::SurfaceProperty, QStringLiteral("content"));
    auto *tabs = new QTabWidget(island);
    tabs->addTab(new QWidget, QStringLiteral("One"));
    auto *card = new QGroupBox(QStringLiteral("Card"), island);
    window.resize(480, 320);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    WinUI3::Private::syncContentSurfacesForBackdrop(&window);
    // The card links are never transparentized: no StyledBackground write on
    // the tab widget or its page. The resolved Window role still reads the
    // island's shared material through inheritance (never armed directly).
    QCOMPARE(tabs->testAttribute(Qt::WA_StyledBackground), false);
    QCOMPARE(tabs->widget(0)->testAttribute(Qt::WA_StyledBackground), false);
    QCOMPARE(tabs->palette().color(QPalette::Window).alpha(), 0);
    QCOMPARE(island->palette().color(QPalette::Window).alpha(), 0);
    // The card keeps its opaque fill by design: it is never transparentized
    // itself (no StyledBackground write). Its resolved Window role inherits
    // the island's transparent role, so assert the no-transparentize state
    // plus an opaque painted card instead of the inherited role value.
    QCOMPARE(card->testAttribute(Qt::WA_StyledBackground), false);
    QStyleOptionGroupBox groupOption;
    groupOption.initFrom(card);
    groupOption.rect = QRect(0, 0, 200, 120);
    groupOption.text = card->title();
    groupOption.state = QStyle::State_Enabled;
    QImage cardFrame(groupOption.rect.size(), QImage::Format_ARGB32_Premultiplied);
    cardFrame.fill(Qt::transparent);
    {
        QPainter painter(&cardFrame);
        style->drawComplexControl(QStyle::CC_GroupBox, &groupOption, &painter, card);
    }
    // Group cards veil over a live material by design (VeiledCardAlpha 178),
    // opaque only off it. Assert the veiled token ink, paired with the
    // no-transparentize state above.
    const QColor veiled =
            WinUI3::Private::veiledCard(WinUI3::Private::tokens(card->palette()).layer, true);
    QCOMPARE(cardFrame.pixelColor(cardFrame.rect().center()), veiled);
    const QImage tabsGrab = tabs->grab().toImage();
    QVERIFY(!tabsGrab.isNull());
    QCOMPARE(tabsGrab.width(), tabs->width());
    WinUI3::Private::restoreContentSurfacesForBackdrop(&window);
    QCOMPARE(tabs->palette().color(QPalette::Window).alpha(), 255);
    QCOMPARE(tabs->widget(0)->palette().color(QPalette::Window).alpha(), 255);
    QCOMPARE(card->palette().color(QPalette::Window).alpha(), 255);
}

void WinUI3MicaBackdropTest::tabPaneCardVeilsOverComposited()
{
    // Collections card: over a live material the QTabWidget pane is a card
    // surface, so PE_FrameTabWidget lays veiledCard(t.layer) (the group-card
    // recipe 1) instead of the recipe-2 Source-clear that left the pane at
    // alpha 0, and the page/list children are never transparentized. The
    // island gap around the card keeps its alpha-0 erase. Compact-parametrized:
    // both densities paint the same veiled card tone (radius contracts live in
    // the density tests). Every pixel assert pairs with a token/geometry
    // assert on the same state.
    DisableAnimationsGuard animations;
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget window;
    window.setProperty("_winui_backdrop", 1);
    window.setProperty("_winui_backdrop_effective", 2); // Composited
    auto *island = new QWidget(&window);
    island->setProperty(WinUI3::Style::SurfaceProperty, QStringLiteral("content"));
    island->setGeometry(0, 0, 480, 320);
    auto *tabs = new QTabWidget(island);
    tabs->setGeometry(40, 40, 300, 200);
    auto *page = new QWidget;
    tabs->addTab(page, QStringLiteral("Collections"));
    auto *list = new QListWidget(page);
    list->setGeometry(8, 8, 284, 100);
    list->addItem(QStringLiteral("Row 0"));
    window.resize(480, 320);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    WinUI3::Private::syncContentSurfacesForBackdrop(&window);
    // Token/geometry pairs for the pixel asserts below: the pane card tone
    // is the veiled Base layer (t.layer), at the exact card geometry.
    const QColor cardTone =
            WinUI3::Private::veiledCard(WinUI3::Private::tokens(tabs->palette()).layer, true);
    QCOMPARE(cardTone.alpha(), tabs->palette().color(QPalette::Base).alpha());
    QCOMPARE(tabs->geometry(), QRect(40, 40, 300, 200));
    // Mechanism: PE_FrameTabWidget over the live material paints the veiled
    // card tone, never a Source-clear (the RED path erased the pane rect to
    // alpha 0). Both densities, seeded red so the erase is observable.
    for (const int density : { 0, 1 }) {
        style->setProperty("densityMode", density);
        QStyleOption option;
        option.initFrom(tabs);
        option.rect = tabs->rect();
        option.state = QStyle::State_Enabled;
        QImage frame(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
        frame.fill(Qt::red);
        {
            QPainter painter(&frame);
            style->drawPrimitive(QStyle::PE_FrameTabWidget, &option, &painter, tabs);
        }
        const QColor pane = frame.pixelColor(frame.rect().center());
        QCOMPARE(pane.alpha(), cardTone.alpha());
        // Premultiplied rounding (±1 per channel) over the erased rect.
        QVERIFY(colorDistance(pane, cardTone) <= 4);
    }
    style->setProperty("densityMode", 0);
    // Mechanism: the island gap around the card still erases to alpha 0
    // (the gap contract above must not regress with the card fill).
    QImage gapCanvas(island->size(), QImage::Format_ARGB32_Premultiplied);
    gapCanvas.fill(Qt::red);
    {
        QStyleOption gapOption;
        gapOption.initFrom(island);
        gapOption.rect = island->rect();
        gapOption.state = QStyle::State_Enabled;
        QPainter painter(&gapCanvas);
        style->drawPrimitive(QStyle::PE_Widget, &gapOption, &painter, island);
    }
    QCOMPARE(gapCanvas.pixelColor(gapCanvas.rect().center()).alpha(), 0);
    // Render into an alpha-preserving ARGB canvas: QWidget::grab() flattens
    // into an opaque pixmap (Source-cleared rows would read opaque black),
    // so it cannot evidence the alpha-0 gap or a punched child viewport.
    // Same render path as grab() otherwise (window background, then the
    // widget tree including the StyledBackground PE_Widget dispatch).
    QImage grab(window.size(), QImage::Format_ARGB32_Premultiplied);
    grab.fill(Qt::transparent);
    window.render(&grab);
    QVERIFY(!grab.isNull());
    QCOMPARE(grab.size(), window.size());
    // Card band: the pane keeps the veiled card tone, not alpha 0.
    const QPoint cardProbe = tabs->geometry().bottomRight() - QPoint(20, 20);
    QVERIFY(QRect(QPoint(0, 0), grab.size()).contains(cardProbe));
    const QColor cardPixel = grab.pixelColor(cardProbe);
    QCOMPARE(cardPixel.alpha(), cardTone.alpha());
    QVERIFY(colorDistance(cardPixel, cardTone) <= 4);
    // Viewport: the list child never Source-clears the card fill.
    const QPoint viewportProbe = list->mapTo(&window, QPoint(20, 70));
    QVERIFY(QRect(QPoint(0, 0), grab.size()).contains(viewportProbe));
    QVERIFY(grab.pixelColor(viewportProbe).alpha() != 0);
    // Island gap around the card keeps the alpha-0 erase end-to-end (paired
    // with the island Window-role assert below).
    QVERIFY(QRect(QPoint(0, 0), grab.size()).contains(QPoint(10, 10)));
    QCOMPARE(grab.pixelColor(10, 10).alpha(), 0);
    // State pairs: neither the card nor its page is transparentized, while
    // the island itself still shares the material (alpha-0 gap above).
    QCOMPARE(tabs->testAttribute(Qt::WA_StyledBackground), false);
    QCOMPARE(page->testAttribute(Qt::WA_StyledBackground), false);
    QCOMPARE(island->palette().color(QPalette::Window).alpha(), 0);
    WinUI3::Private::restoreContentSurfacesForBackdrop(&window);
    QCOMPARE(tabs->palette().color(QPalette::Window).alpha(), 255);
    QCOMPARE(page->palette().color(QPalette::Window).alpha(), 255);
    QCOMPARE(island->palette().color(QPalette::Window).alpha(), 255);
}

void WinUI3MicaBackdropTest::groupCardChildKeepsCardFillOverComposited()
{
    // Dialogs-page card: children of an opaque QGroupBox card (splitter
    // list/editor viewports) must never Source-clear the card fill — the
    // slider/scrollbar card guard extended to the PE_Widget erase path and
    // the sync. The card lays veiledCard(t.layer) itself; a directly
    // dispatched child erase returns with the seeded card tone intact.
    // Every pixel assert pairs with a token/geometry assert on the same state.
    DisableAnimationsGuard animations;
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget window;
    window.setProperty("_winui_backdrop", 1);
    window.setProperty("_winui_backdrop_effective", 2); // Composited
    auto *island = new QWidget(&window);
    island->setProperty(WinUI3::Style::SurfaceProperty, QStringLiteral("content"));
    island->setGeometry(0, 0, 480, 320);
    auto *card = new QGroupBox(QStringLiteral("Card"), island);
    card->setGeometry(40, 40, 400, 240);
    auto *splitter = new QSplitter(Qt::Horizontal, card);
    splitter->setGeometry(20, 50, 360, 150);
    auto *list = new QListWidget;
    list->addItem(QStringLiteral("Row 0"));
    auto *edit = new QPlainTextEdit(QStringLiteral("Body"));
    splitter->addWidget(list);
    splitter->addWidget(edit);
    splitter->setSizes({ 180, 180 });
    window.resize(480, 320);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    WinUI3::Private::syncContentSurfacesForBackdrop(&window);
    // Token/geometry pairs: the shared gate still passes for the child (the
    // card carries no SurfaceProperty), so only the card guard can keep the
    // fill; the card tone is the veiled Base layer at the card's geometry.
    QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(list->viewport()));
    const QColor cardTone =
            WinUI3::Private::veiledCard(WinUI3::Private::tokens(card->palette()).layer, true);
    QCOMPARE(cardTone.alpha(), card->palette().color(QPalette::Base).alpha());
    QCOMPARE(splitter->geometry(), QRect(20, 50, 360, 150));
    // Mechanism: a directly dispatched PE_Widget erase for a child inside
    // the card returns without Source-clearing — the seeded card tone at
    // the canvas center survives (the RED path erased it to alpha 0).
    QImage canvas(400, 240, QImage::Format_ARGB32_Premultiplied);
    canvas.fill(cardTone);
    {
        QStyleOption option;
        option.initFrom(list->viewport());
        option.rect = QRect(QPoint(0, 0), canvas.size());
        option.state = QStyle::State_Enabled;
        QPainter painter(&canvas);
        style->drawPrimitive(QStyle::PE_Widget, &option, &painter, list->viewport());
    }
    QCOMPARE(canvas.pixelColor(canvas.rect().center()), cardTone);
    // Alpha-preserving render (grab() flattens into an opaque pixmap; see
    // the tab-pane twin above for the rationale).
    QImage grab(window.size(), QImage::Format_ARGB32_Premultiplied);
    grab.fill(Qt::transparent);
    window.render(&grab);
    QVERIFY(!grab.isNull());
    QCOMPARE(grab.size(), window.size());
    // Card band outside the children keeps the veiled card tone.
    const QPoint cardProbe = card->mapTo(&window, QPoint(card->width() - 10, card->height() - 20));
    QVERIFY(QRect(QPoint(0, 0), grab.size()).contains(cardProbe));
    const QColor band = grab.pixelColor(cardProbe);
    QCOMPARE(band.alpha(), cardTone.alpha());
    QVERIFY(colorDistance(band, cardTone) <= 4);
    // Child viewports keep the card fill: no Source-clear hole (RED: alpha 0).
    const QPoint listProbe = list->viewport()->mapTo(&window, QPoint(20, 70));
    QVERIFY(QRect(QPoint(0, 0), grab.size()).contains(listProbe));
    QVERIFY(grab.pixelColor(listProbe).alpha() != 0);
    const QPoint editProbe =
            edit->viewport()->mapTo(&window, QPoint(10, edit->viewport()->height() - 10));
    QVERIFY(QRect(QPoint(0, 0), grab.size()).contains(editProbe));
    QVERIFY(grab.pixelColor(editProbe).alpha() != 0);
    // State pairs: the card itself carries no StyledBackground/erase recipe;
    // in-card scroll viewports do (reviewer 2026-10-06 tree overlay): the
    // autofill stops so the Base layer cannot double over the card veil,
    // and the viewport repaints the card tone itself via PE (erase plus
    // Source refill). Roles stay intact for tone reads.
    QCOMPARE(card->testAttribute(Qt::WA_StyledBackground), false);
    QCOMPARE(list->viewport()->autoFillBackground(), false);
    QCOMPARE(list->viewport()->testAttribute(Qt::WA_StyledBackground), true);
    QCOMPARE(edit->viewport()->autoFillBackground(), false);
    QCOMPARE(edit->viewport()->testAttribute(Qt::WA_StyledBackground), true);
    WinUI3::Private::restoreContentSurfacesForBackdrop(&window);
    QCOMPARE(card->palette().color(QPalette::Window).alpha(), 255);
    // Restore mirror: viewport flags return to Qt defaults (autofill back,
    // no StyledBackground) so toggle-off leaves no armed paint path behind.
    QCOMPARE(list->viewport()->autoFillBackground(), true);
    QCOMPARE(list->viewport()->testAttribute(Qt::WA_StyledBackground), false);
    QCOMPARE(edit->viewport()->autoFillBackground(), true);
    QCOMPARE(edit->viewport()->testAttribute(Qt::WA_StyledBackground), false);
}

void WinUI3MicaBackdropTest::comboPopupViewportRebuildsPopupTint()
{
    // Composited combo popup (round-2 mark): PE_PanelMenu lays ONE popup
    // tint over the DWM acrylic and preparePopupSurface re-tints Window/Base
    // to it (178 Dark / 242 Light), so the popup's own view/viewport
    // PE_Widget erase must refill that same tint (recipe 1). Otherwise the
    // Source-cleared viewport rect stays alpha 0 — the fully transparent
    // band behind the row tail in every Mica combo popup (y124..131
    // Standard / y100..107 Compact). NONE never dispatches (the gate is
    // false without a granted backdrop), so the accepted NONE popup is
    // untouched. Compact-parametrized; every pixel assert pairs with a
    // token/geometry assert on the same state.
    DisableAnimationsGuard animations;
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    for (const bool dark : { false, true }) {
        style->setThemeMode(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
        QColor tint = WinUI3::Private::popupSurfaceColor(style->standardPalette());
        tint.setAlpha(dark ? 178 : 242);
        for (const int density : { 0, 1 }) {
            style->setProperty("densityMode", density);
            QWidget popup(nullptr, Qt::Popup);
            popup.setProperty("_winui_backdrop", 3); // Acrylic
            popup.setProperty("_winui_backdrop_effective", 2); // Composited
            popup.setAttribute(Qt::WA_TranslucentBackground);
            popup.setAutoFillBackground(false);
            popup.setContentsMargins(0, 4, 0, 4); // DropdownContentMargin
            popup.resize(200, density == 0 ? 136 : 112); // round-2 marks
            QPalette tinted = popup.palette();
            tinted.setColor(QPalette::Window, tint);
            tinted.setColor(QPalette::Base, tint);
            popup.setPalette(tinted);
            auto *view = new QListView(&popup);
            view->setFrameShape(QFrame::NoFrame);
            view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            view->setGeometry(popup.contentsRect());
            // Qt only runs the scroll-area layout pass on a show/LayoutRequest
            // event; the hidden fixture lays the frameless viewport out
            // directly so its rect matches the covered popup interior.
            view->viewport()->setGeometry(QRect(QPoint(0, 0), popup.contentsRect().size()));
            view->setPalette(tinted);
            // Armed by the scroll-chain sync on a granted popup backdrop.
            view->viewport()->setAttribute(Qt::WA_StyledBackground, true);
            view->viewport()->setPalette(tinted);
            // Token/geometry pairs for the pixel asserts below: the gate
            // passes for the popup viewport (no opaque card ancestor), the
            // popup carries the composited Window/Base tint, and the
            // DropdownContentMargin geometry is the covered 0,4,0,4.
            QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(view->viewport()));
            QCOMPARE(popup.contentsMargins(), QMargins(0, 4, 0, 4));
            QCOMPARE(popup.size(), QSize(200, density == 0 ? 136 : 112));
            QCOMPARE(view->geometry(), popup.contentsRect());
            QCOMPARE(popup.palette().color(QPalette::Window), tint);
            QCOMPARE(view->viewport()->palette().color(QPalette::Window), tint);
            // Mechanism: the viewport PE_Widget erase over the live material
            // must refill the popup tint instead of leaving the rect
            // Source-cleared (RED: body and row tail read alpha 0). Seeded
            // red so a skipped erase is observable too.
            const QRect viewportRect(QPoint(0, 0), view->viewport()->size());
            QCOMPARE(viewportRect, view->viewport()->rect());
            QCOMPARE(viewportRect.size(), QSize(200, density == 0 ? 128 : 104));
            QImage frame(viewportRect.size(), QImage::Format_ARGB32_Premultiplied);
            frame.fill(Qt::red);
            {
                QStyleOption option;
                option.initFrom(view->viewport());
                option.rect = viewportRect;
                option.state = QStyle::State_Enabled;
                QPainter painter(&frame);
                style->drawPrimitive(QStyle::PE_Widget, &option, &painter, view->viewport());
            }
            const QColor body = frame.pixelColor(viewportRect.center());
            QCOMPARE(body.alpha(), tint.alpha());
            QVERIFY(colorDistance(body, tint) <= 4);
            // Row tail: the round-2 band position (last rows of the popup
            // viewport) keeps the tint as well.
            const QPoint tail(viewportRect.center().x(), viewportRect.bottom() - 4);
            QVERIFY(QRect(QPoint(0, 0), frame.size()).contains(tail));
            const QColor tailPixel = frame.pixelColor(tail);
            QCOMPARE(tailPixel.alpha(), tint.alpha());
            QVERIFY(colorDistance(tailPixel, tint) <= 4);
        }
    }
    style->setProperty("densityMode", 0);
    style->setThemeMode(WinUI3::ThemeMode::Light);
}

void WinUI3MicaBackdropTest::standardButtonInsideCardKeepsCardFillOverComposited()
{
    // Dialogs-page card (round-2 mark): a Standard-flavour button child keeps
    // its translucent control fill (dark `QPalette::Button` = white@15) ON
    // TOP of the card's veiled layer. The panel erase Source-clears the
    // button rect (cards carry no SurfaceProperty, so the shared gate cannot
    // see them) and must rebuild the card tone first — the same
    // insideOpaqueCard guard the PE_Widget erase and the range-control
    // erases already use — or the rect reads bare material instead of the
    // card composite (RED: seeded card canvas punched to alpha 0). The
    // rebuild is tied to the erase actually running: without a granted
    // backdrop nothing is re-layered, so the opaque card keeps compositing
    // exactly as before (the #343434 NONE band contract). The
    // Subtle/Navigation rest branch already rebuilt the card tone this way
    // and stays untouched. Compact-parametrized; every pixel assert pairs
    // with a token/geometry assert on the same state.
    DisableAnimationsGuard animations;
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    // One fixture per mode: the same card/button geometry painted either over
    // a granted Composited backdrop (erase runs) or with the backdrop
    // disabled (erase gate false, NONE contract).
    const auto checkButtonOverCard = [&](bool composited) {
        QWidget window;
        if (composited) {
            window.setProperty("_winui_backdrop", 1);
            window.setProperty("_winui_backdrop_effective", 2); // Composited
        }
        auto *island = new QWidget(&window);
        island->setProperty(WinUI3::Style::SurfaceProperty, QStringLiteral("content"));
        island->setGeometry(0, 0, 480, 320);
        auto *card = new QGroupBox(QStringLiteral("Card"), island);
        card->setGeometry(40, 40, 400, 240);
        auto *button = new QPushButton(QStringLiteral("Float"), card);
        button->setGeometry(60, 140, 160, 32);
        window.resize(480, 320);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        if (composited)
            WinUI3::Private::syncContentSurfacesForBackdrop(&window);
        // Token/geometry pairs for the pixel asserts below: the shared gate
        // passes only with the granted backdrop (the card carries no
        // SurfaceProperty), the card walk finds the ancestor card either
        // way, and the card tone is the Active-group veiled refill the card
        // itself paints (cardRefillTone — never a forced-178 veil, which
        // reads as a band against the 76/128 card; see the disabled-slab
        // defect).
        QCOMPARE(WinUI3::Private::paintsDirectlyOnBackdrop(button), composited);
        QVERIFY(WinUI3::Private::insideOpaqueCard(button));
        const QColor cardTone = WinUI3::Private::cardRefillTone(button->palette(), composited);
        QCOMPARE(cardTone.alpha(), card->palette().color(QPalette::Base).alpha());
        QCOMPARE(button->geometry(), QRect(60, 140, 160, 32));
        // Mechanism: the real CE_PushButton path over a seeded card canvas
        // keeps the card tone — the corner pixels the rounded control surface
        // never reaches stay card-toned (RED over Mica: alpha 0; RED over
        // NONE: a second veiled layer punched through the opaque card), and
        // the translucent control fill composites ON the card tone once.
        QImage canvas(button->size(), QImage::Format_ARGB32_Premultiplied);
        canvas.fill(cardTone);
        {
            QStyleOptionButton option;
            option.initFrom(button);
            option.rect = QRect(QPoint(), button->size());
            option.state |= QStyle::State_Enabled;
            QPainter painter(&canvas);
            style->drawControl(QStyle::CE_PushButton, &option, &painter, button);
        }
        const QColor corner = canvas.pixelColor(0, 0);
        QCOMPARE(corner.alpha(), cardTone.alpha());
        QVERIFY(colorDistance(corner, cardTone) <= 4);
        QImage want(1, 1, QImage::Format_ARGB32_Premultiplied);
        want.fill(cardTone);
        {
            QPainter painter(&want);
            painter.fillRect(want.rect(), WinUI3::Private::tokens(button->palette()).control);
        }
        const QColor expected = want.pixelColor(0, 0);
        const QColor center = canvas.pixelColor(canvas.rect().center());
        QCOMPARE(center.alpha(), expected.alpha());
        QVERIFY(colorDistance(center, expected) <= 4);
    };
    for (const bool dark : { false, true }) {
        style->setThemeMode(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
        for (const int density : { 0, 1 }) {
            style->setProperty("densityMode", density);
            checkButtonOverCard(true);
            checkButtonOverCard(false);
        }
    }
    style->setProperty("densityMode", 0);
    style->setThemeMode(WinUI3::ThemeMode::Light);
}

void WinUI3MicaBackdropTest::alternateRowErasesBeforeFill()
{
    // CE_ItemViewItem Alternate over Composited lays the resolved
    // AlternateBase ink after erasing: a second consecutive paint over the
    // first frame is pixel-identical (no translucent accumulation).
    DisableAnimationsGuard animations;
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget window;
    window.setProperty("_winui_backdrop", 1);
    window.setProperty("_winui_backdrop_effective", 2); // Composited
    QListWidget list(&window);
    list.addItems({ QStringLiteral("Documents"), QStringLiteral("Pictures") });
    list.resize(300, 120);
    QStyleOptionViewItem option;
    option.initFrom(list.viewport());
    option.widget = list.viewport();
    option.rect = QRect(0, 0, 280, 40);
    option.features = QStyleOptionViewItem::Alternate;
    option.state = QStyle::State_Enabled;
    const QColor ink = option.palette.color(QPalette::AlternateBase);
    QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(list.viewport()));
    const auto paintRow = [&](QImage &image) {
        QPainter painter(&image);
        list.style()->drawControl(QStyle::CE_ItemViewItem, &option, &painter, list.viewport());
    };
    QImage first(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
    first.fill(Qt::red);
    paintRow(first);
    const QColor center = first.pixelColor(first.rect().center());
    QCOMPARE(center.alpha(), ink.alpha());
    // Premultiplied rounding (±1 per channel) when a translucent role
    // composites over the erased rect; alpha is the exact contract above.
    QVERIFY(colorDistance(center, ink) <= 4);
    QImage second = first.copy();
    paintRow(second);
    QCOMPARE(second, first);
}

void WinUI3MicaBackdropTest::selectedTabErasesBeforeOpaqueFill()
{
    // Selected north tab over Composited erases before its veiled fill:
    // the tab resolves the veiled selectedTabFill ink (same card recipe
    // as CC_GroupBox), and rest-vs-hover frames stay identical outside
    // the tab (no corner smear).
    DisableAnimationsGuard animations;
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget window;
    window.setProperty("_winui_backdrop", 1);
    window.setProperty("_winui_backdrop_effective", 2); // Composited
    QTabWidget tabs(&window);
    tabs.addTab(new QWidget, QStringLiteral("First"));
    tabs.addTab(new QWidget, QStringLiteral("Second"));
    QTabBar *bar = tabs.tabBar();
    QVERIFY(bar);
    QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(bar));
    QStyleOptionTab selected;
    selected.rect = QRect(0, 0, 140, 32);
    selected.shape = QTabBar::RoundedNorth;
    selected.palette = bar->palette();
    selected.state = QStyle::State_Enabled | QStyle::State_Selected;
    const QColor fill = WinUI3::Private::veiledCard(
            WinUI3::Private::tokens(selected.palette).selectedTabFill, true);
    QCOMPARE(fill.alpha(), WinUI3::Private::VeiledCardAlpha);
    QImage selectedFrame(selected.rect.size(), QImage::Format_ARGB32_Premultiplied);
    selectedFrame.fill(Qt::red);
    {
        QPainter painter(&selectedFrame);
        tabs.style()->drawControl(QStyle::CE_TabBarTabShape, &selected, &painter, bar);
    }
    // QColor == can fail on spec/premultiplied rounding despite identical
    // hex (both #b2fbfbfb here), so assert alpha exactly plus tight RGB
    // distance like the alternate-row contract, not strict QColor equality.
    const QColor center = selectedFrame.pixelColor(70, 16);
    QCOMPARE(center.alpha(), fill.alpha());
    QVERIFY(colorDistance(center, fill) <= 4);
    QStyleOptionTab rest = selected;
    rest.state = QStyle::State_Enabled;
    QStyleOptionTab hover = selected;
    hover.state = QStyle::State_Enabled | QStyle::State_MouseOver;
    QImage restFrame(selected.rect.size(), QImage::Format_ARGB32_Premultiplied);
    restFrame.fill(Qt::red);
    QImage hoverFrame(selected.rect.size(), QImage::Format_ARGB32_Premultiplied);
    hoverFrame.fill(Qt::red);
    {
        QPainter painter(&restFrame);
        tabs.style()->drawControl(QStyle::CE_TabBarTabShape, &rest, &painter, bar);
    }
    {
        QPainter painter(&hoverFrame);
        tabs.style()->drawControl(QStyle::CE_TabBarTabShape, &hover, &painter, bar);
    }
    QCOMPARE(restFrame.pixelColor(0, 0), QColor(255, 0, 0, 255));
    QCOMPARE(hoverFrame.pixelColor(0, 0), QColor(255, 0, 0, 255));
    QVERIFY(restFrame != hoverFrame);
}

void WinUI3MicaBackdropTest::settingsCardVeilsOverComposited()
{
    // Settings cards veil over a live material (VeiledCardAlpha), opaque
    // off it. Same recipe as CC_GroupBox: erase first, then veil the
    // resting control fill so the card cannot band over Mica.
    DisableAnimationsGuard animations;
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget window;
    window.setProperty("_winui_backdrop", 1);
    window.setProperty("_winui_backdrop_effective", 2); // Composited
    QFrame card(&window);
    card.setProperty(WinUI3::Style::SettingsCardProperty, true);
    card.resize(300, 96);
    window.resize(480, 320);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(&card));
    QStyleOption option;
    option.initFrom(&card);
    option.rect = card.rect();
    option.state = QStyle::State_Enabled;
    QImage frame(card.size(), QImage::Format_ARGB32_Premultiplied);
    frame.fill(Qt::red);
    {
        QPainter painter(&frame);
        style->drawControl(QStyle::CE_ShapedFrame, &option, &painter, &card);
    }
    const QColor ink = frame.pixelColor(frame.rect().center());
    // Veiled, not opaque: alpha equals VeiledCardAlpha over Composited.
    QCOMPARE(ink.alpha(), WinUI3::Private::VeiledCardAlpha);
    // Repeat paint over a fresh frame is stable (erase-then-veil rebuilds
    // the same ink instead of accumulating over the previous frame).
    QImage second(card.size(), QImage::Format_ARGB32_Premultiplied);
    second.fill(Qt::red);
    {
        QPainter painter(&second);
        style->drawControl(QStyle::CE_ShapedFrame, &option, &painter, &card);
    }
    QCOMPARE(second, frame);
}

void WinUI3MicaBackdropTest::dockTitleVeilsOverComposited()
{
    // Dock title veils its layer fill over a live material instead of
    // banding opaque; off-material it stays opaque.
    DisableAnimationsGuard animations;
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget window;
    window.setProperty("_winui_backdrop", 1);
    window.setProperty("_winui_backdrop_effective", 2); // Composited
    QDockWidget dock(QStringLiteral("Inspector"), &window);
    dock.resize(300, 40);
    window.resize(480, 320);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(&dock));
    QStyleOptionDockWidget option;
    option.initFrom(&dock);
    option.rect = QRect(0, 0, 280, 32);
    option.title = dock.windowTitle();
    option.state = QStyle::State_Enabled;
    QImage frame(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
    frame.fill(Qt::red);
    {
        QPainter painter(&frame);
        style->drawControl(QStyle::CE_DockWidgetTitle, &option, &painter, &dock);
    }
    const QColor expected =
            WinUI3::Private::veiledCard(WinUI3::Private::tokens(option.palette).layer, true);
    QCOMPARE(frame.pixelColor(140, 4), expected);
}

void WinUI3MicaBackdropTest::headerRestVeilsOverComposited()
{
    // Table header rest veils over a live material; hover/pressed stay
    // translucent subtle fills. Compact-parametrized geometry (row height
    // contract lives in density tests; here both densities paint veiled).
    DisableAnimationsGuard animations;
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget window;
    window.setProperty("_winui_backdrop", 1);
    window.setProperty("_winui_backdrop_effective", 2); // Composited
    QTableWidget table(2, 2, &window);
    table.resize(300, 120);
    window.resize(480, 320);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(table.horizontalHeader()));
    for (const int density : { 0, 1 }) {
        style->setProperty("densityMode", density);
        QStyleOptionHeader option;
        option.initFrom(table.horizontalHeader());
        option.rect = QRect(0, 0, 140, 32);
        option.state = QStyle::State_Enabled;
        QImage frame(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
        frame.fill(Qt::red);
        {
            QPainter painter(&frame);
            style->drawControl(QStyle::CE_HeaderSection, &option, &painter,
                               table.horizontalHeader());
        }
        const QColor expected =
                WinUI3::Private::veiledCard(WinUI3::Private::tokens(option.palette).layer, true);
        QCOMPARE(frame.pixelColor(70, 16), expected);
    }
    style->setProperty("densityMode", 0);
}

void WinUI3MicaBackdropTest::tabHoverPillVeilsOverComposited()
{
    // Unselected tab hover pill veils over a live material; rest stays
    // transparent. Erase-first keeps repeat hovers stable.
    DisableAnimationsGuard animations;
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget window;
    window.setProperty("_winui_backdrop", 1);
    window.setProperty("_winui_backdrop_effective", 2); // Composited
    QTabWidget tabs(&window);
    tabs.addTab(new QWidget, QStringLiteral("First"));
    tabs.addTab(new QWidget, QStringLiteral("Second"));
    QTabBar *bar = tabs.tabBar();
    QVERIFY(bar);
    QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(bar));
    QStyleOptionTab hover;
    hover.rect = QRect(0, 0, 140, 32);
    hover.shape = QTabBar::RoundedNorth;
    hover.palette = bar->palette();
    hover.state = QStyle::State_Enabled | QStyle::State_MouseOver;
    const QColor expected =
            WinUI3::Private::veiledCard(WinUI3::Private::tokens(hover.palette).layer, true);
    QImage first(hover.rect.size(), QImage::Format_ARGB32_Premultiplied);
    first.fill(Qt::red);
    {
        QPainter painter(&first);
        tabs.style()->drawControl(QStyle::CE_TabBarTabShape, &hover, &painter, bar);
    }
    QCOMPARE(first.pixelColor(70, 16), expected);
    QImage second = first.copy();
    {
        QPainter painter(&second);
        tabs.style()->drawControl(QStyle::CE_TabBarTabShape, &hover, &painter, bar);
    }
    QCOMPARE(second, first);
}

void WinUI3MicaBackdropTest::richTextFrameVeilsOverComposited()
{
    // Rich-text frame (PE_Frame richTextEditor branch) veils its resting
    // control fill over a live material: erase first, then the 178-alpha
    // veil, so the editor cannot composite opaque over Mica. Repeat paint
    // over a fresh frame is stable (no accumulation).
    DisableAnimationsGuard animations;
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget window;
    window.setProperty("_winui_backdrop", 1);
    window.setProperty("_winui_backdrop_effective", 2); // Composited
    QTextEdit editor(&window);
    editor.setPlainText(QStringLiteral("RichEditBox state"));
    editor.resize(320, 120);
    QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(&editor));
    QStyleOptionFrame option;
    option.initFrom(&editor);
    option.rect = QRect(0, 0, 320, 120);
    option.state = QStyle::State_Enabled;
    const QColor expected =
            WinUI3::Private::veiledCard(WinUI3::Private::tokens(option.palette).control, true);
    QCOMPARE(expected.alpha(), WinUI3::Private::VeiledCardAlpha);
    QImage frame(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
    frame.fill(Qt::red);
    {
        QPainter painter(&frame);
        style->drawPrimitive(QStyle::PE_Frame, &option, &painter, &editor);
    }
    QCOMPARE(frame.size(), option.rect.size());
    const QColor ink = frame.pixelColor(frame.rect().center());
    // Veiled, not opaque: without the veil the 179-alpha resting fill
    // composites over the canvas to opaque (alpha 255).
    QCOMPARE(ink.alpha(), WinUI3::Private::VeiledCardAlpha);
    // Premultiplied rounding (±1 per channel) when the veiled fill lands
    // on the erased rect; alpha is the exact contract above.
    QVERIFY(colorDistance(ink, expected) <= 4);
    QImage second(frame.size(), QImage::Format_ARGB32_Premultiplied);
    second.fill(Qt::red);
    {
        QPainter painter(&second);
        style->drawPrimitive(QStyle::PE_Frame, &option, &painter, &editor);
    }
    QCOMPARE(second, frame);
}

void WinUI3MicaBackdropTest::alternateRowVeilsOverComposited()
{
    // Veil twin of alternateRowErasesBeforeFill with the clamp pinned
    // observable: production AlternateBase roles are already translucent
    // (128 Light / 13 Dark), so the role is pinned to the opaque Window
    // tint — the branch must clamp it to the 178 veil, not paint it
    // opaque. Repeat paint over the frame stays pixel-identical.
    DisableAnimationsGuard animations;
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget window;
    window.setProperty("_winui_backdrop", 1);
    window.setProperty("_winui_backdrop_effective", 2); // Composited
    QListWidget list(&window);
    list.addItems({ QStringLiteral("Documents"), QStringLiteral("Pictures") });
    list.resize(300, 120);
    QStyleOptionViewItem option;
    option.initFrom(list.viewport());
    option.widget = list.viewport();
    option.rect = QRect(0, 0, 280, 40);
    option.features = QStyleOptionViewItem::Alternate;
    option.state = QStyle::State_Enabled;
    // Opaque pin: the VeiledCardAlpha clamp only bites above 178.
    QPalette pinned = option.palette;
    pinned.setColor(QPalette::AlternateBase, pinned.color(QPalette::Window));
    option.palette = pinned;
    const QColor expected =
            WinUI3::Private::veiledCard(option.palette.color(QPalette::AlternateBase), true);
    QCOMPARE(expected.alpha(), WinUI3::Private::VeiledCardAlpha);
    QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(list.viewport()));
    const auto paintRow = [&](QImage &image) {
        QPainter painter(&image);
        list.style()->drawControl(QStyle::CE_ItemViewItem, &option, &painter, list.viewport());
    };
    QImage first(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
    first.fill(Qt::red);
    paintRow(first);
    const QColor center = first.pixelColor(first.rect().center());
    QCOMPARE(center.alpha(), expected.alpha());
    // Premultiplied rounding (±1 per channel) when the veiled fill lands
    // on the erased rect; alpha is the exact contract above.
    QVERIFY(colorDistance(center, expected) <= 4);
    QImage second = first.copy();
    paintRow(second);
    QCOMPARE(second, first);
}

void WinUI3MicaBackdropTest::tabPressedPillVeilsOverComposited()
{
    // Unselected pressed tab pill veils controlPressed over a live
    // material (179 -> 178 clamp); rest stays transparent. The outer
    // corner keeps the canvas: the erase clips to the rounded rect, like
    // the hover-pill twin.
    DisableAnimationsGuard animations;
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget window;
    window.setProperty("_winui_backdrop", 1);
    window.setProperty("_winui_backdrop_effective", 2); // Composited
    QTabWidget tabs(&window);
    tabs.addTab(new QWidget, QStringLiteral("First"));
    tabs.addTab(new QWidget, QStringLiteral("Second"));
    QTabBar *bar = tabs.tabBar();
    QVERIFY(bar);
    QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(bar));
    QStyleOptionTab pressed;
    pressed.rect = QRect(0, 0, 140, 32);
    pressed.shape = QTabBar::RoundedNorth;
    pressed.palette = bar->palette();
    pressed.state = QStyle::State_Enabled | QStyle::State_Sunken;
    const QColor expected = WinUI3::Private::veiledCard(
            WinUI3::Private::tokens(pressed.palette).controlPressed, true);
    QCOMPARE(expected.alpha(), WinUI3::Private::VeiledCardAlpha);
    QImage frame(pressed.rect.size(), QImage::Format_ARGB32_Premultiplied);
    frame.fill(Qt::red);
    {
        QPainter painter(&frame);
        tabs.style()->drawControl(QStyle::CE_TabBarTabShape, &pressed, &painter, bar);
    }
    const QColor pill = frame.pixelColor(70, 16);
    QCOMPARE(pill.alpha(), expected.alpha());
    // Premultiplied rounding (±1 per channel); alpha is the exact
    // contract above (unveiled controlPressed reads 179 here).
    QVERIFY(colorDistance(pill, expected) <= 4);
    QCOMPARE(frame.pixelColor(0, 0), QColor(255, 0, 0, 255));
    QStyleOptionTab rest = pressed;
    rest.state = QStyle::State_Enabled;
    QImage restFrame(pressed.rect.size(), QImage::Format_ARGB32_Premultiplied);
    restFrame.fill(Qt::red);
    {
        QPainter painter(&restFrame);
        tabs.style()->drawControl(QStyle::CE_TabBarTabShape, &rest, &painter, bar);
    }
    QCOMPARE(restFrame.pixelColor(70, 16).alpha(), 0);
    QVERIFY(frame != restFrame);
}

void WinUI3MicaBackdropTest::scrollBarHoverTrackVeilsOverComposited()
{
    // ScrollBar hover track veils t.layer over a live material. The Base
    // role is pinned opaque (production roles are already translucent, so
    // the 178 clamp would be identity) and the painted track must stay at
    // or under VeiledCardAlpha — veiled, not the opaque layer. Both
    // orientations at hover progress 1.0, sampled clear of thumb and
    // arrow glyphs.
    DisableAnimationsGuard animations;
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget window;
    window.setProperty("_winui_backdrop", 1);
    window.setProperty("_winui_backdrop_effective", 2); // Composited
    const auto checkTrack = [&](Qt::Orientation orientation) {
        QScrollBar bar(orientation, &window);
        bar.setRange(0, 100);
        bar.setPageStep(20);
        bar.setValue(30);
        bar.resize(orientation == Qt::Horizontal ? 300 : 12,
                   orientation == Qt::Horizontal ? 12 : 300);
        QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(&bar));
        setFrame(&bar, "_winui_hover_progress", 1.0);
        QCOMPARE(frameReal(&bar, "_winui_hover_progress"), 1.0);
        QStyleOptionSlider option;
        option.initFrom(&bar);
        option.orientation = bar.orientation();
        option.minimum = bar.minimum();
        option.maximum = bar.maximum();
        option.sliderPosition = bar.sliderPosition();
        option.sliderValue = bar.value();
        option.pageStep = bar.pageStep();
        option.upsideDown = false;
        option.rect = bar.rect();
        option.state |= QStyle::State_MouseOver;
        // Opaque pin so the clamp bites (alternateRowVeils twin).
        QPalette pinned = option.palette;
        pinned.setColor(QPalette::Base, pinned.color(QPalette::Window));
        option.palette = pinned;
        const QColor expected =
                WinUI3::Private::veiledCard(WinUI3::Private::tokens(option.palette).layer, true);
        QCOMPARE(expected.alpha(), WinUI3::Private::VeiledCardAlpha);
        const QRect thumb = style->subControlRect(QStyle::CC_ScrollBar, &option,
                                                  QStyle::SC_ScrollBarSlider, &bar);
        const QRect decrease = style->subControlRect(QStyle::CC_ScrollBar, &option,
                                                     QStyle::SC_ScrollBarSubLine, &bar);
        QVERIFY(thumb.isValid());
        QVERIFY(decrease.isValid());
        const QPoint sample = orientation == Qt::Horizontal
                ? QPoint(decrease.right() + 4, bar.height() / 2)
                : QPoint(bar.width() / 2, decrease.bottom() + 4);
        QVERIFY(option.rect.contains(sample));
        QVERIFY(!thumb.contains(sample));
        QImage frame(bar.size(), QImage::Format_ARGB32_Premultiplied);
        frame.fill(Qt::red);
        {
            QPainter painter(&frame);
            style->drawComplexControl(QStyle::CC_ScrollBar, &option, &painter, &bar);
        }
        const QColor track = frame.pixelColor(sample);
        QVERIFY2(track.alpha() <= WinUI3::Private::VeiledCardAlpha,
                 qPrintable(QStringLiteral("track alpha %1").arg(track.alpha())));
        QVERIFY(colorDistance(track, expected) <= 4);
    };
    checkTrack(Qt::Horizontal);
    checkTrack(Qt::Vertical);
}

void WinUI3MicaBackdropTest::bareStyledPanelSyncSharesMaterial()
{
    // Bare QFrame::StyledPanel links (gallery density/footer panels) share
    // the material under a content island and under the centralWidget shell:
    // they arrive armed from polish, the explicit sync below is idempotent,
    // and toggle-off restore returns every link opaque. Tab-pane twin.
    DisableAnimationsGuard animations;
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget window;
    window.setProperty("_winui_backdrop", 1);
    window.setProperty("_winui_backdrop_effective", 2); // Composited
    auto *island = new QWidget(&window);
    island->setProperty(WinUI3::Style::SurfaceProperty, QStringLiteral("content"));
    auto *islandPanel = new QFrame(island);
    islandPanel->setFrameShape(QFrame::StyledPanel);
    islandPanel->resize(200, 60);
    auto *central = new QWidget(&window);
    central->setObjectName(QStringLiteral("centralWidget"));
    auto *shellPanel = new QFrame(central);
    shellPanel->setFrameShape(QFrame::StyledPanel);
    shellPanel->resize(200, 60);
    window.resize(480, 320);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QCOMPARE(islandPanel->palette().color(QPalette::Window).alpha(), 0);
    QCOMPARE(shellPanel->palette().color(QPalette::Window).alpha(), 0);
    WinUI3::Private::syncContentSurfacesForBackdrop(&window);
    QCOMPARE(islandPanel->palette().color(QPalette::Window).alpha(), 0);
    QCOMPARE(shellPanel->palette().color(QPalette::Window).alpha(), 0);
    QCOMPARE(islandPanel->autoFillBackground(), false);
    QCOMPARE(shellPanel->autoFillBackground(), false);
    const QImage islandGrab = islandPanel->grab().toImage();
    QVERIFY(!islandGrab.isNull());
    QCOMPARE(islandGrab.width(), islandPanel->width());
    const QImage shellGrab = shellPanel->grab().toImage();
    QVERIFY(!shellGrab.isNull());
    QCOMPARE(shellGrab.width(), shellPanel->width());
    WinUI3::Private::restoreContentSurfacesForBackdrop(&window);
    QCOMPARE(islandPanel->palette().color(QPalette::Window).alpha(), 255);
    QCOMPARE(shellPanel->palette().color(QPalette::Window).alpha(), 255);
}

void WinUI3MicaBackdropTest::lateScrollChainArmedAtPolish()
{
    // A scroll area created after the material was granted must join the
    // live recipe at polish time, without waiting for the next theme refresh
    // or an explicit sync: otherwise it keeps its opaque fill over the
    // material until something else converges it (ghost band).
    DisableAnimationsGuard animations;
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget window;
    window.setProperty("_winui_backdrop", 1);
    window.setProperty("_winui_backdrop_effective", 2); // Composited
    QWidget island(&window);
    island.setProperty(WinUI3::Style::SurfaceProperty, QStringLiteral("content"));
    window.resize(480, 320);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    // Late arrival: built and shown after the grant, no theme switch, no sync.
    auto *area = new QScrollArea(&island);
    auto *content = new QWidget;
    content->resize(760, 620);
    area->setWidget(content);
    area->resize(240, 160);
    area->show();
    QTRY_VERIFY(area->verticalScrollBar()->isVisible());
    QTRY_VERIFY(area->horizontalScrollBar()->isVisible());
    QCOMPARE(area->palette().color(QPalette::Window).alpha(), 0);
    QCOMPARE(area->viewport()->palette().color(QPalette::Window).alpha(), 0);
    QCOMPARE(area->verticalScrollBar()->palette().color(QPalette::Window).alpha(), 0);
    QCOMPARE(area->horizontalScrollBar()->palette().color(QPalette::Window).alpha(), 0);
    QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(area->viewport()));
    WinUI3::Private::restoreContentSurfacesForBackdrop(&window);
    QCOMPARE(area->palette().color(QPalette::Window).alpha(), 255);
    QCOMPARE(area->viewport()->palette().color(QPalette::Window).alpha(), 255);
}

void WinUI3MicaBackdropTest::reparentedChainReconvergesWithoutSync()
{
    // Moving a scroll chain between a bare shell and a card while the
    // material is granted reconverges both directions at reparent time, with
    // no explicit sync and no theme switch: bare keeps the full no-fill
    // recipe, in-card keeps the card fill with the partial viewport arm.
    DisableAnimationsGuard animations;
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget window;
    window.setProperty("_winui_backdrop", 1);
    window.setProperty("_winui_backdrop_effective", 2); // Composited
    auto *central = new QWidget(&window);
    central->setObjectName(QStringLiteral("centralWidget"));
    auto *card = new QGroupBox(QStringLiteral("Card"), central);
    auto *area = new QScrollArea(central);
    auto *content = new QWidget;
    content->resize(760, 620);
    area->setWidget(content);
    area->resize(240, 160);
    window.resize(640, 420);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QTRY_VERIFY(area->verticalScrollBar()->isVisible());
    QCOMPARE(area->palette().color(QPalette::Window).alpha(), 0);
    area->setParent(card);
    QVERIFY(WinUI3::Private::insideOpaqueCard(area));
    QCOMPARE(area->palette().color(QPalette::Window).alpha(), 255);
    QCOMPARE(area->viewport()->autoFillBackground(), false);
    QCOMPARE(area->verticalScrollBar()->palette().color(QPalette::Window).alpha(), 255);
    QVERIFY(!area->verticalScrollBar()->testAttribute(Qt::WA_StyledBackground));
    area->setParent(central);
    QVERIFY(!WinUI3::Private::insideOpaqueCard(area));
    QCOMPARE(area->palette().color(QPalette::Window).alpha(), 0);
    QCOMPARE(area->viewport()->palette().color(QPalette::Window).alpha(), 0);
    QCOMPARE(area->verticalScrollBar()->palette().color(QPalette::Window).alpha(), 0);
}

QTEST_MAIN(WinUI3MicaBackdropTest)
#include "tst_winui3micabackdrop.moc"
