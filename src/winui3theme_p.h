#pragma once

#include <QColor>
#include <QPalette>

namespace WinUI3::Private {

struct SystemAccentRamp
{
    QColor accent;
    QColor light2;
    QColor dark1;
};

bool systemUsesDarkTheme();
SystemAccentRamp systemAccentRamp();
QColor systemAccentColor();

QPalette standardPalette(bool darkTheme, const QColor &accent,
                         bool explicitAccent);

} // namespace WinUI3::Private
