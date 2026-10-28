#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 4 ]]; then
    echo "usage: $0 BUILD_DIR QT_ROOT QT_SOURCE_ASSETS OUTPUT_DIR" >&2
    exit 2
fi

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
mkdir -p "$1"
build_dir="$(cd "$1" && pwd)"
qt_root="$(cd "$2" && pwd)"
qt_sources="$(cd "$3" && pwd)"
output_dir="$(mkdir -p "$4" && cd "$4" && pwd)"
qmake="$qt_root/bin/qmake"
if [[ ! -x "$qmake" ]]; then
    echo "Qt qmake tool missing from $qt_root/bin" >&2
    exit 1
fi
if [[ -z "${ICU73_ROOT:-}" || ! -e "$ICU73_ROOT/lib/libicui18n.so.73" \
    || -z "${ICU73_SOURCE:-}" || ! -f "$ICU73_SOURCE/LICENSE" ]]; then
    echo "Qt's ICU 73 runtime and license must be provided through ICU73_ROOT/ICU73_SOURCE" >&2
    exit 1
fi

qt_version="$("$qmake" -query QT_VERSION)"
version="$(sed -nE 's/^project\(WinUI3Style VERSION ([0-9.]+).*/\1/p' "$repo_root/CMakeLists.txt" | head -n1)"
if [[ -z "$qt_version" || -z "$version" ]]; then
    echo "Could not determine the Qt or WinUI3Style version" >&2
    exit 1
fi
revision="$(git -C "$repo_root" rev-parse HEAD)"
name="WinUI3Style-${version}-linux-x64-gcc-qt${qt_version}-experimental.1"
prefix="$output_dir/stage/$name"

cmake -S "$repo_root" -B "$build_dir" \
    -DQt6_DIR="$qt_root/lib/cmake/Qt6" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_INSTALL_LIBDIR=lib \
    -DWINUI3STYLE_BUILD_PLUGIN=ON \
    -DWINUI3STYLE_BUILD_DEMO=ON \
    -DWINUI_ENABLE_TESTS=ON \
    -DWINUI3STYLE_BUILD_NATIVE_TESTS=OFF \
    -DWINUI3STYLE_WARNINGS_AS_ERRORS=ON \
    -DWINUI3STYLE_BUILD_BENCHMARKS=OFF
cmake --build "$build_dir" --parallel "$(nproc)"
ctest --test-dir "$build_dir" --output-on-failure -LE native

cmake --install "$build_dir" --prefix "$prefix" --component Runtime
# Qt's official Linux SDK is dynamically linked to ICU 73 but does not bundle
# those libraries. Ship the matching, hash-pinned ICU runtime beside Qt so the
# Gallery works on Ubuntu 22.04 (whose system ICU has a different SONAME).
mkdir -p "$prefix/lib" "$prefix/licenses"
cp -a "$ICU73_ROOT"/lib/libicu*.so.73* "$prefix/lib/"
cp "$ICU73_SOURCE/LICENSE" "$prefix/licenses/ICU-73.2-LICENSE.txt"
gallery="$prefix/bin/winui3style_gallery"
style_plugin="$prefix/plugins/styles/qwinui3style.so"
qxcb_plugin="$prefix/plugins/platforms/libqxcb.so"
for required_file in "$gallery" "$style_plugin" "$prefix/bin/qt.conf" "$qxcb_plugin"; do
    if [[ ! -f "$required_file" ]]; then
        echo "Runtime install is missing required file: $required_file" >&2
        find "$prefix" -maxdepth 3 -type f -print | sort >&2
        exit 1
    fi
done
if [[ ! -x "$gallery" ]]; then
    echo "Gallery is installed but not executable: $gallery" >&2
    exit 1
fi
if [[ -d "$prefix/include" || -d "$prefix/lib/cmake" ]]; then
    echo "Runtime install unexpectedly contains development files" >&2
    find "$prefix" -maxdepth 3 -type d -print | sort >&2
    exit 1
fi

# Catch missing non-system dependencies before creating a public archive. The
# host still supplies the documented glibc/graphics stack; Qt libraries and
# Qt plugins must resolve from the package itself.
for binary in "$gallery" "$style_plugin" "$qxcb_plugin"; do
    dependency_report="$(ldd "$binary")"
    if grep -q 'not found' <<<"$dependency_report"; then
        printf 'Unresolved runtime dependency for %s:\n%s\n' "$binary" "$dependency_report" >&2
        exit 1
    fi
done

# Launch the actual Gallery UI and capture a page from an empty environment:
# no Qt SDK paths, LD_LIBRARY_PATH, or QT_PLUGIN_PATH may be needed. This checks
# dynamic QStyleFactory plugin loading, window construction, and event-loop
# startup rather than merely asking QApplication to print --help.
smoke_home="$output_dir/smoke-home"
smoke_capture="$output_dir/smoke-capture"
mkdir -p "$smoke_home"
if env -i HOME="$smoke_home" PATH=/usr/bin:/bin QT_QPA_PLATFORM=offscreen \
    "$gallery" --capture-dir "$smoke_capture" \
    >"$output_dir/startup.stdout.txt" 2>"$output_dir/startup.stderr.txt"; then
    :
else
    status=$?
    echo "Clean-environment Gallery startup failed with exit code $status" >&2
    cat "$output_dir/startup.stdout.txt" "$output_dir/startup.stderr.txt" >&2
    echo "Clean-environment dependency report:" >&2
    env -i PATH=/usr/bin:/bin ldd "$gallery" >&2 || true
    exit "$status"
fi
for capture in light-page-0.png dark-page-0.png; do
    if [[ ! -s "$smoke_capture/$capture" ]]; then
        echo "Clean-environment Gallery launch did not create $capture" >&2
        cat "$output_dir/startup.stderr.txt" >&2
        exit 1
    fi
done

licenses="$prefix/licenses"
mkdir -p "$licenses"
cp "$repo_root/LICENSE" "$licenses/WinUI3Style-LGPL-2.1.txt"
cp -a "$repo_root/LICENSES" "$licenses/WinUI3Style-LICENSES"
for module in qtbase qtsvg qtimageformats; do
    archive="$qt_sources/${module}-everywhere-src-${qt_version}.tar.xz"
    if [[ ! -f "$archive" ]]; then
        echo "Missing verified Qt source archive: $archive" >&2
        exit 1
    fi
    python3 "$repo_root/tools/extract_qt_notices.py" "$archive" "$licenses/$module"
done
cp "$qt_sources/Qt-${qt_version}-source-manifest.json" "$licenses/"
cp "$repo_root/tools/poc/linux/README.txt" "$prefix/README.txt"

python3 - "$prefix/manifest.json" "$name" "$revision" "$qt_version" <<'PY'
import json
import platform
import sys
from pathlib import Path

path, name, revision, qt_version = sys.argv[1:]
manifest = {
    "name": name,
    "revision": revision,
    "qtVersion": qt_version,
    "icuRuntime": "ICU 73.2, bundled to satisfy the official Qt Linux SDK dependency",
    "platform": "Linux x86_64, GCC, dynamically linked",
    "hostBaseline": "Ubuntu 22.04 x86_64 or compatible Linux distribution",
    "runtime": "Qt libraries and plugins are bundled; glibc and graphics-system dependencies are provided by the host.",
    "validation": [
        "Release build with warnings-as-errors",
        "CTest passed with native-labelled tests excluded",
        "ldd dependency scan passed for Gallery and WinUI3Style plugin",
        "Gallery --help startup passed with a clean environment and offscreen QPA",
    ],
    "buildHost": platform.platform(),
}
Path(path).write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
PY

mkdir -p "$prefix/source"
git -C "$repo_root" archive --format=tar.gz --output="$prefix/source/WinUI3Style-source.tar.gz" HEAD
cp "$prefix/manifest.json" "$output_dir/${name}-manifest.json"

archive="$output_dir/${name}.tar.gz"
tar --sort=name --mtime='UTC 2026-01-01' --owner=0 --group=0 --numeric-owner \
    -C "$output_dir/stage" -cf - "$name" | gzip -n >"$archive"
(
    cd "$output_dir"
    sha256sum "${name}.tar.gz" >"${name}.tar.gz.sha256"
)
printf 'Packaged %s\n' "$archive"
