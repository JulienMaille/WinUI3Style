#pragma once

#include <QHash>
#include <QMetaObject>
#include <QPersistentModelIndex>
#include <QPointer>
#include <QRect>

class QAbstractItemModel;
class QObject;
class QTableView;
class QWidget;

namespace WinUI3::Private {

// Tracks live QTableView editors by model index so painting a normal cell is
// O(1) in the number of open editors. Geometry is deliberately not cached:
// scrolling and layout changes remain correct without extra signal traffic.
class TableEditorTracker final
{
public:
    explicit TableEditorTracker(QObject *connectionContext);

    void track(QTableView *table, QWidget *editor);
    void untrackEditor(QWidget *editor, bool clearProperty = true);
    void untrackTable(QTableView *table, bool clearProperties = true);
    bool overlaps(const QTableView *table, const QModelIndex &index,
                  const QRect &itemRect);

private:
    struct Owner {
        QPointer<QTableView> table;
        QPersistentModelIndex index;
        QMetaObject::Connection destroyedConnection;
    };

    struct TableState {
        QPointer<QAbstractItemModel> model;
        QHash<QPersistentModelIndex, QPointer<QWidget>> editors;
        QMetaObject::Connection aboutToResetConnection;
        QMetaObject::Connection resetConnection;
        QMetaObject::Connection modelDestroyedConnection;
        QMetaObject::Connection tableDestroyedConnection;
    };

    void ensureTable(QTableView *table);
    void clearEditors(QTableView *table, bool clearProperties);
    void connectModel(QTableView *table, TableState &state,
                      QAbstractItemModel *model);
    void reindexMarkedEditors(QTableView *table);
    void trackOnce(QTableView *table, QWidget *editor, bool allowRetry);
    QPersistentModelIndex editorIndex(const QTableView *table,
                                      const QWidget *editor) const;

    QObject *m_context = nullptr;
    QHash<QTableView *, TableState> m_tables;
    QHash<QWidget *, Owner> m_owners;
};

} // namespace WinUI3::Private
