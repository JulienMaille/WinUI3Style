// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <QStyle>

class QPainter;
class QWidget;

namespace WinUI3 {
class Style;

namespace Private {

// Handled-vs-fallback contract: these helpers return true when the element
// was fully painted by the WinUI implementation (the caller must return and
// must NOT fall through to the base QStyle), and false when the element is
// not covered here (the caller must fall back to QProxyStyle/base painting).
bool drawButtonPrimitive(const Style *style, QStyle::PrimitiveElement element,
                         const QStyleOption *option, QPainter *painter, const QWidget *widget);
bool drawButtonControl(const Style *style, QStyle::ControlElement element,
                       const QStyleOption *option, QPainter *painter, const QWidget *widget);

} // namespace Private
} // namespace WinUI3
