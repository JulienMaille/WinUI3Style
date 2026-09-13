// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <QColor>
#include <QPainter>
#include <QRectF>
#include <QStyle>
#include <QStyleOption>
#include <QWidget>

namespace WinUI3 {
class Style;

namespace Private {

bool drawMenuPrimitive(const Style *style, QStyle::PrimitiveElement element,
                       const QStyleOption *option, QPainter *painter, const QWidget *widget);
bool drawMenuControl(const Style *style, QStyle::ControlElement element, const QStyleOption *option,
                     QPainter *painter, const QWidget *widget);

// Shared popup-row pill: combo (CE_MenuItem path) and item-view popup
// rows (PE_PanelItemViewItem path) paint one identical inset pill so hover
// edges match everywhere. Combo evidence pins Margin="5,2,5,2" with a 3px
// item radius (observations.md); menu rows reuse the same pill as a
// consistency extension (no MenuFlyoutItem margin entry in the pinned
// resources). Caller passes the already-inset row rect (menu 4,2 /
// combo 5,2); the pill is always the 3px rounding, so mouse hover and
// keyboard current read as one shape on every popup.
void paintPopupRowPill(QPainter *painter, const QRectF &itemRect, const QColor &fill);

} // namespace Private
} // namespace WinUI3
