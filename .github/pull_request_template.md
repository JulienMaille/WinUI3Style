# PR checklist — routes to docs/METHODOLOGY.md (normative), never duplicates it.

## Mapping
- [ ] Declared the Qt→WinUI mapping in `docs/coverage.md` (Direct / Semantic
  variant / Consistency extension / Compound / Not covered, METHODOLOGY §1).

## Live evidence (METHODOLOGY §3)
- [ ] Ran the live-input sequence (rest, hover, press, disabled, mouse focus,
  Tab focus, keyboard press, popup open/close, drag, animation start/mid/end
  + reversal) in light and dark at 100% scaling.

## Fallback removal (METHODOLOGY §1)
- [ ] If a fallback was removed or `sizeFromContents()` changed: first-open
  test with a nonzero selected index passes (no one-frame jump).

## Snapshot baselines (METHODOLOGY §5a)
- [ ] If the gallery widget hierarchy, capture set, or any painted visual
  changed: regenerated and approved `tests/baselines/gallery` (plus the Qt
  version overlay under `tests/baselines/gallery-qt/<version>` wherever the
  raster differs), reviewing every diff. `winui3style_snapshot_matrix` fails
  on a missing baseline or an extra capture. Do not generate Windows baselines
  on Linux.

## Unpolish restoration (P1 ledger)
- [ ] Every attribute the change touches is restored on unpolish
  (`styleMutationRestoration` covers the touched path).

## Coverage ledger (METHODOLOGY §6)
- [ ] Updated the `docs/coverage.md` row status (`source-audited` vs
  `Verified` — tests alone never promote a row to `Verified`).
