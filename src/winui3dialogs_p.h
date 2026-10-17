// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <winui3style/winui3global.h>

class QDialog;

namespace WinUI3 {
namespace Private {

// ContentDialog surfaces: the dialog state arm/restore, its command footer
// surface and the smoke scrim. Definitions live in winui3dialogs_p.cpp.
void stopDialogAnimations(QDialog *dialog);
void restoreContentDialogState(QDialog *dialog, bool clearSavedState);
void prepareContentDialogState(QDialog *dialog, bool dark);
void showContentDialogScrim(QDialog *dialog);
void hideContentDialogScrim(QDialog *dialog);

} // namespace Private
} // namespace WinUI3
