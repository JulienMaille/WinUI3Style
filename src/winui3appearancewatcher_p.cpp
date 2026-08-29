#include "winui3appearancewatcher_p.h"

#include <QCoreApplication>
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

SystemAppearanceWatcher::SystemAppearanceWatcher(QObject *context,
                                                 Callback callback)
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
    QObject::connect(m_debounceTimer, &QTimer::timeout, callbackContext,
                     [this] {
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

SystemAppearanceWatcher::SystemAppearanceWatcher(Callback callback,
                                                 QObject *context)
    : SystemAppearanceWatcher(context, std::move(callback))
{
}

SystemAppearanceWatcher::~SystemAppearanceWatcher()
{
    setActive(false);
}

void SystemAppearanceWatcher::setActive(bool active)
{
    m_active = active;
#ifdef Q_OS_WIN
    QCoreApplication *application = QCoreApplication::instance();
    if (active && application && !m_installed) {
        application->installNativeEventFilter(this);
        m_installed = true;
    } else if (!active && application && m_installed) {
        if (m_debounceTimer)
            m_debounceTimer->stop();
        application->removeNativeEventFilter(this);
        m_installed = false;
    }
#endif
}

bool SystemAppearanceWatcher::nativeEventFilter(const QByteArray &eventType,
                                                void *message,
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
