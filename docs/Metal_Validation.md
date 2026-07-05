# macOS + Metal Validation Runbook

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

## Follow-up (not in this pass)
Byte-exact Metal offline validation (the 15/15 GPU baseline gate the CUDA/HIP boxes
run) needs the `tools/validation/offline-render` CMake extended to compile the
Metal `.mm` + link the Metal/Foundation/QuartzCore frameworks on Apple (today the
harness uses the Windows/Linux plugin-dlopen path). If Part B compiles clean and
you want bit-exact Metal numbers, ask and I'll add that Apple path to the harness
for a follow-up run.
