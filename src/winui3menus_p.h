#pragma once

#include <QStyle>

class QPainter;
class QWidget;

namespace WinUI3 {
class Style;

namespace Private {

bool drawMenuPrimitive(const Style *style, QStyle::PrimitiveElement element,
                       const QStyleOption *option, QPainter *painter,
                       const QWidget *widget);
bool drawMenuControl(const Style *style, QStyle::ControlElement element,
                     const QStyleOption *option, QPainter *painter,
                     const QWidget *widget);

} // namespace Private
} // namespace WinUI3
