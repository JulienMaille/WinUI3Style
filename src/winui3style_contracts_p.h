#pragma once

#include <QIcon>
#include <QSize>
#include <QStyle>

class QWidget;

namespace WinUI3 {
class Style;

namespace Private {

int pixelMetric(const Style *style, QStyle::PixelMetric metric,
                const QStyleOption *option, const QWidget *widget);
QSize sizeFromContents(const Style *style, QStyle::ContentsType type,
                       const QStyleOption *option, const QSize &contentsSize,
                       const QWidget *widget);
QRect subElementRect(const Style *style, QStyle::SubElement element,
                     const QStyleOption *option, const QWidget *widget);
QRect subControlRect(const Style *style, QStyle::ComplexControl control,
                     const QStyleOptionComplex *option,
                     QStyle::SubControl subControl, const QWidget *widget);
int styleHint(const Style *style, QStyle::StyleHint hint,
              const QStyleOption *option, const QWidget *widget,
              QStyleHintReturn *returnData);
QIcon standardIcon(const Style *style, QStyle::StandardPixmap standard,
                   const QStyleOption *option, const QWidget *widget);

} // namespace Private
} // namespace WinUI3
