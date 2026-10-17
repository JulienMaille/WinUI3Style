// SPDX-License-Identifier: LGPL-2.1-or-later
#include "winui3surfaces_p.h"

#include "winui3paint_p.h"
#include "winui3frameproperties_p.h"
#include "winui3helpers_p.h"
#include "winui3style_properties_p.h"
#include "winui3tokens_p.h"
#include "winui3backdrop_p.h"

#include <winui3style/winui3style.h>
#include "winui3qtcompat_p.h"

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAbstractScrollArea>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QCalendarWidget>
#include <QCompleter>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFontMetrics>
#include <QFrame>
#include <QGuiApplication>
#include <QHash>
#include <QImage>
#include <QLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMetaObject>
#include <QListView>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QMessageBox>
#include <QPaintEvent>
#include <QPainter>
#include <QParallelAnimationGroup>
#include <QPointer>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QSlider>
#include <QStyleOptionSlider>
#include <QTabWidget>
#include <QTimer>
#include <QWidget>
#include <QWindow>

#ifdef Q_OS_WIN
#  define NOMINMAX
#  include <windows.h>
#  include <dwmapi.h>
#endif

namespace WinUI3::Private {

bool calendarView(const QWidget *widget)
{
    for (const QWidget *candidate = widget; candidate; candidate = candidate->parentWidget()) {
        if (qobject_cast<const QCalendarWidget *>(candidate))
            return true;
    }
    return false;
}

using namespace PaintPrivate;
namespace {

// Mica temporarily changes style-owned surfaces after polish. Its snapshot
// must not reuse the style's original* properties: those describe the state
// before polish, not the opaque state immediately before Mica was enabled.
constexpr auto backdropSurfacePaletteProperty = "_winui_backdrop_surface_palette";
constexpr auto backdropSurfacePaletteExplicitProperty = "_winui_backdrop_surface_palette_explicit";
constexpr auto backdropSurfaceAutoFillProperty = "_winui_backdrop_surface_auto_fill";
constexpr auto backdropSurfaceTranslucentProperty = "_winui_backdrop_surface_translucent";
constexpr auto backdropSurfaceNoSystemBackgroundProperty =
        "_winui_backdrop_surface_no_system_background";
constexpr auto backdropSurfaceStyledBackgroundProperty =
        "_winui_backdrop_surface_styled_background";
constexpr auto backdropSurfaceOpaquePaintProperty = "_winui_backdrop_surface_opaque_paint";


// True for a QCalendarWidget or any of its descendants. Shared with the popup
// surface TU, which resolves calendar pickers the same way.

struct LineEditClearButtonCache
{
    QHash<QLineEdit *, QPointer<QAbstractButton>> buttons;
    QHash<QLineEdit *, QMetaObject::Connection> cleanupConnections;
};

LineEditClearButtonCache &lineEditClearButtonCache()
{
    // The cache lives until process exit, like the frame registry, so a
    // widget cannot race its cleanup with static destruction.
    static auto *cache = new LineEditClearButtonCache;
    return *cache;
}

} // namespace

void remember(QWidget *widget, const char *property, const QVariant &value)
{
    if (widget && !widget->property(property).isValid())
        widget->setProperty(property, value);
}

// Qt-mask equivalent of the DWM corner preference for QMenu popups:
// submenus materialize their native HWND after Show (or never resize, so
// the Resize re-apply never fires), leaving them square while the top
// level is rounded. A Qt rounded-rect mask clips the same OverlayRadius
// on every platform, costs no geometry change, and is a no-op under the
// live DWM preference (the native rounding already clips inside it).
// Only ever applied to Qt::Popup QMenu windows from preparePopupSurface,
// so no other surface can pick it up; clearMask() on teardown restores.
namespace {
constexpr auto menuMaskRetryProperty = "_winui_menu_mask_retry";
constexpr int menuMaskMaxRetries = 5;
} // namespace
void applyMenuRoundedMask(QMenu *menu)
{
    if (!menu || !menu->isWindow() || menu->windowType() != Qt::Popup)
        return;
#ifdef Q_OS_WIN
    // A granted DWM corner preference already clips the native flyout and
    // its shadow. Stacking Qt's rasterized region over it creates short dark
    // seams at the top/bottom edge after a Mica cycle. Keep the Qt mask only
    // when native rounding is unavailable (including offscreen captures).
    if (QGuiApplication::platformName() == QStringLiteral("windows")) {
        constexpr DWORD cornerPreferenceAttribute = 33; // DWMWA_WINDOW_CORNER_PREFERENCE
        DWORD preference = 0;
        if (SUCCEEDED(DwmGetWindowAttribute(reinterpret_cast<HWND>(menu->winId()),
                                            cornerPreferenceAttribute, &preference,
                                            sizeof(preference)))
            && preference == 2) { // DWMWCP_ROUND
            if (!menu->mask().isEmpty())
                menu->clearMask();
            return;
        }
    }
#endif
    const QSize size = menu->size();
    if (size.isEmpty()) {
        // Show can arrive before QMenu computes its layout size. Retry
        // queued past the show so the submenu still rounds instead of
        // staying square. The attempt count stays set across retries and
        // is cleared only on success below: clearing it before re-entering
        // would make every retry look like a first attempt and reschedule
        // forever for a menu that never gains a size (a 0ms singleShot
        // chain / CPU spin). Bound the chain instead so a permanently
        // zero-size menu settles unmasked.
        const int attempts = menu->property(menuMaskRetryProperty).toInt();
        if (attempts >= menuMaskMaxRetries)
            return;
        menu->setProperty(menuMaskRetryProperty, attempts + 1);
        QTimer::singleShot(0, menu, [menu] { applyMenuRoundedMask(menu); });
        return;
    }
    menu->setProperty(menuMaskRetryProperty, {});
    // Rasterize the SAME rounded rect paint lays (OverlayRadius) and
    // threshold at 50% alpha: nominal r8 is preserved while paint and
    // mask coincide at the half-coverage line — no dark fringe inside
    // over the composited material, no backdrop hairline outside.
    QPainterPath path;
    path.addRoundedRect(QRectF(0, 0, size.width(), size.height()), OverlayRadius, OverlayRadius);
    QImage raster(size, QImage::Format_ARGB32_Premultiplied);
    raster.fill(Qt::transparent);
    QPainter rasterPainter(&raster);
    rasterPainter.setRenderHint(QPainter::Antialiasing);
    rasterPainter.fillPath(path, Qt::white);
    rasterPainter.end();
    // Scanline runs: kept pixels (>= 50% alpha) union into the region.
    QRegion mask;
    for (int y = 0; y < size.height(); ++y) {
        const QRgb *line = reinterpret_cast<const QRgb *>(raster.constScanLine(y));
        int runStart = -1;
        for (int x = 0; x <= size.width(); ++x) {
            const bool kept = x < size.width() && qAlpha(line[x]) >= 128;
            if (kept && runStart < 0)
                runStart = x;
            else if (!kept && runStart >= 0) {
                mask += QRect(runStart, y, x - runStart, 1);
                runStart = -1;
            }
        }
    }
    menu->setMask(mask);
}

void rememberPalette(QWidget *widget)
{
    if (!widget)
        return;
    remember(widget, originalPaletteExplicitProperty, widget->testAttribute(Qt::WA_SetPalette));
    remember(widget, originalPaletteProperty, QVariant::fromValue(widget->palette()));
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

// transparentized(): preserve RGB, drive alpha toward 0. The tokens file
// owns color values; this helper only modulates an existing role color so
// no numeric QColor literal ever appears in a _p.cpp painter.
inline QColor transparentized(const QColor &color, int alpha)
{
    QColor result = color;
    result.setAlpha(alpha);
    return result;
}

void setBackdropPalette(QWidget *surface, const QPalette &palette)
{
    // Temporary Mica palettes are style writes, not application overrides.
    // Keep the implicit-owner snapshot in step so the effective-surface
    // refresh does not permanently disown scroll-area and shell palettes.
    const QVariant snapshot = surface->property(implicitPaletteSnapshotProperty);
    const bool styleOwned = surface->property(ownedPaletteProperty).toBool() && snapshot.isValid()
            && surface->palette() == snapshot.value<QPalette>();
    // A parent's Window-role change can also re-resolve palettes on its
    // children (navigationPanel is a child of centralWidget). Remember only
    // descendants still matching their style snapshot before that write.
    QList<QPointer<QWidget>> inheritedOwners;
    for (QWidget *child : surface->findChildren<QWidget *>()) {
        const QVariant childSnapshot = child->property(implicitPaletteSnapshotProperty);
        if (child->property(ownedPaletteProperty).toBool() && childSnapshot.isValid()
            && child->palette() == childSnapshot.value<QPalette>())
            inheritedOwners.append(child);
    }
    if (styleOwned)
        surface->setProperty(implicitPaletteSnapshotProperty, QVariant::fromValue(palette));
    surface->setPalette(palette);
    if (styleOwned)
        surface->setProperty(implicitPaletteSnapshotProperty,
                             QVariant::fromValue(surface->palette()));
    for (const QPointer<QWidget> &child : inheritedOwners) {
        if (child)
            child->setProperty(implicitPaletteSnapshotProperty,
                               QVariant::fromValue(child->palette()));
    }
}

void rememberBackdropSurfaceState(QWidget *surface)
{
    if (!surface)
        return;
    remember(surface, backdropSurfacePaletteProperty, QVariant::fromValue(surface->palette()));
    remember(surface, backdropSurfacePaletteExplicitProperty,
             surface->testAttribute(Qt::WA_SetPalette));
    remember(surface, backdropSurfaceAutoFillProperty, surface->autoFillBackground());
    remember(surface, backdropSurfaceTranslucentProperty,
             surface->testAttribute(Qt::WA_TranslucentBackground));
    remember(surface, backdropSurfaceNoSystemBackgroundProperty,
             surface->testAttribute(Qt::WA_NoSystemBackground));
    remember(surface, backdropSurfaceStyledBackgroundProperty,
             surface->testAttribute(Qt::WA_StyledBackground));
    remember(surface, backdropSurfaceOpaquePaintProperty,
             surface->testAttribute(Qt::WA_OpaquePaintEvent));
}

QPalette backdropSurfaceSnapshotPalette(const QWidget *surface)
{
    if (!surface)
        return QPalette();
    const QVariant saved = surface->property(backdropSurfacePaletteProperty);
    return saved.isValid() ? saved.value<QPalette>() : QPalette();
}

void rebaseBackdropSurfaceSnapshotPalette(QWidget *surface, const QPalette &palette)
{
    if (!surface)
        return;
    surface->setProperty(backdropSurfacePaletteProperty, QVariant::fromValue(palette));
}

void transparentizeSurface(QWidget *surface)
{
    if (!surface)
        return;
    rememberBackdropSurfaceState(surface);
    QPalette palette = surface->palette();
    // The Window role reveals the live material; keep text roles intact.
    palette.setColor(QPalette::Window, transparentized(palette.color(QPalette::Window), 0));
    setBackdropPalette(surface, palette);
    // Never auto-fill here: an opaque fill would paint a solid band over
    // the composited material (the white-veil ghost).
    surface->setAutoFillBackground(false);
    surface->setAttribute(Qt::WA_StyledBackground, true);
    surface->update();
}

void restoreTransparentizedSurface(QWidget *surface)
{
    if (!surface || !surface->property(backdropSurfacePaletteProperty).isValid())
        return;
    if (surface->property(backdropSurfacePaletteExplicitProperty).toBool())
        setBackdropPalette(surface,
                           surface->property(backdropSurfacePaletteProperty).value<QPalette>());
    else
        setBackdropPalette(surface, QPalette());
    if (surface->property(backdropSurfaceAutoFillProperty).isValid())
        surface->setAutoFillBackground(surface->property(backdropSurfaceAutoFillProperty).toBool());
    if (surface->property(backdropSurfaceTranslucentProperty).isValid())
        surface->setAttribute(Qt::WA_TranslucentBackground,
                              surface->property(backdropSurfaceTranslucentProperty).toBool());
    if (surface->property(backdropSurfaceNoSystemBackgroundProperty).isValid())
        surface->setAttribute(
                Qt::WA_NoSystemBackground,
                surface->property(backdropSurfaceNoSystemBackgroundProperty).toBool());
    if (surface->property(backdropSurfaceStyledBackgroundProperty).isValid())
        surface->setAttribute(Qt::WA_StyledBackground,
                              surface->property(backdropSurfaceStyledBackgroundProperty).toBool());
    if (surface->property(backdropSurfaceOpaquePaintProperty).isValid())
        surface->setAttribute(Qt::WA_OpaquePaintEvent,
                              surface->property(backdropSurfaceOpaquePaintProperty).toBool());
    surface->setProperty(backdropSurfacePaletteProperty, {});
    surface->setProperty(backdropSurfacePaletteExplicitProperty, {});
    surface->setProperty(backdropSurfaceAutoFillProperty, {});
    surface->setProperty(backdropSurfaceTranslucentProperty, {});
    surface->setProperty(backdropSurfaceNoSystemBackgroundProperty, {});
    surface->setProperty(backdropSurfaceStyledBackgroundProperty, {});
    surface->setProperty(backdropSurfaceOpaquePaintProperty, {});
    surface->update();
}

bool isTransparentizedForBackdrop(const QWidget *surface)
{
    return surface && surface->property(backdropSurfacePaletteProperty).isValid();
}

void transparentizeForBackdrop(QWidget *surface)
{
    transparentizeSurface(surface);
}

void restoreTransparentizedForBackdrop(QWidget *surface)
{
    restoreTransparentizedSurface(surface);
}

// Defined below (after the sync entry points): restores the exact pre-Mica
// style-owned state before repainting. Forward-declared here so the shared
// scroll-chain/container helpers above can mirror every sync link.
static void restoreBackdropSurface(QWidget *widget);

void makeChromeSurfacesTransparent(QWidget *window)
{
    if (!window)
        return;
    // Menu bar, tool bars, status bar: WinUI draws no chrome panels over
    // Mica; the material shows through and controls float on it. Every menu
    // bar is visited (not just the first): nested demo windows carry their
    // own bars and a singular findChild would arm whichever bar the object
    // tree yields first, leaving the real menuBar opaque over the material.
    // In-card samples are skipped: they sit on the card's veiled fill
    // (recipe 1), so transparentizing them would punch the card to alpha 0.
    for (QMenuBar *menuBar : window->findChildren<QMenuBar *>()) {
        if (insideOpaqueCard(menuBar))
            continue;
        transparentizeSurface(menuBar);
    }
    for (QToolBar *toolBar : window->findChildren<QToolBar *>()) {
        if (insideOpaqueCard(toolBar))
            continue;
        transparentizeSurface(toolBar);
    }
    for (QStatusBar *statusBar : window->findChildren<QStatusBar *>()) {
        if (insideOpaqueCard(statusBar))
            continue;
        transparentizeSurface(statusBar);
    }
}

void restoreChromeSurfaces(QWidget *window)
{
    if (!window)
        return;
    // Restore the saved recipe, even if the application has since reparented
    // a bar into a card. Never-armed bars are already a no-op.
    for (QMenuBar *menuBar : window->findChildren<QMenuBar *>()) {
        restoreTransparentizedSurface(menuBar);
    }
    for (QToolBar *toolBar : window->findChildren<QToolBar *>()) {
        restoreTransparentizedSurface(toolBar);
    }
    for (QStatusBar *statusBar : window->findChildren<QStatusBar *>()) {
        restoreTransparentizedSurface(statusBar);
    }
}

namespace {

// One visual surface for the scrolled chain: the area frame, its viewport,
// both bars, and the QScrollArea container widget each paint PE_Widget over
// the live material, so every link gets the same no-fill recipe. The scroll
// bars join the chain too: bars carry style-owned explicit palettes (polish
// registers palette owners, the theme refresh writes them), so they never
// inherit the island's transparentized roles. An unarmed bar keeps its opaque
// Window role and the CC_ScrollBar painter takes the Window-fill fallback
// over the live material.
// Card interiors take a partial arm (reviewer 2026-10-06 tree-selection
// overlay): the viewport's auto-filled Base layer would double over the card
// veil, so only the autofill stops (roles stay intact for tone reads) and
// the viewport repaints the card tone itself via PE (erase + Source refill,
// smear-safe). Bars keep their own paint paths untouched. The arm reuses the
// same backdrop-surface snapshot keys as transparentizeSurface (palette is
// remembered unchanged) so restore is uniform.

void restoreScrollChain(QAbstractScrollArea *area);

void transparentizeScrollChain(QAbstractScrollArea *area)
{
    if (!area)
        return;
    // A reparent can change the recipe during an active backdrop. Return the
    // old arm first so the next snapshot never captures temporary attributes.
    // Bare NavigationView areas are exempt: the navigation prepare owns their
    // area/viewport recipe (Base alpha 0), and rolling it back here would
    // undo settled nav state on every sync while the re-arm below cannot
    // rebuild the Base role. In-card nav keeps its own recipe via the branch
    // below, as before.
    const bool inCard = insideOpaqueCard(area);
    const bool bareNavigation =
            !inCard && area->property(Style::NavigationViewProperty).toBool();
    if (!bareNavigation
        && ((inCard && area->property(backdropSurfacePaletteProperty).isValid())
            || (!inCard && area->viewport()
                && area->viewport()->property(backdropSurfacePaletteProperty).isValid())))
        restoreScrollChain(area);
    if (inCard) {
        // NavigationView-marked views keep their own recipe
        // (prepareNavigationSurface leaves in-card nav opaque + autofilled):
        // never second-guess it here.
        if (!area->property(Style::NavigationViewProperty).toBool()) {
            if (QWidget *viewport = area->viewport()) {
                remember(viewport, backdropSurfacePaletteProperty,
                         QVariant::fromValue(viewport->palette()));
                remember(viewport, backdropSurfacePaletteExplicitProperty,
                         viewport->testAttribute(Qt::WA_SetPalette));
                remember(viewport, backdropSurfaceAutoFillProperty,
                         viewport->autoFillBackground());
                remember(viewport, backdropSurfaceStyledBackgroundProperty,
                         viewport->testAttribute(Qt::WA_StyledBackground));
                remember(viewport, backdropSurfaceOpaquePaintProperty,
                         viewport->testAttribute(Qt::WA_OpaquePaintEvent));
                viewport->setAutoFillBackground(false);
                viewport->setAttribute(Qt::WA_StyledBackground, true);
                viewport->setAttribute(Qt::WA_OpaquePaintEvent, false);
                viewport->update();
            }
        }
        return;
    }
    guardIslandScrollArea(area);
    transparentizeForBackdrop(area);
    if (QWidget *viewport = area->viewport())
        transparentizeForBackdrop(viewport);
    if (QScrollBar *verticalBar = area->verticalScrollBar())
        transparentizeForBackdrop(verticalBar);
    if (QScrollBar *horizontalBar = area->horizontalScrollBar())
        transparentizeForBackdrop(horizontalBar);
    if (auto *scrollArea = qobject_cast<QScrollArea *>(area)) {
        if (QWidget *container = scrollArea->widget())
            transparentizeForBackdrop(container);
    }
}

void restoreScrollChain(QAbstractScrollArea *area)
{
    if (!area)
        return;
    if (QWidget *viewport = area->viewport()) {
        restoreBackdropSurface(viewport);
        viewport->update();
    }
    // Each restore is a no-op for widgets that were never transparentized.
    restoreBackdropSurface(area);
    if (QWidget *viewport = area->viewport())
        restoreBackdropSurface(viewport);
    if (QScrollBar *verticalBar = area->verticalScrollBar())
        restoreBackdropSurface(verticalBar);
    if (QScrollBar *horizontalBar = area->horizontalScrollBar())
        restoreBackdropSurface(horizontalBar);
    if (auto *scrollArea = qobject_cast<QScrollArea *>(area)) {
        if (QWidget *container = scrollArea->widget())
            restoreBackdropSurface(container);
    }
    area->update();
}

// Card keep-opaque gate for the generic container walk below: group cards,
// SettingsCards, the explicit palette preview, the inline calendar frame and
// the QTabWidget pane/pages own their surface fill by design and must never
// punch through to Mica. Delegates to the shared structural card walk so the
// sync, the PE_Widget erase guard and the range-control erases agree on one
// definition.
bool keepOpaqueContainer(const QWidget *widget)
{
    if (!widget)
        return true;
    return isCardSurface(widget);
}

// Generic container links that sit directly on the material with no card:
// plain QFrame panels inside the dockHost subtree and bare StyledPanel links.
// Links inside a card (QTabWidget pane/pages, viewports under a group card)
// are gated out too: the card fill owns that surface. Each link is
// transparentized through the same helper so toggle-off restore returns it.
void transparentizeBareContainer(QWidget *widget)
{
    if (!widget || keepOpaqueContainer(widget) || insideOpaqueCard(widget))
        return;
    transparentizeForBackdrop(widget);
}

void restoreBareContainer(QWidget *widget)
{
    if (!widget)
        return;
    restoreBackdropSurface(widget);
}

// True when the widget sits under an opted-in island or the central shell:
// the same subtree scope the window sync arms scroll chains and bare
// containers in. Anything else keeps its opaque fill by design.
bool underLiveScope(const QWidget *widget)
{
    return anyAncestorToWindow(widget, [](const QWidget *ancestor) {
        if (isContentLayerSurface(ancestor->property(Style::SurfaceProperty)))
            return true;
        return ancestor->objectName() == QStringLiteral("centralWidget");
    });
}

bool underDockHost(const QWidget *widget)
{
    return anyAncestorToWindow(widget, [](const QWidget *ancestor) {
        return ancestor->objectName() == QStringLiteral("dockHost");
    });
}

} // namespace

void armPolishedSurfaceForBackdrop(QWidget *widget)
{
    if (!widget || widget->isWindow())
        return;
    QWidget *win = widget->window();
    if (!win || win->windowType() == Qt::Popup)
        return;
    if (backdropEffectiveSurface(win) != BackdropSurface::Composited)
        return;
    if (insideOpaqueCard(widget)) {
        // Card interiors keep their fill; only a scroll area's viewport takes
        // the partial arm (same branch the window sync uses).
        if (auto *area = qobject_cast<QAbstractScrollArea *>(widget))
            transparentizeScrollChain(area);
        return;
    }
    // Late-created viewport, bars or container join an already-armed chain.
    if (auto *area = qobject_cast<QAbstractScrollArea *>(widget->parentWidget());
        area && isTransparentizedForBackdrop(area)) {
        if (widget == area->viewport() || qobject_cast<QScrollBar *>(widget))
            transparentizeSurface(widget);
        else if (auto *scrollArea = qobject_cast<QScrollArea *>(area);
                 scrollArea && widget == scrollArea->widget())
            transparentizeSurface(widget);
        return;
    }
    if (qobject_cast<QMenuBar *>(widget) || qobject_cast<QToolBar *>(widget)
        || qobject_cast<QStatusBar *>(widget)) {
        transparentizeSurface(widget);
        return;
    }
    if (isContentLayerSurface(widget->property(Style::SurfaceProperty))) {
        if (widget->palette().color(QPalette::Window).alpha() == 0)
            return;
        transparentizeSurface(widget);
        if (auto *islandArea = qobject_cast<QAbstractScrollArea *>(widget))
            guardIslandScrollArea(islandArea);
        return;
    }
    if (auto *area = qobject_cast<QAbstractScrollArea *>(widget)) {
        if (underLiveScope(widget))
            transparentizeScrollChain(area);
        return;
    }
    if (!underLiveScope(widget))
        return;
    if (qobject_cast<QTabWidget *>(widget))
        transparentizeBareContainer(widget);
    else if (auto *frame = qobject_cast<QFrame *>(widget)) {
        if (frame->frameShape() == QFrame::StyledPanel || underDockHost(widget))
            transparentizeBareContainer(frame);
    }
}

void reconvergeSurfaceForBackdrop(QWidget *widget)
{
    if (!widget)
        return;
    // Two passes, parent-first: findChildren yields ancestors before
    // descendants. Restoring everything first lets a scroll area re-arm its
    // chain (including the partial in-card viewport arm) without a later
    // child restore undoing it.
    restoreTransparentizedForBackdrop(widget);
    const QList<QWidget *> subtree = widget->findChildren<QWidget *>();
    for (QWidget *child : subtree)
        restoreTransparentizedForBackdrop(child);
    armPolishedSurfaceForBackdrop(widget);
    for (QWidget *child : subtree)
        armPolishedSurfaceForBackdrop(child);
}

// Generic container links under one scope (island or shell): QTabWidget
// panes, dockHost frames, bare StyledPanel links. Cards keep their fill by
// design via the helper gate. One definition for sync and restore.
void syncContainerLinks(QWidget *scope)
{
    for (QTabWidget *tabs : scope->findChildren<QTabWidget *>()) {
        transparentizeBareContainer(tabs);
        if (QWidget *pane = tabs->widget(tabs->currentIndex()))
            transparentizeBareContainer(pane);
    }
    if (QWidget *dockHost = scope->findChild<QWidget *>(QStringLiteral("dockHost"))) {
        for (QFrame *frame : dockHost->findChildren<QFrame *>())
            transparentizeBareContainer(frame);
    }
    for (QFrame *frame : scope->findChildren<QFrame *>()) {
        if (frame->frameShape() == QFrame::StyledPanel)
            transparentizeBareContainer(frame);
    }
}

void restoreContainerLinks(QWidget *scope)
{
    for (QTabWidget *tabs : scope->findChildren<QTabWidget *>()) {
        restoreBareContainer(tabs);
        for (int index = 0; index < tabs->count(); ++index)
            restoreBareContainer(tabs->widget(index));
    }
    if (QWidget *dockHost = scope->findChild<QWidget *>(QStringLiteral("dockHost"))) {
        for (QFrame *frame : dockHost->findChildren<QFrame *>())
            restoreBareContainer(frame);
    }
    for (QFrame *frame : scope->findChildren<QFrame *>()) {
        if (frame->frameShape() == QFrame::StyledPanel)
            restoreBareContainer(frame);
    }
}

void syncContentSurfacesForBackdrop(QWidget *window)
{
    if (!window)
        return;
    // Content/layer islands opted into the style-owned surface contract
    // reveal the material too; anything else keeps its opaque fill so a
    // child control can never punch through an opaque island. A content
    // island that is itself already transparent keeps walking the gate in
    // paintsDirectlyOnBackdrop instead of disabling clears.
    const QList<QWidget *> islands = window->findChildren<QWidget *>();
    for (QWidget *island : islands) {
        if (!isContentLayerSurface(island->property(Style::SurfaceProperty)))
            continue;
        // The area frame, its viewport, and its container widget are one
        // visual surface with the island: without the chain the
        // Controls-heading band above the first card keeps its opaque fill
        // and ghosts exactly like the island did (healed only by resize).
        // Scroll areas and viewports are armed, not skipped: the queued
        // guard repaint covers their blit smear, and the toggle-off restore
        // below returns every link.
        for (QAbstractScrollArea *area : island->findChildren<QAbstractScrollArea *>())
            transparentizeScrollChain(area);
        if (auto *islandArea = qobject_cast<QAbstractScrollArea *>(island))
            guardIslandScrollArea(islandArea);
        // Generic container links under the island (plain dockHost frames,
        // bare StyledPanel links): same recipe so they stop banding over the
        // live material. Cards keep their fill by design — that now includes
        // the QTabWidget pane/pages (gated out inside the helper), whose
        // PE_FrameTabWidget lays the veiled card tone.
        syncContainerLinks(island);
        if (island->palette().color(QPalette::Window).alpha() == 0)
            continue;
        transparentizeSurface(island);
    }
    // Left shell (centralWidget, navigationPanel): not opt-in islands, but
    // they paint PE_Widget straight onto the live material through the
    // window (no opaque island between them and the window, so the gate
    // passes). Same no-fill recipe, otherwise their backing-store rows keep
    // the opaque frame until a resize reallocates (aliased "Mica backdrop"
    // label, incomplete left Mica background). Toggle-off restore below
    // returns every link (restore is a no-op for never-transparentized).
    if (auto *central = window->findChild<QWidget *>(QStringLiteral("centralWidget"))) {
        transparentizeForBackdrop(central);
        if (auto *nav = central->findChild<QWidget *>(QStringLiteral("navigationPanel")))
            transparentizeForBackdrop(nav);
        // Bare scroll areas under the shell (navigationList and friends):
        // not under any opted-in island, so the loop above never visits
        // them. Same scrolled-chain recipe with the blit guard on each area.
        for (QAbstractScrollArea *area : central->findChildren<QAbstractScrollArea *>())
            transparentizeScrollChain(area);
        syncContainerLinks(central);
    }
}

// Restore the exact pre-Mica style-owned state before repainting. Reverting to
// the pre-polish state instead would make a content island non-opaque.
static void restoreBackdropSurface(QWidget *widget)
{
    if (!widget)
        return;
    restoreTransparentizedForBackdrop(widget);
    widget->update();
}

void restoreContentSurfacesForBackdrop(QWidget *window)
{
    if (!window)
        return;
    const QList<QWidget *> islands = window->findChildren<QWidget *>();
    for (QWidget *island : islands) {
        if (!isContentLayerSurface(island->property(Style::SurfaceProperty)))
            continue;
        // The restore helper returns early for never-transparentized islands,
        // and leaves the style's pre-polish snapshots intact for unpolish.
        restoreBackdropSurface(island);
        // The sync above transparentized the scrolled chain alongside the
        // island: restore every link the same way (each restore is a no-op
        // for widgets that were never transparentized).
        const QList<QAbstractScrollArea *> areas = island->findChildren<QAbstractScrollArea *>();
        for (QAbstractScrollArea *area : areas)
            restoreScrollChain(area);
        restoreContainerLinks(island);
    }
    // Shell links transparentized by the sync above (no-ops if never armed).
    if (auto *central = window->findChild<QWidget *>(QStringLiteral("centralWidget"))) {
        restoreBackdropSurface(central);
        if (auto *nav = central->findChild<QWidget *>(QStringLiteral("navigationPanel"))) {
            restoreBackdropSurface(nav);
        }
        for (QAbstractScrollArea *area : central->findChildren<QAbstractScrollArea *>())
            restoreScrollChain(area);
        restoreContainerLinks(central);
    }
}


namespace {

bool isLineEditClearButton(const QLineEdit *lineEdit, const QAbstractButton *button)
{
    if (!lineEdit || !button)
        return false;
    for (QAction *action : lineEdit->actions())
            if (actionAssociatedWith(action, button))
                return false;
    // QLineEdit's private clear affordance is deliberately not a QAction;
    // custom leading/trailing actions are associated above.
    return true;
}

} // namespace

void cacheLineEditClearButton(QLineEdit *lineEdit, QAbstractButton *button)
{
    if (!lineEdit)
        return;

    auto &cache = lineEditClearButtonCache();
    if (!button) {
        cache.buttons.remove(lineEdit);
        if (const auto connection = cache.cleanupConnections.take(lineEdit))
            QObject::disconnect(connection);
        return;
    }

    cache.buttons.insert(lineEdit, QPointer<QAbstractButton>(button));
    if (!cache.cleanupConnections.contains(lineEdit)) {
        cache.cleanupConnections.insert(lineEdit,
                                        QObject::connect(lineEdit, &QObject::destroyed, [lineEdit] {
                                            auto &cache = lineEditClearButtonCache();
                                            cache.buttons.remove(lineEdit);
                                            cache.cleanupConnections.remove(lineEdit);
                                        }));
    }
}

const QAbstractButton *lineEditClearButton(const QLineEdit *lineEdit)
{
    if (!lineEdit)
        return nullptr;
    const auto &cache = lineEditClearButtonCache();
    const auto button = cache.buttons.constFind(const_cast<QLineEdit *>(lineEdit));
    return button == cache.buttons.cend() ? nullptr : button->data();
}

namespace {

constexpr auto lineEditHelperUpdatePendingProperty = "_winui_line_edit_helper_update_pending";

qulonglong nextLineEditHelperUpdateToken()
{
    // QWidget lifecycle and style events are confined to the GUI thread.
    static qulonglong token = 0;
    return ++token;
}

void scheduleLineEditHelperUpdate(QLineEdit *lineEdit, Style *style)
{
    if (!lineEdit
        || framePropertyRegistry().value(lineEdit, lineEditHelperUpdatePendingProperty).isValid())
        return;

    const qulonglong token = nextLineEditHelperUpdateToken();
    framePropertyRegistry().set(lineEdit, lineEditHelperUpdatePendingProperty,
                                QVariant::fromValue(token));
    const QPointer<QLineEdit> guardedLineEdit(lineEdit);
    const QPointer<Style> guardedStyle(style);
    QTimer::singleShot(0, lineEdit, [guardedLineEdit, guardedStyle, token] {
        if (!guardedLineEdit
            || framePropertyRegistry()
                            .value(guardedLineEdit, lineEditHelperUpdatePendingProperty)
                            .toULongLong()
                    != token)
            return;

        // updateReadOnlyDeleteAffordance() has no style parameter for callers
        // that only need the visibility contract. Resolve it at execution time
        // so a preceding update call can still be coalesced with preparation.
        Style *helperStyle = guardedStyle.data();
        if (!helperStyle)
            helperStyle = qobject_cast<Style *>(guardedLineEdit->style());

        QAbstractButton *clearButton = nullptr;
        for (QAbstractButton *button : guardedLineEdit->findChildren<QAbstractButton *>()) {
            if (helperStyle) {
                // QLineEdit creates its private clear affordance lazily. Depending
                // on that timing, it can miss the parent's polish pass and never
                // deliver hover events to the style. Prepare both the private
                // affordance and QAction helper buttons as soon as they exist.
                button->setAttribute(Qt::WA_Hover, true);
                button->installEventFilter(helperStyle);
                if (!framePropertyRegistry().value(button, hoverProperty).isValid())
                    framePropertyRegistry().set(button, hoverProperty,
                                                button->isEnabled() && button->underMouse() ? 1.0
                                                                                            : 0.0);
                if (!framePropertyRegistry().value(button, pressProperty).isValid())
                    framePropertyRegistry().set(button, pressProperty, 0.0);
            }

            if (isLineEditClearButton(guardedLineEdit, button)) {
                clearButton = button;
                button->setVisible(!guardedLineEdit->isReadOnly() && guardedLineEdit->isEnabled()
                                   && guardedLineEdit->hasFocus()
                                   && guardedLineEdit->isClearButtonEnabled()
                                   && !guardedLineEdit->text().isEmpty());
            }
        }
        cacheLineEditClearButton(guardedLineEdit, clearButton);
        if (framePropertyRegistry()
                    .value(guardedLineEdit, lineEditHelperUpdatePendingProperty)
                    .toULongLong()
            == token) {
            framePropertyRegistry().clear(guardedLineEdit, lineEditHelperUpdatePendingProperty);
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
        framePropertyRegistry().clear(lineEdit, lineEditHelperUpdatePendingProperty);
}

} // namespace WinUI3::Private