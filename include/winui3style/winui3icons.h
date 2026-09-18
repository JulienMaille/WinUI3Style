// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <winui3style/winui3global.h>

#include <QIcon>
#include <QColor>
#include <QPixmap>

namespace WinUI3 {

enum class Icon {
    Add,
    Back,
    Check,
    ChevronDown,
    ChevronLeft,
    ChevronRight,
    ChevronUp,
    Clear,
    Close,
    Delete,
    Edit,
    Error,
    Folder,
    Help,
    Home,
    Info,
    More,
    Pause,
    Play,
    Refresh,
    Save,
    Search,
    Settings,
    Stop,
    Document,
    List,
    Warning
};

WINUI3STYLE_EXPORT QIcon icon(Icon glyph);
WINUI3STYLE_EXPORT QIcon icon(Icon glyph, const QColor &color);
// Best-effort tag check, not a security boundary. QIcon is an
// implicitly-shared value type with no user-extensible tag, so
// discrimination is name-based: true requires the reserved
// "winui3-fluent-icon:<n>" engine name with an in-range glyph index.
// A caller that forges that name (setName()/fromTheme()/custom engine)
// will read as Fluent (false positive); on Qt 5 the engine name override
// does not exist, so this always returns false there. Do not use it to
// validate untrusted icons.
WINUI3STYLE_EXPORT bool isFluentIcon(const QIcon &icon);
// GUI-thread only: returns a QPixmap, which QPixmap construction ties to
// the GUI thread (off-thread/background pixmap generation via
// QtConcurrent or list-model decoration workers is not supported).
// Empty size returns a null pixmap; DPR <= 0 is treated as 1.0; an
// invalid foreground falls back to the uncoloured engine rendering.
WINUI3STYLE_EXPORT QPixmap iconPixmap(const QIcon &icon, const QSize &size, qreal devicePixelRatio,
                                      const QColor &foreground, QIcon::Mode mode = QIcon::Normal,
                                      QIcon::State state = QIcon::Off);

} // namespace WinUI3
