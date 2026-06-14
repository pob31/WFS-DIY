# Linux GPU Enablement — Status & Build Recipes

Native GPU compute (the Metal/macOS and CUDA/Windows backends) on **Linux**, for both
**AMD (ROCm/HIP)** and **NVIDIA (CUDA)**. The CPU reference DSP path remains authoritative;
every GPU backend mirrors it (and the Metal/CUDA twins).

---

## Status

| Target | Backend | State |
|---|---|---|
| Linux / AMD | HIP / ROCm | **Implemented + validated** on gfx1103 (Radeon 780M). Bit-exact WFS delay-and-sum; all 5 kernels hipRTC-compile + init on-device. |
| Linux / NVIDIA | CUDA | **Foundation laid (compile-time ready), unvalidated** — no NVIDIA GPU available to test. The host code is the unchanged Windows CUDA path; only build wiring differs. |
| Windows / NVIDIA | CUDA | Ships. Unchanged. |
| macOS | Metal | Ships. Unchanged. |

Backend selection is compile-time in the five `Source/DSP/gpu/*GpuBackend.h` aliases:
`#if __APPLE__` → Metal, `#elif defined(WFS_GPU_HIP)` → Hip, `#else` → Cuda. `WFS_GPU_NATIVE`
is the master switch the rest of the app keys on (`MainComponent`, `ReverbEngine.h`,
`LevelMeteringManager.h`). The runtime, per-role, multi-vendor device selector (dlopen-based,
CPU-fallback) is **Phase 3** — see the plan; it will supersede the compile-time vendor split.

---

## How the backends work (shared by CUDA and HIP)

- Kernels live as **string literals** in `Cuda*Kernels.h`, compiled **at runtime** in `prepare()`
  via NVRTC (CUDA) / hipRTC (HIP) — no `.cu`/`.hip` files, no `nvcc`/`hipcc` build step. The HIP
  backends **reuse the CUDA kernel strings verbatim** (CUDA-C is valid HIP; no atomics, no warp
  shuffles, no FFT lib — IR FFT is host-side in `IrHostFft.h`).
- Host TUs compile with **plain g++** (the JUCE Linux default); the GPU runtime is only linked,
  not used as a compiler.
- All nine shared host files (`GpuAsyncPipeline.h`, `NativeGpu*Algorithm.h`, `IrHostFft.h`,
  `*HostConfig.h`, `WfsFrHostState.h`, `GpuLevelMeters.h`) are backend-agnostic and reused as-is.

---

## A. AMD / HIP build (works today)

**Toolchain (Ubuntu 24.04, kernel 6.17):**
```bash
# AMD repo via amdgpu-install (check repo.radeon.com for the current version), then:
sudo amdgpu-install --usecase=rocm --no-dkms     # userspace only; in-tree amdgpu/KFD suffices
sudo usermod -aG render,video $USER              # re-login afterwards
```
- **No DKMS** — `amdgpu-dkms` does not build on kernel 6.15-6.17; the in-tree driver works.
- gfx1103 (Phoenix/780M) runs **natively** — `HSA_OVERRIDE_GFX_VERSION` is NOT needed. (If a
  future ROCm refuses it: `HSA_OVERRIDE_GFX_VERSION=11.0.0`.)

**`LINUX_MAKE` exporter config** (already committed as the current Linux variant):
- `defines`: `... WFS_GPU_NATIVE=1 WFS_GPU_HIP=1 __HIP_PLATFORM_AMD__=1`
  — `__HIP_PLATFORM_AMD__=1` is **required** when compiling HIP headers with g++ (hipcc injects it
  automatically; g++ does not, else `#error Must define exactly one of __HIP_PLATFORM_AMD__...`).
- `headerPath += /opt/rocm/include`, `libraryPath += /opt/rocm/lib`
- `extraLinkerFlags="-lamdhip64 -lhiprtc -Wl,-rpath,/opt/rocm/lib"`
  — the jucer `externalLibraries` field does **not** emit `-l` on Linux, so use `extraLinkerFlags`.

**Build:** `Projucer --resave WFS-DIY.jucer` then `cd Builds/LinuxMakefile && make CONFIG=Debug -j$(nproc)`.

**hipify note (for future kernel/backend changes):** `hipify-perl Cuda<X>Backend.cpp` does ~all of
it; manual fixes are the arch string (`--gpu-architecture=compute_NN` → `--offload-arch=` +
`prop.gcnArchName`), the PTX→code-object load (`hiprtcGetCode`/`hipModuleLoadData`), and dropping
the dead MSVC link pragmas. The `Hip*Backend.cpp` `#include` the existing `Cuda*Kernels.h`.

---

## B. NVIDIA / CUDA build on Linux (foundation — untested here)

The `#else → Cuda` branch already targets Linux; this is pure build/link wiring. **It cannot be
run/validated on this machine (no NVIDIA GPU).** A CUDA toolkit is required even to compile/link.

**Toolkit:** `sudo apt install cuda-toolkit` (a `cuda-ubuntu2204` apt repo is already configured;
for 24.04 prefer adding the `cuda-ubuntu2404` repo). Gives `/usr/local/cuda/{include,lib64}` and the
`libcuda` driver stub at `/usr/local/cuda/lib64/stubs/`.

**`LINUX_MAKE` exporter config (CUDA variant — mutually exclusive with the HIP variant above):**
- `defines`: `... WFS_GPU_NATIVE=1`  (NO `WFS_GPU_HIP`, NO `__HIP_PLATFORM_AMD__`)
- `headerPath += /usr/local/cuda/include`
- `libraryPath += /usr/local/cuda/lib64;/usr/local/cuda/lib64/stubs`
- `extraLinkerFlags="-lcudart -lnvrtc -lcuda -Wl,-rpath,/usr/local/cuda/lib64"`
  — the CUDA link pragmas in `Cuda*Backend.cpp` are `_MSC_VER`-only, so Linux must link explicitly.
  At runtime `libcuda.so.1` comes from the installed NVIDIA driver (not the toolkit); the `stubs`
  `libcuda.so` is link-time only.

**Build:** resave + `make` as above. Expected: host TUs compile with g++ (CUDA host APIs are plain
C++); first failure if linkage is incomplete is unresolved `cuLaunchKernel`/`nvrtc*`/`cudaMalloc`.

**Validated (2026-06-14):** all 5 `Cuda*Backend.cpp` compile with g++ against CUDA 13.3 headers, and
the **full app links** against `libcudart.so.13` / `libnvrtc.so.13` / `libcuda.so.1` on Linux. It
cannot be *run* here (no NVIDIA driver supplies `libcuda.so.1` at load time) — runtime needs NVIDIA
hardware.

> **Variant-switch gotcha:** when flipping the `LINUX_MAKE` config between the HIP and CUDA blocks,
> run `make clean` before rebuilding. A flags-only change does not change the source mtimes, so make
> will not recompile the `Cuda*`/`Hip*` TUs and the link will pull stale objects from the other
> variant (empty `Cuda*` objects, or `Hip*` objects referencing `hipModuleUnload`).

**Why one exporter can't hold both:** Projucer's `LINUX_MAKE` has a single flag set, so the
committed config is either HIP or CUDA. Until Phase 3 (runtime dlopen of either vendor in one
binary), switch by editing the `LINUX_MAKE` `defines`/`headerPath`/`libraryPath`/`extraLinkerFlags`
to the block above and re-saving.

---

## C. Windows — plugin parity (CUDA or HIP/ROCm), runtime-selected

Windows uses the **same per-vendor plugin model** as Linux so a single app can drive
CUDA *or* AMD HIP at runtime, with CPU fallback. The shared code is portable
(`Source/DSP/gpu/PlatformDynLib.h` wraps `dlopen`↔`LoadLibrary`,
`/proc/self/exe`↔`GetModuleFileName`); `GpuDeviceManager` enumerates via `nvcuda.dll`
(CUDA driver) and `amdhip64.dll` (AMD HIP SDK for Windows), and `GpuBackendFactory`
loads `wfs_cuda.dll` / `wfs_hip.dll` next to the executable.

**Build the plugin DLLs** (from a *x64 Native Tools / Developer PowerShell*):
```
tools\windows\build-gpu-plugins.ps1            # builds whichever of CUDA / HIP toolchains are present
```
- `wfs_cuda.dll` needs the CUDA Toolkit (`cl.exe` + `%CUDA_PATH%`).
- `wfs_hip.dll` needs the **AMD HIP SDK for Windows** (`hipcc`); run from its "HIP SDK"
  shell so `hipcc` resolves on `PATH`.
- **Exports** — unlike a Linux `.so` (ELF default visibility exports everything), a
  Windows DLL exports **nothing** by default. The C entry points are therefore marked
  `__declspec(dllexport)` (on `_WIN32`) in `GpuVendorPlugin.cpp`; without that,
  `GetProcAddress` returns null and the app **silently falls back to CPU**. The build
  script auto-verifies with `dumpbin /EXPORTS` — expect `wfs_plugin_create_{wfs,ob,ir,
  fdn,sdn}`, `wfs_plugin_vendor`, `wfs_plugin_destroy` (undecorated, since `extern "C"`).
- **Shared CRT** — the script forces the **dynamic CRT** (`/MD` for `cl.exe`,
  `-fms-runtime-lib=dll` for `hipcc`) so each plugin shares the app's heap. The factory
  destroys plugin-allocated backends with an app-side `delete`, which is safe only when
  app + plugin link the same dynamic UCRT (the JUCE VS2022 Release default is `/MD`; do
  not switch the app or plugins to `/MT`).

**Enable plugin mode in the VS2022 exporter** (Projucer, then re-save). Mirror the Linux
`LINUX_MAKE` change:
- `defines` += `WFS_GPU_PLUGINS=1` (keep `WFS_GPU_NATIVE=1`).
- Remove the CUDA `headerPath`/`libraryPath`/`externalLibraries` (`cudart.lib;nvrtc.lib;
  cuda.lib`) and the `/DELAYLOAD` linker flags + the CUDA-DLL post-build copy — the app
  now links **no** GPU runtime.
- Ship the `wfs_*.dll` plugins next to `WFS-DIY.exe` (Inno Setup `[Files]`).

**Status / caveats:**
- The shared/Windows code is in place and compile-tested **on Linux only** (no MSVC here).
  `GpuDeviceManager` Windows enumeration + the plugin DLL build + the VS2022 config flip
  must be validated on a real Windows box. **The committed VS2022 exporter is left on the
  current compile-time CUDA (delay-loaded) build** — flip it to plugin mode once you've
  built + tested the plugins on Windows, then commit the exporter change.
- The HIP-on-Windows DLL is `amdhip64.dll` (versioned fallback `amdhip64_6.dll`); confirm
  the name your AMD HIP SDK ships.
- Same-vendor multi-GPU device-index binding is still deferred (binds device 0).

macOS needs nothing: its GPU path is the in-process Metal backend (compile-time), and
`GpuDeviceManager` reports a single `Metal (default)` device.

---

## Open items
- In-app audio A/B (GPU vs CPU) for HIP — user-driven (GUI/audio).
- CUDA-on-Linux: actual compile/link check once a CUDA toolkit is installed; runtime untestable here.
- Phase 3: runtime device manager — abstract backend interfaces, `dlopen` the matching vendor
  runtime per selected device, CPU always fallback, per-role (direct vs reverb) device dropdowns,
  cross-platform (incl. Windows multi-GPU). This removes the compile-time vendor split and the
  "one exporter = one vendor" limitation above.
- Packaging: bundle `libamdhip64.so*`/`libhiprtc.so*` (or document a system ROCm dependency) in
  `tools/linux/build-app-tarball.sh`; never bundle the kernel-mode driver.
