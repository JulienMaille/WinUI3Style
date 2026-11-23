// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <winui3style/winui3icons.h>

#include <QAbstractSpinBox>
#include <QStyle>

class QPainter;
class QStyleOptionComplex;
class QWidget;

namespace WinUI3 {
class Style;

namespace Private {
struct Tokens;

struct SpinButtonGlyphPair
{
    Icon up;
    Icon down;
};

// Qt's ButtonSymbols selects the step-button glyphs without changing the
// pinned inline/stacked NumberBox geometry or input states. Inline so the
// contract tests can call the same source of truth as the paint path.
inline SpinButtonGlyphPair spinButtonGlyphs(QAbstractSpinBox::ButtonSymbols symbols)
{
    return symbols == QAbstractSpinBox::PlusMinus
            ? SpinButtonGlyphPair{ Icon::Add, Icon::Remove }
            : SpinButtonGlyphPair{ Icon::ChevronUp, Icon::ChevronDown };
}

bool drawEditorComplexControl(const Style *style, QStyle::ComplexControl control,
                              const QStyleOptionComplex *option, QPainter *painter,
                              const QWidget *widget, const Tokens &tokens);

} // namespace Private
} // namespace WinUI3
