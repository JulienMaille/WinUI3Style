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
    Warning,
    // Appended after Warning so every existing enumerator keeps its index and
    // therefore its "winui3-fluent-icon:<n>" engine name.
    Restore,
    // Pinned DataGrid column-header sort glyphs (SortIconAscending E74A /
    // SortIconDescending E74B in the WinUI DataGrid template).
    SortAscending,
    SortDescending,
    // Dialog standard-button glyphs; appended to preserve existing indices.
    Accept,
    ReturnKey,
    // Issue #69: exact Fluent counterparts for standard pixmaps that had
    // none. Appended so every existing glyph keeps its engine index. Code
    // points and names come from the pinned Segoe Fluent Icons font list.
    Forward, // E72A Forward
    FolderOpen, // E838 FolderOpen
    Volume, // E767 Volume
    Mute, // E74F Mute
    Previous, // E892 Previous
    Next, // E893 Next
    Rewind, // EB9E Rewind
    FastForward, // EB9D FastForward
    Minimize, // E921 ChromeMinimize
    Maximize, // E922 ChromeMaximize
    Shield, // EA18 Shield
    Monitor, // E7FB DeviceMonitorNoPic
    StorageOptical, // E958 StorageOptical
    HardDrive, // EDA2 HardDrive
    ChromeClose, // E8BB ChromeClose (title-bar close, distinct from Cancel E711)
    // Issue #69 hand-picked replacements (pinned Segoe Fluent Icons values).
    ButtonMenu, // EDE3 ButtonMenu
    ErrorBadge, // EA39 ErrorBadge
    NewFolder, // E8F4 NewFolder
    GetHelpApp, // EA6B GetHelpApp
    MapDrive, // E8CE MapDrive (network/mapped drive)
    CaretSolidUp, // F090 CaretSolidUp (title-bar shade button)
    CaretSolidDown, // F08E CaretSolidDown (title-bar unshade button)
    // Sentinel: the number of real glyphs. Lookups and the icon cache size
    // must use this, not the last named glyph (issue #69: stopping at
    // SortDescending silently dropped Accept and ReturnKey).
    Count
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
