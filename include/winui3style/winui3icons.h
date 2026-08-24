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
    Warning
};

WINUI3STYLE_EXPORT QIcon icon(Icon glyph);
WINUI3STYLE_EXPORT QIcon icon(Icon glyph, const QColor &color);
WINUI3STYLE_EXPORT bool isFluentIcon(const QIcon &icon);
WINUI3STYLE_EXPORT QPixmap iconPixmap(const QIcon &icon, const QSize &size,
                                      qreal devicePixelRatio,
                                      const QColor &foreground,
                                      QIcon::Mode mode = QIcon::Normal,
                                      QIcon::State state = QIcon::Off);

} // namespace WinUI3
