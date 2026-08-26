#pragma once

#include <QStyle>

class QPainter;
class QWidget;

namespace WinUI3 {
class Style;

namespace Private {

bool drawComplexControl(const Style *style, QStyle::ComplexControl control,
                        const QStyleOptionComplex *option, QPainter *painter,
                        const QWidget *widget);
bool coveredComplex(QStyle::ComplexControl control);

} // namespace Private
} // namespace WinUI3
