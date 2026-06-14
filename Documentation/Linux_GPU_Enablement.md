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

**Why one exporter can't hold both:** Projucer's `LINUX_MAKE` has a single flag set, so the
committed config is either HIP or CUDA. Until Phase 3 (runtime dlopen of either vendor in one
binary), switch by editing the `LINUX_MAKE` `defines`/`headerPath`/`libraryPath`/`extraLinkerFlags`
to the block above and re-saving.

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
