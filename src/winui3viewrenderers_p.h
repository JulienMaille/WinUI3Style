// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <QStyle>

#include <functional>

class QAbstractItemView;
class QPainter;
class QTableView;
class QWidget;
class QModelIndex;

namespace WinUI3 {
class Style;

namespace Private {

using TableEditorOverlap =
        std::function<bool(const QTableView *, const QModelIndex &, const QRect &)>;

bool drawViewPrimitive(const Style *style, QStyle::PrimitiveElement element,
                       const QStyleOption *option, QPainter *painter, const QWidget *widget);
// drawViewControl: tableEditorOverlap may be empty/null, which means "no
// overlap check" (not an error). The paint path must guard with
// `if (tableEditorOverlap)` before invoking it; calling an empty
// std::function would throw std::bad_function_call in the paint path.
bool drawViewControl(const Style *style, QStyle::ControlElement element, const QStyleOption *option,
                     QPainter *painter, const QWidget *widget,
                     const TableEditorOverlap &tableEditorOverlap);

} // namespace Private
} // namespace WinUI3
