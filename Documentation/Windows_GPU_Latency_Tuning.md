# Windows GPU Latency Tuning — measured runbook

Measured 2026-07-25 on the target rig: **RTX 5070** (display GPU, WDDM, sm_120) +
**Tesla T4** (compute-only MCDM, sm_75, blower-cooled), driver 610.74, CUDA 13.3
(driver, toolkit, and staged plugin DLLs all 13.3 — NVRTC compiles arch-exact
SASS at runtime, so there is no stale-kernel-binary failure mode).

Audio shape for every number below: **96 kHz / 64-sample device blocks**
(per-block GPU budget **0.667 ms**), **64 in x 128 out** (8192 pairs), scenario
`moving` (matrices stepping at the app's 50 Hz tick cadence), 30 s per run
(45 000 blocks), warmup 32 blocks excluded from distributions.

All measurements come from `tools/validation/pipeline-bench` (below) — it
drives the REAL `GpuAsyncPipelineT` (pump thread, SPSC rings, depth cushion,
underrun accounting), unlike `offline-render`, which drives the backends
synchronously and is the *bit-exactness* gate, not a latency tool.

> **The gather/scatter tables below were recorded with DEFAULT (free) clock
> and power management.** They are the *unmanaged* baseline and understate
> the T4 by ~7x. See "The 2x2: clocks vs graphs" immediately below for the
> pinned-clock numbers and the (surprising) interaction between the two
> interventions.

### The 2x2: clock pinning vs CUDA graphs (T4 gather, measured)

Post-warmup underruns per 30 s run, 96 kHz/64, 64x128, scenario `moving`:

| Depth | free + legacy | free + graphs | pinned + legacy | pinned + graphs |
|---|---|---|---|---|
| 1 | 82 | 4 | 14 | 8 |
| 2 | 45 | 4 | 4 | 5 |
| 3 | 34 | 9 | 5 | 4 |
| 4 | 24 | 7 | 5 | 2 |
| 5 | 19 | 5 | **0** | 5 |
| 6 | 7 | 1 | **0** | 1 |
| **total 1-6** | **211** | **30** | **28** | **25** |

**The two interventions are largely REDUNDANT, not additive.** Either one
alone takes the T4 from 211 to ~30 (a ~7x improvement); applying both gives
25, i.e. almost nothing extra. They are therefore attacking substantially the
same stall population rather than two independent problems — plausibly because
collapsing five submissions per block into one also removes most of the
idle→busy transitions that invite power-state churn in the first place.

Consequences for how to run the card:

- **Pin the clocks; graphs then become optional on the T4.** The pinned pair
  above was measured back-to-back in one session and is the only clean
  comparison here: pinned+legacy (28) and pinned+graphs (25) are within noise
  of each other. If you prefer not to manage an environment variable, pinned
  clocks alone is a complete answer.
- **Pinned + legacy was the only configuration to reach zero** (depths 5-6).
- Medians barely move in any cell (0.174-0.201 ms throughout) — every one of
  these interventions acts on the **tail**, not on throughput.

> **Caveat on cross-session comparison.** The two "free" columns were recorded
> earlier in the day; the two "pinned" columns were recorded later, after the
> machine had run the app, OCCT, and various setting changes. Only the
> pinned pair is a strict back-to-back A/B. Spike *counts* in particular move
> around between sessions (39 vs 194 for nominally identical graph runs), so
> treat the underrun totals as the robust signal and per-run spike counts as
> indicative only.

> **Mechanism not fully confirmed.** The field intervention was described as
> "maxing the power level"; idle `nvidia-smi` samples showed the T4 at
> P0/1590 MHz both before *and* after it, so the exact mechanism (deep C/P
> state entry between blocks, voltage/rail management, or PCIe link power
> management) is **not pinned down** — only its effect is. The label
> "power-state oscillation" below is the best available explanation, not a
> verified diagnosis.

## The measurement tool

```
# build (VS-bundled cmake)
cmake -S tools/validation/pipeline-bench -B tools/validation/pipeline-bench/build -G "Visual Studio 18 2026"
cmake --build tools/validation/pipeline-bench/build --config Release

# depth sweep at the rig shape (defaults: 96k/64, 64x128, scenario moving)
pipeline-bench --device cuda:0 --path gather --depth-sweep 1..6 --seconds 30 --json out.json
```

Per depth it reports underruns (total / post-warmup, with block indices),
pump-failure status, the backend-time distribution (min/med/p99/**p999**/max vs
the block budget), the simulated-callback wake-jitter distribution (so harness
timer noise is distinguishable from GPU stalls), and a timestamped spike log
(default threshold 1 ms). A run where the pump thread died is marked FAILED —
after a pump failure `popOutput` silence-fills *without* counting underruns
(`GpuAsyncPipeline.h`), so a naive underrun counter would read clean.

## Measured results

### WFS gather (direct path), legacy per-call submission

| Depth | 5070 med | 5070 p999 | 5070 max | 5070 underruns/30s | T4 med | T4 p999 | T4 max | T4 underruns/30s |
|---|---|---|---|---|---|---|---|---|
| 1 | 0.134 | 0.42 | 0.91 | 3 | 0.194 | 2.21 | 11.6 | 82 |
| 2 | 0.130 | 0.37 | 1.53 | 1 | 0.194 | 1.94 | 4.5 | 45 |
| 3 | 0.134 | 0.38 | 0.81 | 1 | 0.197 | 2.03 | 5.2 | 34 |
| 4 | 0.134 | 0.39 | 1.28 | 0 | 0.198 | 2.02 | 5.2 | 24 |
| 5 | 0.129 | 0.37 | 0.77 | 0 | 0.176 | 2.17 | 10.0 | 19 |
| 6 | 0.133 | 0.35 | 0.89 | 0 | 0.176 | 2.10 | 5.2 | 7 |

(ms; budget 0.667 ms)

**Reading it:** the 5070 is comfortably real-time (median 5x under budget) with
*rare* 1-1.5 ms spikes — clean from depth 4, near-clean at depth 2-3, a few
underruns per 30 s at depth 1. That matches the in-code field note
(`GpuAsyncPipeline.h:21-24`): WDDM desktop-compositor transients stall GPU
dispatch a few ms regardless of API; no code change can remove them on a
display GPU.

The T4's problem is **not throughput** — its median (0.19 ms) fits the budget
3x over. It is a heavy stall tail: ~150 spikes ≥1 ms per 30 s (p999 ~2 ms,
worst 11.6 ms), arriving in bursts, at every depth. These are per-call
submission stalls on the MCDM compute stack, and they are exactly what CUDA
graphs fix (next section).

### WFS gather with CUDA graph submission (`WFS_GPU_GRAPHS=1`)

| Depth | 5070 med | 5070 p999 | 5070 underruns/30s | T4 med | T4 p999 | T4 max | T4 underruns/30s | T4 spikes ≥1ms |
|---|---|---|---|---|---|---|---|---|
| 1 | 0.171 | 0.49 | 3 | 0.197 | 0.96 | 2.9 | 5 | 39 |
| 2 | 0.162 | 0.53 | 2 | 0.198 | 1.09 | 3.6 | 5 | 58 |
| 3 | 0.151 | 0.44 | 1 | 0.218 | 1.09 | 7.3 | 9 | 54 |
| 4 | 0.159 | 0.41 | 0 | 0.191 | 1.05 | 6.9 | 7 | 52 |
| 6 | 0.157 | 0.43 | 0 | 0.189 | 1.06 | 3.6 | 1 | 1 underrun |

- **T4: transformative.** Underruns at depth 1 drop 82 → 5; p999 halves
  (2.2 → ~1.0 ms); spike count drops ~4x; worst stall drops 11.6 → ~3-7 ms.
  One graph launch replaces five per-block submissions, giving the MCDM stack
  far fewer opportunities to stall. The T4 becomes usable at depth 1-2.
- **5070: neutral-to-slightly-negative.** Median +~30 µs (the two per-block
  `cuGraphExecKernelNodeSetParams` calls), tail unchanged — the 5070's rare
  spikes are compositor preemption, which a graph launch still rides through.
  Leave graphs OFF for the 5070. **Field-confirmed in the running app**
  (2026-07-25): with graphs enabled, 1-block still produced occasional drops
  on the 5070, i.e. no meaningful change — consistent with the headless
  sweep, and confirming that GUI-rendering contention on the display GPU does
  not alter the verdict.

The toggle is read at `prepare()`: set the environment variable
`WFS_GPU_GRAPHS=1` before launching the app when the WFS device is the T4 (or
any headless/MCDM CUDA device). Default is off. Bit-exactness is proven: with
graphs on and off, all gather scenarios hash identically against the golden
baselines on both sm_120 and sm_75
(`offline-render --path gpu-gather --check baselines/win-rig-{5070,t4}-gpu.json`).

**Implementation note (do not regress this):** only the *constant* per-block
core is in the graph (input H2D x2 → K1 → K2 → output D2H). The M2 upload diet
and prev/curr ping-pong stay outside, their conditional uploads stream-ordered
ahead of the graph launch. A first cut that graphed all matrix uploads
unconditionally **tripled** the per-block median on both GPUs — at 50 Hz tick
rates the diet skips almost every matrix upload, and those saved copies dwarf
the saved launch overhead. The instantiated graph is pre-uploaded at
`prepare()` (`cudaGraphUpload`); without that, the first launch pays a
measured 25-30 ms lazy-init stall.

### OutputBuffer scatter (legacy submission, no graph path yet)

| Depth | 5070 med | 5070 p999 | 5070 underruns/30s (post-warmup) | T4 med | T4 p99 | T4 underruns/30s |
|---|---|---|---|---|---|---|
| 1 | 0.197 | 0.54 | 7 | 0.584 | 0.90 | 291 |
| 4 | 0.197 | 0.51 | 13 | 0.587 | 0.91 | 226 |
| 6 | 0.191 | 0.39 | 14 | 0.580 | 0.91 | 209 |

- The 5070 holds scatter easily (median 0.2 ms).
- **The T4 runs scatter at ~87% of budget**: median 0.58-0.60 ms of a
  0.667 ms budget. This is a genuine compute ceiling (see root-cause section),
  and before the warmup fix the startup stall pushed every run into 200-300
  underruns; with warmup (below) depth 4 absorbs the residual 1-1.4 ms tail.
- **First-block lazy-init stall — FIXED (warmup launch in `prepare()`):**
  the OB backend's first `processBlock` measured 25-30 ms (5070) / ~82 ms
  (T4) of deferred driver/allocation work, an underrun burst right after
  every engine (re)start. `CudaObBackend::prepare()` now runs one silent
  block on the setup thread and then restores pristine first-launch state
  (`reset()` + the `frBasePrev` bootstrap), so the first audible block is
  bit-identical to a fresh prepare — verified against the GPU goldens on
  both devices. Re-measured at depth 4 (15 s): **5070 scatter 0 underruns,
  max 0.43 ms, no block-0 spike; T4 scatter 0 post-warmup underruns** (was
  226), with a residual ~8 ms first-block cost (down from 83 ms) that costs
  ~10 underruns in the first ~10 ms of audio, then clean.

### Root cause of the T4's struggles (it is NOT PCIe bandwidth)

Per-block traffic at this shape is tiny: ~16 KB in + 16 KB FR + 32 KB out,
plus 32 KB per matrix only when a 50 Hz tick changes one. Even the worst-case
fully-active FR per-sample delay buffer (~2 MB) takes ~0.25 ms at PCIe Gen3
x8 (~8 GB/s) — and steady state moves well under 100 KB/block, ~10 µs of bus
time. Gen3 vs Gen5 is irrelevant here. Three real causes:

1. **Power-state oscillation — the biggest operational lever (field-confirmed
   2026-07-25).** Real-time audio is a pathological profile for GPU power
   management: ~0.19 ms of work per 0.667 ms period is a **~28% duty cycle**,
   i.e. the card goes idle-ish thousands of times a second. Left to default
   management the T4 drops P-states in those gaps and pays a clock ramp on the
   next block — both a slower serial walk *and* transition latency, exactly
   the multi-ms stall bursts seen in the tables. Forcing maximum power /
   locked clocks removed most of it in field testing (pump % dropped
   "considerably", to roughly 5070-comparable). Verify with
   `nvidia-smi --query-gpu=pstate,clocks.sm,power.draw --format=csv`: a
   correctly pinned T4 sits at **P0 / 1590 MHz even at ~10% utilisation**; if
   you see it fall to P2/P8 between bursts, management is still free.
   Note this is *not* thermal throttling (the rig's T4 has an added blower and
   runs cool) — it is idle-driven P-state management.
2. **Per-call submission stalls (MCDM).** Independently of clocks, the T4's
   gather median (0.19 ms) fits the budget 3x over, yet legacy submission
   threw ~150 multi-ms stalls per 30 s. Switching the SAME work from five
   per-block submissions to one graph launch collapsed the tail 4-16x
   (underruns 82 → 5 at depth 1). Bandwidth or compute would be indifferent to
   *how* work is submitted, so the per-call submission path is real — but note
   that cause 1 and cause 2 were measured against the same symptom at
   different times, so their relative weight is **not cleanly separated**.
   Fewer submissions also means fewer idle gaps, so graphs may partly be
   mitigating the power-state problem. Apply both; don't assume either alone.
3. **Scatter = genuine compute ceiling.** `ob_pairs` walks all 64 samples
   serially per (in,out) pair (forced by the per-pair IIR shelf recurrence).
   That serial walk runs at the T4's 1.59 GHz Turing clocks vs the 5070's
   3.09 GHz Blackwell — measured 3.0x slower (0.582 vs 0.192 ms median),
   consistent with clock x IPC, and 0.58 ms of a 0.667 ms budget leaves no
   headroom for any jitter at all. Locking clocks *guarantees* the 1.59 GHz
   but cannot exceed it, so this one is structural. The K1a/K1b kernel split
   (roadmap) is the fix: move the sample-parallel work (delay ramp, fractional
   fetch) off the serial critical path so only the tiny IIR recurrence stays
   serial.

## Recommended settings for this rig

**Depth 3-4 is the safe operating point on either card** (field conclusion,
2026-07-25, and consistent with the sweeps). Depth 4 at 96k/64 costs 2.7 ms of
added latency, which the backend pre-subtracts from the delay matrix, so it is
free for any tap whose geometric delay already exceeds it.

1. **Pin GPU clocks first — this is the highest-value knob, especially on the
   T4.** Audio's ~28% duty cycle invites constant P-state churn (see root
   cause 1). Needs an **elevated** shell:
   ```
   nvidia-smi -lgc 1590,1590 -i 1     # T4: pin to its max SM clock
   nvidia-smi -lgc 2000,3090 -i 0     # 5070: floor the idle ramp
   nvidia-smi -rgc -i <n>             # undo
   ```
   Not persistent across reboots — re-apply per session or from a scheduled
   task at logon. On the T4 this was field-measured to drop pump % considerably
   and bring it to roughly 5070-comparable. Verify with
   `nvidia-smi --query-gpu=pstate,clocks.sm --format=csv`: a pinned T4 reads
   P0 / 1590 MHz even at ~10% utilisation. (On the GeForce card the NVIDIA
   Control Panel's *Power management mode → Prefer maximum performance* is an
   equivalent, persistent alternative.)

   > **Thermal cost — pinning is not free.** Holding max clocks holds max
   > voltage, so the card draws near-load power while nearly idle. Measured on
   > this rig: the pinned T4 sat at **38.8 W of its 70 W limit at ~11%
   > utilisation, 62 °C with the aftermarket blower at 95%**. That is well
   > inside the T4's limits (its thermal slowdown is ~85 °C), but it leaves
   > little cooling headroom, and it is a permanent duty on the blower.
   > If temperature or fan noise becomes a problem, try raising the clock
   > **floor** instead of pinning to the ceiling — e.g.
   > `nvidia-smi -lgc 1200,1590 -i 1` — which removes most of the deep-P-state
   > ramp latency at meaningfully lower idle power. Measure with
   > `pipeline-bench` to confirm the floor is high enough before trusting it
   > in a show.
2. **WFS on the RTX 5070** (the default), graphs **off**: depth 3-4 is clean.
   Depth 1 still shows occasional drops even with graphs enabled — field-
   confirmed, matching the headless sweep: those are WDDM compositor stalls,
   and neither graphs nor depth-1 tuning removes them. Don't chase 1-block on
   a display GPU.
3. **T4 as WFS (gather) device**: **pin the clocks** (item 1) and use depth
   3-4; depths 5-6 measured zero underruns per 30 s. `WFS_GPU_GRAPHS=1` is
   **optional once clocks are pinned** — the 2x2 above shows the two
   interventions are redundant (~7x each alone, no meaningful gain together),
   so pick whichever you prefer to manage. With clocks free *and* no graphs
   it degrades to the much worse baseline column.
4. **GPU reverb on the T4 — FDN yes, SDN no.** The two kernels have opposite
   shapes: FDN launches `grid = numNodes, block = 16`
   (`CudaFdnBackend.cpp:366`), so nodes spread across SMs, while SDN launches
   `grid = 1, block = numNodes` (`CudaSdnBackend.cpp:430`) — a single thread
   block on a single SM regardless of node count. The reverb paths also sit
   behind a **~20 ms wet-path cushion** (`kCushionMs = 20.0`,
   `depth = ceil(20 / blockMs)` in `Reverb*AlgorithmGPU.h`) rather than the
   WFS path's D x 0.667 ms, so they absorb the residual multi-ms jitter that
   makes the same card marginal for direct WFS. **Field-validated 2026-07-25:
   32-node FDN on the T4 alongside WFS gather on the 5070 at depth 2 runs
   well** — reverb pump peak **0.35 ms** against its 5.33 ms budget.

   > **Do not run GPU SDN at high node counts.** Production log, same session,
   > same 32 nodes, only the algorithm changed: SDN reverb pump peaked at
   > **7.5-9.4 ms against a 5.33 ms budget** (~25x FDN's 0.35 ms), overrunning
   > its own cushion and logging 60+ reverb underruns per second. Worse, while
   > SDN ran, the **direct WFS pump collapsed from ~0.2 ms to 4.9-5.3 ms peaks
   > and took 2799 underruns in ~6 seconds** at depth 3. Switching to IR and
   > back to FDN restored normal operation immediately (peak 0.34 ms).
   > The log does not record which device the reverb backend was bound to, so
   > whether the WFS collapse was GPU co-tenancy or host-side contention from
   > the saturated reverb pump is **unresolved** — but either way, SDN at this
   > node count is unusable and takes the direct path down with it.
   >
   > **User-confirmed 2026-07-25: SDN at 32 nodes is too much for the RTX 5070
   > as well.** This is not a weak-card problem and **no GPU upgrade fixes
   > it**: `sdn_process` launches `grid = 1`, so it occupies a single SM
   > regardless of how many the card has (1 of ~48 on the 5070), and it
   > `__syncthreads()` once per sample, making the whole block one long serial
   > dependency chain. Node count raises the work inside that one block
   > without spreading it. A faster clock buys a linear factor and nothing
   > more.
   >
   > **If you want SDN character at high node counts, use the CPU SDN path**:
   > it spreads nodes across the `AudioParallelFor` pool (up to 7 workers),
   > which on a 9950X is a far better match for 32 nodes than one SM. GPU SDN
   > is only sensible at low node counts.
5. **Keep the scatter (OutputBuffer) algorithm off the T4** at this shape:
   compute-ceiling-bound at ~87% of budget even with clocks pinned.

## CPU-side tuning (for the CPU reverb path)

Redirecting high-node-count SDN to the CPU (above) makes CPU jitter matter.
**Field result 2026-07-25: 32-node SDN holds on the CPU "most of the time"**
on the 9950X — far better than the GPU at the same node count — with
occasional dropped-reverb-block warnings.

1. **The 7-worker pool cap is NOT the constraint — measured, hypothesis
   rejected.** `ReverbEngine.h:124-126` clamps the pool to 7, which looked
   like the binding limit at 32 nodes on a 32-thread CPU. It is not.
   Measured with `offline-render --path reverb-{sdn,fdn} --in 32 --sr 48000
   --block 256 --blocks 800 --bench --reverb-workers N` (the app's real
   reverb shape — 5.33 ms budget):

   | Workers | SDN wall (768 blk) | SDN /block | FDN wall | FDN /block |
   |---|---|---|---|---|
   | 0 (sequential) | 803 ms | 1.05 ms | 576 ms | 0.75 ms |
   | 3 | 804 ms | 1.05 ms | 224 ms | 0.29 ms |
   | 7 | 787 ms | 1.02 ms | 156 ms | 0.20 ms |
   | 15 | 801 ms | 1.04 ms | 149 ms | 0.19 ms |
   | 23 | 801 ms | 1.04 ms | 149 ms | 0.19 ms |

   - **SDN does not scale at all, by design.**
     `ReverbSDNAlgorithm::setParallelFor` is an **empty override**
     (`ReverbSDNAlgorithm.h:231-235`): SDN runs a synchronous lockstep over
     all nodes, stepping every node together sample by sample, because the
     inter-node coupling is what makes it bit-exact with the GPU backend. It
     never touches the pool, so the cap is irrelevant to it.
   - **FDN scales well and saturates almost exactly at 7** (156 → 149 ms
     going from 7 to 23 workers, ~4%). The existing cap is well chosen, not
     a bottleneck.
   - **Worker-count invariance verified**: hashes were byte-identical across
     0/3/7/15/23 workers for both algorithms.

   **The operational conclusion: 32-node SDN uses ~1.05 ms of its 5.33 ms
   budget single-threaded — about 20%.** Occasional dropped reverb blocks at
   that utilisation are *jitter, not throughput*, so item 2 (CPU idle/power
   states) is the fix and no code change will help.
2. **CPU power/idle states — the exact analogue of the GPU P-state problem.**
   Audio's duty cycle invites deep C-state entry between blocks, and the
   wake latency lands as jitter. On AMD Ryzen the two settings that matter
   in BIOS are **Power Supply Idle Control → Typical Current Idle** and
   **Global C-state Control → Disabled**. In Windows, use the High
   Performance (or Ultimate Performance) plan with **Minimum processor state
   = 100%** and core parking disabled. The app already does its side —
   `HIGH_PRIORITY_CLASS`, the EcoQoS opt-out via `SetProcessInformation`,
   and `timeBeginPeriod(1)` (`Source/Main.cpp`) — so what remains is genuinely
   OS/firmware territory.
3. **CPU vs GPU for reverb at 32 nodes — the CPU wins on this machine.**
   Putting the measured CPU numbers beside the app's logged GPU pump peaks
   (means vs peaks, so indicative rather than strict):

   | Algorithm, 32 nodes | CPU /block | GPU pump peak | Budget |
   |---|---|---|---|
   | SDN | 1.05 ms (single-threaded) | 7.5-9.4 ms | 5.33 ms |
   | FDN | 0.20 ms (7 workers) | 0.35 ms | 5.33 ms |

   SDN is ~9x better on the CPU and *fits* there while it does not on the
   GPU. FDN is roughly a wash. So at this node count on a 16-core CPU, GPU
   reverb buys **offload, not speed** — worth it when the CPU is busy with
   other work, not because the GPU is faster. GPU reverb's real advantage
   should appear at much higher node counts or on weaker CPUs.
4. **CCD placement is unmanaged — and measurement says it does not matter
   here.** The 9950X is a dual-CCD part with separate L3 per CCD, and there
   is **no thread affinity or CCD pinning anywhere in the app or spatcore**
   (verified — only JUCE's unused `setAffinityMask` API exists; also noted as
   a divergence in `audio-engine-map.md`). Measured with process affinity
   masks over the FDN 7-worker pool (32 nodes, 3 reps each):

   | Affinity | FDN wall (768 blk), mean |
   |---|---|
   | All 32 logical CPUs (default) | 159.6 ms |
   | CCD0 only (CPU 0-15) | 158.2 ms |
   | CCD0, one thread per physical core (CPU 0,2,…,14) | 153.2 ms |

   Constraining to a single CCD is **within noise** (~1%); the only real
   effect is avoiding SMT siblings (~4%), and at 0.20 ms of a 5.33 ms budget
   that is operationally irrelevant. Note this measures **throughput, not
   tail latency** — it does not rule out migration-induced jitter, but it
   removes the cache-locality argument as a first-order concern.

   **On the thermal/boost trade-off** (does letting the scheduler rotate the
   hot core buy back boost headroom, at the cost of cache locality?): the
   trade-off is real *in general* but does not bind here. Core rotation only
   buys boost when a core is thermally or power saturated; this workload runs
   at ~20% duty cycle (SDN 1.05 ms of 5.33 ms) at low absolute power, so no
   core ever gets hot enough for Precision Boost to throttle it. Silicon wear
   levelling is not a design consideration at stock voltages — aging is far
   outside any realistic service life. One genuine caveat if you do
   experiment: AMD's CPPC exposes *preferred* (best-binned) cores and Windows
   already schedules onto them, so pinning to an arbitrary core set can cost
   a boost bin. Prefer constraining to a *set* (one CCD, or one logical per
   physical core) over rigid thread↔core pinning — that keeps locality while
   still allowing intra-set migration.

### Transients that are NOT defects

These all cost a handful of blocks and are expected; do not chase them:

- **Changing the reverb algorithm while processing is enabled** — the backend
  re-prepares live (NVRTC compile + allocation). Measured ~4 underruns per
  switch (FDN/IR). Not a normal-operation path.
- **Engine start / depth change** — re-prepare plus pump and worker-pool
  spin-up. The OB backend's warmup launch removed most of it (~83 → ~8 ms
  first block); some starts still show a burst in the first second.
- **Launching another GPU or CPU stress application** — starting OCCT during
  playback cost one block (field-observed 2026-07-25). That is the correct
  outcome: the cushion absorbed what it could, the underrun counter caught
  the rest, and the pump neither cascaded nor failed. Anything that hammers
  the GPU or CPU will perturb a 0.667 ms budget.

### Known-good configuration (field-validated 2026-07-25)

| Role | Device | Settings |
|---|---|---|
| WFS gather (direct) | RTX 5070 | depth **2**, graphs off, clocks pinned |
| FDN reverb, 32 nodes | Tesla T4 | max power / clocks pinned |

Per-role device binding is already exposed in the UI and persisted
(`algorithmDeviceId`, `reverb*GpuDevice`), so this split needs no code change —
and it is a genuinely good division of labour: the display GPU takes the
latency-critical 0.667 ms path, the compute card takes the throughput work
behind a 20 ms cushion where its residual stall tail is invisible.
4. **HAGS** (Hardware-Accelerated GPU Scheduling): not explicitly set in the
   registry on this machine (`HKLM\...\GraphicsDrivers\HwSchMode` absent —
   Windows 11 typically defaults it ON for RTX 50). Each state needs a reboot
   to A/B; re-run the depth-1 gather sweep in both states and compare the
   spike logs. Field experience varies per driver; measure, don't assume.
5. **MPO / compositor load:** the 5070's spikes are compositor-correlated by
   nature. For shows, keep the desktop idle (no browser/video on the 5070),
   consider disabling Multi-Plane Overlay
   (`HKLM\SOFTWARE\Microsoft\Windows\Dwm\OverlayTestMode = 5`, reboot), and
   avoid HDR/auto-refresh-rate toggling mid-show.
6. **Deeper diagnosis** of residual spikes: capture a GPUView trace
   (`wpr -start GPU` / Windows Performance Recorder, or `log.cmd` from the
   GPUView kit) around a spike-log timestamp from `pipeline-bench --json` and
   look for VidMm/scheduler preemption events against the pump thread's
   submissions.

## Optimization roadmap (evidence-ranked)

1. **DONE — CUDA graph submission for WFS gather** (`WFS_GPU_GRAPHS=1`,
   default off): T4 depth-1 underruns 82 → 5. Kernels untouched
   (`kernel_hashes.py` clean); bit-exact on sm_120 + sm_75.
2. **OB scatter graph path** — same core-only capture shape as WFS (the
   FR-tiered row uploads MUST stay outside the graph, like the upload diet).
   Expected to help the T4's submission tail, but the T4 scatter problem is
   throughput — pair it with item 4.
3. **HIP graph port** — deliberate follow-up now that the pattern is proven on
   NVIDIA: `hipStreamBeginCapture` / `hipGraphInstantiate` /
   `hipGraphExecKernelNodeSetParams` / `hipGraphLaunch` mirror the CUDA calls
   ~1:1 in `HipWfsBackend.cpp`. Keep the same env toggle and core-only graph
   shape; validate with the same `--check` goldens on AMD hardware. Metal
   needs nothing (its per-block `MTLCommandBuffer` is already a single
   submission).
4. **`wfs_pairs`/`ob_pairs` occupancy split (K1a/K1b)** — one thread per pair,
   serial over all 64 samples, is the T4's real throughput ceiling (scatter
   median 0.58 ms). Split the sample-parallel work (delay ramp, fractional
   fetch) from the serial IIR-shelf recurrence, preserving recurrence order
   for bit-exactness. This is a **validation event** (kernel-hash `--update`
   + golden re-record) and mirrors to Metal/HIP kernel headers.
5. **DONE — warmup launch at OB `prepare()`** — killed the measured 25-85 ms
   first-block lazy-init stall (5070: fully clean; T4: ~8 ms residual first
   block, clean after). Extend to the reverb GPU backends if their engine
   (re)start bursts ever show up in practice; the WFS gather backend showed
   no measurable first-block stall (its graph path also pre-uploads).
6. **Multi-block SDN kernel** — `sdn_process` is `grid = 1` with a per-sample
   `__syncthreads()`, so it uses one SM on any GPU and is unusable at 32
   nodes on both cards in this rig (see the warning above). Restructuring it
   to spread paths across blocks (with the inter-node coupling handled by a
   second kernel or cooperative groups) is the only thing that makes GPU SDN
   scale; until then, high node counts belong on the CPU SDN path. Also a
   validation event (kernel-hash `--update` + goldens).
7. **Log the reverb backend's device** — the `Reverb GPU pump:` line reports
   peak/budget/underruns but not which GPU it is bound to, which made the
   2026-07-25 SDN episode impossible to attribute (co-tenancy vs host
   contention). Mirror the direct path's `Native GPU WFS active: ... on
   <device>` line at reverb backend start.
8. **Small items:** hoist the per-block `cuCtxSetCurrent`/`cudaSetDevice`
   rebind to first-use-per-thread; make pump failure visible in the GPU UI
   strip and optionally re-arm (open question Q9,
   `docs/architecture/open-questions-audio.md`); input-ring overflow counters
   are compiled out of shipping builds (`REVERB_DIAGNOSTICS`) so input-side
   overruns are invisible — pipeline-bench builds with them on.

## Baselines and validation on this rig

- Per-device GPU goldens: `tools/validation/offline-render/baselines/win-rig-5070-gpu.json`
  and `win-rig-t4-gpu.json` (recorded from the current source build — the
  previously shipped `wfs_cuda.dll` was built 2026-07-06 and predated the FR
  diffusion Max-port and the SDN N-invariant gain fix; rebuild the plugin
  after pulling GPU-touching commits, the app will happily load a stale DLL).
- Validation sequence after any GPU host-code change:
  `build-gpu-plugins.ps1` → `offline-render --path gpu --check <golden>` with
  graphs off AND on → `kernel_hashes.py` → `pipeline-bench` A/B sweep.
