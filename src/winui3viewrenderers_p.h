// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <QStyle>

#include <functional>

#include <winui3style/winui3icons.h>

#include <QIcon>
#include <QPlainTextEdit>
#include <QTextEdit>
#include <QWidget>

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

// PE_Widget backdrop branches the facade used to own: calendar lane,
// card viewport / card interior, island erase and popup tint. Returns false
// when this widget is not a PE_Widget case, so the caller keeps its chain.
bool drawWidgetBackdropPrimitive(const Style *style, const QStyleOption *option, QPainter *painter,
                                 const QWidget *widget);
// Remaining PE_* elements that the facade used to own: focus ring, tooltip
// label, status bar, spin/arrow glyphs, view / rich-edit frames and the
// toolbar separator. Returns false when the element is not one of these, so
// the caller falls through to its assert + base style.
bool drawChromePrimitive(const Style *style, QStyle::PrimitiveElement element,
                         const QStyleOption *option, QPainter *painter, const QWidget *widget);
// Remaining CE_* elements that the facade used to own: SettingsCard frame,
// ComboBox label, ProgressBar groove/contents/label, ToolBar and SizeGrip.
// Returns false when the element is not one of these (or the option cast
// fails), so the caller falls through to its assert + base style.
bool drawChromeControl(const Style *style, QStyle::ControlElement element,
                       const QStyleOption *option, QPainter *painter, const QWidget *widget);

// Shared chrome helpers. Arrow primitive → WinUI chevron glyph mapping.
inline WinUI3::Icon arrowIcon(QStyle::PrimitiveElement element)
{
    switch (element) {
    case QStyle::PE_IndicatorArrowDown:
        return WinUI3::Icon::ChevronDown;
    case QStyle::PE_IndicatorArrowLeft:
        return WinUI3::Icon::ChevronLeft;
    case QStyle::PE_IndicatorArrowRight:
        return WinUI3::Icon::ChevronRight;
    case QStyle::PE_IndicatorArrowUp:
        return WinUI3::Icon::ChevronUp;
    default:
        return WinUI3::Icon::ChevronRight;
    }
}

// Nearest QTextEdit/QPlainTextEdit ancestor (rich-editor frame painting).
inline const QWidget *richTextEditor(const QWidget *widget)
{
    for (const QWidget *candidate = widget; candidate; candidate = candidate->parentWidget()) {
        if (qobject_cast<const QTextEdit *>(candidate)
            || qobject_cast<const QPlainTextEdit *>(candidate))
            return candidate;
    }
    return nullptr;
}

} // namespace Private
} // namespace WinUI3
