# macOS Performance-Core Scheduling for Audio Threads

Standalone reference for why WFS-DIY's heavy DSP was running on the **efficiency cores** of
Apple Silicon Macs, and how it was fixed. Written so the topic can be revisited without
re-deriving everything.

---

## 1. Symptom

A colleague ran WFS-DIY on a **10-core Apple Silicon Mac** (2 efficiency + 8 performance cores)
during a concert. Activity Monitor -> Window -> **CPU History** showed:

- The **2 efficiency cores pinned** near 100%.
- The **8 performance cores nearly idle**.

The same behaviour appeared with **two unrelated audio backends**:

- **DVS** (Dante Virtual Soundcard) — pure software.
- **RME Digiface Dante** — hardware USB interface.

Because the symptom was identical across both backends, **the audio driver is not the cause**.
The one constant across both setups is WFS-DIY's own DSP worker threads.

---

## 2. Root cause

On Apple Silicon, the kernel decides core placement (P-core vs E-core) primarily from a thread's
**QoS class**, *not* from its raw scheduling priority. The relevant facts:

- **The CoreAudio I/O thread is not the problem.** JUCE's CoreAudio device gives the audio
  callback thread (`MainComponent::getNextAudioBlock`) a realtime time-constraint policy
  automatically, so it lands on a P-core. But in WFS-DIY that thread does little real work — it
  patches input, signals the worker threads, and returns. The heavy lifting happens elsewhere.

- **The heavy DSP runs on worker threads started with `juce::Thread::Priority::high`.** In
  JUCE 8.0.13, `getNativeQOS()`
  (`ThirdParty/JUCE/modules/juce_core/native/juce_SharedCode_posix.h`) maps:

  | `juce::Thread::Priority` | macOS QoS class             |
  |--------------------------|-----------------------------|
  | `highest`                | `QOS_CLASS_USER_INTERACTIVE`|
  | **`high`**               | **`QOS_CLASS_USER_INITIATED`** |
  | `normal`                 | `QOS_CLASS_DEFAULT`         |
  | `low`                    | `QOS_CLASS_UTILITY`         |
  | `background`             | `QOS_CLASS_BACKGROUND`      |

  `QOS_CLASS_USER_INITIATED` is **eligible to run on efficiency cores**. The scheduler is
  perfectly happy to park frequently-waking `USER_INITIATED` threads on the E-cores, which is
  exactly what happened.

- **The `AudioParallelFor` fork-join pool had no QoS at all.** Its workers are raw `std::thread`
  (`Source/DSP/AudioParallelFor.h`), so they defaulted to `QOS_CLASS_DEFAULT` — also E-core
  eligible.

- **Windows already handled this; macOS did not.** `Source/Main.cpp` raises the process to
  `HIGH_PRIORITY_CLASS` and opts out of EcoQoS / Efficiency-Mode throttling on Windows. There was
  no macOS equivalent.

**Net effect:** the CoreAudio I/O thread sat on a P-core doing almost nothing, while the actual
WFS summation / reverb / binaural DSP ran on `USER_INITIATED` worker threads that the scheduler
placed on the 2 E-cores — saturating them while 8 P-cores idled.

---

## 3. Fix implemented

The fix upgrades every heavy DSP worker thread to a **realtime time-constraint policy**, which on
macOS forces P-core placement (via `THREAD_TIME_CONSTRAINT_POLICY` / `thread_policy_set`).

### 3a. `juce::Thread`-based workers -> `startRealtimeThread`

JUCE 8.0.13 exposes `Thread::startRealtimeThread(const RealtimeOptions&)`
(`ThirdParty/JUCE/modules/juce_core/threads/juce_Thread.h`), implemented for macOS in
`native/juce_SharedCode_posix.h` (`tryToUpgradeCurrentThreadToRealtime`). Each start site was
changed from:

    startThread (juce::Thread::Priority::high);

to:

    startRealtimeThread (juce::Thread::RealtimeOptions{}
                             .withApproximateAudioProcessingTime (blockSize, sampleRate));

`withApproximateAudioProcessingTime(samplesPerFrame, sampleRate)` sets the time-constraint period
and computation budget to one audio block, which is what each worker processes per wake. On
Windows/Linux `startRealtimeThread` keeps the existing realtime fallback, so behaviour off macOS
is unchanged.

Touched worker start sites (all had `blockSize`/`sampleRate` or member equivalents in scope):

- `Source/DSP/InputBufferAlgorithm.h` — `InputBufferProcessor` (per input), in `prepare` and `reprepare`.
- `Source/DSP/OutputBufferAlgorithm.h` — `OutputBufferProcessor` (per output), in `prepare` and `reprepare`.
- `Source/DSP/ReverbEngine.h` — `ReverbEngine` thread, in `startProcessing` and `setNumNodes` (uses members `currentBlockSize`/`sampleRate`).
- `Source/DSP/BinauralProcessor.h` — `BinauralProcessor` thread, in `startProcessing` and `setNumInputChannels`.
- `Source/MainComponent.cpp` — `ReverbFeedThread`, in `startAudioEngine`.

**Intentionally left at `Priority::normal`:** `InputAnalysisThread` and `OutputMeteringThread`
(`Source/DSP/OutputBufferAlgorithm.h`). These are metering/analysis, not deadline-critical, and
should stay off the P-cores.

### 3b. `AudioParallelFor` `std::thread` pool -> manual time-constraint

The pool can't use the JUCE API (raw `std::thread`), so a small macOS-only helper was added:

- **`Source/DSP/RealtimeThreadUtil.h`** (new) — `setCurrentThreadRealtimeAudio(periodMs, computationMs)`.
  On macOS it fills a `thread_time_constraint_policy_data_t` and calls `thread_policy_set(...)`,
  mirroring JUCE's own `tryToUpgradeCurrentThreadToRealtime` (including the retry-with-50ms fallback
  on `KERN_INVALID_ARGUMENT`). On every other platform it is a no-op. Mach/pthread includes are
  isolated to this one header.

- **`Source/DSP/AudioParallelFor.h`** — `prepare(int numWorkers, double periodMs = 0.0, double computationMs = 0.0)`
  (defaults keep older callers compiling). Each worker calls `setCurrentThreadRealtimeAudio(...)`
  once at the top of `workerLoop()` when `periodMs > 0`. The calling thread that also participates
  in the fork-join is the `ReverbEngine` thread, which is itself realtime via 3a — so the whole
  group is realtime.

- **`Source/DSP/ReverbEngine.h`** — `prepare` computes the block duration from `internalBlockSize`
  and `sampleRate` and passes it into `parallelPool.prepare(maxWorkers, blockMs, blockMs)`. This is
  the only `AudioParallelFor` instance in the codebase.

### 3c. Windows

No functional change. `Main.cpp` keeps the `HIGH_PRIORITY_CLASS` + EcoQoS opt-out + `timeBeginPeriod(1)`
block. `startRealtimeThread` and the `RealtimeThreadUtil.h` no-op compile and behave exactly as before.

### Build / packaging notes

`RealtimeThreadUtil.h` is header-only and `#include`d where needed, so **no Projucer / `.jucer`
regeneration is required**. The change builds clean on Windows (Debug x64).

---

## 4. Verification (macOS only)

This is an Apple Silicon scheduling issue and **cannot be observed on the Windows dev machine**.
Verify on an Apple Silicon Mac:

1. Build the macOS target and run WFS-DIY with a representative project and audio device (DVS or
   the RME Digiface Dante).
2. Open **Activity Monitor -> Window -> CPU History** (the same view as the original screenshot).
   With the fix, load should shift onto the **performance cores**; the 2 efficiency cores should
   no longer be the pinned ones under steady DSP load.
3. For a precise check, attach **Instruments** (or `sample <pid>` / the Time Profiler) and confirm
   the `InputBufferProcessor` / `OutputBufferProcessor` / `ReverbEngine` / `AudioParallelFor`
   threads now report a realtime / time-constraint scheduling policy and run on P-cores.
4. Listen for glitches/dropouts under the show's worst-case input + reverb load to confirm the
   realtime budgets are adequate and there is no audio regression.
5. Sanity-build the Windows target to confirm `startRealtimeThread` + the no-op helper compile and
   behave exactly as before.

---

## 5. Known risk and future options

### Oversubscription (the thing to watch)

`OutputBufferProcessor` is **one worker thread per output (speaker) channel**. On a large WFS rig
this can be dozens of threads, now all promoted to realtime time-constraint, on a machine with only
8 P-cores. The macOS kernel can **demote** realtime threads (back to a normal policy / E-cores) when
the aggregate declared computation exceeds available capacity.

- `withApproximateAudioProcessingTime` keeps each thread's declared computation honest (one block),
  which helps.
- If the Mac test shows glitches or threads falling back to E-cores under high output counts, this
  is the knob to revisit. Do **not** pre-emptively change it without hardware data.

### Deferred enhancements (not implemented)

- **`os_workgroup` membership.** Apple's "blessed" model: join the worker threads to the CoreAudio
  device's audio workgroup so the scheduler treats them as one coordinated realtime workload. More
  robust than raw per-thread time-constraint under heavy load, but requires capturing the workgroup
  from the audio callback context and having each worker join/leave. JUCE 8 exposes `AudioWorkgroup`.
- **Adaptive promotion.** When the output-channel count is very high, promote only a subset of
  threads (or scale the per-thread budget) instead of promoting every per-output thread. Speculative
  — needs the oversubscription data above first.

---

## References

- JUCE QoS mapping: `ThirdParty/JUCE/modules/juce_core/native/juce_SharedCode_posix.h` (`getNativeQOS`, `tryToUpgradeCurrentThreadToRealtime`).
- JUCE realtime API: `ThirdParty/JUCE/modules/juce_core/threads/juce_Thread.h` (`startRealtimeThread`, `RealtimeOptions`).
- WFS-DIY helper: `Source/DSP/RealtimeThreadUtil.h`.
- Windows equivalent: `Source/Main.cpp` (`HIGH_PRIORITY_CLASS`, `PROCESS_POWER_THROTTLING_STATE`, `timeBeginPeriod`).
