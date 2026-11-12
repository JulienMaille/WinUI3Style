# Demo video tooling

Produces `renders/winui3style-widgets-live.mp4`: a 1080x1080, 27 second, two-panel
Light/Dark showcase of real Qt widgets painted by the `winui3` QStyle, with a
visible system cursor and live mouse/keyboard input.

Everything here is Windows-only and needs an **unlocked, interactive desktop**:
the capture script refuses to record when the input desktop is not `Default` or
when the showcase window cannot take foreground.

## Files

| File | Purpose |
|---|---|
| `showcase-main.cpp` | The demo application. Four pages of stock Qt widgets, driven by real `SendInput` mouse and keyboard events. |
| `CMakeLists.txt` | Standalone CMake project for the demo app (`winui3showcase`). |
| `capture-showcase.ps1` | Records one theme to a raw MP4 with `gdigrab` (cursor included). |
| `overlay-filter.txt` | ffmpeg filter graph: hstacks the two themes and draws the marketing callouts. |

## 1. Build the style and the demo app

The demo app loads the `winui3` style from the **plugin** directory, so build
the style first:

```powershell
cmake -S . -B build-wysiwyg-capture -G "Visual Studio 18 2026" -A x64 `
  -DQt6_DIR="C:/Qt/6-build/qt-6.12.0-dynamic-msvc-x64/lib/cmake/Qt6" `
  -DWINUI3STYLE_BUILD_TESTS=OFF
cmake --build build-wysiwyg-capture --config Release --parallel
```

Then the demo app (its own project, separate build tree):

```powershell
cmake -S tools/demo-video -B "$env:TEMP\winui3showcase-build" -G "Visual Studio 18 2026" -A x64 `
  -DQt6_DIR="C:/Qt/6-build/qt-6.12.0-dynamic-msvc-x64/lib/cmake/Qt6"
cmake --build "$env:TEMP\winui3showcase-build" --config Release --parallel
```

## 2. Record each theme

`capture-showcase.ps1` takes `-Theme Light|Dark`, `-QtRoot`, `-BuildDir`, `-Exe`,
`-Ffmpeg`, `-Scale`, `-MonitorX`, `-MonitorY` and `-DurationSeconds`. The
defaults assume the layout used for the published video: the showcase client on
the **second monitor** at `1920,0` and `QT_SCALE_FACTOR=2.109375`, which renders
a 256x512 logical panel into a 540x1080 half of the frame.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/demo-video/capture-showcase.ps1 `
  -Theme Light -Raw "$env:TEMP\demo-light.mp4" `
  -Exe "$env:TEMP\winui3showcase-build\Release\winui3showcase.exe"
powershell -NoProfile -ExecutionPolicy Bypass -File tools/demo-video/capture-showcase.ps1 `
  -Theme Dark -Raw "$env:TEMP\demo-dark.mp4" `
  -Exe "$env:TEMP\winui3showcase-build\Release\winui3showcase.exe"
```

If the window cannot take foreground the script throws rather than writing a
black capture. That happens when Windows has locked the session; unlock it and
retry. The system screen saver is already disabled on the authoring machine, but
a **group policy overrides the idle-sleep timeout to 30 minutes**, so the
session can still lock during a long session.

## 3. Compose the side-by-side movie

```powershell
$filter = Get-Content tools/demo-video/overlay-filter.txt -Raw
ffmpeg -y -i "$env:TEMP\demo-light.mp4" -i "$env:TEMP\demo-dark.mp4" `
  -filter_complex $filter -map '[out]' -an `
  -c:v libx264 -preset medium -crf 17 -pix_fmt yuv420p -r 30 -fps_mode cfr `
  -movflags +faststart renders/winui3style-widgets-live.mp4
```

`overlay-filter.txt` also contains the callout timings. Each scene owns a time
window; adjust the `enable='gte(t,...)'` expressions when the demo script in
`showcase-main.cpp` changes its interaction schedule.

## 4. Review before publishing

Always sample frames before replacing the published movie. Extract a frame at
each scene boundary and check: cursor present and legible, hover states visible,
the ComboBox popup open, the slider value popup visible, the date fully
readable, and the tab switch visible.

```powershell
ffmpeg -y -ss 9 -i renders/winui3style-widgets-live.mp4 -frames:v 1 -update 1 check.png
```

## Diagnostic switches (showcase app)

| Environment variable | Effect |
|---|---|
| `WINUI_DEMO_THEME` | `Dark` selects the dark theme. |
| `WINUI_INSPECT_POPUP` | Opens the ComboBox popup directly and skips the scripted timeline. Use this to study popup rendering in isolation. |
| `WINUI_TEST_NATIVE_REGION` | Applies a native rounded `SetWindowRgn` to the popup window. |
| `WINUI_TEST_NO_NATIVE_ACRYLIC` | Turns the DWM system backdrop off after the popup opens. |

The popup diagnostics write their state to `%TEMP%\winui-combo-inspect.txt` and
`%TEMP%\winui-region-result.txt`.

## Open defect: ComboBox popup corner wedges

The published movie shows gray wedges at the four corners of the open ComboBox
flyout. The state of that investigation is recorded in `spec/coverage.md` under
the reopened `QComboBox` popup corner fringe row. Summary of what is known:

- Reproduced in the running Gallery (not a capture artifact): gray wedges on
  all four corners at `QT_SCALE_FACTOR=2.109375`.
- The live popup really is DWM-acrylic: `DWMWA_SYSTEMBACKDROP_TYPE` reads `3`
  (transient window), `DWMWA_WINDOW_CORNER_PREFERENCE` reads `2` (round). The
  Qt container is translucent, not auto-filled, and its child viewport does not
  auto-fill either.
- Tried and **rejected**: painting the opaque panel stroke square (removes the
  wedges but loses the WinUI rounded silhouette), and clearing the whole
  backing before the rounded repaint (a unit test passed, the live wedges
  stayed). Both experiments were reverted.
- A native `SetWindowRgn` applies successfully (`applied=1`, region type 3) but
  the wedges remained, so the artifact is not the window region alone.
- Turning the DWM backdrop off after the popup opens makes the underlying
  widgets plainly visible through the Qt surface: the compositor material is
  required, it cannot simply be dropped.

Follow `spec/METHODOLOGY.md` §6: reproduce in the running Gallery, add a test
that fails for the observed mechanism, then fix, then re-run the same live
sequence. Compare against the pinned WinUI Gallery
(`Microsoft.WinUI3ControlsGallery` 2.9.3.0) before claiming the row verified.
