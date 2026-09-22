// SPDX-License-Identifier: LGPL-2.1-or-later
#include "winui3complex_p.h"

#include "winui3complex_editors_p.h"
#include "winui3complex_rangecontrols_p.h"
#include "winui3complex_toolgroup_p.h"
#include "winui3tokens_p.h"

#include <QStyleOption>

namespace WinUI3::Private {

bool coveredComplex(QStyle::ComplexControl control)
{
    switch (control) {
    case QStyle::CC_ToolButton:
    case QStyle::CC_GroupBox:
    case QStyle::CC_ComboBox:
    case QStyle::CC_SpinBox:
    case QStyle::CC_Slider:
    case QStyle::CC_ScrollBar:
        return true;
    default:
        return false;
    }
}

bool drawComplexControl(const Style *style, QStyle::ComplexControl control,
                        const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget)
{
    const Tokens t = tokens(option->palette);

    if (control == QStyle::CC_ToolButton || control == QStyle::CC_GroupBox)
        return drawToolGroupComplexControl(style, control, option, painter, widget, t);

    if (control == QStyle::CC_ComboBox || control == QStyle::CC_SpinBox)
        return drawEditorComplexControl(style, control, option, painter, widget, t);

    if (control == QStyle::CC_Slider || control == QStyle::CC_ScrollBar)
        return drawRangeComplexControl(style, control, option, painter, widget, t);

    return false;
}

} // namespace WinUI3::Private
