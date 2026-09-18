#include "winui3appearancewatcher_p.h"

#include <QPointer>
#include <QSignalSpy>
#include <QTest>

#ifdef Q_OS_WIN
#  define NOMINMAX
#  include <windows.h>

#  if !defined(WM_DWMCOLORIZATIONCOLORCHANGE) && defined(WM_DWMCOLORIZATIONCOLORCHANGED)
#    define WM_DWMCOLORIZATIONCOLORCHANGE WM_DWMCOLORIZATIONCOLORCHANGED
#  endif
#  ifndef WM_DWMCOLORIZATIONCOLORCHANGE
#    define WM_DWMCOLORIZATIONCOLORCHANGE 0x0320
#  endif
#endif

using WinUI3::Private::SystemAppearanceWatcher;

class WinUI3AppearanceWatcherTest final : public QObject
{
    Q_OBJECT

private slots:
    void nativeFilterAlwaysPassesTheMessageOn();
    void appearanceMessagesAreDebounced();
    void inactiveWatcherIgnoresMessages();
    void callbackContextCancelsPendingNotification();
    void activeStateSurvivesRepeatedToggles();
    void destroyedAfterHostGoneLeavesNoStaleInstall();
};

void WinUI3AppearanceWatcherTest::nativeFilterAlwaysPassesTheMessageOn()
{
    SystemAppearanceWatcher watcher(this, [] {});
#ifdef Q_OS_WIN
    MSG message{};
    message.message = WM_SETTINGCHANGE;
    QVERIFY(!watcher.nativeEventFilter(QByteArrayLiteral("windows_generic_MSG"), &message,
                                       nullptr));
#else
    QVERIFY(!watcher.nativeEventFilter(QByteArrayLiteral("non-windows"), nullptr, nullptr));
#endif
}

void WinUI3AppearanceWatcherTest::inactiveWatcherIgnoresMessages()
{
    int callbackCount = 0;
    SystemAppearanceWatcher watcher(this, [&callbackCount] { ++callbackCount; });
    watcher.setActive(false);
    QVERIFY(!watcher.isActive());

#ifdef Q_OS_WIN
    MSG message{};
    message.message = WM_THEMECHANGED;
    QVERIFY(!watcher.nativeEventFilter(QByteArrayLiteral("windows_generic_MSG"), &message,
                                       nullptr));
    QTest::qWait(SystemAppearanceWatcher::debounceIntervalMs + 25);
    QCOMPARE(callbackCount, 0);

    watcher.setActive(true);
    QVERIFY(watcher.isActive());
    QVERIFY(!watcher.nativeEventFilter(QByteArrayLiteral("windows_generic_MSG"), &message,
                                       nullptr));
    QTRY_COMPARE_WITH_TIMEOUT(callbackCount, 1, 500);
#else
    QTest::qWait(100);
    QCOMPARE(callbackCount, 0);
#endif
}

void WinUI3AppearanceWatcherTest::appearanceMessagesAreDebounced()
{
    int callbackCount = 0;
    SystemAppearanceWatcher watcher(this, [&callbackCount] { ++callbackCount; });

#ifdef Q_OS_WIN
    MSG message{};
    const UINT appearanceMessages[] = {
        WM_SETTINGCHANGE,
        WM_SYSCOLORCHANGE,
        WM_THEMECHANGED,
        WM_DWMCOLORIZATIONCOLORCHANGE,
    };
    for (const UINT messageId : appearanceMessages) {
        message.message = messageId;
        QVERIFY(!watcher.nativeEventFilter(QByteArrayLiteral("windows_generic_MSG"), &message,
                                           nullptr));
    }
    QTRY_COMPARE_WITH_TIMEOUT(callbackCount, 1, 500);
#else
    QTest::qWait(100);
    QCOMPARE(callbackCount, 0);
#endif
}

void WinUI3AppearanceWatcherTest::callbackContextCancelsPendingNotification()
{
    int callbackCount = 0;
    auto *context = new QObject;
    auto *watcher = new SystemAppearanceWatcher(context, [&callbackCount] { ++callbackCount; });

#ifdef Q_OS_WIN
    MSG message{};
    message.message = WM_THEMECHANGED;
    QVERIFY(!watcher->nativeEventFilter(QByteArrayLiteral("windows_generic_MSG"), &message,
                                        nullptr));
#endif

    delete context;
    QTest::qWait(100);
    QCOMPARE(callbackCount, 0);
}

void WinUI3AppearanceWatcherTest::activeStateSurvivesRepeatedToggles()
{
    // RAII/install-refresh mechanism: repeated activate/deactivate cycles
    // must not accumulate stale teardown state. isInstalled() is the
    // observable side of the tracking; on Windows it also reflects the
    // native filter install.
    SystemAppearanceWatcher watcher(this, [] {});
    QVERIFY(watcher.isActive());
    for (int i = 0; i < 5; ++i) {
        watcher.setActive(false);
        QVERIFY(!watcher.isActive());
#ifdef Q_OS_WIN
        QVERIFY(!watcher.isInstalled());
#endif
        watcher.setActive(true);
        QVERIFY(watcher.isActive());
#ifdef Q_OS_WIN
        QVERIFY(watcher.isInstalled());
#endif
    }
    // No duplicate teardown handlers: deactivation still leaves exactly the
    // uninstalled state, and the watcher keeps passing messages on.
    watcher.setActive(false);
    QVERIFY(!watcher.isActive());
#ifdef Q_OS_WIN
    QVERIFY(!watcher.isInstalled());
    MSG message{};
    message.message = WM_THEMECHANGED;
    QVERIFY(!watcher.nativeEventFilter(QByteArrayLiteral("windows_generic_MSG"), &message,
                                       nullptr));
#endif
}

void WinUI3AppearanceWatcherTest::destroyedAfterHostGoneLeavesNoStaleInstall()
{
    // The in-tree QApplication outlives the watcher, so simulate "host gone"
    // by deactivating first (which clears the install tracking), then
    // verify destruction leaves no stale installed state behind. On Windows
    // the destructor's uninstallNow() must be a safe no-op after the filter
    // was already removed; off-Windows the watcher never installs.
    auto *watcher = new SystemAppearanceWatcher(this, [] {});
    QVERIFY(watcher->isActive());
    watcher->setActive(false);
    QVERIFY(!watcher->isActive());
#ifdef Q_OS_WIN
    QVERIFY(!watcher->isInstalled());
#endif
    QPointer<SystemAppearanceWatcher> guard(watcher);
    QSignalSpy destroyedSpy(watcher, &QObject::destroyed);
    delete watcher;
    QCOMPARE(destroyedSpy.count(), 1);
    QVERIFY(guard.isNull());

    // A fresh watcher starts clean: no stale filter expectation leaks from
    // the destroyed one into the still-running application.
    SystemAppearanceWatcher successor(this, [] {});
    QVERIFY(successor.isActive());
    QVERIFY(!successor.nativeEventFilter(QByteArrayLiteral("non-windows"), nullptr, nullptr));
}

QTEST_MAIN(WinUI3AppearanceWatcherTest)
#include "tst_winui3appearancewatcher.moc"
