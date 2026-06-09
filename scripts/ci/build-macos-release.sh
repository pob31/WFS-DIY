#!/usr/bin/env bash
#
# Build, sign, notarize, staple and package a distributable macOS release of
# WFS-DIY. Works both locally and in CI (GitHub Actions, macos-latest).
#
# Produces, under dist/:
#   WFS-DIY.app                              signed + stapled app bundle
#   WFS-DIY-v<version>-macos.dmg             signed + notarized + stapled
#   WFS-DIY-v<version>-macos.dmg.sha256      checksum
#
# CRITICAL (JUCE-specific): the JUCE-generated Xcode project injects
# com.apple.security.get-task-allow=true even in Release, which Apple's notary
# service rejects (statusCode 4000). We re-sign the .app with
# Builds/MacOSX/release-entitlements.plist (get-task-allow=false, plus
# audio-input + network) before packaging. Do NOT remove that step.
#
# Usage:
#   scripts/ci/build-macos-release.sh                 # full build + sign + notarize
#   SKIP_BUILD=1    scripts/ci/build-macos-release.sh # reuse existing Release/.app
#   SKIP_NOTARIZE=1 scripts/ci/build-macos-release.sh # build + sign + dmg, no notarize
#
# Notarization credentials (resolved in this order):
#   1. App Store Connect API key (preferred, used by CI):
#        NOTARY_API_KEY (path to .p8) + NOTARY_API_KEY_ID + NOTARY_API_ISSUER
#   2. Apple-ID app-specific password:
#        NOTARY_APPLE_ID + NOTARY_PASSWORD + NOTARY_TEAM_ID
#   3. Local default: notarytool keychain profile $NOTARY_PROFILE (WFS-DIY-notary)
#
# Signing identity: $DEVELOPER_ID, or the sole "Developer ID Application" in the
# keychain (CI imports it via apple-actions/import-codesign-certs).

set -euo pipefail

REPO_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$REPO_DIR"

APP_NAME="WFS-DIY"
PROJECT="Builds/MacOSX/WFS-DIY.xcodeproj"
SCHEME="WFS-DIY - App"
BUILD_DIR="Builds/MacOSX/build/Release"
ENTITLEMENTS="Builds/MacOSX/release-entitlements.plist"
NOTARY_PROFILE="${NOTARY_PROFILE:-WFS-DIY-notary}"

# ── Version (single source of truth: JuceHeader.h) ────────────────────────
VERSION="$(sed -nE 's/.*versionString[[:space:]]*=[[:space:]]*"([^"]+)".*/\1/p' \
            JuceLibraryCode/JuceHeader.h | head -n1)"
[ -n "$VERSION" ] || { echo "error: could not read versionString from JuceHeader.h" >&2; exit 1; }
PKG="${APP_NAME}-v${VERSION}-macos"
echo "==> $APP_NAME v${VERSION}"

# ── Signing identity ──────────────────────────────────────────────────────
if [ -z "${DEVELOPER_ID:-}" ]; then
    DEVELOPER_ID="$(security find-identity -v -p codesigning \
        | sed -nE 's/.*"(Developer ID Application: .*)"/\1/p' | head -n1)"
fi
[ -n "$DEVELOPER_ID" ] || { echo "error: no 'Developer ID Application' identity found" >&2; exit 1; }
echo "    signing identity: $DEVELOPER_ID"

# ── Build ─────────────────────────────────────────────────────────────────
APP="$BUILD_DIR/${APP_NAME}.app"
if [ "${SKIP_BUILD:-0}" != "1" ]; then
    echo "==> Building Release"
    rm -rf "$APP"
    xcodebuild -project "$PROJECT" -scheme "$SCHEME" -configuration Release clean build
fi
[ -d "$APP" ] || { echo "error: app not found at $APP" >&2; exit 1; }

# ── Re-sign with notarization entitlements (the get-task-allow=false fix) ──
echo "==> Re-signing with $ENTITLEMENTS"
codesign --force --options runtime --timestamp \
    --entitlements "$ENTITLEMENTS" \
    --sign "$DEVELOPER_ID" "$APP"
codesign --verify --strict --verbose=2 "$APP"
# Sanity-check the entitlement that breaks notarization is actually false.
if codesign -d --entitlements - "$APP" 2>/dev/null | grep -A1 get-task-allow | grep -qi true; then
    echo "error: get-task-allow is still true after re-sign — notarization would fail" >&2
    exit 1
fi

# ── Stage + build the DMG (app + docs + /Applications drop target) ─────────
mkdir -p dist
DMG="dist/${PKG}.dmg"
echo "==> Creating $DMG"
rm -f "$DMG"
STAGE="$(mktemp -d)/dmg"
mkdir -p "$STAGE"
cp -R "$APP" "$STAGE/"
cp LICENSE "$STAGE/LICENSE"
cp README.md "$STAGE/README.txt"
cp THIRD_PARTY_NOTICES.md "$STAGE/THIRD_PARTY_NOTICES.txt"
ln -s /Applications "$STAGE/Applications"

# hdiutil intermittently fails ("Resource busy") on CI — a stale same-name
# volume still attached, or Spotlight touching the fresh tree. Detach + retry.
make_dmg() { hdiutil create -volname "$APP_NAME" -srcfolder "$STAGE" -ov -format UDZO "$DMG"; }
attempt=0
until make_dmg >/dev/null 2>&1; do
    attempt=$((attempt + 1))
    if [ "$attempt" -ge 5 ]; then make_dmg >&2; exit 1; fi
    echo "    hdiutil busy — detach + retry $attempt/5"
    hdiutil detach "/Volumes/$APP_NAME" >/dev/null 2>&1 || true
    sleep 5
done
rm -rf "$(dirname "$STAGE")"

echo "==> Signing DMG"
codesign --force --timestamp --sign "$DEVELOPER_ID" "$DMG"

# ── Notarize + staple ─────────────────────────────────────────────────────
if [ -n "${NOTARY_API_KEY:-}" ] && [ -n "${NOTARY_API_KEY_ID:-}" ] && [ -n "${NOTARY_API_ISSUER:-}" ]; then
    NOTARY_AUTH=(--key "$NOTARY_API_KEY" --key-id "$NOTARY_API_KEY_ID" --issuer "$NOTARY_API_ISSUER")
elif [ -n "${NOTARY_APPLE_ID:-}" ] && [ -n "${NOTARY_PASSWORD:-}" ] && [ -n "${NOTARY_TEAM_ID:-}" ]; then
    NOTARY_AUTH=(--apple-id "$NOTARY_APPLE_ID" --password "$NOTARY_PASSWORD" --team-id "$NOTARY_TEAM_ID")
else
    NOTARY_AUTH=(--keychain-profile "$NOTARY_PROFILE")
fi

if [ "${SKIP_NOTARIZE:-0}" != "1" ]; then
    echo "==> Submitting to Apple notary service"
    xcrun notarytool submit "$DMG" "${NOTARY_AUTH[@]}" --wait
    echo "==> Stapling"
    xcrun stapler staple "$DMG"
    xcrun stapler staple "$APP" || true
    spctl -a -t open --context context:primary-signature -vvv "$DMG" || true
    stapler validate "$DMG"
fi

# ── Checksum + final copy ─────────────────────────────────────────────────
cp -R "$APP" "dist/${APP_NAME}.app"
shasum -a 256 "$DMG" | awk '{print $1}' > "${DMG}.sha256"
echo "==> Done: $DMG"
ls -lh "$DMG" "${DMG}.sha256"
