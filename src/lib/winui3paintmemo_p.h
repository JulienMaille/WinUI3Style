// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

// Paint-time memoization of rarely-changing dynamic properties. Resolving a
// property by name costs a QMetaObject::indexOfProperty lookup on every
// QWidget::property() call, and dense scenes ask the same widgets for the
// same values thousands of times per frame. Entries are keyed by widget
// address and guarded by QPointer, so destroyed widgets are detected on the
// next lookup; the style's event filter (QDynamicPropertyChange) and
// Style::unpolish() invalidate entries when the underlying property changes.

#include <winui3style/winui3global.h>
#include <winui3style/winui3style.h>

#include <QPointer>
#include <optional>

class QWidget;

namespace WinUI3::Private {

struct MemoizedControlRole
{
    // True when the role came from an explicit property. When false the
    // caller must re-run the flat/default fallback live: setFlat() and
    // setDefault() emit no event the style could observe.
    bool propertyValid = false;
    ControlRole role = ControlRole::Standard;
};

WINUI3STYLE_EXPORT std::optional<MemoizedControlRole> memoizedControlRole(const QWidget *widget);
WINUI3STYLE_EXPORT void memoizeControlRole(const QWidget *widget, bool propertyValid,
                                           ControlRole role);

// Cached raw value of "_winui_backdrop" on a window, using QWidget::property
// toInt() semantics (an absent property reads as 0). nullopt means "resolve
// live"; callers store the resolved value back through memoizeWindowBackdrop.
WINUI3STYLE_EXPORT std::optional<int> memoizedWindowBackdrop(const QWidget *window);
WINUI3STYLE_EXPORT void memoizeWindowBackdrop(const QWidget *window, int backdrop);

WINUI3STYLE_EXPORT void clearWidgetPaintMemo(const QWidget *widget);

} // namespace WinUI3::Private
