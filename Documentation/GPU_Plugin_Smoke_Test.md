# GPU Plugin Smoke Test — `test-gpu-plugin`

`tools/test-gpu-plugin.cpp` is a small standalone, headless program that proves a
`wfs_<vendor>` GPU plugin's **full compute path works on real hardware** — with no
audio device, no `GpuAsyncPipeline`, and no UI. It is the fast "does the GPU build
actually run on this box?" check you run after building the plugins on a new
machine or after touching a backend/kernel.

It exercises the end-to-end chain for every backend the plugin exports:

```
plugin load (LoadLibrary / dlopen)
  -> resolve the C entry points (wfs_plugin_create_*)
    -> create backend
      -> prepare()        // device init + runtime kernel compile (NVRTC / hipRTC)
        -> per-backend setup (matrix / IR / params / geometry)
          -> processBlock()   // real GPU launch
            -> assert finite, non-silent, ~correct output
```

`main` returns **exit code 0 iff every backend passes**; any non-zero code names
the backend and failure stage (see [Exit codes](#exit-codes)).

---

## Scope: which configurations this tool covers

The tool loads a **plugin**, so it covers the plugin-based vendors only:

| Platform | CUDA (NVIDIA) | HIP/ROCm (AMD) | Metal (Apple) |
|----------|---------------|----------------|---------------|
| Windows  | `wfs_cuda.dll` ✅ | `wfs_hip.dll` ✅ | — n/a |
| Linux    | `libwfs_cuda.so` ✅ | `libwfs_hip.so` ✅ | — n/a |
| macOS    | — n/a | — n/a | **in-process, no plugin** |

> **macOS / Metal is not exercised by this tool.** On Apple platforms the Metal
> backend is compiled *into the app* (`GpuBackendFactory` returns `nullptr` on
> `__APPLE__`, there is no `wfs_metal` plugin), so it is validated through the app
> itself, not through `test-gpu-plugin`. This tool validates the four
> plugin-loaded configs (Windows/Linux × CUDA/HIP).

---

## The plugin ABI it drives

The entry points come from `spatcore/gpu/plugin/GpuVendorPlugin.cpp`:

| Symbol | Returns | Purpose |
|--------|---------|---------|
| `wfs_plugin_vendor()` | `const char*` | `"cuda"` / `"hip"` / `"metal"` (self-ID) |
| `wfs_plugin_create_wfs(int deviceIndex)` | `IWfsBackend*` | WFS delay-and-sum (gather) |
| `wfs_plugin_create_ob(int deviceIndex)`  | `IObBackend*`  | OutputBuffer (scatter) |
| `wfs_plugin_create_ir(int deviceIndex)`  | `IIrBackend*`  | IR convolution reverb |
| `wfs_plugin_create_fdn(int deviceIndex)` | `IFdnBackend*` | FDN reverb |
| `wfs_plugin_create_sdn(int deviceIndex)` | `ISdnBackend*` | SDN reverb |
| `wfs_plugin_destroy(IGpuBackend*)` | `void` | destroy any of the above |

`deviceIndex` (the optional CLI arg, default `0`) selects which GPU **of that
vendor** to bind — e.g. `1` for the second of two identical cards. The shared
interfaces live in `spatcore/gpu/GpuBackendInterface.h`.

---

## Scenarios — 7 checks across the 5 GPU kernel families

### WFS — `IWfsBackend` (gather / delay-and-sum)

Driven with **constant** inputs; the check reads the **last block's steady-state
mean** (so the per-sample prev→curr ramp has settled).

- **[A] 1×1, gain 1, delay 0** — output settles to the input. `in=0.5 → mean≈0.50`.
- **[B] 2 inputs → 1 output**, gains `[1.0, 0.5]`, inputs `[0.5, 0.2]` —
  `out = g0·in0 + g1·in1 = 0.60`. This exercises the **multi-input reduce sum**;
  a backend that ignored input 1 (a 1×1 passthrough) would return `0.5` and FAIL.

### OutputBuffer — `IObBackend` (scatter, the write-time dual of WFS)

OB shares WFS's exact `prepare` / `setMatrixPointers` / `processBlock` surface and
the same gain indexing `gains[in*numOut + out]`, so it reuses the same constant-
drive check (templated over both backends).

- **[F] 1×1, gain 1** — output settles to the input, `mean≈0.50`.
- **[G] 1 input → 2 outputs**, gains `[1.0, 0.5]`, `in=0.5` — the input is
  **distributed** to two outputs: `out0 = 0.50`, `out1 = 0.25`. A single-output or
  gather-collapsed backend would leave `out1` silent.

> **OB delay contract:** a write-time scatter cannot represent a delay `< 1`
> sample (the head cell was just read and cleared), so OB clamps every scatter
> delay to `≥ 1` sample. That produces a 1-sample startup delay which is invisible
> in the settled last-block mean. At 0 dB the per-tap HF shelf is mathematically
> unity, so a constant input still yields exactly `out = gain · in`.

### Reverbs — `IIrBackend` / `IFdnBackend` / `ISdnBackend` (stateful)

Reverbs have no closed-form output, so a fixed-value check would be brittle.
Instead each is driven with **one impulse on block 0** (node 0, sample 0), then
the assertion is: **output is finite and non-silent in a "tail" window — blocks
after the input has already gone silent.** Energy there can only come from the
algorithm's own internal state (delay-line feedback, node coupling, multi-tap
convolution), which a dry passthrough could not produce.

- **[C] IR convolution** — stage a decaying 4096-sample mono IR (every node
  convolves the same IR), fire one impulse, expect a multi-block convolution
  response. Also asserts `segmentsLoaded == segmentsTotal` (the progressive
  loader finished). Tail window: blocks ≥ 1.
- **[D] FDN reverb** — `setParameters(rt60=1.5, …)`, fire one impulse; the
  feedback rings warm up over the first blocks, so the tail window is blocks ≥ 4,
  where the input is silent and any energy is recirculating feedback.
- **[E] SDN reverb** — `setGeometry` (8 nodes at the corners of a ~4×3×2.5 m box
  so inter-node delays are non-trivial) + `setParameters`, fire one impulse;
  tail window blocks ≥ 2 (after the geometry crossfade settles).

---

## Building

The interface header is guarded by `WFS_GPU_NATIVE`, so define it. The tool links
no GPU runtime itself — it only `dlopen`s the plugin — so it builds with a plain
host compiler.

**Windows** (from a *Developer PowerShell* / *x64 Native Tools* prompt so `cl` is
on `PATH`):

```
cl /nologo /EHsc /std:c++17 /DWFS_GPU_NATIVE=1 /I..\Source\DSP\gpu ^
   test-gpu-plugin.cpp /Fe:test-gpu-plugin.exe
```

**Linux:**

```
g++ -std=c++17 -DWFS_GPU_NATIVE=1 -I../spatcore/gpu test-gpu-plugin.cpp -ldl -o test-gpu-plugin
```

The **plugin itself** is built separately by `tools/windows/build-gpu-plugins.ps1`
or `tools/linux/build-gpu-plugins.sh`, which emit `wfs_cuda.*` / `wfs_hip.*` into
`Builds/VisualStudio2022/x64/Release/App/` (beside the app exe) and stage the CUDA
runtime DLLs next to `wfs_cuda.dll`. Build the test into (or run it from) that
directory so the plugin and its runtime dependencies resolve.

---

## Running

```
test-gpu-plugin <plugin-path> [device-index]
```

Examples:

```
# NVIDIA / CUDA (run from the App dir; ensure the CUDA runtime is resolvable)
test-gpu-plugin ./wfs_cuda.dll

# AMD / HIP, second AMD device
test-gpu-plugin ./wfs_hip.dll 1
```

The plugin's runtime deps must be on the loader path: CUDA (`cudart` / `nvrtc`,
from the CUDA Toolkit `bin` or staged beside the plugin) for `wfs_cuda`, and the
AMD HIP runtime (`amdhip64` / `hiprtc`, from the ROCm/HIP install) for `wfs_hip`.

### Sample output (NVIDIA RTX PRO 2000, CUDA)

```
loaded: ./wfs_cuda.dll   vendor: cuda   deviceIndex: 0
device: NVIDIA RTX PRO 2000 Blackwell Generation Laptop GPU (CUDA)
[A] WFS 1x1 g=1 d=0   out.mean=0.5000 (expect ~0.50)  ...  -> PASS
[B] WFS 2->1 reduce   out.mean=0.6000 (expect 0.60 ...)    -> PASS
[C] IR conv    nodes=4  peak=1.0000  tailPeak=0.8290  segs=16/16  -> PASS
[D] FDN reverb nodes=8  peak=0.0426  tailPeak=0.0223 (feedback tail)  -> PASS
[E] SDN reverb nodes=8  peak=0.0897  tailPeak=0.0897 (coupled tail)   -> PASS
[F] OB 1x1 g=1 d=0    out.mean=0.5000 (expect ~0.50)  -> PASS
[G] OB 1->2 scatter   out0=0.5000 (expect 0.50)  out1=0.2500 (expect 0.25)  -> PASS
PASS: all GPU backends (WFS gather+reduce, OB scatter, IR conv, FDN, SDN) ...
```

---

## Exit codes

`0` = every backend passed. Otherwise the code identifies the backend and stage:

| Code | Meaning |
|------|---------|
| `0`  | all scenarios passed |
| `2`  | usage error (no plugin path given) |
| `3`  | could not load the plugin (`LoadLibrary`/`dlopen` failed) |
| `4`  | `wfs_plugin_create_wfs` not exported (the silent-CPU-fallback trap) |
| `5` / `6` / `7` | WFS create returned null / `prepare()` false / `processBlock` failure |
| `8` / `9` | WFS scenario **A** / **B** produced the wrong value |
| `10` / `11` / `12` / `13` | IR: not exported / null / `prepare()` false / scenario **C** failed |
| `14` / `15` / `16` / `17` | FDN: not exported / null / `prepare()` false / scenario **D** failed |
| `18` / `19` / `20` / `21` | SDN: not exported / null / `prepare()` false / scenario **E** failed |
| `22` / `23` / `24` / `25` | OB: not exported / null / `prepare()` false / `processBlock` failure |
| `26` / `27` | OB scenario **F** / **G** produced the wrong value |

---

## Interpreting results per configuration

- **NVIDIA + CUDA / AMD + HIP with a real device present** → all 7 scenarios
  `PASS`, exit `0`. This proves the whole GPU compute path (including runtime
  kernel compilation) on that card.

- **ROCm/HIP runtime installed but no AMD GPU** (e.g. a dev box with only NVIDIA)
  → the plugin loads and the HIP runtime initialises, but WFS `prepare()` returns
  `false` with *"HIP runtime: no ROCm-capable device is detected"* and the tool
  **exits `6` at scenario [A]**. This is the documented **clean no-op**, not a
  regression — `GpuDeviceManager` would likewise enumerate no AMD device and the
  app would never load the plugin. The reverb scenarios are not reached (they only
  run after WFS succeeds), so functional HIP validation of the reverb/scatter
  kernels requires actual AMD hardware.

- **exit `4`** is the important trap: the plugin loaded but exports no
  `wfs_plugin_*` symbols (on Windows a DLL exports nothing without
  `__declspec(dllexport)`). The app would silently fall back to CPU. Check the
  export markers in `GpuVendorPlugin.cpp`.

---

## Related

- `Documentation/Linux_GPU_Enablement.md` — Linux GPU build/enable notes.
- `spatcore/gpu/GpuBackendInterface.h` — the shared backend interfaces.
- `spatcore/gpu/plugin/GpuVendorPlugin.cpp` — the plugin entry points.
- `tools/windows/build-gpu-plugins.ps1` / `tools/linux/build-gpu-plugins.sh` —
  build the plugin DLLs/SOs this tool loads.
