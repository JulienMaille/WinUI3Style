// SPDX-License-Identifier: LGPL-2.1-or-later
#include <winui3style/winui3style.h>

#include "../src/winui3tokens_p.h"

#include <QAbstractItemView>
#include <QTableWidget>
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

QTEST_MAIN(WinUI3AlternateViewsTest)
#include "tst_winui3alternateviews.moc"
