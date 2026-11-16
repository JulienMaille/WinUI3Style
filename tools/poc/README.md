# WinUI3Style Gallery package

Windows x64 package built against Qt 6.12.0 with MSVC 2022. It is a prerelease
for evaluating the style on stock Qt Widgets, not a production release or a
Microsoft product. `manifest.json` records the exact revision and toolchain.

## Run

Extract the whole folder and run `winui3style_gallery.exe`; do not run it from
inside the archive. Qt does not need to be installed. The VC++ x64 runtime is
included but not installed automatically. Use View → Style to switch between
WinUI 3 and Qt's Windows 11 style for comparison.

To use the style in an existing Qt application, deploy `styles/qwinui3style.dll`
and start with `-style winui3` (or `winui3compact`). The style implementation is
embedded in the plugin, so no `winui3style.dll` companion is needed. Use a
compatible Qt/MSVC build and architecture; this package is x64 only.

Coverage is tracked in `docs/coverage.md`. Native backdrops need Windows support
and may fall back to opaque surfaces. No accessibility certification is claimed.

## Build a package

Configure and build Release first, then:

```powershell
./tools/package_poc.ps1 -BuildDir build -QtRoot C:/Qt/6.12.0/msvc2022_64
```

Add `-SkipTests` to skip the hosted CTest gate. The packager refuses a dirty
source tree, deploys the Qt runtime, writes `manifest.json`, and stages the
folder under `build/poc`.

## Licensing

Qt is dynamically linked; review its LGPL/GPL obligations before distribution
and provide the corresponding Qt sources. WinUI3Style is LGPL-2.1 and its
license is in `licenses/`.
