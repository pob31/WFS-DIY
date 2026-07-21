#!/usr/bin/env bash
# WFS-DIY project bootstrap — run once after cloning.
#
# Initialises / updates all git submodules (JUCE, spatcore, ...).
# Safe to re-run any time (e.g. after re-initing a submodule).
#
# Historical note: before JUCE 9 this script also patched the JUCE submodule
# on Linux to enable the userland evdev multitouch backend. JUCE 9 ships
# native XInput2 multitouch, so the stock submodule needs no patching.
#
# Usage:
#   ./tools/setup.sh

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

echo "==> Initialising submodules (first run can take a while)"
git submodule update --init --recursive

echo
echo "Setup complete. Build with:"
echo "  Linux:   cd Builds/LinuxMakefile && make CONFIG=Release -j\$(nproc)"
echo "  macOS:   open Builds/MacOSX/WFS-DIY.xcodeproj"
echo "  Windows: open Builds/VisualStudio2022/WFS-DIY.sln"
