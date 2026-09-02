#include "winui3interactions_p.h"
#include "winui3qtcompat_p.h"

#include "winui3backdrop_p.h"
#include "winui3frameproperties_p.h"
#include "winui3geometry_p.h"
#include "winui3helpers_p.h"
#include "winui3style_properties_p.h"
#include "winui3surfaces_p.h"
#include "winui3tokens_p.h"

#include <winui3style/winui3style.h>

#include <winui3style/winui3backdrop.h>

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAbstractSpinBox>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDynamicPropertyChangeEvent>
#include <QEvent>
#include <QFocusEvent>
#include <QFrame>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QMessageBox>
#include <QParallelAnimationGroup>
#include <QPainter>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollBar>
#include <QScreen>
#include <QSlider>
#include <QStyleOptionToolButton>
#include <QStyleOptionComboBox>
#include <QTabBar>
#include <QTextEdit>
#include <QToolButton>

namespace WinUI3::Private {
namespace {

bool comboPressOpensPopup(QComboBox *combo, const QPoint &position)
{
    if (!combo)
        return false;
    if (combo->isEditable()) {
        QStyleOptionComboBox option;
        option.initFrom(combo);
        option.rect = combo->rect();
        option.direction = combo->layoutDirection();
        const QRect arrow = combo->style()->subControlRect(
            QStyle::CC_ComboBox, &option, QStyle::SC_ComboBoxArrow, combo);
        return arrow.contains(position);
    }
    return true;
}

bool comboPopupItemView(const QWidget *widget)
{
    const QWidget *candidate = widget;
    while (candidate) {
        if (qobject_cast<const QAbstractItemView *>(candidate))
            break;
        candidate = candidate->parentWidget();
    }
    if (!candidate || !candidate->window()
        || candidate->window()->windowType() != Qt::Popup)
        return false;
    return qobject_cast<const QComboBox *>(candidate->window()->parentWidget());
}

void centerPendingComboPopup(QWidget *popup, QComboBox *combo)
{
    if (!popup || !combo || combo->currentIndex() < 0)
        return;
    QAbstractItemView *view = combo->view();
    if (!view || !view->viewport())
        return;
    const QModelIndex current = combo->model()->index(
        combo->currentIndex(), combo->modelColumn(), combo->rootModelIndex());
    const QRect selected = view->visualRect(current);
    if (!selected.isValid())
        return;
    const QPoint comboCenter = combo->mapToGlobal(combo->rect().center());
    // QComboBox's private popup container can report the viewport's previous
    // global origin during a reused QEvent::Show. Derive the anchor from the
    // stable popup inset and item geometry instead, otherwise later openings
    // drift by the combined 4px top/bottom margin.
    const int selectedCenterInPopup = popup->contentsRect().top()
        + view->frameWidth() + selected.center().y();
    int targetY = comboCenter.y() - selectedCenterInPopup;
    if (QScreen *screen = widgetScreen(popup)) {
        const QRect available = screen->availableGeometry();
        const int maximumY = qMax(available.top(),
                                  available.bottom() - popup->height() + 1);
        targetY = qBound(available.top(), targetY, maximumY);
    }
    if (targetY != popup->y())
        popup->move(popup->x(), targetY);
}

enum class InteractionMotion {
    Hover,
    Press,
    Focus
};

int interactionDuration(const QWidget *widget, InteractionMotion motion,
                        bool active)
{
    if (comboPopupItemView(widget))
        return 167;
    // TextBox and NumberBox switch their common visual states through setters;
    // their templates define no timed transition. The TextBox helper button is
    // discrete as well. Other button-like surfaces use WinUI's faster brush
    // transition, while RadioButton's dot uses the normal duration.
    if (qobject_cast<const QLineEdit *>(widget)
        || qobject_cast<const QAbstractSpinBox *>(widget)
        || qobject_cast<const QTabBar *>(widget)
        || textBoxHelperButton(widget)) {
        return 0;
    }
    if (qobject_cast<const QRadioButton *>(widget))
        return NormalDuration;
    if (qobject_cast<const QSlider *>(widget)) {
        if (motion == InteractionMotion::Focus)
            return 0;
        return active ? NormalDuration : FastDuration;
    }
    if (qobject_cast<const QScrollBar *>(widget))
        return FastDuration;
    return FasterDuration;
}

bool revealsKeyboardFocus(int key)
{
    switch (key) {
    case Qt::Key_Tab:
    case Qt::Key_Backtab:
    case Qt::Key_Left:
    case Qt::Key_Right:
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_Home:
    case Qt::Key_End:
    case Qt::Key_PageUp:
    case Qt::Key_PageDown:
    case Qt::Key_Space:
    case Qt::Key_Return:
    case Qt::Key_Enter:
        return true;
    default:
        return false;
    }
}

} // namespace

StyleInteractionController::StyleInteractionController(
    Style *style, StyleInteractionCallbacks callbacks)
    : m_style(style), m_callbacks(std::move(callbacks))
{
}

bool StyleInteractionController::eventFilter(QObject *watched, QEvent *event)
{
    auto *widget = qobject_cast<QWidget *>(watched);
    if (!widget)
        return false;

    switch (event->type()) {
    case QEvent::EnabledChange: {
        // Qt normally stops delivering pointer events to a disabled widget,
        // but an in-flight style animation has no such protection. Clear the
        // interaction state at the source so disabling during a press cannot
        // leave a stale hover/pressed surface behind.
        m_callbacks.clearPointerInteraction(widget);
        const bool enabled = widget->isEnabled();
        framePropertyRegistry().set(widget, hoverProperty,
                                    enabled && widget->underMouse()
                                        ? 1.0 : 0.0);
        framePropertyRegistry().set(widget, pressProperty, 0.0);
        framePropertyRegistry().set(widget, focusProperty,
                                    enabled && widget->hasFocus() ? 1.0 : 0.0);
        framePropertyRegistry().set(widget, focusVisibleProperty,
                                    enabled && widget->hasFocus()
                                        && *m_callbacks.keyboardInput);
        if (auto *checkBox = qobject_cast<QCheckBox *>(widget)) {
            framePropertyRegistry().set(checkBox, checkProperty,
                                        checkBox->checkState() == Qt::Unchecked
                                            ? 0.0 : 1.0);
            framePropertyRegistry().set(checkBox, togglePositionProperty,
                                        checkBox->isChecked() ? 1.0 : 0.0);
        } else if (auto *radio = qobject_cast<QRadioButton *>(widget)) {
            framePropertyRegistry().set(radio, checkProperty,
                                        radio->isChecked() ? 1.0 : 0.0);
        }
        if (auto *combo = qobject_cast<QComboBox *>(widget)) {
            if (m_comboPressStates.remove(combo)) {
                combo->releaseMouse();
                m_callbacks.releaseComboChevron(combo);
            }
        }
        if (auto *scrollBar = qobject_cast<QScrollBar *>(widget))
            m_callbacks.cancelScrollBarTimer(scrollBar);
        if (auto *slider = qobject_cast<QSlider *>(widget)) {
            m_callbacks.cancelSliderToolTip(slider);
            if (!enabled)
                hideSliderValueToolTip(slider);
        }
        if (qobject_cast<QProgressBar *>(widget))
            m_callbacks.refreshProgressTimer();
        m_callbacks.updateReadOnlyDeleteAffordance(
            qobject_cast<QLineEdit *>(widget));
        widget->update();
        break;
    }
    case QEvent::Paint:
        if (auto *button = qobject_cast<QToolButton *>(widget);
            button && textBoxHelperButton(button)) {
            // QLineEditIconButton has a private paintEvent that bypasses
            // QToolButton's CC_ToolButton path. Own its complete rendering so
            // the WinUI DeleteButton surface is refreshed on real pointer
            // transitions, not only during a forced parent render.
            QStyleOptionToolButton option;
            option.initFrom(button);
            option.rect = button->rect();
            option.icon = button->icon();
            option.iconSize = button->iconSize();
            option.text = button->text();
            option.toolButtonStyle = button->toolButtonStyle();
            option.arrowType = button->arrowType();
            option.features = QStyleOptionToolButton::None;
            QPainter painter(button);
            const QVariant opacity = button->property("opacity");
            if (opacity.isValid())
                painter.setOpacity(opacity.toReal());
            m_style->drawPrimitive(QStyle::PE_PanelButtonTool, &option,
                                   &painter, button);
            m_style->drawControl(QStyle::CE_ToolButtonLabel, &option,
                                 &painter, button);
            return true;
        }
        break;
    case QEvent::UpdateRequest:
        // QProgressBar exposes no rangeChanged signal. Refresh only while the
        // shared clock is stopped, so ordinary timer-driven updates remain
        // O(1) in callbacks and do not rescan the registry per paint.
        if (qobject_cast<QProgressBar *>(widget)
            && !m_callbacks.progressTimerActive())
            m_callbacks.refreshProgressTimer();
        break;
    case QEvent::Enter:
        if (!widget->isEnabled()) {
            m_callbacks.clearPointerInteraction(widget);
            widget->update();
            break;
        }
        if (auto *scrollBar = qobject_cast<QScrollBar *>(widget)) {
            const int generation = framePropertyRegistry()
                .value(widget, scrollBarGenerationProperty).toInt() + 1;
            framePropertyRegistry().set(widget, scrollBarInsideProperty, true);
            framePropertyRegistry().set(widget, scrollBarGenerationProperty,
                                        generation);
            if (!Style::animationsAllowed()) {
                m_callbacks.cancelScrollBarTimer(scrollBar);
                m_callbacks.animate(widget, hoverProperty, 1.0, 0);
            } else if (progress(widget, hoverProperty) > 0.001) {
                // Re-entering during contraction reverses from the current
                // thickness immediately. Waiting for a fresh 400 ms reveal
                // would make the thumb disappear under a stationary pointer.
                m_callbacks.cancelScrollBarTimer(scrollBar);
                m_callbacks.animate(widget, hoverProperty, 1.0, FastDuration);
            } else {
                m_callbacks.scheduleScrollBar(scrollBar, 400);
            }
        } else {
            m_callbacks.animate(widget, hoverProperty, 1.0,
                                interactionDuration(widget,
                                                    InteractionMotion::Hover,
                                                    true));
        }
        break;
    case QEvent::Leave:
        if (!widget->isEnabled()) {
            m_callbacks.clearPointerInteraction(widget);
            widget->update();
            break;
        }
        if (auto *scrollBar = qobject_cast<QScrollBar *>(widget)) {
            const int generation = framePropertyRegistry()
                .value(widget, scrollBarGenerationProperty).toInt() + 1;
            framePropertyRegistry().set(widget, scrollBarInsideProperty, false);
            framePropertyRegistry().set(widget, scrollBarGenerationProperty,
                                        generation);
            if (!Style::animationsAllowed()) {
                m_callbacks.cancelScrollBarTimer(scrollBar);
                m_callbacks.animate(widget, hoverProperty, 0.0, 0);
            } else {
                m_callbacks.scheduleScrollBar(scrollBar, 500);
            }
        } else {
            m_callbacks.animate(widget, hoverProperty, 0.0,
                                interactionDuration(widget,
                                                    InteractionMotion::Hover,
                                                    false));
        }
        if (buttonPressPulse(widget))
            m_callbacks.cancelButtonPress(widget);
        else
            m_callbacks.animate(widget, pressProperty, 0.0,
                                interactionDuration(widget,
                                                    InteractionMotion::Press,
                                                    false));
        break;
    case QEvent::MouseButtonPress:
        *m_callbacks.keyboardInput = false;
        framePropertyRegistry().set(widget, focusVisibleProperty, false);
        if (!widget->isEnabled()) {
            m_callbacks.clearPointerInteraction(widget);
            widget->update();
            break;
        }
        if (auto *viewport = qobject_cast<QAbstractItemView *>(widget->parentWidget());
            viewport && viewport->viewport() == widget) {
            framePropertyRegistry().set(viewport, focusVisibleProperty, false);
            viewport->viewport()->update();
        }
        if (auto *combo = qobject_cast<QComboBox *>(widget)) {
            const auto *mouse = static_cast<QMouseEvent *>(event);
            if (mouse->button() == Qt::LeftButton
                && comboPressOpensPopup(combo, mousePositionPoint(mouse))) {
                // QComboBox's platform-independent default opens its popup
                // from mousePressEvent. WinUI opens on release, so consume
                // this press and keep a grab until release. The grab is
                // important when the pointer leaves the combo: Qt will then
                // still deliver the release here and we can correctly cancel
                // instead of opening a popup at an unrelated target.
                m_comboPressStates.insert(combo);
                combo->grabMouse();
                m_callbacks.animate(combo, comboChevronProperty, 1.0, 150);
                m_callbacks.animate(combo, pressProperty, 1.0,
                                    interactionDuration(
                                        combo, InteractionMotion::Press, true));
                return true;
            }
        }
        if (auto *checkBox = qobject_cast<QCheckBox *>(widget);
            checkBox && toggleSwitch(checkBox)) {
            const auto *mouse = static_cast<QMouseEvent *>(event);
            if (mouse->button() == Qt::LeftButton) {
                const QRect track = toggleTrackRect(checkBox->rect(),
                                                    checkBox->layoutDirection());
                ToggleDragState state;
                state.pressPosition = mousePositionPoint(mouse);
                state.candidate = track.contains(state.pressPosition);
                m_callbacks.toggleDragStates->insert(checkBox, state);
            }
        }
        if (auto *slider = qobject_cast<QSlider *>(widget)) {
            const auto *mouse = static_cast<QMouseEvent *>(event);
            if (mouse->button() == Qt::LeftButton && slider->isEnabled())
                m_callbacks.scheduleSliderToolTip(slider);
        }
        if (static_cast<const QMouseEvent *>(event)->button() != Qt::LeftButton)
            break;
        if (buttonPressPulse(widget))
            m_callbacks.beginButtonPress(widget);
        else
            m_callbacks.animate(widget, pressProperty, 1.0,
                                interactionDuration(widget,
                                                    InteractionMotion::Press,
                                                    true));
        break;
    case QEvent::MouseMove:
        if (auto *slider = qobject_cast<QSlider *>(widget)) {
            const auto *mouse = static_cast<QMouseEvent *>(event);
            if ((mouse->buttons() & Qt::LeftButton) && slider->isEnabled())
                m_callbacks.scheduleSliderToolTip(slider);
        }
        if (auto *checkBox = qobject_cast<QCheckBox *>(widget);
            checkBox && toggleSwitch(checkBox)) {
            auto state = m_callbacks.toggleDragStates->find(checkBox);
            const auto *mouse = static_cast<QMouseEvent *>(event);
            if (state != m_callbacks.toggleDragStates->end() && state->candidate
                && (mouse->buttons() & Qt::LeftButton)) {
                if (!state->dragging
                    && (mousePositionPoint(mouse) - state->pressPosition)
                            .manhattanLength()
                        >= QApplication::startDragDistance()) {
                    state->dragging = true;
                    framePropertyRegistry().set(checkBox,
                                                toggleDraggingProperty, true);
                }
                if (state->dragging) {
                    qreal position;
                    if (checkBox->layoutDirection() == Qt::RightToLeft)
                        position = (checkBox->rect().right() - 10.0
                                    - mousePosition(mouse).x()) / 20.0;
                    else
                        position = (mousePosition(mouse).x()
                                    - checkBox->rect().left() - 10.0) / 20.0;
                    framePropertyRegistry().set(
                        checkBox, togglePositionProperty,
                        qBound<qreal>(0.0, position, 1.0));
                    checkBox->update();
                    return true;
                }
            }
        }
        break;
    case QEvent::MouseButtonRelease:
        if (auto *combo = qobject_cast<QComboBox *>(widget)) {
            auto pending = m_comboPressStates.find(combo);
            if (pending != m_comboPressStates.end()
                && static_cast<const QMouseEvent *>(event)->button()
                       == Qt::LeftButton) {
                const QPoint position = mousePositionPoint(
                    static_cast<const QMouseEvent *>(event));
                m_comboPressStates.erase(pending);
                combo->releaseMouse();
                const bool activate = combo->rect().contains(position);
                m_callbacks.animate(combo, pressProperty, 0.0,
                                    interactionDuration(
                                        combo, InteractionMotion::Press, false));
                if (activate) {
                    // Prepare the first selected row immediately before the
                    // release-triggered popup is shown. This keeps Qt's
                    // keyboard/programmatic path unchanged and avoids a
                    // visible press-time popup/layout pass.
                    m_callbacks.prepareComboPopupFirstFrame(combo);
                    combo->showPopup();
                }
                m_callbacks.releaseComboChevron(combo);
                return true;
            }
        }
        if (!widget->isEnabled()) {
            m_callbacks.clearPointerInteraction(widget);
            widget->update();
            break;
        }
        if (static_cast<const QMouseEvent *>(event)->button() != Qt::LeftButton) {
            if (auto *slider = qobject_cast<QSlider *>(widget)) {
                m_callbacks.cancelSliderToolTip(slider);
                hideSliderValueToolTip(slider);
            }
            break;
        }
        if (auto *slider = qobject_cast<QSlider *>(widget)) {
            m_callbacks.cancelSliderToolTip(slider);
            hideSliderValueToolTip(slider);
        }
        if (qobject_cast<QComboBox *>(widget))
            m_callbacks.releaseComboChevron(widget);
        if (auto *checkBox = qobject_cast<QCheckBox *>(widget);
            checkBox && toggleSwitch(checkBox)) {
            const auto state = m_callbacks.toggleDragStates->take(checkBox);
            if (state.dragging) {
                const bool checked = progress(checkBox, togglePositionProperty)
                    >= 0.5;
                framePropertyRegistry().set(checkBox,
                                            toggleDraggingProperty, false);
                checkBox->setDown(false);
                Q_EMIT checkBox->released();
                if (checkBox->isChecked() != checked)
                    checkBox->setChecked(checked);
                else
                    m_callbacks.animate(checkBox, togglePositionProperty,
                                        checked ? 1.0 : 0.0, FasterDuration);
                Q_EMIT checkBox->clicked(checkBox->isChecked());
                m_callbacks.animate(checkBox, pressProperty, 0.0,
                                    FasterDuration);
                return true;
            }
        }
        if (buttonPressPulse(widget))
            m_callbacks.releaseButtonPress(widget);
        else
            m_callbacks.animate(widget, pressProperty, 0.0,
                                interactionDuration(widget,
                                                    InteractionMotion::Press,
                                                    false));
        break;
    case QEvent::FocusIn:
        if (const auto *focus = static_cast<QFocusEvent *>(event)) {
            const bool keyboard = focus->reason() == Qt::TabFocusReason
                || focus->reason() == Qt::BacktabFocusReason
                || focus->reason() == Qt::ShortcutFocusReason
                || *m_callbacks.keyboardInput;
            framePropertyRegistry().set(widget, focusVisibleProperty, keyboard);
            if (auto *view = qobject_cast<QAbstractItemView *>(widget->parentWidget());
                view && view->viewport() == widget) {
                if (keyboard) {
                    framePropertyRegistry().set(view, focusVisibleProperty, true);
                    view->update();
                }
            }
        }
        m_callbacks.animate(widget, focusProperty, 1.0,
                            interactionDuration(widget,
                                                InteractionMotion::Focus,
                                                true));
        m_callbacks.updateReadOnlyDeleteAffordance(
            qobject_cast<QLineEdit *>(widget));
        break;
    case QEvent::FocusOut:
        if (auto *combo = qobject_cast<QComboBox *>(widget)) {
            if (m_comboPressStates.remove(combo)) {
                combo->releaseMouse();
                m_callbacks.animate(combo, pressProperty, 0.0,
                                    interactionDuration(
                                        combo, InteractionMotion::Press, false));
                m_callbacks.releaseComboChevron(combo);
            }
        }
        if (auto *slider = qobject_cast<QSlider *>(widget)) {
            m_callbacks.cancelSliderToolTip(slider);
            hideSliderValueToolTip(slider);
        }
        framePropertyRegistry().set(widget, focusVisibleProperty, false);
        if (auto *view = qobject_cast<QAbstractItemView *>(widget->parentWidget());
            view && view->viewport() == widget) {
            framePropertyRegistry().set(view, focusVisibleProperty, false);
            view->update();
        }
        m_callbacks.animate(widget, focusProperty, 0.0,
                            interactionDuration(widget,
                                                InteractionMotion::Focus,
                                                false));
        m_callbacks.updateReadOnlyDeleteAffordance(
            qobject_cast<QLineEdit *>(widget));
        break;
    case QEvent::ReadOnlyChange:
        // WinUI TextBox does not expose its delete affordance while it is
        // read-only. QLineEdit keeps the private clear button visible, so
        // suppress it after Qt has processed the property change.
        m_callbacks.updateReadOnlyDeleteAffordance(
            qobject_cast<QLineEdit *>(widget));
        m_callbacks.prepareLineEditHelperButtons(qobject_cast<QLineEdit *>(widget));
        break;
    case QEvent::ChildAdded:
        // The private QLineEdit clear button is created after the line edit in
        // common construction orders; catch that lifecycle deterministically.
        m_callbacks.prepareLineEditHelperButtons(qobject_cast<QLineEdit *>(widget));
        break;
    case QEvent::ChildRemoved:
        // Rebuild the cached clear-button handle if Qt replaces its private
        // helper during a style or layout transition.
        m_callbacks.prepareLineEditHelperButtons(qobject_cast<QLineEdit *>(widget));
        break;
    case QEvent::KeyPress:
        if (auto *combo = qobject_cast<QComboBox *>(widget)) {
            const auto *key = static_cast<QKeyEvent *>(event);
            const bool activates = key->key() == Qt::Key_Space
                || key->key() == Qt::Key_Enter || key->key() == Qt::Key_Return
                || key->key() == Qt::Key_F4
                || (key->key() == Qt::Key_Down
                    && key->modifiers().testFlag(Qt::AltModifier));
            if (activates) {
                m_callbacks.animate(combo, comboChevronProperty, 1.0, 150);
                m_callbacks.prepareComboPopupFirstFrame(combo);
            }
        }
        if (const auto *key = static_cast<QKeyEvent *>(event);
            revealsKeyboardFocus(key->key())) {
            *m_callbacks.keyboardInput = true;
        }
        if (*m_callbacks.keyboardInput) {
            framePropertyRegistry().set(widget, focusVisibleProperty, true);
            widget->update();
        }
        break;
    case QEvent::KeyRelease:
        if (auto *combo = qobject_cast<QComboBox *>(widget)) {
            const auto *key = static_cast<QKeyEvent *>(event);
            if (key->key() == Qt::Key_Space || key->key() == Qt::Key_Enter
                || key->key() == Qt::Key_Return || key->key() == Qt::Key_F4
                || (key->key() == Qt::Key_Down
                    && key->modifiers().testFlag(Qt::AltModifier))) {
                m_callbacks.releaseComboChevron(combo);
            }
        }
        break;
    case QEvent::Show:
        // A tooltip HWND keeps the opaque system backing store; the rounded
        // PE_PanelTipLabel card leaves dark corner rectangles visible. Clip
        // the window to the card's 4px rounding with a window region.
        if (widget->isWindow() && widget->windowType() == Qt::ToolTip
            && widget->windowHandle())
            applyWindowRoundedRegion(widget, 5);
        // Establish popup margins before measuring or anchoring any child view.
        // Doing this after ComboBox centering shifts the reused popup by the
        // combined top/bottom inset on its second opening.
        m_callbacks.preparePopupSurface(widget);
        m_callbacks.updateReadOnlyDeleteAffordance(
            qobject_cast<QLineEdit *>(widget));
        if (textBoxHelperButton(widget))
            m_callbacks.updateReadOnlyDeleteAffordance(
                qobject_cast<QLineEdit *>(widget->parentWidget()));
        m_callbacks.prepareLineEditHelperButtons(qobject_cast<QLineEdit *>(widget));
        // The view is shown before its popup window. Prepare selection and
        // scroll position here so even a programmatic first showPopup() has a
        // stable first composited frame; waiting for the popup Show event is
        // observably too late when the selected item is not row zero.
        if (qobject_cast<QAbstractItemView *>(widget)) {
            if (auto *combo = m_callbacks.comboForPopupWidget(widget)) {
                m_callbacks.prepareComboPopupFirstFrame(combo);
                // A reused popup does not necessarily move or resize when a
                // different row becomes current. Recenter it here, after the
                // view layout is prepared but before the popup is composited.
                centerPendingComboPopup(widget->window(), combo);
            }
        }
        if (widget->isWindow() && widget->windowType() == Qt::Popup) {
            if (auto *combo = qobject_cast<QComboBox *>(widget->parentWidget())) {
                m_callbacks.prepareComboPopupFirstFrame(combo);
                centerPendingComboPopup(widget, combo);
            }
        }
        if (auto *dialog = qobject_cast<QDialog *>(widget);
            dialog && (qobject_cast<QMessageBox *>(dialog)
                       || dialog->property(Style::ContentDialogProperty).toBool())) {
            m_callbacks.prepareContentDialogState(dialog,
                                                   m_callbacks.dark());
            m_callbacks.stopDialogAnimations(dialog);
            showContentDialogScrim(dialog);
            applyDialogCaptionTheme(dialog);
            if (Style::animationsAllowed()) {
                widget->setProperty("_winui_dialog_animating", true);
                auto *group = new QParallelAnimationGroup(dialog);
                group->setObjectName(QStringLiteral("_winui_dialog_animation"));
                auto *opacity = new QPropertyAnimation(dialog, "windowOpacity", group);
                opacity->setStartValue(0.0);
                opacity->setEndValue(1.0);
                opacity->setDuration(FasterDuration);
                QObject::connect(group, &QParallelAnimationGroup::finished,
                                 dialog, [dialog, group] {
                    dialog->setWindowOpacity(1.0);
                    dialog->setProperty("_winui_dialog_animating", false);
                    group->deleteLater();
                });
                group->start();
            }
        }
        if (qobject_cast<QProgressBar *>(widget))
            m_callbacks.refreshProgressTimer();
        m_callbacks.registerPopupPaletteOwners(widget);
        break;
    case QEvent::WinIdChange:
        // Re-apply the native DWM material (Mica on the main window) once Qt
        // has created the HWND. Popups stay opaque and get their rounded
        // corners from the window corner preference instead of a translucent
        // surface, which avoids the cleared-backing artifacts of Acrylic.
        if (widget->isWindow()
            && widget->property("_winui_backdrop").isValid()) {
            applyBackdrop(widget, static_cast<Backdrop>(
                widget->property("_winui_backdrop").toInt()));
        }
        if (qobject_cast<QDialog *>(widget) && widget->isWindow())
            applyDialogCaptionTheme(widget);
        break;
    case QEvent::Move:
        break;
    case QEvent::Resize:
        // The DWM corner preference is resize-stable, but the legacy region
        // fallback is not. Re-apply it after every native popup resize so a
        // combo/menu cannot retain the old rounded region and clip its new
        // edges. Avoid creating a native handle during pre-show layout.
        if (widget->isWindow() && widget->windowType() == Qt::Popup
            && widget->windowHandle())
            applyPopupRoundedCorners(widget);
        if (widget->isWindow() && widget->windowType() == Qt::ToolTip
            && widget->windowHandle())
            applyWindowRoundedRegion(widget, 5);
        break;
    case QEvent::Hide:
        if (auto *combo = qobject_cast<QComboBox *>(widget)) {
            if (m_comboPressStates.remove(combo)) {
                combo->releaseMouse();
                m_callbacks.animate(combo, pressProperty, 0.0,
                                    interactionDuration(
                                        combo, InteractionMotion::Press, false));
                m_callbacks.releaseComboChevron(combo);
            }
        }
        if (qobject_cast<QProgressBar *>(widget))
            m_callbacks.refreshProgressTimer();
        if (auto *scrollBar = qobject_cast<QScrollBar *>(widget))
            m_callbacks.cancelScrollBarTimer(scrollBar);
        if (auto *slider = qobject_cast<QSlider *>(widget)) {
            m_callbacks.cancelSliderToolTip(slider);
            hideSliderValueToolTip(slider);
        }
        if (widget->isWindow() && widget->windowType() == Qt::Popup)
            m_callbacks.finishComboPopupCycle(widget);
        if (auto *dialog = qobject_cast<QDialog *>(widget)) {
            hideContentDialogScrim(dialog);
            if (dialog->property("_winui_dialog_animating").toBool())
                m_callbacks.stopDialogAnimations(dialog);
        }
        break;
    case QEvent::DynamicPropertyChange:
        if (const auto *change = static_cast<QDynamicPropertyChangeEvent *>(event)) {
            const QByteArray name = change->propertyName();
            if (name == Style::ToggleSwitchProperty) {
                if (auto *checkBox = qobject_cast<QCheckBox *>(widget))
                    framePropertyRegistry().set(
                        checkBox, togglePositionProperty,
                        checkBox->isChecked() ? 1.0 : 0.0);
                widget->updateGeometry();
                widget->update();
            } else if (name == Style::ToggleSwitchOnTextProperty
                       || name == Style::ToggleSwitchOffTextProperty) {
                widget->updateGeometry();
                widget->update();
            } else if (name == Style::SettingsCardProperty) {
                if (auto *frame = qobject_cast<QFrame *>(widget)) {
                    if (widget->property(Style::SettingsCardProperty).toBool()) {
                        m_callbacks.remember(frame, originalFrameShapeProperty,
                                             int(frame->frameShape()));
                        frame->setFrameShape(QFrame::StyledPanel);
                    } else if (widget->property(originalFrameShapeProperty).isValid()) {
                        frame->setFrameShape(static_cast<QFrame::Shape>(
                            widget->property(originalFrameShapeProperty).toInt()));
                        widget->setProperty(originalFrameShapeProperty, {});
                    }
                }
                widget->updateGeometry();
                widget->update();
            } else if (name == Style::ContentDialogProperty) {
                if (auto *dialog = qobject_cast<QDialog *>(widget);
                    dialog && !qobject_cast<QMessageBox *>(dialog)) {
                    if (dialog->property(Style::ContentDialogProperty).toBool()) {
                        m_callbacks.prepareContentDialogState(dialog,
                                                               m_callbacks.dark());
                        m_callbacks.registerPaletteOwner(dialog);
                    } else {
                        m_callbacks.unregisterPaletteOwner(dialog);
                        m_callbacks.restoreContentDialogState(dialog, true);
                    }
                }
            } else if (name == Style::VerticalSpinButtonsProperty) {
                widget->updateGeometry();
                widget->update();
            } else if (name == Style::NavigationViewProperty) {
                if (auto *view = qobject_cast<QAbstractItemView *>(widget)) {
                    view->viewport()->setProperty(Style::NavigationViewProperty,
                                                  view->property(Style::NavigationViewProperty));
                    if (view->property(Style::NavigationViewProperty).toBool())
                        m_callbacks.prepareNavigationView(view);
                    else
                        m_callbacks.restoreNavigationView(view);
                    view->viewport()->update();
                }
            }
        }
        break;
    default:
        break;
    }
    return false;
}

} // namespace WinUI3::Private
