#pragma once

#include <QHash>
#include <QPoint>
#include <QVariant>

#include <functional>

class QCheckBox;
class QDialog;
class QFrame;
class QLineEdit;
class QComboBox;
class QProgressBar;
class QScrollBar;
class QSlider;
class QAbstractItemView;
class QObject;
class QEvent;
class QWidget;

namespace WinUI3 {

class Style;

namespace Private {

struct ToggleDragState {
    QPoint pressPosition;
    bool candidate = false;
    bool dragging = false;
};

// The event filter is intentionally kept out of Style's rendering unit.  The
// callbacks are the small seam to StylePrivate: interaction code owns event
// routing, while StylePrivate continues to own animation and lifecycle state.
struct StyleInteractionCallbacks {
    std::function<void(QWidget *, const char *, qreal, int)> animate;
    std::function<void(QWidget *)> beginButtonPress;
    std::function<void(QWidget *)> releaseButtonPress;
    std::function<void(QWidget *)> cancelButtonPress;
    std::function<void(QWidget *)> stopAnimations;
    std::function<void(QWidget *)> clearPointerInteraction;
    std::function<void(QScrollBar *)> cancelScrollBarTimer;
    std::function<void(QScrollBar *, int)> scheduleScrollBar;
    std::function<void(QSlider *)> scheduleSliderToolTip;
    std::function<void(QSlider *)> cancelSliderToolTip;
    std::function<void()> refreshProgressTimer;
    std::function<bool()> progressTimerActive;
    std::function<void(QComboBox *)> prepareComboPopupFirstFrame;
    std::function<void(QWidget *)> releaseComboChevron;
    std::function<void(QWidget *)> finishComboPopupCycle;
    std::function<QComboBox *(QWidget *)> comboForPopupWidget;
    std::function<void(QLineEdit *)> updateReadOnlyDeleteAffordance;
    std::function<void(QLineEdit *)> prepareLineEditHelperButtons;
    std::function<void(QDialog *, bool)> prepareContentDialogState;
    std::function<void(QDialog *)> stopDialogAnimations;
    std::function<void(QWidget *)> preparePopupSurface;
    std::function<void(QWidget *)> registerPopupPaletteOwners;
    std::function<void(QDialog *)> registerPaletteOwner;
    std::function<void(QDialog *)> unregisterPaletteOwner;
    std::function<void(QDialog *, bool)> restoreContentDialogState;
    std::function<void(QWidget *, const char *, const QVariant &)> remember;
    std::function<void(QAbstractItemView *)> prepareNavigationView;
    std::function<void(QAbstractItemView *)> restoreNavigationView;
    std::function<bool()> dark;

    bool *keyboardInput = nullptr;
    QHash<QCheckBox *, ToggleDragState> *toggleDragStates = nullptr;
};

class StyleInteractionController final
{
public:
    StyleInteractionController(Style *style,
                               StyleInteractionCallbacks callbacks);

    bool eventFilter(QObject *watched, QEvent *event);

private:
    Style *m_style = nullptr;
    StyleInteractionCallbacks m_callbacks;
};

} // namespace Private
} // namespace WinUI3
