// SPDX-License-Identifier: LGPL-2.1-or-later
// DataGrid-like table header contracts (docs/coverage.md QTableView row).
// Split out of tst_winui3views.cpp at the 60 KB ratchet. The sort indicator
// is pinned to the WinUI DataGrid column header template: a trailing sort
// column with MinWidth="32", a 12 px SortIcon centred in it, and the
// SortIconAscending E74A / SortIconDescending E74B glyphs.
#include <winui3style/winui3style.h>

#include "winui3testhelpers.h"

#include <QHeaderView>
#include <QImage>
#include <QPainter>
#include <QStyleOptionHeader>
#include <QStyleOptionViewItem>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextLayout>
#include <QtMath>
#include <QtTest>

namespace {

QRect opaqueBounds(const QImage &image)
{
    QRect bounds;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (qAlpha(image.pixel(x, y)))
                bounds |= QRect(x, y, 1, 1);
        }
    }
    return bounds;
}

int opaqueCountInRows(const QImage &image, int firstRow, int lastRow)
{
    int count = 0;
    for (int y = firstRow; y <= lastRow; ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (qAlpha(image.pixel(x, y)))
                ++count;
        }
    }
    return count;
}

// Pixels that the sort glyph adds over the unsorted header: the header fill
// and separator are identical in both frames, so only the indicator ink
// survives the comparison.
QImage glyphInk(const QImage &sorted, const QImage &unsorted)
{
    QImage ink(sorted.size(), QImage::Format_ARGB32_Premultiplied);
    ink.fill(Qt::transparent);
    for (int y = 0; y < sorted.height(); ++y) {
        for (int x = 0; x < sorted.width(); ++x) {
            if (sorted.pixel(x, y) != unsorted.pixel(x, y))
                ink.setPixel(x, y, qRgba(0, 0, 0, 255));
        }
    }
    return ink;
}

} // namespace

class WinUI3TableHeaderTest final : public QObject, private WinUI3StandardTestFixture
{
    Q_OBJECT

private slots:
    void initTestCase();
    void tableHeaderContract();
    void tableCellElidesOverflow_data();
    void tableCellElidesOverflow();
    void tableCellPreservesFittingMultilineText();
    void tableCellElidesLastVisibleLine();
    void tableCellElidesAtHardBreak_data();
    void tableCellElidesAtHardBreak();
    void tableCellKeepsFittingText_data();
    void tableCellKeepsFittingText();
    void tableCellKeepsFittingLineBeforeOverflow();
    void tableCellHonorsElideNone();
    void tableCellPreservesWrappedPrefix_data();
    void tableCellPreservesWrappedPrefix();
    void tableHeaderLabelKeepsLeadingEdgeWithSortColumn();
    void tableResizeToContentsIncludesCellPadding();
    void tableResizeToContentsKeepsHeaderLabelVisible();
    void tableSortIndicatorGeometryContract();
    void tableHeaderPaletteFollowsThemeRoundTrip_data();
    void tableHeaderPaletteFollowsThemeRoundTrip();
    void tableHeaderBandContrastsAlternateRows_data();
    void tableHeaderBandContrastsAlternateRows();
};

void WinUI3TableHeaderTest::initTestCase()
{
    initializeStandardWinUI3TestStyle();
}

void WinUI3TableHeaderTest::tableCellElidesOverflow_data()
{
    QTest::addColumn<bool>("compact");
    QTest::addColumn<bool>("dark");
    QTest::addColumn<bool>("rtl");
    QTest::addColumn<bool>("wrap");
    QTest::addColumn<int>("elideMode");
    QTest::addColumn<QString>("text");
    for (bool compact : { false, true })
        for (bool dark : { false, true })
            for (bool rtl : { false, true })
                for (bool wrap : { false, true })
                    for (const auto mode : { Qt::ElideLeft, Qt::ElideMiddle, Qt::ElideRight })
                        for (const QString &text :
                             { QStringLiteral("src/plugins/help/help_plg.cpp"),
                               QStringLiteral("A long description that does not fit"),
                               QStringLiteral("CMakeUserPresetsWithAnExtraLongName.json"),
                               QStringLiteral(
                                       "Qt \u05e7\u05d5\u05d1\u05e5 \u05d1\u05d3\u05d9\u05e7\u05d4 "
                                       "src/plugins/help/help_plg.cpp"),
                               QStringLiteral(
                                       "Qt \u0645\u0644\u0641 \u0627\u062e\u062a\u0628\u0627\u0631 "
                                       "src/plugins/help/help_plg.cpp") }) {
                            const QByteArray name = QByteArray::number(compact) + '-'
                                    + QByteArray::number(dark) + '-' + QByteArray::number(rtl) + '-'
                                    + QByteArray::number(wrap) + '-' + QByteArray::number(mode)
                                    + '-' + text.toUtf8();
                            QTest::newRow(name.constData())
                                    << compact << dark << rtl << wrap << int(mode) << text;
                        }
}

void WinUI3TableHeaderTest::tableCellElidesOverflow()
{
    QFETCH(bool, compact);
    QFETCH(bool, dark);
    QFETCH(bool, rtl);
    QFETCH(bool, wrap);
    QFETCH(int, elideMode);
    QFETCH(QString, text);
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    style->setThemeMode(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
    QTableWidget table(1, 1);
    table.setProperty("winuiDensity", compact ? "compact" : "standard");
    table.setLayoutDirection(rtl ? Qt::RightToLeft : Qt::LeftToRight);
    table.ensurePolished();
    QCOMPARE(table.wordWrap(), true);
    QStyleOptionViewItem option;
    option.initFrom(table.viewport());
    option.font = table.font();
    option.font.setPixelSize(22);
    option.fontMetrics = QFontMetrics(option.font);
    option.rect = QRect(0, 0, 150, compact ? 28 : 36);
    option.index = table.model()->index(0, 0);
    option.state = QStyle::State_Enabled;
    option.features = QStyleOptionViewItem::HasDisplay;
    option.features.setFlag(QStyleOptionViewItem::WrapText, wrap);
    option.displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;
    option.textElideMode = Qt::TextElideMode(elideMode);
    option.text = text;
    const QRect textRect =
            style->subElementRect(QStyle::SE_ItemViewItemText, &option, table.viewport());
    QCOMPARE(textRect.left(), 12);
    QCOMPARE(textRect.right(), option.rect.right() - 12);
    QVERIFY(textRect.height() < 2 * option.fontMetrics.height());
    const QString elided =
            option.fontMetrics.elidedText(text, option.textElideMode, textRect.width());
    QVERIFY(elided != text);
    QVERIFY(elided.contains(QChar(0x2026)));
    const auto render = [&](const QStyleOptionViewItem &cell) {
        QImage image(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        style->drawControl(QStyle::CE_ItemViewItem, &cell, &painter, table.viewport());
        return image;
    };
    for (Qt::Alignment alignment : { Qt::Alignment(Qt::AlignLeft), Qt::Alignment(Qt::AlignHCenter),
                                     Qt::Alignment(Qt::AlignRight) }) {
        option.displayAlignment = alignment | Qt::AlignVCenter;
        QStyleOptionViewItem expected = option;
        expected.features.setFlag(QStyleOptionViewItem::WrapText, false);
        expected.text = elided;
        QCOMPARE(render(option), render(expected));
    }
}

void WinUI3TableHeaderTest::tableCellPreservesFittingMultilineText()
{
    QTableWidget table(1, 1);
    table.ensurePolished();
    QStyleOptionViewItem option;
    option.initFrom(table.viewport());
    option.font = table.font();
    option.fontMetrics = QFontMetrics(option.font);
    option.rect = QRect(0, 0, 240, 100);
    option.index = table.model()->index(0, 0);
    option.state = QStyle::State_Enabled;
    option.features = QStyleOptionViewItem::HasDisplay | QStyleOptionViewItem::WrapText;
    option.displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;
    option.text = QStringLiteral("First line\nSecond line\nThird line");
    const QRect textRect =
            table.style()->subElementRect(QStyle::SE_ItemViewItemText, &option, table.viewport());
    QCOMPARE(textRect.left(), 12);
    QVERIFY(textRect.height() >= 3 * option.fontMetrics.height());
    QImage actual(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
    actual.fill(Qt::transparent);
    {
        QPainter painter(&actual);
        table.style()->drawControl(QStyle::CE_ItemViewItem, &option, &painter, table.viewport());
    }
    QImage expected(actual.size(), actual.format());
    expected.fill(Qt::transparent);
    QPainter painter(&expected);
    painter.setClipRect(textRect);
    painter.setFont(option.font);
    painter.setPen(WinUI3::Private::tokens(option.palette).textPrimary);
    painter.drawText(textRect, int(option.displayAlignment) | Qt::TextWordWrap, option.text);
    painter.end();
    QCOMPARE(actual, expected);
}

void WinUI3TableHeaderTest::tableCellElidesLastVisibleLine()
{
    QTableWidget table(1, 1);
    table.ensurePolished();
    QStyleOptionViewItem option;
    option.initFrom(table.viewport());
    option.font = table.font();
    option.fontMetrics = QFontMetrics(option.font);
    option.rect = QRect(0, 0, 180, 2 * option.fontMetrics.height() + 2);
    option.index = table.model()->index(0, 0);
    option.state = QStyle::State_Enabled;
    option.features = QStyleOptionViewItem::HasDisplay | QStyleOptionViewItem::WrapText;
    option.displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;
    option.textElideMode = Qt::ElideRight;
    option.text = QStringLiteral("First line\nSecond line has far too much text\nThird line");
    const QRect textRect =
            table.style()->subElementRect(QStyle::SE_ItemViewItemText, &option, table.viewport());
    QCOMPARE(textRect.left(), 12);
    QCOMPARE(textRect.right(), option.rect.right() - 12);
    const QString lastLine = option.fontMetrics.elidedText(
            QStringLiteral("Second line has far too much text"), Qt::ElideRight, textRect.width());
    QVERIFY(lastLine.endsWith(QChar(0x2026)));
    QImage actual(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
    actual.fill(Qt::transparent);
    {
        QPainter painter(&actual);
        table.style()->drawControl(QStyle::CE_ItemViewItem, &option, &painter, table.viewport());
    }
    QImage expected(actual.size(), actual.format());
    expected.fill(Qt::transparent);
    QPainter painter(&expected);
    painter.setClipRect(textRect);
    painter.setFont(option.font);
    painter.setPen(WinUI3::Private::tokens(option.palette).textPrimary);
    painter.drawText(textRect, int(option.displayAlignment) | Qt::TextWordWrap,
                     QStringLiteral("First line\n") + lastLine);
    painter.end();
    QCOMPARE(actual, expected);
}

void WinUI3TableHeaderTest::tableCellElidesAtHardBreak_data()
{
    QTest::addColumn<bool>("compact");
    QTest::addColumn<int>("elideMode");
    QTest::addColumn<QString>("visibleText");
    const QChar ellipsis(0x2026);
    for (bool compact : { false, true }) {
        const QByteArray prefix = compact ? "compact-" : "standard-";
        QTest::newRow((prefix + "left").constData())
                << compact << int(Qt::ElideLeft) << (ellipsis + QStringLiteral("AB"));
        QTest::newRow((prefix + "middle").constData())
                << compact << int(Qt::ElideMiddle)
                << (QStringLiteral("A") + ellipsis + QLatin1Char('B'));
        QTest::newRow((prefix + "right").constData())
                << compact << int(Qt::ElideRight) << (QStringLiteral("AB") + ellipsis);
    }
}

void WinUI3TableHeaderTest::tableCellElidesAtHardBreak()
{
    QFETCH(bool, compact);
    QFETCH(int, elideMode);
    QFETCH(QString, visibleText);
    QTableWidget table(1, 1);
    table.setProperty("winuiDensity", compact ? "compact" : "standard");
    table.ensurePolished();
    QStyleOptionViewItem option;
    option.initFrom(table.viewport());
    option.font = table.font();
    option.font.setPixelSize(16);
    option.fontMetrics = QFontMetrics(option.font);
    option.rect = QRect(0, 0, 200, option.fontMetrics.height());
    option.index = table.model()->index(0, 0);
    option.state = QStyle::State_Enabled;
    option.features = QStyleOptionViewItem::HasDisplay | QStyleOptionViewItem::WrapText;
    option.displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;
    option.textElideMode = Qt::TextElideMode(elideMode);
    option.text = QStringLiteral("AB\nHidden");
    const QRect textRect =
            table.style()->subElementRect(QStyle::SE_ItemViewItemText, &option, table.viewport());
    QCOMPARE(textRect.left(), option.rect.left() + 12);
    QCOMPARE(textRect.height(), option.rect.height());
    QVERIFY(textRect.right() <= option.rect.right() - 12);
    QVERIFY(QFontMetricsF(option.font).horizontalAdvance(visibleText) <= textRect.width());
    QImage actual(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
    actual.fill(Qt::transparent);
    {
        QPainter painter(&actual);
        table.style()->drawControl(QStyle::CE_ItemViewItem, &option, &painter, table.viewport());
    }
    QImage expected(actual.size(), actual.format());
    expected.fill(Qt::transparent);
    QPainter painter(&expected);
    painter.setClipRect(textRect);
    painter.setFont(option.font);
    painter.setPen(WinUI3::Private::tokens(option.palette).textPrimary);
    painter.drawText(textRect, int(option.displayAlignment) | Qt::TextSingleLine, visibleText);
    painter.end();
    QCOMPARE(actual, expected);
}

void WinUI3TableHeaderTest::tableCellKeepsFittingText_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("fontSize");
    QTest::addColumn<int>("width");
    QTest::newRow("tabs") << QStringLiteral("A\tB") << 14 << 50;
    QTest::newRow("carriage-return") << QStringLiteral("A\rB") << 14 << 50;
    QTest::newRow("fractional-line-height")
            << QStringLiteral("First line\nSecond line") << 22 << 200;
}

void WinUI3TableHeaderTest::tableCellKeepsFittingText()
{
    QFETCH(QString, text);
    QFETCH(int, fontSize);
    QFETCH(int, width);
    QTableWidget table(1, 1);
    table.ensurePolished();
    QStyleOptionViewItem option;
    option.initFrom(table.viewport());
    option.font = table.font();
    option.font.setPixelSize(fontSize);
    option.fontMetrics = QFontMetrics(option.font);
    option.displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;
    const int flags = int(option.displayAlignment) | Qt::TextWordWrap;
    const QFontMetricsF metrics(option.font);
    const int height =
            qFloor(metrics.boundingRect(QRectF(0, 0, width, 1000), flags, text).height());
    option.rect = QRect(0, 0, width + 24, height);
    option.index = table.model()->index(0, 0);
    option.state = QStyle::State_Enabled;
    option.features = QStyleOptionViewItem::HasDisplay | QStyleOptionViewItem::WrapText;
    option.textElideMode = Qt::ElideRight;
    option.text = text;
    const QRect textRect =
            table.style()->subElementRect(QStyle::SE_ItemViewItemText, &option, table.viewport());
    QCOMPARE(textRect, QRect(12, 0, width, height));
    QImage actual(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
    actual.fill(Qt::transparent);
    {
        QPainter painter(&actual);
        table.style()->drawControl(QStyle::CE_ItemViewItem, &option, &painter, table.viewport());
    }
    QImage expected(actual.size(), actual.format());
    expected.fill(Qt::transparent);
    QPainter painter(&expected);
    painter.setClipRect(textRect);
    painter.setFont(option.font);
    painter.setPen(WinUI3::Private::tokens(option.palette).textPrimary);
    painter.drawText(textRect, flags, text);
    painter.end();
    QCOMPARE(actual, expected);
}

void WinUI3TableHeaderTest::tableCellKeepsFittingLineBeforeOverflow()
{
    QTableWidget table(1, 1);
    table.ensurePolished();
    QStyleOptionViewItem option;
    option.initFrom(table.viewport());
    option.font = table.font();
    option.fontMetrics = QFontMetrics(option.font);
    const int width =
            qCeil(QFontMetricsF(option.font).horizontalAdvance(QStringLiteral("First line")));
    option.rect = QRect(0, 0, width + 24, 2 * option.fontMetrics.height() + 2);
    option.index = table.model()->index(0, 0);
    option.state = QStyle::State_Enabled;
    option.features = QStyleOptionViewItem::HasDisplay | QStyleOptionViewItem::WrapText;
    option.displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;
    option.textElideMode = Qt::ElideRight;
    option.text = QStringLiteral("First line\nSecondLineWithTooMuchText");
    const QRect textRect =
            table.style()->subElementRect(QStyle::SE_ItemViewItemText, &option, table.viewport());
    QCOMPARE(textRect.width(), width);
    QImage actual(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
    actual.fill(Qt::transparent);
    {
        QPainter painter(&actual);
        table.style()->drawControl(QStyle::CE_ItemViewItem, &option, &painter, table.viewport());
    }
    QImage expected(actual.size(), actual.format());
    expected.fill(Qt::transparent);
    QPainter painter(&expected);
    painter.setClipRect(textRect);
    painter.setFont(option.font);
    painter.setPen(WinUI3::Private::tokens(option.palette).textPrimary);
    painter.drawText(
            textRect, int(option.displayAlignment) | Qt::TextWordWrap,
            QStringLiteral("First line\n")
                    + option.fontMetrics.elidedText(QStringLiteral("SecondLineWithTooMuchText"),
                                                    Qt::ElideRight, width));
    painter.end();
    QCOMPARE(actual, expected);
}

void WinUI3TableHeaderTest::tableCellHonorsElideNone()
{
    for (bool compact : { false, true }) {
        QTableWidget table(1, 1);
        table.setProperty("winuiDensity", compact ? "compact" : "standard");
        table.ensurePolished();
        QStyleOptionViewItem option;
        option.initFrom(table.viewport());
        option.font = table.font();
        option.fontMetrics = QFontMetrics(option.font);
        option.rect = QRect(0, 0, 120, compact ? 28 : 36);
        option.index = table.model()->index(0, 0);
        option.state = QStyle::State_Enabled;
        option.features = QStyleOptionViewItem::HasDisplay | QStyleOptionViewItem::WrapText;
        option.displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;
        option.textElideMode = Qt::ElideNone;
        option.text = QStringLiteral("A long description that does not fit");
        const QRect textRect = table.style()->subElementRect(QStyle::SE_ItemViewItemText, &option,
                                                             table.viewport());
        QCOMPARE(textRect, option.rect.adjusted(12, 0, -12, 0));
        QImage actual(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
        actual.fill(Qt::transparent);
        {
            QPainter painter(&actual);
            table.style()->drawControl(QStyle::CE_ItemViewItem, &option, &painter,
                                       table.viewport());
        }
        QImage expected(actual.size(), actual.format());
        expected.fill(Qt::transparent);
        QPainter painter(&expected);
        painter.setClipRect(textRect);
        painter.setFont(option.font);
        painter.setPen(WinUI3::Private::tokens(option.palette).textPrimary);
        painter.drawText(textRect, int(option.displayAlignment) | Qt::TextWordWrap, option.text);
        painter.end();
        QCOMPARE(actual, expected);
    }
}

void WinUI3TableHeaderTest::tableCellPreservesWrappedPrefix_data()
{
    QTest::addColumn<bool>("compact");
    QTest::addColumn<bool>("justify");
    QTest::addColumn<bool>("bottom");
    QTest::addColumn<bool>("rtl");
    QTest::newRow("standard-justify") << false << true << false << false;
    QTest::newRow("compact-justify") << true << true << false << false;
    QTest::newRow("standard-soft-hyphen") << false << false << false << false;
    QTest::newRow("compact-soft-hyphen") << true << false << false << false;
    QTest::newRow("standard-bottom") << false << false << true << false;
    QTest::newRow("compact-bottom") << true << false << true << false;
    QTest::newRow("standard-rtl") << false << false << false << true;
    QTest::newRow("compact-rtl") << true << false << false << true;
}

void WinUI3TableHeaderTest::tableCellPreservesWrappedPrefix()
{
    QFETCH(bool, compact);
    QFETCH(bool, justify);
    QFETCH(bool, bottom);
    QFETCH(bool, rtl);
    QTableWidget table(1, 1);
    table.setProperty("winuiDensity", compact ? "compact" : "standard");
    table.ensurePolished();
    QStyleOptionViewItem option;
    option.initFrom(table.viewport());
    option.font = table.font();
    option.fontMetrics = QFontMetrics(option.font);
    const int width = justify || bottom || rtl
            ? 100
            : option.fontMetrics.horizontalAdvance(QStringLiteral("hy-"));
    option.rect = QRect(0, 0, width + 24, 2 * option.fontMetrics.height() + 2);
    option.index = table.model()->index(0, 0);
    option.state = QStyle::State_Enabled;
    option.features = QStyleOptionViewItem::HasDisplay | QStyleOptionViewItem::WrapText;
    option.displayAlignment = (justify ? Qt::AlignJustify : Qt::AlignLeft)
            | (bottom ? Qt::AlignBottom : Qt::AlignVCenter);
    option.direction = rtl ? Qt::RightToLeft : Qt::LeftToRight;
    option.textElideMode = Qt::ElideRight;
    option.text = rtl
            ? QStringLiteral(
                      "\u05e9\u05dc\u05d5\u05dd \u05e2\u05d5\u05dc\u05dd \u05e9\u05dc\u05d5\u05dd "
                      "\u05e2\u05d5\u05dc\u05dd \u05e9\u05dc\u05d5\u05dd \u05e2\u05d5\u05dc\u05dd")
            : justify || bottom
            ? QStringLiteral("First line with extra text that wraps into several lines")
            : QStringLiteral("hy\u00adphenLongMoreText tail word");
    const QRect textRect =
            table.style()->subElementRect(QStyle::SE_ItemViewItemText, &option, table.viewport());
    QCOMPARE(textRect.width(), width);
    QTextLayout layout(option.text, option.font);
    QTextOption textOption;
    textOption.setWrapMode(QTextOption::WordWrap);
    textOption.setAlignment(justify ? Qt::AlignJustify : Qt::AlignLeft);
    textOption.setTextDirection(rtl ? Qt::RightToLeft : Qt::LeftToRight);
    layout.setTextOption(textOption);
    layout.beginLayout();
    QTextLine first = layout.createLine();
    first.setLineWidth(width);
    first.setPosition(QPointF(0, 0));
    QTextLine second = layout.createLine();
    QVERIFY(second.isValid());
    second.setLineWidth(width);
    second.setPosition(QPointF(
            0, qCeil(first.ascent() + first.descent() + QFontMetricsF(option.font).leading())));
    const qreal height = second.y() + second.ascent() + second.descent();
    layout.endLayout();
    if (!justify && !bottom && !rtl)
        QCOMPARE(option.text.at(first.textLength() - 1), QChar(0x00ad));
    if (rtl)
        QVERIFY(option.text.at(first.textLength() - 1).isSpace());
    const qreal top = bottom ? textRect.height() - height : (textRect.height() - height) / 2;
    QImage expected(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
    expected.fill(Qt::transparent);
    {
        QPainter painter(&expected);
        painter.setPen(WinUI3::Private::tokens(option.palette).textPrimary);
        if (rtl || bottom) {
            painter.setFont(option.font);
            painter.setLayoutDirection(option.direction);
            const Qt::Alignment horizontal = QStyle::visualAlignment(
                    option.direction,
                    Qt::Alignment(option.displayAlignment)
                            & (Qt::AlignHorizontal_Mask | Qt::AlignAbsolute));
            painter.drawText(QRectF(textRect.left(), top, width, height),
                             int(horizontal) | Qt::AlignTop | Qt::TextWordWrap, option.text);
        } else {
            first.draw(&painter, QPointF(textRect.left(), top));
        }
    }
    QImage actual(expected.size(), expected.format());
    actual.fill(Qt::transparent);
    {
        QPainter painter(&actual);
        painter.setLayoutDirection(option.direction);
        table.style()->drawControl(QStyle::CE_ItemViewItem, &option, &painter, table.viewport());
    }
    const QRect firstLineRect(12, 0, width, qFloor(top + second.y()));
    QVERIFY(firstLineRect.isValid());
    QCOMPARE(actual.copy(firstLineRect), expected.copy(firstLineRect));
}

void WinUI3TableHeaderTest::tableHeaderContract()
{
    QTableWidget table(2, 2);
    table.setLayoutDirection(Qt::RightToLeft);
    table.setHorizontalHeaderLabels({ QStringLiteral("Control"), QStringLiteral("State") });
    table.setSortingEnabled(true);
    table.resize(420, 180);
    table.show();
    QTRY_VERIFY(table.isVisible());
    QVERIFY(table.horizontalHeader()->height() >= 32);
    QVERIFY(table.verticalHeader()->sectionSize(0) >= 36);

    QStyleOptionHeader option;
    option.initFrom(table.horizontalHeader());
    option.rect = QRect(0, 0, 180, 32);
    option.text = QStringLiteral("Control");
    option.textAlignment = Qt::AlignLeft;
    option.sortIndicator = QStyleOptionHeader::SortDown;
    QImage header(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
    header.fill(Qt::transparent);
    {
        QPainter painter(&header);
        table.style()->drawControl(QStyle::CE_HeaderSection, &option, &painter,
                                   table.horizontalHeader());
        table.style()->drawControl(QStyle::CE_HeaderLabel, &option, &painter,
                                   table.horizontalHeader());
    }
    QVERIFY(!header.isNull());
    QVERIFY(header.pixelColor(option.rect.right() - 16, option.rect.center().y()).alpha() > 0);
}

void WinUI3TableHeaderTest::tableHeaderLabelKeepsLeadingEdgeWithSortColumn()
{
    QTableWidget table(2, 2);
    table.setHorizontalHeaderLabels({ QStringLiteral("Control"), QStringLiteral("State") });
    table.setSortingEnabled(true);
    table.resize(420, 180);
    table.show();
    QTRY_VERIFY(table.isVisible());
    QHeaderView *header = table.horizontalHeader();
    // Qt's stock horizontal header defaults to AlignCenter and the Gallery
    // table leaves it there; the pinned DataGrid template does not centre.
    QCOMPARE(header->defaultAlignment(), Qt::Alignment(Qt::AlignCenter));

    const auto render = [&table, header](QStyleOptionHeader::SortIndicator indicator,
                                         const QString &text, int section) {
        QStyleOptionHeader option;
        option.initFrom(header);
        option.rect = QRect(0, 0, 180, 32);
        option.text = text;
        option.section = section;
        option.orientation = Qt::Horizontal;
        option.textAlignment = header->defaultAlignment();
        option.sortIndicator = indicator;
        QImage image(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        table.style()->drawControl(QStyle::CE_Header, &option, &painter, header);
        return image;
    };

    const QImage sorted = render(QStyleOptionHeader::SortDown, QStringLiteral("Control"), 0);
    const QImage unsorted = render(QStyleOptionHeader::None, QStringLiteral("Control"), 0);
    QRect difference;
    for (int y = 0; y < sorted.height(); ++y) {
        for (int x = 0; x < sorted.width(); ++x) {
            if (sorted.pixel(x, y) != unsorted.pixel(x, y))
                difference |= QRect(x, y, 1, 1);
        }
    }
    // The trailing 32 px sort column may add the glyph and nothing else. The
    // template reserves that column while the header is sortable (Opacity 0
    // glyph), so the label must not jump when a sort starts.
    const QRect column(180 - 32, 0, 32, 32);
    QVERIFY(!difference.isEmpty());
    QVERIFY(column.contains(difference));

    // The label sits at the template's Padding="12,0,0,0" leading inset, not
    // at Qt's centred default: the sort column must not push it off-centre
    // relative to the column edge.
    const QRect labelInk =
            opaqueBounds(glyphInk(sorted, render(QStyleOptionHeader::SortDown, QString(), 0)));
    QVERIFY(!labelInk.isEmpty());
    QCOMPARE(labelInk.left(), 12);
    QVERIFY(labelInk.right() < column.left());

    // A header and its cells share one leading edge: the template's header
    // Padding 12 and DataGridTextColumn's 12 px TextBlock margin are the same
    // number, so a cell must not start 4 px left of its header title.
    QStyleOptionViewItem cell;
    cell.initFrom(table.viewport());
    cell.index = table.model()->index(0, 0);
    cell.rect = table.visualRect(cell.index);
    cell.features = QStyleOptionViewItem::HasDisplay;
    const QRect cellText =
            table.style()->subElementRect(QStyle::SE_ItemViewItemText, &cell, table.viewport());
    QVERIFY(cell.rect.isValid());
    QCOMPARE(cellText.left() - cell.rect.left(), labelInk.left());

    // An explicit TextAlignmentRole still wins over the template default.
    table.horizontalHeaderItem(0)->setTextAlignment(Qt::AlignRight);
    const QImage rightAligned = render(QStyleOptionHeader::SortDown, QStringLiteral("Control"), 0);
    const QRect rightAlignedInk = opaqueBounds(
            glyphInk(rightAligned, render(QStyleOptionHeader::SortDown, QString(), 0)));
    QVERIFY(!rightAlignedInk.isEmpty());
    // Right-aligned ink ends at the content edge (the sort column's left
    // edge, since the template adds no trailing header inset) instead of the
    // 12 px leading inset.
    QVERIFY(rightAlignedInk.right() >= column.left() - 2
            && rightAlignedInk.right() <= column.left());
    QVERIFY(rightAlignedInk.left() > labelInk.left());
}

void WinUI3TableHeaderTest::tableResizeToContentsIncludesCellPadding()
{
    // Live CommitForm defect: the Git status table sizes Commit and Status
    // with ResizeToContents, but the WinUI table cell insets its text by 12 px
    // on each side. If the item view's content size omits that gutter, the
    // column collapses to the header label width and the cell text is clipped.
    QTableWidget table(1, 1);
    table.setHorizontalHeaderLabels({ QStringLiteral("Status") });
    table.setItem(0, 0, new QTableWidgetItem(QStringLiteral("Untracked")));
    table.resize(320, 120);
    table.show();
    QTRY_VERIFY(table.isVisible());

    QHeaderView *header = table.horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    header->resizeSections(QHeaderView::ResizeToContents);
    qApp->processEvents();

    const QFontMetrics fm(table.font());
    const int textWidth = fm.horizontalAdvance(QStringLiteral("Untracked"));
    QVERIFY(textWidth > 0);
    // Same shared leading edge as the header: exactly 12 px on both sides.
    QStyleOptionViewItem cell;
    cell.initFrom(table.viewport());
    cell.index = table.model()->index(0, 0);
    cell.rect = table.visualRect(cell.index);
    cell.features = QStyleOptionViewItem::HasDisplay;
    const QRect textRect =
            table.style()->subElementRect(QStyle::SE_ItemViewItemText, &cell, table.viewport());
    QCOMPARE(textRect.left() - cell.rect.left(), 12);
    QCOMPARE(cell.rect.right() - textRect.right(), 12);
    // The mechanism: after "resize to contents" the text slot must actually
    // fit the text it was sized for. Before the fix the column was only as
    // wide as the header title (~43 px), leaving ~19 px for a 64 px string.
    QVERIFY2(textRect.width() >= textWidth,
             qPrintable(QStringLiteral("column=%1 textRect=%2 text=%3")
                                .arg(header->sectionSize(0))
                                .arg(textRect.width())
                                .arg(textWidth)));
    QVERIFY2(header->sectionSize(0) >= textWidth + 24,
             qPrintable(QStringLiteral("column=%1 text=%2")
                                .arg(header->sectionSize(0))
                                .arg(textWidth)));
}

void WinUI3TableHeaderTest::tableResizeToContentsKeepsHeaderLabelVisible()
{
    // Live CommitForm first column: only a check indicator sits in the cells,
    // so the item view hint is narrower than the "Commit" header title. A
    // ResizeToContents section must still reserve the header's 12 px leading
    // edge and the full title, or the header is elided away.
    QTableWidget table(1, 1);
    table.setHorizontalHeaderLabels({ QStringLiteral("Commit") });
    auto *commit = new QTableWidgetItem;
    commit->setCheckState(Qt::Unchecked);
    table.setItem(0, 0, commit);
    table.resize(320, 120);
    table.show();
    QTRY_VERIFY(table.isVisible());

    QHeaderView *header = table.horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    header->resizeSections(QHeaderView::ResizeToContents);
    qApp->processEvents();

    QStyleOptionHeader option;
    option.initFrom(header);
    option.section = 0;
    option.orientation = Qt::Horizontal;
    option.text = QStringLiteral("Commit");
    QFont font = header->font();
    font.setBold(true);
    option.fontMetrics = QFontMetrics(font);
    const int textWidth = option.fontMetrics.horizontalAdvance(option.text);
    QVERIFY(textWidth > 0);

    const QSize hint =
            header->style()->sizeFromContents(QStyle::CT_HeaderSection, &option, QSize(), header);
    QVERIFY2(hint.width() >= textWidth + 24,
             qPrintable(QStringLiteral("hint=%1 text=%2").arg(hint.width()).arg(textWidth)));

    option.rect = QRect(0, 0, header->sectionSize(0), header->height());
    option.textAlignment = header->defaultAlignment();
    const QRect labelRect =
            header->style()->subElementRect(QStyle::SE_HeaderLabel, &option, header);
    QVERIFY2(header->sectionSize(0) >= textWidth + 24,
             qPrintable(QStringLiteral("column=%1 text=%2")
                                .arg(header->sectionSize(0))
                                .arg(textWidth)));
    QVERIFY2(labelRect.width() >= textWidth,
             qPrintable(
                     QStringLiteral("labelRect=%1 text=%2").arg(labelRect.width()).arg(textWidth)));
}

void WinUI3TableHeaderTest::tableSortIndicatorGeometryContract()
{
    QTableWidget table(1, 1);
    table.resize(240, 80);
    table.show();
    QTRY_VERIFY(table.isVisible());

    const auto render = [&table](Qt::LayoutDirection direction,
                                 QStyleOptionHeader::SortIndicator indicator, int height) {
        QStyleOptionHeader option;
        option.initFrom(table.horizontalHeader());
        option.rect = QRect(0, 0, 180, height);
        option.direction = direction;
        option.text.clear();
        option.textAlignment = Qt::AlignLeft;
        option.sortIndicator = indicator;
        QImage image(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        table.style()->drawControl(QStyle::CE_Header, &option, &painter, table.horizontalHeader());
        return image;
    };

    // Pinned DataGrid column header template: header padding ends at the
    // section's trailing edge, and the sort column is 32 px wide and as tall
    // as the header. LTR trails right, RTL trails left.
    const QRect ltrColumn(180 - 32, 0, 32, 33);
    const QRect rtlColumn(0, 0, 32, 33);

    for (const Qt::LayoutDirection direction : { Qt::LeftToRight, Qt::RightToLeft }) {
        const QRect column = direction == Qt::LeftToRight ? ltrColumn : rtlColumn;
        const QImage unsorted = render(direction, QStyleOptionHeader::None, 33);
        const QImage descending =
                glyphInk(render(direction, QStyleOptionHeader::SortDown, 33), unsorted);
        const QRect difference = opaqueBounds(descending);
        QVERIFY(!difference.isEmpty());
        // The pinned glyph is a full-height sort arrow painted into a 12 px
        // box, not a 16 px chevron slot, and sits inside the 32 px trailing
        // column, roughly centred (advance-centred, like the FontIcon).
        QVERIFY(difference.height() >= 10 && difference.height() <= 13);
        // Ink bounds vary slightly with Qt's platform font rasterizer even
        // though the renderer always paints into the pinned 12x12 glyph box.
        // Keep the contract on the actual reserved 32 px column below rather
        // than treating rasterized ink width as the glyph's logical size.
        QVERIFY(difference.width() <= column.width());
        QVERIFY(column.contains(difference));
        // Keep the glyph's rasterized ink inside the reserved sort column;
        // its exact trailing ink gap depends on the platform font rasterizer.
        // SortDown puts the two diagonal head strokes in the lower half;
        // comparing half-glyph ink is robust to rasterizer endpoint rounding.
        const int middle = difference.center().y();
        QVERIFY(opaqueCountInRows(descending, middle + 1, difference.bottom())
                > opaqueCountInRows(descending, difference.top(), middle));
        const QImage ascending =
                glyphInk(render(direction, QStyleOptionHeader::SortUp, 33), unsorted);
        const QRect ascendingInk = opaqueBounds(ascending);
        QVERIFY(!ascendingInk.isEmpty());
        QVERIFY(ascendingInk.height() >= 10);
        const int ascendingMiddle = ascendingInk.center().y();
        QVERIFY(opaqueCountInRows(ascending, ascendingInk.top(), ascendingMiddle)
                > opaqueCountInRows(ascending, ascendingMiddle + 1, ascendingInk.bottom()));
        QVERIFY(ascending != descending);
    }
}

void WinUI3TableHeaderTest::tableHeaderPaletteFollowsThemeRoundTrip_data()
{
    QTest::addColumn<QString>("density");
    QTest::newRow("standard") << QStringLiteral("standard");
    QTest::newRow("compact") << QStringLiteral("compact");
}

void WinUI3TableHeaderTest::tableHeaderPaletteFollowsThemeRoundTrip()
{
    QFETCH(QString, density);
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    // Offscreen palettes carry surface alpha; the theme contract is about the
    // RGB the header resolves, so compare with alpha normalized away.
    const auto rgb = [](const QColor &c) {
        QColor n = c;
        n.setAlpha(255);
        return n;
    };
    QTableWidget table(4, 3);
    table.setProperty("winuiDensity", density);
    table.setHorizontalHeaderLabels(
            { QStringLiteral("Control"), QStringLiteral("State"), QStringLiteral("Coverage") });
    table.resize(560, 220);
    table.show();
    QTRY_VERIFY(table.isVisible());
    const auto headerBase = [&table, &rgb] {
        return rgb(table.horizontalHeader()->palette().color(QPalette::Base));
    };
    const auto viewBase = [&table, &rgb] { return rgb(table.palette().color(QPalette::Base)); };

    style->setThemeMode(WinUI3::ThemeMode::Dark);
    qApp->processEvents();
    const QColor darkBase = viewBase();
    QCOMPARE(headerBase(), darkBase);

    style->setThemeMode(WinUI3::ThemeMode::Light);
    qApp->processEvents();
    const QColor lightBase = viewBase();
    QVERIFY(lightBase != darkBase);
    QCOMPARE(headerBase(), lightBase);

    // Issue #63: after dark -> light -> dark the header kept the Light palette
    // (stale implicit-owner snapshot disowned it) while the view rebased fine.
    style->setThemeMode(WinUI3::ThemeMode::Dark);
    qApp->processEvents();
    QCOMPARE(headerBase(), darkBase);
    QCOMPARE(headerBase(), viewBase());

    style->setThemeMode(WinUI3::ThemeMode::Light);
    qApp->processEvents();
}

void WinUI3TableHeaderTest::tableHeaderBandContrastsAlternateRows_data()
{
    QTest::addColumn<QString>("density");
    QTest::newRow("standard") << QStringLiteral("standard");
    QTest::newRow("compact") << QStringLiteral("compact");
}

void WinUI3TableHeaderTest::tableHeaderBandContrastsAlternateRows()
{
    QFETCH(QString, density);
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QTableWidget table(6, 3);
    table.setProperty("winuiDensity", density);
    table.setAlternatingRowColors(true);
    table.horizontalHeader()->setSortIndicatorShown(false);
    table.setHorizontalHeaderLabels(
            { QStringLiteral("Control"), QStringLiteral("State"), QStringLiteral("Coverage") });
    for (int r = 0; r < 6; ++r)
        for (int c = 0; c < 3; ++c)
            table.setItem(r, c, new QTableWidgetItem(QStringLiteral("r%1").arg(r)));
    table.resize(560, 260);
    table.show();
    QTRY_VERIFY(table.isVisible());

    for (WinUI3::ThemeMode mode : { WinUI3::ThemeMode::Dark, WinUI3::ThemeMode::Light }) {
        style->setThemeMode(mode);
        qApp->processEvents();
        const QImage img = table.grab().toImage();
        const int left = table.verticalHeader()->width() + 1;
        const int top = table.horizontalHeader()->height() + 1;
        const int headerX = left + table.horizontalHeader()->sectionViewportPosition(0)
                + table.horizontalHeader()->sectionSize(0) - 6;
        const QColor header = img.pixelColor(headerX, top / 2);
        const QRect r0 = table.visualRect(table.model()->index(0, 1));
        const QRect r1 = table.visualRect(table.model()->index(1, 1));
        const QColor odd = img.pixelColor(left + r0.right() - 8, top + r0.center().y());
        const QColor even = img.pixelColor(left + r1.right() - 8, top + r1.center().y());
        const WinUI3::Private::Tokens t = WinUI3::Private::tokens(table.palette());
        if (mode == WinUI3::ThemeMode::Dark) {
            // Dark band rests on the Base layer, a touch above the alternate row.
            QCOMPARE(t.tableHeaderFill, t.layer);
            QVERIFY(header.red() > even.red());
        } else {
            // Light band sits a touch BELOW the alternate row (issue #63 follow-up).
            QCOMPARE(t.tableHeaderFill,
                     WinUI3::Private::mix(table.palette().color(QPalette::AlternateBase),
                                          table.palette().color(QPalette::WindowText), 0.05));
            QVERIFY(header.red() < even.red());
        }
        QVERIFY(qAbs(header.red() - even.red()) <= 8);
    }
    style->setThemeMode(WinUI3::ThemeMode::Light);
    qApp->processEvents();
}

QTEST_MAIN(WinUI3TableHeaderTest)
#include "tst_winui3tableheader.moc"
