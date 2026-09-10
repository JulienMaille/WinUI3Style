// SPDX-License-Identifier: LGPL-2.1-or-later
#include "winui3buttons_p.h"

#include "winui3paint_p.h"
#include "winui3frameproperties_p.h"
#include "winui3geometry_p.h"
#include "winui3helpers_p.h"
#include "winui3style_properties_p.h"
#include "winui3surfaces_p.h"
#include "winui3tokens_p.h"

#include <winui3style/winui3icons.h>
#include <winui3style/winui3style.h>

#include <QAbstractButton>
#include <QAbstractSpinBox>
#include <QApplication>
#include <QCheckBox>
#include <QCommandLinkButton>
#include <QComboBox>
#include <QGroupBox>
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

#include <cmath>

namespace WinUI3::Private {
using namespace PaintPrivate;

namespace {

bool spinBoxEditor(const QWidget *widget)
{
    return qobject_cast<const QLineEdit *>(widget)
            && qobject_cast<const QAbstractSpinBox *>(widget->parentWidget());
}

bool comboBoxEditor(const QWidget *widget)
{
    return qobject_cast<const QLineEdit *>(widget)
            && qobject_cast<const QComboBox *>(widget->parentWidget());
}

QRectF devicePixelCenteredRect(const QPainter *painter, const QRectF &rect)
{
    if (!painter || rect.isEmpty())
        return rect;
    bool invertible = false;
    const QTransform device = painter->deviceTransform();
    const QTransform logical = device.inverted(&invertible);
    if (!invertible)
        return rect;
    QRectF pixels = device.mapRect(rect);
    pixels.setLeft(std::floor(pixels.left()) + 0.5);
    pixels.setTop(std::floor(pixels.top()) + 0.5);
    pixels.setRight(std::ceil(pixels.right()) - 0.5);
    pixels.setBottom(std::ceil(pixels.bottom()) - 0.5);
    return logical.mapRect(pixels);
}

void drawEditorScopedClearSurface(const QStyleOption *option, QPainter *painter,
                                  const QLineEdit *lineEdit, const QColor &fill)
{
    const auto *button = lineEditClearButton(lineEdit);
    if (!option || !painter || !button)
        return;

    QRectF surface(
            button->geometry().translated(option->rect.topLeft() - lineEdit->rect().topLeft()));
    surface.setTop(option->rect.top() + 3.0);
    // QRectF's bottom/right edges are geometric (exclusive for the final
    // raster row/column); subtract two from the QRect's inclusive edge so
    // the visible surface occupies rows/columns through the three-pixel
    // outline inset.
    surface.setBottom(option->rect.bottom() - 2.0);
    const qreal side = surface.height();
    if (option->direction == Qt::RightToLeft) {
        surface.setLeft(option->rect.left() + 3.0);
        surface.setRight(surface.left() + side);
    } else {
        surface.setRight(option->rect.right() - 2.0);
        surface.setLeft(surface.right() - side);
    }
    surface = surface.intersected(option->rect);
    if (!surface.isEmpty())
        roundedRect(painter, surface, fill, Qt::transparent, ControlRadius);
}

} // namespace

bool drawButtonPrimitive(const Style *, QStyle::PrimitiveElement element,
                         const QStyleOption *option, QPainter *painter, const QWidget *widget)
{
    if (element != QStyle::PE_PanelButtonCommand && element != QStyle::PE_PanelButtonTool
        && element != QStyle::PE_IndicatorCheckBox && element != QStyle::PE_IndicatorRadioButton
        && element != QStyle::PE_PanelLineEdit && element != QStyle::PE_FrameLineEdit) {
        return false;
    }

    const Tokens t = tokens(option->palette);
    const bool enabled = option->state & QStyle::State_Enabled;
    const bool hovered = enabled && (option->state & QStyle::State_MouseOver);
    const bool pressed = enabled && (option->state & QStyle::State_Sunken);
    const qreal hover = enabled ? progress(widget, hoverProperty, hovered ? 1.0 : 0.0) : 0.0;
    // State_Sunken is the authoritative instantaneous state. The property is
    // an animation/pulse cache and can briefly still contain zero when Qt has
    // entered the pressed state (notably on rapid press/reversal sequences).
    const qreal press = enabled
            ? qMax(progress(widget, pressProperty, pressed ? 1.0 : 0.0), pressed ? 1.0 : 0.0)
            : 0.0;

    if (element == QStyle::PE_PanelButtonCommand || element == QStyle::PE_PanelButtonTool) {
        // Subtle-at-rest is the WinUI no-fill case: the parent surface must
        // show straight through. Erase the hover ghost, then repaint the
        // parent card tone when inside a group card (directly on the
        // material transparent is correct). Interactive states keep the
        // standard erase-then-fill path below.
        if ((Style::controlRole(widget) == ControlRole::Subtle
             || Style::controlRole(widget) == ControlRole::Navigation)
            && paintsDirectlyOnBackdrop(widget)
            && !(option->state & (QStyle::State_MouseOver | QStyle::State_Sunken))
            && progress(widget, hoverProperty, 0.0) < 0.01
            && progress(widget, pressProperty, 0.0) < 0.01) {
            eraseForBackdrop(painter, widget, option->rect, ControlRadius);
            for (const QWidget *ancestor = widget ? widget->parentWidget() : nullptr; ancestor;
                 ancestor = ancestor->parentWidget()) {
                if (!qobject_cast<const QGroupBox *>(ancestor))
                    continue;
                roundedRect(painter, QRectF(option->rect), veiledCard(t.layer, true),
                            Qt::transparent, ControlRadius);
                break;
            }
            return true;
        }
        // The private clear button of a QLineEdit must not paint its own
        // small hover surface: PE_PanelLineEdit already paints the
        // editor-scoped full-height clear surface, so a per-button surface
        // shows up as an unwanted little square around the glyph.
        if (const auto *widgetButton = qobject_cast<const QAbstractButton *>(widget)) {
            if (const auto *lineEdit =
                        qobject_cast<const QLineEdit *>(widgetButton->parentWidget())) {
                if (lineEditClearButton(lineEdit) == widgetButton)
                    return true;
            }
        }
        // A translucent QWidget backing store retains previous SourceOver
        // pixels. Rebuild button frames from transparent when the button sits
        // directly on DWM Mica; otherwise repeated hover frames accumulate
        // into a visible ghost. Opaque content/layer ancestors are excluded.
        // No radius clip: the fill below repaints the full frame shape every
        // pass, so a clipped erase would leave stale hover pixels outside
        // the rounded path (the accumulate-contract failure).
        eraseForBackdrop(painter, widget, option->rect);
        const ControlRole role = Style::controlRole(widget);
        const bool textHelper = textBoxHelperButton(widget);
        const bool toolbarButton = element == QStyle::PE_PanelButtonTool && widget
                && qobject_cast<const QToolBar *>(widget->parentWidget());
        // Subtle toolbar buttons intentionally have a transparent resting
        // fill. On a native backdrop that transparency cannot erase the
        // previous animated hover frame, so restore the declared opaque
        // parent surface before painting the new state.
        if (element == QStyle::PE_PanelButtonTool && widget && widget->parentWidget()
            && widget->parentWidget()->property(Style::SurfaceProperty).isValid()) {
            painter->fillRect(option->rect,
                              widget->parentWidget()->palette().color(QPalette::Window));
        }
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
                       && (toolbarButton || textHelper))) {
            if (option->state & QStyle::State_On) {
                // Command-bar toggle buttons retain a quiet selected surface,
                // but PointerOver/Pressed remain distinct visual states.  The
                // previous late assignment erased both interaction states.
                const QColor checked = mix(t.subtleHover, t.accentFill, 0.14);
                const QColor checkedHover = mix(t.subtleHover, t.accentFillHover, 0.14);
                const QColor checkedPressed = mix(t.subtlePressed, t.accentFillPressed, 0.14);
                fill = mix(checked, checkedHover, hover);
                fill = mix(fill, checkedPressed, press);
            } else {
                fill = Qt::transparent;
                fill = mix(fill, t.subtleHover, hover);
                fill = mix(fill, t.subtlePressed, press);
            }
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
            // Keep a local fallback for the private child and action-backed
            // helpers; the private clear button also gets its full-height
            // editor-scoped surface from PE_PanelLineEdit above.
            const QRect logical = option->rect.adjusted(0, 3, -3, -3);
            surfaceRect = QStyle::visualRect(option->direction, option->rect, logical);
        } else if (toolbarButton) {
            // A fill-only rounded rect drawn exactly on a QWidget backing-
            // store boundary can lose the antialiased half-pixel on one side,
            // making one pair of corners look square.  Keep the 4 px WinUI
            // radius but place all four edges on equivalent device-pixel
            // centres at fractional scaling factors.
            surfaceRect = devicePixelCenteredRect(painter, surfaceRect);
        }
        if (stroke.alpha() == 0) {
            roundedRect(painter, surfaceRect, fill, Qt::transparent, ControlRadius);
        } else if (role == ControlRole::Accent || role == ControlRole::Destructive
                   || ((option->state & QStyle::State_On) && role == ControlRole::Standard)) {
            controlSurface(painter, surfaceRect.toRect(), fill, t.accentStroke,
                           t.accentStrokeSecondary, ControlRadius);
        } else {
            controlSurface(painter, surfaceRect.toRect(), fill, t.stroke, t.strokeSecondary,
                           ControlRadius);
        }
        return true;
    }

    if (element == QStyle::PE_IndicatorCheckBox || element == QStyle::PE_IndicatorRadioButton) {
        const bool checked = option->state & (QStyle::State_On | QStyle::State_NoChange);
        const qreal checkAmount = progress(widget, checkProperty, checked ? 1.0 : 0.0);
        // WinUI changes the rectangle's fill and stroke with discrete
        // keyframes at state entry. Only AnimatedAcceptVisualSource is
        // progressive; fading the accent surface made the whole control feel
        // slower even with the correct 167 ms glyph duration.
        const qreal stateAmount = checked ? 1.0 : 0.0;
        // WinUI's template owns a 20 x 20 logical indicator. Do not shrink
        // that layout slot before painting it: the one-pixel border is part
        // of the template geometry and UseLayoutRounding is disabled there.
        const QRectF indicator(option->rect);
        const qreal indicatorHover = progress(widget, hoverProperty, hovered ? 1.0 : 0.0);
        const qreal indicatorPress =
                progress(widget, pressProperty, option->state & QStyle::State_Sunken ? 1.0 : 0.0);
        QColor fill = mix(t.layer, t.accentFill, stateAmount);
        QColor stroke = mix(t.strokeStrong, t.accentFill, stateAmount);
        if (!enabled) {
            fill = mix(t.controlDisabled, t.accentFillDisabled, stateAmount);
            stroke = mix(t.textDisabled, t.accentFillDisabled, stateAmount);
        } else {
            const QColor hoverFill =
                    mix(mix(t.layer, t.textPrimary, 0.05), t.accentFillHover, stateAmount);
            const QColor pressedFill = mix(t.controlPressed, t.accentFillPressed, stateAmount);
            fill = mix(fill, hoverFill, indicatorHover);
            fill = mix(fill, pressedFill, indicatorPress);
            if (!checked)
                stroke = mix(stroke, t.textDisabled, indicatorPress);
        }
        // Fill to the slot edge, then the inside-stroke outline: the outer
        // pixel keeps the parent fill, like every roundedRect surface.
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(Qt::NoPen);
        painter->setBrush(fill);
        if (element == QStyle::PE_IndicatorRadioButton)
            painter->drawEllipse(indicator);
        else
            painter->drawRoundedRect(indicator, 3, 3);
        painter->restore();
        roundedOutline(painter, indicator, stroke, 3);

        if (checkAmount > 0.001) {
            const QColor onAccent = enabled ? t.controlOnAccentPrimary : t.controlOnAccentDisabled;
            if (element == QStyle::PE_IndicatorRadioButton) {
                painter->setBrush(onAccent);
                painter->setPen(Qt::NoPen);
                // WinUI: 12 px at rest, 14 px on pointer-over, 10 px pressed.
                const qreal diameter = ((12.0 + 2.0 * indicatorHover) * (1.0 - indicatorPress)
                                        + 10.0 * indicatorPress)
                        * checkAmount;
                painter->drawEllipse(snappedEllipseRect(indicator, diameter, painter));
            } else if (option->state & QStyle::State_NoChange) {
                painter->setPen(
                        QPen(onAccent, 1.1666667, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
                const qreal halfWidth = 5.0 * checkAmount;
                painter->drawLine(
                        QPointF(indicator.center().x() - halfWidth, indicator.center().y()),
                        QPointF(indicator.center().x() + halfWidth, indicator.center().y()));
            } else {
                // Port the generated AnimatedAcceptVisualSource geometry:
                // a 48 px canvas, 0.7 scale, (24,23) offset, rounded 4-unit
                // stroke. AnimatedIcon jumps to the Start marker (frame 15)
                // before playback, so the visible frame-15..34 reveal maps
                // directly onto this transition with no additional hold.
                const qreal reveal = checkAmount;
                painter->setPen(QPen(onAccent, 4.0 * 0.7 * 20.0 / 48.0, Qt::SolidLine, Qt::RoundCap,
                                     Qt::RoundJoin));
                if (reveal > 0.0) {
                    // The generated accept glyph is anchored slightly above the
                    // indicator's geometric center; nudge it down one logical
                    // pixel to match WinUI's optical centering.
                    painter->translate(0.0, 1.0);
                    painter->drawPath(animatedAcceptTrimmedPath(indicator, reveal));
                }
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
        // The same applies to the QLineEdit embedded in an editable
        // QComboBox: CC_ComboBox paints the whole surface (including the
        // focus underline), so a second panel here shows up as a nested
        // "double outline".
        if (comboBoxEditor(widget))
            return true;
        // Same erase as buttons/combos/groups: the editor sits directly
        // on the live material and Qt keeps backing-store rows between
        // repaints (blit + partial expose). Without a clear the fill
        // re-blends over stale light rows: white field on hover in dark,
        // and a stuck light fossil after a Light->Dark switch (only a
        // resize reallocates and heals).
        eraseForBackdrop(painter, widget, option->rect, ControlRadius);
        if (widget && widget->parentWidget()
            && widget->parentWidget()->property(Style::SurfaceProperty).isValid()
            && !paintsDirectlyOnBackdrop(widget)) {
            painter->fillRect(option->rect,
                              widget->parentWidget()->palette().color(QPalette::Window));
        }
        const bool focused = option->state & QStyle::State_HasFocus;
        const qreal lineEditHover = progress(widget, hoverProperty,
                                             option->state & QStyle::State_MouseOver ? 1.0 : 0.0);
        // Keep the focused surface opaque (see CC_ComboBox): the old
        // translucent dark fill washed out toward white over a light
        // material.
        QColor fill = !enabled ? t.controlDisabled : focused ? t.editorFocusedFill : t.control;
        if (enabled && !focused)
            fill = mix(fill, t.controlHover, lineEditHover);
        controlSurface(painter, option->rect, fill, t.stroke, t.strokeSecondary, ControlRadius);
        if (const auto *lineEdit = qobject_cast<const QLineEdit *>(widget)) {
            const auto *clearButton = lineEditClearButton(lineEdit);
            const qreal clearHover = progress(clearButton, hoverProperty, 0.0);
            const qreal clearPress = progress(clearButton, pressProperty, 0.0);
            QColor clearFill = mix(Qt::transparent, t.subtleHover, clearHover);
            clearFill = mix(clearFill, t.subtlePressed, clearPress);
            if (clearHover > 0.001 || clearPress > 0.001)
                drawEditorScopedClearSurface(option, painter, lineEdit, clearFill);
        }
        if (focused)
            drawEditorFocusUnderline(painter, QRectF(option->rect), t.accentFill, ControlRadius);
        return true;
    }

    return false;
}

bool drawButtonControl(const Style *style, QStyle::ControlElement element,
                       const QStyleOption *option, QPainter *painter, const QWidget *widget)
{
    if (element != QStyle::CE_PushButton && element != QStyle::CE_CheckBox
        && element != QStyle::CE_RadioButton && element != QStyle::CE_PushButtonLabel
        && element != QStyle::CE_ToolButtonLabel) {
        return false;
    }

    const Tokens t = tokens(option->palette);

    if (element == QStyle::CE_PushButton) {
        if (const auto *button = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            style->drawPrimitive(QStyle::PE_PanelButtonCommand, button, painter, widget);
            style->drawControl(QStyle::CE_PushButtonLabel, button, painter, widget);
            if (button->features & QStyleOptionButton::HasMenu) {
                paintDropdownChevron(
                        painter, WinUI3::icon(Icon::ChevronDown), button->rect, button->direction,
                        button->state & QStyle::State_Enabled ? t.textPrimary : t.textDisabled,
                        button->state & QStyle::State_Enabled ? QIcon::Normal : QIcon::Disabled,
                        button->state & QStyle::State_On ? QIcon::On : QIcon::Off);
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
            indicator.rect = style->subElementRect(radio ? QStyle::SE_RadioButtonIndicator
                                                         : QStyle::SE_CheckBoxIndicator,
                                                   button, widget);
            style->drawPrimitive(radio ? QStyle::PE_IndicatorRadioButton
                                       : QStyle::PE_IndicatorCheckBox,
                                 &indicator, painter, widget);

            QRect contents = style->subElementRect(radio ? QStyle::SE_RadioButtonContents
                                                         : QStyle::SE_CheckBoxContents,
                                                   button, widget);
            const bool enabled = button->state & QStyle::State_Enabled;
            if (!button->icon.isNull()) {
                const QSize iconSize =
                        button->iconSize.isValid() ? button->iconSize : QSize(16, 16);
                const QRect logical(contents.left(), contents.center().y() - iconSize.height() / 2,
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
            painter->drawText(
                    contents,
                    QStyle::visualAlignment(button->direction, Qt::AlignLeft | Qt::AlignVCenter)
                            | Qt::TextShowMnemonic,
                    button->text);
            if ((button->state & QStyle::State_HasFocus) && keyboardFocusVisible(widget)) {
                paintFocusRing(painter, QRectF(button->rect), t.focusOuter, t.focusInner, 1, 3, 5,
                               3);
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
            const qreal position = progress(widget, togglePositionProperty, checked ? 1.0 : 0.0);
            const bool dragging =
                    framePropertyRegistry().value(widget, toggleDraggingProperty).toBool();

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
                knob = withAlpha(checked ? t.controlOnAccentDisabled : t.textDisabled, 150);
            } else if (dragging) {
                trackFill = mix(t.toggleOff, t.accentFillPressed, position);
                trackStroke = mix(t.strokeStrong, t.accentFillPressed, position);
                knob = mix(t.textSecondary, t.controlOnAccentPrimary, position);
            } else if (checked) {
                trackFill = mix(t.accentFill, t.accentFillHover, hover * (1.0 - press));
                trackFill = mix(trackFill, t.accentFillPressed, press);
                trackStroke = trackFill;
                knob = t.controlOnAccentPrimary;
            } else {
                trackFill = mix(t.toggleOff, t.toggleOffHover, hover * (1.0 - press));
                trackFill = mix(trackFill, t.toggleOffPressed, press);
                trackStroke = t.strokeStrong;
                knob = t.textSecondary;
            }
            roundedRect(painter, track, trackFill, trackStroke, 10.0);

            const QRectF knobRect = toggleKnobRect(track, position, hover, press, check->direction);
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setPen(Qt::NoPen);
            painter->setBrush(knob);
            painter->drawEllipse(knobRect);
            painter->restore();

            const QVariant stateText =
                    widget->property(checked ? Style::ToggleSwitchOnTextProperty
                                             : Style::ToggleSwitchOffTextProperty);
            const QString label = stateText.isValid() ? stateText.toString() : check->text;
            if (!label.isEmpty()) {
                painter->setFont(widget->font());
                painter->setPen(enabled ? t.textPrimary : t.textDisabled);
                const QRect labelRect = check->direction == Qt::RightToLeft
                        ? check->rect.adjusted(0, 0, -50, 0)
                        : check->rect.adjusted(50, 0, 0, 0);
                const Qt::Alignment horizontal =
                        check->direction == Qt::RightToLeft ? Qt::AlignRight : Qt::AlignLeft;
                painter->drawText(labelRect, horizontal | Qt::AlignVCenter | Qt::TextShowMnemonic,
                                  label);
            }
            if (keyboardFocusVisible(widget))
                paintFocusRing(painter, track, t.focusOuter, t.focusInner, -3, -1, 12, 11);
            return true;
        }
        return false;
    }

    if (element == QStyle::CE_PushButtonLabel || element == QStyle::CE_ToolButtonLabel) {
        if (element == QStyle::CE_PushButtonLabel) {
            if (const auto *button = qstyleoption_cast<const QStyleOptionButton *>(option)) {
                const bool enabled = button->state & QStyle::State_Enabled;
                const ControlRole role = Style::controlRole(widget);
                const bool accent = role == ControlRole::Accent || role == ControlRole::Destructive
                        || ((button->state & QStyle::State_On) && role == ControlRole::Standard);
                const bool pressed = enabled && (button->state & QStyle::State_Sunken);
                const QColor textColor = !enabled ? t.textDisabled
                        : accent ? (pressed ? t.textOnAccentSecondary : t.textOnAccentPrimary)
                                 : (pressed ? t.textSecondary : t.textPrimary);
                QRect content =
                        style->subElementRect(QStyle::SE_PushButtonContents, button, widget);
                if (qobject_cast<const QCommandLinkButton *>(widget)) {
                    // QCommandLinkButton paints its title and description
                    // after asking the style for CE_PushButtonLabel. Drawing
                    // its icon or either string here duplicates the
                    // widget-owned command-link contents. The surrounding
                    // Fluent surface remains entirely style-owned.
                    return true;
                }
                if (button->features & QStyleOptionButton::HasMenu) {
                    const QRect logical = content.adjusted(0, 0, -22, 0);
                    content = QStyle::visualRect(button->direction, button->rect, logical);
                }
                const QFontMetrics metrics(button->fontMetrics);
                const int textWidth =
                        button->text.isEmpty() ? 0 : metrics.horizontalAdvance(button->text);
                const QSize iconSize =
                        button->iconSize.isValid() ? button->iconSize : QSize(16, 16);
                const bool hasIcon = !button->icon.isNull();
                const int gap = hasIcon && textWidth > 0 ? 8 : 0;
                const int totalWidth = (hasIcon ? iconSize.width() : 0) + gap + textWidth;
                const int logicalStart =
                        content.left() + qMax(0, (content.width() - totalWidth) / 2);
                if (hasIcon) {
                    const QRect iconRect = QStyle::visualRect(
                            button->direction, content,
                            QRect(logicalStart, content.center().y() - iconSize.height() / 2,
                                  iconSize.width(), iconSize.height()));
                    paintThemedIcon(painter, button->icon, iconRect, Qt::AlignCenter, textColor,
                                    enabled ? QIcon::Normal : QIcon::Disabled,
                                    button->state & QStyle::State_On ? QIcon::On : QIcon::Off);
                }
                if (textWidth > 0) {
                    const QRect textRect = QStyle::visualRect(
                            button->direction, content,
                            QRect(logicalStart + (hasIcon ? iconSize.width() + gap : 0),
                                  content.top(), textWidth, content.height()));
                    painter->save();
                    painter->setFont(widget ? widget->font() : QApplication::font());
                    painter->setPen(textColor);
                    painter->drawText(textRect, Qt::AlignCenter | Qt::TextShowMnemonic,
                                      button->text);
                    painter->restore();
                }
                return true;
            }
            return false;
        } else if (const auto *tool = qstyleoption_cast<const QStyleOptionToolButton *>(option)) {
            const bool enabled = tool->state & QStyle::State_Enabled;
            const ControlRole role = Style::controlRole(widget);
            const bool accent = role == ControlRole::Accent || role == ControlRole::Destructive
                    || ((tool->state & QStyle::State_On) && role == ControlRole::Standard);
            const bool textHelper = textBoxHelperButton(widget);
            const auto *helperEditor =
                    widget ? qobject_cast<const QLineEdit *>(widget->parentWidget()) : nullptr;
            const bool clearHelper = helperEditor && lineEditClearButton(helperEditor) == widget;
            const bool pressed = tool->state & QStyle::State_Sunken;
            const QColor textColor = !enabled ? t.textDisabled
                    : textHelper              ? (pressed ? t.textTertiary : t.textSecondary)
                    : accent ? (pressed ? t.textOnAccentSecondary : t.textOnAccentPrimary)
                             : (pressed ? t.textSecondary : t.textPrimary);
            const QRectF buttonRect(style->subControlRect(QStyle::CC_ToolButton, tool,
                                                          QStyle::SC_ToolButton, widget));
            // WinUI's 30 px DeleteButton contains a 12 px E894 glyph inside
            // TextBoxInnerButtonMargin 0,4,4,4. Mirror that asymmetric margin
            // in RTL instead of centring Qt's default 16 px icon in the slot.
            const QRectF content = clearHelper
                    ? visualRectF(tool->direction, buttonRect,
                                  buttonRect.adjusted(0.0, 4.0, -4.0, -4.0))
                    : buttonRect.adjusted(4.0, 2.0, -4.0, -2.0);
            Qt::ToolButtonStyle buttonStyle = Qt::ToolButtonIconOnly;
            if (const auto *toolButton = qobject_cast<const QToolButton *>(widget))
                buttonStyle = toolButton->toolButtonStyle();
            if (buttonStyle == Qt::ToolButtonFollowStyle)
                buttonStyle = Qt::ToolButtonIconOnly;
            const bool hasIcon = !tool->icon.isNull();
            const QSize iconSize = clearHelper
                    ? QSize(12, 12)
                    : (tool->iconSize.isValid() ? tool->iconSize : QSize(16, 16));
            const QFontMetrics metrics(tool->fontMetrics);
            const int textWidth = tool->text.isEmpty() ? 0 : metrics.horizontalAdvance(tool->text);
            if (buttonStyle == Qt::ToolButtonTextOnly || !hasIcon) {
                painter->save();
                painter->setFont(tool->font);
                painter->setPen(textColor);
                painter->drawText(content, Qt::AlignCenter | Qt::TextShowMnemonic, tool->text);
                painter->restore();
            } else if (buttonStyle == Qt::ToolButtonTextBesideIcon && textWidth > 0) {
                const qreal total = iconSize.width() + 6.0 + textWidth;
                const qreal logicalStart =
                        content.left() + qMax<qreal>(0.0, (content.width() - total) / 2.0);
                const QRectF iconRect = visualRectF(
                        tool->direction, content,
                        QRectF(logicalStart, content.center().y() - iconSize.height() / 2.0,
                               iconSize.width(), iconSize.height()));
                paintThemedIcon(painter, tool->icon, iconRect, Qt::AlignCenter, textColor,
                                enabled ? QIcon::Normal : QIcon::Disabled,
                                tool->state & QStyle::State_On ? QIcon::On : QIcon::Off);
                const QRectF textRect =
                        visualRectF(tool->direction, content,
                                    QRectF(logicalStart + iconSize.width() + 6.0, content.top(),
                                           textWidth, content.height()));
                painter->save();
                painter->setFont(tool->font);
                painter->setPen(textColor);
                painter->drawText(textRect, Qt::AlignCenter | Qt::TextShowMnemonic, tool->text);
                painter->restore();
            } else if (buttonStyle == Qt::ToolButtonTextUnderIcon && textWidth > 0) {
                paintThemedIcon(painter, tool->icon,
                                QRectF(content.center().x() - iconSize.width() / 2.0, content.top(),
                                       iconSize.width(), iconSize.height()),
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
                QRectF iconRect(content.center().x() - iconSize.width() / 2.0,
                                content.center().y() - iconSize.height() / 2.0, iconSize.width(),
                                iconSize.height());
                // Segoe Fluent's E894 ink sits three pixels above/left of its
                // nominal 12 px advance box in Qt. WinUI's GlyphElement
                // compensates through its text layout; match the visible ink
                // centre explicitly in the private QLineEdit button.
                if (clearHelper) {
                    // The private Qt button is not centred in the editor-scoped
                    // DeleteButton surface.  WinUI's asymmetric 0,4,4,4 inner
                    // margin requires a small trailing/down compensation, but
                    // the previous 3 px shift put the X visibly low and right.
                    iconRect.translate(2.0, 1.0);
                }
                paintThemedIcon(painter, tool->icon, iconRect, Qt::AlignCenter, textColor,
                                enabled ? QIcon::Normal : QIcon::Disabled,
                                tool->state & QStyle::State_On ? QIcon::On : QIcon::Off);
            }
            if (tool->features & QStyleOptionToolButton::MenuButtonPopup) {
                // Center the shared chevron in the dropdown half (official
                // SplitButton: centered, right padding 0), never on the
                // divider line.
                const QRect menuZone = style->subControlRect(QStyle::CC_ToolButton, tool,
                                                             QStyle::SC_ToolButtonMenu, widget);
                paintDropdownChevron(painter, WinUI3::icon(Icon::ChevronDown), menuZone,
                                     tool->direction, textColor,
                                     enabled ? QIcon::Normal : QIcon::Disabled,
                                     tool->state & QStyle::State_On ? QIcon::On : QIcon::Off, true);
            }
            return true;
        }
        return false;
    }

    return false;
}

} // namespace WinUI3::Private
