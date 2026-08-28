#include "winui3surfaces_p.h"

#include "winui3paint_p.h"
#include "winui3frameproperties_p.h"
#include "winui3style_properties_p.h"
#include "winui3tokens_p.h"
#include "winui3backdrop_p.h"

#include <winui3style/winui3style.h>

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QLayout>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QPaintEvent>
#include <QPainter>
#include <QParallelAnimationGroup>
#include <QPointer>
#include <QScreen>
#include <QSlider>
#include <QStyleOptionSlider>
#include <QTimer>
#include <QWidget>

namespace WinUI3::Private {
using namespace PaintPrivate;

void remember(QWidget *widget, const char *property, const QVariant &value)
{
    if (widget && !widget->property(property).isValid())
        widget->setProperty(property, value);
}

void rememberPalette(QWidget *widget)
{
    if (!widget)
        return;
    remember(widget, originalPaletteExplicitProperty,
             widget->testAttribute(Qt::WA_SetPalette));
    remember(widget, originalPaletteProperty,
             QVariant::fromValue(widget->palette()));
}

void restoreRememberedPalette(QWidget *widget)
{
    if (!widget || !widget->property(originalPaletteProperty).isValid())
        return;
    if (widget->property(originalPaletteExplicitProperty).toBool())
        widget->setPalette(widget->property(originalPaletteProperty).value<QPalette>());
    else
        widget->setPalette(QPalette());
}

QPalette effectivePopupPalette(QWidget *widget, const QPalette &fallback)
{
    if (!widget || !widget->property(originalPaletteExplicitProperty).toBool())
        return fallback;
    return widget->property(originalPaletteProperty).value<QPalette>().resolve(fallback);
}

void stopDialogAnimations(QDialog *dialog)
{
    if (!dialog)
        return;
    const auto groups = dialog->findChildren<QParallelAnimationGroup *>(
        QStringLiteral("_winui_dialog_animation"), Qt::FindDirectChildrenOnly);
    for (QParallelAnimationGroup *group : groups) {
        group->stop();
        delete group;
    }
    dialog->setWindowOpacity(1.0);
    dialog->setProperty("_winui_dialog_animating", false);
}

void restoreContentDialogState(QDialog *dialog, bool clearSavedState)
{
    if (!dialog)
        return;
    stopDialogAnimations(dialog);
    restoreRememberedPalette(dialog);
    if (dialog->property(originalAutoFillProperty).isValid())
        dialog->setAutoFillBackground(
            dialog->property(originalAutoFillProperty).toBool());
    if (dialog->property(originalMinimumSizeProperty).isValid())
        dialog->setMinimumSize(
            dialog->property(originalMinimumSizeProperty).value<QSize>());
    if (QLayout *layout = dialog->layout()) {
        if (dialog->property(originalMarginsProperty).isValid())
            layout->setContentsMargins(
                dialog->property(originalMarginsProperty).value<QMargins>());
        if (dialog->property(originalSpacingProperty).isValid())
            layout->setSpacing(dialog->property(originalSpacingProperty).toInt());
    }
    dialog->setProperty(ownedPaletteProperty, {});
    if (clearSavedState) {
        dialog->setProperty(originalPaletteProperty, {});
        dialog->setProperty(originalPaletteExplicitProperty, {});
        dialog->setProperty(originalAutoFillProperty, {});
        dialog->setProperty(originalMinimumSizeProperty, {});
        dialog->setProperty(originalMarginsProperty, {});
        dialog->setProperty(originalSpacingProperty, {});
    }
}

void prepareContentDialogState(QDialog *dialog, bool dark)
{
    if (!dialog)
        return;
    rememberPalette(dialog);
    remember(dialog, originalAutoFillProperty, dialog->autoFillBackground());
    remember(dialog, originalMinimumSizeProperty,
             QVariant::fromValue(dialog->minimumSize()));
    if (QLayout *layout = dialog->layout()) {
        remember(dialog, originalMarginsProperty,
                 QVariant::fromValue(layout->contentsMargins()));
        remember(dialog, originalSpacingProperty, layout->spacing());
        layout->setContentsMargins(24, 24, 24, 24);
        layout->setSpacing(12);
    }
    dialog->setProperty(ownedPaletteProperty, true);
    QPalette palette = dialog->palette();
    palette.setColor(QPalette::Window,
                     dark ? QColor(32, 32, 32) : QColor(255, 255, 255));
    dialog->setPalette(palette);
    dialog->setAutoFillBackground(true);
    dialog->setMinimumSize(320, 184);
}

namespace {

class SliderValueTip final : public QWidget
{
public:
    explicit SliderValueTip(QSlider *slider)
        : QWidget(slider, Qt::Tool | Qt::FramelessWindowHint
                            | Qt::WindowDoesNotAcceptFocus
                            | Qt::WindowTransparentForInput)
    {
        setObjectName(QStringLiteral("_winui_slider_value_tip"));
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_ShowWithoutActivating);
    }

    void showValue(const QString &text, const QPoint &anchor, bool horizontal)
    {
        m_text = text;
        const QFontMetrics metrics(font());
        resize(qMax(32, metrics.horizontalAdvance(text) + 16), 32);
        QPoint position = horizontal
            ? QPoint(anchor.x() - width() / 2, anchor.y() - height())
            : QPoint(anchor.x(), anchor.y() - height() / 2);
        if (QScreen *screen = QGuiApplication::screenAt(anchor)) {
            const QRect available = screen->availableGeometry();
            position.setX(qBound(available.left(), position.x(),
                                 available.right() - width() + 1));
            position.setY(qBound(available.top(), position.y(),
                                 available.bottom() - height() + 1));
        }
        move(position);
        show();
        raise();
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        const Private::Tokens t = Private::tokens(palette());
        QColor fill = palette().color(QPalette::ToolTipBase);
        fill.setAlpha(242);
        QPainter painter(this);
        roundedRect(&painter, QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5),
                    fill, t.strokeSecondary, Private::ControlRadius);
        painter.setPen(palette().color(QPalette::ToolTipText));
        painter.drawText(rect().adjusted(8, 0, -8, 0),
                         Qt::AlignCenter, m_text);
    }

private:
    QString m_text;
};

constexpr auto sliderToolTipDebounceTimerName =
    "_winui_slider_tooltip_debounce_timer";
constexpr int sliderToolTipDebounceInterval = 16;

SliderValueTip *sliderValueTip(QSlider *slider)
{
    if (!slider)
        return nullptr;
    return static_cast<SliderValueTip *>(slider->findChild<QWidget *>(
        QStringLiteral("_winui_slider_value_tip"), Qt::FindDirectChildrenOnly));
}

void clearSliderValueToolTip(QSlider *slider)
{
    if (!slider)
        return;

    if (auto *timer = slider->findChild<QTimer *>(
            QString::fromLatin1(sliderToolTipDebounceTimerName),
            Qt::FindDirectChildrenOnly)) {
        timer->stop();
    }
    framePropertyRegistry().set(slider, sliderToolTipVisibleProperty, false);
    framePropertyRegistry().clear(slider, sliderToolTipValueProperty);
    framePropertyRegistry().clear(slider, sliderToolTipSurfacePendingProperty);
    if (auto *tip = sliderValueTip(slider))
        tip->hide();
}

void updateSliderValueToolTipNow(QSlider *slider)
{
    if (!slider)
        return;
    if (!slider->isEnabled()) {
        // A slider can be disabled while the trailing debounce timer is
        // pending. Clear both the inspectable state and the native popup;
        // otherwise the last value remains visible until the next drag.
        clearSliderValueToolTip(slider);
        return;
    }

    QStyleOptionSlider option;
    option.initFrom(slider);
    option.orientation = slider->orientation();
    option.minimum = slider->minimum();
    option.maximum = slider->maximum();
    option.sliderPosition = slider->sliderPosition();
    option.sliderValue = slider->value();
    option.singleStep = slider->singleStep();
    option.pageStep = slider->pageStep();
    option.upsideDown = slider->orientation() == Qt::Horizontal
        ? (slider->invertedAppearance()
           != (slider->layoutDirection() == Qt::RightToLeft))
        : !slider->invertedAppearance();
    const QRect handle = slider->style()->subControlRect(
        QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, slider);
    const QPoint anchor = slider->orientation() == Qt::Horizontal
        ? QPoint(handle.center().x(), handle.top() - 8)
        : QPoint(handle.right() + 8, handle.center().y());
    const QString valueText = QString::number(slider->value());
    framePropertyRegistry().set(slider, sliderToolTipVisibleProperty, true);
    framePropertyRegistry().set(slider, sliderToolTipValueProperty, valueText);
    // The headless platform cannot host a non-activating tool window and
    // synthesizes a FocusOut when one is shown. State/value remain testable;
    // the popup itself is covered by the native Windows test path.
    if (QGuiApplication::platformName() == QStringLiteral("offscreen"))
        return;
    auto *tip = sliderValueTip(slider);
    if (!tip)
        tip = new SliderValueTip(slider);
    tip->showValue(valueText, slider->mapToGlobal(anchor),
                   slider->orientation() == Qt::Horizontal);
}

QTimer *sliderToolTipDebounceTimer(QSlider *slider)
{
    if (!slider)
        return nullptr;
    if (auto *timer = slider->findChild<QTimer *>(
            QString::fromLatin1(sliderToolTipDebounceTimerName),
            Qt::FindDirectChildrenOnly)) {
        return timer;
    }
    auto *timer = new QTimer(slider);
    timer->setObjectName(QString::fromLatin1(sliderToolTipDebounceTimerName));
    timer->setSingleShot(true);
    timer->setInterval(sliderToolTipDebounceInterval);
    const QPointer<QSlider> guarded(slider);
    QObject::connect(timer, &QTimer::timeout, slider, [guarded] {
        if (!guarded)
            return;
        // Close the current frame's leading-edge window so the next pointer
        // move can schedule a fresh refresh.
        framePropertyRegistry().set(guarded,
                                    sliderToolTipSurfacePendingProperty, false);
        if (guarded->isEnabled()
            && framePropertyRegistry().value(
                   guarded, sliderToolTipVisibleProperty).toBool())
            updateSliderValueToolTipNow(guarded);
    });
    return timer;
}

bool isLineEditClearButton(const QLineEdit *lineEdit,
                           const QAbstractButton *button)
{
    if (!lineEdit || !button)
        return false;
    for (QAction *action : lineEdit->actions())
        if (action->associatedObjects().contains(button))
            return false;
    // QLineEdit's private clear affordance is deliberately not a QAction;
    // custom leading/trailing actions are associated above.
    return true;
}

constexpr auto lineEditHelperUpdatePendingProperty =
    "_winui_line_edit_helper_update_pending";

qulonglong nextLineEditHelperUpdateToken()
{
    // QWidget lifecycle and style events are confined to the GUI thread.
    static qulonglong token = 0;
    return ++token;
}

void scheduleLineEditHelperUpdate(QLineEdit *lineEdit, Style *style)
{
    if (!lineEdit
        || framePropertyRegistry().value(lineEdit,
                                         lineEditHelperUpdatePendingProperty)
               .isValid())
        return;

    const qulonglong token = nextLineEditHelperUpdateToken();
    framePropertyRegistry().set(lineEdit, lineEditHelperUpdatePendingProperty,
                                QVariant::fromValue(token));
    const QPointer<QLineEdit> guardedLineEdit(lineEdit);
    const QPointer<Style> guardedStyle(style);
    QTimer::singleShot(0, lineEdit, [guardedLineEdit, guardedStyle, token] {
        if (!guardedLineEdit
            || framePropertyRegistry().value(
                   guardedLineEdit, lineEditHelperUpdatePendingProperty)
                   .toULongLong() != token)
            return;

        // updateReadOnlyDeleteAffordance() has no style parameter for callers
        // that only need the visibility contract. Resolve it at execution time
        // so a preceding update call can still be coalesced with preparation.
        Style *helperStyle = guardedStyle.data();
        if (!helperStyle)
            helperStyle = qobject_cast<Style *>(guardedLineEdit->style());

        for (QAbstractButton *button
             : guardedLineEdit->findChildren<QAbstractButton *>()) {
            if (helperStyle) {
                // QLineEdit creates its private clear affordance lazily. Depending
                // on that timing, it can miss the parent's polish pass and never
                // deliver hover events to the style. Prepare both the private
                // affordance and QAction helper buttons as soon as they exist.
                button->setAttribute(Qt::WA_Hover, true);
                button->installEventFilter(helperStyle);
                if (!framePropertyRegistry().value(button, hoverProperty).isValid())
                    framePropertyRegistry().set(button, hoverProperty,
                                                button->isEnabled()
                                                        && button->underMouse()
                                                    ? 1.0
                                                    : 0.0);
                if (!framePropertyRegistry().value(button, pressProperty).isValid())
                    framePropertyRegistry().set(button, pressProperty, 0.0);
            }

            if (isLineEditClearButton(guardedLineEdit, button))
                button->setVisible(!guardedLineEdit->isReadOnly()
                                   && guardedLineEdit->isEnabled()
                                   && guardedLineEdit->isClearButtonEnabled()
                                   && !guardedLineEdit->text().isEmpty());
        }
        if (framePropertyRegistry().value(guardedLineEdit,
                                          lineEditHelperUpdatePendingProperty)
                .toULongLong() == token) {
            framePropertyRegistry().clear(guardedLineEdit,
                                          lineEditHelperUpdatePendingProperty);
        }
    });
}

} // namespace

void updateReadOnlyDeleteAffordance(QLineEdit *lineEdit)
{
    scheduleLineEditHelperUpdate(lineEdit, nullptr);
}

void prepareLineEditHelperButtons(QLineEdit *lineEdit, Style *style)
{
    scheduleLineEditHelperUpdate(lineEdit, style);
}

void cancelLineEditHelperUpdate(QLineEdit *lineEdit)
{
    if (lineEdit)
        framePropertyRegistry().clear(lineEdit,
                                      lineEditHelperUpdatePendingProperty);
}

void showSliderValueToolTip(QSlider *slider)
{
    if (!slider || !slider->isEnabled())
        return;
    // Synchronous inspectable state is always current, regardless of how the
    // native popup is coalesced.
    framePropertyRegistry().set(slider, sliderToolTipVisibleProperty, true);
    framePropertyRegistry().set(slider, sliderToolTipValueProperty,
                                QString::number(slider->value()));
    // The headless platform cannot host a non-activating tool window; keep the
    // state synchronous and avoid allocating a timer that cannot fire.
    if (QGuiApplication::platformName() == QStringLiteral("offscreen"))
        return;
    // A refresh is already scheduled in this frame; its timeout picks up the
    // newest value, so subsequent pointer moves within the frame are coalesced.
    if (framePropertyRegistry().value(
            slider, sliderToolTipSurfacePendingProperty).toBool())
        return;
    // Leading edge of a new frame (or the first show): refresh the popup
    // immediately so it tracks the handle with no trailing-edge lag, then arm
    // a one-shot timer to re-apply the latest value at the frame boundary. The
    // trailing design in 7e2e480 only repainted after the mouse paused, which
    // made the tooltip visibly trail the handle during a fast drag.
    framePropertyRegistry().set(slider, sliderToolTipSurfacePendingProperty, true);
    if (auto *timer = sliderToolTipDebounceTimer(slider))
        timer->start();
    updateSliderValueToolTipNow(slider);
}

void hideSliderValueToolTip(QSlider *slider)
{
    clearSliderValueToolTip(slider);
}

void preparePopupSurface(QWidget *widget)
{
    if (!widget || !widget->window() || widget->window()->windowType() != Qt::Popup)
        return;
    QWidget *popup = widget->window();
    rememberPalette(popup);
    remember(popup, originalAutoFillProperty, popup->autoFillBackground());
    // Popup widgets keep an explicit palette after their first polish. Rebase
    // every show on the current application palette so runtime theme changes
    // cannot leave stale text or surface roles behind. The surface is opaque;
    // rounded corners come from the native window corner preference.
    QPalette popupPalette = effectivePopupPalette(popup, QApplication::palette());
    const Private::Tokens popupTokens = Private::tokens(popupPalette);
    const QColor popupSurface = popupTokens.dark ? QColor(44, 44, 44)
                                                 : QColor(252, 252, 252);
    popupPalette.setColor(QPalette::Window, popupSurface);
    popupPalette.setColor(QPalette::Base, popupSurface);
    popup->setPalette(popupPalette);
    popup->setAutoFillBackground(true);
    popup->setAttribute(Qt::WA_TranslucentBackground, false);
    popup->setAttribute(Qt::WA_NoSystemBackground, false);
    applyPopupRoundedCorners(popup);
    if (auto *menu = qobject_cast<QMenu *>(widget)) {
        remember(popup, originalMarginsProperty,
                 QVariant::fromValue(popup->contentsMargins()));
        menu->setContentsMargins(0, 2, 0, 2);
    }
    QAbstractItemView *view = qobject_cast<QAbstractItemView *>(widget);
    if (!view)
        view = popup->findChild<QAbstractItemView *>();
    if (view) {
        rememberPalette(view);
        const QPalette viewPalette = effectivePopupPalette(view, popupPalette);
        view->setPalette(viewPalette);
        rememberPalette(view->viewport());
        remember(view->viewport(), originalAutoFillProperty,
                 view->viewport()->autoFillBackground());
        remember(view->viewport(), originalOpaquePaintProperty,
                 view->viewport()->testAttribute(Qt::WA_OpaquePaintEvent));
        view->viewport()->setPalette(
            effectivePopupPalette(view->viewport(), viewPalette));
        view->viewport()->setAutoFillBackground(true);
        view->viewport()->setAttribute(Qt::WA_OpaquePaintEvent, true);
        if (auto *list = qobject_cast<QListView *>(view)) {
            remember(list, originalListSpacingProperty, list->spacing());
            list->setSpacing(0);
        }
    }
}

void prepareComboPopupFirstFrameImpl(QComboBox *combo)
{
    if (!combo || combo->count() <= 0)
        return;

    QAbstractItemView *view = combo->view();
    if (!view)
        return;

    preparePopupSurface(view);
    const QModelIndex current = combo->model()->index(
        combo->currentIndex(), combo->modelColumn(), combo->rootModelIndex());
    if (!current.isValid())
        return;

    view->setCurrentIndex(current);
    view->doItemsLayout();
    view->scrollTo(current, QAbstractItemView::PositionAtCenter);
}

QComboBox *comboForPopupWidget(QWidget *widget)
{
    if (!widget)
        return nullptr;
    QWidget *popup = widget->window();
    if (!popup || popup->windowType() != Qt::Popup)
        return nullptr;
    return qobject_cast<QComboBox *>(popup->parentWidget());
}

} // namespace WinUI3::Private
