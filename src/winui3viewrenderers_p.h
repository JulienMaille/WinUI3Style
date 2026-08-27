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

using TableEditorOverlap = std::function<bool(const QTableView *,
                                             const QModelIndex &,
                                             const QRect &)>;

bool drawViewPrimitive(const Style *style, QStyle::PrimitiveElement element,
                       const QStyleOption *option, QPainter *painter,
                       const QWidget *widget);
bool drawViewControl(const Style *style, QStyle::ControlElement element,
                     const QStyleOption *option, QPainter *painter,
                     const QWidget *widget,
                     const TableEditorOverlap &tableEditorOverlap);

} // namespace Private
} // namespace WinUI3
