# Linux validation of `feat/metal-sdn-node-parallel` — **complete**

Validated on the Ubuntu box **2026-07-26**. Linux was the last platform standing
between this branch and `main`; Windows (CUDA + HIP) and Apple (Metal) were
already signed off. **Every gate passes.** This file was originally a forward
handoff; it is now the record of that run, with the assumptions that turned out
wrong called out so they don't mislead the next reader.

Branch at `b168345` (parent), spatcore re-pinned `58f7811 → 4aa2a95`.

**Box:** Ubuntu 22.04.5, kernel 6.8.0-124, GTX 1650 Mobile (sm_75), driver
610.43.02, CUDA 13.3, g++ 11.4.0, glibc 2.35, cmake 3.22.1.

## Results

| Gate | Result |
|---|---|
| App build (`CONFIG=Debug`) | Clean — 0 errors, links, no missing `ldd` libs |
| `kernel_hashes.py` | OK (10 headers unchanged) |
| `spatcore_dep_lint.py` | OK |
| `experiments_path_lint.py` | OK (8 paths in 8 build scripts resolve) |
| GPU plugins | `libwfs_cuda.so` + `libwfs_hip.so` rebuilt from the bumped submodule |
| CUDA plugin smoke test | **7/7 PASS, exit 0** |
| Node-parallel mapping | Engages, `chunk=256` |
| `baselines/linux-gtx1650.json` (CPU) | 15 combos, re-checked green **twice** |
| `baselines/linux-gtx1650-gpu.json` (GPU) | 15 combos, re-checked green **twice** |

All three lint gates were re-run after the submodule re-pin and stayed green.

## The node-parallel win

`--path gpu-reverb-sdn --scenario static --in 32 --block 256 --blocks 400 --bench`
(384 blocks after warmup, 5.3333 ms budget):

| mapping | wall ms | xRealtime | launchMs med | p99 |
|---|---|---|---|---|
| node-parallel | 459.71 | **4.45** | 0.858 | 1.248 |
| lockstep (`WFS_SDN_NODE_PARALLEL=0`) | 2983.42 | **0.69** | 7.144 | 7.595 |

**6.5× wall-clock, 8.1× on launch time.** The important part is the budget: at 32
nodes lockstep was *over* it (7.14 ms med vs 5.33 ms → 0.69× realtime), while
node-parallel sits at 0.86 ms. On this GPU the port converts a config that could
not hold realtime into one with ~6× headroom.

Both runs produced the **same** `sha256=451328a4…`, so the two mappings are
bit-identical, not merely close.

## Where the original assumptions were wrong

**The JUCE 9 dependency section was a false lead on this box.** All 13 apt
packages from `ci.yml` were already installed (the earlier CUDA session left
them), `libXi.so` unversioned included, and the app compiled with zero errors.
The real blockers were two packages *no runbook mentions*:

- **`cmake`** — absent, and it blocks the offline-render harness completely.
  Needs ≥ 3.22; jammy's 3.22.1 exactly meets it.
- **`ccache`** — absent, yet the documented build command invokes
  `CXX="ccache g++"`.

Both are now installed. Note `sudo` on this box requires a password, so this is
not a step an unattended agent can complete.

**The merge hazard had already resolved itself.** The original text warned that
`git -C spatcore branch -r --contains HEAD` returned exactly one branch, so a
squash-merge would orphan `58f7811`. It now returns three including
`origin/main`: spatcore PR #2 was merged as a **`--no-ff` commit `4aa2a95`**
(parents `f2ba3c9` + `58f7811`), so the SHA stays reachable and the
`submodules: recursive` breakage cannot happen. Step 1 of the merge order was
already done; steps 2–3 are done here — the re-pin is content-neutral (the
merge commit's tree is byte-identical to `58f7811`), which is why no hash moved.

**control-replay cannot run on Linux at all** — it was listed as an
uninvestigated unknown; it is in fact structurally Windows-only.
`tools/validation/control-replay/common.py` uses `ctypes.windll.user32`,
`taskkill /IM`, `PostMessageW(WM_CLOSE)`, and its `EXE_CANDIDATES` lists only
`Builds/VisualStudio2022/**/WFS-DIY.exe`. This needs a port, not a run, and
should be tracked separately rather than sitting on the Linux gate list.

**The GPU-vs-CPU SDN cross-check was Metal-specific and reads as a false
alarm on CUDA.** The original note said `gpu-reverb-sdn/*` hashes come out
byte-identical to CPU `reverb-sdn/*`, since the kernel preserves CPU summation
order. That holds on Metal. On CUDA it does **not** — and not just here: the
already-validated `win-dev-nvidia` baselines differ on SDN too. So it is a
CUDA-wide trait (almost certainly nvcc FMA contraction), not a Linux fault and
not branch-specific.

> The genuine CPU-fallback signal is GPU matching CPU on **every** reverb path
> including FDN and IR. On a healthy CUDA box **all 15 combos differ**. Confirm
> the plugin really loaded from the per-path
> `note: <path> device: NVIDIA … (CUDA)` line instead.

**"CPU baselines are not portable" is true across OS families but overstated in
general.** win-vs-mac is 0/15 (confirmed), but **Linux and Windows CPU share
7/15**. The pattern: every `moving` scenario diverges and all of `reverb-ir`
diverges, while the `static` / `fr-toggle` simple paths match. So the two new
Linux files still prove determinism and self-consistency rather than
"no regression" — but the x86-64 agreement is real and worth knowing.

## Reproducing it

No CUDA or ROCm toolkit is needed to build the *app*: `WFS_GPU_NATIVE=1` and
`WFS_GPU_PLUGINS=1` are baked into `JUCE_CPPFLAGS` and every
`spatcore/gpu/{Cuda,Hip}*Backend.cpp` self-guards to an empty TU, so GPU compute
arrives at runtime via a dlopened `libwfs_<vendor>.so`.

```bash
sudo apt-get install -y \
  build-essential pkg-config ccache cmake \
  libasound2-dev libjack-jackd2-dev \
  libfreetype-dev libfontconfig1-dev \
  libgl1-mesa-dev libcurl4-openssl-dev \
  libgtk-3-dev libwebkit2gtk-4.1-dev \
  libudev-dev libxi-dev

make -C Builds/LinuxMakefile CONFIG=Debug CXX="ccache g++" -j"$(nproc)"

python3 tools/validation/kernel_hashes.py
python3 tools/validation/spatcore_dep_lint.py
python3 tools/validation/experiments_path_lint.py
```

`libjack-jackd2-dev` is a hard compile failure without it (the exporter sets
`JUCE_JACK=1`) and `README.md:75-77` still omits it. `libxi-dev` matters because
JUCE 9 dlopens the *unversioned* `libXi.so`, which only the `-dev` package ships
— with just `libxi6` the app builds but loses multitouch silently.

The parent's CI triggers only on `push: [main]` and PRs to `main`, so a quiet
branch means nothing; run the gates by hand. `kernel_hashes.py` is a cross-repo
assertion — **re-run it after any submodule re-pin.**

### GPU plugins and smoke test

```bash
tools/linux/build-gpu-plugins.sh     # -> spatcore/tools/gpu/build-gpu-plugins.sh
g++ -std=c++17 -DWFS_GPU_NATIVE=1 -I spatcore/gpu tools/test-gpu-plugin.cpp -ldl \
    -o tools/test-gpu-plugin
export LD_LIBRARY_PATH="/usr/local/cuda/lib64:$LD_LIBRARY_PATH"
./tools/test-gpu-plugin ./Builds/LinuxMakefile/build/libwfs_cuda.so 0
```

Pass = 7/7, exit 0; exit-code table in
`Documentation/GPU_Plugin_Smoke_Test.md`. The HIP backend emits ~13 pre-existing
`-Wunused-result` warnings on `CK_DRV`; they are noise, not errors.

> **The stale-plugin trap is live on Linux** and it fired during this run — the
> committed `.so` files were from an earlier session and predated the SDN
> sources by 20 days. On macOS the backend is compiled into the app; here it is
> a dlopened `.so`, so baselining against an old plugin passes *trivially*.
> Always rebuild `libwfs_*.so` from the bumped submodule first.

`WFS_SDN_TRACE=1` prints the chosen mapping. Expect **two** lines, not one:

```
[sdn] mapping=lockstep      chunk=256/256 minDelay=1
[sdn] mapping=node-parallel chunk=256/256 minDelay=349
```

The backend picks per configuration — lockstep where `minDelay` is too small for
chunking to be safe, node-parallel where it clears. A lockstep line on its own
does **not** mean the port failed.

### The baselines

```bash
cmake -S tools/validation/offline-render -B tools/validation/offline-render/build \
      -DCMAKE_BUILD_TYPE=Release        # MANDATORY: no default, and an empty
                                        # build type means no -O flag at all
cmake --build tools/validation/offline-render/build -j"$(nproc)"

cd tools/validation/offline-render
BIN=build/offline-render_artefacts/Release/offline-render

./$BIN --path cpu --scenario all --check baselines/linux-gtx1650.json --update
./$BIN --path cpu --scenario all --check baselines/linux-gtx1650.json
```

`--plugin-dir` is a **no-op on POSIX** (it only prints a note; dlopen does not
re-read the environment). `GpuBackendFactory` tries `exeDir()` first and then the
loader search path, so co-locate the plugin beside the harness binary:

```bash
ln -sf "$PWD/../../../Builds/LinuxMakefile/build/libwfs_cuda.so" \
       build/offline-render_artefacts/Release/libwfs_cuda.so
export LD_LIBRARY_PATH="/usr/local/cuda/lib64:$LD_LIBRARY_PATH"
./$BIN --path gpu --scenario all --device cuda:0 \
       --check baselines/linux-gtx1650-gpu.json --update
./$BIN --path gpu --scenario all --device cuda:0 \
       --check baselines/linux-gtx1650-gpu.json
```

CPU and GPU must be separate files and separate invocations. Record at the
**default shape** (`sr=48000 block=512 blocks=200 in=8 out=16`) — the JSON stores
no shape metadata, so a baseline taken at a non-default `--in/--block/--sr`
silently mismatches later. Exit codes: 0 ok, 1 mismatch, 2 usage, 3 drain
timeout, 4 IR-load timeout, 5 self-test, 6 GPU/plugin unavailable, 7 GPU runtime
failure.

Harmless on this box: repeated `warning: startRealtimeThread failed, using
normal priority` — the renders are offline, so it does not affect the hashes
(both baselines re-checked identical twice).

## Windows × HIP rebuild + check — **4a + 4b done** (added 2026-07-26, run 2026-07-26)

Linux × HIP is done (see above). Windows × HIP is **now validated too** — 4a and 4b
both pass, results inline below. **4c (Windows × CUDA) remains outstanding** and
could not run on the box that did 4a/4b.

Done in parent `4257f78`, spatcore `d1e6f66`. **Box:** Windows 11 Pro 26200,
Radeon 780M (gfx1103), ROCm 7.1, VS 18.8 Community / MSVC 14.51.

The procedure below is kept as the standing recipe for the next rebuild, including
the **do 4a before 4b** ordering — that still governs, because 4b is actively
misleading against a stale DLL.

### 4a — rebuild `tools\windows\prebuilt\wfs_hip.dll` (mandatory first) — ✅ done

> **Run 2026-07-26.** Rebuilt from spatcore `d1e6f66`: 0 errors, both plugins export
> `wfs_plugin_*`, only the known `nodiscard` / `-Wunused-result` noise on `CK_DRV`.
> 225792 → 250368 bytes. Imports still `amdhip64_7.dll` + `hiprtc0701.dll`, so the
> ROCm 7.x floor is unchanged. Committed with the README provenance line in
> `4257f78`.

The committed DLL had been built **2026-07-20** from spatcore `b0f35ef`, predating
*both* the node-parallel SDN port (`c12f3b6`) and the `WFS_SDN_TRACE` port
(`3da516a`, merged as `d1e6f66`). The plugin ABI is the seven unchanged
`extern "C"` entry points, so **a stale DLL loads happily and silently runs the
older kernel** — you would see zero `[sdn]` lines and wrongly conclude the trace
port failed.

Needs a Windows box with the **AMD HIP SDK (ROCm 7.x)** — the DLL imports
`amdhip64_7.dll` + `hiprtc0701.dll`, so the SDK floor is set by whoever rebuilds
it — **and MSVC**, from a *Developer PowerShell* so `cl.exe` and the SDK are on
`PATH`.

```powershell
git pull
git submodule update --init --recursive     # must land spatcore d1e6f66
tools\windows\build-gpu-plugins.ps1          # -> Builds\VisualStudio2022\x64\Release\App\
Copy-Item Builds\VisualStudio2022\x64\Release\App\wfs_hip.dll tools\windows\prebuilt\ -Force
```

Then update the **"Current binary"** provenance line in
`tools\windows\prebuilt\README.md` to spatcore `d1e6f66` (noting it now includes
the node-parallel SDN port and the `WFS_SDN_TRACE` mapping log), and commit both.
`release.yml` packages this committed DLL as-is, so an un-refreshed one ships.

> **If the HIP build fails to compile**, look at `HipSdnBackend.cpp` first. This TU
> is built on Windows with the HIP SDK's **clang++ against the MSVC STL** (not
> `hipcc` — see the comment block in `spatcore/tools/gpu/build-gpu-plugins.ps1`).
> The explicit `#include <cstdio>` there exists precisely for that path: libstdc++
> pulls it in transitively and MSVC's headers do not, so a missing `std::fprintf`
> would break Windows while Linux stays green. CI cannot catch this — the Windows
> runner has no AMD HIP SDK.

### 4b — validate Windows × HIP — ✅ done

> **Run 2026-07-26, all gates pass.** `test-gpu-plugin.exe wfs_hip.dll 0` → **7/7
> PASS, exit 0**, on `AMD Radeon 780M Graphics (HIP)`. SDN peak **0.0633**, matching
> the Linux gfx1103 reference exactly. `WFS_SDN_TRACE=1` produced **both** expected
> lines verbatim (`lockstep minDelay=1` + `node-parallel minDelay=349`), and
> `WFS_SDN_NODE_PARALLEL=0` correctly collapsed to the lockstep line alone.
>
> | mapping | SDN peak | launchMs |
> |---|---|---|
> | node-parallel (default) | 0.0633 | 1.199 |
> | lockstep (`WFS_SDN_NODE_PARALLEL=0`) | 0.0633 | 2.666 |
>
> Peak identical, timing 2.2× — the same shape as the Linux gfx1103 numbers below
> (2.4×). The three lint gates were also re-run green after the re-pin to `d1e6f66`.
>
> **Deviation from the order below:** 4b was run against the *freshly built* DLL
> before copying it into `prebuilt\` and committing, rather than after. Same
> guarantee — validation never sees a stale binary — but an unvalidated binary
> never enters history. Recommended for future rebuilds.
>
> The harness needs the ROCm `bin` on `PATH` (the Windows analogue of the Linux
> `LD_LIBRARY_PATH` export): unlike the app, `tools/test-gpu-plugin.cpp` does not
> call `ensureVendorRuntimeSearchPath`, so `amdhip64_7.dll` / `hiprtc0701.dll` do
> not otherwise resolve.

Build the harness from `tools\` (**note the include path** — it is `spatcore\gpu`
now, not the pre-extraction `Source\DSP\gpu`):

```
cl /nologo /EHsc /std:c++17 /DWFS_GPU_NATIVE=1 /I..\spatcore\gpu ^
   test-gpu-plugin.cpp /Fe:test-gpu-plugin.exe

.\test-gpu-plugin.exe ..\Builds\VisualStudio2022\x64\Release\App\wfs_hip.dll 0
```

Pass = **7/7, exit 0**. Then the mapping trace:

```powershell
$env:WFS_SDN_TRACE = "1"
.\test-gpu-plugin.exe ..\Builds\VisualStudio2022\x64\Release\App\wfs_hip.dll 0
```

Expect **two** lines, matching what Linux × HIP produces:

```
[sdn] mapping=lockstep      chunk=256/256 minDelay=1
[sdn] mapping=node-parallel chunk=256/256 minDelay=349
```

**Zero `[sdn]` lines means the DLL is stale — go back to 4a.** It does not mean
the port failed.

Linux reference, gfx1103 / ROCm 7.1.1, 2026-07-26, for comparison:

| | SDN peak | launchMs |
|---|---|---|
| node-parallel (default) | 0.0633 | 0.90 |
| lockstep (`WFS_SDN_NODE_PARALLEL=0`) | 0.0633 | 2.19 |

The two mappings agreeing on **peak** is the correctness property; only the timing
should move. Note 0.0633 supersedes the `0.0897` recorded in
`docs/Linux_HIP_Validation.md` (2026-07-06) — that shift came from `c12f3b6`
reshaping the kernel, not from the mapping or the ROCm version (6.4.3 and 7.1.1
were verified byte-identical across all 7 scenarios).

### 4c — Windows × CUDA sanity check — ⏳ still outstanding

> **Could not run on the 4a/4b box (2026-07-26): it has no NVIDIA GPU** — Radeon
> 780M only. This needs the `win-dev-nvidia` machine. `wfs_cuda.dll` *was* rebuilt
> there by the same `build-gpu-plugins.ps1` and exports `wfs_plugin_*` fine, so
> only the runtime half is unverified.
>
> **New trap worth knowing:** force-loading `wfs_cuda.dll` with no NVIDIA driver
> **access-violates** in the harness (`0xC0000005`) instead of exiting `6`
> (GPU/plugin unavailable), which the exit-code table would lead you to expect.
> The *app* never hits this — `GpuDeviceManager` enumerates first and skips the
> plugin, the clean no-op — so it is harness robustness on wrong-vendor hardware,
> not an app defect. Do not read it as a CUDA-path regression.

Not merely a formality on this branch: `ac94932` (node-parallel CUDA SDN kernel,
14-16× at 32 nodes) and `ab0bd34` (warmup launch for both SDN kernels at
`prepare()`) are both ancestors of the pinned spatcore, so the CUDA SDN path
genuinely changed. `wfs_cuda.dll` is built fresh by the same
`build-gpu-plugins.ps1` (and by `release.yml` in CI), so no prebuilt staleness
applies. CUDA has always had the trace, so expect the same two `[sdn]` lines.

**Do not false-alarm on SDN GPU-vs-CPU hash differences** — see *"Where the
original assumptions were wrong"* above: that is nvcc FMA contraction, a CUDA-wide
trait, and the already-validated `win-dev-nvidia` baselines differ on SDN too. The
genuine CPU-fallback signal is GPU matching CPU on *every* reverb path including
FDN and IR; confirm the plugin really loaded via the per-path
`note: <path> device: NVIDIA … (CUDA)` line.

## What still remains

1. **Merge the parent.** spatcore is already on `main` (`4aa2a95`, since re-pinned
   to `d1e6f66`); the re-pin and its gate re-run are done. Of step 4, **4a and 4b
   are done** (`4257f78`); only **4c — Windows × CUDA** is left, and it needs the
   `win-dev-nvidia` box.
2. **Port control-replay to POSIX**, or explicitly scope it Windows-only. Until
   then `tools/bump-spatcore.ps1` step 6 is unverifiable on Linux.
3. **`aarch64` is still unvalidated.** `ThirdParty/juce_simpleweb/libs/Linux/`
   ships only `x86_64` and `armv7` static libssl/libcrypto, so an arm64 link
   fails with no `-lssl`/`-lcrypto`. This run was x86-64 only.

## Known-stale things (re-verified 2026-07-26; one now RESOLVED, rest still true)

- ~~**`tools/windows/prebuilt/wfs_hip.dll`**~~ — **RESOLVED 2026-07-26 in
  `4257f78`.** Was last committed 2026-07-20 (`a359e2a`) and stale against both
  `c12f3b6` (node-parallel SDN) and `3da516a` (`WFS_SDN_TRACE`). Now rebuilt from
  spatcore `d1e6f66` on Windows + ROCm 7.1 and validated 7/7 — see *Windows × HIP
  rebuild + check*, 4a/4b, above. The README correction from `b168345` still
  stands: the binary imports `amdhip64_7.dll` + `hiprtc0701.dll`, so the SDK floor
  is set by whoever rebuilds it.
- **`Documentation/Linux_GPU_Enablement.md:51`** still presents a
  `WFS_GPU_HIP=1` / ROCm-linked `LINUX_MAKE` config as "the current Linux
  variant". Two architecture generations out of date — the app is plugin-mode.
- **`Documentation/Beta_Distribution_Checklist.md`** is stale (version `0.1.0`,
  "zero distribution infrastructure", "ships CPU-only"). Live release procedure
  is `docs/CI_SETUP.md:62-78`.
- **`spatcore/examples/minimal-app/CMakeLists.txt:41`** pins JUCE **8.0.14**
  (`2cdfca8feb`) with the comment "pinned to the version the apps use" — but the
  submodule is now 9.0.0 (`f8f8864`). `2cdfca8feb` is literally the *previous*
  submodule pin, so this is a leftover, and it is the only live JUCE-version
  disagreement.
- **Exporter version drift:** the generated exporters carry
  `JUCE_APP_VERSION=1.0.0beta38` while `WFS-DIY.jucer` says **beta39** — a stale
  Projucer re-save. This branch does not bump the version.
- **In-tree resources:** the Linux exporter has no postbuild step, so `lang/`
  and the MCP resources are not copied next to the binary and there is no Linux
  dev-tree fallback in the resource search. Expect untranslated UI when running
  from the build tree; tarball staging is what puts them in place.

## Packaging (release-time, not a merge gate)

`tools/linux/build-app-tarball.sh` produces
`WFS-DIY-Linux-x86_64-<version>.tar.gz` and, when ROCm/CUDA are present, bundles
the full GPU runtime closure (hipRTC + comgr + device bitcode, or NVRTC) so the
target needs no SDK. **It does not run `make` itself** — do a clean Release build
first. Runbook: `docs/Linux_AMD_Release_Tarball.md`. Nothing on this branch
invalidates a Linux packaging artifact, so leave this for the next release cut.
