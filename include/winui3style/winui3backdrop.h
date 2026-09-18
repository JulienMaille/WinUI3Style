// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <winui3style/winui3global.h>

class QWidget;

namespace WinUI3 {

enum class Backdrop { None, Mica, MicaAlt, Acrylic };

// Applies a Windows 11 backdrop material to a top-level window.
//
// Contract (enforced with an early return false where noted):
// - window must be non-null, alive, and a top-level window (isWindow());
//   any other value returns false without touching DWM state.
// - Must be called on the GUI thread; the implementation touches QWidget
//   attributes, palettes, and the native window handle.
// - Windows-only: on other platforms the call is a safe no-op that returns
//   false. Without DWM composition the DWM attributes are a no-op and the
//   transparent popup corners fall back to the compositor-less pipeline.
// - Idempotent per window: reapplying the same backdrop or disabling with
//   Backdrop::None is safe and restores the remembered opaque surface.
WINUI3STYLE_EXPORT bool applyBackdrop(QWidget *window, Backdrop backdrop);

} // namespace WinUI3
