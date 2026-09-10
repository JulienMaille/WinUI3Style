// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <winui3style/winui3backdrop.h>

#include <QVariant>
#include <QWidget>

namespace WinUI3::Private {

// Effective surface states for a requested window backdrop. Painters may
// clear pixels strictly in the Composited state; the Painted and Solid
// states are fully opaque.
enum class BackdropSurface { Solid = 0, Painted = 1, Composited = 2 };

// Single source of truth for the published effective state. The integer
// mapping matches BackdropSurface above.
inline constexpr auto effectiveBackdropProperty = "_winui_backdrop_effective";

// Published effective state read from the window's dynamic property.
// Queries only; never touches attributes, palettes or the native handle.
inline BackdropSurface backdropEffectiveSurface(const QWidget *window)
{
    if (!window)
        return BackdropSurface::Solid;
    const QVariant state = window->property(effectiveBackdropProperty);
    if (!state.isValid())
        return BackdropSurface::Solid;
    switch (state.toInt()) {
    case int(BackdropSurface::Composited):
        return BackdropSurface::Composited;
    case int(BackdropSurface::Painted):
        return BackdropSurface::Painted;
    default:
        return BackdropSurface::Solid;
    }
}

// Configure QWidget's surface before Qt creates the native popup handle.
// DWM attributes themselves are applied by applyBackdrop after WinIdChange.
void prepareBackdropSurface(QWidget *window, Backdrop backdrop);

// Round the corners of an opaque popup window. Prefers the Windows 11 corner
// preference so the native border clips cleanly; older builds fall back to a
// rounded window region.
void applyPopupRoundedCorners(QWidget *window);

// Clip an opaque frameless window (e.g. a tooltip) to a rounded rectangle of
// the given radius using a plain window region.
void applyWindowRoundedRegion(QWidget *window, int radius);

// Follows the application theme for a dialog window's native title bar: sets
// the immersive dark-mode attribute and title text color. No-op until the
// native handle exists and on platforms without DWM.
void applyDialogCaptionTheme(QWidget *window);

} // namespace WinUI3::Private
