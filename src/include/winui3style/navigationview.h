// SPDX-License-Identifier: LGPL-2.1-or-later
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

    // Ownership: addPage() reparents the page into the internal
    // AnimatedStack (QStackedWidget::addWidget reparents), so the
    // NavigationView owns the page from the call onward. removePage()
    // removes the page from the stack and reparents it to nullptr without
    // deleting it; the caller takes ownership back and must delete or
    // reparent the returned page. Use widget() to retrieve the page before
    // removal.
    int addPage(QWidget *page, const QIcon &icon, const QString &title);
    // Index contract: valid pages are 0 <= index < count(). An empty view
    // has count() == 0 and currentIndex() == -1. Out-of-range input is
    // ignored (no clamp, no assert): setCurrentIndex()/removePage() return
    // without effect and widget() returns nullptr.
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
    // Filtering hides QListWidget rows; the AnimatedStack stays densely
    // indexed. Row positions are never used as page indices: each item
    // carries its stable page index in Qt::UserRole (maintained by
    // addPage()/removePage()/setCurrentIndex()/activateItem()), so hidden
    // or reordered rows still activate the correct page.
    void filter(const QString &text);

private:
    QLineEdit *m_search = nullptr;
    QListWidget *m_list = nullptr;
    AnimatedStack *m_stack = nullptr;
};

} // namespace WinUI3
