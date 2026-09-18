// SPDX-License-Identifier: LGPL-2.1-or-later
// Thread-safety mechanism tests for the system-appearance cache
// (src/winui3theme_p.cpp). The cache is the only appearance surface designed
// for concurrent access: worker threads may read the theme/accent while the
// GUI thread invalidates. Standalone like winui3style_appearancewatcher_tests
// (compiles the private theme source directly) because the cache symbols are
// DLL-internal.
#include "winui3theme_p.h"

#include <QApplication>
#include <QMutex>
#include <QMutexLocker>
#include <QThread>
#include <QtTest>

#include <atomic>

using WinUI3::Private::invalidateSystemAppearanceCache;
using WinUI3::Private::SystemAccentRamp;
using WinUI3::Private::systemAccentRamp;
using WinUI3::Private::systemUsesDarkTheme;

class WinUI3ThemeThreadTest final : public QObject
{
    Q_OBJECT

private slots:
    void concurrentRampReadAndInvalidate();
    void concurrentDarkReadAndInvalidate();
};

static bool sameHueFamily(const QColor &a, const QColor &b)
{
    // Same coherence rule as the accent-atomicity contract in
    // tst_winui3buttons.cpp: a near-gray accent has no stable hue; its ramp
    // roles are still coherent when both are achromatic.
    const QColor first = a.toHsv();
    const QColor second = b.toHsv();
    if (first.saturation() < 16 || second.saturation() < 16)
        return first.saturation() < 16 && second.saturation() < 16;
    const int distance = qAbs(first.hue() - second.hue());
    return qMin(distance, 360 - distance) <= 8;
}

static bool rampIsCoherent(const SystemAccentRamp &ramp)
{
    if (!ramp.accent.isValid() || !ramp.light1.isValid() || !ramp.light2.isValid()
        || !ramp.dark1.isValid())
        return false;
    // METHODOLOGY §2: the ramp must come from one coherent snapshot — a
    // mixed-source ramp (SystemAccentColor from one probe, Light/Dark
    // entries from a stale one) leaves the hue family.
    return sameHueFamily(ramp.accent, ramp.light1) && sameHueFamily(ramp.accent, ramp.light2)
            && sameHueFamily(ramp.accent, ramp.dark1);
}

void WinUI3ThemeThreadTest::concurrentRampReadAndInvalidate()
{
    struct Probe
    {
        QMutex mutex;
        bool incoherent = false;
        int reads = 0;
        QColor firstAccent;
        QColor lastAccent;
    };
    Probe probe;

    std::atomic<bool> stop{ false };
    QThread *worker = QThread::create([&probe, &stop] {
        bool hasFirst = false;
        while (!stop.load()) {
            const SystemAccentRamp ramp = systemAccentRamp();
            const QMutexLocker locker(&probe.mutex);
            ++probe.reads;
            if (!rampIsCoherent(ramp)) {
                probe.incoherent = true;
                return;
            }
            if (!hasFirst) {
                probe.firstAccent = ramp.accent;
                hasFirst = true;
            }
            probe.lastAccent = ramp.accent;
        }
    });
    worker->start();

    // Churn the cache from the GUI thread while the worker reads: this is
    // the watcher's invalidation path racing a concurrent probe.
    for (int i = 0; i < 300; ++i) {
        invalidateSystemAppearanceCache();
        if (i % 10 == 0)
            QTest::qWait(1);
    }
    stop.store(true);
    QVERIFY(worker->wait(10000));
    delete worker;

    QVERIFY(probe.reads > 0);
    QVERIFY2(!probe.incoherent, "worker accent-ramp read was incoherent under GUI invalidation");
    QVERIFY(probe.firstAccent.isValid());
    QVERIFY(probe.lastAccent.isValid());
}

void WinUI3ThemeThreadTest::concurrentDarkReadAndInvalidate()
{
    struct Probe
    {
        QMutex mutex;
        int reads = 0;
        bool firstDark = false;
        bool lastDark = false;
        bool hasFirst = false;
    };
    Probe probe;

    std::atomic<bool> stop{ false };
    QThread *worker = QThread::create([&probe, &stop] {
        while (!stop.load()) {
            const bool dark = systemUsesDarkTheme();
            const QMutexLocker locker(&probe.mutex);
            ++probe.reads;
            if (!probe.hasFirst) {
                probe.firstDark = dark;
                probe.hasFirst = true;
            }
            probe.lastDark = dark;
        }
    });
    worker->start();

    for (int i = 0; i < 300; ++i) {
        invalidateSystemAppearanceCache();
        if (i % 10 == 0)
            QTest::qWait(1);
    }
    stop.store(true);
    QVERIFY(worker->wait(10000));
    delete worker;

    // Completion without crashing is the mechanism assertion (the pre-fix
    // data race is undefined behavior, not a deterministic wrong value), and
    // the read count proves the worker actually raced the invalidations.
    QVERIFY(probe.reads > 0);
    QVERIFY(probe.hasFirst);
    // Token-level coherence on the same state: a settled read matches the
    // GUI thread's own settled read — the cache converges instead of
    // sticking on a torn or permanently stale value.
    invalidateSystemAppearanceCache();
    QCOMPARE(systemUsesDarkTheme(), probe.lastDark);
}

QTEST_MAIN(WinUI3ThemeThreadTest)
#include "tst_winui3theme.moc"
