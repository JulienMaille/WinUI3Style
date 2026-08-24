# Visual reference workflow

The normative workflow and definition of done are in
[`METHODOLOGY.md`](METHODOLOGY.md); the per-control evidence ledger is in
[`coverage.md`](coverage.md). The notes below describe the capture mechanics.

The reference is the Microsoft WinUI 3 Gallery plus the corresponding files in
Microsoft's `microsoft-ui-xaml` repository. Capture each control at 100% display
scaling in light and dark themes, including rest, pointer-over, pressed, checked,
disabled, keyboard-focus, and transition midpoints.

Crop the WinUI and Qt captures to identical logical bounds, then run:

```text
python tools/compare_images.py reference.png candidate.png --diff diff.png
```

The comparator fails on dimension differences and never resizes. Its report
includes RMS, differing-pixel count, maximum channel delta, and the difference
bounding box. RMS is a regression signal, not the acceptance criterion. Font rasterization and
the compositor differ between XAML and Qt, so geometry, token colors, state
ordering, and timing are also reviewed against `winui-2.4/manifest.json`.

Generate the Qt matrix with `winui3style_gallery --capture-dir <absolute-dir>`.
Capture mode deliberately disables the native DWM material and paints WinUI's
solid fallback base; this removes compositor alpha and wallpaper-dependent tint
from the PNGs. Mica and Desktop Acrylic are validated separately in the live
side-by-side pass because their output is environment-dependent.

Acceptance requires both passes: the deterministic light/dark page matrix plus
live interaction checks for pointer-over, press, mouse focus, keyboard focus,
popup open/close, theme switching, and transition midpoints. A successful build
or a crash-free screenshot is not visual validation.

For automated repeatability, run the CTest target
`winui3style_snapshot_matrix`; it creates two fresh gallery processes, checks
the strict manifest and pixel identity, and compares an approved
`spec/baselines/gallery` directory when present.
