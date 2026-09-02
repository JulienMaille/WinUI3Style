#include "winui3complex_p.h"

#include "winui3paint_p.h"
#include "winui3frameproperties_p.h"
#include "winui3helpers_p.h"
#include "winui3style_properties_p.h"
#include "winui3tokens_p.h"

#include <winui3style/winui3icons.h>
#include <winui3style/winui3style.h>

#include <QAbstractSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QApplication>
#include <QGroupBox>
#include <QPainter>
#include <QStyleOptionButton>
#include <QStyleOptionComboBox>
#include <QStyleOptionGroupBox>
#include <QStyleOptionSlider>
#include <QStyleOptionSpinBox>
#include <QStyleOptionToolButton>

namespace WinUI3::Private {
using namespace PaintPrivate;

namespace {

bool verticalSpinButtons(const QWidget *widget)
{
    return qobject_cast<const QAbstractSpinBox *>(widget)
        && widget->property(Style::VerticalSpinButtonsProperty).toBool();
}

} // namespace

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
                        const QStyleOptionComplex *option, QPainter *painter,
                        const QWidget *widget)
{
    const Tokens t = tokens(option->palette);

    if (control == QStyle::CC_ToolButton) {
        if (const auto *tool = qstyleoption_cast<const QStyleOptionToolButton *>(option)) {
            style->drawPrimitive(QStyle::PE_PanelButtonTool, tool, painter, widget);
            style->drawControl(QStyle::CE_ToolButtonLabel, tool, painter, widget);
            if (tool->features & QStyleOptionToolButton::MenuButtonPopup) {
                const QRect menuRect = style->subControlRect(QStyle::CC_ToolButton, tool,
                                                       QStyle::SC_ToolButtonMenu, widget);
                painter->save();
                painter->setPen(t.stroke);
                const int x = option->direction == Qt::RightToLeft
                    ? menuRect.right() : menuRect.left();
                painter->drawLine(x, menuRect.top() + 5, x, menuRect.bottom() - 5);
                painter->restore();
            }
            return true;
        }
    }

    if (control == QStyle::CC_GroupBox) {
        if (const auto *group = qstyleoption_cast<const QStyleOptionGroupBox *>(option)) {
            const bool enabled = group->state & QStyle::State_Enabled;
            roundedRect(painter, group->rect, t.layer, t.stroke, 6.0);

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
                painter->drawText(label,
                                  QStyle::visualAlignment(group->direction,
                                                          Qt::AlignLeft | Qt::AlignVCenter),
                                  group->text);
                painter->restore();
            }
            return true;
        }
    }

    if (control == QStyle::CC_ComboBox) {
        if (const auto *combo = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            // Like buttons, controls painted directly on a native Mica window
            // have a translucent backing store.  Clear the previous animated
            // frame before compositing this one or hover/press fills accumulate.
            if (paintsDirectlyOnBackdrop(widget)) {
                painter->save();
                painter->setCompositionMode(QPainter::CompositionMode_Source);
                painter->fillRect(combo->rect, Qt::transparent);
                painter->restore();
            } else if (widget && widget->parentWidget()
                && widget->parentWidget()->property(Style::SurfaceProperty).isValid()) {
                painter->fillRect(combo->rect,
                                  widget->parentWidget()->palette().color(QPalette::Window));
            }
            const bool enabled = combo->state & QStyle::State_Enabled;
            const bool hovered = combo->state & QStyle::State_MouseOver;
            const bool pressed = combo->state & (QStyle::State_Sunken | QStyle::State_On);
            const bool editable = combo->editable;
            const QLineEdit *comboEditor = editable ? widget
                ? widget->findChild<QLineEdit *>() : nullptr : nullptr;
            const bool editableFocused = editable && enabled
                && (combo->state & QStyle::State_HasFocus
                    || (widget && widget->isActiveWindow()
                        && comboEditor && comboEditor->hasFocus()));
            QColor fill = enabled ? t.control : t.controlDisabled;
            // An editable ComboBox behaves like a TextBox once it owns
            // keyboard focus: flat light surface, no hover tint.
            if (!editableFocused) {
                const qreal hover = progress(widget, hoverProperty, hovered ? 1.0 : 0.0);
                const qreal press = progress(widget, pressProperty, pressed ? 1.0 : 0.0);
                fill = mix(fill, t.controlHover, hover);
                fill = mix(fill, t.controlPressed, press);
            } else {
                fill = t.dark ? QColor(30, 30, 30, 179) : QColor(255, 255, 255);
            }
            if (combo->subControls & QStyle::SC_ComboBoxFrame)
                controlSurface(painter, combo->rect, fill, t.stroke, t.strokeSecondary,
                               ControlRadius);
            if (combo->subControls & QStyle::SC_ComboBoxArrow) {
                // WinUI places a 12 px AnimatedIcon box 14 px from the trailing
                // edge.  Its AnimatedChevronDownSmall artwork is narrower than
                // the 12 px Segoe Fluent fallback, so render that fallback at
                // 10 px while preserving the official box position.
                constexpr int glyphBoxSize = 12;
                constexpr int glyphTrailingMargin = 14;
                constexpr int fallbackGlyphSize = 10;
                const QRect logicalGlyphBox(
                    combo->rect.right() - glyphTrailingMargin
                        - glyphBoxSize + 1,
                    combo->rect.top()
                        + (combo->rect.height() - glyphBoxSize) / 2,
                    glyphBoxSize, glyphBoxSize);
                const QRect logicalChevron(
                    logicalGlyphBox.left()
                        + (glyphBoxSize - fallbackGlyphSize) / 2,
                    logicalGlyphBox.top()
                        + (glyphBoxSize - fallbackGlyphSize) / 2,
                    fallbackGlyphSize, fallbackGlyphSize);
                const QRect chevronRect = QStyle::visualRect(combo->direction,
                                                             combo->rect,
                                                             logicalChevron);
                const qreal chevron = progress(widget, comboChevronProperty, 0.0);
                painter->save();
                painter->translate(0.0, 1.875 * chevron);
                paintThemedIcon(painter, icon(Icon::ChevronDown), chevronRect,
                                Qt::AlignCenter,
                                enabled ? t.textPrimary : t.textDisabled,
                                enabled ? QIcon::Normal : QIcon::Disabled);
                painter->restore();
            }
            if (editableFocused)
                drawEditorFocusUnderline(painter, combo->rect, t.accentFill,
                                         ControlRadius);
            if (keyboardFocusVisible(widget) && !editableFocused) {
                painter->save();
                painter->setRenderHint(QPainter::Antialiasing);
                painter->setBrush(Qt::NoBrush);
                painter->setPen(QPen(t.focusOuter, 2));
                painter->drawRoundedRect(QRectF(combo->rect).adjusted(1, 1, -1, -1), 7, 7);
                painter->setPen(QPen(t.focusInner, 1));
                painter->drawRoundedRect(QRectF(combo->rect).adjusted(3, 3, -3, -3), 5, 5);
                painter->restore();
            }
            return true;
        }
    }
    if (control == QStyle::CC_SpinBox) {
        if (const auto *spin = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
            const bool enabled = spin->state & QStyle::State_Enabled;
            const bool focused = spin->state & QStyle::State_HasFocus;
            const bool verticalButtons = verticalSpinButtons(widget);
            const qreal hover = progress(widget, hoverProperty,
                                         spin->state & QStyle::State_MouseOver ? 1.0 : 0.0);
            QColor fill = !enabled ? t.controlDisabled
                : focused ? (t.dark ? QColor(30, 30, 30, 179) : QColor(255, 255, 255))
                          : mix(t.control, t.controlHover, hover);
            controlSurface(painter, spin->rect, fill, t.stroke, t.strokeSecondary,
                           ControlRadius);
            if (verticalButtons) {
                const QRect editField = style->subControlRect(QStyle::CC_SpinBox, spin,
                                                       QStyle::SC_SpinBoxEditField,
                                                       widget);
                const int separatorX = spin->direction == Qt::RightToLeft
                    ? editField.left() : editField.right();
                painter->save();
                painter->setPen(QPen(t.stroke, 1, Qt::SolidLine, Qt::FlatCap));
                painter->drawLine(separatorX, spin->rect.top() + 1,
                                  separatorX, spin->rect.bottom() - 1);
                painter->restore();
            }
            if (focused)
                drawEditorFocusUnderline(painter, spin->rect, t.accentFill,
                                         ControlRadius);
            const auto drawStep = [&](QStyle::SubControl subControl, Icon glyph) {
                if (!(spin->subControls & subControl)) return;
                const QRect rect = style->subControlRect(QStyle::CC_SpinBox, spin, subControl, widget);
                const bool stepEnabled = enabled && (subControl == QStyle::SC_SpinBoxUp
                    ? spin->stepEnabled & QAbstractSpinBox::StepUpEnabled
                    : spin->stepEnabled & QAbstractSpinBox::StepDownEnabled);
                const QRectF visualRect = verticalButtons
                    ? (subControl == QStyle::SC_SpinBoxUp
                        ? QRectF(rect).adjusted(4, 3, -4, 0)
                        : QRectF(rect).adjusted(4, 0, -4, -3))
                    : (subControl == QStyle::SC_SpinBoxUp
                        ? QRectF(rect).adjusted(4, 4, 0, -4)
                        : QRectF(rect).adjusted(0, 4, -4, -4));
                if (stepEnabled && (spin->activeSubControls & subControl)
                    && (spin->state & QStyle::State_MouseOver)) {
                    roundedRect(painter, visualRect,
                                spin->state & QStyle::State_Sunken ? t.subtlePressed : t.subtleHover,
                                Qt::transparent, ControlRadius);
                }
                const QPoint center = visualRect.center().toPoint();
                icon(glyph, stepEnabled ? t.textPrimary : t.textDisabled).paint(
                    painter, QRect(center.x() - 6, center.y() - 6, 12, 12),
                    Qt::AlignCenter, stepEnabled ? QIcon::Normal : QIcon::Disabled);
            };
            drawStep(QStyle::SC_SpinBoxUp, Icon::ChevronUp);
            drawStep(QStyle::SC_SpinBoxDown, Icon::ChevronDown);
            return true;
        }
    }

    if (control == QStyle::CC_Slider) {
        if (const auto *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            const bool horizontal = slider->orientation == Qt::Horizontal;
            QRect groove = style->subControlRect(QStyle::CC_Slider, slider,
                                                  QStyle::SC_SliderGroove, widget);
            const QRect handle = style->subControlRect(QStyle::CC_Slider, slider,
                                                QStyle::SC_SliderHandle,
                                                widget);
            const qreal hover = progress(widget, hoverProperty,
                option->state & QStyle::State_MouseOver ? 1.0 : 0.0);
            const qreal pressed = progress(widget, pressProperty,
                option->state & QStyle::State_Sunken ? 1.0 : 0.0);
            const bool enabled = option->state & QStyle::State_Enabled;
            QColor track = enabled ? t.strokeStrong : t.textDisabled;
            QColor valueColor = enabled
                ? mix(t.accentFill, t.accentFillHover, hover)
                : t.accentFillDisabled;
            valueColor = mix(valueColor, t.accentFillPressed, pressed);
            roundedRect(painter, groove, track, Qt::transparent, 2);

            QRect value = groove;
            if (horizontal) {
                if (slider->upsideDown)
                    value.setLeft(handle.center().x());
                else
                    value.setRight(handle.center().x());
            } else {
                if (slider->upsideDown)
                    value.setTop(handle.center().y());
                else
                    value.setBottom(handle.center().y());
            }
            roundedRect(painter, value, valueColor, Qt::transparent, 2);

            if (slider->tickPosition != QSlider::NoTicks
                && slider->maximum > slider->minimum) {
                const qint64 minimum = slider->minimum;
                const qint64 maximum = slider->maximum;
                const qint64 range = maximum - minimum;
                const qint64 requested = slider->tickInterval > 0
                    ? qint64(slider->tickInterval)
                    : qint64(qMax(1, slider->pageStep));
                const qint64 interval = qMax<qint64>(1, qMax(requested,
                    (range + 99) / 100));
                painter->save();
                painter->setPen(QPen(enabled ? t.strokeStrong : t.textDisabled, 1));
                for (qint64 tick = minimum;;) {
                    const int span = horizontal ? groove.width() - 1
                                                 : groove.height() - 1;
                    const int offset = QStyle::sliderPositionFromValue(
                        slider->minimum, slider->maximum,
                        int(qBound(minimum, tick, maximum)), span,
                        slider->upsideDown);
                    if (horizontal) {
                        const int x = groove.left() + offset;
                        if (slider->tickPosition & QSlider::TicksAbove)
                            painter->drawLine(x, groove.top() - 8, x,
                                              groove.top() - 5);
                        if (slider->tickPosition & QSlider::TicksBelow)
                            painter->drawLine(x, groove.bottom() + 5, x,
                                              groove.bottom() + 8);
                    } else {
                        const int y = groove.top() + offset;
                        if (slider->tickPosition & QSlider::TicksLeft)
                            painter->drawLine(groove.left() - 8, y,
                                              groove.left() - 5, y);
                        if (slider->tickPosition & QSlider::TicksRight)
                            painter->drawLine(groove.right() + 5, y,
                                              groove.right() + 8, y);
                    }
                    if (tick >= maximum || interval > maximum - tick)
                        break;
                    tick += interval;
                }
                painter->restore();
            }

            qreal innerDiameter = 10.32 + (14.0 - 10.32) * hover;
            innerDiameter += (8.52 - innerDiameter) * pressed;
            if (!enabled)
                innerDiameter = 14.0;
            QColor thumbColor = enabled ? valueColor : t.accentFillDisabled;
            const QColor outerThumb = t.dark ? QColor(69, 69, 69)
                                             : QColor(255, 255, 255);
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setBrush(outerThumb);
            painter->setPen(QPen(t.strokeSecondary, 1));
            painter->drawEllipse(QPointF(handle.center()), 10.5, 10.5);
            painter->setBrush(thumbColor);
            painter->setPen(Qt::NoPen);
            painter->drawEllipse(QPointF(handle.center()), innerDiameter / 2.0,
                                 innerDiameter / 2.0);
            if ((option->state & QStyle::State_HasFocus) && keyboardFocusVisible(widget)) {
                painter->setBrush(Qt::NoBrush);
                painter->setPen(QPen(t.focusOuter, 2));
                painter->drawRoundedRect(QRectF(option->rect).adjusted(1, 1, -1, -1),
                                         ControlRadius, ControlRadius);
                painter->setPen(QPen(t.focusInner, 1));
                painter->drawRoundedRect(QRectF(option->rect).adjusted(3, 3, -3, -3),
                                         ControlRadius - 1, ControlRadius - 1);
            }
            painter->restore();
            return true;
        }
    }

    if (control == QStyle::CC_ScrollBar) {
        if (const auto *scroll = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            // QAbstractSlider's paint path does not guarantee that a standalone
            // scrollbar's backing store was cleared. WinUI's transparent rest
            // state therefore has to be composited over the widget surface here.
            QColor background = option->palette.color(QPalette::Window);
            for (const QWidget *ancestor = widget ? widget->parentWidget() : nullptr;
                 ancestor; ancestor = ancestor->parentWidget()) {
                if (!qobject_cast<const QGroupBox *>(ancestor))
                    continue;
                QColor opaqueLayer = t.layer;
                const qreal opacity = opaqueLayer.alphaF();
                opaqueLayer.setAlpha(255);
                background = mix(background, opaqueLayer, opacity);
            }
            painter->fillRect(option->rect, background);
            const bool enabled = option->state & QStyle::State_Enabled;
            // The WinUI ScrollBarThumb template fades the thumb to zero in its
            // Disabled state; arrows and track are suppressed with it.
            if (!enabled)
                return true;
            const QRect thumb = style->subControlRect(QStyle::CC_ScrollBar, scroll,
                                                      QStyle::SC_ScrollBarSlider,
                                                      widget);
            const qreal expanded = progress(widget, hoverProperty,
                option->state & QStyle::State_MouseOver ? 1.0 : 0.0);
            const bool horizontal = scroll->orientation == Qt::Horizontal;
            if (expanded > 0.001) {
                QColor track = t.layer;
                track.setAlphaF(track.alphaF() * expanded);
                roundedRect(painter, option->rect, track, Qt::transparent, 3);
            }

            const qreal thickness = 8.0 + 4.0 * expanded;
            QRectF visualThumb(thumb);
            if (horizontal)
                visualThumb.setTop(thumb.bottom() + 1.0 - thickness);
            else
                visualThumb.setLeft(thumb.right() + 1.0 - thickness);
            const QColor thumbColor = t.strokeStrong;
            roundedRect(painter, visualThumb, thumbColor, Qt::transparent,
                        thickness / 2.0);

            if (expanded > 0.001) {
                const QRect decrease = style->subControlRect(QStyle::CC_ScrollBar, scroll,
                    QStyle::SC_ScrollBarSubLine, widget);
                const QRect increase = style->subControlRect(QStyle::CC_ScrollBar, scroll,
                    QStyle::SC_ScrollBarAddLine, widget);
                const bool pressed = option->state & QStyle::State_Sunken;
                const auto drawArrow = [&](const QRect &rect, QStyle::SubControl sub,
                                           Icon glyph) {
                    if (!(scroll->subControls & sub))
                        return;
                    const bool active = (scroll->activeSubControls & sub)
                        && (option->state & QStyle::State_MouseOver);
                    if (active) {
                        const QColor fill = pressed ? t.subtlePressed
                                                    : t.subtleHover;
                        roundedRect(painter, QRectF(rect).adjusted(2, 2, -2, -2),
                                    fill, Qt::transparent, 3);
                    }
                    painter->save();
                    painter->setOpacity(expanded);
                    QRect glyphRect(rect.center().x() - 4, rect.center().y() - 4,
                                    8, 8);
                    if (active && pressed)
                        glyphRect = QRect(rect.center().x() - 3,
                                          rect.center().y() - 3, 7, 7);
                    icon(glyph, t.textPrimary).paint(painter, glyphRect,
                                                     Qt::AlignCenter,
                                                     QIcon::Normal);
                    painter->restore();
                };
                if (horizontal) {
                    const bool rtl = option->direction == Qt::RightToLeft;
                    drawArrow(decrease, QStyle::SC_ScrollBarSubLine,
                              rtl ? Icon::ChevronRight : Icon::ChevronLeft);
                    drawArrow(increase, QStyle::SC_ScrollBarAddLine,
                              rtl ? Icon::ChevronLeft : Icon::ChevronRight);
                } else {
                    drawArrow(decrease, QStyle::SC_ScrollBarSubLine, Icon::ChevronUp);
                    drawArrow(increase, QStyle::SC_ScrollBarAddLine, Icon::ChevronDown);
                }
            }
            return true;
        }
    }

    return false;
}

} // namespace WinUI3::Private
