#include "winui3buttons_p.h"

#include "winui3paint_p.h"
#include "winui3frameproperties_p.h"
#include "winui3geometry_p.h"
#include "winui3style_properties_p.h"
#include "winui3tokens_p.h"

#include <winui3style/winui3icons.h>
#include <winui3style/winui3style.h>

#include <QAbstractButton>
#include <QAbstractSpinBox>
#include <QApplication>
#include <QCheckBox>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QRadioButton>
#include <QStyleOptionButton>
#include <QStyleOptionToolButton>
#include <QToolBar>
#include <QToolButton>
#include <QVariant>

namespace WinUI3::Private {
using namespace PaintPrivate;

namespace {

bool toggleSwitch(const QWidget *widget)
{
    return qobject_cast<const QCheckBox *>(widget)
        && widget->property(Style::ToggleSwitchProperty).toBool();
}

bool spinBoxEditor(const QWidget *widget)
{
    return qobject_cast<const QLineEdit *>(widget)
        && qobject_cast<const QAbstractSpinBox *>(widget->parentWidget());
}

bool textBoxHelperButton(const QWidget *widget)
{
    return qobject_cast<const QAbstractButton *>(widget)
        && qobject_cast<const QLineEdit *>(widget->parentWidget());
}

qreal progress(const QWidget *widget, const char *name, qreal fallback = 0.0)
{
    return framePropertyRegistry().real(widget, name, fallback);
}

bool keyboardFocusVisible(const QWidget *widget)
{
    return widget && framePropertyRegistry().value(widget, focusVisibleProperty).toBool();
}

} // namespace

bool drawButtonPrimitive(const Style *, QStyle::PrimitiveElement element,
                         const QStyleOption *option, QPainter *painter,
                         const QWidget *widget)
{
    if (element != QStyle::PE_PanelButtonCommand
        && element != QStyle::PE_PanelButtonTool
        && element != QStyle::PE_IndicatorCheckBox
        && element != QStyle::PE_IndicatorRadioButton
        && element != QStyle::PE_PanelLineEdit
        && element != QStyle::PE_FrameLineEdit) {
        return false;
    }

    const Tokens t = tokens(option->palette);
    const bool enabled = option->state & QStyle::State_Enabled;
    const bool hovered = enabled && (option->state & QStyle::State_MouseOver);
    const bool pressed = enabled && (option->state & QStyle::State_Sunken);
    const qreal hover = enabled
        ? progress(widget, hoverProperty, hovered ? 1.0 : 0.0) : 0.0;
    // State_Sunken is the authoritative instantaneous state. The property is
    // an animation/pulse cache and can briefly still contain zero when Qt has
    // entered the pressed state (notably on rapid press/reversal sequences).
    const qreal press = enabled
        ? qMax(progress(widget, pressProperty, pressed ? 1.0 : 0.0),
              pressed ? 1.0 : 0.0)
        : 0.0;

    if (element == QStyle::PE_PanelButtonCommand || element == QStyle::PE_PanelButtonTool) {
        const ControlRole role = Style::controlRole(widget);
        const bool textHelper = textBoxHelperButton(widget);
        QColor fill = t.control;
        QColor stroke = t.stroke;

        if (role == ControlRole::Accent) {
            fill = enabled ? t.accentFill : t.accentFillDisabled;
            fill = mix(fill, t.accentFillHover, hover);
            fill = mix(fill, t.accentFillPressed, press);
            stroke = fill.darker(t.dark ? 90 : 112);
        } else if (role == ControlRole::Destructive) {
            fill = enabled ? t.danger : mix(t.surface, t.danger, 0.35);
            fill = mix(fill, fill.lighter(112), hover);
            fill = mix(fill, fill.darker(112), press);
            stroke = fill.darker(112);
        } else if (role == ControlRole::Subtle || role == ControlRole::Navigation
                   || (element == QStyle::PE_PanelButtonTool && widget
                       && (qobject_cast<const QToolBar *>(widget->parentWidget())
                           || textHelper))) {
            fill = Qt::transparent;
            fill = mix(fill, t.subtleHover, hover);
            fill = mix(fill, t.subtlePressed, press);
            if (option->state & QStyle::State_On)
                fill = mix(t.subtleHover, t.accentFill, 0.14);
            stroke = Qt::transparent;
        } else {
            if (option->state & QStyle::State_On) {
                fill = enabled ? t.accentFill : t.accentFillDisabled;
                fill = mix(fill, t.accentFillHover, hover);
                fill = mix(fill, t.accentFillPressed, press);
                stroke = fill;
            } else {
                fill = enabled ? fill : t.controlDisabled;
                fill = mix(fill, t.controlHover, hover);
                fill = mix(fill, t.controlPressed, press);
            }
        }

        QRectF surfaceRect = option->rect;
        if (textHelper) {
            const QRect logical = option->rect.adjusted(0, 4, -4, -4);
            surfaceRect = QStyle::visualRect(option->direction, option->rect, logical);
        }
        if (stroke.alpha() == 0) {
            roundedRect(painter, surfaceRect, fill, Qt::transparent, ControlRadius);
        } else if (role == ControlRole::Accent || role == ControlRole::Destructive
                   || ((option->state & QStyle::State_On) && role == ControlRole::Standard)) {
            controlSurface(painter, surfaceRect.toRect(), fill,
                           t.accentStroke, t.accentStrokeSecondary, ControlRadius);
        } else {
            controlSurface(painter, surfaceRect.toRect(), fill,
                           t.stroke, t.strokeSecondary, ControlRadius);
        }
        return true;
    }

    if (element == QStyle::PE_IndicatorCheckBox || element == QStyle::PE_IndicatorRadioButton) {
        const bool checked = option->state & (QStyle::State_On | QStyle::State_NoChange);
        const qreal checkAmount = progress(widget, checkProperty,
                                           checked ? 1.0 : 0.0);
        // WinUI's template owns a 20 x 20 logical indicator. Do not shrink
        // that layout slot before painting it: the one-pixel border is part
        // of the template geometry and UseLayoutRounding is disabled there.
        const QRectF indicator(option->rect);
        const qreal indicatorHover = progress(widget, hoverProperty, hovered ? 1.0 : 0.0);
        const qreal indicatorPress = progress(widget, pressProperty,
                                     option->state & QStyle::State_Sunken ? 1.0 : 0.0);
        QColor fill = mix(t.layer, t.accentFill, checkAmount);
        QColor stroke = mix(t.strokeStrong, t.accentFill, checkAmount);
        if (!enabled) {
            fill = mix(t.controlDisabled, t.accentFillDisabled, checkAmount);
            stroke = mix(t.textDisabled, t.accentFillDisabled, checkAmount);
        } else {
            const QColor hoverFill = mix(mix(t.layer, t.textPrimary, 0.05),
                                         t.accentFillHover, checkAmount);
            const QColor pressedFill = mix(t.controlPressed,
                                           t.accentFillPressed, checkAmount);
            fill = mix(fill, hoverFill, indicatorHover);
            fill = mix(fill, pressedFill, indicatorPress);
            if (checkAmount < 0.001)
                stroke = mix(stroke, t.textDisabled, indicatorPress);
        }
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setBrush(fill);
        painter->setPen(QPen(stroke, 1));
        if (element == QStyle::PE_IndicatorRadioButton)
            // Inset the centered one-pixel border inside the 20 px slot,
            // matching the CheckBox ring for equal edge sharpness.
            painter->drawEllipse(indicator.adjusted(0.5, 0.5, -0.5, -0.5));
        else
            // Keep the template slot at 20 px while keeping the centered
            // one-pixel border inside that slot, as the XAML Rectangle does.
            painter->drawRoundedRect(indicator.adjusted(0.5, 0.5, -0.5, -0.5),
                                     3, 3);

        if (checkAmount > 0.001) {
            const QColor onAccent = enabled ? t.textOnAccentPrimary
                                             : t.textOnAccentDisabled;
            if (element == QStyle::PE_IndicatorRadioButton) {
                painter->setBrush(onAccent);
                painter->setPen(Qt::NoPen);
                // WinUI: 12 px at rest, 14 px on pointer-over, 10 px pressed.
                const qreal diameter = ((12.0 + 2.0 * indicatorHover)
                    * (1.0 - indicatorPress) + 10.0 * indicatorPress) * checkAmount;
                painter->drawEllipse(snappedEllipseRect(indicator, diameter,
                                                        painter));
            } else if (option->state & QStyle::State_NoChange) {
                painter->setPen(QPen(onAccent, 1.1666667, Qt::SolidLine,
                                     Qt::RoundCap, Qt::RoundJoin));
                const qreal halfWidth = 5.0 * checkAmount;
                painter->drawLine(QPointF(indicator.center().x() - halfWidth,
                                          indicator.center().y()),
                                  QPointF(indicator.center().x() + halfWidth,
                                          indicator.center().y()));
            } else {
                // Port the generated AnimatedAcceptVisualSource geometry:
                // a 48 px canvas, 0.7 scale, (24,23) offset, rounded 4-unit
                // stroke. The source holds until 15/160 of its timeline and
                // completes the visible reveal over the fast transition.
                constexpr qreal hold = 15.0 / 160.0;
                // Mapping the generated 34/160 keyframe directly onto our
                // normalized progress compressed the stroke to about 20 ms,
                // making it appear instantaneous. Keep the official hold,
                // then use the remainder of the 167 ms transition.
                constexpr qreal end = 1.0;
                const qreal reveal = checkAmount <= hold
                    ? 0.0
                    : qBound<qreal>(0.0, (checkAmount - hold) / (end - hold),
                                    1.0);
                painter->setPen(QPen(onAccent, 4.0 * 0.7 * 20.0 / 48.0,
                                     Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
                if (reveal > 0.0)
                    painter->drawPath(animatedAcceptTrimmedPath(indicator, reveal));
            }
        }
        painter->restore();
        return true;
    }

    if (element == QStyle::PE_PanelLineEdit || element == QStyle::PE_FrameLineEdit) {
        // QAbstractSpinBox owns the complete NumberBox surface. Its private
        // QLineEdit must paint only text, cursor and selection; painting a
        // second TextBox panel here creates a nested rectangular "cell" on
        // hover and focus.
        if (spinBoxEditor(widget))
            return true;
        const bool focused = option->state & QStyle::State_HasFocus;
        const qreal lineEditHover = progress(widget, hoverProperty,
                                     option->state & QStyle::State_MouseOver ? 1.0 : 0.0);
        QColor fill = !enabled ? t.controlDisabled
            : focused ? (t.dark ? QColor(30, 30, 30, 179) : QColor(255, 255, 255))
                      : t.control;
        if (enabled && !focused)
            fill = mix(fill, t.controlHover, lineEditHover);
        controlSurface(painter, option->rect, fill, t.stroke, t.strokeSecondary,
                       ControlRadius);
        if (focused) {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            QPainterPath clip;
            clip.addRoundedRect(QRectF(option->rect).adjusted(0.5, 0.5, -0.5, -0.5),
                                ControlRadius, ControlRadius);
            painter->setClipPath(clip);
            painter->setPen(QPen(t.accentFill, 2.0, Qt::SolidLine, Qt::FlatCap));
            painter->drawLine(option->rect.left(), option->rect.bottom() - 1,
                              option->rect.right(), option->rect.bottom() - 1);
            painter->restore();
        }
        return true;
    }

    return false;
}

bool drawButtonControl(const Style *style, QStyle::ControlElement element,
                       const QStyleOption *option, QPainter *painter,
                       const QWidget *widget)
{
    if (element != QStyle::CE_PushButton && element != QStyle::CE_CheckBox
        && element != QStyle::CE_RadioButton
        && element != QStyle::CE_PushButtonLabel
        && element != QStyle::CE_ToolButtonLabel) {
        return false;
    }

    const Tokens t = tokens(option->palette);

    if (element == QStyle::CE_PushButton) {
        if (const auto *button = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            style->drawPrimitive(QStyle::PE_PanelButtonCommand, button, painter, widget);
            style->drawControl(QStyle::CE_PushButtonLabel, button, painter, widget);
            if (button->features & QStyleOptionButton::HasMenu) {
                const QRect logical(button->rect.right() - 22,
                                    button->rect.center().y() - 7, 14, 14);
                WinUI3::icon(Icon::ChevronDown,
                     button->state & QStyle::State_Enabled ? t.textPrimary : t.textDisabled)
                    .paint(painter, QStyle::visualRect(button->direction, button->rect, logical),
                           Qt::AlignCenter,
                           button->state & QStyle::State_Enabled
                               ? QIcon::Normal : QIcon::Disabled);
            }
            return true;
        }
        return false;
    }

    if ((element == QStyle::CE_CheckBox && !toggleSwitch(widget))
        || element == QStyle::CE_RadioButton) {
        if (const auto *button = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            const bool radio = element == QStyle::CE_RadioButton;
            QStyleOptionButton indicator = *button;
            indicator.rect = style->subElementRect(
                radio ? QStyle::SE_RadioButtonIndicator : QStyle::SE_CheckBoxIndicator,
                button, widget);
            style->drawPrimitive(radio ? QStyle::PE_IndicatorRadioButton
                                       : QStyle::PE_IndicatorCheckBox,
                          &indicator, painter, widget);

            QRect contents = style->subElementRect(
                radio ? QStyle::SE_RadioButtonContents : QStyle::SE_CheckBoxContents,
                button, widget);
            const bool enabled = button->state & QStyle::State_Enabled;
            if (!button->icon.isNull()) {
                const QSize iconSize = button->iconSize.isValid()
                    ? button->iconSize : QSize(16, 16);
                const QRect logical(contents.left(),
                    contents.center().y() - iconSize.height() / 2,
                    iconSize.width(), iconSize.height());
                const QRect iconRect = QStyle::visualRect(button->direction, contents, logical);
                paintThemedIcon(painter, button->icon, iconRect, Qt::AlignCenter,
                    enabled ? t.textPrimary : t.textDisabled,
                    enabled ? QIcon::Normal : QIcon::Disabled,
                    button->state & QStyle::State_On ? QIcon::On : QIcon::Off);
                if (button->direction == Qt::RightToLeft)
                    contents.setRight(iconRect.left() - 6);
                else
                    contents.setLeft(iconRect.right() + 6);
            }
            painter->save();
            painter->setFont(widget ? widget->font() : QApplication::font());
            painter->setPen(enabled ? t.textPrimary : t.textDisabled);
            painter->drawText(contents,
                QStyle::visualAlignment(button->direction, Qt::AlignLeft | Qt::AlignVCenter)
                    | Qt::TextShowMnemonic,
                button->text);
            if ((button->state & QStyle::State_HasFocus) && keyboardFocusVisible(widget)) {
                painter->setRenderHint(QPainter::Antialiasing);
                painter->setBrush(Qt::NoBrush);
                painter->setPen(QPen(t.focusOuter, 2));
                painter->drawRoundedRect(QRectF(button->rect).adjusted(1, 1, -1, -1),
                                         5, 5);
                painter->setPen(QPen(t.focusInner, 1));
                painter->drawRoundedRect(QRectF(button->rect).adjusted(3, 3, -3, -3),
                                         3, 3);
            }
            painter->restore();
            return true;
        }
        return false;
    }

    if (element == QStyle::CE_CheckBox && toggleSwitch(widget)) {
        if (const auto *check = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            const bool enabled = check->state & QStyle::State_Enabled;
            const bool checked = check->state & (QStyle::State_On | QStyle::State_NoChange);
            const qreal hover = progress(widget, hoverProperty,
                                         check->state & QStyle::State_MouseOver ? 1.0 : 0.0);
            const qreal press = progress(widget, pressProperty,
                                         check->state & QStyle::State_Sunken ? 1.0 : 0.0);
            const qreal position = progress(widget, togglePositionProperty,
                                            checked ? 1.0 : 0.0);
            const bool dragging = framePropertyRegistry()
                .value(widget, toggleDraggingProperty).toBool();

            // WinUI's template owns a 40 x 20 track. Snap the slot to whole
            // device pixels so the pill's flat top/bottom edges render as one
            // solid pixel row instead of splitting ~50/50 across two rows.
            QRectF track = toggleTrackRect(check->rect, check->direction);
            track = snappedRect(track, painter);
            QColor trackFill;
            QColor trackStroke;
            QColor knob;
            if (!enabled) {
                trackFill = checked ? t.accentFillDisabled : Qt::transparent;
                trackStroke = withAlpha(t.strokeStrong, 40);
                knob = withAlpha(checked ? t.textOnAccentDisabled : t.textDisabled, 150);
            } else if (dragging) {
                const QColor off = t.dark ? QColor(0, 0, 0, 25)
                                          : QColor(0, 0, 0, 6);
                trackFill = mix(off, t.accentFillPressed, position);
                trackStroke = mix(t.strokeStrong, t.accentFillPressed, position);
                knob = mix(t.textSecondary, t.textOnAccentPrimary, position);
            } else if (checked) {
                trackFill = mix(t.accentFill, t.accentFillHover, hover * (1.0 - press));
                trackFill = mix(trackFill, t.accentFillPressed, press);
                trackStroke = trackFill;
                knob = t.textOnAccentPrimary;
            } else {
                const QColor off = t.dark ? QColor(0, 0, 0, 25)
                                          : QColor(0, 0, 0, 6);
                const QColor offHover = t.dark ? QColor(255, 255, 255, 11)
                                               : QColor(0, 0, 0, 15);
                const QColor offPressed = t.dark ? QColor(255, 255, 255, 18)
                                                 : QColor(0, 0, 0, 24);
                trackFill = mix(off, offHover, hover * (1.0 - press));
                trackFill = mix(trackFill, offPressed, press);
                trackStroke = t.strokeStrong;
                knob = t.textSecondary;
            }
            roundedRect(painter, track, trackFill, trackStroke, 10.0);

            const qreal knobWidth = 12.0 + 2.0 * hover + 3.0 * press;
            const qreal knobHeight = 12.0 + 2.0 * hover;
            qreal visualPosition = position;
            if (check->direction == Qt::RightToLeft)
                visualPosition = 1.0 - visualPosition;
            const qreal knobCenter = track.left() + 10.0 + 20.0 * visualPosition;
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setPen(Qt::NoPen);
            painter->setBrush(knob);
            painter->drawEllipse(QRectF(knobCenter - knobWidth / 2.0,
                                        track.center().y() - knobHeight / 2.0,
                                        knobWidth, knobHeight));
            painter->restore();

            const QVariant stateText = widget->property(
                checked ? Style::ToggleSwitchOnTextProperty : Style::ToggleSwitchOffTextProperty);
            const QString label = stateText.isValid() ? stateText.toString() : check->text;
            if (!label.isEmpty()) {
                painter->setFont(widget->font());
                painter->setPen(enabled ? t.textPrimary : t.textDisabled);
                const QRect labelRect = check->direction == Qt::RightToLeft
                    ? check->rect.adjusted(0, 0, -50, 0)
                    : check->rect.adjusted(50, 0, 0, 0);
                const Qt::Alignment horizontal = check->direction == Qt::RightToLeft
                    ? Qt::AlignRight : Qt::AlignLeft;
                painter->drawText(labelRect,
                                  horizontal | Qt::AlignVCenter
                                      | Qt::TextShowMnemonic,
                                  label);
            }

            if (keyboardFocusVisible(widget)) {
                painter->save();
                painter->setRenderHint(QPainter::Antialiasing);
                painter->setBrush(Qt::NoBrush);
                painter->setPen(QPen(t.focusOuter, 2.0));
                painter->drawRoundedRect(track.adjusted(-3, -3, 3, 3), 12, 12);
                painter->setPen(QPen(t.focusInner, 1.0));
                painter->drawRoundedRect(track.adjusted(-1, -1, 1, 1), 11, 11);
                painter->restore();
            }
            return true;
        }
        return false;
    }

    if (element == QStyle::CE_PushButtonLabel
        || element == QStyle::CE_ToolButtonLabel) {
        if (element == QStyle::CE_PushButtonLabel) {
            if (const auto *button = qstyleoption_cast<const QStyleOptionButton *>(option)) {
                const bool enabled = button->state & QStyle::State_Enabled;
                const ControlRole role = Style::controlRole(widget);
                const bool accent = role == ControlRole::Accent
                    || role == ControlRole::Destructive
                    || ((button->state & QStyle::State_On) && role == ControlRole::Standard);
                const QColor textColor = !enabled ? t.textDisabled
                    : accent ? t.textOnAccentPrimary : t.textPrimary;
                QRect content = style->subElementRect(QStyle::SE_PushButtonContents, button, widget);
                if (button->features & QStyleOptionButton::HasMenu) {
                    const QRect logical = content.adjusted(0, 0, -22, 0);
                    content = QStyle::visualRect(button->direction, button->rect, logical);
                }
                const QFontMetrics metrics(button->fontMetrics);
                const int textWidth = button->text.isEmpty()
                    ? 0 : metrics.horizontalAdvance(button->text);
                const QSize iconSize = button->iconSize.isValid()
                    ? button->iconSize : QSize(16, 16);
                const bool hasIcon = !button->icon.isNull();
                const int gap = hasIcon && textWidth > 0 ? 8 : 0;
                const int totalWidth = (hasIcon ? iconSize.width() : 0)
                    + gap + textWidth;
                const int logicalStart = content.left()
                    + qMax(0, (content.width() - totalWidth) / 2);
                if (hasIcon) {
                    const QRect iconRect = QStyle::visualRect(button->direction, content,
                        QRect(logicalStart,
                              content.center().y() - iconSize.height() / 2,
                              iconSize.width(), iconSize.height()));
                    paintThemedIcon(painter, button->icon, iconRect,
                        Qt::AlignCenter, textColor,
                        enabled ? QIcon::Normal : QIcon::Disabled,
                        button->state & QStyle::State_On ? QIcon::On : QIcon::Off);
                }
                if (textWidth > 0) {
                    const QRect textRect = QStyle::visualRect(button->direction, content,
                        QRect(logicalStart
                                  + (hasIcon ? iconSize.width() + gap : 0),
                              content.top(), textWidth, content.height()));
                    painter->save();
                    painter->setFont(widget ? widget->font() : QApplication::font());
                    painter->setPen(textColor);
                    painter->drawText(textRect,
                                      Qt::AlignCenter | Qt::TextShowMnemonic,
                                      button->text);
                    painter->restore();
                }
                return true;
            }
            return false;
        } else if (const auto *tool = qstyleoption_cast<const QStyleOptionToolButton *>(option)) {
            const bool enabled = tool->state & QStyle::State_Enabled;
            const ControlRole role = Style::controlRole(widget);
            const bool accent = role == ControlRole::Accent
                || role == ControlRole::Destructive
                || ((tool->state & QStyle::State_On) && role == ControlRole::Standard);
            const bool textHelper = textBoxHelperButton(widget);
            const bool pressed = tool->state & QStyle::State_Sunken;
            const QColor textColor = !enabled ? t.textDisabled
                : textHelper ? (pressed ? t.textTertiary : t.textSecondary)
                : accent ? t.textOnAccentPrimary : t.textPrimary;
            const QRectF content = QRectF(style->subControlRect(QStyle::CC_ToolButton, tool,
                                                         QStyle::SC_ToolButton, widget))
                                       .adjusted(4.0, 2.0, -4.0, -2.0);
            Qt::ToolButtonStyle buttonStyle = Qt::ToolButtonIconOnly;
            if (const auto *toolButton = qobject_cast<const QToolButton *>(widget))
                buttonStyle = toolButton->toolButtonStyle();
            if (buttonStyle == Qt::ToolButtonFollowStyle)
                buttonStyle = Qt::ToolButtonIconOnly;
            const bool hasIcon = !tool->icon.isNull();
            const QSize iconSize = tool->iconSize.isValid() ? tool->iconSize : QSize(16, 16);
            const QFontMetrics metrics(tool->fontMetrics);
            const int textWidth = tool->text.isEmpty()
                ? 0 : metrics.horizontalAdvance(tool->text);
            if (buttonStyle == Qt::ToolButtonTextOnly)
                painter->setPen(textColor);
            if (buttonStyle == Qt::ToolButtonTextOnly || !hasIcon) {
                painter->save();
                painter->setFont(tool->font);
                painter->setPen(textColor);
                painter->drawText(content, Qt::AlignCenter | Qt::TextShowMnemonic,
                                  tool->text);
                painter->restore();
            } else if (buttonStyle == Qt::ToolButtonTextBesideIcon && textWidth > 0) {
                const qreal total = iconSize.width() + 6.0 + textWidth;
                const qreal logicalStart = content.left()
                    + qMax<qreal>(0.0, (content.width() - total) / 2.0);
                const QRectF iconRect = visualRectF(tool->direction, content,
                    QRectF(logicalStart,
                           content.center().y() - iconSize.height() / 2.0,
                           iconSize.width(), iconSize.height()));
                paintThemedIcon(painter, tool->icon,
                    iconRect, Qt::AlignCenter, textColor,
                    enabled ? QIcon::Normal : QIcon::Disabled,
                    tool->state & QStyle::State_On ? QIcon::On : QIcon::Off);
                const QRectF textRect = visualRectF(tool->direction, content,
                    QRectF(logicalStart + iconSize.width() + 6.0, content.top(),
                           textWidth, content.height()));
                painter->save();
                painter->setFont(tool->font);
                painter->setPen(textColor);
                painter->drawText(textRect,
                                  Qt::AlignCenter | Qt::TextShowMnemonic, tool->text);
                painter->restore();
            } else if (buttonStyle == Qt::ToolButtonTextUnderIcon && textWidth > 0) {
                paintThemedIcon(painter, tool->icon,
                    QRectF(content.center().x() - iconSize.width() / 2.0,
                           content.top(), iconSize.width(), iconSize.height()),
                    Qt::AlignCenter, textColor,
                    enabled ? QIcon::Normal : QIcon::Disabled,
                    tool->state & QStyle::State_On ? QIcon::On : QIcon::Off);
                painter->save();
                painter->setFont(tool->font);
                painter->setPen(textColor);
                painter->drawText(content.adjusted(0, iconSize.height(), 0, 0),
                                  Qt::AlignCenter | Qt::TextShowMnemonic, tool->text);
                painter->restore();
            } else if (hasIcon) {
                paintThemedIcon(painter, tool->icon,
                    QRectF(content.center().x() - iconSize.width() / 2.0,
                           content.center().y() - iconSize.height() / 2.0,
                           iconSize.width(), iconSize.height()), Qt::AlignCenter, textColor,
                    enabled ? QIcon::Normal : QIcon::Disabled,
                    tool->state & QStyle::State_On ? QIcon::On : QIcon::Off);
            }
            if (tool->features & QStyleOptionToolButton::MenuButtonPopup) {
                const QRect menuRect = style->subControlRect(QStyle::CC_ToolButton, tool,
                                                       QStyle::SC_ToolButtonMenu, widget);
                WinUI3::icon(Icon::ChevronDown, textColor).paint(painter, menuRect, Qt::AlignCenter,
                                             enabled ? QIcon::Normal : QIcon::Disabled);
            }
            return true;
        }
        return false;
    }

    return false;
}

} // namespace WinUI3::Private
