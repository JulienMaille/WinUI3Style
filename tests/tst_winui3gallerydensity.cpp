// SPDX-License-Identifier: LGPL-2.1-or-later
#include <winui3style/winui3style.h>

#include "gallerywindow.h"
#include "../src/lib/winui3tokens_p.h"

#include <QComboBox>
#include <QFrame>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTableWidget>
#include <QTreeWidget>
#include <QtTest>

class WinUI3GalleryDensityTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void galleryTableDensitySwitch_data();
    void galleryTableDensitySwitch();
    void galleryTreeHeaderDensitySwitch_data();
    void galleryTreeHeaderDensitySwitch();
    void galleryTreeRowHighlight_data();
    void galleryTreeRowHighlight();
    void galleryDensityPanelsExposeVerticalNumberBoxes();
};

void WinUI3GalleryDensityTest::initTestCase()
{
    qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light));
}

void WinUI3GalleryDensityTest::galleryTableDensitySwitch_data()
{
    QTest::addColumn<bool>("dark");
    QTest::addColumn<bool>("customColumnSize");
    QTest::newRow("light") << false << false;
    QTest::newRow("dark") << true << false;
    QTest::newRow("light-custom-column-size") << false << true;
    QTest::newRow("dark-custom-column-size") << true << true;
}

void WinUI3GalleryDensityTest::galleryTableDensitySwitch()
{
    QFETCH(bool, dark);
    QFETCH(bool, customColumnSize);
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    style->setThemeMode(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
    GalleryWindow gallery;
    auto *density = gallery.findChild<QComboBox *>(QStringLiteral("densityCombo"));
    auto *pages = gallery.findChild<QStackedWidget *>(QStringLiteral("pages"));
    auto *tabs = gallery.findChild<QTabWidget *>(QStringLiteral("collectionsTabs"));
    auto *table = gallery.findChild<QTableWidget *>(QStringLiteral("tableViewTab"));
    QVERIFY(density && pages && tabs && table);
    density->setCurrentIndex(0);
    pages->setCurrentIndex(1);
    tabs->setCurrentWidget(table);
    auto *header = table->horizontalHeader();
    constexpr int columnSize = 160;
    if (customColumnSize) {
        header->setStretchLastSection(false);
        header->setDefaultSectionSize(columnSize);
    }
    gallery.show();
    QVERIFY(QTest::qWaitForWindowExposed(&gallery));
    const int standardRow = table->rowHeight(0);
    QCOMPARE(table->horizontalHeader()->height(), 32);
    QVERIFY(standardRow >= 36);
    density->setCurrentIndex(1);
    QCOMPARE(header->height(), 24);
    if (customColumnSize)
        QCOMPARE(table->columnWidth(0), columnSize);
    QTRY_COMPARE(table->rowHeight(0), 28);
    const QSize compactViewport = table->viewport()->size();
    gallery.resize(gallery.size() + QSize(80, 60));
    QTRY_VERIFY(table->viewport()->size() != compactViewport);
    QCOMPARE(table->rowHeight(0), 28);
    QCOMPARE(table->horizontalHeader()->height(), 24);
    const QPoint cell = table->visualRect(table->model()->index(0, 0)).center();
    QTest::mouseMove(table->viewport(), cell);
    QTest::mousePress(table->viewport(), Qt::LeftButton, Qt::NoModifier, cell);
    QTest::mouseRelease(table->viewport(), Qt::LeftButton, Qt::NoModifier, cell);
    QCOMPARE(table->currentRow(), 0);
    QTest::keyClick(table, Qt::Key_Down);
    QCOMPARE(table->currentRow(), 1);
    QTest::keyClick(table, Qt::Key_F2);
    auto *editor = table->findChild<QLineEdit *>();
    QTRY_VERIFY(editor && editor->isVisible());
    QVERIFY(editor->height() >= editor->sizeHint().height());
    QTest::keyClick(editor, Qt::Key_Escape);
    density->setCurrentIndex(0);
    QCOMPARE(header->height(), 32);
    if (customColumnSize)
        QCOMPARE(table->columnWidth(0), columnSize);
    QTRY_COMPARE(table->rowHeight(0), standardRow);
}

void WinUI3GalleryDensityTest::galleryTreeHeaderDensitySwitch_data()
{
    QTest::addColumn<bool>("dark");
    QTest::newRow("light") << false;
    QTest::newRow("dark") << true;
}

void WinUI3GalleryDensityTest::galleryTreeHeaderDensitySwitch()
{
    QFETCH(bool, dark);
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    style->setThemeMode(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
    GalleryWindow gallery;
    auto *density = gallery.findChild<QComboBox *>(QStringLiteral("densityCombo"));
    auto *pages = gallery.findChild<QStackedWidget *>(QStringLiteral("pages"));
    auto *tabs = gallery.findChild<QTabWidget *>(QStringLiteral("collectionsTabs"));
    auto *tree = gallery.findChild<QTreeWidget *>(QStringLiteral("plainTreeViewTab"));
    QVERIFY(density && pages && tabs && tree);
    density->setCurrentIndex(0);
    pages->setCurrentIndex(1);
    tabs->setCurrentWidget(tree);
    gallery.show();
    QVERIFY(QTest::qWaitForWindowExposed(&gallery));
    QVERIFY(!tree->isHeaderHidden());
    QCOMPARE(tree->columnCount(), 2);
    QCOMPARE(tree->header()->height(), 32);
    const int columnWidth = tree->columnWidth(0);
    const QFont font = tree->header()->font();
    auto *root = tree->topLevelItem(0);
    QVERIFY(root && root->isExpanded());
    QCOMPARE(tree->visualItemRect(root).height(), 28);
    density->setCurrentIndex(1);
    QCOMPARE(tree->visualItemRect(root).height(), 24);
    QCOMPARE(tree->header()->height(), 24);
    QCOMPARE(tree->columnWidth(0), columnWidth);
    QCOMPARE(tree->header()->font(), font);
    auto *child = root->child(0);
    QVERIFY(child);
    const QPoint cell = tree->visualItemRect(child).center();
    QTest::mouseMove(tree->viewport(), cell);
    QTest::mousePress(tree->viewport(), Qt::LeftButton, Qt::NoModifier, cell);
    QTest::mouseRelease(tree->viewport(), Qt::LeftButton, Qt::NoModifier, cell);
    QCOMPARE(tree->currentItem(), child);
    QTest::keyClick(tree, Qt::Key_Down);
    QCOMPARE(tree->currentItem(), root->child(1));
    const QPoint headerCell(tree->header()->sectionViewportPosition(0) + columnWidth / 2, 12);
    const Qt::SortOrder previousSort = tree->header()->sortIndicatorOrder();
    QTest::mouseMove(tree->header()->viewport(), headerCell);
    QTest::mousePress(tree->header()->viewport(), Qt::LeftButton, Qt::NoModifier, headerCell);
    QTest::mouseRelease(tree->header()->viewport(), Qt::LeftButton, Qt::NoModifier, headerCell);
    QCOMPARE(tree->header()->sortIndicatorOrder(),
             previousSort == Qt::AscendingOrder ? Qt::DescendingOrder : Qt::AscendingOrder);
    const QSize viewport = tree->viewport()->size();
    gallery.resize(gallery.size() + QSize(80, 60));
    QTRY_VERIFY(tree->viewport()->size() != viewport);
    QCOMPARE(tree->header()->height(), 24);
    QCOMPARE(tree->columnWidth(0), columnWidth);
    tree->setCurrentItem(root);
    QTest::keyClick(tree, Qt::Key_Left);
    QVERIFY(!root->isExpanded());
    QTest::keyClick(tree, Qt::Key_Right);
    QVERIFY(root->isExpanded());
    QCOMPARE(tree->visualItemRect(root->child(0)).height(), 24);
    density->setCurrentIndex(0);
    QTRY_COMPARE(tree->header()->height(), 32);
    QCOMPARE(tree->visualItemRect(root).height(), 28);
    QCOMPARE(tree->columnWidth(0), columnWidth);
    QVERIFY(root->isExpanded());
}

void WinUI3GalleryDensityTest::galleryTreeRowHighlight_data()
{
    QTest::addColumn<bool>("compact");
    QTest::addColumn<bool>("rtl");
    QTest::newRow("standard-ltr") << false << false;
    QTest::newRow("compact-ltr") << true << false;
    QTest::newRow("standard-rtl") << false << true;
    QTest::newRow("compact-rtl") << true << true;
}

void WinUI3GalleryDensityTest::galleryTreeRowHighlight()
{
    QFETCH(bool, compact);
    QFETCH(bool, rtl);
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    style->setThemeMode(WinUI3::ThemeMode::Dark);
    GalleryWindow gallery;
    auto *pages = gallery.findChild<QStackedWidget *>(QStringLiteral("pages"));
    auto *tabs = gallery.findChild<QTabWidget *>(QStringLiteral("collectionsTabs"));
    auto *tree = gallery.findChild<QTreeWidget *>(QStringLiteral("plainTreeViewTab"));
    QVERIFY(pages && tabs && tree);
    pages->setCurrentIndex(1);
    tabs->setCurrentWidget(tree);
    tree->setLayoutDirection(rtl ? Qt::RightToLeft : Qt::LeftToRight);
    WinUI3::Style::setDensityMode(
            tree, compact ? WinUI3::DensityMode::Compact : WinUI3::DensityMode::Standard);
    tree->header()->setStretchLastSection(false);
    tree->setColumnWidth(1, 1000);
    tree->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    gallery.show();
    QVERIFY(QTest::qWaitForWindowExposed(&gallery));
    QVERIFY(tree->horizontalScrollBar()->maximum() > 30);
    tree->horizontalScrollBar()->setValue(30);
    auto *child = tree->topLevelItem(0)->child(0);
    QVERIFY(child);
    const QRect row = tree->visualItemRect(child);
    QCOMPARE(row.height(), compact ? 24 : 28);
    QCOMPARE(tree->selectionBehavior(), QAbstractItemView::SelectRows);
    const int seam = rtl ? tree->columnViewportPosition(0) : tree->columnViewportPosition(1);
    // Sample on either side of the seam, away from fractionally scaled clip edges.
    const int sampleInset = tree->devicePixelRatioF() == 1.0 ? 0 : 2;
    const QPoint before(seam - 1 - sampleInset, row.center().y());
    const QPoint after(seam + sampleInset, row.center().y());
    QVERIFY(tree->viewport()->rect().contains(before));
    QVERIFY(tree->viewport()->rect().contains(after));
    tree->clearSelection();
    QEvent leave(QEvent::Leave);
    QCoreApplication::sendEvent(tree->viewport(), &leave);
    const QImage rest = tree->viewport()->grab().toImage();
    const auto tokens = WinUI3::Private::tokens(tree->palette());
    const auto checkHighlight = [&] {
        const QImage painted = tree->viewport()->grab().toImage();
        for (const QPoint point : { before, after }) {
            const qreal dpr = painted.devicePixelRatio();
            const QPoint sample(int((point.x() + 0.5) * dpr), int((point.y() + 0.5) * dpr));
            QImage expected(1, 1, QImage::Format_ARGB32_Premultiplied);
            expected.fill(rest.pixelColor(sample));
            QPainter painter(&expected);
            painter.fillRect(expected.rect(), tokens.subtleHover);
            painter.end();
            QCOMPARE(painted.pixelColor(sample), expected.pixelColor(0, 0));
        }
    };
    const QPoint hoverPoint = rtl ? before - QPoint(28, 0) : after + QPoint(28, 0);
    QMouseEvent move(QEvent::MouseMove, QPointF(hoverPoint),
                     QPointF(tree->viewport()->mapToGlobal(hoverPoint)), Qt::NoButton, Qt::NoButton,
                     Qt::NoModifier);
    QCoreApplication::sendEvent(tree->viewport(), &move);
    checkHighlight();
    QCoreApplication::sendEvent(tree->viewport(), &leave);
    child->setSelected(true);
    checkHighlight();
}

void WinUI3GalleryDensityTest::galleryDensityPanelsExposeVerticalNumberBoxes()
{
    GalleryWindow gallery;
    gallery.show();
    QVERIFY(QTest::qWaitForWindowExposed(&gallery));
    QCoreApplication::processEvents();
    for (const QString prefix : { QStringLiteral("standard"), QStringLiteral("compact") }) {
        auto *panel = gallery.findChild<QFrame *>(prefix + QStringLiteral("DensityPanel"));
        auto *spin =
                gallery.findChild<QSpinBox *>(prefix + QStringLiteral("DensityVerticalNumber"));
        auto *label =
                gallery.findChild<QLabel *>(prefix + QStringLiteral("DensityVerticalNumberLabel"));
        QVERIFY2(panel && spin && label, qPrintable(prefix));
        QVERIFY(WinUI3::Style::hasVerticalSpinButtons(spin));
        QCOMPARE(spin->value(), 42);
        auto *horizontal = gallery.findChild<QSpinBox *>(prefix + QStringLiteral("DensityNumber"));
        QVERIFY(horizontal);
        QVERIFY2(spin->width() >= horizontal->width() - 1,
                 qPrintable(QStringLiteral("%1 vertical NumberBox is narrower: %2 vs %3")
                                    .arg(prefix)
                                    .arg(spin->width())
                                    .arg(horizontal->width())));
        QVERIFY(!label->text().isEmpty());
        QCOMPARE(panel->property("winuiDensity").toString(),
                 prefix == QStringLiteral("standard") ? QStringLiteral("standard")
                                                      : QStringLiteral("compact"));
    }
}

QTEST_MAIN(WinUI3GalleryDensityTest)
#include "tst_winui3gallerydensity.moc"
