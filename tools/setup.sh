#!/usr/bin/env bash
# WFS-DIY project bootstrap — run once after cloning.
#
# Collapses post-clone setup into a single idempotent command:
#   1. Initialise / update all git submodules (JUCE, GPUAudioSDK, ...).
#   2. On Linux, apply the JUCE multitouch patch so touch works in local
#      builds (see tools/apply-linux-juce-patches.sh). The patch only edits
#      juce_Windowing_linux.cpp, so it is skipped on macOS/Windows.
#
# The Linux Makefile exporter has no pre-build hook (unlike the Xcode/VS
# exporters), so the patch can't be applied automatically at build time on
# Linux — this script is how a cloner gets multitouch without having to know
# about the patch. Safe to re-run any time (e.g. after re-initing a submodule).
#
# Usage:
#   ./tools/setup.sh

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

echo "==> Initialising submodules (first run can take a while)"
git submodule update --init --recursive

if [[ "$(uname -s)" == "Linux" ]]; then
    echo "==> Applying Linux JUCE patches"
    tools/apply-linux-juce-patches.sh
else
    echo "==> Host is $(uname -s); skipping Linux-only JUCE patches"
fi

echo
echo "Setup complete. Build with:"
echo "  Linux:   cd Builds/LinuxMakefile && make CONFIG=Release -j\$(nproc)"
echo "  macOS:   open Builds/MacOSX/WFS-DIY.xcodeproj"
echo "  Windows: open Builds/VisualStudio2022/WFS-DIY.sln"
