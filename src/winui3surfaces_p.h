#pragma once

#include <QPalette>
#include <QVariant>

class QComboBox;
class QDialog;
class QLineEdit;
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

void updateReadOnlyDeleteAffordance(QLineEdit *lineEdit);
void prepareLineEditHelperButtons(QLineEdit *lineEdit, Style *style);
void showSliderValueToolTip(QSlider *slider);
void hideSliderValueToolTip(QSlider *slider);

void preparePopupSurface(QWidget *widget);
void prepareComboPopupFirstFrameImpl(QComboBox *combo);
QComboBox *comboForPopupWidget(QWidget *widget);

} // namespace Private
} // namespace WinUI3
