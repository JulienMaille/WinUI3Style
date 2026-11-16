WinUI3Style Gallery — Linux x86_64 prerelease

Run `bin/winui3style_gallery` from the extracted directory. Built with Qt
6.12.0 (GCC) for Ubuntu 22.04 and compatible distributions. Qt and ICU 73
libraries are bundled; glibc, the graphics stack and XCB/XKB runtime libraries
come from the host. Missing system dependencies are reported by the loader.

On Ubuntu, the XCB/XKB packages used by the release gate are libxcb-cursor0,
libxcb-icccm4, libxcb-image0, libxcb-keysyms1, libxcb-randr0,
libxcb-render-util0, libxcb-shape0, libxcb-sync1, libxcb-xfixes0,
libxcb-xinerama0, libxcb-xkb1 and libxkbcommon-x11-0.

Headless startup check:
    QT_QPA_PLATFORM=offscreen bin/winui3style_gallery --help

manifest.json records the exact source revision and validation limits. Qt and
WinUI3Style licenses and third-party notices are in licenses/. This is not a Qt
SDK; it is only the Gallery runtime.
