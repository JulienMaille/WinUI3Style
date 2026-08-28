# WinUI3Style

A Qt 6 Widgets style inspired by the released WinUI 3 controls in Windows App
SDK 2.4. It provides light and dark themes, system accent colors, animated
interaction states, Fluent glyphs, Mica main-window and opaque rounded popup
surfaces, property-driven native-widget variants, settings/navigation
composition, a demo gallery, and Qt Test coverage.

The project deliberately contains no QSS. All visuals are produced by
`QStyle`, widget painting, palettes, and native Windows backdrop APIs.

## Build

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_PREFIX_PATH=C:/Qt/6-build/qt-6.9.2-dynamic-msvc-x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The `winui3style_gallery` executable is the interactive reference application.
The style can also be loaded through `QStyleFactory::create("winui3")` when the
plugin is deployed under Qt's `styles` plugin directory.

The visual comparison workflow and pinned Microsoft sources are documented in
`spec/`. The public CMake target is `WinUI3::Widgets`. The implementation uses
`QCommonStyle` only as a small, public Qt fallback for semantic text/icon
layout and explicitly unsupported controls; all covered visual and interaction
contracts live in `WinUI3::Style`.

## Opt-in render benchmark

The render benchmark is deliberately not registered with normal CTest. Build it
with `-DWINUI3STYLE_BUILD_BENCHMARKS=ON` and run
`winui3style_render_benchmark.exe` with `QT_QPA_PLATFORM=offscreen`. It reports
p50/p95 render times for list, tree, table, and a rich toolbar/tab/header
surface. `--no-icons`, `--rows=N`, and `--iterations=N` keep comparisons
repeatable without making performance work part of the normal test gate.

## Native widget variants

Visual variants retain Qt's native semantics and are selected with dynamic
properties. A ToggleSwitch is fundamentally a `QCheckBox`:

```cpp
auto *toggle = new QCheckBox;
WinUI3::Style::setToggleSwitch(toggle);
WinUI3::Style::setToggleSwitchText(toggle, tr("On"), tr("Off")); // optional
```

The equivalent direct property contract is `winuiToggleSwitch=true`, with
optional `winuiOnText` and `winuiOffText` strings. QStyle owns its complete
geometry, painting, focus treatment, click/drag interaction, and state
animation. Applications that prefer a named type, including Qt Designer forms,
can use the style-library convenience class without introducing application-side
painting or animation:

```cpp
#include <winui3style/toggleswitch.h>

auto *toggle = new WinUI3::ToggleSwitch;
toggle->setOnText(tr("On"));
toggle->setOffText(tr("Off"));
```

The same opt-in model is available for style-owned settings-card chrome and
navigation items through `Style::setSettingsCard()` and
`Style::setNavigationView()`. Compound helper classes remain only where Qt does
not provide page composition or expansion behavior.

`QSpinBox` and `QDoubleSpinBox` use WinUI's horizontal inline buttons by
default. Applications that prefer the traditional Qt arrangement can opt into
stacked buttons without subclassing:

```cpp
WinUI3::Style::setVerticalSpinButtons(spinBox);
```

The equivalent dynamic property is `winuiVerticalSpinButtons=true`.

Item views use standard Qt models and delegates. `QListView` maps to WinUI
ListView, while a headerless `QTreeView` maps to TreeView. Multi-column tree
headers and `QTableView` are documented Fluent consistency extensions because
WinUI does not expose a matching core widget contract.

ContentDialog client chrome can be applied to an ordinary `QDialog` without a
custom dialog subclass:

```cpp
WinUI3::Style::setContentDialog(dialog);
```

The style supplies the surface, spacing, primary-button role and entrance
motion. The application continues to own the dialog's content and command
composition. `QMessageBox` receives the same surface automatically, including
official Segoe Fluent information, warning, error and help glyphs.
