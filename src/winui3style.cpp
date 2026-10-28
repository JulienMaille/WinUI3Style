// SPDX-License-Identifier: LGPL-2.1-or-later
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
#include "winui3dialogs_p.h"
#include "winui3slidertooltip_p.h"
#include "winui3lineedithelpers_p.h"
#include "winui3popupsurface_p.h"
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
#include <QDialogButtonBox>
#include <QDynamicPropertyChangeEvent>
#include <QEvent>
#include <QFrame>
#include <QFontDatabase>
#include <QGroupBox>
#include <QGuiApplication>
#include <QLinearGradient>
#include <QListView>
#include <QLabel>
#include <QLineEdit>
#include <QLayout>
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
#include <QStatusBar>
#include <QStyleOptionSizeGrip>
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
#include <QThread>
#include <QToolBar>
#include <QWizard>
#include <QToolButton>
#include <QTimer>
#include <QVector>
#include <QWidget>
#include <QWindow>

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

bool hasRuntimePaletteOverride(const QWidget *widget)
{
    const QVariant snapshot = widget->property(implicitPaletteSnapshotProperty);
    return snapshot.isValid() && widget->testAttribute(Qt::WA_SetPalette)
            && widget->palette() != snapshot.value<QPalette>();
}

const QAbstractItemView *itemView(const QWidget *widget)
{
    if (const auto *view = qobject_cast<const QAbstractItemView *>(widget))
        return view;
    for (const QWidget *candidate = widget; candidate; candidate = candidate->parentWidget()) {
        if (const auto *view = qobject_cast<const QAbstractItemView *>(candidate))
            return view;
    }
    return nullptr;
}

void repaintPaletteOwnedWidgetTree(QWidget *widget)
{
    if (!widget)
        return;
    widget->update();
    if (auto *view = qobject_cast<QAbstractItemView *>(widget)) {
        view->update();
        if (view->viewport())
            view->viewport()->update();
    }
}

bool insideCalendarWidget(const QWidget *widget)
{
    for (const QWidget *candidate = widget; candidate; candidate = candidate->parentWidget()) {
        if (qobject_cast<const QCalendarWidget *>(candidate))
            return true;
    }
    return false;
}

// CalendarViewBackground is InputActive over its enclosing content/card,
// not the translucent item-view Base and not the popup acrylic recipe.
QColor inlineCalendarSurface(const QWidget *widget, const QPalette &applicationPalette)
{
    const QWidget *calendar = widget;
    while (calendar && !qobject_cast<const QCalendarWidget *>(calendar))
        calendar = calendar->parentWidget();
    const Private::Tokens t = Private::tokens(applicationPalette);
    const bool composited = widget
            && Private::backdropEffectiveSurface(widget->window())
                    == Private::BackdropSurface::Composited;
    QImage pixel(1, 1, QImage::Format_ARGB32_Premultiplied);
    // Granted Mica has no opaque Window under the card: start transparent so
    // the card veil and InputActive compose their alphas instead of baking
    // onto an opaque seed. Refused/offscreen fallbacks keep the opaque
    // surface seed and resolve fully opaque below.
    if (composited)
        pixel.fill(Qt::transparent);
    else
        pixel.fill(t.surface);
    QPainter painter(&pixel);
    QList<const QWidget *> ancestors;
    for (const QWidget *parent = calendar ? calendar->parentWidget() : nullptr; parent;
         parent = parent->parentWidget())
        ancestors.prepend(parent);
    for (const QWidget *parent : ancestors) {
        if (parent->property(Style::SurfaceProperty).toString() == QLatin1String("layer"))
            painter.fillRect(pixel.rect(), Private::popupSurfaceColor(applicationPalette));
        else if (qobject_cast<const QGroupBox *>(parent))
            painter.fillRect(
                    pixel.rect(),
                    Private::veiledCard(t.layer,
                                        composited && Private::paintsDirectlyOnBackdrop(parent)));
    }
    painter.fillRect(pixel.rect(), t.editorFocusedFill);
    painter.end();
    return pixel.pixelColor(0, 0);
}

void rebaseBackdropIslands(QWidget *window, const QPalette &applicationPalette)
{
    if (!window->property("_winui_backdrop").isValid())
        return;

    // Islands transparentized by syncContentSurfacesForBackdrop carry no
    // palette-owner registration, so the owner loop below never visits them.
    // Rebase their Window RGB here, preserving the live alpha: opaque islands
    // follow the theme, transparent live-material islands keep revealing DWM
    // with fresh RGB. Runs before the owner loop so registered owners keep
    // the last word on their own recipe below.
    const QList<QWidget *> islands = window->findChildren<QWidget *>();
    for (QWidget *island : islands) {
        if (!isContentLayerSurface(island->property(Style::SurfaceProperty)))
            continue;
        QPalette rebased = island->palette();
        QColor rebasedWindow = applicationPalette.color(QPalette::Window);
        rebasedWindow.setAlpha(rebased.color(QPalette::Window).alpha());
        if (rebased.color(QPalette::Window) == rebasedWindow)
            continue;
        rebased.setColor(QPalette::Window, rebasedWindow);
        island->setPalette(rebased);
        island->update();
    }
}

// The navigation-bar lane (qt_calendar_navigationbar) paints its empty
// stretches with its own Window fill while the month/year/prev/next
// QToolButtons paint over it. The lane reads as one surface only when every
// button child shares the bar's opaque Window surface instead of the default
// Button role/palette.
bool insideCalendarNavigationBar(const QWidget *widget)
{
    const QWidget *parent = widget ? widget->parentWidget() : nullptr;
    return parent && parent->objectName() == QStringLiteral("qt_calendar_navigationbar")
            && insideCalendarWidget(widget);
}

void refreshCalendarNavigationLanes(QWidget *window, const QPalette &applicationPalette,
                                    const QVector<QPointer<QWidget>> &owners)
{
    // Phase 2: the lane copies the post-phase-1 bar exactly (RGB+alpha),
    // guaranteed same pass. Lane buttons paint Subtle flat at rest over
    // the now-translucent bar; roles stay untouched here per the two-phase
    // contract (Standard->Subtle happens in polish only).
    for (const QPointer<QWidget> &guarded : owners) {
        QWidget *widget = guarded.data();
        if (!widget || !insideCalendarNavigationBar(widget))
            continue;
        if (QWidget *ownerWindow = widget->window();
            ownerWindow != window && ownerWindow->isWindow())
            continue;
        if (widget->property(originalPaletteExplicitProperty).toBool()
            && !widget->property(ownedPaletteProperty).toBool())
            continue;
        const QWidget *navigationBar = widget->parentWidget();
        const QColor laneWindow = navigationBar ? navigationBar->palette().color(QPalette::Window)
                                                : applicationPalette.color(QPalette::Window);
        QPalette lanePalette = applicationPalette;
        lanePalette.setColor(QPalette::Window, laneWindow);
        // The year editor frame fills from the Button role: repoint it at
        // the lane surface so hover-free pixels match the bar's empty
        // stretches; Button/Window/Base stay one identical tint.
        lanePalette.setColor(QPalette::Button, laneWindow);
        if (qobject_cast<QAbstractSpinBox *>(widget))
            lanePalette.setColor(QPalette::Base, laneWindow);
        widget->setPalette(lanePalette);
        repaintPaletteOwnedWidgetTree(widget);
    }
}

QPalette ownedPaletteForWidget(QWidget *widget, QWidget *window, const QPalette &applicationPalette,
                               const Private::Tokens &applicationTokens,
                               const QColor &applicationAccent)
{
    QPalette palette = applicationPalette;
    if (qobject_cast<QStatusBar *>(widget) || qobject_cast<QWizard *>(widget)) {
        palette.setColor(QPalette::Window, Private::popupSurfaceColor(applicationPalette));
    } else if (qobject_cast<QWizardPage *>(widget)) {
        palette.setColor(QPalette::Window, Private::popupSurfaceColor(applicationPalette));
    } else if (auto *calendarGrid = qobject_cast<QTableView *>(widget);
               calendarGrid && insideCalendarWidget(widget)) {
        // Transient pickers spawn a QCalendarWidget in Qt::Popup (gets
        // the preparePopupSurface palette rebase) while the Dialogs
        // persistentCalendar is inline (keeps the app palette). The
        // shared calendarPopupView day chrome paints its own accent
        // circle, so neutralize the native Highlight role for ALL
        // calendar grids here, not only popups. Keep Base on the
        // content surface (only real popups get the flyout tint) and
        // kill just the native blue rect/strip.
        palette.setColor(QPalette::Highlight, Qt::transparent);
        palette.setColor(QPalette::HighlightedText, applicationTokens.textPrimary);
    } else if (widget->property(Style::SurfaceProperty)
                       .toString()
                       .compare(QLatin1String("layer"), Qt::CaseInsensitive)
               == 0) {
        const QColor layer = Private::popupSurfaceColor(applicationPalette);
        palette.setColor(QPalette::Window, layer);
        if (qobject_cast<QAbstractItemView *>(widget))
            palette.setColor(QPalette::Base, layer);
    } else if (qobject_cast<QTableView *>(widget)) {
        palette.setColor(QPalette::Highlight, applicationTokens.subtleHover);
        palette.setColor(QPalette::HighlightedText, applicationTokens.textPrimary);
    } else if (auto *editor = qobject_cast<QLineEdit *>(widget); editor && itemView(editor)) {
        palette.setColor(QPalette::Highlight, applicationAccent);
        palette.setColor(QPalette::HighlightedText, applicationTokens.textOnAccentPrimary);
    } else if (widget->objectName() == QStringLiteral("qt_calendar_navigationbar")
               && qobject_cast<QCalendarWidget *>(widget->parentWidget())) {
        // Popup bars are handled by preparePopupSurface above. Inline
        // bars follow the same Composited-aware surface as the day
        // grid (mirroring polish): veiled under a granted backdrop so
        // bar, lane, and body read as one surface; opaque Window only
        // on the refused/offscreen fallback.
        QColor navigationWindow;
        if (Private::backdropEffectiveSurface(widget->window())
            == Private::BackdropSurface::Composited)
            navigationWindow = inlineCalendarSurface(widget, applicationPalette);
        else {
            navigationWindow = applicationPalette.color(QPalette::Window);
            navigationWindow.setAlpha(255);
        }
        palette.setColor(QPalette::Window, navigationWindow);
    } else if (qobject_cast<QDialog *>(widget)) {
        palette.setColor(QPalette::Window, Private::popupSurfaceColor(applicationPalette));
    }
    if (insideCalendarWidget(widget)) {
        const QColor surface = inlineCalendarSurface(widget, applicationPalette);
        palette.setColor(QPalette::Window, surface);
        palette.setColor(QPalette::Base, surface);
    }
    // applyBackdrop owns the top-level alpha, including the synchronous
    // effective-surface notification that brought us here. Rebase the
    // theme RGB without replacing that in-flight material recipe.
    if (widget == window && window->property("_winui_backdrop").isValid())
        palette.setColor(QPalette::Window,
                         withAlpha(palette.color(QPalette::Window),
                                   widget->palette().color(QPalette::Window).alpha()));
    return palette;
}

bool refreshNestedPopupCalendarPalette(QWidget *widget, const QPalette &applicationPalette)
{
    if (!widget || !widget->window() || widget->window()->windowType() != Qt::Popup)
        return false;

    auto *calendar = qobject_cast<QCalendarWidget *>(widget);
    if (!calendar || widget->windowType() == Qt::Popup)
        return false;

    // Hidden calendar popups skip the owned-palette branches
    // (preparePopupSurface re-asserts the surface on show), but
    // their chrome still needs the surface-roles semantics now:
    // converge bar and lane onto the uniform popup tint (same
    // popupSurfaceColor recipe + alpha preparePopupSurface paints)
    // without manufacturing winId/DWM state (the callee already
    // guards windowHandle/offscreen itself). Bar first, lane copies
    // the bar exactly, preserving the calendar -> bar -> lane order.
    preparePopupSurface(widget);
    if (QWidget *bar =
                calendar->findChild<QWidget *>(QStringLiteral("qt_calendar_navigationbar"))) {
        QPalette barPalette = applicationPalette;
        const QColor barWindow = widget->window()->palette().color(QPalette::Window);
        barPalette.setColor(QPalette::Window, barWindow);
        barPalette.setColor(QPalette::Base, barWindow);
        bar->setPalette(barPalette);
        bar->update();
        calendar->update();
    }
    repaintPaletteOwnedWidgetTree(widget);
    return true;
}

void refreshOwnedPalette(QWidget *widget, QWidget *window, const QPalette &applicationPalette,
                         const Private::Tokens &applicationTokens, const QColor &applicationAccent,
                         bool darkTheme);

void refreshOwnedPalette(QWidget *widget, QWidget *window, const QPalette &applicationPalette,
                         const Private::Tokens &applicationTokens, const QColor &applicationAccent,
                         bool darkTheme)
{
    if (!widget)
        return;

    // Per-window pass: refreshOwnedPalettes() is also invoked on a single
    // window when its backdrop surface toggles, so owners belonging to a
    // different real window are left untouched this pass. Owners whose
    // window is not a real top-level window (Designer-assembled or
    // orphan hierarchies) keep the old visit-every-pass behavior.
    if (QWidget *ownerWindow = widget->window(); ownerWindow != window && ownerWindow->isWindow())
        return;
    if (hasRuntimePaletteOverride(widget)) {
        // A palette installed after polish belongs to the application. The
        // implicit owner registration must not turn it into a style palette.
        widget->setProperty(originalPaletteProperty, QVariant::fromValue(widget->palette()));
        widget->setProperty(originalPaletteExplicitProperty, true);
        widget->setProperty(ownedPaletteProperty, false);
        widget->setProperty(implicitPaletteSnapshotProperty, {});
        return;
    }
    if (insideCalendarNavigationBar(widget))
        return;
    if (widget->property(Style::NavigationViewProperty).toBool()
        && !widget->property(Style::SurfaceProperty).isValid()) {
        // The delegate owns temporary navigation palettes and their saved
        // inheritance. Do not overwrite them in the generic owner pass.
        if (auto *view = qobject_cast<QAbstractItemView *>(widget))
            NavigationPrivate::refreshNavigationPalette(view, applicationPalette);
        return;
    }
    if (widget->window() && widget->window()->windowType() == Qt::Popup) {
        if (refreshNestedPopupCalendarPalette(widget, applicationPalette))
            return;
        preparePopupSurface(widget);
    } else if (auto *wizard = qobject_cast<QWizard *>(widget)) {
        refreshWizardSurface(wizard, applicationPalette);
    } else if (widget->property(originalPaletteExplicitProperty).toBool()
               && !widget->property(ownedPaletteProperty).toBool()) {
        // The widget carried an explicit palette before the style touched
        // it, and the style never claimed it (no surface, no implicit
        // registration). A style-wide theme refresh must not clobber
        // user-set colors; painters already derive their tokens from the
        // widget's own palette at draw time. Style-claimed widgets
        // (ownedPaletteProperty) always rebase: their "explicit" flag
        // only records the factory palette Qt set before polish.
        return;
    } else {
        QPalette palette = ownedPaletteForWidget(widget, window, applicationPalette,
                                                 applicationTokens, applicationAccent);
        // Single-pass theme refresh: a surface the backdrop sync already
        // transparentized keeps its live-material recipe (alpha 0) while its
        // RGB follows the new theme. Attributes (no autofill, StyledBackground)
        // are untouched by this pass, so no corrective re-sync is needed.
        if (Private::backdropEffectiveSurface(window) == Private::BackdropSurface::Composited
            && Private::isTransparentizedForBackdrop(widget))
            palette.setColor(QPalette::Window, withAlpha(palette.color(QPalette::Window), 0));
        Private::setBackdropPalette(widget, palette);
        if (auto *dialog = qobject_cast<QDialog *>(widget))
            prepareContentDialogState(dialog, darkTheme);
    }
    repaintPaletteOwnedWidgetTree(widget);
}

const QEasingCurve &fluentCurve()
{
    static const QEasingCurve curve = [] {
        QEasingCurve result(QEasingCurve::BezierSpline);
        result.addCubicBezierSegment(QPointF(0.0, 0.0), QPointF(0.0, 1.0), QPointF(1.0, 1.0));
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

constexpr auto completerLastPopupProperty = "_winui_completer_last_popup";
constexpr auto completerOriginalDensityProperty = "_winui_completer_original_density";
constexpr auto completerOriginalDensityValidProperty = "_winui_completer_original_density_valid";

void syncCompleterPopupDensity(QLineEdit *editor)
{
    if (!editor || !editor->completer() || !editor->completer()->popup())
        return;
    QWidget *popup = editor->completer()->popup();
    const quintptr owner = reinterpret_cast<quintptr>(editor->style());
    auto restorePopup = [owner](QWidget *candidate) {
        if (!candidate || candidate->property(completerOwnerProperty).value<quintptr>() != owner)
            return;
        if (candidate->property(completerOriginalDensityValidProperty).toBool())
            candidate->setProperty(Style::DensityProperty,
                                   candidate->property(completerOriginalDensityProperty));
        else
            candidate->setProperty(Style::DensityProperty, {});
        for (const char *property : { completerOwnerProperty, completerOriginalDensityProperty,
                                      completerOriginalDensityValidProperty })
            candidate->setProperty(property, {});
    };
    auto *previous = qobject_cast<QWidget *>(
            editor->property(completerLastPopupProperty).value<QObject *>());
    if (previous && previous != popup)
        restorePopup(previous);
    if (previous != popup) {
        editor->setProperty(completerLastPopupProperty, QVariant::fromValue<QObject *>(popup));
        QObject::connect(popup, &QObject::destroyed, editor, [editor](QObject *destroyed) {
            if (editor->property(completerLastPopupProperty).value<QObject *>() == destroyed)
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
    for (const char *property : { completerOwnerProperty, completerOriginalDensityProperty,
                                  completerOriginalDensityValidProperty })
        popup->setProperty(property, {});
    editor->setProperty(completerLastPopupProperty, {});
}

void invalidateDensityTree(QWidget *root)
{
    if (!root)
        return;
    // Bottom-up : les enfants d'abord, le parent ensuite. Un layout parent
    // repositionne ses enfants ; si le parent repeint avant, ses lignes
    // backing-store gardent les enfants a l'ancienne geometrie (doublons
    // "Palette lab", combos superposes). Le layout doit etre actif avant le
    // repaint, sinon updateGeometry() ne fait que marquer dirty.
    const auto invalidateWidget = [](QWidget *widget) {
        if (widget->layout())
            widget->layout()->activate();
        widget->updateGeometry();
        // Density changes resize rows/controls in place: every backing store
        // keeps stale rows exactly like a scroll blit (the left-pane smear
        // when switching Standard/Compact). update() only repaints the
        // exposed strip, so force the synchronous full repaint here, like
        // the island scroll guard does after a valueChanged tick.
        widget->repaint();
        if (auto *editor = qobject_cast<QLineEdit *>(widget))
            syncCompleterPopupDensity(editor);
        if (auto *completerView = qobject_cast<QAbstractItemView *>(widget)) {
            // Hidden completer popups are top-level native popups: no density
            // ancestor reaches them from the editor tree, and the delegate
            // caches its sizeHint. Re-sync any completer view (claimed or
            // fresh from QCompleter::popup()) through the same editor path
            // so a hidden switch lands before the next show.
            const bool completerViewClaimed =
                    completerView->property(completerOwnerProperty).isValid()
                    || qobject_cast<QCompleter *>(completerView->parent());
            if (completerViewClaimed) {
                if (auto *completer = qobject_cast<QCompleter *>(completerView->parent())) {
                    if (auto *reanchor = qobject_cast<QLineEdit *>(completer->widget()))
                        syncCompleterPopupDensity(reanchor);
                }
            }
        }
        if (qobject_cast<QMenuBar *>(widget)) {
            QEvent styleChange(QEvent::StyleChange);
            QCoreApplication::sendEvent(widget, &styleChange);
        } else if (auto *combo = qobject_cast<QComboBox *>(widget)) {
            QEvent styleChange(QEvent::StyleChange);
            QCoreApplication::sendEvent(combo, &styleChange);
            combo->updateGeometry();
            // Open popup: rows and geometry follow density. Rows are already
            // re-laid out (doItemsLayout below), but Qt never re-lays out a
            // visible popup on its own: its height keeps the Standard rows
            // (fail-first openComboPopupFollowsDensitySwitch: 220x120 instead
            // of 220x96 for 3 rows 40 -> 32). Re-prepare the frame, then
            // resize the popup to the new rows.
            if (combo->view() && combo->view()->isVisible()) {
                combo->view()->doItemsLayout();
                Private::prepareComboPopupFirstFrameImpl(combo);
                if (QWidget *popup = combo->view()->window()) {
                    // Measure through the delegate (sizeHint exposes the
                    // comboPopupItemHeight metric), not through visualRect:
                    // the viewport layout has not picked up the new rows
                    // yet at switch time.
                    const QModelIndex first =
                            combo->model()->index(0, combo->modelColumn(), combo->rootModelIndex());
                    QStyleOptionViewItem itemOption;
                    itemOption.initFrom(combo->view()->viewport());
                    itemOption.index = first;
                    const QSize rowSize = combo->style()->sizeFromContents(
                            QStyle::CT_ItemViewItem, &itemOption, QSize(), combo->view());
                    const int margins =
                            popup->contentsMargins().top() + popup->contentsMargins().bottom();
                    popup->resize(popup->width(), combo->count() * rowSize.height() + margins);
                }
            }
        } else if (auto *spinBox = qobject_cast<QAbstractSpinBox *>(widget)) {
            QEvent styleChange(QEvent::StyleChange);
            QCoreApplication::sendEvent(spinBox, &styleChange);
            spinBox->updateGeometry();
        } else if (auto *button = qobject_cast<QAbstractButton *>(widget)) {
            QEvent styleChange(QEvent::StyleChange);
            QCoreApplication::sendEvent(button, &styleChange);
            button->updateGeometry();
        } else if (auto *menu = qobject_cast<QMenu *>(widget)) {
            QEvent styleChange(QEvent::StyleChange);
            QCoreApplication::sendEvent(menu, &styleChange);
            // Visible popup: rows follow the profile like the open combo
            // popup above. QMenu caches its layout while shown, so force a
            // resize to the new row profile.
            if (menu->isVisible() && !menu->actions().isEmpty()) {
                const int compact = qobject_cast<const Style *>(menu->style())
                        ? densityMetricsFor(menu, qobject_cast<const Style *>(menu->style()))
                                  .menuBarItemHeight
                        : densityMetricsFor(menu).menuBarItemHeight;
                const int row = compact == 24 ? 32 : 36;
                const int width = menu->width();
                menu->resize(width, menu->actions().size() * row);
            }
            menu->updateGeometry();
        }
        if (auto *view = qobject_cast<QAbstractItemView *>(widget)) {
            view->doItemsLayout();
            if (view->viewport())
                view->viewport()->update();
        }
    };
    // Bottom-up: leaves first. findChildren returns parents before children,
    // so walk in reverse. Each level is repositioned and repainted before
    // its parent freezes its own rows.
    const auto descendants = root->findChildren<QWidget *>(QString(), Qt::FindChildrenRecursively);
    for (auto it = descendants.crbegin(); it != descendants.crend(); ++it)
        invalidateWidget(*it);
    invalidateWidget(root);
}

// Elements the style owns end to end: their paint paths never fall through to
// QCommonStyle, so Style::drawPrimitive asserts on any that does.
//
// Rule for adding one: list an element here only when EVERY case reaches a
// renderer. An element handled just for specific widget conditions must NOT be
// listed — falling through to the base style is the intended behavior for the
// remaining cases, and the assert would fire on a legitimate fallback. Today
// those deliberately partial elements are:
//   * PE_Frame — owned for popup windows, item views and rich-text editors;
//     a generic QFrame keeps the base frame;
//   * CE_ShapedFrame — owned only for winuiSettingsCard widgets;
//   * CE_CheckBoxLabel / CE_RadioButtonLabel — never painted, they only
//     receive the shared button-family backdrop pre-erase;
//   * the CE_* buttons dispatched through the renderer seams — a seam returns
//     false when it does not handle the element, which is a fallthrough, not a
//     missed case.
// The rule is checked mechanically by tools/covered_elements_check.cpp, wired
// as the winui3style_covered_elements CTest, so a newly claimed element cannot
// be added silently.
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
    case QStyle::PE_FrameMenu:
    case QStyle::PE_Widget:
    case QStyle::PE_PanelItemViewItem:
    case QStyle::PE_IndicatorBranch:
    case QStyle::PE_IndicatorHeaderArrow:
    case QStyle::PE_IndicatorTabClose:
    case QStyle::PE_PanelTipLabel:
    case QStyle::PE_IndicatorToolBarSeparator:
    case QStyle::PE_FrameDockWidget:
    case QStyle::PE_IndicatorDockWidgetResizeHandle:
    case QStyle::PE_PanelStatusBar:
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
    case QStyle::CE_SizeGrip:
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

    explicit StylePrivate(Style *owner, ThemeMode initialMode, WinUI3::DensityMode initialDensity)
        : q(owner),
          mode(initialMode),
          density(initialDensity),
          animationDriver(owner),
          tableEditorTracker(owner)
    {
        Private::StyleInteractionCallbacks callbacks;
        callbacks.animate = [this](QWidget *widget, const char *property, qreal target,
                                   int duration) { animate(widget, property, target, duration); };
        callbacks.beginButtonPress = [this](QWidget *widget) { beginButtonPress(widget); };
        callbacks.releaseButtonPress = [this](QWidget *widget) { releaseButtonPress(widget); };
        callbacks.cancelButtonPress = [this](QWidget *widget) { cancelButtonPress(widget); };
        callbacks.stopAnimations = [this](QWidget *widget) { stopAnimations(widget); };
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
        callbacks.cancelSliderToolTip = [this](QSlider *slider) { cancelSliderToolTip(slider); };
        callbacks.refreshProgressTimer = [this] { refreshProgressTimer(); };
        callbacks.progressTimerActive = [this] {
            return progressTimer && progressTimer->isActive();
        };
        callbacks.prepareComboPopupFirstFrame = [this](QComboBox *combo) {
            prepareComboPopupFirstFrame(combo);
        };
        callbacks.releaseComboChevron = [this](QWidget *widget) { releaseComboChevron(widget); };
        callbacks.finishComboPopupCycle = [this](QWidget *popup) { finishComboPopupCycle(popup); };
        callbacks.comboForPopupWidget = [](QWidget *widget) { return comboForPopupWidget(widget); };
        callbacks.updateReadOnlyDeleteAffordance = [](QLineEdit *lineEdit) {
            updateReadOnlyDeleteAffordance(lineEdit);
        };
        callbacks.prepareLineEditHelperButtons = [owner](QLineEdit *lineEdit) {
            prepareLineEditHelperButtons(lineEdit, owner);
        };
        callbacks.prepareContentDialogState = [](QDialog *dialog, bool dark) {
            prepareContentDialogState(dialog, dark);
        };
        callbacks.stopDialogAnimations = [](QDialog *dialog) { stopDialogAnimations(dialog); };
        callbacks.preparePopupSurface = [](QWidget *widget) { preparePopupSurface(widget); };
        callbacks.registerPopupPaletteOwners = [this](QWidget *widget) {
            registerPopupPaletteOwners(widget);
        };
        callbacks.registerPaletteOwner = [this](QDialog *dialog) { registerPaletteOwner(dialog); };
        callbacks.unregisterPaletteOwner = [this](QDialog *dialog) {
            unregisterPaletteOwner(dialog);
        };
        callbacks.restoreContentDialogState = [](QDialog *dialog, bool visible) {
            restoreContentDialogState(dialog, visible);
        };
        callbacks.remember = [](QWidget *widget, const char *property, const QVariant &value) {
            remember(widget, property, value);
        };
        callbacks.prepareNavigationView = [](QAbstractItemView *view) {
            NavigationPrivate::prepareNavigationView(view);
        };
        callbacks.restoreNavigationView = [](QAbstractItemView *view) {
            NavigationPrivate::restoreNavigationView(view);
        };
        callbacks.dark = [this] { return dark(); };
        callbacks.keyboardInput = &keyboardInput;
        callbacks.toggleDragStates = &toggleDragStates;
        interactionController =
                std::make_unique<Private::StyleInteractionController>(owner, std::move(callbacks));
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
        paletteOwnerConnections.insert(
                widget, QObject::connect(widget, &QObject::destroyed, q, [this, widget] {
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
        const qreal phase =
                allowed ? qreal(QDateTime::currentMSecsSinceEpoch() % 1500) / 1500.0 : 0.35;
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
        progressBarStateConnections.insert(
                progressBar,
                QObject::connect(progressBar, &QProgressBar::valueChanged, q,
                                 [this](int) { refreshProgressTimer(); }));
        QObject::connect(progressBar, &QObject::destroyed, q,
                         [this, progressBar] { unregisterProgressBar(progressBar); });
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
        if (auto it = scrollBarTimers.find(scrollBar); it != scrollBarTimers.end() && it->data()) {
            return it->data();
        }
        auto *timer = new QTimer(scrollBar);
        timer->setObjectName(QStringLiteral("_winui_scrollbar_timer"));
        timer->setSingleShot(true);
        const QPointer<QScrollBar> guarded(scrollBar);
        QObject::connect(timer, &QTimer::timeout, q, [this, guarded] {
            if (!guarded || !guarded->isVisible() || !guarded->isEnabled()
                || !framePropertyRegistry().value(guarded, scrollBarInsideProperty).isValid()) {
                return;
            }
            if (framePropertyRegistry().value(guarded, scrollBarInsideProperty).toBool()) {
                animate(guarded, hoverProperty, 1.0, Private::FastDuration);
            } else {
                animate(guarded, hoverProperty, 0.0, Private::FastDuration);
            }
        });
        scrollBarTimers.insert(scrollBar, QPointer<QTimer>(timer));
        // Pure parent ownership: the timer is parented to the scroll bar, so
        // Qt deletes it with its parent. The destroyed connection is tracked
        // explicitly so unregister (e.g. unpolish) can disconnect it and no
        // connection accumulates across polish/unpolish cycles.
        if (const auto previous = scrollBarDestroyConnections.take(scrollBar))
            QObject::disconnect(previous);
        scrollBarDestroyConnections.insert(
                scrollBar, QObject::connect(scrollBar, &QObject::destroyed, q, [this, scrollBar] {
                    unregisterScrollBar(scrollBar);
                }));
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
        if (auto it = scrollBarTimers.find(scrollBar); it != scrollBarTimers.end() && it->data()) {
            it->data()->stop();
        }
    }

    void unregisterScrollBar(QScrollBar *scrollBar)
    {
        if (!scrollBar)
            return;
        // Pure parent ownership: stop and erase only. The timer is a child
        // of the scroll bar, so an explicit delete here would double-delete
        // during parent teardown. Disconnect the tracked destroyed
        // connection so repeated polish/unpolish cannot accumulate it.
        if (const auto connection = scrollBarDestroyConnections.take(scrollBar))
            QObject::disconnect(connection);
        if (auto it = scrollBarTimers.find(scrollBar); it != scrollBarTimers.end()) {
            if (QTimer *timer = it->data())
                timer->stop();
            scrollBarTimers.erase(it);
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
        // Pure parent ownership, mirroring the scroll bar timers above: the
        // timer is parented to the slider and the destroyed connection is
        // tracked so unregister can disconnect it (no accumulation).
        if (const auto previous = sliderToolTipDestroyConnections.take(slider))
            QObject::disconnect(previous);
        sliderToolTipDestroyConnections.insert(
                slider, QObject::connect(slider, &QObject::destroyed, q, [this, slider] {
                    unregisterSlider(slider);
                }));
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
        // Pure parent ownership, mirroring unregisterScrollBar: stop and
        // erase only (no manual delete; the parented timer dies with the
        // slider), and disconnect the tracked destroyed connection.
        if (const auto connection = sliderToolTipDestroyConnections.take(slider))
            QObject::disconnect(connection);
        if (auto it = sliderToolTipTimers.find(slider); it != sliderToolTipTimers.end()) {
            if (QTimer *timer = it->data())
                timer->stop();
            sliderToolTipTimers.erase(it);
        }
    }

    void animate(QWidget *widget, const char *property, qreal target, int duration)
    {
        animationDriver.animate(widget, property, target, duration, animationsAllowed(),
                                fluentCurve());
    }

    void stopAnimations(QWidget *widget) { animationDriver.stop(widget); }

    void beginButtonPress(QWidget *widget)
    {
        if (!widget)
            return;
        const qulonglong generation =
                framePropertyRegistry().value(widget, buttonPressGenerationProperty).toULongLong()
                + 1;
        framePropertyRegistry().set(widget, buttonPressGenerationProperty,
                                    QVariant::fromValue(generation));
        framePropertyRegistry().set(widget, buttonPressReleasePendingProperty, false);
        // A synchronous pressed frame is intentional. It makes a very fast
        // click observable and also cancels a release animation already in
        // flight before the next press starts.
        animate(widget, pressProperty, 1.0, 0);
    }

    void releaseButtonPress(QWidget *widget)
    {
        if (!widget)
            return;
        const qulonglong generation =
                framePropertyRegistry().value(widget, buttonPressGenerationProperty).toULongLong()
                + 1;
        framePropertyRegistry().set(widget, buttonPressGenerationProperty,
                                    QVariant::fromValue(generation));
        framePropertyRegistry().set(widget, buttonPressReleasePendingProperty, true);
        const QPointer<QWidget> guardedWidget(widget);
        QTimer::singleShot(16, q, [this, guardedWidget, generation] {
            if (!guardedWidget
                || framePropertyRegistry()
                                .value(guardedWidget, buttonPressGenerationProperty)
                                .toULongLong()
                        != generation
                || !framePropertyRegistry()
                            .value(guardedWidget, buttonPressReleasePendingProperty)
                            .toBool()) {
                return;
            }
            framePropertyRegistry().set(guardedWidget, buttonPressReleasePendingProperty, false);
            animate(guardedWidget, pressProperty, 0.0, Private::FasterDuration);
        });
    }

    void cancelButtonPress(QWidget *widget)
    {
        if (!widget)
            return;
        const qulonglong generation =
                framePropertyRegistry().value(widget, buttonPressGenerationProperty).toULongLong()
                + 1;
        framePropertyRegistry().set(widget, buttonPressGenerationProperty,
                                    QVariant::fromValue(generation));
        framePropertyRegistry().set(widget, buttonPressReleasePendingProperty, false);
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
        animationDriver.animate(widget, comboChevronProperty, 0.0, 300, true, fluentCurve(),
                                { { 0.28, QVariant(-0.4) } }, start);
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
        comboPopupPopupConnections.insert(
                popup, QObject::connect(popup, &QObject::destroyed, q, [this, popup] {
                    unregisterComboPopup(popup);
                }));
        comboPopupComboConnections.insert(
                combo, QObject::connect(combo, &QObject::destroyed, q, [this, combo] {
                    unregisterComboPopup(combo);
                }));
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

    bool tableEditorOverlaps(const QTableView *table, const QModelIndex &index,
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
    // Tracked destroyed connections for the parent-owned timers above, so
    // repeated polish/unpolish can disconnect instead of accumulating.
    QHash<QScrollBar *, QMetaObject::Connection> scrollBarDestroyConnections;
    QHash<QSlider *, QMetaObject::Connection> sliderToolTipDestroyConnections;
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
    QFont lastStyleApplicationFont;
    QPalette originalApplicationPalette;
    QPalette lastStyleApplicationPalette;
    QPalette originalToolTipPalette;
    QPalette lastStyleToolTipPalette;
    std::unique_ptr<Private::StyleInteractionController> interactionController;
};

Style::Style(ThemeMode mode) : Style(mode, WinUI3::DensityMode::Standard) { }

Style::Style(WinUI3::DensityMode density) : Style(ThemeMode::System, density) { }

Style::Style(ThemeMode mode, WinUI3::DensityMode density) : Style(nullptr, mode, density) { }

Style::Style(QStyle *base, ThemeMode mode, WinUI3::DensityMode density)
    : QProxyStyle(base ? base : new QCommonStyle),
      d(std::make_unique<StylePrivate>(this, mode, density))
{
    setObjectName(QStringLiteral("winui3"));
    d->progressTimer = new QTimer(this);
    d->progressTimer->setObjectName(QStringLiteral("_winui_progress_timer"));
    d->progressTimer->setInterval(16);
    connect(d->progressTimer, &QTimer::timeout, this, [this] { d->advanceProgressBars(); });
    d->systemAppearanceWatchdog = new QTimer(this);
    d->systemAppearanceWatchdog->setObjectName(QStringLiteral("_winui_system_appearance_watchdog"));
    d->systemAppearanceWatchdog->setInterval(15000);
    connect(d->systemAppearanceWatchdog, &QTimer::timeout, this, &Style::checkSystemAppearance);
    d->systemAppearanceWatcher =
            new SystemAppearanceWatcher(this, [this] { checkSystemAppearance(); });
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
    for (const QWidget *candidate = widget; candidate; candidate = candidate->parentWidget()) {
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

namespace {
bool g_altMnemonicsVisible = false;

// GUI-thread-only contract: the flag is written by setAltMnemonicsVisible()
// (Alt key tracking from the style's event filter / application code) and
// read from SH_UnderlineShortcut paint paths. Both run on the GUI thread,
// and the setter below touches top-level widgets, which is itself
// GUI-thread-only. The assert documents and enforces this; a plain bool is
// sufficient because no cross-thread access is permitted.
void assertGuiThread(const char *operation)
{
    Q_ASSERT_X(!QCoreApplication::instance()
                       || QThread::currentThread() == QCoreApplication::instance()->thread(),
               operation, "must be called on the GUI thread");
    Q_UNUSED(operation);
}
} // namespace

bool Style::altMnemonicsVisible()
{
    assertGuiThread("Style::altMnemonicsVisible");
    return g_altMnemonicsVisible;
}

void Style::setAltMnemonicsVisible(bool visible)
{
    assertGuiThread("Style::setAltMnemonicsVisible");
    if (g_altMnemonicsVisible == visible)
        return;
    g_altMnemonicsVisible = visible;
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        if (auto *menuBar = qobject_cast<QMenuBar *>(widget))
            menuBar->update();
        else if (widget->isWindow())
            widget->update();
    }
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
    if (!d->applicationStyleActive && d->paletteOwners.isEmpty())
        return;
    const bool applicationPaletteStillOwned = qApp->palette() == d->lastStyleApplicationPalette;
    const QPalette applicationPalette =
            applicationPaletteStillOwned ? standardPalette() : qApp->palette();
    const bool toolTipStillOwned = QToolTip::palette() == d->lastStyleToolTipPalette;
    if (applicationPaletteStillOwned) {
        qApp->setPalette(applicationPalette);
        d->lastStyleApplicationPalette = applicationPalette;
    }
    if (toolTipStillOwned) {
        QToolTip::setPalette(applicationPalette);
        d->lastStyleToolTipPalette = applicationPalette;
    }
    // A popup's view and viewport are often created after their combo box was
    // polished. Prepare and register them now that the popup exists, before
    // walking the bounded owner registry below.
    for (QWidget *window : qApp->topLevelWidgets()) {
        const QVariant backdrop = window->property("_winui_backdrop");
        if (backdrop.isValid()) {
            // The shot fires a full event-loop turn later: revalidate the
            // request then, or a backdrop disabled in between would be
            // re-enabled behind the application's back (stale re-apply).
            const QPointer<QWidget> guarded = window;
            const int requested = backdrop.toInt();
            QTimer::singleShot(0, window, [guarded, requested] {
                if (!guarded || !guarded->property("_winui_backdrop").isValid()
                    || guarded->property("_winui_backdrop").toInt() != requested)
                    return;
                applyBackdrop(guarded.data(), static_cast<Backdrop>(requested));
            });
        } else if (auto *wizard = qobject_cast<QWizard *>(window)) {
            refreshWizardSurface(wizard, applicationPalette);
            const QPointer<QWizard> guardedWizard(wizard);
            const QPointer<Style> guardedStyle(this);
            QTimer::singleShot(0, wizard, [guardedWizard, guardedStyle] {
                if (guardedWizard && guardedStyle)
                    refreshWizardSurface(guardedWizard, guardedStyle->standardPalette());
            });
        } else if (qobject_cast<QDialog *>(window)
                   || window->property(Private::effectiveBackdropProperty).isValid()) {
            // Disabling a material restores opaque content, but its native
            // caption remains style-owned and must follow later theme changes.
            applyDialogCaptionTheme(window);
        }
        if (window->windowType() == Qt::Popup) {
            preparePopupSurface(window);
            d->registerPopupPaletteOwners(window);
        }
    }
    d->prunePaletteOwners();
    // Backdrop islands transparentized by syncContentSurfacesForBackdrop carry
    // no palette-owner registration (only polish-registered widgets are
    // visited below), so a Light->Dark switch leaves their stale fill behind
    // as a ghost. Rebase every opted-in island's Window role on the new
    // application palette here, preserving its current alpha: opaque islands
    // follow the theme, transparent live-material islands keep revealing DWM
    // with fresh RGB. Toggle-off still restores the remembered state.
    // A backdrop toggle re-runs this same per-window pass through
    // refreshOwnedPalettes() so the inline calendar surface (computed at
    // polish) converges onto the granted material instead of staying opaque.
    for (QWidget *window : qApp->topLevelWidgets())
        refreshOwnedPalettes(window);
    // The popup may be newly created and not yet have delivered its first
    // Show event. Refresh visible popup children as well as the window itself.
    for (QWidget *topLevel : qApp->topLevelWidgets()) {
        if (topLevel->windowType() == Qt::Popup && topLevel->isVisible()) {
            topLevel->update();
            if (auto *view = topLevel->findChild<QAbstractItemView *>()) {
                view->update();
                if (view->viewport())
                    view->viewport()->update();
            }
        }
    }
}

void Style::refreshOwnedPalettes(QWidget *window)
{
    if (!window)
        return;
    d->prunePaletteOwners();
    const QPalette applicationPalette = qApp ? qApp->palette() : standardPalette();
    const Private::Tokens applicationTokens = Private::tokens(applicationPalette);
    const QColor applicationAccent = accentColor();
    const bool darkTheme = d->dark();
    rebaseBackdropIslands(window, applicationPalette);
    // Calendar header ordering: the owner loop visits widgets in
    // registration order, and lane children are polished (registered)
    // before the bar. In a single pass the lane is therefore computed
    // from the pre-refresh (stale-theme) bar. Phase 1 rebases every
    // surface except the lane; phase 2 then reads the post-phase-1 bar,
    // so calendar -> bar -> lane converges in one pass. Phase-skip
    // predicates match insideCalendarNavigationBar exactly.
    // PaletteChange handlers can polish or delete children synchronously,
    // mutating the registry. Keep a stable, guarded snapshot for both phases.
    const auto paletteOwners = d->paletteOwners;
    for (const QPointer<QWidget> &guarded : paletteOwners)
        refreshOwnedPalette(guarded.data(), window, applicationPalette, applicationTokens,
                            applicationAccent, darkTheme);
    refreshCalendarNavigationLanes(window, applicationPalette, paletteOwners);
    // No corrective re-sync here by design. The passes above preserve the
    // live recipe (alpha-preserving owner rebase and island rebase), and
    // newly arrived surfaces converge at their own lifecycle points: polish,
    // reparent (ParentChange) and SurfaceProperty opt-in arm them, while
    // applyBackdrop owns the enable transition.
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
        widget->setProperty(originalRoleWasValidProperty, widget->property(roleProperty).isValid());
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

    for (const QWidget *candidate = widget; candidate; candidate = candidate->parentWidget()) {
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
        if (const auto *button = qobject_cast<const QPushButton *>(widget)) {
            if (button->isFlat())
                return ControlRole::Subtle;
            if (button->isDefault())
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

void Style::setToggleSwitchText(QCheckBox *checkBox, const QString &onText, const QString &offText)
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

void Style::drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter,
                          const QWidget *widget) const
{
    if (Private::drawMenuPrimitive(this, element, option, painter, widget))
        return;
    if (Private::drawButtonPrimitive(this, element, option, painter, widget))
        return;

    if (Private::drawViewPrimitive(this, element, option, painter, widget))
        return;

    if (element == PE_Widget && widget
        && Private::drawWidgetBackdropPrimitive(this, option, painter, widget))
        return;
    if (Private::drawChromePrimitive(this, element, option, painter, widget))
        return;

    Q_ASSERT_X(!coveredPrimitive(element), "WinUI3::Style::drawPrimitive",
               "a covered primitive reached QCommonStyle");
    QProxyStyle::drawPrimitive(element, option, painter, widget);
}

void Style::drawControl(ControlElement element, const QStyleOption *option, QPainter *painter,
                        const QWidget *widget) const
{
    if (Private::drawMenuControl(this, element, option, painter, widget))
        return;
    if (Private::drawButtonControl(this, element, option, painter, widget))
        return;

    Private::eraseButtonFamilyBackdrop(element, option, painter, widget);

    if (Private::drawViewControl(
                this, element, option, painter, widget,
                [this](const QTableView *table, const QModelIndex &index, const QRect &rect) {
                    return d->tableEditorOverlaps(table, index, rect);
                })) {
        return;
    }

    if (Private::drawChromeControl(this, element, option, painter, widget))
        return;

    Q_ASSERT_X(!coveredControl(element), "WinUI3::Style::drawControl",
               "a covered control reached QCommonStyle");
    QProxyStyle::drawControl(element, option, painter, widget);
}

void Style::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                               QPainter *painter, const QWidget *widget) const
{
    if (Private::drawComplexControl(this, control, option, painter, widget))
        return;
    // Uncovered complex controls keep the shared pre-erase, then fall through
    // to the covered guard and the base style.
    Private::eraseComplexBackdrop(control, option, painter, widget);

    Q_ASSERT_X(!Private::coveredComplex(control), "WinUI3::Style::drawComplexControl",
               "a covered complex control reached QCommonStyle");
    QProxyStyle::drawComplexControl(control, option, painter, widget);
}

int Style::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
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

QRect Style::subControlRect(ComplexControl control, const QStyleOptionComplex *option,
                            SubControl subControl, const QWidget *widget) const
{
    return Private::subControlRect(this, control, option, subControl, widget);
}
QStyle::SubControl Style::hitTestComplexControl(ComplexControl control,
                                                const QStyleOptionComplex *option,
                                                const QPoint &position, const QWidget *widget) const
{
    if (const auto result = Private::complexControlHitTest(control, option, position, widget))
        return *result;
    return QProxyStyle::hitTestComplexControl(control, option, position, widget);
}
int Style::styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget,
                     QStyleHintReturn *returnData) const
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
        d->originalToolTipPalette = QToolTip::palette();
        d->applicationStateSaved = true;
    }
    QByteArray overrideFamily = qgetenv("WINUI3STYLE_APP_FONT");
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // Qt 6 renders the Windows 11 variable font natively.
    QString preferred = overrideFamily.isEmpty() ? QStringLiteral("Segoe UI Variable Text")
                                                 : QString::fromLocal8Bit(overrideFamily);
    if (!QFontDatabase::families().contains(preferred))
        preferred = QStringLiteral("Segoe UI");
#else
    // Qt 5.x's DirectWrite path can rasterize the variable-font outlines
    // with touching/overlapping glyphs ("o"+"a" looking glued).  Use the
    // static family unless the app opts in via WINUI3STYLE_APP_FONT.
    QString preferred = overrideFamily.isEmpty() ? QStringLiteral("Segoe UI")
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
    d->lastStyleApplicationFont = font;
    const QPalette stylePalette = standardPalette();
    application->setPalette(stylePalette);
    d->lastStyleApplicationPalette = stylePalette;
    QToolTip::setPalette(stylePalette);
    d->lastStyleToolTipPalette = stylePalette;
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
    remember(widget, originalHoverAttributeProperty, widget->testAttribute(Qt::WA_Hover));
    remember(widget, originalRoleProperty, widget->property(roleProperty));
    widget->setAttribute(Qt::WA_Hover, true);
    // Idempotent install: polish can run again on an already-polished widget
    // (repolish paths), so remove first to avoid a duplicate filter entry.
    // unpolish(QWidget*) removes it symmetrically.
    widget->removeEventFilter(this);
    widget->installEventFilter(this);
    // Any widget without a user-set palette belongs to the style's palette
    // contract: a later theme/accent change must rebase it on the new
    // standardPalette, exactly like the surface islands below. Without this
    // only explicitly registered owners (islands, popups, chrome) follow the
    // theme, and a plain QLineEdit keeps its light factory palette inside a
    // dark window (white "Search settings" field). The explicit-palette
    // guard in the refresh loop still protects user-set colors, and unpolish
    // still restores the remembered original, so this only widens the set of
    // widgets the refresh visits, never what it writes to custom palettes.
    if (!widget->testAttribute(Qt::WA_SetPalette)) {
        widget->setProperty(ownedPaletteProperty, true);
        d->registerPaletteOwner(widget);
    }
    polishSurfacePalette(widget);
    polishWizard(widget);
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
                                widget->isEnabled() && widget->underMouse() ? 1.0 : 0.0);
    framePropertyRegistry().set(widget, pressProperty, 0.0);
    framePropertyRegistry().set(widget, focusProperty, widget->hasFocus() ? 1.0 : 0.0);
    framePropertyRegistry().set(widget, focusVisibleProperty,
                                widget->hasFocus() && d->keyboardInput);
    polishDensityWidget(widget);
    if (qobject_cast<QScrollBar *>(widget)) {
        framePropertyRegistry().set(widget, scrollBarInsideProperty, widget->underMouse());
        framePropertyRegistry().set(widget, scrollBarGenerationProperty, 0);
    }
    polishCheckableWidget(widget);

    polishProgressBar(widget);
    polishNavigationView(widget);

    polishTableViewSelectionPalette(widget);
    polishTableEditor(widget);

    if (auto *toolButton = qobject_cast<QAbstractButton *>(widget)) {
        // The calendar navigation-bar lane paints Subtle flat at rest over
        // the now-translucent tinted bar (see the lane-children block
        // below): no erase can punch a hole because the bar beneath is the
        // same tint, not an opaque slab.
        if (qobject_cast<QToolBar *>(toolButton->parentWidget())
            || qobject_cast<QTabBar *>(toolButton->parentWidget()))
            setControlRole(toolButton, ControlRole::Subtle);
    }

    polishCalendarWidget(widget);

    if (auto *combo = qobject_cast<QComboBox *>(widget)) {
        framePropertyRegistry().set(widget, comboChevronProperty, 0.0);
        // Install DropdownContentMargin before Qt negotiates popup geometry.
        // Mutating it during Show clips rows and requires a second resize.
        QWidget *popup = combo->view()->window();
        remember(popup, originalMarginsProperty, QVariant::fromValue(popup->contentsMargins()));
        popup->setContentsMargins(0, 4, 0, 4);
    }

    polishMenuMargins(widget);

    polishContentDialog(widget);
    if (!widget->testAttribute(Qt::WA_SetPalette) && widget->property(ownedPaletteProperty).toBool()
        && widget->window()->windowType() != Qt::Popup && !qobject_cast<QWizard *>(widget->window())
        && !insideCalendarWidget(widget) && !widget->property(BackdropProperty).isValid())
        widget->setProperty(implicitPaletteSnapshotProperty,
                            QVariant::fromValue(widget->palette()));
    // A widget polished while its window already presents a live material
    // joins that recipe now instead of waiting for the next theme refresh.
    Private::armPolishedSurfaceForBackdrop(widget);
}

void Style::polishTableEditor(QWidget *widget)
{
    if (auto *editor = qobject_cast<QLineEdit *>(widget);
        editor && qobject_cast<const QTableView *>(itemView(editor))) {
        widget->setProperty(ownedPaletteProperty, true);
        d->registerPaletteOwner(widget);
        QPalette palette = editor->palette();
        const Private::Tokens editorTokens = Private::tokens(standardPalette());
        palette.setColor(QPalette::Highlight, accentColor());
        palette.setColor(QPalette::HighlightedText, editorTokens.textOnAccentPrimary);
        editor->setPalette(palette);
    }

    if (auto *view = qobject_cast<QTableView *>(const_cast<QAbstractItemView *>(itemView(widget)));
        view && widget->parentWidget() == view->viewport()) {
        // Editors are children of the viewport and are polished after the
        // delegate creates them. Track that lifecycle in the style so item
        // painting can suppress display text even when Qt omits
        // State_Editing from the real delegate option.
        d->trackTableEditor(view, widget);
    }
}

void Style::polishTableViewSelectionPalette(QWidget *widget)
{
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
        d->tableConnections.insert(
                table,
                connect(this, &Style::themeChanged, table,
                        [applyTableSelectionPalette](ThemeMode) { applyTableSelectionPalette(); }));
    }
}

void Style::polishNavigationView(QWidget *widget)
{
    if (auto *view = qobject_cast<QAbstractItemView *>(widget)) {
        d->registerPaletteOwner(view);
        d->registerPaletteOwner(view->viewport());
        NavigationPrivate::prepareNavigationView(view);
    }
}

void Style::polishProgressBar(QWidget *widget)
{
    if (auto *progressBar = qobject_cast<QProgressBar *>(widget)) {
        d->registerProgressBar(progressBar);
        framePropertyRegistry().set(progressBar, progressPhaseProperty,
                                    Style::animationsAllowed() ? 0.0 : 0.35);
        d->refreshProgressTimer();
    }
}

void Style::polishDensityWidget(QWidget *widget)
{
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
    } else if (auto *spinBox = qobject_cast<QAbstractSpinBox *>(widget)) {
        if (effectiveDensityMode(spinBox) == DensityMode::Compact) {
            QEvent styleChange(QEvent::StyleChange);
            QCoreApplication::sendEvent(spinBox, &styleChange);
            spinBox->updateGeometry();
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
}

void Style::polishSurfacePalette(QWidget *widget)
{
    const QVariant surface = widget->property(SurfaceProperty);
    applyExplicitSurfacePalette(widget);

    // QDialogButtonBox is a layout container, not a command-surface panel.
    // Filling its own inset geometry creates a rectangular footer inside a
    // ContentDialog. It must inherit the dialog surface instead. QWizard has
    // a separate full-width footer surface installed by refreshWizardSurface.
    const bool automaticCommandSurface = qobject_cast<QStatusBar *>(widget);
    const bool automaticDialogContent = qobject_cast<QWizardPage *>(widget);
    if (!surface.isValid() && (automaticCommandSurface || automaticDialogContent)) {
        widget->setProperty(ownedPaletteProperty, true);
        d->registerPaletteOwner(widget);
        QPalette palette = standardPalette();
        if (automaticCommandSurface)
            palette.setColor(QPalette::Window, Private::popupSurfaceColor(palette));
        else if (automaticDialogContent)
            palette.setColor(QPalette::Window, Private::popupSurfaceColor(palette));
        widget->setPalette(palette);
        widget->setAutoFillBackground(true);
    }
}

void Style::applyExplicitSurfacePalette(QWidget *widget)
{
    const QVariant surface = widget->property(SurfaceProperty);
    const QString surfaceName = surface.toString();
    if (!isContentLayerSurface(surface, surfaceName))
        return;
    widget->setProperty(implicitPaletteSnapshotProperty, {});

    // A native backdrop makes the top-level Window role transparent.
    // Standard stacked/page widgets otherwise retain stale backing-store
    // pixels while scrolling or switching pages. An explicit content
    // layer is the Qt equivalent of WinUI's opaque content surface.
    widget->setProperty(ownedPaletteProperty, true);
    d->registerPaletteOwner(widget);
    remember(widget, originalOpaquePaintProperty, widget->testAttribute(Qt::WA_OpaquePaintEvent));
    QPalette palette = standardPalette();
    if (surfaceName.compare(QLatin1String("layer"), Qt::CaseInsensitive) == 0) {
        const QColor layer = Private::popupSurfaceColor(palette);
        palette.setColor(QPalette::Window, layer);
        if (qobject_cast<QAbstractItemView *>(widget))
            palette.setColor(QPalette::Base, layer);
    }
    widget->setPalette(palette);
    widget->setAutoFillBackground(true);
    widget->setAttribute(Qt::WA_OpaquePaintEvent, true);
}

void Style::polishCheckableWidget(QWidget *widget)
{
    if (auto *checkBox = qobject_cast<QCheckBox *>(widget)) {
        if (const auto previous = d->toggleConnections.take(widget))
            disconnect(previous);
        framePropertyRegistry().set(widget, checkProperty,
                                    checkBox->checkState() == Qt::Unchecked ? 0.0 : 1.0);
        framePropertyRegistry().set(widget, togglePositionProperty,
                                    checkBox->isChecked() ? 1.0 : 0.0);
        d->toggleConnections.insert(
                widget,
                connect(checkBox,
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
                        &QCheckBox::checkStateChanged,
#else
                        &QCheckBox::stateChanged,
#endif
                        this, [this, checkBox](int state) {
                            const bool on = state != Qt::Unchecked;
                            // AnimatedAcceptVisualSource's NormalOnToNormalOff segment
                            // removes the stroke immediately. Only the acceptance path
                            // is animated; an on-transition remains interruptible by
                            // starting from its current progress.
                            d->animate(checkBox, checkProperty, on ? 1.0 : 0.0,
                                       on ? (toggleSwitch(checkBox) ? Private::FastDuration
                                                                    : Private::CheckBoxDuration)
                                          : 0);
                            if (toggleSwitch(checkBox))
                                d->animate(checkBox, togglePositionProperty, on ? 1.0 : 0.0,
                                           Private::FasterDuration);
                        }));
    } else if (auto *radio = qobject_cast<QRadioButton *>(widget)) {
        if (const auto previous = d->radioConnections.take(radio))
            disconnect(previous);
        framePropertyRegistry().set(widget, checkProperty, radio->isChecked() ? 1.0 : 0.0);
        d->radioConnections.insert(
                radio, connect(radio, &QAbstractButton::toggled, this, [this, radio](bool checked) {
                    // RadioButton checked-state switch is discrete: only the dot
                    // hover/press sizes animate (Normal 250ms via pressProperty).
                    // Duration 0 lands instantly from current progress, so rapid
                    // reversals track state exactly like checkbox uncheck.
                    d->animate(radio, checkProperty, checked ? 1.0 : 0.0, 0);
                }));
    } else if (auto *groupBox = qobject_cast<QGroupBox *>(widget);
               groupBox && groupBox->isCheckable()) {
        if (const auto previous = d->toggleConnections.take(widget))
            disconnect(previous);
        framePropertyRegistry().set(widget, checkProperty, groupBox->isChecked() ? 1.0 : 0.0);
        d->toggleConnections.insert(
                widget,
                connect(groupBox, &QGroupBox::toggled, this, [this, groupBox](bool checked) {
                    d->animate(groupBox, checkProperty, checked ? 1.0 : 0.0,
                               checked ? Private::CheckBoxDuration : 0);
                }));
    }
}

void Style::polishCalendarWidget(QWidget *widget)
{
    // Qt hard-codes QCalendarWidget's navigation bar to the Highlight role,
    // producing an accent-blue strip unrelated to WinUI's CalendarView. Keep
    // the native calendar implementation but place its navigation controls on
    // the same neutral popup surface as the day grid.
    if (widget->objectName() == QStringLiteral("qt_calendar_navigationbar")
        && qobject_cast<QCalendarWidget *>(widget->parentWidget())) {
        widget->setBackgroundRole(QPalette::Window);
        // Uniform acrylic: popup calendars carry the composited popup tint
        // (same popupSurfaceColor RGB + 178/242 alpha preparePopupSurface
        // paints); inline calendars keep the opaque content surface. Claim
        // the palette so the appearance refresh keeps the same contract.
        widget->setProperty(ownedPaletteProperty, true);
        d->registerPaletteOwner(widget);
        const QPalette navigationSource = standardPalette();
        const bool popupHeader = widget->window() && widget->window()->windowType() == Qt::Popup;
        QColor navigationWindow;
        if (popupHeader) {
            navigationWindow = Private::popupSurfaceColor(navigationSource);
            if (Private::backdropEffectiveSurface(widget->window())
                == Private::BackdropSurface::Composited)
                navigationWindow = widget->window()->palette().color(QPalette::Window);
        } else {
            navigationWindow = inlineCalendarSurface(widget, navigationSource);
        }
        QPalette navigationPalette = navigationSource;
        navigationPalette.setColor(QPalette::Window, navigationWindow);
        widget->setPalette(navigationPalette);
        if (!popupHeader) {
            // Route the inline bar background through the PE_Widget branch
            // above so the granted-backdrop empty stretch Source-composites
            // exactly once; no Qt auto-fill pre-blend. Popups keep their
            // existing attributes.
            widget->setAutoFillBackground(false);
            widget->setAttribute(Qt::WA_StyledBackground, true);
        }
        widget->update();
    }

    // The lane children (month/year/prev/next buttons, year editor) default
    // to the Button background role with the application palette, while the
    // bar paints its empty stretches from its own header fill. The lane
    // reads as two backgrounds unless every child shares the bar's fill
    // exactly, so claim them the same way and keep them there across theme
    // refreshes. Lane buttons are Subtle flat at rest on popup calendars
    // (same tint beneath, no erase hole); inline lane buttons stay Standard
    // with the opaque bar (a Subtle rest over an opaque slab would read as
    // a missing fill, and the erase gate is closed there anyway).
    if (insideCalendarNavigationBar(widget)) {
        const bool popupCalendar = widget->window() && widget->window()->windowType() == Qt::Popup;
        if (auto *laneButton = qobject_cast<QAbstractButton *>(widget))
            setControlRole(laneButton, ControlRole::Subtle);
        widget->setBackgroundRole(QPalette::Window);
        widget->setProperty(ownedPaletteProperty, true);
        d->registerPaletteOwner(widget);
        QPalette lanePalette = standardPalette();
        // The bar's contract is the header fill derived from the application
        // palette: popup tint on popup calendars, opaque Window surface
        // inline. Never the factory Highlight a freshly-created bar may
        // still carry when a lane child is polished first.
        QColor laneWindow;
        if (popupCalendar) {
            laneWindow = Private::popupSurfaceColor(lanePalette);
            if (Private::backdropEffectiveSurface(widget->window())
                == Private::BackdropSurface::Composited)
                laneWindow = widget->window()->palette().color(QPalette::Window);
        } else {
            laneWindow = inlineCalendarSurface(widget, lanePalette);
        }
        lanePalette.setColor(QPalette::Window, laneWindow);
        lanePalette.setColor(QPalette::Button, laneWindow);
        if (qobject_cast<QAbstractSpinBox *>(widget))
            lanePalette.setColor(QPalette::Base, laneWindow);
        widget->setPalette(lanePalette);
        widget->update();
    }

    // The day grid qt_calendar_calendarview is itself a QTableView, so the
    // generic table branch above claimed it with a subtleHover Highlight.
    // That leaves a native full-cell rect behind the CalendarView accent
    // circle, and an un-rebased inline grid keeps the app Highlight blue.
    // The day chrome itself (circle-select, text/disabled/outside-month
    // colors) is already shared for popup and inline grids by the calendar
    // branches in winui3viewrenderers_p.cpp; only the palette is
    // neutralized here. Keep Base on the content surface (no flyout tint
    // inline) and kill just the native rect/strip. Single tableConnections
    // slot per widget: taking the generic entry first keeps the symmetry
    // gate, and generic unpolish (remembered-palette restore +
    // tableConnections.take) covers both palette and connection.
    if (auto *calendarGrid = qobject_cast<QTableView *>(widget);
        calendarGrid && insideCalendarWidget(widget)) {
        if (const auto previous = d->tableConnections.take(widget))
            disconnect(previous);
        const auto applyCalendarSelectionPalette = [this, calendarGrid] {
            QPalette palette = calendarGrid->palette();
            const Private::Tokens calendarTokens = Private::tokens(standardPalette());
            palette.setColor(QPalette::Highlight, Qt::transparent);
            palette.setColor(QPalette::HighlightedText, calendarTokens.textPrimary);
            calendarGrid->setPalette(palette);
        };
        applyCalendarSelectionPalette();
        d->tableConnections.insert(widget,
                                   connect(this, &Style::themeChanged, widget,
                                           [applyCalendarSelectionPalette](ThemeMode) {
                                               applyCalendarSelectionPalette();
                                           }));
    }

    // The native selection rect reads the viewport option palette, so the
    // grid's viewport neutralizes itself in its own polish. Each widget's
    // rememberPalette runs in its own polish, so generic unpolish restores
    // the exact remembered state; no poke-across from the grid's polish.
    if (auto *calendarTable = qobject_cast<QTableView *>(widget->parentWidget()); calendarTable
        && widget == calendarTable->viewport() && insideCalendarWidget(calendarTable)) {
        QPalette viewportPalette = widget->palette();
        const Private::Tokens viewportTokens = Private::tokens(standardPalette());
        viewportPalette.setColor(QPalette::Highlight, Qt::transparent);
        viewportPalette.setColor(QPalette::HighlightedText, viewportTokens.textPrimary);
        widget->setPalette(viewportPalette);
    }

    if (insideCalendarWidget(widget) && widget->window()->windowType() != Qt::Popup) {
        const QColor calendarSurface = inlineCalendarSurface(widget, standardPalette());
        QPalette palette = widget->palette();
        palette.setColor(QPalette::Window, calendarSurface);
        palette.setColor(QPalette::Base, calendarSurface);
        widget->setProperty(ownedPaletteProperty, true);
        d->registerPaletteOwner(widget);
        widget->setPalette(palette);
    }
}

void Style::polishMenuMargins(QWidget *widget)
{
    auto *menu = qobject_cast<QMenu *>(widget);
    if (!menu)
        return;

    // QMenu computes its first popup geometry after polish but before Show.
    // Install the layout inset here; the opaque palette is refreshed on Show.
    remember(menu, originalMarginsProperty, QVariant::fromValue(menu->contentsMargins()));
    menu->setContentsMargins(0, 2, 0, 2);
}

void Style::polishWizard(QWidget *widget)
{
    auto *wizard = qobject_cast<QWizard *>(widget);
    if (!wizard)
        return;

    // Keep the wizard in the style-owned palette registry so a later
    // theme change refreshes its internal page and command surfaces too.
    widget->setProperty(ownedPaletteProperty, true);
    d->registerPaletteOwner(widget);
    for (QWizard::WizardButton button : { QWizard::NextButton, QWizard::FinishButton }) {
        if (QAbstractButton *primary = wizard->button(button))
            primary->setProperty(ControlRoleProperty, QStringLiteral("accent"));
    }
    refreshWizardSurface(wizard, standardPalette());
}

void Style::polishContentDialog(QWidget *widget)
{
    auto *dialog = qobject_cast<QDialog *>(widget);
    if (!dialog
        || (!qobject_cast<QMessageBox *>(dialog)
            && !dialog->property(ContentDialogProperty).toBool()))
        return;

    prepareContentDialogState(dialog, d->dark());
    d->registerPaletteOwner(dialog);
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
        const bool toolTipStillOwned = QToolTip::palette() == d->lastStyleToolTipPalette;
        if (application->font() == d->lastStyleApplicationFont)
            application->setFont(d->originalApplicationFont);
        if (application->palette() == d->lastStyleApplicationPalette)
            application->setPalette(d->originalApplicationPalette);
        if (toolTipStillOwned)
            QToolTip::setPalette(d->originalToolTipPalette);
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
        if (hasRuntimePaletteOverride(widget)) {
            widget->setProperty(originalPaletteProperty, QVariant::fromValue(widget->palette()));
            widget->setProperty(originalPaletteExplicitProperty, true);
        }
        restoreRememberedPalette(widget);
        if (widget->property(originalAutoFillProperty).isValid())
            widget->setAutoFillBackground(widget->property(originalAutoFillProperty).toBool());
        if (widget->property(originalHoverAttributeProperty).isValid())
            widget->setAttribute(Qt::WA_Hover,
                                 widget->property(originalHoverAttributeProperty).toBool());
        if (widget->property(originalOpaquePaintProperty).isValid())
            widget->setAttribute(Qt::WA_OpaquePaintEvent,
                                 widget->property(originalOpaquePaintProperty).toBool());
        if (widget->property(originalTranslucentBackgroundProperty).isValid())
            widget->setAttribute(Qt::WA_TranslucentBackground,
                                 widget->property(originalTranslucentBackgroundProperty).toBool());
        if (widget->property(originalNoSystemBackgroundProperty).isValid())
            widget->setAttribute(Qt::WA_NoSystemBackground,
                                 widget->property(originalNoSystemBackgroundProperty).toBool());
        if (widget->property(originalMarginsProperty).isValid()
            && !qobject_cast<QDialog *>(widget)) {
            widget->setContentsMargins(widget->property(originalMarginsProperty).value<QMargins>());
        }
        if (auto *list = qobject_cast<QListView *>(widget))
            if (widget->property(originalListSpacingProperty).isValid())
                list->setSpacing(widget->property(originalListSpacingProperty).toInt());
        unpolishWizard(widget);
        unpolishContentDialog(widget);
        if (auto *frame = qobject_cast<QFrame *>(widget))
            if (widget->property(originalFrameShapeProperty).isValid())
                frame->setFrameShape(static_cast<QFrame::Shape>(
                        widget->property(originalFrameShapeProperty).toInt()));
        if (widget->property(originalRoleWasValidProperty).isValid()) {
            if (widget->property(originalRoleWasValidProperty).toBool())
                widget->setProperty(roleProperty, widget->property(originalRoleProperty));
            else
                widget->setProperty(roleProperty, {});
        }
        unpolishNavigationView(widget);
        unpolishTableEditor(widget);
        unpolishCheckableWidget(widget);
        unpolishTableViewSelectionPalette(widget);
        if (auto *checkBox = qobject_cast<QCheckBox *>(widget))
            d->toggleDragStates.remove(checkBox);
        if (auto *slider = qobject_cast<QSlider *>(widget))
            d->unregisterSlider(slider);
        unpolishProgressBar(widget);
        if (auto *scrollBar = qobject_cast<QScrollBar *>(widget)) {
            d->cancelScrollBarTimer(scrollBar);
            d->unregisterScrollBar(scrollBar);
        }
        if (auto *slider = qobject_cast<QSlider *>(widget))
            hideSliderValueToolTip(slider);
        framePropertyRegistry().clearObject(widget);
        widget->setProperty(ownedPaletteProperty, {});
        widget->setProperty(implicitPaletteSnapshotProperty, {});
        widget->setProperty(originalPaletteProperty, {});
        widget->setProperty(originalPaletteExplicitProperty, {});
        widget->setProperty(originalAutoFillProperty, {});
        widget->setProperty(originalHoverAttributeProperty, {});
        widget->setProperty(originalOpaquePaintProperty, {});
        widget->setProperty(originalTranslucentBackgroundProperty, {});
        widget->setProperty(originalNoSystemBackgroundProperty, {});
        widget->setProperty(originalListSpacingProperty, {});
        widget->setProperty(originalMinimumSizeProperty, {});
        widget->setProperty(originalMaximumSizeProperty, {});
        widget->setProperty(originalLayoutConstraintProperty, {});
        widget->setProperty(originalFrameShapeProperty, {});
        widget->setProperty(originalMarginsProperty, {});
        widget->setProperty(originalSpacingProperty, {});
        widget->setProperty(originalRoleProperty, {});
        widget->setProperty(originalRoleWasValidProperty, {});
    }
}

void Style::unpolishWizard(QWidget *widget)
{
    if (auto *wizard = qobject_cast<QWizard *>(widget))
        deleteWizardFooterSurface(wizard);
}

void Style::unpolishContentDialog(QWidget *widget)
{
    if (auto *dialog = qobject_cast<QDialog *>(widget))
        restoreContentDialogState(dialog, false);
}

void Style::unpolishTableEditor(QWidget *widget)
{
    if (auto *table = qobject_cast<QTableView *>(widget))
        d->untrackTable(table);
    else if (qobject_cast<const QTableView *>(itemView(widget)))
        d->untrackTableEditor(widget);
}

void Style::unpolishTableViewSelectionPalette(QWidget *widget)
{
    if (const auto connection = d->tableConnections.take(widget))
        disconnect(connection);
}

void Style::unpolishNavigationView(QWidget *widget)
{
    if (auto *view = qobject_cast<QAbstractItemView *>(widget))
        NavigationPrivate::restoreNavigationView(view);
}

void Style::unpolishProgressBar(QWidget *widget)
{
    if (auto *progressBar = qobject_cast<QProgressBar *>(widget))
        d->unregisterProgressBar(progressBar);
}

void Style::unpolishCheckableWidget(QWidget *widget)
{
    if (const auto connection = d->toggleConnections.take(widget))
        disconnect(connection);
    if (auto *radio = qobject_cast<QRadioButton *>(widget))
        if (const auto connection = d->radioConnections.take(radio))
            disconnect(connection);
}

bool Style::eventFilter(QObject *watched, QEvent *event)
{
    if (auto *wizard = qobject_cast<QWizard *>(watched);
        wizard && (event->type() == QEvent::Show || event->type() == QEvent::ChildAdded)) {
        const QPointer<QWizard> guardedWizard(wizard);
        const QPointer<Style> guardedStyle(this);
        QTimer::singleShot(0, wizard, [guardedWizard, guardedStyle] {
            if (guardedWizard && guardedStyle)
                refreshWizardSurface(guardedWizard, guardedStyle->standardPalette());
        });
    }
    if (auto *editor = qobject_cast<QLineEdit *>(watched); editor
        && (event->type() == QEvent::FocusIn || event->type() == QEvent::KeyPress
            || event->type() == QEvent::MouseButtonPress)) {
        // setCompleter() has no change signal. User interaction is the point
        // at which a replacement popup can first become visible.
        syncCompleterPopupDensity(editor);
    }
    if (event->type() == QEvent::ParentChange) {
        // A reparent can move a surface into or out of the live recipe while
        // the material is granted. Converge the subtree without waiting for
        // the next theme refresh (restore-first, so either direction lands).
        if (auto *widget = qobject_cast<QWidget *>(watched))
            Private::reconvergeSurfaceForBackdrop(widget);
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
                applyExplicitSurfacePalette(widget);
                // An island opted in while the material is already granted
                // joins the live recipe now instead of painting opaque until
                // the next theme refresh.
                Private::armPolishedSurfaceForBackdrop(widget);
                widget->update();
            } else if (change->propertyName() == BackdropProperty && widget->isWindow()) {
                const Backdrop requested = backdropFromProperty(widget->property(BackdropProperty));
                const bool leavingCompositedBackdrop = requested == Backdrop::None
                        && Private::backdropEffectiveSurface(widget)
                                == Private::BackdropSurface::Composited;
                applyBackdrop(widget, requested);
                if (leavingCompositedBackdrop) {
                    // A hidden QMenu can retain its acrylic HWND across the
                    // owner's Mica transition. DWM then keeps sampling the
                    // old dark material on later opens even though the Qt
                    // popup palette and the requested backdrop are unchanged.
                    // Recreate only those hidden native windows; Qt retains
                    // the QMenu, actions, parent, and style state.
                    for (QMenu *menu : widget->findChildren<QMenu *>()) {
                        if (menu->isWindow() && !menu->isVisible()) {
                            if (QWindow *native = menu->windowHandle())
                                native->destroy();
                        }
                    }
                }
                // Navigation surfaces are transparent only while a real
                // backdrop is active. Refresh just the opted-in views when a
                // window changes backdrop so offscreen/opaque windows never
                // retain transparent backing-store rows. The view frame and
                // its viewport repaint synchronously: delegate rows otherwise
                // keep the previous surface's pixels until the next hover
                // (the mica panel-vs-list skew), and the frame area outside
                // the viewport keeps them past a re-enable (stale rows the
                // viewport-only repaint never rebuilds).
                const auto refreshNavigationView = [](QAbstractItemView *candidate) {
                    if (candidate && candidate->property(NavigationViewProperty).toBool()) {
                        NavigationPrivate::prepareNavigationView(candidate);
                        candidate->repaint();
                        candidate->viewport()->repaint();
                    }
                };
                refreshNavigationView(qobject_cast<QAbstractItemView *>(widget));
                for (QAbstractItemView *view : widget->findChildren<QAbstractItemView *>())
                    refreshNavigationView(view);
            } else if (change->propertyName() == Private::effectiveBackdropProperty
                       && widget->isWindow() && widget->windowType() != Qt::Popup) {
                // applyBackdrop just published a new effective surface for this
                // main window (granted, refused, or restored). Re-run the
                // owned-palette pass so surfaces computed at polish time —
                // notably the inline calendar surface — converge onto the
                // granted material instead of keeping the stale opaque fill.
                // Popups are excluded: their surface is re-asserted on Show by
                // preparePopupSurface, and refreshing them here would re-enter
                // that path during Show dispatch.
                refreshOwnedPalettes(widget);
            }
        }
    }
    if (d->interactionController->eventFilter(watched, event))
        return true;
    return QProxyStyle::eventFilter(watched, event);
}
} // namespace WinUI3
