// SPDX-License-Identifier: LGPL-2.1-or-later
#include <winui3style/navigationview.h>
#include <winui3style/animatedstack.h>
#include <winui3style/winui3style.h>

#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QPointer>
#include <QSignalBlocker>
#include <QVBoxLayout>

namespace WinUI3 {

namespace {
bool matchesSearchFilter(const QString &text, const QString &filter)
{
    return text.contains(filter, Qt::CaseInsensitive);
}
} // namespace

NavigationView::NavigationView(QWidget *parent)
    : QWidget(parent),
      m_search(new QLineEdit(this)),
      m_list(new QListWidget(this)),
      m_stack(new AnimatedStack(this))
{
    auto *navigationLayout = new QVBoxLayout;
    navigationLayout->setContentsMargins(0, 0, 0, 0);
    navigationLayout->setSpacing(8);
    m_search->setPlaceholderText(tr("Search settings"));
    m_search->setClearButtonEnabled(true);
    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setSpacing(2);
    m_list->setIconSize(QSize(16, 16));
    Style::setNavigationView(m_list);
    navigationLayout->addWidget(m_search);
    navigationLayout->addWidget(m_list, 1);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(24);
    layout->addLayout(navigationLayout);
    layout->addWidget(m_stack, 1);
    layout->setStretch(0, 0);
    navigationLayout->setSizeConstraint(QLayout::SetMinimumSize);
    m_list->setMinimumWidth(220);
    m_list->setMaximumWidth(280);
    m_search->setMinimumWidth(220);

    // One selection change represents one navigation. Connecting both
    // itemClicked and itemActivated makes a double-click request the same
    // page twice and used to interrupt its transition.
    connect(m_list, &QListWidget::currentItemChanged, this,
            [this](QListWidgetItem *current) { activateItem(current); });
    connect(m_search, &QLineEdit::textChanged, this, &NavigationView::filter);
    connect(m_stack, &QStackedWidget::widgetRemoved, this, [this](int index) {
        const bool settingCurrentItem = m_settingCurrentItem;
        m_settingCurrentItem = true;
        for (int row = 0; row < m_list->count(); ++row) {
            QListWidgetItem *item = m_list->item(row);
            const int stored = item->data(Qt::UserRole).toInt();
            if (stored == index) {
                delete m_list->takeItem(row);
                --row;
            } else if (stored > index) {
                item->setData(Qt::UserRole, stored - 1);
            }
        }
        for (int row = 0; row < m_list->count(); ++row) {
            QListWidgetItem *item = m_list->item(row);
            if (item->data(Qt::UserRole).toInt() == m_stack->currentIndex()) {
                m_list->setCurrentItem(item);
                break;
            }
        }
        m_settingCurrentItem = settingCurrentItem;
    });
    connect(m_stack, &AnimatedStack::currentChanged, this, [this](int index) {
        QListWidgetItem *selected = nullptr;
        for (int row = 0; row < m_list->count(); ++row) {
            QListWidgetItem *item = m_list->item(row);
            if (item->data(Qt::UserRole).toInt() == index) {
                selected = item;
                break;
            }
        }
        const bool wasSettingCurrentItem = m_settingCurrentItem;
        m_settingCurrentItem = true;
        m_list->setCurrentItem(selected);
        m_settingCurrentItem = wasSettingCurrentItem;
        emit currentIndexChanged(index);
    });
}

NavigationView::~NavigationView()
{
    disconnect(m_stack, nullptr, this, nullptr);
}

int NavigationView::addPage(QWidget *page, const QIcon &icon, const QString &title)
{
    const int existingIndex = m_stack->indexOf(page);
    if (existingIndex >= 0)
        return existingIndex;
    const int index = m_stack->addWidget(page);
    auto *item = new QListWidgetItem(icon, title, m_list);
    item->setData(Qt::UserRole, index);
    // filter() only runs when the search text changes. A page inserted while
    // a filter is already active must obey it from the moment it appears.
    item->setHidden(!matchesSearchFilter(title, m_search->text()));
    if (m_list->count() == 1) {
        m_list->setCurrentItem(item);
        m_stack->QStackedWidget::setCurrentIndex(index);
    }
    return index;
}

void NavigationView::removePage(int index)
{
    const QPointer<AnimatedStack> stack = m_stack;
    const QPointer<QWidget> page = stack->widget(index);
    if (!page)
        return;
    const bool removedCurrent = stack->currentWidget() == page;
    // Qt still uses the removed widget after emitting widgetRemoved, and
    // removeWidget's layout scan can remove a reinserted page repeatedly.
    // Deliver the stack notifications only after Qt releases the layout item.
    {
        const QSignalBlocker blocker(stack);
        stack->removeWidget(page);
    }
    if (removedCurrent)
        emit stack->currentChanged(stack->currentIndex());
    if (stack)
        emit stack->widgetRemoved(index);
    if (stack && page && page->parentWidget() == stack && stack->indexOf(page) < 0)
        page->setParent(nullptr);
}

int NavigationView::count() const
{
    return m_stack->count();
}
int NavigationView::currentIndex() const
{
    return m_stack->currentIndex();
}
QWidget *NavigationView::widget(int index) const
{
    return m_stack->widget(index);
}
bool NavigationView::isSearchVisible() const
{
    return m_search->isVisible();
}
void NavigationView::setSearchVisible(bool visible)
{
    m_search->setVisible(visible);
}
QListWidget *NavigationView::navigationList() const
{
    return m_list;
}
AnimatedStack *NavigationView::stack() const
{
    return m_stack;
}

void NavigationView::setCurrentIndex(int index)
{
    if (index < 0 || index >= count())
        return;
    const auto request = ++m_navigationRequest;
    for (int row = 0; row < m_list->count(); ++row) {
        QListWidgetItem *item = m_list->item(row);
        if (item->data(Qt::UserRole).toInt() == index) {
            const bool wasSettingCurrentItem = m_settingCurrentItem;
            m_settingCurrentItem = true;
            m_list->setCurrentItem(item);
            m_settingCurrentItem = wasSettingCurrentItem;
            break;
        }
    }
    if (request == m_navigationRequest)
        m_stack->setCurrentIndex(index);
}

void NavigationView::activateItem(QListWidgetItem *item)
{
    if (m_settingCurrentItem || !item)
        return;
    setCurrentIndex(item->data(Qt::UserRole).toInt());
}

void NavigationView::filter(const QString &text)
{
    for (int row = 0; row < m_list->count(); ++row)
        m_list->item(row)->setHidden(!matchesSearchFilter(m_list->item(row)->text(), text));
}

} // namespace WinUI3
