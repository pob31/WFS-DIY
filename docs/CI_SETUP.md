# CI / Release automation (GitHub Actions)

Two workflows under `.github/workflows/`:

- **`ci.yml`** — on every push / PR to `main`: a Debug build on macOS, Windows and
  Linux so a broken build is caught immediately. No secrets, no packaging.
- **`release.yml`** — when a GitHub **Release is published** (or via manual
  *Run workflow* against an existing tag): builds Release on all three OSes,
  **signs + notarizes + staples the macOS DMG**, and attaches every artifact to
  the Release. Windows ships as an unsigned Inno Setup `.exe` installer, Linux
  as a `.tar.gz`.

The macOS build/sign/notarize/staple logic lives in
[`scripts/ci/build-macos-release.sh`](../scripts/ci/build-macos-release.sh) and runs
the same locally (it falls back to the `WFS-DIY-notary` keychain profile when the
CI env vars are absent).

> JUCE-specific gotcha (already handled by the script): the generated Xcode
> project injects `com.apple.security.get-task-allow=true` even in Release, which
> Apple's notary rejects (statusCode 4000). The script re-signs the `.app` with
> `Builds/MacOSX/release-entitlements.plist` (which sets it `false`) before
> packaging. Don't remove that step.

---

## One-time setup: secrets

Create a protected environment named **`release`** (repo → Settings →
Environments → New environment → `release`; optionally add required reviewers),
then add these **5 secrets** to that environment:

| Secret | What it is |
|---|---|
| `MACOS_CERT_P12_BASE64` | Your "Developer ID Application" cert **+ private key**, exported as `.p12`, base64-encoded |
| `MACOS_CERT_PASSWORD` | The password you set when exporting the `.p12` |
| `NOTARY_API_KEY_BASE64` | App Store Connect API key (`AuthKey_XXXX.p8`), base64-encoded |
| `NOTARY_API_KEY_ID` | The key's **Key ID** (e.g. `ABCD1234EF`) |
| `NOTARY_API_ISSUER` | The **Issuer ID** (a UUID, shown on the API-keys page) |

> The App Store Connect API key is **team-wide** — the exact one created for
> S21_HiJack works here too. Reuse those three notary secret values.

### Generating the certificate `.p12`
1. Keychain Access → **login** keychain → Certificates.
2. Right-click **"Developer ID Application: Pierre-Olivier Boulant (TVYU3CS2N7)"**,
   expand it, select **both** the certificate **and** its private key → **Export 2 items…** → `.p12`, set a password.
3. Encode for the secret:
   ```bash
   base64 -i DeveloperID.p12 | pbcopy   # paste into MACOS_CERT_P12_BASE64
   ```

### Generating the App Store Connect API key (skip if reusing S21's)
1. https://appstoreconnect.apple.com → **Users and Access → Integrations → App Store Connect API**.
2. Generate a key with **Developer** access → download `AuthKey_XXXX.p8` (downloadable **once**).
3. Note the **Key ID** and **Issuer ID** on that page.
   ```bash
   base64 -i AuthKey_XXXX.p8 | pbcopy   # paste into NOTARY_API_KEY_BASE64
   ```

---

## Cutting a release

1. Bump the version in the `.jucer` (`version=` attribute), then run
   `Projucer --resave WFS-DIY.jucer` to propagate it into JuceHeader.h /
   Info-App.plist / pbxproj / vcxproj / resources.rc / LinuxMakefile — don't
   hand-edit the generated files individually, they'll drift.
2. Commit + push to `main`.
3. Create a GitHub **Release** with tag **`v<version>`** (e.g. `v1.0.0beta26`) and publish it.
   - `verify-version` fails fast if the tag ≠ `versionString` in `JuceHeader.h`.
4. The three jobs build and attach: signed/notarized **DMG**, Windows Inno Setup
   **.exe** installer, Linux **tar.gz**.

Manual re-run against an existing tag: Actions → **Release** → *Run workflow* → enter the tag.

---

## Notes / known iteration points
- **JUCE and spatcore are submodules** — checkout uses `submodules: recursive` in
  both workflows (JUCE is the slow one; spatcore is small). spatcore is public, so
  the default `GITHUB_TOKEN` fetches it fine. A spatcore bump is a normal PR via
  `tools/bump-spatcore.ps1` + the gate ritual it prints.
- **`tools/windows/prebuilt/wfs_hip.dll` is a committed binary** that release CI
  bundles (the runner has no HIP SDK). It must be manually rebuilt + recommitted
  whenever `spatcore/gpu` HIP sources change — the bump-spatcore ritual reminds
  you; provenance lives in `tools/windows/prebuilt/README.md`.
- **Windows is unsigned** by design (no code-signing cert). The installer will show a SmartScreen prompt on first run.
- **Linux HIP is not built in CI.** The Linux job installs the CUDA toolkit (so
  `libwfs_cuda.so` builds and bundles its runtime closure via
  `tools/linux/build-app-tarball.sh`'s `BUNDLE_GPU=auto`), but there's no
  equivalent ROCm install step — a full ROCm install is multi-GB and slow/flaky
  on a shared runner, and there's no small prebuilt fallback like Windows' committed
  `tools/windows/prebuilt/wfs_hip.dll` (the Linux HIP runtime closure is far
  larger than one DLL). The Linux release tarball therefore ships without
  `libwfs_hip.so` — AMD users on Linux fall back to CPU until this is revisited.
- These workflows are a first cut; the per-OS **packaging paths** (Windows `x64/Release/App`, Linux `build/WFS-DIY`) may need a tweak after the first real run — check the Actions logs.
- **Local macOS release** (no CI): `scripts/ci/build-macos-release.sh` uses the `WFS-DIY-notary` keychain profile; `SKIP_NOTARIZE=1` to build+sign+DMG only.
