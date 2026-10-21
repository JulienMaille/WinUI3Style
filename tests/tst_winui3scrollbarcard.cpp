// SPDX-License-Identifier: LGPL-2.1-or-later
// ScrollBar rest groove inside an opaque group card on a live Mica window
// (Gallery Controls card chain: standalone QScrollBars under an opaque
// QGroupBox with no winuiSurface). The WinUI groove at rest is transparent
// to its parent surface (the card fill), not to the window material, so the
// card guard must rebuild the veiled card tone instead of leaving the
// backdrop erase as a transparent hole through the card. Mechanism twin of
// backdropSliderInsideOpaqueCardKeepsCardFill; split out of
// tst_winui3views.cpp per the 60 KB test-size ratchet.
#include <winui3style/winui3style.h>

#include "winui3testhelpers.h"

#include "../src/winui3tokens_p.h"

#include <QGroupBox>
#include <QImage>
#include <QPainter>
#include <QScrollBar>
#include <QStyleOptionSlider>
#include <QtTest>

class WinUI3ScrollBarCardTest final : public QObject, private WinUI3StandardTestFixture
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void scrollBarInsideOpaqueCardKeepsCardFill();
    void disabledBarGrooveKeepsRoleAlpha();
};

void WinUI3ScrollBarCardTest::initTestCase()
{
    initializeStandardWinUI3TestStyle();
}

void WinUI3ScrollBarCardTest::init()
{
    resetStandardWinUI3TestStyle();
}

void WinUI3ScrollBarCardTest::cleanup()
{
    cleanupStandardWinUI3TestWidgets();
}

void WinUI3ScrollBarCardTest::scrollBarInsideOpaqueCardKeepsCardFill()
{
    // A standalone QScrollBar inside an opaque QGroupBox (no SurfaceProperty)
    // on a Composited-effective Mica window. The shared gate still reports
    // paintsDirectlyOnBackdrop (the card carries no SurfaceProperty, so the
    // walk cannot see it), so only the card guard keeps the rest groove on
    // the card fill instead of erasing a hole through the veil. Same state,
    // token plus geometry asserts; never grab().
    DisableAnimationsGuard animations;
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget window;
    window.setProperty("_winui_backdrop", 1);
    window.setProperty("_winui_backdrop_effective", 2); // Composited
    QGroupBox card(QStringLiteral("Values"), &window);
    card.resize(220, 160);
    QScrollBar bar(Qt::Vertical, &card);
    bar.setRange(0, 100);
    bar.setPageStep(24);
    bar.setValue(35);
    bar.setGeometry(180, 24, 12, 112);
    window.resize(320, 240);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QCoreApplication::processEvents();
    // Gate state: the card carries no SurfaceProperty, so the shared gate
    // still passes for the bar (only the per-control card guard sees it).
    QVERIFY(!card.property(WinUI3::Style::SurfaceProperty).isValid());
    QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(&bar));
    // Geometry: a real thumb exists (rest groove plus visible thumb).
    setFrame(&bar, "_winui_hover_progress", 0.0);
    QStyleOptionSlider option;
    option.initFrom(&bar);
    option.orientation = bar.orientation();
    option.minimum = bar.minimum();
    option.maximum = bar.maximum();
    option.sliderPosition = bar.sliderPosition();
    option.sliderValue = bar.value();
    option.pageStep = bar.pageStep();
    option.rect = bar.rect();
    const QRect thumb =
            style->subControlRect(QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarSlider, &bar);
    QCOMPARE(thumb.width(), 12);
    QVERIFY(thumb.height() >= 30);
    QImage frame(bar.size(), QImage::Format_ARGB32_Premultiplied);
    frame.fill(card.palette().color(QPalette::Window));
    QPainter painter(&frame);
    style->drawComplexControl(QStyle::CC_ScrollBar, &option, &painter, &bar);
    painter.end();
    // Token assert: no pixel punched to transparent inside the bar rect.
    // The groove keeps the role's own alpha (min-clamp veil of t.layer on
    // a live material), so the groove keeps the parent surface, not Mica.
    const QColor cardTone =
            WinUI3::Private::veiledCard(WinUI3::Private::tokens(card.palette()).layer, true);
    for (int y = 0; y < frame.height(); y += 4) {
        for (int x = 0; x < frame.width(); x += 4) {
            QVERIFY2(frame.pixelColor(x, y).alpha() != 0,
                     qPrintable(QStringLiteral("hole at %1,%2").arg(x).arg(y)));
        }
    }
    // Token pairing: groove pixels (outside the thumb) keep the card tone.
    const QColor groovePixel = frame.pixelColor(2, 4);
    // Geometry assert: the thumb still paints (pixels differ from card tone).
    QImage plain(frame.size(), QImage::Format_ARGB32_Premultiplied);
    plain.fill(card.palette().color(QPalette::Window));
    QVERIFY(frame != plain);
    QCOMPARE(groovePixel, cardTone);
}

void WinUI3ScrollBarCardTest::disabledBarGrooveKeepsRoleAlpha()
{
    // Disabled QScrollBar inside an opaque QGroupBox card, Composited
    // backdrop, dark/light x Standard/Compact: the Disabled Base role is
    // faint by design, so the mica groove must keep the role's own alpha
    // (min-clamp veil), never force exact-178 — forcing it turned the
    // disabled groove into an opaque white band in dark Mica (reviewer
    // screenshot). ScrollBar metrics are density-invariant (extent 12,
    // slider minimum 30 in both profiles), so each density keeps the same
    // groove alpha-exact + thumb-suppressed + bar-rect asserts. Same token
    // plus geometry asserts; never grab().
    DisableAnimationsGuard animations;
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    for (const bool dark : { false, true }) {
        style->setThemeMode(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
        for (const WinUI3::DensityMode density :
             { WinUI3::DensityMode::Standard, WinUI3::DensityMode::Compact }) {
            style->setDensityMode(density);
            QWidget window;
            window.setProperty("_winui_backdrop", 1);
            window.setProperty("_winui_backdrop_effective", 2); // Composited
            QGroupBox card(QStringLiteral("Values"), &window);
            card.resize(220, 160);
            QScrollBar bar(Qt::Horizontal, &card);
            bar.setRange(0, 100);
            bar.setPageStep(24);
            bar.setValue(35);
            bar.setEnabled(false);
            bar.setGeometry(24, 24, 172, 12);
            window.resize(320, 240);
            window.show();
            QVERIFY(QTest::qWaitForWindowExposed(&window));
            QCoreApplication::processEvents();
            QCOMPARE(WinUI3::Style::densityMode(&bar), density);
            QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(&bar));
            const QColor expected =
                    WinUI3::Private::veiledCard(WinUI3::Private::tokens(bar.palette()).layer, true);
            QImage frame(bar.size(), QImage::Format_ARGB32_Premultiplied);
            frame.fill(Qt::red);
            QStyleOptionSlider option;
            option.initFrom(&bar);
            option.orientation = bar.orientation();
            option.minimum = bar.minimum();
            option.maximum = bar.maximum();
            option.sliderPosition = bar.sliderPosition();
            option.sliderValue = bar.value();
            option.pageStep = bar.pageStep();
            option.rect = bar.rect();
            option.state &= ~QStyle::State_Enabled;
            QPainter painter(&frame);
            style->drawComplexControl(QStyle::CC_ScrollBar, &option, &painter, &bar);
            painter.end();
            // Alpha-exact: the groove keeps the role alpha, never exact-178
            // unless the role itself carries it (RED pre-fix: white@178 slab).
            const QColor groove = frame.pixelColor(frame.width() / 2, frame.height() / 2);
            QCOMPARE(groove.alpha(), expected.alpha());
            QVERIFY(colorDistance(groove, expected) <= 6);
            // Geometry pairing: the bar rect is fully covered (no hole), thumb
            // suppressed by design (disabled-zero-opacity).
            QVERIFY(frame.pixelColor(2, frame.height() / 2).alpha() != 0);
            // Bar-rect pairing: the thickness is density-invariant (extent 12
            // in both profiles) and the thumb slot still resolves inside it.
            QCOMPARE(bar.height(), 12);
            const QRect grooveRect = style->subControlRect(QStyle::CC_ScrollBar, &option,
                                                           QStyle::SC_ScrollBarGroove, &bar);
            const QRect thumb = style->subControlRect(QStyle::CC_ScrollBar, &option,
                                                      QStyle::SC_ScrollBarSlider, &bar);
            QVERIFY(!grooveRect.isEmpty());
            QVERIFY(!thumb.isEmpty());
            QVERIFY(grooveRect.contains(thumb));
            QCOMPARE(thumb.height(), bar.height());
            // Thumb-suppressed pairing: with the thumb at zero opacity every
            // groove sample keeps the card tone (an enabled twin paints thumb
            // ink here — see scrollBarInsideOpaqueCardKeepsCardFill).
            for (int x = grooveRect.left(); x <= grooveRect.right(); x += 8) {
                QVERIFY2(colorDistance(frame.pixelColor(x, frame.height() / 2), expected) <= 6,
                         qPrintable(QStringLiteral("thumb ink at %1").arg(x)));
            }
        }
    }
    style->setDensityMode(WinUI3::DensityMode::Standard);
    style->setThemeMode(WinUI3::ThemeMode::Light);
}

QTEST_MAIN(WinUI3ScrollBarCardTest)
#include "tst_winui3scrollbarcard.moc"
