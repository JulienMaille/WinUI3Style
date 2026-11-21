// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <winui3style/winui3global.h>

class QAbstractButton;
class QLineEdit;
class QPainter;
class QStyleOption;

namespace WinUI3 {
class Style;

namespace Private {

// QLineEdit helper affordances: the private clear button and the QAction helper
// buttons, prepared through one coalesced pass. Definitions live in
// winui3lineedithelpers_p.cpp.
void updateReadOnlyDeleteAffordance(QLineEdit *lineEdit);
void prepareLineEditHelperButtons(QLineEdit *lineEdit, Style *style);
void cancelLineEditHelperUpdate(QLineEdit *lineEdit);
void cacheLineEditClearButton(QLineEdit *lineEdit, QAbstractButton *button);
const QAbstractButton *lineEditClearButton(const QLineEdit *lineEdit);

// KeyTip-derived keycaps for a QKeySequenceEdit's private line edit. There is
// no WinUI recorder control, so this is a consistency extension claimed
// through the TextBox contract (docs/coverage.md). The editor's live text is
// read at paint time, so keys appear while they are still being pressed, and
// the focused editor state supplies the pressed cap fill.
void paintShortcutKeycaps(QPainter *painter, const QStyleOption *option, const QLineEdit *editor);

} // namespace Private
} // namespace WinUI3
