#pragma once

#include <QRect>
#include <QStyle>
#include <optional>

class QStyleOption;
class QStyleOptionComplex;
class QWidget;

namespace WinUI3::Private
{

std::optional<int> pixelMetricValue(QStyle::PixelMetric metric,
                                    bool toggleSwitch);

std::optional<QRect> complexControlRect(QStyle::ComplexControl control,
                                        const QStyleOptionComplex *option,
                                        QStyle::SubControl subControl,
                                        const QWidget *widget);

std::optional<QStyle::SubControl>
complexControlHitTest(QStyle::ComplexControl control,
                      const QStyleOptionComplex *option, const QPoint &position,
                      const QWidget *widget);

} // namespace WinUI3::Private
