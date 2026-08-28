#include "winui3appearancewatcher_p.h"

#include <QTest>

#ifdef Q_OS_WIN
#  define NOMINMAX
#  include <windows.h>

#  ifndef WM_DWMCOLORIZATIONCOLORCHANGE
#    define WM_DWMCOLORIZATIONCOLORCHANGE WM_DWMCOLORIZATIONCOLORCHANGED
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
};

void WinUI3AppearanceWatcherTest::nativeFilterAlwaysPassesTheMessageOn()
{
    SystemAppearanceWatcher watcher(this, [] {});
#ifdef Q_OS_WIN
    MSG message{};
    message.message = WM_SETTINGCHANGE;
    QVERIFY(!watcher.nativeEventFilter(QByteArrayLiteral("windows_generic_MSG"),
                                       &message, nullptr));
#else
    QVERIFY(!watcher.nativeEventFilter(QByteArrayLiteral("non-windows"),
                                       nullptr, nullptr));
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
    QVERIFY(!watcher.nativeEventFilter(
        QByteArrayLiteral("windows_generic_MSG"), &message, nullptr));
    QTest::qWait(SystemAppearanceWatcher::debounceIntervalMs + 25);
    QCOMPARE(callbackCount, 0);

    watcher.setActive(true);
    QVERIFY(watcher.isActive());
    QVERIFY(!watcher.nativeEventFilter(
        QByteArrayLiteral("windows_generic_MSG"), &message, nullptr));
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
        QVERIFY(!watcher.nativeEventFilter(
            QByteArrayLiteral("windows_generic_MSG"), &message, nullptr));
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
    auto *watcher = new SystemAppearanceWatcher(
        context, [&callbackCount] { ++callbackCount; });

#ifdef Q_OS_WIN
    MSG message{};
    message.message = WM_THEMECHANGED;
    QVERIFY(!watcher->nativeEventFilter(
        QByteArrayLiteral("windows_generic_MSG"), &message, nullptr));
#endif

    delete context;
    QTest::qWait(100);
    QCOMPARE(callbackCount, 0);
}

QTEST_MAIN(WinUI3AppearanceWatcherTest)
#include "tst_winui3appearancewatcher.moc"
