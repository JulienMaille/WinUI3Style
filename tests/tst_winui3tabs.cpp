// SPDX-License-Identifier: LGPL-2.1-or-later
#include <winui3style/winui3style.h>

#include "winui3testhelpers.h"

#include <QImage>
#include <QPainter>
#include <QStyleOptionTab>
#include <QTabBar>
#include <QtTest>

class WinUI3TabsTest final : public QObject, private WinUI3StandardTestFixture
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void dockTabLabelOrientation_data();
    void dockTabLabelOrientation();
};

void WinUI3TabsTest::initTestCase()
{
    initializeStandardWinUI3TestStyle();
}

void WinUI3TabsTest::init()
{
    resetStandardWinUI3TestStyle();
}

void WinUI3TabsTest::cleanup()
{
    cleanupStandardWinUI3TestWidgets();
}

void WinUI3TabsTest::dockTabLabelOrientation_data()
{
    QTest::addColumn<int>("densityMode");
    QTest::newRow("standard") << int(WinUI3::DensityMode::Standard);
    QTest::newRow("compact") << int(WinUI3::DensityMode::Compact);
}

void WinUI3TabsTest::dockTabLabelOrientation()
{
    QFETCH(int, densityMode);
    qApp->style()->setProperty("densityMode", densityMode);
    QCOMPARE(qApp->style()->property("densityMode").toInt(), densityMode);

    const auto inkBounds = [](const QImage &image, const QColor &background) {
        QRect bounds;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                if (colorDistance(image.pixelColor(x, y), background) > 32)
                    bounds |= QRect(QPoint(x, y), QSize(1, 1));
            }
        }
        return bounds;
    };
    const auto renderLabel = [](QSize size, QTabBar::Shape shape) {
        QStyleOptionTab option;
        option.rect = QRect(QPoint(0, 0), size);
        option.shape = shape;
        option.palette = qApp->palette();
        option.state = QStyle::State_Enabled | QStyle::State_Selected;
        option.text = QStringLiteral("First dock");
        QImage image(size, QImage::Format_ARGB32_Premultiplied);
        const QColor background = option.palette.color(QPalette::Window);
        image.fill(background);
        {
            QPainter painter(&image);
            qApp->style()->drawControl(QStyle::CE_TabBarTabLabel, &option, &painter);
        }
        return qMakePair(image, background);
    };

    const QSize verticalGeometry(32, 120);
    for (QTabBar::Shape shape : { QTabBar::RoundedWest, QTabBar::RoundedEast }) {
        const auto vertical = renderLabel(verticalGeometry, shape);
        const QRect verticalInk = inkBounds(vertical.first, vertical.second);
        QCOMPARE(vertical.first.size(), verticalGeometry);
        QVERIFY(!verticalInk.isEmpty());
        QVERIFY(verticalInk.height() > verticalInk.width());
    }

    QTabBar verticalBar;
    verticalBar.setShape(QTabBar::RoundedWest);
    verticalBar.addTab(QStringLiteral("First dock"));
    verticalBar.addTab(QStringLiteral("Second dock"));
    verticalBar.resize(32, 240);
    verticalBar.show();
    QTRY_VERIFY(verticalBar.isVisible());
    QStyleOptionTab verticalOption;
    verticalOption.initFrom(&verticalBar);
    verticalOption.shape = QTabBar::RoundedWest;
    verticalOption.text = QStringLiteral("First dock");
    const QSize verticalSize = verticalBar.style()->sizeFromContents(
            QStyle::CT_TabBarTab, &verticalOption,
            verticalBar.fontMetrics().size(Qt::TextSingleLine, verticalOption.text), &verticalBar);
    QCOMPARE(verticalSize.width(), 32);
    QVERIFY(verticalSize.height() >= 100);
    QCOMPARE(verticalBar.tabRect(0).width(), 32);
    QVERIFY(verticalBar.tabRect(0).height() >= 100);

    const QSize horizontalGeometry(180, 32);
    const auto horizontal = renderLabel(horizontalGeometry, QTabBar::RoundedNorth);
    const QRect horizontalInk = inkBounds(horizontal.first, horizontal.second);
    QCOMPARE(horizontal.first.size(), horizontalGeometry);
    QVERIFY(!horizontalInk.isEmpty());
    QVERIFY(qAbs(horizontalInk.center().x() - horizontalGeometry.width() / 2) <= 2);
}

QTEST_MAIN(WinUI3TabsTest)
#include "tst_winui3tabs.moc"
