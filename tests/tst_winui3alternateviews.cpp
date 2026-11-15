// SPDX-License-Identifier: LGPL-2.1-or-later
#include <winui3style/winui3style.h>
#include <winui3style/winui3icons.h>

#include "../src/lib/winui3icons_p.h"
#include "../src/lib/winui3tokens_p.h"

#include <QAbstractItemView>
#include <QImage>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QTableWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QtTest>

#include "winui3testhelpers.h"

class WinUI3AlternateViewsTest final : public QObject, private WinUI3StandardTestFixture
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void darkAlternateRowsStaySubtle_data();
    void darkAlternateRowsStaySubtle();
    void treeBranchGlyphUsesWinUIGlyphSize_data();
    void treeBranchGlyphUsesWinUIGlyphSize();
    void treeIconTextGapMatchesWinUI_data();
    void treeIconTextGapMatchesWinUI();
};

void WinUI3AlternateViewsTest::initTestCase()
{
    initializeStandardWinUI3TestStyle();
}

void WinUI3AlternateViewsTest::init()
{
    resetStandardWinUI3TestStyle();
}

void WinUI3AlternateViewsTest::cleanup()
{
    cleanupStandardWinUI3TestWidgets();
}

void WinUI3AlternateViewsTest::darkAlternateRowsStaySubtle_data()
{
    QTest::addColumn<int>("densityMode");
    QTest::newRow("standard") << int(WinUI3::DensityMode::Standard);
    QTest::newRow("compact") << int(WinUI3::DensityMode::Compact);
}

void WinUI3AlternateViewsTest::darkAlternateRowsStaySubtle()
{
    QFETCH(int, densityMode);
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setThemeMode(WinUI3::ThemeMode::Dark);
    style->setProperty("densityMode", densityMode);
    QCOMPARE(style->property("densityMode").toInt(), densityMode);

    QTableWidget table(2, 1);
    table.horizontalHeader()->hide();
    table.verticalHeader()->hide();
    table.setShowGrid(false);
    table.setAlternatingRowColors(true);
    table.setSelectionMode(QAbstractItemView::NoSelection);
    table.setEditTriggers(QAbstractItemView::NoEditTriggers);
    table.setColumnWidth(0, 220);
    table.setRowHeight(0, 32);
    table.setRowHeight(1, 32);
    table.setItem(0, 0, new QTableWidgetItem);
    table.setItem(1, 0, new QTableWidgetItem);
    table.resize(240, 72);
    table.show();
    QTRY_VERIFY(table.isVisible());

    const QRect baseRow = table.visualRect(table.model()->index(0, 0));
    const QRect alternateRow = table.visualRect(table.model()->index(1, 0));
    QCOMPARE(baseRow.width(), 220);
    QVERIFY(baseRow.height() > 0);
    QCOMPARE(alternateRow.size(), baseRow.size());
    const QColor alternateToken = table.palette().color(QPalette::AlternateBase);
    QCOMPARE(alternateToken, QColor(255, 255, 255, 1));

    const QImage viewport = table.viewport()->grab().toImage();
    const int sampleX = qMin(baseRow.right(), alternateRow.right()) - 4;
    const QColor basePixel = viewport.pixelColor(sampleX, baseRow.center().y());
    const QColor alternatePixel = viewport.pixelColor(sampleX, alternateRow.center().y());
    const int maxChannelDelta = qMax(qAbs(basePixel.red() - alternatePixel.red()),
                                     qMax(qAbs(basePixel.green() - alternatePixel.green()),
                                          qAbs(basePixel.blue() - alternatePixel.blue())));
    QVERIFY2(maxChannelDelta <= 3,
             qPrintable(QStringLiteral("Base %1 vs alternate %2 (max delta %3)")
                                .arg(basePixel.name(), alternatePixel.name())
                                .arg(maxChannelDelta)));
}

void WinUI3AlternateViewsTest::treeBranchGlyphUsesWinUIGlyphSize_data()
{
    QTest::addColumn<QString>("density");
    QTest::newRow("standard") << QStringLiteral("standard");
    QTest::newRow("compact") << QStringLiteral("compact");
}

void WinUI3AlternateViewsTest::treeBranchGlyphUsesWinUIGlyphSize()
{
    QFETCH(QString, density);
    // Force the vector artwork so the glyph footprint is platform-independent.
    WinUI3::Private::setIconFontAvailabilityOverrideForTest(0);
    struct RestoreFontDetection
    {
        ~RestoreFontDetection() { WinUI3::Private::setIconFontAvailabilityOverrideForTest(-1); }
    } restore;

    QTreeWidget tree;
    tree.setHeaderHidden(true);
    tree.setProperty("winuiDensity", density);
    auto *root = new QTreeWidgetItem(&tree, { QStringLiteral("Documents") });
    new QTreeWidgetItem(root, { QStringLiteral("ProjectProposal") });
    tree.expandAll();
    tree.resize(320, 120);
    tree.show();
    QTRY_VERIFY(tree.isVisible());

    const auto inkBounds = [](const QImage &img) {
        QRect bounds;
        for (int y = 0; y < img.height(); ++y)
            for (int x = 0; x < img.width(); ++x)
                if (img.pixelColor(x, y).alpha() > 40)
                    bounds |= QRect(x, y, 1, 1);
        return bounds;
    };

    QStyleOption branch;
    branch.initFrom(tree.viewport());
    branch.rect = QRect(0, 0, 20, 28);
    branch.state = QStyle::State_Enabled | QStyle::State_Children | QStyle::State_Open;
    QImage image(branch.rect.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    {
        QPainter painter(&image);
        tree.style()->drawPrimitive(QStyle::PE_IndicatorBranch, &branch, &painter, tree.viewport());
    }
    const QRect glyph = inkBounds(image);
    QVERIFY(!glyph.isEmpty());

    // TreeViewItem.xaml paints the disclosure glyph at GlyphSize=8 inside a 12x12
    // box with Padding=2 (issue #55). The branch ink must match that footprint,
    // not a glyph scaled to the full 12 px slot.
    QImage reference(8, 8, QImage::Format_ARGB32_Premultiplied);
    reference.fill(Qt::transparent);
    {
        QPainter painter(&reference);
        WinUI3::icon(WinUI3::Icon::ChevronDown, QColor(Qt::white))
                .paint(&painter, QRect(0, 0, 8, 8));
    }
    const QRect referenceInk = inkBounds(reference);
    QCOMPARE(glyph.width(), referenceInk.width());
    QCOMPARE(glyph.height(), referenceInk.height());
    QVERIFY(glyph.width() <= 8);
}

void WinUI3AlternateViewsTest::treeIconTextGapMatchesWinUI_data()
{
    QTest::addColumn<QString>("density");
    QTest::newRow("standard") << QStringLiteral("standard");
    QTest::newRow("compact") << QStringLiteral("compact");
}

void WinUI3AlternateViewsTest::treeIconTextGapMatchesWinUI()
{
    QFETCH(QString, density);
    QTreeWidget tree;
    tree.setHeaderHidden(true);
    tree.setIconSize(QSize(16, 16));
    tree.setProperty("winuiDensity", density);
    auto *root = new QTreeWidgetItem(&tree, { QStringLiteral("Documents") });
    root->setIcon(0, WinUI3::icon(WinUI3::Icon::Folder));
    new QTreeWidgetItem(root, { QStringLiteral("ProjectProposal") });
    tree.expandAll();
    tree.resize(320, 120);
    tree.show();
    QTRY_VERIFY(tree.isVisible());

    QStyleOptionViewItem option;
    option.initFrom(tree.viewport());
    option.widget = tree.viewport();
    option.index = treeIndexFromItem(&tree, root);
    option.rect = tree.visualRect(option.index);
    option.features = QStyleOptionViewItem::HasDecoration | QStyleOptionViewItem::HasDisplay;
    option.decorationSize = QSize(16, 16);
    option.icon = WinUI3::icon(WinUI3::Icon::Folder);
    option.text = root->text(0);
    const QRect decoration = tree.style()->subElementRect(QStyle::SE_ItemViewItemDecoration,
                                                          &option, tree.viewport());
    const QRect text =
            tree.style()->subElementRect(QStyle::SE_ItemViewItemText, &option, tree.viewport());
    // The WinUI Gallery TreeView template separates the icon from the label with
    // a 10 px spacer (TextBlock Margin="0,0,10,0"); issue #55.
    QCOMPARE(text.left() - decoration.right() - 1, 10);
}

QTEST_MAIN(WinUI3AlternateViewsTest)
#include "tst_winui3alternateviews.moc"
