# Offline Render Harness — Design (Phase 0 of the spatcore extraction)

Status: **implemented** at `tools/validation/offline-render/` (CMake console app,
`Plugin/CMakeLists.txt` precedent) — CPU gather/scatter + SDN/FDN/IR paths, 3
scenarios each, all 15 combos run-to-run deterministic; machine baseline at
`baselines/win-dev-nvidia.json`. Milestone 2 (GPU) is also done: gpu-gather /
gpu-scatter drive the vendor backends synchronously exactly as §3 below
(`WFS_GPU_NATIVE=1 WFS_GPU_PLUGINS=1`, plugin dlopen — no GPU runtime linked;
`--device cuda:0`, `--plugin-dir` / auto-probed app build dir). GPU hashes are
per device+driver and live in the separate `baselines/win-dev-nvidia-gpu.json`,
checked in a separate `--path gpu` invocation so the CPU baseline stays
portable; `--path all` skips the gpu paths with a note when no GPU/plugin is
present. GPU determinism verified: all 6 gpu combos identical across 5 runs
(CUDA reduce order stable on this device/driver).

Purpose: the **bit-exact gate** for every extraction phase. Renders a fixed
input through the four WFS renderers and the three reverb algorithms, entirely
headless, and hashes the output (SHA-256 of the float PCM). Identical hash
pre/post every file move = gate passes.

## Verified interface facts (from code, 2026-07-02)

All five target classes are header-only and **app-type-clean** — no ValueTree,
no `MainComponent`, raw `const float*` matrices only. Matrix families and
layouts (all input-major `[in * numOutputs + out]`):

| Matrix | Units | Consumer |
|---|---|---|
| `delayTimesMs` | ms | all four WFS renderers |
| `levels` | linear | " |
| `hfAttenuation` | dB | " |
| `frDelayTimesMs` | **extra** ms on top of direct | " |
| `frLevels` | linear | " |
| `frHFAttenuation` | dB | " |
| reverb feed | linear, `[in * stride + node]` | harness feed-mix (app: `ReverbFeedThread`) |
| reverb return | linear, `[node * stride + out]` | harness return-mix (app: `MainComponent` ~4909) |

Every class is **asynchronous** (worker threads + lock-free rings); none has a
synchronous public entry at the algorithm-wrapper level. The harness therefore
drives one level lower, where synchronous/drainable entry points exist —
**no production-code changes are needed**:

## Drive strategy per path

1. **CPU gather** — instantiate `InputBufferProcessor` (public ctor:
   `(inputIndex, numOutputs, 6 matrix ptrs)`) per input, `prepare` + start.
   Per block: `pushInput(data, n)` on every processor, then for each (in, out)
   pair **drain-pull**: loop `pullOutput(out, tmp, remaining)` (it returns the
   count actually read) with `Thread::yield` until `n` samples accumulate; sum
   into the output in the app's fixed in→out loop order
   (`InputBufferAlgorithm.h:137-158`), which fixes float summation order.
   Workers process fixed 64-sample sub-blocks regardless of caller block size,
   so state evolution is invariant to our drive pattern.
2. **CPU scatter** — create `numInputs` × `SharedInputRingBuffer` (public),
   write input to them per block; instantiate `OutputBufferProcessor` per
   output wired to those rings (public ctor), drain-pull each output.
3. **GPU (gather + scatter)** — bypass `NativeGpu*Algorithm`/`GpuAsyncPipelineT`
   entirely: `makeWfsBackend(deviceId)` / `makeObBackend(deviceId)` →
   `prepare(..., pipelineLatencyMs = 0, ...)` → `setMatrixPointers(...)` →
   `processBlock(in, out)` — the backend call is synchronous (this is the
   pattern the `Experiments/cuda-*-test` spikes already use). With
   `pipelineLatencyMs = 0` there is no −L delay pre-subtraction and no primed
   silence. NOTE: GPU output is a **self-consistency gate only** (GPU vs its
   own golden on the same device+driver); GPU↔CPU differ by design (per-sample
   FR ramps on GPU vs 50 Hz-stepped + `DelayTargetSmoother` on CPU).
4. **Reverb (SDN/FDN/IR)** — bypass `ReverbEngine`'s thread/rings/cushion:
   instantiate `SDNAlgorithm`/`FDNAlgorithm`/`IRAlgorithm` directly,
   `prepare`, set an `AudioParallelFor` prepared with **0 workers**
   (sequential fallback — results are worker-count-invariant anyway since each
   node owns its output channel, but 0 removes all doubt), push
   `AlgorithmParameters` (POD, `ReverbAlgorithm.h:13`) + `NodePosition`
   geometry before the first block, then call `processBlock` synchronously.
   Optionally wrap with `ReverbPreProcessor`/`ReverbPostProcessor` (also POD
   param structs). Feed/return mixing per the matrix table above.

## Determinism notes (verified)

- FR diffusion "jitter" is **hash-keyed** (`FrDiffusionModel.h`, Squirrel hash
  over each stream's own segment counter + (in,out) key) — bit-reproducible,
  no RNG; a stream's trajectory depends only on samples advanced since reset.
- `DelayTargetSmoother` is sample-index-driven — deterministic.
- Per-channel CPU threads do no cross-thread summation; reverb parallelFor is
  per-node with disjoint outputs — worker count never changes results.
- CPU-load telemetry (`getMillisecondCounterHiRes`) never enters audio math;
  build the harness with `REVERB_DIAGNOSTICS=0`; leave metering off (default).
- No RNG anywhere in these five classes (LFO/TestSignal RNG is upstream and
  replaced by the replayed matrices).
- **IR path (found during implementation):** `juce::dsp::Convolution` loads IRs
  on a background thread and installs the engine mid-`process()` with a 50 ms
  crossfade (juce_Convolution.cpp:1055-1130) — the install block index is
  timing-dependent. The harness warms up by probing (impulse + silence) until
  every node's convolver shows a tail, then calls `reset()` (kills engine
  crossfade, juce_Convolution.cpp:1277-1281); everything after is
  deterministic. CPU paths also require `--block` to be a multiple of the
  64-sample worker chunk or the drain stalls.

## Control input: scripted deterministic timelines (not a MainComponent recorder)

The gate only needs *identical input → identical output pre/post move*, so v1
generates matrix timelines **inside the harness** from scripted scenarios
(static scene, moving source with delay/level ramps at a 50 Hz tick cadence,
FR on/off toggles, reverb param change mid-run) instead of instrumenting
`MainComponent` with a recorder. A recorder for captured *golden sessions*
remains a later nice-to-have (adds realism, not required for the gate) — this
drops the riskiest Phase-0 item (7k-line `MainComponent` surgery) from the
critical path.

Scenario → per-tick matrix values must be pure functions of the tick index
(no RNG, no time). Timeline application = write the six arrays between blocks
at tick boundaries, exactly as the app's 50 Hz timer does (the algorithms
re-smooth internally).

## Deliverable shape

```
tools/validation/offline-render/
├── CMakeLists.txt        # juce_add_console_app; modules: juce_core,
│                         # juce_events, juce_audio_basics, juce_audio_formats,
│                         # juce_dsp (IR convolution); WFS_GPU_NATIVE=1 for the
│                         # GPU paths on machines with a toolkit, else CPU-only
├── main.cpp              # scenario runner: --path {cpu-gather|cpu-scatter|
│                         #   gpu-gather|gpu-scatter|reverb-sdn|reverb-fdn|
│                         #   reverb-ir|effects} --scenario <name> [--device <id>]
│                         #   [--blocks N --block 512 --sr 48000 --in 8 --out 16]
│                         # prints SHA-256 + writes optional WAV for listening
├── scenarios.h           # the scripted deterministic timelines
└── baselines/            # committed per-machine hash tables
    └── <machine>.json    # { "<path>/<scenario>": "<sha256>", ... }
```

Runner exit code: 0 when all requested hashes match the machine's baseline
file; non-zero lists mismatches (same contract as `kernel_hashes.py`). Input
signal: deterministic in-code generation (impulses + fixed-phase sine bank +
hash-noise via the same Squirrel hash) — no WAV fixtures needed for v1.

## Open implementation details

- `OutputBufferProcessor` wiring: mirror `OutputBufferAlgorithm::prepare`
  (`OutputBufferAlgorithm.h:23,218`) for ring creation/sizing.
- IR path needs a deterministic in-code IR (decaying hash-noise, like the GPU
  smoke test) rather than a file.
- Drain-pull timeout: bound each spin (e.g. 5 s) so a hung worker fails the
  gate loudly instead of deadlocking CI.
- The harness compiles app headers in place today; when files move into
  `spatcore/`, only its include paths change — the gate itself must not change
  behavior across the move (hashes prove it).

## Building and running it

`cmake` is **not on PATH** on a stock Visual Studio install on this machine
(`which cmake` and `Get-Command cmake` both find nothing). Use the copy VS
ships, by full path, or run the two commands from a VS Developer prompt where
PATH already has it:

```
CMAKE="C:/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"
"$CMAKE" -S tools/validation/offline-render -B tools/validation/offline-render/build -G "Visual Studio 18 2026"
"$CMAKE" --build tools/validation/offline-render/build --config Release
```

The exe lands at
`tools/validation/offline-render/build/offline-render_artefacts/Release/offline-render.exe`,
and every `--check` path below is **relative to the current directory**, so run
it from `tools/validation/offline-render/`.

**The `spatcore` submodule pointer is part of this tool.** The harness compiles
`spatcore/effects/` in place, so a checkout whose recorded pointer predates
`effects/` cannot build it at all. The pointer bump therefore ships in the same
commit as any harness change that needs it (a4e4e57 or later for the effects
path) — it is never an incidental working-tree leftover. `CMakeLists.txt`
checks for the headers at configure time and fails with that instruction
rather than with dozens of missing-include errors.

### Render shape

Hashes are only comparable at the shape they were rendered at, and **only the
default shape (`sr=48000 block=512 blocks=200 in=8 out=16`) is ever
baselined**. The shape is stamped into the baseline file under the reserved
`"#shape"` key, and `--check`/`--update` are **refused with exit 2** when the
run shape differs from it, or when the file was recorded at another shape.

This matters most for `--update`, which is the only way the `effects/*` entries
ever get created on the mac and Linux machines. A wrong-shape `--check` would
merely MISMATCH, but a wrong-shape `--update` used to record a truncated run as
the golden and exit 0: `--blocks 30` stops at tick ~31, before the bypass window
closes and long before either variant switch, so the recorded hash would gate
roughly a quarter of the script it is named after. Record with the shape left
alone:

```
offline-render --path effects --scenario all --check baselines/<machine>.json --update
```

A file with no `#shape` key (anything recorded before this guard) still reads;
the next `--update` stamps it. If the default shape itself is ever changed,
every entry in every baseline file is stale: delete the files and re-record.

### Not wired into CI

`grep -rn offline-render .github/workflows/` returns nothing: neither the
effects path nor the older cpu/gpu paths run automatically, so in practice this
gate fires only when a human runs it. That is deliberate rather than an
oversight — the hashes are per machine (the effects ones emphatically so, see
below), and a GitHub runner is a different machine on every job, so there is no
golden for CI to check against. What CI *could* usefully run one day is the
path without `--check`: a build plus a render still catches a crash, a hang and
a tripped NaN trap (exit 8). Until then, the honest answer to "what does this
gate" is "whatever was run before the commit".

## Effects path (`--path effects`)

The same gate for `spatcore/effects/`: **ten scenarios** — one per module type
plus one whole-chain — recorded in the machine's *CPU* baseline file under
`effects/<scenario>` keys.

```
offline-render --path effects --scenario all --check baselines/<machine>.json
```

Passing prints `offline-render baseline check OK (10 combos match <machine>.json)`
and exits 0. The whole path renders in well under a second.

Drive strategy (a sixth entry for §"Drive strategy per path" above):

6. **Effects** — no thread, no ring, no device: the modules are mono,
   synchronous and in-place. A module scenario goes through a **`ModuleSlot`**
   rather than the bare module, because the three behaviours worth gating live
   in the slot — the bypass crossfade, the reset-at-silence, and
   `commitPendingVariant()` for a change that cannot be interpolated. Driving a
   bare module would render it at whatever `applyParams` last set and never
   exercise any of that; driving it at defaults would render a *bypassed*
   module, which hashes the input straight back. The chain scenario drives an
   **`EffectChain`**, which additionally owns the reorder envelope and the
   chain bypass/mute envelopes.

   One scenario renders `--in` **independent chains** (default 8), one per
   input stream, each prepared with its own `ChainConfig::noiseKey`, so the
   per-channel keyed noise — bitcrusher dither, the random LFO shapes, the
   reverb model's node identity — is in the hash instead of eight copies of
   channel 0. `--out` is unused on this path.

Scenario timelines (`scenarios.h`, `scenario::effectsParams`) are pure
functions of the tick, stepped at the same 50 Hz cadence as the matrix
timelines, and share one script so all ten read the same way (default shape =
200 blocks x 512 @ 48 kHz = ticks 0..106):

| ticks | what happens |
|---|---|
| 0..19 | active, every continuous parameter sweeping |
| 20..34 | **bypassed** — the slot fades out and resets the module at silence |
| 35.. | active again, including the fade back in |
| 50 | **variant switch A** — fade out, reset, `commitPendingVariant()`, fade in |
| 75 | **variant switch B** |
| 40 / 60 / 85 | per-module discrete edits (a stage on, an LFO shape, a dither floor) |

The variant per module: distortion = oversampling factor; dynamics = detector
mode then lookahead; modulation = voice count then through-zero; phaser = stage
count; reverb = size (rebuilds the FDN); bitcrusher = decimation filter. The EQ
has no `variantPending`, so a band that changes **shape** stands in for one; the
multitap delay reports none either (its discrete edits glide), so its tap
count, pattern, manual/pattern mode and feedback tap are scripted instead.
Tremolo has no variant at all — its sweeps and the bypass toggle are its whole
surface. The `silentResets` counter printed to stderr after each render is the
evidence that the script fired, and every scenario has an expected value:

| scenario | expected `silentResets` | why |
|---|---|---|
| eq, trem, delay | **8** | 8 chains x 1 bypass window; these three report no `variantPending` |
| dist, dyn, mod, phaser, reverb, crush | **24** | 8 chains x (1 bypass + 2 variant commits) |
| chain | **24** | 8 chains x (eq2 bypass + dyn2 variant + dyn2 bypass) |

A number other than the one in this table means the timeline did not do what
it says it does. `silentResets` counts only **`ModuleSlot`** arrivals at
settled silence, so the chain's own envelopes contribute nothing to it: the
chain-bypass window resets the slots through `EffectChain::resetSlots()`,
which is a different path and deliberately does not count.

If a **NaN trap** fires, the stderr line says `WARNING:` instead of `note:`
and the run exits **8** without checking or recording anything. A hash taken
from a tripped render is the trap's output, not the module's, so it is not a
gate — and silently baselining one would freeze the fault in place.

The **chain** scenario runs all eleven slots live and does what no unit test
can do at render scale: **three reorders** (ticks 40, 58, 70, parsed from order
strings through the shipped `parseChainOrder`) and a **chain bypass** window
(ticks 55..64) plus a **mute** window (ticks 90..95). The reorder at 58 falls
inside the bypass window on purpose, so both swap paths are covered — the muted
swap at a block boundary, and `EffectChain`'s "silent anyway, just take it"
branch.

It also carries the **only slot-level coverage `eq2` and `dyn2` get anywhere**.
Instance 1 of the EQ and of the dynamics exists only inside the chain — no
module scenario instantiates it — so left at steady settings those two slots
would never go through a bypass fade, a reset-at-silence or a
`commitPendingVariant()`, and a `ModuleSlot` regression specific to instance 1
would pass the gate. The timeline therefore bypasses `eq2` over ticks 20..34,
switches `dyn2`'s detector (peak to RMS, a variant that has to be taken at
silence) at tick 45, and bypasses `dyn2` over ticks 72..86 — all three placed
clear of the chain-bypass and mute windows so the fades land in audible output.
That is where the chain's `silentResets = 24` comes from.

A fault in the order data would leave the default order running silently (the
parser refuses a whole string rather than half-applying it), which would turn
the reorder gate into no gate at all. `scenario::effectsSelfTestFailure()`
checks before every effects render and exits 5 with the reason, which is one of
**three different faults**, not just a typo:

- a string that does not parse, or parses to something that is not a valid
  permutation of the slots;
- `kOrderDefault` not equalling the shipped default order;
- a reorder string equal to the default, or two reorder strings equal to each
  other — either way one of the three reorders reorders nothing.

The message names the offending constant and which of the three it is, so a
duplicated order does not send the reader hunting a parse bug that is not
there.

Two things the effects path does **not** share with the older paths:

- **Baselines are per machine, more strongly than the WFS ones.** The modules
  call `std::tanh`, `std::cos`, `std::exp` and `std::tan` (distortion, phaser,
  the LFOs, every filter coefficient). `FastDecibels` and
  `FrDiffusion::hashNoiseBipolar` are libm-free and identical everywhere, but
  those four are not, so a Windows hash will not necessarily match macOS or
  Linux. The `effects/*` entries must be recorded on each machine with
  `--update`, never copied between baseline files, and always at the default
  render shape — leave `--blocks` / `--block` / `--sr` / `--in` / `--out`
  alone and the harness stamps and enforces it for you (see §Render shape):

  ```
  cd tools/validation/offline-render
  offline-render --path effects --scenario all --check baselines/<machine>.json --update
  ```

- **Scenario families do not cross.** `--scenario all` means the four matrix
  timelines on a render path and the ten module timelines on `--path effects`;
  naming a scenario explicitly drops the paths of the other family, so
  `--path all --scenario static` still renders exactly the ten paths it always
  did. `--path cpu` is deliberately still the five WFS/reverb render paths —
  widening it would change what the documented CPU baseline invocation renders.
  `--path all` does include `effects`.
