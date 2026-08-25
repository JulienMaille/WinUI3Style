#pragma once

#include <winui3style/winui3global.h>

#include <QFrame>
#include <QIcon>
#include <QPointer>

class QLabel;
class QGridLayout;
class QHideEvent;
class QResizeEvent;
class QVBoxLayout;
class QVariantAnimation;
class QWidget;
class QEvent;

namespace WinUI3 {

class WINUI3STYLE_EXPORT SettingsCard final : public QFrame
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged)
    Q_PROPERTY(QIcon icon READ icon WRITE setIcon NOTIFY iconChanged)
    Q_PROPERTY(bool expanded READ isExpanded WRITE setExpanded NOTIFY expandedChanged)
    Q_PROPERTY(qreal expansionProgress READ expansionProgress WRITE setExpansionProgress)

public:
    explicit SettingsCard(QWidget *parent = nullptr);
    ~SettingsCard() override;

    QString title() const;
    void setTitle(const QString &title);
    QString description() const;
    void setDescription(const QString &description);
    QIcon icon() const;
    void setIcon(const QIcon &icon);

    QWidget *trailingWidget() const;
    void setTrailingWidget(QWidget *widget);
    QWidget *expandableWidget() const;
    void setExpandableWidget(QWidget *widget);

    bool isExpanded() const;

public slots:
    void setExpanded(bool expanded);

signals:
    void activated();
    void titleChanged(const QString &title);
    void descriptionChanged(const QString &description);
    void iconChanged(const QIcon &icon);
    void expandedChanged(bool expanded);

protected:
    void changeEvent(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    qreal expansionProgress() const;
    void setExpansionProgress(qreal progress);
    bool headerContains(const QPoint &position) const;
    void refreshIconPixmap();
    void refreshChevronPixmap();
    void invalidateExpandableHeight();
    void scheduleExpandableHeightRefresh();
    void refreshHeaderGeometry();
    int expandableContentHeight();
    void resetExpansionState(bool notify);

    QGridLayout *m_headerLayout = nullptr;
    QVBoxLayout *m_rootLayout = nullptr;
    QWidget *m_headerHost = nullptr;
    QLabel *m_iconLabel = nullptr;
    QLabel *m_titleLabel = nullptr;
    QLabel *m_descriptionLabel = nullptr;
    QLabel *m_chevronLabel = nullptr;
    QWidget *m_trailingWidget = nullptr;
    QPointer<QWidget> m_expandableWidget;
    QWidget *m_expandableHost = nullptr;
    QVariantAnimation *m_expansionAnimation = nullptr;
    QIcon m_icon;
    QMetaObject::Connection m_expandableDestroyedConnection;
    qreal m_expansionProgress = 0.0;
    int m_expandableContentHeight = 0;
    int m_expandableContentWidth = -1;
    int m_headerHeight = 0;
    int m_headerWidth = -1;
    bool m_expandableHeightValid = false;
    bool m_expandableHeightRefreshPending = false;
    bool m_expanded = false;
    bool m_pressed = false;
};

} // namespace WinUI3
