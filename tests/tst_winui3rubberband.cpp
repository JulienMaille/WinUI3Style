// SPDX-License-Identifier: LGPL-2.1-or-later
// Rubber-band marquee contract. WinUI 3 has no rubber band element, so this is
// a consistency extension: the marquee borrows the style's selection accent
// (QPalette::Highlight, the role the item-view selection marker uses) and the
// accent fill at 20% so the rows it sweeps stay readable. No pinned WinUI XAML
// backs the value; the row stays source-audited until a live pass accepts it.
#include <winui3style/winui3style.h>

#include "../src/lib/winui3tokens_p.h"
#include "winui3testhelpers.h"

#include <QImage>
#include <QPainter>
#include <QRubberBand>
#include <QStyleOptionRubberBand>
#include <QtTest>

namespace {

bool closeEnough(const QColor &a, const QColor &b, int tolerance = 2)
{
    return qAbs(a.red() - b.red()) <= tolerance && qAbs(a.green() - b.green()) <= tolerance
            && qAbs(a.blue() - b.blue()) <= tolerance;
}

} // namespace

class WinUI3RubberBandTest final : public QObject, private WinUI3StandardTestFixture
{
    Q_OBJECT

private slots:
    void initTestCase();
    void rubberBandUsesSelectionAccent();
};

void WinUI3RubberBandTest::initTestCase()
{
    initializeStandardWinUI3TestStyle();
}

void WinUI3RubberBandTest::rubberBandUsesSelectionAccent()
{
    QRubberBand band(QRubberBand::Rectangle);
    band.resize(120, 80);
    band.show();
    QVERIFY(QTest::qWaitForWindowExposed(&band));
    QVERIFY(band.isVisible());

    QStyleOptionRubberBand option;
    option.initFrom(&band);
    option.shape = QRubberBand::Rectangle;
    option.opaque = false;
    option.rect = QRect(0, 0, 120, 80);
    QImage frame(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
    frame.fill(Qt::transparent);
    {
        QPainter painter(&frame);
        band.style()->drawControl(QStyle::CE_RubberBand, &option, &painter, &band);
    }

    const WinUI3::Private::Tokens t = WinUI3::Private::tokens(band.palette());
    QVERIFY(t.selectionAccent.isValid());
    // 1 px stroke on the outer edge, the selection accent at full strength.
    QCOMPARE(frame.pixelColor(0, 0), t.selectionAccent);
    QCOMPARE(frame.pixelColor(119, 0), t.selectionAccent);
    QCOMPARE(frame.pixelColor(0, 79), t.selectionAccent);
    QCOMPARE(frame.pixelColor(119, 79), t.selectionAccent);
    // Body: the same accent at 20%, never opaque, so swept rows stay legible.
    const QColor body = frame.pixelColor(60, 40);
    QCOMPARE(body.alpha(), 51);
    QVERIFY(closeEnough(body, t.selectionAccent));
    // Qt asks for an opaque band on drag previews; honour it without changing
    // the stroke or the geometry.
    option.opaque = true;
    QImage solid(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
    solid.fill(Qt::transparent);
    {
        QPainter painter(&solid);
        band.style()->drawControl(QStyle::CE_RubberBand, &option, &painter, &band);
    }
    QCOMPARE(solid.pixelColor(60, 40).alpha(), 255);
    QVERIFY(closeEnough(solid.pixelColor(60, 40), t.selectionAccent));
    QCOMPARE(solid.pixelColor(0, 0), t.selectionAccent);
}

QTEST_MAIN(WinUI3RubberBandTest)
#include "tst_winui3rubberband.moc"
