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

QRectF snappedEllipseRect(const QRectF &logicalBounds, qreal logicalDiameter,
                          const QPainter *painter);

QPointF animatedAcceptPoint(const QRectF &indicator, qreal x, qreal y);
QPainterPath animatedAcceptPath(const QRectF &indicator);
QPainterPath animatedAcceptTrimmedPath(const QRectF &indicator,
                                        qreal progress);

QRect headerSortIndicatorRect(const QStyleOptionHeader &header);

QRectF snappedSplitterGrip(const QRectF &grip, bool horizontal,
                           const QPainter *painter);

} // namespace WinUI3::PaintPrivate
