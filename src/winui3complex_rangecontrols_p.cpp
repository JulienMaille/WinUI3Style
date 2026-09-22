// SPDX-License-Identifier: LGPL-2.1-or-later
#include "winui3complex_rangecontrols_p.h"

#include "winui3helpers_p.h"
#include "winui3paint_p.h"
#include "winui3style_properties_p.h"
#include "winui3tokens_p.h"

#include <winui3style/winui3icons.h>
#include <winui3style/winui3style.h>

#include <QGroupBox>
#include <QPainter>
#include <QStyleOptionSlider>

namespace WinUI3::Private {
using namespace PaintPrivate;

namespace {

bool isInsideOpaqueCard(const QWidget *widget)
{
    for (const QWidget *ancestor = widget ? widget->parentWidget() : nullptr; ancestor;
         ancestor = ancestor->parentWidget()) {
        if (qobject_cast<const QGroupBox *>(ancestor)
            && ancestor->palette().color(QPalette::Window).alpha() != 0) {
            return true;
        }
    }
    return false;
}

} // namespace

bool drawRangeComplexControl(const Style *style, QStyle::ComplexControl control,
                             const QStyleOptionComplex *option, QPainter *painter,
                             const QWidget *widget, const Tokens &t)
{
    if (control == QStyle::CC_Slider) {
        if (const auto *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            // Sliders paint only groove/thumb/focus: no opaque fill of their
            // own. Erasing to transparent is correct straight onto the live
            // material, but inside an opaque group card it would punch a hole
            // through the card fill (the mica-mode band). Group cards carry
            // no SurfaceProperty, so the gate cannot see them: skip the erase
            // when an opaque QGroupBox ancestor intervenes and keep the card
            // fill the parent already painted.
            const bool insideOpaqueCard = isInsideOpaqueCard(widget);
            if (!insideOpaqueCard)
                eraseForBackdrop(painter, widget, option->rect);
            const bool horizontal = slider->orientation == Qt::Horizontal;
            QRect groove = style->subControlRect(QStyle::CC_Slider, slider, QStyle::SC_SliderGroove,
                                                 widget);
            const QRect handle = style->subControlRect(QStyle::CC_Slider, slider,
                                                       QStyle::SC_SliderHandle, widget);
            const qreal hover = progress(widget, hoverProperty,
                                         option->state & QStyle::State_MouseOver ? 1.0 : 0.0);
            const qreal pressed = progress(widget, pressProperty,
                                           option->state & QStyle::State_Sunken ? 1.0 : 0.0);
            const bool enabled = option->state & QStyle::State_Enabled;
            QColor track = enabled ? t.strokeStrong : t.textDisabled;
            QColor valueColor =
                    enabled ? mix(t.accentFill, t.accentFillHover, hover) : t.accentFillDisabled;
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

            if (slider->tickPosition != QSlider::NoTicks && slider->maximum > slider->minimum) {
                const qint64 minimum = slider->minimum;
                const qint64 maximum = slider->maximum;
                const qint64 range = maximum - minimum;
                const qint64 requested = slider->tickInterval > 0
                        ? qint64(slider->tickInterval)
                        : qint64(qMax(1, slider->pageStep));
                const qint64 interval = qMax<qint64>(1, qMax(requested, (range + 99) / 100));
                painter->save();
                painter->setPen(QPen(enabled ? t.strokeStrong : t.textDisabled, 1));
                for (qint64 tick = minimum;;) {
                    const int span = horizontal ? groove.width() - 1 : groove.height() - 1;
                    const int offset = QStyle::sliderPositionFromValue(
                            slider->minimum, slider->maximum, int(qBound(minimum, tick, maximum)),
                            span, slider->upsideDown);
                    if (horizontal) {
                        const int x = groove.left() + offset;
                        if (slider->tickPosition & QSlider::TicksAbove)
                            painter->drawLine(x, groove.top() - 8, x, groove.top() - 5);
                        if (slider->tickPosition & QSlider::TicksBelow)
                            painter->drawLine(x, groove.bottom() + 5, x, groove.bottom() + 8);
                    } else {
                        const int y = groove.top() + offset;
                        if (slider->tickPosition & QSlider::TicksLeft)
                            painter->drawLine(groove.left() - 8, y, groove.left() - 5, y);
                        if (slider->tickPosition & QSlider::TicksRight)
                            painter->drawLine(groove.right() + 5, y, groove.right() + 8, y);
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
            const QColor outerThumb = t.sliderThumbOuter;
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
                painter->drawRoundedRect(QRectF(option->rect).adjusted(1, 1, -1, -1), ControlRadius,
                                         ControlRadius);
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
            for (const QWidget *ancestor = widget ? widget->parentWidget() : nullptr; ancestor;
                 ancestor = ancestor->parentWidget()) {
                if (!qobject_cast<const QGroupBox *>(ancestor))
                    continue;
                QColor opaqueLayer = t.layer;
                const qreal opacity = opaqueLayer.alphaF();
                opaqueLayer.setAlpha(255);
                background = mix(background, opaqueLayer, opacity);
            }
            // Ghost scrollbar: a scrollbar painted straight onto a live
            // backdrop keeps its backing-store head across frames. Rebuild
            // from transparent first so the rest state never smears. WinUI
            // keeps the ScrollBar groove transparent at rest: once the
            // hover track fades out there is no fill left, so erase-only
            // with no repainted background lets the parent show through
            // instead of a mismatched Window-role band on live Mica.
            // Group cards carry no SurfaceProperty, so the gate cannot see
            // them (same guard as CC_Slider above): inside an opaque group
            // card the groove is transparent to the card fill, not to the
            // window material, and a bare erase punches a Mica hole through
            // the veil. Rebuild the veiled card tone instead (the erase
            // still clears stale hover/thumb frames); outer bars keep the
            // erase-only rest. Hover track and thumb below are unchanged.
            const bool insideOpaqueCard = isInsideOpaqueCard(widget);
            const bool onBackdrop = eraseForBackdrop(painter, widget, option->rect);
            const bool enabled = option->state & QStyle::State_Enabled;
            if (!onBackdrop)
                painter->fillRect(option->rect, background);
            else if (insideOpaqueCard)
                painter->fillRect(option->rect, veiledCard(t.layer, true));
            // The WinUI ScrollBarThumb template fades the thumb to zero in its
            // Disabled state; arrows and track are suppressed with it.
            if (!enabled)
                return true;
            const QRect thumb = style->subControlRect(QStyle::CC_ScrollBar, scroll,
                                                      QStyle::SC_ScrollBarSlider, widget);
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
            roundedRect(painter, visualThumb, thumbColor, Qt::transparent, thickness / 2.0);

            if (expanded > 0.001) {
                const QRect decrease = style->subControlRect(QStyle::CC_ScrollBar, scroll,
                                                             QStyle::SC_ScrollBarSubLine, widget);
                const QRect increase = style->subControlRect(QStyle::CC_ScrollBar, scroll,
                                                             QStyle::SC_ScrollBarAddLine, widget);
                const bool pressed = option->state & QStyle::State_Sunken;
                const auto drawArrow = [&](const QRect &rect, QStyle::SubControl sub, Icon glyph) {
                    if (!(scroll->subControls & sub))
                        return;
                    const bool active = (scroll->activeSubControls & sub)
                            && (option->state & QStyle::State_MouseOver);
                    if (active) {
                        const QColor fill = pressed ? t.subtlePressed : t.subtleHover;
                        roundedRect(painter, QRectF(rect).adjusted(2, 2, -2, -2), fill,
                                    Qt::transparent, 3);
                    }
                    painter->save();
                    painter->setOpacity(expanded);
                    QRect glyphRect(rect.center().x() - 4, rect.center().y() - 4, 8, 8);
                    if (active && pressed)
                        glyphRect = QRect(rect.center().x() - 3, rect.center().y() - 3, 7, 7);
                    icon(glyph, t.textPrimary)
                            .paint(painter, glyphRect, Qt::AlignCenter, QIcon::Normal);
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
