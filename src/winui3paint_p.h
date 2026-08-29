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
void roundedRect(QPainter *painter, const QRectF &rect, const QColor &fill,
                 const QColor &stroke, qreal radius,
                 qreal strokeWidth = 1.0);

void paintThemedIcon(QPainter *painter, const QIcon &source, const QRectF &rect,
                     Qt::Alignment alignment, const QColor &foreground,
                     QIcon::Mode mode = QIcon::Normal,
                     QIcon::State state = QIcon::Off);

QRectF visualRectF(Qt::LayoutDirection direction, const QRectF &bounds,
                   const QRectF &logical);

void controlSurface(QPainter *painter, const QRectF &rect, const QColor &fill,
                    const QColor &strokeTop, const QColor &strokeBottom,
                    qreal radius, qreal strokeWidth = 1.0);

// Draw the WinUI editor focus underline: a 2 px accent line along the bottom
// edge, clipped to the control's rounded-corner path so the ends follow the
// chamfer instead of running edge-to-edge. Shared by TextBox and NumberBox.
void drawEditorFocusUnderline(QPainter *painter, const QRectF &rect,
                              const QColor &accent, qreal radius);

QRectF snappedEllipseRect(const QRectF &logicalBounds, qreal logicalDiameter,
                          const QPainter *painter);

// Snap a rect's origin to a whole device pixel while preserving its logical
// size. Unlike snappedEllipseRect, the rect's edges (not just the center) are
// aligned, giving crisp flat segments on pill-shaped surfaces.
QRectF snappedRect(const QRectF &logicalRect, const QPainter *painter);

QPointF animatedAcceptPoint(const QRectF &indicator, qreal x, qreal y);
QPainterPath animatedAcceptPath(const QRectF &indicator);
QPainterPath animatedAcceptTrimmedPath(const QRectF &indicator,
                                        qreal progress);

QRect headerSortIndicatorRect(const QStyleOptionHeader &header);

QRectF snappedSplitterGrip(const QRectF &grip, bool horizontal,
                           const QPainter *painter);

} // namespace WinUI3::PaintPrivate
