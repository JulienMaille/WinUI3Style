// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <winui3style/winui3global.h>

class QDialog;
class QPalette;
class QWizard;

namespace WinUI3 {
namespace Private {

// Dialog surfaces: wizard/content-dialog footer painting, dialog state
// arm/restore and the smoke scrim. Definitions live in winui3dialogs_p.cpp.
void stopDialogAnimations(QDialog *dialog);
void restoreContentDialogState(QDialog *dialog, bool clearSavedState);
void prepareContentDialogState(QDialog *dialog, bool dark);
void showContentDialogScrim(QDialog *dialog);
void hideContentDialogScrim(QDialog *dialog);
void refreshWizardSurface(QWizard *wizard, const QPalette &applicationPalette);
void deleteWizardFooterSurface(QWizard *wizard);

} // namespace Private
} // namespace WinUI3
