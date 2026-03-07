# Beta Distribution Checklist for WFS-DIY

## Context
The project builds successfully on both Windows (VS2022) and macOS (Xcode) as a standalone JUCE app. There is currently zero distribution infrastructure. The goal is to document what's needed to produce shareable beta binaries, not to build automation yet.

## Already done
- [x] Bundle ID updated to `com.wfsdiy.app` (jucer, plist, pbxproj)
- [x] Version set to `0.1.0` (jucer, plist, pbxproj, vcxproj)

## GPU Audio & Platform Requirements
- GPU Audio SDK is **fully disabled at compile time** (all code paths commented out)
- The beta ships CPU-only — no GPU dependency whatsoever
- GPU Audio requires: Windows 11 + macOS 13 (Ventura)
- **Current deployment targets are fine for CPU-only beta:**
  - Windows: 10+ (works)
  - macOS: currently 10.13 in pbxproj (JUCE minimum), consider bumping to 13.0 when GPU Audio is enabled
- When GPU is eventually enabled: runtime check exists (`GpuAudioManager::GetGpuAudio()` returns nullptr if unavailable), app won't crash but **outputs silence instead of falling back to CPU** — needs improvement later

---

## Windows Checklist

### Build
- [ ] Build Release: `cd Builds/VisualStudio2022 && MSBuild WFS-DIY.sln -p:Configuration=Release -p:Platform=x64 -m`
- [ ] Output: `Builds/VisualStudio2022/x64/Release/App/WFS-DIY.exe`

### Package
- [ ] Zip the `.exe` (and any required DLLs/resources alongside it) into `WFS-DIY-v0.1.0-beta1-win64.zip`
- [ ] Verify it runs on a clean machine (no VS installed) — may need VC++ Redistributable

### Code signing (optional for beta)
- Without signing: users get a SmartScreen warning on first run ("Windows protected your PC" → "More info" → "Run anyway")
- To remove this: purchase an EV code signing certificate ($200-400/yr) and sign with `signtool`
- Not necessary for a small technical beta audience

### Installer (optional, later)
- Inno Setup (free) or NSIS can wrap the exe into a proper installer
- Not needed for initial beta — zip is fine

---

## macOS Checklist

### Prerequisites
- [x] Update bundle ID — done (`com.wfsdiy.app`)
- [ ] Consider bumping deployment target from 10.13 to 13.0 (for future GPU Audio compatibility)
- [x] Set version to `0.1.0` — done

### Build
- [ ] Open `Builds/MacOSX/WFS-DIY.xcodeproj` in Xcode
- [ ] Select Release scheme, build for "Any Mac (Apple Silicon, Intel)"
- [ ] Output: `WFS-DIY.app` bundle

### Package
- [ ] Zip the `.app` bundle: `zip -r WFS-DIY-v0.1.0-beta1-mac.zip WFS-DIY.app`
- [ ] Or create a DMG with drag-to-Applications (later polish)

### Unsigned distribution (no Apple Developer account needed)
- macOS Gatekeeper will block unsigned apps
- Testers must do ONE of:
  - Right-click the app → Open → confirm the dialog (first launch only)
  - Or run: `xattr -cr /path/to/WFS-DIY.app` in Terminal before first launch
- Include a short README with these instructions in the zip

### Code signing + notarization (requires $99/yr Apple Developer Program)
- Not required for beta with technical testers
- Required later for broader distribution
- Steps when ready:
  1. Enroll in Apple Developer Program
  2. Create a "Developer ID Application" certificate
  3. Sign: `codesign --deep --force --sign "Developer ID Application: Your Name" WFS-DIY.app`
  4. Notarize: `xcrun notarytool submit WFS-DIY.zip --apple-id X --team-id Y --password Z --wait`
  5. Staple: `xcrun stapler staple WFS-DIY.app`

---

## Both Platforms

### Versioning
- [x] Version `0.1.0` already set in `.jucer`, plist, pbxproj, and vcxproj

### Distribution
- [ ] Host on GitHub Releases (free, supports large binaries, provides download links)
- [ ] Write brief release notes (known issues, setup instructions, feedback channel)
- [ ] Include the unsigned-app workaround instructions for Mac testers

### Testing before sharing
- [ ] Windows: test on a machine without Visual Studio installed
- [ ] macOS: test on a machine where the app was never built (to catch Gatekeeper issues)
- [ ] Verify audio I/O works, Stream Deck connects, OSC communication functions

---

## Summary: Remaining steps to ship a beta

1. ~~Fix the macOS bundle ID~~ — done
2. ~~Set version to 0.1.0~~ — done
3. Build Release on both platforms
4. Zip each binary
5. Write a short README with the macOS `xattr -cr` workaround
6. Upload to GitHub Releases (or any file sharing)
