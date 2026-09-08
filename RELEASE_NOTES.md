# Release notes

## 0.1.0

### Public C++ API

Applications may link the `WinUI3::Widgets` CMake target and use the exported
headers under `include/winui3style/`. The public surface includes
`WinUI3::Style` (theme, density, accent, control-role, and Designer-property
helpers), the `ThemeMode`, `DensityMode`, and `ControlRole` enums, Fluent icon
helpers, `applyBackdrop()`, and the optional `ToggleSwitch`, `SettingsCard`,
`NavigationView`, and `AnimatedStack` classes. The style is also available as
the Qt factory keys `winui3` and `winui3compact` when the plugin is installed.

### Qt Designer properties

Existing Qt widgets can opt into documented visual variants through dynamic
properties in Designer: `winuiControlRole`, `winuiBackdrop`, `winuiSurface`,
`winuiToggleSwitch`, `winuiOnText`, `winuiOffText`, `winuiSettingsCard`,
`winuiNavigationView`, `winuiVerticalSpinButtons`, `winuiContentDialog`, and
inherited `winuiDensity`. These properties preserve Qt widget semantics; the
style owns the corresponding geometry, painting, focus, and state animation.

### Experimental and extension behavior

Qt mappings without a direct WinUI control are consistency extensions. This
includes the DataGrid-like `QTableView`/headers, multi-column tree headers,
stacked spin buttons, command-link and other composition surfaces, and related
settings/navigation composition. They are implementation extensions and are
not claims of official WinUI templates. The public `SettingsCard`,
`NavigationView`, and `AnimatedStack` helpers remain optional composition
APIs; a standard widget plus its documented property is preferred where the
README describes that route.

Coverage is currently source-audited. Light/dark rendering, popup first-frame
stability, pointer and keyboard transitions, focus modality, high-DPI behavior,
and Mica/acrylic compositor results still require live comparison with the
pinned WinUI Gallery. Offscreen tests and snapshot similarity do not promote an
extension or native backdrop result to fully verified behavior.
