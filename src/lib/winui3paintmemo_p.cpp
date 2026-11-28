// SPDX-License-Identifier: LGPL-2.1-or-later
#include "winui3paintmemo_p.h"

#include "winui3backdrop_p.h"
#include "winui3style_properties_p.h"

#include <QAbstractButton>
#include <QCoreApplication>
#include <QHash>
#include <QPointer>
#include <QThread>
#include <QWidget>

namespace WinUI3::Private {

namespace {

struct WidgetPaintMemo
{
    QPointer<const Style> owner;
    QMetaObject::Connection destroyed;
    bool cacheRole = false;
    bool cacheBackdrop = false;
    std::optional<MemoizedControlRole> role;
    std::optional<int> backdrop;
};

QHash<const QWidget *, WidgetPaintMemo> &paintMemos()
{
    static QHash<const QWidget *, WidgetPaintMemo> memos;
    return memos;
}

// Widget polish and paint run on the GUI thread. Other callers resolve
// properties live without accessing the cache.
bool onGuiThread()
{
    const QCoreApplication *application = QCoreApplication::instance();
    return application && QThread::currentThread() == application->thread();
}

WidgetPaintMemo *paintMemo(const QWidget *widget)
{
    if (!widget || !onGuiThread())
        return nullptr;
    auto &memos = paintMemos();
    auto memo = memos.find(widget);
    return memo == memos.end() || memo->owner.isNull() ? nullptr : &memo.value();
}

} // namespace

void observeWidgetPaintMemo(QWidget *widget, const Style *style)
{
    const bool window = widget->isWindow();
    const bool button = qobject_cast<const QAbstractButton *>(widget) != nullptr;
    if ((!window && !button) || !onGuiThread())
        return;
    // Q_PROPERTY writes do not send DynamicPropertyChange. Only memoize
    // names for which the installed style filter will see writes.
    const QMetaObject *meta = widget->metaObject();
    const bool cacheRole = button && meta->indexOfProperty(Style::ControlRoleProperty) < 0
            && meta->indexOfProperty(roleProperty) < 0;
    const bool cacheBackdrop = window && meta->indexOfProperty(backdropStateProperty) < 0;
    if (!cacheRole && !cacheBackdrop)
        return;

    auto &memo = paintMemos()[widget];
    if (!memo.destroyed) {
        memo.destroyed = QObject::connect(widget, &QObject::destroyed, QCoreApplication::instance(),
                                          [widget] { paintMemos().remove(widget); });
    }
    memo.owner = style;
    memo.cacheRole = cacheRole;
    memo.cacheBackdrop = cacheBackdrop;
    memo.role.reset();
    memo.backdrop.reset();
}

void forgetWidgetPaintMemo(const QWidget *widget, const Style *style)
{
    if (!onGuiThread())
        return;
    auto &memos = paintMemos();
    auto memo = memos.find(widget);
    if (memo == memos.end() || memo->owner != style)
        return;
    QObject::disconnect(memo->destroyed);
    memos.erase(memo);
}

std::optional<MemoizedControlRole> memoizedControlRole(const QWidget *widget)
{
    const auto *memo = paintMemo(widget);
    return memo ? memo->role : std::nullopt;
}

void memoizeControlRole(const QWidget *widget, bool propertyValid, ControlRole role)
{
    if (auto *memo = paintMemo(widget); memo && memo->cacheRole)
        memo->role = MemoizedControlRole{ propertyValid, role };
}

std::optional<int> memoizedWindowBackdrop(const QWidget *window)
{
    const auto *memo = paintMemo(window);
    return memo ? memo->backdrop : std::nullopt;
}

void memoizeWindowBackdrop(const QWidget *window, int backdrop)
{
    if (auto *memo = paintMemo(window); memo && memo->cacheBackdrop)
        memo->backdrop = backdrop;
}

void clearWidgetPaintMemo(const QWidget *widget)
{
    if (auto *memo = paintMemo(widget)) {
        memo->role.reset();
        memo->backdrop.reset();
    }
}

} // namespace WinUI3::Private
