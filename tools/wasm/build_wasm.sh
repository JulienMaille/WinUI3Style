#!/usr/bin/env bash
# Configure, build and package the WebAssembly gallery.
#
# Required environment:
#   EMSDK        root of an Emscripten SDK checkout; Qt 6.8.x WebAssembly
#                binaries are built with Emscripten 3.1.56
#   QT_WASM_DIR  Qt for WebAssembly installation
#                (e.g. $HOME/Qt/6.8.3/wasm_singlethread)
#   QT_HOST_DIR  matching desktop Qt installation for moc/uic/rcc
#                (e.g. $HOME/Qt/6.8.3/gcc_64)
#
# Optional environment:
#   BUILD_DIR    CMake build tree (default: <repo>/build-wasm)
#   OUT_DIR      packaged site (default: <BUILD_DIR>/web)
#   JOBS         parallel compile jobs (default: nproc)
#
# See docs/wasm.md for the one-time toolchain setup.
set -euo pipefail

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_root=$(CDPATH= cd -- "$script_dir/../.." && pwd)

EMSDK=${EMSDK:-}
QT_WASM_DIR=${QT_WASM_DIR:-}
QT_HOST_DIR=${QT_HOST_DIR:-}
build_dir=${BUILD_DIR:-"$repo_root/build-wasm"}
out_dir=${OUT_DIR:-"$build_dir/web"}

die() {
    echo "error: $*" >&2
    exit 1
}

[ -n "$EMSDK" ] || die "EMSDK is not set (see docs/wasm.md)"
[ -n "$QT_WASM_DIR" ] || die "QT_WASM_DIR is not set (see docs/wasm.md)"
[ -n "$QT_HOST_DIR" ] || die "QT_HOST_DIR is not set (see docs/wasm.md)"

[ -d "$EMSDK" ] || die "EMSDK does not exist: $EMSDK"
[ -d "$QT_WASM_DIR" ] || die "QT_WASM_DIR does not exist: $QT_WASM_DIR"
[ -d "$QT_HOST_DIR" ] || die "QT_HOST_DIR does not exist: $QT_HOST_DIR"

toolchain=$QT_WASM_DIR/lib/cmake/Qt6/qt.toolchain.cmake
[ -f "$toolchain" ] || die "not a Qt for WebAssembly installation: $QT_WASM_DIR"

# The Qt toolchain file locates emcc through EMSDK; activate it in this shell.
# shellcheck disable=SC1091
if [ -f "$EMSDK/emsdk_env.sh" ]; then
    . "$EMSDK/emsdk_env.sh" >/dev/null 2>&1 || true
fi
command -v emcc >/dev/null 2>&1 || die "emcc not found after sourcing $EMSDK/emsdk_env.sh"

generator=()
if command -v ninja >/dev/null 2>&1; then
    generator=(-G Ninja)
fi

echo "Configuring WebAssembly build in $build_dir"
cmake_args=()
cmake_args+=(-DCMAKE_BUILD_TYPE=Release)
cmake_args+=(-DCMAKE_TOOLCHAIN_FILE="$toolchain")
cmake_args+=(-DQT_HOST_PATH="$QT_HOST_DIR")
# Qt's wasm helpers copy their app files (js, wasm, qtloader.js) into the
# target's runtime output directory; package_wasm.sh collects them from there.
cmake_args+=(-DCMAKE_RUNTIME_OUTPUT_DIRECTORY="$build_dir/wasm")

cmake -S "$repo_root" -B "$build_dir" "${generator[@]}" "${cmake_args[@]}"

echo "Building"
cmake --build "$build_dir" --parallel "${JOBS:-$(nproc)}"

bash "$script_dir/package_wasm.sh" "$build_dir" "$out_dir"
