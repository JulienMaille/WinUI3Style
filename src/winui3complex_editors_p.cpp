// SPDX-License-Identifier: LGPL-2.1-or-later
#include "winui3complex_editors_p.h"

#include "winui3helpers_p.h"
#include "winui3paint_p.h"
#include "winui3style_properties_p.h"
#include "winui3tokens_p.h"

#include <winui3style/winui3icons.h>
#include <winui3style/winui3style.h>

#include <QAbstractSpinBox>
#include <QLineEdit>
#include <QPainter>
#include <QStyleOptionComboBox>
#include <QStyleOptionSpinBox>

namespace WinUI3::Private {
using namespace PaintPrivate;

bool drawEditorComplexControl(const Style *style, QStyle::ComplexControl control,
                              const QStyleOptionComplex *option, QPainter *painter,
                              const QWidget *widget, const Tokens &t)
{
    if (control == QStyle::CC_ComboBox) {
        if (const auto *combo = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            // Rebuild the frame from transparent over a live material (see
            // eraseForBackdrop); otherwise hover/press fills accumulate. No
            // radius clip: the fill below repaints the full frame every pass.
            eraseForBackdrop(painter, widget, combo->rect);
            if (!paintsDirectlyOnBackdrop(widget) && widget && widget->parentWidget()
                && widget->parentWidget()->property(Style::SurfaceProperty).isValid()) {
                painter->fillRect(combo->rect,
                                  widget->parentWidget()->palette().color(QPalette::Window));
            }
            const bool enabled = combo->state & QStyle::State_Enabled;
            const bool hovered = combo->state & QStyle::State_MouseOver;
            const bool pressed = combo->state & (QStyle::State_Sunken | QStyle::State_On);
            const bool editable = combo->editable;
            const QLineEdit *comboEditor =
                    editable ? widget ? widget->findChild<QLineEdit *>() : nullptr : nullptr;
            const bool editableFocused = editable && enabled
                    && (combo->state & QStyle::State_HasFocus
                        || (widget && widget->isActiveWindow() && comboEditor
                            && comboEditor->hasFocus()));
            QColor fill = enabled ? t.control : t.controlDisabled;
            // An editable ComboBox behaves like a TextBox once it owns
            // keyboard focus: flat light surface, no hover tint.
            if (!editableFocused) {
                const qreal hover = progress(widget, hoverProperty, hovered ? 1.0 : 0.0);
                const qreal press = progress(widget, pressProperty, pressed ? 1.0 : 0.0);
                fill = mix(fill, t.controlHover, hover);
                fill = mix(fill, t.controlPressed, press);
            } else {
                fill = t.editorFocusedFill;
            }
            if (combo->subControls & QStyle::SC_ComboBoxFrame)
                controlSurface(painter, combo->rect, fill, t.stroke, t.strokeSecondary,
                               ControlRadius);
            if (combo->subControls & QStyle::SC_ComboBoxArrow) {
                const qreal chevron = progress(widget, comboChevronProperty, 0.0);
                painter->save();
                painter->translate(0.0, 1.875 * chevron);
                paintDropdownChevron(painter, icon(Icon::ChevronDown), combo->rect,
                                     combo->direction, enabled ? t.textPrimary : t.textDisabled,
                                     enabled ? QIcon::Normal : QIcon::Disabled);
                painter->restore();
            }
            if (editableFocused)
                drawEditorFocusUnderline(painter, combo->rect, t.accentFill, ControlRadius);
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
            eraseForBackdrop(painter, widget, spin->rect);
            const bool enabled = spin->state & QStyle::State_Enabled;
            const bool focused = spin->state & QStyle::State_HasFocus;
            const bool verticalButtons = verticalSpinButtons(widget);
            const qreal hover = progress(widget, hoverProperty,
                                         spin->state & QStyle::State_MouseOver ? 1.0 : 0.0);
            QColor fill = !enabled ? t.controlDisabled
                    : focused      ? t.editorFocusedFill
                                   : mix(t.control, t.controlHover, hover);
            controlSurface(painter, spin->rect, fill, t.stroke, t.strokeSecondary, ControlRadius);
            if (verticalButtons) {
                const QRect editField = style->subControlRect(QStyle::CC_SpinBox, spin,
                                                              QStyle::SC_SpinBoxEditField, widget);
                const int separatorX =
                        spin->direction == Qt::RightToLeft ? editField.left() : editField.right();
                painter->save();
                painter->setPen(QPen(t.stroke, 1, Qt::SolidLine, Qt::FlatCap));
                painter->drawLine(separatorX, spin->rect.top() + 1, separatorX,
                                  spin->rect.bottom() - 1);
                painter->restore();
            }
            if (focused)
                drawEditorFocusUnderline(painter, spin->rect, t.accentFill, ControlRadius);
            const auto drawStep = [&](QStyle::SubControl subControl, Icon glyph) {
                if (!(spin->subControls & subControl))
                    return;
                const QRect rect =
                        style->subControlRect(QStyle::CC_SpinBox, spin, subControl, widget);
                const bool stepEnabled = enabled
                        && (subControl == QStyle::SC_SpinBoxUp
                                    ? spin->stepEnabled & QAbstractSpinBox::StepUpEnabled
                                    : spin->stepEnabled & QAbstractSpinBox::StepDownEnabled);
                const QRectF visualRect = verticalButtons
                        ? (subControl == QStyle::SC_SpinBoxUp ? QRectF(rect).adjusted(4, 3, -4, 0)
                                                              : QRectF(rect).adjusted(4, 0, -4, -3))
                        : (subControl == QStyle::SC_SpinBoxUp
                                   ? QRectF(rect).adjusted(4, 4, 0, -4)
                                   : QRectF(rect).adjusted(0, 4, -4, -4));
                if (stepEnabled && (spin->activeSubControls & subControl)
                    && (spin->state & QStyle::State_MouseOver)) {
                    roundedRect(painter, visualRect,
                                spin->state & QStyle::State_Sunken ? t.subtlePressed
                                                                   : t.subtleHover,
                                Qt::transparent, ControlRadius);
                }
                const QPoint center = visualRect.center().toPoint();
                icon(glyph, stepEnabled ? t.textPrimary : t.textDisabled)
                        .paint(painter, QRect(center.x() - 6, center.y() - 6, 12, 12),
                               Qt::AlignCenter, stepEnabled ? QIcon::Normal : QIcon::Disabled);
            };
            drawStep(QStyle::SC_SpinBoxUp, Icon::ChevronUp);
            drawStep(QStyle::SC_SpinBoxDown, Icon::ChevronDown);
            return true;
        }
    }

    return false;
}

} // namespace WinUI3::Private
