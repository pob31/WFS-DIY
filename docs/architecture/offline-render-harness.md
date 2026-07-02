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
  over a monotonic per-block index + (in,out) key) — bit-reproducible, no RNG.
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
│                         #   reverb-ir} --scenario <name> [--device <id>]
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
