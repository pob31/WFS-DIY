#!/usr/bin/env bash
# Apply Linux-only patches to the vendored JUCE submodule.
#
# Why this exists:
#   The JUCE 8.0.x distribution returns false from
#   detail::MouseInputSourceList::canUseTouch() on Linux, which means
#   peer->handleMouseEvent(InputSourceType::touch, ...) silently drops
#   every event. WFS-DIY's userland evdev multitouch backend
#   (Source/Controllers/Touch/Linux/) requires this to be true.
#
# Usage:
#   tools/apply-linux-juce-patches.sh
#
# Run once after cloning, and again whenever you re-init the JUCE submodule.
# Idempotent: skips patches that are already applied.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
JUCE_DIR="$REPO_ROOT/ThirdParty/JUCE"
PATCH_DIR="$REPO_ROOT/assets/linux/patches"

if [[ ! -d "$JUCE_DIR" ]]; then
    echo "JUCE submodule not initialised. Run 'git submodule update --init --recursive' first." >&2
    exit 1
fi

shopt -s nullglob
patches=("$PATCH_DIR"/*.patch)

if [[ ${#patches[@]} -eq 0 ]]; then
    echo "No patches found in $PATCH_DIR"
    exit 0
fi

cd "$JUCE_DIR"
for p in "${patches[@]}"; do
    name="$(basename "$p")"
    if git apply --check "$p" 2>/dev/null; then
        git apply "$p"
        echo "applied: $name"
    elif git apply --check --reverse "$p" 2>/dev/null; then
        echo "skipped (already applied): $name"
    else
        echo "FAILED: $name (does not apply cleanly to current JUCE checkout)" >&2
        exit 1
    fi
done

echo "Done."
