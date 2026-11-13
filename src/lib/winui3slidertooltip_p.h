// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <winui3style/winui3global.h>

class QSlider;

namespace WinUI3 {
namespace Private {

// Slider value tooltip surface. Definitions live in winui3slidertooltip_p.cpp;
// the debounce/tool-window plumbing stays file-local there.
void showSliderValueToolTip(QSlider *slider);
void hideSliderValueToolTip(QSlider *slider);

} // namespace Private
} // namespace WinUI3
