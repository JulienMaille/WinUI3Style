#pragma once

class QAbstractItemView;

namespace WinUI3::NavigationPrivate {

void prepareNavigationView(QAbstractItemView *view);
void restoreNavigationView(QAbstractItemView *view);

} // namespace WinUI3::NavigationPrivate
