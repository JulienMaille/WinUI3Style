// SPDX-License-Identifier: LGPL-2.1-or-later
// Mechanism regression tests for the Fluent icon font cache
// (spec/METHODOLOGY.md section 6: reproduce the mechanism, add a failing
// test, then fix). Two defects in src/winui3icons.cpp: the worker-thread
// paint fallback mutated the GUI-thread-only IconRuntime (QPointer,
// QString, bool, QCache), and IconRuntime::fluentFont() returned a
// cache-owned const QFont& that invalidate()/clear() or LRU eviction could
// delete out from under the caller.
#include "winui3icons_p.h"

#include <winui3style/winui3icons.h>

#include <QApplication>
#include <QFont>
#include <QThread>
#include <QtTest>

namespace {

class OffThreadProbe final : public QThread
{
public:
    QFont font;
    bool resolvedAfter = false;

    void run() override
    {
        // Mirrors the worker-thread paint fallback precisely: resolve the
        // Fluent font without GUI-thread cache access, then observe whether
        // the shared IconRuntime was mutated from this thread.
        font = WinUI3::Private::offThreadFluentFontForTest(16);
        resolvedAfter = WinUI3::Private::iconFontCacheResolvedForTest();
    }
};

} // namespace

class WinUI3IconsTest final : public QObject
{
    Q_OBJECT

private slots:
    void cachedFontSurvivesInvalidate();
    void workerThreadResolutionIsIsolated();
};

void WinUI3IconsTest::cachedFontSurvivesInvalidate()
{
    // No-aliasing contract: two successive lookups must not expose the same
    // mutable cache slot. Under the old const QFont& signature both binds
    // aliased one QCache-owned object, so a later invalidate()/clear() or
    // LRU eviction deleted it from under every outstanding reference. The
    // by-value fix hands each caller its own implicitly-shared copy instead.
    // (const-ref binding compiles against both signatures: pre-fix it binds
    // the cache slot, post-fix it lifetime-extends a distinct temporary.)
    const QFont &firstRef = WinUI3::Private::fluentFontForTest(16);
    const QFont &secondRef = WinUI3::Private::fluentFontForTest(16);
    QVERIFY2(&firstRef != &secondRef, "cached font escapes as a shared reference");

    // Copy-by-value contract: the returned font must stay usable after the
    // cache that produced it is cleared. Pre-fix, flushing the QCache
    // deleted the object a live reference still pointed at (use-after-free
    // on the next read); post-fix the copy keeps its shared data alive.
    const QFont before = WinUI3::Private::fluentFontForTest(16);
    QCOMPARE(before.pixelSize(), 16);
    const QString familyBefore = before.family();

    WinUI3::Private::invalidateIconCachesForTest();

    // Still owns a valid font after the cache was dropped: the value copy
    // kept the implicitly-shared data alive.
    QCOMPARE(before.pixelSize(), 16);
    QCOMPARE(before.family(), familyBefore);

    // The cache genuinely dropped its entries: the next lookup rebuilds
    // with identical observable behavior.
    const QFont after = WinUI3::Private::fluentFontForTest(16);
    QCOMPARE(after.pixelSize(), 16);
    QCOMPARE(after.family(), familyBefore);
}

void WinUI3IconsTest::workerThreadResolutionIsIsolated()
{
    // GUI-thread baseline: resolve once so the shared runtime is populated.
    WinUI3::Private::invalidateIconCachesForTest();
    const QFont gui = WinUI3::Private::fluentFontForTest(16);
    QCOMPARE(gui.pixelSize(), 16);
    QVERIFY(WinUI3::Private::iconFontCacheResolvedForTest());

    OffThreadProbe probe;
    probe.start();
    QVERIFY(probe.wait(10000));
    // The worker thread produced the requested Fluent family without the GUI
    // database query path. Do not compare QFont::family() with the GUI font:
    // on hosts without Segoe Fluent Icons the GUI cache deliberately requests
    // the MDL2 fallback, while the off-thread path leaves fallback resolution
    // to Qt when the font is used.
    QCOMPARE(probe.font.pixelSize(), 16);
    QCOMPARE(probe.font.family(), QStringLiteral("Segoe Fluent Icons"));
    // ...and without mutating the shared runtime from off-thread. Under the
    // old fallback (iconRuntime().fluentFontFamily() from a worker thread)
    // this resolution wrote m_fontFamilyResolved on the shared cache. The
    // probe observes no new mutation: invalidate first, then require the
    // worker pass to leave the resolved flag untouched.
    WinUI3::Private::invalidateIconCachesForTest();
    QVERIFY(!WinUI3::Private::iconFontCacheResolvedForTest());
    OffThreadProbe isolated;
    isolated.start();
    QVERIFY(isolated.wait(10000));
    QCOMPARE(isolated.font.family(), probe.font.family());
    QVERIFY(!isolated.resolvedAfter);
    QVERIFY(!WinUI3::Private::iconFontCacheResolvedForTest());
}

QTEST_MAIN(WinUI3IconsTest)
#include "tst_winui3icons.moc"
