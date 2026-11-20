// SPDX-License-Identifier: LGPL-2.1-or-later
#include <winui3style/winui3style.h>

#include "gallerywindow.h"

#include <QComboBox>
#include <QHeaderView>
#include <QLineEdit>
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
};

void WinUI3GalleryDensityTest::initTestCase()
{
    qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light));
}

void WinUI3GalleryDensityTest::galleryTableDensitySwitch_data()
{
    QTest::addColumn<bool>("dark");
    QTest::newRow("light") << false;
    QTest::newRow("dark") << true;
}

void WinUI3GalleryDensityTest::galleryTableDensitySwitch()
{
    QFETCH(bool, dark);
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
    gallery.show();
    QVERIFY(QTest::qWaitForWindowExposed(&gallery));
    const int standardRow = table->rowHeight(0);
    QCOMPARE(table->horizontalHeader()->height(), 32);
    QVERIFY(standardRow >= 36);
    density->setCurrentIndex(1);
    QTRY_COMPARE(table->rowHeight(0), 28);
    QTRY_COMPARE(table->horizontalHeader()->height(), 24);
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
    QTRY_COMPARE(table->rowHeight(0), standardRow);
    QTRY_COMPARE(table->horizontalHeader()->height(), 32);
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

QTEST_MAIN(WinUI3GalleryDensityTest)
#include "tst_winui3gallerydensity.moc"
