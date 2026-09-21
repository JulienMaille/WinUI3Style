// SPDX-License-Identifier: LGPL-2.1-or-later
#include <winui3style/winui3backdrop.h>
#include <winui3style/winui3style.h>

#include "winui3backdrop_p.h"
#include "winui3tokens_p.h"
#include "winui3surfaces_p.h"

#include <QApplication>
#include <QGuiApplication>
#include <QPalette>
#include <QWidget>
#include <QWindow>

#ifdef Q_OS_WIN
#  define NOMINMAX
#  include <windows.h>
#  include <dwmapi.h>
#endif

namespace WinUI3 {

namespace {

constexpr auto backdropProperty = "_winui_backdrop";
constexpr auto originalWindowColorProperty = "_winui_original_window_color";
constexpr auto originalPaletteProperty = "_winui_backdrop_original_palette";
constexpr auto originalPaletteExplicitProperty = "_winui_backdrop_original_palette_explicit";
constexpr auto originalTranslucentProperty = "_winui_backdrop_original_translucent";
constexpr auto originalNoSystemBackgroundProperty = "_winui_backdrop_original_no_system_background";
constexpr auto originalOpaquePaintProperty = "_winui_backdrop_original_opaque_paint";
constexpr auto originalAutoFillProperty = "_winui_backdrop_original_auto_fill";

#ifdef Q_OS_WIN
QColor themedWindowColor()
{
    QColor result = QApplication::palette().color(QPalette::Window);
    if (const auto *style = qobject_cast<const Style *>(QApplication::style()))
        result = style->standardPalette().color(QPalette::Window);
    return result;
}
#endif

void rememberBackdropState(QWidget *window)
{
    const auto remember = [window](const char *name, const QVariant &value) {
        if (!window->property(name).isValid())
            window->setProperty(name, value);
    };
    remember(originalPaletteProperty, QVariant::fromValue(window->palette()));
    remember(originalPaletteExplicitProperty, window->testAttribute(Qt::WA_SetPalette));
    remember(originalTranslucentProperty, window->testAttribute(Qt::WA_TranslucentBackground));
    remember(originalNoSystemBackgroundProperty, window->testAttribute(Qt::WA_NoSystemBackground));
    remember(originalOpaquePaintProperty, window->testAttribute(Qt::WA_OpaquePaintEvent));
    remember(originalAutoFillProperty, window->autoFillBackground());
    remember(originalWindowColorProperty, window->palette().color(QPalette::Window));
}

// Switching the presentation mode rebuilds the whole buffer: stale frames
// would otherwise linger under the new material. update() alone does not
// dirty already-clean children, so force a synchronous full repaint of the
// window hierarchy here. A parent repaint clips children out, so every
// widget repaints itself. Must run only after the target palettes converged
// and the effective state was published, or painters rebuild from stale
// roles (transparent window paint onto a window becoming opaque).
void repaintBackdropHierarchy(QWidget *window)
{
    if (!window)
        return;
    window->repaint();
    const QList<QWidget *> subtree = window->findChildren<QWidget *>();
    for (QWidget *child : subtree) {
        if (child->isVisible())
            child->repaint();
    }
}

void restoreBackdropState(QWidget *window)
{
    window->setProperty(Private::effectiveBackdropProperty,
                        static_cast<int>(Private::BackdropSurface::Solid));
    if (window->property(originalTranslucentProperty).isValid())
        window->setAttribute(Qt::WA_TranslucentBackground,
                             window->property(originalTranslucentProperty).toBool());
    if (window->property(originalNoSystemBackgroundProperty).isValid())
        window->setAttribute(Qt::WA_NoSystemBackground,
                             window->property(originalNoSystemBackgroundProperty).toBool());
    if (window->property(originalOpaquePaintProperty).isValid())
        window->setAttribute(Qt::WA_OpaquePaintEvent,
                             window->property(originalOpaquePaintProperty).toBool());
    if (window->property(originalAutoFillProperty).isValid())
        window->setAutoFillBackground(window->property(originalAutoFillProperty).toBool());
    if (window->property(originalPaletteProperty).isValid()) {
        if (window->property(originalPaletteExplicitProperty).toBool())
            window->setPalette(window->property(originalPaletteProperty).value<QPalette>());
        else
            window->setPalette(QPalette());
    }
    window->setProperty(backdropProperty, {});
    window->setProperty(originalWindowColorProperty, {});
    window->setProperty(originalPaletteProperty, {});
    window->setProperty(originalPaletteExplicitProperty, {});
    window->setProperty(originalTranslucentProperty, {});
    window->setProperty(originalNoSystemBackgroundProperty, {});
    window->setProperty(originalOpaquePaintProperty, {});
    window->setProperty(originalAutoFillProperty, {});
}

} // namespace

namespace Private {

void applyWindowRoundedRegion(QWidget *window, int radius);

bool popupBackdropGrantAlive(QWidget *window)
{
#ifdef Q_OS_WIN
    if (!window || !window->isWindow() || window->windowType() != Qt::Popup)
        return false;
    if (QGuiApplication::platformName() == QStringLiteral("offscreen"))
        return false;
    // Composited alone can outlive a destroyed HWND. Keep only a live,
    // existing platform window; never create one to evaluate this predicate.
    QWindow *nativeWindow = window->windowHandle();
    if (!nativeWindow || !nativeWindow->handle()
        || !IsWindow(reinterpret_cast<HWND>(window->internalWinId())))
        return false;
    const HWND hwnd = reinterpret_cast<HWND>(nativeWindow->winId());
    return IsWindow(hwnd) && backdropEffectiveSurface(window) == BackdropSurface::Composited;
#else
    Q_UNUSED(window)
    return false;
#endif
}

void prepareBackdropSurface(QWidget *window, Backdrop backdrop)
{
#ifdef Q_OS_WIN
    if (!window || !window->isWindow() || backdrop == Backdrop::None)
        return;

    rememberBackdropState(window);
    window->setProperty(backdropProperty, static_cast<int>(backdrop));

    // Qt documents WA_TranslucentBackground as a pre-create attribute on
    // Windows. Popup HWNDs are created lazily between polish and Show, so the
    // alpha-capable surface must be configured before that lifetime boundary.
    // The offscreen QPA has no HWND or DWM surface; changing its backing-store
    // alpha mode would alter the deterministic fallback PNGs instead.
    if (QGuiApplication::platformName() != QStringLiteral("offscreen")) {
        window->setAttribute(Qt::WA_TranslucentBackground, true);
        window->setAttribute(Qt::WA_NoSystemBackground, true);
        window->setAttribute(Qt::WA_OpaquePaintEvent, false);
        window->setAutoFillBackground(false);
    }
#else
    Q_UNUSED(window)
    Q_UNUSED(backdrop)
#endif
}

void applyPopupRoundedCorners(QWidget *window)
{
#ifdef Q_OS_WIN
    if (!window || !window->isWindow())
        return;
    // Offscreen captures have no HWND and must not depend on the native
    // corner; the painted surface keeps deterministic PNGs.
    if (QGuiApplication::platformName() == QStringLiteral("offscreen"))
        return;
    const HWND hwnd = reinterpret_cast<HWND>(window->winId());
    // DWMWA_WINDOW_CORNER_PREFERENCE rounds an opaque window's native border
    // on Windows 11 (build 22000+); unknown attributes fail harmlessly on
    // older builds, which then use the region fallback below.
    constexpr DWORD cornerPreferenceAttribute = 33;
    constexpr int cornerRound = 2; // DWMWCP_ROUND
    if (SUCCEEDED(DwmSetWindowAttribute(hwnd, cornerPreferenceAttribute, &cornerRound,
                                        sizeof(cornerRound))))
        return;
    applyWindowRoundedRegion(window, OverlayRadius);
#else
    Q_UNUSED(window)
#endif
}

void applyWindowRoundedRegion(QWidget *window, int radius)
{
#ifdef Q_OS_WIN
    if (!window || !window->isWindow())
        return;
    if (QGuiApplication::platformName() == QStringLiteral("offscreen"))
        return;
    const HWND hwnd = reinterpret_cast<HWND>(window->winId());
    const QRect rect = window->rect();
    const HRGN region =
            CreateRoundRectRgn(0, 0, rect.width() + 1, rect.height() + 1, radius * 2, radius * 2);
    if (region && !SetWindowRgn(hwnd, region, TRUE))
        DeleteObject(region); // ownership transfers only on success
#else
    Q_UNUSED(window)
    Q_UNUSED(radius)
#endif
}

void applyDialogCaptionTheme(QWidget *window)
{
#ifdef Q_OS_WIN
    if (!window || !window->isWindow() || !window->windowHandle())
        return;
    if (QGuiApplication::platformName() == QStringLiteral("offscreen"))
        return;
    const HWND hwnd = reinterpret_cast<HWND>(window->winId());
    constexpr DWORD immersiveDarkModeAttribute = 20;
    constexpr DWORD captionColorAttribute = 35;
    constexpr DWORD textColorAttribute = 36;
    // Caption and text are an inseparable pair. Leaving the native accent
    // caption in place while forcing dark text produces black-on-blue title
    // bars in a light application theme. Match the caption to the dialog
    // surface and derive the foreground from that exact color.
    const QColor surface = themedWindowColor();
    const BOOL dark = qGray(surface.rgb()) < 128;
    DwmSetWindowAttribute(hwnd, immersiveDarkModeAttribute, &dark, sizeof(dark));
    const COLORREF caption = RGB(surface.red(), surface.green(), surface.blue());
    DwmSetWindowAttribute(hwnd, captionColorAttribute, &caption, sizeof(caption));
    const COLORREF text = dark ? RGB(255, 255, 255) : RGB(0, 0, 0);
    DwmSetWindowAttribute(hwnd, textColorAttribute, &text, sizeof(text));
#else
    Q_UNUSED(window)
#endif
}

} // namespace Private

bool applyBackdrop(QWidget *window, Backdrop backdrop)
{
#ifdef Q_OS_WIN
    if (!window || !window->isWindow())
        return false;

    // Remember state even for a direct None request. This keeps disabling an
    // unknown/external backdrop idempotent instead of manufacturing an
    // explicit palette or changing QWidget attributes as a side effect.
    rememberBackdropState(window);
    if (backdrop != Backdrop::None)
        Private::prepareBackdropSurface(window, backdrop);
    window->setProperty(backdropProperty, static_cast<int>(backdrop));
    const bool nativeSurface = QGuiApplication::platformName() != QStringLiteral("offscreen");

    if (backdrop == Backdrop::None) {
        // A popup may have been polished (and its alpha surface prepared) but
        // never shown. Do not manufacture an HWND during unpolish/destruction
        // merely to clear DWM attributes that were never applied.
        if (!window->windowHandle()) {
            restoreBackdropState(window);
            return true;
        }
        window->setAttribute(Qt::WA_TranslucentBackground, false);
        window->setAttribute(Qt::WA_NoSystemBackground, false);
        window->setAttribute(Qt::WA_OpaquePaintEvent, false);
        window->setAutoFillBackground(true);
        // Disabling is fully native teardown: chrome returns to its
        // remembered opaque palettes. No translucent pixel may survive.
        // The full-buffer repaint is deferred below until the opaque Window
        // palette converges and restoreBackdropState publishes Solid: every
        // paint must rebuild from restored roles, never from the
        // transparent-era ones still live at this point.
        Private::restoreChromeSurfaces(window);
        Private::restoreContentSurfacesForBackdrop(window);
    } else {
        // These paint-surface flags are needed even when the platform has no
        // DWM compositor, so the offscreen fallback retains its transparent
        // popup corners. The translucent HWND flag is handled by the
        // pre-create helper only for native Windows surfaces.
        window->setAttribute(Qt::WA_NoSystemBackground, true);
        window->setAttribute(Qt::WA_OpaquePaintEvent, false);
        window->setAutoFillBackground(false);
    }

    QPalette materialPalette = window->palette();
    const QColor themedWindowColor = WinUI3::themedWindowColor();
    QColor windowColor = backdrop == Backdrop::None
            ? window->property(originalWindowColorProperty).value<QColor>()
            : themedWindowColor;
    windowColor.setAlpha(backdrop == Backdrop::None ? 255 : 0);
    materialPalette.setColor(QPalette::Window, windowColor);
    window->setPalette(materialPalette);
    if (backdrop != Backdrop::None && nativeSurface && window->windowType() == Qt::Popup) {
        // Popup convergence (live compositor only; the offscreen fallback
        // below keeps its opaque painted surface): resolve Window/Base on
        // that preparePopupSurface's composited branch paints from (popup
        // surface, alpha 178 dark / 242 light). applyBackdrop is re-entered
        // from the WinIdChange handler when the Show-time winId() creates
        // the popup HWND; that re-entry used to leave Window fully
        // transparent, so the first open settled on a different grey than
        // the reopen (whose HWND already exists, keeping the Show-time
        // tint). Converging here makes every entry point — Show-time prep,
        // WinIdChange re-entry, reopen — resolve identical roles, and the
        // composited branch in preparePopupSurface re-asserts the same tint
        // idempotently. The base mirrors preparePopupSurface exactly
        // (remembered original resolved over the application palette), so
        // the WinIdChange value is bit-identical to the Show-time value.
        // Main-window Mica keeps the transparent Window role (the live
        // material shows through), so this stays popup-only.
        const QPalette popupBase = Private::effectivePopupPalette(window, QApplication::palette());
        QColor popupTint = Private::popupSurfaceColor(popupBase);
        popupTint.setAlpha(qGray(themedWindowColor.rgb()) < 128 ? 178 : 242);
        materialPalette.setColor(QPalette::Window, popupTint);
        materialPalette.setColor(QPalette::Base, popupTint);
        window->setPalette(materialPalette);
    }

    if (!nativeSurface) {
        if (backdrop == Backdrop::None) {
            restoreBackdropState(window);
            // Deferred disable repaint (same full-buffer rationale as the
            // enable path): every widget repaints from its restored opaque
            // palette now that Solid is published. Repainting earlier kept
            // the last composited frame on screen and rebuilt child backing
            // stores from transparent-era roles.
            repaintBackdropHierarchy(window);
        } else {
            // Deterministic offscreen snapshots: opaque painted surface and
            // an explicit Painted state; painters must never clear here.
            window->setProperty(Private::effectiveBackdropProperty,
                                static_cast<int>(Private::BackdropSurface::Painted));
        }
        return true;
    }

    constexpr DWORD systemBackdropAttribute = 38;
    constexpr DWORD useHostBackdropBrushAttribute = 17;
    constexpr DWORD immersiveDarkModeAttribute = 20;
    constexpr DWORD borderColorAttribute = 34;
    constexpr DWORD captionColorAttribute = 35;
    constexpr DWORD textColorAttribute = 36;
    constexpr DWORD redirectionBitmapAlphaAttribute = 39;
    constexpr COLORREF colorNone = 0xFFFFFFFE;
    constexpr int backdropAuto = 0;
    constexpr int backdropNone = 1;
    constexpr int backdropMainWindow = 2;
    constexpr int backdropTransientWindow = 3;
    constexpr int backdropTabbedWindow = 4;

    int value = backdropNone;
    switch (backdrop) {
    case Backdrop::None:
        value = backdropNone;
        break;
    case Backdrop::Mica:
        value = backdropMainWindow;
        break;
    case Backdrop::MicaAlt:
        value = backdropTabbedWindow;
        break;
    case Backdrop::Acrylic:
        value = backdropTransientWindow;
        break;
    default:
        value = backdropAuto;
        break;
    }

    // Resolve the current platform HWND on each application/rearm, not the
    // QWidget cache. Preserve first-time creation for the public API only
    // when there is no platform window yet.
    QWindow *nativeWindow = window->windowHandle();
    const HWND hwnd = reinterpret_cast<HWND>(
            nativeWindow && nativeWindow->handle() ? nativeWindow->winId() : window->winId());
    const BOOL dark = qGray(themedWindowColor.rgb()) < 128;
    DwmSetWindowAttribute(hwnd, immersiveDarkModeAttribute, &dark, sizeof(dark));
    const COLORREF caption = backdrop == Backdrop::None
            ? RGB(themedWindowColor.red(), themedWindowColor.green(), themedWindowColor.blue())
            : colorNone;
    const COLORREF text = dark ? RGB(255, 255, 255) : RGB(0, 0, 0);
    DwmSetWindowAttribute(hwnd, borderColorAttribute, &colorNone, sizeof(colorNone));
    DwmSetWindowAttribute(hwnd, captionColorAttribute, &caption, sizeof(caption));
    DwmSetWindowAttribute(hwnd, textColorAttribute, &text, sizeof(text));
    // DWMWA_USE_HOSTBACKDROPBRUSH opts into a Win32 Composition brush created
    // by the application. Qt does not create that brush; enabling it here
    // leaves the client surface empty. System backdrops are instead drawn by
    // DWMWA_SYSTEMBACKDROP_TYPE, so explicitly disable the host-brush path.
    const BOOL useHostBackdrop = FALSE;
    DwmSetWindowAttribute(hwnd, useHostBackdropBrushAttribute, &useHostBackdrop,
                          sizeof(useHostBackdrop));
    // Qt's translucent backing store is premultiplied ARGB. This attribute is
    // available on newer Windows 11 builds; older builds simply reject it,
    // while the system-backdrop result remains authoritative below.
    const BOOL useRedirectionAlpha = backdrop != Backdrop::None;
    DwmSetWindowAttribute(hwnd, redirectionBitmapAlphaAttribute, &useRedirectionAlpha,
                          sizeof(useRedirectionAlpha));
    const HRESULT backdropResult =
            DwmSetWindowAttribute(hwnd, systemBackdropAttribute, &value, sizeof(value));

    MARGINS margins{};
    if (backdrop != Backdrop::None)
        margins = { -1, -1, -1, -1 };
    const HRESULT frameResult = DwmExtendFrameIntoClientArea(hwnd, &margins);
    const bool applied = SUCCEEDED(backdropResult) && SUCCEEDED(frameResult);
    if (backdrop == Backdrop::None) {
        restoreBackdropState(window);
        // Same deferred-repaint contract as the offscreen None path above:
        // the hierarchy rebuilds only after Solid is published and the
        // opaque palette converged, so no painter observes stale roles.
        repaintBackdropHierarchy(window);
    } else if (applied) {
        // Window chrome (menu bar, tool bars, status bar) reveals the live
        // material instead of painting opaque panels over it. Content islands
        // follow under the full-Mica contract (sync is a no-op for windows
        // without opted-in descendants).
        Private::makeChromeSurfacesTransparent(window);
        Private::syncContentSurfacesForBackdrop(window);
        // Publish Composited only once chrome and islands converged: the
        // publish notifies owned-palette refreshes (notably the inline
        // calendar surface, which veils through paintsDirectlyOnBackdrop),
        // and that gate reads the island alphas the sync above just wrote.
        // Publishing first would refresh against stale opaque islands and
        // stick the calendar opaque under a granted material.
        window->setProperty(Private::effectiveBackdropProperty,
                            static_cast<int>(Private::BackdropSurface::Composited));
        repaintBackdropHierarchy(window);
    } else {
        // DWM refused the material: opaque painted fallback, explicit
        // Painted state so clears stay off (black/stale pixels otherwise).
        window->setProperty(Private::effectiveBackdropProperty,
                            static_cast<int>(Private::BackdropSurface::Painted));
        if (window->property(originalTranslucentProperty).isValid())
            window->setAttribute(Qt::WA_TranslucentBackground,
                                 window->property(originalTranslucentProperty).toBool());
        window->setAttribute(Qt::WA_NoSystemBackground, false);
        if (window->property(originalOpaquePaintProperty).isValid())
            window->setAttribute(Qt::WA_OpaquePaintEvent,
                                 window->property(originalOpaquePaintProperty).toBool());
        window->setAutoFillBackground(true);
        QPalette fallback = window->palette();
        QColor fallbackWindowColor = themedWindowColor;
        fallbackWindowColor.setAlpha(255);
        fallback.setColor(QPalette::Window, fallbackWindowColor);
        if (window->windowType() == Qt::Popup) {
            // Refused popup re-attempt (reused HWND): converge the pair on
            // the SAME popup flyout surface the opaque branch of
            // preparePopupSurface just claimed, not the themed main-window
            // grey over a stale flyout Base — otherwise a reopened File
            // menu resolves Window and Base to two different greys (the
            // live wrong-background reopen defect). Main windows keep the
            // themed role above; this stays popup-only like the tint block.
            const QPalette popupBase =
                    Private::effectivePopupPalette(window, QApplication::palette());
            QColor popupSurface = Private::popupSurfaceColor(popupBase);
            popupSurface.setAlpha(255);
            fallback.setColor(QPalette::Window, popupSurface);
            fallback.setColor(QPalette::Base, popupSurface);
            // Second-menu parity: a refused HWND paints the same token pill
            // (subtleHover over ink) as its granted sibling, so the pill ink
            // must resolve identically — rebase the text roles from the same
            // fresh popup resolution as the surface above (mirrors the
            // keep-path rebase in preparePopupSurface). Stale text roles
            // from a previous granted cycle would otherwise tint the pill
            // differently across the two HWNDs. Roles only: no paint
            // branching, no DWM re-attempt; offscreen never reaches here.
            fallback.setColor(QPalette::WindowText, popupBase.color(QPalette::WindowText));
            fallback.setColor(QPalette::Text, popupBase.color(QPalette::Text));
            fallback.setColor(QPalette::ButtonText, popupBase.color(QPalette::ButtonText));
            fallback.setColor(QPalette::HighlightedText,
                              popupBase.color(QPalette::HighlightedText));
            // A previous cycle's grant (SYSTEMBACKDROP_TYPE =
            // TransientWindow + extended frame + redirection alpha) can
            // still be armed on this reused HWND even though DWM refused
            // to re-issue it: the stale half-torn-down frame is exactly
            // the ugly reopened-popup edge and shadow. Fall back to a
            // shadow-only opaque frame so the refused cycle keeps its
            // standard DWM shadow without re-requesting TransientWindow:
            // disarm the material, extend a 1px frame (zero would kill
            // the shadow), keep DWMWCP_ROUND, clear redirection alpha.
            // On a fresh handle whose first attempt failed this arms
            // only the shadow frame (the same attributes simply fail or
            // no-op harmlessly).
            const int disarmBackdrop = backdropNone;
            DwmSetWindowAttribute(hwnd, systemBackdropAttribute, &disarmBackdrop,
                                  sizeof(disarmBackdrop));
            const MARGINS shadowFrameMargins{ 1, 1, 1, 1 };
            DwmExtendFrameIntoClientArea(hwnd, &shadowFrameMargins);
            constexpr DWORD cornerPreferenceAttribute = 33;
            constexpr int cornerRoundPreference = 2; // DWMWCP_ROUND
            DwmSetWindowAttribute(hwnd, cornerPreferenceAttribute, &cornerRoundPreference,
                                  sizeof(cornerRoundPreference));
            const BOOL noRedirectionAlpha = FALSE;
            DwmSetWindowAttribute(hwnd, redirectionBitmapAlphaAttribute, &noRedirectionAlpha,
                                  sizeof(noRedirectionAlpha));
        }
        window->setPalette(fallback);
    }
    return applied;
#else
    Q_UNUSED(window)
    Q_UNUSED(backdrop)
    return false;
#endif
}

} // namespace WinUI3
