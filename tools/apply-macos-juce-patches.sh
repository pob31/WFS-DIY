#!/usr/bin/env bash
# Apply spatcore's macOS JUCE patches to ThirdParty/JUCE.
#
# The patches and the script that applies them live in spatcore
# (spatcore/patches/juce/macos/, spatcore/tools/juce-patches/), so every app
# built on spatcore shares them. Today there is one: canUseTouch() returns
# true, which the USB touch panel backend needs - without it every injected
# finger is dropped, and WFS_TEST_TOUCH_INJECT=1 fails.
#
# Run after cloning and after every 'git submodule update' that touches JUCE
# (a submodule checkout puts the file back). Safe to run again.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
exec "$REPO_ROOT/spatcore/tools/juce-patches/apply-juce-patches.sh" "$REPO_ROOT/ThirdParty/JUCE" macos
