# WFS-DIY VST Plugin Suite — Product Requirements Document

**Status:** Design frozen. Ready for phased implementation.
**Target location in repo:** `Plugin/docs/PRD.md`
**Companion document:** `docs/WFS-UI_input.csv` (authoritative OSC parameter reference)
**Implementation tool:** Claude Code (full repo context)

---

## 1. Purpose and positioning

The WFS-DIY VST plugin suite is a DAW-native controller for the WFS-DIY
application. It complements — does not replace — the existing QLab
integration, which uses meta-OSC commands from WFS-DIY to populate
QLab with network cues for cue-based workflows.

| User population  | Tool             | Workflow                                  |
| ---------------- | ---------------- | ----------------------------------------- |
| QLab users       | Existing meta-OSC| WFS-DIY populates QLab with network cues  |
| **DAW users**    | **This project** | **Draw and record automation curves**     |
| Live performers  | Android Remote   | Touch-surface control                     |
| Hardware ctlrs   | OSC/MIDI mapping | Direct OSC into WFS-DIY                   |

The VST is the curve-drawing surface for DAW-centric users
(Reaper, Ableton, Logic, Cubase, Studio One, Bitwig).

A Max4Live equivalent exists separately and is maintained by the
author with a straightforward adaptation of the legacy WFS-DIY Max
version to the new OSC paths.

---

## 2. Final-state binaries (after all phases)

Six plugin binaries built from one JUCE project:

1. **WFS-DIY Master** — one instance per DAW project. Owns all network I/O.
2. **WFS-DIY Track (Cartesian)** — native WFS-DIY OSC, X/Y/Z in meters.
3. **WFS-DIY Track (Cylindrical)** — native WFS-DIY OSC, R/Theta/Z.
4. **WFS-DIY Track (Spherical)** — native WFS-DIY OSC, R/Theta/Phi.
5. **WFS-DIY Track (ADM Cartesian)** — ADM-OSC normalized -1..+1.
6. **WFS-DIY Track (ADM Polar)** — ADM-OSC normalized polar.

### Plugin formats

| Platform | Formats                                 | Architectures              |
| -------- | --------------------------------------- | -------------------------- |
| macOS    | VST3, AU (`.component`), Standalone     | Universal (arm64 + x86_64) |
| Windows  | VST3, Standalone                        | x64                        |

No AAX (Pro Tools) — out of scope. Logic Pro runs AUs in its
shared `AUHostingService` process; in-process singleton visibility
between Master and Track plugins is preserved.

---

## 3. Architecture overview

### Single connection through Master

Master owns all network I/O. Track plugins have no network code.

```
┌───────────────────────────────┐
│ WFS-DIY Master (1 inst.)      │
│  - UDP OSC socket             │
│  - OSC Query HTTP/WebSocket   │
│  - Connection state           │
│  - Discovery + subscription   │
│  - Dispatch hub               │
│  - Rate limiting              │
└──┬────────────────┬───────────┘
   │  in-process    │
   │  shared        │
   │  singleton     │
┌──▼────────┐ ┌─────▼────────┐ ┌───────────────┐
│ Track     │ │ Track        │ │ Track         │
│ (Cart) #1 │ │ (Cyl) #2     │ │ (ADM-Pol) #3  │
│ Input 1   │ │ Input 1      │ │ Input 2       │
└───────────┘ └──────────────┘ └───────────────┘
```

### Coordinate-addressing abstraction

The Track plugin processor is **variant-pluggable**: each variant is
a build-time configuration that defines:

- Three position parameter names, ranges, units
- The OSC paths to send/receive for each position parameter
- Whether values are normalized (ADM-OSC) or in real units (native)

This abstraction is in place from Phase 0 even though only the
Cartesian variant ships in Phase 1. Adding a variant in later phases
is a configuration change, not a code refactor.

### Shared-singleton communication

Master and Track plugins communicate through a C++ singleton within
the host process. On macOS/Logic this is `AUHostingService`;
elsewhere it's the DAW's main process. Pattern works in Reaper,
Ableton, Logic, Studio One, Cubase, Bitwig.

**Track plugin lifecycle:**

- On `prepareToPlay()`: find Master singleton; register with
  `(input ID, variant identity)` tuple.
- On parameter change: post change to Master's outgoing queue.
- On destruction: deregister from Master.
- On Master absence: show "No WFS-DIY Master found" in plugin UI.

**Master plugin responsibilities:**

- Maintains connection to WFS-DIY app (host, port).
- Performs OSC Query discovery on connection.
- Maintains Track registry: `InputID → [Track*]`.
- Subscribes to parameters for registered inputs.
- Rate-limits outgoing OSC to 50 Hz with change detection.
- Dispatches incoming subscription updates to all matching Tracks.

### Rate limiting and interpolation

- Outgoing OSC rate-limited to 50 Hz (20 ms windows).
- Change-detection epsilon = 0.0001 × (max − min) per parameter.
- Coalesce within window: only final value sent.
- **No `transition time` field in continuous streams** — WFS-DIY's
  internal 50 Hz interpolator handles smoothing. Transition-time is
  reserved for QLab single-shot cues, not this project.
- Bypassed plugins do not send OSC.

### Feedback loop handling

WFS-DIY's server already implements origin suppression: a client
that writes a parameter does not receive that change back via
subscription. Validated in production with Android Remote and
in-app interactions.

The plugin does **not** need client-side feedback-loop mitigation.
Subscription updates received by the plugin genuinely originated
elsewhere and should be reflected in parameter state. When DAW is
in Touch/Write mode, this records automation — a core feature.

To prevent the plugin's reception from re-echoing outward: when
applying an inbound update, set `isApplyingRemoteChange = true`
around the parameter write. The outbound listener checks this flag
and skips sending. Standard JUCE `AudioProcessorValueTreeState`
pattern.

### Multi-variant stacking

A user wanting different coordinate systems for different timeline
regions stacks multiple Track variants on the same DAW track,
targeting the same Input ID, with bypass automation controlling
which is active when. WFS-DIY accepts any coordinate system on
input and stores internally as Cartesian, so non-overlapping writes
from different variants cause no conflict. This is the supported
mechanism for mid-timeline coordinate system changes.

---

## 4. Track plugin parameters

All variants share the same six non-positional parameters. Position
parameters vary per variant (defined in each phase's section).

### Shared parameters (identical across all variants)

| # | Name         | OSC Path                      | Type  | Range       | Unit | Automatable |
|---|--------------|-------------------------------|-------|-------------|------|-------------|
| 1 | Input ID     | (determines `<ID>` in paths)  | INT   | 1..64       | —    | No          |
| 2 | Attenuation  | `/wfs/input/attenuation`      | FLOAT | -92.0..0.0  | dB   | Yes         |
| 3 | Position 1   | (per variant)                 | FLOAT | (per variant) | (per variant) | Yes |
| 4 | Position 2   | (per variant)                 | FLOAT | (per variant) | (per variant) | Yes |
| 5 | Position 3   | (per variant)                 | FLOAT | (per variant) | (per variant) | Yes |
| 6 | Directivity  | `/wfs/input/directivity`      | INT   | 2..360      | °    | Yes         |
| 7 | Rotation     | `/wfs/input/rotation`         | INT   | -179..180   | °    | Yes         |
| 8 | Tilt         | `/wfs/input/tilt`             | INT   | -90..90     | °    | Yes         |
| 9 | HF Shelf     | `/wfs/input/HFshelf`          | FLOAT | -24.0..0.0  | dB   | Yes         |
|10 | LFO Active   | `/wfs/input/LFOactive`        | BOOL  | 0..1        | —    | Yes         |

Nine DAW-automatable parameters. Input ID is plugin-state (chosen
per instance, saved with project).

### Read-only display state (subscribed via OSC Query)

- **Input Name** — `/wfs/input/name` — shown in plugin title bar
- **Coordinate Mode** — `/wfs/input/coordinateMode` — informational
  only; does not affect plugin behavior

---

## 5. Phased implementation plan

Each phase ends with an installable build and explicit verification
gates. Subsequent phases do not begin until the current phase's
gates pass.

---

### Phase 0 — Foundation and installer plumbing

**Goal:** prove the plumbing works end-to-end with a single empty
plugin pair (Master + Cartesian Track shell).

**Deliverables:**

- JUCE project at `Plugin/` with all six plugin targets defined in
  the build system. Five Track variants are stubs at this stage —
  only Cartesian Track is wired up, and only with the six shared
  non-position parameters (Position 1/2/3 declared but inert).
- `Shared/` folder containing OSC transport, OSC Query client (using
  vendored `juce_simpleweb`), shared singleton, rate limiter.
- Master plugin with full functionality: connection config UI,
  OSC Query discovery, subscription management, rate-limited send,
  dispatch hub, connection status indicator.
- Coordinate-addressing abstraction in place — defined as a config
  struct or template parameter that the future variants will fill in.
- Inno Setup installer for Windows (matching the main app's pattern).
- macOS `.pkg` installer with codesign + notarization (Developer ID
  Application for plugin bundles, Developer ID Installer for the pkg).
- Build script (CMake target or shell script) that produces both
  installers from a clean checkout.

**Verification gates (all must pass):**

1. JUCE project builds cleanly on macOS and Windows; all six plugin
   targets compile (even if four are stubs).
2. Both installers complete cleanly. Plugins install to standard
   locations (`{commoncf64}\VST3\` on Windows; `~/Library/Audio/...`
   and `/Library/Audio/...` on macOS).
3. Plugins are recognized by Reaper, Logic (AU), and Ableton Live
   after installation.
4. Master ↔ Cartesian Track singleton communication works in Reaper.
   Master registers; Track finds Master; Track posts a test
   message; Master receives.
5. Master ↔ Cartesian Track singleton communication works under
   Logic AU sandboxing. Verify single `AUHostingService` process in
   Activity Monitor.
6. Master performs OSC Query handshake with WFS-DIY app, retrieves
   parameter tree, confirms expected paths exist.
7. Round-trip parameter change for Attenuation (a non-position
   parameter): Track sends → Master sends OSC → WFS-DIY updates →
   no feedback echo back to Master.
8. Reverse direction: Attenuation changed in WFS-DIY UI → Master
   receives subscription update → Track parameter updates → DAW in
   Write mode records automation.

**End state:** installable build with one functional Track variant
(Cartesian, six parameters working). Foundation proven.

**Internal milestone, not yet a public release.**

---

### Phase 1 — Cartesian variant complete + UI refinement

**Goal:** ship a Cartesian-only beta. Refine UI before duplicating
patterns to other variants.

**Deliverables:**

- Cartesian Track variant: position parameters fully wired.
  - Position X → `/wfs/input/positionX`, FLOAT, 0.0..50.0 m
  - Position Y → `/wfs/input/positionY`, FLOAT, 0.0..50.0 m
  - Position Z → `/wfs/input/positionZ`, FLOAT, 0.0..50.0 m
- Plugin editor UI: clean layout, parameter grouping, value
  formatting with units, Input ID selector prominent.
- UI iteration round: gather feedback (self or trusted testers),
  refine spacing, controls, behavior. This is the prototype that
  shakes out UX issues before they're duplicated.
- Master UI: connection config, status indicator, list of
  registered Track instances (debugging aid).
- README with installation notes, basic usage, known limitations.

**Verification gates:**

1. All nine automatable parameters round-trip correctly between
   plugin and WFS-DIY.
2. DAW automation recording works for all parameters in Reaper,
   Logic, Ableton.
3. Plugin can be saved and reloaded with a DAW project; state
   restoration is correct (Input ID, all parameter values).
4. Multi-instance: 8+ Cartesian Track plugins on different tracks,
   targeting different Input IDs, all functioning concurrently
   without interference.
5. Bypass behavior: bypassed plugin stops sending OSC; unbypassing
   resumes correctly.
6. UI feels right. Subjective gate, but explicit.

**End state:** Cartesian-only beta release. Versioned (e.g.
`WFS-DIY Plugins Beta 0.1.0`), installer signed, GitHub release page.

**First public release point.** Optional — can defer to Phase 2 if
preferred.

---

### Phase 2 — Cylindrical variant + multi-variant stacking proof + Polar UI

**Goal:** add Cylindrical variant, prove the multi-variant stacking
pattern works, design polar-coordinate UI components.

**Deliverables:**

- Cylindrical Track variant: position parameters wired.
  - Position R → `/wfs/input/positionR`, FLOAT, 0.0..50.0 m
  - Position Theta → `/wfs/input/positionTheta`, FLOAT, -180..180°
  - Position Z → `/wfs/input/positionZ`, FLOAT, 0.0..50.0 m
- Polar UI components: circular position display, azimuth dial,
  distance/radius display, designed for reuse in Phase 3 and 4.
- Plugin title bar reflects coordinate variant clearly so stacked
  variants on one track are visually distinguishable.

**Verification gates:**

1. All gates from Phase 1 pass for the Cylindrical variant.
2. Stack a Cartesian and a Cylindrical Track plugin on the same DAW
   track, both targeting the same Input ID. Automate bypass: A
   active 0–4 bars, B active 4–8 bars, A active 8–12 bars.
3. At each bypass transition, source position transitions cleanly
   in WFS-DIY (no glitches, no OSC storms, no parameter conflicts).
4. Both plugins' automation is preserved correctly in the DAW
   project across save/reload.
5. Coordinate-addressing abstraction confirmed clean: adding the
   Cylindrical variant required only configuration, not new code in
   the OSC transport, rate limiter, or singleton layers.

**End state:** Cartesian + Cylindrical beta release.
`WFS-DIY Plugins Beta 0.2.0`. Polar UI vocabulary established.

---

### Phase 3 — Spherical variant

**Goal:** complete the native variants. Validate that variant
addition is now trivial.

**Deliverables:**

- Spherical Track variant: position parameters wired.
  - Position R → `/wfs/input/positionRsph`, FLOAT, 0.0..50.0 m
  - Position Theta → `/wfs/input/positionTheta`, FLOAT, -180..180°
  - Position Phi → `/wfs/input/positionPhi`, FLOAT, -90..90°
- Reuses polar UI components from Phase 2; adds elevation display
  element (vertical or pie slice).

**Verification gates:**

1. All gates from Phase 1 pass for the Spherical variant.
2. Adding the Spherical variant required only configuration changes
   — no modifications to shared infrastructure.
3. Three-way stacking test: Cartesian + Cylindrical + Spherical on
   one track, bypass-automated to swap across regions. All work.

**End state:** all three native variants shipping.
`WFS-DIY Plugins Beta 0.3.0`.

---

### Phase 4 — ADM-OSC Cartesian + ADM-OSC Polar variants

**Goal:** complete the suite with venue-portable ADM-OSC variants.

**Background:** WFS-DIY has eight configurable ADM-OSC mapping
presets (Cartesian 1–4, Polar 1–4) accessible in the network tab.
Each input channel can be assigned one of these presets. The preset
defines normalized -1..+1 (or 0..1 for polar distance) input ranges
and their mapping to physical meters, with breakpoints, flips, and
azimuth offsets. The ADM-OSC plugin variants send and receive in
normalized space; the WFS-DIY app applies the per-input mapping
preset to translate to physical position. **The plugin does not
need to know which preset is assigned** — that's an app-side
configuration.

**Deliverables:**

- ADM-OSC Cartesian Track variant.
  - Position X → `/adm/obj/<ID>/x` (or actual ADM-OSC path used by
    WFS-DIY — confirm against existing implementation), FLOAT,
    -1.0..+1.0, normalized
  - Position Y → equivalent, -1.0..+1.0
  - Position Z → equivalent, -1.0..+1.0
  - Non-position parameters (attenuation, directivity, rotation,
    tilt, HF shelf, LFO active) use **native WFS-DIY OSC paths**
    as in other variants. ADM-OSC does not cover these; native
    addressing is appropriate.
- ADM-OSC Polar Track variant.
  - Distance → normalized 0.0..1.0
  - Azimuth → normalized -1.0..+1.0 (or whatever ADM-OSC convention
    WFS-DIY uses)
  - Elevation → normalized -1.0..+1.0
  - Non-position parameters: native, as above.
- README section explaining ADM-OSC variants are venue-portable
  via the WFS-DIY mapping presets, and noting the per-input mapping
  is configured in the app, not the plugin.

**Verification gates:**

1. All gates from Phase 1 pass for both ADM-OSC variants.
2. Venue-portability test: configure two different mapping presets
   in WFS-DIY (e.g., Cartesian 1 = 10m × 10m, Cartesian 2 = 40m ×
   40m). Assign one input to each preset. Run the same DAW
   automation on both — sources move through proportionally
   different physical spaces.
3. Mixed-variant stacking: a native Cartesian and an ADM Cartesian
   plugin on one track, bypass-automated to swap. Both work.
4. Five-way stacking (all variants on one track) — extreme stress
   test, should work even if rarely used in practice.

**End state:** complete suite shipping. Five Track variants + Master.
First non-beta release: `WFS-DIY Plugins 1.0.0`.

---

## 6. OSC Query usage

- **Protocol reference:** Vidvox OSCQuery proposal
  (https://github.com/Vidvox/OSCQueryProposal). No code from OSSIA
  or other reference implementations is used.
- **Transport:** HTTP + WebSocket via vendored `juce_simpleweb`
  module (already in main project, credited in
  `THIRD_PARTY_NOTICES.md`).
- **On Master connection:** GET `/`, walk parameter tree, cache
  structure.
- **On Track plugin registration:** request current values for
  subscribed paths; subscribe (LISTEN) to mapped paths for that
  input.
- **On Track plugin deregistration:** send IGNORE for paths no
  longer referenced.

The plugin is an OSC Query **client only**. No server-side
implementation.

---

## 7. Out of scope (entire project, not just v1)

The following are explicitly **never** in scope, regardless of phase.
Different from "deferred to later phase" — these are intentional
non-goals.

### Set-and-forget parameters (configured in `input.xml` or snapshots)

- `inputMinimalLatency`, `inputHeightFactor`
- `inputOffsetX/Y/Z`, `inputOffsetR/Theta`, `inputOffsetRsph/Phi`
- `inputConstraintX/Y/Z`, `inputFlipX/Y/Z`
- `inputConstraintDistance`, `inputConstraintDistanceMin/Max`
- `inputTrackingActive`, `inputTrackingID`, `inputTrackingSmooth`
- `inputMaxSpeedActive`, `inputMaxSpeed`
- `inputPathModeActive`
- `inputCluster` (cluster assignment is configuration)
- `inputName` (display only; not editable from plugin)
- `inputCoordinateMode` (display only; variants define their own
  coordinate system per instance)

### Duplicates an existing in-app generator

- All AutomOtion parameters (`inputOtomo*`). WFS-DIY has its own
  AutomOtion engine. Triggering it from the DAW adds no value over
  drawing equivalent curves directly on position parameters, and
  QLab's existing meta-OSC integration already covers cue-based
  AutomOtion triggering.

### Configuration or UI-only

- All Live Source Tamer (`inputLS*`)
- All Hackoustics / Floor Reflections (`inputFR*`)
- All Sidelines (`inputSidelines*`)
- All Gradient Map (`gm*`) — visual output concern only
- Detail attenuation parameters (`inputDistanceAttenuation`,
  `inputDistanceRatio`, `inputCommonAtten`, `inputAttenuationLaw`)
- LFO detail (rate, depth, shape, phase, gyrophone) — only
  `inputLFOactive` is exposed as a toggle
- All output-side parameters (`inputMutes`, `inputArrayAtten*`,
  `inputMuteReverbSends`, snapshot ops)

### Functional non-goals

- Pro Tools / AAX format (no plans to add)
- Snapshot recall, global transport — Master v2+ candidates only if
  user demand emerges, not committed
- Automation curve translation between coordinate systems (VST3/AU
  APIs do not expose this; design works around the limitation via
  multi-variant stacking)
- Auto-discovery of input channel additions/removals — manual
  dual-side operation by design

---

## 8. Known limitations (documented, not solved)

1. **Automation rewriting impossible.** VST3/AU APIs don't expose
   automation as mutable. Plugin cannot translate curves between
   coordinate systems. *Mitigation:* multi-variant stacking
   (Phase 2+).

2. **Angle wrap-around in Cylindrical/Spherical Theta.** Linear
   automation interpolation between +170° and −170° takes the long
   way (340°). *Mitigation:* users draw curves explicitly crossing
   0° or use discrete breakpoints. v2+ candidate: continuous-phase
   tracking.

3. **Host plugin sandboxing may isolate Master from Tracks.**
   Reaper "Run as separate process" or Bitwig per-plugin sandboxing
   break the singleton when enabled. *Mitigation:* plugin detects
   "Master not found" and shows clear UI message with fix
   instructions.

4. **AUHostingService process recovery.** Logic may restart its AU
   hosting process. All WFS-DIY plugin instances re-initialize.
   *Handled:* Tracks re-register with Master on every
   `prepareToPlay()`, not just construction.

5. **Adding/removing input channels is dual-side manual.** Adding
   an input in WFS-DIY does not create a Track plugin. Adding a
   Track plugin does not create an input in WFS-DIY. Intentional.

---

## 9. Licensing

- **License:** GPL-3.0, matching the WFS-DIY app.
- **Third-party code reused:** `juce_simpleweb` (Ben Kuperberg,
  GPLv3) and its vendored dependencies (standalone ASIO under Boost
  1.0). Already in `THIRD_PARTY_NOTICES.md` for the app; no changes
  needed for the plugin.
- **Protocol references:** OSCQuery (Vidvox). Specification only,
  no code derivation. OSCQuery entry to be added to
  `THIRD_PARTY_NOTICES.md` (already noted by author as pending).
- **No new third-party dependencies introduced.**

---

## 10. Distribution

- **Separate installer** from the main WFS-DIY app. Typical
  deployment is two machines: WFS processing on one, DAW/QLab on
  the other. Single-machine users install both.
- **Independent versioning** from the main app. Plugin README
  documents minimum compatible WFS-DIY app version. OSC Query
  discovery tolerates forward-compatible parameter additions.
- **macOS:** signed `.pkg` using existing Developer ID
  Application (plugin bundles) and Developer ID Installer (pkg)
  certificates. Notarized with Apple. Each plugin bundle signed
  individually before packaging.
- **Windows:** Inno Setup installer matching the main app's
  pattern. Unsigned for early phases; code signing funded through
  GitHub Sponsors and added when sponsorship covers the
  certificate cost. README includes "Windows installation notes"
  explaining SmartScreen warnings and pointing to sponsorship.
- **Windows VST3 install path:** `{commoncf64}\VST3\` (i.e.
  `C:\Program Files\Common Files\VST3\`). Admin privileges required.
- **Installer dependencies:** VC++ runtime bundled on Windows. No
  external runtime dependencies on macOS.
- **Release labels:** Beta 0.x.0 through Phase 3, 1.0.0 at Phase 4
  completion.

---

## 11. References

- `docs/WFS-UI_input.csv` — authoritative OSC parameter reference
- `THIRD_PARTY_NOTICES.md` — third-party attributions
- `CLAUDE.md` — main WFS-DIY project development notes
- Vidvox OSCQuery proposal:
  https://github.com/Vidvox/OSCQueryProposal
- JUCE framework: https://juce.com

---

## 12. Implementation guidance for Claude Code

This PRD is the contract. Suggested workflow when opening Claude
Code in the WFS-DIY repo:

**Initial session:**

> Read `Plugin/docs/PRD.md` and `docs/WFS-UI_input.csv`. Implement
> Phase 0 only: scaffold the JUCE project at `Plugin/`, define all
> six plugin build targets (with four as stubs), build the Inno
> Setup and macOS pkg installers, and prove the foundation gates
> (sections 5, Phase 0 verification gates 1–8). Reuse
> `juce_simpleweb` from the main project. Stop at Phase 0 gate
> completion for review.

**Subsequent sessions:** one session per phase, instructed to
implement just that phase's deliverables and verify against that
phase's gates. Review and approve between phases.

**Architectural priorities throughout:**

- The coordinate-addressing abstraction must be pluggable by
  configuration, not by code branching. Adding a variant in
  Phases 2–4 should require no changes to the OSC transport,
  rate limiter, singleton, or feedback handling.
- Threading discipline: parameter changes from audio thread to OSC
  send via lock-free FIFO; OSC send and receive on dedicated
  message-thread or network thread. No OSC on the audio thread.
- Tests are not specified in this PRD but Claude Code is encouraged
  to add unit tests for the coordinate-addressing abstraction, the
  rate limiter, and the singleton registration logic.
