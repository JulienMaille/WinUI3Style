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

class WinUI3NavigationTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void navigationTransition();
    void navigationInteractiveFrames();
    void renderCommonStates();
    void pluginFactory();
    void navigationModelReconnectAndScroll();
    void navigationDelegateLifecycle();
};

void WinUI3NavigationTest::initTestCase()
{
    qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light));
}

void WinUI3NavigationTest::init()
{
    if (auto *style = qobject_cast<WinUI3::Style *>(qApp->style())) {
        style->setThemeMode(WinUI3::ThemeMode::Light);
        style->setAccentColor({});
    }
}

void WinUI3NavigationTest::cleanup()
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


void WinUI3NavigationTest::navigationTransition()
{
    WinUI3::NavigationView view;
    view.resize(720, 480);
    view.addPage(new QLabel(QStringLiteral("One")), QIcon(), QStringLiteral("One"));
    view.addPage(new QLabel(QStringLiteral("Two")), QIcon(), QStringLiteral("Two"));
    view.stack()->setDuration(20);
    view.show();
    QVERIFY(view.navigationList()->property(
        WinUI3::Style::NavigationViewProperty).toBool());
    QVERIFY(view.navigationList()->property(
        "_winui_navigation_delegate").value<QObject *>()
            == view.navigationList()->itemDelegate());
    const QRect second = view.navigationList()->visualItemRect(
        view.navigationList()->item(1));
    QTest::mouseMove(view.navigationList()->viewport(), second.center());
    QTest::mouseClick(view.navigationList()->viewport(), Qt::LeftButton,
                      Qt::NoModifier, second.center());
    QTRY_COMPARE(view.currentIndex(), 1);
    QTest::qWait(90);
    const qreal indicator = frameReal(view.navigationList()->viewport(),
                                      "_winui_navigation_indicator_y");
    QVERIFY(indicator > 0.0 && indicator < second.top());
    QTRY_VERIFY(qAbs(frameReal(view.navigationList()->viewport(),
                              "_winui_navigation_indicator_y")
                     - second.top()) < 0.5);
}

void WinUI3NavigationTest::navigationInteractiveFrames()
{
    WinUI3::NavigationView view;
    view.resize(720, 420);
    auto *one = new SolidPage(QColor(180, 55, 55), QStringLiteral("One"));
    auto *two = new SolidPage(QColor(55, 165, 85), QStringLiteral("Two"));
    auto *three = new SolidPage(QColor(55, 85, 190), QStringLiteral("Three"));
    view.addPage(one, QIcon(), QStringLiteral("One"));
    view.addPage(two, QIcon(), QStringLiteral("Two"));
    view.addPage(three, QIcon(), QStringLiteral("Three"));
    view.stack()->setDuration(120);
    view.show();
    QCoreApplication::processEvents();

    auto *stack = view.stack();
    const QPoint sample(8, 8);
    const QRect pageRect = stack->rect();
    QVERIFY(!pageRect.isEmpty());
    QCOMPARE(one->geometry(), pageRect);
    QCOMPARE(two->geometry(), pageRect);
    QCOMPARE(three->geometry(), pageRect);

    const QRect secondItem = view.navigationList()->visualItemRect(
        view.navigationList()->item(1));
    QTest::mouseClick(view.navigationList()->viewport(), Qt::LeftButton,
                      Qt::NoModifier, secondItem.center());
    QCoreApplication::processEvents();
    QVERIFY(stack->isAnimating());
    QCOMPARE(stack->currentWidget(), two);
    QCOMPARE(one->isVisible(), false);
    QCOMPARE(two->isVisible(), false);
    QCOMPARE(three->isVisible(), false);
    QCOMPARE(one->geometry(), pageRect);
    QCOMPARE(two->geometry(), pageRect);
    QCOMPARE(three->geometry(), pageRect);
    auto overlays = stack->findChildren<QWidget *>(
        QStringLiteral("_winui_animated_stack_overlay"),
        Qt::FindDirectChildrenOnly);
    QCOMPARE(overlays.size(), 1);
    QCOMPARE(overlays.constFirst()->isVisible(), true);
    QVERIFY(overlays.constFirst()->graphicsEffect() == nullptr);
    const QImage first = stack->grab().toImage();
    const QColor firstPixel = first.pixelColor(sample);
    QVERIFY(colorDistance(firstPixel, QColor(55, 165, 85))
            < colorDistance(firstPixel, QColor(180, 55, 55)));

    const auto containsOutgoingPageColor = [](const QImage &image) {
        const QColor outgoing(180, 55, 55);
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                if (colorDistance(image.pixelColor(x, y), outgoing) < 12)
                    return true;
            }
        }
        return false;
    };
    QVERIFY2(!containsOutgoingPageColor(first),
             "The first transition frame still contains outgoing-page pixels");

    auto *group = stack->findChild<QParallelAnimationGroup *>(
        QStringLiteral("_winui_animated_stack_group"),
        Qt::FindDirectChildrenOnly);
    QVERIFY(group);
    group->setCurrentTime(group->duration() / 2);
    QCoreApplication::processEvents();
    const QImage midpoint = stack->grab().toImage();
    QVERIFY(midpoint != first);
    const QColor midpointPixel = midpoint.pixelColor(sample);
    QVERIFY(colorDistance(midpointPixel, QColor(55, 165, 85))
            < colorDistance(midpointPixel, QColor(180, 55, 55)));
    QVERIFY2(!containsOutgoingPageColor(midpoint),
             "The midpoint still blends outgoing and incoming page content");

    // Interrupt before completion, then immediately reverse again. There is
    // always one composited snapshot and one current page; no stale page or
    // effect is allowed to remain visible underneath the new target.
    view.setCurrentIndex(2);
    QCoreApplication::processEvents();
    QCOMPARE(stack->currentWidget(), three);
    QCOMPARE(stack->isAnimating(), true);
    view.setCurrentIndex(0);
    QCoreApplication::processEvents();
    QCOMPARE(stack->currentWidget(), one);
    QCOMPARE(stack->isAnimating(), true);
    overlays = stack->findChildren<QWidget *>(
        QStringLiteral("_winui_animated_stack_overlay"),
        Qt::FindDirectChildrenOnly);
    QCOMPARE(overlays.size(), 1);
    QCOMPARE(one->isVisible(), false);
    QCOMPARE(two->isVisible(), false);
    QCOMPARE(three->isVisible(), false);
    QCOMPARE(one->geometry(), stack->rect());
    QCOMPARE(two->geometry(), stack->rect());
    QCOMPARE(three->geometry(), stack->rect());

    group = stack->findChild<QParallelAnimationGroup *>(
        QStringLiteral("_winui_animated_stack_group"),
        Qt::FindDirectChildrenOnly);
    QVERIFY(group);
    group->setCurrentTime(group->duration());
    QCoreApplication::processEvents();
    QVERIFY(!stack->isAnimating());
    QCOMPARE(stack->currentWidget(), one);
    QCOMPARE(one->isVisible(), true);
    QVERIFY(one->graphicsEffect() == nullptr);
    QVERIFY(two->graphicsEffect() == nullptr);
    QVERIFY(three->graphicsEffect() == nullptr);
    stack->hide();
    QVERIFY(!stack->isAnimating());
    stack->show();
    QCOMPARE(stack->currentWidget(), one);

    // currentChanged is synchronous. A consumer may redirect navigation from
    // that signal while the first transition is still being constructed.
    WinUI3::AnimatedStack reentrant;
    reentrant.setDuration(60);
    reentrant.addWidget(new SolidPage(QColor(190, 70, 70), QStringLiteral("A")));
    reentrant.addWidget(new SolidPage(QColor(70, 190, 90), QStringLiteral("B")));
    reentrant.addWidget(new SolidPage(QColor(70, 90, 190), QStringLiteral("C")));
    reentrant.resize(320, 160);
    reentrant.show();
    bool redirected = false;
    connect(&reentrant, &QStackedWidget::currentChanged,
            [&reentrant, &redirected](int index) {
        if (index == 1 && !redirected) {
            redirected = true;
            reentrant.setCurrentIndex(2);
        }
    });
    reentrant.setCurrentIndex(1);
    QCoreApplication::processEvents();
    auto *reentrantGroup = reentrant.findChild<QParallelAnimationGroup *>(
        QStringLiteral("_winui_animated_stack_group"),
        Qt::FindDirectChildrenOnly);
    QVERIFY(reentrantGroup);
    reentrantGroup->setCurrentTime(reentrantGroup->duration());
    QCoreApplication::processEvents();
    QVERIFY(redirected);
    QCOMPARE(reentrant.currentIndex(), 2);
    QVERIFY(!reentrant.isAnimating());
    QCOMPARE(reentrant.findChildren<QWidget *>(
                 QStringLiteral("_winui_animated_stack_overlay"),
                 Qt::FindDirectChildrenOnly).size(), 0);
}

void WinUI3NavigationTest::renderCommonStates()
{
    QWidget host;
    auto *layout = new QVBoxLayout(&host);
    auto *normal = new QPushButton(QStringLiteral("Normal"));
    auto *accent = new QPushButton(QStringLiteral("Accent"));
    WinUI3::Style::setControlRole(accent, WinUI3::ControlRole::Accent);
    auto *disabled = new QPushButton(QStringLiteral("Disabled"));
    disabled->setEnabled(false);
    layout->addWidget(normal);
    layout->addWidget(accent);
    layout->addWidget(disabled);
    host.resize(320, 180);
    host.show();
    QTest::mouseMove(normal, normal->rect().center());
    QTest::qWait(100);
    const QImage image = host.grab().toImage();
    QCOMPARE(image.size(), host.size());
    QVERIFY(!image.isNull());
    QVERIFY(image.pixelColor(0, 0).isValid());
}

void WinUI3NavigationTest::pluginFactory()
{
#ifdef WINUI3STYLE_TEST_PLUGIN
    QVERIFY(QStyleFactory::keys().contains(QStringLiteral("winui3"), Qt::CaseInsensitive));
    QScopedPointer<QStyle> loaded(QStyleFactory::create(QStringLiteral("winui3")));
    QVERIFY(loaded);
    QCOMPARE(loaded->objectName(), QStringLiteral("winui3"));
#else
    QSKIP("Style plugin was disabled at configure time");
#endif
}

void WinUI3NavigationTest::navigationModelReconnectAndScroll()
{
    QListView view;
    view.setLayoutDirection(Qt::RightToLeft);
    WinUI3::Style::setNavigationView(&view);
    QStandardItemModel first(40, 1);
    for (int row = 0; row < first.rowCount(); ++row)
        first.setData(first.index(row, 0), QStringLiteral("First %1").arg(row));
    view.setModel(&first);
    view.setCurrentIndex(first.index(10, 0));
    view.resize(260, 120);
    view.show();
    QTRY_VERIFY(view.property("_winui_navigation_delegate").isValid());
    view.scrollTo(first.index(10, 0), QAbstractItemView::PositionAtCenter);
    const qreal before = frameReal(view.viewport(),
                                   "_winui_navigation_indicator_y");

    QStandardItemModel second(40, 1);
    for (int row = 0; row < second.rowCount(); ++row)
        second.setData(second.index(row, 0), QStringLiteral("Second %1").arg(row));
    view.setModel(&second);
    auto *replacementSelection = new QItemSelectionModel(&second, &view);
    view.setSelectionModel(replacementSelection);
    view.setCurrentIndex(second.index(20, 0));
    view.scrollTo(second.index(20, 0), QAbstractItemView::PositionAtCenter);
    QCoreApplication::processEvents();
    const qreal after = frameReal(view.viewport(),
                                  "_winui_navigation_indicator_y");
    QVERIFY(std::isfinite(after));
    QVERIFY(before != after || view.currentIndex().row() == 20);

    second.clear();
    QCoreApplication::processEvents();
    QVERIFY(!view.currentIndex().isValid());
    QVERIFY(!frameValue(view.viewport(),
                        "_winui_navigation_indicator_y").isValid());
}

void WinUI3NavigationTest::navigationDelegateLifecycle()
{
    QListView view;
    view.setProperty(WinUI3::Style::BackdropProperty, QStringLiteral("mica"));
    view.resize(280, 140);
    QStandardItemModel model(40, 1);
    for (int row = 0; row < model.rowCount(); ++row)
        model.setData(model.index(row, 0), QStringLiteral("Item %1").arg(row));
    view.setModel(&model);
    view.setCurrentIndex(model.index(4, 0));
    const QPalette originalViewPalette = view.palette();
    const QPalette originalViewportPalette = view.viewport()->palette();
    const QFrame::Shape originalFrameShape = view.frameShape();
    const bool originalViewportAutoFill = view.viewport()->autoFillBackground();
    const bool originalViewportOpaque =
        view.viewport()->testAttribute(Qt::WA_OpaquePaintEvent);
    WinUI3::Style::setNavigationView(&view);
    view.show();
    QTRY_VERIFY(view.property("_winui_navigation_delegate").isValid());
    // Transparent reveal preserves the RGB for token derivation
    // (transparentized): alpha 0 with the palette ink intact.
    QCOMPARE(view.palette().color(QPalette::Base).alpha(), 0);
    QCOMPARE(view.viewport()->palette().color(QPalette::Base).alpha(), 0);
    QCOMPARE(view.frameShape(), QFrame::NoFrame);

    // Re-entering while the previous delegate is deferred for deletion must
    // not create a second model/scrollbar subscription.
    for (int cycle = 0; cycle < 6; ++cycle) {
        WinUI3::Style::setNavigationView(&view, false);
        WinUI3::Style::setNavigationView(&view, true);
    }
    QCoreApplication::processEvents();
    QVERIFY(view.itemDelegate());
    QVERIFY(view.property("_winui_navigation_delegate").isValid());

    auto *external = new QStyledItemDelegate(&view);
    view.setItemDelegate(external);
    WinUI3::Style::setNavigationView(&view, false);
    QCoreApplication::processEvents();
    QCOMPARE(view.itemDelegate(), external);
    QVERIFY(!frameValue(view.viewport(),
                        "_winui_navigation_indicator_y").isValid());

    // The original delegate can disappear before restoration. The style must
    // install a valid owned fallback instead of restoring a dangling pointer.
    auto *original = new QStyledItemDelegate(&view);
    view.setItemDelegate(original);
    WinUI3::Style::setNavigationView(&view, true);
    QTRY_VERIFY(view.property("_winui_navigation_delegate").isValid());
    delete original;
    WinUI3::Style::setNavigationView(&view, false);
    QCoreApplication::processEvents();
    QVERIFY(view.itemDelegate());
    QVERIFY(!view.property("_winui_navigation_delegate").isValid());
    QVERIFY(!frameValue(view.viewport(),
                        "_winui_navigation_indicator_y").isValid());

    // A model reset while the indicator is moving must leave no stale target.
    WinUI3::Style::setNavigationView(&view, true);
    view.setCurrentIndex(model.index(20, 0));
    model.clear();
    QCoreApplication::processEvents();
    QVERIFY(!frameValue(view.viewport(),
                        "_winui_navigation_indicator_y").isValid());

    WinUI3::Style::setNavigationView(&view, false);
    QCOMPARE(view.palette(), originalViewPalette);
    QCOMPARE(view.viewport()->palette(), originalViewportPalette);
    QCOMPARE(view.frameShape(), originalFrameShape);
    QCOMPARE(view.viewport()->autoFillBackground(), originalViewportAutoFill);
    QCOMPARE(view.viewport()->testAttribute(Qt::WA_OpaquePaintEvent),
             originalViewportOpaque);

    QListView opaqueView;
    const QPalette opaquePalette = opaqueView.palette();
    WinUI3::Style::setNavigationView(&opaqueView);
    opaqueView.show();
    QCoreApplication::processEvents();
    QCOMPARE(opaqueView.palette().color(QPalette::Base),
             opaquePalette.color(QPalette::Base));
    opaqueView.setProperty(WinUI3::Style::BackdropProperty,
                           QStringLiteral("mica"));
    QTRY_VERIFY(opaqueView.palette().color(QPalette::Base).alpha() == 0);
    opaqueView.setProperty(WinUI3::Style::BackdropProperty,
                           QStringLiteral("none"));
    QTRY_COMPARE(opaqueView.palette().color(QPalette::Base),
                 opaquePalette.color(QPalette::Base));

    QWidget backdropHost;
    QListView inheritedView(&backdropHost);
    WinUI3::Style::setNavigationView(&inheritedView);
    backdropHost.show();
    QCoreApplication::processEvents();
    const bool inheritedViewPaletteExplicit =
        inheritedView.testAttribute(Qt::WA_SetPalette);
    const bool inheritedViewportPaletteExplicit =
        inheritedView.viewport()->testAttribute(Qt::WA_SetPalette);
    backdropHost.setProperty(WinUI3::Style::BackdropProperty,
                             QStringLiteral("mica"));
    QTRY_VERIFY(inheritedView.palette().color(QPalette::Base).alpha() == 0);
    backdropHost.setProperty(WinUI3::Style::BackdropProperty,
                             QStringLiteral("none"));
    QTRY_COMPARE(inheritedView.testAttribute(Qt::WA_SetPalette),
                 inheritedViewPaletteExplicit);
    QTRY_COMPARE(inheritedView.viewport()->testAttribute(Qt::WA_SetPalette),
                 inheritedViewportPaletteExplicit);
}

QTEST_MAIN(WinUI3NavigationTest)
#include "tst_winui3navigation.moc"
