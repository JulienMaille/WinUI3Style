#include <winui3style/navigationview.h>
#include <winui3style/animatedstack.h>
#include <winui3style/winui3style.h>

#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QVBoxLayout>

namespace WinUI3 {

NavigationView::NavigationView(QWidget *parent)
    : QWidget(parent)
    , m_search(new QLineEdit(this))
    , m_list(new QListWidget(this))
    , m_stack(new AnimatedStack(this))
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
    connect(m_stack, &AnimatedStack::currentChanged,
            this, &NavigationView::currentIndexChanged);
}

NavigationView::~NavigationView() = default;

int NavigationView::addPage(QWidget *page, const QIcon &icon, const QString &title)
{
    const int index = m_stack->addWidget(page);
    auto *item = new QListWidgetItem(icon, title, m_list);
    item->setData(Qt::UserRole, index);
    if (m_list->count() == 1) {
        m_list->setCurrentItem(item);
        m_stack->QStackedWidget::setCurrentIndex(index);
    }
    return index;
}

void NavigationView::removePage(int index)
{
    QWidget *page = m_stack->widget(index);
    if (!page) return;
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
    m_stack->removeWidget(page);
    page->setParent(nullptr);
}

int NavigationView::count() const { return m_stack->count(); }
int NavigationView::currentIndex() const { return m_stack->currentIndex(); }
QWidget *NavigationView::widget(int index) const { return m_stack->widget(index); }
bool NavigationView::isSearchVisible() const { return m_search->isVisible(); }
void NavigationView::setSearchVisible(bool visible) { m_search->setVisible(visible); }
QListWidget *NavigationView::navigationList() const { return m_list; }
AnimatedStack *NavigationView::stack() const { return m_stack; }

void NavigationView::setCurrentIndex(int index)
{
    if (index < 0 || index >= count()) return;
    for (int row = 0; row < m_list->count(); ++row) {
        QListWidgetItem *item = m_list->item(row);
        if (item->data(Qt::UserRole).toInt() == index) {
            m_list->setCurrentItem(item);
            break;
        }
    }
    m_stack->setCurrentIndex(index);
}

void NavigationView::activateItem(QListWidgetItem *item)
{
    if (item) setCurrentIndex(item->data(Qt::UserRole).toInt());
}

void NavigationView::filter(const QString &text)
{
    for (int row = 0; row < m_list->count(); ++row)
        m_list->item(row)->setHidden(!m_list->item(row)->text().contains(text, Qt::CaseInsensitive));
}

} // namespace WinUI3
