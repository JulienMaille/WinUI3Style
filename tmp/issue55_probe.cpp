#include <winui3style/winui3style.h>
#include <winui3style/winui3icons.h>
#include <QApplication>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QImage>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QDebug>
#include <cstdio>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QApplication::setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light));
    QFont f(QStringLiteral("Segoe UI"), 10);
    app.setFont(f);
    QTreeWidget tree;
    tree.setFont(f);
    tree.setHeaderHidden(true);
    tree.setRootIsDecorated(true);
    tree.setIconSize(QSize(16, 16));
    tree.setIndentation(20);
    QIcon folder = WinUI3::icon(WinUI3::Icon::Folder);
    auto *docs = new QTreeWidgetItem(&tree, {QStringLiteral("Documents")});
    docs->setIcon(0, folder);
    new QTreeWidgetItem(docs, {QStringLiteral("ProjectProposal")});
    new QTreeWidgetItem(docs, {QStringLiteral("BudgetReport")});
    auto *proj = new QTreeWidgetItem(&tree, {QStringLiteral("Projects")});
    proj->setIcon(0, folder);
    new QTreeWidgetItem(proj, {QStringLiteral("Project Plan")});
    docs->setExpanded(true);
    proj->setExpanded(true);
    tree.resize(320, 200);
    tree.show();
    app.processEvents();
    QImage img = tree.grab().toImage();
    img.save(QStringLiteral("tree_before.png"));

    // Measure decoration/text rects for the Documents row (row 0).
    QStyleOptionViewItem opt;
    opt.initFrom(&tree);
    QModelIndex idx = tree.model()->index(0, 0);
    opt.rect = tree.visualRect(idx);
    opt.features = QStyleOptionViewItem::HasDecoration | QStyleOptionViewItem::HasDisplay;
    opt.decorationSize = QSize(16, 16);
    opt.icon = folder;
    opt.text = QStringLiteral("Documents");
    QRect deco = tree.style()->subElementRect(QStyle::SE_ItemViewItemDecoration, &opt, &tree);
    QRect text = tree.style()->subElementRect(QStyle::SE_ItemViewItemText, &opt, &tree);
    fprintf(stderr, "row rect x%d y%d w%d h%d | deco x%d w%d right%d | text x%d | gap %d\n",
            opt.rect.x(), opt.rect.y(), opt.rect.width(), opt.rect.height(),
            deco.x(), deco.width(), deco.right(), text.left(), text.left() - deco.right() - 1);
    return 0;
}