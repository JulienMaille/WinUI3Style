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
Qt 6.5 (`find_package(Qt6 6.5 QUIET ...)`), falling back to Qt 5.5.12 when
Qt 6 is absent; Qt 5.12 is the only Qt 5 version exercised in CI.

`windows-2022` is pinned because it still ships VS17 with its setup
instance; `windows-latest` moved on and CMake's `Visual Studio 17 2022`
generator can no longer find VS there.

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
