// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

class QAbstractItemView;
class QPalette;

namespace WinUI3::NavigationPrivate {

// GUI-thread only: both helpers mutate QWidgets (item delegate, palettes,
// frame shape, mouse tracking, viewport properties) and touch GUI-thread
// IconRuntime-adjacent state via framePropertyRegistry. Callers must invoke
// them on the GUI thread with a live view; a null view is a no-op, and the
// view must outlive its paired restore (neither helper takes ownership).
// prepare/restore pairing is idempotent: prepare-prepare and
// restore-without-prepare are safe no-ops owned by the caller.
void prepareNavigationView(QAbstractItemView *view);
void restoreNavigationView(QAbstractItemView *view);
void refreshNavigationPalette(QAbstractItemView *view, const QPalette &palette);

} // namespace WinUI3::NavigationPrivate
