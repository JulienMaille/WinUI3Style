// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <QPalette>
#include <QVariant>

class QComboBox;
class QDialog;
class QLineEdit;
class QAbstractButton;
class QSlider;
class QWidget;

namespace WinUI3 {
class Style;

namespace Private {

void remember(QWidget *widget, const char *property, const QVariant &value);
void rememberPalette(QWidget *widget);
void restoreRememberedPalette(QWidget *widget);
QPalette effectivePopupPalette(QWidget *widget, const QPalette &fallback);

void stopDialogAnimations(QDialog *dialog);
void restoreContentDialogState(QDialog *dialog, bool clearSavedState);
void prepareContentDialogState(QDialog *dialog, bool dark);
void showContentDialogScrim(QDialog *dialog);
void hideContentDialogScrim(QDialog *dialog);

void updateReadOnlyDeleteAffordance(QLineEdit *lineEdit);
void prepareLineEditHelperButtons(QLineEdit *lineEdit, Style *style);
void cancelLineEditHelperUpdate(QLineEdit *lineEdit);
void cacheLineEditClearButton(QLineEdit *lineEdit, QAbstractButton *button);
const QAbstractButton *lineEditClearButton(const QLineEdit *lineEdit);
void showSliderValueToolTip(QSlider *slider);
void hideSliderValueToolTip(QSlider *slider);

void preparePopupSurface(QWidget *widget);
void prepareComboPopupFirstFrameImpl(QComboBox *combo);
QComboBox *comboForPopupWidget(QWidget *widget);

// Window chrome (menu bar, tool bars, status bar) reveals the live DWM
// material instead of painting opaque panels over it. Content/layer
// islands follow under the full-Mica contract. Both are no-ops for
// windows without matching descendants, and restore returns every
// touched widget to its remembered palette, attributes and autofill.
void makeChromeSurfacesTransparent(QWidget *window);
void restoreChromeSurfaces(QWidget *window);
void syncContentSurfacesForBackdrop(QWidget *window);

} // namespace Private
} // namespace WinUI3
