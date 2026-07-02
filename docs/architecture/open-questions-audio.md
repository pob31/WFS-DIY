# Open Questions — Audio Engine Extraction

> Judgment calls that belong to Pierre-Olivier, not to this analysis session. Each item states
> what the code actually shows (so the decision is grounded), the options, and a lean
> recommendation. `spatcore` is the placeholder core-library name throughout.

---

## Q1. Repo topology: mono-repo vs. multi-repo for `spatcore` + three apps

**Context.** WFS-DIY is one ~100k-line JUCE app built from `WFS-DIY.jucer` (Projucer) plus a
separate `Plugin/CMakeLists.txt` bridge, and the GPU plugins build from standalone scripts
(`tools/{windows,linux}/build-gpu-plugins.*`). The GPL-3.0 license and the shared kernel-source
string (`CudaWfsKernels.h` included by HIP) already assume tight source coupling.

**Decision.** One repo containing `spatcore/` + `wfs-diy/` + `xoa/` + `tight-wfs/`, or separate
repos with `spatcore` vendored/submoduled.

- **Mono-repo (recommended):** atomic cross-cutting changes (a `spatcore` API change + all three
  app call-sites in one commit), one CI matrix, trivial bit-exact regression across apps. Cost:
  a bigger tree; needs clear target boundaries (already proposed in the boundary doc).
- **Multi-repo:** independent versioning/release cadence per app; but every `spatcore` change
  becomes a submodule bump + N PRs, and the GPU-plugin/kernel coupling makes lockstep changes
  frequent.

**Recommendation:** mono-repo with a hard CMake target boundary (§3 of the boundary doc). The
kernel-source sharing and bit-exact-parity needs make lockstep the common case; multi-repo taxes
exactly the workflow you'll use most.

---

## Q2. GPU scaffolding: "reverb sidecar" → main-path renderer — *when*?

**Context — this reframes the brief's question.** The brief asks whether to generalize the GPU
from a reverb sidecar to a main-path renderer "now or in Tight-WFS." **The main-path GPU renderer
already exists and ships:** `NativeGpuWfsAlgorithm` (gather) and `NativeGpuOutputBufferAlgorithm`
(scatter) are selectable production algorithms gated by `WFS_GPU_NATIVE=1`
(`WFS-DIY.jucer:525-559`), with the identical matrix surface as the CPU algorithms and full
metering/Live-Source-Tamer parity (`NativeGpuWfsAlgorithm.h:8`). The GPU already renders five
kernel families, not one.

**Decision.** Given it exists, the real question is: does `spatcore` expose the GPU main-path WFS
renderer as a **first-class, documented core capability** now, or keep treating it as a WFS-DIY
internal until Tight-WFS forces it?

- **Promote to first-class core now (recommended):** Tight-WFS is "GPU-first fractional-delay
  rendering at fat blocks" — that is precisely `NativeGpuWfsAlgorithm` + `GpuAsyncPipelineT` with
  a larger depth. Extract it as a supported `spatcore::gpu` renderer with the async pipeline and
  device-selection already generalized. Low marginal cost because the code is already generic.
- **Defer / keep app-internal:** less surface to stabilize now, but Tight-WFS then re-derives a
  renderer that already exists, and the two implementations risk diverging.

**Recommendation:** promote now. The expensive work (kernels, pump, device manager, matrix
double-buffering with latency compensation) is done and validated; leaving it app-private just
invites a fork.

---

## Q3. CUDA/HIP/Metal abstraction: complete it during extraction, or freeze as-is?

**Context.** The abstraction is further along than the brief implies: three complete backends per
family sharing one CUDA-C kernel source string (compiled at runtime via NVRTC/hipRTC/Metal),
selected by a compile-time alias inside each plugin, dispatched at runtime by
`GpuBackendFactory`/`GpuDeviceManager` on Windows/Linux. **Known incompletenesses:** Metal has
**no plugin/factory path** (`GpuBackendFactory.h:55-57` returns nullptr for Apple — "TODO") and
enumerates a single hard-coded `metal:0` (`GpuDeviceManager.h:112-115`); and per-role multi-GPU
across *different vendors* is structurally supported but untested (no AMD GPU on the dev machine —
see memory).

**Decision.** During extraction do you (a) finish the Metal per-device factory path and unify the
in-process vs plugin dispatch, or (b) freeze the abstraction and just move it?

- **Freeze + move (recommended for the extraction itself):** the abstraction is good enough to
  carry all three apps; completing Metal device selection is orthogonal to the CORE/APP boundary
  and adds regression risk to the move.
- **Complete during extraction:** tempting because you're already in the GPU code, but it couples
  a behaviour change to a structural move — harder to bisect if parity breaks.

**Recommendation:** freeze and move; file Metal per-device selection and cross-vendor multi-GPU
validation as *post-extraction* `spatcore` tickets. (The cross-vendor path especially needs
hardware the dev machine lacks.)

---

## Q4. Is the missing √(jω) / +3 dB-per-octave WFS prefilter intentional?

**Context.** The brief lists this filter as a landmark; it **does not exist** in the code (§4 of
the map). The only spectral shaping is an 800 Hz air-absorption high-shelf. For a 2.5-D WFS
driving function the +3 dB/oct pre-emphasis is textbook; its absence means the synthesized field's
spectral balance differs from canonical WFS.

**Decision (design, but it gates the core API).** Is the omission (a) deliberate — baked into
loudspeaker/room tuning and out of scope — or (b) a real gap to fill? And if filled, does it
belong in `spatcore` (both XOA and Tight-WFS would want a correct field-correction stage) or stay
app-side?

**Recommendation:** treat as a design decision *you* own, but note that Tight-WFS ("first-class
focused sources") and a rigorous XOA will both likely want a proper driving-function prefilter, so
if you add it, add it as an **optional `spatcore` pre-emphasis primitive** rather than a WFS-DIY
one. Do not silently add it during extraction (it changes WFS-DIY's sound).

---

## Q5. Denormal protection — add during extraction or defer?

**Context.** There is **zero** denormal protection anywhere (no `ScopedNoDenormals`/FTZ/DAZ),
despite many recursive biquads and feedback reverb (§4). This is a latent CPU-stall risk.

**Decision.** Add FTZ/DAZ (or `ScopedNoDenormals` at the callback + reverb entry points) as part
of the extraction, or keep behaviour byte-identical and defer?

**Recommendation:** defer it *out of* the extraction commit (it can change output LSBs and would
muddy the bit-exact gate), then land it immediately after as its own tested change. It is a real
bug worth fixing — just not while you're also proving the move is behaviour-preserving.

---

## Q6. The 32-node SDN hard cap for XOA / Tight-WFS

**Context.** `maxReverbChannels = 32` is a **hard** cap baked into GPU kernel scratch
`float incoming[32]` (`CudaSdnKernels.h:93`, `MetalSdnKernels.h:120`) and several
`std::array<…,MAX_NODES>` mirrors (§6). XOA uses "GPU for SH-domain reverb"; a 10th-order SH-domain
FDN/SDN could want far more than 32 channels.

**Decision.** Keep 32 as a `spatcore` capability bound, or re-architect the SDN kernel to a
runtime-sized (heap/shared-memory) scratch so nodes can scale?

**Recommendation:** keep 32 as a documented CORE bound for the *coupled SDN* (its cost is
quadratic in nodes anyway), and, if XOA needs high channel counts, prefer the **FDN/IR** families
(already heap-strided, `FdnHostConfig.h`) or a dedicated SH-domain reverb rather than scaling the
scattering network. Revisit only if profiling says a >32-node SDN is actually required.

---

## Q7. Which WFS path is canonical for `spatcore` and Tight-WFS: gather or scatter?

**Context.** Two CPU algorithms coexist (InputBuffer=gather, OutputBuffer=scatter) plus their two
GPU duals — four renderers with the same matrix surface. Tight-WFS is a *dense uniform array*;
that topology usually favours one memory-access pattern.

**Decision.** Carry all four into `spatcore` as interchangeable strategies, or designate a
canonical path for Tight-WFS and treat the others as WFS-DIY legacy?

**Recommendation:** keep all four in `spatcore` behind the common interface (they already share
the matrix contract, so the cost is near-zero), but benchmark gather-GPU vs scatter-GPU on a
representative dense array early in Tight-WFS to pick its default. Don't prune renderers during
extraction.

---

## Q8. Deterministic offline-render / replay harness — build it first?

**Context.** There is **no offline/deterministic render mode**; the engine is driven by the live
device callback and a wall-clock 50 Hz timer with async OSC/tracking input (§5.2 of the boundary
doc). The CPU path *is* deterministic given fixed inputs + a fixed matrix timeline (the FR
"diffusion" is a deterministic ramp, not RNG), so a golden-file bit-exact gate is *achievable* but
not *available*.

**Decision.** Invest in an offline-render + matrix-replay harness before extraction (so R1/R4
regressions are caught automatically), or rely on `tools/test-gpu-plugin.cpp` + manual A/B?

**Recommendation:** build the harness first. It is a small app-side addition (drive
`getNextAudioBlock` from a WAV at fixed block size; replay a captured `target*` timeline) and it
converts the highest-risk part of the migration (bit-exact parity of the delay-sum and reverb)
from "listen carefully" into a CI assertion. This is the single highest-leverage prerequisite.

---

## Q9. GPU pump permanent-failure semantics

**Context.** If `backend->processBlock` fails once, the pump thread sets `pumpFailed`, clears
`readyFlag`, and **exits permanently**; afterwards `popOutput` silence-fills without even counting
underruns (`GpuAsyncPipeline.h:212-219`). A transient GPU hiccup (TDR, driver reset) thus silently
and permanently drops the GPU path until the algorithm is re-prepared.

**Decision.** Is silent-permanent-drop the desired behaviour, or should the pump attempt a bounded
re-arm / fall back to CPU / surface a user-visible error?

**Recommendation:** treat as a `spatcore` robustness decision to make explicit: at minimum surface
`hasPumpFailed()` to the UI (WFS-DIY already exposes GPU status for reverb) and consider an
automatic CPU fallback on permanent pump failure. Decide the policy in CORE so all three apps
inherit it; don't leave it implicit.

---

## Q10. Core library naming

**Context.** `spatcore` is the placeholder used throughout these docs. The three apps are spatial
audio processors (WFS, Ambisonics, dense-array WFS) sharing DSP + GPU + RT infrastructure.

**Decision.** Final name for the library (and its C++ namespace).

**Options / notes:** `spatcore` reads clearly and is unclaimed-sounding; alternatives you might
weigh: `wavecore` (WFS-leaning, undersells Ambisonics), `fieldcore` (nice for "sound field," a bit
generic), `sonarc`/`aurora`-style brandable names (risk collision). Whatever you pick, make the
C++ namespace match and reserve it early so the extraction lands with the final name (renaming a
namespace across three apps later is pure churn).

**Recommendation:** keep `spatcore` unless a brand reason overrides — it names exactly what the
library is (spatial-audio core) and covers WFS + Ambisonics + dense-array without bias.

---

## Appendix — smaller decisions worth a line

- **Constants as runtime args vs compile-time** (Q4/§4 of boundary doc): recommend runtime
  ctor/`prepare()` args for input/output counts; keep the GPU SDN node cap compile-time.
- **`ReverbDiagnostics.h` macro** (`REVERB_DIAGNOSTICS`) leaks into `LockFreeRingBuffer.h`: decide
  whether ring overflow/underflow counters are a permanent CORE feature or a debug-only compile
  flag in `spatcore`.
- **Reverb sample-rate policy** (48 kHz decimation when device SR is a 48 k multiple,
  `MainComponent.cpp:4581-4588`): keep app-side or make a `spatcore::reverb` policy? Recommend
  CORE policy with an app-set target rate.
- **Metal `.mm` parity** with the CUDA/HIP `−L` compensation and 20 ms cushion was not
  re-verified in this pass (Metal backends weren't opened line-by-line); confirm before relying on
  identical latency behaviour on macOS.
