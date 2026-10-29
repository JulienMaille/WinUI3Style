// SPDX-License-Identifier: LGPL-2.1-or-later
#include "winui3popupsurface_p.h"

#include "winui3surfaces_p.h"

#include "winui3paint_p.h"
#include "winui3frameproperties_p.h"
#include "winui3helpers_p.h"
#include "winui3qtcompat_p.h"
#include "winui3style_properties_p.h"
#include "winui3tokens_p.h"
#include "winui3backdrop_p.h"

#include <winui3style/winui3backdrop.h>
#include <winui3style/winui3style.h>

#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QCompleter>
#include <QGuiApplication>
#include <QImage>
#include <QLayout>
#include <QListView>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QRegion>
#include <QTimer>
#include <QWidget>
#include <QWindow>

#ifdef Q_OS_WIN
#  define NOMINMAX
#  include <windows.h>
#  include <dwmapi.h>
#endif

namespace WinUI3::Private {
namespace {

// DWM readback gate for the popup re-arm loop (reopened-menu refusal):
// DWM can ACK Set(DWMWA_SYSTEMBACKDROP_TYPE=38) during Show dispatch
// while ignoring the value — HRESULT S_OK but a later Get(38) still reads
// 0/AUTO, so HRESULT success alone must not stop the bounded retry. Only
// S_OK-with-value-!=3 (Refused below) is a definitive refusal. A failed
// Get itself (E_HANDLE and friends during Show dispatch on some builds)
// is Unknown, not evidence: the caller then keeps the HRESULT-based
// acceptance for that attempt.
#ifdef Q_OS_WIN
constexpr DWORD popupBackdropTypeAttribute = 38;
constexpr DWORD popupBackdropTransientValue = 3; // DWMSBT_TRANSIENTWINDOW.
#endif

enum class PopupBackdropReadback {
    Unknown,
    Granted,
    Refused,
};

PopupBackdropReadback popupBackdropReadback(QWidget *popup)
{
#ifdef Q_OS_WIN
    if (!popup || !popup->isWindow() || popup->windowType() != Qt::Popup)
        return PopupBackdropReadback::Unknown;
    if (QGuiApplication::platformName() == QStringLiteral("offscreen"))
        return PopupBackdropReadback::Unknown;
    QWindow *nativeWindow = popup->windowHandle();
    if (!nativeWindow || !nativeWindow->handle())
        return PopupBackdropReadback::Unknown;
    const HWND hwnd = reinterpret_cast<HWND>(nativeWindow->winId());
    if (!hwnd)
        return PopupBackdropReadback::Unknown;
    DWORD value = 0;
    if (FAILED(DwmGetWindowAttribute(hwnd, popupBackdropTypeAttribute, &value, sizeof(value))))
        return PopupBackdropReadback::Unknown;
    return value == popupBackdropTransientValue ? PopupBackdropReadback::Granted
                                                : PopupBackdropReadback::Refused;
#else
    Q_UNUSED(popup);
    return PopupBackdropReadback::Unknown;
#endif
}

} // namespace

// Converge a refused popup to the honest opaque fallback: opaque surface
// roles, full opaque paint recipe, Painted state. A refused DWM grant must
// never leave the translucent tint over a white backing store (light bands
// in the combo container margins). Mirrors the fresh opaque Show path
// below; popupSurfaceColor resolves opaque from any palette (the lift
// drops the tint alpha by construction).
void convergePopupOpaqueFallback(QWidget *popup)
{
    const QColor surface = Private::popupSurfaceColor(popup->palette());
    QPalette opaque = popup->palette();
    opaque.setColor(QPalette::Window, surface);
    opaque.setColor(QPalette::Base, surface);
    popup->setPalette(opaque);
    popup->setAutoFillBackground(true);
    popup->setAttribute(Qt::WA_TranslucentBackground, false);
    popup->setAttribute(Qt::WA_NoSystemBackground, false);
    popup->setAttribute(Qt::WA_StyledBackground, false);
    popup->setAttribute(Qt::WA_OpaquePaintEvent, true);
    popup->setProperty(effectiveBackdropProperty, static_cast<int>(BackdropSurface::Painted));
    popup->update();
}

bool prepareWaylandMenuSurface(QWidget *widget, const QString &platformName)
{
    auto *menu = qobject_cast<QMenu *>(widget);
    if (!menu || !waylandPlatform(platformName))
        return false;

    // Wayland applies QWidget masks to input and opaque regions, not to the
    // rendered pixels. Request alpha before QMenu creates its wl_surface;
    // the style then paints transparent corners around the rounded flyout.
    remember(menu, originalTranslucentBackgroundProperty,
             menu->testAttribute(Qt::WA_TranslucentBackground));
    remember(menu, originalNoSystemBackgroundProperty,
             menu->testAttribute(Qt::WA_NoSystemBackground));
    remember(menu, originalOpaquePaintProperty, menu->testAttribute(Qt::WA_OpaquePaintEvent));
    remember(menu, originalAutoFillProperty, menu->autoFillBackground());
    menu->setAttribute(Qt::WA_TranslucentBackground, true);
    menu->setAttribute(Qt::WA_NoSystemBackground, true);
    menu->setAttribute(Qt::WA_OpaquePaintEvent, false);
    menu->setAutoFillBackground(false);
    return true;
}

} // namespace WinUI3::Private

namespace WinUI3::Private {

void preparePopupSurface(QWidget *widget)
{
    if (!widget || !widget->window() || widget->window()->windowType() != Qt::Popup)
        return;
    const bool waylandMenuAlpha =
            prepareWaylandMenuSurface(widget, QGuiApplication::platformName());
    QWidget *popup = widget->window();
    QAbstractItemView *view = qobject_cast<QAbstractItemView *>(widget);
    if (!view)
        view = popup->findChild<QAbstractItemView *>();
    const bool completerPopup = view
            && (view->property(completerOwnerProperty).isValid()
                || qobject_cast<QCompleter *>(view->parent()));
    rememberPalette(popup);
    remember(popup, originalAutoFillProperty, popup->autoFillBackground());
    remember(popup, originalTranslucentBackgroundProperty,
             popup->testAttribute(Qt::WA_TranslucentBackground));
    remember(popup, originalNoSystemBackgroundProperty,
             popup->testAttribute(Qt::WA_NoSystemBackground));
    // preparePopupSurface runs on every Show and ends either opaque (the
    // deterministic fallback) or composited (DWM acrylic). Remember the
    // pre-show attribute recipe once, so a hide-time DWM teardown that
    // cleared the opaque claim cannot leak into the next cycle: the opaque
    // branch below then restores the full recipe instead of inheriting the
    // composited one (square corners, washed veil, lost shadow).
    remember(popup, originalOpaquePaintProperty, popup->testAttribute(Qt::WA_OpaquePaintEvent));
    // Popup widgets keep an explicit palette after their first polish. Rebase
    // every show on the current application palette so runtime theme changes
    // cannot leave stale text or surface roles behind. The surface is opaque;
    // rounded corners come from the native window corner preference.
    // QCompleter creates its private popup while the editor is configured.
    // Qt marks that inherited palette as explicit, so preserving it can retain
    // a dark popup after the application switches to light (and vice versa).
    // It is framework-owned rather than an application override: always rebase
    // it on the current application palette.
    QPalette popupPalette = completerPopup ? QApplication::palette()
                                           : effectivePopupPalette(popup, QApplication::palette());
    const Private::Tokens popupTokens = Private::tokens(popupPalette);
    const QColor popupSurface = Private::popupSurfaceColor(popupPalette);
    // Symmetric re-show (branch-flip lane): when the previous cycle's DWM
    // grant is still live on this same HWND, preserve it — re-assert the
    // full composited recipe idempotently instead of tearing it down to
    // opaque and gambling on a fresh grant DWM refuses on reused HWNDs.
    // The tint is rebased from the current palette, so a theme switch
    // while hidden still converges. No live grant (first show, revoked
    // grant, offscreen) takes the existing rebuild + attempt path below.
    const bool keepComposited = Private::popupBackdropGrantAlive(popup);
    if (keepComposited) {
        // Keep-path text rebase: the DWM grant stays live on this reused
        // HWND, so the full rebuild above is skipped and the translucent
        // Window/Base tint below is re-asserted from the current palette.
        // Rebase only the non-background text roles from the fresh
        // popupPalette; Window/Base/Highlight stay untouched so the tint
        // and combo/completer contracts below are unaffected.
        QPalette rebased = popup->palette();
        rebased.setColor(QPalette::WindowText, popupPalette.color(QPalette::WindowText));
        rebased.setColor(QPalette::Text, popupPalette.color(QPalette::Text));
        rebased.setColor(QPalette::ButtonText, popupPalette.color(QPalette::ButtonText));
        rebased.setColor(QPalette::HighlightedText, popupPalette.color(QPalette::HighlightedText));
        popup->setPalette(rebased);
    }
    if (!keepComposited) {
        popupPalette.setColor(QPalette::Window, popupSurface);
        popupPalette.setColor(QPalette::Base, popupSurface);
        popup->setPalette(popupPalette);
        if (waylandMenuAlpha) {
            popup->setAutoFillBackground(false);
            popup->setAttribute(Qt::WA_TranslucentBackground, true);
            popup->setAttribute(Qt::WA_NoSystemBackground, true);
            popup->setAttribute(Qt::WA_OpaquePaintEvent, false);
            popup->setAttribute(Qt::WA_StyledBackground, false);
        } else {
            popup->setAutoFillBackground(true);
            popup->setAttribute(Qt::WA_TranslucentBackground, false);
            popup->setAttribute(Qt::WA_NoSystemBackground, false);
            popup->setAttribute(Qt::WA_StyledBackground, false);
            // Claim the whole popup paint: QMenu's native erase fills the Selected
            // row background full-bleed behind our inset pill, and nothing Qt-side
            // insets it. Our CE_MenuItem/PE_PanelMenu cover every pixel. Always
            // restore the opaque claim on the opaque path (not just for QMenu): a
            // hide-time DWM teardown clears it, and the next Show must repaint
            // opaquely again instead of inheriting a composited recipe with square
            // corners and a washed veil. The composited branch below clears it again
            // when acrylic actually applies, so this is a no-op there.
            popup->setAttribute(Qt::WA_OpaquePaintEvent, true);
        }
    }
    applyPopupRoundedCorners(popup);
    // WinUI Desktop Acrylic: on a live compositor the popup reveals a
    // blurred, wallpaper-tinted backdrop instead of the opaque fallback
    // above (ComboBox "Acrylic popup", MenuFlyout "Acrylic presenter" in the
    // acceptance matrix). Offscreen and failed DWM paths keep the opaque
    // surface, and the deterministic snapshots with it.
    bool compositedPopup = keepComposited;
    if (!keepComposited && QGuiApplication::platformName() != QStringLiteral("offscreen")) {
        if (WinUI3::applyBackdrop(popup, WinUI3::Backdrop::Acrylic)
            && backdropEffectiveSurface(popup) == BackdropSurface::Composited)
            compositedPopup = true;
    }
    // Grant re-assert past Show dispatch (DWM reads refuse there):
    // same-HWND keep re-applies the live grant; a fresh QMenu whose
    // sync attempt was refused re-attempts deferred. Coalesced per
    // popup for submenu storms; combo/completer excluded, native only.
    // Bounded retries: a reused HWND refusal can be transient (Show
    // dispatch / modal menu loop), so re-attempt up to N times with
    // growing delays before settling for the honest opaque fallback.
    // Without retries the second opening of a persistent menu (File,
    // split/menu-button popups) sticks opaque while the first holds
    // acrylic — the good-then-ugly shadow cycle.
    constexpr auto rearmAttemptsProperty = "_winui_backdrop_rearm_attempts";
    constexpr int rearmMaxAttempts = 4;
    if (QGuiApplication::platformName() != QStringLiteral("offscreen")
        && (keepComposited
            || (!keepComposited && !compositedPopup && qobject_cast<QMenu *>(widget)))) {
        if (!popup->property("_winui_backdrop_rearm").toBool()) {
            popup->setProperty("_winui_backdrop_rearm", true);
            if (popup->property(rearmAttemptsProperty).isNull())
                popup->setProperty(rearmAttemptsProperty, 0);
            const auto rearmStep = [popup] {
                const int attempt = popup->property(rearmAttemptsProperty).toInt();
                if (!popup || !popup->isWindow() || popup->windowType() != Qt::Popup
                    || !popup->isVisible() || !popup->windowHandle()) {
                    popup->setProperty("_winui_backdrop_rearm", {});
                    popup->setProperty(rearmAttemptsProperty, {});
                    return;
                }
                // Keep-path re-verifies Composited; the fresh path runs
                // only while still refused, then applies the same
                // composited tint semantics on success (failure: update).
                // Readback gate (reopened-menu refusal): DWM can ACK
                // Set(38) during Show dispatch while ignoring the value
                // (S_OK but Get(38) still reads 0/AUTO), so HRESULT
                // success alone must not stop the bounded retry. Only
                // S_OK-with-value-!=3 is a definitive refusal and clears
                // the grant for another attempt; a failed Get itself
                // (E_HANDLE and friends during Show dispatch on some
                // builds) is unreadable, not evidence, so that attempt
                // keeps the HRESULT-based acceptance. Unverified success
                // skips the translucent re-tint below so the opaque
                // fallback stands until truly granted.
                const bool keepPath =
                        backdropEffectiveSurface(popup) == BackdropSurface::Composited;
                bool granted = false;
                if (keepPath) {
                    granted = WinUI3::applyBackdrop(popup, WinUI3::Backdrop::Acrylic);
                    // Definitive refusal (S_OK-with-value-!=3): converge to
                    // the honest opaque fallback now (no translucent tint
                    // over a white backing store meanwhile) and fall through
                    // to the retry below. A later verified grant re-tints
                    // through the fresh branch. An unreadable Get keeps the
                    // HRESULT-based acceptance above (Refused is false there
                    // by construction).
                    if (granted && popupBackdropReadback(popup) == PopupBackdropReadback::Refused) {
                        granted = false;
                        convergePopupOpaqueFallback(popup);
                    }
                    popup->update();
                } else if (WinUI3::applyBackdrop(popup, WinUI3::Backdrop::Acrylic)
                           && backdropEffectiveSurface(popup) == BackdropSurface::Composited) {
                    const PopupBackdropReadback readback = popupBackdropReadback(popup);
                    if (readback == PopupBackdropReadback::Refused) {
                        // Definitive refusal: converge to the honest opaque
                        // fallback now (never the translucent tint over a
                        // white backing store) and force compositedPopup off
                        // so the tint branch below cannot re-apply it. The
                        // translucent re-tint runs only on a verified grant
                        // or an unreadable Get (which keeps the HRESULT
                        // acceptance like the keep path above). Retry below
                        // for QMenu; combos re-attempt at their next Show
                        // through the fresh path. (No compositedPopup flag
                        // to clear here: the Show-time tint branch already
                        // ran; the helper replaces its palette outright.)
                        convergePopupOpaqueFallback(popup);
                    } else {
                        granted = true;
                        QPalette translucent = popup->palette();
                        const Private::Tokens retint = Private::tokens(translucent);
                        QColor windowTint = Private::popupSurfaceColor(translucent);
                        windowTint.setAlpha(retint.dark ? 178 : 242);
                        translucent.setColor(QPalette::Window, windowTint);
                        QColor baseTint = windowTint;
                        translucent.setColor(QPalette::Base, baseTint);
                        popup->setPalette(translucent);
                        popup->setAutoFillBackground(false);
                        popup->setAttribute(Qt::WA_OpaquePaintEvent, false);
                        popup->setAttribute(Qt::WA_StyledBackground, true);
                        popup->update();
                    }
                } else {
                    popup->update();
                }
                if (granted || attempt + 1 >= rearmMaxAttempts) {
                    if (!granted
                        && popupBackdropReadback(popup) == PopupBackdropReadback::Refused) {
                        // Durable refusal: the cached Composited property
                        // would otherwise lie at every later Show (light
                        // bands over a white backing store). Converge to
                        // the honest opaque fallback; the next Show retries
                        // the grant through the fresh path. Unknown (failed
                        // Get) keeps the acceptance: DWM may still hold the
                        // effect, and tearing down a live grant flashes.
                        convergePopupOpaqueFallback(popup);
                    }
                    popup->setProperty("_winui_backdrop_rearm", {});
                    popup->setProperty(rearmAttemptsProperty, {});
                    return;
                }
                // Still refused and attempts remain: retry deferred with a
                // growing delay so a transient dispatch-time refusal can
                // converge instead of sticking opaque. The refused branch
                // of applyBackdrop already converged the Qt recipe to the
                // honest opaque fallback + shadow-only frame meanwhile.
                popup->setProperty(rearmAttemptsProperty, attempt + 1);
                const int delayMs = attempt == 0 ? 32 : attempt == 1 ? 96 : 200;
                QTimer::singleShot(delayMs, popup, [popup] {
                    if (!popup->property("_winui_backdrop_rearm").toBool()) {
                        return;
                    }
                    popup->setProperty("_winui_backdrop_rearm", false);
                    // Re-enter through the same Show-time entry so the
                    // coalesce flag is re-armed for the next attempt.
                    preparePopupSurface(popup);
                });
            };
            QTimer::singleShot(0, popup, rearmStep);
        }
    }
    if (compositedPopup) {
        // DWM supplies blur and tint; Qt paints translucent ink over it.
        // Never auto-fill here (opaque white band, same as window chrome).
        // Official transient-background recipe (issue #3478) resolves over
        // the fallback #2C2C2C (dark) / #FCFCFC (light), which doubles as
        // the visible surface when the compositor contributes no tint: a
        // near-solid paper, never a washed veil. Recompute from the
        // fallback rather than layering over the stale translucent roles.
        QPalette translucent = popup->palette();
        QColor windowTint = popupSurface;
        windowTint.setAlpha(popupTokens.dark ? 178 : 242);
        translucent.setColor(QPalette::Window, windowTint);
        QColor baseTint = windowTint;
        translucent.setColor(QPalette::Base, baseTint);
        popup->setPalette(translucent);
        popup->setAutoFillBackground(false);
        popup->setAttribute(Qt::WA_OpaquePaintEvent, false);
        popup->setAttribute(Qt::WA_StyledBackground, true);
    }
    if (!compositedPopup) {
        // Opaque fallback: demote any leaked Composited publish. A native
        // acrylic success arms it on the first show, and a hide-time DWM
        // teardown (or a checkable-toggle hide/show round-trip) can leave
        // it on the popup while the recipe above was rebuilt opaque. Every
        // painter gates its Source clear on Composited, so the stale publish
        // punches holes in the opaque backing and DWM keeps the shadow-less
        // translucent recipe for the next cycle (the gallery File/autoSave
        // reopen defect). applyBackdrop re-arms Painted/Composited itself on
        // the paths that need it, so clearing here is a no-op there.
        popup->setProperty(effectiveBackdropProperty, {});
    }
    // Calendar chrome follows the *effective* body recipe, not Qt::Popup
    // alone. Reapply on show/reopen as well as appearance refresh, after the
    // compositor decision. Inline calendars never enter this function.
    if (view && calendarView(view)) {
        auto *calendar = popup->findChild<QCalendarWidget *>();
        if (calendar) {
            const QColor surface = popup->palette().color(QPalette::Window);
            QPalette calendarPalette = calendar->palette();
            calendarPalette.setColor(QPalette::Window, surface);
            calendarPalette.setColor(QPalette::Base, surface);
            rememberPalette(calendar);
            calendar->setPalette(calendarPalette);
            if (auto *bar = calendar->findChild<QWidget *>(
                        QStringLiteral("qt_calendar_navigationbar"))) {
                rememberPalette(bar);
                remember(bar, originalAutoFillProperty, bar->autoFillBackground());
                QPalette headerPalette = popup->palette();
                headerPalette.setColor(QPalette::Button, surface);
                bar->setPalette(headerPalette);
                // PE_Widget rebuilds exactly one surface layer. Native
                // autofill would SourceOver a second tint onto the popup.
                bar->setAutoFillBackground(false);
                bar->setAttribute(Qt::WA_StyledBackground, true);
                for (QWidget *lane :
                     bar->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly)) {
                    rememberPalette(lane);
                    lane->setPalette(headerPalette);
                }
                bar->update();
            }
        }
    }
    const bool comboPopup = qobject_cast<QComboBox *>(popup->parentWidget());
    if (comboPopup) {
        // The inset is installed by polish before Qt computes the height.
        // Flush layout before anchoring without changing policy during Show.
        if (QLayout *layout = popup->layout())
            layout->activate();
    }
    if (auto *menu = qobject_cast<QMenu *>(widget)) {
        remember(popup, originalMarginsProperty, QVariant::fromValue(popup->contentsMargins()));
        menu->setContentsMargins(0, 2, 0, 2);
        // Submenu HWNDs materialize after Show (or the submenu never
        // resizes, so the Resize re-apply never fires): the DWM corner
        // preference above is a no-op for them and they stay square while
        // the top level rounds. The Qt rounded mask is the same
        // OverlayRadius on every platform, clips no geometry, and nests
        // harmlessly inside the live native rounding.
        applyMenuRoundedMask(menu);
    }
    if (view) {
        rememberPalette(view);
        // The completer popup IS the view window: its frame is the flyout
        // border. A styled frame paints a square PE_Frame over the rounded
        // popup surface prep, so the first/last hovered row's 4,2 pill sits
        // flush against a square edge — the reported ugly mouseover edges.
        // NoFrame drops that border (the DWM rounded region from
        // applyPopupRoundedCorners keeps the corners); the remembered shape
        // restores the stock frame on unpolish. Non-completer popups keep
        // their own frames: QMenu draws its stroke via PE_PanelMenu and the
        // combo container is frameless already.
        if (completerPopup) {
            if (auto *frame = qobject_cast<QFrame *>(view)) {
                remember(popup, originalFrameShapeProperty,
                         QVariant::fromValue(frame->frameShape()));
                frame->setFrameShape(QFrame::NoFrame);
            }
        }
        QPalette viewPalette =
                completerPopup ? popupPalette : effectivePopupPalette(view, popupPalette);
        if (calendarView(view)) {
            // QTableView paints its native rectangular selection underneath
            // the delegate. CalendarView uses its own rounded day chrome.
            viewPalette.setColor(QPalette::Highlight, Qt::transparent);
            viewPalette.setColor(QPalette::HighlightedText, popupTokens.textPrimary);
        }
        view->setPalette(viewPalette);
        rememberPalette(view->viewport());
        remember(view->viewport(), originalAutoFillProperty,
                 view->viewport()->autoFillBackground());
        remember(view->viewport(), originalOpaquePaintProperty,
                 view->viewport()->testAttribute(Qt::WA_OpaquePaintEvent));
        view->viewport()->setPalette(
                completerPopup ? viewPalette
                               : effectivePopupPalette(view->viewport(), viewPalette));
        view->viewport()->setAutoFillBackground(true);
        // A QCompleter's QListView is itself the native popup. Its delegate
        // paints translucent item-state layers rather than every viewport
        // pixel, so claiming an opaque paint event suppresses Qt's background
        // erase and retains old frames as dark Mica-like ghosts.
        view->viewport()->setAttribute(Qt::WA_OpaquePaintEvent, !completerPopup);
        if (compositedPopup) {
            // Acrylic presenter: the view and its viewport reveal the
            // composited popup tint instead of the opaque fallback above.
            // Never auto-fill (opaque white band) and never claim an opaque
            // paint event (retained frames); item delegates rebuild rows
            // from transparent (see CE_MenuItem/CE_ItemViewItem). Same
            // fallback-resolved tint as the popup window above.
            QColor viewTint = popupSurface;
            viewTint.setAlpha(popupTokens.dark ? 178 : 242);
            QPalette translucentView = view->palette();
            translucentView.setColor(QPalette::Base, viewTint);
            translucentView.setColor(QPalette::Window, viewTint);
            view->setPalette(translucentView);
            QPalette translucentViewport = view->viewport()->palette();
            translucentViewport.setColor(QPalette::Base, viewTint);
            translucentViewport.setColor(QPalette::Window, viewTint);
            view->viewport()->setPalette(translucentViewport);
            view->viewport()->setAutoFillBackground(false);
            view->viewport()->setAttribute(Qt::WA_OpaquePaintEvent, false);
        }
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
    const QModelIndex current = combo->model()->index(combo->currentIndex(), combo->modelColumn(),
                                                      combo->rootModelIndex());
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

} // namespace WinUI3::Private
