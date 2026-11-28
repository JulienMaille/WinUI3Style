// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

// Cache dynamic properties only while WinUI3Style observes the widget.
// Unobserved widgets and declared properties are resolved live.

#include <winui3style/winui3global.h>
#include <winui3style/winui3style.h>

#include <optional>

class QWidget;

namespace WinUI3::Private {

void observeWidgetPaintMemo(QWidget *widget, const Style *style);
void forgetWidgetPaintMemo(const QWidget *widget, const Style *style);

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
