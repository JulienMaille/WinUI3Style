#pragma once

#include <winui3style/winui3global.h>

#include <QIcon>
#include <QWidget>

class QLineEdit;
class QListWidget;
class QListWidgetItem;

namespace WinUI3 {

class AnimatedStack;

class WINUI3STYLE_EXPORT NavigationView final : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(bool searchVisible READ isSearchVisible WRITE setSearchVisible)

public:
    explicit NavigationView(QWidget *parent = nullptr);
    ~NavigationView() override;

    int addPage(QWidget *page, const QIcon &icon, const QString &title);
    void removePage(int index);
    int count() const;
    int currentIndex() const;
    QWidget *widget(int index) const;

    bool isSearchVisible() const;
    void setSearchVisible(bool visible);
    QListWidget *navigationList() const;
    AnimatedStack *stack() const;

public slots:
    void setCurrentIndex(int index);

signals:
    void currentIndexChanged(int index);

private slots:
    void activateItem(QListWidgetItem *item);
    void filter(const QString &text);

private:
    QLineEdit *m_search = nullptr;
    QListWidget *m_list = nullptr;
    AnimatedStack *m_stack = nullptr;
};

} // namespace WinUI3

