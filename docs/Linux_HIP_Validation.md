# Linux + AMD HIP/ROCm Validation

Goal: functionally validate the **Linux × HIP** GPU path (`libwfs_hip.so`, all 5
kernel families) on real AMD hardware — plugin load, runtime kernel compilation
(hipRTC), and actual GPU launches for WFS gather, OutputBuffer scatter, IR
convolution, FDN and SDN.

**Status: ✅ PASS** — validated 2026-07-06 on an AMD **gfx1103** (Radeon 780M) iGPU.
This fills the **Linux × HIP** cell of the platform matrix. (The Linux × CUDA cell
remains runtime-pending — no NVIDIA GPU on this box; see
`docs/Linux_CUDA_Validation.md`.)

Companion: `Documentation/GPU_Plugin_Smoke_Test.md` (scenarios, exit codes).

## Environment

| Item | Value |
|------|-------|
| GPU | AMD Radeon 780M iGPU — **gfx1103** (Phoenix3, wave32), Ryzen 7 8745H |
| ROCm | 6.4.3-128 (`/opt/rocm`) |
| HIP | `hipcc` 6.4.43484-123eb5128 |
| Kernel | 6.17.0-35-generic (in-tree amdgpu/KFD; no DKMS) |
| Compiler | g++ 13.3.0 (C++17) |
| JUCE | 8.0.14 · app `1.0.0beta26` |
| Code | `main` @ `c83e534` (identical to `gpu-host/m3-parallel-prep`; covers GPU-host M0–M3) |

`gfx1103` runs natively — **no `HSA_OVERRIDE_GFX_VERSION`** needed. User is in the
`render` + `video` groups.

## What was run

**1 — CPU-safe Release app** (links no GPU runtime; dlopens the plugin):
```bash
cd Builds/LinuxMakefile && make CONFIG=Release -j"$(nproc)" && cd ../..
# -> Builds/LinuxMakefile/build/WFS-DIY
```

**2 — HIP plugin** (via the spatcore wrapper; also builds `libwfs_cuda.so` when a
CUDA toolkit is present):
```bash
tools/linux/build-gpu-plugins.sh
# -> Builds/LinuxMakefile/build/libwfs_hip.so
```

**3 — headless smoke test** (the core check):
```bash
g++ -std=c++17 -DWFS_GPU_NATIVE=1 -I spatcore/gpu tools/test-gpu-plugin.cpp -ldl -o test-gpu-plugin
./test-gpu-plugin Builds/LinuxMakefile/build/libwfs_hip.so 0
```

Linkage confirmed with `readelf -d`:
- `WFS-DIY` — **no** GPU runtime in `NEEDED` (CPU-safe ✓)
- `libwfs_hip.so` — `libhiprtc.so.6` + `libamdhip64.so.6`

## Result — smoke test (verbatim)

```
loaded: Builds/LinuxMakefile/build/libwfs_hip.so   vendor: hip   deviceIndex: 0
device: AMD Radeon Graphics (HIP)
[A] WFS 1x1 g=1 d=0   out.mean=0.5000 (expect ~0.50)  peak=0.5000  launchMs=0.093  -> PASS
[B] WFS 2->1 reduce   out.mean=0.6000 (expect 0.60 = 1.0*0.5 + 0.5*0.2)  peak=0.6000  launchMs=0.102  -> PASS
[C] IR conv    nodes=4  peak=1.0000  tailPeak=0.8290  segs=16/16  launchMs=0.059  -> PASS
[D] FDN reverb nodes=8  peak=0.0426  tailPeak=0.0223 (feedback tail)  launchMs=1.725  -> PASS
[E] SDN reverb nodes=8  peak=0.0897  tailPeak=0.0897 (coupled tail)  launchMs=2.230  -> PASS
[F] OB 1x1 g=1 d=0    out.mean=0.5000 (expect ~0.50)  peak=0.5000  launchMs=0.150  -> PASS
[G] OB 1->2 scatter   out0=0.5000 (expect 0.50)  out1=0.2500 (expect 0.25)  peak=0.5000  launchMs=0.153  -> PASS
PASS: all GPU backends (WFS gather+reduce, OB scatter, IR conv, FDN, SDN) produced finite, non-silent, ~correct output
exit code: 0
```

All 7 scenarios PASS → the whole HIP compute path (plugin load → hipRTC kernel
compile for gfx1103 → real GPU launches → correct output) works on Linux/AMD.

## Notes / not yet covered

- **Headless only.** The in-app audio pass (Step 4 in the CUDA runbook — launch the
  app, pick the AMD device in the WFS Processor / reverb selectors, play audio, A/B
  vs CPU) was **not** done here. The smoke test exercises the full compute path but
  not the JUCE audio-callback integration or the device-selection UI.
- **CUDA on this box:** `libwfs_cuda.so` also **builds and links** cleanly against
  CUDA 13.3 (g++ + `libcudart.so.13` / `libnvrtc.so.13` / `libcuda.so.1`), but is
  **not** runtime-tested — no NVIDIA GPU here. Runtime validation stays pending per
  `docs/Linux_CUDA_Validation.md`.
- **Single-GPU only.** gfx1103 is the sole AMD device; multi-AMD device-index
  binding (`deviceIndex` > 0) is untested.
