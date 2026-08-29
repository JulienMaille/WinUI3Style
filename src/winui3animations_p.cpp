#include "winui3animations_p.h"

#include "winui3frameproperties_p.h"

#include <QLineEdit>
#include <QVariantAnimation>
#include <QWidget>

#include <cmath>
#include <utility>

namespace WinUI3::Private {

namespace {

// QLineEdit's private clear button paints its own child surface, but the
// editor's backing store is what callers normally capture/repaint. Cache the
// direct editor parent when an animation is created so each frame can request
// both repaints without walking the child tree or allocating temporary state.
QLineEdit *lineEditParent(QWidget *widget)
{
    return widget ? qobject_cast<QLineEdit *>(widget->parentWidget()) : nullptr;
}

void updateWidgetAndEditor(QWidget *widget, QLineEdit *editor)
{
    if (!widget)
        return;
    widget->update();
    if (editor)
        editor->update();
}

} // namespace

FrameAnimationDriver::FrameAnimationDriver(QObject *context)
    : m_context(context)
{
}

FrameAnimationDriver::~FrameAnimationDriver()
{
    const auto widgets = m_animations.keys();
    for (QWidget *widget : widgets)
        stop(widget);
}

QVariantAnimation *FrameAnimationDriver::find(QWidget *widget,
                                               const char *property) const
{
    if (!widget)
        return nullptr;
    const auto widgetIt = m_animations.constFind(widget);
    if (widgetIt == m_animations.cend())
        return nullptr;
    const auto propertyIt = widgetIt->constFind(QByteArray(property));
    return propertyIt == widgetIt->cend() ? nullptr : propertyIt->data();
}

void FrameAnimationDriver::forget(QWidget *widget, const QByteArray &property,
                                  QVariantAnimation *expected)
{
    if (!widget || !expected)
        return;
    auto widgetIt = m_animations.find(widget);
    if (widgetIt == m_animations.end())
        return;
    const auto propertyIt = widgetIt->find(property);
    if (propertyIt == widgetIt->end() || propertyIt->data() != expected)
        return;
    const QPointer<QVariantAnimation> animation = propertyIt->data();
    widgetIt->erase(propertyIt);
    if (widgetIt->isEmpty()) {
        m_animations.erase(widgetIt);
        if (const auto connection = m_cleanupConnections.take(widget))
            QObject::disconnect(connection);
    }
    if (animation) {
        animation->stop();
        delete animation;
    }
}

QVariantAnimation *FrameAnimationDriver::ensure(QWidget *widget,
                                                const char *property)
{
    if (!widget)
        return nullptr;
    auto widgetIt = m_animations.find(widget);
    if (widgetIt == m_animations.end())
        widgetIt = m_animations.insert(widget, {});
    const QByteArray propertyName(property);
    auto propertyIt = widgetIt->find(propertyName);
    if (propertyIt != widgetIt->end() && propertyIt->data())
        return propertyIt->data();

    auto *animation = new QVariantAnimation(m_context);
    widgetIt->insert(propertyName, QPointer<QVariantAnimation>(animation));
    if (!m_cleanupConnections.contains(widget)) {
        m_cleanupConnections.insert(widget,
            QObject::connect(widget, &QObject::destroyed, m_context,
                             [this, widget] { stop(widget); }));
    }
    const QPointer<QWidget> guardedWidget(widget);
    const QPointer<QLineEdit> guardedEditor(lineEditParent(widget));
    QObject::connect(animation, &QVariantAnimation::valueChanged, m_context,
                     [guardedWidget, guardedEditor,
                      propertyName](const QVariant &value) {
        if (!guardedWidget)
            return;
        framePropertyRegistry().set(guardedWidget, propertyName, value);
        guardedWidget->update();
        if (guardedEditor)
            guardedEditor->update();
    });
    QObject::connect(animation, &QVariantAnimation::finished, m_context,
                     [this, widget, propertyName, animation] {
        auto widgetIt = m_animations.find(widget);
        if (widgetIt == m_animations.end())
            return;
        const auto propertyIt = widgetIt->find(propertyName);
        if (propertyIt == widgetIt->end() || propertyIt->data() != animation)
            return;
        widgetIt->erase(propertyIt);
        if (widgetIt->isEmpty()) {
            m_animations.erase(widgetIt);
            if (const auto connection = m_cleanupConnections.take(widget))
                QObject::disconnect(connection);
        }
        delete animation;
    });
    return animation;
}

void FrameAnimationDriver::animate(QWidget *widget, const char *property,
                                   qreal target, int duration, bool allowed,
                                   const QEasingCurve &curve,
                                   const QVector<QPair<qreal, QVariant>> &keyValues,
                                   qreal startOverride)
{
    if (!widget)
        return;

    const qreal start = std::isnan(startOverride)
        ? framePropertyRegistry().real(widget, property, 1.0 - target)
        : startOverride;
    QPointer<QVariantAnimation> previous = find(widget, property);
    if (previous)
        previous->stop();
    if (duration <= 0 || !allowed || qFuzzyCompare(start, target)) {
        forget(widget, QByteArray(property), previous);
        framePropertyRegistry().set(widget, property, target);
        updateWidgetAndEditor(widget, lineEditParent(widget));
        return;
    }

    auto *animation = ensure(widget, property);
    if (!animation)
        return;
    animation->setKeyValues(keyValues);
    animation->setStartValue(start);
    animation->setEndValue(target);
    animation->setDuration(duration);
    animation->setEasingCurve(curve);
    animation->start();
}

void FrameAnimationDriver::stop(QWidget *widget)
{
    if (!widget)
        return;
    if (auto it = m_animations.find(widget); it != m_animations.end()) {
        const auto propertyAnimations = std::move(it.value());
        m_animations.erase(it);
        for (const QPointer<QVariantAnimation> &animation : propertyAnimations) {
            if (animation) {
                animation->stop();
                delete animation;
            }
        }
    }
    if (const auto connection = m_cleanupConnections.take(widget))
        QObject::disconnect(connection);
}

} // namespace WinUI3::Private
