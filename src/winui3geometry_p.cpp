#include "winui3geometry_p.h"

#include <winui3style/winui3style.h>

#include <QAbstractSpinBox>
#include <QStyleOptionComplex>
#include <QStyleOptionGroupBox>
#include <QStyleOptionSlider>
#include <QStyleOptionSpinBox>
#include <QStyleOptionToolButton>

namespace WinUI3::Private
{
namespace
{

bool verticalSpinButtons(const QWidget *widget)
{
    return qobject_cast<const QAbstractSpinBox *>(widget) &&
           widget->property(Style::VerticalSpinButtonsProperty).toBool();
}

} // namespace

QRect toggleTrackRect(const QRect &bounds, Qt::LayoutDirection direction,
                      DensityMode mode)
{
    const DensityMetrics &metrics = densityMetrics(mode);
    const int trackWidth = metrics.toggleTrackWidth;
    const int trackHeight = metrics.toggleTrackHeight;
    const int left = direction == Qt::RightToLeft
        ? bounds.right() - trackWidth + 1
        : bounds.left();
    return QRect(left, bounds.center().y() - trackHeight / 2,
                 trackWidth, trackHeight);
}

QRect toggleTrackRect(const QRect &bounds, Qt::LayoutDirection direction)
{
    return toggleTrackRect(bounds, direction, DensityMode::Standard);
}

std::optional<int> pixelMetricValue(QStyle::PixelMetric metric,
                                    bool toggleSwitch,
                                    const QWidget *widget)
{
    const DensityMetrics &metrics = densityMetricsFor(widget);
    if (toggleSwitch)
    {
        if (metric == QStyle::PM_IndicatorWidth)
            return metrics.toggleTrackWidth;
        if (metric == QStyle::PM_IndicatorHeight)
            return metrics.toggleTrackHeight;
    }
    switch (metric)
    {
    case QStyle::PM_ButtonMargin:
        // This legacy Qt metric is not the CT_PushButton content inset.
        // Preserve the established contract for layouts querying it.
        return 8;
    case QStyle::PM_ButtonDefaultIndicator:
        return 0;
    case QStyle::PM_DefaultFrameWidth:
    case QStyle::PM_ComboBoxFrameWidth:
    case QStyle::PM_SpinBoxFrameWidth:
        return 1;
    case QStyle::PM_IndicatorWidth:
    case QStyle::PM_IndicatorHeight:
    case QStyle::PM_ExclusiveIndicatorWidth:
    case QStyle::PM_ExclusiveIndicatorHeight:
        return metrics.indicatorSize;
    case QStyle::PM_ScrollBarExtent:
        return metrics.scrollBarExtent;
    case QStyle::PM_ScrollBarSliderMin:
        return metrics.scrollBarSliderMinimum;
    case QStyle::PM_SliderThickness:
        return metrics.sliderThickness;
    case QStyle::PM_SliderLength:
        return metrics.sliderLength;
    case QStyle::PM_TabCloseIndicatorWidth:
        return metrics.tabCloseWidth;
    case QStyle::PM_TabCloseIndicatorHeight:
        return metrics.tabCloseHeight;
    case QStyle::PM_SmallIconSize:
    case QStyle::PM_ButtonIconSize:
        return 16;
    case QStyle::PM_ToolBarIconSize:
        return 20;
    case QStyle::PM_DockWidgetSeparatorExtent:
    case QStyle::PM_DockWidgetHandleExtent:
    case QStyle::PM_SplitterWidth:
        return 6;
    case QStyle::PM_DockWidgetFrameWidth:
        return 1;
    case QStyle::PM_DockWidgetTitleMargin:
        return 8;
    case QStyle::PM_DockWidgetTitleBarButtonMargin:
        return 4;
    case QStyle::PM_HeaderMargin:
        return 12;
    case QStyle::PM_HeaderMarkSize:
        return 12;
    case QStyle::PM_HeaderGripMargin:
        return 4;
    case QStyle::PM_HeaderDefaultSectionSizeVertical:
        // QHeaderView's default section is historically 36 px even though
        // CT_HeaderSection has a 32 px minimum. Compact Sizing does not cover
        // data-grid headers, so both profiles retain that distinction.
        return 36;
    case QStyle::PM_HeaderDefaultSectionSizeHorizontal:
        return 100;
    default:
        return std::nullopt;
    }
}

std::optional<QRect> complexControlRect(QStyle::ComplexControl control,
                                        const QStyleOptionComplex *option,
                                        QStyle::SubControl subControl,
                                        const QWidget *widget)
{
    const DensityMetrics &metrics = densityMetricsFor(widget);
    if (control == QStyle::CC_Slider)
    {
        if (const auto *slider =
                qstyleoption_cast<const QStyleOptionSlider *>(option))
        {
            const bool horizontal = slider->orientation == Qt::Horizontal;
            const int preMargin = metrics.sliderGrooveMargin;
            const int grooveThickness = metrics.sliderGrooveThickness;
            const QRect groove =
                horizontal
                    ? QRect(slider->rect.left() + preMargin,
                            slider->rect.center().y() - grooveThickness / 2,
                            qMax(1, slider->rect.width() - 2 * preMargin),
                            grooveThickness)
                    : QRect(slider->rect.center().x() - grooveThickness / 2,
                            slider->rect.top() + preMargin, grooveThickness,
                            qMax(1, slider->rect.height() - 2 * preMargin));
            if (subControl == QStyle::SC_SliderGroove)
                return groove;
            if (subControl == QStyle::SC_SliderHandle)
            {
                const int span = qMax(
                    0, (horizontal ? groove.width() : groove.height()) - 1);
                const int offset = QStyle::sliderPositionFromValue(
                    slider->minimum, slider->maximum, slider->sliderPosition,
                    span, slider->upsideDown);
                const QPoint center =
                    horizontal
                        ? QPoint(groove.left() + offset, groove.center().y())
                        : QPoint(groove.center().x(), groove.top() + offset);
                const int handleSize = metrics.sliderHandleSize;
                // QRect::center() rounds an even-sized rectangle toward its
                // top/left.  Use the historical asymmetric offset so the
                // reported handle center stays exactly on the groove end.
                const int handleOffset = (handleSize - 1) / 2;
                return QRect(center.x() - handleOffset,
                             center.y() - handleOffset,
                             handleSize, handleSize);
            }
            if (subControl == QStyle::SC_SliderTickmarks)
                return slider->rect;
        }
    }
    if (control == QStyle::CC_ScrollBar)
    {
        if (const auto *scroll =
                qstyleoption_cast<const QStyleOptionSlider *>(option))
        {
            const bool horizontal = scroll->orientation == Qt::Horizontal;
            const int buttonLength = metrics.scrollBarExtent;
            const int axisLength =
                horizontal ? scroll->rect.width() : scroll->rect.height();
            const int grooveLength = qMax(0, axisLength - 2 * buttonLength);
            const QRect logicalSub =
                horizontal ? QRect(scroll->rect.left(), scroll->rect.top(),
                                   buttonLength, scroll->rect.height())
                           : QRect(scroll->rect.left(), scroll->rect.top(),
                                   scroll->rect.width(), buttonLength);
            const QRect logicalAdd =
                horizontal ? QRect(scroll->rect.right() - buttonLength + 1,
                                   scroll->rect.top(), buttonLength,
                                   scroll->rect.height())
                           : QRect(scroll->rect.left(),
                                   scroll->rect.bottom() - buttonLength + 1,
                                   scroll->rect.width(), buttonLength);
            const QRect groove =
                horizontal ? QRect(scroll->rect.left() + buttonLength,
                                   scroll->rect.top(), grooveLength,
                                   scroll->rect.height())
                           : QRect(scroll->rect.left(),
                                   scroll->rect.top() + buttonLength,
                                   scroll->rect.width(), grooveLength);
            if (subControl == QStyle::SC_ScrollBarSubLine)
                return QStyle::visualRect(scroll->direction, scroll->rect,
                                          logicalSub);
            if (subControl == QStyle::SC_ScrollBarAddLine)
                return QStyle::visualRect(scroll->direction, scroll->rect,
                                          logicalAdd);
            if (subControl == QStyle::SC_ScrollBarGroove)
                return groove;

            const qint64 range = qMax<qint64>(0, qint64(scroll->maximum) -
                                                     qint64(scroll->minimum));
            int thumbLength = grooveLength;
            if (range > 0)
            {
                const qint64 denominator = qint64(range) + scroll->pageStep;
                thumbLength = denominator > 0
                                  ? int(qint64(grooveLength) *
                                        scroll->pageStep / denominator)
                                  : 0;
                const int minimumThumb = qMin(metrics.scrollBarSliderMinimum,
                                              grooveLength);
                thumbLength = qBound(minimumThumb, thumbLength, grooveLength);
            }
            const int available = qMax(0, grooveLength - thumbLength);
            const int offset = QStyle::sliderPositionFromValue(
                scroll->minimum, scroll->maximum, scroll->sliderPosition,
                available, scroll->upsideDown);
            const QRect thumb =
                horizontal ? QRect(groove.left() + offset, groove.top(),
                                   thumbLength, groove.height())
                           : QRect(groove.left(), groove.top() + offset,
                                   groove.width(), thumbLength);
            if (subControl == QStyle::SC_ScrollBarSlider)
                return thumb;

            if (subControl == QStyle::SC_ScrollBarSubPage)
            {
                if (horizontal)
                {
                    return scroll->upsideDown
                               ? QRect(thumb.right() + 1, groove.top(),
                                       qMax(0, groove.right() - thumb.right()),
                                       groove.height())
                               : QRect(groove.left(), groove.top(),
                                       qMax(0, thumb.left() - groove.left()),
                                       groove.height());
                }
                return scroll->upsideDown
                           ? QRect(groove.left(), thumb.bottom() + 1,
                                   groove.width(),
                                   qMax(0, groove.bottom() - thumb.bottom()))
                           : QRect(groove.left(), groove.top(), groove.width(),
                                   qMax(0, thumb.top() - groove.top()));
            }
            if (subControl == QStyle::SC_ScrollBarAddPage)
            {
                if (horizontal)
                {
                    return scroll->upsideDown
                               ? QRect(groove.left(), groove.top(),
                                       qMax(0, thumb.left() - groove.left()),
                                       groove.height())
                               : QRect(thumb.right() + 1, groove.top(),
                                       qMax(0, groove.right() - thumb.right()),
                                       groove.height());
                }
                return scroll->upsideDown
                           ? QRect(groove.left(), groove.top(), groove.width(),
                                   qMax(0, thumb.top() - groove.top()))
                           : QRect(groove.left(), thumb.bottom() + 1,
                                   groove.width(),
                                   qMax(0, groove.bottom() - thumb.bottom()));
            }
        }
    }
    if (control == QStyle::CC_GroupBox)
    {
        if (const auto *group =
                qstyleoption_cast<const QStyleOptionGroupBox *>(option))
        {
            const bool checkable =
                group->subControls & QStyle::SC_GroupBoxCheckBox;
            if (subControl == QStyle::SC_GroupBoxFrame)
                return group->rect;
            if (subControl == QStyle::SC_GroupBoxCheckBox)
                return QStyle::visualRect(group->direction, group->rect,
                                          QRect(group->rect.left() + 12,
                                                group->rect.top() + 8, 20, 20));
            if (subControl == QStyle::SC_GroupBoxLabel)
            {
                const int left = group->rect.left() + (checkable ? 40 : 12);
                return QStyle::visualRect(
                    group->direction, group->rect,
                    QRect(left, group->rect.top() + 4,
                          qMax(0, group->rect.right() - left - 12), 28));
            }
            if (subControl == QStyle::SC_GroupBoxContents)
                return QStyle::visualRect(
                    group->direction, group->rect,
                    group->rect.adjusted(12, 36, -12, -12));
        }
    }
    if (control == QStyle::CC_ComboBox)
    {
        if (subControl == QStyle::SC_ComboBoxArrow)
        {
            const QRect logical(option->rect.right() - metrics.comboArrowWidth + 1,
                                option->rect.top(), metrics.comboArrowWidth,
                                option->rect.height());
            return QStyle::visualRect(option->direction, option->rect, logical);
        }
        if (subControl == QStyle::SC_ComboBoxEditField)
        {
            const QRect logical = option->rect.adjusted(
                metrics.comboEditLeftPadding, 1, -metrics.comboArrowWidth, -1);
            return QStyle::visualRect(option->direction, option->rect, logical);
        }
    }
    if (control == QStyle::CC_ToolButton)
    {
        const auto *tool =
            qstyleoption_cast<const QStyleOptionToolButton *>(option);
        if (subControl == QStyle::SC_ToolButton)
        {
            if (tool &&
                (tool->features & QStyleOptionToolButton::MenuButtonPopup))
            {
                const QRect logical = option->rect.adjusted(
                    0, 0, -metrics.toolButtonMenuWidth, 0);
                return QStyle::visualRect(option->direction, option->rect,
                                          logical);
            }
            return option->rect;
        }
        if (subControl == QStyle::SC_ToolButtonMenu)
        {
            const QRect logical(option->rect.right() - metrics.toolButtonMenuWidth + 1,
                                option->rect.top(), metrics.toolButtonMenuWidth,
                                option->rect.height());
            return QStyle::visualRect(option->direction, option->rect, logical);
        }
    }
    if (control == QStyle::CC_SpinBox)
    {
        const bool verticalButtons = verticalSpinButtons(widget);
        const int buttonWidth = verticalButtons ? metrics.verticalSpinButtonWidth
                                                : metrics.spinButtonWidth;
        QRect logical;
        if (verticalButtons && subControl == QStyle::SC_SpinBoxUp)
        {
            const int upperHeight = option->rect.height() / 2;
            logical = QRect(option->rect.right() - buttonWidth + 1,
                            option->rect.top(), buttonWidth, upperHeight);
        }
        else if (verticalButtons && subControl == QStyle::SC_SpinBoxDown)
        {
            const int upperHeight = option->rect.height() / 2;
            logical = QRect(option->rect.right() - buttonWidth + 1,
                            option->rect.top() + upperHeight, buttonWidth,
                            option->rect.height() - upperHeight);
        }
        else if (verticalButtons && subControl == QStyle::SC_SpinBoxEditField)
        {
            logical = option->rect.adjusted(12, 1, -buttonWidth, -1);
        }
        else if (subControl == QStyle::SC_SpinBoxUp)
        {
            logical =
                QRect(option->rect.right() - 2 * buttonWidth + 1,
                      option->rect.top(), buttonWidth, option->rect.height());
        }
        else if (subControl == QStyle::SC_SpinBoxDown)
        {
            logical =
                QRect(option->rect.right() - buttonWidth + 1,
                      option->rect.top(), buttonWidth, option->rect.height());
        }
        else if (subControl == QStyle::SC_SpinBoxEditField)
        {
            logical = option->rect.adjusted(12, 1, -2 * buttonWidth, -1);
        }
        if (logical.isValid())
            return QStyle::visualRect(option->direction, option->rect, logical);
        if (subControl == QStyle::SC_SpinBoxFrame)
            return option->rect;
    }
    return std::nullopt;
}

std::optional<QStyle::SubControl>
complexControlHitTest(QStyle::ComplexControl control,
                      const QStyleOptionComplex *option, const QPoint &position,
                      const QWidget *widget)
{
    if (!option || !option->rect.contains(position))
        return QStyle::SC_None;

    const auto contains = [&](QStyle::SubControl subControl)
    {
        const auto rect =
            complexControlRect(control, option, subControl, widget);
        return rect && rect->contains(position);
    };
    switch (control)
    {
    case QStyle::CC_ToolButton:
        if (const auto *tool =
                qstyleoption_cast<const QStyleOptionToolButton *>(option);
            tool &&
            (tool->features & QStyleOptionToolButton::MenuButtonPopup) &&
            contains(QStyle::SC_ToolButtonMenu))
            return QStyle::SC_ToolButtonMenu;
        return contains(QStyle::SC_ToolButton) ? QStyle::SC_ToolButton
                                               : QStyle::SC_None;
    case QStyle::CC_ComboBox:
        if (contains(QStyle::SC_ComboBoxArrow))
            return QStyle::SC_ComboBoxArrow;
        if (contains(QStyle::SC_ComboBoxEditField))
            return QStyle::SC_ComboBoxEditField;
        return QStyle::SC_ComboBoxFrame;
    case QStyle::CC_GroupBox:
        if (const auto *group =
                qstyleoption_cast<const QStyleOptionGroupBox *>(option);
            group && (group->subControls & QStyle::SC_GroupBoxCheckBox))
        {
            const auto check =
                complexControlRect(QStyle::CC_GroupBox, group,
                                   QStyle::SC_GroupBoxCheckBox, widget);
            const auto label = complexControlRect(
                QStyle::CC_GroupBox, group, QStyle::SC_GroupBoxLabel, widget);
            if (check && label && check->united(*label).contains(position))
                return QStyle::SC_GroupBoxCheckBox;
        }
        if (contains(QStyle::SC_GroupBoxLabel))
            return QStyle::SC_GroupBoxLabel;
        if (contains(QStyle::SC_GroupBoxContents))
            return QStyle::SC_GroupBoxContents;
        return QStyle::SC_GroupBoxFrame;
    case QStyle::CC_SpinBox:
        if (contains(QStyle::SC_SpinBoxUp))
            return QStyle::SC_SpinBoxUp;
        if (contains(QStyle::SC_SpinBoxDown))
            return QStyle::SC_SpinBoxDown;
        if (contains(QStyle::SC_SpinBoxEditField))
            return QStyle::SC_SpinBoxEditField;
        return QStyle::SC_SpinBoxFrame;
    case QStyle::CC_Slider:
        if (contains(QStyle::SC_SliderHandle))
            return QStyle::SC_SliderHandle;
        return contains(QStyle::SC_SliderGroove) ? QStyle::SC_SliderGroove
                                                 : QStyle::SC_None;
    case QStyle::CC_ScrollBar:
        for (QStyle::SubControl sub :
             {QStyle::SC_ScrollBarSubLine, QStyle::SC_ScrollBarAddLine,
              QStyle::SC_ScrollBarSlider, QStyle::SC_ScrollBarSubPage,
              QStyle::SC_ScrollBarAddPage, QStyle::SC_ScrollBarGroove})
            if (contains(sub))
                return sub;
        return QStyle::SC_None;
    default:
        return std::nullopt;
    }
}

} // namespace WinUI3::Private
