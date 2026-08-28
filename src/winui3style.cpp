#include <winui3style/winui3style.h>

#include <winui3style/winui3backdrop.h>
#include <winui3style/winui3icons.h>

#include "winui3geometry_p.h"
#include "winui3animations_p.h"
#include "winui3buttons_p.h"
#include "winui3frameproperties_p.h"
#include "winui3menus_p.h"
#include "winui3style_contracts_p.h"
#include "winui3complex_p.h"
#include "winui3viewrenderers_p.h"
#include "navigationview_p.h"
#include "winui3paint_p.h"
#include "winui3style_properties_p.h"
#include "winui3interactions_p.h"
#include "winui3surfaces_p.h"
#include "winui3tableeditors_p.h"
#include "winui3theme_p.h"
#include "winui3tokens_p.h"
#include "winui3appearancewatcher_p.h"

#include <QAbstractButton>
#include <QAbstractItemDelegate>
#include <QAbstractItemView>
#include <QAbstractScrollArea>
#include <QAbstractSpinBox>
#include <QApplication>
#include <QComboBox>
#include <QCheckBox>
#include <QCommonStyle>
#include <QDateTime>
#include <QDialog>
#include <QDockWidget>
#include <QDynamicPropertyChangeEvent>
#include <QEvent>
#include <QFocusEvent>
#include <QFrame>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QGroupBox>
#include <QGuiApplication>
#include <QHeaderView>
#include <QLinearGradient>
#include <QListView>
#include <QLineF>
#include <QLineEdit>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QParallelAnimationGroup>
#include <QPainter>
#include <QPainterPath>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPointer>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollBar>
#include <QSet>
#include <QSlider>
#include <QStyleOption>
#include <QStyleOptionButton>
#include <QStyleOptionDockWidget>
#include <QStyleOptionFocusRect>
#include <QStyleOptionGroupBox>
#include <QStyleOptionProgressBar>
#include <QStyleOptionTab>
#include <QStyleOptionToolButton>
#include <QStyleHints>
#include <QStyledItemDelegate>
#include <QTabBar>
#include <QTableView>
#include <QTextEdit>
#include <QToolBar>
#include <QToolButton>
#include <QTimer>
#include <QTreeView>
#include <QVariantAnimation>
#include <QVector>
#include <QWidget>

#include <cmath>

namespace WinUI3 {
using namespace PaintPrivate;
using namespace Private;
namespace {

bool toggleSwitch(const QWidget *widget)
{
    return qobject_cast<const QCheckBox *>(widget)
        && widget->property(Style::ToggleSwitchProperty).toBool();
}

bool verticalSpinButtons(const QWidget *widget)
{
    return qobject_cast<const QAbstractSpinBox *>(widget)
        && widget->property(Style::VerticalSpinButtonsProperty).toBool();
}

bool spinBoxEditor(const QWidget *widget)
{
    return qobject_cast<const QLineEdit *>(widget)
        && qobject_cast<const QAbstractSpinBox *>(widget->parentWidget());
}

bool textBoxHelperButton(const QWidget *widget)
{
    return qobject_cast<const QAbstractButton *>(widget)
        && qobject_cast<const QLineEdit *>(widget->parentWidget());
}

bool buttonPressPulse(const QWidget *widget)
{
    return !textBoxHelperButton(widget)
        && (qobject_cast<const QPushButton *>(widget)
            || qobject_cast<const QToolButton *>(widget));
}

const QAbstractItemView *itemView(const QWidget *widget)
{
    if (const auto *view = qobject_cast<const QAbstractItemView *>(widget))
        return view;
    for (const QWidget *candidate = widget; candidate;
         candidate = candidate->parentWidget()) {
        if (const auto *view = qobject_cast<const QAbstractItemView *>(candidate))
            return view;
    }
    return nullptr;
}

const QWidget *richTextEditor(const QWidget *widget)
{
    for (const QWidget *candidate = widget; candidate;
         candidate = candidate->parentWidget()) {
        if (qobject_cast<const QTextEdit *>(candidate)
            || qobject_cast<const QPlainTextEdit *>(candidate)) {
            return candidate;
        }
    }
    return nullptr;
}

enum class InteractionMotion {
    Hover,
    Press,
    Focus
};

int interactionDuration(const QWidget *widget, InteractionMotion motion,
                        bool active)
{
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
        return Private::NormalDuration;
    if (qobject_cast<const QSlider *>(widget)) {
        if (motion == InteractionMotion::Focus)
            return 0;
        return active ? Private::NormalDuration : Private::FastDuration;
    }
    if (qobject_cast<const QScrollBar *>(widget))
        return Private::FastDuration;
    return Private::FasterDuration;
}

qreal progress(const QWidget *widget, const char *name, qreal fallback = 0.0);
const QEasingCurve &fluentCurve();
bool keyboardFocusVisible(const QWidget *widget);
bool animationsAllowed();

qreal progress(const QWidget *widget, const char *name, qreal fallback)
{
    return framePropertyRegistry().real(widget, name, fallback);
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

bool animationsAllowed()
{
    return Style::animationsAllowed();
}

bool keyboardFocusVisible(const QWidget *widget)
{
    return widget && framePropertyRegistry().value(widget, focusVisibleProperty).toBool();
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

Icon arrowIcon(QStyle::PrimitiveElement element)
{
    switch (element) {
    case QStyle::PE_IndicatorArrowDown: return Icon::ChevronDown;
    case QStyle::PE_IndicatorArrowLeft: return Icon::ChevronLeft;
    case QStyle::PE_IndicatorArrowRight: return Icon::ChevronRight;
    case QStyle::PE_IndicatorArrowUp: return Icon::ChevronUp;
    default: return Icon::ChevronRight;
    }
}

bool coveredPrimitive(QStyle::PrimitiveElement element)
{
    switch (element) {
    case QStyle::PE_PanelMenuBar:
    case QStyle::PE_FrameTabBarBase:
    case QStyle::PE_FrameTabWidget:
    case QStyle::PE_PanelButtonCommand:
    case QStyle::PE_PanelButtonTool:
    case QStyle::PE_IndicatorCheckBox:
    case QStyle::PE_IndicatorRadioButton:
    case QStyle::PE_PanelLineEdit:
    case QStyle::PE_FrameLineEdit:
    case QStyle::PE_FrameFocusRect:
    case QStyle::PE_IndicatorArrowDown:
    case QStyle::PE_IndicatorArrowLeft:
    case QStyle::PE_IndicatorArrowRight:
    case QStyle::PE_IndicatorArrowUp:
    case QStyle::PE_PanelMenu:
    case QStyle::PE_PanelItemViewItem:
    case QStyle::PE_IndicatorBranch:
    case QStyle::PE_IndicatorHeaderArrow:
    case QStyle::PE_IndicatorToolBarSeparator:
    case QStyle::PE_FrameDockWidget:
    case QStyle::PE_IndicatorDockWidgetResizeHandle:
        return true;
    default:
        return false;
    }
}

bool coveredControl(QStyle::ControlElement element)
{
    switch (element) {
    case QStyle::CE_PushButton:
    case QStyle::CE_PushButtonLabel:
    case QStyle::CE_CheckBox:
    case QStyle::CE_RadioButton:
    case QStyle::CE_MenuBarItem:
    case QStyle::CE_ItemViewItem:
    case QStyle::CE_ComboBoxLabel:
    case QStyle::CE_ToolButtonLabel:
    case QStyle::CE_ProgressBar:
    case QStyle::CE_ProgressBarGroove:
    case QStyle::CE_ProgressBarContents:
    case QStyle::CE_ProgressBarLabel:
    case QStyle::CE_ToolBar:
    case QStyle::CE_Splitter:
    case QStyle::CE_DockWidgetTitle:
    case QStyle::CE_MenuBarEmptyArea:
    case QStyle::CE_TabBarTabShape:
    case QStyle::CE_TabBarTabLabel:
    case QStyle::CE_TabBarTab:
    case QStyle::CE_HeaderSection:
    case QStyle::CE_Header:
    case QStyle::CE_HeaderLabel:
    case QStyle::CE_MenuItem:
        return true;
    default:
        return false;
    }
}

} // namespace

class StylePrivate
{
public:
    using ToggleDragState = Private::ToggleDragState;

    explicit StylePrivate(Style *owner, ThemeMode initialMode)
        : q(owner), mode(initialMode), animationDriver(owner),
          tableEditorTracker(owner)
    {
        Private::StyleInteractionCallbacks callbacks;
        callbacks.animate = [this](QWidget *widget, const char *property,
                                    qreal target, int duration) {
            animate(widget, property, target, duration);
        };
        callbacks.beginButtonPress = [this](QWidget *widget) {
            beginButtonPress(widget);
        };
        callbacks.releaseButtonPress = [this](QWidget *widget) {
            releaseButtonPress(widget);
        };
        callbacks.cancelButtonPress = [this](QWidget *widget) {
            cancelButtonPress(widget);
        };
        callbacks.stopAnimations = [this](QWidget *widget) {
            stopAnimations(widget);
        };
        callbacks.clearPointerInteraction = [this](QWidget *widget) {
            clearPointerInteraction(widget);
        };
        callbacks.cancelScrollBarTimer = [this](QScrollBar *scrollBar) {
            cancelScrollBarTimer(scrollBar);
        };
        callbacks.scheduleScrollBar = [this](QScrollBar *scrollBar, int delay) {
            scheduleScrollBar(scrollBar, delay);
        };
        callbacks.scheduleSliderToolTip = [this](QSlider *slider) {
            scheduleSliderToolTip(slider);
        };
        callbacks.cancelSliderToolTip = [this](QSlider *slider) {
            cancelSliderToolTip(slider);
        };
        callbacks.refreshProgressTimer = [this] {
            refreshProgressTimer();
        };
        callbacks.progressTimerActive = [this] {
            return progressTimer && progressTimer->isActive();
        };
        callbacks.prepareComboPopupFirstFrame = [this](QComboBox *combo) {
            prepareComboPopupFirstFrame(combo);
        };
        callbacks.releaseComboChevron = [this](QWidget *widget) {
            releaseComboChevron(widget);
        };
        callbacks.finishComboPopupCycle = [this](QWidget *popup) {
            finishComboPopupCycle(popup);
        };
        callbacks.comboForPopupWidget = [](QWidget *widget) {
            return comboForPopupWidget(widget);
        };
        callbacks.updateReadOnlyDeleteAffordance = [](QLineEdit *lineEdit) {
            updateReadOnlyDeleteAffordance(lineEdit);
        };
        callbacks.prepareLineEditHelperButtons = [owner](QLineEdit *lineEdit) {
            prepareLineEditHelperButtons(lineEdit, owner);
        };
        callbacks.prepareContentDialogState = [](QDialog *dialog, bool dark) {
            prepareContentDialogState(dialog, dark);
        };
        callbacks.stopDialogAnimations = [](QDialog *dialog) {
            stopDialogAnimations(dialog);
        };
        callbacks.preparePopupSurface = [](QWidget *widget) {
            preparePopupSurface(widget);
        };
        callbacks.registerPopupPaletteOwners = [this](QWidget *widget) {
            registerPopupPaletteOwners(widget);
        };
        callbacks.registerPaletteOwner = [this](QDialog *dialog) {
            registerPaletteOwner(dialog);
        };
        callbacks.unregisterPaletteOwner = [this](QDialog *dialog) {
            unregisterPaletteOwner(dialog);
        };
        callbacks.restoreContentDialogState = [](QDialog *dialog, bool visible) {
            restoreContentDialogState(dialog, visible);
        };
        callbacks.remember = [](QWidget *widget, const char *property,
                                const QVariant &value) {
            remember(widget, property, value);
        };
        callbacks.prepareNavigationView = [](QAbstractItemView *view) {
            NavigationPrivate::prepareNavigationView(view);
        };
        callbacks.restoreNavigationView = [](QAbstractItemView *view) {
            NavigationPrivate::restoreNavigationView(view);
        };
        callbacks.dark = [this] {
            return dark();
        };
        callbacks.keyboardInput = &keyboardInput;
        callbacks.toggleDragStates = &toggleDragStates;
        interactionController = std::make_unique<Private::StyleInteractionController>(
            owner, std::move(callbacks));
    }

    bool needsSystemAppearancePolling() const
    {
        return mode == ThemeMode::System || !accent.isValid();
    }

    void restartSystemAppearanceWatchdog()
    {
        if (!systemAppearanceWatchdog)
            return;
        if (!applicationStyleActive || !needsSystemAppearancePolling()) {
            systemAppearanceWatchdog->stop();
            return;
        }
        Private::invalidateSystemAppearanceCache();
        if (mode == ThemeMode::System)
            lastSystemDark = Private::systemUsesDarkTheme();
        if (!accent.isValid())
            lastSystemAccent = Private::systemAccentColor();
        // Native notifications provide the fast path on Windows. Keep a
        // deliberately slow watchdog for missed broadcasts and portable
        // platforms where the watcher is a no-op.
        systemAppearanceWatchdog->start();
    }

    void prunePaletteOwners()
    {
        for (auto it = paletteOwners.begin(); it != paletteOwners.end();) {
            QWidget *widget = it->data();
            if (!widget || !widget->property(ownedPaletteProperty).toBool()) {
                if (widget) {
                    if (const auto connection = paletteOwnerConnections.take(widget))
                        QObject::disconnect(connection);
                }
                it = paletteOwners.erase(it);
            } else {
                ++it;
            }
        }
    }

    void registerPaletteOwner(QWidget *widget)
    {
        if (!widget || !widget->property(ownedPaletteProperty).toBool())
            return;
        prunePaletteOwners();
        for (const QPointer<QWidget> &owner : paletteOwners) {
            if (owner.data() == widget)
                return;
        }
        paletteOwners.append(QPointer<QWidget>(widget));
        paletteOwnerConnections.insert(widget,
            QObject::connect(widget, &QObject::destroyed, q,
                             [this, widget] {
            unregisterPaletteOwner(widget);
        }));
    }

    void unregisterPaletteOwner(QWidget *widget)
    {
        if (!widget)
            return;
        if (const auto connection = paletteOwnerConnections.take(widget))
            QObject::disconnect(connection);
        for (auto it = paletteOwners.begin(); it != paletteOwners.end();) {
            if (it->isNull() || it->data() == widget)
                it = paletteOwners.erase(it);
            else
                ++it;
        }
    }

    void clearPaletteOwners()
    {
        for (const auto &connection : paletteOwnerConnections)
            QObject::disconnect(connection);
        paletteOwnerConnections.clear();
        paletteOwners.clear();
    }

    void registerPopupPaletteOwners(QWidget *widget)
    {
        if (!widget)
            return;
        QWidget *popup = widget->window();
        if (!popup || popup->windowType() != Qt::Popup)
            return;

        // preparePopupSurface() has just replaced these palettes with a
        // style-owned, application-palette-based surface. Mark only those
        // surfaces as owned; explicit palettes are still rebased by the
        // effectivePopupPalette() path when they are refreshed.
        popup->setProperty(ownedPaletteProperty, true);
        registerPaletteOwner(popup);
        QAbstractItemView *view = qobject_cast<QAbstractItemView *>(widget);
        if (!view)
            view = popup->findChild<QAbstractItemView *>();
        if (!view)
            return;
        view->setProperty(ownedPaletteProperty, true);
        registerPaletteOwner(view);
        if (QWidget *viewport = view->viewport()) {
            viewport->setProperty(ownedPaletteProperty, true);
            registerPaletteOwner(viewport);
        }
    }

    bool progressBarNeedsAnimation(const QProgressBar *progressBar) const
    {
        return progressBar && progressBar->minimum() == progressBar->maximum()
            && progressBar->isVisible() && Style::animationsAllowed();
    }

    void refreshProgressTimer()
    {
        for (auto it = progressBars.begin(); it != progressBars.end();) {
            if (it->isNull())
                it = progressBars.erase(it);
            else
                ++it;
        }

        bool active = false;
        for (const QPointer<QProgressBar> &guarded : progressBars) {
            if (progressBarNeedsAnimation(guarded)) {
                active = true;
                break;
            }
        }
        if (active)
            progressTimer->start();
        else
            progressTimer->stop();
    }

    void advanceProgressBars()
    {
        const bool allowed = Style::animationsAllowed();
        const qreal phase = allowed
            ? qreal(QDateTime::currentMSecsSinceEpoch() % 1500) / 1500.0
            : 0.35;
        bool active = false;
        for (auto it = progressBars.begin(); it != progressBars.end();) {
            const QPointer<QProgressBar> guarded = *it;
            if (!guarded) {
                it = progressBars.erase(it);
                continue;
            }
            if (progressBarNeedsAnimation(guarded)) {
                active = true;
                framePropertyRegistry().set(guarded, progressPhaseProperty, phase);
                guarded->update();
            }
            ++it;
        }
        if (!active)
            progressTimer->stop();
    }

    void registerProgressBar(QProgressBar *progressBar)
    {
        if (!progressBar)
            return;
        if (progressBarStateConnections.contains(progressBar)) {
            refreshProgressTimer();
            return;
        }
        progressBars.append(QPointer<QProgressBar>(progressBar));
        // QProgressBar has no rangeChanged signal. valueChanged covers the
        // normal range-reset path, while UpdateRequest below closes the case
        // where a range changes without changing the current value.
        progressBarStateConnections.insert(progressBar,
            QObject::connect(progressBar, &QProgressBar::valueChanged, q,
                             [this](int) {
                refreshProgressTimer();
            }));
        QObject::connect(progressBar, &QObject::destroyed, q,
                         [this, progressBar] {
            unregisterProgressBar(progressBar);
        });
        refreshProgressTimer();
    }

    void unregisterProgressBar(QProgressBar *progressBar)
    {
        if (!progressBar)
            return;
        if (const auto connection = progressBarStateConnections.take(progressBar))
            QObject::disconnect(connection);
        for (auto it = progressBars.begin(); it != progressBars.end();) {
            if (it->isNull() || it->data() == progressBar)
                it = progressBars.erase(it);
            else
                ++it;
        }
        refreshProgressTimer();
    }

    QTimer *ensureScrollBarTimer(QScrollBar *scrollBar)
    {
        if (!scrollBar)
            return nullptr;
        if (auto it = scrollBarTimers.find(scrollBar);
            it != scrollBarTimers.end() && it->data()) {
            return it->data();
        }
        auto *timer = new QTimer(scrollBar);
        timer->setObjectName(QStringLiteral("_winui_scrollbar_timer"));
        timer->setSingleShot(true);
        const QPointer<QScrollBar> guarded(scrollBar);
        QObject::connect(timer, &QTimer::timeout, q, [this, guarded] {
            if (!guarded || !guarded->isVisible() || !guarded->isEnabled()
                || !framePropertyRegistry().value(guarded, scrollBarInsideProperty)
                       .isValid()) {
                return;
            }
            if (framePropertyRegistry().value(guarded, scrollBarInsideProperty)
                    .toBool()) {
                animate(guarded, hoverProperty, 1.0, Private::FastDuration);
            } else {
                animate(guarded, hoverProperty, 0.0, Private::FastDuration);
            }
        });
        scrollBarTimers.insert(scrollBar, QPointer<QTimer>(timer));
        QObject::connect(scrollBar, &QObject::destroyed, q,
                         [this, scrollBar] {
            unregisterScrollBar(scrollBar);
        });
        return timer;
    }

    void scheduleScrollBar(QScrollBar *scrollBar, int delay)
    {
        if (auto *timer = ensureScrollBarTimer(scrollBar))
            timer->start(delay);
    }

    void cancelScrollBarTimer(QScrollBar *scrollBar)
    {
        if (!scrollBar)
            return;
        if (auto it = scrollBarTimers.find(scrollBar);
            it != scrollBarTimers.end() && it->data()) {
            it->data()->stop();
        }
    }

    void unregisterScrollBar(QScrollBar *scrollBar)
    {
        if (!scrollBar)
            return;
        if (auto it = scrollBarTimers.find(scrollBar);
            it != scrollBarTimers.end()) {
            QTimer *timer = it->data();
            if (timer)
                timer->stop();
            scrollBarTimers.erase(it);
            delete timer;
        }
    }

    QTimer *ensureSliderToolTipTimer(QSlider *slider)
    {
        if (!slider)
            return nullptr;
        if (auto it = sliderToolTipTimers.find(slider);
            it != sliderToolTipTimers.end() && it->data()) {
            return it->data();
        }
        auto *timer = new QTimer(slider);
        timer->setObjectName(QStringLiteral("_winui_slider_tooltip_timer"));
        timer->setSingleShot(true);
        const QPointer<QSlider> guarded(slider);
        QObject::connect(timer, &QTimer::timeout, q, [guarded] {
            if (guarded && guarded->isEnabled())
                showSliderValueToolTip(guarded);
        });
        sliderToolTipTimers.insert(slider, QPointer<QTimer>(timer));
        QObject::connect(slider, &QObject::destroyed, q,
                         [this, slider] {
            unregisterSlider(slider);
        });
        return timer;
    }

    void scheduleSliderToolTip(QSlider *slider)
    {
        if (!slider || !slider->isEnabled())
            return;
        if (auto *timer = ensureSliderToolTipTimer(slider))
            timer->start(0);
    }

    void cancelSliderToolTip(QSlider *slider)
    {
        if (!slider)
            return;
        if (auto it = sliderToolTipTimers.find(slider);
            it != sliderToolTipTimers.end() && it->data()) {
            it->data()->stop();
        }
    }

    void unregisterSlider(QSlider *slider)
    {
        if (!slider)
            return;
        if (auto it = sliderToolTipTimers.find(slider);
            it != sliderToolTipTimers.end()) {
            QTimer *timer = it->data();
            if (timer)
                timer->stop();
            sliderToolTipTimers.erase(it);
            delete timer;
        }
    }

    void animate(QWidget *widget, const char *property, qreal target, int duration)
    {
        animationDriver.animate(widget, property, target, duration,
                                animationsAllowed(), fluentCurve());
    }

    void stopAnimations(QWidget *widget)
    {
        animationDriver.stop(widget);
    }

    void beginButtonPress(QWidget *widget)
    {
        if (!widget)
            return;
        const qulonglong generation = framePropertyRegistry()
            .value(widget, buttonPressGenerationProperty).toULongLong() + 1;
        framePropertyRegistry().set(widget, buttonPressGenerationProperty,
                                    QVariant::fromValue(generation));
        framePropertyRegistry().set(widget, buttonPressReleasePendingProperty,
                                    false);
        // A synchronous pressed frame is intentional. It makes a very fast
        // click observable and also cancels a release animation already in
        // flight before the next press starts.
        animate(widget, pressProperty, 1.0, 0);
    }

    void releaseButtonPress(QWidget *widget)
    {
        if (!widget)
            return;
        const qulonglong generation = framePropertyRegistry()
            .value(widget, buttonPressGenerationProperty).toULongLong() + 1;
        framePropertyRegistry().set(widget, buttonPressGenerationProperty,
                                    QVariant::fromValue(generation));
        framePropertyRegistry().set(widget, buttonPressReleasePendingProperty,
                                    true);
        const QPointer<QWidget> guardedWidget(widget);
        QTimer::singleShot(16, q, [this, guardedWidget, generation] {
            if (!guardedWidget
                || framePropertyRegistry()
                           .value(guardedWidget, buttonPressGenerationProperty)
                           .toULongLong() != generation
                || !framePropertyRegistry()
                           .value(guardedWidget,
                                  buttonPressReleasePendingProperty)
                           .toBool()) {
                return;
            }
            framePropertyRegistry().set(guardedWidget,
                                        buttonPressReleasePendingProperty,
                                        false);
            animate(guardedWidget, pressProperty, 0.0, Private::FasterDuration);
        });
    }

    void cancelButtonPress(QWidget *widget)
    {
        if (!widget)
            return;
        const qulonglong generation = framePropertyRegistry()
            .value(widget, buttonPressGenerationProperty).toULongLong() + 1;
        framePropertyRegistry().set(widget, buttonPressGenerationProperty,
                                    QVariant::fromValue(generation));
        framePropertyRegistry().set(widget, buttonPressReleasePendingProperty,
                                    false);
        animate(widget, pressProperty, 0.0, Private::FasterDuration);
    }

    void clearPointerInteraction(QWidget *widget)
    {
        if (!widget)
            return;
        if (buttonPressPulse(widget))
            cancelButtonPress(widget);
        stopAnimations(widget);
        framePropertyRegistry().set(widget, hoverProperty, 0.0);
        framePropertyRegistry().set(widget, pressProperty, 0.0);
    }

    void releaseComboChevron(QWidget *widget)
    {
        if (!widget)
            return;
        const qreal start = progress(widget, comboChevronProperty, 0.0);
        if (!animationsAllowed() || qFuzzyIsNull(start)) {
            animationDriver.stop(widget);
            framePropertyRegistry().set(widget, comboChevronProperty, 0.0);
            widget->update();
            return;
        }
        // AnimatedChevronDownSmallVisualSource: PressedToNormal moves from
        // y=31.5 to y=21 then y=24 on a 48 px canvas. At the 12 px ComboBox
        // glyph this is +1.875 px, -0.75 px, then rest over about 300 ms.
        animationDriver.animate(widget, comboChevronProperty, 0.0, 300,
                                true, fluentCurve(),
                                {{0.28, QVariant(-0.4)}}, start);
    }

    void unregisterComboPopup(QWidget *popup)
    {
        if (!popup)
            return;
        const auto association = comboPopupAssociations.find(popup);
        if (association == comboPopupAssociations.end())
            return;
        QComboBox *combo = association->data();
        comboPopupAssociations.erase(association);
        preparedComboPopups.remove(popup);
        if (const auto connection = comboPopupPopupConnections.take(popup))
            QObject::disconnect(connection);
        if (combo && comboPopupByCombo.value(combo) == popup) {
            comboPopupByCombo.remove(combo);
            if (const auto connection = comboPopupComboConnections.take(combo))
                QObject::disconnect(connection);
        }
    }

    void unregisterComboPopup(QComboBox *combo)
    {
        if (!combo)
            return;
        QWidget *popup = comboPopupByCombo.value(combo);
        if (popup)
            unregisterComboPopup(popup);
        else
            comboPopupComboConnections.remove(combo);
    }

    void associateComboPopup(QComboBox *combo, QWidget *popup)
    {
        if (!combo || !popup || popup->windowType() != Qt::Popup)
            return;
        if (QWidget *previousPopup = comboPopupByCombo.value(combo);
            previousPopup && previousPopup != popup) {
            unregisterComboPopup(previousPopup);
        }
        if (const auto association = comboPopupAssociations.find(popup);
            association != comboPopupAssociations.end()) {
            if (association->data() == combo) {
                comboPopupByCombo.insert(combo, popup);
                return;
            }
            unregisterComboPopup(popup);
        }
        comboPopupAssociations.insert(popup, QPointer<QComboBox>(combo));
        comboPopupByCombo.insert(combo, popup);
        comboPopupPopupConnections.insert(popup,
            QObject::connect(popup, &QObject::destroyed, q,
                             [this, popup] { unregisterComboPopup(popup); }));
        comboPopupComboConnections.insert(combo,
            QObject::connect(combo, &QObject::destroyed, q,
                             [this, combo] { unregisterComboPopup(combo); }));
    }

    void prepareComboPopupFirstFrame(QComboBox *combo)
    {
        if (!combo || !combo->view())
            return;
        QWidget *popup = combo->view()->window();
        associateComboPopup(combo, popup);
        if (!popup || preparedComboPopups.contains(popup))
            return;
        preparedComboPopups.insert(popup);
        prepareComboPopupFirstFrameImpl(combo);
    }

    void finishComboPopupCycle(QWidget *popup)
    {
        if (!popup)
            return;
        if (const auto association = comboPopupAssociations.constFind(popup);
            association != comboPopupAssociations.constEnd()) {
            if (QComboBox *combo = association->data())
                releaseComboChevron(combo);
            preparedComboPopups.remove(popup);
        }
    }

    void trackTableEditor(QTableView *table, QWidget *editor)
    {
        tableEditorTracker.track(table, editor);
    }

    void untrackTableEditor(QWidget *editor, bool clearProperty = true)
    {
        tableEditorTracker.untrackEditor(editor, clearProperty);
    }

    void untrackTable(QTableView *table, bool clearProperties = true)
    {
        tableEditorTracker.untrackTable(table, clearProperties);
    }

    bool tableEditorOverlaps(const QTableView *table,
                             const QModelIndex &index,
                             const QRect &itemRect)
    {
        return tableEditorTracker.overlaps(table, index, itemRect);
    }

    bool dark() const
    {
        return mode == ThemeMode::Dark
            || (mode == ThemeMode::System && Private::systemUsesDarkTheme());
    }

    Style *q = nullptr;
    ThemeMode mode = ThemeMode::System;
    QColor accent;
    FrameAnimationDriver animationDriver;
    QVector<QPointer<QProgressBar>> progressBars;
    QHash<QProgressBar *, QMetaObject::Connection> progressBarStateConnections;
    QHash<QScrollBar *, QPointer<QTimer>> scrollBarTimers;
    QHash<QSlider *, QPointer<QTimer>> sliderToolTipTimers;
    QHash<QWidget *, QMetaObject::Connection> toggleConnections;
    QHash<QRadioButton *, QMetaObject::Connection> radioConnections;
    QHash<QWidget *, QMetaObject::Connection> tableConnections;
    TableEditorTracker tableEditorTracker;
    QHash<QCheckBox *, ToggleDragState> toggleDragStates;
    QHash<QWidget *, QPointer<QComboBox>> comboPopupAssociations;
    QHash<QComboBox *, QWidget *> comboPopupByCombo;
    QHash<QWidget *, QMetaObject::Connection> comboPopupPopupConnections;
    QHash<QComboBox *, QMetaObject::Connection> comboPopupComboConnections;
    QSet<QWidget *> preparedComboPopups;
    QVector<QPointer<QWidget>> paletteOwners;
    QHash<QWidget *, QMetaObject::Connection> paletteOwnerConnections;
    bool keyboardInput = false;
    bool applicationStateSaved = false;
    bool applicationStyleActive = false;
    bool lastSystemDark = false;
    QColor lastSystemAccent;
    QTimer *progressTimer = nullptr;
    SystemAppearanceWatcher *systemAppearanceWatcher = nullptr;
    QTimer *systemAppearanceWatchdog = nullptr;
    QFont originalApplicationFont;
    QPalette originalApplicationPalette;
    std::unique_ptr<Private::StyleInteractionController> interactionController;
};

Style::Style(ThemeMode mode)
    : QProxyStyle(new QCommonStyle), d(std::make_unique<StylePrivate>(this, mode))
{
    setObjectName(QStringLiteral("winui3"));
    d->progressTimer = new QTimer(this);
    d->progressTimer->setObjectName(QStringLiteral("_winui_progress_timer"));
    d->progressTimer->setInterval(16);
    connect(d->progressTimer, &QTimer::timeout, this,
            [this] { d->advanceProgressBars(); });
    d->systemAppearanceWatchdog = new QTimer(this);
    d->systemAppearanceWatchdog->setObjectName(
        QStringLiteral("_winui_system_appearance_watchdog"));
    d->systemAppearanceWatchdog->setInterval(15000);
    connect(d->systemAppearanceWatchdog, &QTimer::timeout,
            this, &Style::checkSystemAppearance);
    d->systemAppearanceWatcher = new SystemAppearanceWatcher(
        this, [this] { checkSystemAppearance(); });
    d->systemAppearanceWatcher->setActive(false);
    if (QStyleHints *hints = QGuiApplication::styleHints()) {
        connect(hints, &QStyleHints::colorSchemeChanged, this,
                [this](Qt::ColorScheme) { checkSystemAppearance(); });
    }
}

Style::~Style() = default;

ThemeMode Style::themeMode() const
{
    return d->mode;
}

void Style::setThemeMode(ThemeMode mode)
{
    if (d->mode == mode)
        return;
    d->mode = mode;
    refreshApplicationAppearance();
    d->restartSystemAppearanceWatchdog();
    emit themeChanged(mode);
}

QColor Style::accentColor() const
{
    return d->accent.isValid() ? d->accent : Private::systemAccentColor();
}

bool Style::animationsAllowed()
{
    return !qEnvironmentVariableIsSet("WINUI3STYLE_DISABLE_ANIMATIONS");
}

void Style::setAccentColor(const QColor &color)
{
    if (d->accent == color)
        return;
    d->accent = color;
    refreshApplicationAppearance();
    d->restartSystemAppearanceWatchdog();
    emit accentColorChanged(accentColor());
}

void Style::refreshApplicationAppearance()
{
    if (!qApp)
        return;
    const QPalette applicationPalette = standardPalette();
    const Private::Tokens applicationTokens = Private::tokens(applicationPalette);
    const QColor applicationAccent = accentColor();
    const bool darkTheme = d->dark();
    qApp->setPalette(applicationPalette);
    // A popup's view and viewport are often created after their combo box was
    // polished. Prepare and register them now that the popup exists, before
    // walking the bounded owner registry below.
    for (QWidget *window : qApp->topLevelWidgets()) {
        const QVariant backdrop = window->property("_winui_backdrop");
        if (backdrop.isValid()) {
            QTimer::singleShot(0, window, [window, backdrop] {
                applyBackdrop(window, static_cast<Backdrop>(backdrop.toInt()));
            });
        }
        if (window->windowType() == Qt::Popup) {
            preparePopupSurface(window);
            d->registerPopupPaletteOwners(window);
        }
    }
    d->prunePaletteOwners();
    for (const QPointer<QWidget> &guarded : d->paletteOwners) {
        QWidget *widget = guarded.data();
        if (!widget)
            continue;
        if (widget->window() && widget->window()->windowType() == Qt::Popup) {
            preparePopupSurface(widget);
        } else {
            QPalette palette = applicationPalette;
            if (qobject_cast<QTableView *>(widget)) {
                palette.setColor(QPalette::Highlight,
                                 applicationTokens.subtleHover);
                palette.setColor(QPalette::HighlightedText,
                                 applicationTokens.textPrimary);
            } else if (auto *editor = qobject_cast<QLineEdit *>(widget);
                       editor && itemView(editor)) {
                palette.setColor(QPalette::Highlight, applicationAccent);
                palette.setColor(QPalette::HighlightedText,
                                 applicationTokens.textOnAccentPrimary);
            } else if (qobject_cast<QDialog *>(widget)) {
                palette.setColor(QPalette::Window,
                    darkTheme ? QColor(32, 32, 32) : QColor(255, 255, 255));
            }
            widget->setPalette(palette);
        }
        widget->update();
        if (auto *view = qobject_cast<QAbstractItemView *>(widget)) {
            view->update();
            if (view->viewport())
                view->viewport()->update();
        }
    }
    // The popup may be newly created and not yet have delivered its first
    // Show event. Refresh visible popup children as well as the window itself.
    for (QWidget *window : qApp->topLevelWidgets()) {
        if (window->windowType() == Qt::Popup && window->isVisible()) {
            window->update();
            if (auto *view = window->findChild<QAbstractItemView *>()) {
                view->update();
                if (view->viewport())
                    view->viewport()->update();
            }
        }
    }
}

void Style::checkSystemAppearance()
{
    Private::invalidateSystemAppearanceCache();
    if (!d->needsSystemAppearancePolling()) {
        d->systemAppearanceWatchdog->stop();
        return;
    }
    bool themeChangedAtRuntime = false;
    bool accentChangedAtRuntime = false;
    QColor systemAccent;
    if (d->mode == ThemeMode::System) {
        const bool systemDark = Private::systemUsesDarkTheme();
        themeChangedAtRuntime = d->lastSystemDark != systemDark;
        d->lastSystemDark = systemDark;
    }
    if (!d->accent.isValid()) {
        systemAccent = Private::systemAccentColor();
        accentChangedAtRuntime = d->lastSystemAccent != systemAccent;
        d->lastSystemAccent = systemAccent;
    }
    if (!themeChangedAtRuntime && !accentChangedAtRuntime)
        return;
    refreshApplicationAppearance();
    if (themeChangedAtRuntime)
        emit themeChanged(ThemeMode::System);
    if (accentChangedAtRuntime)
        emit accentColorChanged(systemAccent);
}

void Style::setControlRole(QWidget *widget, ControlRole role)
{
    if (!widget)
        return;
    if (!widget->property(originalRoleWasValidProperty).isValid()) {
        widget->setProperty(originalRoleWasValidProperty,
                            widget->property(roleProperty).isValid());
        widget->setProperty(originalRoleProperty, widget->property(roleProperty));
    }
    widget->setProperty(roleProperty, static_cast<int>(role));
    widget->update();
}

ControlRole Style::controlRole(const QWidget *widget)
{
    if (!widget)
        return ControlRole::Standard;
    if (!widget->property(roleProperty).isValid()) {
        if (const auto *button = qobject_cast<const QPushButton *>(widget);
            button && button->isDefault()) {
            return ControlRole::Accent;
        }
    }
    return static_cast<ControlRole>(widget->property(roleProperty).toInt());
}

void Style::setToggleSwitch(QCheckBox *checkBox, bool enabled)
{
    if (!checkBox)
        return;
    checkBox->setProperty(ToggleSwitchProperty, enabled);
    checkBox->setTristate(false);
    checkBox->updateGeometry();
    checkBox->update();
}

bool Style::isToggleSwitch(const QCheckBox *checkBox)
{
    return checkBox && checkBox->property(ToggleSwitchProperty).toBool();
}

void Style::setToggleSwitchText(QCheckBox *checkBox, const QString &onText,
                                const QString &offText)
{
    if (!checkBox)
        return;
    checkBox->setProperty(ToggleSwitchOnTextProperty, onText);
    checkBox->setProperty(ToggleSwitchOffTextProperty, offText);
    checkBox->updateGeometry();
    checkBox->update();
}

void Style::setSettingsCard(QFrame *frame, bool enabled)
{
    if (!frame)
        return;
    if (enabled) {
        remember(frame, originalFrameShapeProperty, int(frame->frameShape()));
        frame->setProperty(SettingsCardProperty, true);
        frame->setFrameShape(QFrame::StyledPanel);
    } else {
        frame->setProperty(SettingsCardProperty, false);
        if (frame->property(originalFrameShapeProperty).isValid()) {
            frame->setFrameShape(static_cast<QFrame::Shape>(
                frame->property(originalFrameShapeProperty).toInt()));
            frame->setProperty(originalFrameShapeProperty, {});
        }
    }
    frame->updateGeometry();
    frame->update();
}

void Style::setNavigationView(QAbstractItemView *view, bool enabled)
{
    if (!view)
        return;
    view->setProperty(NavigationViewProperty, enabled);
    if (view->viewport())
        view->viewport()->setProperty(NavigationViewProperty, enabled);
    view->updateGeometry();
    view->viewport()->update();
}

void Style::setVerticalSpinButtons(QAbstractSpinBox *spinBox, bool enabled)
{
    if (!spinBox)
        return;
    spinBox->setProperty(VerticalSpinButtonsProperty, enabled);
    spinBox->updateGeometry();
    spinBox->update();
}

bool Style::hasVerticalSpinButtons(const QAbstractSpinBox *spinBox)
{
    return spinBox && spinBox->property(VerticalSpinButtonsProperty).toBool();
}

void Style::setContentDialog(QDialog *dialog, bool enabled)
{
    if (!dialog)
        return;
    dialog->setProperty(ContentDialogProperty, enabled);
    if (!enabled && !qobject_cast<QMessageBox *>(dialog))
        restoreContentDialogState(dialog, true);
    dialog->updateGeometry();
    dialog->update();
}

QPalette Style::standardPalette() const
{
    const bool darkTheme = d->dark();
    const QColor accent = accentColor();
    return Private::standardPalette(darkTheme, accent, d->accent.isValid());
}

void Style::drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                          QPainter *painter, const QWidget *widget) const
{
    if (Private::drawMenuPrimitive(this, element, option, painter, widget))
        return;
    if (Private::drawButtonPrimitive(this, element, option, painter, widget))
        return;
    using namespace Private;
    const Tokens t = tokens(option->palette);
    const bool enabled = option->state & State_Enabled;
    const bool hovered = enabled && (option->state & State_MouseOver);
    const bool pressed = enabled && (option->state & State_Sunken);
    const qreal hover = enabled
        ? progress(widget, hoverProperty, hovered ? 1.0 : 0.0) : 0.0;
    // State_Sunken is the authoritative instantaneous state. The property is
    // an animation/pulse cache and can briefly still contain zero when Qt has
    // entered the pressed state (notably on rapid press/reversal sequences).
    const qreal press = enabled
        ? qMax(progress(widget, pressProperty, pressed ? 1.0 : 0.0),
              pressed ? 1.0 : 0.0)
        : 0.0;


    if (Private::drawViewPrimitive(this, element, option, painter, widget))
        return;

    if (element == PE_FrameFocusRect) {
        if (!keyboardFocusVisible(widget))
            return;
        const qreal focus = progress(widget, focusProperty,
                                     option->state & State_HasFocus ? 1.0 : 0.0);
        if (focus <= 0.01)
            return;
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        QColor outer = t.focusOuter;
        QColor inner = t.focusInner;
        outer.setAlphaF(outer.alphaF() * focus);
        inner.setAlphaF(inner.alphaF() * focus);
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(outer, 2));
        painter->drawRoundedRect(QRectF(option->rect).adjusted(1, 1, -1, -1),
                                 ControlRadius + 2, ControlRadius + 2);
        painter->setPen(QPen(inner, 1));
        painter->drawRoundedRect(QRectF(option->rect).adjusted(3, 3, -3, -3),
                                 ControlRadius, ControlRadius);
        painter->restore();
        return;
    }

    if (element == PE_IndicatorArrowDown || element == PE_IndicatorArrowLeft
        || element == PE_IndicatorArrowRight || element == PE_IndicatorArrowUp) {
        WinUI3::icon(arrowIcon(element), enabled ? t.textPrimary : t.textDisabled)
            .paint(painter, option->rect, Qt::AlignCenter,
                   enabled ? QIcon::Normal : QIcon::Disabled);
        return;
    }


    if (element == PE_Frame && widget && widget->window()
        && widget->window()->windowType() == Qt::Popup) {
        const QColor popupSurface = t.dark ? QColor(44, 44, 44)
                                           : QColor(252, 252, 252);
        controlSurface(painter, option->rect, popupSurface,
                       t.dark ? QColor(0, 0, 0, 51) : QColor(0, 0, 0, 15),
                       t.dark ? QColor(0, 0, 0, 51) : QColor(0, 0, 0, 15),
                       OverlayRadius);
        return;
    }

    if (element == PE_Frame && qobject_cast<const QAbstractItemView *>(widget)) {
        roundedRect(painter, QRectF(option->rect).adjusted(0.5, 0.5, -0.5, -0.5),
                    Qt::transparent, t.stroke, ControlRadius);
        return;
    }

    if (element == PE_Frame && richTextEditor(widget)) {
        const QWidget *editor = richTextEditor(widget);
        const bool focused = editor->hasFocus();
        const bool editorEnabled = option->state & State_Enabled;
        const QColor fill = !editorEnabled ? t.controlDisabled
            : focused ? (t.dark ? QColor(30, 30, 30, 179)
                              : QColor(255, 255, 255))
                      : (option->state & State_MouseOver ? t.controlHover
                                                        : t.control);
        controlSurface(painter, option->rect, fill, t.stroke, t.strokeSecondary,
                       ControlRadius);
        if (focused) {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            QPainterPath clip;
            clip.addRoundedRect(QRectF(option->rect).adjusted(0.5, 0.5, -0.5, -0.5),
                                ControlRadius, ControlRadius);
            painter->setClipPath(clip);
            painter->setPen(QPen(t.accentFill, 2.0, Qt::SolidLine, Qt::FlatCap));
            painter->drawLine(option->rect.left(), option->rect.bottom() - 1,
                              option->rect.right(), option->rect.bottom() - 1);
            painter->restore();
        }
        return;
    }

    if (element == PE_IndicatorToolBarSeparator) {
        painter->save();
        painter->setPen(QPen(t.stroke, 1));
        if (option->state & State_Horizontal) {
            const int x = option->rect.center().x();
            painter->drawLine(x, option->rect.top() + 8,
                              x, option->rect.bottom() - 8);
        } else {
            const int y = option->rect.center().y();
            painter->drawLine(option->rect.left() + 8, y,
                              option->rect.right() - 8, y);
        }
        painter->restore();
        return;
    }

    Q_ASSERT_X(!coveredPrimitive(element), "WinUI3::Style::drawPrimitive",
               "a covered primitive reached QCommonStyle");
    QProxyStyle::drawPrimitive(element, option, painter, widget);
}

void Style::drawControl(ControlElement element, const QStyleOption *option,
                        QPainter *painter, const QWidget *widget) const
{
    if (Private::drawMenuControl(this, element, option, painter, widget))
        return;
    if (Private::drawButtonControl(this, element, option, painter, widget))
        return;
    using namespace Private;
    const Tokens t = tokens(option->palette);

    if (element == CE_ProgressBar) {
        if (const auto *bar = qstyleoption_cast<const QStyleOptionProgressBar *>(option)) {
            drawControl(CE_ProgressBarGroove, bar, painter, widget);
            drawControl(CE_ProgressBarContents, bar, painter, widget);
            if (bar->textVisible)
                drawControl(CE_ProgressBarLabel, bar, painter, widget);
            return;
        }
    }

    if (Private::drawViewControl(this, element, option, painter, widget,
                                 [this](const QTableView *table,
                                        const QModelIndex &index,
                                        const QRect &rect) {
        return d->tableEditorOverlaps(table, index, rect);
    })) {
        return;
    }

    if (element == CE_ShapedFrame
        && widget && widget->property(SettingsCardProperty).toBool()) {
        const qreal hover = progress(widget, hoverProperty,
                                     option->state & State_MouseOver ? 1.0 : 0.0);
        const qreal press = progress(widget, pressProperty,
                                     option->state & State_Sunken ? 1.0 : 0.0);
        QColor fill = mix(t.control, t.controlHover, hover * (1.0 - press));
        fill = mix(fill, t.controlPressed, press);
        controlSurface(painter, option->rect, fill, t.stroke, t.strokeSecondary,
                       OverlayRadius);
        if (keyboardFocusVisible(widget)) {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setBrush(Qt::NoBrush);
            painter->setPen(QPen(t.focusOuter, 2.0));
            painter->drawRoundedRect(QRectF(option->rect).adjusted(2, 2, -3, -3),
                                     6, 6);
            painter->setPen(QPen(t.focusInner, 1.0));
            painter->drawRoundedRect(QRectF(option->rect).adjusted(4, 4, -5, -5),
                                     5, 5);
            painter->restore();
        }
        return;
    }

    if (element == CE_ComboBoxLabel) {
        if (const auto *combo = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            if (combo->editable)
                return;
            QRect content = subControlRect(CC_ComboBox, combo,
                                            SC_ComboBoxEditField, widget);
            if (!combo->currentIcon.isNull()) {
                const QSize iconSize = combo->iconSize.isValid() ? combo->iconSize : QSize(16, 16);
                const QRect logicalIcon(content.left(),
                                        content.center().y() - iconSize.height() / 2,
                                        iconSize.width(), iconSize.height());
                const QRect iconRect = visualRect(option->direction, content,
                                                  logicalIcon);
                paintThemedIcon(painter, combo->currentIcon, iconRect,
                    Qt::AlignCenter,
                    option->state & State_Enabled ? t.textPrimary : t.textDisabled,
                    option->state & State_Enabled ? QIcon::Normal : QIcon::Disabled);
                if (option->direction == Qt::RightToLeft)
                    content.setRight(iconRect.left() - 8);
                else
                    content.setLeft(iconRect.right() + 8);
            }
            painter->setPen(option->state & State_Enabled ? t.textPrimary : t.textDisabled);
            painter->drawText(content,
                              visualAlignment(option->direction,
                                              Qt::AlignLeft | Qt::AlignVCenter),
                              option->fontMetrics.elidedText(combo->currentText,
                                                              Qt::ElideRight, content.width()));
            return;
        }
    }


    if (element == CE_ProgressBarGroove) {
        const auto *bar = qstyleoption_cast<const QStyleOptionProgressBar *>(option);
        const auto *progressBar = qobject_cast<const QProgressBar *>(widget);
        const bool horizontal = !progressBar
            || progressBar->orientation() == Qt::Horizontal;
        const QRect groove = horizontal
            ? QRect(option->rect.left(), option->rect.center().y() - 2,
                    option->rect.width(), 4)
            : QRect(option->rect.center().x() - 2, option->rect.top(),
                    4, option->rect.height());
        roundedRect(painter, groove, t.stroke, Qt::transparent, 2);
        return;
    }

    if (element == CE_ProgressBarContents) {
        if (const auto *bar = qstyleoption_cast<const QStyleOptionProgressBar *>(option)) {
            const auto *progressBar = qobject_cast<const QProgressBar *>(widget);
            const bool horizontal = !progressBar
                || progressBar->orientation() == Qt::Horizontal;
            const bool inverted = progressBar ? progressBar->invertedAppearance()
                                               : bar->invertedAppearance;
            const QRect track = horizontal
                ? QRect(option->rect.left(), option->rect.center().y() - 2,
                        option->rect.width(), 4)
                : QRect(option->rect.center().x() - 2, option->rect.top(),
                        4, option->rect.height());
            const QColor indicatorColor = bar->state & State_Enabled
                ? t.accentFill : t.accentFillDisabled;
            if (bar->minimum == 0 && bar->maximum == 0) {
                // ProgressRing-style indeterminate progress uses two
                // independently moving indicators.  The phase is advanced
                // by a widget-owned timer, so every repaint is intentional
                // and capture mode can freeze it at a stable value.
                const qreal phase = animationsAllowed()
                    ? qBound<qreal>(0.0, progress(widget, progressPhaseProperty, 0.0), 1.0)
                    : 0.35;
                const int axis = horizontal ? track.width() : track.height();
                const int firstLength = qMax(12, axis / 4);
                const int secondLength = qMax(10, axis / 6);
                const bool reverse = horizontal
                    ? (inverted
                       != (bar->direction == Qt::RightToLeft))
                    : !bar->invertedAppearance;
                const auto drawIndicator = [&](int length, qreal offset) {
                    const qreal travel = axis + length;
                    const int distance = qRound(travel * std::fmod(phase + offset, 1.0));
                    QRect indicator = track;
                    if (horizontal) {
                        const int left = reverse ? axis - distance : distance - length;
                        indicator.setLeft(track.left() + left);
                        indicator.setWidth(length);
                    } else {
                        const int top = reverse ? axis - distance : distance - length;
                        indicator.setTop(track.top() + top);
                        indicator.setHeight(length);
                    }
                    indicator = indicator.intersected(track);
                    if (!indicator.isEmpty())
                        roundedRect(painter, indicator, indicatorColor,
                                    Qt::transparent, 2);
                };
                drawIndicator(firstLength, 0.0);
                drawIndicator(secondLength, 0.5);
                return;
            }
            const qint64 range = qint64(bar->maximum) - qint64(bar->minimum);
            const qint64 value = qint64(bar->progress) - qint64(bar->minimum);
            const qreal ratio = range > 0
                ? qBound<qreal>(0, qreal(value) / qreal(range), 1)
                : 0;
            QRect fill = track;
            if (horizontal) {
                const int length = qRound(track.width() * ratio);
                if (inverted
                    != (bar->direction == Qt::RightToLeft))
                    fill.setLeft(track.right() - length + 1);
                else
                    fill.setWidth(length);
            } else {
                const int length = qRound(track.height() * ratio);
                if (inverted)
                    fill.setTop(track.bottom() - length + 1);
                else
                    fill.setHeight(length);
            }
            if (!fill.isEmpty())
                roundedRect(painter, fill, indicatorColor, Qt::transparent, 2);
            return;
        }
    }

    if (element == CE_ProgressBarLabel)
        return;

    if (element == CE_ToolBar) {
        return;
    }



    Q_ASSERT_X(!coveredControl(element), "WinUI3::Style::drawControl",
               "a covered control reached QCommonStyle");
    QProxyStyle::drawControl(element, option, painter, widget);
}

void Style::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                               QPainter *painter, const QWidget *widget) const
{
    if (Private::drawComplexControl(this, control, option, painter, widget))
        return;

    Q_ASSERT_X(!Private::coveredComplex(control), "WinUI3::Style::drawComplexControl",
               "a covered complex control reached QCommonStyle");
    QProxyStyle::drawComplexControl(control, option, painter, widget);
}


int Style::pixelMetric(PixelMetric metric, const QStyleOption *option,
                       const QWidget *widget) const
{
    return Private::pixelMetric(this, metric, option, widget);
}
QSize Style::sizeFromContents(ContentsType type, const QStyleOption *option,
                              const QSize &contentsSize, const QWidget *widget) const
{
    return Private::sizeFromContents(this, type, option, contentsSize, widget);
}

QRect Style::subElementRect(SubElement element, const QStyleOption *option,
                            const QWidget *widget) const
{
    return Private::subElementRect(this, element, option, widget);
}

QRect Style::subControlRect(ComplexControl control,
                            const QStyleOptionComplex *option,
                            SubControl subControl, const QWidget *widget) const
{
    return Private::subControlRect(this, control, option, subControl, widget);
}
QStyle::SubControl Style::hitTestComplexControl(
    ComplexControl control, const QStyleOptionComplex *option,
    const QPoint &position, const QWidget *widget) const
{
    if (const auto result = Private::complexControlHitTest(
            control, option, position, widget))
        return *result;
    return QProxyStyle::hitTestComplexControl(
        control, option, position, widget);
}
int Style::styleHint(StyleHint hint, const QStyleOption *option,
                     const QWidget *widget, QStyleHintReturn *returnData) const
{
    return Private::styleHint(this, hint, option, widget, returnData);
}

QIcon Style::standardIcon(StandardPixmap standard, const QStyleOption *option,
                           const QWidget *widget) const
{
    return Private::standardIcon(this, standard, option, widget);
}

void Style::polish(QApplication *application)
{
    QProxyStyle::polish(application);
    d->applicationStyleActive = true;
    if (!d->applicationStateSaved) {
        d->originalApplicationFont = application->font();
        d->originalApplicationPalette = application->palette();
        d->applicationStateSaved = true;
    }
    QFont font(QStringLiteral("Segoe UI Variable Text"));
    if (!QFontDatabase::families().contains(font.family()))
        font.setFamily(QStringLiteral("Segoe UI"));
    font.setPixelSize(14);
    application->setFont(font);
    application->setPalette(standardPalette());
    d->systemAppearanceWatcher->setActive(true);
    d->restartSystemAppearanceWatchdog();
}

void Style::polish(QWidget *widget)
{
    QProxyStyle::polish(widget);
    if (!widget)
        return;
    rememberPalette(widget);
    remember(widget, originalAutoFillProperty, widget->autoFillBackground());
    remember(widget, originalHoverAttributeProperty,
             widget->testAttribute(Qt::WA_Hover));
    remember(widget, originalRoleProperty, widget->property(roleProperty));
    widget->setAttribute(Qt::WA_Hover, true);
    widget->installEventFilter(this);
    framePropertyRegistry().set(widget, hoverProperty,
                                widget->isEnabled() && widget->underMouse()
                                    ? 1.0 : 0.0);
    framePropertyRegistry().set(widget, pressProperty, 0.0);
    framePropertyRegistry().set(widget, focusProperty,
                                widget->hasFocus() ? 1.0 : 0.0);
    framePropertyRegistry().set(widget, focusVisibleProperty,
                                widget->hasFocus() && d->keyboardInput);
    if (auto *lineEdit = qobject_cast<QLineEdit *>(widget))
        prepareLineEditHelperButtons(lineEdit, this);
    if (qobject_cast<QScrollBar *>(widget)) {
        framePropertyRegistry().set(widget, scrollBarInsideProperty,
                                    widget->underMouse());
        framePropertyRegistry().set(widget, scrollBarGenerationProperty, 0);
    }
    if (auto *checkBox = qobject_cast<QCheckBox *>(widget)) {
        if (const auto previous = d->toggleConnections.take(widget))
            disconnect(previous);
        framePropertyRegistry().set(widget, checkProperty,
                                    checkBox->checkState() == Qt::Unchecked
                                        ? 0.0 : 1.0);
        framePropertyRegistry().set(widget, togglePositionProperty,
                                    checkBox->isChecked() ? 1.0 : 0.0);
        d->toggleConnections.insert(widget,
            connect(checkBox, &QCheckBox::stateChanged, this,
                    [this, checkBox](int state) {
                const bool on = state != Qt::Unchecked;
                // AnimatedAcceptVisualSource's NormalOnToNormalOff segment
                // removes the stroke immediately. Only the acceptance path
                // is animated; an on-transition remains interruptible by
                // starting from its current progress.
                d->animate(checkBox, checkProperty, on ? 1.0 : 0.0,
                           on ? (toggleSwitch(checkBox)
                                     ? Private::FastDuration
                                     : Private::CheckBoxDuration)
                              : 0);
                if (toggleSwitch(checkBox))
                    d->animate(checkBox, togglePositionProperty,
                               on ? 1.0 : 0.0,
                               Private::FasterDuration);
            }));
    } else if (auto *radio = qobject_cast<QRadioButton *>(widget)) {
        if (const auto previous = d->radioConnections.take(radio))
            disconnect(previous);
        framePropertyRegistry().set(widget, checkProperty,
                                    radio->isChecked() ? 1.0 : 0.0);
        d->radioConnections.insert(radio,
            connect(radio, &QAbstractButton::toggled, this,
                    [this, radio](bool checked) {
                d->animate(radio, checkProperty, checked ? 1.0 : 0.0,
                           Private::FastDuration);
            }));
    } else if (auto *groupBox = qobject_cast<QGroupBox *>(widget);
               groupBox && groupBox->isCheckable()) {
        if (const auto previous = d->toggleConnections.take(widget))
            disconnect(previous);
        framePropertyRegistry().set(widget, checkProperty,
                                    groupBox->isChecked() ? 1.0 : 0.0);
        d->toggleConnections.insert(widget,
            connect(groupBox, &QGroupBox::toggled, this,
                    [this, groupBox](bool checked) {
                d->animate(groupBox, checkProperty, checked ? 1.0 : 0.0,
                           checked ? Private::CheckBoxDuration : 0);
                    }));
    }

    if (auto *progressBar = qobject_cast<QProgressBar *>(widget)) {
        d->registerProgressBar(progressBar);
        framePropertyRegistry().set(progressBar, progressPhaseProperty,
                                    Style::animationsAllowed() ? 0.0 : 0.35);
        d->refreshProgressTimer();
    }
    if (auto *view = qobject_cast<QAbstractItemView *>(widget))
        NavigationPrivate::prepareNavigationView(view);

    if (auto *table = qobject_cast<QTableView *>(widget)) {
        if (const auto previous = d->tableConnections.take(widget))
            disconnect(previous);
        widget->setProperty(ownedPaletteProperty, true);
        d->registerPaletteOwner(widget);
        const auto applyTableSelectionPalette = [this, table] {
            QPalette palette = table->palette();
            const Private::Tokens tableTokens = Private::tokens(standardPalette());
            palette.setColor(QPalette::Highlight, tableTokens.subtleHover);
            palette.setColor(QPalette::HighlightedText, tableTokens.textPrimary);
            table->setPalette(palette);
        };
        applyTableSelectionPalette();
        d->tableConnections.insert(table,
            connect(this, &Style::themeChanged, table,
                    [applyTableSelectionPalette](ThemeMode) {
                applyTableSelectionPalette();
            }));
    } else if (auto *editor = qobject_cast<QLineEdit *>(widget);
               editor && qobject_cast<const QTableView *>(itemView(editor))) {
        widget->setProperty(ownedPaletteProperty, true);
        d->registerPaletteOwner(widget);
        QPalette palette = editor->palette();
        const Private::Tokens editorTokens = Private::tokens(standardPalette());
        palette.setColor(QPalette::Highlight, accentColor());
        palette.setColor(QPalette::HighlightedText, editorTokens.textOnAccentPrimary);
        editor->setPalette(palette);
    }

    if (auto *view = qobject_cast<QTableView *>(
            const_cast<QAbstractItemView *>(itemView(widget))); view
        && widget->parentWidget() == view->viewport()) {
        // Editors are children of the viewport and are polished after the
        // delegate creates them. Track that lifecycle in the style so item
        // painting can suppress display text even when Qt omits
        // State_Editing from the real delegate option.
        d->trackTableEditor(view, widget);
    }

    if (auto *toolButton = qobject_cast<QAbstractButton *>(widget)) {
        if (qobject_cast<QToolBar *>(toolButton->parentWidget())
            || qobject_cast<QTabBar *>(toolButton->parentWidget()))
            setControlRole(toolButton, ControlRole::Subtle);
    }

    if (qobject_cast<QComboBox *>(widget))
        framePropertyRegistry().set(widget, comboChevronProperty, 0.0);

    // QMenu computes its first popup geometry after polish but before Show.
    // Install the layout inset here; the opaque palette is refreshed on Show.
    if (auto *menu = qobject_cast<QMenu *>(widget)) {
        remember(menu, originalMarginsProperty,
                 QVariant::fromValue(menu->contentsMargins()));
        menu->setContentsMargins(0, 2, 0, 2);
    }

    if (auto *dialog = qobject_cast<QDialog *>(widget);
        dialog && (qobject_cast<QMessageBox *>(dialog)
                   || dialog->property(ContentDialogProperty).toBool())) {
        prepareContentDialogState(dialog, d->dark());
        d->registerPaletteOwner(dialog);
    }

}

void Style::polish(QPalette &palette)
{
    palette = standardPalette();
}

void Style::unpolish(QApplication *application)
{
    d->applicationStyleActive = false;
    d->systemAppearanceWatchdog->stop();
    d->systemAppearanceWatcher->setActive(false);
    d->clearPaletteOwners();
    if (application && d->applicationStateSaved) {
        application->setFont(d->originalApplicationFont);
        application->setPalette(d->originalApplicationPalette);
        d->applicationStateSaved = false;
    }
    QProxyStyle::unpolish(application);
}

void Style::unpolish(QWidget *widget)
{
    if (widget) {
        d->unregisterPaletteOwner(widget);
        widget->removeEventFilter(this);
        if (auto *lineEdit = qobject_cast<QLineEdit *>(widget))
            cancelLineEditHelperUpdate(lineEdit);
    }
    if (auto *combo = qobject_cast<QComboBox *>(widget))
        d->unregisterComboPopup(combo);
    if (widget && widget->isWindow() && widget->windowType() == Qt::Popup)
        d->unregisterComboPopup(widget);
    // Let the base style release its state before restoring application-owned
    // values. Some Qt widgets (notably item views) recompute frame margins in
    // QCommonStyle::unpolish(); restoring first would immediately lose the
    // original values again.
    QProxyStyle::unpolish(widget);
    if (widget) {
        d->stopAnimations(widget);
        if (widget->property("_winui_backdrop").isValid())
            applyBackdrop(widget, Backdrop::None);
        restoreRememberedPalette(widget);
        if (widget->property(originalAutoFillProperty).isValid())
            widget->setAutoFillBackground(
                widget->property(originalAutoFillProperty).toBool());
        if (widget->property(originalHoverAttributeProperty).isValid())
            widget->setAttribute(Qt::WA_Hover,
                widget->property(originalHoverAttributeProperty).toBool());
        if (widget->property(originalOpaquePaintProperty).isValid())
            widget->setAttribute(Qt::WA_OpaquePaintEvent,
                widget->property(originalOpaquePaintProperty).toBool());
        if (widget->property(originalTranslucentBackgroundProperty).isValid())
            widget->setAttribute(
                Qt::WA_TranslucentBackground,
                widget->property(originalTranslucentBackgroundProperty).toBool());
        if (widget->property(originalNoSystemBackgroundProperty).isValid())
            widget->setAttribute(
                Qt::WA_NoSystemBackground,
                widget->property(originalNoSystemBackgroundProperty).toBool());
        if (widget->property(originalMarginsProperty).isValid()
            && !qobject_cast<QDialog *>(widget))
            widget->setContentsMargins(
                widget->property(originalMarginsProperty).value<QMargins>());
        if (auto *list = qobject_cast<QListView *>(widget))
            if (widget->property(originalListSpacingProperty).isValid())
                list->setSpacing(widget->property(originalListSpacingProperty).toInt());
        if (auto *dialog = qobject_cast<QDialog *>(widget)) {
            restoreContentDialogState(dialog, false);
        }
        if (auto *frame = qobject_cast<QFrame *>(widget))
            if (widget->property(originalFrameShapeProperty).isValid())
                frame->setFrameShape(static_cast<QFrame::Shape>(
                    widget->property(originalFrameShapeProperty).toInt()));
        if (widget->property(originalRoleWasValidProperty).isValid()) {
            if (widget->property(originalRoleWasValidProperty).toBool())
                widget->setProperty(roleProperty,
                                    widget->property(originalRoleProperty));
            else
                widget->setProperty(roleProperty, {});
        }
        if (auto *view = qobject_cast<QAbstractItemView *>(widget))
            NavigationPrivate::restoreNavigationView(view);
        if (auto *table = qobject_cast<QTableView *>(widget))
            d->untrackTable(table);
        else if (qobject_cast<const QTableView *>(itemView(widget)))
            d->untrackTableEditor(widget);
        if (const auto connection = d->toggleConnections.take(widget))
            disconnect(connection);
        if (auto *radio = qobject_cast<QRadioButton *>(widget))
            if (const auto connection = d->radioConnections.take(radio))
                disconnect(connection);
        if (const auto connection = d->tableConnections.take(widget))
            disconnect(connection);
        if (auto *checkBox = qobject_cast<QCheckBox *>(widget))
            d->toggleDragStates.remove(checkBox);
        if (auto *slider = qobject_cast<QSlider *>(widget))
            d->unregisterSlider(slider);
        if (auto *progressBar = qobject_cast<QProgressBar *>(widget))
            d->unregisterProgressBar(progressBar);
        if (auto *scrollBar = qobject_cast<QScrollBar *>(widget)) {
            d->cancelScrollBarTimer(scrollBar);
            d->unregisterScrollBar(scrollBar);
        }
        if (auto *slider = qobject_cast<QSlider *>(widget))
            hideSliderValueToolTip(slider);
        widget->removeEventFilter(this);
        framePropertyRegistry().clearObject(widget);
        widget->setProperty(ownedPaletteProperty, {});
        widget->setProperty(originalPaletteProperty, {});
        widget->setProperty(originalPaletteExplicitProperty, {});
        widget->setProperty(originalAutoFillProperty, {});
        widget->setProperty(originalHoverAttributeProperty, {});
        widget->setProperty(originalOpaquePaintProperty, {});
        widget->setProperty(originalTranslucentBackgroundProperty, {});
        widget->setProperty(originalNoSystemBackgroundProperty, {});
        widget->setProperty("_winui_original_mouse_tracking", {});
        widget->setProperty(originalListSpacingProperty, {});
        widget->setProperty(originalMinimumSizeProperty, {});
        widget->setProperty(originalFrameShapeProperty, {});
        widget->setProperty(originalMarginsProperty, {});
        widget->setProperty(originalSpacingProperty, {});
        widget->setProperty(originalRoleProperty, {});
        widget->setProperty(originalRoleWasValidProperty, {});
    }
}

bool Style::eventFilter(QObject *watched, QEvent *event)
{
    if (d->interactionController->eventFilter(watched, event))
        return true;
    return QProxyStyle::eventFilter(watched, event);
}
} // namespace WinUI3
