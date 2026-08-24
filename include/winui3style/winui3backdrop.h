#pragma once

#include <winui3style/winui3global.h>

class QWidget;

namespace WinUI3 {

enum class Backdrop {
    None,
    Mica,
    MicaAlt,
    Acrylic
};

WINUI3STYLE_EXPORT bool applyBackdrop(QWidget *window, Backdrop backdrop);

} // namespace WinUI3

