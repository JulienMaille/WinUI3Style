// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

// WinUI typography mapping (issue #112).
//
// TextBlock_themeresources.xaml (microsoft-ui-xaml 9f89c2da) gives
// BaseTextBlockStyle the generic XamlAutoFontFamily and SemiBold weight;
// TitleTextBlockStyle (28 px) and BodyStrongTextBlockStyle (14 px) inherit both
// and override only the size. XamlAutoFontFamily is the variable Segoe UI
// Variable family, whose optical-size axis follows the requested pixel size.
//
// Qt's Generic problem here is concrete: `Segoe UI Variable Text`, the family
// installed as the Qt 6 application font, has no Semibold face, so requesting
// QFont::DemiBold (600) resolves to Bold with exactMatch() == false. The generic
// `Segoe UI Variable` family resolves 600 to the exact Semibold face. This
// header owns that one choice so every title/body-strong site agrees.

#include "winui3qtcompat_p.h"
#include "winui3tokens_p.h"

#include <QFont>
#include <QFontInfo>
#include <QString>

namespace WinUI3::Private {

// The generic variable family when the host exposes it, the static Segoe UI
// face on Qt 5 / older Windows, and empty where neither exists (the caller then
// keeps the application's family).
inline QString winuiVariableFontFamily()
{
    // Only the real variable family carries the optical-size axis WinUI uses.
    // Hosts without it (older Windows / CI images) keep the application font
    // untouched: a substitution would change their approved rasters and would
    // not reproduce the pinned WinUI outlines anyway.
    const QStringList families = fontFamilies();
    if (families.contains(QLatin1String(VariableFontFamily)))
        return QLatin1String(VariableFontFamily);
    return QString();
}

// Build a WinUI strong (SemiBold) font from `base`. When `pixelSize` is 0 the
// base's own pixel size is kept (controlLabelFont callers have already applied
// density). An explicit application font family is never replaced: only the
// style's own default families are promoted to the generic variable family.
inline QFont winuiStrongFont(const QFont &base, int pixelSize = 0)
{
    QFont font = base;
    const int size = pixelSize > 0 ? pixelSize
            : font.pixelSize() > 0 ? font.pixelSize()
                                   : QFontInfo(font).pixelSize();
    if (size > 0)
        font.setPixelSize(size);
    font.setWeight(static_cast<QFont::Weight>(TitleFontWeight));

    const QString family = font.family();
    const bool styleOwned = family == QLatin1String(VariableFontFamily)
            || family == QLatin1String(TextFontFamily) || family == QLatin1String(StaticFontFamily);
    const QString generic = winuiVariableFontFamily();
    if (styleOwned && !generic.isEmpty() && qgetenv("WINUI3STYLE_APP_FONT").isEmpty())
        font.setFamily(generic);

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    // Qt does not itself map the requested size onto the optical-size axis;
    // pin it so 28 px picks the Display outlines and 14 px the Text outlines,
    // independent of Qt version and of the font that happened to be inherited.
    if (size > 0 && font.family() == QLatin1String(VariableFontFamily))
        font.setVariableAxis(QFont::Tag("opsz"), static_cast<float>(size));
#endif
    return font;
}

} // namespace WinUI3::Private
