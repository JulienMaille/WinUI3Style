#pragma once

#include <winui3style/winui3global.h>

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QMetaObject>
#include <QVariant>

class QObject;

namespace WinUI3::Private {

// A small, private store for transient per-widget frame state.
//
// Qt dynamic properties are deliberately not used here.  The registry owns
// only QVariant values and weakly associates them with QObject addresses;
// entries are removed when the object emits destroyed().  The store is
// bounded so a caller that continually creates objects or property names
// cannot make it grow without limit.
//
// All access is GUI-thread-only.  This is intentional: the registry is read
// from widget paint paths, so it has no mutex.  Callers must not use it from a
// worker thread; debug builds assert this contract and release builds reject
// off-thread accesses without touching the shared state.
class WINUI3STYLE_EXPORT FramePropertyRegistry final
{
public:
    static FramePropertyRegistry &instance();

    QVariant value(const QObject *object, const QByteArray &name) const;
    qreal real(const QObject *object, const QByteArray &name,
               qreal fallback = 0.0) const;

    void set(QObject *object, const QByteArray &name, const QVariant &value);
    void clear(QObject *object, const QByteArray &name);
    void clearObject(QObject *object);

private:
    struct ObjectState
    {
        QHash<QByteArray, QVariant> values;
        QList<QByteArray> insertionOrder;
        QMetaObject::Connection destroyedConnection;
    };

    FramePropertyRegistry() = default;

    ObjectState *ensureObject(QObject *object);
    void removeObject(QObject *object, bool disconnectDestroyedSignal);
    void trimObject(ObjectState *state);

    // These limits are intentionally conservative.  Frame state consists of
    // a handful of scalar values in normal use, while the limits keep both
    // memory use and cleanup work bounded under hostile/churn-heavy input.
    static constexpr qsizetype MaxObjects = 2048;
    static constexpr qsizetype MaxPropertiesPerObject = 64;

    QHash<QObject *, ObjectState> m_objects;
    QList<QObject *> m_objectInsertionOrder;
};

inline FramePropertyRegistry &framePropertyRegistry()
{
    return FramePropertyRegistry::instance();
}

} // namespace WinUI3::Private
