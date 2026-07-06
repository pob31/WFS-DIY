# Controllers Extraction — spatcore/controllers (Phase 5a)

Status: implemented on branch `spatcore/phase-5a-controllers` (commits `1bb9250`,
`5510a2c`), pushed, **merge blocked on a user hardware check** (no automated gate
exercises HID devices). This doc records the classification and decisions so the
reasoning survives independent of the branch/PR.

## Scope and motivation

The controller device layer (SpaceMouse position control, StreamDeck+/Xencelabs
Quick Keys dials & buttons, ROLI Lightpad, touchscreen input) was requested for
extraction so the two future apps (XOA, Tight-WFS) inherit hardware controller
support rather than reimplementing HID/BLOCKS protocol handling from scratch.

Source: `Source/Controllers/{DialsAndButtons,PositionControl,Sampler,Touch}/`
(32 files). Target: `spatcore/controllers/` (21 files moved).

## Classification

| File | Class | Disposition |
|---|---|---|
| PositionControl/ControllerEvent.h | DEVICE | moved |
| PositionControl/ControllerDevice.h | DEVICE (thread+timer+hotplug base) | moved |
| PositionControl/ControllerMapping.h | DEVICE (generic action-string mapping, no app types) | moved |
| PositionControl/SpaceMouseDevice.h | DEVICE (hidapi 6DOF + 3DxWare conflict utils) | moved → `spacemouse/` |
| PositionControl/ControllerManager.h | APP BINDING | **stays** — tab-aware routing (Map/Clusters/Inputs indices), cluster-vs-input semantics; `Callbacks` struct is already the seam at MainComponent |
| DialsAndButtons/StreamDeckDevice.h | DEVICE (HID protocol, JPEG framebuffer packing) | moved → `streamdeck/` |
| DialsAndButtons/StreamDeckPage.h | DEVICE (param-system-agnostic binding model) | moved → `streamdeck/` |
| DialsAndButtons/StreamDeckRenderer.h | DEVICE (page→button/LCD rendering) | moved → `streamdeck/` |
| DialsAndButtons/StreamDeckManager.h | MIXED | moved with seams (brightness getter injected) |
| DialsAndButtons/XencelabsDevice.h | DEVICE (HID protocol, OLED/LED/wheel) | moved → `xencelabs/` |
| DialsAndButtons/QuickKeysPage.h | DEVICE (binding model, reuses DialBinding) | moved → `xencelabs/` |
| DialsAndButtons/QuickKeysManager.h | MIXED (only OriginTag) | moved, requalified |
| DialsAndButtons/pages/*.h (9 files) | APP BINDING (bind WFSValueTreeState/WFSParameterIDs/Defaults + LocalizationManager; some pull GradientMapEditor/PatchMatrixComponent/TestSignalGenerator/CoordinateConverter/AppSettings) | **stay** |
| Sampler/LightpadTypes.h | DEVICE (zone encoding, colours, pixel font) | moved → `lightpad/` |
| Sampler/LightpadDevice.h | DEVICE (BLOCKS protocol, already module-clean) | moved → `lightpad/` |
| Sampler/LightpadManager.h | MIXED | moved with seams (dead WfsParameters& removed, sensitivity ctor arg) |
| Touch/TouchDeviceMapping.h | DEVICE | moved → `touch/` |
| Touch/TouchManager.h | DEVICE (platform switcher) | moved → `touch/` |
| Touch/TouchManagerStub.h | DEVICE | moved → `touch/` |
| Touch/Linux/Evdev{Device,Manager}.{h,cpp} | DEVICE (one app-branding string, seamed) | moved → `touch/linux/` |

**Held-back rationale**: `ControllerManager` and the 9 page bindings are real WFS
product policy (which tab is active, which parameter a dial writes, geometry
conversions), not accidental coupling — extracting them would mean inventing a
generic parameter-binding API under time pressure. Left as the documented
future-work seam (`Callbacks` struct at the MainComponent boundary already exists).

## Namespace

Single `namespace spatcore::controllers` (subdirs are organizational only,
matching the flat `spatcore::gpu` precedent). Every moved header carries
extraction-compat aliases at the bottom (the Phase-1 pattern): global `using`s
for previously-global types, `namespace X = ...` aliases for
`ControllerActions`/`LightpadColours`/`LightpadPixelFont`, and
`namespace WFSTouch { using ... }` re-export blocks (the MCPCompat pattern) — zero
app call-site churn.

## Build / dependency decisions

- New **`spatcore-controllers`** CMake STATIC target behind
  `option(SPATCORE_CONTROLLERS ON)` so headless consumers (offline-render, a
  future server-mode app) can drop HID/BLOCKS entirely.
- **hidapi**: consumer-provides contract mirroring `juce_simpleweb` — guarded
  `FATAL_ERROR` on a missing `SPATCORE_HIDAPI_INCLUDE_DIR`, **headers only**; the
  `hid_*` implementations stay app-side (`ThirdParty/hidapi/HidApiPlatform.c`).
  This works because every HID call sits in implicitly-inline member functions
  never odr-used by the spatcore compile-check TU.
- **roli_blocks_basics** (Lightpad/BLOCKS): consumer `juce_add_module()` contract,
  same shape.
- Linux adds `pkg-config libudev` (mirrors the app's `linuxExtraPkgConfig`).
- One real bug fixed in passing: the two HID headers include raw `<windows.h>`,
  which poisons `min`/`max` for any JUCE header parsed after them in a
  self-sufficient TU — added a guarded `NOMINMAX` (no-op in the app build, where
  JUCE's windows.h always came first anyway).
- `tools/validation/spatcore_dep_lint.py` gained `AppSettings` in its forbidden
  list — the one real app-coupling this phase severed.

## Seams cut (mechanical only — see Held-back rationale above for what wasn't)

- **OriginTagScope{Hardware}** in all three managers: requalified directly to
  `spatcore::control::osc::` (the `WFSNetwork::` spelling was already a
  re-export of that exact type; core→core dependency, header-inline, no link
  cost).
- **StreamDeckManager**: `AppSettings::getStreamDeckBrightness()` →
  `std::function<int()> getConnectBrightness` (unset → 100, the prior default);
  MainComponent binds it.
- **LightpadManager**: dead `WfsParameters&` member + ctor arg deleted (stored,
  never read); initial sensitivity moved to a ctor arg, MainComponent passes
  `WFSParameterDefaults::lightpadSensitivityDefault`.
- **EvdevTouchManager**: hard-coded `"WFS-DIY"` PropertiesFile identity → ctor
  arg (the stub mirrors the signature); MainComponent passes `"WFS-DIY"` so the
  on-disk settings path is unchanged.

## Validation status

All automated gates green ×2 (per-commit): app build (v145), dependency lint,
kernel hashes (untouched — no DSP code moved), spatcore standalone build + tests,
all three control replays (session/OSC/MCP — these launch the real app, so
manager construction with the new wiring executed at runtime, not just compiled),
offline-render CPU 15/15 regression check.

**No automated gate exercises HID hardware.** The merge to `main` is gated on a
physical check:

1. **SpaceMouse** — enumerates on launch; Map tab move/twist/prev-next-buttons;
   hotplug (unplug/replug, ~2 s re-detect); Clusters move/scale.
2. **Stream Deck+** — pages render (buttons + LCD); dial rotate/press/fine-mode;
   **brightness set → unplug/replug must re-apply** (the cut seam — a broken
   binding would silently jump to the 100 default instead).
3. **Xencelabs Quick Keys** — OLED labels, wheel traverse/adjust toggle, LED
   ring modes.
4. **ROLI Lightpad** — zone split/colours; touch moves the input at the
   **same sensitivity as before** (the injected default is 0.05; a wrong wire
   would feel different).
5. **Linux touchscreen box** — builds via LinuxMakefile (evdev paths are
   compile-only-verified on Windows); mappings **persist across restart**
   (same `~/.config/WFS-DIY` file — the ctor-arg seam must not have moved it).
6. Cross-check: one dial write shows `Hardware` origin in the network log /
   change records (confirms the OriginTag requalification).

## Relationship to the rest of the extraction

Follows the same conventions as Phases 1–4 (two-commit discipline: verbatim move
→ gate → seams → gate; compat-alias namespacing; consumer-provides for
GPL/vendor deps). Sequenced as **Phase 5a**, ahead of the Phase 5 engine seams
(RtSnapshot adoption, LiveSourceTamer/AutomOtion) and the Phase 6 repo split —
agreed order: controllers → Phase 5 → split (JOINT session).
