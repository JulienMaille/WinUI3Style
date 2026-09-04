# QWidget coverage backlog

This document records the coverage audit performed on 2026-09-04. It is a
planning ledger, not a claim of WinUI fidelity. `coverage.md` remains the
evidence ledger for implemented controls.

## Deferred defects in existing coverage

- `QSplitter`: the visual separator/grip can be displaced by one device pixel
  at some scale factors.
- `QScrollBar`: the remaining reported scrollbar/"ascenseur" defect must be
  reproduced and reduced to a precise geometry or interaction contract before
  changing the renderer.
- Dialogs: ContentDialog and wizard-like surfaces must preserve the two WinUI
  layers (content surface and command/footer surface) in both themes.

## Implemented families that received gallery coverage in this pass

- checkable `QPushButton` (ToggleButton) and a button with a menu;
- standalone `QToolButton`, including checkable and split-menu variants;
- `QDoubleSpinBox`, combined `QDateTimeEdit`, and disabled/icon-bearing
  `QComboBox` variants;
- vertical and inverted `QProgressBar` variants;
- `QCommandLinkButton`, including its disabled state;
- the main-window `QStatusBar` and its automatic `QSizeGrip`;
- a standard `QWizard` reachable from the dialogs page;
- disabled and RTL item-view variants;
- an explicit floating/docked toggle for `QDockWidget`;
- persistent `QDialogButtonBox` and `QCalendarWidget` examples.

## Next common widgets

1. `QGraphicsView`, `QRubberBand`, `QKeySequenceEdit`, `QFontComboBox`, and `QToolBox`.
2. Non-native `QFileDialog`, `QColorDialog`, `QFontDialog`, `QInputDialog`, and
   `QProgressDialog` composition.

## Lower-priority or specialised widgets

- `QColumnView` and `QUndoView`;
- `QMdiArea` / `QMdiSubWindow`;
- `QLCDNumber` and `QDial`.

Subclass/composite widgets are not considered covered merely because their
children happen to inherit a useful palette. Each row must receive geometry,
mouse, keyboard, disabled, theme, density, RTL (where applicable), lifecycle,
and visual regression evidence before being promoted to `coverage.md`.

## Designer-only deployment note

The gallery still instantiates `WinUI3::SettingsCard` as a promoted Designer
widget. A later compatibility pass should determine whether all examples can
be represented by a standard `QFrame`/`QGroupBox` with
`winuiSettingsCard=true`, keeping the style-plugin-only deployment path free of
application recompilation requirements.
