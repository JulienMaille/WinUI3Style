// SPDX-License-Identifier: LGPL-2.1-or-later
// Slider value tooltip surface (S3 surfaces split): the non-activating tool
// window, its debounce timer and the inspectable state that stays synchronous
// under the headless platform. Moved verbatim from winui3surfaces_p.cpp.
#include "winui3slidertooltip_p.h"

#include "winui3frameproperties_p.h"
#include "winui3paint_p.h"
#include "winui3style_properties_p.h"
#include "winui3tokens_p.h"

#include <winui3style/winui3style.h>

#include <QGuiApplication>
#include <QPainter>
#include <QPointer>
#include <QSlider>
#include <QStyleOptionSlider>
#include <QTimer>
#include <QWidget>

namespace WinUI3::Private {

using namespace PaintPrivate;

namespace {

class SliderValueTip final : public QWidget
{
public:
    explicit SliderValueTip(QSlider *slider)
        : QWidget(slider,
                  Qt::Tool | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus
                          | Qt::WindowTransparentForInput)
    {
        setObjectName(QStringLiteral("_winui_slider_value_tip"));
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_ShowWithoutActivating);
    }

    void showValue(const QString &text, const QPoint &anchor, bool horizontal)
    {
        m_text = text;
        const QFontMetrics metrics(font());
        resize(qMax(32, metrics.horizontalAdvance(text) + 16), 32);
        QPoint position = horizontal ? QPoint(anchor.x() - width() / 2, anchor.y() - height())
                                     : QPoint(anchor.x(), anchor.y() - height() / 2);
        if (QScreen *screen = QGuiApplication::screenAt(anchor)) {
            const QRect available = screen->availableGeometry();
            position.setX(qBound(available.left(), position.x(), available.right() - width() + 1));
            position.setY(qBound(available.top(), position.y(), available.bottom() - height() + 1));
        }
        move(position);
        show();
        raise();
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        const Private::Tokens t = Private::tokens(palette());
        QColor fill = palette().color(QPalette::ToolTipBase);
        fill.setAlpha(242);
        QPainter painter(this);
        roundedRect(&painter, QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), fill,
                    t.strokeSecondary, Private::ControlRadius);
        painter.setPen(palette().color(QPalette::ToolTipText));
        painter.drawText(rect().adjusted(8, 0, -8, 0), Qt::AlignCenter, m_text);
    }

private:
    QString m_text;
};

constexpr auto sliderToolTipDebounceTimerName = "_winui_slider_tooltip_debounce_timer";
constexpr int sliderToolTipDebounceInterval = 16;

SliderValueTip *sliderValueTip(QSlider *slider)
{
    if (!slider)
        return nullptr;
    return static_cast<SliderValueTip *>(slider->findChild<QWidget *>(
            QStringLiteral("_winui_slider_value_tip"), Qt::FindDirectChildrenOnly));
}

void clearSliderValueToolTip(QSlider *slider)
{
    if (!slider)
        return;

    if (auto *timer = slider->findChild<QTimer *>(
                QString::fromLatin1(sliderToolTipDebounceTimerName), Qt::FindDirectChildrenOnly)) {
        timer->stop();
    }
    framePropertyRegistry().set(slider, sliderToolTipVisibleProperty, false);
    framePropertyRegistry().clear(slider, sliderToolTipValueProperty);
    framePropertyRegistry().clear(slider, sliderToolTipSurfacePendingProperty);
    if (auto *tip = sliderValueTip(slider))
        tip->hide();
}

void updateSliderValueToolTipNow(QSlider *slider)
{
    if (!slider)
        return;
    if (!slider->isEnabled()) {
        // A slider can be disabled while the trailing debounce timer is
        // pending. Clear both the inspectable state and the native popup;
        // otherwise the last value remains visible until the next drag.
        clearSliderValueToolTip(slider);
        return;
    }

    QStyleOptionSlider option;
    option.initFrom(slider);
    option.orientation = slider->orientation();
    option.minimum = slider->minimum();
    option.maximum = slider->maximum();
    option.sliderPosition = slider->sliderPosition();
    option.sliderValue = slider->value();
    option.singleStep = slider->singleStep();
    option.pageStep = slider->pageStep();
    option.upsideDown = slider->orientation() == Qt::Horizontal
            ? (slider->invertedAppearance() != (slider->layoutDirection() == Qt::RightToLeft))
            : !slider->invertedAppearance();
    const QRect handle = slider->style()->subControlRect(QStyle::CC_Slider, &option,
                                                         QStyle::SC_SliderHandle, slider);
    const QPoint anchor = slider->orientation() == Qt::Horizontal
            ? QPoint(handle.center().x(), handle.top() - 8)
            : QPoint(handle.right() + 8, handle.center().y());
    const QString valueText = QString::number(slider->value());
    framePropertyRegistry().set(slider, sliderToolTipVisibleProperty, true);
    framePropertyRegistry().set(slider, sliderToolTipValueProperty, valueText);
    // The headless platform cannot host a non-activating tool window and
    // synthesizes a FocusOut when one is shown. State/value remain testable;
    // the popup itself is covered by the native Windows test path.
    if (QGuiApplication::platformName() == QStringLiteral("offscreen"))
        return;
    auto *tip = sliderValueTip(slider);
    if (!tip)
        tip = new SliderValueTip(slider);
    tip->showValue(valueText, slider->mapToGlobal(anchor), slider->orientation() == Qt::Horizontal);
}

QTimer *sliderToolTipDebounceTimer(QSlider *slider)
{
    if (!slider)
        return nullptr;
    if (auto *timer = slider->findChild<QTimer *>(
                QString::fromLatin1(sliderToolTipDebounceTimerName), Qt::FindDirectChildrenOnly)) {
        return timer;
    }
    auto *timer = new QTimer(slider);
    timer->setObjectName(QString::fromLatin1(sliderToolTipDebounceTimerName));
    timer->setSingleShot(true);
    timer->setInterval(sliderToolTipDebounceInterval);
    const QPointer<QSlider> guarded(slider);
    QObject::connect(timer, &QTimer::timeout, slider, [guarded] {
        if (!guarded)
            return;
        // Close the current frame's leading-edge window so the next pointer
        // move can schedule a fresh refresh.
        framePropertyRegistry().set(guarded, sliderToolTipSurfacePendingProperty, false);
        if (guarded->isEnabled()
            && framePropertyRegistry().value(guarded, sliderToolTipVisibleProperty).toBool())
            updateSliderValueToolTipNow(guarded);
    });
    return timer;
}

} // namespace

void showSliderValueToolTip(QSlider *slider)
{
    if (!slider || !slider->isEnabled())
        return;
    // Synchronous inspectable state is always current, regardless of how the
    // native popup is coalesced.
    framePropertyRegistry().set(slider, sliderToolTipVisibleProperty, true);
    framePropertyRegistry().set(slider, sliderToolTipValueProperty,
                                QString::number(slider->value()));
    // The headless platform cannot host a non-activating tool window; keep the
    // state synchronous and avoid allocating a timer that cannot fire.
    if (QGuiApplication::platformName() == QStringLiteral("offscreen"))
        return;
    // A refresh is already scheduled in this frame; its timeout picks up the
    // newest value, so subsequent pointer moves within the frame are coalesced.
    if (framePropertyRegistry().value(slider, sliderToolTipSurfacePendingProperty).toBool())
        return;
    // Leading edge of a new frame (or the first show): refresh the popup
    // immediately so it tracks the handle with no trailing-edge lag, then arm
    // a one-shot timer to re-apply the latest value at the frame boundary. The
    // trailing design in 7e2e480 only repainted after the mouse paused, which
    // made the tooltip visibly trail the handle during a fast drag.
    framePropertyRegistry().set(slider, sliderToolTipSurfacePendingProperty, true);
    if (auto *timer = sliderToolTipDebounceTimer(slider))
        timer->start();
    updateSliderValueToolTipNow(slider);
}

void hideSliderValueToolTip(QSlider *slider)
{
    clearSliderValueToolTip(slider);
}

} // namespace WinUI3::Private
