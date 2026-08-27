#pragma once

#include <QStyle>

class QPainter;
class QWidget;

namespace WinUI3 {
class Style;

namespace Private {

bool drawButtonPrimitive(const Style *style, QStyle::PrimitiveElement element,
                         const QStyleOption *option, QPainter *painter,
                         const QWidget *widget);
bool drawButtonControl(const Style *style, QStyle::ControlElement element,
                       const QStyleOption *option, QPainter *painter,
                       const QWidget *widget);

} // namespace Private
} // namespace WinUI3
