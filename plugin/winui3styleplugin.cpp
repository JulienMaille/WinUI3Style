#include "winui3styleplugin.h"

#include <winui3style/winui3style.h>

QStyle *WinUI3StylePlugin::create(const QString &key)
{
    if (key.compare(QStringLiteral("winui3"), Qt::CaseInsensitive) == 0)
        return new WinUI3::Style;
    return nullptr;
}
