# Windows snapshot review

The issue 57 snapshot refresh uses authentic Windows CI captures, not Linux replacements.

## Provenance

- Run: https://github.com/JulienMaille/WinUI3Style/actions/runs/43602280237
- Source revision: `28c0c4fa6f4fd418148100fe4809e529f2f08eab`.
- Canonical artifact: `snapshot-debug-6.8.3-Release`, artifact ID `14117541037`, `build/snapshot-captures/first/`.
- `candidate-sha256.txt` records every candidate PNG hash.
- Manifest SHA256: `08fc2b2e29c22019054f79efd5676155b010307929c7a986cc19672dee8dffee`.

## Review result

All 68 candidate images match across eight capture sets: two captures for each Qt 6.8.3/6.11.1 Debug/Release job.

The refresh replaces 36 PNGs. All 21,781 changed pixels lie inside the intended rounded editor strokes. No changed pixel lies outside those strokes. Of those images, 26 change only the sidebar search border. The remaining 10 also change unfocused TextBox or NumberBox borders. Main review inspected both theme comparison panels. Independent review checked every changed image against exact stroke masks in all RGBA channels.

The other 32 images remain byte-identical to their approved baselines. No threshold or comparison gate changed. Masks classified the differences; masks did not alter the images or comparison gate.

## Later focus correction

The later production correction changes only focused editor callers. Gallery capture clears focus before each whole-page capture and excludes the focused TextBox capture. Its source remains unchanged.

Isolated old/current Linux builds each produced two complete capture sets. All 68 files matched byte-for-byte across those four sets. This establishes renderer equivalence for the focus-cleared captures. No Linux capture supplies a Windows baseline.

This review approves the deterministic Windows snapshot refresh. It does not claim current-head Windows CI success, official WinUI side-by-side validation, or native Windows compositor coverage.
