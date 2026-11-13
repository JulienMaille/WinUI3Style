// SPDX-License-Identifier: LGPL-2.1-or-later
#include "winui3appearancewatcher_p.h"

#include <QCoreApplication>
#include <QPointer>
#include <QThread>
#include <QTimer>

#include <utility>

#ifdef Q_OS_WIN
#  define NOMINMAX
#  include <windows.h>

// The Win32 SDK spells this notification ...COLORCHANGED. Keep the shorter
// name used by the appearance contract as a local alias for SDK portability.
#  if !defined(WM_DWMCOLORIZATIONCOLORCHANGE) && defined(WM_DWMCOLORIZATIONCOLORCHANGED)
#    define WM_DWMCOLORIZATIONCOLORCHANGE WM_DWMCOLORIZATIONCOLORCHANGED
#  endif
// Older MinGW w32api headers (e.g. Qt 5.15 MinGW builds) expose neither
// spelling; fall back to the documented message value 0x0320.
#  ifndef WM_DWMCOLORIZATIONCOLORCHANGE
#    define WM_DWMCOLORIZATIONCOLORCHANGE 0x0320
#  endif
#endif

namespace WinUI3::Private {

SystemAppearanceWatcher::SystemAppearanceWatcher(QObject *context, Callback callback)
    : QObject(context), m_callback(std::move(callback))
{
#ifdef Q_OS_WIN
    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(debounceIntervalMs);

    // Use the context as the connection context as well as the QObject
    // parent. If it is null, this object still has a well-defined lifetime
    // context and the callback can run until the watcher is destroyed.
    QObject *callbackContext = context ? context : this;
    QObject::connect(m_debounceTimer, &QTimer::timeout, callbackContext, [this] {
        // Keep a local copy so a callback may safely delete
        // the watcher (or its context) without this lambda
        // touching the destroyed object afterward.
        Callback callback = m_callback;
        if (callback)
            callback();
    });

    setActive(true);
#else
    Q_UNUSED(context);
#endif
}

SystemAppearanceWatcher::SystemAppearanceWatcher(Callback callback, QObject *context)
    : SystemAppearanceWatcher(context, std::move(callback))
{
}

SystemAppearanceWatcher::~SystemAppearanceWatcher()
{
    uninstallNow();
    m_active = false;
}

void SystemAppearanceWatcher::setActive(bool active)
{
    m_active = active;
#ifdef Q_OS_WIN
    if (active)
        ensureInstalled();
    else
        uninstallNow();
#else
    Q_UNUSED(active);
#endif
}

void SystemAppearanceWatcher::ensureInstalled()
{
#ifdef Q_OS_WIN
    if (!m_active || m_installed)
        return;
    // Native filters live on the GUI thread. Retrying from a worker thread
    // would install on the wrong thread, so defer until the GUI thread can
    // perform the install.
    if (QCoreApplication *existing = QCoreApplication::instance()) {
        if (QThread::currentThread() != existing->thread()) {
            // Guard the watcher lifetime: the functor carries no QObject
            // context (it must run on the application's thread, not the
            // watcher's), so a QPointer bails out if the watcher is
            // destroyed before the queued delivery runs.
            QPointer<SystemAppearanceWatcher> guard(this);
            QMetaObject::invokeMethod(
                    existing,
                    [guard] {
                        if (guard)
                            guard->ensureInstalled();
                    },
                    Qt::QueuedConnection);
            return;
        }
    }
    QCoreApplication *application = QCoreApplication::instance();
    if (!application) {
        // Constructed before QApplication exists: retry once the application
        // object's event loop can deliver. This must be a queued invocation,
        // not a QTimer::singleShot: starting a timer needs an event
        // dispatcher, which does not exist yet before the first application
        // object is constructed, so the single-shot would silently die. A
        // queued call is parked in the thread's posted-event list and runs
        // after the new application's constructor completes, so instance()
        // is valid there. Same-thread destruction is safe: QObject's
        // destructor removes its own posted events.
        QMetaObject::invokeMethod(this, [this] { ensureInstalled(); }, Qt::QueuedConnection);
        return;
    }
    if (m_hostApplication && m_hostApplication != application) {
        // A previous host was torn down without uninstalling (see
        // uninstallNow): never forward that stale filter into the new app.
        m_installed = false;
        m_hostApplication.clear();
    }
    application->installNativeEventFilter(this);
    m_installed = true;
    m_hostApplication = application;
    // If this host application is destroyed while the watcher outlives it,
    // clear the install state so a later QApplication gets a fresh install
    // (QObject::destroyed carries the host as sender; this as receiver
    // keeps delivery on the right thread and auto-disconnects). The
    // connection is tracked so uninstallNow() can disconnect it: without
    // that, repeated activate/deactivate cycles would accumulate one
    // destroyed() handler per cycle on a long-lived application.
    QObject::disconnect(m_hostDestroyedConnection);
    m_hostDestroyedConnection = connect(application, &QObject::destroyed, this,
                                        [this] { onHostApplicationDestroyed(); });
#endif
}

void SystemAppearanceWatcher::onHostApplicationDestroyed()
{
    m_installed = false;
    m_hostApplication.clear();
    m_hostDestroyedConnection = QMetaObject::Connection{};
}

void SystemAppearanceWatcher::uninstallNow()
{
#ifdef Q_OS_WIN
    if (m_debounceTimer)
        m_debounceTimer->stop();
    // Clear the installed flag even when no application exists anymore: the
    // host QPointer auto-nulls on QCoreApplication destruction, but an
    // attempt to remove the filter from a null instance would otherwise
    // leave m_installed stale and leak the expectation of a filter into a
    // future QApplication. Disconnecting the tracked teardown handler keeps
    // activate/deactivate cycles from accumulating duplicate destroyed()
    // handlers on a long-lived application.
    m_installed = false;
    QObject::disconnect(m_hostDestroyedConnection);
    m_hostDestroyedConnection = QMetaObject::Connection{};
    if (m_hostApplication) {
        m_hostApplication->removeNativeEventFilter(this);
        m_hostApplication.clear();
    } else if (QCoreApplication *application = QCoreApplication::instance()) {
        application->removeNativeEventFilter(this);
    }
#endif
}

bool SystemAppearanceWatcher::nativeEventFilter(const QByteArray &eventType, void *message,
                                                NativeMessageResult *result)
{
    Q_UNUSED(eventType);
    Q_UNUSED(result);

#ifdef Q_OS_WIN
    if (!m_active || !message)
        return false;

    const MSG *nativeMessage = static_cast<const MSG *>(message);
    switch (nativeMessage->message) {
    case WM_SETTINGCHANGE:
    case WM_SYSCOLORCHANGE:
    case WM_THEMECHANGED:
    case WM_DWMCOLORIZATIONCOLORCHANGE:
        scheduleNotification();
        break;
    default:
        break;
    }
#else
    Q_UNUSED(message);
#endif

    // This filter observes the message but deliberately leaves dispatch to
    // Qt and the application.
    return false;
}

void SystemAppearanceWatcher::scheduleNotification()
{
#ifdef Q_OS_WIN
    if (m_debounceTimer)
        m_debounceTimer->start();
#endif
}

} // namespace WinUI3::Private
