#pragma once

// Qt 5 / Qt 6 compatibility shims for WinUI3Style internals.
//
// Qt 6 renamed several ubiquitous APIs (QMouseEvent::pos() -> position(),
// added static QFontDatabase accessors, qintptr in
// QAbstractNativeEventFilter, ...). These helpers keep the style
// implementation written against the Qt 6 surface compilable under Qt 5.15,
// which is required to load the style plugin into Qt 5 applications such as
// SoulseekQt.

#include <QtGlobal>

#include <QIcon>
#include <QMouseEvent>
#include <QPixmap>
#include <QScreen>
#include <QSize>
#include <QPoint>
#include <QPointF>
#include <QWidget>
#include <QApplication>
#include <QWindow>

namespace WinUI3::Private {

// QMouseEvent::position() / globalPosition() replacement (Qt 6 API spelled
// with Qt 5 fallbacks).
inline QPointF mousePosition(const QMouseEvent *e)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return e->position();
#else
    return e->localPos();
#endif
}

inline QPoint mousePositionPoint(const QMouseEvent *e)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return e->position().toPoint();
#else
    return e->pos();
#endif
}

inline QPointF mouseGlobalPosition(const QMouseEvent *e)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return e->globalPosition();
#else
    return e->globalPos();
#endif
}

// QWidget::screen() is Qt 6 only. On Qt 5 resolve the screen from the
// widget's window handle, falling back to the screen under its center.
inline QScreen *widgetScreen(const QWidget *widget)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return widget ? widget->screen() : nullptr;
#else
    if (!widget)
        return nullptr;
    if (QWindow *window = widget->windowHandle())
        if (QScreen *screen = window->screen())
            return screen;
    return QApplication::screenAt(widget->geometry().center());
#endif
}

// QIcon::pixmap(size, dpr, mode, state) exists from Qt 6. On Qt 5 request
// the device-resolution pixmap and pin its DPR instead.
inline QPixmap iconPixmap(const QIcon &icon, const QSize &size, qreal dpr,
                          QIcon::Mode mode, QIcon::State state)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return icon.pixmap(size, dpr, mode, state);
#else
    const qreal factor = qMax<qreal>(dpr, 1.0);
    QPixmap pixmap = icon.pixmap(size * factor, mode, state);
    pixmap.setDevicePixelRatio(factor);
    return pixmap;
#endif
}

} // namespace WinUI3::Private

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
// Qt 5 does not register QMargins with the meta-object system, which breaks
// QVariant value<T> extraction for the stored-margins widget properties.
#  include <QMargins>
#  include <QMetaType>
Q_DECLARE_METATYPE(QMargins)
#endif
