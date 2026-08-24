#include <winui3style/animatedstack.h>
#include <winui3style/winui3style.h>

#include <QGraphicsEffect>
#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QVariantAnimation>

namespace WinUI3 {

AnimatedStack::AnimatedStack(QWidget *parent)
    : QStackedWidget(parent)
{
    connect(this, &QStackedWidget::widgetRemoved, this,
            &AnimatedStack::handleWidgetRemoved);
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
    m_outgoing = outgoing;
    m_incoming = incoming;
    m_overlayStartGeometry = finalGeometry;
    m_overlayEndGeometry = outgoingEnd;

    auto *overlay = new QLabel(this);
    overlay->setObjectName(QStringLiteral("_winui_animated_stack_overlay"));
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
    m_group->setObjectName(QStringLiteral("_winui_animated_stack_group"));
    auto add = [this](QObject *target, const char *property,
                      const QVariant &start, const QVariant &end) {
        auto *animation = new QPropertyAnimation(target, property, m_group);
        animation->setStartValue(start);
        animation->setEndValue(end);
        animation->setDuration(m_duration);
        animation->setEasingCurve(QEasingCurve::OutCubic);
        m_group->addAnimation(animation);
    };

    auto *geometry = new QVariantAnimation(m_group);
    geometry->setStartValue(0.0);
    geometry->setEndValue(1.0);
    geometry->setDuration(m_duration);
    geometry->setEasingCurve(QEasingCurve::OutCubic);
    m_geometryAnimation = geometry;
    connect(geometry, &QVariantAnimation::valueChanged, this,
            [this](const QVariant &value) {
        if (!m_overlay)
            return;
        const qreal progress = qBound<qreal>(0.0, value.toReal(), 1.0);
        const QRectF start(m_overlayStartGeometry);
        const QRectF end(m_overlayEndGeometry);
        const QRectF frame(
            start.left() + (end.left() - start.left()) * progress,
            start.top() + (end.top() - start.top()) * progress,
            start.width() + (end.width() - start.width()) * progress,
            start.height() + (end.height() - start.height()) * progress);
        m_overlay->setGeometry(frame.toRect());
    });
    m_group->addAnimation(geometry);
    add(outOpacity, "opacity", 1.0, 0.0);
    connect(m_group, &QParallelAnimationGroup::finished,
            this, &AnimatedStack::finishTransition);
    m_group->start();
}

void AnimatedStack::resizeEvent(QResizeEvent *event)
{
    QStackedWidget::resizeEvent(event);
    if (m_incoming && indexOf(m_incoming) >= 0)
        m_incoming->setGeometry(rect());
    if (m_overlay) {
        const int offset = m_overlayEndGeometry.left()
            - m_overlayStartGeometry.left();
        m_overlayStartGeometry = rect();
        m_overlayEndGeometry = rect().translated(offset, 0);
        m_overlay->setGeometry(rect());
    }
}

void AnimatedStack::handleWidgetRemoved(int index)
{
    if (!m_group)
        return;

    // Indices after widgetRemoved() are already shifted by QStackedWidget. The
    // guarded page pointers remain authoritative, but keeping both indices in
    // sync prevents stale values from being used by diagnostic code or a
    // re-entrant navigation request.
    if (m_from == index)
        m_from = -1;
    else if (m_from > index)
        --m_from;
    if (m_to == index)
        m_to = -1;
    else if (m_to > index)
        --m_to;
    cancelTransition();
}

void AnimatedStack::destroyTransitionObjects()
{
    if (m_group) {
        QParallelAnimationGroup *group = m_group.data();
        m_group = nullptr;
        disconnect(group, nullptr, this, nullptr);
        group->stop();
        delete group;
    }
    m_geometryAnimation = nullptr;
    if (m_overlay) {
        QWidget *overlay = m_overlay.data();
        m_overlay = nullptr;
        overlay->hide();
        delete overlay;
    }
}

void AnimatedStack::cancelTransition()
{
    if (!m_group)
        return;

    QPointer<QWidget> finalPage = m_incoming;
    if (!finalPage || indexOf(finalPage) < 0)
        finalPage = m_outgoing;
    destroyTransitionObjects();
    m_incoming = nullptr;
    m_outgoing = nullptr;

    if (finalPage && indexOf(finalPage) >= 0) {
        QStackedWidget::setCurrentWidget(finalPage);
        finalPage->setGeometry(rect());
    } else if (count() > 0) {
        const int fallback = qBound(0, currentIndex(), count() - 1);
        QStackedWidget::setCurrentIndex(fallback);
        if (QWidget *page = widget(fallback))
            page->setGeometry(rect());
    }
    m_from = m_to = -1;
}

void AnimatedStack::finishTransition()
{
    if (!m_group)
        return;
    QPointer<QWidget> incoming = m_incoming;
    if (!incoming || indexOf(incoming) < 0)
        incoming = m_outgoing;
    const int completed = incoming ? indexOf(incoming) : -1;
    destroyTransitionObjects();
    m_incoming = nullptr;
    m_outgoing = nullptr;
    if (completed >= 0) {
        QStackedWidget::setCurrentIndex(completed);
        if (QWidget *page = widget(completed))
            page->setGeometry(rect());
    }
    m_from = m_to = -1;
    if (completed >= 0)
        emit transitionFinished(completed);
}

} // namespace WinUI3
