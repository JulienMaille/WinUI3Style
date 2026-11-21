// SPDX-License-Identifier: LGPL-2.1-or-later
#include <winui3style/winui3style.h>
#include <winui3style/winui3icons.h>

#include "../src/lib/winui3helpers_p.h"
#include "../src/lib/winui3style_properties_p.h"
#include "../src/lib/winui3tokens_p.h"

#include <QAbstractButton>
#include <QColor>
#include <QDockWidget>
#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QImage>
#include <QLabel>
#include <QMainWindow>
#include <QPainter>
#include <QPoint>
#include <QPointer>
#include <QScopeGuard>
#include <QSize>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleOptionDockWidget>
#include <QStyleOptionToolButton>
#include <QTest>
#include <QToolButton>

#include "winui3testhelpers.h"

class WinUI3DockTitlesTest final : public QObject, private WinUI3StandardTestFixture
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void dockTitleButtonsStayFlatAndInset();
    void dockTitleComposition_data();
    void dockTitleComposition();
    void dockTitleOwnershipLifecycle();
    void dockTitleStyleSwitch_data();
    void dockTitleStyleSwitch();
    void dockTitleButtonBackdrop_data();
    void dockTitleButtonBackdrop();
    void dockTitleLargeFont_data();
    void dockTitleLargeFont();
    void dockTitleModifiedPlaceholder();
};

void WinUI3DockTitlesTest::initTestCase()
{
    initializeStandardWinUI3TestStyle();
}

void WinUI3DockTitlesTest::init()
{
    resetStandardWinUI3TestStyle();
}

void WinUI3DockTitlesTest::cleanup()
{
    cleanupStandardWinUI3TestWidgets();
}

class DockTitleProbe final : public QDockWidget
{
public:
    using QDockWidget::QDockWidget;

    void fillStyleOption(QStyleOptionDockWidget *option) const
    {
        const_cast<DockTitleProbe *>(this)->initStyleOption(option);
    }
};

void WinUI3DockTitlesTest::dockTitleButtonsStayFlatAndInset()
{
    const auto verifyDockTitleButtons = [](WinUI3::DensityMode mode) {
        QMainWindow host;
        DockTitleProbe dock(QStringLiteral("Inspector"), &host);
        dock.setWidget(new QLabel(QStringLiteral("Dock content")));
        dock.setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable
                         | QDockWidget::DockWidgetMovable);
        host.addDockWidget(Qt::RightDockWidgetArea, &dock);
        host.resize(520, 260);
        host.show();
        QVERIFY(QTest::qWaitForWindowExposed(&host));

        QStyle *style = host.style();
        QVERIFY(style);
        if (mode == WinUI3::DensityMode::Compact) {
            QVERIFY(style->setProperty("densityMode", int(WinUI3::DensityMode::Compact)));
            QCoreApplication::processEvents();
        }
        QStyleOptionDockWidget option;
        dock.fillStyleOption(&option);
        const QRect titleArea = option.rect;
        QVERIFY(!titleArea.isNull());

        QCOMPARE(style->styleHint(QStyle::SH_DockWidget_ButtonsHaveFrame, &option, &dock), 0);

        QCOMPARE(style->pixelMetric(QStyle::PM_DockWidgetTitleMargin, &option, &dock), 8);
        const QRect closeRect =
                style->subElementRect(QStyle::SE_DockWidgetCloseButton, &option, &dock);
        const QRect floatRect =
                style->subElementRect(QStyle::SE_DockWidgetFloatButton, &option, &dock);
        QVERIFY(!closeRect.isNull());
        QVERIFY(!floatRect.isNull());
        QCOMPARE(titleArea.right() - closeRect.right(), 8);
        QCOMPARE(titleArea.right() - floatRect.right(), 8 + closeRect.width() + 1);
        QVERIFY2(floatRect.right() < closeRect.left(),
                 qPrintable(QStringLiteral("float right=%1 close left=%2")
                                    .arg(floatRect.right())
                                    .arg(closeRect.left())));
        QVERIFY(!closeRect.intersects(floatRect));
        QCOMPARE(closeRect.height(), floatRect.height());
        QCOMPARE(closeRect.center().y(), floatRect.center().y());

        QStyleOptionDockWidget mirrored = option;
        mirrored.direction = Qt::RightToLeft;
        const QRect mirroredClose =
                style->subElementRect(QStyle::SE_DockWidgetCloseButton, &mirrored, &dock);
        QVERIFY(!mirroredClose.isNull());
        QCOMPARE(mirroredClose.left() - titleArea.left(), 8);
        QCOMPARE(mirroredClose.width(), closeRect.width());
        QCOMPARE(mirroredClose.y(), closeRect.y());

        const QRect textRect =
                style->subElementRect(QStyle::SE_DockWidgetTitleBarText, &option, &dock);
        QVERIFY(!textRect.isNull());
        QCOMPARE(textRect.left() - titleArea.left(), 8);
        const QRect mirroredText =
                style->subElementRect(QStyle::SE_DockWidgetTitleBarText, &mirrored, &dock);
        QCOMPARE(titleArea.right() - mirroredText.right(), 8);

        QAbstractButton *closeButton = nullptr;
        QAbstractButton *floatButton = nullptr;
        QVERIFY(dock.titleBarWidget());
        for (QAbstractButton *button : dock.titleBarWidget()->findChildren<QAbstractButton *>()) {
            if (closeRect.contains(button->geometry().center()))
                closeButton = button;
            if (floatRect.contains(button->geometry().center()))
                floatButton = button;
        }
        QVERIFY2(closeButton, "qt dock title close button not found");
        QVERIFY2(floatButton, "qt dock title float button not found");
        QCOMPARE(style->pixelMetric(QStyle::PM_SmallIconSize, nullptr, closeButton), 12);
        QCOMPARE(style->pixelMetric(QStyle::PM_SmallIconSize, nullptr, floatButton), 12);
        QCOMPARE(closeButton->sizeHint(), closeRect.size());
        QCOMPARE(floatButton->sizeHint(), floatRect.size());
        QVERIFY(WinUI3::isFluentIcon(
                style->standardIcon(QStyle::SP_TitleBarNormalButton, &option, &dock)));
        const qreal dpr = closeButton->devicePixelRatioF();
        const QImage dockImage = dock.grab().toImage();
        QVERIFY(!dockImage.isNull());
        const QColor cardFill =
                dockImage.pixelColor(qRound((floatRect.right() + closeRect.left()) / 2.0 * dpr),
                                     qRound(closeRect.center().y() * dpr));
        const int closeInk = inkPixels(dockImage, closeRect, dpr, cardFill);
        const int floatInk = inkPixels(dockImage, floatRect, dpr, cardFill);
        QVERIFY2(closeInk > 0, "dock title close glyph painted no ink");
        QVERIFY2(floatInk > 0, "dock title float glyph painted no ink");
        QImage expected(dockImage.size(), QImage::Format_ARGB32_Premultiplied);
        expected.setDevicePixelRatio(dpr);
        expected.fill(cardFill);
        {
            QPainter painter(&expected);
            const QColor ink = WinUI3::Private::tokens(dock.palette()).textPrimary;
            for (const auto &entry : { qMakePair(QStyle::SP_TitleBarNormalButton, floatRect),
                                       qMakePair(QStyle::SP_TitleBarCloseButton, closeRect) }) {
                const QPixmap glyph = WinUI3::iconPixmap(
                        style->standardIcon(entry.first, &option, &dock), QSize(12, 12), dpr, ink);
                QCOMPARE(WinUI3::Private::deviceIndependentSize(glyph), QSizeF(12, 12));
                painter.drawPixmap(QRectF(entry.second).center() - QPointF(6, 6), glyph);
            }
        }
        QCOMPARE(floatInk, inkPixels(expected, floatRect, dpr, cardFill));
        QCOMPARE(closeInk, inkPixels(expected, closeRect, dpr, cardFill));
    };

    verifyDockTitleButtons(WinUI3::DensityMode::Standard);
    verifyDockTitleButtons(WinUI3::DensityMode::Compact);
}

void WinUI3DockTitlesTest::dockTitleComposition_data()
{
    QTest::addColumn<bool>("compact");
    QTest::addColumn<bool>("vertical");
    QTest::addColumn<bool>("rtl");
    QTest::addColumn<bool>("dark");
    for (bool compact : { false, true })
        for (bool vertical : { false, true })
            for (bool rtl : { false, true })
                for (bool dark : { false, true })
                    QTest::newRow(qPrintable(QStringLiteral("%1-%2-%3-%4")
                                                     .arg(compact)
                                                     .arg(vertical)
                                                     .arg(rtl)
                                                     .arg(dark)))
                            << compact << vertical << rtl << dark;
}

void WinUI3DockTitlesTest::dockTitleComposition()
{
    QFETCH(bool, compact);
    QFETCH(bool, vertical);
    QFETCH(bool, rtl);
    QFETCH(bool, dark);
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setThemeMode(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
    style->setDensityMode(compact ? WinUI3::DensityMode::Compact : WinUI3::DensityMode::Standard);
    QMainWindow host;
    host.setLayoutDirection(rtl ? Qt::RightToLeft : Qt::LeftToRight);
    DockTitleProbe dock(QStringLiteral("Inspector"), &host);
    dock.setWidget(new QLabel(QStringLiteral("Dock content")));
    if (vertical)
        dock.setFeatures(dock.features() | QDockWidget::DockWidgetVerticalTitleBar);
    host.addDockWidget(Qt::RightDockWidgetArea, &dock);
    host.resize(520, 260);
    host.show();
    QTRY_VERIFY(dock.isVisible());
    QStyleOptionDockWidget option;
    dock.fillStyleOption(&option);
    QCOMPARE(vertical ? option.rect.width() : option.rect.height(), compact ? 24 : 32);
    QWidget *title = dock.titleBarWidget();
    QVERIFY(title);
    QCOMPARE(title->metaObject(), &QWidget::staticMetaObject);
    auto *close = title->findChild<QToolButton *>(QStringLiteral("_winui_dock_close"));
    auto *floating = title->findChild<QToolButton *>(QStringLiteral("_winui_dock_float"));
    QVERIFY(close);
    QVERIFY(floating);
    QCOMPARE(close->metaObject(), &QToolButton::staticMetaObject);
    QCOMPARE(close->focusPolicy(), Qt::NoFocus);
    QCOMPARE(close->accessibleName(), QStringLiteral("Close"));
    QCOMPARE(floating->accessibleName(), QStringLiteral("Float"));
    const auto verifyGeometry = [&] {
        dock.fillStyleOption(&option);
        option.rect = title->rect();
        QCOMPARE(close->geometry(),
                 style->subElementRect(QStyle::SE_DockWidgetCloseButton, &option, &dock));
        QCOMPARE(floating->geometry(),
                 style->subElementRect(QStyle::SE_DockWidgetFloatButton, &option, &dock));
        QCOMPARE(close->size(), QSize(20, 20));
        QCOMPARE(close->iconSize(), QSize(12, 12));
        QCOMPARE(WinUI3::Style::controlRole(close), WinUI3::ControlRole::Subtle);
    };
    verifyGeometry();
    const auto t = WinUI3::Private::tokens(title->palette());
    const auto titlePixel = [&](const QColor &overlay) {
        QImage pixel(1, 1, QImage::Format_ARGB32_Premultiplied);
        pixel.fill(t.surface);
        QPainter painter(&pixel);
        painter.fillRect(pixel.rect(), t.layer);
        painter.fillRect(pixel.rect(), overlay);
        painter.end();
        return pixel.pixelColor(0, 0);
    };
    QEvent leave(QEvent::Leave);
    QCoreApplication::sendEvent(close, &leave);
    QCoreApplication::sendEvent(floating, &leave);
    setFrame(close, WinUI3::Private::hoverProperty, 0.0);
    setFrame(close, WinUI3::Private::pressProperty, 0.0);
    const QPoint sample =
            close->mapTo(title, QPoint(3, close->height() / 2)) * title->devicePixelRatioF();
    QCOMPARE(title->grab().toImage().pixelColor(sample), titlePixel(Qt::transparent));
    QEvent enter(QEvent::Enter);
    QCoreApplication::sendEvent(close, &enter);
    QTRY_VERIFY(frameReal(close, WinUI3::Private::hoverProperty) > 0.99);
    QCOMPARE(WinUI3::Style::controlRole(close), WinUI3::ControlRole::Subtle);
    QCOMPARE(title->grab().toImage().pixelColor(sample), titlePixel(t.subtleHover));
    QTest::mousePress(close, Qt::LeftButton);
    QVERIFY(close->isDown());
    QCOMPARE(close->size(), QSize(20, 20));
    QCOMPARE(title->grab().toImage().pixelColor(sample), titlePixel(t.subtlePressed));
    QTest::mouseRelease(close, Qt::LeftButton, Qt::NoModifier, QPoint(-2, -2));
    dock.setEnabled(false);
    QVERIFY(!close->isEnabled());
    QCOMPARE(WinUI3::Style::controlRole(close), WinUI3::ControlRole::Subtle);
    QCOMPARE(title->grab().toImage().pixelColor(sample), titlePixel(Qt::transparent));
    dock.setEnabled(true);
    dock.setWindowTitle(QStringLiteral("Updated Inspector"));
    QCOMPARE(dock.titleBarWidget(), title);
    const QPoint titlePoint =
            style->subElementRect(QStyle::SE_DockWidgetTitleBarText, &option, &dock).center();
    QTest::mouseDClick(title, Qt::LeftButton, Qt::NoModifier, titlePoint);
    QTRY_VERIFY(dock.isFloating());
    QTest::mouseDClick(title, Qt::LeftButton, Qt::NoModifier, titlePoint);
    QTRY_VERIFY(!dock.isFloating());
    QTest::mouseClick(floating, Qt::LeftButton);
    QTRY_VERIFY(dock.isFloating());
    QCOMPARE(dock.titleBarWidget(), title);
    QVERIFY(dock.windowFlags() & Qt::FramelessWindowHint);
    dock.resize(380, 230);
    QCoreApplication::processEvents();
    verifyGeometry();
    QTest::mouseClick(floating, Qt::LeftButton);
    QTRY_VERIFY(!dock.isFloating());
    dock.setFeatures(vertical ? QDockWidget::DockWidgetVerticalTitleBar
                              : QDockWidget::NoDockWidgetFeatures);
    QTRY_VERIFY(close->isHidden());
    QTRY_VERIFY(floating->isHidden());
    QCoreApplication::processEvents();
    QCOMPARE(vertical ? title->sizeHint().width() : title->sizeHint().height(), compact ? 24 : 32);
    QVERIFY(!title->geometry().intersects(dock.widget()->geometry()));
    for (auto feature : { QDockWidget::DockWidgetClosable, QDockWidget::DockWidgetFloatable }) {
        dock.setFeatures(feature
                         | (vertical ? QDockWidget::DockWidgetVerticalTitleBar
                                     : QDockWidget::NoDockWidgetFeatures));
        QCoreApplication::processEvents();
        dock.fillStyleOption(&option);
        option.rect = title->rect();
        QToolButton *visible = feature == QDockWidget::DockWidgetClosable ? close : floating;
        QToolButton *hidden = feature == QDockWidget::DockWidgetClosable ? floating : close;
        QVERIFY(!visible->isHidden());
        QVERIFY(hidden->isHidden());
        const auto visibleElement = feature == QDockWidget::DockWidgetClosable
                ? QStyle::SE_DockWidgetCloseButton
                : QStyle::SE_DockWidgetFloatButton;
        const auto hiddenElement = feature == QDockWidget::DockWidgetClosable
                ? QStyle::SE_DockWidgetFloatButton
                : QStyle::SE_DockWidgetCloseButton;
        QCOMPARE(visible->geometry(), style->subElementRect(visibleElement, &option, &dock));
        QVERIFY(style->subElementRect(hiddenElement, &option, &dock).isEmpty());
        QVERIFY(!visible->geometry().intersects(
                style->subElementRect(QStyle::SE_DockWidgetTitleBarText, &option, &dock)));
    }
    dock.setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    QTRY_VERIFY(!close->isHidden());
    host.setProperty("winuiDensity", compact ? "standard" : "compact");
    QTRY_COMPARE(title->height(), compact ? 32 : 24);
    dock.setLayoutDirection(rtl ? Qt::LeftToRight : Qt::RightToLeft);
    QCoreApplication::processEvents();
    verifyGeometry();
    QTest::mouseClick(close, Qt::LeftButton);
    QTRY_VERIFY(!dock.isVisible());
}

void WinUI3DockTitlesTest::dockTitleLargeFont_data()
{
    dockTitleComposition_data();
}

void WinUI3DockTitlesTest::dockTitleLargeFont()
{
    QFETCH(bool, compact);
    QFETCH(bool, vertical);
    QFETCH(bool, rtl);
    QFETCH(bool, dark);
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    style->setDensityMode(compact ? WinUI3::DensityMode::Compact : WinUI3::DensityMode::Standard);
    style->setThemeMode(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
    QMainWindow host;
    host.setLayoutDirection(rtl ? Qt::RightToLeft : Qt::LeftToRight);
    DockTitleProbe dock(QStringLiteral("Large title"), &host);
    dock.setWidget(new QLabel(QStringLiteral("Content")));
    if (vertical)
        dock.setFeatures(dock.features() | QDockWidget::DockWidgetVerticalTitleBar);
    host.addDockWidget(Qt::RightDockWidgetArea, &dock);
    host.resize(640, 360);
    host.show();
    QTRY_VERIFY(dock.isVisible());
    QWidget *title = dock.titleBarWidget();
    QVERIFY(title);
    QFont font = dock.font();
    font.setPixelSize(40);
    dock.setFont(font);
    font.setWeight(QFont::DemiBold);
    const int extent = qMax(20, QFontMetrics(font).height()) + (compact ? 4 : 12);
    QTRY_COMPARE(vertical ? title->width() : title->height(), extent);
    QCOMPARE(vertical ? title->sizeHint().width() : title->sizeHint().height(), extent);
    QTRY_VERIFY(!title->geometry().intersects(dock.widget()->geometry()));
    font.setPixelSize(12);
    dock.setFont(font);
    QTRY_COMPARE(vertical ? title->width() : title->height(), compact ? 24 : 32);
    QTRY_VERIFY(!title->geometry().intersects(dock.widget()->geometry()));
}

void WinUI3DockTitlesTest::dockTitleModifiedPlaceholder()
{
    QMainWindow host;
    DockTitleProbe dock(QStringLiteral("Document[*]"), &host);
    dock.setWidget(new QLabel(QStringLiteral("Content")));
    host.addDockWidget(Qt::RightDockWidgetArea, &dock);
    host.resize(520, 260);
    host.show();
    QTRY_VERIFY(dock.isVisible());
    QWidget *title = dock.titleBarWidget();
    QVERIFY(title);
    for (const QString &caption : { QStringLiteral("Document[*]"), QStringLiteral("Document[*][*]"),
                                    QStringLiteral("Document[*][*][*] - Editor[*]"),
                                    QStringLiteral("[*] title [*] suffix") }) {
        dock.setWindowTitle(caption);
        for (bool modified : { false, true, false }) {
            dock.setWindowModified(modified);
            QCoreApplication::processEvents();
            QStyleOptionDockWidget native;
            dock.fillStyleOption(&native);
            QStyleOptionDockWidget owned;
            WinUI3::Private::dockTitleStyleOption(title, &owned);
            QCOMPARE(owned.title, native.title);
        }
    }
}

void WinUI3DockTitlesTest::dockTitleOwnershipLifecycle()
{
    QMainWindow host;
    QDockWidget custom(QStringLiteral("Custom"), &host);
    auto *appTitle = new QWidget;
    custom.setTitleBarWidget(appTitle);
    host.addDockWidget(Qt::LeftDockWidgetArea, &custom);
    host.show();
    custom.ensurePolished();
    QCOMPARE(custom.titleBarWidget(), appTitle);
    QStyle *style = host.style();
    style->unpolish(&custom);
    QCOMPARE(custom.titleBarWidget(), appTitle);
    style->polish(&custom);
    QCOMPARE(custom.titleBarWidget(), appTitle);

    QDockWidget dock(QStringLiteral("Owned"), &host);
    host.addDockWidget(Qt::RightDockWidgetArea, &dock);
    dock.show();
    dock.ensurePolished();
    QPointer<QWidget> owned = dock.titleBarWidget();
    QVERIFY(owned);
    style->polish(&dock);
    QCOMPARE(dock.titleBarWidget(), owned.data());
    style->unpolish(&dock);
    QVERIFY(!dock.titleBarWidget());
    QVERIFY(owned); // Qt's style-switch allWidgets snapshot must stay valid.
    style->polish(&dock);
    QVERIFY(dock.titleBarWidget());
    QVERIFY(dock.titleBarWidget() != owned);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(!owned);
    QPointer<QWidget> cleared = dock.titleBarWidget();
    dock.setTitleBarWidget(nullptr);
    style->polish(&dock);
    QVERIFY(dock.titleBarWidget());
    QVERIFY(dock.titleBarWidget() != cleared);
    QVERIFY(cleared);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(!cleared);
    QPointer<QWidget> second = dock.titleBarWidget();
    auto *replacement = new QWidget;
    dock.setTitleBarWidget(replacement);
    style->polish(&dock);
    QCOMPARE(dock.titleBarWidget(), replacement);
    style->unpolish(&dock);
    QCOMPARE(dock.titleBarWidget(), replacement);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(!second);
}

void WinUI3DockTitlesTest::dockTitleStyleSwitch_data()
{
    dockTitleComposition_data();
}

void WinUI3DockTitlesTest::dockTitleStyleSwitch()
{
    QFETCH(bool, compact);
    QFETCH(bool, vertical);
    QFETCH(bool, rtl);
    QFETCH(bool, dark);
    const auto restoreStyle =
            qScopeGuard([] { qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light)); });
    QMainWindow host;
    host.setProperty("winuiDensity", compact ? "compact" : "standard");
    host.setLayoutDirection(rtl ? Qt::RightToLeft : Qt::LeftToRight);
    QDockWidget dock(QStringLiteral("Inspector"), &host);
    dock.setWidget(new QLabel(QStringLiteral("Dock content")));
    if (vertical)
        dock.setFeatures(dock.features() | QDockWidget::DockWidgetVerticalTitleBar);
    host.addDockWidget(Qt::RightDockWidgetArea, &dock);
    host.show();
    dock.setFloating(true);
    QTRY_VERIFY(dock.isVisible());
    QPointer<QWidget> oldTitle = dock.titleBarWidget();
    QVERIFY(oldTitle);
    QPointer<QToolButton> oldClose =
            oldTitle->findChild<QToolButton *>(QStringLiteral("_winui_dock_close"));
    QVERIFY(oldClose);
    qApp->setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    QVERIFY(!dock.titleBarWidget());
    QVERIFY(oldTitle);
    QVERIFY(oldClose);
    QVERIFY(oldTitle->isHidden());
    qApp->setStyle(new WinUI3::Style(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light));
    QWidget *newTitle = dock.titleBarWidget();
    QVERIFY(newTitle);
    QVERIFY(newTitle != oldTitle);
    QTRY_COMPARE(vertical ? newTitle->width() : newTitle->height(), compact ? 24 : 32);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(!oldTitle);
    QVERIFY(!oldClose);
    dock.setFloating(false);
    QCOMPARE(dock.titleBarWidget(), newTitle);
}

void WinUI3DockTitlesTest::dockTitleButtonBackdrop_data()
{
    dockTitleComposition_data();
}

void WinUI3DockTitlesTest::dockTitleButtonBackdrop()
{
    QFETCH(bool, compact);
    QFETCH(bool, vertical);
    QFETCH(bool, rtl);
    QFETCH(bool, dark);
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setThemeMode(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
    style->setDensityMode(compact ? WinUI3::DensityMode::Compact : WinUI3::DensityMode::Standard);
    QMainWindow host;
    host.setLayoutDirection(rtl ? Qt::RightToLeft : Qt::LeftToRight);
    QDockWidget dock(QStringLiteral("Inspector"), &host);
    dock.setWidget(new QLabel(QStringLiteral("Dock content")));
    if (vertical)
        dock.setFeatures(dock.features() | QDockWidget::DockWidgetVerticalTitleBar);
    host.addDockWidget(Qt::RightDockWidgetArea, &dock);
    host.resize(520, 260);
    host.show();
    QTRY_VERIFY(dock.titleBarWidget());
    QWidget *title = dock.titleBarWidget();
    QTRY_COMPARE(vertical ? title->width() : title->height(), compact ? 24 : 32);
    host.setProperty("_winui_backdrop", 1);
    host.setProperty("_winui_backdrop_effective", 2);
    QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(title));
    const auto t = WinUI3::Private::tokens(title->palette());
    const QColor surface = WinUI3::Private::cardRefillTone(title->palette(), true);
    const auto buttons = title->findChildren<QToolButton *>();
    QCOMPARE(buttons.size(), 2);
    for (auto *button : buttons) {
        QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(button));
        for (int state : { 0, 1, 2, 3 }) {
            QStyleOptionToolButton option;
            option.initFrom(button);
            option.rect = button->rect();
            option.state = state == 3 ? QStyle::State_None : QStyle::State_Enabled;
            if (state == 1)
                option.state |= QStyle::State_MouseOver;
            if (state == 2)
                option.state |= QStyle::State_Sunken;
            setFrame(button, WinUI3::Private::hoverProperty, state == 1 ? 1.0 : 0.0);
            setFrame(button, WinUI3::Private::pressProperty, state == 2 ? 1.0 : 0.0);
            QCOMPARE(button->size(), QSize(20, 20));
            QCOMPARE(WinUI3::Style::controlRole(button), WinUI3::ControlRole::Subtle);
            QImage expected(button->size(), QImage::Format_ARGB32_Premultiplied);
            expected.fill(surface);
            {
                QPainter painter(&expected);
                painter.fillRect(expected.rect(),
                                 state == 1           ? t.subtleHover
                                         : state == 2 ? t.subtlePressed
                                                      : QColor(Qt::transparent));
            }
            QImage frame(button->size(), QImage::Format_ARGB32_Premultiplied);
            frame.fill(Qt::red);
            {
                QPainter painter(&frame);
                style->drawPrimitive(QStyle::PE_PanelButtonTool, &option, &painter, button);
            }
            QCOMPARE(frame.pixelColor(3, 10), expected.pixelColor(3, 10));
            QVERIFY(frame.pixelColor(3, 10).alpha() >= surface.alpha());
            const QImage first = frame;
            {
                QPainter painter(&frame);
                style->drawPrimitive(QStyle::PE_PanelButtonTool, &option, &painter, button);
            }
            QCOMPARE(button->iconSize(), QSize(12, 12));
            QCOMPARE(frame, first); // No accumulation or command-shaped alpha holes.
        }
    }
}

QTEST_MAIN(WinUI3DockTitlesTest)
#include "tst_winui3docktitles.moc"
