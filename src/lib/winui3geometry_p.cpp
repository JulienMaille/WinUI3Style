// SPDX-License-Identifier: LGPL-2.1-or-later
#include "winui3geometry_p.h"

#include "winui3helpers_p.h"

#include <winui3style/winui3style.h>

#include <QHeaderView>
#include <QTableView>
#include <QStyleOptionComplex>
#include <QStyleOptionGroupBox>
#include <QStyleOptionSlider>
#include <QStyleOptionSpinBox>
#include <QStyleOptionToolButton>

namespace WinUI3::Private {

namespace {
// ToggleSwitch template thumb anchors, in logical pixels from the track's
// left edge. toggleKnobRect uses them for painting and
// togglePositionFromTrackX inverts the rest anchors for pointer dragging.
// Keep both consumers on these values rather than restating 9.5 / 20.0 at
// each call site.
constexpr qreal toggleKnobRestInset = 9.5;
constexpr qreal toggleKnobRestTravel = 20.0;
constexpr qreal toggleKnobPressedInset = 11.5;
constexpr qreal toggleKnobPressedTravel = 17.0;
} // namespace

QRect toggleTrackRect(const QRect &bounds, Qt::LayoutDirection direction, DensityMode mode)
{
    const DensityMetrics &metrics = densityMetrics(mode);
    const int trackWidth = metrics.toggleTrackWidth;
    const int trackHeight = metrics.toggleTrackHeight;
    const int left = direction == Qt::RightToLeft ? bounds.right() - trackWidth + 1 : bounds.left();
    return QRect(left, bounds.center().y() - trackHeight / 2, trackWidth, trackHeight);
}

QRect toggleTrackRect(const QRect &bounds, Qt::LayoutDirection direction)
{
    return toggleTrackRect(bounds, direction, DensityMode::Standard);
}

QRectF toggleKnobRect(const QRectF &track, qreal position, qreal hover, qreal press,
                      Qt::LayoutDirection direction)
{
    // Copied verbatim from the toggle paint path: hover grows 12 px to
    // 14 px, press stretches to 17 x 14, and the XAML template interpolates
    // the thumb centre between the normal and pressed anchors.
    const qreal hoverSize = 12.0 + 2.0 * hover;
    const qreal knobWidth = hoverSize + (17.0 - hoverSize) * press;
    const qreal knobHeight = hoverSize + (14.0 - hoverSize) * press;
    qreal visualPosition = position;
    if (direction == Qt::RightToLeft)
        visualPosition = 1.0 - visualPosition;
    const qreal normalCenter = toggleKnobRestInset + toggleKnobRestTravel * visualPosition;
    const qreal pressedCenter = toggleKnobPressedInset + toggleKnobPressedTravel * visualPosition;
    const qreal knobCenter = track.left() + normalCenter + (pressedCenter - normalCenter) * press;
    return QRectF(knobCenter - knobWidth / 2.0, track.center().y() - knobHeight / 2.0, knobWidth,
                  knobHeight);
}

qreal togglePositionFromTrackX(qreal trackX, Qt::LayoutDirection direction)
{
    const qreal visualPosition = (trackX - toggleKnobRestInset) / toggleKnobRestTravel;
    return direction == Qt::RightToLeft ? 1.0 - visualPosition : visualPosition;
}

std::optional<int> pixelMetricValue(QStyle::PixelMetric metric, const QWidget *widget)
{
    switch (metric) {
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
    case QStyle::PM_ToolTipLabelFrameWidth:
        return 6;
    case QStyle::PM_IndicatorWidth: {
        const auto &metrics = densityMetricsFor(widget);
        return toggleSwitch(widget) ? metrics.toggleTrackWidth : metrics.indicatorSize;
    }
    case QStyle::PM_IndicatorHeight: {
        const auto &metrics = densityMetricsFor(widget);
        return toggleSwitch(widget) ? metrics.toggleTrackHeight : metrics.indicatorSize;
    }
    case QStyle::PM_ExclusiveIndicatorWidth:
    case QStyle::PM_ExclusiveIndicatorHeight:
        return densityMetricsFor(widget).indicatorSize;
    case QStyle::PM_ScrollBarExtent:
        return densityMetricsFor(widget).scrollBarExtent;
    case QStyle::PM_ScrollBarSliderMin:
        return densityMetricsFor(widget).scrollBarSliderMinimum;
    case QStyle::PM_SliderThickness:
        return densityMetricsFor(widget).sliderThickness;
    case QStyle::PM_SliderLength:
        return densityMetricsFor(widget).sliderLength;
    case QStyle::PM_TabCloseIndicatorWidth:
        return densityMetricsFor(widget).tabCloseWidth;
    case QStyle::PM_TabCloseIndicatorHeight:
        return densityMetricsFor(widget).tabCloseHeight;
    case QStyle::PM_SmallIconSize:
    case QStyle::PM_ButtonIconSize:
        // Qt sizes a dock title button's glyph from PM_SmallIconSize and paints
        // it into the slot SE_DockWidgetCloseButton reports, so the 16 px
        // toolbar mark fills the 20 px slot edge to edge. Fluent's title row
        // carries a 12 px glyph in that slot instead, matching the 12 px marks
        // its other 20 px command surfaces use.
        if (metric == QStyle::PM_SmallIconSize && widget
            && (widget->inherits("QDockWidgetTitleButton")
                || widget->property(dockTitleButtonProperty).toBool()))
            return DockTitleIconExtent;
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
        return DockTitleInset;
    case QStyle::PM_DockWidgetTitleBarButtonMargin:
        return (DockTitleButtonExtent - DockTitleIconExtent) / 2;
    case QStyle::PM_SizeGripSize:
        return 16;
    case QStyle::PM_HeaderMargin:
        if (const auto *header = qobject_cast<const QHeaderView *>(widget); header
            && header->orientation() == Qt::Vertical
            && qobject_cast<const QTableView *>(header->parentWidget())
            && header->parentWidget()->objectName() != QLatin1String("qt_calendar_calendarview")
            && densityModeFor(widget) == DensityMode::Compact) {
            // Qt's minimum section must fit the inline editor plus its grid pixel.
            QStyleOptionFrame editor;
            editor.initFrom(header->parentWidget());
            const int editorHeight = header->style()
                                             ->sizeFromContents(QStyle::CT_LineEdit, &editor,
                                                                QSize(), header->parentWidget())
                                             .height();
            if (editorHeight + 1 <= densityMetrics(DensityMode::Compact).tableItemHeight)
                return CompactItemViewHeaderVerticalPadding;
            return qMax(CompactItemViewHeaderVerticalPadding,
                        (editorHeight + 2 - header->fontMetrics().height()) / 2);
        }
        return 12;
    case QStyle::PM_HeaderMarkSize:
        return 12;
    case QStyle::PM_HeaderGripMargin:
        return 4;
    case QStyle::PM_HeaderDefaultSectionSizeVertical:
        if (const auto *header = qobject_cast<const QHeaderView *>(widget); header
            && qobject_cast<const QTableView *>(header->parentWidget())
            && header->parentWidget()->objectName() != QLatin1String("qt_calendar_calendarview"))
            return densityMetricsFor(widget).tableItemHeight;
        return 36;
    case QStyle::PM_HeaderDefaultSectionSizeHorizontal:
        return 100;
    default:
        return std::nullopt;
    }
}

std::optional<QRect> complexControlRect(QStyle::ComplexControl control,
                                        const QStyleOptionComplex *option,
                                        QStyle::SubControl subControl, const QWidget *widget)
{
    if (!option)
        return std::nullopt;
    const DensityMetrics &metrics = densityMetricsFor(widget);
    if (control == QStyle::CC_Slider) {
        if (const auto *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            const bool horizontal = slider->orientation == Qt::Horizontal;
            const int preMargin = metrics.sliderGrooveMargin;
            const int grooveThickness = metrics.sliderGrooveThickness;
            const QRect groove = horizontal
                    ? QRect(slider->rect.left() + preMargin,
                            slider->rect.center().y() - grooveThickness / 2,
                            qMax(1, slider->rect.width() - 2 * preMargin), grooveThickness)
                    : QRect(slider->rect.center().x() - grooveThickness / 2,
                            slider->rect.top() + preMargin, grooveThickness,
                            qMax(1, slider->rect.height() - 2 * preMargin));
            if (subControl == QStyle::SC_SliderGroove)
                return groove;
            if (subControl == QStyle::SC_SliderHandle) {
                const int handleSize = metrics.sliderHandleSize;
                const int grooveLength = horizontal ? groove.width() : groove.height();
                // Match QSliderPrivate::pixelPosToRangeValue's leading-edge travel span.
                const int span = qMax(0, grooveLength - handleSize);
                const int offset = QStyle::sliderPositionFromValue(slider->minimum, slider->maximum,
                                                                   slider->sliderPosition, span,
                                                                   slider->upsideDown);
                const int handleOffset = (handleSize - 1) / 2;
                const QPoint topLeft = horizontal
                        ? QPoint(groove.left() + offset, groove.center().y() - handleOffset)
                        : QPoint(groove.center().x() - handleOffset, groove.top() + offset);
                return QRect(topLeft, QSize(handleSize, handleSize));
            }
            if (subControl == QStyle::SC_SliderTickmarks)
                return slider->rect;
        }
    }
    if (control == QStyle::CC_ScrollBar) {
        if (const auto *scroll = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            const bool horizontal = scroll->orientation == Qt::Horizontal;
            const int buttonLength = metrics.scrollBarExtent;
            const int axisLength = horizontal ? scroll->rect.width() : scroll->rect.height();
            const int grooveLength = qMax(0, axisLength - 2 * buttonLength);
            const QRect logicalSub = horizontal ? QRect(scroll->rect.left(), scroll->rect.top(),
                                                        buttonLength, scroll->rect.height())
                                                : QRect(scroll->rect.left(), scroll->rect.top(),
                                                        scroll->rect.width(), buttonLength);
            const QRect logicalAdd = horizontal
                    ? QRect(scroll->rect.right() - buttonLength + 1, scroll->rect.top(),
                            buttonLength, scroll->rect.height())
                    : QRect(scroll->rect.left(), scroll->rect.bottom() - buttonLength + 1,
                            scroll->rect.width(), buttonLength);
            const QRect groove = horizontal
                    ? QRect(scroll->rect.left() + buttonLength, scroll->rect.top(), grooveLength,
                            scroll->rect.height())
                    : QRect(scroll->rect.left(), scroll->rect.top() + buttonLength,
                            scroll->rect.width(), grooveLength);
            // Geometry is computed in logical (left-to-right) coordinates.
            // Mirror every subcontrol for RTL, not just the arrow buttons:
            // the thumb and page halves must follow the same visual direction.
            const auto visual = [scroll](const QRect &rect) {
                return QStyle::visualRect(scroll->direction, scroll->rect, rect);
            };
            if (subControl == QStyle::SC_ScrollBarSubLine)
                return QStyle::visualRect(scroll->direction, scroll->rect, logicalSub);
            if (subControl == QStyle::SC_ScrollBarAddLine)
                return QStyle::visualRect(scroll->direction, scroll->rect, logicalAdd);
            if (subControl == QStyle::SC_ScrollBarGroove)
                return visual(groove);

            const qint64 range = qMax<qint64>(0, qint64(scroll->maximum) - qint64(scroll->minimum));
            int thumbLength = grooveLength;
            if (range > 0) {
                const qint64 denominator = qint64(range) + scroll->pageStep;
                thumbLength = denominator > 0
                        ? int(qint64(grooveLength) * scroll->pageStep / denominator)
                        : 0;
                const int minimumThumb = qMin(metrics.scrollBarSliderMinimum, grooveLength);
                thumbLength = qBound(minimumThumb, thumbLength, grooveLength);
            }
            const int available = qMax(0, grooveLength - thumbLength);
            const int offset = QStyle::sliderPositionFromValue(scroll->minimum, scroll->maximum,
                                                               scroll->sliderPosition, available,
                                                               scroll->upsideDown);
            const QRect thumb = horizontal
                    ? QRect(groove.left() + offset, groove.top(), thumbLength, groove.height())
                    : QRect(groove.left(), groove.top() + offset, groove.width(), thumbLength);
            if (subControl == QStyle::SC_ScrollBarSlider)
                return visual(thumb);

            if (subControl == QStyle::SC_ScrollBarSubPage) {
                if (horizontal) {
                    return visual(scroll->upsideDown
                                          ? QRect(thumb.right() + 1, groove.top(),
                                                  qMax(0, groove.right() - thumb.right()),
                                                  groove.height())
                                          : QRect(groove.left(), groove.top(),
                                                  qMax(0, thumb.left() - groove.left()),
                                                  groove.height()));
                }
                return visual(scroll->upsideDown
                                      ? QRect(groove.left(), thumb.bottom() + 1, groove.width(),
                                              qMax(0, groove.bottom() - thumb.bottom()))
                                      : QRect(groove.left(), groove.top(), groove.width(),
                                              qMax(0, thumb.top() - groove.top())));
            }
            if (subControl == QStyle::SC_ScrollBarAddPage) {
                if (horizontal) {
                    return visual(scroll->upsideDown
                                          ? QRect(groove.left(), groove.top(),
                                                  qMax(0, thumb.left() - groove.left()),
                                                  groove.height())
                                          : QRect(thumb.right() + 1, groove.top(),
                                                  qMax(0, groove.right() - thumb.right()),
                                                  groove.height()));
                }
                return visual(scroll->upsideDown
                                      ? QRect(groove.left(), groove.top(), groove.width(),
                                              qMax(0, thumb.top() - groove.top()))
                                      : QRect(groove.left(), thumb.bottom() + 1, groove.width(),
                                              qMax(0, groove.bottom() - thumb.bottom())));
            }
        }
    }
    if (control == QStyle::CC_GroupBox) {
        if (const auto *group = qstyleoption_cast<const QStyleOptionGroupBox *>(option)) {
            const bool checkable = group->subControls & QStyle::SC_GroupBoxCheckBox;
            if (subControl == QStyle::SC_GroupBoxFrame)
                return group->rect;
            if (subControl == QStyle::SC_GroupBoxCheckBox)
                return QStyle::visualRect(
                        group->direction, group->rect,
                        QRect(group->rect.left() + 12, group->rect.top() + 8, 20, 20));
            if (subControl == QStyle::SC_GroupBoxLabel) {
                const int left = group->rect.left() + (checkable ? 40 : 12);
                return QStyle::visualRect(group->direction, group->rect,
                                          QRect(left, group->rect.top() + 4,
                                                qMax(0, group->rect.right() - left - 12), 28));
            }
            if (subControl == QStyle::SC_GroupBoxContents)
                return QStyle::visualRect(group->direction, group->rect,
                                          group->rect.adjusted(12, 36, -12, -12));
        }
    }
    if (control == QStyle::CC_ComboBox) {
        if (subControl == QStyle::SC_ComboBoxArrow) {
            const QRect logical(option->rect.right() - metrics.comboArrowWidth + 1,
                                option->rect.top(), metrics.comboArrowWidth, option->rect.height());
            return QStyle::visualRect(option->direction, option->rect, logical);
        }
        if (subControl == QStyle::SC_ComboBoxEditField) {
            const QRect logical = option->rect.adjusted(metrics.comboEditLeftPadding, 1,
                                                        -metrics.comboArrowWidth, -1);
            return QStyle::visualRect(option->direction, option->rect, logical);
        }
    }
    if (control == QStyle::CC_ToolButton) {
        const auto *tool = qstyleoption_cast<const QStyleOptionToolButton *>(option);
        if (subControl == QStyle::SC_ToolButton) {
            if (tool && (tool->features & QStyleOptionToolButton::MenuButtonPopup)) {
                const QRect logical = option->rect.adjusted(0, 0, -metrics.toolButtonMenuWidth, 0);
                return QStyle::visualRect(option->direction, option->rect, logical);
            }
            return option->rect;
        }
        if (subControl == QStyle::SC_ToolButtonMenu) {
            const QRect logical(option->rect.right() - metrics.toolButtonMenuWidth + 1,
                                option->rect.top(), metrics.toolButtonMenuWidth,
                                option->rect.height());
            return QStyle::visualRect(option->direction, option->rect, logical);
        }
    }
    if (control == QStyle::CC_SpinBox) {
        const auto *spin = qstyleoption_cast<const QStyleOptionSpinBox *>(option);
        const bool noButtons = spin && spin->buttonSymbols == QAbstractSpinBox::NoButtons;
        const bool verticalButtons = !noButtons && verticalSpinButtons(widget);
        const int buttonWidth =
                verticalButtons ? metrics.verticalSpinButtonWidth : metrics.spinButtonWidth;
        QRect logical;
        if (noButtons) {
            // NoButtons has no step rectangles to reserve: give the whole
            // field back to the editor, keeping only the usual left inset.
            if (subControl == QStyle::SC_SpinBoxEditField)
                logical = option->rect.adjusted(12, 1, 0, -1);
        } else if (verticalButtons && subControl == QStyle::SC_SpinBoxUp) {
            const int upperHeight = option->rect.height() / 2;
            logical = QRect(option->rect.right() - buttonWidth + 1, option->rect.top(), buttonWidth,
                            upperHeight);
        } else if (verticalButtons && subControl == QStyle::SC_SpinBoxDown) {
            const int upperHeight = option->rect.height() / 2;
            logical =
                    QRect(option->rect.right() - buttonWidth + 1, option->rect.top() + upperHeight,
                          buttonWidth, option->rect.height() - upperHeight);
        } else if (verticalButtons && subControl == QStyle::SC_SpinBoxEditField) {
            logical = option->rect.adjusted(12, 1, -buttonWidth, -1);
        } else if (subControl == QStyle::SC_SpinBoxUp) {
            logical = QRect(option->rect.right() - 2 * buttonWidth + 1, option->rect.top(),
                            buttonWidth, option->rect.height());
        } else if (subControl == QStyle::SC_SpinBoxDown) {
            logical = QRect(option->rect.right() - buttonWidth + 1, option->rect.top(), buttonWidth,
                            option->rect.height());
        } else if (subControl == QStyle::SC_SpinBoxEditField) {
            logical = option->rect.adjusted(12, 1, -2 * buttonWidth, -1);
        }
        if (logical.isValid())
            return QStyle::visualRect(option->direction, option->rect, logical);
        if (subControl == QStyle::SC_SpinBoxFrame)
            return option->rect;
    }
    return std::nullopt;
}

std::optional<QStyle::SubControl> complexControlHitTest(QStyle::ComplexControl control,
                                                        const QStyleOptionComplex *option,
                                                        const QPoint &position,
                                                        const QWidget *widget)
{
    if (!option || !option->rect.contains(position))
        return QStyle::SC_None;

    const auto contains = [&](QStyle::SubControl subControl) {
        const auto rect = complexControlRect(control, option, subControl, widget);
        return rect && rect->contains(position);
    };
    switch (control) {
    case QStyle::CC_ToolButton:
        if (const auto *tool = qstyleoption_cast<const QStyleOptionToolButton *>(option); tool
            && (tool->features & QStyleOptionToolButton::MenuButtonPopup)
            && contains(QStyle::SC_ToolButtonMenu))
            return QStyle::SC_ToolButtonMenu;
        return contains(QStyle::SC_ToolButton) ? QStyle::SC_ToolButton : QStyle::SC_None;
    case QStyle::CC_ComboBox:
        if (contains(QStyle::SC_ComboBoxArrow))
            return QStyle::SC_ComboBoxArrow;
        if (contains(QStyle::SC_ComboBoxEditField))
            return QStyle::SC_ComboBoxEditField;
        return QStyle::SC_ComboBoxFrame;
    case QStyle::CC_GroupBox:
        if (const auto *group = qstyleoption_cast<const QStyleOptionGroupBox *>(option);
            group && (group->subControls & QStyle::SC_GroupBoxCheckBox)) {
            const auto check = complexControlRect(QStyle::CC_GroupBox, group,
                                                  QStyle::SC_GroupBoxCheckBox, widget);
            const auto label = complexControlRect(QStyle::CC_GroupBox, group,
                                                  QStyle::SC_GroupBoxLabel, widget);
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
        return contains(QStyle::SC_SliderGroove) ? QStyle::SC_SliderGroove : QStyle::SC_None;
    case QStyle::CC_ScrollBar:
        for (QStyle::SubControl sub : { QStyle::SC_ScrollBarSubLine, QStyle::SC_ScrollBarAddLine,
                                        QStyle::SC_ScrollBarSlider, QStyle::SC_ScrollBarSubPage,
                                        QStyle::SC_ScrollBarAddPage, QStyle::SC_ScrollBarGroove })
            if (contains(sub))
                return sub;
        return QStyle::SC_None;
    default:
        return std::nullopt;
    }
}

} // namespace WinUI3::Private
