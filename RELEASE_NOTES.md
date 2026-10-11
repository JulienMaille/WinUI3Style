# Release notes

## 0.1.0-experimental.1 — release candidate, not published

The CMake library version remains `0.1.0`. The first experimental release's
intended tag is `v0.1.0-experimental.1`; it has not been created or pushed.
This preview evaluates stock Qt Widgets with a WinUI-inspired QStyle, not
production readiness or complete pixel-identical WinUI coverage.

### Distribution and installation

The Windows x64 archive must contain the gallery, `winui3style.dll`,
`styles/qwinui3style.dll`, compatible unmodified Qt DLLs/platform plugins,
`qt.conf`, license/third-party notices, corresponding-source information,
the WinUI3Style source archive and a SHA-256 manifest. The published build
must identify its exact commit, Qt version and compiler; no patched Qt is
required by the library.

Extract the complete archive and run `winui3style_gallery.exe`. Install the
current Microsoft Visual C++ v14 x64 Redistributable if absent. Windows 11
is needed to evaluate native Mica and rounded windows. Do not redistribute
Windows fonts or copy the bundled Qt DLLs over another application's runtime.

For a compatible Qt/MSVC x64 application, deploy the style plugin in its
Qt `styles` plugin directory and the style library beside its executable,
then launch with `-style winui3` or `-style winui3compact`. The package is not
a Qt 5, MinGW, ARM64 or 32-bit binary. Build those configurations separately.

### Known limitations and publication gates

- Per-control fidelity remains governed by `spec/coverage.md`, including
  partial native/compositor evidence and Qt consistency extensions.
- Mica/acrylic support depends on Windows and may fall back to opaque fills.
  Splitter and scrollbar details still need visual review.
- The RGB/alpha comparison checks all channels against the user-approved
  70-frame reference. Local Release CTest passes 59/59 with official Qt 6.11.1;
  no tolerance or assertion was weakened to adopt that reference.
- The earlier local archive used a patched developer Qt runtime. It is not
  the intended public artifact: use an unmodified official runtime with
  verifiable corresponding sources and notices.
- Windows-only PATH isolation of the earlier preview is not a clean-machine
  certification of the new artifact. Validate the exact extracted release
  archive before publication; earlier manual checks do not certify it.
- Require green hosted CI for the exact candidate commit. Local
  clang-format 18.1.8 passes; hosted CI for this candidate remains to confirm.
- A buttons-suite failure occurred during parallel CTest execution without
  an attributable assertion. Serial validation passes; persistent QtTest
  logs now retain diagnostics. The intermittent cause remains unconfirmed.

Local preflight on 2026-10-12: official Qt 6.11.1 Release configure/build,
whole-tree clang-format 18.1.8, all 59 CTest entries and all 26 split domain
binaries pass. The five package source-guard cases and the strict 70-frame
snapshot matrix are included. Calendar snapshots preserve the approved
2026-10-11 fixture without changing the live Gallery's date. The native
submenu fallback uses nested keyboard input, retaining the parent snapshot
assertions when pointer delivery is blocked. Neither result establishes
complete official WinUI parity or native shadow/motion verification.
Packaging refuses dirty/untracked sources and requires the complete Release
suite before staging; no archive is produced when that gate is red.

Create the annotated tag only after these gates pass. Neither an old green
run nor the local candidate ZIP certifies the new release revision.

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
