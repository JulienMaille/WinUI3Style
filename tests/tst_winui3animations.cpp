// SPDX-License-Identifier: LGPL-2.1-or-later
// Regression test for the FrameAnimationDriver self-deletion bug: the driver
// deleted the finished-signal sender (and stop()'d animations) synchronously
// from inside their own emission / QAbstractAnimation::stop() epilogue.
// Deleting a QObject while it is emitting (or while stop() is on the stack)
// is unsafe because the emission/stop epilogue may touch `this` after slots
// return. The fix defers every destruction with deleteLater().
//
// Standalone like tst_winui3frameproperties.cpp: FrameAnimationDriver is a
// private class compiled into this binary (see tests/CMakeLists.txt), so
// this file uses the registry API directly instead of the shared helpers
// header (which would pull in the public library).
#include "winui3animations_p.h"
#include "winui3frameproperties_p.h"

#include <QAbstractAnimation>
#include <QEasingCurve>
#include <QObject>
#include <QPointer>
#include <QSignalSpy>
#include <QTest>
#include <QVariantAnimation>
#include <QWidget>

using WinUI3::Private::FrameAnimationDriver;

class WinUI3AnimationsTest final : public QObject
{
    Q_OBJECT

private slots:
    void finishedHandlerDefersDeletion();
    void instantAnimateForgetsRunningAnimationSafely();
    void stopFromInsideValueChangedIsSafe();
};

void WinUI3AnimationsTest::finishedHandlerDefersDeletion()
{
    QObject context;
    FrameAnimationDriver driver(&context);
    QWidget widget;

    driver.animate(&widget, "_winui_animation_self_delete_progress", 1.0, 40, true,
                   QEasingCurve(QEasingCurve::Linear), {}, 0.0);
    const auto animations = context.findChildren<QVariantAnimation *>();
    QVERIFY(!animations.isEmpty());
    const QPointer<QVariantAnimation> guarded(animations.constFirst());
    QVERIFY(!guarded.isNull());

    QSignalSpy finishedSpy(guarded.data(), &QAbstractAnimation::finished);
    QVERIFY(finishedSpy.isValid());
    bool aliveWhenDelivered = false;
    // Connected after the driver's own finished slot, so this runs after the
    // driver has unlinked the animation: the sender must still be alive here.
    // A synchronous `delete animation` in the driver's slot would leave the
    // sender dead while this emission (and QAbstractAnimation::stop()'s
    // epilogue) is still on the stack, so this probe fails pre-fix.
    QObject::connect(guarded.data(), &QAbstractAnimation::finished, &context,
                     [&] { aliveWhenDelivered = !guarded.isNull(); });

    QTRY_VERIFY_WITH_TIMEOUT(!finishedSpy.isEmpty(), 2000);
    QVERIFY(aliveWhenDelivered);
    // Destruction is deferred to the event loop, so the sender outlives the
    // emission and is reaped shortly after.
    QTRY_VERIFY_WITH_TIMEOUT(guarded.isNull(), 2000);
}

void WinUI3AnimationsTest::instantAnimateForgetsRunningAnimationSafely()
{
    QObject context;
    FrameAnimationDriver driver(&context);
    QWidget widget;

    driver.animate(&widget, "_winui_animation_forget_progress", 1.0, 10000, true,
                   QEasingCurve(QEasingCurve::Linear), {}, 0.0);
    const auto animations = context.findChildren<QVariantAnimation *>();
    QVERIFY(!animations.isEmpty());
    const QPointer<QVariantAnimation> oldGuarded(animations.constFirst());
    QVERIFY(!oldGuarded.isNull());
    QSignalSpy finishedSpy(oldGuarded.data(), &QAbstractAnimation::finished);
    QVERIFY(finishedSpy.isValid());

    // An instant retarget takes the forget() path for the running animation:
    // erase it from the map, stop it, and destroy it without running the
    // finished epilogue from inside stop().
    driver.animate(&widget, "_winui_animation_forget_progress", 0.25, 0, true,
                   QEasingCurve(QEasingCurve::Linear), {}, 0.0);

    // stop() must not emit finished for the discarded animation.
    QCOMPARE(finishedSpy.count(), 0);
    // Deferred destruction: the old animation is stopped but still alive
    // until the event loop runs. A synchronous delete would fail here.
    QVERIFY(!oldGuarded.isNull());
    // State assertion paired with the cleanup check: the instant target
    // lands in the frame registry on the same state.
    QCOMPARE(WinUI3::Private::framePropertyRegistry().real(
                     &widget, "_winui_animation_forget_progress", -1.0),
             qreal(0.25));
    QTRY_VERIFY_WITH_TIMEOUT(oldGuarded.isNull(), 2000);
    QVERIFY(context.findChildren<QVariantAnimation *>().isEmpty());
}

void WinUI3AnimationsTest::stopFromInsideValueChangedIsSafe()
{
    QObject context;
    FrameAnimationDriver driver(&context);
    QWidget widget;

    driver.animate(&widget, "_winui_animation_reentrant_progress", 1.0, 10000, true,
                   QEasingCurve(QEasingCurve::Linear), {}, 0.0);
    const auto animations = context.findChildren<QVariantAnimation *>();
    QVERIFY(!animations.isEmpty());
    const QPointer<QVariantAnimation> guarded(animations.constFirst());
    QVERIFY(!guarded.isNull());

    // Re-enter the driver from inside the animation's own valueChanged
    // emission, mirroring Style::unpolish()/stopAnimations running during a
    // live frame update. stop() must not destroy the sender synchronously
    // while its emission is on the stack.
    QObject::connect(guarded.data(), &QVariantAnimation::valueChanged, &context,
                     [&](const QVariant &) { driver.stop(&widget); });

    // The first delivered frame triggers the re-entrant stop; the deferred
    // delete is reaped by the event loop without touching freed memory on
    // return into QVariantAnimation's emission epilogue.
    QTRY_VERIFY_WITH_TIMEOUT(guarded.isNull(), 2000);

    // The map entry was erased: re-animating the same key starts exactly one
    // fresh animation and it settles on its own.
    driver.animate(&widget, "_winui_animation_reentrant_progress", 0.0, 40, true,
                   QEasingCurve(QEasingCurve::Linear), {}, 1.0);
    QCOMPARE(context.findChildren<QVariantAnimation *>().size(), 1);
    QTRY_VERIFY_WITH_TIMEOUT(context.findChildren<QVariantAnimation *>().isEmpty(), 2000);
}

QTEST_MAIN(WinUI3AnimationsTest)
#include "tst_winui3animations.moc"
