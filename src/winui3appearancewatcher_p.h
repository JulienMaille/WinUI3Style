// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <QAbstractNativeEventFilter>
#include <QMetaObject>
#include <QObject>
#include <QPointer>

#include <functional>

class QCoreApplication;
class QTimer;

// QAbstractNativeEventFilter changed its result type from long to qintptr in
// Qt 6. Keep the signature portable so the Qt 5 build also overrides.
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
using NativeMessageResult = qintptr;
#else
using NativeMessageResult = long;
#endif

namespace WinUI3::Private {

// Bridges the native Windows appearance notifications to a Qt callback. The
// callback is coalesced because Windows commonly emits several notifications
// for one Settings change. On non-Windows platforms this remains a QObject
// that never installs a native filter and nativeEventFilter() always returns
// false.
class SystemAppearanceWatcher final : public QObject, public QAbstractNativeEventFilter
{
public:
    using Callback = std::function<void()>;

    // The context owns the watcher and is also the lifetime context for the
    // callback. A null context is accepted for consistency with QObject APIs,
    // but callers should normally pass the object whose state the callback
    // reads or updates.
    explicit SystemAppearanceWatcher(QObject *context, Callback callback);
    explicit SystemAppearanceWatcher(Callback callback, QObject *context);
    ~SystemAppearanceWatcher() override;

    void setActive(bool active);
    bool isActive() const { return m_active; }
    bool isInstalled() const { return m_installed; }

    bool nativeEventFilter(const QByteArray &eventType, void *message,
                           NativeMessageResult *result) override;

    static constexpr int debounceIntervalMs = 75;

private:
    void ensureInstalled();
    void uninstallNow();
    void onHostApplicationDestroyed();
    void scheduleNotification();

    QTimer *m_debounceTimer = nullptr;
    Callback m_callback;
    bool m_active = false;
    bool m_installed = false;
    // Tracks the application that owns the installed filter so a stale
    // install cannot linger across application teardown (QPointer auto-nulls
    // when the application object is destroyed).
    QPointer<QCoreApplication> m_hostApplication;
    // The host-teardown connection is disconnected on every uninstall so
    // repeated activate/deactivate cycles cannot accumulate duplicate
    // destroyed() handlers on a long-lived application.
    QMetaObject::Connection m_hostDestroyedConnection;
};

} // namespace WinUI3::Private
