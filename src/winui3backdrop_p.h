#pragma once

#include <winui3style/winui3backdrop.h>

class QWidget;

namespace WinUI3::Private {

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
