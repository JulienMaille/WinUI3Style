// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <QColor>
#include <QIcon>
#include <QPainterPath>
#include <QRectF>
#include <QStyleOption>

class QPainter;

namespace WinUI3::PaintPrivate {

// Stateless drawing and device-pixel alignment primitives shared by the
// style facade and private item delegates. Keeping these here avoids each
// rendering path growing a subtly different copy of the same operation.
void roundedRect(QPainter *painter, const QRectF &rect, const QColor &fill, const QColor &stroke,
                 qreal radius, qreal strokeWidth = 1.0);

// Inside-stroke outline for shapes that own no fill pass here (checkbox and
// radio indicators, slider thumb ring, popup frames): one call draws the
// stroke fully inside the given rect so the outer pixel keeps the parent
// fill, like roundedRect above. Factor of the per-site adjusted(0.5) copies.
void roundedOutline(QPainter *painter, const QRectF &rect, const QColor &stroke, qreal radius,
                    qreal strokeWidth = 1.0);
void paintThemedIcon(QPainter *painter, const QIcon &source, const QRectF &rect,
                     Qt::Alignment alignment, const QColor &foreground,
                     QIcon::Mode mode = QIcon::Normal, QIcon::State state = QIcon::Off);

QRectF visualRectF(Qt::LayoutDirection direction, const QRectF &bounds, const QRectF &logical);

void controlSurface(QPainter *painter, const QRectF &rect, const QColor &fill,
                    const QColor &strokeTop, const QColor &strokeBottom, qreal radius,
                    qreal strokeWidth = 1.0);

// Draw the WinUI editor focus underline: a 2 px accent line along the bottom
// edge, clipped to the control's rounded-corner path so the ends follow the
// chamfer instead of running edge-to-edge. Shared by TextBox and NumberBox.
void drawEditorFocusUnderline(QPainter *painter, const QRectF &rect, const QColor &accent,
                              qreal radius);

// WinUI keyboard focus ring: 2 px outer + 1 px inner rounded rects. The
// per-site adjusted() insets and radii are copied into the call args, never
// unified by judgment; visual diffs hide there.
void paintFocusRing(QPainter *painter, const QRectF &rect, const QColor &outer, const QColor &inner,
                    qreal outerInset, qreal innerInset, qreal outerRadius, qreal innerRadius);

QRectF snappedEllipseRect(const QRectF &logicalBounds, qreal logicalDiameter,
                          const QPainter *painter);

// Snap a rect's origin to a whole device pixel while preserving its logical
// size. Unlike snappedEllipseRect, the rect's edges (not just the center) are
// aligned, giving crisp flat segments on pill-shaped surfaces.
QRectF snappedRect(const QRectF &logicalRect, const QPainter *painter);

QPointF animatedAcceptPoint(const QRectF &indicator, qreal x, qreal y);
QPainterPath animatedAcceptPath(const QRectF &indicator);
QPainterPath animatedAcceptTrimmedPath(const QRectF &indicator, qreal progress);

QRect headerSortIndicatorRect(const QStyleOptionHeader &header);

QRectF snappedSplitterGrip(const QRectF &grip, bool horizontal, const QPainter *painter);

// WinUI's 12 px dropdown chevron: 10 px Segoe Fluent artwork centered in a
// 12 px box. For full-width buttons the box sits 14 px from the trailing
// edge; inside a split-button dropdown half it centers in the half
// (official SplitButton: chevron horizontally centered, right padding 0).
// One definition for the ComboBox arrow, PushButton/ToolButton menu
// indicators and submenu chevrons so every dropdown glyph renders
// identically.
void paintDropdownChevron(QPainter *painter, const QIcon &source, const QRect &bounds,
                          Qt::LayoutDirection direction, const QColor &foreground,
                          QIcon::Mode mode = QIcon::Normal, QIcon::State state = QIcon::Off,
                          bool centerInBounds = false);

} // namespace WinUI3::PaintPrivate
