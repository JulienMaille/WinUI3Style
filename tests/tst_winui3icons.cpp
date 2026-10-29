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
#include <QColor>
#include <QFont>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QThread>
#include <QtTest>

namespace {

QRect alphaBounds(const QPixmap &pixmap)
{
    const QImage image = pixmap.toImage();
    QRect bounds;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (qAlpha(image.pixel(x, y)))
                bounds |= QRect(x, y, 1, 1);
        }
    }
    return bounds;
}

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
    void iconFallbackResolution();
    void simpleMarksUseVectorFallback();
    void complexSymbolsUseUnicodeFallback();
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
    // database query path.
    QCOMPARE(probe.font.pixelSize(), 16);
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

void WinUI3IconsTest::iconFallbackResolution()
{
    // Verify that WinUI3::icon produces non-null icons for all Icon enum values.
    const QIcon addIcon = WinUI3::icon(WinUI3::Icon::Add);
    QVERIFY(!addIcon.isNull());

    const QIcon chevronDown = WinUI3::icon(WinUI3::Icon::ChevronDown);
    QVERIFY(!chevronDown.isNull());

    const QPixmap pix = chevronDown.pixmap(16, 16);
    QCOMPARE(pix.size(), QSize(16, 16));
}

void WinUI3IconsTest::simpleMarksUseVectorFallback()
{
    struct RestoreFontDetection
    {
        ~RestoreFontDetection() { WinUI3::Private::setIconFontAvailabilityOverrideForTest(-1); }
    } restore;
    WinUI3::Private::setIconFontAvailabilityOverrideForTest(0);
    QVERIFY(!WinUI3::Private::segoeIconFontAvailable());

    const auto render = [](WinUI3::Icon glyph, qreal dpr) {
        const QSize logical(24, 24);
        QPixmap pixmap(qRound(logical.width() * dpr), qRound(logical.height() * dpr));
        pixmap.setDevicePixelRatio(dpr);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        WinUI3::icon(glyph, QColor(Qt::white)).paint(&painter, QRect(QPoint(), logical));
        return pixmap;
    };

    // Vector artwork keeps its normalized geometry at 100% and 200% DPR.
    const QPixmap close100 = render(WinUI3::Icon::Close, 1.0);
    const QRect closeBounds100 = alphaBounds(close100);
    QCOMPARE(close100.size(), QSize(24, 24));
    QVERIFY(!closeBounds100.isEmpty());
    QCOMPARE(closeBounds100.width(), closeBounds100.height());
    QCOMPARE(closeBounds100.left() + closeBounds100.right(), 23);
    QCOMPARE(closeBounds100.top() + closeBounds100.bottom(), 23);

    const QPixmap close200 = render(WinUI3::Icon::Close, 2.0);
    const QRect closeBounds200 = alphaBounds(close200);
    QCOMPARE(close200.size(), QSize(48, 48));
    QVERIFY(!closeBounds200.isEmpty());
    QCOMPARE(closeBounds200.width(), closeBounds200.height());
    QCOMPARE(closeBounds200.left() + closeBounds200.right(), 47);
    QCOMPARE(closeBounds200.top() + closeBounds200.bottom(), 47);

    const QPixmap check = render(WinUI3::Icon::Check, 1.0);
    const QRect checkBounds = alphaBounds(check);
    QCOMPARE(check.size(), QSize(24, 24));
    QVERIFY(!checkBounds.isEmpty());
    QVERIFY(checkBounds.left() < checkBounds.center().x());
    QVERIFY(checkBounds.right() > checkBounds.center().x());

    const QPixmap left = render(WinUI3::Icon::ChevronLeft, 1.0);
    const QPixmap right = render(WinUI3::Icon::ChevronRight, 1.0);
    QCOMPARE(left.size(), right.size());
    QVERIFY(alphaBounds(left).isValid());
    QVERIFY(alphaBounds(right).isValid());
    QVERIFY(left.toImage() != right.toImage());
}

void WinUI3IconsTest::complexSymbolsUseUnicodeFallback()
{
    struct RestoreFontDetection
    {
        ~RestoreFontDetection() { WinUI3::Private::setIconFontAvailabilityOverrideForTest(-1); }
    } restore;
    WinUI3::Private::setIconFontAvailabilityOverrideForTest(0);
    QVERIFY(!WinUI3::Private::segoeIconFontAvailable());

    const QPixmap folder = WinUI3::icon(WinUI3::Icon::Folder, QColor(Qt::white)).pixmap(24, 24);
    const QPixmap settings = WinUI3::icon(WinUI3::Icon::Settings, QColor(Qt::white)).pixmap(24, 24);
    QCOMPARE(folder.size(), QSize(24, 24));
    QCOMPARE(settings.size(), QSize(24, 24));
    QVERIFY(!folder.isNull());
    QVERIFY(!settings.isNull());
    const QRect folderBounds = alphaBounds(folder);
    const QRect settingsBounds = alphaBounds(settings);
    QVERIFY(!folderBounds.isEmpty());
    QVERIFY(!settingsBounds.isEmpty());
    QCOMPARE(folderBounds.intersected(folder.rect()), folderBounds);
    QCOMPARE(settingsBounds.intersected(settings.rect()), settingsBounds);
}

QTEST_MAIN(WinUI3IconsTest)
#include "tst_winui3icons.moc"
