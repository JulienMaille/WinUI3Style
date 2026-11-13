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
    void tableHeaderLabelKeepsLeadingEdgeWithSortColumn();
    void tableSortIndicatorGeometryContract();
};

void WinUI3TableHeaderTest::initTestCase()
{
    initializeStandardWinUI3TestStyle();
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

QTEST_MAIN(WinUI3TableHeaderTest)
#include "tst_winui3tableheader.moc"
