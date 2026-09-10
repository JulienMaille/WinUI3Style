// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include "winui3backdrop_p.h"

#include <QAbstractScrollArea>
#include <QObject>
#include <QPalette>
#include <QPointer>
#include <QScrollBar>
#include <QVariant>
#include <QWidget>

class QComboBox;
class QDialog;
class QLineEdit;
class QAbstractButton;
class QSlider;

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
void restoreContentSurfacesForBackdrop(QWidget *window);
// The no-fill recipe for any widget that paints straight onto the live
// material through a translucent island: transparent Window role, no
// autofill, StyledBackground so Qt leaves the erase to the style's
// Source-clear branch. State is remembered so toggle-off restores exactly.
void transparentizeForBackdrop(QWidget *surface);
void restoreTransparentizedForBackdrop(QWidget *surface);
// Small-delta scrolls inside a translucent content/layer island smear: Qt
// scrolls the viewport backing store with a blit and only repaints the
// exposed strip, so shifted pixels accumulate over the live material. Armed
// once per scroll area by the sync below; fires on the bars' valueChanged
// (all wheel/drag/bar paths) and schedules one queued full viewport update
// past the scroll. Gated on Composited so offscreen snapshots stay
// deterministic.
inline void guardIslandScrollArea(QAbstractScrollArea *area)
{
    if (!area || !area->viewport())
        return;
    constexpr auto guardProperty = "_winui_island_scroll_guard";
    if (area->property(guardProperty).isValid())
        return;
    area->setProperty(guardProperty, true);
    // Queued, not direct: the bars may emit several valueChanged ticks per
    // frame, and they must coalesce into one heal. repaint() (not update())
    // so the blitted rows rebuild before the next present; then heal the
    // viewport's children too, since a parent repaint clips children out and
    // each child keeps its own blitted rows.
    const auto heal = [viewport = QPointer<QWidget>(area->viewport())] {
        if (!viewport || !viewport->isVisible())
            return;
        if (backdropEffectiveSurface(viewport->window()) != BackdropSurface::Composited)
            return;
        viewport->repaint();
        const QList<QWidget *> children =
                viewport->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly);
        for (QWidget *child : children) {
            if (child->isVisible())
                child->repaint();
        }
    };
    QObject::connect(area->verticalScrollBar(), &QScrollBar::valueChanged, area->viewport(), heal,
                     Qt::QueuedConnection);
    QObject::connect(area->horizontalScrollBar(), &QScrollBar::valueChanged, area->viewport(),
                     heal, Qt::QueuedConnection);
}

} // namespace Private
} // namespace WinUI3
