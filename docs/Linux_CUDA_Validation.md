# Linux + NVIDIA CUDA Validation Runbook

Goal: functionally validate the **Linux × CUDA** GPU path (`libwfs_cuda.so`, all 5
kernel families) on real NVIDIA hardware. This is the **one never-runtime-tested
cell** of the platform matrix — Linux CUDA has always *built and linked* but was
never exercised on a GPU (no NVIDIA Linux box on hand until now). A clean run here
closes that gap and, as a bonus, validates the GPU host-path optimizations
(blocking-sync events, upload diet, parallel prep) on Linux CUDA.

Target: the aging Ubuntu laptop + **GTX 1650** (Turing, compute capability
**sm_75**). The runtime-cubin compile path (NVRTC → arch-exact SASS) targets sm_75
automatically from the device — the PTX-vs-driver mismatch that bit Windows does
not apply here.

Companion: `Documentation/GPU_Plugin_Smoke_Test.md` (scenarios, exit codes);
`Documentation/Linux_GPU_Enablement.md` (general Linux GPU build notes).

## Prerequisites on the Ubuntu box

1. **NVIDIA driver** (any recent one that supports Turing) — provides `libcuda.so`.
   Check: `nvidia-smi` prints the GTX 1650 and a driver/CUDA version.
2. **CUDA Toolkit** (for build-time headers + `libcudart`/`libnvrtc`). Any 11.x/12.x
   supports sm_75. Default location `/usr/local/cuda`; if elsewhere, `export
   CUDA_PATH=/path/to/cuda` before building. Check: `ls $CUDA_PATH/include/cuda.h`.
3. **Build toolchain**: `g++` (C++17), `make`, and the JUCE Linux dev packages
   (ALSA, X11, freetype, curl, webkit2gtk, etc.). If the app link fails on missing
   `-l...`, install the JUCE Linux deps (the usual `libasound2-dev libx11-dev
   libxext-dev libxinerama-dev libxrandr-dev libxcursor-dev libfreetype6-dev
   libcurl4-openssl-dev libwebkit2gtk-4.1-dev` set).
4. This repo, on `main` (which milestone to test is confirmed separately — see the
   note at the end):
   ```
   git clone --recurse-submodules <repo-url> && cd WFS_DIY_v1
   git checkout main && git pull
   ```

## Step 1 — Build the CPU-safe app

The Linux app links **no** GPU runtime; it dlopens the plugin at runtime.

```bash
cd Builds/LinuxMakefile
make CONFIG=Release -j"$(nproc)"      # -> Builds/LinuxMakefile/build/WFS-DIY
cd ../..
```

(If the codegen prebuild complains about Python, ensure `python3` is on PATH — same
silent-skip behaviour as elsewhere.)

## Step 2 — Build the CUDA plugin

```bash
# CUDA_PATH defaults to /usr/local/cuda; export it first if your toolkit is elsewhere.
tools/linux/build-gpu-plugins.sh
# -> Builds/LinuxMakefile/build/libwfs_cuda.so  (beside the app binary)
```

Expected: `Building libwfs_cuda.so (g++ + CUDA /usr/local/cuda) ...` →
`-> …/libwfs_cuda.so`. (It also builds `libwfs_hip.so` **only** if `hipcc` is
present — irrelevant here, safe to ignore.)

## Step 3 — Build + run the headless smoke test (the core check)

```bash
cd tools
g++ -std=c++17 -DWFS_GPU_NATIVE=1 -I../spatcore/gpu test-gpu-plugin.cpp -ldl -o test-gpu-plugin
cd ..

# Run against the freshly built plugin. LD_LIBRARY_PATH belt-and-suspenders so the
# plugin's cudart/nvrtc resolve even if the .so rpath doesn't cover your layout.
LD_LIBRARY_PATH="${CUDA_PATH:-/usr/local/cuda}/lib64:$LD_LIBRARY_PATH" \
  ./tools/test-gpu-plugin ./Builds/LinuxMakefile/build/libwfs_cuda.so
echo "exit code: $?"
```

**Pass = all 7 scenarios PASS, exit 0.** That proves plugin load, NVRTC runtime
kernel compilation for sm_75, and real GPU launches for WFS gather, OB scatter, IR
convolution, FDN and SDN — the whole CUDA compute path on Linux for the first time.

Record verbatim: the full tool output (it prints the device name), `nvidia-smi`
driver/CUDA version, and the CUDA Toolkit version (`nvcc --version`).

### If it fails
- **exit 3** (could not load): the `.so`'s deps aren't resolvable — set
  `LD_LIBRARY_PATH` to your CUDA `lib64` (above), and confirm the driver's
  `libcuda.so.1` is present (`ldconfig -p | grep libcuda`).
- **exit 6 at scenario [A]** with an NVRTC/driver message: capture it — on Linux the
  cubin path targets the device arch directly, so a toolchain/driver-version error
  here is a real finding, not the documented Windows PTX no-op.
- **exit 4**: plugin exported no `wfs_plugin_*` symbols — build problem; re-check
  Step 2 output.
- Other codes: see the exit-code table in `Documentation/GPU_Plugin_Smoke_Test.md`.

## Step 4 — In-app pass (after the smoke test passes)

Launch `Builds/LinuxMakefile/build/WFS-DIY`, System Config → WFS Processor: pick the
GTX 1650 in the GPU/algorithm device selector, select the GPU WFS renderer, play
audio, confirm output + stable levels. Then Reverb tab → Algorithm: try IR / FDN /
SDN on the GPU. The GTX 1650 is a small GPU; large reverb configs may run near
budget — note any underruns (the GPU pipeline strip in the Level Meter window shows
pump ms vs budget + underruns).

## Step 5 — Report back

Smoke output + exit code, `nvidia-smi` + `nvcc` versions, in-app observations. This
becomes the **Linux × CUDA** row of the platform matrix (previously "pending —
test machine not ready") and the Linux-CUDA baseline for the host-path work.

---

**Which milestone to test:** run this against `main`. The build steps are identical
regardless of milestone; testing after the GPU-host M3 merge lands covers M1
(blocking-sync events) + M2 (upload diet) + M3 (parallel prep) in one pass. Confirm
with the maintainer that M3 is merged before starting, or run now against M0–M2 if
you want to shake out any Linux-CUDA-specific build issues early (they'd be
independent of the milestone).
