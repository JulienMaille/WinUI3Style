// SPDX-License-Identifier: LGPL-2.1-or-later
// Domain split of tst_winui3interaction.cpp (plan step 8): the AnimatedStack
// widget-lifetime tests outgrew the 60 KB source ratchet alongside the
// settings-card and interaction suites, so they moved here. Test-function
// names stay identical to the pre-split file so the DPI reruns in
// tests/CMakeLists.txt keep addressing tests by function name.
#include <winui3style/animatedstack.h>
#include <winui3style/winui3style.h>

#include <QCoreApplication>
#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QParallelAnimationGroup>
#include <QPointer>
#include <QTest>

class WinUI3AnimatedStackTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void animatedStackEffectsAndInterruption();
    void animatedStackLifecycleStress();
    void animatedStackFinishRetrigger();
    void animatedStackDeferredReplay();
};

void WinUI3AnimatedStackTest::initTestCase()
{
    qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light));
}

void WinUI3AnimatedStackTest::init()
{
    if (auto *style = qobject_cast<WinUI3::Style *>(qApp->style())) {
        style->setThemeMode(WinUI3::ThemeMode::Light);
        style->setAccentColor({});
    }
}

void WinUI3AnimatedStackTest::cleanup()
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

void WinUI3AnimatedStackTest::animatedStackEffectsAndInterruption()
{
    WinUI3::AnimatedStack stack;
    auto *first = new QLabel(QStringLiteral("First"));
    auto *second = new QLabel(QStringLiteral("Second"));
    auto *effect = new QGraphicsOpacityEffect;
    effect->setOpacity(0.7);
    first->setGraphicsEffect(effect);
    stack.addWidget(first);
    stack.addWidget(second);
    stack.setDuration(120);
    stack.resize(240, 80);
    stack.show();
    stack.setCurrentIndex(1);
    QCOMPARE(second->geometry(), stack.rect());
    QTest::qWait(25);
    QVERIFY(stack.isAnimating());
    stack.setCurrentIndex(0);
    QTRY_VERIFY(!stack.isAnimating());
    QCOMPARE(first->graphicsEffect(), effect);
    stack.setCurrentIndex(1);
    QTest::qWait(20);
    stack.removeWidget(second);
    QTRY_VERIFY(!stack.isAnimating());
    QCOMPARE(first->graphicsEffect(), effect);

    auto *third = new QLabel(QStringLiteral("Third"));
    stack.addWidget(third);
    stack.setCurrentIndex(stack.indexOf(third));
    QTest::qWait(20);
    stack.hide();
    QTRY_VERIFY(!stack.isAnimating());
    stack.show();
    QCOMPARE(stack.currentWidget(), third);
    QCOMPARE(first->graphicsEffect(), effect);

    qputenv("WINUI3STYLE_DISABLE_ANIMATIONS", "1");
    stack.setCurrentIndex(0);
    QVERIFY(!stack.isAnimating());
    qunsetenv("WINUI3STYLE_DISABLE_ANIMATIONS");

    WinUI3::AnimatedStack replacementStack;
    replacementStack.setDuration(100);
    replacementStack.addWidget(new QLabel(QStringLiteral("Outgoing")));
    auto *incoming = new QLabel(QStringLiteral("Incoming"));
    replacementStack.addWidget(incoming);
    replacementStack.resize(240, 80);
    replacementStack.show();
    replacementStack.setCurrentIndex(1);
    QTRY_VERIFY(replacementStack.isAnimating());
    auto *applicationEffect = new QGraphicsOpacityEffect;
    applicationEffect->setOpacity(0.63);
    incoming->setGraphicsEffect(applicationEffect);
    QTRY_VERIFY(!replacementStack.isAnimating());
    QCOMPARE(incoming->graphicsEffect(), applicationEffect);
    QCOMPARE(applicationEffect->opacity(), 0.63);
}

void WinUI3AnimatedStackTest::animatedStackLifecycleStress()
{
    WinUI3::AnimatedStack stack;
    stack.setDuration(1000);
    for (int i = 0; i < 7; ++i)
        stack.addWidget(new QLabel(QStringLiteral("Page %1").arg(i)));
    stack.resize(320, 120);
    stack.show();
    QCoreApplication::processEvents();

    auto settle = [&stack] {
        if (auto *group = stack.findChild<QParallelAnimationGroup *>(
                    QStringLiteral("_winui_animated_stack_group"), Qt::FindDirectChildrenOnly)) {
            group->setCurrentTime(group->duration());
            QCoreApplication::processEvents();
        }
        QCOMPARE(stack.findChildren<QParallelAnimationGroup *>(
                              QStringLiteral("_winui_animated_stack_group"),
                              Qt::FindDirectChildrenOnly)
                         .size(),
                 0);
        QCOMPARE(stack.findChildren<QWidget *>(QStringLiteral("_winui_animated_stack_overlay"),
                                               Qt::FindDirectChildrenOnly)
                         .size(),
                 0);
    };

    stack.setCurrentIndex(1);
    QVERIFY(stack.isAnimating());
    stack.resize(480, 160);
    auto overlays = stack.findChildren<QWidget *>(QStringLiteral("_winui_animated_stack_overlay"),
                                                  Qt::FindDirectChildrenOnly);
    QCOMPARE(overlays.size(), 1);
    QCOMPARE(overlays.constFirst()->geometry(), stack.rect());

    // Remove a non-current page while the source/cible pair is alive. The
    // target pointer, rather than its old numeric index, must remain final.
    stack.removeWidget(stack.widget(5));
    QCOMPARE(stack.currentWidget(), stack.widget(1));
    settle();

    // Remove the outgoing/source page itself while the target is entering.
    stack.setCurrentIndex(2);
    QWidget *outgoing = stack.widget(1);
    QVERIFY(outgoing);
    stack.removeWidget(outgoing);
    QVERIFY(!stack.isAnimating());
    QCOMPARE(stack.currentWidget(), stack.widget(1));
    settle();

    // Removing the incoming page must fall back to the guarded outgoing page.
    stack.setCurrentIndex(2);
    QVERIFY(stack.isAnimating());
    QWidget *incoming = stack.currentWidget();
    stack.removeWidget(incoming);
    QVERIFY(!stack.isAnimating());
    QVERIFY(stack.currentWidget());
    settle();

    // Remove the page currently being displayed, then exercise a long burst
    // of direction reversals. No iteration may create more than one live
    // animation group or overlay.
    QWidget *current = stack.currentWidget();
    stack.removeWidget(current);
    QVERIFY(stack.currentWidget());
    for (int i = 0; i < 100; ++i) {
        const int target = i % stack.count();
        stack.setCurrentIndex(target,
                              i % 3 == 0 ? WinUI3::AnimatedStack::Transition::Backward
                                         : WinUI3::AnimatedStack::Transition::Forward);
        QVERIFY(stack.findChildren<QParallelAnimationGroup *>(
                             QStringLiteral("_winui_animated_stack_group"),
                             Qt::FindDirectChildrenOnly)
                        .size()
                <= 1);
        QVERIFY(stack.findChildren<QWidget *>(QStringLiteral("_winui_animated_stack_overlay"),
                                              Qt::FindDirectChildrenOnly)
                        .size()
                <= 1);
        if (i % 10 == 0) {
            stack.resize(320 + i, 120 + (i % 4) * 10);
            overlays = stack.findChildren<QWidget *>(
                    QStringLiteral("_winui_animated_stack_overlay"), Qt::FindDirectChildrenOnly);
            if (!overlays.isEmpty())
                QCOMPARE(overlays.constFirst()->geometry(), stack.rect());
        }
    }
    settle();
    QCOMPARE(stack.currentWidget()->geometry(), stack.rect());
}

void WinUI3AnimatedStackTest::animatedStackFinishRetrigger()
{
    WinUI3::AnimatedStack stack;
    for (int i = 0; i < 3; ++i)
        stack.addWidget(new QLabel(QString::number(i)));
    stack.resize(240, 80);
    stack.show();
    stack.setDuration(60);
    connect(&stack, &WinUI3::AnimatedStack::transitionFinished,
            [&](int index) { if (index == 1) stack.setCurrentIndex(2); });
    stack.setCurrentIndex(1);
    const QPointer<QParallelAnimationGroup> first(stack.findChild<QParallelAnimationGroup *>());
    QVERIFY(first);
    QTRY_VERIFY(!stack.isAnimating());
    QTRY_VERIFY(first.isNull());
    QCOMPARE(stack.currentIndex(), 2);
}

void WinUI3AnimatedStackTest::animatedStackDeferredReplay()
{
    WinUI3::AnimatedStack stack;
    for (int i = 0; i < 4; ++i)
        stack.addWidget(new QLabel(QString::number(i)));
    stack.setDuration(60);
    stack.resize(240, 80);
    stack.show();
    bool redirected = false;
    connect(&stack, &QStackedWidget::currentChanged, [&](int index) {
        if (index == 1 && !redirected) {
            redirected = true;
            stack.setCurrentIndex(2);
            stack.removeWidget(stack.widget(1));
        }
    });
    stack.setCurrentIndex(1);
    QTRY_VERIFY(!stack.isAnimating());
    // The deferred redirect targeted page 2, but the currentChanged handler
    // removed page 1 before the deferred request replayed. The deferred
    // target follows the widget (dense-index adjustment, like m_from/m_to),
    // so the original page 2 now lives at index 1.
    QCOMPARE(stack.currentIndex(), 1);
    QCOMPARE(static_cast<QLabel *>(stack.currentWidget())->text(), QStringLiteral("2"));
}

QTEST_MAIN(WinUI3AnimatedStackTest)
#include "tst_winui3animatedstack.moc"