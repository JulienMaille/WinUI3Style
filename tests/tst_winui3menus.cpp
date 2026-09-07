// SPDX-License-Identifier: LGPL-2.1-or-later
// Domain split of tst_winui3style.cpp (plan step 8): test-function names
// kept identical so the DPI reruns in tests/CMakeLists.txt keep working.
#include <winui3style/animatedstack.h>
#include <winui3style/navigationview.h>
#include <winui3style/settingscard.h>
#include <winui3style/toggleswitch.h>
#include <winui3style/winui3backdrop.h>
#include <winui3style/winui3style.h>
#include <winui3style/winui3icons.h>

#include "../src/winui3frameproperties_p.h"
#include "../src/winui3helpers_p.h"
#include "../src/winui3tokens_p.h"

#include <QLabel>
#include <QListWidget>
#include <QListView>
#include <QLineEdit>
#include <QAction>
#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAccessible>
#include <QCheckBox>
#include <QComboBox>
#include <QCommandLinkButton>
#include <QDialog>
#include <QDialogButtonBox>
#include <QEvent>
#include <QDockWidget>
#include <QFocusEvent>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QGroupBox>
#include <QHeaderView>
#include <QImage>
#include <QKeySequence>
#include <QKeyEvent>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QParallelAnimationGroup>
#include <QPointer>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollBar>
#include <QScrollArea>
#include <QSplitter>
#include <QSignalSpy>
#include <QSizeGrip>
#include <QSpinBox>
#include <QStatusBar>
#include <QStyleFactory>
#include <QStyleOptionGroupBox>
#include <QStyleOptionButton>
#include <QStyleOptionComboBox>
#include <QStyleOptionHeader>
#include <QStyleOptionMenuItem>
#include <QStyleOptionSlider>
#include <QStyleOptionSpinBox>
#include <QStyleOptionTab>
#include <QStyledItemDelegate>
#include <QStyleOptionToolButton>
#include <QSlider>
#include <QTabBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QToolButton>
#include <QToolBar>
#include <QTimer>
#include <QTest>
#include <QTextEdit>
#include <QTreeWidget>
#include <QVariantAnimation>
#include <QVBoxLayout>
#include <QWizard>
#include <QWizardPage>
#include <QtMath>
#include <QStandardItemModel>

#include <cmath>
#include <limits>


#include "winui3testhelpers.h"

class WinUI3MenusTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void menuSizingContract();
    void menuSubmenuChevronGeometry();
    void menuPaintTabParsing();
    void compactMenuBarTextFits();
    void menuBarOnlyActiveActionIsHighlighted();
    void groupBoxContract();
    void splitterHandleContract();
    void splitterGripPixelAlignment();
    void dockWidgetContract();
    void statusBarAndSizeGripContract();
};

void WinUI3MenusTest::initTestCase()
{
    qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light));
}

void WinUI3MenusTest::init()
{
    if (auto *style = qobject_cast<WinUI3::Style *>(qApp->style())) {
        style->setThemeMode(WinUI3::ThemeMode::Light);
        style->setAccentColor({});
        style->setDensityMode(WinUI3::DensityMode::Standard);
    }
}

void WinUI3MenusTest::cleanup()
{
    for (QWidget *widget : qApp->topLevelWidgets()) {
        if (widget->windowType() == Qt::Popup || widget->windowType() == Qt::ToolTip)
            widget->hide();
        else
            widget->close();
    }
    if (QWidget *focus = qApp->focusWidget())
        focus->clearFocus();
    qApp->processEvents();
}


void WinUI3MenusTest::menuSizingContract()
{
    QMenu menu;
    menu.setLayoutDirection(Qt::RightToLeft);
    QStyleOptionMenuItem option;
    option.initFrom(&menu);
    option.menuItemType = QStyleOptionMenuItem::Normal;
    option.font = menu.font();
    option.text = QStringLiteral("Open a recent project with a deliberately long name\tCtrl+Shift+O");
    const QFontMetrics metrics(option.font);
    const int expected = 42
        + metrics.horizontalAdvance(QStringLiteral("Open a recent project with a deliberately long name"))
        + 16 + 20 + metrics.horizontalAdvance(QStringLiteral("Ctrl+Shift+O"));
    const QSize result = menu.style()->sizeFromContents(QStyle::CT_MenuItem, &option,
                                                        QSize(), &menu);
    QVERIFY2(result.width() >= expected,
             qPrintable(QStringLiteral("%1 < %2").arg(result.width()).arg(expected)));
    QVERIFY(result.height() >= 36);

    const QString popupLabel = QStringLiteral("Open recent project");
    const int popupExpected = 42 + metrics.horizontalAdvance(popupLabel) + 16;
    auto *action = menu.addAction(popupLabel);
    action->setCheckable(true);
    PopupGeometryProbe probe;
    probe.popup = &menu;
    menu.installEventFilter(&probe);
    menu.popup(QPoint(80, 80));
    QTRY_VERIFY(menu.isVisible());
    const QRect firstGeometry = menu.geometry();
    QCOMPARE(firstGeometry, probe.geometryAtShow);
    // Live: the open slide converges within 167 ms (offscreen has no
    // slide and must freeze at Show).
    if (QGuiApplication::platformName() == QStringLiteral("offscreen")) {
        QTest::qWait(60);
        QCOMPARE(menu.geometry(), firstGeometry);
        QCOMPARE(probe.movesAfterShow, 0);
    } else {
        QTRY_VERIFY_WITH_TIMEOUT(menu.geometry().y() >= firstGeometry.y() - 1,
                                 1000);
    }
    QCOMPARE(probe.resizesAfterShow, 0);
    menu.hide();
    probe.reset();
    menu.popup(QPoint(80, 80));
    QTRY_VERIFY(menu.isVisible());
    QCOMPARE(menu.geometry(), firstGeometry);
    QCOMPARE(menu.geometry(), probe.geometryAtShow);
    if (QGuiApplication::platformName() == QStringLiteral("offscreen")) {
        QTest::qWait(60);
        QCOMPARE(menu.geometry(), firstGeometry);
        QCOMPARE(probe.movesAfterShow, 0);
    } else {
        QTRY_VERIFY_WITH_TIMEOUT(menu.geometry().y() >= firstGeometry.y() - 1,
                                 1000);
    }
    QCOMPARE(probe.resizesAfterShow, 0);
    const QRect actionRect = menu.actionGeometry(action);
    QVERIFY(actionRect.width() >= popupExpected);
    QTest::mouseMove(&menu, actionRect.center());
    QCOMPARE(menu.activeAction(), action);
    QTest::mouseClick(&menu, Qt::LeftButton, Qt::NoModifier,
                      actionRect.center());
    QVERIFY(action->isChecked());

    // Menu flyout items follow the combo popup rows in Compact mode.
    if (auto *style = qobject_cast<WinUI3::Style *>(qApp->style())) {
        style->setDensityMode(WinUI3::DensityMode::Compact);
        const QSize compactResult = menu.style()->sizeFromContents(
            QStyle::CT_MenuItem, &option, QSize(), &menu);
        style->setDensityMode(WinUI3::DensityMode::Standard);
        QCOMPARE(compactResult.height(), 32);
        const QSize backToStandard = menu.style()->sizeFromContents(
            QStyle::CT_MenuItem, &option, QSize(), &menu);
        QVERIFY(backToStandard.height() >= 36);
    }
}

void WinUI3MenusTest::menuSubmenuChevronGeometry()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);

    constexpr int chevronSlotSize = 16;
    constexpr int chevronRightPadding = 9;
    for (const Qt::LayoutDirection direction : {Qt::LeftToRight,
                                                Qt::RightToLeft}) {
        QStyleOptionMenuItem option;
        option.rect = QRect(0, 0, 240, 36);
        option.direction = direction;
        option.palette = qApp->palette();
        option.state = QStyle::State_Enabled;
        option.menuItemType = QStyleOptionMenuItem::SubMenu;
        option.font = qApp->font();
        option.fontMetrics = QFontMetrics(option.font);

        QImage image(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        {
            QPainter painter(&image);
            style->drawControl(QStyle::CE_MenuItem, &option, &painter);
        }

        QRect ink;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                if (image.pixelColor(x, y).alpha() > 20)
                    ink |= QRect(x, y, 1, 1);
            }
        }
        QVERIFY2(!ink.isEmpty(), "submenu chevron produced no pixels");

        const QRect logicalSlot(
            option.rect.right() - chevronRightPadding - chevronSlotSize + 1,
            option.rect.center().y() - chevronSlotSize / 2,
            chevronSlotSize, chevronSlotSize);
        const QRect slot = QStyle::visualRect(direction, option.rect,
                                              logicalSlot);
        QVERIFY(slot.contains(ink.topLeft()));
        QVERIFY(slot.contains(ink.bottomRight()));
        // The WinUI template uses FontSize=12 in a 16px Viewbox. On the
        // reference font that produces an approximately 8px-tall visible
        // chevron, rather than the old 16px icon-engine paint (12px tall).
        QVERIFY(ink.width() <= 8);
        // Some font engines expose a one-pixel antialiasing fringe around the
        // 8px body. The old icon path was 12px tall, so 9 remains a strict
        // regression ceiling while keeping the DPI/font test portable.
        QVERIFY(ink.height() <= 9);
        QVERIFY(ink.width() >= 4);
        QVERIFY(ink.height() >= 6);
        QCOMPARE(slot.size(), QSize(chevronSlotSize, chevronSlotSize));
        QVERIFY(qAbs(slot.center().y() - option.rect.center().y()) <= 1);
    }
}

void WinUI3MenusTest::menuPaintTabParsing()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);

    auto render = [style](const QString &text) {
        QStyleOptionMenuItem option;
        option.rect = QRect(0, 0, 320, 36);
        option.direction = Qt::LeftToRight;
        option.palette = qApp->palette();
        option.state = QStyle::State_Enabled;
        option.menuItemType = QStyleOptionMenuItem::Normal;
        option.font = qApp->font();
        option.fontMetrics = QFontMetrics(option.font);
        option.text = text;

        QImage result(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
        result.fill(Qt::transparent);
        QPainter painter(&result);
        style->drawControl(QStyle::CE_MenuItem, &option, &painter);
        return result;
    };

    // QString::split() historically rendered only fields 0 and 1. Keep a
    // third tabbed field from changing pixels while replacing that allocation
    // heavy parsing in the paint path.
    const QImage twoFields = render(QStringLiteral("Open\tCtrl+O"));
    const QImage extraField = render(QStringLiteral("Open\tCtrl+O\tignored"));
    QVERIFY(twoFields == extraField);
}

void WinUI3MenusTest::compactMenuBarTextFits()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    const WinUI3::DensityMode previous = style->densityMode();
    style->setDensityMode(WinUI3::DensityMode::Compact);

    QMenuBar bar;
    QAction *action = bar.addAction(QStringLiteral("WWWWWW"));
    bar.resize(bar.sizeHint());
    bar.show();
    QVERIFY(QTest::qWaitForWindowExposed(&bar));
    const QRect actionRect = bar.actionGeometry(action);
    const int required = bar.fontMetrics().horizontalAdvance(action->text())
        + 2 * 8;
    QVERIFY2(actionRect.width() >= required,
             qPrintable(QStringLiteral("action=%1 required=%2")
                            .arg(actionRect.width()).arg(required)));

    const QImage rendered = bar.grab().toImage();
    const QColor background = bar.palette().color(QPalette::Window);
    int rightmostInk = -1;
    for (int y = actionRect.top(); y <= actionRect.bottom(); ++y) {
        for (int x = actionRect.left(); x <= actionRect.right(); ++x) {
            if (colorDistance(rendered.pixelColor(x, y), background) > 24)
                rightmostInk = qMax(rightmostInk, x);
        }
    }
    // With the stale 10 px renderer inset, the final W is clipped and its
    // right edge lands at least two pixels earlier than this compact contract.
    QVERIFY2(rightmostInk >= actionRect.right() - 8,
             qPrintable(QStringLiteral("rightmost ink=%1 action right=%2")
                            .arg(rightmostInk).arg(actionRect.right())));

    style->setDensityMode(previous);
}

void WinUI3MenusTest::menuBarOnlyActiveActionIsHighlighted()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);

    QMenuBar bar;
    bar.setProperty(WinUI3::Style::SurfaceProperty, QStringLiteral("layer"));
    QAction *file = bar.addAction(QStringLiteral("File"));
    QAction *view = bar.addAction(QStringLiteral("View"));
    bar.resize(180, 32);
    bar.show();
    (void)QTest::qWaitForWindowExposed(&bar);
    setFrame(&bar, "_winui_hover_progress", 1.0);

    for (const Qt::LayoutDirection direction : {Qt::LeftToRight,
                                                Qt::RightToLeft}) {
        bar.setLayoutDirection(direction);
        const QRect fileRect = bar.actionGeometry(file);
        const QRect viewRect = bar.actionGeometry(view);
        QVERIFY(!fileRect.intersects(viewRect));

        QImage image(bar.size(), QImage::Format_ARGB32_Premultiplied);
        const QColor surface = bar.palette().color(QPalette::Window);
        image.fill(Qt::black);
        QStyleOption emptyArea;
        emptyArea.initFrom(&bar);
        emptyArea.rect = image.rect();
        {
            QPainter painter(&image);
            style->drawControl(QStyle::CE_MenuBarEmptyArea, &emptyArea,
                               &painter, &bar);
        }
        QCOMPARE(image.pixelColor(image.width() - 2, image.height() / 2), surface);
        auto drawItem = [&](QAction *action, const QRect &rect,
                            QStyle::State state) {
            QStyleOptionMenuItem option;
            option.initFrom(&bar);
            option.rect = rect;
            option.state = state;
            option.text = action->text();
            option.font = bar.font();
            option.fontMetrics = QFontMetrics(option.font);
            option.menuItemType = QStyleOptionMenuItem::Normal;
            QPainter painter(&image);
            style->drawControl(QStyle::CE_MenuBarItem, &option,
                               &painter, &bar);
        };
        drawItem(file, fileRect, QStyle::State_Enabled | QStyle::State_Selected);
        drawItem(view, viewRect, QStyle::State_Enabled);

        const QPoint fileSample(fileRect.left() + 4, fileRect.top() + 4);
        const QPoint viewSample(viewRect.left() + 4, viewRect.top() + 4);
        QVERIFY(image.pixelColor(fileSample) != surface);
        QCOMPARE(image.pixelColor(viewSample), surface);
        drawItem(file, fileRect, QStyle::State_Enabled);
        QCOMPARE(image.pixelColor(fileSample), surface);
    }

    QListWidget navigation;
    navigation.setProperty(WinUI3::Style::NavigationViewProperty, true);
    navigation.setProperty(WinUI3::Style::SurfaceProperty,
                           QStringLiteral("layer"));
    navigation.addItem(QStringLiteral("Controls"));
    navigation.resize(220, 120);
    navigation.show();
    QTRY_VERIFY(navigation.isVisible());
    const QColor navigationWindow = navigation.palette().color(QPalette::Window);
    QCOMPARE(navigation.palette().color(QPalette::Base), navigationWindow);
    QCOMPARE(navigationWindow.alpha(), 255);
}

void WinUI3MenusTest::groupBoxContract()
{
    QGroupBox group(QStringLiteral("Enable diagnostics"));
    group.setCheckable(true);
    group.setChecked(false);
    group.resize(320, 120);
    group.show();

    QStyleOptionGroupBox option;
    option.initFrom(&group);
    option.text = group.title();
    option.subControls = QStyle::SC_GroupBoxFrame | QStyle::SC_GroupBoxLabel
        | QStyle::SC_GroupBoxCheckBox | QStyle::SC_GroupBoxContents;
    const QRect indicator = group.style()->subControlRect(QStyle::CC_GroupBox, &option,
                                                          QStyle::SC_GroupBoxCheckBox,
                                                          &group);
    const QRect label = group.style()->subControlRect(QStyle::CC_GroupBox, &option,
                                                      QStyle::SC_GroupBoxLabel,
                                                      &group);
    const QRect contents = group.style()->subControlRect(QStyle::CC_GroupBox, &option,
                                                         QStyle::SC_GroupBoxContents,
                                                         &group);
    QCOMPARE(indicator.size(), QSize(20, 20));
    QVERIFY(contents.top() > indicator.bottom());

    const QPoint gap((indicator.right() + label.left()) / 2,
                     indicator.center().y());
    QVERIFY(!indicator.contains(gap));
    QVERIFY(!label.contains(gap));
    QCOMPARE(group.style()->hitTestComplexControl(QStyle::CC_GroupBox, &option,
                                                   gap, &group),
             QStyle::SC_GroupBoxCheckBox);
    QTest::mouseClick(&group, Qt::LeftButton, Qt::NoModifier, gap);
    QVERIFY(group.isChecked());
    QTest::mouseClick(&group, Qt::LeftButton, Qt::NoModifier, gap);
    QVERIFY(!group.isChecked());

    group.setChecked(true);
    QTest::qWait(55);
    const qreal midway = frameReal(&group, "_winui_check_progress");
    QVERIFY(midway > 0.0 && midway < 1.0);
    QTRY_VERIFY(frameReal(&group, "_winui_check_progress") > 0.99);

    QTest::mouseClick(&group, Qt::LeftButton, Qt::NoModifier,
                      indicator.center());
    QVERIFY(!group.isChecked());
    QCOMPARE(frameReal(&group, "_winui_check_progress"), 0.0);

    group.setLayoutDirection(Qt::RightToLeft);
    option.initFrom(&group);
    option.text = group.title();
    option.subControls = QStyle::SC_GroupBoxFrame | QStyle::SC_GroupBoxLabel
        | QStyle::SC_GroupBoxCheckBox | QStyle::SC_GroupBoxContents;
    const QRect rtlIndicator = group.style()->subControlRect(
        QStyle::CC_GroupBox, &option, QStyle::SC_GroupBoxCheckBox, &group);
    const QRect rtlLabel = group.style()->subControlRect(
        QStyle::CC_GroupBox, &option, QStyle::SC_GroupBoxLabel, &group);
    const QPoint rtlGap((rtlLabel.right() + rtlIndicator.left()) / 2,
                        rtlIndicator.center().y());
    QCOMPARE(group.style()->hitTestComplexControl(QStyle::CC_GroupBox, &option,
                                                   rtlGap, &group),
             QStyle::SC_GroupBoxCheckBox);
}

void WinUI3MenusTest::splitterHandleContract()
{
    ExposedSplitter splitter(Qt::Horizontal);
    splitter.addWidget(new QLabel(QStringLiteral("Left")));
    splitter.addWidget(new QLabel(QStringLiteral("Right")));
    splitter.resize(420, 120);
    splitter.show();
    auto *handle = splitter.handle(1);
    QVERIFY(handle);
    QCOMPARE(handle->width(), 6);

    // The offscreen platform may report the synthetic cursor over a newly
    // shown handle. Establish a known rest state before testing the explicit
    // Enter transition; otherwise the controller can legitimately ignore a
    // duplicate Enter and leave the frame registry at its initial value.
    QEvent leave(QEvent::Leave);
    QCoreApplication::sendEvent(handle, &leave);
    QTRY_VERIFY(frameReal(handle, "_winui_hover_progress") < 0.01);
    QEvent enter(QEvent::Enter);
    QCoreApplication::sendEvent(handle, &enter);
    QTRY_VERIFY(frameReal(handle, "_winui_hover_progress") > 0.0);
    QTRY_VERIFY(frameReal(handle, "_winui_hover_progress") > 0.99);

    const QList<int> beforeDrag = splitter.sizes();
    const QPoint handleCenter = handle->rect().center();
    QTest::mousePress(handle, Qt::LeftButton, Qt::NoModifier, handleCenter);
    QTest::mouseMove(handle, handleCenter + QPoint(60, 0), 20);
    QTest::mouseRelease(handle, Qt::LeftButton, Qt::NoModifier,
                        handleCenter + QPoint(60, 0));
    QVERIFY(splitter.sizes().at(0) > beforeDrag.at(0));
    splitter.moveSplitter(-1000, 1);
    QVERIFY(splitter.sizes().at(0) >= 0);
    splitter.moveSplitter(splitter.width() + 1000, 1);
    QVERIFY(splitter.sizes().at(1) >= 0);

    ExposedSplitter vertical(Qt::Vertical);
    vertical.addWidget(new QLabel(QStringLiteral("Top")));
    vertical.addWidget(new QLabel(QStringLiteral("Bottom")));
    vertical.resize(180, 300);
    vertical.show();
    auto *verticalHandle = vertical.handle(1);
    QVERIFY(verticalHandle);
    QCOMPARE(verticalHandle->height(), 6);
    const QList<int> beforeVerticalDrag = vertical.sizes();
    const QPoint verticalCenter = verticalHandle->rect().center();
    QTest::mousePress(verticalHandle, Qt::LeftButton, Qt::NoModifier, verticalCenter);
    QTest::mouseMove(verticalHandle, verticalCenter + QPoint(0, 45), 20);
    QTest::mouseRelease(verticalHandle, Qt::LeftButton, Qt::NoModifier,
                        verticalCenter + QPoint(0, 45));
    QVERIFY(vertical.sizes().at(0) > beforeVerticalDrag.at(0));
    vertical.moveSplitter(-1000, 1);
    QVERIFY(vertical.sizes().at(0) >= 0);
    vertical.moveSplitter(vertical.height() + 1000, 1);
    QVERIFY(vertical.sizes().at(1) >= 0);
}

void WinUI3MenusTest::splitterGripPixelAlignment()
{
    QSplitter splitter(Qt::Horizontal);
    splitter.addWidget(new QLabel(QStringLiteral("Left")));
    splitter.addWidget(new QLabel(QStringLiteral("Right")));

    const auto gripCenter = [&splitter](const QRect &rect, bool horizontal,
                                        qreal dpr) {
        constexpr int logicalWidth = 180;
        constexpr int logicalHeight = 140;
        QImage image(qCeil(logicalWidth * dpr), qCeil(logicalHeight * dpr),
                     QImage::Format_ARGB32_Premultiplied);
        image.setDevicePixelRatio(dpr);
        image.fill(Qt::transparent);
        QStyleOption option;
        option.initFrom(&splitter);
        option.rect = rect;
        option.state = QStyle::State_Enabled;
        if (horizontal)
            option.state |= QStyle::State_Horizontal;
        {
            QPainter painter(&image);
            splitter.style()->drawControl(QStyle::CE_Splitter, &option,
                                          &painter, splitter.handle(1));
        }
        qreal weighted = 0.0;
        qreal weight = 0.0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                const qreal alpha = qAlpha(image.pixel(x, y));
                weighted += alpha * (horizontal ? x + 0.5 : y + 0.5);
                weight += alpha;
            }
        }
        const qreal actual = weight > 0.0 ? weighted / weight : -1.0;
        const qreal logicalCenter = horizontal
            ? QRectF(rect).center().x() : QRectF(rect).center().y();
        const qreal expected = qRound(logicalCenter * dpr - 0.5) + 0.5;
        return qMakePair(actual, expected);
    };

    for (const qreal dpr : {1.0, 1.25, 1.5, 2.0}) {
        for (const bool horizontal : {true, false}) {
            const QRect even = horizontal ? QRect(20, 20, 6, 100)
                                          : QRect(20, 20, 100, 6);
            const QRect odd = horizontal ? QRect(21, 21, 7, 99)
                                         : QRect(21, 21, 99, 7);
            const auto evenCenter = gripCenter(even, horizontal, dpr);
            const auto oddCenter = gripCenter(odd, horizontal, dpr);
            QVERIFY2(evenCenter.first >= 0.0,
                     qPrintable(QStringLiteral("DPR %1 %2 even empty")
                                    .arg(dpr).arg(horizontal ? "H" : "V")));
            QVERIFY2(oddCenter.first >= 0.0,
                     qPrintable(QStringLiteral("DPR %1 %2 odd empty")
                                    .arg(dpr).arg(horizontal ? "H" : "V")));
            QVERIFY2(qAbs(evenCenter.first - evenCenter.second) < 0.75,
                     qPrintable(QStringLiteral("DPR %1 %2 even=%3 expected=%4")
                                    .arg(dpr).arg(horizontal ? "H" : "V")
                                    .arg(evenCenter.first).arg(evenCenter.second)));
            QVERIFY2(qAbs(oddCenter.first - oddCenter.second) < 0.75,
                     qPrintable(QStringLiteral("DPR %1 %2 odd=%3 expected=%4")
                                    .arg(dpr).arg(horizontal ? "H" : "V")
                                    .arg(oddCenter.first).arg(oddCenter.second)));
        }
    }
}

void WinUI3MenusTest::dockWidgetContract()
{
    QMainWindow host;
    auto *dock = new QDockWidget(QStringLiteral("Inspector"), &host);
    dock->setWidget(new QLabel(QStringLiteral("Dock content")));
    host.addDockWidget(Qt::RightDockWidgetArea, dock);
    host.resize(520, 260);
    host.show();

    QCOMPARE(host.style()->pixelMetric(QStyle::PM_DockWidgetSeparatorExtent,
                                       nullptr, dock), 6);
    QCOMPARE(host.style()->pixelMetric(QStyle::PM_DockWidgetFrameWidth,
                                       nullptr, dock), 1);
    QCOMPARE(host.style()->pixelMetric(QStyle::PM_DockWidgetTitleMargin,
                                       nullptr, dock), 8);
    const QImage image = host.grab().toImage();
    QVERIFY(!image.isNull());
    QCOMPARE(image.size(), host.size());
}

void WinUI3MenusTest::statusBarAndSizeGripContract()
{
    QMainWindow window;
    window.resize(420, 240);
    QStatusBar *bar = window.statusBar();
    bar->setSizeGripEnabled(true);
    bar->showMessage(QStringLiteral("Ready"));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto *grip = bar->findChild<QSizeGrip *>();
    QVERIFY(grip);
    QVERIFY(grip->isVisible());
    QCOMPARE(window.style()->pixelMetric(QStyle::PM_SizeGripSize,
                                         nullptr, grip), 16);
    const QImage barImage = bar->grab().toImage();
    QVERIFY(barImage.pixelColor(barImage.width() / 2, 0)
            != barImage.pixelColor(barImage.width() / 2,
                                   qMin(barImage.height() - 1, 6)));
    const QImage gripImage = grip->grab().toImage();
    const QColor gripBackground = grip->palette().color(QPalette::Window);
    bool hasGripInk = false;
    for (int y = 0; y < gripImage.height() && !hasGripInk; ++y)
        for (int x = 0; x < gripImage.width(); ++x)
            if (colorDistance(gripImage.pixelColor(x, y), gripBackground) > 24) {
                hasGripInk = true;
                break;
            }
    QVERIFY(hasGripInk);
}

QTEST_MAIN(WinUI3MenusTest)
#include "tst_winui3menus.moc"
