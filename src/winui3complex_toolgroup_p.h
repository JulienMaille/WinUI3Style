// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <QStyle>

class QPainter;
class QStyleOptionComplex;
class QWidget;

namespace WinUI3 {
class Style;

namespace Private {
struct Tokens;

bool drawToolGroupComplexControl(const Style *style, QStyle::ComplexControl control,
                                 const QStyleOptionComplex *option, QPainter *painter,
                                 const QWidget *widget, const Tokens &tokens);

} // namespace Private
} // namespace WinUI3
