#!/usr/bin/env bash
# Package the WebAssembly build of the gallery as a static site.
#
# Usage: tools/wasm/package_wasm.sh [build-dir] [output-dir]
#
# build-dir  CMake build tree of the Emscripten configuration
#            (default: <repo>/build-wasm)
# output-dir site root to write (default: <build-dir>/web)
#
# The output is plain static files (HTML/JS/wasm), relative to each other,
# so it can be served from any path, including a GitHub Pages project site.
set -euo pipefail

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_root=$(CDPATH= cd -- "$script_dir/../.." && pwd)

build_dir=${1:-"$repo_root/build-wasm"}
out_dir=${2:-"$build_dir/web"}
app=winui3style_gallery
app_dir=$build_dir/wasm

require_file() {
    if [ ! -f "$1" ]; then
        echo "error: missing $1" >&2
        echo "       build the WebAssembly target first: tools/wasm/build_wasm.sh" >&2
        exit 1
    fi
}

require_file "$app_dir/$app.js"
require_file "$app_dir/$app.wasm"
require_file "$app_dir/qtloader.js"
require_file "$script_dir/index.html"

mkdir -p "$out_dir"

cp -f "$app_dir/$app.js" "$out_dir/"
cp -f "$app_dir/$app.wasm" "$out_dir/"
cp -f "$app_dir/qtloader.js" "$out_dir/"
cp -f "$script_dir/index.html" "$out_dir/index.html"
# GitHub Pages runs Jekyll by default, which drops files and directories that
# start with an underscore; the empty marker file turns that off.
: > "$out_dir/.nojekyll"

echo "WebAssembly gallery packaged into $out_dir"
echo "  index.html               $(du -h "$out_dir/index.html" | cut -f1)"
echo "  $app.js                 $(du -h "$out_dir/$app.js" | cut -f1)"
echo "  $app.wasm               $(du -h "$out_dir/$app.wasm" | cut -f1)"
echo "  qtloader.js              $(du -h "$out_dir/qtloader.js" | cut -f1)"
echo
echo "Serve it over HTTP (file:// URLs cannot load WebAssembly modules):"
echo "  python3 -m http.server --directory "$out_dir" 8000"
echo "  open http://localhost:8000/"
