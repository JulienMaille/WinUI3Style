# Compatibility matrix

How to reproduce every validated configuration, and what each one proves.
All claims below are wired in `.github/workflows/ci.yml` or in the CTest
suite; a configuration not listed here is not claimed as supported.

## Windows / Qt

| Qt | Toolchain | Runner | Gate |
|----|-----------|--------|------|
| 6.8.3 (oldest supported) | MSVC 2022 x64 (`win64_msvc2022_64`) | `windows-2022` | Full `msvc-gate`: configure + build (Debug and Release) + install smoke + CTest (native excluded) + snapshot matrix, with `WINUI3STYLE_WARNINGS_AS_ERRORS=ON` |
| 6.11.1 (development version) | MSVC 2022 x64 (`win64_msvc2022_64`) | `windows-2022` | Same full gate as 6.8.3. Qt install uses the pinned `aqtinstall` dev snapshot (`16db45a`) because stock aqt 3.3.x has no checksum entry for the 6.11.1 Windows archives |
| 5.12.12 | MinGW 7.3 32-bit (`win32_mingw73` + `tools_win32_mingw730`) | `windows-2022` | Build only (`MinGW Makefiles`, Release, benchmarks and native tests off). No snapshot or DPI gate: offscreen fidelity and the Pillow matrix belong to the MSVC jobs |

Local development uses Qt 6.9.2 (MSVC x64). The `CMakeLists.txt` floor is
Qt 6.5 (`find_package(Qt6 6.5 QUIET ...)`), falling back to Qt 5.12 when
Qt 6 is absent; Qt 5.12 is the only Qt 5 version exercised in CI.

`windows-2022` is pinned because it still ships VS17 with its setup
instance; `windows-latest` moved on and CMake's `Visual Studio 17 2022`
generator can no longer find VS there.

## Linux / Qt

| Qt | Toolchain | Runner | Gate |
|----|-----------|--------|------|
| 6.8.3 (oldest supported) | GCC (`linux_gcc_64`) | `ubuntu-latest` | `linux-gate`: configure + build (Release) + CTest (native excluded) with `WINUI3STYLE_WARNINGS_AS_ERRORS=ON`, plus the consumer configurations below |

The library, the plugin, the gallery, and the portable test suite build on
non-Windows hosts (issue #21). What is Windows-only and compiles out:

- Mica/acrylic backdrop, caption colours, and the DWM grant
  (`winui3backdrop.cpp`, `winui3surfaces_p.cpp`);
- the system-appearance watcher and registry accent reads
  (`winui3appearancewatcher_p.cpp`, `winui3theme_p.cpp`);
- everything under `LABELS native` (real tooltips, menus, popups, focus,
  compositor corners) and `winui3style_snapshot_matrix`, which is not even
  registered off Windows: `spec/baselines/gallery` is the pinned Windows raster
  (Segoe UI, Fluent glyphs, `QT_QPA_FONTDIR`), so Linux font substitution would
  fail it for reasons that are not regressions. The oracle it uses,
  `winui3style_image_comparison_contract`, is portable and does run on Linux.

A consumer embedding the style selects the same two switches everywhere:

```cmake
set(WINUI_ENABLE_TESTS OFF)          # no Qt::Test, no Pillow, no test binaries
set(WINUI3STYLE_BUILD_DEMO OFF)      # no gallery
add_subdirectory(third_party/WinUI3Style)
```

Both default to `OFF` under `add_subdirectory()`/`FetchContent` and to `ON` for
the top-level developer build. `WINUI3STYLE_BUILD_TESTS` is the deprecated
spelling of `WINUI_ENABLE_TESTS` and still works.

## Capture reproduction

Snapshots are deterministic by construction (`spec/METHODOLOGY.md` sections
5/5a): two fresh gallery processes must produce identical file sets and
byte-identical PNGs, then match `spec/baselines/gallery`:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
ctest --test-dir build -C Release -R winui3style_snapshot_matrix -V
```

Or directly:

```powershell
build\demo\Release\winui3style_gallery.exe --capture-dir <absolute-dir>
python tools/compare_images.py <baseline.png> <candidate.png>
```

The matrix runs offscreen (`QT_QPA_PLATFORM=offscreen`,
`QT_QPA_FONTDIR=$env:SystemRoot/Fonts` so Segoe and Fluent glyphs render,
`QT_ACCESSIBILITY=0`, `WINUI3STYLE_DISABLE_ANIMATIONS=1`) and skips the
`paletteLabPage`. Mica/acrylic composition and native focus behavior are
not observable offscreen; they belong to the live compositor pass
(`winui3style_native`, serial, `QT_QPA_PLATFORM=windows`, excluded from the
hosted gate) and to side-by-side comparison with the pinned WinUI 3
Gallery 2.9.3 (`spec/winui-2.4/manifest.json`, Windows App Runtime per the
manifest's release link).
