// SPDX-License-Identifier: LGPL-2.1-or-later
// QToolBox accordion header contract. WinUI 3 maps it to the Expander header:
// Expander_themeresources.xaml (microsoft-ui-xaml 9f89c2da, schema 2.4) gives
// ExpanderHeaderPadding 16,0,0,0, a 32 px chevron button with
// ExpanderChevronMargin 20,0,8,0 and a 12 px glyph (E70D collapsed, E70E
// expanded), and a header card whose fill/stroke do not change with state.
// Expander.xaml's ExpandDown state converts the header to top corners only.
// Two values are deliberately NOT asserted against the pin: Qt owns the header
// height through QToolBoxButton::sizeHint (fontMetrics + 8, measured 27 px
// against ExpanderMinHeight 48) and owns the page body surface (a plain
// QScrollArea, so ExpanderContentBackground/expander bottom corners are
// unreachable). Both shortfalls live in the ledger instead of being faked here.
#include <winui3style/winui3style.h>

#include "winui3testhelpers.h"
#include "../src/winui3tokens_p.h"

#include <QAbstractButton>
#include <QImage>
#include <QPainter>
#include <QToolBox>
#include <QtTest>

namespace {

// Standard alpha compositing, the same operation the painter performs when the
// card fill/stroke carry the palette's veil alpha over the toolbox surface.
QColor over(const QColor &top, const QColor &bottom)
{
    const qreal a = top.alphaF();
    return QColor(qRound(top.red() * a + bottom.red() * (1.0 - a)),
                  qRound(top.green() * a + bottom.green() * (1.0 - a)),
                  qRound(top.blue() * a + bottom.blue() * (1.0 - a)));
}

QImage renderHeader(const QStyle *style, const QPalette &palette, bool selected,
                    bool enabled = true)
{
    QWidget host;
    host.setPalette(palette);
    QStyleOptionToolBox option;
    option.initFrom(&host);
    option.palette = palette;
    option.rect = QRect(0, 0, 320, 27);
    option.text = QStringLiteral("Expander one");
    option.state |= QStyle::State_Enabled;
    if (!enabled)
        option.state &= ~QStyle::State_Enabled;
    if (selected)
        option.state |= QStyle::State_Selected;
    QImage image(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
    // The header sits on the toolbox surface; painting into a transparent
    // image would composite the veil alpha over nothing.
    image.fill(WinUI3::Private::tokens(palette).surface);
    QPainter painter(&image);
    style->drawControl(QStyle::CE_ToolBoxTab, &option, &painter, &host);
    painter.end();
    return image;
}

// Ink bounds of the trailing chevron region, ignoring the card surface.
QRect chevronInk(const QImage &image, const QColor &background)
{
    QRect ink;
    const int left = image.width() - 8 - 32;
    for (int y = 2; y < image.height() - 2; ++y)
        for (int x = left; x < image.width() - 2; ++x)
            if (colorDistance(image.pixelColor(x, y), background) > 40)
                ink |= QRect(x, y, 1, 1);
    return ink;
}

// Per-row ink width of the glyph, used to prove E70D (collapsed) and E70E
// (expanded) are the pinned vertical mirrors of one another.
QList<int> rowInk(const QImage &image, const QRect &ink, const QColor &background)
{
    QList<int> rows;
    for (int y = ink.top(); y <= ink.bottom(); ++y) {
        int count = 0;
        for (int x = ink.left(); x <= ink.right(); ++x)
            if (colorDistance(image.pixelColor(x, y), background) > 40)
                ++count;
        rows.append(count);
    }
    return rows;
}

} // namespace

class WinUI3ToolBoxTest final : public QObject, private WinUI3StandardTestFixture
{
    Q_OBJECT

private slots:
    void initTestCase();
    void expanderHeaderUsesPinnedGeometry_data();
    void expanderHeaderUsesPinnedGeometry();
    void headersShareOneCardAcrossPages();
};

void WinUI3ToolBoxTest::initTestCase()
{
    initializeStandardWinUI3TestStyle();
}

void WinUI3ToolBoxTest::expanderHeaderUsesPinnedGeometry_data()
{
    QTest::addColumn<int>("theme");
    QTest::addColumn<int>("density");
    QTest::newRow("light-standard")
            << int(WinUI3::ThemeMode::Light) << int(WinUI3::DensityMode::Standard);
    QTest::newRow("light-compact")
            << int(WinUI3::ThemeMode::Light) << int(WinUI3::DensityMode::Compact);
    QTest::newRow("dark-standard")
            << int(WinUI3::ThemeMode::Dark) << int(WinUI3::DensityMode::Standard);
    QTest::newRow("dark-compact") << int(WinUI3::ThemeMode::Dark)
                                  << int(WinUI3::DensityMode::Compact);
}

void WinUI3ToolBoxTest::expanderHeaderUsesPinnedGeometry()
{
    QFETCH(int, theme);
    QFETCH(int, density);
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setThemeMode(WinUI3::ThemeMode(theme));
    style->setDensityMode(WinUI3::DensityMode(density));

    const QPalette palette = style->standardPalette();
    const WinUI3::Private::Tokens t = WinUI3::Private::tokens(palette);
    const QColor card = over(t.layer, t.surface);
    const QColor stroke = over(t.stroke, card);
    const QImage collapsed = renderHeader(style, palette, false);
    const QImage selected = renderHeader(style, palette, true);

    // Card surface: layer composited over the toolbox surface, 1 px card
    // stroke, identical in both states (ExpanderHeaderBackground/BorderBrush
    // are the same theme resources for Normal and Checked). Compositing rounds
    // in premultiplied 8-bit space, so the comparison allows one step.
    const auto expectPixel = [](const QColor &actual, const QColor &expected, int tolerance,
                                const char *what) {
        QVERIFY2(colorDistance(actual, expected) <= tolerance,
                 qPrintable(QStringLiteral("%1: actual %2 expected %3")
                                    .arg(QString::fromLatin1(what), actual.name(QColor::HexArgb),
                                         expected.name(QColor::HexArgb))));
    };
    expectPixel(collapsed.pixelColor(250, 3), card, 3, "collapsed card");
    expectPixel(selected.pixelColor(250, 3), card, 3, "selected card");
    expectPixel(collapsed.pixelColor(160, 0), stroke, 3, "collapsed stroke");
    expectPixel(selected.pixelColor(160, 0), stroke, 3, "selected stroke");

    // ExpandDown keeps only the top corners on the header; a collapsed header
    // is a whole card. The 4 px ControlCornerRadius leaves (0,0) on the
    // toolbox surface in both states, while the bottom-left corner is card
    // only for the expanded header.
    expectPixel(collapsed.pixelColor(0, 0), t.surface, 3, "collapsed corner");
    expectPixel(selected.pixelColor(0, 0), t.surface, 3, "selected corner");
    expectPixel(collapsed.pixelColor(4, 4), card, 3, "collapsed body");
    expectPixel(selected.pixelColor(4, 4), card, 3, "selected body");
    expectPixel(selected.pixelColor(1, selected.height() - 3), card, 3, "expanded bottom corner");
    QVERIFY2(colorDistance(collapsed.pixelColor(1, collapsed.height() - 3),
                           selected.pixelColor(1, selected.height() - 3))
                     > 10,
             "a collapsed header must keep its bottom-left corner rounded");

    // Chevron: 12 px glyph centered in the 32 px button 8 px from the trailing
    // edge (ExpanderChevronMargin 20,0,8,0, ExpanderChevronGlyphSize 12).
    const QRect collapsedInk = chevronInk(collapsed, card);
    const QRect selectedInk = chevronInk(selected, card);
    QVERIFY(!collapsedInk.isEmpty());
    QVERIFY(!selectedInk.isEmpty());
    // Pinned box: 32 px wide, its right edge 8 px from the trailing edge.
    const int chevronBoxLeft = 320 - 8 - 32;
    const int chevronBoxRight = 320 - 8 - 1;
    const int chevronCenter = (chevronBoxLeft + chevronBoxRight) / 2;
    QVERIFY2(qAbs(collapsedInk.center().x() - chevronCenter) <= 2,
             qPrintable(QString::number(collapsedInk.center().x())));
    QVERIFY2(qAbs(selectedInk.center().x() - chevronCenter) <= 2,
             qPrintable(QString::number(selectedInk.center().x())));
    QVERIFY(collapsedInk.left() >= chevronBoxLeft && collapsedInk.right() <= chevronBoxRight);
    QVERIFY(selectedInk.left() >= chevronBoxLeft && selectedInk.right() <= chevronBoxRight);
    QVERIFY(collapsedInk.width() <= 12);
    QVERIFY(collapsedInk.height() <= 12);
    // Collapsed shows E70D, expanded shows E70E; the pinned glyphs are
    // vertical mirrors, so every row's ink width must match the opposite row.
    const QList<int> collapsedRows = rowInk(collapsed, collapsedInk, card);
    const QList<int> selectedRows = rowInk(selected, selectedInk, card);
    QCOMPARE(selectedRows.size(), collapsedRows.size());
    for (int row = 0; row < collapsedRows.size(); ++row)
        QVERIFY2(qAbs(collapsedRows.at(row) - selectedRows.at(collapsedRows.size() - 1 - row)) <= 1,
                 qPrintable(QStringLiteral("row %1: %2 vs %3")
                                    .arg(row)
                                    .arg(collapsedRows.at(row))
                                    .arg(selectedRows.at(collapsedRows.size() - 1 - row))));

    // Header text starts at the 16 px ExpanderHeaderPadding inset.
    QRect textInk;
    for (int y = 3; y < collapsed.height() - 3; ++y)
        for (int x = 4; x < chevronCenter - 40; ++x)
            if (colorDistance(collapsed.pixelColor(x, y), card) > 40)
                textInk |= QRect(x, y, 1, 1);
    QVERIFY(!textInk.isEmpty());
    QVERIFY2(textInk.left() >= 16 && textInk.left() <= 18,
             qPrintable(QString::number(textInk.left())));

    // Disabled keeps the header card.
    const QImage disabled = renderHeader(style, palette, false, false);
    expectPixel(disabled.pixelColor(250, 3), card, 3, "disabled card");
}

void WinUI3ToolBoxTest::headersShareOneCardAcrossPages()
{
    QToolBox box;
    box.addItem(new QWidget, QStringLiteral("Expander one"));
    box.addItem(new QWidget, QStringLiteral("Expander two"));
    box.addItem(new QWidget, QStringLiteral("Expander three"));
    box.resize(320, 300);
    box.setCurrentIndex(1);
    box.show();
    QVERIFY(QTest::qWaitForWindowExposed(&box));

    const auto buttons = box.findChildren<QAbstractButton *>();
    QCOMPARE(buttons.size(), 3);
    // Qt recolors each page button's Window role from the page widget that
    // follows it; the veiled card fill must still land on one surface so the
    // three headers read as the same card.
    QColor firstFill;
    QColor firstStroke;
    for (auto *button : buttons) {
        const QImage frame = button->grab().toImage();
        const QColor fill = frame.pixelColor(frame.width() - 70, 3);
        const QColor strokePixel = frame.pixelColor(frame.width() / 2, 0);
        if (!firstFill.isValid())
            firstFill = fill;
        if (!firstStroke.isValid())
            firstStroke = strokePixel;
        QCOMPARE(fill, firstFill);
        QCOMPARE(strokePixel, firstStroke);
    }
}

QTEST_MAIN(WinUI3ToolBoxTest)
#include "tst_winui3toolbox.moc"
