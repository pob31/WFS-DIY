#!/usr/bin/env bash
# WFS-DIY Plugin Suite - macOS .pkg builder
#
# Builds a .pkg from a CMake Release build tree. Codesigning and notarization
# are gated behind env vars so unsigned local builds still produce a usable pkg.
#
#   CODESIGN_IDENTITY_APP     - "Developer ID Application: ..." (optional)
#   CODESIGN_IDENTITY_INSTALL - "Developer ID Installer: ..."   (optional)
#
# Notarization credentials (resolved in this order, same contract as
# scripts/ci/build-macos-release.sh; notarization runs when any is set):
#   1. App Store Connect API key (used by CI):
#        NOTARY_API_KEY (path to .p8) + NOTARY_API_KEY_ID + NOTARY_API_ISSUER
#   2. Apple-ID app-specific password:
#        NOTARY_APPLE_ID + NOTARY_PASSWORD + NOTARY_TEAM_ID
#   3. notarytool keychain profile: NOTARIZE_KEYCHAIN_PROFILE
#
# Input tree layout (matches JUCE CMake):
#   Plugin/Builds/CMake-macOS/WFSPlugin<Name>_artefacts/Release/<Format>/...
#   Plugin/Builds/CMake-macOS/Release/libWFS-DIY-PluginBridge.dylib

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLUGIN_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PLUGIN_DIR/Builds/CMake-macOS"
OUTPUT_DIR="$SCRIPT_DIR/Output"
VERSION="${WFS_PLUGIN_VERSION:-0.0.3}"

mkdir -p "$OUTPUT_DIR"

STAGE_VST3="$OUTPUT_DIR/stage-vst3"
STAGE_AU="$OUTPUT_DIR/stage-au"
rm -rf "$STAGE_VST3" "$STAGE_AU"
mkdir -p "$STAGE_VST3" "$STAGE_AU"

copy_artefact() {
    local target="$1"
    local prod_name="$2"
    local vst3_src="$BUILD_DIR/${target}_artefacts/Release/VST3/${prod_name}.vst3"
    local au_src="$BUILD_DIR/${target}_artefacts/Release/AU/${prod_name}.component"
    [ -d "$vst3_src" ] && cp -R "$vst3_src" "$STAGE_VST3/" || echo "Skip VST3: $vst3_src"
    [ -d "$au_src"   ] && cp -R "$au_src"   "$STAGE_AU/"   || echo "Skip AU:   $au_src"
}

copy_artefact "WFSPluginMaster"    "WFS-DIY Master"
copy_artefact "WFSPluginTrackCart" "WFS-DIY Track - Cartesian"
copy_artefact "WFSPluginTrackCyl"  "WFS-DIY Track - Cylindrical"
copy_artefact "WFSPluginTrackSph"  "WFS-DIY Track - Spherical"
copy_artefact "WFSPluginTrackAdmC" "WFS-DIY Track - ADM Cartesian"
copy_artefact "WFSPluginTrackAdmP" "WFS-DIY Track - ADM Polar"

# Bridge library - drop into both staging areas so Master/Tracks find it.
BRIDGE_SRC="$BUILD_DIR/libWFS-DIY-PluginBridge.dylib"
if [ ! -f "$BRIDGE_SRC" ]; then
    BRIDGE_SRC="$BUILD_DIR/Release/libWFS-DIY-PluginBridge.dylib"
fi
if [ -f "$BRIDGE_SRC" ]; then
    mkdir -p "$STAGE_VST3/WFS-DIY"
    cp "$BRIDGE_SRC" "$STAGE_VST3/WFS-DIY/"
    mkdir -p "$STAGE_AU/WFS-DIY"
    cp "$BRIDGE_SRC" "$STAGE_AU/WFS-DIY/"
else
    echo "WARN: bridge library not found, skipping." >&2
fi

if [ -n "${CODESIGN_IDENTITY_APP:-}" ]; then
    echo "Codesigning with: $CODESIGN_IDENTITY_APP"
    find "$STAGE_VST3" "$STAGE_AU" -name "*.vst3" -or -name "*.component" -or -name "*.dylib" 2>/dev/null | while read -r bundle; do
        codesign --force --deep --options runtime --timestamp --sign "$CODESIGN_IDENTITY_APP" "$bundle"
    done
fi

PKG_VST3="$OUTPUT_DIR/wfs-diy-vst3.pkg"
PKG_AU="$OUTPUT_DIR/wfs-diy-au.pkg"
PKG_FINAL="$OUTPUT_DIR/WFS-DIY-Plugins-${VERSION}.pkg"

pkgbuild --root "$STAGE_VST3" \
         --identifier "net.wfsdiy.pkg.vst3" \
         --version "$VERSION" \
         --install-location "/Library/Audio/Plug-Ins/VST3" \
         "$PKG_VST3"

pkgbuild --root "$STAGE_AU" \
         --identifier "net.wfsdiy.pkg.au" \
         --version "$VERSION" \
         --install-location "/Library/Audio/Plug-Ins/Components" \
         "$PKG_AU"

DISTRIBUTION_XML="$OUTPUT_DIR/distribution.xml"
cat > "$DISTRIBUTION_XML" <<XML
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="1">
    <title>WFS-DIY Plugin Suite</title>
    <pkg-ref id="net.wfsdiy.pkg.vst3"/>
    <pkg-ref id="net.wfsdiy.pkg.au"/>
    <options customize="never" require-scripts="false"/>
    <choices-outline>
        <line choice="default">
            <line choice="net.wfsdiy.pkg.vst3"/>
            <line choice="net.wfsdiy.pkg.au"/>
        </line>
    </choices-outline>
    <choice id="default"/>
    <choice id="net.wfsdiy.pkg.vst3" visible="false"><pkg-ref id="net.wfsdiy.pkg.vst3"/></choice>
    <choice id="net.wfsdiy.pkg.au" visible="false"><pkg-ref id="net.wfsdiy.pkg.au"/></choice>
    <pkg-ref id="net.wfsdiy.pkg.vst3" version="$VERSION" onConclusion="none">wfs-diy-vst3.pkg</pkg-ref>
    <pkg-ref id="net.wfsdiy.pkg.au"   version="$VERSION" onConclusion="none">wfs-diy-au.pkg</pkg-ref>
</installer-gui-script>
XML

productbuild --distribution "$DISTRIBUTION_XML" \
             --package-path "$OUTPUT_DIR" \
             "$PKG_FINAL"

if [ -n "${CODESIGN_IDENTITY_INSTALL:-}" ]; then
    echo "Installer signing with: $CODESIGN_IDENTITY_INSTALL"
    productsign --sign "$CODESIGN_IDENTITY_INSTALL" "$PKG_FINAL" "$PKG_FINAL.signed"
    mv "$PKG_FINAL.signed" "$PKG_FINAL"
fi

NOTARY_AUTH=()
if [ -n "${NOTARY_API_KEY:-}" ] && [ -n "${NOTARY_API_KEY_ID:-}" ] && [ -n "${NOTARY_API_ISSUER:-}" ]; then
    NOTARY_AUTH=(--key "$NOTARY_API_KEY" --key-id "$NOTARY_API_KEY_ID" --issuer "$NOTARY_API_ISSUER")
elif [ -n "${NOTARY_APPLE_ID:-}" ] && [ -n "${NOTARY_PASSWORD:-}" ] && [ -n "${NOTARY_TEAM_ID:-}" ]; then
    NOTARY_AUTH=(--apple-id "$NOTARY_APPLE_ID" --password "$NOTARY_PASSWORD" --team-id "$NOTARY_TEAM_ID")
elif [ -n "${NOTARIZE_KEYCHAIN_PROFILE:-}" ]; then
    NOTARY_AUTH=(--keychain-profile "$NOTARIZE_KEYCHAIN_PROFILE")
fi

if [ "${#NOTARY_AUTH[@]}" -gt 0 ]; then
    # Apple rejects unsigned installer packages outright.
    if [ -z "${CODESIGN_IDENTITY_INSTALL:-}" ]; then
        echo "ERROR: notarization requested but CODESIGN_IDENTITY_INSTALL is not set" >&2
        echo "       (an unsigned pkg would be rejected by the notary service)" >&2
        exit 1
    fi
    echo "Submitting for notarization"
    xcrun notarytool submit "$PKG_FINAL" "${NOTARY_AUTH[@]}" --wait
    xcrun stapler staple "$PKG_FINAL"
    xcrun stapler validate "$PKG_FINAL"
fi

echo "Produced: $PKG_FINAL"
