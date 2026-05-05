#!/usr/bin/env bash
# Build a Linux release tarball for the WFS-DIY standalone app.
#
# Usage:
#   tools/linux/build-app-tarball.sh
#
# Produces:
#   Builds/LinuxMakefile/release/WFS-DIY-Linux-x86_64-<version>.tar.gz
#
# The tarball contains the binary, lang/ and MCP/resources/ runtime data,
# the udev rules for HID controllers + touchscreens, the JUCE patch + apply
# script, an install.sh and uninstall.sh, and a .desktop entry. Mirrors the
# macOS .pkg / Windows .exe release flow on Linux.
#
# Pre-requisite: a Release build of the app must already be made
#   (cd Builds/LinuxMakefile && make CONFIG=Release -j$(nproc))
# This script does not invoke make itself; that's a separate concern, and
# keeping them split lets CI parallelise build vs. package.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="$REPO_ROOT/Builds/LinuxMakefile/build"
RELEASE_DIR="$REPO_ROOT/Builds/LinuxMakefile/release"

BIN="$BUILD_DIR/WFS-DIY"
if [[ ! -x "$BIN" ]]; then
    echo "ERROR: $BIN not found or not executable." >&2
    echo "       Run 'cd Builds/LinuxMakefile && make CONFIG=Release -j\$(nproc)' first." >&2
    exit 1
fi

# Pull version from the JUCERPROJECT element's version attribute (single
# source of truth). The element spans multiple lines, so use perl in slurp
# mode to skip past the unrelated <?xml version="1.0"?> declaration.
VERSION="$(perl -0777 -ne '/<JUCERPROJECT\b[^>]*\sversion="([^"]+)"/s and print $1' "$REPO_ROOT/WFS-DIY.jucer")"
if [[ -z "$VERSION" ]]; then
    echo "ERROR: could not read version from WFS-DIY.jucer" >&2
    exit 1
fi

ARCH="$(uname -m)"
STAGE_NAME="WFS-DIY-Linux-${ARCH}-${VERSION}"
STAGE_DIR="$RELEASE_DIR/$STAGE_NAME"
TARBALL="$RELEASE_DIR/$STAGE_NAME.tar.gz"

echo "==> Staging $STAGE_NAME"
rm -rf "$STAGE_DIR"
mkdir -p "$STAGE_DIR"

# Binary + runtime resources. We read lang/ and MCP/resources/ straight from
# the source-of-truth directories rather than the Makefile postbuild output —
# Projucer regens on macOS/Windows wipe the postbuild step, and we don't want
# the tarball to fall back to whatever stale lang/ files happened to be left
# next to the binary from a previous build.
cp "$BIN" "$STAGE_DIR/WFS-DIY"
mkdir -p "$STAGE_DIR/lang" "$STAGE_DIR/MCP/resources"
cp -R "$REPO_ROOT/Resources/lang/."                "$STAGE_DIR/lang/"
cp -R "$REPO_ROOT/Documentation/MCP/resources/."   "$STAGE_DIR/MCP/resources/"

# udev rules and the JUCE patch infrastructure.
mkdir -p "$STAGE_DIR/share"
cp "$REPO_ROOT/assets/linux/70-wfs-diy.rules" "$STAGE_DIR/share/"
cp "$REPO_ROOT/Resources/WFS-DIY_logo.png"     "$STAGE_DIR/share/"

# .desktop entry — installer rewrites Exec=/Icon= paths during install.
cat > "$STAGE_DIR/share/wfs-diy.desktop" <<'EOF'
[Desktop Entry]
Type=Application
Name=WFS-DIY
GenericName=Wave Field Synthesis Workstation
Comment=Live spatial audio app for Wave Field Synthesis rigs
Exec=__PREFIX__/bin/WFS-DIY
Icon=__PREFIX__/share/wfs-diy/WFS-DIY_logo.png
Terminal=false
Categories=AudioVideo;Audio;
StartupNotify=true
EOF

# Install script — supports per-user (~/.local) and system (/opt + /usr/local) layouts.
cat > "$STAGE_DIR/install.sh" <<'INSTALL_EOF'
#!/usr/bin/env bash
# WFS-DIY app installer.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"

MODE="${1:-}"
case "$MODE" in
    --user)   PREFIX="$HOME/.local"; SUDO="" ;;
    --system) PREFIX="/opt/wfs-diy"; SUDO="sudo" ;;
    *)
        echo "Usage: $0 [--user | --system]"
        echo
        echo "  --user    Install for current user only (~/.local). Default if stdin is a tty."
        echo "  --system  Install system-wide to /opt/wfs-diy (requires sudo)."
        if [[ -t 0 ]]; then
            read -r -p "Install for current user only? [Y/n] " ans
            case "${ans,,}" in n|no) MODE="--system"; PREFIX="/opt/wfs-diy"; SUDO="sudo" ;; *) MODE="--user"; PREFIX="$HOME/.local"; SUDO="" ;; esac
        else
            exit 1
        fi
        ;;
esac

BIN_DIR="$PREFIX/bin"
APP_DIR="$PREFIX/share/wfs-diy"
DESKTOP_DIR="$PREFIX/share/applications"

echo "==> Installing to $PREFIX (mode: $MODE)"
$SUDO mkdir -p "$BIN_DIR" "$APP_DIR" "$DESKTOP_DIR"

$SUDO install -m 0755 "$HERE/WFS-DIY"                  "$BIN_DIR/WFS-DIY"
$SUDO cp -R          "$HERE/lang"                      "$APP_DIR/lang"
$SUDO cp -R          "$HERE/MCP"                       "$APP_DIR/MCP"
$SUDO install -m 0644 "$HERE/share/WFS-DIY_logo.png"   "$APP_DIR/WFS-DIY_logo.png"

# .desktop entry, with prefix substituted.
sed "s|__PREFIX__|$PREFIX|g" "$HERE/share/wfs-diy.desktop" \
    | $SUDO tee "$DESKTOP_DIR/wfs-diy.desktop" >/dev/null

if command -v update-desktop-database >/dev/null 2>&1; then
    $SUDO update-desktop-database "$DESKTOP_DIR" || true
fi

# udev rules — always system-wide; offer to install.
echo
if [[ -t 0 ]]; then
    read -r -p "Install udev rules for HID controllers + touchscreens (sudo)? [Y/n] " ans
    case "${ans,,}" in n|no) ;; *)
        sudo install -m 0644 "$HERE/share/70-wfs-diy.rules" /etc/udev/rules.d/70-wfs-diy.rules
        sudo udevadm control --reload-rules
        sudo udevadm trigger
        echo "    udev rules installed. Replug HID/touchscreen devices to apply."
        ;;
    esac
else
    echo "Stdin not a tty — skipping udev rules. Copy share/70-wfs-diy.rules to /etc/udev/rules.d/ manually if needed."
fi

echo
echo "Done. Launch from your application menu, or run: $BIN_DIR/WFS-DIY"
echo "To uninstall: $APP_DIR/uninstall.sh"
INSTALL_EOF
chmod +x "$STAGE_DIR/install.sh"

cat > "$STAGE_DIR/uninstall.sh" <<'UNINSTALL_EOF'
#!/usr/bin/env bash
set -euo pipefail
PREFIX="${1:-$HOME/.local}"
SUDO=""; [[ "$PREFIX" != "$HOME/"* ]] && SUDO="sudo"

$SUDO rm -f  "$PREFIX/bin/WFS-DIY"
$SUDO rm -rf "$PREFIX/share/wfs-diy"
$SUDO rm -f  "$PREFIX/share/applications/wfs-diy.desktop"
[[ -n "$SUDO" && -f /etc/udev/rules.d/70-wfs-diy.rules ]] \
    && sudo rm -f /etc/udev/rules.d/70-wfs-diy.rules \
    && sudo udevadm control --reload-rules || true

echo "Uninstalled WFS-DIY from $PREFIX."
UNINSTALL_EOF
chmod +x "$STAGE_DIR/uninstall.sh"

# README for the tarball
cat > "$STAGE_DIR/README.txt" <<EOF
WFS-DIY ${VERSION} — Linux ${ARCH}

Quick install:
    ./install.sh --user      # ~/.local (no sudo)
    ./install.sh --system    # /opt/wfs-diy (sudo)

Or just run in place:
    ./WFS-DIY

The app expects lang/ and MCP/ to sit next to the binary, which they do
in this archive. udev rules under share/70-wfs-diy.rules grant non-root
access to Stream Deck, Xencelabs Quick Keys, 3Dconnexion SpaceMouse and
USB touchscreens. The installer offers to drop them in /etc/udev/rules.d
on request.

Source: https://github.com/pob31/WFS-DIY
EOF

# Build the tarball.
cd "$RELEASE_DIR"
echo "==> Creating $TARBALL"
tar --owner=0 --group=0 -czf "$TARBALL" "$STAGE_NAME"

# Cleanup the staging dir after archiving (the tarball is the artefact).
rm -rf "$STAGE_DIR"

echo
ls -lh "$TARBALL"
echo "Done."
