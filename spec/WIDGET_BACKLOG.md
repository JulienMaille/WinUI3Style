# QWidget coverage backlog

This is a short list of follow-up candidates, not a coverage claim. The
authoritative mapping and evidence status for implemented controls is
[`coverage.md`](coverage.md); acceptance rules are in
[`METHODOLOGY.md`](METHODOLOGY.md). Remove a candidate here once it has a
coverage row, rather than copying implementation details into this list.

## Validation still open

- `QSplitter`: the coverage row is source-audited; high-DPI separator and live
  drag review remain. Reproduce the older one-device-pixel report before
  changing geometry.
- `QScrollBar`: the coverage row is source-audited; native Light/Dark review
  remains. Reproduce the older reported defect before changing behavior.
- `QDialog` / `QMessageBox`: content/footer contracts are tested; live modal,
  dismissal, and Dark comparison remain (see the coverage row).

## Widget families without a coverage row

Scope these before implementation; the list does not imply a WinUI equivalent:

- `QGraphicsView`, `QFontComboBox`, `QColumnView`, and `QUndoView`;
- `QMdiArea` / `QMdiSubWindow`, `QLCDNumber`, and `QDial`;
- non-native `QFileDialog`, `QColorDialog`, `QFontDialog`, `QInputDialog`, and
  `QProgressDialog` composition.

Any candidate that is implemented must first receive an explicit Qt-to-WinUI
mapping in `coverage.md`. Composite widgets are not covered merely because
their children inherit a useful palette; geometry, input, disabled state,
theme, density, RTL where applicable, lifecycle, and visual evidence all
belong in that mapping's acceptance criteria.
