#include "winui3appearancewatcher_p.h"

#include <QCoreApplication>
#include <QTimer>

#include <utility>

#ifdef Q_OS_WIN
#  define NOMINMAX
#  include <windows.h>

// The Win32 SDK spells this notification ...COLORCHANGED. Keep the shorter
// name used by the appearance contract as a local alias for SDK portability.
#  ifndef WM_DWMCOLORIZATIONCOLORCHANGE
#    define WM_DWMCOLORIZATIONCOLORCHANGE WM_DWMCOLORIZATIONCOLORCHANGED
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

    if (QCoreApplication *application = QCoreApplication::instance())
        application->installNativeEventFilter(this);
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
#ifdef Q_OS_WIN
    if (QCoreApplication *application = QCoreApplication::instance())
        application->removeNativeEventFilter(this);
#endif
}

bool SystemAppearanceWatcher::nativeEventFilter(const QByteArray &eventType,
                                                void *message,
                                                qintptr *result)
{
    Q_UNUSED(eventType);
    Q_UNUSED(result);

#ifdef Q_OS_WIN
    if (!message)
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
