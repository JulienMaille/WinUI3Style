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

## Border compositing refresh (2026-11-14)

A live user report (dark-mode input borders lighter than the official Gallery)
exposed a systematic compositing deviation: `borderedSurface` filled to the
control edge and stroked inside, blending translucent strokes over the fill.
XAML `Border` insets the background inside the border ring, so official
strokes composite over the parent surface. Measured on the supplied reference:
top `#303030` = white@18 over `#202020`, bottom `#9A9A9A` = white@139 over the
same background; the pre-fix Qt raster blended both over the fill instead
(`#424242` / `#A3A3A3` on the matching fill). Light mode hid the defect
(~2-3 levels); Dark shows ~14.

The fix insets the fill by the stroke extent (logical DIP for the editor
brush, device pixel for cosmetic strokes) and keeps the stroke ring fully
inside the bounds. `editorElevationBorder` now asserts stroke-over-surface for
the ring and fill-over-surface for interior rows (288 rows RED before the
fix); `buttonBorderCompositesOverParentSurface` replaces the
outer-pixel-keeps-canvas contract with the same XAML Border semantics.

Refresh provenance: local Windows Qt 6.11.1 Release offscreen captures,
deterministic across the two matrix runs. 50 of 68 PNGs change; 18 remain
byte-identical. 142,986 changed pixels; a 3x3 erosion audit leaves zero
survivors in every image, i.e. every delta is a <=2 px border ring and no
interior fill, text, or geometry pixel changed. Numeric spot check:
dark-state-button-rest outer ring (48,48,48) equals white@18 over the
(32,32,32) canvas; interior (45,45,45) equals the fill over the canvas.
Before/after panel: issue57-border-compositing-before-after.png.

The focused editor path composites through the same helper and therefore also
changes; the earlier "focused byte-identical to main" claim no longer applies
to this revision. Windows CI acceptance of this refresh at Qt 6.8.3 and 6.11.1
is pending at the time of writing; official WinUI side-by-side and native
compositor coverage remain unverified.
