#include "winui3styleplugin.h"

#include <winui3style/winui3style.h>

QStyle *WinUI3StylePlugin::create(const QString &key)
{
    if (key.compare(QStringLiteral("winui3"), Qt::CaseInsensitive) == 0)
        return new WinUI3::Style;
    if (key.compare(QStringLiteral("winui3compact"), Qt::CaseInsensitive) == 0)
        return new WinUI3::Style(WinUI3::ThemeMode::System,
                                 WinUI3::DensityMode::Compact);
    return nullptr;
}
