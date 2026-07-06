# macOS + Metal Validation Runbook

**Status: Part B done** (build + in-app CPU/GPU power characterization on a Mac
mini M4 Pro, 2026-07-06/07 — see **Results** at the end). **Part A** (spatcore
host tests + ThreadSanitizer) **not yet run**.

Goal: validate the two things that have never been exercised on a Mac —
(1) the **spatcore host code under a real macOS scheduler + ThreadSanitizer**
(the `GpuHostWorkPool` fork-join pool, `RtThreadPriority`'s mach branch), and
(2) the **Metal GPU backends**, whose M2 (upload diet) and M3 (parallel host
prep) changes have so far only been mirrored blind — never compiled, since there
is no Mac on the dev bench. Compiling them is itself a milestone (catches any
ObjC++/ARC error); running them validates the Metal compute path.

Target: the Mac mini. Test against **`main`** (M1+M2+M3 all merged).

```
git clone --recurse-submodules <repo-url> && cd WFS_DIY_v1
git checkout main && git pull
```

Prereqs: Xcode + command-line tools (`xcodebuild`, `clang++`), CMake
(`brew install cmake`).

---

## Part A — spatcore host tests + ThreadSanitizer (highest value, do first)

The `GpuHostWorkPool` had two concurrency bugs the audit caught and fixed; they
could not be reproduced under MSVC on the NVIDIA box (no TSan there). **clang on
macOS has TSan** — this is the deterministic race check we couldn't run.

### A1 — plain build + run (RtThreadPriority mach branch + pool stress)
```bash
cmake -S spatcore/tests/standalone -B build/spatcore-mac -G Xcode
cmake --build build/spatcore-mac --config Release
build/spatcore-mac/spatcore/tests/spatcore-tests_artefacts/Release/spatcore-tests
echo "exit: $?"        # 0 = all pass
```
This is the first compile+run of `RtThreadPriority.h`'s macOS `thread_time_
constraint_policy` branch, and it runs the pool determinism + cross-generation +
re-prepare stress tests under the macOS scheduler (different from Windows — extra
race exposure).

### A2 — ThreadSanitizer run (the deterministic race check)
```bash
cmake -S spatcore/tests/standalone -B build/spatcore-tsan \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=thread -g -O1" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread"
cmake --build build/spatcore-tsan
./build/spatcore-tsan/spatcore/tests/spatcore-tests_artefacts/spatcore-tests
echo "exit: $?"
```
**Pass = exit 0 with NO `WARNING: ThreadSanitizer: data race` lines.** The pool
stress tests hammer the fork-join pool (oversubscribed workers, tight back-to-back
generations, 500 re-prepare cycles); TSan instruments every access, so a residual
race in the pool would be reported deterministically here even if it never crashes.
This is the strongest available confirmation that the two audit fixes are complete.
If TSan flags anything, capture the full report verbatim — that's a real finding.

(If the Xcode generator fights TSan, use `-G Ninja` or plain Makefiles instead;
the flags are what matter. A few JUCE-internal TSan notes in module code can be
ignored — we care about `GpuHostWorkPool.h` / `RtThreadPriority.h` frames.)

---

## Part B — Metal app build + in-app GPU pass

On macOS the Metal backends compile **into the app** (no plugin). Building the app
is the first compilation of all the M2+M3 Metal mirror code.

### B1 — build the app
```bash
# The committed Xcode project; resave from Projucer first only if it is stale.
xcodebuild -project Builds/MacOSX/WFS-DIY.xcodeproj -configuration Release \
  -scheme "WFS-DIY - App" build
```
Or open `Builds/MacOSX/WFS-DIY.xcodeproj` in Xcode and build. **A clean build is
the milestone** — it proves the Metal `.mm` upload-diet (M2) and parallel-prep
(M3) mirrors compile under ARC (the `id<MTLBuffer> slot[2]` ping-pong members, the
`GpuHostWorkPool` member, the `RtThreadPriority` call in the worker path). If it
fails to compile, that is the expected-and-useful finding — send the exact errors;
they're mechanical mirror fixes.

### B2 — in-app GPU pass
Launch the app, System Config → WFS Processor: select the Metal device, choose the
GPU WFS renderer, play audio — confirm output and stable levels. Reverb tab →
Algorithm: run IR / FDN / SDN on the GPU. Open the Level Meter window and watch the
GPU pipeline strip (pump ms vs budget, underruns). Leave it ~10 min; note any
dropouts.

---

## Report back
- Part A: A1 pass/fail; **A2 TSan result** (clean or the race report verbatim).
- Part B: **does the app compile?** (the key Metal-mirror question); in-app GPU
  observations (output OK, underruns, wattage if available).

---

## Results — Part B: build + in-app CPU/GPU power characterization (M4 Pro)

### Environment

| Item | Value |
| --- | --- |
| Machine | Mac mini, Apple **M4 Pro**, 24 GB |
| macOS | 15.7.7 (24G720) |
| Xcode | 26.3 |
| JUCE | 8.0.14 |
| App | `1.0.0beta26` |
| Code | `spatcore/phase-5-engine-seams` @ `66aa49c` |
| Audio I/O | RME Fireface (USB); TotalMix FX + Dante Controller running alongside |
| Test signal | WFS 32 in × 32 out (Metal input buffer), SDN 32-node reverb (Metal), 96 kHz |

### B1 — build

`xcodebuild -project Builds/MacOSX/WFS-DIY.xcodeproj -scheme "WFS-DIY - App" -configuration Release clean build`
→ **BUILD SUCCEEDED**, 0 errors, 0 warnings. Confirms the M2/M3 Metal mirror code
(upload-diet ping-pong buffers, `GpuHostWorkPool`-driven parallel host prep)
compiles clean under ARC on real hardware — the same clean result held across
several rebuilds this session on `gpu-host/m3-parallel-prep`, `main`, and
`spatcore/phase-5-engine-seams`. Binary confirmed `arm64`, links
`Metal.framework` + `MetalKit.framework` (weak), `WFS_GPU_NATIVE=1` set in the
Release config.

### B2 — in-app GPU pass: CPU/GPU power characterization

Measured with `sudo powermetrics --samplers cpu_power,gpu_power(,tasks) -i 1000
-n ~10` while the app processed live audio through the RME interface. Five
configs tested, varying host buffer size and which stage (WFS input-buffer
gather/scatter vs. SDN reverb) ran on CPU vs. Metal:

| # | Buffer | Input buffer | SDN reverb | CPU Power | GPU Power | GPU active/idle | Audio |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | 64 samples | GPU (Metal) | GPU (Metal) | 1908 mW | 2009 mW | 100% / 0% | **noisy** |
| 2 | 128 samples† | GPU (Metal) | GPU (Metal) | 1543–1881 mW | 1850–1889 mW | 100% / 0% | **still scratchy** |
| 3 | 128 samples | CPU | GPU (Metal) | 5911 mW | 1873–1886 mW | 100% / 0% | not confirmed‡ |
| 4 | 128 samples | CPU | CPU | 6899 mW | 108 mW | 20% / 80% | **clean** |
| 5 | 64 samples | CPU | CPU | 7095–7207 mW | 96–122 mW | 18–24% / 76–82% | **clean** |

† Internal main-processing block count was halved (4→2) alongside the buffer
doubling, keeping total direct-sound buffering constant at 256 samples —
isolates host-buffer size from the deadline math.
‡ GPU numbers are indistinguishable from #2 (same 100%/0% pin, same ~1.85–1.9 W)
— strongly suggests still scratchy, but not directly confirmed by ear.

### Findings

1. **The SDN Metal reverb kernel's dispatch — not the WFS input-buffer
   gather/scatter, not host-buffer size — is what pins the GPU at exactly 100%
   active / 0% idle.** Configs 1–3 all show the identical GPU signature
   (100%/0%, ~1.85–2.0 W, parked at 1578 MHz / P10) regardless of buffer size
   or whether the input buffer shares the GPU. Zero idle residency means zero
   dispatch slack; the single-threadgroup, `mem_device`-barrier design used for
   SDN's coupled-node correctness appears to have a real-time viability ceiling
   at or below **32 nodes / 96 kHz / 64–128-sample buffers** on this M4 Pro
   GPU.
2. **Moving SDN to CPU drops GPU load to background levels** (96–122 mW,
   18–24% active — just WindowServer/compositor, not the app) and produces
   clean audio at **both** buffer sizes tested (configs 4 and 5, both
   confirmed).
3. **The CPU path has real headroom.** Halving the buffer 128→64 with
   everything on CPU increased combined power only ~3–4% (7.0 W → 7.2–7.3 W)
   despite doubling the callback rate, and stayed clean — unlike the GPU-SDN
   configs, which stayed pinned at 100% regardless of buffer size and never
   produced clean audio in any tested config.
4. **Unexplained anomaly:** `coreaudiod`'s own deadline-count scaled exactly
   with buffer size (750/s → 1503/s, matching 96000/128 and 96000/64
   precisely), but `WFS-DIY`'s self-reported `Deadlines(<2ms)` metric fell from
   ~25,186/s (config 4) to ~109/s (config 5) — the opposite direction physics
   would suggest. Not yet understood; flagged for follow-up rather than
   explained away.
5. **Practical implication:** for this node count/buffer/rate combination,
   GPU-accelerated SDN is not real-time-safe on this hardware today. Until the
   kernel's per-block barrier overhead is reduced, default to CPU SDN at this
   scale (or gate GPU SDN to lower node counts / larger buffers where headroom
   exists).

### Open items
- Audio-quality confirmation still pending for config 3 (CPU input + GPU SDN).
- Part A (spatcore host tests + ThreadSanitizer) not run this session — still
  open per the original runbook.
- The `Deadlines(<2ms)` anomaly (finding 4) is unexplained.

## Follow-up (not in this pass)
Byte-exact Metal offline validation (the 15/15 GPU baseline gate the CUDA/HIP boxes
run) needs the `tools/validation/offline-render` CMake extended to compile the
Metal `.mm` + link the Metal/Foundation/QuartzCore frameworks on Apple (today the
harness uses the Windows/Linux plugin-dlopen path). If Part B compiles clean and
you want bit-exact Metal numbers, ask and I'll add that Apple path to the harness
for a follow-up run.
