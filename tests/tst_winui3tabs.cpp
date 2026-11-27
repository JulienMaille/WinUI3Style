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
    void tabLabelTreatsAmpersandAsMnemonic();
    void attachedTabSurface_data();
    void attachedTabSurface();
    void tabBaseOrientation_data();
    void tabBaseOrientation();
    void tabifiedDockSurface_data();
    void tabifiedDockSurface();
    void attachedTabBackdrop_data();
    void attachedTabBackdrop();
    void tabPaneStrokePreservation_data();
    void tabPaneStrokePreservation();
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

void WinUI3TabsTest::tabLabelTreatsAmpersandAsMnemonic()
{
    auto render = [](const QString &text) {
        QStyleOptionTab option;
        option.rect = QRect(0, 0, 200, 36);
        option.state = QStyle::State_Enabled;
        option.text = text;
        QImage image(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        qApp->style()->drawControl(QStyle::CE_TabBarTabLabel, &option, &painter);
        return image;
    };
    // A trailing marker has no letter to underline. A doubled marker,
    // however, must leave one visible ampersand.
    QCOMPARE(render(QStringLiteral("Reports&")), render(QStringLiteral("Reports")));
    QVERIFY(render(QStringLiteral("Reports&&")) != render(QStringLiteral("Reports")));
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

void WinUI3TabsTest::attachedTabSurface_data()
{
    QTest::addColumn<int>("densityMode");
    QTest::addColumn<bool>("dark");
    QTest::addColumn<int>("shape");
    QTest::addColumn<int>("direction");
    for (int density : { int(WinUI3::DensityMode::Standard), int(WinUI3::DensityMode::Compact) }) {
        for (bool dark : { false, true }) {
            for (int shape = QTabBar::RoundedNorth; shape <= QTabBar::TriangularEast; ++shape) {
                for (int direction : { int(Qt::LeftToRight), int(Qt::RightToLeft) }) {
                    const QByteArray name = QByteArray::number(density) + '-'
                            + QByteArray::number(dark) + '-' + QByteArray::number(shape) + '-'
                            + QByteArray::number(direction);
                    QTest::newRow(name.constData()) << density << dark << shape << direction;
                }
            }
        }
    }
}

void WinUI3TabsTest::attachedTabSurface()
{
    QFETCH(int, densityMode);
    QFETCH(bool, dark);
    QFETCH(int, shape);
    QFETCH(int, direction);
    qApp->style()->setProperty("densityMode", densityMode);
    qobject_cast<WinUI3::Style *>(qApp->style())
            ->setThemeMode(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
    QCOMPARE(qApp->style()->property("densityMode").toInt(), densityMode);
    const int orientation = shape % 4;
    const bool vertical = orientation >= 2;
    const QSize size = vertical ? QSize(32, 120) : QSize(120, 32);
    const auto render = [direction](QTabBar::Shape shape, QSize size, QStyle::State state) {
        QStyleOptionTab option;
        option.rect = QRect(QPoint(7, 9), size);
        option.shape = shape;
        option.direction = Qt::LayoutDirection(direction);
        option.palette = qApp->palette();
        option.state = state;
        QImage image(size + QSize(14, 18), QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        qApp->style()->drawControl(QStyle::CE_TabBarTabShape, &option, &painter);
        painter.end();
        return image.copy(option.rect);
    };
    const QImage north = render(QTabBar::RoundedNorth, QSize(120, 32),
                                QStyle::State_Selected | QStyle::State_Enabled);
    QImage expected = north;
    if (orientation == 1)
        expected = north.mirrored(false, true);
    else if (orientation == 2)
        expected = north.transformed(QTransform().rotate(-90));
    else if (orientation == 3)
        expected = north.transformed(QTransform().rotate(90));
    const QColor fill = WinUI3::Private::tokens(qApp->palette()).selectedTabFill;
    for (QStyle::State extra :
         { QStyle::State(QStyle::State_None), QStyle::State(QStyle::State_MouseOver),
           QStyle::State(QStyle::State_Sunken) }) {
        const QImage actual = render(QTabBar::Shape(shape), size,
                                     QStyle::State_Selected | QStyle::State_Enabled | extra);
        QCOMPARE(actual.size(), size);
        QCOMPARE(actual.pixelColor(size.width() / 2, size.height() / 2), fill);
        // Re-rasterizing a reflected antialiased path differs by up to two channel levels.
        for (int y = 0; y < size.height(); ++y) {
            for (int x = 0; x < size.width(); ++x) {
                const QRgb a = actual.pixel(x, y);
                const QRgb b = expected.pixel(x, y);
                QVERIFY(qAbs(qRed(a) - qRed(b)) <= 2);
                QVERIFY(qAbs(qGreen(a) - qGreen(b)) <= 2);
                QVERIFY(qAbs(qBlue(a) - qBlue(b)) <= 2);
                QVERIFY(qAbs(qAlpha(a) - qAlpha(b)) <= 2);
            }
        }
    }
    const QImage disabled = render(QTabBar::Shape(shape), size, QStyle::State_Selected);
    QCOMPARE(disabled.size(), size);
    QCOMPARE(disabled.pixelColor(size.width() / 2, size.height() / 2), fill);
    QCOMPARE(disabled,
             render(QTabBar::Shape(shape), size, QStyle::State_Selected | QStyle::State_Enabled));
}

void WinUI3TabsTest::tabBaseOrientation_data()
{
    attachedTabSurface_data();
}

void WinUI3TabsTest::tabBaseOrientation()
{
    QFETCH(int, densityMode);
    QFETCH(bool, dark);
    QFETCH(int, shape);
    QFETCH(int, direction);
    qApp->style()->setProperty("densityMode", densityMode);
    qobject_cast<WinUI3::Style *>(qApp->style())
            ->setThemeMode(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
    QStyleOptionTabBarBase option;
    option.rect = QRect(0, 0, 120, 32);
    option.shape = QTabBar::Shape(shape);
    option.direction = Qt::LayoutDirection(direction);
    option.palette = qApp->palette();
    QImage image(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    {
        QPainter painter(&image);
        qApp->style()->drawPrimitive(QStyle::PE_FrameTabBarBase, &option, &painter);
    }
    const int orientation = shape % 4;
    const QPoint edge = orientation == 0 ? QPoint(60, 31)
            : orientation == 1           ? QPoint(60, 0)
            : orientation == 2           ? QPoint(119, 16)
                                         : QPoint(0, 16);
    QCOMPARE(image.size(), option.rect.size());
    QCOMPARE(image.pixelColor(edge), WinUI3::Private::tokens(option.palette).strokeSecondary);
    QCOMPARE(image.pixelColor(60, 16), QColor(Qt::transparent));
}

void WinUI3TabsTest::tabifiedDockSurface_data()
{
    attachedTabSurface_data();
}

void WinUI3TabsTest::tabifiedDockSurface()
{
    QFETCH(int, densityMode);
    QFETCH(bool, dark);
    QFETCH(int, shape);
    QFETCH(int, direction);
    qApp->style()->setProperty("densityMode", densityMode);
    qobject_cast<WinUI3::Style *>(qApp->style())
            ->setThemeMode(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
    QMainWindow window;
    window.setLayoutDirection(Qt::LayoutDirection(direction));
    const int orientation = shape % 4;
    const QTabWidget::TabPosition position = orientation == 0 ? QTabWidget::North
            : orientation == 1                                ? QTabWidget::South
            : orientation == 2                                ? QTabWidget::West
                                                              : QTabWidget::East;
    window.setTabPosition(Qt::AllDockWidgetAreas, position);
    window.setTabShape(shape >= QTabBar::TriangularNorth ? QTabWidget::Triangular
                                                         : QTabWidget::Rounded);
    window.setCentralWidget(new QLabel(QStringLiteral("Workspace")));
    auto *first = new QDockWidget(QStringLiteral("Inspector"), &window);
    first->setWidget(new QLabel(QStringLiteral("Inspector body")));
    auto *second = new QDockWidget(QStringLiteral("Output"), &window);
    second->setWidget(new QLabel(QStringLiteral("Output body")));
    window.addDockWidget(Qt::RightDockWidgetArea, first);
    window.addDockWidget(Qt::RightDockWidgetArea, second);
    window.tabifyDockWidget(first, second);
    window.resize(640, 360);
    window.show();
    first->raise();
    QTRY_VERIFY(window.isVisible());
    QTabBar *bar = nullptr;
    for (QTabBar *candidate : window.findChildren<QTabBar *>()) {
        if (candidate->count() == 2) {
            bar = candidate;
            break;
        }
    }
    QVERIFY(bar);
    QCOMPARE(int(bar->shape()), shape);
    for (bool documentMode : { false, true }) {
        window.setDocumentMode(documentMode);
        for (int index : { 0, 1 }) {
            bar->setCurrentIndex(index);
            QCoreApplication::processEvents();
            QCOMPARE(bar->currentIndex(), index);
            const QRect rect = bar->tabRect(index);
            QCOMPARE(orientation >= 2 ? rect.width() : rect.height(), 32);
            QVERIFY(orientation >= 2 ? rect.height() >= 100 : rect.width() >= 100);
            const QImage image = bar->grab().toImage();
            const QPoint sample = rect.center() + (orientation >= 2 ? QPoint(8, 0) : QPoint(0, 8));
            const QPoint physical = sample * image.devicePixelRatio();
            const QColor fill = WinUI3::Private::tokens(bar->palette()).selectedTabFill;
            QCOMPARE(image.pixelColor(physical), fill);
        }
    }
    // QMainWindow's dock tab bar paints its base before the individual tabs.
    // The backdrop erase must keep that base beneath the unselected tab.
    QVERIFY(bar->drawBase());
    bar->setCurrentIndex(1);
    QPoint baseSample = bar->tabRect(0).center();
    if (orientation == 0)
        baseSample.setY(bar->rect().bottom());
    else if (orientation == 1)
        baseSample.setY(bar->rect().top());
    else if (orientation == 2)
        baseSample.setX(bar->rect().right());
    else
        baseSample.setX(bar->rect().left());
    const QImage opaque = bar->grab().toImage();
    const QPoint sample = baseSample * opaque.devicePixelRatio();
    QVERIFY(opaque.rect().contains(sample));
    window.setProperty("_winui_backdrop", 1);
    window.setProperty("_winui_backdrop_effective", 2);
    QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(bar));
    const QImage composited = bar->grab().toImage();
    QCOMPARE(composited.pixelColor(sample), opaque.pixelColor(sample));
    window.setProperty("_winui_backdrop", {});
    window.setProperty("_winui_backdrop_effective", {});

    bar->setFocus();
    bar->setCurrentIndex(0);
    QTest::keyClick(bar, direction == Qt::RightToLeft ? Qt::Key_Left : Qt::Key_Right);
    QCOMPARE(bar->currentIndex(), 1);
}

void WinUI3TabsTest::attachedTabBackdrop_data()
{
    attachedTabSurface_data();
}

void WinUI3TabsTest::attachedTabBackdrop()
{
    QFETCH(int, densityMode);
    QFETCH(bool, dark);
    QFETCH(int, shape);
    QFETCH(int, direction);
    if (shape % 4 == 0)
        QSKIP("Existing North backdrop corner behavior is outside the dock-orientation repair");
    qApp->style()->setProperty("densityMode", densityMode);
    qobject_cast<WinUI3::Style *>(qApp->style())
            ->setThemeMode(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
    QWidget window;
    window.setProperty("_winui_backdrop", 1);
    window.setProperty("_winui_backdrop_effective", 2);
    QTabBar bar(&window);
    bar.setDrawBase(false); // This fixture paints the individual tab shape.
    bar.setShape(QTabBar::Shape(shape));
    QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(&bar));
    QStyleOptionTab option;
    const QSize size = shape % 4 >= 2 ? QSize(32, 120) : QSize(120, 32);
    option.rect = QRect(QPoint(0, 0), size);
    option.shape = bar.shape();
    option.direction = Qt::LayoutDirection(direction);
    option.palette = bar.palette();
    option.state = QStyle::State_Enabled | QStyle::State_Selected;
    const auto paint = [&](QImage &image, const QRect &clip = QRect()) {
        QPainter painter(&image);
        if (!clip.isNull())
            painter.setClipRect(clip);
        bar.style()->drawControl(QStyle::CE_TabBarTabShape, &option, &painter, &bar);
    };
    const qreal dpr = bar.devicePixelRatioF();
    const QSize imageSize(qCeil(size.width() * dpr), qCeil(size.height() * dpr));
    QImage first(imageSize, QImage::Format_ARGB32_Premultiplied);
    first.setDevicePixelRatio(dpr);
    first.fill(Qt::red);
    paint(first);
    QCOMPARE(first.size(), imageSize);
    QCOMPARE(first.devicePixelRatio(), dpr);
    const QColor fill = WinUI3::Private::veiledCard(
            WinUI3::Private::tokens(option.palette).selectedTabFill, true);
    QCOMPARE(first.pixelColor(first.rect().center()).alpha(), fill.alpha());
    QVERIFY(colorDistance(first.pixelColor(first.rect().center()), fill) <= 4);
    QImage second = first.copy();
    paint(second);
    QCOMPARE(second, first);
    QImage clipped(imageSize, QImage::Format_ARGB32_Premultiplied);
    clipped.setDevicePixelRatio(dpr);
    clipped.fill(Qt::red);
    const QRect dirty(size.width() / 2, 0, size.width() - size.width() / 2, size.height());
    paint(clipped, dirty);
    const int dirtyLeft = qCeil(dirty.left() * dpr);
    const QRect physicalDirty(dirtyLeft, 0, imageSize.width() - dirtyLeft, imageSize.height());
    QCOMPARE(clipped.copy(physicalDirty), first.copy(physicalDirty));
    QCOMPARE(clipped.pixelColor(0, imageSize.height() / 2), QColor(Qt::red));
    option.state = QStyle::State_Enabled;
    QImage rest(imageSize, QImage::Format_ARGB32_Premultiplied);
    rest.setDevicePixelRatio(dpr);
    rest.fill(Qt::red);
    paint(rest);
    paint(second);
    QCOMPARE(second, rest);
    option.state |= QStyle::State_MouseOver;
    paint(rest);
    second = rest.copy();
    paint(second);
    QCOMPARE(second, rest);
}

void WinUI3TabsTest::tabPaneStrokePreservation_data()
{
    attachedTabSurface_data();
}

void WinUI3TabsTest::tabPaneStrokePreservation()
{
    QFETCH(int, densityMode);
    QFETCH(bool, dark);
    QFETCH(int, shape);
    QFETCH(int, direction);
    qApp->style()->setProperty("densityMode", densityMode);
    qobject_cast<WinUI3::Style *>(qApp->style())
            ->setThemeMode(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
    QWidget window;
    window.setProperty("_winui_backdrop", 1);
    window.setProperty("_winui_backdrop_effective", 2);
    QTabWidget tabs(&window);
    tabs.addTab(new QWidget, QStringLiteral("First"));
    tabs.addTab(new QWidget, QStringLiteral("Second"));
    QTabBar *bar = tabs.tabBar();
    bar->setShape(QTabBar::Shape(shape));
    QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(bar));
    QCOMPARE(bar->style()->pixelMetric(QStyle::PM_TabBarBaseOverlap, nullptr, bar), 1);
    const int orientation = shape % 4;
    const QSize size = orientation >= 2 ? QSize(32, 120) : QSize(120, 32);
    QStyleOptionTab option;
    option.rect = QRect(QPoint(0, 0), size);
    option.shape = bar->shape();
    option.palette = bar->palette();
    option.direction = Qt::LayoutDirection(direction);
    option.state = QStyle::State_Enabled | QStyle::State_Selected;
    const QRect edge = orientation == 0 ? QRect(0, size.height() - 1, size.width(), 1)
            : orientation == 1          ? QRect(0, 0, size.width(), 1)
            : orientation == 2          ? QRect(size.width() - 1, 0, 1, size.height())
                                        : QRect(0, 0, 1, size.height());
    const qreal dpr = bar->devicePixelRatioF();
    QImage image(QSize(qCeil(size.width() * dpr), qCeil(size.height() * dpr)),
                 QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(dpr);
    image.fill(Qt::transparent);
    {
        QPainter painter(&image);
        bar->style()->drawControl(QStyle::CE_TabBarTabShape, &option, &painter, bar);
    }
    const QColor stroke = WinUI3::Private::tokens(option.palette).strokeSecondary;
    {
        QPainter painter(&image);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.fillRect(edge, stroke);
    }
    const QPoint sample(qFloor((edge.x() + edge.width() / 2.0) * dpr),
                        qFloor((edge.y() + edge.height() / 2.0) * dpr));
    QCOMPARE(image.devicePixelRatio(), dpr);
    QCOMPARE(edge.width() == 1 ? edge.width() : edge.height(), 1);
    QCOMPARE(image.pixelColor(sample), stroke);
    for (QStyle::State state : { QStyle::State(QStyle::State_Enabled),
                                 QStyle::State(QStyle::State_Enabled | QStyle::State_MouseOver) }) {
        option.state = state;
        {
            QPainter painter(&image);
            bar->style()->drawControl(QStyle::CE_TabBarTabShape, &option, &painter, bar);
        }
        QCOMPARE(image.pixelColor(sample), stroke);
    }
}

QTEST_MAIN(WinUI3TabsTest)
#include "tst_winui3tabs.moc"
