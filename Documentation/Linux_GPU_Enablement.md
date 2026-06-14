# Linux GPU (CUDA) Enablement — Handover

**Audience:** an agent working directly on Ubuntu to turn on native GPU acceleration for the Linux build, testing incrementally on real NVIDIA+CUDA hardware.

**Status of this doc:** findings from a Windows-side audit (branch `gpu-native`). The Linux build currently ships **CPU-only by design**. Nothing here has been applied to the Linux exporter yet — that's your job.

---

## TL;DR

- The Linux build has GPU acceleration **fully off, by design** — it is not broken. `WFS_GPU_NATIVE` is defined for the Windows (`VS2022`) and macOS (`XCODE_MAC`) exporters but **not** for `LINUX_MAKE`, so every CUDA backend compiles to an empty object file and the app runs the CPU reference DSP path.
- **The CUDA host code is portable.** It uses only the standard CUDA Driver + Runtime + NVRTC APIs (identical headers on Linux) and C++17. Kernels are NVRTC **string literals** (no `nvcc`, no `.cu` files). The audit found **no Windows-only host code** — enabling GPU on Linux is **build/packaging wiring, not a source rewrite**.
- The **one** source-adjacent snag: the CUDA import libs are linked via MSVC-only `#pragma comment(lib, ...)` (`#if defined(_MSC_VER)`), so on Linux you must pass `-lcudart -lnvrtc -lcuda` to the linker yourself.

---

## How the gating works (so you know what flips on)

- Backend selection — [Source/DSP/gpu/WfsGpuBackend.h](../Source/DSP/gpu/WfsGpuBackend.h): `#if defined(__APPLE__)` -> Metal, `#else` -> `CudaWfsBackend`. Linux is the `#else` branch, so it already aliases the CUDA backend (this header is JUCE-free and CUDA-include-free, so it compiles regardless).
- Every CUDA `.cpp` is wrapped in `#if WFS_GPU_NATIVE && !defined(__APPLE__)` and only pulls in `cuda.h` / `cuda_runtime.h` / `nvrtc.h` inside that guard. Empty TU when the macro is undefined:
  - `Source/DSP/gpu/CudaWfsBackend.cpp` (line 27), `CudaObBackend.cpp` (29), `CudaSdnBackend.cpp` (20), `CudaFdnBackend.cpp` (19), `CudaIrBackend.cpp` (28).
  - The MSVC-only link pragmas live right after each guard, e.g. [CudaWfsBackend.cpp:41-45](../Source/DSP/gpu/CudaWfsBackend.cpp#L41).
- GPU members/algorithms are `#if WFS_GPU_NATIVE`'d out in `MainComponent.h/.cpp`, `ReverbEngine.h`, `LevelMeteringManager.h`. So defining the macro is the single switch that turns the whole layer on.
- The `.mm` Metal files are macOS-only and are not compiled by the Linux Makefile — ignore them.

---

## Enablement plan (incremental, test at each step)

### Step 0 — Baseline
Confirm a clean CPU-only build still works before changing anything:
```
cd Builds/LinuxMakefile && make clean && make CONFIG=Debug -j$(nproc)
```
Run it; verify normal CPU operation. This is your rollback reference.

### Step 1 — Edit the `LINUX_MAKE` exporter in `WFS-DIY.jucer`
Edit both the Debug and Release `<CONFIGURATION>` blocks under `<LINUX_MAKE>` (currently [WFS-DIY.jucer:493-498](../WFS-DIY.jucer#L493)). Mirror what `VS2022` already does ([lines 522-531](../WFS-DIY.jucer#L522)):

- **defines** — add `WFS_GPU_NATIVE=1`. Current Linux value: `SIMPLEWEB_SECURE_SUPPORTED=0 JUCE_JACK=1` -> `SIMPLEWEB_SECURE_SUPPORTED=0 JUCE_JACK=1 WFS_GPU_NATIVE=1`.
- **headerPath** — append the CUDA include dir, e.g. `/usr/local/cuda/include` (adjust to your toolkit, e.g. `/usr/lib/cuda/include` on distro packages).
- **externalLibraries** — add `cudart;nvrtc;cuda` (Projucer emits these as `-lcudart -lnvrtc -lcuda` into the Makefile's `JUCE_LDFLAGS`).
- **libraryPath** — add the CUDA lib dir, e.g. `/usr/local/cuda/lib64`. The `libcuda` **driver stub** is usually under `/usr/local/cuda/lib64/stubs` — you may need that on the link search path too (at runtime the real `libcuda.so.1` is provided by the installed NVIDIA driver, not the toolkit).

Note on `externalLibraries`: on **Windows** this field does NOT reach the VS linker (hence the source pragmas) — but on **Linux** Projucer does turn it into `-l` flags. If it doesn't take for some reason, fall back to `extraLinkerFlags` with the explicit `-L.../lib64 -L.../lib64/stubs -lcudart -lnvrtc -lcuda`.

### Step 2 — Regenerate the Makefile
After editing the `.jucer`, resave with Projucer (do **not** hand-edit the generated Makefile — it gets overwritten):
```
/path/to/Projucer --resave WFS-DIY.jucer
```
Confirm the generated `Builds/LinuxMakefile/Makefile` now carries `-DWFS_GPU_NATIVE=1` in `JUCE_CPPFLAGS` and the CUDA `-I`, `-L`, `-l` flags in `JUCE_LDFLAGS`.

### Step 3 — Build and resolve link errors
```
cd Builds/LinuxMakefile && make clean && make CONFIG=Debug -j$(nproc)
```
Expected first failure if linkage is incomplete: unresolved `cuLaunchKernel` / `cudaMalloc` / `nvrtc*` symbols (the MSVC pragmas do nothing on GCC/Clang). Fix by ensuring `-lcudart -lnvrtc -lcuda` and the `-L` dirs (including `stubs` for `libcuda`) are present. No source changes should be required.

### Step 4 — Runtime: make the `.so`s findable, then exercise the GPU path
- Ensure the loader can find `libcudart.so`, `libnvrtc.so`, `libnvrtc-builtins.so` at runtime (system CUDA install + `ldconfig`, or `LD_LIBRARY_PATH=/usr/local/cuda/lib64`). `libcuda.so.1` comes from the NVIDIA driver.
- **Turn the GPU on in the app and confirm it activates** (the GPU options only exist in a `WFS_GPU_NATIVE` build):
  - Main WFS GPU: select the GPU processing algorithm (`ProcessingAlgorithm::NativeGpuWfs`, see [MainComponent.cpp:4350](../Source/MainComponent.cpp#L4350)). On success the log shows `Native GPU WFS active: <device> ...` ([~4394](../Source/MainComponent.cpp#L4394)) / `Native GPU OutputBuffer active: ...` ([~4447](../Source/MainComponent.cpp#L4447)). There's a `GpuPipelineDepth` config param.
  - Reverb GPU: the per-algorithm toggles `reverbSDNGpu` / `reverbFDNGpu` / `reverbIRGpu` (default 0 = CPU) in System Config / Reverb. Success logs e.g. `GPU SDN reverb active: <N> nodes`.
  - On failure, the backends report via `getLastError()` and the app falls back to CPU — check the Network Log / WFSLogger output.
- A/B the GPU vs CPU path for audible parity (the CUDA backends are written to mirror the Metal twin and the CPU reference bit-for-bit).

### Step 5 — Packaging (optional, for shipping)
The release tarball script [tools/linux/build-app-tarball.sh](../tools/linux/build-app-tarball.sh) copies only the binary + `lang/` + `MCP/resources/` ([lines 62-65](../tools/linux/build-app-tarball.sh#L62)) — it does **not** ship any CUDA runtime. Decide between:
- **System CUDA dependency** (simplest): document that users need a matching CUDA toolkit / driver; ship nothing extra.
- **Bundle the runtime**: copy `libcudart.so*`, `libnvrtc.so*`, `libnvrtc-builtins.so*` into the stage dir, install them into `$APP_DIR`, and set an `rpath` (`$ORIGIN`) on the binary or wrap launch with `LD_LIBRARY_PATH`. Do **not** bundle `libcuda.so` (driver-owned; must come from the host's NVIDIA driver). Watch versioned soname matching.

The Windows equivalent is the `VS2022` postbuild DLL copy ([WFS-DIY.jucer:527](../WFS-DIY.jucer#L527)).

---

## Canonical CUDA paths (Ubuntu)
- Toolkit install (`.run` / NVIDIA repo): `/usr/local/cuda/{include,lib64}`, driver stub at `/usr/local/cuda/lib64/stubs/libcuda.so`.
- Distro package (`nvidia-cuda-toolkit`): headers/libs under `/usr/lib/cuda` or `/usr/include` + `/usr/lib/x86_64-linux-gnu`. Adjust the `.jucer` paths accordingly.

---

## Unrelated Linux build-parity gaps (noticed during the audit; not GPU)
- **MCP tool generation is Windows-only.** `VS2022` has a prebuild `python tools/generate_mcp_tools.py` ([WFS-DIY.jucer:526](../WFS-DIY.jucer#L526)); `LINUX_MAKE` has no prebuild hook, so `Source/Network/MCP/generated_tools.json` is not regenerated during a Linux build. The committed JSON is used as-is. Consider adding a Linux prebuild step (or run the generator manually after CSV edits — `generated_tools.json` is loaded at runtime from the source tree).
- **Resource copying** (`lang/`, `MCP/resources/`) is per-platform; Linux relies on `build-app-tarball.sh` rather than a Makefile postbuild. Existing behavior, just be aware.
- **No Linux GPU docs / setup detection.** `README.md` documents the Linux build but has no CUDA prerequisites; `tools/setup.sh` does the JUCE multitouch patch + submodules but no CUDA detection. Add notes once GPU works.

---

## Key files
| File | Role |
|---|---|
| [WFS-DIY.jucer](../WFS-DIY.jucer) | `LINUX_MAKE` exporter (lines ~490-519) — the changes above go here, then resave |
| [Source/DSP/gpu/WfsGpuBackend.h](../Source/DSP/gpu/WfsGpuBackend.h) | compile-time Metal/CUDA selection |
| `Source/DSP/gpu/Cuda*Backend.cpp` | the 5 CUDA backends (WFS, OutputBuffer, SDN, FDN, IR) — guarded + MSVC-only link pragmas |
| [Source/MainComponent.cpp](../Source/MainComponent.cpp) | `#if WFS_GPU_NATIVE` GPU algorithm wiring + activation logs (~4350-4530) |
| [tools/linux/build-app-tarball.sh](../tools/linux/build-app-tarball.sh) | Linux release packaging (add `.so` bundling here if shipping GPU) |
