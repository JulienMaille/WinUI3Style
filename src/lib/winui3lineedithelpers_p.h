// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <winui3style/winui3global.h>

class QAbstractButton;
class QLineEdit;

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

} // namespace Private
} // namespace WinUI3
