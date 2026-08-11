#!/usr/bin/env bash
# Build the wfs_headtrack webcam head-tracking plugin (Linux/macOS) and stage
# it next to the app. OpenCV comes from the system package manager:
#   Linux:  sudo apt install libopencv-dev
#   macOS:  brew install opencv
# or, for release packaging, set BUNDLED_OPENCV=1 to build a minimal STATIC
# OpenCV from source into the plugin (self-contained .so/.dylib — nothing to
# bundle or codesign besides the plugin itself; ~10 min extra on first build).
#
# Usage: [BUNDLED_OPENCV=1] tools/headtrack/build-headtrack-plugin.sh [Release|Debug] [stageDir]

set -euo pipefail

CONFIG="${1:-Release}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Plain string, not an array: macOS ships bash 3.2, where expanding an EMPTY
# array under `set -u` is an "unbound variable" error (broke CI). The value is
# a single space-free token, so unquoted expansion is safe.
EXTRA_DEFS=""
BUILD="$SCRIPT_DIR/build"
if [[ "${BUNDLED_OPENCV:-0}" == "1" ]]; then
    EXTRA_DEFS="-DWFS_HEADTRACK_BUNDLED_OPENCV=ON"
    BUILD="$SCRIPT_DIR/build-bundled"   # keep system-OpenCV and static caches apart
fi

# shellcheck disable=SC2086  # intentional word splitting of the single define
cmake -S "$SCRIPT_DIR" -B "$BUILD" -DCMAKE_BUILD_TYPE="$CONFIG" $EXTRA_DEFS

# --parallel with no count means unbounded `make -j` under the Makefiles
# generator; the static-OpenCV build then OOMs 16 GB CI runners (exit 143).
NJOBS="$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"
cmake --build "$BUILD" --parallel "$NJOBS"

if [[ "$(uname)" == "Darwin" ]]; then
    LIB="libwfs_headtrack.dylib"
else
    LIB="libwfs_headtrack.so"
fi

OUT="$BUILD"
[[ -f "$BUILD/$LIB" ]] || OUT="$BUILD/$CONFIG"

# Export self-check: a plugin without its ABI symbol would be silently useless.
if command -v nm >/dev/null 2>&1; then
    if ! nm -D --defined-only "$OUT/$LIB" 2>/dev/null | grep -q wfs_headtrack_abi_version \
       && ! nm -gU "$OUT/$LIB" 2>/dev/null | grep -q wfs_headtrack_abi_version; then
        echo "WARNING: $LIB is missing wfs_headtrack_abi_version - the app would not load it" >&2
    fi
fi

STAGE="${2:-}"
if [[ -z "$STAGE" ]]; then
    if [[ "$(uname)" == "Darwin" ]]; then
        STAGE="$ROOT/Builds/MacOSX/build/$CONFIG/WFS-DIY.app/Contents/MacOS"
    else
        STAGE="$ROOT/Builds/LinuxMakefile/build"
    fi
fi

if [[ -d "$STAGE" ]]; then
    cp "$OUT/$LIB" "$STAGE/"
    cp "$OUT/face_detection_yunet_2023mar.onnx" "$STAGE/"
    echo "Staged $LIB + model into $STAGE"
else
    echo "App output dir not found ($STAGE) - plugin left in $OUT"
fi

echo "OK: $OUT/$LIB"
