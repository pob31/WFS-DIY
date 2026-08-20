# Handoff — Stereo Channel Type with Panning-Based Decomposition

**Project:** WFS-DIY (JUCE/C++, GPL-3.0)
**Status:** Design settled at the level described here; parameter-level decisions deferred to implementation discussion.

> **2026-08-19 — superseded in part by the stable-channel-numbers rework.** The
> config-level "the LAST `stereoInputChannels` channels are stereo" split described in
> this document was replaced: every `<Input>` now carries a per-channel
> `inputChannelType` and a channel number (`id`) that is PERMANENT from the moment the
> session is in use; tree order is the user's display order (drag-to-reorder moves node
> + patch row together); count edits append after the last channel / remove the
> highest-numbered of that type, and deletions leave permanent number gaps. While the
> session is still fresh — nothing loaded, and no tab, patch window, snapshot or
> external controller having yet observed a number — every structural edit instead
> renumbers the list to display order; the `channelNumbersUserOwned` latch on the `<IO>`
> node separates the two regimes and is one-way. The DSP-side content of this document
> (slice layout, decomposition contract, Phase 1 plan) still applies unchanged. See
> "Stereo Pair Input Channels + Stable Channel Numbers" in `Documentation/CLAUDE.md`.
**Scope:** New *stereo pair* input channel type that internally decomposes a stereo signal into several spatial slice feeds rendered across the array. The split is invisible to the user: one channel object in, N derived renderer sources out.

---

## 1. Motivation

Stereo program material (playback tracks in theatre/dance, DJ decks, music cues) currently enters as two unrelated mono inputs or a position-clustered pair. Neither uses the spatial information already encoded in the mix. Amplitude-panned sources have a per-bin inter-channel magnitude ratio equal to their pan law; time-frequency sparsity of music means most bins are dominated by one source. The mix can therefore be *unfolded* into an azimuth decomposition deterministically — no ML, no trained weights, no licensing questions — and mapped onto the array as several virtual sources plus an ambience bed.

Reference lineage (for orientation, not required reading): Avendano & Jot (JAES 2004, panning index + coherence), Faller (JAES 2006, multi-loudspeaker playback of stereo), Barry et al. ADRess (DAFx-04), Merimaa/Goodwin/Jot (AES 2007, ambience extraction).

## 2. Phasing — build order is part of the design

**Phase 0 — pass-through stereo channel.** N = 2, slice 1 = L, slice 2 = R. No STFT, no added latency, no DSP beyond routing. Purpose: land the channel-type plumbing (channel-count field, snapshots, OSC address space, cluster behaviour, level meter) against a trivial and easily verified backend, and provide the permanent A/B control condition. This phase is independently useful in live shows and ships on its own.

**Phase 1 — panning decomposition.** The DSP described in §3, behind the same channel interface. The front end must stay abstracted so a future separation backend (e.g. ML stem separation) can replace it without touching the channel type, snapshots, or render mapping. Design for that swap; do **not** ship a backend selector UI.

**Phase 2 (not in scope, listed so interfaces anticipate it):** per-slice structural refinements (HPSS on the centre slice, transient/steady-state split), separator-informed placement.

## 3. DSP — Phase 1 backend

Per stereo channel, on the **CPU**. Estimated cost is ~100 Mflop/s per channel at the parameters below; three to four channels total expected (owner's stated ceiling: 3–4 stereo pairs even for music-heavy theatre shows). No GPU involvement and no per-channel CPU/GPU choice — the analysis is three orders of magnitude below the scatter stage and does not justify a PCIe round trip or a settings row.

Pipeline per frame:

1. **STFT**, WOLA, 75% overlap. Suggested starting point 21.3 ms window (2048 @ 96 kHz / 1024 @ 48 kHz — window size in *samples* must derive from sample rate to hold constant time/frequency behaviour; see §7).
2. **Crossover**: below ~100–120 Hz the signal bypasses decomposition entirely and sums to the channel's anchor feed (mono). Frequency resolution at practical window lengths is useless there and the band is spatially inert on the array.
3. **Coherence estimate** L↔R per bin → soft primary/ambient split. Not optional: decorrelated content (reverb, wide synths, chorus) has no defined pan position and must not pollute the azimuth slices.
4. **Panning index** on the primary part from the inter-channel magnitude ratio. Gate on inter-channel phase difference ≈ 0 as a per-bin confidence measure; bins failing the gate drift toward the anchor/ambient rather than getting a bogus intermediate position.
5. **Azimuth binning** into N slices with overlapping raised-cosine weights forming a **partition of unity** across azimuth. Same real magnitude weight applied to both channels, original phase preserved. Consequence: Σ slices + ambient ≡ input, structurally, bit-exact up to float. This reconstruction identity is a hard invariant — it is what makes the whole feature safe to put in front of an audience — and it gets a test (§8).
6. **ISTFT** per slice + ambient → N+1 feeds into the renderer as derived sources.

Slice-count guidance: default **5 + ambience** (hard L, mid L, centre, mid R, hard R). Kernel *width* is the selectivity control, set by estimator variance; N samples that continuous function (~2 slices per kernel width). Centre slice is anchored; non-uniform spacing (denser toward the sides) suits centre-heavy program. Over-slicing splits one source into coherent copies at different positions → comb filtering; more slices is *not* conservative.

## 4. Channel model — derived, not ganged

The snapshot stores **one** channel object: position, width, and the (few) stereo-specific parameters. Slice positions and gains are **computed at render time** from the decomposition state — slices are not persistent inputs, do not appear in the input list, and are not individually addressable via OSC/MCP for editing. This deliberately avoids re-introducing the persisted-gang complexity previously rejected for input clusters.

Mapping: ~~decomposition extremes map to the **usable array span**, not beyond it~~ — **SUPERSEDED.** Extremes map to an absolute distance in **metres**: `inputStereoWidth` (0.0–50.0 m, default 4.0) is the full L↔R distance between the pair's legs, azimuth ±1 sits `width × 0.5` from the anchor, and the channel's stored position is the **centre** of the pair. `inputStereoAxisOffset` (−179..180°, 0 = automatic) rotates the spread axis away from its automatic tangential orientation. No speaker position is read anywhere in the mapping.

The array-span reference went because it is one-dimensional and the app's rigs are not. It was the array's X extent: on a circular array of radius R that extent is 2R, so 100% spread ±R wherever the source stood; on side/surround arrays running along Y it referenced an axis those arrays barely extend along; and on a straight array with every speaker at the same X the half-span is 0, so the width dial did nothing at any setting — a silent total failure with no error path. Metres mean the same thing on straight, curved, circular and side rigs.

The legs are deliberately **not clamped** to the array. A width wider than the rig puts legs outside it where the low-density array produces no focused source; that is documented behaviour stated in the parameter's help text, not a case to correct silently. Clamping would put the rig's geometry back into the dial's law, which is exactly what was removed. A default derived from array density (aliasing frequency) is likewise off the table for the same reason — 4 m is a fixed, rig-independent default.

Confidence-driven placement: each slice carries a running confidence (coherence + phase-gate statistics). Low confidence collapses that slice's rendered spread toward the channel anchor; high confidence lets it sit at the estimated position. This replaces manual per-slice offsets as the default behaviour — no per-slice offset UI in the first iteration.

ADM-OSC: the channel maps to a single object with position + width. No address-space extension required.

## 5. Parameters

**Ruled out for this channel type** (settled — do not carry over): floor reflections, live source damping, gradient maps, sampler.

**Everything else per-channel is TBD at implementation** — do not enumerate or decide the remaining existing parameters in this document's scope; bring them to discussion with a proposed applicability table (applies to whole channel / applies per-slice / not applicable).

**New, stereo-specific (expected set, exact list to be settled):**
- slice count N (with the 5+ambience default)
- `inputStereoWidth` — FLOAT, 0.0–50.0 m, default 4.0. Full L↔R distance between the pair's legs; per-leg offset is `width × 0.5`. Live on `/wfs/input/stereoWidth` (+ `/remoteInput/` twin). **Settled, shipped in Phase 0** — this is what "global width factor" resolved to, and it is a distance, not a factor.
- `inputStereoAxisOffset` — INT, −179..180°, default 0. Rotation applied to the automatic tangential axis (0 = automatic), positive counter-clockwise viewed from above, the same convention as `inputRotation`; ±180 is an explicit L/R swap. Applied in the world frame and NOT mirrored by `inputFlipX/Y` (the flips already mirror the anchor, and the automatic axis follows it — mirroring the offset too would double-mirror). Live on `/wfs/input/stereoAxisOffset` (+ `/remoteInput/` twin). **Settled, shipped in Phase 0.**
- crossover frequency
- ambient-bed handling (global envelopment layer vs per-slice send — open question)
- centre-anchor behaviour
- confidence→collapse response (probably a fixed curve first, parameter later if needed)

Bias strongly toward *fewer* visible parameters. The GUI is considered cluttered already, and counteracting-parameter pairs have a track record of reading as double negations to operators learning the system.

## 6. Real-time integration

- **Latency bookkeeping.** The STFT adds ~21.3 ms to this channel type *only*. Mono channels are unaffected. The delay-alignment reference must therefore become channel-type-aware, or stereo playback and live mics will sit ~21 ms apart. Treat this like the existing GPU pipeline-depth pre-subtraction: known, constant, compensated in the WFS delays.
- **Hop staggering.** At 512-sample hop, analysis fires every 8th audio callback. Multiple stereo channels landing their FFT work on the same block produce a periodic spike against the 0.667 ms budget — the exact spike-tail pattern pipeline-bench flags. Stagger STFT phase per channel by hop/N_channels from the first commit.
- **Renderer load.** Slices are real renderer sources hitting the scatter stage (measured at 0.58 ms of the 0.667 ms budget on the T4). 4 pairs × 6 feeds = 24 derived sources. The channel-count field must display the derived source total, not only the pair count, so the budget consumption is visible.
- All analysis state per channel is preallocated; no allocation, locks, or logging on the audio thread (house RT rules apply).

## 7. Sample-rate note

Window/hop are specified in **time**, derived in samples per rate. At 96 kHz the FFT costs double for identical latency and frequency resolution versus 48 kHz — the usual higher-rate latency argument does not apply inside this stage. Do not hard-code 2048.

## 8. Validation

- **Reconstruction invariant:** Σ slices + ambient − input ≤ −120 dBFS over a test corpus, per block, as an automated test. Any DSP change that breaks this is wrong by definition.
- **Null test vs Phase 0:** with N=2 and decomposition bypassed, output must be identical to the pass-through channel.
- **Adjacent-slice correlation:** offline tool (or pipeline-bench-style harness) reporting normalised cross-correlation between adjacent slice outputs over a track; >~0.8 sustained indicates over-slicing for that material.
- **Spike behaviour:** pipeline-bench run with 4 stereo channels active, staggered vs unstaggered, before/after comparison.
- **Listening control:** the Phase 0 pass-through (two plane waves) is the standing reference. The decomposition must beat it audibly on real program material or it doesn't ship on by default.

## 9. Diagnostics

Slice inspection lives in the **level-meter detached window**, extending the existing solo/single mode: for a soloed stereo channel, show per-slice levels and confidence alongside the output contributions. Nothing decomposition-related is added to the main GUI. Without this view a bad decomposition is undebuggable in situ.

## 10. Known limits (documented behaviour, not bugs)

- Centre-heavy masters put most energy in the centre slice; the method exposes the spatial intent the mix contains and cannot invent separation that isn't there.
- Anti-correlated widener content (Haas, all-pass) reads as outside ±1; clamp to the extremes or route to ambient.
- Joint-stereo lossy sources degrade the HF inter-channel detail the estimator relies on.
- Summing several stereo programs into one stereo channel breaks the one-source-per-bin assumption — hence multiple stereo channels are first-class, and crossfaded decks belong on separate channels.

---

## Appendix A — Phase 0 implementation state (feat/stereo-input-channel)

**STATUS 2026-08-19: Phase 0 is CODE-COMPLETE — all 11 commits landed, every gate green.**
Remaining before merge (Windows box + hardware): regenerate control-replay goldens and the
Windows offline-render baselines for the new `stereo` scenario keys, run pipeline-bench at
104 sources, spot-check StreamDeck sub-tab routing on hardware, and an in-app listening
pass (stereo channel patched L/R → two plane waves; binaural studio preview).

**What Phase 1 inherits unchanged:** the `StereoDecomposer` interface (swap the pass-through
for the STFT backend, nothing else moves), the reconstruction-invariant tests (written
against the base class), the slot map, the RtSnapshot pair (config carries crossover +
stagger), the azimuth→metres mapping (app-side, `Source/Helpers/StereoImageGeometry.h`),
the render-latency reference hook (backend
reports its real latency and alignment engages automatically — add the mode-1 UI toggle
only then), and the `--stereo-null` harness gate.

**Two Phase-1 anchors moved with the metre rework.** `StereoDecomposerConfig` no longer carries
a `widthFactor`: width is a distance in metres, which has no 0..1 form a backend could act on,
and azimuth is normalised by contract, so no backend needs it — a field pinned at 1.0 would only
invite a Phase 1 implementer to re-derive the retired percentage-of-array semantics from it.
And the **confidence-collapse curve hooks into the CALLER of `WFSStereoImage::sliceOffset`**
(`MainComponent::refreshStereoSliceGeometry()`), not into the helper: scale the slice's azimuth
toward 0 before the call. The helper is shared with the Map tab's leg markers and must stay a
pure azimuth→metres function, or the Map would draw an image the renderer is not producing.

The sections below were the working notes for commits 7–11; kept for line-level context.

**2026-08-19 addendum (post-review rework, commits b442660..58624d5):** three design
changes after the first UI review. (1) The per-channel `inputChannelType` was REPLACED
by a config-level split — System Config "Mono Inputs" + "Stereo Inputs" (`ioTree
stereoInputChannels`); the LAST stereo channels of the list are pairs, their two patch
columns reserved upfront (per-channel flipping had to steal a neighbour's patch column
mid-project — DiGiCo-style adjacent-pair desks made that untenable). The per-channel
parameter is gone end-to-end (never released, no migration). (2) Slice 0 is now the
CENTRE/anchor feed in every backend (pass-through: slice 0 silent-but-active, 1 = L,
2 = R) — per-channel terms are genuinely anchor-computed, the Visualisation tab shows
the centre row, and the layout already matches Phase 1's. (3) The width axis is
perpendicular to the origin→anchor bearing in XY (tangential); flip mirrors the image
automatically (no explicit azimuth negation — it would double-mirror).

**Later addendum — width magnitude.** The tangential *direction* of (3) survived; only the
magnitude reference changed. Width became metres (see §4) and `inputStereoAxisOffset` was added
to rotate the axis, so the axis is no longer purely automatic. The axis is latched within
`WFSStereoImage::kAxisFreezeRadius` (1 m) of the origin: the automatic bearing sweeps 180° over a
few centimetres there, which would whip a source crossing the middle of an in-the-round rig from
left to right; holding the axis it arrived with is the fix, and no hysteresis band is wanted
(at exactly r == the radius the held value IS what the live branch computes).

### Landed (commits 1–6 of 11, every gate green)

Parent `979a263 → 6f5d764`, spatcore branch `feat/stereo-input-channel` at `8c50e28`
(commit spatcore first, then bump the pointer in the parent; spatcore git identity is
configured locally).

1. `InputSubTab` logical ids — enum values ARE the historical bar indices and are the
   wire contract to `StreamDeckManager::setSubTab`. Use `removeSubTab(InputSubTab::…)`,
   `isCurrentSubTab(…)`; never compare `getCurrentTabIndex()` to a literal again.
2. Reverb-sends mute now lives on the Mute-Macros row (Input Parameters ▸ Mutes column).
3. **Render-source split.** `MainComponent::numRenderSources` (renderer dimension) vs
   `numInputChannels` (channel identity). `recomputeRenderSourceCount()`
   (MainComponent.cpp:2618) is called at all four `numInputChannels` assignment sites
   and currently builds the **identity** map — C7 flips exactly this one function to
   `RenderSourceMap::build()` over the channel-type vector.
   `WFSCalculationEngine` matrix ROWS are already `maxRenderSources` (104); per-channel
   arrays stay 64. `LiveSourceTamerEngine` rows already 104 (lockstep, was a latent OOB).
   Baselines: 30/30 byte-identical after this refactor.
4. `inputChannelType` / `inputStereoWidth` exist end-to-end (tree, snapshots policy,
   OSC for width only, MCP tier 3/2, langs, CSV, dirty predicate). Nothing *writes*
   the type yet — no UI (C8), no OSC (by design), MCP tier-3 gated.
5. Patch matrix: `rowCapacityProvider` wired from `inputChannelType` in
   `PatchMatrixShim.cpp` (input patch only). Lower column = L. Row header shows
   `[L3 R?]` yellow when half-patched.
6. `spatcore/dsp/StereoDecomposer.h` + `PassThroughStereoDecomposer` + contract tests
   (`checkStereoReconstruction` is written against the base class — reuse it for the
   Phase 1 backend unchanged).

### C7 design decisions (settled during analysis — do not re-derive)

**Audio callback seam** (MainComponent.cpp, current line anchors):
`applyInputPatch` :5037-region → sampler :5040s → AutomOtion fade :5069 → **stage goes
here** → shared-ring publish :5077-5090 → smoothing → processBlock :5136+.
- `applyInputPatch` writes stereo rows ONLY into a new `stereoRawBuffer`
  (2 ch per stereo ordinal, preallocated in `prepareToPlay` next to the
  `patchedInputBuffer.setSize` added there), never into `patchedInputBuffer` — the
  decomposition stage is the sole writer of a stereo channel's 6 slots, so no
  aliasing inside `process()`. Build `inputPatchSecondaryHw[row]` (the higher column)
  in `loadAudioPatches` (~:2880); `inputPatchMap[hw]=row` itself needs no change.
- AutomOtion fade: apply the channel gain to BOTH `stereoRawBuffer` channels for a
  stereo row (linear gain pre-decomposition ≡ post, by the reconstruction identity).
- The stage also writes `L+R` … no — superseded: rings carry **per-source** audio
  (slices render individually in binaural, user decision). Ring count follows
  `numRenderSources` (already flipped at :5077/:5084 loop bounds); the reverb feed
  matrix input axis is already source-dimensioned end-to-end.
- Publish per stereo channel one `RtSnapshot<StereoSliceState[6]>` (audio→UI) and
  acquire one `RtSnapshot<StereoDecomposerConfig>` (UI→audio). Owner: new
  `Source/DSP/StereoChannelManager.h` (`unique_ptr<StereoDecomposer>` per stereo
  ordinal, scratch, both snapshots; prepared from `startAudioEngine`/`prepareToPlay`).

**Engine derived rows** (WFSCalculationEngine.cpp): the main loop is ALREADY
per-channel-scoped — `minDelay` (:1384 block), `commonAttenAdjustment` (:1493 block,
stored per input at :1562, reused by the reverb feed at :1811), mode-change ramps —
all computed once per `inIdx`. The C7 change: inside the same `inIdx` iteration, after
the primary row, compute the channel's derived rows (slice positions = anchor +
offset from `RenderSourceMap` desc, refreshed at 50 Hz from azimuth × width) using the
SAME per-channel terms. Never recompute minDelay/commonAtten per slice — per-slice
values break Σslices≡input at the array (comb filtering). Derived-row matrixIdx =
`firstDerivedSlot[inIdx] + (slice-1)` … times numOutputs stride. FR rows for derived
sources: forced off (push "off" in the :6076-region FR loop — already iterates
numRenderSources). `getCompositeInputPosition` is bounds-guarded (returns {} ≥
numInputs) — derived sources need a separate `sourcePositions[]` keyed by render
source, NOT an extension of compositeInputPositions (Map/clusters/ADM read that one).
Dirty flags are per channel; a channel dirty ⇒ recompute all its rows.

**Binaural:** `BinauralProcessor.h` — reverb-return base index is already
`numRenderSources + r` (flipped in commit 3, :559-region). Per-source positions and
gains flow through the same arrays the reverb-return append uses
(`hrtfPositions`/`hrtfSourceGains`, sized `maxSources` in prepareToPlay — grow to
`maxRenderSources + kMaxReverbNodes`).

**Meters:** `LevelMeteringManager::setSourceMap()` — per-channel aggregate over the
channel's sources: max peak, energy-sum RMS. Meter window stays one meter per channel.

**Null test (the C7 gate):** width=0 stereo channel at P fed L,R must hash
bit-identical to two mono channels at P fed L,R. Plus: baselines unchanged with no
stereo channel configured.

### C8/C9 anchors
- UI strings already shipped: `inputs.labels.channelType|stereoWidth`,
  `inputs.channelTypes.mono|stereo`, `inputs.help.channelTypeSelector|stereoWidthDial`.
- Stopped-only enforcement for the type combo: mirror the Audio Interface window
  policy; also drop a stereo row's second patch on stereo→mono revert.
- C9: `renderLatencyReference = max(sourceIntrinsicLatencyMs)` added in BOTH delay
  branches — including mode 1 (:1438-region), which today applies no latency terms
  (needs an explicit comment) — and the reverb-feed duplicate (:1959-region).
  Provably zero in Phase 0 ⇒ baselines gate it.

### Gotchas (cost time once already)
- Lang JSONs: never `json.dump` round-trip (reformats, un-escapes `°`); insert
  surgically with string ops and validate with `json.loads`.
- `git add -A` sweeps `build-spatcore-tests/` and control-replay temp dirs — both now
  gitignored.
- pbxproj: headers are added manually (PBXFileReference + one group children line);
  do NOT re-export from Projucer (manual fixes would be lost).
- Windows-box follow-ups before merge: regenerate control-replay goldens
  (hidden_tool_count 382→384 + total_parameters census; harness is taskkill/windll),
  and run pipeline-bench at 104 sources (`kMaxStereoChannels` in RenderSourceMap.h +
  WFSParameterDefaults.h is the one knob; 4 pairs = 84 sources if the T4 budget
  doesn't fit).
