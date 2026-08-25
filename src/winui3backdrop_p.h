#pragma once

#include <winui3style/winui3backdrop.h>

class QWidget;

namespace WinUI3::Private {

// Configure QWidget's surface before Qt creates the native popup handle.
// DWM attributes themselves are applied by applyBackdrop after WinIdChange.
void prepareBackdropSurface(QWidget *window, Backdrop backdrop);

} // namespace WinUI3::Private
