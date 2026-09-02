#include "navigationview_p.h"

#include "winui3paint_p.h"
#include "winui3frameproperties_p.h"
#include "winui3density_p.h"
#include "winui3helpers_p.h"
#include "winui3style_properties_p.h"
#include "winui3tokens_p.h"

#include <winui3style/winui3icons.h>
#include <winui3style/winui3style.h>

#include <QAbstractItemDelegate>
#include <QAbstractItemView>
#include <QFrame>
#include <QItemSelectionModel>
#include <QPainter>
#include <QPointer>
#include <QScrollBar>
#include <QStyledItemDelegate>
#include <QVariantAnimation>

namespace WinUI3::NavigationPrivate {
namespace {

using namespace WinUI3::PaintPrivate;
using WinUI3::Private::progress;
using WinUI3::Private::keyboardFocusVisible;

constexpr auto navigationIndicatorProperty = "_winui_navigation_indicator_y";
constexpr auto navigationDelegateProperty = "_winui_navigation_delegate";
constexpr auto navigationOriginalDelegateProperty = "_winui_navigation_original_delegate";
constexpr auto navigationStateProperty = "_winui_navigation_state";
constexpr auto originalMouseTrackingProperty = "_winui_original_mouse_tracking";

bool animationsAllowed()
{
    return Style::animationsAllowed();
}

const QEasingCurve &fluentCurve()
{
    static const QEasingCurve curve = [] {
        QEasingCurve result(QEasingCurve::BezierSpline);
        result.addCubicBezierSegment(QPointF(0.0, 0.0), QPointF(0.0, 1.0),
                                     QPointF(1.0, 1.0));
        return result;
    }();
    return curve;
}

class NavigationItemDelegate final : public QStyledItemDelegate
{
public:
    explicit NavigationItemDelegate(QAbstractItemView *view,
                                    QAbstractItemDelegate *original)
        : QStyledItemDelegate(view), m_view(view), m_original(original), m_indicatorAnimation(this)
    {
        QObject::connect(&m_indicatorAnimation, &QVariantAnimation::valueChanged, this,
                         [this](const QVariant &value) {
            m_indicatorY = value.toReal();
            if (m_view) {
                WinUI3::Private::framePropertyRegistry().set(
                    m_view->viewport(), navigationIndicatorProperty, m_indicatorY);
                m_view->viewport()->update();
            }
        });
        attachSelectionModel();
        m_verticalConnection = QObject::connect(view->verticalScrollBar(), &QScrollBar::valueChanged,
                                                this, [this] { syncIndicatorToViewport(); });
        m_horizontalConnection = QObject::connect(view->horizontalScrollBar(), &QScrollBar::valueChanged,
                                                  this, [this] { syncIndicatorToViewport(); });
    }

    ~NavigationItemDelegate() override { shutdown(false); }
    QAbstractItemDelegate *originalDelegate() const { return m_original.data(); }

    void shutdown(bool clearViewport = true)
    {
        if (m_shutdown)
            return;
        m_shutdown = true;
        m_indicatorAnimation.stop();
        QObject::disconnect(m_selectionConnection);
        QObject::disconnect(m_verticalConnection);
        QObject::disconnect(m_horizontalConnection);
        m_selectionConnection = {};
        m_verticalConnection = {};
        m_horizontalConnection = {};
        if (clearViewport && m_view && m_view->viewport()) {
            WinUI3::Private::framePropertyRegistry().clear(
                m_view->viewport(), navigationIndicatorProperty);
            m_view->viewport()->update();
        }
        m_selectionModel.clear();
    }

    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override
    {
        return {220, Private::densityMetricsFor(m_view).navigationItemHeight};
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        if (m_shutdown)
            return;
        const_cast<NavigationItemDelegate *>(this)->attachSelectionModel();
        const auto t = Private::tokens(option.palette);
        const bool selected = option.state & QStyle::State_Selected;
        const bool hovered = option.state & QStyle::State_MouseOver;
        const qreal press = progress(option.widget, "_winui_press_progress",
                                     option.state & QStyle::State_Sunken ? 1.0 : 0.0);
        // A transparent NavigationView is composited over the window backdrop.
        // QAbstractItemView does not erase transparent rows before asking the
        // delegate to repaint them, so the previous selection fill/indicator
        // otherwise remains in the backing store after currentRow changes.
        // Clear this row's pixels with Source composition before rebuilding it.
        if (m_view && m_view->palette().color(QPalette::Base).alpha() == 0) {
            painter->save();
            painter->setCompositionMode(QPainter::CompositionMode_Source);
            painter->fillRect(option.rect, Qt::transparent);
            painter->restore();
        }
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        QColor fill = selected ? t.subtlePressed : Qt::transparent;
        if (hovered)
            fill = Private::mix(fill, t.subtleHover, 1.0 - press);
        fill = Private::mix(fill, t.subtlePressed, press);
        if (fill.alpha() > 0)
            roundedRect(painter, QRectF(option.rect).adjusted(2, 2, -2, -2), fill,
                        Qt::transparent, Private::ControlRadius);
        const QIcon itemIcon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        if (!itemIcon.isNull()) {
            const QRect logicalIcon(option.rect.left() + 14, option.rect.center().y() - 8, 16, 16);
            paintThemedIcon(painter, itemIcon,
                            QStyle::visualRect(option.direction, option.rect, logicalIcon),
                            Qt::AlignCenter,
                            option.state & QStyle::State_Enabled ? t.textPrimary : t.textDisabled,
                            option.state & QStyle::State_Enabled ? QIcon::Normal : QIcon::Disabled);
        }
        painter->setPen(option.state & QStyle::State_Enabled ? t.textPrimary : t.textDisabled);
        const QRect textRect = QStyle::visualRect(option.direction, option.rect,
                                                  option.rect.adjusted(42, 0, -12, 0));
        painter->drawText(textRect, QStyle::visualAlignment(option.direction,
                        Qt::AlignLeft | Qt::AlignVCenter),
                          option.fontMetrics.elidedText(index.data().toString(), Qt::ElideRight,
                                                        textRect.width()));
        if (m_view && m_indicatorY < 0.0 && m_view->currentIndex().isValid())
            m_indicatorY = m_view->visualRect(m_view->currentIndex()).top();
        if (m_indicatorY >= 0.0) {
            const qreal indicatorX = option.direction == Qt::RightToLeft ? option.rect.right() - 5.0
                                                                           : option.rect.left() + 2.0;
            const int rowHeight = Private::densityMetricsFor(m_view).navigationItemHeight;
            const QRectF indicator(indicatorX,
                                   m_indicatorY + (rowHeight - 16.0) / 2.0,
                                   3.0, 16.0);
            if (indicator.intersects(option.rect))
                roundedRect(painter, indicator, t.selectionAccent, Qt::transparent, 1.5);
        }
        if ((option.state & QStyle::State_HasFocus) && keyboardFocusVisible(m_view)) {
            painter->setBrush(Qt::NoBrush);
            painter->setPen(QPen(t.focusOuter, 2));
            painter->drawRoundedRect(QRectF(option.rect).adjusted(3, 3, -3, -3), 4, 4);
            painter->setPen(QPen(t.focusInner, 1));
            painter->drawRoundedRect(QRectF(option.rect).adjusted(5, 5, -5, -5), 3, 3);
        }
        painter->restore();
    }

private:
    void attachSelectionModel()
    {
        if (m_shutdown || !m_view || m_selectionModel == m_view->selectionModel())
            return;
        QObject::disconnect(m_selectionConnection);
        m_selectionModel = m_view->selectionModel();
        if (!m_selectionModel)
            return;
        m_selectionConnection = QObject::connect(m_selectionModel, &QItemSelectionModel::currentChanged,
                                                 this, [this](const QModelIndex &current) {
            setIndicatorTarget(current, true);
        });
        setIndicatorTarget(m_selectionModel->currentIndex(), false);
    }

    void setIndicatorTarget(const QModelIndex &current, bool animate)
    {
        if (m_shutdown || !m_view)
            return;
        if (!current.isValid()) {
            m_indicatorAnimation.stop();
            m_indicatorY = -1.0;
            if (m_view->viewport()) {
                WinUI3::Private::framePropertyRegistry().clear(
                    m_view->viewport(), navigationIndicatorProperty);
                m_view->viewport()->update();
            }
            return;
        }
        const qreal target = m_view->visualRect(current).top();
        if (m_indicatorY < 0.0 || !animate || !animationsAllowed()) {
            m_indicatorAnimation.stop();
            m_indicatorY = target;
            if (m_view->viewport()) {
                WinUI3::Private::framePropertyRegistry().set(
                    m_view->viewport(), navigationIndicatorProperty, m_indicatorY);
                m_view->viewport()->update();
            }
            return;
        }
        m_indicatorAnimation.stop();
        m_indicatorAnimation.setStartValue(m_indicatorY);
        m_indicatorAnimation.setEndValue(target);
        m_indicatorAnimation.setDuration(Private::NormalDuration);
        m_indicatorAnimation.setEasingCurve(fluentCurve());
        m_indicatorAnimation.start();
    }

    void syncIndicatorToViewport()
    {
        attachSelectionModel();
        if (m_selectionModel)
            setIndicatorTarget(m_selectionModel->currentIndex(), false);
    }

    QPointer<QAbstractItemView> m_view;
    QPointer<QItemSelectionModel> m_selectionModel;
    QPointer<QAbstractItemDelegate> m_original;
    QMetaObject::Connection m_selectionConnection, m_verticalConnection, m_horizontalConnection;
    mutable qreal m_indicatorY = -1.0;
    QVariantAnimation m_indicatorAnimation;
    bool m_shutdown = false;
};

class NavigationViewState final : public QObject
{
public:
    explicit NavigationViewState(QAbstractItemView *view) : QObject(view)
    {
        setObjectName(QString::fromLatin1(navigationStateProperty));
    }
    QPointer<NavigationItemDelegate> delegate;
    QPointer<QAbstractItemDelegate> original;
    QPalette viewPalette;
    QPalette viewportPalette;
    QFrame::Shape frameShape = QFrame::NoFrame;
    bool viewportAutoFill = false;
    bool viewportOpaque = false;
    bool viewPaletteExplicit = false;
    bool viewportPaletteExplicit = false;
    bool surfaceStateSaved = false;
};

void restoreNavigationSurface(QAbstractItemView *view,
                              NavigationViewState *state);

void prepareNavigationSurface(QAbstractItemView *view,
                              NavigationViewState *state)
{
    if (!view || !view->viewport() || !state)
        return;
    // An explicit surface is an application override (for example a list on
    // an opaque content card).  Only the default NavigationView surface is
    // transparent so a top-level Mica backdrop can show through.
    if (view->property(Style::SurfaceProperty).isValid())
        return;
    const QString backdrop = view->window()
        ? view->window()->property(Style::BackdropProperty).toString()
        : QString();
    const bool translucentBackdrop = backdrop.compare(
        QLatin1String("mica"), Qt::CaseInsensitive) == 0
        || backdrop.compare(QLatin1String("acrylic"), Qt::CaseInsensitive) == 0;
    if (!translucentBackdrop) {
        restoreNavigationSurface(view, state);
        return;
    }
    if (state->surfaceStateSaved)
        return;
    state->viewPalette = view->palette();
    state->viewportPalette = view->viewport()->palette();
    state->frameShape = view->frameShape();
    state->viewportAutoFill = view->viewport()->autoFillBackground();
    state->viewportOpaque = view->viewport()->testAttribute(Qt::WA_OpaquePaintEvent);
    state->viewPaletteExplicit = view->testAttribute(Qt::WA_SetPalette);
    state->viewportPaletteExplicit = view->viewport()->testAttribute(Qt::WA_SetPalette);
    state->surfaceStateSaved = true;

    QPalette transparent = view->palette();
    transparent.setColor(QPalette::Base, Qt::transparent);
    transparent.setColor(QPalette::Window, Qt::transparent);
    view->setPalette(transparent);
    view->setFrameShape(QFrame::NoFrame);
    view->viewport()->setPalette(transparent);
    view->viewport()->setAutoFillBackground(false);
    view->viewport()->setAttribute(Qt::WA_OpaquePaintEvent, false);
}

void restoreNavigationSurface(QAbstractItemView *view,
                              NavigationViewState *state)
{
    if (!view || !state || !state->surfaceStateSaved)
        return;
    view->setPalette(state->viewPaletteExplicit ? state->viewPalette : QPalette());
    view->setFrameShape(state->frameShape);
    if (view->viewport()) {
        view->viewport()->setPalette(state->viewportPaletteExplicit
            ? state->viewportPalette : QPalette());
        view->viewport()->setAutoFillBackground(state->viewportAutoFill);
        view->viewport()->setAttribute(Qt::WA_OpaquePaintEvent,
                                       state->viewportOpaque);
    }
    state->surfaceStateSaved = false;
}

NavigationViewState *navigationState(QAbstractItemView *view, bool create)
{
    if (!view)
        return nullptr;
    QObject *object = view->findChild<QObject *>(QString::fromLatin1(navigationStateProperty),
                                                  Qt::FindDirectChildrenOnly);
    auto *state = dynamic_cast<NavigationViewState *>(object);
    if (!state && create)
        state = new NavigationViewState(view);
    return state;
}

void clearNavigationProperties(QAbstractItemView *view)
{
    if (!view)
        return;
    view->setProperty(navigationDelegateProperty, {});
    view->setProperty(navigationOriginalDelegateProperty, {});
    if (view->viewport())
        WinUI3::Private::framePropertyRegistry().clear(
            view->viewport(), navigationIndicatorProperty);
}

void retireNavigationDelegate(QAbstractItemView *view, NavigationViewState *state)
{
    if (!view || !state)
        return;
    NavigationItemDelegate *delegate = state->delegate.data();
    const bool installed = delegate && view->itemDelegate() == delegate;
    QAbstractItemDelegate *original = state->original.data();
    if (delegate)
        delegate->shutdown();
    state->delegate.clear();
    state->original.clear();
    clearNavigationProperties(view);
    if (installed)
        view->setItemDelegate(original ? original : new QStyledItemDelegate(view));
    if (delegate)
        delegate->deleteLater();
}

} // namespace

void prepareNavigationView(QAbstractItemView *view)
{
    if (!view || !view->property(Style::NavigationViewProperty).toBool())
        return;
    NavigationViewState *state = navigationState(view, true);
    prepareNavigationSurface(view, state);
    if (state->delegate && view->itemDelegate() == state->delegate)
        return;
    if (state->delegate)
        retireNavigationDelegate(view, state);
    QAbstractItemDelegate *original = view->itemDelegate();
    auto *delegate = new NavigationItemDelegate(view, original);
    state->original = original;
    state->delegate = delegate;
    view->setProperty(navigationOriginalDelegateProperty, QVariant::fromValue<QObject *>(original));
    view->setProperty(navigationDelegateProperty, QVariant::fromValue<QObject *>(delegate));
    const QPointer<QAbstractItemView> guardedView(view);
    const QPointer<NavigationViewState> guardedState(state);
    QObject::connect(delegate, &QObject::destroyed, state, [guardedView, guardedState, delegate] {
        if (!guardedView || !guardedState || guardedState->delegate.data() != delegate)
            return;
        guardedState->delegate.clear();
        if (guardedView->itemDelegate() == delegate)
            guardedView->setItemDelegate(new QStyledItemDelegate(guardedView));
        guardedView->setProperty(navigationDelegateProperty, {});
    });
    if (original) {
        QObject::connect(original, &QObject::destroyed, state, [guardedView, guardedState] {
            if (!guardedView || !guardedState)
                return;
            guardedState->original.clear();
            guardedView->setProperty(navigationOriginalDelegateProperty, {});
        });
    }
    view->setItemDelegate(delegate);
    view->viewport()->setProperty(Style::NavigationViewProperty, true);
    if (!view->viewport()->property(originalMouseTrackingProperty).isValid())
        view->viewport()->setProperty(originalMouseTrackingProperty,
                                      view->viewport()->hasMouseTracking());
    view->viewport()->setMouseTracking(true);
}

void restoreNavigationView(QAbstractItemView *view)
{
    if (!view)
        return;
    NavigationViewState *state = navigationState(view, false);
    if (state) {
        retireNavigationDelegate(view, state);
        restoreNavigationSurface(view, state);
    } else
        clearNavigationProperties(view);
    if (view->viewport()->property(originalMouseTrackingProperty).isValid())
        view->viewport()->setMouseTracking(view->viewport()->property(originalMouseTrackingProperty).toBool());
    view->viewport()->setProperty(originalMouseTrackingProperty, {});
    view->viewport()->setProperty(Style::NavigationViewProperty, {});
    WinUI3::Private::framePropertyRegistry().clear(
        view->viewport(), navigationIndicatorProperty);
}

} // namespace WinUI3::NavigationPrivate
