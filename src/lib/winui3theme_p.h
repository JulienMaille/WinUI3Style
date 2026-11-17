// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <winui3style/winui3global.h>

#include <QColor>
#include <QPalette>

namespace WinUI3::Private {

struct SystemAccentRamp
{
    QColor accent;
    QColor light1;
    QColor light2;
    QColor dark1;
};

bool systemUsesDarkTheme();
SystemAccentRamp systemAccentRamp();
QColor systemAccentColor();

// Effective Windows animation preference (SPI_GETCLIENTAREAANIMATION, the same
// signal as WinRT UISettings.AnimationsEnabled / the Settings "Animation
// effects" switch). True off Windows and when the native query fails: an
// unreadable preference must never silently remove motion. Same cache and
// invalidation contract as the theme/accent probes above.
WINUI3STYLE_EXPORT bool systemAnimationsAllowed();

// Threading contract: every function in this header must be called from the
// GUI thread. The Windows appearance cache behind these queries (see
// winui3theme_p.cpp) is mutex-guarded so concurrent cache reads/writes are
// safe, but the underlying probes are GUI-thread-only Qt APIs: QSettings
// native-registry access and DWM queries on Windows, QStyleHints /
// QApplication::palette() on other platforms. The native
// SystemAppearanceWatcher already invokes
// invalidateSystemAppearanceCache() on the GUI thread; background threads
// must post to the GUI thread instead of calling these directly.

// Drops the cached Windows appearance values so the next theme/accent query
// observes the values published by the shell. This is a no-op on platforms
// where the appearance helpers do not maintain a native cache.
WINUI3STYLE_EXPORT void invalidateSystemAppearanceCache();

// Builds the application palette for the given theme.
// Accent roles: when explicitAccent is false the accent argument is IGNORED
// and every accent-derived role (Highlight, Link, LinkVisited, and Accent on
// Qt >= 6.6) comes from the live system ramp (systemAccentRamp()), keeping
// one coherent source per METHODOLOGY section 2. When explicitAccent is true
// the accent argument drives those roles (Highlight/Link = accent,
// LinkVisited = accent.darker(112), Accent = the theme-mapped AccentFill
// ramp entry). An invalid/null accent is never published: it degrades to the
// !explicitAccent path (system ramp) so Highlight/Link cannot go invalid
// while AccentFill stays on the system ramp (a mixed-source violation).
QPalette standardPalette(bool darkTheme, const QColor &accent, bool explicitAccent);

} // namespace WinUI3::Private
