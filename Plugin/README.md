# WFS-DIY Plugin Suite

A DAW-native controller plugin suite for the [WFS-DIY](https://wfs-diy.net) spatial audio app. Complements the existing QLab / Android Remote / hardware-controller paths with DAW-based curve drawing and automation recording.

See [docs/PRD.md](docs/PRD.md) for the full product requirements document.

## Status

**Phases 0–4 functionally complete.** All five Track variants plus Master are wired end-to-end for their basic parameters and exchange positions with the WFS-DIY app. PRD gate items that remain are subjective UI polish, real-session multi-variant stacking validation, and release-label bookkeeping — see `docs/PRD.md` for the full phase roadmap.

## Contents

- **WFS-DIY Master** — one instance per DAW project; owns all network I/O to the WFS-DIY app.
- **WFS-DIY Track - Cartesian** — native WFS-DIY OSC, X/Y/Z in meters.
- **WFS-DIY Track - Cylindrical** — native, R/Theta/Z.
- **WFS-DIY Track - Spherical** — native, R/Theta/Phi.
- **WFS-DIY Track - ADM Cartesian** — ADM-OSC normalized.
- **WFS-DIY Track - ADM Polar** — ADM-OSC normalized polar.

## Formats

| Platform | Formats                                  | Architectures              |
|----------|------------------------------------------|----------------------------|
| macOS    | VST3, AU, Standalone                     | Universal (arm64 + x86_64) |
| Windows  | VST3, Standalone                         | x64                        |
| Linux    | VST3, LV2, Standalone                    | x86_64                     |

No AAX. CLAP deferred until JUCE adds native support.

On Windows touchscreens the plugin editors use the OS gesture recognition
that JUCE 9 enables by default (no raw multi-touch opt-in) — the right fit
for knob/slider panels. The WFS-DIY app itself opts back into raw
per-finger touch for its multi-finger surfaces; plugin editors have no
such surfaces.

## Building

Plugin is its own CMake subproject — independent of the main app's Projucer flow.

Requirements:
- CMake 3.22+
- A C++17 toolchain (MSVC 2022, Xcode 14+, or Clang/GCC on Linux)
- JUCE 9.0.0 and `juce_simpleweb` pulled in via the repo's `ThirdParty/` (git submodules, see main repo README)
- Windows: [Inno Setup 6](https://jrsoftware.org/isdl.php) to build the installer
- macOS: standard command-line tools for `pkgbuild`/`productbuild`

### Windows

From the repo root, in PowerShell:

    pwsh Plugin/Scripts/build-all.ps1

Produces `Plugin/Installer/Output/WFS-DIY-Plugins-Setup-0.0.2.exe`. Run it with admin privileges to install VST3s to `C:\Program Files\Common Files\VST3\`.

### macOS

    Plugin/Scripts/build-all.sh

Produces `Plugin/Installer/Output/WFS-DIY-Plugins-0.0.2.pkg`. Double-click to install:
- VST3s → `/Library/Audio/Plug-Ins/VST3/`
- AUs → `/Library/Audio/Plug-Ins/Components/`

For signed/notarized macOS builds, set these env vars before running:

    export CODESIGN_IDENTITY_APP="Developer ID Application: Your Name (TEAMID)"
    export CODESIGN_IDENTITY_INSTALL="Developer ID Installer: Your Name (TEAMID)"
    export NOTARIZE_KEYCHAIN_PROFILE="your-keychain-profile"

### Linux

    Plugin/Scripts/build-all.sh

Produces `Plugin/Installer/Output/WFS-DIY-Plugins-Linux-x86_64-0.0.2.tar.gz` containing all VST3 + LV2 + Standalone bundles plus an `install.sh`. Extract and run:

    tar xzf WFS-DIY-Plugins-Linux-x86_64-0.0.2.tar.gz
    cd WFS-DIY-Plugins-Linux-x86_64-0.0.2
    ./install.sh --user        # ~/.vst3 + ~/.lv2 (no sudo)
    # or ./install.sh --system # /usr/local/lib/{vst3,lv2} (sudo)

### Skip the installer (just the binaries)

    pwsh Plugin/Scripts/build-all.ps1 -SkipInstaller
    Plugin/Scripts/build-all.sh --skip-installer

## CI and releases

`.github/workflows/plugins-ci.yml` build-checks all three OSes on every
push/PR touching `Plugin/**` (or the JUCE/juce_simpleweb third-party dirs)
and uploads unsigned installer artifacts.

`.github/workflows/plugins-release.yml` builds the distributable installers
(signed + notarized `.pkg` on macOS) and attaches them to a GitHub Release.
The plugin suite versions independently from the app — release ritual:

1. Bump the version in **three** places (the release workflow refuses to run
   if they disagree): `project(WFS-DIY-Plugins VERSION x.y.z)` and
   `WFS_PLUGIN_VERSION_STRING` in `Plugin/CMakeLists.txt`, and
   `MyAppVersion` in `Plugin/Installer/WFS-DIY-Plugins.iss`.
2. Tag `plugins-vx.y.z` and publish a GitHub Release for that tag
   (app releases keep their plain `v*` tags and skip this workflow).
   Publish it as NOT the latest release — uncheck "Set as the latest
   release" in the UI or use `gh release create --latest=false` — the
   repo's "latest" is reserved for main-app `v*` releases (the workflow
   also passes `make_latest: false` when attaching assets as a backstop).
3. The workflow attaches `WFS-DIY-Plugins-Setup-x.y.z.exe`,
   `WFS-DIY-Plugins-x.y.z.pkg`, `WFS-DIY-Plugins-Linux-x86_64-x.y.z.tar.gz`
   and `.sha256` sidecars. macOS signing/notary secrets live in the
   protected `WFS-DIY` environment (same as the app release).

## Architecture

All plugins share one code base. Master owns the network connection to the WFS-DIY app (UDP OSC + OSC Query HTTP/WebSocket). Track plugins have no network code — they talk to Master through a process-wide singleton exposed by `WFS-DIY-PluginBridge` (a tiny shared library installed next to the VST3 bundles).

The coordinate-system difference across the five Track variants is a pure compile-time configuration (`wfs::plugin::VariantConfig`) — no branching inside the Track processor or the shared infrastructure.

See the PRD §3 for details.

## Host compatibility

Designed and tested with the following hosts:
- Reaper
- Ableton Live
- Logic Pro (AU)
- Studio One
- Cubase / Nuendo
- Bitwig Studio

Per-plugin sandboxing modes (Reaper "Run as separate process", Bitwig per-plugin sandboxing) break the shared-singleton mechanism. When this happens the plugin UI shows "No WFS-DIY Master found" — disable sandboxing for Master + Tracks on the same project.

## License

GPL-3.0, matching the main WFS-DIY app. See [LICENSE](../LICENSE) and [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md).

## Code signing

- **macOS** — official `.pkg` releases are codesigned (Developer ID Application for the plugin bundles, Developer ID Installer for the installer) and notarized. Gatekeeper opens them without warnings.
- **Windows** — installers are currently **unsigned**. Windows SmartScreen will warn on first run; click "More info → Run anyway". Windows certificate-based code signing isn't planned unless an external sponsor covers the certificate cost — see [GitHub Sponsors](https://github.com/sponsors/pob31).
