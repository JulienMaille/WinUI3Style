// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

// Qt 5 / Qt 6 compatibility shims for WinUI3Style internals.
//
// Qt 6 renamed several ubiquitous APIs (QMouseEvent::pos() -> position(),
// added static QFontDatabase accessors, qintptr in
// QAbstractNativeEventFilter, ...). These helpers keep the style
// implementation written against the Qt 6 surface compilable under Qt 5.15,
// which is required to load the style plugin into Qt 5 applications.
//
// What lives where (S5):
//   * a bridge needed at more than one call site lives here (below);
//   * a version difference that occurs at a single call site stays there with
//     an inline note — hoisting it would only add indirection. The current
//     single-site adaptations are: the Segoe UI Variable Text font choice
//     (Style::polish), QMouseEvent's pointingDevice() argument (SettingsCard
//     forwarding), QEvent::DevicePixelRatioChange (SettingsCard), the
//     non-const QIconEngine::iconName() (icon engine), QHoverEvent's 4th
//     argument (interaction controller), SP_TabCloseButton (contracts),
//     QStyleHints::colorSchemeChanged, QPalette::Accent, and
//     QCheckBox::checkStateChanged;
//   * Q_OS_WIN blocks are platform gates (DWM, registry, native message
//     hooks), not shims: they stay with the platform code they guard.

#include <QtGlobal>
#include <QtMath>

#include <QIcon>
#include <QMouseEvent>
#include <QPixmap>
#include <QScreen>
#include <QSize>
#include <QPoint>
#include <QPointF>
#include <QAction>
#include <QFontDatabase>
#include <QStringList>
#include <QSizeF>
#include <QWidget>
#include <QApplication>
#include <QWindow>

namespace WinUI3::Private {

// Qt 5 / Qt 6 shims live here when the SAME bridge is needed at more than one
// call site. A version difference that occurs at a single call site stays at
// that call site with an inline note — moving it here would only add
// indirection. Platform gates (Q_OS_WIN: DWM, registry, native message hooks)
// are not shims and stay where the platform code lives.
//
// QFontDatabase::families() became static in Qt 6.
inline QStringList fontFamilies()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return QFontDatabase::families();
#else
    return QFontDatabase().families();
#endif
}

// QPixmap::deviceIndependentSize() was added in Qt 6; on Qt 5 divide by the
// device pixel ratio instead.
inline QSizeF deviceIndependentSize(const QPixmap &pixmap)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return pixmap.deviceIndependentSize();
#else
    const qreal dpr = qMax<qreal>(pixmap.devicePixelRatioF(), 1.0);
    return QSizeF(pixmap.size()) / dpr;
#endif
}

// QAction::associatedObjects() is Qt 6; Qt 5.12 has associatedWidgets().
// Membership only, so widget pointers are compared.
inline bool actionAssociatedWith(const QAction *action, const QWidget *widget)
{
    if (!action || !widget)
        return false;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return action->associatedObjects().contains(const_cast<QWidget *>(widget));
#else
    return action->associatedWidgets().contains(const_cast<QWidget *>(widget));
#endif
}

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
// widget->geometry() is parent-relative for child widgets, but
// QApplication::screenAt() takes a global point: map to global first so
// the screen query sees the widget's real position.
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
    const QPoint globalCenter = widget->mapToGlobal(widget->rect().center());
    return QApplication::screenAt(globalCenter);
#endif
}

// QIcon::pixmap(size, dpr, mode, state) exists from Qt 6. On Qt 5 request
// the device-resolution pixmap and pin its DPR instead. Round the scaled
// size up (qCeil) so fractional DPRs (125%/150%) keep full requested
// coverage; truncating would return a pixmap narrower than the layout slot
// and leave a one-pixel gap or blur.
inline QPixmap iconPixmap(const QIcon &icon, const QSize &size, qreal dpr, QIcon::Mode mode,
                          QIcon::State state)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return icon.pixmap(size, dpr, mode, state);
#else
    const qreal factor = qMax<qreal>(dpr, 1.0);
    const QSize deviceSize(qCeil(size.width() * factor), qCeil(size.height() * factor));
    QPixmap pixmap = icon.pixmap(deviceSize, mode, state);
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
