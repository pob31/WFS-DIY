#!/usr/bin/env bash
# Build a Linux release tarball for the WFS-DIY plugin suite.
#
# Usage:
#   Plugin/Installer/build-tarball-linux.sh
#
# Produces:
#   Plugin/Installer/Output/WFS-DIY-Plugins-Linux-x86_64-<version>.tar.gz
#
# Tarball layout:
#   <stem>/VST3/*.vst3/             — six VST3 bundles
#   <stem>/LV2/*.lv2/               — six LV2 bundles
#   <stem>/Standalone/              — six Standalone executables
#   <stem>/install.sh               — per-user (~/.vst3, ~/.lv2) or system install
#   <stem>/uninstall.sh
#   <stem>/README.txt
#
# Mirrors macOS/Windows installer flow on Linux. JUCE plugins on Linux must
# land at conventional filesystem paths (~/.vst3, ~/.lv2) — AppImage doesn't
# fit so a tarball + install.sh is the standard delivery mechanism.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PLUGIN_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
REPO_ROOT="$(cd "$PLUGIN_DIR/.." && pwd)"
BUILD_DIR="$PLUGIN_DIR/Builds/CMake-Linux"
OUTPUT_DIR="$SCRIPT_DIR/Output"

if [[ ! -d "$BUILD_DIR" ]]; then
    echo "ERROR: $BUILD_DIR not found." >&2
    echo "       Run 'Plugin/Scripts/build-all.sh --skip-installer' first." >&2
    exit 1
fi

# Pull version from the project's CMakeLists.txt — single source of truth.
VERSION="$(grep -oP 'project\(WFS-DIY-Plugins\s+VERSION\s+\K[0-9]+\.[0-9]+\.[0-9]+' "$PLUGIN_DIR/CMakeLists.txt")"
if [[ -z "$VERSION" ]]; then
    echo "ERROR: could not read VERSION from Plugin/CMakeLists.txt" >&2
    exit 1
fi

ARCH="$(uname -m)"
STAGE_NAME="WFS-DIY-Plugins-Linux-${ARCH}-${VERSION}"
STAGE_DIR="$OUTPUT_DIR/$STAGE_NAME"
TARBALL="$OUTPUT_DIR/$STAGE_NAME.tar.gz"

echo "==> Staging $STAGE_NAME"
mkdir -p "$OUTPUT_DIR"
rm -rf "$STAGE_DIR"
mkdir -p "$STAGE_DIR/VST3" "$STAGE_DIR/LV2" "$STAGE_DIR/Standalone"

# Collect bundles. The CMake build produces them under
# WFSPlugin*_artefacts/Release/{VST3,LV2,Standalone}/.
shopt -s nullglob
copied_vst3=0
copied_lv2=0
copied_sa=0
for artdir in "$BUILD_DIR"/WFSPlugin*_artefacts/Release; do
    for bundle in "$artdir"/VST3/*.vst3; do
        cp -R "$bundle" "$STAGE_DIR/VST3/"
        copied_vst3=$((copied_vst3 + 1))
    done
    for bundle in "$artdir"/LV2/*.lv2; do
        cp -R "$bundle" "$STAGE_DIR/LV2/"
        copied_lv2=$((copied_lv2 + 1))
    done
    for sa in "$artdir"/Standalone/WFS-DIY*; do
        cp "$sa" "$STAGE_DIR/Standalone/"
        chmod +x "$STAGE_DIR/Standalone/$(basename "$sa")"
        copied_sa=$((copied_sa + 1))
    done
done

# Bridge .so — needed by all plugins at load time. Lives next to the VST3s
# in install layout (per existing macOS/Windows convention).
BRIDGE_SO="$BUILD_DIR/libWFS-DIY-PluginBridge.so"
if [[ ! -f "$BRIDGE_SO" ]]; then
    echo "ERROR: WFS-DIY-PluginBridge .so not found at $BRIDGE_SO" >&2
    exit 1
fi
cp "$BRIDGE_SO" "$STAGE_DIR/VST3/"
cp "$BRIDGE_SO" "$STAGE_DIR/LV2/"

if [[ "$copied_vst3" -lt 6 || "$copied_lv2" -lt 6 ]]; then
    echo "WARNING: expected 6 VST3 + 6 LV2 bundles, got $copied_vst3 + $copied_lv2." >&2
    echo "         Make sure both formats are in WFS_PLUGIN_FORMATS in Plugin/CMakeLists.txt." >&2
fi
echo "    VST3: $copied_vst3 bundles, LV2: $copied_lv2 bundles, Standalone: $copied_sa binaries"

# Install / uninstall scripts.
cat > "$STAGE_DIR/install.sh" <<'INSTALL_EOF'
#!/usr/bin/env bash
# WFS-DIY Plugins installer.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"

MODE="${1:-}"
if [[ -z "$MODE" && -t 0 ]]; then
    read -r -p "Install for current user only (~/.vst3, ~/.lv2)? [Y/n] " ans
    case "${ans,,}" in n|no) MODE="--system" ;; *) MODE="--user" ;; esac
fi

case "${MODE:-}" in
    --user)
        VST3_DIR="$HOME/.vst3"
        LV2_DIR="$HOME/.lv2"
        SA_DIR="$HOME/.local/bin"
        SUDO=""
        ;;
    --system)
        VST3_DIR="/usr/local/lib/vst3"
        LV2_DIR="/usr/local/lib/lv2"
        SA_DIR="/usr/local/bin"
        SUDO="sudo"
        ;;
    *)
        echo "Usage: $0 [--user | --system]"
        exit 1
        ;;
esac

echo "==> Installing plugins to:"
echo "    VST3:       $VST3_DIR"
echo "    LV2:        $LV2_DIR"
echo "    Standalone: $SA_DIR"

$SUDO mkdir -p "$VST3_DIR" "$LV2_DIR" "$SA_DIR"

# VST3 bundles + the bridge .so they all share.
for bundle in "$HERE"/VST3/*.vst3; do
    name="$(basename "$bundle")"
    $SUDO rm -rf "$VST3_DIR/$name"
    $SUDO cp -R "$bundle" "$VST3_DIR/"
done
$SUDO install -m 0755 "$HERE/VST3/libWFS-DIY-PluginBridge.so" "$VST3_DIR/libWFS-DIY-PluginBridge.so"

# LV2 bundles + bridge .so.
for bundle in "$HERE"/LV2/*.lv2; do
    name="$(basename "$bundle")"
    $SUDO rm -rf "$LV2_DIR/$name"
    $SUDO cp -R "$bundle" "$LV2_DIR/"
done
$SUDO install -m 0755 "$HERE/LV2/libWFS-DIY-PluginBridge.so" "$LV2_DIR/libWFS-DIY-PluginBridge.so"

# Standalone binaries (optional; useful for quick testing without a host).
for sa in "$HERE"/Standalone/WFS-DIY*; do
    $SUDO install -m 0755 "$sa" "$SA_DIR/"
done

echo
echo "Done. Restart your DAW to pick up the new plugins."
echo "Look for 'WFS-DIY' in the plugin browser (manufacturer: Pix et Bel)."
echo "To uninstall: $HERE/uninstall.sh ${MODE}"
INSTALL_EOF
chmod +x "$STAGE_DIR/install.sh"

cat > "$STAGE_DIR/uninstall.sh" <<'UNINSTALL_EOF'
#!/usr/bin/env bash
set -euo pipefail
MODE="${1:-}"
case "$MODE" in
    --user)
        VST3_DIR="$HOME/.vst3"
        LV2_DIR="$HOME/.lv2"
        SA_DIR="$HOME/.local/bin"
        SUDO=""
        ;;
    --system)
        VST3_DIR="/usr/local/lib/vst3"
        LV2_DIR="/usr/local/lib/lv2"
        SA_DIR="/usr/local/bin"
        SUDO="sudo"
        ;;
    *)
        echo "Usage: $0 --user | --system"
        exit 1
        ;;
esac

for d in "$VST3_DIR"/WFS-DIY*.vst3 "$LV2_DIR"/WFS-DIY*.lv2; do
    [[ -e "$d" ]] && $SUDO rm -rf "$d"
done
$SUDO rm -f "$VST3_DIR/libWFS-DIY-PluginBridge.so" "$LV2_DIR/libWFS-DIY-PluginBridge.so"
for f in "$SA_DIR"/WFS-DIY*; do
    [[ -e "$f" ]] && $SUDO rm -f "$f"
done

echo "Uninstalled WFS-DIY plugins."
UNINSTALL_EOF
chmod +x "$STAGE_DIR/uninstall.sh"

cat > "$STAGE_DIR/README.txt" <<EOF
WFS-DIY Plugins ${VERSION} — Linux ${ARCH}

Installs:
  - 6 VST3 plugins  (Master + 5 Track variants)
  - 6 LV2 plugins   (same set)
  - 6 Standalone executables

Quick install:
    ./install.sh --user      # ~/.vst3 + ~/.lv2 (no sudo)
    ./install.sh --system    # /usr/local/lib/vst3 + /usr/local/lib/lv2 (sudo)

Hosts known to load these:
  VST3 — Reaper, Bitwig, Ardour ≥6.9, Tracktion Waveform, Mixbus
  LV2  — Ardour, Carla, Qtractor, Hydrogen, Mixbus
Live (Linux unsupported), Logic Pro (macOS only) and FL Studio (Windows
only) are not relevant on Linux.

Plugin sandboxing breaks the shared bridge between Master and Track
plugins. Disable per-plugin sandboxing in your host (Reaper "Run as
separate process", Bitwig per-plugin sandboxing) for the WFS-DIY plugins
on the same project.

Source: https://github.com/pob31/WFS-DIY
EOF

# Build the tarball.
cd "$OUTPUT_DIR"
echo "==> Creating $TARBALL"
tar --owner=0 --group=0 -czf "$TARBALL" "$STAGE_NAME"

# Cleanup the staging dir after archiving.
rm -rf "$STAGE_DIR"

echo
ls -lh "$TARBALL"
echo "Done."
