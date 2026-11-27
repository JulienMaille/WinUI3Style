// SPDX-License-Identifier: LGPL-2.1-or-later
#include "winui3paintmemo_p.h"

#include <QCoreApplication>
#include <QHash>
#include <QThread>
#include <QWidget>

namespace WinUI3::Private {

namespace {

struct ControlRoleEntry
{
    QPointer<const QWidget> guard;
    MemoizedControlRole value;
};

struct BackdropEntry
{
    QPointer<const QWidget> guard;
    int value = 0;
};

// Event filters do not reliably observe QEvent::Destroy, so entries of
// destroyed widgets are dropped lazily: a null guard on lookup, or a sweep
// once the table grows unusually large.
constexpr int memoSweepThreshold = 8192;

QHash<const QWidget *, ControlRoleEntry> &controlRoleMemo()
{
    static QHash<const QWidget *, ControlRoleEntry> memo;
    return memo;
}

QHash<const QWidget *, BackdropEntry> &windowBackdropMemo()
{
    static QHash<const QWidget *, BackdropEntry> memo;
    return memo;
}

// The memo is only touched from the paint and polish paths, which run on the
// GUI thread. Off-thread callers fall back to live property resolution.
bool onGuiThread()
{
    const QCoreApplication *application = QCoreApplication::instance();
    return application && QThread::currentThread() == application->thread();
}

template <typename Memo>
void pruneDestroyedEntries(Memo &memo)
{
    if (memo.size() < memoSweepThreshold)
        return;
    for (auto it = memo.begin(); it != memo.end();) {
        if (it.value().guard.isNull())
            it = memo.erase(it);
        else
            ++it;
    }
}

} // namespace

std::optional<MemoizedControlRole> memoizedControlRole(const QWidget *widget)
{
    if (!widget || !onGuiThread())
        return std::nullopt;
    QHash<const QWidget *, ControlRoleEntry> &memo = controlRoleMemo();
    const auto it = memo.find(widget);
    if (it == memo.end())
        return std::nullopt;
    if (it.value().guard.isNull()) {
        memo.erase(it);
        return std::nullopt;
    }
    return it.value().value;
}

void memoizeControlRole(const QWidget *widget, bool propertyValid, ControlRole role)
{
    if (!widget || !onGuiThread())
        return;
    QHash<const QWidget *, ControlRoleEntry> &memo = controlRoleMemo();
    pruneDestroyedEntries(memo);
    memo.insert(widget, ControlRoleEntry{ widget, MemoizedControlRole{ propertyValid, role } });
}

std::optional<int> memoizedWindowBackdrop(const QWidget *window)
{
    if (!window || !onGuiThread())
        return std::nullopt;
    QHash<const QWidget *, BackdropEntry> &memo = windowBackdropMemo();
    const auto it = memo.find(window);
    if (it == memo.end())
        return std::nullopt;
    if (it.value().guard.isNull()) {
        memo.erase(it);
        return std::nullopt;
    }
    return it.value().value;
}

void memoizeWindowBackdrop(const QWidget *window, int backdrop)
{
    if (!window || !onGuiThread())
        return;
    QHash<const QWidget *, BackdropEntry> &memo = windowBackdropMemo();
    pruneDestroyedEntries(memo);
    memo.insert(window, BackdropEntry{ window, backdrop });
}

void clearWidgetPaintMemo(const QWidget *widget)
{
    if (!widget || !onGuiThread())
        return;
    controlRoleMemo().remove(widget);
    windowBackdropMemo().remove(widget);
}

} // namespace WinUI3::Private
