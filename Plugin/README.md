# WFS-DIY Plugin Suite

A DAW-native controller plugin suite for the [WFS-DIY](https://wfs-diy.net) spatial audio app. Complements the existing QLab / Android Remote / hardware-controller paths with DAW-based curve drawing and automation recording.

See [docs/PRD.md](docs/PRD.md) for the full product requirements document.

## Status

**Phase 0 — Foundation scaffold.** Subproject compiles; six plugin targets build; one variant (Cartesian Track) has the six non-position parameters wired up. Position parameters and full network round-trip implementation come in Phase 1. See the PRD for phase definitions.

## Contents

- **WFS-DIY Master** — one instance per DAW project; owns all network I/O to the WFS-DIY app.
- **WFS-DIY Track - Cartesian** — native WFS-DIY OSC, X/Y/Z in meters.
- **WFS-DIY Track - Cylindrical** — native, R/Theta/Z.
- **WFS-DIY Track - Spherical** — native, R/Theta/Phi.
- **WFS-DIY Track - ADM Cartesian** — ADM-OSC normalized.
- **WFS-DIY Track - ADM Polar** — ADM-OSC normalized polar.

Four of the five Track variants are stubs at Phase 0; they compile, install, and load, but changing their controls has no effect until Phase 2/3/4.

## Formats

| Platform | Formats                                  | Architectures              |
|----------|------------------------------------------|----------------------------|
| macOS    | VST3, AU, Standalone                     | Universal (arm64 + x86_64) |
| Windows  | VST3, Standalone                         | x64                        |

No AAX.

## Building

Plugin is its own CMake subproject — independent of the main app's Projucer flow.

Requirements:
- CMake 3.22+
- A C++17 toolchain (MSVC 2022, Xcode 14+, or Clang/GCC on Linux)
- JUCE 8.0.12 and `juce_simpleweb` pulled in via the repo's `ThirdParty/` (git submodules, see main repo README)
- Windows: [Inno Setup 6](https://jrsoftware.org/isdl.php) to build the installer
- macOS: standard command-line tools for `pkgbuild`/`productbuild`

### Windows

From the repo root, in PowerShell:

    pwsh Plugin/Scripts/build-all.ps1

Produces `Plugin/Installer/Output/WFS-DIY-Plugins-Setup-0.0.1.exe`. Run it with admin privileges to install VST3s to `C:\Program Files\Common Files\VST3\`.

### macOS

    Plugin/Scripts/build-all.sh

Produces `Plugin/Installer/Output/WFS-DIY-Plugins-0.0.1.pkg`. Double-click to install:
- VST3s → `/Library/Audio/Plug-Ins/VST3/`
- AUs → `/Library/Audio/Plug-Ins/Components/`

For signed/notarized macOS builds, set these env vars before running:

    export CODESIGN_IDENTITY_APP="Developer ID Application: Your Name (TEAMID)"
    export CODESIGN_IDENTITY_INSTALL="Developer ID Installer: Your Name (TEAMID)"
    export NOTARIZE_KEYCHAIN_PROFILE="your-keychain-profile"

### Skip the installer (just the binaries)

    pwsh Plugin/Scripts/build-all.ps1 -SkipInstaller
    Plugin/Scripts/build-all.sh --skip-installer

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

## Windows SmartScreen

Phase 0 installers are unsigned. Windows SmartScreen will warn on first run. Click "More info → Run anyway". Code signing is funded through [GitHub Sponsors](https://github.com/sponsors/pob31) and will be enabled once sponsorship covers the certificate.
