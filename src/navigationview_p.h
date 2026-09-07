// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

class QAbstractItemView;

namespace WinUI3::NavigationPrivate {

void prepareNavigationView(QAbstractItemView *view);
void restoreNavigationView(QAbstractItemView *view);

} // namespace WinUI3::NavigationPrivate
