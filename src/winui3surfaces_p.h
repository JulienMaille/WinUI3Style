// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include "winui3backdrop_p.h"

#include <winui3style/winui3global.h>

#include <QAbstractScrollArea>
#include <QObject>
#include <QPalette>
#include <QPointer>
#include <QScrollBar>
#include <QTimer>
#include <QVariant>
#include <QWidget>

class QComboBox;
class QDialog;
class QLineEdit;
class QAbstractButton;
class QSlider;
class QMenu;

namespace WinUI3 {
class Style;

namespace Private {

void remember(QWidget *widget, const char *property, const QVariant &value);
// True for a QCalendarWidget or any of its descendants.
bool calendarView(const QWidget *widget);
void rememberPalette(QWidget *widget);
void restoreRememberedPalette(QWidget *widget);
QPalette effectivePopupPalette(QWidget *widget, const QPalette &fallback);
// A backdrop palette write may re-resolve inherited child palettes. Preserve
// implicit style ownership for the surface and its still-owned descendants.
void setBackdropPalette(QWidget *surface, const QPalette &palette);

// ContentDialog surfaces (dialog state arm/restore, command footer surface,
// smoke scrim) live in winui3dialogs_p.h.

// QLineEdit helper affordances (clear button cache, QAction helper buttons)
// live in winui3lineedithelpers_p.h.

// Slider value tooltip surface (tool window, debounce timer, synchronous state)
// lives in winui3slidertooltip_p.h.

void applyMenuRoundedMask(QMenu *menu);
// Popup surface lifecycle (palette/attribute ownership, DWM acrylic grant
// readback, first-show anchoring) lives in winui3popupsurface_p.h.

// Window chrome (menu bar, tool bars, status bar) reveals the live DWM
// material instead of painting opaque panels over it. Content/layer
// islands follow under the full-Mica contract. Both are no-ops for
// windows without matching descendants, and restore returns every
// touched widget to its remembered palette, attributes and autofill.
// Exported for the offscreen mechanism tests (same seam as the content
// sync below): the DWM-applied branch that calls the chrome chain never
// runs offscreen, so tests drive make/restore directly.
WINUI3STYLE_EXPORT void makeChromeSurfacesTransparent(QWidget *window);
WINUI3STYLE_EXPORT void restoreChromeSurfaces(QWidget *window);
// Exported for the offscreen mechanism tests (same seam as the exported
// FramePropertyRegistry): the DWM-applied branch that calls the chain below
// never runs offscreen, so tests drive sync/restore directly.
WINUI3STYLE_EXPORT void syncContentSurfacesForBackdrop(QWidget *window);
WINUI3STYLE_EXPORT void restoreContentSurfacesForBackdrop(QWidget *window);
// The no-fill recipe for any widget that paints straight onto the live
// material through a translucent island: transparent Window role, no
// autofill, StyledBackground so Qt leaves the erase to the style's
// Source-clear branch (backdrop erase policy, recipe 1: see
// winui3helpers_p.h). State is remembered so toggle-off restores exactly.
void transparentizeForBackdrop(QWidget *surface);
void restoreTransparentizedForBackdrop(QWidget *surface);
bool isTransparentizedForBackdrop(const QWidget *surface);
// Remembers the pre-backdrop palette, explicit flag and backdrop attributes
// once (first write wins). Shared by transparentizeSurface below and by the
// NavigationView surface prepare, so a view armed by both keeps one snapshot
// and both restores stay idempotent in either order.
void rememberBackdropSurfaceState(QWidget *surface);
// Saved pre-material palette for a surface armed through
// rememberBackdropSurfaceState (null when never armed).
QPalette backdropSurfaceSnapshotPalette(const QWidget *surface);
// Rebase the saved snapshot when the runtime theme moves (style-owned
// surfaces follow the theme while keeping their live recipe).
void rebaseBackdropSurfaceSnapshotPalette(QWidget *surface, const QPalette &palette);
// Arms one freshly polished widget when its window already presents a live
// material (Composited): chrome, islands, scroll chains and bare containers
// converge without waiting for the next theme refresh. Same remember-once
// helpers and scope rules as the window sync below; no-op otherwise.
// Re-runs one subtree after a reparent: restore first (no-op if never
// armed), then arm, so a recipe change in either direction converges.
void armPolishedSurfaceForBackdrop(QWidget *widget);
void reconvergeSurfaceForBackdrop(QWidget *widget);
// Small-delta scrolls inside a translucent content/layer island smear: Qt
// scrolls the viewport backing store with a blit and only repaints the
// exposed strip, so shifted pixels accumulate over the live material. Armed
// once per scroll area by the sync below; fires on the bars' valueChanged
// (all wheel/drag/bar paths) and schedules one queued full viewport update
// past the scroll. Gated on Composited so offscreen snapshots stay
// deterministic.
inline void guardIslandScrollArea(QAbstractScrollArea *area)
{
    if (!area || !area->viewport())
        return;
    constexpr auto guardProperty = "_winui_island_scroll_guard";
    if (area->property(guardProperty).isValid())
        return;
    area->setProperty(guardProperty, true);
    // Heal through the area, not the viewport captured at arm time: the
    // connect() context below is the area itself, so a setViewport()
    // replacement keeps the connections alive and the heal resolves the
    // current viewport on execution. (The previous viewport-context form
    // auto-disconnected when the old viewport was destroyed while the guard
    // bit above blocked re-arming, leaving the new viewport unhealed.)
    // Coalesced, not one queued heal per tick: valueChanged fires on every
    // wheel/drag tick and Qt::QueuedConnection never coalesces, so N ticks
    // queued N synchronous viewport-plus-children repaints per frame. The
    // scheduler below runs direct, sets one pending flag on the area, and
    // arms a single 0ms shot; the shot clears the flag first (on every path,
    // including the Composited-gate early returns) so later frames re-arm.
    // repaint() (not update()) so the blitted rows rebuild before the next
    // present; then heal the viewport's children too, since a parent repaint
    // clips children out and each child keeps its own blitted rows.
    const auto heal = [guardedArea = QPointer<QAbstractScrollArea>(area)] {
        if (!guardedArea)
            return;
        constexpr auto pendingProperty = "_winui_island_scroll_heal_pending";
        guardedArea->setProperty(pendingProperty, {});
        QWidget *viewport = guardedArea->viewport();
        if (!viewport || !viewport->isVisible())
            return;
        if (backdropEffectiveSurface(viewport->window()) != BackdropSurface::Composited)
            return;
        viewport->repaint();
        const QList<QWidget *> children =
                viewport->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly);
        for (QWidget *child : children) {
            if (child->isVisible())
                child->repaint();
        }
    };
    const auto schedule = [guardedArea = QPointer<QAbstractScrollArea>(area), heal] {
        constexpr auto pendingProperty = "_winui_island_scroll_heal_pending";
        if (!guardedArea || guardedArea->property(pendingProperty).isValid())
            return;
        guardedArea->setProperty(pendingProperty, true);
        QTimer::singleShot(0, guardedArea.data(), heal);
    };
    QObject::connect(area->verticalScrollBar(), &QScrollBar::valueChanged, area, schedule);
    QObject::connect(area->horizontalScrollBar(), &QScrollBar::valueChanged, area, schedule);
}

} // namespace Private
} // namespace WinUI3
