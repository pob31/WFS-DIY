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

echo "==> Configuring CMake"
cmake -S "$PLUGIN_DIR" -B "$BUILD_DIR" -G "Ninja" -DCMAKE_BUILD_TYPE=Release

echo "==> Building WFSPluginsAll target"
cmake --build "$BUILD_DIR" --target WFSPluginsAll --parallel

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
