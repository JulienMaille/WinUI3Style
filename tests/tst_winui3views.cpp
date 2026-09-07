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

class WinUI3ViewsTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void sliderGeometryContract();
    void sliderStateMotion();
    void sliderDragInteraction();
    void sliderValueToolTipAndFocus();
    void scrollBarContract();
    void scrollBarHorizontalAndReentry();
    void scrollAreaScrollBarIntegration();
    void tabViewContract();
    void listViewContract();
    void itemViewGutterContract();
    void treeViewContract();
    void treeSelectionMarkerLeadingEdge();
    void tableHeaderContract();
    void tableSortIndicatorGeometryContract();
    void tableEditingPaintContract();
    void tableLiveEditorSuppressesDisplay();
    void richEditBoxContract();
    void sliderExtremeRangeTicks();
    void itemViewMouseFocusReset();
    void dpiGeometry();
    void dpiHitTestContracts();
};

void WinUI3ViewsTest::initTestCase()
{
    qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light));
}

void WinUI3ViewsTest::init()
{
    if (auto *style = qobject_cast<WinUI3::Style *>(qApp->style())) {
        style->setThemeMode(WinUI3::ThemeMode::Light);
        style->setAccentColor({});
    }
}

void WinUI3ViewsTest::cleanup()
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


void WinUI3ViewsTest::sliderGeometryContract()
{
    const auto verifyEndPoints = [](QSlider &slider) {
        QStyleOptionSlider option;
        slider.setValue(slider.minimum());
        option.initFrom(&slider);
        option.orientation = slider.orientation();
        option.minimum = slider.minimum();
        option.maximum = slider.maximum();
        option.sliderPosition = slider.sliderPosition();
        option.sliderValue = slider.value();
        option.upsideDown = slider.orientation() == Qt::Horizontal
            ? (slider.invertedAppearance()
               != (slider.layoutDirection() == Qt::RightToLeft))
            : !slider.invertedAppearance();
        const QRect groove = slider.style()->subControlRect(
            QStyle::CC_Slider, &option, QStyle::SC_SliderGroove, &slider);
        const QRect minimumHandle = slider.style()->subControlRect(
            QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, &slider);
        QCOMPARE(minimumHandle.size(), QSize(18, 18));
        QCOMPARE(slider.orientation() == Qt::Horizontal ? groove.height()
                                                       : groove.width(), 4);
        const int minimumCenter = slider.orientation() == Qt::Horizontal
            ? minimumHandle.center().x() : minimumHandle.center().y();
        const int minimumEnd = slider.orientation() == Qt::Horizontal
            ? (option.upsideDown ? groove.right() : groove.left())
            : (option.upsideDown ? groove.bottom() : groove.top());
        QCOMPARE(minimumCenter, minimumEnd);

        slider.setValue(slider.maximum());
        option.sliderPosition = slider.sliderPosition();
        option.sliderValue = slider.value();
        const QRect maximumHandle = slider.style()->subControlRect(
            QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, &slider);
        const int maximumCenter = slider.orientation() == Qt::Horizontal
            ? maximumHandle.center().x() : maximumHandle.center().y();
        const int maximumEnd = slider.orientation() == Qt::Horizontal
            ? (option.upsideDown ? groove.left() : groove.right())
            : (option.upsideDown ? groove.top() : groove.bottom());
        QCOMPARE(maximumCenter, maximumEnd);
    };

    QSlider horizontal(Qt::Horizontal);
    horizontal.setRange(0, 100);
    horizontal.resize(320, 32);
    verifyEndPoints(horizontal);
    horizontal.setLayoutDirection(Qt::RightToLeft);
    verifyEndPoints(horizontal);
    horizontal.setInvertedAppearance(true);
    verifyEndPoints(horizontal);

    QSlider vertical(Qt::Vertical);
    vertical.setRange(0, 100);
    vertical.resize(32, 320);
    verifyEndPoints(vertical);
    vertical.setInvertedAppearance(true);
    verifyEndPoints(vertical);
}

void WinUI3ViewsTest::sliderStateMotion()
{
    QSlider slider(Qt::Horizontal);
    slider.setRange(0, 100);
    slider.setValue(40);
    slider.resize(320, 32);
    slider.show();

    QEvent leave(QEvent::Leave);
    QCoreApplication::sendEvent(&slider, &leave);
    QTRY_VERIFY(frameReal(&slider, "_winui_hover_progress") < 0.01);
    QEvent enter(QEvent::Enter);
    QCoreApplication::sendEvent(&slider, &enter);
    QTest::qWait(100);
    const qreal hoverMidway = frameReal(&slider, "_winui_hover_progress");
    QVERIFY(hoverMidway > 0.0 && hoverMidway < 1.0);
    QTRY_VERIFY(frameReal(&slider, "_winui_hover_progress") > 0.99);

    QStyleOptionSlider option;
    option.initFrom(&slider);
    option.orientation = slider.orientation();
    option.minimum = slider.minimum();
    option.maximum = slider.maximum();
    option.sliderPosition = slider.sliderPosition();
    const QRect handle = slider.style()->subControlRect(
        QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, &slider);
    QTest::mousePress(&slider, Qt::LeftButton, Qt::NoModifier, handle.center());
    QTest::qWait(100);
    const qreal pressMidway = frameReal(&slider, "_winui_press_progress");
    QVERIFY(pressMidway > 0.0 && pressMidway < 1.0);
    QTest::mouseRelease(&slider, Qt::LeftButton, Qt::NoModifier, handle.center());
    QTRY_VERIFY(frameReal(&slider, "_winui_press_progress") < 0.01);
}

void WinUI3ViewsTest::sliderDragInteraction()
{
    QSlider slider(Qt::Horizontal);
    slider.setRange(0, 100);
    slider.setValue(20);
    slider.resize(320, 40);
    slider.show();

    QStyleOptionSlider option;
    option.initFrom(&slider);
    option.orientation = slider.orientation();
    option.minimum = slider.minimum();
    option.maximum = slider.maximum();
    option.sliderPosition = slider.sliderPosition();
    option.sliderValue = slider.value();
    const QRect handle = slider.style()->subControlRect(QStyle::CC_Slider, &option,
                                                        QStyle::SC_SliderHandle,
                                                        &slider);
    QTest::mousePress(&slider, Qt::LeftButton, Qt::NoModifier, handle.center());
    QMouseEvent move(QEvent::MouseMove, QPointF(handle.center() + QPoint(120, 0)),
                     Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&slider, &move);
    QTest::mouseRelease(&slider, Qt::LeftButton, Qt::NoModifier,
                        handle.center() + QPoint(120, 0));
    QVERIFY(slider.value() > 20);
}

void WinUI3ViewsTest::sliderValueToolTipAndFocus()
{
    QSlider slider(Qt::Horizontal);
    slider.setRange(0, 100);
    slider.setValue(42);
    slider.resize(320, 40);
    slider.move(300, 300);
    slider.show();

    QStyleOptionSlider option;
    option.initFrom(&slider);
    option.orientation = slider.orientation();
    option.minimum = slider.minimum();
    option.maximum = slider.maximum();
    option.sliderPosition = slider.sliderPosition();
    option.sliderValue = slider.value();
    const QRect handle = slider.style()->subControlRect(
        QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, &slider);

    QTest::mousePress(&slider, Qt::LeftButton, Qt::NoModifier, handle.center());
    QTRY_VERIFY(frameBool(&slider, "_winui_slider_tooltip_visible"));
    QCOMPARE(frameValue(&slider, "_winui_slider_tooltip_value").toString(),
             QStringLiteral("42"));
    if (QGuiApplication::platformName() != QStringLiteral("offscreen")) {
        QTRY_VERIFY(slider.findChild<QWidget *>(
            QStringLiteral("_winui_slider_value_tip"))->isVisible());
    }
    QMouseEvent move(QEvent::MouseMove, QPointF(handle.center() + QPoint(90, 0)),
                     Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&slider, &move);
    QTRY_VERIFY(slider.value() > 42);
    QTRY_COMPARE(frameValue(&slider, "_winui_slider_tooltip_value").toString(),
                 QString::number(slider.value()));
    QTest::mouseRelease(&slider, Qt::LeftButton, Qt::NoModifier,
                        handle.center() + QPoint(90, 0));
    QTRY_VERIFY(!frameBool(&slider, "_winui_slider_tooltip_visible"));
    if (auto *tip = slider.findChild<QWidget *>(
            QStringLiteral("_winui_slider_value_tip"))) {
        QTRY_VERIFY(!tip->isVisible());
    }

    const auto render = [&slider](QStyleOptionSlider renderOption) {
        QImage image(renderOption.rect.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        slider.style()->drawComplexControl(QStyle::CC_Slider, &renderOption,
                                            &painter, &slider);
        return image;
    };
    option.initFrom(&slider);
    option.orientation = slider.orientation();
    option.minimum = slider.minimum();
    option.maximum = slider.maximum();
    option.sliderPosition = slider.sliderPosition();
    option.sliderValue = slider.value();
    option.rect = slider.rect();

    QFocusEvent mouseFocus(QEvent::FocusIn, Qt::MouseFocusReason);
    QCoreApplication::sendEvent(&slider, &mouseFocus);
    option.state |= QStyle::State_HasFocus;
    const QImage mouseFocused = render(option);
    QVERIFY(!frameBool(&slider, "_winui_focus_visible"));

    QFocusEvent tabFocus(QEvent::FocusIn, Qt::TabFocusReason);
    QCoreApplication::sendEvent(&slider, &tabFocus);
    QVERIFY(frameBool(&slider, "_winui_focus_visible"));
    const QImage keyboardFocused = render(option);
    QVERIFY(mouseFocused != keyboardFocused);

    slider.clearFocus();
    slider.setEnabled(false);
    const int disabledValue = slider.value();
    QTest::mouseClick(&slider, Qt::LeftButton, Qt::NoModifier,
                      slider.rect().center() + QPoint(80, 0));
    QCOMPARE(slider.value(), disabledValue);
    option.initFrom(&slider);
    option.orientation = slider.orientation();
    option.minimum = slider.minimum();
    option.maximum = slider.maximum();
    option.sliderPosition = slider.sliderPosition();
    option.sliderValue = slider.value();
    option.rect = slider.rect();
    const QImage disabled = render(option);
    QVERIFY(disabled != mouseFocused);
}

void WinUI3ViewsTest::scrollBarContract()
{
    DisableAnimationsGuard animations;
    QScrollBar bar(Qt::Vertical);
    bar.setRange(0, 100);
    bar.setPageStep(20);
    bar.setValue(30);
    bar.resize(12, 300);
    bar.show();

    QStyleOptionSlider option;
    option.initFrom(&bar);
    option.orientation = bar.orientation();
    option.minimum = bar.minimum();
    option.maximum = bar.maximum();
    option.sliderPosition = bar.sliderPosition();
    option.sliderValue = bar.value();
    option.pageStep = bar.pageStep();
    option.upsideDown = false;
    const QRect decrease = bar.style()->subControlRect(
        QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarSubLine, &bar);
    const QRect increase = bar.style()->subControlRect(
        QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarAddLine, &bar);
    const QRect thumb = bar.style()->subControlRect(
        QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarSlider, &bar);
    QCOMPARE(decrease.height(), 12);
    QCOMPARE(increase.height(), 12);
    QCOMPARE(thumb.width(), 12);
    QVERIFY(thumb.height() >= 30);

    const auto colorDistance = [](const QColor &a, const QColor &b) {
        return qAbs(a.red() - b.red()) + qAbs(a.green() - b.green())
            + qAbs(a.blue() - b.blue());
    };
    setFrame(&bar, "_winui_hover_progress", 0.0);
    QImage collapsedGeometry = bar.grab().toImage();
    const QColor background = bar.palette().color(QPalette::Window);
    const int sampleY = thumb.center().y();
    QVERIFY(colorDistance(collapsedGeometry.pixelColor(2, sampleY), background)
            < 5);
    QVERIFY(colorDistance(collapsedGeometry.pixelColor(4, sampleY), background)
            > 10);
    QVERIFY(colorDistance(collapsedGeometry.pixelColor(11, sampleY), background)
            > 10);

    QEvent enter(QEvent::Enter);
    QCoreApplication::sendEvent(&bar, &enter);
    QCOMPARE(frameReal(&bar, "_winui_hover_progress"), 1.0);

    const int beforeArrow = bar.value();
    QTest::mouseClick(&bar, Qt::LeftButton, Qt::NoModifier, increase.center());
    QVERIFY(bar.value() > beforeArrow);

    option.sliderPosition = bar.sliderPosition();
    option.sliderValue = bar.value();
    const QRect movedThumb = bar.style()->subControlRect(
        QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarSlider, &bar);
    const int beforeDrag = bar.value();
    QTest::mousePress(&bar, Qt::LeftButton, Qt::NoModifier, movedThumb.center());
    QMouseEvent move(QEvent::MouseMove,
                     QPointF(movedThumb.center() + QPoint(0, 45)),
                     Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&bar, &move);
    QTest::mouseRelease(&bar, Qt::LeftButton, Qt::NoModifier,
                        movedThumb.center() + QPoint(0, 45));
    QVERIFY(bar.value() > beforeDrag);

    QEvent leave(QEvent::Leave);
    QCoreApplication::sendEvent(&bar, &leave);
    QCOMPARE(frameReal(&bar, "_winui_hover_progress"), 0.0);
    const QImage collapsed = bar.grab().toImage();
    QCOMPARE(collapsed.pixelColor(0, collapsed.height() / 2),
             bar.palette().color(QPalette::Window));
}

void WinUI3ViewsTest::scrollBarHorizontalAndReentry()
{
    DisableAnimationsGuard animations;
    QScrollBar bar(Qt::Horizontal);
    bar.setRange(0, 100);
    bar.setPageStep(20);
    bar.setValue(30);
    bar.resize(300, 12);
    bar.show();

    QStyleOptionSlider option;
    option.initFrom(&bar);
    option.orientation = bar.orientation();
    option.minimum = bar.minimum();
    option.maximum = bar.maximum();
    option.sliderPosition = bar.sliderPosition();
    option.sliderValue = bar.value();
    option.pageStep = bar.pageStep();
    option.upsideDown = false;
    const QRect decrease = bar.style()->subControlRect(
        QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarSubLine, &bar);
    const QRect increase = bar.style()->subControlRect(
        QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarAddLine, &bar);
    const QRect thumb = bar.style()->subControlRect(
        QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarSlider, &bar);
    QCOMPARE(decrease.width(), 12);
    QCOMPARE(increase.width(), 12);
    QCOMPARE(thumb.height(), 12);
    QVERIFY(thumb.width() >= 30);
    QCOMPARE(bar.style()->pixelMetric(QStyle::PM_ScrollBarSliderMin,
                                       &option, &bar), 30);

    const auto colorDistance = [](const QColor &a, const QColor &b) {
        return qAbs(a.red() - b.red()) + qAbs(a.green() - b.green())
            + qAbs(a.blue() - b.blue());
    };
    setFrame(&bar, "_winui_hover_progress", 0.0);
    QImage collapsedGeometry = bar.grab().toImage();
    const QColor background = bar.palette().color(QPalette::Window);
    const int sampleX = thumb.center().x();
    QVERIFY(colorDistance(collapsedGeometry.pixelColor(sampleX, 2), background)
            < 5);
    QVERIFY(colorDistance(collapsedGeometry.pixelColor(sampleX, 4), background)
            > 10);
    QVERIFY(colorDistance(collapsedGeometry.pixelColor(sampleX, 11), background)
            > 10);

    QEvent enter(QEvent::Enter);
    QCoreApplication::sendEvent(&bar, &enter);
    QCOMPARE(frameReal(&bar, "_winui_hover_progress"), 1.0);

    const int beforeArrow = bar.value();
    QTest::mousePress(&bar, Qt::LeftButton, Qt::NoModifier, increase.center());
    const qreal pressed = frameReal(&bar, "_winui_press_progress");
    QVERIFY(pressed > 0.0);
    QTest::mouseRelease(&bar, Qt::LeftButton, Qt::NoModifier, increase.center());
    QVERIFY(bar.value() > beforeArrow);

    QEvent leave(QEvent::Leave);
    QCoreApplication::sendEvent(&bar, &leave);
    QCOMPARE(frameReal(&bar, "_winui_hover_progress"), 0.0);
    QCoreApplication::sendEvent(&bar, &enter);
    QCOMPARE(frameReal(&bar, "_winui_hover_progress"), 1.0);

    bar.setLayoutDirection(Qt::RightToLeft);
    option.direction = Qt::RightToLeft;
    option.upsideDown = true;
    option.sliderPosition = bar.sliderPosition();
    option.sliderValue = bar.value();
    const QRect rtlDecrease = bar.style()->subControlRect(
        QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarSubLine, &bar);
    const QRect rtlIncrease = bar.style()->subControlRect(
        QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarAddLine, &bar);
    QVERIFY(rtlDecrease.left() > rtlIncrease.left());

    option.sliderPosition = bar.sliderPosition();
    option.sliderValue = bar.value();
    const QRect currentThumb = bar.style()->subControlRect(
        QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarSlider, &bar);
    bar.setEnabled(false);
    const QImage disabled = bar.grab().toImage();
    QCOMPARE(disabled.pixelColor(currentThumb.center()),
             bar.palette().color(QPalette::Window));
}

void WinUI3ViewsTest::scrollAreaScrollBarIntegration()
{
    QScrollArea area;
    auto *content = new QWidget;
    content->resize(760, 620);
    area.setWidget(content);
    area.resize(260, 180);
    area.show();

    QScrollBar *vertical = area.verticalScrollBar();
    QScrollBar *horizontal = area.horizontalScrollBar();
    QTRY_VERIFY(vertical->isVisible());
    QTRY_VERIFY(horizontal->isVisible());
    QCOMPARE(vertical->width(), 12);
    QCOMPARE(horizontal->height(), 12);
    QVERIFY(vertical->maximum() > 0);
    QVERIFY(horizontal->maximum() > 0);

    vertical->setFocus(Qt::TabFocusReason);
    const int keyboardBefore = vertical->value();
    QTest::keyClick(vertical, Qt::Key_Down);
    QVERIFY(vertical->value() > keyboardBefore);

    const int verticalBefore = vertical->value();
    QTest::mouseClick(vertical, Qt::LeftButton, Qt::NoModifier,
                      QPoint(vertical->width() / 2, vertical->height() - 6));
    QVERIFY(vertical->value() > verticalBefore);
    const int horizontalBefore = horizontal->value();
    QTest::mouseClick(horizontal, Qt::LeftButton, Qt::NoModifier,
                      QPoint(horizontal->width() - 6, horizontal->height() / 2));
    QVERIFY(horizontal->value() > horizontalBefore);
}

void WinUI3ViewsTest::tabViewContract()
{
    QTabWidget tabs;
    tabs.setLayoutDirection(Qt::RightToLeft);
    tabs.setTabsClosable(true);
    tabs.addTab(new QWidget, QStringLiteral("First"));
    tabs.addTab(new QWidget, QStringLiteral("Second"));
    tabs.resize(420, 220);
    tabs.show();

    QTabBar *bar = tabs.tabBar();
    QVERIFY(bar);
    QCOMPARE(bar->tabRect(0).height(), 32);
    QVERIFY(bar->tabRect(0).width() >= 100);
    QCOMPARE(tabs.style()->pixelMetric(QStyle::PM_TabCloseIndicatorWidth,
                                       nullptr, bar), 32);
    QCOMPARE(tabs.style()->pixelMetric(QStyle::PM_TabCloseIndicatorHeight,
                                       nullptr, bar), 24);
    QVERIFY(!tabs.style()->standardIcon(QStyle::SP_TabCloseButton).isNull());

    QStyleOptionTab selected;
    selected.rect = QRect(0, 0, 120, 32);
    selected.shape = QTabBar::RoundedNorth;
    selected.palette = tabs.palette();
    selected.state = QStyle::State_Enabled | QStyle::State_Selected;
    QImage image(selected.rect.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(tabs.palette().color(QPalette::Window));
    {
        QPainter painter(&image);
        tabs.style()->drawControl(QStyle::CE_TabBarTabShape, &selected,
                                  &painter, bar);
    }
    const QColor accent = tabs.palette().color(QPalette::Highlight);
    QVERIFY(image.pixelColor(selected.rect.center().x(),
                             selected.rect.bottom() - 2) != accent);
    QVERIFY(image.pixelColor(selected.rect.center().x(), 5) != accent);

    const auto renderAdjacentTab = [&](QStyleOptionTab::SelectedPosition position,
                                       Qt::LayoutDirection direction) {
        QStyleOptionTab adjacent;
        adjacent.rect = QRect(0, 0, 120, 32);
        adjacent.shape = QTabBar::RoundedNorth;
        adjacent.palette = tabs.palette();
        adjacent.state = QStyle::State_Enabled;
        adjacent.selectedPosition = position;
        adjacent.direction = direction;
        QImage rendered(adjacent.rect.size(),
                        QImage::Format_ARGB32_Premultiplied);
        rendered.fill(adjacent.palette.color(QPalette::Window));
        QPainter painter(&rendered);
        tabs.style()->drawControl(QStyle::CE_TabBarTabShape, &adjacent,
                                  &painter, bar);
        return rendered;
    };
    const QColor tabBackground = tabs.palette().color(QPalette::Window);
    const QImage regularLtr = renderAdjacentTab(
        QStyleOptionTab::NotAdjacent, Qt::LeftToRight);
    const QImage adjacentLtr = renderAdjacentTab(
        QStyleOptionTab::NextIsSelected, Qt::LeftToRight);
    QVERIFY(colorDistance(regularLtr.pixelColor(119, 16), tabBackground) > 4);
    QCOMPARE(adjacentLtr.pixelColor(119, 16), tabBackground);

    const QImage regularRtl = renderAdjacentTab(
        QStyleOptionTab::NotAdjacent, Qt::RightToLeft);
    const QImage adjacentRtl = renderAdjacentTab(
        QStyleOptionTab::NextIsSelected, Qt::RightToLeft);
    QVERIFY(colorDistance(regularRtl.pixelColor(0, 16), tabBackground) > 4);
    QCOMPARE(adjacentRtl.pixelColor(0, 16), tabBackground);
}

void WinUI3ViewsTest::listViewContract()
{
    QListWidget list;
    list.addItems({QStringLiteral("Documents"), QStringLiteral("Pictures")});
    list.setCurrentRow(0);
    list.resize(320, 160);
    list.show();
    QTRY_VERIFY(list.isVisible());
    QCOMPARE(list.sizeHintForRow(0), 40);

    QStyleOptionViewItem option;
    option.initFrom(list.viewport());
    option.widget = list.viewport();
    option.rect = QRect(0, 0, 300, 40);
    option.index = list.model()->index(0, 0);
    option.text = QStringLiteral("Documents");
    option.features = QStyleOptionViewItem::HasDisplay;
    option.state = QStyle::State_Enabled | QStyle::State_Selected;
    QImage selected(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
    selected.fill(list.palette().color(QPalette::Base));
    {
        QPainter painter(&selected);
        list.style()->drawControl(QStyle::CE_ItemViewItem, &option,
                                  &painter, list.viewport());
    }
    const QColor accent = list.palette().color(QPalette::Highlight);
    bool indicatorFound = false;
    for (int y = 8; y < 32; ++y) {
        const QColor pixel = selected.pixelColor(2, y);
        if (qAbs(pixel.red() - accent.red()) < 12
            && qAbs(pixel.green() - accent.green()) < 12
            && qAbs(pixel.blue() - accent.blue()) < 12) {
            indicatorFound = true;
            break;
        }
    }
    QVERIFY(indicatorFound);
}

void WinUI3ViewsTest::itemViewGutterContract()
{
    QListWidget list;
    auto *item = new QListWidgetItem(QStringLiteral("Checked item"), &list);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(Qt::Checked);
    list.resize(320, 80);
    list.show();
    QTRY_VERIFY(list.isVisible());

    QStyleOptionViewItem option;
    option.initFrom(list.viewport());
    option.widget = list.viewport();
    option.rect = QRect(0, 0, 300, 40);
    option.index = list.model()->index(0, 0);
    option.features = QStyleOptionViewItem::HasDisplay
        | QStyleOptionViewItem::HasDecoration
        | QStyleOptionViewItem::HasCheckIndicator;
    option.decorationSize = QSize(16, 16);
    option.direction = Qt::LeftToRight;
    const QRect listCheck = list.style()->subElementRect(
        QStyle::SE_ItemViewItemCheckIndicator, &option, list.viewport());
    const QRect listDecoration = list.style()->subElementRect(
        QStyle::SE_ItemViewItemDecoration, &option, list.viewport());
    const QRect listText = list.style()->subElementRect(
        QStyle::SE_ItemViewItemText, &option, list.viewport());
    QVERIFY(listCheck.left() >= 12);
    QVERIFY(listDecoration.left() >= 12);
    QVERIFY(listText.left() >= 12);
    QVERIFY(listCheck.right() < listText.left());

    option.direction = Qt::RightToLeft;
    const QRect rtlText = list.style()->subElementRect(
        QStyle::SE_ItemViewItemText, &option, list.viewport());
    QVERIFY(rtlText.right() <= option.rect.right() - 12);

    QTreeWidget tree;
    tree.setHeaderHidden(true);
    auto *root = new QTreeWidgetItem(&tree, {QStringLiteral("Root")});
    auto *child = new QTreeWidgetItem(root, {QStringLiteral("Child")});
    tree.expandAll();
    tree.resize(320, 120);
    tree.show();
    QTRY_VERIFY(tree.isVisible());

    auto treeOption = option;
    treeOption.widget = tree.viewport();
    treeOption.direction = Qt::LeftToRight;
    treeOption.index = tree.indexFromItem(root);
    const QRect rootText = tree.style()->subElementRect(
        QStyle::SE_ItemViewItemText, &treeOption, tree.viewport());
    treeOption.index = tree.indexFromItem(child);
    const QRect childText = tree.style()->subElementRect(
        QStyle::SE_ItemViewItemText, &treeOption, tree.viewport());
    QCOMPARE(childText.left() - rootText.left(), tree.indentation());

    QTableWidget table(1, 1);
    table.resize(320, 80);
    table.show();
    QTRY_VERIFY(table.isVisible());
    auto tableOption = option;
    tableOption.widget = table.viewport();
    tableOption.index = table.model()->index(0, 0);
    tableOption.direction = Qt::LeftToRight;
    const QRect tableText = table.style()->subElementRect(
        QStyle::SE_ItemViewItemText, &tableOption, table.viewport());
    QVERIFY(tableText.left() < listText.left());
}

void WinUI3ViewsTest::treeViewContract()
{
    QTreeWidget tree;
    tree.setHeaderHidden(true);
    auto *root = new QTreeWidgetItem(&tree, {QStringLiteral("Workspace")});
    new QTreeWidgetItem(root, {QStringLiteral("src")});
    tree.expandAll();
    tree.resize(320, 180);
    tree.show();
    QTRY_VERIFY(tree.isVisible());
    QCOMPARE(tree.sizeHintForIndex(tree.indexFromItem(root)).height(), 28);

    QStyleOption branch;
    branch.initFrom(tree.viewport());
    branch.rect = QRect(0, 0, 20, 28);
    branch.state = QStyle::State_Enabled | QStyle::State_Children
        | QStyle::State_Open;
    QImage image(branch.rect.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    {
        QPainter painter(&image);
        tree.style()->drawPrimitive(QStyle::PE_IndicatorBranch, &branch,
                                    &painter, tree.viewport());
    }
    bool glyphFound = false;
    for (int y = 0; y < image.height() && !glyphFound; ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (image.pixelColor(x, y).alpha() > 40) {
                glyphFound = true;
                break;
            }
        }
    }
    QVERIFY(glyphFound);
}

void WinUI3ViewsTest::treeSelectionMarkerLeadingEdge()
{
    QTreeWidget tree;
    tree.setHeaderHidden(true);
    auto *root = new QTreeWidgetItem(&tree, {QStringLiteral("Root")});
    auto *child = new QTreeWidgetItem(root, {QStringLiteral("Indented child")});
    tree.expandAll();
    tree.resize(320, 120);
    tree.show();
    QTRY_VERIFY(tree.isVisible());

    const auto renderSelected = [&](Qt::LayoutDirection direction) {
        tree.setLayoutDirection(direction);
        QStyleOptionViewItem option;
        option.initFrom(tree.viewport());
        option.widget = tree.viewport();
        option.direction = direction;
        option.rect = tree.visualRect(tree.indexFromItem(child));
        option.index = tree.indexFromItem(child);
        option.text = child->text(0);
        option.features = QStyleOptionViewItem::HasDisplay;
        option.state = QStyle::State_Enabled | QStyle::State_Selected;

        QImage image(tree.viewport()->size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(tree.palette().color(QPalette::Base));
        QPainter painter(&image);
        tree.style()->drawControl(QStyle::CE_ItemViewItem, &option,
                                  &painter, tree.viewport());
        return image;
    };

    const QColor accent = tree.palette().color(QPalette::Highlight);
    const auto hasAccentNear = [&](const QImage &image, int left) {
        const int y = tree.visualRect(tree.indexFromItem(child)).center().y();
        for (int x = left; x < left + 8; ++x)
            if (image.rect().contains(x, y)
                && colorDistance(image.pixelColor(x, y), accent) < 18)
                return true;
        return false;
    };
    const QImage ltr = renderSelected(Qt::LeftToRight);
    QVERIFY(hasAccentNear(ltr, 0));
    const QImage rtl = renderSelected(Qt::RightToLeft);
    const int rightLeading = rtl.width() - 8;
    QVERIFY(hasAccentNear(rtl, rightLeading));
}

void WinUI3ViewsTest::tableHeaderContract()
{
    QTableWidget table(2, 2);
    table.setLayoutDirection(Qt::RightToLeft);
    table.setHorizontalHeaderLabels({QStringLiteral("Control"),
                                     QStringLiteral("State")});
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
        table.style()->drawControl(QStyle::CE_HeaderSection, &option,
                                   &painter, table.horizontalHeader());
        table.style()->drawControl(QStyle::CE_HeaderLabel, &option,
                                   &painter, table.horizontalHeader());
    }
    QVERIFY(!header.isNull());
    QVERIFY(header.pixelColor(option.rect.right() - 16,
                              option.rect.center().y()).alpha() > 0);
}

void WinUI3ViewsTest::tableSortIndicatorGeometryContract()
{
    QTableWidget table(1, 1);
    table.resize(240, 80);
    table.show();
    QTRY_VERIFY(table.isVisible());

    const auto render = [&table](Qt::LayoutDirection direction, bool sorted,
                                 int height) {
        QStyleOptionHeader option;
        option.initFrom(table.horizontalHeader());
        option.rect = QRect(0, 0, 180, height);
        option.direction = direction;
        option.text.clear();
        option.textAlignment = Qt::AlignLeft;
        option.sortIndicator = sorted ? QStyleOptionHeader::SortDown
                                      : QStyleOptionHeader::None;
        QImage image(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        table.style()->drawControl(QStyle::CE_Header, &option, &painter,
                                   table.horizontalHeader());
        return image;
    };

    for (const Qt::LayoutDirection direction : {Qt::LeftToRight,
                                                 Qt::RightToLeft}) {
        const QImage sorted = render(direction, true, 33);
        const QImage unsorted = render(direction, false, 33);
        QRect difference;
        for (int y = 0; y < sorted.height(); ++y) {
            for (int x = 0; x < sorted.width(); ++x) {
                if (sorted.pixel(x, y) != unsorted.pixel(x, y))
                    difference |= QRect(x, y, 1, 1);
            }
        }
        QVERIFY(!difference.isEmpty());
        // The chevron ink does not fill its 16 px slot; verify that the ink
        // remains inside the vertically centered slot for an odd header.
        QVERIFY(difference.top() >= 8);
        QVERIFY(difference.bottom() <= 24);
        if (direction == Qt::LeftToRight)
            QVERIFY(difference.left() > 130);
        else
            QVERIFY(difference.right() < 40);
    }
}

void WinUI3ViewsTest::tableEditingPaintContract()
{
    QTableWidget table(1, 1);
    table.resize(320, 80);
    table.show();
    QTRY_VERIFY(table.isVisible());

    QStyleOptionViewItem option;
    option.initFrom(table.viewport());
    option.widget = table.viewport();
    option.rect = QRect(0, 0, 240, 36);
    option.index = table.model()->index(0, 0);
    option.text = QStringLiteral("Painted underneath editor");
    option.icon = WinUI3::icon(WinUI3::Icon::Settings);
    option.features = QStyleOptionViewItem::HasDisplay
        | QStyleOptionViewItem::HasDecoration;
    option.state = QStyle::State_Enabled | QStyle::State_Selected
        | QStyle::State_Editing;

    const auto render = [&](const QStyleOptionViewItem &source) {
        QImage image(source.rect.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(table.palette().color(QPalette::Base));
        QPainter painter(&image);
        table.style()->drawControl(QStyle::CE_ItemViewItem, &source,
                                   &painter, table.viewport());
        return image;
    };

    const QImage editing = render(option);
    auto withoutDisplay = option;
    withoutDisplay.features &= ~QStyleOptionViewItem::HasDisplay;
    const QImage expected = render(withoutDisplay);
    QCOMPARE(editing, expected);

    auto unselected = option;
    unselected.state &= ~QStyle::State_Selected;
    QVERIFY(editing.pixelColor(120, 18) != render(unselected).pixelColor(120, 18));
    QVERIFY(editing.pixelColor(8, 18).alpha() > 0);
}

void WinUI3ViewsTest::tableLiveEditorSuppressesDisplay()
{
    QTableWidget table(1, 1);
    table.setItem(0, 0, new QTableWidgetItem(
        QStringLiteral("Painted underneath the live editor")));
    table.setEditTriggers(QAbstractItemView::AllEditTriggers);
    table.resize(320, 80);
    table.show();
    QTRY_VERIFY(table.isVisible());

    const QModelIndex index = table.model()->index(0, 0);
    table.editItem(table.item(0, 0));
    QTRY_VERIFY_WITH_TIMEOUT(!table.findChildren<QLineEdit *>().isEmpty(), 1000);
    QLineEdit *editor = table.findChildren<QLineEdit *>().constFirst();
    QVERIFY(editor->isVisible());
    QVERIFY(editor->property("_winui_table_editor").toBool());
    QStyleOptionViewItem option;
    option.initFrom(table.viewport());
    option.widget = table.viewport();
    option.rect = table.visualRect(index);
    option.index = index;
    option.text = QStringLiteral("Painted underneath the live editor");
    option.features = QStyleOptionViewItem::HasDisplay;
    option.state = QStyle::State_Enabled;

    const auto render = [&](const QStyleOptionViewItem &source) {
        QImage image(source.rect.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(table.palette().color(QPalette::Base));
        QPainter painter(&image);
        table.style()->drawControl(QStyle::CE_ItemViewItem, &source,
                                   &painter, table.viewport());
        return image;
    };

    auto withoutDisplay = option;
    withoutDisplay.features &= ~QStyleOptionViewItem::HasDisplay;
    QCOMPARE(render(option), render(withoutDisplay));
}

void WinUI3ViewsTest::richEditBoxContract()
{
    QTextEdit editor;
    editor.setPlainText(QStringLiteral("RichEditBox state"));
    editor.resize(320, 120);
    editor.show();
    editor.setFocus(Qt::TabFocusReason);
    QTRY_VERIFY(editor.hasFocus());
    QStyleOptionFrame option;
    option.initFrom(&editor);
    option.rect = editor.rect();
    option.state |= QStyle::State_HasFocus;
    QImage focused(editor.size(), QImage::Format_ARGB32_Premultiplied);
    focused.fill(editor.palette().color(QPalette::Window));
    {
        QPainter painter(&focused);
        editor.style()->drawPrimitive(QStyle::PE_Frame, &option, &painter,
                                      &editor);
    }
    const QColor accent = editor.palette().color(QPalette::Accent);
    const QColor underline = focused.pixelColor(focused.width() / 2,
                                                 focused.height() - 2);
    const int distance = qAbs(underline.red() - accent.red())
        + qAbs(underline.green() - accent.green())
        + qAbs(underline.blue() - accent.blue());
    QVERIFY2(distance < 100, qPrintable(QString::number(distance)));
}

void WinUI3ViewsTest::sliderExtremeRangeTicks()
{
    QSlider slider(Qt::Horizontal);
    slider.setRange(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    slider.setValue(0);
    slider.setTickPosition(QSlider::TicksBelow);
    slider.setTickInterval(1);
    slider.resize(480, 40);
    slider.show();
    QVERIFY(!slider.grab().isNull());

    QStyleOptionSlider option;
    option.initFrom(&slider);
    option.rect = slider.rect();
    option.minimum = slider.minimum();
    option.maximum = slider.maximum();
    option.sliderPosition = slider.sliderPosition();
    option.sliderValue = slider.value();
    option.orientation = Qt::Horizontal;
    option.upsideDown = false;
    const QRect handle = slider.style()->subControlRect(
        QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, &slider);
    QVERIFY(handle.isValid());
}

void WinUI3ViewsTest::itemViewMouseFocusReset()
{
    QListWidget list;
    list.addItems({QStringLiteral("First"), QStringLiteral("Second")});
    list.resize(240, 100);
    list.show();
    QTRY_VERIFY(list.hasFocus());
    QFocusEvent keyboardFocus(QEvent::FocusIn, Qt::TabFocusReason);
    QCoreApplication::sendEvent(list.viewport(), &keyboardFocus);
    QVERIFY(frameBool(&list, "_winui_focus_visible"));
    QTest::mouseClick(list.viewport(), Qt::LeftButton, Qt::NoModifier,
                      list.visualItemRect(list.item(0)).center());
    QVERIFY(!frameBool(&list, "_winui_focus_visible"));
    QVERIFY(!frameBool(list.viewport(), "_winui_focus_visible"));
}

void WinUI3ViewsTest::dpiGeometry()
{
    QPushButton button(QStringLiteral("DPI"));
    button.resize(button.sizeHint());
    button.show();
    const qreal devicePixelRatio = button.devicePixelRatioF();
    QVERIFY(devicePixelRatio >= 1.0);
    if (qEnvironmentVariableIsSet("QT_SCALE_FACTOR")) {
        bool ok = false;
        const qreal requestedScale = qEnvironmentVariable("QT_SCALE_FACTOR").toDouble(&ok);
        if (ok)
            QVERIFY(qAbs(devicePixelRatio - requestedScale) < 0.05);
    }
    const QPixmap grabbed = button.grab();
    QVERIFY(qAbs(grabbed.width() - qRound(button.width() * grabbed.devicePixelRatioF()))
            <= 2);
    QVERIFY(qAbs(grabbed.height() - qRound(button.height() * grabbed.devicePixelRatioF()))
            <= 2);
    QVERIFY(button.style()->pixelMetric(QStyle::PM_DefaultFrameWidth,
                                        nullptr, &button) >= 1);

    QComboBox combo;
    combo.addItem(QStringLiteral("DPI"));
    combo.resize(220, 32);
    combo.show();
    QStyleOptionComboBox comboOption;
    comboOption.initFrom(&combo);
    comboOption.rect = combo.rect();
    const QRect comboArrow = combo.style()->subControlRect(
        QStyle::CC_ComboBox, &comboOption, QStyle::SC_ComboBoxArrow, &combo);
    QCOMPARE(comboArrow.size(), QSize(38, 32));

    QScrollBar scrollBar(Qt::Vertical);
    scrollBar.setRange(0, 100);
    scrollBar.setPageStep(20);
    scrollBar.resize(12, 300);
    scrollBar.show();
    QStyleOptionSlider scrollOption;
    scrollOption.initFrom(&scrollBar);
    scrollOption.orientation = scrollBar.orientation();
    scrollOption.minimum = scrollBar.minimum();
    scrollOption.maximum = scrollBar.maximum();
    scrollOption.sliderPosition = scrollBar.sliderPosition();
    scrollOption.sliderValue = scrollBar.value();
    scrollOption.pageStep = scrollBar.pageStep();
    scrollOption.upsideDown = false;
    const QRect scrollThumb = scrollBar.style()->subControlRect(
        QStyle::CC_ScrollBar, &scrollOption, QStyle::SC_ScrollBarSlider,
        &scrollBar);
    QCOMPARE(scrollThumb.width(), 12);
    QCOMPARE(scrollBar.style()->pixelMetric(QStyle::PM_ScrollBarExtent,
                                            &scrollOption, &scrollBar), 12);
}

void WinUI3ViewsTest::dpiHitTestContracts()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);

    QComboBox combo;
    combo.addItem(QStringLiteral("Combo text"));
    combo.resize(220, 32);
    QSpinBox spin;
    spin.setRange(0, 100);
    spin.setValue(42);
    spin.resize(180, 32);
    QToolButton tool;
    tool.setText(QStringLiteral("Tool button"));
    tool.setPopupMode(QToolButton::MenuButtonPopup);
    tool.resize(220, 36);
    QSlider slider(Qt::Horizontal);
    slider.setRange(0, 100);
    slider.setValue(50);
    slider.resize(320, 32);
    QScrollBar scrollBar(Qt::Horizontal);
    scrollBar.setRange(0, 100);
    scrollBar.setPageStep(25);
    scrollBar.setValue(40);
    scrollBar.resize(260, 12);
    setFrame(&scrollBar, "_winui_hover_progress", 1.0);
    QGroupBox group(QStringLiteral("Group title"));
    group.setCheckable(true);
    group.setChecked(true);
    group.resize(320, 120);

    const qreal dpr = combo.devicePixelRatioF();
    QVERIFY(dpr >= 1.0);
    if (qEnvironmentVariableIsSet("QT_SCALE_FACTOR")) {
        bool ok = false;
        const qreal requested = qEnvironmentVariable("QT_SCALE_FACTOR").toDouble(&ok);
        if (ok)
            QVERIFY2(qAbs(dpr - requested) < 0.05,
                     qPrintable(QStringLiteral("DPR=%1 requested=%2")
                                    .arg(dpr).arg(requested)));
    }

    const auto verifyImage = [&](const QWidget &widget,
                                 QStyle::ComplexControl control,
                                 const QStyleOptionComplex &option,
                                 const QList<QRect> &inkRegions) {
        const QImage image = renderComplex(style, control, &option, &widget, dpr);
        QCOMPARE(image.size(), QSize(qRound(option.rect.width() * dpr),
                                     qRound(option.rect.height() * dpr)));
        const QColor background = widget.palette().color(QPalette::Window);
        int totalInk = inkPixels(image, option.rect, dpr, background);
        QVERIFY(totalInk > 0);
        for (const QRect &region : inkRegions)
            if (region.isValid())
                QVERIFY2(inkPixels(image, region, dpr, background) > 0,
                         qPrintable(QStringLiteral("no ink in %1,%2 %3x%4")
                                        .arg(region.x()).arg(region.y())
                                        .arg(region.width()).arg(region.height())));
    };

    for (const Qt::LayoutDirection direction : {Qt::LeftToRight,
                                                Qt::RightToLeft}) {
        combo.setLayoutDirection(direction);
        QStyleOptionComboBox comboOption;
        comboOption.initFrom(&combo);
        comboOption.direction = direction;
        comboOption.rect = combo.rect();
        comboOption.currentText = combo.currentText();
        comboOption.subControls = QStyle::SC_ComboBoxFrame
            | QStyle::SC_ComboBoxEditField | QStyle::SC_ComboBoxArrow;
        const QRect comboEdit = style->subControlRect(
            QStyle::CC_ComboBox, &comboOption, QStyle::SC_ComboBoxEditField,
            &combo);
        const QRect comboArrow = style->subControlRect(
            QStyle::CC_ComboBox, &comboOption, QStyle::SC_ComboBoxArrow, &combo);
        QVERIFY(comboEdit.isValid());
        QVERIFY(comboArrow.isValid());
        QVERIFY(!comboEdit.intersects(comboArrow));
        if (direction == Qt::LeftToRight)
            QCOMPARE(comboEdit.right() + 1, comboArrow.left());
        else
            QCOMPARE(comboArrow.right() + 1, comboEdit.left());
        verifyHitSurface(style, QStyle::CC_ComboBox, &comboOption, &combo);
        QCOMPARE(style->hitTestComplexControl(QStyle::CC_ComboBox, &comboOption,
                                               comboArrow.center(), &combo),
                 QStyle::SC_ComboBoxArrow);
        QCOMPARE(style->hitTestComplexControl(QStyle::CC_ComboBox, &comboOption,
                                               comboEdit.center(), &combo),
                 QStyle::SC_ComboBoxEditField);
        verifyImage(combo, QStyle::CC_ComboBox, comboOption,
                    {comboEdit, comboArrow});

        spin.setLayoutDirection(direction);
        QStyleOptionSpinBox spinOption;
        spinOption.initFrom(&spin);
        spinOption.direction = direction;
        spinOption.rect = spin.rect();
        spinOption.frame = true;
        spinOption.buttonSymbols = spin.buttonSymbols();
        spinOption.stepEnabled = QAbstractSpinBox::StepUpEnabled
            | QAbstractSpinBox::StepDownEnabled;
        spinOption.subControls = QStyle::SC_SpinBoxFrame
            | QStyle::SC_SpinBoxEditField | QStyle::SC_SpinBoxUp
            | QStyle::SC_SpinBoxDown;
        const QRect spinEdit = style->subControlRect(
            QStyle::CC_SpinBox, &spinOption, QStyle::SC_SpinBoxEditField, &spin);
        const QRect spinUp = style->subControlRect(
            QStyle::CC_SpinBox, &spinOption, QStyle::SC_SpinBoxUp, &spin);
        const QRect spinDown = style->subControlRect(
            QStyle::CC_SpinBox, &spinOption, QStyle::SC_SpinBoxDown, &spin);
        QVERIFY(spinEdit.isValid() && spinUp.isValid() && spinDown.isValid());
        QVERIFY(!spinEdit.intersects(spinUp));
        QVERIFY(!spinEdit.intersects(spinDown));
        QVERIFY(!spinUp.intersects(spinDown));
        verifyHitSurface(style, QStyle::CC_SpinBox, &spinOption, &spin);
        QCOMPARE(style->hitTestComplexControl(QStyle::CC_SpinBox, &spinOption,
                                               spinUp.center(), &spin),
                 QStyle::SC_SpinBoxUp);
        QCOMPARE(style->hitTestComplexControl(QStyle::CC_SpinBox, &spinOption,
                                               spinDown.center(), &spin),
                 QStyle::SC_SpinBoxDown);
        QCOMPARE(style->hitTestComplexControl(QStyle::CC_SpinBox, &spinOption,
                                               spinEdit.center(), &spin),
                 QStyle::SC_SpinBoxEditField);
        verifyImage(spin, QStyle::CC_SpinBox, spinOption,
                    {spinEdit, spinUp, spinDown});

        tool.setLayoutDirection(direction);
        QStyleOptionToolButton toolOption;
        toolOption.initFrom(&tool);
        toolOption.direction = direction;
        toolOption.rect = tool.rect();
        toolOption.text = tool.text();
        toolOption.features = QStyleOptionToolButton::MenuButtonPopup;
        toolOption.subControls = QStyle::SC_ToolButton
            | QStyle::SC_ToolButtonMenu;
        const QRect toolMain = style->subControlRect(
            QStyle::CC_ToolButton, &toolOption, QStyle::SC_ToolButton, &tool);
        const QRect toolMenu = style->subControlRect(
            QStyle::CC_ToolButton, &toolOption, QStyle::SC_ToolButtonMenu, &tool);
        QVERIFY(toolMain.isValid() && toolMenu.isValid());
        QVERIFY(!toolMain.intersects(toolMenu));
        QCOMPARE(toolMain.united(toolMenu), tool.rect());
        verifyHitSurface(style, QStyle::CC_ToolButton, &toolOption, &tool);
        QCOMPARE(style->hitTestComplexControl(QStyle::CC_ToolButton, &toolOption,
                                               toolMain.center(), &tool),
                 QStyle::SC_ToolButton);
        QCOMPARE(style->hitTestComplexControl(QStyle::CC_ToolButton, &toolOption,
                                               toolMenu.center(), &tool),
                 QStyle::SC_ToolButtonMenu);
        verifyImage(tool, QStyle::CC_ToolButton, toolOption,
                    {toolMain, toolMenu});

        slider.setLayoutDirection(direction);
        QStyleOptionSlider sliderOption;
        sliderOption.initFrom(&slider);
        sliderOption.direction = direction;
        sliderOption.rect = slider.rect();
        sliderOption.orientation = slider.orientation();
        sliderOption.minimum = slider.minimum();
        sliderOption.maximum = slider.maximum();
        sliderOption.sliderPosition = slider.sliderPosition();
        sliderOption.sliderValue = slider.value();
        sliderOption.upsideDown = direction == Qt::RightToLeft;
        sliderOption.subControls = QStyle::SC_SliderGroove
            | QStyle::SC_SliderHandle;
        const QRect sliderGroove = style->subControlRect(
            QStyle::CC_Slider, &sliderOption, QStyle::SC_SliderGroove, &slider);
        const QRect sliderHandle = style->subControlRect(
            QStyle::CC_Slider, &sliderOption, QStyle::SC_SliderHandle, &slider);
        QVERIFY(sliderGroove.isValid() && sliderHandle.isValid());
        QVERIFY(sliderGroove.intersects(sliderHandle));
        verifyHitSurface(style, QStyle::CC_Slider, &sliderOption, &slider,
                         sliderGroove, {sliderHandle});
        QCOMPARE(style->hitTestComplexControl(QStyle::CC_Slider, &sliderOption,
                                               sliderHandle.center(), &slider),
                 QStyle::SC_SliderHandle);
        verifyImage(slider, QStyle::CC_Slider, sliderOption,
                    {sliderGroove, sliderHandle});

        scrollBar.setLayoutDirection(direction);
        QStyleOptionSlider scrollOption;
        scrollOption.initFrom(&scrollBar);
        scrollOption.direction = direction;
        scrollOption.rect = scrollBar.rect();
        scrollOption.orientation = scrollBar.orientation();
        scrollOption.minimum = scrollBar.minimum();
        scrollOption.maximum = scrollBar.maximum();
        scrollOption.sliderPosition = scrollBar.sliderPosition();
        scrollOption.sliderValue = scrollBar.value();
        scrollOption.pageStep = scrollBar.pageStep();
        scrollOption.upsideDown = direction == Qt::RightToLeft;
        scrollOption.subControls = QStyle::SC_ScrollBarSubLine
            | QStyle::SC_ScrollBarAddLine | QStyle::SC_ScrollBarSubPage
            | QStyle::SC_ScrollBarAddPage | QStyle::SC_ScrollBarGroove
            | QStyle::SC_ScrollBarSlider;
        const QRect scrollSub = style->subControlRect(
            QStyle::CC_ScrollBar, &scrollOption, QStyle::SC_ScrollBarSubLine,
            &scrollBar);
        const QRect scrollAdd = style->subControlRect(
            QStyle::CC_ScrollBar, &scrollOption, QStyle::SC_ScrollBarAddLine,
            &scrollBar);
        const QRect scrollThumb = style->subControlRect(
            QStyle::CC_ScrollBar, &scrollOption, QStyle::SC_ScrollBarSlider,
            &scrollBar);
        QVERIFY(scrollSub.isValid() && scrollAdd.isValid() && scrollThumb.isValid());
        verifyHitSurface(style, QStyle::CC_ScrollBar, &scrollOption, &scrollBar);
        QCOMPARE(style->hitTestComplexControl(QStyle::CC_ScrollBar, &scrollOption,
                                               scrollSub.center(), &scrollBar),
                 QStyle::SC_ScrollBarSubLine);
        QCOMPARE(style->hitTestComplexControl(QStyle::CC_ScrollBar, &scrollOption,
                                               scrollAdd.center(), &scrollBar),
                 QStyle::SC_ScrollBarAddLine);
        QCOMPARE(style->hitTestComplexControl(QStyle::CC_ScrollBar, &scrollOption,
                                               scrollThumb.center(), &scrollBar),
                 QStyle::SC_ScrollBarSlider);
        verifyImage(scrollBar, QStyle::CC_ScrollBar, scrollOption,
                    {scrollSub, scrollAdd, scrollThumb});

        group.setLayoutDirection(direction);
        QStyleOptionGroupBox groupOption;
        groupOption.initFrom(&group);
        groupOption.direction = direction;
        groupOption.rect = group.rect();
        groupOption.text = group.title();
        groupOption.subControls = QStyle::SC_GroupBoxFrame
            | QStyle::SC_GroupBoxLabel | QStyle::SC_GroupBoxCheckBox
            | QStyle::SC_GroupBoxContents;
        const QRect groupIndicator = style->subControlRect(
            QStyle::CC_GroupBox, &groupOption, QStyle::SC_GroupBoxCheckBox, &group);
        const QRect groupLabel = style->subControlRect(
            QStyle::CC_GroupBox, &groupOption, QStyle::SC_GroupBoxLabel, &group);
        const QRect groupContents = style->subControlRect(
            QStyle::CC_GroupBox, &groupOption, QStyle::SC_GroupBoxContents, &group);
        QVERIFY(groupIndicator.isValid() && groupLabel.isValid()
                && groupContents.isValid());
        verifyHitSurface(style, QStyle::CC_GroupBox, &groupOption, &group);
        QCOMPARE(style->hitTestComplexControl(QStyle::CC_GroupBox, &groupOption,
                                               groupIndicator.center(), &group),
                 QStyle::SC_GroupBoxCheckBox);
        QCOMPARE(style->hitTestComplexControl(QStyle::CC_GroupBox, &groupOption,
                                               groupContents.center(), &group),
                 QStyle::SC_GroupBoxContents);
        verifyImage(group, QStyle::CC_GroupBox, groupOption,
                    {groupIndicator, groupLabel, groupContents});
    }
}

QTEST_MAIN(WinUI3ViewsTest)
#include "tst_winui3views.moc"
