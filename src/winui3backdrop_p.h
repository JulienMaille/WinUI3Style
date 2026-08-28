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

} // namespace WinUI3::Private
