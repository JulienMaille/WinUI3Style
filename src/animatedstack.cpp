#include <winui3style/animatedstack.h>
#include <winui3style/winui3style.h>

#include <QHideEvent>
#include <QPainter>
#include <QParallelAnimationGroup>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QVariantAnimation>

namespace WinUI3 {

namespace {

class SnapshotOverlay final : public QWidget
{
public:
    SnapshotOverlay(const QPixmap &pixmap, QWidget *parent)
        : QWidget(parent)
        , m_pixmap(pixmap)
    {
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setObjectName(QStringLiteral("_winui_animated_stack_overlay"));
        setProperty("_winui_animated_stack_opacity", 1.0);
    }

    void setOpacity(qreal opacity)
    {
        m_opacity = qBound<qreal>(0.0, opacity, 1.0);
        setProperty("_winui_animated_stack_opacity", m_opacity);
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        const QSize target(qRound(width() * devicePixelRatioF()),
                           qRound(height() * devicePixelRatioF()));
        painter.setRenderHint(QPainter::SmoothPixmapTransform,
                               m_pixmap.size() != target);
        painter.setOpacity(m_opacity);
        painter.drawPixmap(rect(), m_pixmap);
    }

private:
    QPixmap m_pixmap;
    qreal m_opacity = 1.0;
};

} // namespace

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
    if (m_transitionStarting) {
        // QStackedWidget emits currentChanged synchronously. A consumer can
        // request another page from that signal before this transition has
        // finished constructing its snapshot and animation group. Retain
        // only the latest request and apply it after the group is running.
        m_deferredIndex = index;
        m_deferredTransition = transition;
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
        m_transitionStarting = true;
        QStackedWidget::setCurrentIndex(index);
        m_transitionStarting = false;
        if (QWidget *page = widget(index))
            page->setGeometry(rect());
        emit transitionFinished(index);
        if (m_deferredIndex >= 0) {
            const int deferred = m_deferredIndex;
            const Transition deferredTransition = m_deferredTransition;
            m_deferredIndex = -1;
            setCurrentIndex(deferred, deferredTransition);
        }
        return;
    }

    m_from = QStackedWidget::currentIndex();
    m_to = index;
    QPointer<QWidget> outgoing = widget(m_from);
    QPointer<QWidget> incoming = widget(m_to);
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
    m_transitionProgress = 0.0;

    QPixmap frame = interruptedFrame;
    if (frame.isNull())
        frame = grab();
    if (frame.isNull())
        frame = outgoing->grab();
    auto *overlay = new SnapshotOverlay(frame, this);
    overlay->setGeometry(finalGeometry);
    m_overlay = overlay;

    m_transitionStarting = true;
    QStackedWidget::setCurrentIndex(m_to);
    m_transitionStarting = false;
    if (!outgoing || !incoming || indexOf(incoming) < 0) {
        // A currentChanged handler is allowed to remove a page synchronously.
        // Do not dereference the raw local widget after that callback, and do
        // not leave the unstarted overlay alive as a stale child.
        destroyTransitionObjects();
        m_incoming = nullptr;
        m_outgoing = nullptr;
        if (incoming && indexOf(incoming) >= 0) {
            QStackedWidget::setCurrentWidget(incoming);
            incoming->setGeometry(rect());
        } else if (count() > 0) {
            const int fallback = qBound(0, currentIndex(), count() - 1);
            QStackedWidget::setCurrentIndex(fallback);
            if (QWidget *page = widget(fallback))
                page->setGeometry(rect());
        }
        m_from = m_to = -1;
        m_transitionProgress = 0.0;
        m_deferredIndex = -1;
        return;
    }
    // Keep the real page at its layout geometry for the entire transition.
    // Only the outgoing snapshot moves; this prevents a navigation animation
    // from causing layout churn or a second geometry negotiation.
    incoming->setGeometry(finalGeometry);
    incoming->show();
    overlay->show();
    overlay->raise();

    m_group = new QParallelAnimationGroup(this);
    m_group->setObjectName(QStringLiteral("_winui_animated_stack_group"));
    auto *geometry = new QVariantAnimation(m_group);
    geometry->setStartValue(0.0);
    geometry->setEndValue(1.0);
    geometry->setDuration(m_duration);
    geometry->setEasingCurve(QEasingCurve::OutCubic);
    m_geometryAnimation = geometry;
    connect(geometry, &QVariantAnimation::valueChanged, this,
            [this](const QVariant &value) {
        m_transitionProgress = qBound<qreal>(0.0, value.toReal(), 1.0);
        updateOverlayGeometry(m_transitionProgress);
    });
    m_group->addAnimation(geometry);

    auto *opacity = new QVariantAnimation(m_group);
    opacity->setStartValue(1.0);
    opacity->setEndValue(0.0);
    opacity->setDuration(m_duration);
    opacity->setEasingCurve(QEasingCurve::OutCubic);
    connect(opacity, &QVariantAnimation::valueChanged, this,
            [this](const QVariant &value) {
        if (auto *overlay = dynamic_cast<SnapshotOverlay *>(m_overlay.data()))
            overlay->setOpacity(value.toReal());
    });
    m_group->addAnimation(opacity);
    connect(m_group, &QParallelAnimationGroup::finished,
            this, &AnimatedStack::finishTransition);
    m_group->start();

    if (m_deferredIndex >= 0) {
        const int deferred = m_deferredIndex;
        const Transition deferredTransition = m_deferredTransition;
        m_deferredIndex = -1;
        setCurrentIndex(deferred, deferredTransition);
    }
}

void AnimatedStack::hideEvent(QHideEvent *event)
{
    if (m_group)
        cancelTransition();
    QStackedWidget::hideEvent(event);
}

void AnimatedStack::resizeEvent(QResizeEvent *event)
{
    QStackedWidget::resizeEvent(event);
    // QStackedWidget only lays out the current child. Keeping every page at
    // the stack geometry prevents a hidden page from flashing at its stale
    // construction size when it becomes the incoming page.
    for (int index = 0; index < count(); ++index) {
        if (QWidget *page = widget(index))
            page->setGeometry(rect());
    }
    if (m_overlay) {
        const QPoint offset = m_overlayEndGeometry.topLeft()
            - m_overlayStartGeometry.topLeft();
        m_overlayStartGeometry = rect();
        m_overlayEndGeometry = rect().translated(offset);
        updateOverlayGeometry(m_transitionProgress);
    }
}

void AnimatedStack::updateOverlayGeometry(qreal progress)
{
    if (!m_overlay)
        return;
    const QRectF start(m_overlayStartGeometry);
    const QRectF end(m_overlayEndGeometry);
    const QRectF frame(
        start.left() + (end.left() - start.left()) * progress,
        start.top() + (end.top() - start.top()) * progress,
        start.width() + (end.width() - start.width()) * progress,
        start.height() + (end.height() - start.height()) * progress);
    m_overlay->setGeometry(frame.toRect());
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
    m_transitionProgress = 0.0;
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
    m_transitionProgress = 0.0;
    if (completed >= 0)
        emit transitionFinished(completed);
}

} // namespace WinUI3
