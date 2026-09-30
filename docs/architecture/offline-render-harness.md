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

The same gate for `spatcore/effects/`: **eleven scenarios** — one per module
type, one whole-chain, and one that drives the **engine** itself — recorded in
the machine's *CPU* baseline file under `effects/<scenario>` keys.

```
offline-render --path effects --scenario all --check baselines/<machine>.json
```

Passing prints `offline-render baseline check OK (11 combos match <machine>.json)`
and exits 0. The whole path still renders in well under a second.

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

7. **Effects engine** (`--scenario engine`) — `EffectsEngineCore` itself,
   driven **synchronously on the render thread** with `workerThreads = 0`: no
   `EffectsEngine` juce::Thread, no sleeping, no hoping. That is not a
   convenience, it is what makes the hash deterministic by construction —
   and it is exactly why the engine was split into a thread-free core in the
   first place (`effects/EffectsEngineCore.h`, "WHY THE THREAD IS NOT IN
   HERE"). Worker-count invariance is the engine's own contract and
   spatcore's `testEffectsEngineWorkerDeterminism` is what gates it; pinning
   0 here keeps this hash from depending on the core count of the machine
   that recorded it.

   The drive is one device callback per block, in the app's order and in the
   order spatcore's engine tests use: pull every return (at the **top** of
   the callback — that ordering *is* the one-block ledger), write every input
   into its render-source ring, write every popped return into the
   render-source row that belongs to it, then `drainAvailable()`. One
   complete block is waiting on every source, so exactly one batch runs per
   callback — except across the three scripted **driver stalls**, which are
   that last step not running while the callback keeps going, and which is
   the only way the engine's backlog, lap and discard branches are reachable
   at all. The four hashed streams are the four returns **as the callback
   popped them**, so the cushion latency, every underrun and every discard
   are inside the hash rather than beside it.

Scenario timelines (`scenarios.h`, `scenario::effectsParams`) are pure
functions of the tick, stepped at the same 50 Hz cadence as the matrix
timelines. The ten **module** scenarios share one script, so all ten read the
same way (default shape = 200 blocks x 512 @ 48 kHz = ticks 0..106). The engine
scenario is the exception and has its own — `scenario::engineChannelParams`,
per *channel* rather than per scenario, against its own timeline — because what
it drives is the engine rather than a module:

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

### The engine scenario (`--scenario engine`)

The ten module scenarios above render the DSP *inside* the engine. This one
renders the engine: **8 input sources + 4 effects channels = 12 render
sources**, the returns contiguous and last (`firstEffectSourceRow = 8`), a
feed matrix stepped at the same 50 Hz tick cadence, and four chains with four
different jobs. It exists because every behaviour worth gating here is one no
unit test reaches — they all need many blocks of real audio through a real
matrix, and several of them need the *driver* to misbehave:

| what | where in the timeline |
|---|---|
| the **block ledger** over 200 callbacks | asserted against an exact prediction: 190 batches, 44 underruns, 4 discards, 0 overflows, 1 source skip, 1 ring wrap, 2 clears |
| a **chain reorder** | fx0 at ticks 14 and 68 — the second inside its own guard's hold |
| a **chain bypass** window | fx1, ticks 22..30 |
| a **channel mute** window | fx2, ticks 56..64 (the chain's mute: its reverb keeps running and the tail moves on) |
| an **engine mute** window | ticks 95..100 — `EffectsEngineCore::setMuted`, which silences every channel's *feed* and lets every chain keep running. A different thing from the chain mute, and only the engine has it |
| the **effect-to-effect** path | fx0 ↔ fx1 as a loop, plus fx0 → fx3 one-way (the A → B hop, two blocks) |
| the **loop guard** tripping *and* releasing | the loop is cranked at tick 4 and taken back to safe levels at tick 11 |
| the **return veto** delaying a release | fx0's return is pinned at 3.185 over ticks 18..37 with its loop feed already calm — the one state the veto exists for |
| the **backoff ladder** doubling a hold | fx1 re-trips at tick ~46, its second *consecutive* trip, so its hold is two release times rather than one |
| an emergency **Clear**, both branches | `requestClear(2)` at tick 58 (one chain's tails) and `requestClear(-1)` at tick 103 (every chain *and* the shared feed history) |
| the global **loop-guard switch** | off over ticks 86..91 — which is what ends fx1's doubled hold, by snapping the held feed back to unity |
| the **routed** count differing from the **live** count | the matrix declares three effects over ticks 76..83 while four run: fx3 is unrouted and still has to hold its tails and feed the callback |
| three **driver stalls** | blocks 44 (one missed wake), 116–117 (two) and 132–139 (eight) |

**The driver stalls are the only way three of the engine's branches are
reachable at all**, and they are counted in *callbacks* rather than ticks
because that is what a stalled driver thread is: the audio callback keeps
running — it keeps pulling returns, and keeps counting underruns while it gets
none — and the engine's own thread misses wakes. One missed wake leaves two
blocks resident, which the next wake makes up in **two batches**, and the pull
after that finds two blocks in a ring whose cushion is one: the `pullReturn`
**discard** rule then has to choose what to trim. Two missed wakes leave three,
past `maxSourceBacklogBlocks`, so `processBatch` **jumps forward** instead of
working through the backlog. Eight leave nine blocks in an eight-block ring,
which is a **lap** — something the per-cursor "available" cannot express and
only the additive `getTotalWritten()` counter can see.

**A scenario where a behaviour never fires does not gate it**, so everything
above is *asserted*, not left to the hash: a hash notices that a behaviour
changed, never that it stopped happening at all — and on a machine recording
its first `effects/engine` baseline with `--update`, the assertions are the
only gate there is. The stderr note line is the evidence, and its expected
reading at the default shape is:

```
note: effects/engine: sources=12 channels=4 batches=190/200 guardTrips=3 [1,2,0,0] underruns=44 discards=4 overflows=0 skips=1 wraps=1 clears=2 nanTrips=0
```

Per channel, not just the total: `1,2,0,0` is one trip on fx0, two on fx1 — the
second is what doubles its hold — and none at all on the two channels that are
not in the loop. A total of three reached any other way is a different engine,
and the vector is compared, not printed and hoped over. Anything wrong exits
**5** with the reason, before a baseline is consulted or written:

- the ledger did not read what the stall table predicts — batches, underruns,
  discards, overflows, source skips, ring wraps or clears;
- the run went past the runaway and the guard never tripped;
- the run ended with a guard still holding a feed down — the release half is
  not being gated;
- the guards that tripped are not the ones the timeline drives;
- `LoopGuard::peakOf` does not keep a non-finite sample as the peak (see
  below).

Every figure in that prediction comes from `scenario::engine::expectedLedger()`,
which derives it from the same stall table the drive loop reads and the same
four shape constants the engine is configured with, so the two cannot drift
apart. The guard assertions and the trip vector stand down on a run that stops
before, or inside, the event they describe — `--blocks 30` stops at tick 15 and
reports `[1,1,0,0]`; `--blocks 85` stops mid-trip and says so — and so does the
ledger when a shape truncates a stall mid-window. The guard's thresholds are
*times*, so the same timeline trips and releases at any block size:
`--block 64 --blocks 1600` renders the same 2.13 s and reports the same trip
vector and the same ledger.

**Why the loop is bounded by a clip.** Both loop legs end at a hard-clipping
distortion (`shape = 0`, oversampling off), so each return is bounded by
construction at about 0.8 and a runaway builds to a *known* ceiling rather than
to infinity. A render that exploded would trip the NaN trap and gate the trap
instead of the engine. The send level is then chosen so the pre-gain
effect-to-effect bus clears the guard's +6 dBFS ceiling two to three times over
— no reliance on a chain gain to three figures — and the safe levels sit a
third or more under the release threshold. Every loop-guard *setting* is left
at the shipped default: a gate that tuned its own ceiling would gate its own
numbers instead of the product's.

**Why the veto needs a hot return over a calm feed.** The guard never touches
input-to-effect feeds, which is exactly how a channel ends up far over the
ceiling with its loop already cut — compressor makeup, distortion drive, a long
tail, a hot mix. That is what fx0's two HOT windows are: its input sends are
multiplied by 200 and its distortion is given +12 dB of *post-clip* makeup, so
its return is pinned at 0.8 × 3.981 = 3.185 whatever arrives — over the ceiling
by 60 %, bounded by the clip, and independent of what the loop is doing. The
reorder at tick 14 (`kOrderB` puts the clipper last) is what makes that figure
exact, which is why it comes *before* the first hot window rather than after.
The sends out of fx0 are 0.10 rather than 0.25 for the same reason: at 3.185 a
quarter would put fx1's bus back over *its* release threshold and fx1 would
never let go.

There is no `silentResets` figure for this scenario: the engine owns its chains
privately, so the harness cannot read their slot counters. The note line above
is its equivalent.

#### What this scenario still does not gate

Honest limits, because a gate that is believed to cover more than it does is
worse than one that says where it stops:

- **`LoopGuard::peakOf`'s NaN policy is gated by a self-test, not by the
  render**, and that is structural rather than lazy. The timeline is
  clip-bounded precisely so that a runaway gates the *engine* instead of the
  NaN trap, and a render that did go non-finite would be refused (exit 8)
  before its hash was recorded — so no hashed render can ever exercise it.
  `scenario::engine::loopGuardSelfTestFailure()` checks it directly instead:
  that `peakOf` keeps a NaN buried mid-block (a plain running max steps over
  one, because every comparison against a NaN is false), that it keeps a
  `+inf`, and that a guard fed a NaN feed peak trips and never releases. The
  engine's own NaN trap, the `WARNING:` variant of the note line and the exit-8
  route are consequently still dead in this scenario.
- **The veto's *budget* is out of reach at this shape.** `kVetoReleaseTimes` is
  four release times — 2 s — and the whole render is 2.13 s. The veto
  *mechanism* is gated (removing it moves the hash, because fx0's release
  arrives twenty ticks early), but the bound that keeps it from latching would
  need a run several times longer than one a per-scenario hash can afford.
- **The backoff ladder's *forget* rule is out of reach for the same reason.**
  `kBackoffForgetFactor` is four release times of armed, restored, calm feed —
  2 s again. The ladder's *doubling* is gated (fx1's second consecutive trip is
  held twice as long, and flattening `activeReleaseSeconds()` moves the hash);
  forgetting it is not.
- **`workerThreads` is pinned to 0**, so `AudioParallelFor`'s fork/join never
  runs here. That is deliberate — worker-count invariance is the engine's own
  contract and spatcore's `testEffectsEngineWorkerDeterminism` is what gates
  it; re-litigating it once per render would only make this hash depend on the
  core count of the machine that recorded it.
- **`returnCushionBlocks` is pinned to 1**, so the auto-cushion-of-2 the app
  selects at ≤ 128-sample blocks is never rendered. Stating it is what makes
  the ledger read the same way at any `--block`.
- **`matrixStride == numEffects == 4` exactly**, so the *stride* half of
  `processBatch`'s clamping — the one that guards against reading the next
  source's row, "an unbounded-loudness bug rather than a crash" — is not
  exercised. The *count* half is: the unrouted window at ticks 76..83 publishes
  three effects while four are live, and a sweep that ran over the routed count
  instead of the live one starves the callback within a block.

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

  The engine scenario is no different — it renders the same modules through
  the same libm — so `effects/engine` is recorded per machine too, and only
  in that machine's own file. Note what that means on the machine doing the
  recording: there is no hash to compare against yet, so its **assertions are
  the entire gate** for that run. That is why they are exact — the ledger, the
  resync counters and the per-channel trip vector, not just "something
  tripped" — and why an `--update` that exits 5 must be fixed rather than
  re-run.

- **Scenario families do not cross.** `--scenario all` means the four matrix
  timelines on a render path and the eleven effects timelines on
  `--path effects`; naming a scenario explicitly drops the paths of the other
  family, so
  `--path all --scenario static` still renders exactly the ten paths it always
  did. `--path cpu` is deliberately still the five WFS/reverb render paths —
  widening it would change what the documented CPU baseline invocation renders.
  `--path all` does include `effects`.
