#include "winui3paint_p.h"

#include <winui3style/winui3icons.h>

#include <QLineF>
#include <QLinearGradient>
#include <QPainter>

#include <cmath>

namespace WinUI3::PaintPrivate {

void roundedRect(QPainter *painter, const QRectF &rect, const QColor &fill,
                 const QColor &stroke, qreal radius, qreal strokeWidth)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setBrush(fill);
    painter->setPen(stroke.alpha() > 0 ? QPen(stroke, strokeWidth) : Qt::NoPen);
    const qreal half = stroke.alpha() > 0 ? strokeWidth / 2.0 : 0.0;
    painter->drawRoundedRect(rect.adjusted(half, half, -half, -half), radius,
                             radius);
    painter->restore();
}

void paintThemedIcon(QPainter *painter, const QIcon &source, const QRectF &rect,
                     Qt::Alignment alignment, const QColor &foreground,
                     QIcon::Mode mode, QIcon::State state)
{
    if (source.isNull() || rect.isEmpty())
        return;
    const qreal dpr = painter->device() ? painter->device()->devicePixelRatioF()
                                        : 1.0;
    const QSize requestedSize(qMax(1, qRound(rect.width())),
                              qMax(1, qRound(rect.height())));
    const QPixmap pixmap = iconPixmap(source, requestedSize, dpr, foreground,
                                      mode, state);
    if (pixmap.isNull())
        return;
    const QSizeF logicalSize = pixmap.deviceIndependentSize();
    QPointF topLeft = rect.topLeft();
    if (alignment & Qt::AlignRight)
        topLeft.setX(rect.right() - logicalSize.width());
    else if (alignment & Qt::AlignHCenter)
        topLeft.setX(rect.center().x() - logicalSize.width() / 2.0);
    if (alignment & Qt::AlignBottom)
        topLeft.setY(rect.bottom() - logicalSize.height());
    else if (alignment & Qt::AlignVCenter)
        topLeft.setY(rect.center().y() - logicalSize.height() / 2.0);
    painter->drawPixmap(topLeft, pixmap);
}

QRectF visualRectF(Qt::LayoutDirection direction, const QRectF &bounds,
                   const QRectF &logical)
{
    if (direction == Qt::LeftToRight)
        return logical;
    return QRectF(bounds.left() + bounds.right() - logical.right(),
                  logical.top(), logical.width(), logical.height());
}

void controlSurface(QPainter *painter, const QRectF &rect, const QColor &fill,
                    const QColor &strokeTop, const QColor &strokeBottom,
                    qreal radius, qreal strokeWidth)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setBrush(fill);
    QLinearGradient border(rect.topLeft(), rect.bottomLeft());
    border.setColorAt(0.0, strokeTop);
    border.setColorAt(1.0, strokeBottom);
    painter->setPen(QPen(QBrush(border), strokeWidth));
    const qreal half = strokeWidth / 2.0;
    painter->drawRoundedRect(rect.adjusted(half, half, -half, -half), radius,
                             radius);
    painter->restore();
}

void drawEditorFocusUnderline(QPainter *painter, const QRectF &rect,
                              const QColor &accent, qreal radius)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    QPainterPath clip;
    clip.addRoundedRect(rect.adjusted(0.5, 0.5, -0.5, -0.5), radius, radius);
    painter->setClipPath(clip);
    painter->setPen(QPen(accent, 2.0, Qt::SolidLine, Qt::FlatCap));
    painter->drawLine(rect.left(), rect.bottom() - 1,
                      rect.right(), rect.bottom() - 1);
    painter->restore();
}

QRectF snappedEllipseRect(const QRectF &logicalBounds, qreal logicalDiameter,
                          const QPainter *painter)
{
    if (!painter || !painter->device())
        return QRectF(logicalBounds.center().x() - logicalDiameter / 2.0,
                      logicalBounds.center().y() - logicalDiameter / 2.0,
                      logicalDiameter, logicalDiameter);

    const QTransform device = painter->deviceTransform();
    const qreal sx = qAbs(device.m11());
    const qreal sy = qAbs(device.m22());
    if (sx <= 0.0 || sy <= 0.0 || !std::isfinite(sx) || !std::isfinite(sy))
        return QRectF(logicalBounds.center().x() - logicalDiameter / 2.0,
                      logicalBounds.center().y() - logicalDiameter / 2.0,
                      logicalDiameter, logicalDiameter);

    // Snap the center to a whole device pixel but keep the diameter
    // fractional: quantizing the size during an animated grow (e.g. the radio
    // dot's 12 -> 14 px hover expansion) makes it lurch in discrete steps
    // while the surrounding brushes ease continuously.
    const QPointF physicalCenter = device.map(logicalBounds.center());
    const QPointF snappedCenter(qRound(physicalCenter.x()),
                                qRound(physicalCenter.y()));
    const QPointF logicalCenter = device.inverted().map(snappedCenter);
    return QRectF(logicalCenter.x() - logicalDiameter / 2.0,
                  logicalCenter.y() - logicalDiameter / 2.0,
                  logicalDiameter, logicalDiameter);
}

QRectF snappedRect(const QRectF &logicalRect, const QPainter *painter)
{
    if (!painter || !painter->device())
        return logicalRect;

    const QTransform device = painter->deviceTransform();
    const qreal sx = qAbs(device.m11());
    const qreal sy = qAbs(device.m22());
    if (sx <= 0.0 || sy <= 0.0 || !std::isfinite(sx) || !std::isfinite(sy))
        return logicalRect;

    const QPointF physicalTopLeft = device.map(logicalRect.topLeft());
    const QPointF snappedTopLeft(qRound(physicalTopLeft.x()),
                                 qRound(physicalTopLeft.y()));
    const QPointF logicalTopLeft = device.inverted().map(snappedTopLeft);
    return QRectF(logicalTopLeft.x(), logicalTopLeft.y(),
                  logicalRect.width(), logicalRect.height());
}

QPointF animatedAcceptPoint(const QRectF &indicator, qreal x, qreal y)
{
    constexpr qreal canvas = 48.0;
    constexpr qreal sourceScale = 0.7;
    return QPointF(indicator.left()
                       + indicator.width() * (24.0 + sourceScale * x) / canvas,
                   indicator.top()
                       + indicator.height() * (23.0 + sourceScale * y) / canvas);
}

QPainterPath animatedAcceptPath(const QRectF &indicator)
{
    QPainterPath path;
    path.moveTo(animatedAcceptPoint(indicator, -15.172, 0.016));
    path.lineTo(animatedAcceptPoint(indicator, -5.0, 10.188));
    path.lineTo(animatedAcceptPoint(indicator, 15.337, -10.337));
    return path;
}

QPainterPath animatedAcceptTrimmedPath(const QRectF &indicator, qreal progress)
{
    const QPainterPath full = animatedAcceptPath(indicator);
    const QPointF start = full.elementAt(0);
    const QPointF middle = full.elementAt(1);
    const QPointF end = full.elementAt(2);
    const qreal firstLength = QLineF(start, middle).length();
    const qreal secondLength = QLineF(middle, end).length();
    const qreal totalLength = firstLength + secondLength;
    const qreal visible = qBound<qreal>(0.0, progress, 1.0) * totalLength;

    QPainterPath result;
    result.moveTo(start);
    if (visible <= firstLength) {
        result.lineTo(QPointF(start.x() + (middle.x() - start.x())
                                  * visible / firstLength,
                              start.y() + (middle.y() - start.y())
                                  * visible / firstLength));
    } else {
        result.lineTo(middle);
        const qreal second = visible - firstLength;
        result.lineTo(QPointF(middle.x() + (end.x() - middle.x())
                                  * second / secondLength,
                              middle.y() + (end.y() - middle.y())
                                  * second / secondLength));
    }
    return result;
}

QRect headerSortIndicatorRect(const QStyleOptionHeader &header)
{
    const QRect slot = header.rect.adjusted(10, 0, -10, 0);
    return QStyle::alignedRect(header.direction,
                               Qt::AlignRight | Qt::AlignVCenter,
                               QSize(16, 16), slot);
}

QRectF snappedSplitterGrip(const QRectF &grip, bool horizontal,
                           const QPainter *painter)
{
    if (!painter || !painter->device())
        return grip;

    const QTransform device = painter->deviceTransform();
    const qreal signedScale = horizontal ? device.m11() : device.m22();
    const qreal scale = qAbs(signedScale);
    if (scale <= 0.0 || !std::isfinite(scale)
        || !std::isfinite(signedScale))
        return grip;

    const qreal origin = horizontal ? device.dx() : device.dy();
    const qreal center = horizontal ? grip.center().x() : grip.center().y();
    const qreal physicalCenter = origin + center * signedScale;
    const qreal snappedCenter = qRound(physicalCenter - 0.5) + 0.5;
    const qreal physicalOffset = snappedCenter - physicalCenter;
    if (qAbs(physicalOffset) < 0.001)
        return grip;
    const qreal logicalOffset = physicalOffset / signedScale;
    QRectF result = grip;
    if (horizontal)
        result.translate(logicalOffset, 0.0);
    else
        result.translate(0.0, logicalOffset);
    return result;
}

} // namespace WinUI3::PaintPrivate
