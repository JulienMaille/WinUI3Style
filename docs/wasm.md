# WebAssembly gallery

`winui3style_gallery` also builds for the browser with
[Qt for WebAssembly](https://doc.qt.io/qt-6/wasm.html). The result is a static
site — `index.html`, one JavaScript file and one `.wasm` module — that runs the
same widget gallery as the desktop build, with the WinUI 3 style applied.

The page is published by the `wasm-gallery` workflow to GitHub Pages:
<https://julienmaille.github.io/WinUI3Style/> (after the one-time Pages setup
below).

## How the browser build differs

Qt for WebAssembly ships without a runtime plugin loader in the
single-threaded configuration the demo targets, so the style cannot be loaded
from a `.dll`/`.so` the way desktop builds do. The `EMSCRIPTEN` configuration
in `CMakeLists.txt` therefore:

- builds `winui3style` as a static library instead of a shared one,
- skips the `winui3styleplugin` target, the CTest suites and the benchmark,
- links the static library into `winui3style_gallery`, whose `main()` creates
  `WinUI3::Style` directly instead of going through `QStyleFactory`.

Nothing in the style implementation changes; the same widget painting,
animation and density behavior runs in the browser.

## One-time toolchain setup

The pinned versions match Qt's WebAssembly binaries for Qt 6.8.x:

- [Emscripten SDK](https://emscripten.org/) 3.1.56
- Qt 6.8.3 for WebAssembly (`wasm_singlethread`)
- Qt 6.8.3 desktop build for the host tools (`moc`, `uic`, `rcc`)

```bash
git clone https://github.com/emscripten-core/emsdk.git "$HOME/emsdk"
"$HOME/emsdk/emsdk" install 3.1.56
"$HOME/emsdk/emsdk" activate 3.1.56

python3 -m pip install --user aqtinstall
aqt install-qt linux desktop 6.8.3 linux_gcc_64   --outputdir "$HOME/Qt"
aqt install-qt linux desktop 6.8.3 wasm_singlethread --outputdir "$HOME/Qt"
```

The scripts need a bash shell; on Windows run them from WSL.

## Build and run

```bash
export EMSDK="$HOME/emsdk"
export QT_HOST_DIR="$HOME/Qt/6.8.3/gcc_64"
export QT_WASM_DIR="$HOME/Qt/6.8.3/wasm_singlethread"

tools/wasm/build_wasm.sh
python3 -m http.server --directory build-wasm/web 8000
```

Open <http://localhost:8000/>. The build tree is `build-wasm/`; the packaged
site is `build-wasm/web/`. `BUILD_DIR` and `OUT_DIR` override both paths.

`tools/wasm/build_wasm.sh` configures with
`-DCMAKE_TOOLCHAIN_FILE=$QT_WASM_DIR/lib/cmake/Qt6/qt.toolchain.cmake` and
`-DQT_HOST_PATH=$QT_HOST_DIR`, then packages the output with
`tools/wasm/package_wasm.sh`.

The packaging step copies the application module, the Qt loader and
`tools/wasm/index.html`, which replaces Qt's generic generated launcher with a
page that shows download progress, surfaces startup errors with a Retry button,
and keeps a small "WebAssembly demo" badge after the gallery is running.

## Publishing to GitHub Pages

1. In the repository, open **Settings > Pages** and set **Source** to
   **GitHub Actions**.
2. Push to `main` (or run the `wasm-gallery` workflow manually from the
   Actions tab).

The workflow builds on every push and pull request; only `main` deploys. The
site is served from `/WinUI3Style/`, and all asset URLs are relative, so no base
path configuration is needed.

## Troubleshooting

- **Blank page, console mentions `application/wasm`** — the server must send
  the `application/wasm` MIME type for `.wasm` files. `python3 -m http.server`
  and GitHub Pages do; some minimal static servers do not.
- **`both async and sync fetching of the wasm failed`** — the `.wasm` file is
  missing from the site directory; re-run `tools/wasm/package_wasm.sh` and
  check that it reports `winui3style_gallery.wasm`.
- **`emcc not found`** — `EMSDK` is unset or the SDK is not activated; run
  `"$EMSDK/emsdk" activate 3.1.56` or let `build_wasm.sh` source
  `emsdk_env.sh`.
- **Qt version mismatch** — the WebAssembly Qt and the desktop Qt must be the
  same version; `QT_HOST_PATH` points at the desktop build.
- **Cross-origin isolation headers** — the single-threaded build does not need
  `Cross-Origin-Opener-Policy`/`Cross-Origin-Embedder-Policy`. If you switch to
  a threaded Qt build, serve it with those headers.