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

class WinUI3ScrollBarCardTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void scrollBarInsideOpaqueCardKeepsCardFill();
};

void WinUI3ScrollBarCardTest::initTestCase()
{
    qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light));
}

void WinUI3ScrollBarCardTest::init()
{
    if (auto *style = qobject_cast<WinUI3::Style *>(qApp->style())) {
        style->setThemeMode(WinUI3::ThemeMode::Light);
        style->setAccentColor({});
    }
}

void WinUI3ScrollBarCardTest::cleanup()
{
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
    // The card tone comes from the token home (178-alpha veil of t.layer on
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

QTEST_MAIN(WinUI3ScrollBarCardTest)
#include "tst_winui3scrollbarcard.moc"
