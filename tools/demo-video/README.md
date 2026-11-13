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

## Popup corner repair (2026-11-12)

The full-rect Source clear removes stale backing-store pixels but did not fix
all native corner arcs. The remaining defect was reproduced with the actual
Gallery widgets: Qt DPR 4.21875 versus HWND DPI 192. Windows ignores
`QT_SCALE_FACTOR`, so the painted r8 curve was larger than the native corner,
exposing untinted Acrylic between them. Popup fill, stroke and menu-row refill
now use r8 converted from native HWND DPI to Qt coordinates; offscreen and
masked fallbacks retain Qt r8. `nativePopupPaintMatchesCornerDpi` is RED→GREEN
in all 12 Light/Dark, Standard/Compact, ComboBox/menu/tool-button cases.
The installed official Gallery 3.0.4.0 corner comparison completed in Light/Dark
at 200% monitor scaling: repaired Qt ComboBox/menu/tool-button panels show no
untinted corner wedges across hover, press and repeated opens. This newer,
self-contained Windows App SDK 2.5 experimental reference does not replace the
project's 2.4 pin. Opening motion differs (Qt moves the HWND; official WinUI
reveals content within fixed bounds), and exact first-presented-frame and
outer-shadow parity were not established. The earlier Verified claim remains
withdrawn; the coverage row stays partial. See the supplementary comparison in
`spec/winui-2.4/observations.md`. Movie work is paused.

Still apply `spec/METHODOLOGY.md` §6 when changing popup painting: reproduce in
the running Gallery, add a mechanism test, fix, then re-run the live sequence.
