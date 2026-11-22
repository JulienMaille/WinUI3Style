// SPDX-License-Identifier: LGPL-2.1-or-later
// ScrollBar contracts split from tst_winui3views.cpp to respect the 60 KB cap.
#include <winui3style/winui3style.h>

#include "../src/lib/winui3frameproperties_p.h"
#include "../src/lib/winui3surfaces_p.h"

#include <QApplication>
#include <QColor>
#include <QCoreApplication>
#include <QEvent>
#include <QImage>
#include <QMouseEvent>
#include <QPalette>
#include <QRect>
#include <QScrollArea>
#include <QScrollBar>
#include <QStyle>
#include <QStyleOptionSlider>
#include <QTest>
#include <QWidget>
#include <QtMath>

#include "winui3testhelpers.h"

class WinUI3ScrollBarTest final : public QObject, private WinUI3StandardTestFixture
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void scrollBarContract();
    void scrollBarHorizontalAndReentry();
    void scrollAreaScrollBarIntegration();
    void scrollBarBackdropChainContract();
};

void WinUI3ScrollBarTest::initTestCase()
{
    initializeStandardWinUI3TestStyle();
}

void WinUI3ScrollBarTest::init()
{
    resetStandardWinUI3TestStyle();
}

void WinUI3ScrollBarTest::cleanup()
{
    cleanupStandardWinUI3TestWidgets();
}

void WinUI3ScrollBarTest::scrollBarContract()
{
    DisableAnimationsGuard animations;
    QScrollBar bar(Qt::Vertical);
    bar.setRange(0, 100);
    bar.setPageStep(20);
    bar.setValue(30);
    bar.resize(12, 300);
    bar.show();

    QStyleOptionSlider option;
    option.initFrom(&bar);
    option.orientation = bar.orientation();
    option.minimum = bar.minimum();
    option.maximum = bar.maximum();
    option.sliderPosition = bar.sliderPosition();
    option.sliderValue = bar.value();
    option.pageStep = bar.pageStep();
    option.upsideDown = false;
    const QRect decrease = bar.style()->subControlRect(QStyle::CC_ScrollBar, &option,
                                                       QStyle::SC_ScrollBarSubLine, &bar);
    const QRect increase = bar.style()->subControlRect(QStyle::CC_ScrollBar, &option,
                                                       QStyle::SC_ScrollBarAddLine, &bar);
    const QRect thumb = bar.style()->subControlRect(QStyle::CC_ScrollBar, &option,
                                                    QStyle::SC_ScrollBarSlider, &bar);
    QCOMPARE(decrease.height(), 12);
    QCOMPARE(increase.height(), 12);
    QCOMPARE(thumb.width(), 12);
    QVERIFY(thumb.height() >= 30);

    const auto colorDistance = [](const QColor &a, const QColor &b) {
        return qAbs(a.red() - b.red()) + qAbs(a.green() - b.green()) + qAbs(a.blue() - b.blue());
    };
    setFrame(&bar, "_winui_hover_progress", 0.0);
    QImage collapsedGeometry = bar.grab().toImage();
    const QColor background = bar.palette().color(QPalette::Window);
    const int sampleY = thumb.center().y();
    QVERIFY(colorDistance(collapsedGeometry.pixelColor(2, sampleY), background) < 5);
    // Pinned 6 px stroke: hairline at x=7..8.
    QVERIFY(colorDistance(collapsedGeometry.pixelColor(5, sampleY), background) < 5);
    QVERIFY(colorDistance(collapsedGeometry.pixelColor(7, sampleY), background) > 10);
    QVERIFY(colorDistance(collapsedGeometry.pixelColor(8, sampleY), background) > 10);
    QVERIFY(colorDistance(collapsedGeometry.pixelColor(11, sampleY), background) < 5);

    QEvent enter(QEvent::Enter);
    QCoreApplication::sendEvent(&bar, &enter);
    QCOMPARE(frameReal(&bar, "_winui_hover_progress"), 1.0);

    // Issue #109: solid caret, not hollow chevron.
    for (WinUI3::ThemeMode theme : { WinUI3::ThemeMode::Light, WinUI3::ThemeMode::Dark }) {
        qobject_cast<WinUI3::Style *>(QApplication::style())->setThemeMode(theme);
        QCOMPARE(frameReal(&bar, "_winui_hover_progress"), 1.0);
        const QImage expandedArrow = bar.grab().toImage();
        const QColor arrowBackground = bar.palette().color(QPalette::Window);
        int centreRun = 0;
        int solidCentreRun = 0;
        int arrowInkSpan = 0;
        for (int y = decrease.top(); y < decrease.bottom(); ++y) {
            const bool centreInk = colorDistance(expandedArrow.pixelColor(decrease.center().x(), y),
                                                 arrowBackground)
                    > 24;
            centreRun = centreInk ? centreRun + 1 : 0;
            solidCentreRun = qMax(solidCentreRun, centreRun);
            for (int x = decrease.left(); x < decrease.right(); ++x)
                if (colorDistance(expandedArrow.pixelColor(x, y), arrowBackground) > 24)
                    ++arrowInkSpan;
        }
        QVERIFY2(solidCentreRun >= 4, "the hover arrow is a hollow chevron, not a solid caret");
        QVERIFY2(arrowInkSpan <= 60, "the hover arrow is wider than the 8 px pinned arrow box");

        // Caret ink shares the thumb centre.
        int arrowMin = decrease.right();
        int arrowMax = decrease.left();
        for (int y = decrease.top(); y < decrease.bottom(); ++y)
            for (int x = decrease.left(); x < decrease.right(); ++x)
                if (colorDistance(expandedArrow.pixelColor(x, y), arrowBackground) > 24) {
                    arrowMin = qMin(arrowMin, x);
                    arrowMax = qMax(arrowMax, x);
                }
        int thumbMin = thumb.right();
        int thumbMax = thumb.left();
        for (int x = thumb.left(); x < thumb.right(); ++x)
            if (colorDistance(expandedArrow.pixelColor(x, thumb.center().y()), arrowBackground)
                > 24) {
                thumbMin = qMin(thumbMin, x);
                thumbMax = qMax(thumbMax, x);
            }
        const qreal arrowCentre = (arrowMin + arrowMax) / 2.0;
        const qreal thumbCentre = (thumbMin + thumbMax) / 2.0;
        QVERIFY2(qAbs(arrowCentre - thumbCentre) <= 0.5,
                 "the hover arrow ink is not centred on the thumb");
    }

    const int beforeArrow = bar.value();
    QTest::mouseClick(&bar, Qt::LeftButton, Qt::NoModifier, increase.center());
    QVERIFY(bar.value() > beforeArrow);

    option.sliderPosition = bar.sliderPosition();
    option.sliderValue = bar.value();
    const QRect movedThumb = bar.style()->subControlRect(QStyle::CC_ScrollBar, &option,
                                                         QStyle::SC_ScrollBarSlider, &bar);
    const int beforeDrag = bar.value();
    QTest::mousePress(&bar, Qt::LeftButton, Qt::NoModifier, movedThumb.center());
    QMouseEvent move(QEvent::MouseMove, QPointF(movedThumb.center() + QPoint(0, 45)),
                     QPointF(movedThumb.center() + QPoint(0, 45)), Qt::NoButton, Qt::LeftButton,
                     Qt::NoModifier);
    QCoreApplication::sendEvent(&bar, &move);
    QTest::mouseRelease(&bar, Qt::LeftButton, Qt::NoModifier, movedThumb.center() + QPoint(0, 45));
    QVERIFY(bar.value() > beforeDrag);

    QEvent leave(QEvent::Leave);
    QCoreApplication::sendEvent(&bar, &leave);
    QCOMPARE(frameReal(&bar, "_winui_hover_progress"), 0.0);
    const QImage collapsed = bar.grab().toImage();
    QCOMPARE(collapsed.pixelColor(0, collapsed.height() / 2),
             bar.palette().color(QPalette::Window));
}

void WinUI3ScrollBarTest::scrollBarHorizontalAndReentry()
{
    DisableAnimationsGuard animations;
    QScrollBar bar(Qt::Horizontal);
    bar.setRange(0, 100);
    bar.setPageStep(20);
    bar.setValue(30);
    bar.resize(300, 12);
    bar.show();

    QStyleOptionSlider option;
    option.initFrom(&bar);
    option.orientation = bar.orientation();
    option.minimum = bar.minimum();
    option.maximum = bar.maximum();
    option.sliderPosition = bar.sliderPosition();
    option.sliderValue = bar.value();
    option.pageStep = bar.pageStep();
    option.upsideDown = false;
    const QRect decrease = bar.style()->subControlRect(QStyle::CC_ScrollBar, &option,
                                                       QStyle::SC_ScrollBarSubLine, &bar);
    const QRect increase = bar.style()->subControlRect(QStyle::CC_ScrollBar, &option,
                                                       QStyle::SC_ScrollBarAddLine, &bar);
    const QRect thumb = bar.style()->subControlRect(QStyle::CC_ScrollBar, &option,
                                                    QStyle::SC_ScrollBarSlider, &bar);
    QCOMPARE(decrease.width(), 12);
    QCOMPARE(increase.width(), 12);
    QCOMPARE(thumb.height(), 12);
    QVERIFY(thumb.width() >= 30);
    QCOMPARE(bar.style()->pixelMetric(QStyle::PM_ScrollBarSliderMin, &option, &bar), 30);

    const auto colorDistance = [](const QColor &a, const QColor &b) {
        return qAbs(a.red() - b.red()) + qAbs(a.green() - b.green()) + qAbs(a.blue() - b.blue());
    };
    setFrame(&bar, "_winui_hover_progress", 0.0);
    QImage collapsedGeometry = bar.grab().toImage();
    const QColor background = bar.palette().color(QPalette::Window);
    const int sampleX = thumb.center().x();
    QVERIFY(colorDistance(collapsedGeometry.pixelColor(sampleX, 2), background) < 5);
    // Hairline at y=7..8 (see vertical contract).
    QVERIFY(colorDistance(collapsedGeometry.pixelColor(sampleX, 5), background) < 5);
    QVERIFY(colorDistance(collapsedGeometry.pixelColor(sampleX, 7), background) > 10);
    QVERIFY(colorDistance(collapsedGeometry.pixelColor(sampleX, 8), background) > 10);
    QVERIFY(colorDistance(collapsedGeometry.pixelColor(sampleX, 11), background) < 5);

    QEvent enter(QEvent::Enter);
    QCoreApplication::sendEvent(&bar, &enter);
    QCOMPARE(frameReal(&bar, "_winui_hover_progress"), 1.0);

    const int beforeArrow = bar.value();
    QTest::mousePress(&bar, Qt::LeftButton, Qt::NoModifier, increase.center());
    const qreal pressed = frameReal(&bar, "_winui_press_progress");
    QVERIFY(pressed > 0.0);
    QTest::mouseRelease(&bar, Qt::LeftButton, Qt::NoModifier, increase.center());
    QVERIFY(bar.value() > beforeArrow);

    QEvent leave(QEvent::Leave);
    QCoreApplication::sendEvent(&bar, &leave);
    QCOMPARE(frameReal(&bar, "_winui_hover_progress"), 0.0);
    QCoreApplication::sendEvent(&bar, &enter);
    QCOMPARE(frameReal(&bar, "_winui_hover_progress"), 1.0);

    bar.setLayoutDirection(Qt::RightToLeft);
    option.direction = Qt::RightToLeft;
    option.upsideDown = true;
    option.sliderPosition = bar.sliderPosition();
    option.sliderValue = bar.value();
    const QRect rtlDecrease = bar.style()->subControlRect(QStyle::CC_ScrollBar, &option,
                                                          QStyle::SC_ScrollBarSubLine, &bar);
    const QRect rtlIncrease = bar.style()->subControlRect(QStyle::CC_ScrollBar, &option,
                                                          QStyle::SC_ScrollBarAddLine, &bar);
    QVERIFY(rtlDecrease.left() > rtlIncrease.left());

    option.sliderPosition = bar.sliderPosition();
    option.sliderValue = bar.value();
    const QRect currentThumb = bar.style()->subControlRect(QStyle::CC_ScrollBar, &option,
                                                           QStyle::SC_ScrollBarSlider, &bar);
    bar.setEnabled(false);
    const QImage disabled = bar.grab().toImage();
    QCOMPARE(disabled.pixelColor(currentThumb.center()), bar.palette().color(QPalette::Window));
}

void WinUI3ScrollBarTest::scrollAreaScrollBarIntegration()
{
    QScrollArea area;
    auto *content = new QWidget;
    content->resize(760, 620);
    area.setWidget(content);
    area.resize(260, 180);
    area.show();

    QScrollBar *vertical = area.verticalScrollBar();
    QScrollBar *horizontal = area.horizontalScrollBar();
    QTRY_VERIFY(vertical->isVisible());
    QTRY_VERIFY(horizontal->isVisible());
    QCOMPARE(vertical->width(), 12);
    QCOMPARE(horizontal->height(), 12);
    QVERIFY(vertical->maximum() > 0);
    QVERIFY(horizontal->maximum() > 0);

    vertical->setFocus(Qt::TabFocusReason);
    const int keyboardBefore = vertical->value();
    QTest::keyClick(vertical, Qt::Key_Down);
    QVERIFY(vertical->value() > keyboardBefore);

    const int verticalBefore = vertical->value();
    QTest::mouseClick(vertical, Qt::LeftButton, Qt::NoModifier,
                      QPoint(vertical->width() / 2, vertical->height() - 6));
    QVERIFY(vertical->value() > verticalBefore);
    const int horizontalBefore = horizontal->value();
    QTest::mouseClick(horizontal, Qt::LeftButton, Qt::NoModifier,
                      QPoint(horizontal->width() - 6, horizontal->height() / 2));
    QVERIFY(horizontal->value() > horizontalBefore);
}

void WinUI3ScrollBarTest::scrollBarBackdropChainContract()
{
    // Scrollbars inside a transparentized scrolled chain join the same
    // no-fill recipe as the area, viewport, and container: sync arms both
    // bars (remembered palettes, zero-alpha Window role, no autofill) and
    // restore returns every link. On an effective-Composited window the
    // theme refresh already re-runs the shell sync (refreshOwnedPalettes),
    // so bars arrive armed — zero-alpha Window, no fill, open gate — and the
    // explicit sync below is idempotent. Toggle-off restore returns every
    // link to its remembered opaque Window role.
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget window;
    window.setProperty("_winui_backdrop", 1);
    window.setProperty("_winui_backdrop_effective", 2); // Composited
    QWidget island(&window);
    island.setProperty(WinUI3::Style::SurfaceProperty, QStringLiteral("content"));
    auto *area = new QScrollArea(&island);
    auto *content = new QWidget;
    content->resize(760, 620);
    area->setWidget(content);
    area->resize(260, 180);
    window.resize(640, 420);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QScrollBar *vertical = area->verticalScrollBar();
    QScrollBar *horizontal = area->horizontalScrollBar();
    style->setThemeMode(WinUI3::ThemeMode::Dark);
    QVERIFY(vertical->testAttribute(Qt::WA_SetPalette));
    QVERIFY(horizontal->testAttribute(Qt::WA_SetPalette));
    // The theme refresh on an effective-Composited window already re-syncs
    // the transparentized chain (refreshOwnedPalettes re-runs the shell
    // sync), so the bars arrive armed — zero-alpha Window role, no autofill,
    // open gate — exactly like a themed gallery with Mica on.
    QCOMPARE(vertical->palette().color(QPalette::Window).alpha(), 0);
    QCOMPARE(horizontal->palette().color(QPalette::Window).alpha(), 0);
    QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(vertical));
    QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(horizontal));

    WinUI3::Private::syncContentSurfacesForBackdrop(&window);
    QCOMPARE(area->palette().color(QPalette::Window).alpha(), 0);
    QCOMPARE(area->viewport()->palette().color(QPalette::Window).alpha(), 0);
    QCOMPARE(vertical->palette().color(QPalette::Window).alpha(), 0);
    QCOMPARE(horizontal->palette().color(QPalette::Window).alpha(), 0);
    QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(vertical));
    QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(horizontal));

    WinUI3::Private::restoreContentSurfacesForBackdrop(&window);
    QCOMPARE(vertical->palette().color(QPalette::Window).alpha(), 255);
    QCOMPARE(horizontal->palette().color(QPalette::Window).alpha(), 255);
    QVERIFY(!vertical->testAttribute(Qt::WA_StyledBackground));
    QVERIFY(!horizontal->testAttribute(Qt::WA_StyledBackground));
    style->setThemeMode(WinUI3::ThemeMode::Light);
}

QTEST_MAIN(WinUI3ScrollBarTest)
#include "tst_winui3scrollbar.moc"
