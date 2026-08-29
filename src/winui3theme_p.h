#pragma once

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

// Drops the cached Windows appearance values so the next theme/accent query
// observes the values published by the shell. This is a no-op on platforms
// where the appearance helpers do not maintain a native cache.
void invalidateSystemAppearanceCache();

QPalette standardPalette(bool darkTheme, const QColor &accent,
                         bool explicitAccent);

} // namespace WinUI3::Private
