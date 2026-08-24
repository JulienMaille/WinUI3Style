#pragma once

#include <winui3style/winui3global.h>

#include <QStackedWidget>
#include <QPointer>
#include <QPixmap>

class QParallelAnimationGroup;

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

private:
    void cancelTransition();
    void finishTransition();

    int m_duration = 250;
    int m_from = -1;
    int m_to = -1;
    QParallelAnimationGroup *m_group = nullptr;
    QWidget *m_overlay = nullptr;
    QPointer<QWidget> m_incoming;
};

} // namespace WinUI3
