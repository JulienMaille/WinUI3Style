#include <winui3style/animatedstack.h>
#include <winui3style/winui3style.h>

#include <QHideEvent>
#include <QLayout>
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
        const QImage image = m_pixmap.toImage();
        m_background = image.isNull() ? palette().color(QPalette::Window)
                                      : image.pixelColor(0, 0);
        if (m_background.alpha() < 255)
            m_background = palette().color(QPalette::Window);
    }

    void setOffset(qreal offset)
    {
        m_offset = offset;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.fillRect(rect(), m_background);
        const QSize target(qRound(width() * devicePixelRatioF()),
                           qRound(height() * devicePixelRatioF()));
        painter.setRenderHint(QPainter::SmoothPixmapTransform,
                               m_pixmap.size() != target);
        painter.drawPixmap(QRectF(rect()).translated(m_offset, 0.0),
                           m_pixmap, QRectF(m_pixmap.rect()));
    }

private:
    QPixmap m_pixmap;
    QColor m_background;
    qreal m_offset = 0.0;
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
    if (isAnimating()) {
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

    // Never attach an effect to an application page. More importantly, do
    // not cross-fade an outgoing snapshot over the incoming page: text from
    // both pages remains readable during that blend and produces a visible
    // ghost. We switch first, capture the fully laid-out incoming page, then
    // slide that opaque snapshot over an opaque background.
    m_outgoing = outgoing;
    m_incoming = incoming;
    m_overlayStartOffset = direction * distance;
    m_transitionProgress = 0.0;

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
    incoming->ensurePolished();
    if (incoming->layout())
        incoming->layout()->activate();
    QPixmap frame = incoming->grab();
    if (frame.isNull())
        frame = grab();
    // The snapshot is the only visible representation of the entering page
    // while it moves. Leaving the real page visible underneath would draw
    // every label twice at different horizontal positions.
    incoming->hide();
    auto *overlay = new SnapshotOverlay(frame, this);
    overlay->setGeometry(finalGeometry);
    overlay->setOffset(m_overlayStartOffset);
    m_overlay = overlay;
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
        updateOverlayGeometry(m_transitionProgress);
    }
}

void AnimatedStack::updateOverlayGeometry(qreal progress)
{
    if (!m_overlay)
        return;
    m_overlay->setGeometry(rect());
    if (auto *overlay = dynamic_cast<SnapshotOverlay *>(m_overlay.data())) {
        overlay->setOffset(m_overlayStartOffset * (1.0 - progress));
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
        finalPage->show();
    } else if (count() > 0) {
        const int fallback = qBound(0, currentIndex(), count() - 1);
        QStackedWidget::setCurrentIndex(fallback);
        if (QWidget *page = widget(fallback)) {
            page->setGeometry(rect());
            page->show();
        }
    }
    m_from = m_to = -1;
    m_overlayStartOffset = 0.0;
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
        if (QWidget *page = widget(completed)) {
            page->setGeometry(rect());
            page->show();
        }
    }
    m_from = m_to = -1;
    m_overlayStartOffset = 0.0;
    m_transitionProgress = 0.0;
    if (completed >= 0)
        emit transitionFinished(completed);
}

} // namespace WinUI3
