#include "winui3frameproperties_p.h"

#include <algorithm>
#include <QCoreApplication>
#include <QObject>
#include <QThread>
#include <cstring>
#include <utility>

namespace WinUI3::Private {

namespace {

bool checkGuiThread(const char *operation)
{
    const QCoreApplication *application = QCoreApplication::instance();
    const bool onGuiThread = !application
        || QThread::currentThread() == application->thread();
    Q_ASSERT_X(onGuiThread, "FramePropertyRegistry", operation);
    return onGuiThread;
}

} // namespace

FramePropertyRegistry &FramePropertyRegistry::instance()
{
    // The registry intentionally lives until process exit.  This avoids a
    // static-destruction-order issue with QObject instances that may emit
    // destroyed() while the application is shutting down.
    static auto *registry = new FramePropertyRegistry;
    return *registry;
}

QVariant FramePropertyRegistry::value(const QObject *object,
                                      const QByteArray &name) const
{
    if (!checkGuiThread(Q_FUNC_INFO) || !object || name.isEmpty())
        return {};

    const auto objectIt = m_objects.constFind(const_cast<QObject *>(object));
    if (objectIt == m_objects.cend())
        return {};

    const auto valueIt = objectIt->values.constFind(name);
    return valueIt == objectIt->values.cend() ? QVariant{} : valueIt.value();
}

QVariant FramePropertyRegistry::value(const QObject *object,
                                      const char *staticName) const
{
    if (!staticName)
        return {};
    // Frame keys are private string literals. A non-owning QByteArray avoids
    // a heap allocation on every paint-path lookup while preserving QHash's
    // existing QByteArray hashing and equality semantics.
    const QByteArray name = QByteArray::fromRawData(
        staticName, qsizetype(std::strlen(staticName)));
    return value(object, name);
}

qreal FramePropertyRegistry::real(const QObject *object, const QByteArray &name,
                                  qreal fallback) const
{
    if (!checkGuiThread(Q_FUNC_INFO))
        return fallback;
    const QVariant stored = value(object, name);
    if (!stored.isValid())
        return fallback;

    bool ok = false;
    const qreal converted = stored.toReal(&ok);
    return ok ? converted : fallback;
}

qreal FramePropertyRegistry::real(const QObject *object,
                                  const char *staticName,
                                  qreal fallback) const
{
    const QVariant stored = value(object, staticName);
    if (!stored.isValid())
        return fallback;
    bool ok = false;
    const qreal converted = stored.toReal(&ok);
    return ok ? converted : fallback;
}

FramePropertyRegistry::ObjectState *
FramePropertyRegistry::ensureObject(QObject *object)
{
    if (!object)
        return nullptr;

    auto objectIt = m_objects.find(object);
    if (objectIt != m_objects.end())
        return &objectIt.value();

    while (m_objects.size() >= MaxObjects && !m_objectInsertionOrder.isEmpty()) {
        QObject *oldest = m_objectInsertionOrder.front();
        m_objectInsertionOrder.pop_front();
        removeObject(oldest, true);
    }

    // The order list can only be empty when the hash is empty, but retain the
    // guard so the invariant remains true if this code is changed later.
    if (m_objects.size() >= MaxObjects)
        return nullptr;

    ObjectState state;
    state.destroyedConnection = QObject::connect(
        object, &QObject::destroyed, [this](QObject *destroyedObject) {
            removeObject(destroyedObject, false);
        });

    objectIt = m_objects.insert(object, std::move(state));
    m_objectInsertionOrder.push_back(object);
    return &objectIt.value();
}

void FramePropertyRegistry::trimObject(ObjectState *state)
{
    if (!state)
        return;

    while (state->values.size() > MaxPropertiesPerObject
           && !state->insertionOrder.isEmpty()) {
        const QByteArray oldest = state->insertionOrder.front();
        state->insertionOrder.pop_front();
        state->values.remove(oldest);
    }
}

void FramePropertyRegistry::set(QObject *object, const QByteArray &name,
                                const QVariant &value)
{
    if (!checkGuiThread(Q_FUNC_INFO) || !object || name.isEmpty())
        return;

    // Match QObject::setProperty's useful "invalid means remove" behavior,
    // while keeping clear() available at call sites where intent is clearer.
    if (!value.isValid()) {
        clear(object, name);
        return;
    }

    ObjectState *state = ensureObject(object);
    if (!state)
        return;

    if (!state->values.contains(name))
        state->insertionOrder.push_back(name);
    state->values.insert(name, value);
    trimObject(state);
}

void FramePropertyRegistry::clear(QObject *object, const QByteArray &name)
{
    if (!checkGuiThread(Q_FUNC_INFO) || !object || name.isEmpty())
        return;

    auto objectIt = m_objects.find(object);
    if (objectIt == m_objects.end())
        return;

    ObjectState &state = objectIt.value();
    if (state.values.remove(name) == 0)
        return;

    const auto orderIt = std::find(state.insertionOrder.cbegin(),
                                   state.insertionOrder.cend(), name);
    if (orderIt != state.insertionOrder.cend())
        state.insertionOrder.erase(orderIt);

    if (state.values.isEmpty())
        removeObject(object, true);
}

void FramePropertyRegistry::removeObject(QObject *object,
                                          bool disconnectDestroyedSignal)
{
    if (!object)
        return;

    const auto objectIt = m_objects.find(object);
    if (objectIt == m_objects.end())
        return;

    if (disconnectDestroyedSignal)
        QObject::disconnect(objectIt->destroyedConnection);

    m_objects.erase(objectIt);

    const auto orderIt = std::find(m_objectInsertionOrder.cbegin(),
                                   m_objectInsertionOrder.cend(), object);
    if (orderIt != m_objectInsertionOrder.cend())
        m_objectInsertionOrder.erase(orderIt);
}

void FramePropertyRegistry::clearObject(QObject *object)
{
    if (!checkGuiThread(Q_FUNC_INFO))
        return;
    removeObject(object, true);
}

} // namespace WinUI3::Private
