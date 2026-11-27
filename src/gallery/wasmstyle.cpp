// SPDX-License-Identifier: LGPL-2.1-or-later

#if defined(EMSCRIPTEN)

#  include <winui3style/winui3style.h>

#  include <QStyle>

// Qt for WebAssembly ships without a runtime plugin loader, so the browser
// gallery embeds the style directly instead of loading it through
// QStyleFactory (docs/wasm.md). Kept in its own translation unit so the
// designer gallery contract keeps the desktop client plugin-only.
QStyle *embeddedGalleryStyle();

QStyle *embeddedGalleryStyle()
{
    return new WinUI3::Style(WinUI3::ThemeMode::Light);
}

#endif
