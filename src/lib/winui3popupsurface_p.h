// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

// Popup surface lifecycle: palette/attribute ownership for Qt::Popup windows
// (ComboBox flyouts, QMenu popups, calendar pickers, AutoSuggest), the DWM
// acrylic grant readback, and the combo first-frame anchoring helpers. Split
// out of winui3surfaces_p.cpp so the backdrop sync and the popup recipe no
// longer share one translation unit.

#include <QStyle>

class QString;
class QComboBox;
class QMenu;
class QWidget;

namespace WinUI3 {
class Style;

namespace Private {

// Prepares one popup window's surface for its next Show: remembers the
// pre-show palette/attribute recipe, rebases the palette on the current
// application palette, then either claims the opaque deterministic fallback
// or re-asserts the composited acrylic recipe. GUI-thread-only; may be called
// repeatedly for the same popup (remember-once semantics).
void preparePopupSurface(QWidget *widget);
// Enables a real alpha surface for QMenu on Wayland, where QWidget masks only
// constrain input and opaque regions, not the rendered buffer. Call before
// the popup's native surface is created.
bool prepareWaylandMenuSurface(QWidget *widget, const QString &platformName);
// Anchors a combo popup's container height and selected-row geometry on its
// first show at the current density.
void prepareComboPopupFirstFrameImpl(QComboBox *combo);
// Resolves the QComboBox owning a popup widget, or null.
QComboBox *comboForPopupWidget(QWidget *widget);
// Clips a Qt::Popup QMenu to OverlayRadius when the native DWM corner
// preference is unavailable (submenu HWNDs materialize after Show). Called
// from preparePopupSurface for menu popups and from the menu interaction path.
void applyMenuRoundedMask(QMenu *menu);

} // namespace Private
} // namespace WinUI3
