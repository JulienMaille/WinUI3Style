#include "winui3tableeditors_p.h"

#include "winui3style_properties_p.h"

#include <QAbstractItemModel>
#include <QTableView>
#include <QTimer>
#include <QWidget>

namespace WinUI3::Private {

TableEditorTracker::TableEditorTracker(QObject *connectionContext)
    : m_context(connectionContext)
{
}

QPersistentModelIndex TableEditorTracker::editorIndex(
    const QTableView *table, const QWidget *editor) const
{
    if (!table || !table->viewport() || !table->model() || !editor)
        return {};
    const QRect geometry(editor->mapTo(table->viewport(), QPoint()),
                         editor->size());
    QModelIndex index = table->indexAt(geometry.center());
    if (!index.isValid()) {
        const QPoint candidates[] = {geometry.topLeft(), geometry.topRight(),
                                     geometry.bottomLeft(), geometry.bottomRight()};
        for (const QPoint &point : candidates) {
            index = table->indexAt(point);
            if (index.isValid())
                break;
        }
    }
    return QPersistentModelIndex(index);
}

void TableEditorTracker::connectModel(QTableView *table, TableState &state,
                                      QAbstractItemModel *model)
{
    state.model = model;
    if (!model)
        return;
    const QPointer<QTableView> guardedTable(table);
    state.aboutToResetConnection = QObject::connect(
        model, &QAbstractItemModel::modelAboutToBeReset, m_context,
        [this, guardedTable] {
            if (guardedTable)
                clearEditors(guardedTable, false);
        });
    state.resetConnection = QObject::connect(
        model, &QAbstractItemModel::modelReset, m_context,
        [this, guardedTable] {
            if (!guardedTable)
                return;
            reindexMarkedEditors(guardedTable);
            if (guardedTable->viewport())
                guardedTable->viewport()->update();
        });
    state.modelDestroyedConnection = QObject::connect(
        model, &QObject::destroyed, m_context, [this, guardedTable] {
            if (guardedTable)
                clearEditors(guardedTable, true);
        });
}

void TableEditorTracker::ensureTable(QTableView *table)
{
    if (!table)
        return;
    auto it = m_tables.find(table);
    if (it == m_tables.end()) {
        TableState state;
        const QPointer<QTableView> guardedTable(table);
        state.tableDestroyedConnection = QObject::connect(
            table, &QObject::destroyed, m_context,
            [this, table] { untrackTable(table, false); });
        it = m_tables.insert(table, state);
        connectModel(table, it.value(), table->model());
        return;
    }
    if (it->model.data() == table->model())
        return;

    clearEditors(table, true);
    it = m_tables.find(table);
    if (it == m_tables.end())
        return;
    QObject::disconnect(it->aboutToResetConnection);
    QObject::disconnect(it->resetConnection);
    QObject::disconnect(it->modelDestroyedConnection);
    connectModel(table, it.value(), table->model());
}

void TableEditorTracker::track(QTableView *table, QWidget *editor)
{
    if (!table || !editor || !table->viewport()
        || editor->parentWidget() != table->viewport())
        return;
    editor->setProperty(tableEditorProperty, true);
    ensureTable(table);
    const QPersistentModelIndex index = editorIndex(table, editor);
    if (!index.isValid()) {
        const QPointer<QTableView> guardedTable(table);
        const QPointer<QWidget> guardedEditor(editor);
        QTimer::singleShot(0, m_context, [this, guardedTable, guardedEditor] {
            if (guardedTable && guardedEditor)
                track(guardedTable, guardedEditor);
        });
        return;
    }

    if (const auto owner = m_owners.constFind(editor);
        owner != m_owners.constEnd() && owner->table == table
        && owner->index == index) {
        return;
    }
    untrackEditor(editor, false);
    auto tableIt = m_tables.find(table);
    if (tableIt == m_tables.end())
        return;
    if (QWidget *previous = tableIt->editors.value(index).data();
        previous && previous != editor)
        untrackEditor(previous, true);
    tableIt = m_tables.find(table);
    if (tableIt == m_tables.end())
        return;
    tableIt->editors.insert(index, editor);
    Owner owner{table, index, {}};
    owner.destroyedConnection = QObject::connect(
        editor, &QObject::destroyed, m_context,
        [this, editor] { untrackEditor(editor, false); });
    m_owners.insert(editor, owner);
}

void TableEditorTracker::untrackEditor(QWidget *editor, bool clearProperty)
{
    if (!editor)
        return;
    const auto ownerIt = m_owners.find(editor);
    if (ownerIt != m_owners.end()) {
        const Owner owner = ownerIt.value();
        m_owners.erase(ownerIt);
        QObject::disconnect(owner.destroyedConnection);
        if (owner.table) {
            auto tableIt = m_tables.find(owner.table);
            if (tableIt != m_tables.end()
                && tableIt->editors.value(owner.index).data() == editor)
                tableIt->editors.remove(owner.index);
        }
    }
    if (clearProperty)
        editor->setProperty(tableEditorProperty, {});
}

void TableEditorTracker::clearEditors(QTableView *table, bool clearProperties)
{
    auto tableIt = m_tables.find(table);
    if (tableIt == m_tables.end())
        return;
    const auto editors = tableIt->editors;
    tableIt->editors.clear();
    for (const QPointer<QWidget> &guarded : editors) {
        QWidget *editor = guarded.data();
        if (!editor)
            continue;
        const auto ownerIt = m_owners.find(editor);
        if (ownerIt != m_owners.end()) {
            QObject::disconnect(ownerIt->destroyedConnection);
            m_owners.erase(ownerIt);
        }
        if (clearProperties)
            editor->setProperty(tableEditorProperty, {});
    }
}

void TableEditorTracker::reindexMarkedEditors(QTableView *table)
{
    if (!table || !table->viewport())
        return;
    const auto children = table->viewport()->findChildren<QWidget *>(
        QString{}, Qt::FindDirectChildrenOnly);
    for (QWidget *child : children) {
        if (child->property(tableEditorProperty).toBool())
            track(table, child);
    }
}

void TableEditorTracker::untrackTable(QTableView *table, bool clearProperties)
{
    if (!table)
        return;
    clearEditors(table, clearProperties);
    const auto it = m_tables.find(table);
    if (it == m_tables.end())
        return;
    QObject::disconnect(it->aboutToResetConnection);
    QObject::disconnect(it->resetConnection);
    QObject::disconnect(it->modelDestroyedConnection);
    QObject::disconnect(it->tableDestroyedConnection);
    m_tables.erase(it);
}

bool TableEditorTracker::overlaps(const QTableView *table,
                                  const QModelIndex &index,
                                  const QRect &itemRect)
{
    if (!table || !table->viewport() || !index.isValid())
        return false;
    auto *mutableTable = const_cast<QTableView *>(table);
    ensureTable(mutableTable);
    const auto tableIt = m_tables.constFind(mutableTable);
    if (tableIt == m_tables.constEnd())
        return false;
    QWidget *editor = tableIt->editors.value(QPersistentModelIndex(index)).data();
    if (!editor || !editor->isVisible()
        || !editor->property(tableEditorProperty).toBool())
        return false;
    const QRect editorRect(editor->mapTo(table->viewport(), QPoint()),
                           editor->size());
    return editorRect.intersects(itemRect);
}

} // namespace WinUI3::Private
