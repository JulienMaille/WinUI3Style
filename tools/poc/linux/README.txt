WinUI3Style 0.1.0 experimental.1 — Linux x86_64 Gallery

Run `bin/winui3style_gallery` from the extracted directory. The archive
contains the WinUI3Style library and style plugin plus the Qt libraries and
platform plugins required by the Gallery. It was built with official Qt
6.11.1 for Linux x86_64 using GCC.

This package targets Ubuntu 22.04 x86_64 and compatible distributions. It is
not a static binary: glibc, the system graphics stack, and XCB/XKB runtime
libraries are supplied by the host. A missing system dependency will be
reported by the dynamic loader; install the matching package for your
distribution. On Ubuntu, the XCB/XKB runtime packages used by the release test
are `libxcb-cursor0`, `libxcb-icccm4`, `libxcb-image0`, `libxcb-keysyms1`,
`libxcb-randr0`, `libxcb-render-util0`, `libxcb-shape0`, `libxcb-sync1`,
`libxcb-xfixes0`, `libxcb-xinerama0`, `libxcb-xkb1`, and
`libxkbcommon-x11-0`. For a headless startup check, run:

    QT_QPA_PLATFORM=offscreen bin/winui3style_gallery --help

The package manifest records the exact source revision and validation limits.
Qt and WinUI3Style license and third-party notices are in `licenses/`. The
matching Qt source archives and their SHA-256 values are attached to the
GitHub prerelease. Verify the detached checksum before extracting:

    sha256sum -c WinUI3Style-0.1.0-linux-x64-gcc-qt6.11.1-experimental.1.tar.gz.sha256

This experimental Gallery package is not a Qt SDK or a development SDK for
building other applications.
