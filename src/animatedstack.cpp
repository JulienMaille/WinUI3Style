#include <winui3style/animatedstack.h>
#include <winui3style/winui3style.h>

#include <QGraphicsEffect>
#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>

namespace WinUI3 {

AnimatedStack::AnimatedStack(QWidget *parent)
    : QStackedWidget(parent)
{
    connect(this, &QStackedWidget::widgetRemoved, this, [this] {
        // A page can disappear while an animation is running (for example
        // when a navigation model is reset).  Finish cleanup synchronously so
        // no animation retains a deleted page or stale index.
        if (m_group)
            cancelTransition();
    });
}

AnimatedStack::~AnimatedStack() = default;

int AnimatedStack::duration() const { return m_duration; }
void AnimatedStack::setDuration(int duration) { m_duration = qMax(0, duration); }
bool AnimatedStack::isAnimating() const { return m_group && m_group->state() == QAbstractAnimation::Running; }

void AnimatedStack::setCurrentIndex(int index)
{
    const Transition transition = index >= QStackedWidget::currentIndex()
        ? Transition::Forward : Transition::Backward;
    setCurrentIndex(index, transition);
}

void AnimatedStack::setCurrentIndex(int index, Transition transition)
{
    if (index < 0 || index >= count()) return;
    if (index == QStackedWidget::currentIndex()) {
        // QAbstractItemView can emit both clicked and activated for the same
        // gesture. Re-selecting the page that is already entering must not
        // cancel the transition and expose a discontinuous final frame.
        return;
    }
    QPixmap interruptedFrame;
    if (isAnimating()) {
        // Capture the actually composited frame before committing the
        // interrupted transition. The replacement transition starts from
        // this image, so reversing direction cannot flash an opaque page.
        interruptedFrame = grab();
        cancelTransition();
    }
    if (m_duration == 0 || !isVisible() || !Style::animationsAllowed()) {
        QStackedWidget::setCurrentIndex(index);
        if (QWidget *page = widget(index))
            page->setGeometry(rect());
        emit transitionFinished(index);
        return;
    }

    m_from = QStackedWidget::currentIndex();
    m_to = index;
    QWidget *outgoing = widget(m_from);
    QWidget *incoming = widget(m_to);
    if (!outgoing || !incoming) {
        QStackedWidget::setCurrentIndex(index);
        return;
    }

    if (transition == Transition::Automatic)
        transition = m_to >= m_from ? Transition::Forward : Transition::Backward;
    const int direction = transition == Transition::Backward ? -1 : 1;
    const int distance = transition == Transition::Entrance ? 28 : 36;
    const QRect finalGeometry = rect();
    const QRect outgoingEnd = finalGeometry.translated(-direction * distance / 2, 0);

    // Never attach an effect to an application page. Fading the opaque
    // outgoing snapshot above the already-visible incoming page is the same
    // source-over cross-fade, without borrowing QGraphicsEffect ownership or
    // leaving QPropertyAnimation with a destroyed target if the application
    // installs/replaces an effect during the transition.
    m_incoming = incoming;

    auto *overlay = new QLabel(this);
    overlay->setPixmap(interruptedFrame.isNull() ? outgoing->grab()
                                                 : interruptedFrame);
    overlay->setScaledContents(true);
    overlay->setAttribute(Qt::WA_TransparentForMouseEvents);
    overlay->setGeometry(finalGeometry);
    auto *outOpacity = new QGraphicsOpacityEffect(overlay);
    overlay->setGraphicsEffect(outOpacity);
    outOpacity->setOpacity(1.0);
    m_overlay = overlay;

    QStackedWidget::setCurrentIndex(m_to);
    // Keep the real page at its layout geometry for the entire transition.
    // Only the outgoing snapshot moves; this prevents a navigation animation
    // from causing layout churn or a second geometry negotiation.
    incoming->setGeometry(finalGeometry);
    incoming->show();
    overlay->show();
    overlay->raise();

    m_group = new QParallelAnimationGroup(this);
    auto add = [this](QObject *target, const char *property,
                      const QVariant &start, const QVariant &end) {
        auto *animation = new QPropertyAnimation(target, property, m_group);
        animation->setStartValue(start);
        animation->setEndValue(end);
        animation->setDuration(m_duration);
        animation->setEasingCurve(QEasingCurve::OutCubic);
        m_group->addAnimation(animation);
    };
    add(overlay, "geometry", finalGeometry, outgoingEnd);
    add(outOpacity, "opacity", 1.0, 0.0);
    connect(m_group, &QParallelAnimationGroup::finished,
            this, &AnimatedStack::finishTransition);
    m_group->start();
}

void AnimatedStack::cancelTransition()
{
    if (!m_group)
        return;

    disconnect(m_group, nullptr, this, nullptr);
    m_group->stop();
    m_group->deleteLater();
    m_group = nullptr;

    m_incoming = nullptr;

    if (m_overlay) {
        m_overlay->hide();
        m_overlay->deleteLater();
        m_overlay = nullptr;
    }

    if (m_to >= 0 && m_to < count()) {
        QStackedWidget::setCurrentIndex(m_to);
        if (QWidget *page = widget(m_to))
            page->setGeometry(rect());
    }
    m_from = m_to = -1;
}

void AnimatedStack::finishTransition()
{
    if (!m_group)
        return;
    QWidget *incoming = m_incoming ? m_incoming.data() : widget(m_to);
    if (incoming) {
        incoming->setGeometry(rect());
    }
    m_incoming = nullptr;
    if (m_overlay) {
        m_overlay->deleteLater();
        m_overlay = nullptr;
    }
    QStackedWidget::setCurrentIndex(m_to);
    const int completed = m_to;
    m_group->deleteLater();
    m_group = nullptr;
    m_from = m_to = -1;
    emit transitionFinished(completed);
}

} // namespace WinUI3
