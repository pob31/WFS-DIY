# Windows + AMD HIP Validation Runbook

**Status: VALIDATED** — ran on Windows 11 + ROCm 7.1 + Radeon 780M (gfx1103):
the smoke tool passed 7/7 (exit 0) and the app runs WFS + SDN reverb on the GPU
over live ASIO. See the **Results** section at the end.

Goal: functionally validate the **Windows** HIP GPU path (`wfs_hip.dll`, all 5 kernel
families) on real AMD hardware. Linux HIP is already hardware-tested (gfx1103,
ROCm 6.4); Windows HIP so far only builds and cleanly no-ops on the NVIDIA dev
machine. This closes the last untested cell of the vendor/platform matrix
(`docs/architecture/open-questions-audio.md` Q3) and provides the AMD-side GPU
baseline before the spatcore extraction starts.

Companion reference: `Documentation/GPU_Plugin_Smoke_Test.md` (scenarios, exit codes).

## Prerequisites on the AMD machine

1. **AMD HIP SDK for Windows** (ROCm 6.x or 7.x — 7.x supported since commit
   `24da65d`). The consumer Adrenalin driver alone is NOT enough: hipRTC ships
   only with the SDK. After install, `hipcc` should resolve in a fresh shell, or
   `HIP_PATH` should point at the SDK root (e.g. `C:\Program Files\AMD\ROCm\7.1\`).
2. **Visual Studio with the C++ workload** (2022 or newer). The HIP SDK's
   `clang++` finds the MSVC toolchain and Windows SDK through the Developer
   shell's `INCLUDE`/`LIB`, so all build steps below run from a
   **"Developer PowerShell for VS" / x64 Native Tools prompt**.
3. This repo, on the branch that includes this file:
   ```
   git clone --recurse-submodules <repo-url>
   git checkout gpu-cuda-cubin        # or main once merged
   ```

## Step 1 — Build the HIP plugin + smoke tool

From a Developer PowerShell at the repo root:

```powershell
tools\windows\build-gpu-plugins.ps1        # builds wfs_hip.dll (skips CUDA if no toolkit — fine)

cd tools
cl /nologo /EHsc /std:c++17 /DWFS_GPU_NATIVE=1 /I..\Source\DSP\gpu `
   test-gpu-plugin.cpp `
   /Fe:..\Builds\VisualStudio2022\x64\Release\App\test-gpu-plugin.exe
```

Expected script output: `Building wfs_hip.dll ...` → `exports OK: wfs_hip.dll`.
(The `hipError_t`/`nodiscard` warnings are known noise.)

## Step 2 — Run the smoke test

The standalone tool does not do the app's ROCm runtime discovery, so put the SDK
`bin` on `PATH` first:

```powershell
cd Builds\VisualStudio2022\x64\Release\App
$env:PATH = "$env:HIP_PATH\bin;$env:PATH"
.\test-gpu-plugin.exe .\wfs_hip.dll
echo "exit code: $LASTEXITCODE"
```

**Pass = all 7 scenarios PASS, exit 0.** That proves plugin load, hipRTC runtime
kernel compilation for the local gfx arch, and real GPU launches for WFS gather,
OB scatter, IR convolution, FDN and SDN.

Record verbatim: the full tool output (it prints the device name), the SDK
version, and the Adrenalin driver version.

### If it fails

- **exit 3** (could not load): `amdhip64`/`hiprtc` not resolvable — check the
  `PATH` line above and that the SDK (not just the driver) is installed.
- **exit 6 with "no ROCm-capable device"**: the HIP runtime does not expose the
  iGPU. Windows HIP SDK official support targets discrete GPUs; RDNA3 iGPUs
  (gfx1103 etc.) work in some SDK versions and not others. This is still a
  useful finding — record the SDK version and the iGPU model; retest with the
  newest SDK, and note that full Windows HIP validation may need a discrete
  AMD card. (Unlike Linux ROCm there is no `HSA_OVERRIDE_GFX_VERSION` escape
  hatch on Windows.)
- **exit 6 with a hipRTC compile error**: paste the full error — that is a
  kernel-source portability finding we need to fix.
- Other codes: see the exit-code table in `Documentation/GPU_Plugin_Smoke_Test.md`.

## Step 3 — In-app pass (after Step 2 passes)

Build the app on the AMD machine (`Builds\VisualStudio2022\WFS-DIY.sln`,
Release), or copy a Windows Release build from the dev machine into the same
folder as the freshly built `wfs_hip.dll`. Then:

1. Launch WFS-DIY, System Config tab → WFS Processor: select the AMD device in
   the GPU/algorithm device selector (it should list the iGPU via
   `GpuDeviceManager`; the app locates the ROCm runtime itself — no PATH setup
   needed here).
2. Select the GPU WFS renderer, play audio, confirm output and stable levels.
3. Reverb tab → Algorithm: switch through IR, FDN, SDN with the AMD device
   selected for the reverb role; confirm each produces a tail and no dropouts.
4. Leave it running ~10 minutes (pump robustness: a single failed GPU block
   currently drops the GPU path silently — if audio goes dry, that is finding
   Q9, note what preceded it).

## Step 4 — Report

Bring back: smoke-test output + exit code, device/SDK/driver versions, in-app
observations. These become the Windows/AMD row of the Phase-0 GPU baseline for
the spatcore extraction (`docs/architecture/` plan).

## Results (ROCm 7.1 / gfx1103)

First Windows+AMD hardware run, 2026-07-02.

| Field | Value |
| --- | --- |
| Machine | Windows 11, AMD Ryzen 7 8745H |
| GPU | AMD Radeon 780M — RDNA3 iGPU, **gfx1103** |
| HIP SDK | **7.1** (patch 51803, githash `d3a86bd04`) |
| Adrenalin driver | 32.0.21030.2001 (2025-09-25) |
| Toolchain | VS 18 (MSVC 14.51) + ROCm `clang++` host-mode |
| Smoke test | **7/7 PASS, exit 0** |

Smoke-test output (`test-gpu-plugin.exe wfs_hip.dll`):

```
device: AMD Radeon 780M Graphics (HIP)
[A] WFS 1x1 g=1 d=0    out.mean=0.5000 (expect ~0.50)                         -> PASS
[B] WFS 2->1 reduce    out.mean=0.6000 (expect 0.60 = 1.0*0.5 + 0.5*0.2)      -> PASS
[C] IR conv  nodes=4   peak=1.0000  tailPeak=0.8290  segs=16/16              -> PASS
[D] FDN reverb nodes=8 peak=0.0426  tailPeak=0.0223 (feedback tail)          -> PASS
[E] SDN reverb nodes=8 peak=0.0897  tailPeak=0.0897 (coupled tail)           -> PASS
[F] OB 1x1 g=1 d=0     out.mean=0.5000 (expect ~0.50)                        -> PASS
[G] OB 1->2 scatter    out0=0.5000 (expect 0.50)  out1=0.2500 (expect 0.25)  -> PASS
PASS: all GPU backends (WFS gather+reduce, OB scatter, IR conv, FDN, SDN) produced ~correct output
```

**In-app pass:** with the HIP device selected, the app ran WFS (8 in x 16 out) and
SDN reverb on the 780M over live ASIO (RME MADIface USB, 48 kHz / 128), logging
`Native GPU WFS active: 8 in x 16 out on AMD Radeon 780M Graphics (HIP)`.

**Findings:**

- **gfx1103 works on Windows HIP with ROCm 7.1** — resolves the Step-2 "exit 6 /
  no ROCm-capable device" concern; this RDNA3 iGPU is usable on this SDK version.
- **hipRTC JIT-compiles all five kernel families** for the local arch at runtime.
- This is a **correctness** validation, not a performance baseline. The 780M is an
  underpowered iGPU (shared memory bandwidth, laptop power budget); at real reverb
  sizes it runs near budget — a brief warmup-underrun burst on first enable, then
  stable with occasional spikes. Take performance numbers from a discrete AMD GPU.
