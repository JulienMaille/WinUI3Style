#pragma once

#include <winui3style/winui3global.h>

#include <QStackedWidget>
#include <QPointer>
#include <QPixmap>

class QParallelAnimationGroup;
class QHideEvent;
class QResizeEvent;
class QVariantAnimation;

namespace WinUI3 {

class WINUI3STYLE_EXPORT AnimatedStack final : public QStackedWidget
{
    Q_OBJECT
    Q_PROPERTY(int duration READ duration WRITE setDuration)

public:
    enum class Transition {
        Automatic,
        Forward,
        Backward,
        Entrance
    };
    Q_ENUM(Transition)

    explicit AnimatedStack(QWidget *parent = nullptr);
    ~AnimatedStack() override;

    int duration() const;
    void setDuration(int duration);
    bool isAnimating() const;

public slots:
    void setCurrentIndex(int index);
    void setCurrentIndex(int index, Transition transition);

signals:
    void transitionFinished(int index);

protected:
    void hideEvent(QHideEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void handleWidgetRemoved(int index);
    void cancelTransition();
    void finishTransition();
    void destroyTransitionObjects();
    void updateOverlayGeometry(qreal progress);

    int m_duration = 250;
    int m_from = -1;
    int m_to = -1;
    QPointer<QParallelAnimationGroup> m_group;
    QPointer<QWidget> m_outgoing;
    QPointer<QWidget> m_incoming;
    QPointer<QWidget> m_overlay;
    QPointer<QVariantAnimation> m_geometryAnimation;
    QRect m_overlayStartGeometry;
    QRect m_overlayEndGeometry;
    qreal m_transitionProgress = 0.0;
    bool m_transitionStarting = false;
    int m_deferredIndex = -1;
    Transition m_deferredTransition = Transition::Automatic;
};

} // namespace WinUI3
