# WinUI3Style

A Qt 6 Widgets style inspired by the released WinUI 3 controls in Windows App
SDK 2.4. It provides light and dark themes, system accent colors, animated
interaction states, Standard and Compact density profiles, Fluent glyphs,
Mica main-window and opaque rounded popup
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
Its complete widget hierarchy lives in `demo/gallerywindow.ui`: it links only
Qt Widgets and deliberately uses no `WinUI3::*` API. This makes it an executable
example of the same deployment model available to an existing Designer-based
application.
The style can also be loaded through `QStyleFactory::create("winui3")` when the
plugin is deployed under Qt's `styles` plugin directory. The optional
`winui3compact` factory key creates the same style with Compact as its global
default; it is an alias, not a second implementation.

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
properties. In Qt Designer, add these properties with the Property Editor's
`+` button. A ToggleSwitch is fundamentally a `QCheckBox` with
`winuiToggleSwitch=true`; `winuiOnText` and `winuiOffText` are optional strings.
No application-side painting or animation is required.

Other Designer properties are:

- `winuiControlRole`: `standard`, `accent`, `subtle`, `navigation`, or
  `destructive` on buttons.
- `winuiVerticalSpinButtons=true` on `QSpinBox`/`QDoubleSpinBox`.
- `winuiSettingsCard=true` on a standard `QFrame` or `QGroupBox`.
- `winuiNavigationView=true` on a standard Qt item view.
- `winuiContentDialog=true` on a standard `QDialog`.
- `winuiBackdrop`: `none`, `mica`, `mica-alt`, or `acrylic` on a top-level
  window.
- `winuiSurface=content` or `layer` on client regions placed over a native
  backdrop. These create opaque WinUI content and navigation layers and prevent
  backing-store trails during hover, scrolling and page changes.
- `winuiDensity=standard` or `compact` on any container or control. The value
  is inherited by its child widgets and can be changed while the application is
  running.

## Standard and Compact density

Density is independent of light/dark theme. `winui3` starts in Standard mode;
`winui3compact` starts in Compact mode. Applications can change the global
profile without replacing the style:

```cpp
qApp->style()->setProperty("densityMode", 1); // Compact
```

For mixed-density forms, set the inherited `winuiDensity` dynamic property on
a page, panel, or individual control in Qt Designer. Compact follows WinUI's
Compact Sizing scope: TextBox/LineEdit, ComboBox and its popup rows,
QDateEdit/QTimeEdit, ListView, TreeView, NavigationView, and MenuBar become
shorter. NumberBox-style QSpinBox/QDoubleSpinBox controls follow the TextBox
height as a Qt consistency extension. Buttons, check/radio and toggle controls,
sliders, toolbars, tabs, menu flyouts, tables, and headers retain their Standard
metrics.

The equivalent C++ property call, when a form cannot be edited, is simply:

```cpp
auto *toggle = new QCheckBox;
toggle->setProperty("winuiToggleSwitch", true);
toggle->setProperty("winuiOnText", tr("On"));
toggle->setProperty("winuiOffText", tr("Off"));
```

QStyle owns its complete geometry, painting, focus treatment, click/drag
interaction, and state animation. The public helper methods and convenience
classes remain source-compatible wrappers, but applications do not need them.

`QSpinBox` and `QDoubleSpinBox` use WinUI's horizontal inline buttons by
default. Applications that prefer the traditional Qt arrangement can opt into
stacked buttons without subclassing:

Set `winuiVerticalSpinButtons=true` for the traditional stacked arrangement.

Item views use standard Qt models and delegates. `QListView` maps to WinUI
ListView, while a headerless `QTreeView` maps to TreeView. Multi-column tree
headers and `QTableView` are documented Fluent consistency extensions because
WinUI does not expose a matching core widget contract.

ContentDialog client chrome can be applied to an ordinary `QDialog` without a
custom dialog subclass:

Set `winuiContentDialog=true` on the dialog.

The style supplies the surface, spacing, primary-button role and entrance
motion. The application continues to own the dialog's content and command
composition. `QMessageBox` receives the same surface automatically, including
official Segoe Fluent information, warning, error and help glyphs.
