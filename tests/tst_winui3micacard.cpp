// SPDX-License-Identifier: LGPL-2.1-or-later
// Mica card-nested backdrop samples: in-card nav/menu veils (card tone,
// never transparentized). Shell chrome and the implicitly-backgrounded
// palette preview stay on the bare material (uniform mica). Split out of
// tst_winui3micabackdrop.cpp per the 60 KB test-size ratchet. Every
// grab()/pixelColor() assertion pairs with a QCOMPARE token/geometry
// assertion on the same state.
#include <winui3style/winui3backdrop.h>
#include <winui3style/winui3style.h>

#include "winui3testhelpers.h"

#include "ui_gallerywindow.h"

#include "../src/winui3surfaces_p.h"
#include "../src/winui3tokens_p.h"

#include <QCommandLinkButton>
#include <QMainWindow>
#include <QTreeWidget>

class WinUI3MicaCardTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void navSampleInCardKeepsCardFillOverComposited();
    void menuSampleInCardKeepsCardFillOverComposited();
    void palettePreviewCardKeepsCardFillOverComposited();
    void treeSelectionKeepsCardFillOverComposited();
    void galleryTreeKeepsCardFillOverComposited();
    void disabledInCardButtonKeepsCardToneOverComposited();
    void reparentedBackdropSurfacesRestore_data();
    void reparentedBackdropSurfacesRestore();
};

void WinUI3MicaCardTest::initTestCase()
{
    qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light));
}

void WinUI3MicaCardTest::init()
{
    if (auto *style = qobject_cast<WinUI3::Style *>(qApp->style())) {
        style->setThemeMode(WinUI3::ThemeMode::Light);
        style->setAccentColor({});
    }
}

void WinUI3MicaCardTest::cleanup()
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

void WinUI3MicaCardTest::reparentedBackdropSurfacesRestore_data()
{
    QTest::addColumn<bool>("dark");
    QTest::addColumn<bool>("compact");
    QTest::addColumn<bool>("startsInCard");
    QTest::addColumn<bool>("resync");
    for (const bool dark : { false, true }) {
        for (const bool compact : { false, true }) {
            for (const bool inCard : { false, true }) {
                const QByteArray name = QByteArray(dark ? "dark" : "light")
                        + (compact ? "-compact" : "-standard")
                        + (inCard ? "-card-to-bare" : "-bare-to-card");
                for (const bool resync : { false, true }) {
                    const QByteArray scenario = name + (resync ? "-resync" : "-disable");
                    QTest::newRow(scenario.constData()) << dark << compact << inCard << resync;
                }
            }
        }
    }
}

void WinUI3MicaCardTest::reparentedBackdropSurfacesRestore()
{
    QFETCH(bool, dark);
    QFETCH(bool, compact);
    QFETCH(bool, startsInCard);
    QFETCH(bool, resync);
    DisableAnimationsGuard animations;
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setThemeMode(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
    QMainWindow window;
    window.setProperty(WinUI3::Style::DensityProperty,
                       compact ? QStringLiteral("compact") : QStringLiteral("standard"));
    auto *central = new QWidget;
    central->setObjectName(QStringLiteral("centralWidget"));
    window.setCentralWidget(central);
    auto *card = new QGroupBox(QStringLiteral("Card"), central);
    auto *frame = new QFrame(startsInCard ? static_cast<QWidget *>(card) : central);
    frame->setFrameShape(QFrame::StyledPanel);
    auto *area = new QScrollArea(frame);
    area->setWidget(new QWidget);
    auto *menu = new QMenuBar(frame);
    menu->addAction(QStringLiteral("File"));
    const QList<QWidget *> surfaces = { frame,
                                        area,
                                        area->viewport(),
                                        area->widget(),
                                        area->verticalScrollBar(),
                                        area->horizontalScrollBar(),
                                        menu };
    window.resize(480, 320);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    struct SavedSurface
    {
        QWidget *widget;
        QPalette palette;
        bool explicitPalette;
        bool autoFill;
        bool styled;
        bool opaque;
    };
    QList<SavedSurface> saved;
    for (QWidget *surface : surfaces) {
        // Explicit palettes make the restoration expectation independent of
        // inherited colors changing when the application reparents a widget.
        surface->setPalette(style->standardPalette());
        saved.append({ surface, surface->palette(), surface->testAttribute(Qt::WA_SetPalette),
                       surface->autoFillBackground(),
                       surface->testAttribute(Qt::WA_StyledBackground),
                       surface->testAttribute(Qt::WA_OpaquePaintEvent) });
    }
    window.setProperty("_winui_backdrop", 1);
    window.setProperty("_winui_backdrop_effective", 2);
    WinUI3::Private::makeChromeSurfacesTransparent(&window);
    WinUI3::Private::syncContentSurfacesForBackdrop(&window);
    QCOMPARE(WinUI3::Private::insideOpaqueCard(area), startsInCard);
    QCOMPARE(area->viewport()->autoFillBackground(), false);
    if (!startsInCard)
        QCOMPARE(area->palette().color(QPalette::Window).alpha(), 0);
    frame->setParent(startsInCard ? central : static_cast<QWidget *>(card));
    frame->show();
    QCOMPARE(WinUI3::Private::insideOpaqueCard(area), !startsInCard);
    if (resync) {
        WinUI3::Private::makeChromeSurfacesTransparent(&window);
        WinUI3::Private::syncContentSurfacesForBackdrop(&window);
    }
    // Teardown must use saved state, not the application's new ancestry.
    window.setProperty("_winui_backdrop", 0);
    WinUI3::Private::restoreChromeSurfaces(&window);
    WinUI3::Private::restoreContentSurfacesForBackdrop(&window);
    for (const SavedSurface &state : saved) {
        QCOMPARE(state.widget->palette(), state.palette);
        QCOMPARE(state.widget->testAttribute(Qt::WA_SetPalette), state.explicitPalette);
        QCOMPARE(state.widget->autoFillBackground(), state.autoFill);
        QCOMPARE(state.widget->testAttribute(Qt::WA_StyledBackground), state.styled);
        QCOMPARE(state.widget->testAttribute(Qt::WA_OpaquePaintEvent), state.opaque);
        QVERIFY(!state.widget->property("_winui_backdrop_surface_palette").isValid());
        QVERIFY(!state.widget->property("_winui_card_viewport_autofill").isValid());
    }
}

void WinUI3MicaCardTest::navSampleInCardKeepsCardFillOverComposited()
{
    // Density-panel nav (reviewer screenshot): in-card sample is recipe 1
    // (card tone, never transparentized); shell nav stays recipe 2. Dark /
    // light x Standard/Compact; every pixel assert pairs with a token or
    // geometry assert on the same state.
    DisableAnimationsGuard animations;
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    const WinUI3::DensityMode previousDensity = style->densityMode();
    const auto densities = { WinUI3::DensityMode::Standard, WinUI3::DensityMode::Compact };
    for (const bool dark : { false, true }) {
        style->setThemeMode(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
        for (const WinUI3::DensityMode density : densities) {
            style->setDensityMode(density);
            QWidget window; // + content island + card + in-card/shell nav pair
            window.setProperty(WinUI3::Style::BackdropProperty, QStringLiteral("mica"));
            auto *island = new QWidget(&window);
            island->setProperty(WinUI3::Style::SurfaceProperty, QStringLiteral("content"));
            island->setGeometry(0, 0, 480, 320);
            auto *card = new QGroupBox(QStringLiteral("Density"), island);
            card->setGeometry(20, 20, 300, 200);
            auto *nav = new QListWidget(card);
            nav->setProperty(WinUI3::Style::NavigationViewProperty, true);
            nav->addItems({ QStringLiteral("Home"), QStringLiteral("Collections") });
            nav->setGeometry(10, 30, 280, 120);
            auto *shellNav = new QListWidget(&window);
            shellNav->setProperty(WinUI3::Style::NavigationViewProperty, true);
            shellNav->addItems({ QStringLiteral("Home"), QStringLiteral("Collections") });
            shellNav->setGeometry(340, 20, 120, 120);
            window.resize(480, 320);
            window.show();
            QVERIFY(QTest::qWaitForWindowExposed(&window));
            // The probe rows are painted with an explicit rest state below,
            // so the paint never depends on the view's live current index.
            window.setProperty("_winui_backdrop", 1);
            window.setProperty("_winui_backdrop_effective", 2); // Composited
            WinUI3::Private::syncContentSurfacesForBackdrop(&window);
            QVERIFY(WinUI3::Private::insideOpaqueCard(nav));
            QVERIFY(!WinUI3::Private::insideOpaqueCard(shellNav));
            const QColor cardTone = WinUI3::Private::cardRefillTone(card->palette(), true);
            QCOMPARE(cardTone.alpha(), card->palette().color(QPalette::Base).alpha());
            const QRect rowRect = nav->visualItemRect(nav->item(0));
            QCOMPARE(rowRect.height(), density == WinUI3::DensityMode::Standard ? 40 : 32);
            QCOMPARE(nav->palette().color(QPalette::Base).alpha(),
                     card->palette().color(QPalette::Base).alpha());
            QCOMPARE(nav->viewport()->palette().color(QPalette::Base).alpha(),
                     card->palette().color(QPalette::Base).alpha());
            QCOMPARE(shellNav->palette().color(QPalette::Base).alpha(), 0);
            const auto paintRow = [&](QListWidget *view, const QColor &seed) {
                QImage canvas(view->viewport()->size(), QImage::Format_ARGB32_Premultiplied);
                canvas.fill(seed);
                QStyleOptionViewItem rowOption;
                rowOption.initFrom(view->viewport());
                rowOption.widget = view->viewport();
                rowOption.rect = view->visualItemRect(view->item(0));
                // Explicit rest state: no selection, hover, press or focus, so
                // the delegate paints no pill or indicator of its own.
                rowOption.state = QStyle::State_Enabled;
                QPainter painter(&canvas);
                // Paint through the real delegate with an invalid index: the
                // row then carries no icon, text, or selection ink, so the
                // seed must survive the whole rect exactly (RED in-card: the
                // delegate Source-clear punched it to alpha 0).
                view->itemDelegate()->paint(&painter, rowOption, QModelIndex());
                painter.end();
                // Probe at the row rect's own center: the canvas matches the
                // viewport size, so the row center is the erase site (the
                // canvas center can fall on another row for multi-row views).
                return canvas.pixelColor(rowOption.rect.center());
            };
            // Rest rows paint no fill of their own: the seeded card tone must
            // survive (RED: the delegate Source-clear punched it to alpha 0).
            // Alpha-exact plus the sibling RGB bound: premultiplied rounding
            // over the seeded canvas can drift Ã‚Â±1 per channel. The shell twin
            // stays recipe 2 (erase-means-transparent, uniform material with
            // its window parent): with no fill of its own the composited row
            // rect stays cleared, so the same probe reads the seed
            // Source-cleared to alpha 0.
            QCOMPARE(paintRow(nav, cardTone).alpha(), cardTone.alpha());
            QVERIFY(colorDistance(paintRow(nav, cardTone), cardTone) <= 4);
            QCOMPARE(paintRow(shellNav, Qt::red).alpha(), 0);
            QImage grab(window.size(), QImage::Format_ARGB32_Premultiplied);
            grab.fill(Qt::transparent);
            window.render(&grab);
            QCOMPARE(grab.size(), window.size());
            const QPoint rowProbe = nav->viewport()->mapTo(
                    &window, QPoint(4, nav->visualItemRect(nav->item(0)).center().y()));
            QVERIFY(grab.pixelColor(rowProbe).alpha() != 0);
        }
    }
    style->setDensityMode(previousDensity);
    style->setThemeMode(WinUI3::ThemeMode::Light);
}

void WinUI3MicaCardTest::menuSampleInCardKeepsCardFillOverComposited()
{
    // In-card menu-bar sample (reviewer screenshot): recipe 1 veiled refill;
    // shell bar stays recipe 2; password field keeps its editor fill.
    // Dark/light x Standard/Compact; every pixel assert pairs with a
    // token/geometry assert on the same state.
    DisableAnimationsGuard animations;
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    const WinUI3::DensityMode previousDensity = style->densityMode();
    const auto densities = { WinUI3::DensityMode::Standard, WinUI3::DensityMode::Compact };
    for (const bool dark : { false, true }) {
        style->setThemeMode(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
        for (const WinUI3::DensityMode density : densities) {
            style->setDensityMode(density);
            QWidget window; // + island + card + in-card bar + shell bar + password
            window.setProperty(WinUI3::Style::BackdropProperty, QStringLiteral("mica"));
            auto *island = new QWidget(&window);
            island->setProperty(WinUI3::Style::SurfaceProperty, QStringLiteral("content"));
            island->setGeometry(0, 0, 480, 320);
            auto *card = new QGroupBox(QStringLiteral("Density"), island);
            card->setGeometry(20, 20, 300, 200);
            auto *bar = new QMenuBar(card);
            QAction *file = bar->addAction(QStringLiteral("File"));
            bar->setGeometry(10, 30, 260, 32);
            auto *shellBar = new QMenuBar(&window);
            shellBar->addAction(QStringLiteral("File"));
            shellBar->setGeometry(340, 20, 120, 32);
            auto *password = new QLineEdit(card);
            password->setGeometry(10, 70, 260, 32);
            window.resize(480, 320);
            window.show();
            QVERIFY(QTest::qWaitForWindowExposed(&window));
            const int opaqueBarWindow = bar->palette().color(QPalette::Window).alpha();
            window.setProperty("_winui_backdrop", 1);
            window.setProperty("_winui_backdrop_effective", 2); // Composited
            WinUI3::Private::syncContentSurfacesForBackdrop(&window);
            WinUI3::Private::makeChromeSurfacesTransparent(&window);
            QVERIFY(WinUI3::Private::insideOpaqueCard(bar));
            QVERIFY(!WinUI3::Private::insideOpaqueCard(shellBar));
            const QColor cardTone = WinUI3::Private::cardRefillTone(card->palette(), true);
            QCOMPARE(cardTone.alpha(), card->palette().color(QPalette::Base).alpha());
            const QRect actionRect = bar->actionGeometry(file);
            QVERIFY(!actionRect.isEmpty());
            QCOMPARE(bar->palette().color(QPalette::Window).alpha(), opaqueBarWindow);
            QCOMPARE(bar->testAttribute(Qt::WA_StyledBackground), false);
            QCOMPARE(shellBar->palette().color(QPalette::Window).alpha(), 0);
            QStyleOption emptyArea;
            emptyArea.initFrom(bar);
            emptyArea.rect = bar->rect();
            QImage emptyFrame(bar->size(), QImage::Format_ARGB32_Premultiplied);
            emptyFrame.fill(cardTone);
            {
                QPainter painter(&emptyFrame);
                style->drawControl(QStyle::CE_MenuBarEmptyArea, &emptyArea, &painter, bar);
            }
            const QColor emptyInk = emptyFrame.pixelColor(bar->width() - 8, bar->height() / 2);
            QCOMPARE(emptyInk.alpha(), cardTone.alpha());
            QVERIFY(colorDistance(emptyInk, cardTone) <= 4);
            QStyleOptionMenuItem itemOption;
            itemOption.rect = actionRect;
            itemOption.state = QStyle::State_Enabled;
            itemOption.text = file->text();
            itemOption.font = bar->font();
            itemOption.fontMetrics = QFontMetrics(itemOption.font);
            itemOption.menuItemType = QStyleOptionMenuItem::Normal;
            QImage itemFrame(bar->size(), QImage::Format_ARGB32_Premultiplied);
            itemFrame.fill(cardTone);
            {
                QPainter painter(&itemFrame);
                style->drawControl(QStyle::CE_MenuBarItem, &itemOption, &painter, bar);
            }
            const QColor itemInk =
                    itemFrame.pixelColor(actionRect.left() + 4, actionRect.center().y());
            QCOMPARE(itemInk.alpha(), cardTone.alpha());
            QVERIFY(colorDistance(itemInk, cardTone) <= 4);
            QStyleOption shellArea;
            shellArea.initFrom(shellBar);
            shellArea.rect = shellBar->rect();
            QImage shellFrame(shellBar->size(), QImage::Format_ARGB32_Premultiplied);
            shellFrame.fill(QColor(255, 0, 0, 255));
            {
                QPainter painter(&shellFrame);
                style->drawControl(QStyle::CE_MenuBarEmptyArea, &shellArea, &painter, shellBar);
            }
            QCOMPARE(shellFrame.pixelColor(60, 16).alpha(), 0);
            QStyleOption passwordOption;
            passwordOption.initFrom(password);
            passwordOption.rect = password->rect();
            passwordOption.state = QStyle::State_Enabled;
            QImage passwordFrame(password->size(), QImage::Format_ARGB32_Premultiplied);
            passwordFrame.fill(QColor(255, 0, 0, 255));
            {
                QPainter painter(&passwordFrame);
                style->drawPrimitive(QStyle::PE_PanelLineEdit, &passwordOption, &painter, password);
            }
            const QColor editorFill = WinUI3::Private::tokens(password->palette()).control;
            QCOMPARE(passwordFrame.pixelColor(passwordFrame.rect().center()).alpha(),
                     editorFill.alpha());
            QImage grab(window.size(), QImage::Format_ARGB32_Premultiplied);
            grab.fill(Qt::transparent);
            window.render(&grab);
            QCOMPARE(grab.size(), window.size());
            QVERIFY(grab.pixelColor(bar->mapTo(&window, bar->rect().center())).alpha() != 0);
            QVERIFY(grab.pixelColor(password->mapTo(&window, password->rect().center())).alpha()
                    != 0);
        }
    }
    style->setDensityMode(previousDensity);
    style->setThemeMode(WinUI3::ThemeMode::Light);
}

void WinUI3MicaCardTest::palettePreviewCardKeepsCardFillOverComposited()
{
    // Palette-lab preview card (reviewer dark-Mica screenshot + 2026-10-06
    // uniform-mica direction): the plain QWidget#palettePreview carries NO
    // fill under Mica Ã¢â‚¬â€ its NONE background is implicit (uniform with the
    // page), so under Mica it stays on the bare material like its parent
    // instead of taking a veil. Its push / check / lineedit children paint
    // their own opaque fills and strokes over that material. Dark/light x
    // Standard/Compact; every pixel assert pairs with a token/geometry
    // assert on the same state.
    DisableAnimationsGuard animations;
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    const WinUI3::DensityMode previousDensity = style->densityMode();
    const auto densities = { WinUI3::DensityMode::Standard, WinUI3::DensityMode::Compact };
    for (const bool dark : { false, true }) {
        style->setThemeMode(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
        for (const WinUI3::DensityMode density : densities) {
            style->setDensityMode(density);
            QWidget window; // + island + preview card + button/check/edit children
            window.setProperty(WinUI3::Style::BackdropProperty, QStringLiteral("mica"));
            auto *island = new QWidget(&window);
            island->setProperty(WinUI3::Style::SurfaceProperty, QStringLiteral("content"));
            island->setGeometry(0, 0, 480, 360);
            auto *card = new QWidget(island);
            card->setObjectName(QStringLiteral("palettePreview"));
            // Mirrors the .ui (autoFillBackground false since 2026-10-06):
            // no fill of its own in either mode, uniform with the page.
            card->setAutoFillBackground(false);
            card->setGeometry(20, 20, 300, 240);
            auto *button = new QPushButton(QStringLiteral("Preview button"), card);
            button->setGeometry(10, 10, 160, 32);
            auto *check = new QCheckBox(QStringLiteral("Preview checkbox"), card);
            check->setChecked(true);
            check->setGeometry(10, 60, 220, 32);
            auto *edit = new QLineEdit(card);
            edit->setText(QStringLiteral("Typed text preview"));
            edit->setGeometry(10, 110, 260, 32);
            window.resize(480, 360);
            window.show();
            QVERIFY(QTest::qWaitForWindowExposed(&window));
            window.setProperty("_winui_backdrop", 1);
            window.setProperty("_winui_backdrop_effective", 2); // Composited
            WinUI3::Private::syncContentSurfacesForBackdrop(&window);
            QVERIFY(!WinUI3::Private::insideOpaqueCard(button));
            QVERIFY(!WinUI3::Private::insideOpaqueCard(check));
            QVERIFY(!WinUI3::Private::insideOpaqueCard(edit));
            QCOMPARE(card->geometry(), QRect(20, 20, 300, 240));
            // No veil arm, no StyledBackground: nothing dispatches paint for
            // the bare panel, so it can never gain a fill (uniform material).
            QCOMPARE(card->testAttribute(Qt::WA_StyledBackground), false);
            QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(card));
            // Mechanism: PE_Widget lays no fill on the bare panel Ã¢â‚¬â€ a seeded
            // canvas comes back Source-cleared to alpha 0 (RED pre-revert:
            // the recipe-1 refill painted the veil tone here).
            QImage cardFrame(card->size(), QImage::Format_ARGB32_Premultiplied);
            cardFrame.fill(Qt::red);
            {
                QStyleOption cardOption;
                cardOption.initFrom(card);
                cardOption.rect = card->rect();
                cardOption.state = QStyle::State_Enabled;
                QPainter painter(&cardFrame);
                style->drawPrimitive(QStyle::PE_Widget, &cardOption, &painter, card);
            }
            QCOMPARE(cardFrame.pixelColor(cardFrame.rect().center()).alpha(), 0);
            // Mechanism: the checkbox paints its own indicator and text over
            // the material (no card tone to keep). Probe the indicator box,
            // which is always inked when checked (RED: nothing painted).
            QImage checkFrame(check->size(), QImage::Format_ARGB32_Premultiplied);
            checkFrame.fill(Qt::transparent);
            {
                QStyleOptionButton checkOption;
                checkOption.initFrom(check);
                checkOption.rect = check->rect();
                checkOption.state |= QStyle::State_Enabled | QStyle::State_On;
                checkOption.text = check->text();
                QPainter painter(&checkFrame);
                style->drawControl(QStyle::CE_CheckBox, &checkOption, &painter, check);
            }
            QVERIFY(checkFrame.pixelColor(10, 16).alpha() != 0);
            // Mechanism: the lineedit paints its own editor surface; the
            // rounded corners it never reaches stay cleared (RED: veil).
            QImage editFrame(edit->size(), QImage::Format_ARGB32_Premultiplied);
            editFrame.fill(Qt::transparent);
            {
                QStyleOption editOption;
                editOption.initFrom(edit);
                editOption.rect = edit->rect();
                editOption.state = QStyle::State_Enabled;
                QPainter painter(&editFrame);
                style->drawPrimitive(QStyle::PE_PanelLineEdit, &editOption, &painter, edit);
            }
            QCOMPARE(editFrame.pixelColor(0, 0).alpha(), 0);
            QVERIFY(editFrame.pixelColor(editFrame.rect().center()).alpha() != 0);
            // Reviewer constraint (dark-Mica screenshot): Mica must never
            // remove borders or shift geometry. NONE twin: same
            // construction, no backdrop opt-in, autoFill false like the .ui.
            QWidget noneWindow;
            auto *noneIsland = new QWidget(&noneWindow);
            noneIsland->setGeometry(0, 0, 480, 360);
            auto *noneCard = new QWidget(noneIsland);
            noneCard->setObjectName(QStringLiteral("palettePreview"));
            noneCard->setAutoFillBackground(false);
            noneCard->setGeometry(20, 20, 300, 240);
            auto *noneButton = new QPushButton(QStringLiteral("Preview button"), noneCard);
            noneButton->setGeometry(10, 10, 160, 32);
            auto *noneCheck = new QCheckBox(QStringLiteral("Preview checkbox"), noneCard);
            noneCheck->setChecked(true);
            noneCheck->setGeometry(10, 60, 220, 32);
            auto *noneEdit = new QLineEdit(noneCard);
            noneEdit->setText(QStringLiteral("Typed text preview"));
            noneEdit->setGeometry(10, 110, 260, 32);
            noneWindow.resize(480, 360);
            noneWindow.show();
            QVERIFY(QTest::qWaitForWindowExposed(&noneWindow));
            QCOMPARE(button->geometry(), noneButton->geometry());
            QCOMPARE(check->geometry(), noneCheck->geometry());
            QCOMPARE(edit->geometry(), noneEdit->geometry());
            QCOMPARE(card->geometry(), noneCard->geometry());
            // Border strokes repaint at the same pixels under Mica. The
            // seeded canvases isolate the style paint over transparency (no
            // card tone anywhere now): strokes must be present in both modes
            // at the same geometry. The inside stroke is translucent by
            // design (white@18 dark, black@15 light), so over the cleared
            // Mica interior vs the NONE one only alpha/composition may
            // differ Ã¢â‚¬â€ and the Mica stack must never read more opaque.
            // A wider erase rect or a skipped border branch would read
            // alpha 0 (the screenshot's vanished borders) or a shifted
            // stroke.
            const auto strokeInk = [&](const QWidget *target, bool lineEdit) {
                QImage frame(target->size(), QImage::Format_ARGB32_Premultiplied);
                frame.fill(Qt::transparent);
                QStyleOption option;
                option.initFrom(target);
                option.rect = target->rect();
                option.state = QStyle::State_Enabled;
                QPainter painter(&frame);
                style->drawPrimitive(lineEdit ? QStyle::PE_PanelLineEdit
                                              : QStyle::PE_PanelButtonCommand,
                                     &option, &painter, target);
                painter.end();
                return frame.pixelColor(0, frame.height() / 2);
            };
            const QColor micaEditStroke = strokeInk(edit, true);
            const QColor noneEditStroke = strokeInk(noneEdit, true);
            QVERIFY(micaEditStroke.alpha() != 0);
            QVERIFY(noneEditStroke.alpha() != 0);
            // The inside stroke is translucent by design (white@18 dark,
            // black@15 light), so over the cleared Mica interior vs the
            // seeded NONE one only alpha/composition may differ Ã¢â‚¬â€ and the
            // veiled Mica stack must never read more opaque than NONE.
            QVERIFY(micaEditStroke.alpha() <= noneEditStroke.alpha());
            const QColor micaButtonStroke = strokeInk(button, false);
            const QColor noneButtonStroke = strokeInk(noneButton, false);
            QVERIFY(micaButtonStroke.alpha() != 0);
            QCOMPARE(micaButtonStroke.alpha(), noneButtonStroke.alpha());
            QVERIFY(colorDistance(micaButtonStroke, noneButtonStroke) <= 4);
            QImage grab(window.size(), QImage::Format_ARGB32_Premultiplied);
            grab.fill(Qt::transparent);
            window.render(&grab);
            QCOMPARE(grab.size(), window.size());
            // The card center sits inside the edit: the editor's own fill
            // covers it. The bare bottom strip has no child cover and must
            // stay cleared (RED pre-revert: the recipe-1 refill veiled it).
            QVERIFY(grab.pixelColor(card->mapTo(&window, card->rect().center())).alpha() != 0);
            const QPoint bareCardProbe =
                    card->mapTo(&window, QPoint(card->width() / 2, card->height() - 8));
            QCOMPARE(grab.pixelColor(bareCardProbe).alpha(), 0);
            QVERIFY(grab.pixelColor(edit->mapTo(&window, edit->rect().center())).alpha() != 0);
            // Real-render mirror of the stroke probes: the live border
            // pixels stay opaque under Mica at the twin's positions.
            QImage noneGrab(noneWindow.size(), QImage::Format_ARGB32_Premultiplied);
            noneGrab.fill(Qt::transparent);
            noneWindow.render(&noneGrab);
            const QPoint micaEditBorder = edit->mapTo(&window, QPoint(0, edit->height() / 2));
            const QPoint noneEditBorder =
                    noneEdit->mapTo(&noneWindow, QPoint(0, noneEdit->height() / 2));
            QVERIFY(grab.pixelColor(micaEditBorder).alpha() != 0);
            QVERIFY(noneGrab.pixelColor(noneEditBorder).alpha() != 0);
        }
    }
    style->setDensityMode(previousDensity);
    style->setThemeMode(WinUI3::ThemeMode::Light);
}

void WinUI3MicaCardTest::galleryTreeKeepsCardFillOverComposited()
{
    // Live-tree mirror of the reviewer Gallery run (real Designer tree, not
    // a synthetic island): pages(content island) > controlsPage scroll chain
    // > controlsBody > densityComparisonGroup(QGroupBox) >
    // standardDensityPanel(QFrame StyledPanel) > standardDensityNav/Menu, and
    // paletteLabPage > paletteBody > palettePreview + previewButton /
    // previewCheck / previewEdit. Drives the REAL applyBackdrop pre-sync
    // state (zero-alpha Window role rebound on the window itself,
    // NoSystemBackground, no autofill Ã¢â‚¬â€ winui3backdrop.cpp:279-286) plus the
    // chrome + content syncs in live order, after show like the live Mica
    // toggle. Dark/light; every pixel assert pairs with a token/geometry
    // assert on the same state.
    DisableAnimationsGuard animations;
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    const WinUI3::DensityMode previousDensity = style->densityMode();
    style->setDensityMode(WinUI3::DensityMode::Standard);
    const auto densities = { WinUI3::DensityMode::Standard, WinUI3::DensityMode::Compact };
    for (const bool dark : { false, true }) {
        style->setThemeMode(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
        for (const WinUI3::DensityMode density : densities) {
            style->setDensityMode(density);
            QMainWindow window;
            Ui::GalleryWindow ui;
            ui.setupUi(&window);
            for (QListWidget *nav : { ui.standardDensityNav, ui.compactDensityNav })
                nav->addItems({ QStringLiteral("Home"), QStringLiteral("Collections") });
            window.resize(1180, 780);
            window.show();
            QVERIFY(QTest::qWaitForWindowExposed(&window));
            window.setProperty(WinUI3::Style::BackdropProperty, QStringLiteral("mica"));
            QPalette material = window.palette();
            QColor windowColor = material.color(QPalette::Window);
            windowColor.setAlpha(0);
            material.setColor(QPalette::Window, windowColor);
            window.setPalette(material);
            window.setAttribute(Qt::WA_NoSystemBackground, true);
            window.setAttribute(Qt::WA_OpaquePaintEvent, false);
            window.setAutoFillBackground(false);
            window.setProperty("_winui_backdrop", 1);
            window.setProperty("_winui_backdrop_effective", 2); // Composited
            WinUI3::Private::makeChromeSurfacesTransparent(&window);
            WinUI3::Private::syncContentSurfacesForBackdrop(&window);
            qApp->processEvents();
            QVERIFY(WinUI3::Private::insideOpaqueCard(ui.standardDensityNav));
            QVERIFY(WinUI3::Private::insideOpaqueCard(ui.standardDensityMenu));
            // palettePreview is deliberately NOT a card (uniform-mica design):
            // no fill of its own, uniform with the material page, so its
            // children paint their own fills without card-tone refills.
            QVERIFY(!WinUI3::Private::insideOpaqueCard(ui.previewCheck));
            QVERIFY(!WinUI3::Private::insideOpaqueCard(ui.previewEdit));
            QVERIFY(!WinUI3::Private::insideOpaqueCard(ui.previewButton));
            const QColor cardTone =
                    WinUI3::Private::cardRefillTone(ui.densityComparisonGroup->palette(), true);
            QCOMPARE(cardTone.alpha(),
                     ui.densityComparisonGroup->palette().color(QPalette::Base).alpha());
            const QRect rowRect =
                    ui.standardDensityNav->visualItemRect(ui.standardDensityNav->item(0));
            QVERIFY(!rowRect.isEmpty());
            QAction *fileAction = ui.standardDensityMenu->actions().value(0);
            QVERIFY(fileAction);
            const QRect actionRect = ui.standardDensityMenu->actionGeometry(fileAction);
            QVERIFY(!actionRect.isEmpty());
            QImage grab(window.size(), QImage::Format_ARGB32_Premultiplied);
            grab.fill(Qt::transparent);
            window.render(&grab);
            QCOMPARE(grab.size(), window.size());
            const QPoint rowProbe = ui.standardDensityNav->viewport()->mapTo(
                    &window, QPoint(4, rowRect.center().y()));
            QVERIFY(grab.pixelColor(rowProbe).alpha() != 0);
            QVERIFY(grab.pixelColor(ui.standardDensityMenu->mapTo(&window, actionRect.center()))
                            .alpha()
                    != 0);
            QVERIFY(WinUI3::Private::insideOpaqueCard(ui.compactDensityNav));
            const QRect compactRowRect =
                    ui.compactDensityNav->visualItemRect(ui.compactDensityNav->item(0));
            QVERIFY(!compactRowRect.isEmpty());
            QCOMPARE(compactRowRect.height(), 32);
            ui.pages->setCurrentWidget(ui.paletteLabPage);
            qApp->processEvents();
            QImage labGrab(window.size(), QImage::Format_ARGB32_Premultiplied);
            labGrab.fill(Qt::transparent);
            window.render(&labGrab);
            QCOMPARE(labGrab.size(), window.size());
            QVERIFY(labGrab.pixelColor(
                                   ui.previewEdit->mapTo(&window, ui.previewEdit->rect().center()))
                            .alpha()
                    != 0);
            // The edit's rounded corner never reaches (0,0): uniform material.
            QCOMPARE(labGrab.pixelColor(ui.previewEdit->mapTo(&window, QPoint(0, 0))).alpha(), 0);
            // Bare preview strip (no child covers the panel's bottom): uniform
            // material, never a veil (RED pre-revert: recipe-1 refill). The
            // viewport/menu filler probes below stay veiled (real card parents).
            const QPoint bareCard = ui.palettePreview->mapTo(
                    &window,
                    QPoint(ui.palettePreview->width() / 2, ui.palettePreview->height() - 8));
            QCOMPARE(labGrab.pixelColor(bareCard).alpha(), 0);
            const QPoint bareView = ui.standardDensityNav->viewport()->mapTo(
                    &window, QPoint(4, ui.standardDensityNav->viewport()->height() - 4));
            if (ui.standardDensityNav
                        ->visualItemRect(
                                ui.standardDensityNav->item(ui.standardDensityNav->count() - 1))
                        .bottom()
                < ui.standardDensityNav->viewport()->height() - 8) {
                QVERIFY(grab.pixelColor(bareView).alpha() != 0);
            }
            // The standard panel pins Standard rows through winuiDensity no
            // matter the global mode (per-widget walk beats the global mode).
            QCOMPARE(rowRect.height(), 40);
            // Live-vs-harness state pins: the sync must never transparentize
            // anything inside the card chain (roles stay opaque; only islands
            // and chrome go material). Compact twins probed too Ã¢â‚¬â€ they share
            // the live page.
            QVERIFY(ui.standardDensityNav->palette().color(QPalette::Base).alpha() != 0);
            QVERIFY(ui.standardDensityNav->viewport()->palette().color(QPalette::Base).alpha()
                    != 0);
            QVERIFY(ui.standardDensityNav->viewport()->autoFillBackground());
            QVERIFY(ui.standardDensityMenu->palette().color(QPalette::Base).alpha() != 0);
            QVERIFY(ui.standardDensityPassword->palette().color(QPalette::Base).alpha() != 0);
            QVERIFY(!ui.standardDensityPanel->testAttribute(Qt::WA_StyledBackground));
            // Live-only actions the static probes above never perform. Rows 7-9
            // sit below the fold: scroll the page there (blit + exposed-strip
            // repaints, scrollbar heal guard) and probe the revealed
            // Navigation/MenuBar/Password samples, then toggle the backdrop off
            // and on again (restore mirror + re-arm) and re-probe everything.
            // Render-based probes repaint fully, so any persistent hole they
            // show is paint state, not a transient blit.
            ui.pages->setCurrentWidget(ui.controlsPage);
            qApp->processEvents();
            auto *controlsScroll = ui.controlsPage;
            QVERIFY(controlsScroll->verticalScrollBar());
            controlsScroll->verticalScrollBar()->setValue(
                    controlsScroll->verticalScrollBar()->maximum());
            qApp->processEvents();
            const QRect scrolledRowRect =
                    ui.standardDensityNav->visualItemRect(ui.standardDensityNav->item(0));
            QVERIFY(!scrolledRowRect.isEmpty());
            QImage scrolledGrab(window.size(), QImage::Format_ARGB32_Premultiplied);
            scrolledGrab.fill(Qt::transparent);
            window.render(&scrolledGrab);
            const QPoint scrolledRowProbe = ui.standardDensityNav->viewport()->mapTo(
                    &window, QPoint(4, scrolledRowRect.center().y()));
            if (ui.standardDensityNav->viewport()->rect().contains(
                        ui.standardDensityNav->viewport()->mapFrom(&window, scrolledRowProbe))) {
                QVERIFY(scrolledGrab.pixelColor(scrolledRowProbe).alpha() != 0);
            }
            // Same-row twin in the compact panel, now scrolled into view.
            const QPoint compactRowProbe = ui.compactDensityNav->viewport()->mapTo(
                    &window, QPoint(4, compactRowRect.center().y()));
            if (QRect(QPoint(0, 0), window.size()).contains(compactRowProbe))
                QVERIFY(scrolledGrab.pixelColor(compactRowProbe).alpha() != 0);
            QVERIFY(grab.pixelColor(ui.standardDensityMenu->mapTo(&window, actionRect.center()))
                            .alpha()
                    != 0);
            const QPoint passwordProbe = ui.standardDensityPassword->mapTo(
                    &window, ui.standardDensityPassword->rect().center());
            if (ui.standardDensityPassword->isVisibleTo(&window)
                && QRect(QPoint(0, 0), window.size()).contains(passwordProbe)) {
                QVERIFY(scrolledGrab.pixelColor(passwordProbe).alpha() != 0);
            }
            WinUI3::Private::restoreChromeSurfaces(&window);
            WinUI3::Private::restoreContentSurfacesForBackdrop(&window);
            qApp->processEvents();
            QVERIFY(!ui.palettePreview->testAttribute(Qt::WA_StyledBackground));
            window.setProperty("_winui_backdrop", 1);
            window.setProperty("_winui_backdrop_effective", 2); // Composited
            WinUI3::Private::makeChromeSurfacesTransparent(&window);
            WinUI3::Private::syncContentSurfacesForBackdrop(&window);
            qApp->processEvents();
            // No veil arm anymore: the bare panel never gains StyledBackground.
            QVERIFY(!ui.palettePreview->testAttribute(Qt::WA_StyledBackground));
            ui.pages->setCurrentWidget(ui.paletteLabPage);
            qApp->processEvents();
            QImage retoggledGrab(window.size(), QImage::Format_ARGB32_Premultiplied);
            retoggledGrab.fill(Qt::transparent);
            window.render(&retoggledGrab);
            QVERIFY(retoggledGrab
                            .pixelColor(
                                    ui.previewEdit->mapTo(&window, ui.previewEdit->rect().center()))
                            .alpha()
                    != 0);
            QVERIFY(retoggledGrab.pixelColor(bareCard).alpha() == 0);
        }
    }
    style->setDensityMode(previousDensity);
    style->setThemeMode(WinUI3::ThemeMode::Light);
}

void WinUI3MicaCardTest::treeSelectionKeepsCardFillOverComposited()
{
    // TreeView selection overlay (reviewer dark-Mica screenshot): contiguous
    // selected rows inside a group card must not punch material holes at
    // the pill seams and rounded corners — each selected/hovered row erases
    // its full rect but paints an inset rounded pill, so without a card-tone
    // rebuild the corners and inter-row gaps read alpha 0 (visible edges
    // around the selection overlay). Dark/light x Standard/Compact; every
    // pixel assert pairs with a token/geometry assert on the same state.
    DisableAnimationsGuard animations;
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    const WinUI3::DensityMode previousDensity = style->densityMode();
    const auto densities = { WinUI3::DensityMode::Standard, WinUI3::DensityMode::Compact };
    for (const bool dark : { false, true }) {
        style->setThemeMode(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
        for (const WinUI3::DensityMode density : densities) {
            style->setDensityMode(density);
            QWidget window; // + content island + group card + tree, 3 rows selected
            window.setProperty(WinUI3::Style::BackdropProperty, QStringLiteral("mica"));
            auto *island = new QWidget(&window);
            island->setProperty(WinUI3::Style::SurfaceProperty, QStringLiteral("content"));
            island->setGeometry(0, 0, 480, 360);
            auto *card = new QGroupBox(QStringLiteral("Tracks"), island);
            card->setGeometry(20, 20, 300, 240);
            auto *tree = new QTreeWidget(card);
            tree->setHeaderHidden(true);
            tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
            tree->setGeometry(10, 30, 280, 180);
            for (const QString &track :
                 { QStringLiteral("01 — First track"), QStringLiteral("02 — Second track"),
                   QStringLiteral("03 — Third track") }) {
                auto *item = new QTreeWidgetItem(tree, { track });
                item->setSelected(true);
            }
            tree->setCurrentItem(tree->topLevelItem(0));
            window.resize(480, 360);
            window.show();
            QVERIFY(QTest::qWaitForWindowExposed(&window));
            window.setProperty("_winui_backdrop", 1);
            window.setProperty("_winui_backdrop_effective", 2); // Composited
            WinUI3::Private::makeChromeSurfacesTransparent(&window);
            WinUI3::Private::syncContentSurfacesForBackdrop(&window);
            qApp->processEvents();
            // Live-path pins: the gate passes for the viewport (RED if the
            // harness diverges live), rows are contiguous and equal height.
            // Diagnostic guards (applyBackdrop race): alternating must stay
            // off (default) and the island must still be transparent right
            // before render — a deferred applyBackdrop re-opaquing it would
            // stack fills instead of erasing them.
            QVERIFY(!tree->alternatingRowColors());
            QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(tree->viewport()));
            QCOMPARE(island->palette().color(QPalette::Window).alpha(), 0);
            const QRect row0 = tree->visualItemRect(tree->topLevelItem(0));
            QVERIFY(!row0.isEmpty());
            const QRect row1 = tree->visualItemRect(tree->topLevelItem(1));
            QVERIFY(!row1.isEmpty());
            QCOMPARE(row0.height(), row1.height());
            QCOMPARE(row1.top(), row0.bottom() + 1);
            // Viewport sync pins: the autofill stops (no double-fill over
            // the card) while roles stay intact for tone reads; the PE
            // branch repaints the card tone itself.
            QCOMPARE(tree->viewport()->autoFillBackground(), false);
            QCOMPARE(tree->viewport()->testAttribute(Qt::WA_StyledBackground), true);
            const QColor cardTone = WinUI3::Private::veiledCard(
                    WinUI3::Private::tokens(card->palette()).layer, true);
            QImage grab(window.size(), QImage::Format_ARGB32_Premultiplied);
            grab.fill(Qt::transparent);
            window.render(&grab);
            QCOMPARE(grab.size(), window.size());
            const int cx = row0.center().x();
            // Inter-row seam: pill of row0 ends 2px above its bottom edge,
            // pill of row1 starts 2px below its top edge — the seam pixels
            // belong to no pill (RED pre-fix: alpha 0 material dots).
            const QPoint seamProbe = tree->viewport()->mapTo(&window, QPoint(cx, row0.bottom()));
            const QColor seamInk = grab.pixelColor(seamProbe);
            QCOMPARE(seamInk.alpha(), cardTone.alpha());
            QVERIFY(colorDistance(seamInk, cardTone) <= 8);
            // Row corner: the rounded pill starts 4px/2px inside the rect.
            const QPoint cornerProbe =
                    tree->viewport()->mapTo(&window, row1.topLeft() + QPoint(1, 1));
            const QColor cornerInk = grab.pixelColor(cornerProbe);
            QCOMPARE(cornerInk.alpha(), cardTone.alpha());
            QVERIFY(colorDistance(cornerInk, cardTone) <= 8);
        }
    }
    style->setDensityMode(previousDensity);
    style->setThemeMode(WinUI3::ThemeMode::Light);
}

void WinUI3MicaCardTest::disabledInCardButtonKeepsCardToneOverComposited()
{
    // Disabled Standard button + disabled QCommandLinkButton inside an
    // opaque group card under a Composited backdrop (user dark-Mica
    // screenshots: "Disabled" button and "Unavailable command" painted an
    // opaque light slab over the dark material). The disabled option
    // palette resolves t.layer to Disabled Base (dark white@8), and the
    // in-card refill must min-clamp it (veiledCard, scrollbar precedent)
    // instead of forcing it to 178 (cardVeil -> white@178 slab). Dark /
    // light x Standard/Compact; no grab(): seeded-canvas mechanism probes.
    DisableAnimationsGuard animations;
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    const WinUI3::DensityMode previousDensity = style->densityMode();
    const auto densities = { WinUI3::DensityMode::Standard, WinUI3::DensityMode::Compact };
    for (const bool dark : { false, true }) {
        style->setThemeMode(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
        for (const WinUI3::DensityMode density : densities) {
            style->setDensityMode(density);
            QWidget window; // + content island + group card + disabled button pair
            window.setProperty(WinUI3::Style::BackdropProperty, QStringLiteral("mica"));
            auto *island = new QWidget(&window);
            island->setProperty(WinUI3::Style::SurfaceProperty, QStringLiteral("content"));
            island->setGeometry(0, 0, 480, 360);
            auto *card = new QGroupBox(QStringLiteral("Dialogs"), island);
            card->setGeometry(20, 20, 300, 240);
            auto *button = new QPushButton(QStringLiteral("Disabled"), card);
            button->setEnabled(false);
            button->setGeometry(10, 30, 160, 32);
            auto *commandLink =
                    new QCommandLinkButton(QStringLiteral("Unavailable command"),
                                           QStringLiteral("This operation is disabled."), card);
            commandLink->setEnabled(false);
            commandLink->setGeometry(10, 80, 280, 64);
            window.resize(480, 360);
            window.show();
            QVERIFY(QTest::qWaitForWindowExposed(&window));
            window.setProperty("_winui_backdrop", 1);
            window.setProperty("_winui_backdrop_effective", 2); // Composited
            WinUI3::Private::syncContentSurfacesForBackdrop(&window);
            qApp->processEvents();
            QVERIFY(WinUI3::Private::insideOpaqueCard(button));
            QVERIFY(WinUI3::Private::insideOpaqueCard(commandLink));
            QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(button));
            // Token pins on the same state: the card paints veiledCard of
            // the Active Base; the disabled option resolves Disabled Base.
            const QColor cardTone = WinUI3::Private::veiledCard(
                    WinUI3::Private::tokens(card->palette()).layer, true);
            QStyleOptionButton disabledOption;
            disabledOption.initFrom(button);
            disabledOption.rect = button->rect();
            disabledOption.state &= ~QStyle::State_Enabled;
            disabledOption.text = button->text();
            const QColor disabledLayer = WinUI3::Private::tokens(disabledOption.palette).layer;
            QCOMPARE(disabledLayer, button->palette().color(QPalette::Disabled, QPalette::Base));
            if (dark)
                QCOMPARE(disabledLayer.alpha(), 8);
            // Geometry pin: the button rect is the erase/refill site.
            QCOMPARE(button->geometry(), QRect(10, 30, 160, 32));
            const auto paintDisabled = [&](QWidget *target, const QStyleOptionButton &opt) {
                QImage canvas(target->size(), QImage::Format_ARGB32_Premultiplied);
                canvas.fill(cardTone);
                QPainter painter(&canvas);
                style->drawPrimitive(QStyle::PE_PanelButtonCommand, &opt, &painter, target);
                painter.end();
                return canvas;
            };
            const QImage buttonFrame = paintDisabled(button, disabledOption);
            // True corner cutout (0,0): outside the ControlRadius rounded
            // surface (arc center (4,4) r4 leaves (0,0) uncovered — (1,1) is
            // inside the shape and takes the full disabled fill), so it
            // reads the square refill exactly: the card tone, not a slab.
            const QColor haloInk = buttonFrame.pixelColor(0, 0);
            QCOMPARE(haloInk.alpha(), cardTone.alpha());
            QVERIFY(colorDistance(haloInk, cardTone) <= 4);
            // Center: the designed disabled ink (controlDisabled token)
            // SourceOver the exact card refill — faint lightening, never a
            // slab. Pin the composite, not a radius: a token change must
            // update this test explicitly.
            const QColor centerInk = buttonFrame.pixelColor(buttonFrame.rect().center());
            QVERIFY(centerInk.alpha() != 0);
            const QColor disabledInk =
                    WinUI3::Private::tokens(disabledOption.palette).controlDisabled;
            const qreal aS = disabledInk.alphaF();
            const qreal aD = cardTone.alphaF();
            const qreal aOut = aS + aD * (1.0 - aS);
            QColor expectedCenter;
            expectedCenter.setAlphaF(aOut);
            expectedCenter.setRedF((disabledInk.redF() * aS + cardTone.redF() * aD * (1.0 - aS))
                                   / aOut);
            expectedCenter.setGreenF(
                    (disabledInk.greenF() * aS + cardTone.greenF() * aD * (1.0 - aS)) / aOut);
            expectedCenter.setBlueF((disabledInk.blueF() * aS + cardTone.blueF() * aD * (1.0 - aS))
                                    / aOut);
            QVERIFY(colorDistance(centerInk, expectedCenter) <= 8);
            QStyleOptionButton linkOption;
            linkOption.initFrom(commandLink);
            linkOption.rect = commandLink->rect();
            linkOption.state &= ~QStyle::State_Enabled;
            linkOption.text = commandLink->text();
            const QImage linkFrame = paintDisabled(commandLink, linkOption);
            const QColor linkHalo = linkFrame.pixelColor(0, 0);
            QCOMPARE(linkHalo.alpha(), cardTone.alpha());
            QVERIFY(colorDistance(linkHalo, cardTone) <= 4);
        }
    }
    style->setDensityMode(previousDensity);
    style->setThemeMode(WinUI3::ThemeMode::Light);
}

QTEST_MAIN(WinUI3MicaCardTest)
#include "tst_winui3micacard.moc"
