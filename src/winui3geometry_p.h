// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <QRect>
#include <QRectF>
#include <QStyle>
#include "winui3density_p.h"
#include <optional>

class QStyleOption;
class QStyleOptionComplex;
class QWidget;

namespace WinUI3::Private {

std::optional<int> pixelMetricValue(QStyle::PixelMetric metric, bool toggleSwitch,
                                    const QWidget *widget = nullptr);

// The toggle track is a fixed 40 x 20 logical-pixel slot.  Keep its layout
// direction math in one place so painting and pointer interaction cannot
// disagree at the inclusive right edge of QRect.
QRect toggleTrackRect(const QRect &bounds, Qt::LayoutDirection direction);
QRect toggleTrackRect(const QRect &bounds, Qt::LayoutDirection direction, DensityMode mode);
// Toggle thumb center/size math beside toggleTrackRect so the paint path and
// any future hit-test consumer share one definition.
QRectF toggleKnobRect(const QRectF &track, qreal position, qreal hover, qreal press,
                      Qt::LayoutDirection direction);

std::optional<QRect> complexControlRect(QStyle::ComplexControl control,
                                        const QStyleOptionComplex *option,
                                        QStyle::SubControl subControl, const QWidget *widget);

std::optional<QStyle::SubControl> complexControlHitTest(QStyle::ComplexControl control,
                                                        const QStyleOptionComplex *option,
                                                        const QPoint &position,
                                                        const QWidget *widget);

} // namespace WinUI3::Private
