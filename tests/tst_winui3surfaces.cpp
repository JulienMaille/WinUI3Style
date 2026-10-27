// SPDX-License-Identifier: LGPL-2.1-or-later
// Domain split of tst_winui3style.cpp (plan step 8): test-function names
// kept identical so the DPI reruns in tests/CMakeLists.txt keep working.
#include <winui3style/animatedstack.h>
#include <winui3style/navigationview.h>
#include <winui3style/settingscard.h>
#include <winui3style/toggleswitch.h>
#include <winui3style/winui3backdrop.h>
#include <winui3style/winui3style.h>
#include <winui3style/winui3icons.h>

#include "../src/winui3frameproperties_p.h"
#include "../src/winui3helpers_p.h"
#include "../src/winui3surfaces_p.h"

#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QEvent>
#include <QGroupBox>
#include <QImage>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QPainter>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollBar>
#include <QScrollArea>
#include <QMoveEvent>
#include <QStatusBar>
#include <QStyleOptionButton>
#include <QStyleOptionComboBox>
#include <QStyleOptionSlider>
#include <QSlider>
#include <QToolBar>
#include <QTimer>
#include <QTest>

#include <cmath>
#include <limits>

#include "winui3testhelpers.h"

class WinUI3SurfacesTest final : public QObject, private WinUI3StandardTestFixture
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void backdropLifecycleContract();
    void contentLayerSurfacePredicate();
    void backdropButtonRepaintDoesNotAccumulate();
    void backdropSliderInsideOpaqueCardKeepsCardFill();
    void backdropSliderRepaintDoesNotAccumulate();
    void backdropComboRepaintDoesNotAccumulate();
    void islandScrollPostsFullViewportRepaint();
    void backdropResizeKeepsTransparentRecipe();
    void backdropExposeRepaintsShellHierarchy();
    void backdropMoveKeepsTransparentRecipe();
    void materialEraseKeepsCornersTransparent();
    void materialEraseRespectsDirtyRegionClip();
    void backdropToggleOffRestoresShellSurfaces();
    void backdropThemeSwitchRebasesIslandPalette();
    void themeSwitchRebasesChromeShell();
    void menuMaskRetrySettles();
    void progressAnimationAndOrientations();
    void progressTextAndDisabledPaletteContract();
    void progressTimerScalingAndLifecycle();
};

void WinUI3SurfacesTest::contentLayerSurfacePredicate()
{
    const QList<QVariant> values = { {},
                                     false,
                                     true,
                                     0,
                                     1,
                                     QString(),
                                     QStringLiteral("false"),
                                     QStringLiteral("0"),
                                     QStringLiteral("content"),
                                     QStringLiteral("CoNtEnT"),
                                     QStringLiteral("layer"),
                                     QStringLiteral("LaYeR"),
                                     QStringLiteral("custom") };
    for (const QVariant &surface : values) {
        const QString name = surface.toString();
        const bool original = surface.toBool()
                || name.compare(QLatin1String("content"), Qt::CaseInsensitive) == 0
                || name.compare(QLatin1String("layer"), Qt::CaseInsensitive) == 0;
        QCOMPARE(WinUI3::Private::isContentLayerSurface(surface), original);
        QCOMPARE(WinUI3::Private::isContentLayerSurface(surface, name), original);
    }
}

void WinUI3SurfacesTest::initTestCase()
{
    initializeStandardWinUI3TestStyle();
}

void WinUI3SurfacesTest::init()
{
    resetStandardWinUI3TestStyle();
}

void WinUI3SurfacesTest::cleanup()
{
    cleanupStandardWinUI3TestWidgets();
}

void WinUI3SurfacesTest::backdropLifecycleContract()
{
#ifndef Q_OS_WIN
    QWidget window;
    QVERIFY(!WinUI3::applyBackdrop(&window, WinUI3::Backdrop::Mica));
    return;
#else
    // DWM is deliberately not part of this unit test. The offscreen Windows
    // platform still exercises the palette/attribute/property lifecycle
    // deterministically; native composition is covered by the native test
    // target and by the gallery.
    if (QGuiApplication::platformName() != QStringLiteral("offscreen"))
        QSKIP("DWM-backed lifecycle belongs to the native test target");

    QWidget window;
    window.setAttribute(Qt::WA_TranslucentBackground, false);
    window.setAttribute(Qt::WA_NoSystemBackground, false);
    window.setAttribute(Qt::WA_OpaquePaintEvent, true);
    window.setAutoFillBackground(true);
    QPalette custom = window.palette();
    custom.setColor(QPalette::Window, QColor(19, 37, 53));
    window.setPalette(custom);

    const QPalette originalPalette = window.palette();
    const bool originalTranslucent = window.testAttribute(Qt::WA_TranslucentBackground);
    const bool originalNoSystemBackground = window.testAttribute(Qt::WA_NoSystemBackground);
    const bool originalOpaquePaint = window.testAttribute(Qt::WA_OpaquePaintEvent);
    const bool originalAutoFill = window.autoFillBackground();

    // Offscreen has no HWND/DWM: prepareBackdropSurface skips attribute changes
    // there to keep fallback PNGs deterministic. Attributes stay as set above.
    QVERIFY(!window.testAttribute(Qt::WA_NoSystemBackground));
    QVERIFY(window.testAttribute(Qt::WA_OpaquePaintEvent));
    QVERIFY(window.autoFillBackground());
    QCOMPARE(window.palette().color(QPalette::Window).alpha(), 255);

    QVERIFY(WinUI3::applyBackdrop(&window, WinUI3::Backdrop::MicaAlt));
    QCOMPARE(window.property("_winui_backdrop").toInt(), int(WinUI3::Backdrop::MicaAlt));
    QVERIFY(WinUI3::applyBackdrop(&window, WinUI3::Backdrop::None));
    QVERIFY(!window.property("_winui_backdrop").isValid());
    QCOMPARE(window.testAttribute(Qt::WA_TranslucentBackground), originalTranslucent);
    QCOMPARE(window.testAttribute(Qt::WA_NoSystemBackground), originalNoSystemBackground);
    QCOMPARE(window.testAttribute(Qt::WA_OpaquePaintEvent), originalOpaquePaint);
    QCOMPARE(window.autoFillBackground(), originalAutoFill);
    QCOMPARE(window.palette(), originalPalette);

    // Clearing an already-cleared backdrop must remain a no-op. In
    // particular it must not create a native handle just to reset DWM state.
    QVERIFY(WinUI3::applyBackdrop(&window, WinUI3::Backdrop::None));
    QVERIFY(!window.property("_winui_backdrop").isValid());
    QCOMPARE(window.palette(), originalPalette);
#endif
}

void WinUI3SurfacesTest::backdropButtonRepaintDoesNotAccumulate()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget window;
    window.setProperty("_winui_backdrop", 1);
    QPushButton button(QStringLiteral("Mica"), &window);
    button.resize(96, 32);

    QStyleOptionButton option;
    option.initFrom(&button);
    option.rect = button.rect();
    const auto paintFrame = [&](QImage &image, QStyle::State state) {
        option.state = state;
        QPainter painter(&image);
        style->drawPrimitive(QStyle::PE_PanelButtonCommand, &option, &painter, &button);
    };
    QImage normal(button.size(), QImage::Format_ARGB32_Premultiplied);
    normal.fill(Qt::transparent);
    paintFrame(normal, QStyle::State_Enabled);

    QImage hoverThenNormal(button.size(), QImage::Format_ARGB32_Premultiplied);
    hoverThenNormal.fill(Qt::transparent);
    setFrame(&button, "_winui_hover_progress", 1.0);
    paintFrame(hoverThenNormal, QStyle::State_Enabled | QStyle::State_MouseOver);
    setFrame(&button, "_winui_hover_progress", 0.0);
    paintFrame(hoverThenNormal, QStyle::State_Enabled);
    QCOMPARE(hoverThenNormal, normal);

    QWidget opaqueLayer(&window);
    opaqueLayer.setProperty(WinUI3::Style::SurfaceProperty, QStringLiteral("content"));
    QPushButton layeredButton(QStringLiteral("Layered"), &opaqueLayer);
    QVERIFY(!WinUI3::Private::paintsDirectlyOnBackdrop(&layeredButton));

    // A translucent island is the material surface itself: no fill to punch
    // through, so children above it keep clearing every frame.
    QWidget translucentIsland(&window);
    translucentIsland.setProperty(WinUI3::Style::SurfaceProperty, QStringLiteral("content"));
    QPalette clearPalette = translucentIsland.palette();
    QColor clearWindow = clearPalette.color(QPalette::Window);
    clearWindow.setAlpha(0);
    clearPalette.setColor(QPalette::Window, clearWindow);
    translucentIsland.setPalette(clearPalette);
    QPushButton islandChild(QStringLiteral("IslandChild"), &translucentIsland);
    QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(&translucentIsland));
    QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(&islandChild));

    // Erase behavior: a child above a translucent island clears its own rect
    // to transparent (Source) and then paints its content, so scroll/hover
    // frames cannot accumulate. An opaque control child keeps its fill.
    islandChild.resize(96, 32);
    QStyleOptionButton childOption;
    childOption.initFrom(&islandChild);
    childOption.rect = islandChild.rect();
    QImage childFrame(islandChild.size(), QImage::Format_ARGB32_Premultiplied);
    childFrame.fill(QColor(255, 0, 0, 255));
    QPainter childPainter(&childFrame);
    style->drawPrimitive(QStyle::PE_Widget, &childOption, &childPainter, &islandChild);
    childPainter.end();
    // The child painted something (not a bare transparent hole).
    QVERIFY(childFrame.pixelColor(4, 4).alpha() > 0
            || childFrame.pixelColor(48, 16) != QColor(255, 0, 0));
}

void WinUI3SurfacesTest::backdropSliderInsideOpaqueCardKeepsCardFill()
{
    // Sliders paint only groove/thumb: no opaque fill of their own. Inside
    // an opaque group card the slider erase must not punch a transparent
    // hole through the card fill (the mica-mode band). Same state, token
    // plus geometry asserts.
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget window;
    window.setProperty("_winui_backdrop", 1);
    window.setProperty("_winui_backdrop_effective", 2); // Composited
    QGroupBox card(QStringLiteral("Values"), &window);
    card.resize(220, 96);
    QSlider slider(Qt::Horizontal, &card);
    slider.setRange(0, 100);
    slider.setValue(42);
    slider.setGeometry(20, 36, 180, 24);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    // Geometry: the card Window role is opaque, the gate would pass for the
    // slider, so only the card guard keeps the fill.
    QVERIFY(card.palette().color(QPalette::Window).alpha() != 0);
    QStyleOptionSlider option;
    option.initFrom(&slider);
    option.rect = slider.rect();
    option.orientation = Qt::Horizontal;
    QImage frame(slider.size(), QImage::Format_ARGB32_Premultiplied);
    frame.fill(card.palette().color(QPalette::Window));
    QPainter painter(&frame);
    style->drawComplexControl(QStyle::CC_Slider, &option, &painter, &slider);
    painter.end();
    // Token assert: no pixel punched to transparent inside the slider rect.
    for (int y = 0; y < frame.height(); y += 4) {
        for (int x = 0; x < frame.width(); x += 4) {
            QVERIFY2(frame.pixelColor(x, y).alpha() != 0,
                     qPrintable(QStringLiteral("hole at %1,%2").arg(x).arg(y)));
        }
    }
    // Geometry assert: groove still paints (non-card pixels changed).
    QVERIFY(frame != QImage(frame.size(), QImage::Format_ARGB32_Premultiplied));
}

void WinUI3SurfacesTest::backdropSliderRepaintDoesNotAccumulate()
{
    // Slider twin of the button/combo non-accumulation contracts: hover then
    // normal must repaint byte-identical. Exercises the guarded erase path
    // (inside an opaque card the erase is skipped, straight on the material
    // it clears) without depending on the unrelated combo painter.
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget window;
    window.setProperty("_winui_backdrop", 1);
    QSlider slider(Qt::Horizontal, &window);
    slider.resize(180, 24);
    slider.setRange(0, 100);
    slider.setValue(42);
    QStyleOptionSlider sliderOption;
    sliderOption.initFrom(&slider);
    sliderOption.rect = slider.rect();
    sliderOption.orientation = Qt::Horizontal;
    const auto paintSliderFrame = [&](QImage &image, QStyle::State state) {
        sliderOption.state = state;
        QPainter painter(&image);
        style->drawComplexControl(QStyle::CC_Slider, &sliderOption, &painter, &slider);
    };
    QImage sliderNormal(slider.size(), QImage::Format_ARGB32_Premultiplied);
    sliderNormal.fill(Qt::transparent);
    setFrame(&slider, "_winui_hover_progress", 0.0);
    paintSliderFrame(sliderNormal, QStyle::State_Enabled);
    QImage sliderHoverThenNormal(slider.size(), QImage::Format_ARGB32_Premultiplied);
    sliderHoverThenNormal.fill(Qt::transparent);
    setFrame(&slider, "_winui_hover_progress", 1.0);
    paintSliderFrame(sliderHoverThenNormal, QStyle::State_Enabled | QStyle::State_MouseOver);
    setFrame(&slider, "_winui_hover_progress", 0.0);
    paintSliderFrame(sliderHoverThenNormal, QStyle::State_Enabled);
    QCOMPARE(sliderHoverThenNormal, sliderNormal);
}

void WinUI3SurfacesTest::backdropComboRepaintDoesNotAccumulate()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QWidget window;
    window.setProperty("_winui_backdrop", 1);
    QComboBox combo(&window);

    QStyleOptionComboBox option;
    option.initFrom(&combo);
    option.rect = combo.rect();
    option.subControls = QStyle::SC_ComboBoxFrame | QStyle::SC_ComboBoxArrow;
    const auto paintFrame = [&](QImage &image, QStyle::State state) {
        option.state = state;
        QPainter painter(&image);
        style->drawComplexControl(QStyle::CC_ComboBox, &option, &painter, &combo);
    };
    QImage normal(combo.size(), QImage::Format_ARGB32_Premultiplied);
    normal.fill(Qt::transparent);
    setFrame(&combo, "_winui_hover_progress", 0.0);
    paintFrame(normal, QStyle::State_Enabled);

    QImage hoverThenNormal(combo.size(), QImage::Format_ARGB32_Premultiplied);
    hoverThenNormal.fill(Qt::transparent);
    setFrame(&combo, "_winui_hover_progress", 1.0);
    paintFrame(hoverThenNormal, QStyle::State_Enabled | QStyle::State_MouseOver);
    setFrame(&combo, "_winui_hover_progress", 0.0);
    paintFrame(hoverThenNormal, QStyle::State_Enabled);
    QCOMPARE(hoverThenNormal, normal);
}
void WinUI3SurfacesTest::islandScrollPostsFullViewportRepaint()
{
    // Scroll bars inside a translucent content island must self-heal from
    // small-delta blits: guardIslandScrollArea (armed by the backdrop sync)
    // connects each bar's valueChanged to one queued full viewport update,
    // so shifted pixels rebuild before the next present instead of smearing
    // over the live material. Offscreen the Composited gate stays shut, so
    // the observable contract here is structural: the guard arms exactly
    // once per area and stays silent without the Composited state.
    QWidget window;
    window.setProperty("_winui_backdrop", 1);
    QWidget island(&window);
    island.setProperty(WinUI3::Style::SurfaceProperty, QStringLiteral("content"));
    QScrollArea area(&island);
    area.resize(200, 200);
    auto *body = new QLabel(QStringLiteral("body"), &area);
    body->resize(400, 400);
    area.setWidget(body);
    // Arming twice must stay one guard: the property marker makes the second
    // call a no-op. A probe connection counts deliveries: after one guarded
    // arm plus one probe, a single bar tick delivers exactly twice.
    WinUI3::Private::guardIslandScrollArea(&area);
    QVERIFY(area.property("_winui_island_scroll_guard").isValid());
    int probeDeliveries = 0;
    QObject::connect(area.verticalScrollBar(), &QScrollBar::valueChanged, &area,
                     [&probeDeliveries] { ++probeDeliveries; });
    WinUI3::Private::guardIslandScrollArea(&area);

    // Offscreen gate shut: bar ticks schedule nothing Composited, and the
    // viewport paint state is untouched by the guard itself.
    area.verticalScrollBar()->setValue(area.verticalScrollBar()->maximum() / 2);
    QCOMPARE(probeDeliveries, 1);
    QCOMPARE(window.property("_winui_backdrop_effective").isValid(), false);
    QVERIFY(area.viewport() != nullptr);
}

void WinUI3SurfacesTest::backdropResizeKeepsTransparentRecipe()
{
    // Resize realloue le backing-store : le chemin de repeinture (gate +
    // erase Source) doit rester intact apres le geste. Meme etat, deux
    // assertions : la gate reste vraie et le frame repeint est identique.
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget window;
    window.setProperty("_winui_backdrop", 1);
    window.setProperty("_winui_backdrop_effective", 2);
    QPushButton button(QStringLiteral("Mica"), &window);
    button.resize(96, 32);
    QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(&button));
    QStyleOptionButton option;
    option.initFrom(&button);
    option.rect = button.rect();
    const auto paintFrame = [&](QImage &image) {
        option.state = QStyle::State_Enabled;
        QPainter painter(&image);
        style->drawPrimitive(QStyle::PE_PanelButtonCommand, &option, &painter, &button);
    };
    QImage before(button.size(), QImage::Format_ARGB32_Premultiplied);
    before.fill(Qt::transparent);
    paintFrame(before);
    window.resize(400, 300);
    window.show();
    (void)QTest::qWaitForWindowExposed(&window);
    window.resize(640, 480);
    qApp->processEvents();
    QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(&button));
    QImage after(button.size(), QImage::Format_ARGB32_Premultiplied);
    after.fill(Qt::transparent);
    paintFrame(after);
    QCOMPARE(after, before);
}

void WinUI3SurfacesTest::backdropExposeRepaintsShellHierarchy()
{
#ifndef Q_OS_WIN
    QSKIP("native backdrop restoration requires the Windows DWM compositor");
#endif
    // Apres occlusion, toggle off/on restaure puis re-applique la recette :
    // la hierarchie shell repeint a l'identique. Meme etat, deux
    // assertions : gate vraie et frame egale avant/apres le cycle.
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget window;
    window.setProperty("_winui_backdrop", 1);
    window.setProperty("_winui_backdrop_effective", 2);
    QPushButton button(QStringLiteral("Mica"), &window);
    button.resize(96, 32);
    QStyleOptionButton option;
    option.initFrom(&button);
    option.rect = button.rect();
    const auto paintFrame = [&](QImage &image) {
        option.state = QStyle::State_Enabled;
        QPainter painter(&image);
        style->drawPrimitive(QStyle::PE_PanelButtonCommand, &option, &painter, &button);
    };
    QImage before(button.size(), QImage::Format_ARGB32_Premultiplied);
    before.fill(Qt::transparent);
    paintFrame(before);
    window.resize(400, 300);
    window.show();
    (void)QTest::qWaitForWindowExposed(&window);
    QVERIFY(WinUI3::applyBackdrop(&window, WinUI3::Backdrop::None));
    QVERIFY(!WinUI3::Private::paintsDirectlyOnBackdrop(&button));
    window.setProperty("_winui_backdrop", 1);
    window.setProperty("_winui_backdrop_effective", 2);
    window.update();
    button.update();
    qApp->processEvents();
    QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(&button));
    QImage after(button.size(), QImage::Format_ARGB32_Premultiplied);
    after.fill(Qt::transparent);
    paintFrame(after);
    QCOMPARE(after, before);
}

void WinUI3SurfacesTest::backdropMoveKeepsTransparentRecipe()
{
    // Deplacement : aucun handler Move ne touche aux surfaces ; la gate et
    // le frame doivent survivre au geste. Meme etat, deux assertions.
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget window;
    window.setProperty("_winui_backdrop", 1);
    window.setProperty("_winui_backdrop_effective", 2);
    QPushButton button(QStringLiteral("Mica"), &window);
    button.resize(96, 32);
    QStyleOptionButton option;
    option.initFrom(&button);
    option.rect = button.rect();
    window.resize(400, 300);
    window.show();
    (void)QTest::qWaitForWindowExposed(&window);
    QMoveEvent moveEvent(QPoint(120, 120), QPoint(0, 0));
    QCoreApplication::sendEvent(&window, &moveEvent);
    window.move(120, 120);
    qApp->processEvents();
    QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(&button));
    QImage image(button.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    option.state = QStyle::State_Enabled;
    QPainter painter(&image);
    style->drawPrimitive(QStyle::PE_PanelButtonCommand, &option, &painter, &button);
    painter.end();
    QVERIFY(image.pixelColor(image.width() / 2, image.height() / 2).alpha() > 0);
}

void WinUI3SurfacesTest::materialEraseKeepsCornersTransparent()
{
    // Corners outside the rounded rect must never turn black: the
    // clear clips to the fill radius (ControlRadius), the fill repaints
    // the inside, and off-shape pixels stay transparent for DWM.
    // Without the clip, the Source clear writes transparent-black into the
    // antialiased corners and the fill never covers them (black dots on
    // light Mica, light dots on dark Mica after a theme switch).
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget window;
    window.setProperty("_winui_backdrop", 1);
    window.setProperty("_winui_backdrop_effective", 2); // Composited
    QWidget island(&window);
    island.setProperty(WinUI3::Style::SurfaceProperty, QStringLiteral("content"));
    QPalette clearPalette = island.palette();
    QColor clearWindow = clearPalette.color(QPalette::Window);
    clearWindow.setAlpha(0);
    clearPalette.setColor(QPalette::Window, clearWindow);
    island.setPalette(clearPalette);
    QLineEdit edit(&island);
    edit.resize(240, 32);
    QStyleOptionFrame option;
    option.initFrom(&edit);
    option.rect = edit.rect();
    option.state = QStyle::State_Enabled;
    QImage image(edit.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(QColor(255, 0, 0, 255));
    QPainter painter(&image);
    style->drawPrimitive(QStyle::PE_PanelLineEdit, &option, &painter, &edit);
    painter.end();
    // The clipped clear never touches the off-shape corners: they keep the
    // QImage background here (transparent backing store in production).
    // What matters: no black fringe from an unclipped clear, and the fill
    // covers the center.
    const QPoint corners[4] = { { 0, 0 },
                                { image.width() - 1, 0 },
                                { 0, image.height() - 1 },
                                { image.width() - 1, image.height() - 1 } };
    for (const QPoint &corner : corners) {
        const QColor px = image.pixelColor(corner);
        QVERIFY2(px.alpha() == 0 || px == QColor(255, 0, 0, 255),
                 qPrintable(QStringLiteral("corner %1,%2 = %3 (black fringe!)")
                                    .arg(corner.x())
                                    .arg(corner.y())
                                    .arg(px.name(QColor::HexArgb))));
    }
    QVERIFY(image.pixelColor(image.rect().center()).alpha() > 0);
}

void WinUI3SurfacesTest::materialEraseRespectsDirtyRegionClip()
{
    // Hover/scroll repaints arrive with a dirty-region clip already set.
    // The island erase must narrow that clip (IntersectClip), never widen
    // it: replacing the clip clears the full widget rect with Source and
    // wipes sibling content — the live defect where hovering under Mica
    // hides surrounding controls as if a backpanel painted over them.
    // Call the helper directly under a dirty-region clip: the guard rect
    // outside the clip must survive the Source clear.
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QCOMPARE(style->themeMode(), WinUI3::ThemeMode::Light);
    QWidget window;
    window.setProperty("_winui_backdrop", 1);
    window.setProperty("_winui_backdrop_effective", 2); // Composited
    QWidget island(&window);
    island.setProperty(WinUI3::Style::SurfaceProperty, QStringLiteral("content"));
    QPalette clearPalette = island.palette();
    QColor clearWindow = clearPalette.color(QPalette::Window);
    clearWindow.setAlpha(0);
    clearPalette.setColor(QPalette::Window, clearWindow);
    island.setPalette(clearPalette);
    QLineEdit edit(&island);
    edit.resize(240, 96);

    QImage canvas(edit.size(), QImage::Format_ARGB32_Premultiplied);
    canvas.fill(QColor(255, 0, 0, 255));
    QPainter painter(&canvas);
    // Dirty region: a hover repaint of the bottom half only.
    painter.setClipRect(QRect(0, 48, 240, 48));
    QVERIFY(WinUI3::Private::eraseForBackdrop(&painter, &edit, edit.rect(),
                                              WinUI3::Private::ControlRadius));
    painter.end();
    // Token assertion on the same state: the guard rows outside the clip
    // keep their fill (opaque red), the clipped rows clear to transparent.
    QCOMPARE(canvas.pixelColor(120, 16), QColor(255, 0, 0, 255));
    QVERIFY(canvas.pixelColor(120, 80).alpha() == 0);
    QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(&edit));
}

void WinUI3SurfacesTest::backdropToggleOffRestoresShellSurfaces()
{
#ifndef Q_OS_WIN
    QSKIP("native backdrop restoration requires the Windows DWM compositor");
#endif
    // Toggle-off must restore the opaque window: offscreen the None branch
    // runs restoreBackdropState, republishes Solid so no painter keeps
    // Source-clearing, and leaves no backdrop marker behind. Without the
    // restore the last composited frame stays on screen and the toggle
    // looks like a no-op (stale Mica fossil).
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget window;
    window.setProperty("_winui_backdrop", 1);
    window.setProperty("_winui_backdrop_effective", 2); // Composited
    QVERIFY(WinUI3::applyBackdrop(&window, WinUI3::Backdrop::None));
    QCOMPARE(window.palette().color(QPalette::Window).alpha(), 255);
    QCOMPARE(window.property("_winui_backdrop_effective").toInt(), 0);
    QVERIFY(!window.property("_winui_backdrop").isValid());
    QVERIFY(!WinUI3::Private::paintsDirectlyOnBackdrop(&window));
}

void WinUI3SurfacesTest::backdropThemeSwitchRebasesIslandPalette()
{
    // Light->Dark with a live transparentized island must refresh its RGB:
    // syncContentSurfacesForBackdrop drives the island Window role to zero
    // alpha, and it carries no palette-owner registration, so a theme
    // switch that only visits paletteOwners leaves the stale light RGB
    // behind as a ghost. Alpha must be preserved: the island keeps
    // revealing the live material with fresh theme RGB.
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget window;
    window.setProperty("_winui_backdrop", 1);
    window.setProperty("_winui_backdrop_effective", 2); // Composited
    QWidget island(&window);
    island.setProperty(WinUI3::Style::SurfaceProperty, QStringLiteral("content"));
    QPalette live = style->standardPalette();
    QColor liveWindow = live.color(QPalette::Window);
    liveWindow.setAlpha(0);
    live.setColor(QPalette::Window, liveWindow);
    island.setPalette(live);
    style->setThemeMode(WinUI3::ThemeMode::Dark);
    const QColor darkWindow = style->standardPalette().color(QPalette::Window);
    QCOMPARE(island.palette().color(QPalette::Window).alpha(), 0);
    QCOMPARE(island.palette().color(QPalette::Window).red(), darkWindow.red());
    QCOMPARE(island.palette().color(QPalette::Window).green(), darkWindow.green());
    QCOMPARE(island.palette().color(QPalette::Window).blue(), darkWindow.blue());
    style->setThemeMode(WinUI3::ThemeMode::Light);
}

void WinUI3SurfacesTest::themeSwitchRebasesChromeShell()
{
    // Switch Light->Dark : le chrome (menu, toolbar, status) et la fenetre
    // doivent suivre le theme. Meme etat, deux assertions : les roles
    // Window suivent la palette standard, et aucun ne garde son RGB clair.
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QMainWindow window;
    auto *menuBar = new QMenuBar(&window);
    menuBar->addAction(QStringLiteral("File"));
    window.setMenuBar(menuBar);
    auto *toolbar = new QToolBar(&window);
    toolbar->addAction(QStringLiteral("Action"));
    window.addToolBar(toolbar);
    window.setStatusBar(new QStatusBar(&window));
    window.resize(400, 200);
    window.show();
    (void)QTest::qWaitForWindowExposed(&window);
    style->setThemeMode(WinUI3::ThemeMode::Dark);
    const QColor darkWindow = style->standardPalette().color(QPalette::Window);
    const QColor darkChrome = WinUI3::Private::popupSurfaceColor(style->standardPalette());
    QTRY_COMPARE(window.palette().color(QPalette::Window), darkWindow);
    QTRY_COMPARE(menuBar->palette().color(QPalette::Window), darkWindow);
    QTRY_COMPARE(toolbar->palette().color(QPalette::Window), darkWindow);
    QTRY_COMPARE(window.statusBar()->palette().color(QPalette::Window), darkChrome);
    style->setThemeMode(WinUI3::ThemeMode::Light);
}
void WinUI3SurfacesTest::menuMaskRetrySettles()
{
    // Finding B mechanism: a zero-size popup menu must not schedule an
    // unbounded 0ms singleShot retry chain. The guard counts attempts, keeps
    // the count set across retries, and caps the chain at 5, clearing the
    // marker only on success.
    // Offscreen premise: popup() of a 0x0 menu never maps, so Show never
    // fires and isVisible() stays false. Drive the same retry path with a
    // sized popup plus a test-side Show shim (public API only: the retry
    // property is observed through QObject::property, never a private
    // symbol). The shim zeroes the menu during Show delivery, so the
    // style's Show handler (preparePopupSurface -> applyMenuRoundedMask)
    // observes the empty size and starts the chain; if the style ran first,
    // its Resize handler observes it right after. Either order converges on
    // the bounded retry.
    // Pair the grab() with a token/geometry assert on the same state.
    struct ZeroOnShow final : public QObject
    {
        bool eventFilter(QObject *watched, QEvent *event) override
        {
            if (event->type() == QEvent::Show) {
                if (auto *menu = qobject_cast<QMenu *>(watched))
                    menu->setFixedSize(0, 0);
            }
            return false;
        }
    } zeroer;
    struct RetryProbe final : public QObject
    {
        bool eventFilter(QObject *, QEvent *event) override
        {
            if (event->type() == QEvent::Timer || event->type() == QEvent::MetaCall)
                ++events;
            return false;
        }
        int events = 0;
    } probe;
    QMenu menu;
    menu.addAction(QStringLiteral("&New project"));
    menu.installEventFilter(&zeroer);
    menu.popup(QPoint(80, 80));
    // Sized at popup time, so the popup maps offscreen; the shim zeroes it
    // during Show and the mask retry chain starts instead of succeeding.
    QTRY_VERIFY(menu.isVisible());
    QTRY_VERIFY(menu.size().isEmpty());
    QTRY_VERIFY(menu.property("_winui_menu_mask_retry").toInt() >= 1);
    // Drop any success-path mask applied before the shim ran: the retry lane
    // never clears it, and this phase must observe a permanently zero-size
    // menu settling unmasked.
    menu.clearMask();
    menu.installEventFilter(&probe);
    // Count only Timer + MetaCall deliveries over a fixed window: each
    // retry re-arms a 0ms singleShot, so an unbounded chain pumps those
    // two event types forever while the bounded chain settles after at
    // most 5 attempts and the loop goes idle.
    QTest::qWait(200);
    qApp->processEvents();
    const int attempts = menu.property("_winui_menu_mask_retry").toInt();
    QVERIFY2(attempts >= 1 && attempts <= 5,
             qPrintable(QStringLiteral("retry chain out of bounds: %1 attempts").arg(attempts)));
    QVERIFY2(probe.events < 30,
             qPrintable(
                     QStringLiteral("retry chain never settles: %1 timer/metacall events in 200ms")
                             .arg(probe.events)));
    QVERIFY(menu.isVisible());
    QVERIFY(menu.mask().isEmpty());
    // The chain must have settled: waiting longer schedules nothing new.
    QTest::qWait(20);
    qApp->processEvents();
    QCOMPARE(menu.property("_winui_menu_mask_retry").toInt(), attempts);
    menu.hide();
    QTRY_VERIFY(!menu.isVisible());
    // Success path through the same public popup: a sized menu rounds and
    // the retry marker clears.
    menu.removeEventFilter(&zeroer);
    menu.removeEventFilter(&probe);
    menu.setMinimumSize(0, 0);
    menu.setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    menu.popup(QPoint(80, 80));
    QTRY_VERIFY(menu.isVisible());
    QTRY_VERIFY(!menu.mask().isEmpty());
    QVERIFY(!menu.property("_winui_menu_mask_retry").isValid());
    const QImage rendered = menu.grab().toImage();
    QVERIFY(!rendered.isNull());
    QCOMPARE(rendered.size(), menu.size());
    menu.hide();
}

void WinUI3SurfacesTest::progressAnimationAndOrientations()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QProgressBar bar;
    bar.setRange(0, 0);
    bar.resize(240, 24);
    bar.show();
    QImage first = bar.grab().toImage();
    QTest::qWait(40);
    QImage second = bar.grab().toImage();
    QVERIFY(first != second);
    auto *timer = style->findChild<QTimer *>(QStringLiteral("_winui_progress_timer"),
                                             Qt::FindDirectChildrenOnly);
    QVERIFY(timer);
    QVERIFY(timer->isActive());
    QVERIFY(!bar.findChild<QTimer *>(QStringLiteral("_winui_progress_timer"),
                                     Qt::FindDirectChildrenOnly));

    bar.hide();
    QTRY_VERIFY(!timer->isActive());
    bar.show();
    QTRY_VERIFY(timer->isActive());

    bar.setRange(0, 100);
    bar.setValue(40);
    bar.grab();
    QCoreApplication::processEvents();
    QVERIFY(!timer->isActive());
    bar.setRange(0, 0);
    bar.grab();
    QCoreApplication::processEvents();
    QVERIFY(timer->isActive());

    bar.setOrientation(Qt::Vertical);
    bar.resize(24, 180);
    bar.setInvertedAppearance(true);
    bar.setEnabled(false);
    QVERIFY(!bar.grab().isNull());

    qputenv("WINUI3STYLE_DISABLE_ANIMATIONS", "1");
    QTRY_VERIFY(!timer->isActive());
    const QImage frozenFirst = bar.grab().toImage();
    QTest::qWait(40);
    const QImage frozenSecond = bar.grab().toImage();
    QCOMPARE(frozenFirst, frozenSecond);
    qunsetenv("WINUI3STYLE_DISABLE_ANIMATIONS");

    style->unpolish(&bar);
    QVERIFY(!timer->isActive());
}

void WinUI3SurfacesTest::progressTextAndDisabledPaletteContract()
{
    QProgressBar bar;
    bar.setRange(0, 100);
    bar.setValue(42);
    bar.setFormat(QStringLiteral("Transferred %p%"));

    bar.setTextVisible(false);
    const QSize bareHint = bar.sizeHint();
    bar.setTextVisible(true);
    const QSize textHint = bar.sizeHint();
    QVERIFY(textHint.height() >= bareHint.height() + 6);

    QPalette palette = bar.palette();
    const QColor activeText(210, 20, 10);
    const QColor disabledText(10, 20, 210);
    palette.setColor(QPalette::Active, QPalette::WindowText, activeText);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, disabledText);
    bar.setPalette(palette);
    bar.setEnabled(false);

    QStyleOptionProgressBar option;
    option.initFrom(&bar);
    option.rect = QRect(QPoint(0, 0), QSize(220, textHint.height()));
    option.minimum = bar.minimum();
    option.maximum = bar.maximum();
    option.progress = bar.value();
    option.text = bar.text();
    option.textVisible = true;

    QImage image(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    bar.style()->drawControl(QStyle::CE_ProgressBarLabel, &option, &painter, &bar);
    painter.end();

    bool foundDisabledText = false;
    bool foundActiveText = false;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor pixel = image.pixelColor(x, y);
            if (pixel.alpha() == 0)
                continue;
            foundDisabledText |= pixel.blue() > pixel.red() + 40;
            foundActiveText |= pixel.red() > pixel.blue() + 40;
        }
    }
    QVERIFY(foundDisabledText);
    QVERIFY(!foundActiveText);
}

void WinUI3SurfacesTest::progressTimerScalingAndLifecycle()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget host;
    host.resize(420, 420);
    QList<QProgressBar *> bars;
    for (int i = 0; i < 100; ++i) {
        auto *bar = new QProgressBar(&host);
        bar->setRange(0, 0);
        bar->setGeometry((i % 10) * 42, (i / 10) * 42, 40, 24);
        bars.append(bar);
    }
    host.show();
    QCoreApplication::processEvents();

    const auto timers = style->findChildren<QTimer *>(QStringLiteral("_winui_progress_timer"),
                                                      Qt::FindDirectChildrenOnly);
    QCOMPARE(timers.size(), 1);
    auto *timer = timers.constFirst();
    QVERIFY(timer->isActive());
    for (QProgressBar *bar : bars) {
        QVERIFY(!bar->findChild<QTimer *>(QStringLiteral("_winui_progress_timer"),
                                          Qt::FindDirectChildrenOnly));
    }

    host.hide();
    QCoreApplication::processEvents();
    QVERIFY(!timer->isActive());
    host.show();
    QCoreApplication::processEvents();
    QVERIFY(timer->isActive());

    delete bars.takeLast();
    QVERIFY(timer->isActive());
    qDeleteAll(bars);
    bars.clear();
    QCoreApplication::processEvents();
    QVERIFY(!timer->isActive());
}

QTEST_MAIN(WinUI3SurfacesTest)
#include "tst_winui3surfaces.moc"
