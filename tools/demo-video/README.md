# Demo video tooling

Produces `docs/images/winui3style-widgets-live.mp4`: a 1080x652, 30 second,
two-panel Light/Dark showcase of real Qt widgets painted by the `winui3` QStyle,
with a visible system cursor and live mouse/keyboard input, plus the animated
`docs/images/winui3style-widgets-live.webp` preview embedded in the top-level
README.

Everything here is Windows-only and needs an **unlocked, interactive desktop**:
the capture script refuses to record when the input desktop is not `Default` or
when the showcase window cannot take foreground. Recording takes over the real
mouse and keyboard for about 30 seconds per theme; do not touch them.

## Files

| File | Purpose |
|---|---|
| `showcase-main.cpp` | The demo application. Four pages of stock Qt widgets, driven by real `SendInput` mouse and keyboard events. |
| `CMakeLists.txt` | Standalone CMake project for the demo app (`winui3showcase`). |
| `capture-showcase.ps1` | Records one theme to a raw MP4 with `gdigrab` (cursor included). |
| `overlay-filter.txt` | ffmpeg filter graph: stacks the two themes under a separate caption header. |

## 1. Build the style and the demo app

The demo app loads the `winui3` style from the repository's `build/plugins`
directory, so build the style first:

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 `
  -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/msvc2022_64 -DWINUI_ENABLE_TESTS=OFF
cmake --build build --config Release --parallel
```

Then the demo app (its own project, separate build tree):

```powershell
cmake -S tools/demo-video -B "$env:TEMP\winui3showcase-build" -G "Visual Studio 18 2026" -A x64 `
  -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/msvc2022_64
cmake --build "$env:TEMP\winui3showcase-build" --config Release --parallel
```

## 2. Record each theme

`capture-showcase.ps1` takes `-Theme Light|Dark`, `-Raw`, `-QtRoot`, `-BuildDir`,
`-Exe`, `-Ffmpeg`, `-Scale`, `-MonitorX`, `-MonitorY` and `-DurationSeconds`.
The defaults place the client area at the primary monitor origin with
`QT_SCALE_FACTOR=2.109375`, which renders the 256x256 logical window into one
540x540 half of the frame on a 96 DPI monitor. Use a monitor scale of 100%, or
lower `-Scale` accordingly.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/demo-video/capture-showcase.ps1 `
  -Theme Light -Raw "$env:TEMP\demo-light.mp4"
powershell -NoProfile -ExecutionPolicy Bypass -File tools/demo-video/capture-showcase.ps1 `
  -Theme Dark -Raw "$env:TEMP\demo-dark.mp4"
```

If the window cannot take foreground the script throws rather than writing a
black capture. That happens when Windows has locked the session; unlock it and
retry.

## 3. Compose the movie and the README preview

ffmpeg 7 and later read the filter graph from a file with `-/filter_complex`:

```powershell
ffmpeg -y -i "$env:TEMP\demo-light.mp4" -i "$env:TEMP\demo-dark.mp4" `
  -/filter_complex tools/demo-video/overlay-filter.txt -map '[out]' -an `
  -c:v libx264 -preset medium -crf 17 -pix_fmt yuv420p -r 30 -fps_mode cfr `
  -movflags +faststart docs/images/winui3style-widgets-live.mp4
ffmpeg -y -i docs/images/winui3style-widgets-live.mp4 `
  -vf "fps=12,scale=720:-1:flags=lanczos" -c:v libwebp_anim -lossless 1 `
  -compression_level 6 -loop 0 -an docs/images/winui3style-widgets-live.webp
```

The lossless WebP keeps the accent colours exact; a palette GIF of the same
clip shifts them visibly.

`overlay-filter.txt` holds the caption timings. The scene cuts are at about
5.1 s, 16.2 s and 23.0 s of video time (the scripted `after()` time plus about
1.2 s of capture start-up). Find the actual cuts after changing the schedule in
`showcase-main.cpp`:

```powershell
ffmpeg -i "$env:TEMP\demo-light.mp4" -vf "select='gt(scene,0.08)',showinfo" -f null - 2>&1 |
  Select-String pts_time
```

## 4. Review before publishing

Always sample frames before replacing the published movie. Extract a frame in
each scene and check: cursor present and legible, hover states visible, the
ComboBox popup open, the slider value popup visible, real typing and the clear
button in the line edit, the More menu chevron and its icons fully inside the
frame, the date and time stepping, and the tab switch.

```powershell
ffmpeg -y -ss 9 -i docs/images/winui3style-widgets-live.mp4 -frames:v 1 -update 1 check.png
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
`docs/winui-2.4/observations.md`.

Still apply `docs/METHODOLOGY.md` §6 when changing popup painting: reproduce in
the running Gallery, add a mechanism test, fix, then re-run the live sequence.
