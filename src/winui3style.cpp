#include <winui3style/winui3style.h>
#include "winui3qtcompat_p.h"

#include <winui3style/winui3backdrop.h>
#include <winui3style/winui3icons.h>

#include "winui3geometry_p.h"
#include "winui3density_p.h"
#include "winui3animations_p.h"
#include "winui3backdrop_p.h"
#include "winui3buttons_p.h"
#include "winui3frameproperties_p.h"
#include "winui3helpers_p.h"
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
#include <QCalendarWidget>
#include <QComboBox>
#include <QCompleter>
#include <QCheckBox>
#include <QCommonStyle>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QDialog>
#include <QDynamicPropertyChangeEvent>
#include <QEvent>
#include <QFrame>
#include <QFontDatabase>
#include <QGroupBox>
#include <QGuiApplication>
#include <QLinearGradient>
#include <QListView>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPointer>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollBar>
#include <QSlider>
#include <QStyleOption>
#include <QStyleOptionProgressBar>
#include <QToolTip>
#include <QStyleHints>
#include <QStyledItemDelegate>
#include <QTabBar>
#include <QTableView>
#include <QTextEdit>
#include <QToolBar>
#include <QToolButton>
#include <QTimer>
#include <QVariantAnimation>
#include <QVector>
#include <QWidget>

#include <cmath>

namespace WinUI3 {
using namespace PaintPrivate;
using namespace Private;
namespace {

Backdrop backdropFromProperty(const QVariant &value)
{
    const QString name = value.toString().trimmed().toLower();
    if (name == QLatin1String("mica"))
        return Backdrop::Mica;
    if (name == QLatin1String("micaalt") || name == QLatin1String("mica-alt"))
        return Backdrop::MicaAlt;
    if (name == QLatin1String("acrylic"))
        return Backdrop::Acrylic;
    if (name == QLatin1String("none"))
        return Backdrop::None;
    bool ok = false;
    const int numeric = value.toInt(&ok);
    if (ok && numeric >= static_cast<int>(Backdrop::None)
        && numeric <= static_cast<int>(Backdrop::Acrylic))
        return static_cast<Backdrop>(numeric);
    return Backdrop::None;
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

bool densityModeFromProperty(const QVariant &value, WinUI3::DensityMode *mode)
{
    return Private::parseDensity(value, mode);
}

constexpr auto completerOwnerProperty = "_winui_completer_owner";
constexpr auto completerLastPopupProperty = "_winui_completer_last_popup";
constexpr auto completerOriginalDensityProperty = "_winui_completer_original_density";
constexpr auto completerOriginalDensityValidProperty =
    "_winui_completer_original_density_valid";

void syncCompleterPopupDensity(QLineEdit *editor)
{
    if (!editor || !editor->completer() || !editor->completer()->popup())
        return;
    QWidget *popup = editor->completer()->popup();
    const quintptr owner = reinterpret_cast<quintptr>(editor->style());
    auto restorePopup = [owner](QWidget *candidate) {
        if (!candidate || candidate->property(completerOwnerProperty).value<quintptr>()
                != owner)
            return;
        if (candidate->property(completerOriginalDensityValidProperty).toBool())
            candidate->setProperty(Style::DensityProperty,
                candidate->property(completerOriginalDensityProperty));
        else
            candidate->setProperty(Style::DensityProperty, {});
        for (const char *property : {completerOwnerProperty,
                 completerOriginalDensityProperty, completerOriginalDensityValidProperty})
            candidate->setProperty(property, {});
    };
    auto *previous = qobject_cast<QWidget *>(
        editor->property(completerLastPopupProperty).value<QObject *>());
    if (previous && previous != popup)
        restorePopup(previous);
    if (previous != popup) {
        editor->setProperty(completerLastPopupProperty,
                            QVariant::fromValue<QObject *>(popup));
        QObject::connect(popup, &QObject::destroyed, editor,
                         [editor](QObject *destroyed) {
            if (editor->property(completerLastPopupProperty).value<QObject *>()
                == destroyed)
                editor->setProperty(completerLastPopupProperty, {});
        });
    }
    if (!popup->property(completerOwnerProperty).isValid()) {
        popup->setProperty(completerOwnerProperty, QVariant::fromValue(owner));
        popup->setProperty(completerOriginalDensityValidProperty,
                           popup->property(Style::DensityProperty).isValid());
        popup->setProperty(completerOriginalDensityProperty,
                           popup->property(Style::DensityProperty));
    }
    if (popup->property(completerOwnerProperty).value<quintptr>() != owner)
        return;
    const QVariant density = QVariant::fromValue(Style::densityMode(editor));
    if (popup->property(Style::DensityProperty) != density)
        popup->setProperty(Style::DensityProperty, density);
    // QCompleter's private delegate caches its size hint. FontChange is the
    // least invasive public invalidation event that clears that cache; a
    // geometry update alone leaves the old 12 px native row in place.
    QEvent fontChange(QEvent::FontChange);
    QCoreApplication::sendEvent(popup, &fontChange);
    popup->updateGeometry();
    if (auto *view = qobject_cast<QAbstractItemView *>(popup)) {
        view->doItemsLayout();
        if (view->viewport())
            view->viewport()->update();
    }
}

void restoreCompleterPopup(QLineEdit *editor, Style *style)
{
    if (!editor)
        return;
    auto *popup = qobject_cast<QWidget *>(
        editor->property(completerLastPopupProperty).value<QObject *>());
    if (!popup) {
        editor->setProperty(completerLastPopupProperty, {});
        return;
    }
    if (popup->property(completerOwnerProperty).value<quintptr>()
        != reinterpret_cast<quintptr>(style))
        return;
    if (popup->property(completerOriginalDensityValidProperty).toBool())
        popup->setProperty(Style::DensityProperty,
                           popup->property(completerOriginalDensityProperty));
    else
        popup->setProperty(Style::DensityProperty, {});
    for (const char *property : {completerOwnerProperty,
             completerOriginalDensityProperty, completerOriginalDensityValidProperty})
        popup->setProperty(property, {});
    editor->setProperty(completerLastPopupProperty, {});
}

void invalidateDensityTree(QWidget *root)
{
    if (!root)
        return;
    const auto invalidateWidget = [](QWidget *widget) {
        widget->updateGeometry();
        widget->update();
        if (auto *editor = qobject_cast<QLineEdit *>(widget))
            syncCompleterPopupDensity(editor);
        if (qobject_cast<QMenuBar *>(widget)) {
            QEvent styleChange(QEvent::StyleChange);
            QCoreApplication::sendEvent(widget, &styleChange);
        } else if (auto *combo = qobject_cast<QComboBox *>(widget)) {
            QEvent styleChange(QEvent::StyleChange);
            QCoreApplication::sendEvent(combo, &styleChange);
            combo->updateGeometry();
        } else if (auto *dateTime = qobject_cast<QDateTimeEdit *>(widget)) {
            QEvent styleChange(QEvent::StyleChange);
            QCoreApplication::sendEvent(dateTime, &styleChange);
            dateTime->updateGeometry();
        } else if (auto *button = qobject_cast<QAbstractButton *>(widget)) {
            QEvent styleChange(QEvent::StyleChange);
            QCoreApplication::sendEvent(button, &styleChange);
            button->updateGeometry();
        }
        if (auto *view = qobject_cast<QAbstractItemView *>(widget)) {
            view->doItemsLayout();
            if (view->viewport())
                view->viewport()->update();
        }
    };
    invalidateWidget(root);
    const auto descendants = root->findChildren<QWidget *>();
    for (QWidget *widget : descendants)
        invalidateWidget(widget);
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
    case QStyle::PE_IndicatorTabClose:
    case QStyle::PE_PanelTipLabel:
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

    explicit StylePrivate(Style *owner, ThemeMode initialMode,
                         WinUI3::DensityMode initialDensity)
        : q(owner), mode(initialMode), density(initialDensity), animationDriver(owner),
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
        if (!popup)
            return;
        // QComboBox can change the popup viewport geometry between the view's
        // Show event and the popup window's Show event. Re-running this
        // idempotent preparation makes the selected-row anchor deterministic
        // on both the first and later openings.
        prepareComboPopupFirstFrameImpl(combo);
    }

    void finishComboPopupCycle(QWidget *popup)
    {
        if (!popup)
            return;
        if (const auto association = comboPopupAssociations.constFind(popup);
            association != comboPopupAssociations.constEnd()) {
            if (QComboBox *combo = association->data()) {
                if (combo->view() && combo->view()->viewport()) {
                    QWidget *viewport = combo->view()->viewport();
                    animationDriver.stop(viewport);
                    framePropertyRegistry().set(viewport, pressProperty, 0.0);
                }
                releaseComboChevron(combo);
            }
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
    WinUI3::DensityMode density = WinUI3::DensityMode::Standard;
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
    : Style(mode, WinUI3::DensityMode::Standard)
{
}

Style::Style(WinUI3::DensityMode density)
    : Style(ThemeMode::System, density)
{
}

Style::Style(ThemeMode mode, WinUI3::DensityMode density)
    : QProxyStyle(new QCommonStyle),
      d(std::make_unique<StylePrivate>(this, mode, density))
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
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    if (QStyleHints *hints = QGuiApplication::styleHints()) {
        connect(hints, &QStyleHints::colorSchemeChanged, this,
                [this](Qt::ColorScheme) { checkSystemAppearance(); });
    }
#endif
}

Style::~Style() = default;

ThemeMode Style::themeMode() const
{
    return d->mode;
}

WinUI3::DensityMode Style::densityMode() const
{
    return d->density;
}

WinUI3::DensityMode Style::effectiveDensityMode(const QWidget *widget) const
{
    for (const QWidget *candidate = widget; candidate;
         candidate = candidate->parentWidget()) {
        WinUI3::DensityMode local = WinUI3::DensityMode::Standard;
        if (densityModeFromProperty(candidate->property(DensityProperty), &local))
            return local;
    }
    return d->density;
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

void Style::setDensityMode(WinUI3::DensityMode mode)
{
    if (d->density == mode)
        return;
    d->density = mode;
    invalidateDensity();
    emit densityChanged(mode);
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
    QToolTip::setPalette(applicationPalette);
    // A popup's view and viewport are often created after their combo box was
    // polished. Prepare and register them now that the popup exists, before
    // walking the bounded owner registry below.
    for (QWidget *window : qApp->topLevelWidgets()) {
        const QVariant backdrop = window->property("_winui_backdrop");
        if (backdrop.isValid()) {
            QTimer::singleShot(0, window, [window, backdrop] {
                applyBackdrop(window, static_cast<Backdrop>(backdrop.toInt()));
            });
        } else if (qobject_cast<QDialog *>(window)) {
            applyDialogCaptionTheme(window);
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
        } else if (widget->property(originalPaletteExplicitProperty).toBool()) {
            // The widget carried an explicit palette before the style touched
            // it. A style-wide theme refresh must not clobber user-set
            // colors; painters already derive their tokens from the widget's
            // own palette at draw time.
            continue;
        } else {
            QPalette palette = applicationPalette;
            if (widget->property(SurfaceProperty).toString().compare(
                    QLatin1String("layer"), Qt::CaseInsensitive) == 0) {
                const QColor layer = Private::popupSurfaceColor(applicationPalette);
                palette.setColor(QPalette::Window, layer);
                if (qobject_cast<QAbstractItemView *>(widget))
                    palette.setColor(QPalette::Base, layer);
            } else if (qobject_cast<QTableView *>(widget)) {
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
                // SolidBackgroundFillColorBase (#202020 dark / #F3F3F3 light).
                palette.setColor(QPalette::Window,
                    darkTheme ? QColor(0x20, 0x20, 0x20)
                              : QColor(0xF3, 0xF3, 0xF3));
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

void Style::invalidateDensity(QWidget *scope)
{
    if (scope) {
        invalidateDensityTree(scope);
        return;
    }
    if (!qApp)
        return;
    const auto topLevels = qApp->topLevelWidgets();
    if (!topLevels.isEmpty()) {
        for (QWidget *window : topLevels)
            invalidateDensityTree(window);
        return;
    }
    // Widgets can exist before they are assigned a top-level window (for
    // example while a Designer form is being assembled).
    const auto widgets = QApplication::allWidgets();
    for (QWidget *widget : widgets)
        invalidateDensityTree(widget);
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

void Style::setDensityMode(QWidget *widget, WinUI3::DensityMode mode)
{
    if (!widget)
        return;
    widget->setProperty(DensityProperty, QVariant::fromValue(mode));
    invalidateDensityTree(widget);
}

WinUI3::DensityMode Style::densityMode(const QWidget *widget)
{
    if (!widget)
        return qApp && qobject_cast<const Style *>(qApp->style())
            ? qobject_cast<const Style *>(qApp->style())->densityMode()
            : WinUI3::DensityMode::Standard;

    if (const auto *style = qobject_cast<const Style *>(widget->style()))
        return style->effectiveDensityMode(widget);

    for (const QWidget *candidate = widget; candidate;
         candidate = candidate->parentWidget()) {
        WinUI3::DensityMode local = WinUI3::DensityMode::Standard;
        if (densityModeFromProperty(candidate->property(DensityProperty), &local))
            return local;
    }

    if (qApp) {
        if (const auto *style = qobject_cast<const Style *>(qApp->style()))
            return style->densityMode();
    }
    return WinUI3::DensityMode::Standard;
}

void Style::clearDensityMode(QWidget *widget)
{
    if (!widget)
        return;
    widget->setProperty(DensityProperty, QVariant());
    invalidateDensityTree(widget);
}

ControlRole Style::controlRole(const QWidget *widget)
{
    if (!widget)
        return ControlRole::Standard;
    const QVariant designerRole = widget->property(ControlRoleProperty);
    if (designerRole.isValid()) {
        const QString name = designerRole.toString().trimmed().toLower();
        if (name == QLatin1String("accent"))
            return ControlRole::Accent;
        if (name == QLatin1String("subtle"))
            return ControlRole::Subtle;
        if (name == QLatin1String("navigation"))
            return ControlRole::Navigation;
        if (name == QLatin1String("destructive"))
            return ControlRole::Destructive;
        if (name == QLatin1String("standard"))
            return ControlRole::Standard;
        bool converted = false;
        const int numericRole = designerRole.toInt(&converted);
        if (converted && numericRole >= int(ControlRole::Standard)
            && numericRole <= int(ControlRole::Destructive)) {
            return static_cast<ControlRole>(numericRole);
        }
    }
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

    if (element == PE_PanelTipLabel) {
        const QColor fill = t.dark ? QColor(43, 43, 43) : QColor(249, 249, 249);
        const QColor stroke = withAlpha(t.dark ? QColor(Qt::white) : QColor(Qt::black), 20);
        roundedRect(painter, QRectF(option->rect).adjusted(1, 1, -1, -1),
                    fill, stroke, 4);
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
        // preparePopupSurface already rebound the popup palette's Window role
        // to the raised translucent-layer stand-in color.
        const QColor popupSurface = option->palette.color(QPalette::Window);
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

    if (element == PE_Frame) {
        if (const QWidget *editor = richTextEditor(widget)) {
            const bool focused = editor->hasFocus();
            const bool editorEnabled = option->state & State_Enabled;
            const QColor fill = !editorEnabled ? t.controlDisabled
                : focused ? (t.dark ? QColor(30, 30, 30, 179)
                                  : QColor(255, 255, 255))
                          : (option->state & State_MouseOver ? t.controlHover
                                                            : t.control);
            controlSurface(painter, option->rect, fill, t.stroke, t.strokeSecondary,
                           ControlRadius);
            if (focused)
                drawEditorFocusUnderline(painter, QRectF(option->rect),
                                         t.accentFill, ControlRadius);
            return;
        }
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
        const bool textAboveLine = horizontal && bar && bar->textVisible
            && !bar->text.isEmpty();
        const QRect groove = horizontal
            ? (textAboveLine
               // The label owns the bar's body; the track becomes a thin
               // underline so the text never overlaps the fill.
               ? QRect(option->rect.left(), option->rect.bottom() - 2,
                       option->rect.width(), 3)
               : QRect(option->rect.left(), option->rect.center().y() - 2,
                       option->rect.width(), 4))
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
            const bool textAboveLine = horizontal && bar->textVisible
                && !bar->text.isEmpty();
            const QRect track = horizontal
                ? (textAboveLine
                   ? QRect(option->rect.left(), option->rect.bottom() - 2,
                           option->rect.width(), 3)
                   : QRect(option->rect.left(), option->rect.center().y() - 2,
                           option->rect.width(), 4))
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
                // Qt fills a vertical, non-inverted bar from the bottom up
                // (QCommonStyle flips "reverse" for vertical orientation), so
                // invertedAppearance must grow from the top instead.
                const int length = qRound(track.height() * ratio);
                if (inverted)
                    fill.setHeight(length);
                else
                    fill.setTop(track.bottom() - length + 1);
            }
            if (!fill.isEmpty())
                roundedRect(painter, fill, indicatorColor, Qt::transparent, 2);
            return;
        }
    }

    if (element == CE_ProgressBarLabel) {
        // WinUI's ProgressBar shows no inline percentage text, but Qt-based
        // applications may render meaningful information there with
        // textVisible=true. Keep their label: paint
        // it centered over the bar using the style's text color.
        if (const auto *bar = qstyleoption_cast<const QStyleOptionProgressBar *>(option)) {
            if (bar->textVisible && !bar->text.isEmpty()) {
                painter->save();
                const QPalette &pal = option->palette;
                const QPalette::ColorGroup group = !(bar->state & State_Enabled)
                    ? QPalette::Disabled
                    : (bar->state & State_Active
                       ? QPalette::Active : QPalette::Inactive);
                const QColor fg = pal.color(group, QPalette::WindowText);
                painter->setPen(QPen(fg));
                const auto *progressBar = qobject_cast<const QProgressBar *>(widget);
                const bool horizontal = !progressBar
                    || progressBar->orientation() == Qt::Horizontal;
                if (horizontal) {
                    // The thin track sits at the bottom of the rect; the label
                    // owns the area above it.  Elide so long transfer strings
                    // (e.g. "2,513,696 (22.0%)") never bleed past the bar.
                    const QRect labelRect = bar->rect.adjusted(0, 0, 0, -6);
                    painter->drawText(labelRect, Qt::AlignCenter,
                                      option->fontMetrics.elidedText(
                                          bar->text, Qt::ElideRight,
                                          labelRect.width()));
                } else {
                    painter->drawText(bar->rect, Qt::AlignCenter, bar->text);
                }
                painter->restore();
            }
            return;
        }
    }

    if (element == CE_ToolBar) {
        if (widget && widget->property(SurfaceProperty).isValid())
            painter->fillRect(option->rect,
                              widget->palette().color(QPalette::Window));
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
    QByteArray overrideFamily = qgetenv("WINUI3STYLE_APP_FONT");
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // Qt 6 renders the Windows 11 variable font natively.
    QString preferred = overrideFamily.isEmpty()
        ? QStringLiteral("Segoe UI Variable Text")
        : QString::fromLocal8Bit(overrideFamily);
    if (!QFontDatabase::families().contains(preferred))
        preferred = QStringLiteral("Segoe UI");
#else
    // Qt 5.x's DirectWrite path can rasterize the variable-font outlines
    // with touching/overlapping glyphs ("o"+"a" looking glued).  Use the
    // static family unless the app opts in via WINUI3STYLE_APP_FONT.
    QString preferred = overrideFamily.isEmpty()
        ? QStringLiteral("Segoe UI")
        : QString::fromLocal8Bit(overrideFamily);
    // QFontDatabase::families() is static from Qt 6 onward; instantiate on
    // Qt 5.
    const QFontDatabase fontDatabase;
    if (!fontDatabase.families().contains(preferred))
        preferred = QStringLiteral("Segoe UI");
#endif
    QFont font(preferred);
    font.setPixelSize(14);
    application->setFont(font);
    application->setPalette(standardPalette());
    QToolTip::setPalette(standardPalette());
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
    const QVariant surface = widget->property(SurfaceProperty);
    const QString surfaceName = surface.toString();
    if (surface.toBool()
        || surfaceName.compare(QLatin1String("content"),
                               Qt::CaseInsensitive) == 0
        || surfaceName.compare(QLatin1String("layer"),
                               Qt::CaseInsensitive) == 0) {
        // A native backdrop makes the top-level Window role transparent.
        // Standard stacked/page widgets otherwise retain stale backing-store
        // pixels while scrolling or switching pages. An explicit content
        // layer is the Qt equivalent of WinUI's opaque content surface.
        widget->setProperty(ownedPaletteProperty, true);
        d->registerPaletteOwner(widget);
        remember(widget, originalOpaquePaintProperty,
                 widget->testAttribute(Qt::WA_OpaquePaintEvent));
        QPalette palette = standardPalette();
        if (surfaceName.compare(QLatin1String("layer"),
                                Qt::CaseInsensitive) == 0) {
            const QColor layer = Private::popupSurfaceColor(palette);
            palette.setColor(QPalette::Window, layer);
            if (qobject_cast<QAbstractItemView *>(widget))
                palette.setColor(QPalette::Base, layer);
        }
        widget->setPalette(palette);
        widget->setAutoFillBackground(true);
        widget->setAttribute(Qt::WA_OpaquePaintEvent, true);
    }
    if (widget->isWindow()) {
        const QVariant backdrop = widget->property(BackdropProperty);
        if (backdrop.isValid()) {
            QPointer<QWidget> guarded(widget);
            QTimer::singleShot(0, widget, [guarded, backdrop] {
                if (guarded)
                    applyBackdrop(guarded, backdropFromProperty(backdrop));
            });
        }
    }
    framePropertyRegistry().set(widget, hoverProperty,
                                widget->isEnabled() && widget->underMouse()
                                    ? 1.0 : 0.0);
    framePropertyRegistry().set(widget, pressProperty, 0.0);
    framePropertyRegistry().set(widget, focusProperty,
                                widget->hasFocus() ? 1.0 : 0.0);
    framePropertyRegistry().set(widget, focusVisibleProperty,
                                widget->hasFocus() && d->keyboardInput);
    if (widget->property(DensityProperty).isValid())
        invalidateDensityTree(widget);
    if (auto *lineEdit = qobject_cast<QLineEdit *>(widget)) {
        prepareLineEditHelperButtons(lineEdit, this);
        syncCompleterPopupDensity(lineEdit);
    } else if (auto *combo = qobject_cast<QComboBox *>(widget)) {
        if (effectiveDensityMode(combo) == DensityMode::Compact) {
            QEvent styleChange(QEvent::StyleChange);
            QCoreApplication::sendEvent(combo, &styleChange);
            combo->updateGeometry();
        }
    } else if (auto *dateTime = qobject_cast<QDateTimeEdit *>(widget)) {
        if (effectiveDensityMode(dateTime) == DensityMode::Compact) {
            QEvent styleChange(QEvent::StyleChange);
            QCoreApplication::sendEvent(dateTime, &styleChange);
            dateTime->updateGeometry();
        }
    } else if (auto *button = qobject_cast<QAbstractButton *>(widget)) {
        if (effectiveDensityMode(button) == DensityMode::Compact) {
            QEvent styleChange(QEvent::StyleChange);
            QCoreApplication::sendEvent(button, &styleChange);
            button->updateGeometry();
        }
    }
    if (auto *view = qobject_cast<QAbstractItemView *>(widget)) {
        if (auto *completer = qobject_cast<QCompleter *>(view->parent())) {
            if (auto *editor = qobject_cast<QLineEdit *>(completer->widget()))
                syncCompleterPopupDensity(editor);
        }
    }
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
            || qobject_cast<QTabBar *>(toolButton->parentWidget())
            || (toolButton->parentWidget()
                && toolButton->parentWidget()->objectName()
                    == QStringLiteral("qt_calendar_navigationbar")))
            setControlRole(toolButton, ControlRole::Subtle);
    }

    // Qt hard-codes QCalendarWidget's navigation bar to the Highlight role,
    // producing an accent-blue strip unrelated to WinUI's CalendarView. Keep
    // the native calendar implementation but place its navigation controls on
    // the same neutral popup surface as the day grid.
    if (widget->objectName() == QStringLiteral("qt_calendar_navigationbar")
        && qobject_cast<QCalendarWidget *>(widget->parentWidget())) {
        widget->setBackgroundRole(QPalette::Window);
        widget->update();
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
        if (auto *lineEdit = qobject_cast<QLineEdit *>(widget)) {
            cancelLineEditHelperUpdate(lineEdit);
            cacheLineEditClearButton(lineEdit, nullptr);
            restoreCompleterPopup(lineEdit, this);
        }
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
    if (auto *editor = qobject_cast<QLineEdit *>(watched);
        editor && (event->type() == QEvent::FocusIn
                   || event->type() == QEvent::KeyPress
                   || event->type() == QEvent::MouseButtonPress)) {
        // setCompleter() has no change signal. User interaction is the point
        // at which a replacement popup can first become visible.
        syncCompleterPopupDensity(editor);
    }
    if (event->type() == QEvent::DynamicPropertyChange) {
        auto *change = static_cast<QDynamicPropertyChangeEvent *>(event);
        if (auto *widget = qobject_cast<QWidget *>(watched)) {
            if (change->propertyName() == DensityProperty) {
                // Density is inherited, so a change on a container affects
                // every descendant's geometry as well as its paint state.
                invalidateDensityTree(widget);
            } else if (change->propertyName() == ControlRoleProperty) {
                widget->update();
            } else if (change->propertyName() == SurfaceProperty) {
                const QVariant surface = widget->property(SurfaceProperty);
                const QString surfaceName = surface.toString();
                const bool enabled = surface.toBool()
                    || surfaceName.compare(QLatin1String("content"),
                                           Qt::CaseInsensitive) == 0
                    || surfaceName.compare(QLatin1String("layer"),
                                           Qt::CaseInsensitive) == 0;
                if (enabled) {
                    widget->setProperty(ownedPaletteProperty, true);
                    d->registerPaletteOwner(widget);
                    remember(widget, originalOpaquePaintProperty,
                             widget->testAttribute(Qt::WA_OpaquePaintEvent));
                    QPalette palette = standardPalette();
                    if (surfaceName.compare(QLatin1String("layer"),
                                            Qt::CaseInsensitive) == 0) {
                        const QColor layer = Private::popupSurfaceColor(palette);
                        palette.setColor(QPalette::Window, layer);
                        if (qobject_cast<QAbstractItemView *>(widget))
                            palette.setColor(QPalette::Base, layer);
                    }
                    widget->setPalette(palette);
                    widget->setAutoFillBackground(true);
                    widget->setAttribute(Qt::WA_OpaquePaintEvent, true);
                }
                widget->update();
            } else if (change->propertyName() == BackdropProperty
                       && widget->isWindow()) {
                applyBackdrop(widget, backdropFromProperty(
                    widget->property(BackdropProperty)));
                // Navigation surfaces are transparent only while a real
                // backdrop is active. Refresh just the opted-in views when a
                // window changes backdrop so offscreen/opaque windows never
                // retain transparent backing-store rows.
                if (auto *view = qobject_cast<QAbstractItemView *>(widget)) {
                    if (view->property(NavigationViewProperty).toBool())
                        NavigationPrivate::prepareNavigationView(view);
                }
                const auto views = widget->findChildren<QAbstractItemView *>();
                for (QAbstractItemView *view : views) {
                    if (view->property(NavigationViewProperty).toBool())
                        NavigationPrivate::prepareNavigationView(view);
                }
            }
        }
    }
    if (d->interactionController->eventFilter(watched, event))
        return true;
    return QProxyStyle::eventFilter(watched, event);
}
} // namespace WinUI3
