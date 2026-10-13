// SPDX-License-Identifier: LGPL-2.1-or-later
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
// Pre-erase shared by the facade's uncovered complex-control fallback path.
bool eraseComplexBackdrop(QStyle::ComplexControl control, const QStyleOptionComplex *option,
                          QPainter *painter, const QWidget *widget);

} // namespace Private
} // namespace WinUI3
