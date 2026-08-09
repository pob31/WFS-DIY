# XOA HOA-to-Binaural — Handoff

Audience: the agent (or human) implementing binaural monitoring in XOA, the
Ambisonics sibling app (10th order, 121 SH channels). Written 2026-08 right
after the WFS-DIY binaural renderer + head tracking shipped, so the reuse
boundary is captured while it is fresh. Everything referenced by path lives in
this spatcore checkout unless marked *(WFS-DIY app repo)*.

## 1. The one architectural decision already made

WFS-DIY renders binaural **per source** (N mono sources → direction in head
frame → ITD + HRTF per ear). That was a deliberate WFS choice, and it was
equally deliberate that **XOA should NOT shoehorn HOA into it**: an Ambisonic
scene is already a sound field, and the idiomatic binaural path is

    HOA signals → SH-domain ROTATION (head tracking) → static SH→ear filters → L/R

i.e. head rotation is a (2·order+1)-band block-diagonal matrix multiply in the
SH domain (Wigner-D), and the expensive HRTF machinery collapses into a fixed
bank of per-SH-channel, per-ear FIR filters computed **once at HRTF-load
time** (magnitude-least-squares "MagLS" above ~2 kHz is the current standard;
plain least-squares below). Head motion then costs a rotation matrix update
per block — no per-direction HRIR selection, no crossfading, no per-source
delay lines. Do not import `BinauralEngine`/`StructuralHrtfRenderer`/
`SofaHrtfRenderer` for this; they solve a different problem.

What IS designed for you to take wholesale is everything around the renderer:
head-orientation sources, the webcam tracker plugin, the SOFA pipeline up to
"HRIR set in memory", the RT hand-off primitives, and the conventions.

## 2. Reuse inventory

### Take as-is (spatcore, app-agnostic)

| Piece | Path | Notes |
|---|---|---|
| Orientation-source contract | `binaural/HeadOrientationSource.h` | `getOrientation()` RT-safe POD copy per block; `setZero()` calibration; `valid=false` ⇒ fall back to manual params. `SnapshotHeadOrientationSource` base does the RtSnapshot publish/acquire. |
| Head attitude POD | `binaural/BinauralTypes.h` (`HeadOrientation`) | Radians, offsets from a "facing the stage" baseline, reserved 6DOF fields. **The angle conventions here are load-bearing** (§4). |
| Angle/matrix math | `binaural/HeadFrame.h` | ypr↔matrix both directions (gimbal-guarded), baseline composition, transpose, multiply. Your Wigner-D/SH rotation wants ypr or a matrix — both available. Unit tests in `tests/SpatcoreTests.cpp` (roundtrip + zero-composition). |
| Tracker plugin C ABI | `binaural/plugin/HeadTrackPluginApi.h` | Versioned, POD-only. Any `wfs_headtrack.{dll,so,dylib}` built once works for every consumer app unchanged. |
| 1-Euro filter | `dsp/OneEuroFilter.h` | Unit-agnostic; WFS-DIY uses minCutoff 1.5 Hz / beta 3.0 / dCutoff 1 Hz on radians — start there. |
| Dynamic-loader shim | `gpu/PlatformDynLib.h` | dlopen/dlsym/exeDir/pluginName on all 3 OSes (macOS branch added for exactly this purpose — do not re-guard it). |
| RT hand-off | `rt/RtSnapshot.h` | The message→RT POD publish/acquire primitive; read its header contract (pre-cooked values, publish-before-enable, ONE publisher). |
| SOFA loading | `binaural/SofaLoader.h`, `binaural/HrirSet.h` | libmysofa open/validate/resample + bake onto a uniform az×el grid with **ITD-extracted, time-aligned HRIRs** (xcorr + loud-ear onset split — threshold-only onset detection mis-splits the shadowed ear, see header comments). Reuse through the bake, then diverge: your consumer is the SH filter computation, not `CookedHrirSet`'s partitioned-FFT cook. |

### Copy-and-adapt (WFS-DIY app repo — app-flavoured, ~small)

| Piece | Path *(WFS-DIY)* | What to adapt |
|---|---|---|
| Webcam tracker source | `Source/DSP/CameraHeadTrackerSource.h` | ~350 lines. Only app-specific bits are `WFSLogger` and `AppSettings` (camera index); swap for XOA's equivalents. Contains the plugin load/ABI-check/zero-calibration/filter/staleness logic you do not want to rewrite. |
| Tracker manager + UI | `Source/DSP/HeadTrackerManager.h`, tracker dropdown + Set Zero in `Source/gui/SystemConfigTab.h` | Manager is ~120 lines. UI: stable-id persistence, missing-device→manual-without-overwriting-the-id, rescan-on-popup, "Set Zero visible only when a tracker is active", config-selectable-before-rendering-enabled. |
| Fast-path read in the worker | `Source/DSP/BinauralProcessor.h` (`processBlockHrtf`) | The pattern: orientation read fresh per block from the active source, bypassing the damped parameter pipeline; manual fallback when invalid; light slew downstream. In XOA this feeds the SH rotation matrix instead of a pose. |
| Plugin build | `tools/headtrack/` | The plugin itself is app-agnostic (capture + YuNet + geometric estimator, see its README for why it is NOT a solvePnP). Consume WFS-DIY's built artefacts, or lift `tools/headtrack/` verbatim — it has no WFS-DIY dependency. Windows gotchas are encoded in `build-headtrack-plugin.ps1` (OpenCV prebuilt needs `-DOpenCV_RUNTIME=vc16` pinned under VS2026; ship `opencv_world` + `msmf` DLLs, NOT the 25 MB ffmpeg one). |
| SOFA UX | IR-picker-style combo + `<project>/sofa` folder + 50 Hz poll/push idiom (`MainComponent.cpp` around `resolveBinauralSofaFile`) | Session-relative filenames, built-in default set (SADIE II KU100, `assets/SOFA`, repacked — see `tools/repack_sofa.py`: keep float64, shuffle+deflate; libmysofa rejects float32). |

### Build new (XOA only)

- SH rotation from `HeadOrientation` (yaw/pitch/roll → Wigner-D per block;
  recursive evaluation, e.g. Ivanic–Ruedenberg, is standard at order 10).
- SH→binaural decoder computation from the baked HRIR grid (MagLS or
  virtual-speaker sampling to start; this consumes `HrirSet` after the bake).
- The 2×(N+1)² static convolution (2×121 FIRs at order 10 — a shared-FFT
  partitioned scheme like `binaural/PartitionedFirConvolver.h` is a good
  reference even though the routing differs).

## 3. The latency split (same shape as WFS-DIY — keep it)

Scene/source content follows the damped control path (whatever XOA's
equivalent of the 50 Hz pipeline is). Head attitude must NOT: the render
worker reads `activeSource->getOrientation()` fresh every block, applies a
few-ms slew, and rotates. Manual orientation travels in the slow snapshot;
tracker orientation bypasses it entirely. Motion-to-ear with the webcam
tracker measures ~30–50 ms; a future IMU replaces the plugin behind the same
ABI and the same dropdown.

## 4. Load-bearing conventions (do not reinterpret)

- Angles: **+yaw = turn right, +pitch = look up, +roll = right ear down**,
  radians, offsets from the facing-the-stage baseline. Both the plugin ABI
  and `HeadOrientation` state this; the webcam plugin's signs were validated
  empirically against a human.
- Zero calibration is **matrix composition** (`R_zeroInv · R_raw`, then
  `matrixToYawPitchRoll`) — never per-angle subtraction; there is a unit test
  proving they differ.
- RtSnapshot: one producer thread, POD only, publish before enabling the
  consumer.
- Plugin loading: check `wfs_headtrack_abi_version()` FIRST; destroy through
  the plugin; log every load failure with its remedy (the GPU factory's
  silent-null fallback is a documented design flaw — don't copy it).

## 5. Suggested milestones

1. **Orientation plumbing first, renderer second**: port the manager/source/
   Set Zero seam and drive a debug printout of ypr in XOA — validates the
   whole tracking chain with zero DSP.
2. Static SH→binaural decode (no rotation): render a first-order test scene
   through decoder filters computed from the bundled KU100 set; verify L/R
   against a reference decode offline.
3. SH rotation: yaw-only first (trivial in SH: per-m phase/mixing within each
   band), verify a frontal source stays put under head yaw; then full
   Wigner-D.
4. Webcam tracker end-to-end + Set Zero UX.
5. Order-10 performance pass (121 filters; the shared-FFT trick amortizes).

## 6. Open decisions for XOA (not pre-made)

- Decoder design: MagLS vs virtual-speaker vs time-aligned LS; diffuse-field
  equalization; whether near-field compensation matters for monitoring.
- Whether XOA's "manual orientation" baseline is facing-origin like WFS-DIY
  or scene-fixed (affects only the baseline composition, not the sources).
- Whether the SH rotation lands in spatcore (`binaural/` or a new `hoa/`) —
  if TightWFS or a future WFS-DIY HOA-bus ever needs it, spatcore is the
  right home; otherwise app-side is fine.
