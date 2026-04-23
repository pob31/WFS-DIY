#!/usr/bin/env bash
# WFS-DIY Plugin Suite - macOS/Linux build orchestration
#
# Usage:
#   Plugin/Scripts/build-all.sh [--skip-installer]
#
# Runs: cmake configure -> cmake build Release -> pkgbuild on macOS.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLUGIN_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

SKIP_INSTALLER=0
for arg in "$@"; do
    case "$arg" in
        --skip-installer) SKIP_INSTALLER=1 ;;
    esac
done

OS_NAME="$(uname -s)"
case "$OS_NAME" in
    Darwin) BUILD_DIR="$PLUGIN_DIR/Builds/CMake-macOS" ;;
    Linux)  BUILD_DIR="$PLUGIN_DIR/Builds/CMake-Linux" ;;
    *)      BUILD_DIR="$PLUGIN_DIR/Builds/CMake" ;;
esac

mkdir -p "$BUILD_DIR"

if command -v ninja >/dev/null 2>&1; then
    CMAKE_GENERATOR="Ninja"
elif [ "$OS_NAME" = "Darwin" ]; then
    # Makefile generator mis-escapes parentheses in JUCE product names
    # (e.g. "WFS-DIY Track (Cartesian)"). Xcode handles them correctly.
    CMAKE_GENERATOR="Xcode"
else
    CMAKE_GENERATOR="Unix Makefiles"
fi

echo "==> Configuring CMake (generator: $CMAKE_GENERATOR)"
if [ "$CMAKE_GENERATOR" = "Xcode" ]; then
    # Xcode is multi-config — CMAKE_BUILD_TYPE is ignored at configure time.
    cmake -S "$PLUGIN_DIR" -B "$BUILD_DIR" -G "$CMAKE_GENERATOR"
else
    cmake -S "$PLUGIN_DIR" -B "$BUILD_DIR" -G "$CMAKE_GENERATOR" -DCMAKE_BUILD_TYPE=Release
fi

echo "==> Building WFSPluginsAll target"
cmake --build "$BUILD_DIR" --target WFSPluginsAll --config Release --parallel

if [ "$SKIP_INSTALLER" = "1" ]; then
    echo "Build complete. Installer skipped."
    exit 0
fi

if [ "$OS_NAME" = "Darwin" ]; then
    echo "==> Building macOS pkg"
    bash "$PLUGIN_DIR/Installer/build-pkg.sh"
else
    echo "No installer step defined for $OS_NAME; artefacts are under $BUILD_DIR."
fi
