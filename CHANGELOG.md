# Changelog

All notable changes to WinUI3Style are recorded here. Versioning follows
`CMakeLists.txt` (`project(WinUI3Style VERSION x.y.z)`); the current version
is 0.1.0. See `RELEASE_NOTES.md` for the public API / Designer / experimental
split and `spec/coverage.md` for the per-control evidence ledger (all rows
remain `source-audited` until a live WinUI comparison promotes them).

## Unreleased

- `drawButtonControl` split per element with a `switch` dispatcher
  (`drawPushButtonControl`, `drawCheckRadioControl`,
  `drawToggleSwitchControl`, `drawPushButtonLabelControl`,
  `drawToolButtonLabelControl`); paint bodies unchanged. Guard:
  `buttonControlOwnershipByElement` (5 elements x Light/Dark x
  Standard/Compact).
- Shared warning policy `winui3style_warnings` (`/W4` + `/WX` on MSVC,
  `-Wall -Wextra -Wpedantic` + `-Werror` on GCC/Clang) applied to the
  library, plugin, gallery, all tests, and the benchmark. Migrated 13
  deprecated 5-argument `QMouseEvent` constructions to the Qt 6.9 6-argument
  form; marked shared test helpers `[[maybe_unused]]`.
- Unified backdrop erase strategy: one gate
  (`paintsDirectlyOnBackdrop`) with three documented recipes in
  `src/winui3helpers_p.h` (erase-then-fill, erase-means-transparent,
  `clearForBackdropFill` for acrylic popup pills); the hand-rolled
  `CompositionMode_Source` path in `src/winui3menus_p.cpp` now routes
  through the shared helper.
- Menu separator geometry contract (`menuSeparatorGeometryContract`): 7 px
  slot in both densities, 12 px line insets.
- Open ComboBox popup follows density switches (rows 40 -> 32 with popup
  resize via `sizeFromContents(CT_ItemViewItem)`); guard
  `openComboPopupFollowsDensitySwitch`.
- Missing Compact gallery demos (Password, AutoSuggest, Time picker,
  ListView, TreeView) plus a Compact collections tab; approved
  `dark/light-page-1-tab-5` snapshot baselines.
- Backdrop resize/expose/move contracts; chrome-shell and persistent-dialog
  theme-switch contracts; keyboard focus-visible restored on `KeyPress`
  after the Alt-reveal refactor; checked toolbar toggle fill kept over
  Mica; closed-combo slot covers longest item text plus 16+8 px icon
  chrome; affix spinbox prefix/suffix sizing contract.

## 0.1.0

Initial public surface, as described in `RELEASE_NOTES.md`:

- `WinUI3::Style` (`QStyle`, no QSS) with Light, Dark, and System themes,
  Standard and Compact densities, installable plugin keys `winui3` and
  `winui3compact`.
- Designer dynamic properties (`winuiControlRole`, `winuiBackdrop`,
  `winuiSurface`, `winuiToggleSwitch`, `winuiOnText`, `winuiOffText`,
  `winuiSettingsCard`, `winuiNavigationView`, `winuiVerticalSpinButtons`,
  `winuiContentDialog`, inherited `winuiDensity`).
- Optional composition helpers (`ToggleSwitch`, `SettingsCard`,
  `NavigationView`, `AnimatedStack`); Fluent icon helpers;
  `applyBackdrop()` for Mica / Mica Alt / Acrylic via
  `DWMWA_SYSTEMBACKDROP_TYPE`.
- Deterministic offscreen snapshot matrix (`winui3style_snapshot_matrix`),
  41 CTest cases (native interactive test excluded), and the
  `winui3style_source_contracts` / `winui3style_designer_gallery_contract`
  gates.
