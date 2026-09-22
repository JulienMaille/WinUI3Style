// SPDX-License-Identifier: LGPL-2.1-or-later
#include "winui3complex_p.h"

#include "winui3complex_editors_p.h"
#include "winui3complex_rangecontrols_p.h"
#include "winui3helpers_p.h"
#include "winui3paint_p.h"
#include "winui3tokens_p.h"

#include <winui3style/winui3style.h>

#include <QApplication>
#include <QPainter>
#include <QStyleOptionButton>
#include <QStyleOptionGroupBox>
#include <QStyleOptionToolButton>

namespace WinUI3::Private {
using namespace PaintPrivate;

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

    if (control == QStyle::CC_ToolButton) {
        if (const auto *tool = qstyleoption_cast<const QStyleOptionToolButton *>(option)) {
            style->drawPrimitive(QStyle::PE_PanelButtonTool, tool, painter, widget);
            style->drawControl(QStyle::CE_ToolButtonLabel, tool, painter, widget);
            if (tool->features & QStyleOptionToolButton::MenuButtonPopup) {
                // WinUI SplitButton divider: a 1px full-height separator at
                // the leading edge of the secondary (dropdown) half.
                const QRect menuRect = style->subControlRect(QStyle::CC_ToolButton, tool,
                                                             QStyle::SC_ToolButtonMenu, widget);
                painter->save();
                painter->setPen(QPen(t.stroke, 1));
                const int x =
                        option->direction == Qt::RightToLeft ? menuRect.right() : menuRect.left();
                painter->drawLine(x, menuRect.top() + 4, x, menuRect.bottom() - 4);
                painter->restore();
            }
            return true;
        }
    }

    if (control == QStyle::CC_GroupBox) {
        if (const auto *group = qstyleoption_cast<const QStyleOptionGroupBox *>(option)) {
            const bool enabled = group->state & QStyle::State_Enabled;
            eraseForBackdrop(painter, widget, group->rect, 6.0);
            roundedRect(painter, group->rect, veiledCard(t.layer, paintsDirectlyOnBackdrop(widget)),
                        t.stroke, 6.0);

            if (group->subControls & QStyle::SC_GroupBoxCheckBox) {
                QStyleOptionButton indicator;
                indicator.rect = style->subControlRect(QStyle::CC_GroupBox, group,
                                                       QStyle::SC_GroupBoxCheckBox, widget);
                indicator.state = group->state;
                indicator.palette = group->palette;
                style->drawPrimitive(QStyle::PE_IndicatorCheckBox, &indicator, painter, widget);
            }
            if (group->subControls & QStyle::SC_GroupBoxLabel) {
                const QRect label = style->subControlRect(QStyle::CC_GroupBox, group,
                                                          QStyle::SC_GroupBoxLabel, widget);
                painter->save();
                QFont titleFont = widget ? widget->font() : QApplication::font();
                titleFont.setWeight(QFont::DemiBold);
                painter->setFont(titleFont);
                painter->setPen(enabled ? t.textPrimary : t.textDisabled);
                painter->drawText(
                        label,
                        QStyle::visualAlignment(group->direction, Qt::AlignLeft | Qt::AlignVCenter),
                        group->text);
                painter->restore();
            }
            return true;
        }
    }

    if (control == QStyle::CC_ComboBox || control == QStyle::CC_SpinBox)
        return drawEditorComplexControl(style, control, option, painter, widget, t);

    if (control == QStyle::CC_Slider || control == QStyle::CC_ScrollBar)
        return drawRangeComplexControl(style, control, option, painter, widget, t);

    return false;
}

} // namespace WinUI3::Private
