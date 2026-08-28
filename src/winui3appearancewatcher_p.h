#pragma once

#include <QAbstractNativeEventFilter>
#include <QObject>

#include <functional>

class QTimer;

namespace WinUI3::Private {

// Bridges the native Windows appearance notifications to a Qt callback. The
// callback is coalesced because Windows commonly emits several notifications
// for one Settings change. On non-Windows platforms this remains a QObject
// that never installs a native filter and nativeEventFilter() always returns
// false.
class SystemAppearanceWatcher final : public QObject,
                                      public QAbstractNativeEventFilter
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

    bool nativeEventFilter(const QByteArray &eventType, void *message,
                           qintptr *result) override;

    static constexpr int debounceIntervalMs = 75;

private:
    void scheduleNotification();

    QTimer *m_debounceTimer = nullptr;
    Callback m_callback;
    bool m_active = false;
    bool m_installed = false;
};

} // namespace WinUI3::Private
