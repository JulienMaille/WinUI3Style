#pragma once

#include <QByteArray>
#include <QEasingCurve>
#include <QHash>
#include <QMetaObject>
#include <QPointer>
#include <QPair>
#include <QVariant>
#include <QVector>

#include <limits>

class QObject;
class QVariantAnimation;
class QWidget;

namespace WinUI3::Private {

// Drives transient frame values without putting them in QObject's dynamic
// property bag.  The driver owns one QVariantAnimation per widget/key and
// writes each valueChanged frame through FramePropertyRegistry.
class FrameAnimationDriver final
{
public:
    explicit FrameAnimationDriver(QObject *context);
    ~FrameAnimationDriver();

    void animate(QWidget *widget, const char *property, qreal target,
                 int duration, bool allowed, const QEasingCurve &curve,
                 const QVector<QPair<qreal, QVariant>> &keyValues = {},
                 qreal startOverride = std::numeric_limits<qreal>::quiet_NaN());
    void stop(QWidget *widget);

private:
    QVariantAnimation *find(QWidget *widget, const char *property) const;
    void forget(QWidget *widget, const QByteArray &property,
                QVariantAnimation *expected);
    QVariantAnimation *ensure(QWidget *widget, const char *property);

    QObject *m_context = nullptr;
    QHash<QWidget *, QHash<QByteArray, QPointer<QVariantAnimation>>> m_animations;
    QHash<QWidget *, QMetaObject::Connection> m_cleanupConnections;
};

} // namespace WinUI3::Private
