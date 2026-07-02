# `spatcore` Boundary Proposal — Audio Engine

> **Companion to** `audio-engine-map.md`. Every classification cites the evidence there or the
> code directly. `spatcore` is the placeholder name for the shared core library that WFS-DIY,
> **XOA** (Ambisonics) and **Tight-WFS** (dense-array) will link against.
> **Guiding principle:** the map showed the hot path is already decoupled behind raw
> pointers / POD structs / lock-free rings, while app coupling concentrates in six control-rate
> ValueTree-listening engines. The boundary follows that existing seam rather than inventing one.

---

## 1. Classification

Legend: **CORE** = generic, moves to `spatcore` as-is or nearly · **APP** = WFS-DIY-specific,
stays · **TANGLED** = generic in nature but currently welded to app specifics.

### 1.1 CORE — move to `spatcore`

| Component | File(s) | Why CORE |
|---|---|---|
| SPSC ring buffer | `Source/LockFreeRingBuffer.h` | Generic acquire/release SPSC ring; only dep is `ReverbDiagnostics.h` (a diagnostics macro header, itself trivially CORE). Used by GPU pipeline + reverb. |
| SP/MC ring buffer | `Source/DSP/SharedInputRingBuffer.h` | Generic single-producer/multi-consumer ring (per-consumer cursor). |
| C1 delay smoother | `Source/DSP/DelayTargetSmoother.h` | Header-only box-filter + teleport envelope; zero JUCE/app deps (cstdint/cmath/algorithm). The heart of click-free moving-source rendering. |
| Position smoothing | `Source/DSP/InputSpeedLimiter.h`, `Source/DSP/TrackingPositionFilter.h` | Self-contained tanh speed cap + 1-Euro filter; broadly reusable. |
| Biquad primitives | `WFSBiquadFilter.h`, `WFSHighShelfFilter.h`, `OutputEQBiquadFilter.h`, `ReverbBiquadFilter.h` | RBJ-cookbook DFI biquads, no app/UI coupling. |
| Numeric guard | `Source/Helpers/NumericGuards.h` | Dependency-light `safeClamp`. |
| Fork-join pool + RT utils | `Source/DSP/AudioParallelFor.h`, `Source/DSP/AudioWorkgroupCoordinator.h`, `Source/DSP/RealtimeThreadUtil.h` | RT thread primitives (fork-join, macOS workgroup join, P-core policy); no app deps. |
| Per-channel WFS delay-sum workers | `Source/DSP/InputBufferProcessor.h`, `Source/DSP/OutputBufferProcessor.h` | Take only `const float*` matrix pointers + scalar setters (`InputBufferProcessor.h:27-42`); depend only on DSP-local siblings (rings, biquads, smoother) + JUCE Thread. *The delay-sum kernel itself is generic; the “which driving function” lives in the matrices, which are supplied by the app.* |
| WFS/OB algorithm shells | `Source/DSP/InputBufferAlgorithm.h`, `Source/DSP/OutputBufferAlgorithm.h` | Fork-join over the processors via rings + raw matrix pointers; no `MainComponent` internals. |
| Reverb engine + branch | `Source/DSP/ReverbEngine.h`, `ReverbFeedThread.h`, `ReverbPreProcessor.h`, `ReverbPostProcessor.h` | POD-struct + `std::string` deviceId interface, ring audio I/O, no ValueTree. Generic wet-send engine plumbing. |
| Reverb algorithm math | `ReverbFDNAlgorithm.h`(+GPU), `ReverbIRAlgorithm.h`(+GPU) | FDN (Walsh-Hadamard, prime delays) and partitioned convolution are textbook, node-independent reverbs; no room/PA knowledge. |
| GPU scaffolding | `Source/DSP/gpu/GpuAsyncPipeline.h`, `GpuDeviceManager.h`, `GpuBackendFactory.h`, `GpuBackendInterface.h`, `PlatformDynLib.h`, `plugin/GpuVendorPlugin.cpp` | Backend-agnostic pump, device enumeration, vendor plugin factory, dynlib shim — self-contained plumbing any host can reuse. |
| GPU compute backends + kernels | `Cuda*/Hip*/Metal*Backend`, `*Kernels.h`, `*HostConfig.h`, `*HostState.h`, `IrHostFft.h` | Domain compute + FFT helper; no app/UI dependency. |

### 1.2 APP — stays in WFS-DIY

| Component | File(s) | Why APP |
|---|---|---|
| Parameter/state store | `Source/Parameters/WFSValueTreeState.*`, `WFSParameterIDs.h`, `WFSParameterDefaults.h` | The app state/undo/OSC-address surface; the thing DSP must be decoupled *from*. (Channel-count constants could become `spatcore` ctor args — see §4.) |
| The broker | `Source/MainComponent.cpp` engine sections | Owns calc engines + matrix arrays, timer→audio matrix copy, wires OSC/MCP/UI/GPU status. The orchestration glue. |
| WFS driving functions | `WFSCalculationEngine.cpp` geometry math (once the ValueTree binding is peeled off — see TANGLED) | Sparse/curved-array delay/level/HF from source+speaker geometry: "this is a PA system." |
| SDN geometry + placement | `ReverbSDNAlgorithm.h` node placement, `MainComponent.cpp:5844-5850` position feed, `Network/MCP/tools/ReverbAutoLayoutTool.h` | SDN inter-node delays are physical-node-position-derived ("PA in a room"); auto-layout writes venue geometry into the ValueTree. |
| Speaker/array geometry | `Source/Helpers/ArrayGeometryCalculator.*`, patch maps | Loudspeaker layout handling. |
| GUI device selectors | `SystemConfigTab.h`, `ReverbTab.h` per-device combos | Presentation of the CORE device/algorithm selection. |
| Binaural PA specifics | `BinauralProcessor.h`/`BinauralCalculationEngine.h` position→HRTF selection | HRTF *filtering* is generic; the position-driven selection + `-0.3 dB/m` shelf are app curves. |

### 1.3 TANGLED — generic math welded to app specifics

| Component | Weld | Minimal refactor to free it | Risk |
|---|---|---|---|
| `WFSCalculationEngine.cpp/.h` | Is a `juce::ValueTree::Listener`, takes `WFSValueTreeState&`, includes `Parameters/`, reads properties directly in `propertyChanged`/recalc. | Split into (a) a pure **`GeometryDrivingFunction`** core: positions + params **in**, the seven matrices **out**, no JUCE ValueTree; and (b) a thin app-side adapter that listens to the ValueTree and pushes a param snapshot into the core. The math is already geometry-only. | Medium: it produces 7 coupled matrices under `matrixLock`; the snapshot seam must preserve the single-reader-thread invariant and the dirty-flag caching or 50 Hz cost regresses. |
| `LiveSourceTamerEngine.h` | Takes `WFSValueTreeState&` **and** `WFSCalculationEngine&`; shares a bare `const float*` gains pointer (`setLSGainsPtr`). | Feed it a per-input param snapshot + write gains into a caller-owned buffer passed by span; drop the raw cross-object pointer. | Medium: the LS gains are multiplied into the level matrix inside the calc engine — the ownership/lifetime of the shared buffer must be formalized. |
| `AutomOtionProcessor.h` | Includes both `Parameters/WFSValueTreeState.h` and `Network/OSCProtocolTypes.h` (`.h:4-9`). | Move it out of `DSP/`; it is app automation, not a DSP primitive. Give it a narrow "set return-gain(ch)" output the broker reads. | Low–Medium: mostly a relocation + include cleanup; behaviour unchanged. |
| `LFOProcessor.h`, `ClusterLFOProcessor.h`, `BinauralCalculationEngine.h` | Include `WFSValueTreeState.h`; control-rate ValueTree listeners. | Same param-snapshot pattern: LFO/rotation *math* is generic (→ CORE), the ValueTree binding stays app-side. | Low: pure control-rate, no hot-path rewrite. |
| GPU latency compensation | `GpuAsyncPipeline` owns depth/latency, but the `−L` pre-subtract is applied inside the **WFS-specific** backend delay-matrix snapshot (`CudaWfsBackend.cpp:382`). | Keep the compensation *policy* (how many ms) in the pipeline and expose `getLatencyMs()`; let the app/driving-function apply it to the matrix. Already 90% there. | Low: it is a one-line arithmetic move; risk is bit-exactness of the clamp. |
| `GpuLevelMeters.h` | Generic level metering fused with app **Live Source Tamer** GR semantics + CPU-parity contract. | Factor the LS coupling out; keep the follower/meter as CORE. | Low. |
| `NativeGpuWfsAlgorithm.h` / `Reverb*AlgorithmGPU.h` | Bind CORE GPU backends to the app's matrices / `ReverbAlgorithm` interface / param conventions. | These are the *adapter* layer; keep the adapter app-side, the backends CORE. | Low: intended split point. |

> **Note on `InputBufferProcessor`/`OutputBufferProcessor`.** The `dsp-primitives` pass flagged
> them TANGLED (they mix delay/interp/biquad DSP with `juce::Thread` lifecycle, rings, metering).
> The `coupling` pass flagged them CORE (no app/param/UI coupling — only DSP-local siblings). Both
> are right: they are **CORE**, because everything they depend on (rings, biquads, smoother,
> workgroup coordinator) is *also* CORE. No refactor is needed to move them; the "tangle" is
> internal complexity, not app coupling.

---

## 2. Header-level API sketch for the CORE modules

Signatures below are proposals (not current code) showing ownership and the threading contract.
Namespacing under `spatcore::`.

```cpp
// spatcore/rt/LockFreeRingBuffer.h  — SPSC, one producer thread + one consumer thread.
namespace spatcore::rt {
class LockFreeRingBuffer {                     // OWNS its float buffer
public:
    void setSize (int numSamples);             // NOT RT-safe: allocates. Call from prepare.
    int  write   (const float* src, int n) noexcept;  // producer thread only
    int  read    (float* dst, int n) noexcept;        // consumer thread only
    int  getAvailableData () const noexcept;   // either thread (both cursors acquire)
    int  getFreeSpace     () const noexcept;
};

// SP/MC variant — one producer, N consumers, each owning its cursor.
class SharedInputRingBuffer {
public:
    void setSize (int numSamples);
    void write   (const float* src, int n) noexcept;                    // producer only
    int  readWithPosition (int& consumerCursor, float* dst, int n) const noexcept; // per-consumer
};
} // namespace spatcore::rt
```

```cpp
// spatcore/rt/RealtimeThread.h  — RT scheduling + fork-join, platform-abstracted.
namespace spatcore::rt {
bool setCurrentThreadRealtimeAudio (double periodMs, double computationMs); // macOS P-core; no-op else

class AudioWorkgroupCoordinator {   // macOS os_workgroup join; no-op elsewhere
public:
    void set (void* deviceWorkgroupHandle);          // prepare / device-change thread
    void joinIfChanged (Token& tok, uint32_t& seenGen) noexcept; // worker thread, lock-free fast path
};

class AudioParallelFor {            // persistent std::thread fork-join
public:
    void prepare (int maxWorkers, double periodMs, double compMs, AudioWorkgroupCoordinator*);
    void parallelFor (int count, const std::function<void(int)>& body); // caller participates; falls back to serial if 0 workers
    void shutdown ();               // joins workers — NOT RT-safe
};
} // namespace spatcore::rt
```

```cpp
// spatcore/dsp/DelayTargetSmoother.h  — per-tap C1 delay + teleport envelope. Single-thread (owning processor).
namespace spatcore::dsp {
class DelayTargetSmoother {
public:
    void  prepare (int windowSamples);              // ~10 ms box
    void  setTeleportThreshold (float samples);     // default 3*window
    void  pushTarget (float delaySamples);          // control-rate target
    struct Sample { float delay; float gain; };
    Sample next (int sampleIndexInBlock) noexcept;  // per-sample query, O(1)
    void  reset ();
};

// RBJ Direct-Form-I biquad family (LowCut/HighShelf/Peak/...); coefficients computed, not tabulated.
class Biquad {
public:
    void  setLowCut (double fc, double sr);
    void  setHighShelf (double fc, double gainDb, double q, double sr);
    float processSample (float x) noexcept;         // recursive; single-thread
    void  reset ();
};
} // namespace spatcore::dsp
```

```cpp
// spatcore/wfs/DelaySumProcessor.h  — the gather/scatter per-channel delay-line worker.
// OWNS: 1 s circular delay lines (direct + FR), biquads, smoothers, level detector.
// Threading: one instance per channel runs on its own RT thread; the audio callback
// communicates only via the rings + notify(). Reads routing via raw const float* matrices
// owned by the caller (single-reader-thread invariant; see migration risk R3).
namespace spatcore::wfs {
class InputDelaySumProcessor /* : rt::RealtimeThread */ {
public:
    void prepare (int channelIndex, int numPeers, double sr, int blockSize,
                  const float* delaysMs, const float* levels,
                  const float* frDelaysMs, const float* frLevels,
                  const float* hfAttenDb, const float* frHfAttenDb);
    void pushInput (const float* src, int n) noexcept;  // audio callback
    int  pullOutput (int peerIndex, float* dst, int n) noexcept;
};
// OutputDelaySumProcessor: mirror, scatter-write variant.
} // namespace spatcore::wfs
```

```cpp
// spatcore/gpu/GpuBackendInterface.h  — minimal pump-facing contract + 5 family interfaces.
namespace spatcore::gpu {
struct IGpuBackend {
    virtual bool  prepare (double sr, int blockSize, int numIn, int numOut) = 0;
    virtual bool  processBlock (const float* const* in, float* const* out) noexcept = 0; // pump thread; blocking launch
    virtual bool  isReady () const noexcept = 0;
    virtual const char* getLastError () const noexcept = 0;
    virtual double getLastLaunchMs () const noexcept = 0;
    virtual const char* getDeviceName () const noexcept = 0;
    virtual void  release () = 0;
    virtual ~IGpuBackend() = default;
};
struct IWfsBackend : IGpuBackend { virtual void setMatrixPointers (/*delays,gains,fr,hf,latencyMs*/) = 0; };
struct IObBackend  : IGpuBackend { /* shares the WFS matrix surface */ };
struct IIrBackend  : IGpuBackend { virtual void loadIR (const float*, int) = 0; };
struct IFdnBackend : IGpuBackend { virtual void setParameters (/*RT60, decay bands*/) = 0; };
struct ISdnBackend : IGpuBackend { virtual void setGeometry (const float* xyz, int n) = 0; };

// Deadline-isolation pump: audio thread pushes/pops rings and never blocks; a private RT pump
// thread drives one synchronous processBlock per iteration; depth D primes D silence blocks.
template <typename BackendT> class GpuAsyncPipelineT { /* pushInput/popOutput/getLatencyMs/hasPumpFailed */ };

// Runtime device enumeration + per-vendor plugin factory (dlopen; never links vendor runtimes).
class GpuDeviceManager;      // singleton: devices(), find(id), firstGpuId()
std::unique_ptr<IWfsBackend> makeWfsBackend (const std::string& deviceId); // nullptr → caller uses CPU
} // namespace spatcore::gpu
```

```cpp
// spatcore/reverb/ReverbEngine.h  — decoupled wet-send engine (own RT thread + fork-join pool).
// Communicates via POD structs + std::string deviceIds; NO ValueTree.
namespace spatcore::reverb {
struct AlgorithmParameters { /* RT60 bands, diffusion, sdnScale, ... POD */ };
struct NodePosition { float x, y, z; };
class ReverbEngine /* : rt::RealtimeThread */ {
public:
    void prepare (double sr, int internalBlockSize, int numNodes);
    void setAlgorithm (int type /*SDN|FDN|IR*/);
    void setAlgorithmParameters (const AlgorithmParameters&);
    void updateGeometry (const std::vector<NodePosition>&);  // SDN uses it; FDN no-op
    void setDeviceIds (std::string ir, std::string fdn, std::string sdn);
    void pushNodeInput (int node, const float* src, int n) noexcept;  // feed thread
    int  pullNodeOutput (int node, float* dst, int n) noexcept;       // audio callback; zero-pads on underrun
};
} // namespace spatcore::reverb
```

**Ownership summary:** rings, delay lines, GPU device buffers, and reverb node state are all
owned by the CORE object that allocates them in `prepare()` (not RT-safe) and frees them in
`release()`/dtor. Routing **matrices are owned by the caller** (the app broker) and passed as raw
`const float*`; CORE never writes them. Every `processBlock`/`push`/`pull`/`pump` entry point is
`noexcept` and allocation-free.

---

## 3. Dependency-direction rules & enforcement

**Rule:** `spatcore` must never include, reference, or name anything from an app
(`Parameters/`, `Network/`, `gui/`, `MainComponent`, `WFSValueTreeState`, `WFSParameterIDs`,
`OSCManager`, `MCP*`). Dependencies point **app → spatcore** only, never back.

Enforcement:
1. **Directory / target split.** `spatcore/` is a separate source root built as its own CMake
   target (`spatcore` static lib). Apps (`wfs-diy`, `xoa`, `tight-wfs`) are targets that
   `target_link_libraries(<app> PRIVATE spatcore)`. `spatcore`'s target has **no** include path
   into any app tree.
2. **Allowed deps for `spatcore`:** JUCE modules it genuinely needs (`juce_audio_basics`,
   `juce_dsp`, `juce_core`) + vendor GPU headers (behind the plugin boundary). Nothing else.
3. **CI guard:** a grep/lint step fails the build if any file under `spatcore/` matches
   `#include ".*(Parameters|Network|gui|MainComponent|WFSValueTreeState|WFSParameterIDs)`.
   (Today this would already pass for the CORE list in §1.1 except for the constants dependency
   noted in §4.)
4. **No app enums in CORE signatures.** Where CORE currently references app enum *values* by
   comment (e.g. `ReverbEngine.h:265` "matching `WFSParameterIDs::reverbAlgoType`"), replace with
   a `spatcore` enum the app maps onto.

---

## 4. The constants problem (`64/128/32`)

`WFSParameterDefaults.h` is **APP**, but CORE currently `#include`s it
(`WFSCalculationEngine.cpp:13-15`, `SdnHostConfig.h:36` references `maxReverbChannels`). To keep
the dependency direction clean:

- Pass `maxInputs/maxOutputs/maxNodes` as **runtime ctor/`prepare()` args** to CORE, defaulted by
  the app from `WFSParameterDefaults.h`.
- The GPU SDN kernel scratch `float incoming[32]` (`CudaSdnKernels.h:93`, `MetalSdnKernels.h:120`)
  is a **compile-time** array — it must stay a `spatcore` compile-time constant
  (`spatcore::gpu::kMaxSdnNodes`) that the app must not exceed; document it as a CORE capability
  bound, not an app knob.
- XOA (10th-order Ambisonics → 121 SH channels) and Tight-WFS (dense arrays → hundreds of
  outputs) will stress these bounds differently; making them CORE runtime parameters is a
  prerequisite for both.

---

## 5. Migration risk list & validation plan

### 5.1 Risks when WFS-DIY relinks against extracted `spatcore`

| # | Risk | Mitigation |
|---|---|---|
| R1 | **Bit-exact regression** in the CPU delay-sum (linear interp, scatter accumulation order) or reverb (FDN `×0.25`/`×4.0` gain staging, IR `0.125/√E` normalization, SDN Householder order). These constants are load-bearing for CPU↔GPU parity (`ReverbIRAlgorithmGPU.h:245-256` documents an 18 dB error caught by ear). | Move files **verbatim** (no reformat, no "cleanup"); lock behaviour with the golden-file test in §5.2 before and after. |
| R2 | **GPU kernel-source drift.** HIP/CUDA share the `CudaWfsKernels.h` string; Metal is a hand-kept mirror. A move that reorders includes or namespaces the string could change PTX. | Keep the kernel `.h` strings byte-identical; assert the compiled-PTX hash or re-run `tools/test-gpu-plugin.cpp` (all 5 families) post-move. |
| R3 | **Single-reader-thread invariant** on the matrices. CORE processors read raw `const float*` with no lock; today safety rests on the app broker copying on the timer thread before the audio thread reads. A relink that changes who writes/reads the matrices could introduce a real race. | Make the invariant explicit in the CORE API contract (§2) and keep the app as the sole matrix writer; do not "helpfully" add locking inside CORE. |
| R4 | **Latency compensation split** (`−L` currently applied inside the WFS backend, `CudaWfsBackend.cpp:382`). Moving the policy without moving the arithmetic identically shifts delays. | Move as a single unit; verify with a delay-impulse test that the compensated arrival time is unchanged. |
| R5 | **Constants become runtime args.** If a default is dropped, GPU scratch overflow (`incoming[32]`) is silent. | Add a `prepare()`-time assertion `numNodes <= kMaxSdnNodes`. |
| R6 | **Build-flag surface.** `WFS_GPU_NATIVE`/`WFS_GPU_PLUGINS`/`WFS_GPU_HIP` gating (`WFS-DIY.jucer:525-559`) must be reproduced on the `spatcore` target and the per-vendor plugin builds (`tools/*/build-gpu-plugins.*`). | Port the jucer/CMake defines to `spatcore`'s target and keep the plugin build scripts pointing at `spatcore` sources. |
| R7 | **Denormals.** CORE has no FTZ/DAZ today; if a future `spatcore` build flag (`-ffast-math`) differs from WFS-DIY's, recursive biquad/reverb output could change. | Freeze the FP flags on the `spatcore` target to match WFS-DIY; consider adding an explicit FTZ/DAZ guard as a separate, tested change (not part of the move). |

### 5.2 Validation plan — can we do bit-exact replay?

**Gold standard:** a deterministic reference-session replay that compares output sample-for-sample
before vs after extraction.

- **Feasible for the CPU direct path.** It is deterministic given identical inputs + identical
  `target*` matrices: the algorithms are per-sample arithmetic with no RNG on the audio path
  (the FR "diffusion grain" is a *block-stepped, deterministic ramp*, not `rand()` —
  `OutputBufferProcessor.h:545-551`). A harness that (1) feeds a fixed input WAV through the
  offline-renderable algorithm with a captured matrix timeline, and (2) diffs the output, gives a
  bit-exact gate.
- **What's missing for turnkey replay:** the engine has **no offline/deterministic-render mode**
  today — it is driven by the live device callback and a wall-clock 50 Hz timer, and control
  input arrives over OSC/tracking asynchronously. To make replay deterministic you need to add
  (in the app, not CORE): a way to (a) drive `getNextAudioBlock` from a file at a fixed block
  size, and (b) replay the `target*` matrix timeline from a captured log rather than the live
  timer. Neither exists; both are small app-side harnesses.
- **Reverb parity** is already partly tooled: the CPU SDN runs a synchronous per-sample lockstep
  *specifically to match the GPU bit-for-bit* (`ReverbSDNAlgorithm.h:126-130`), and
  `tools/test-gpu-plugin.cpp` exercises all 5 GPU families. Extend that tool into the CORE
  regression suite.
- **Practical gate if bit-exact replay isn't stood up first:** (1) run `tools/test-gpu-plugin.cpp`
  (all families) pre/post; (2) a null test — sum the pre-extraction and phase-inverted
  post-extraction offline renders of a fixed session and assert the residual is ≤ the LSB floor;
  (3) manual A/B on a reference session for the reverb tail (the historically error-prone path).

**Recommendation:** build the deterministic offline-render + matrix-replay harness *before*
starting the extraction, so R1/R4 are caught automatically rather than by ear.
