# WinUI3Style 0.1.0 — experimental PoC

Windows x64, MSVC 2022, Qt 6.9.2. This is a preview for evaluating a QStyle
on stock Qt Widgets, not a production release or a Microsoft product.
The package manifest records the exact source revision and binary hashes.

## Run the portable gallery

Extract the entire ZIP into a writable folder and run
`winui3style_gallery.exe`. Do not run it inside the ZIP or copy only the EXE.
Qt does not need to be installed separately. If the MSVC runtime is absent,
install Microsoft's current Visual C++ v14 x64 Redistributable (at least as
recent as the MSVC toolchain used to build the package):
https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist

Use Windows 11 for native Mica and rounded-window evaluation. Fonts are supplied
by Windows; the package does not redistribute Segoe fonts. Light/Dark/System,
Standard/Compact and the Mica toggle are available in the gallery. Animations
are enabled normally; deterministic capture mode disables them intentionally.

## Try an existing Qt application

The factory keys are `winui3` and `winui3compact`. Deploy
`styles/qwinui3style.dll` into that application's Qt style-plugin directory
and put `winui3style.dll` beside its executable (or on its DLL search path).
Start the application with `-style winui3` or `-style winui3compact`.

Use a compatible Qt/MSVC build and architecture. This x64 Qt 6 package is not
a Qt 5, MinGW, ARM64 or 32-bit plugin. Do not overwrite another application's
Qt libraries with those from this gallery package.

The plugin is not a standalone DLL: it depends on `winui3style.dll`, Qt
Core/Gui/Widgets and the MSVC runtime. It does not require Windows App SDK or
an additional third-party style library. Designer dynamic properties remain
optional and are documented in the repository README.

## Scope and limitations

The preview demonstrates controls, collections, settings, dialogs, density,
theme switching and native backdrop integration. Coverage remains governed by
`spec/coverage.md`; green tests do not certify every WinUI state or animation.
Native backdrops depend on Windows support and may fall back to opaque
surfaces. CommandLinkButton, tables and some compound Qt widgets are Fluent
consistency extensions rather than exact WinUI control templates. Splitter
and scrollbar details still need a subsequent visual review.

## Produce the package

Build Release and run the normal CTest suite first, then:

```powershell
./tools/package_poc.ps1 -BuildDir build `
  -QtRoot C:/Qt/6-build/qt-6.9.2-dynamic-msvc-x64 `
  -QtSourceRoot C:/qt-6.9.2
```

The script creates a fresh folder under `build/poc`, deploys Qt, copies
licenses/third-party attribution files and SBOMs, includes a WinUI3Style
source archive, and writes the package manifest and ZIP SHA-256. It refuses
textual tracked-source changes so the source archive matches the product
revision. It checks startup with only Windows on PATH and no Qt plugin-path
override; this is a local isolation test, not a clean-machine certification.
It never tags, pushes, uploads or publishes a release.

## Before public distribution

The generated ZIP is a **local candidate**, not automatically publication-ready.
Test the extracted ZIP on a Windows machine without Qt/developer tools and
exercise both themes, both densities, menus, suggestions, dialogs and repeated
Mica on/off cycles. Confirm the exact candidate commit's hosted CI is green.

The local packaged gallery produces the same 66 PNGs as the current build.
However, a direct image comparison still detects RGB differences against the
approved baseline while the existing snapshot CTest passes: its RGBA
`getbbox()` check can overlook RGB-only differences with identical alpha.
This pre-existing oracle defect must be reproduced and repaired in a separate
test batch; do not treat the current green snapshot gate as pixel-parity proof.

Qt is dynamically linked and replaceable. Review its LGPL/GPL license texts,
third-party notices and corresponding-source requirements before distributing
the bundled runtime: https://www.qt.io/licensing/open-source-lgpl-obligations
Supply the exact corresponding Qt sources, including local modifications and
build instructions, through the release's source-download mechanism. Installed
SBOM provenance alone is not a substitute for those sources. The source tree
passed to the script supplies notice files; its mere existence does not prove
that it matches the compiled Qt libraries. The local Qt SBOM records a modified
build, so an unmodified upstream 6.9.2 download must not be claimed to match it.

WinUI3Style's license is included in `licenses/WinUI3Style-LGPL-2.1.txt` and its
sources in `WinUI3Style-source.zip`. Microsoft reference resources are not
executed or shipped as XAML. No accessibility certification is claimed by this
preview; existing Qt semantics and tests are retained.
