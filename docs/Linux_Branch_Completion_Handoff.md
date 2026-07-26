# Handoff: finish `feat/metal-sdn-node-parallel` on Linux

For the next session **on the Ubuntu box**. Written 2026-07-26, after the Metal
node-parallel SDN port landed and was validated on a Mac mini M4 Pro. This is
the last platform standing between the branch and `main`.

Branch: `feat/metal-sdn-node-parallel` in **both** repos (parent `622e44e`,
spatcore pinned at `58f7811`).

```bash
git clone --recurse-submodules https://github.com/pob31/WFS-DIY.git
cd WFS-DIY && git checkout feat/metal-sdn-node-parallel
git submodule update --init --recursive
```

## What this branch actually changes (read this first)

**Nothing the Linux Makefile compiles.** The spatcore bump `f2ba3c9 → 58f7811`
is Metal-only (`MetalSdn{Kernels.h,Backend.mm}`, `MetalObBackend.mm`,
`MetalWfsBackend.mm`, plus `CMakeLists.txt` and CI); the parent delta is
`Experiments/`, `tools/validation/`, `.github/workflows/ci.yml` and two new mac
baselines. The CUDA and HIP SDN kernels are untouched here — their
node-parallel reshape already landed on spatcore `main` (`ac94932`, `c12f3b6`).

So a Linux compile failure on this branch is **not** caused by the branch. It is
almost certainly the JUCE 9 migration that came in with `main`. Start there.

## 1. The likely compile blocker: JUCE 9 dependencies

`main` moved to **JUCE 9.0.0** (submodule pin `f8f8864`, tag `9.0.0`,
2026-07-21). JUCE 9 needs deps an older box will not have. Install the exact set
CI uses (`.github/workflows/ci.yml:101-111`):

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential pkg-config ccache \
  libasound2-dev libjack-jackd2-dev \
  libfreetype-dev libfontconfig1-dev \
  libgl1-mesa-dev libcurl4-openssl-dev \
  libgtk-3-dev libwebkit2gtk-4.1-dev \
  libudev-dev libxi-dev
```

Three of these bite specifically:

- **`libxi-dev`** — new for JUCE 9's native XInput2 multitouch (`JUCE_USE_XINPUT`).
  JUCE dlopens the *unversioned* `libXi.so`, which only the `-dev` package ships,
  so a box with just `libxi6` builds but loses multitouch silently at runtime.
- **`libjack-jackd2-dev`** — the Linux exporter sets `JUCE_JACK=1`
  (`WFS-DIY.jucer:518/521`), so this is a hard compile failure without it.
  **`README.md:75-77` omits it** (verified). The README is otherwise current —
  it already carries `libxi-dev` and explains the JUCE 9 reason — so use the CI
  list above rather than the README, but only this one package is missing.
- **`libfreetype-dev`** — CI uses this name; the README says `libfreetype6-dev`.
  On current Ubuntu the older name is a transitional package that still
  resolves, so this is a naming drift to be aware of rather than a breakage.

Then build exactly as CI does:

```bash
make -C Builds/LinuxMakefile CONFIG=Debug CXX="ccache g++" -j"$(nproc)"
# binary -> Builds/LinuxMakefile/build/WFS-DIY   (same path for both CONFIGs)
```

`tools/setup.sh` is current and JUCE-9-aware — it is submodule-init only now;
the old JUCE multitouch patching step is gone because JUCE 9 ships it natively.

**No CUDA or ROCm toolkit is needed to build the app.** `WFS_GPU_NATIVE=1` and
`WFS_GPU_PLUGINS=1` are baked into `JUCE_CPPFLAGS` (`Makefile:42/:63`), and every
`spatcore/gpu/{Cuda,Hip}*Backend.cpp` self-guards to an empty TU under
`WFS_GPU_PLUGINS`. GPU compute arrives at runtime via dlopened
`libwfs_<vendor>.so`.

> **aarch64 warning:** `ThirdParty/juce_simpleweb/libs/Linux/` ships only
> `x86_64` and `armv7` static libssl/libcrypto. On an arm64 Ubuntu box the link
> will fail with no `-lssl`/`-lcrypto` to find.

## 2. Make CI green

The Linux job is four steps (`ci.yml:77-121`). Reproduce it locally:

```bash
python3 tools/validation/kernel_hashes.py
python3 tools/validation/spatcore_dep_lint.py
python3 tools/validation/experiments_path_lint.py
make -C Builds/LinuxMakefile CONFIG=Debug CXX="ccache g++" -j"$(nproc)"
```

All three gates pass on this branch as of `622e44e`. Note the parent repo's CI
triggers only on `push: [main]` and PRs to `main`, so **a quiet branch means
nothing** — the gates first run when you open the PR. Run them by hand.

`kernel_hashes.py` is a cross-repo assertion: the parent's
`tools/validation/kernel_hashes.json` must match whatever the parent's gitlink
checks out of `spatcore/gpu/*Kernels.h`. Both halves moved together here. **Re-run
it after any submodule re-pin** (see §5).

## 3. GPU plugin + smoke test

```bash
tools/linux/build-gpu-plugins.sh          # thin wrapper -> spatcore/tools/gpu/build-gpu-plugins.sh
ls -l Builds/LinuxMakefile/build/libwfs_{cuda,hip}.so
```

It builds `libwfs_hip.so` when `hipcc` is on PATH and `libwfs_cuda.so` when
`/usr/local/cuda/include` exists; it exits 1 if neither is present.

```bash
g++ -std=c++17 -DWFS_GPU_NATIVE=1 -I spatcore/gpu tools/test-gpu-plugin.cpp -ldl -o tools/test-gpu-plugin
./tools/test-gpu-plugin ./Builds/LinuxMakefile/build/libwfs_cuda.so 0 ; echo "exit=$?"
```

**Pass = 7/7 scenarios, exit 0.** Exit-code table in
`Documentation/GPU_Plugin_Smoke_Test.md`. Since the CUDA/HIP SDN reshape landed
after the last Linux validation, this is worth re-running even though it passed
before — and confirm the new mapping actually engages:

```bash
WFS_SDN_TRACE=1 ...        # expect "mapping=node-parallel chunk=<n>/<block>" with chunk > 1
WFS_SDN_NODE_PARALLEL=0    # forces the old lockstep; both must produce identical output
```

> **The stale-plugin trap is live on Linux** in a way it was not on macOS. On a
> Mac the backend is compiled into the app; here it is a dlopened `.so`. If you
> baseline against a plugin built before the submodule bump, the goldens pass
> *trivially*. Rebuild `libwfs_*.so` from the bumped submodule first.

## 4. Record the first Linux goldens

**There is no Linux baseline of any kind** — `baselines/` holds only `win-*` and
the `mac-m4pro*` pair added last session. CPU baselines are demonstrably *not*
portable (all 15 CPU hashes differ between `win-dev-nvidia.json` and
`mac-m4pro.json`), so you are creating two new files, not checking an existing
contract. That proves determinism and self-consistency; it cannot prove
"no regression".

```bash
cmake -S tools/validation/offline-render -B tools/validation/offline-render/build \
      -DCMAKE_BUILD_TYPE=Release          # MANDATORY: no default, and an empty
                                          # build type means no -O flag at all
cmake --build tools/validation/offline-render/build -j"$(nproc)"

cd tools/validation/offline-render
BIN=build/offline-render_artefacts/Release/offline-render

# CPU (no GPU needed). Rename to match the box.
./$BIN --path cpu --scenario all --check baselines/linux-<box>.json --update
./$BIN --path cpu --scenario all --check baselines/linux-<box>.json ; echo "exit=$?"
```

For GPU, `--plugin-dir` is a **no-op on POSIX** (it only prints a note; dlopen
does not re-read the environment). Co-locate the plugin beside the harness
binary instead — `GpuBackendFactory` tries `exeDir()` first:

```bash
ln -sf "$PWD/../../../Builds/LinuxMakefile/build/libwfs_cuda.so" \
       build/offline-render_artefacts/Release/libwfs_cuda.so

export LD_LIBRARY_PATH="${CUDA_PATH:-/usr/local/cuda}/lib64:$LD_LIBRARY_PATH"
./$BIN --path gpu --scenario all --device cuda:0 --check baselines/linux-<box>-gpu.json --update
./$BIN --path gpu --scenario all --device cuda:0 --check baselines/linux-<box>-gpu.json ; echo "exit=$?"
```

CPU and GPU must be **separate files and separate invocations** (the usage text
says so). Record at the **default shape** — the JSON stores no shape metadata, so
a baseline taken at a non-default `--in/--block/--sr` silently mismatches later.
Run each twice and diff: identical hashes across runs is the cheap determinism
check. Exit codes: 0 ok, 1 mismatch, 2 usage, 6 GPU/plugin unavailable, 7 GPU
runtime failure.

Optional but useful — the Apple-side analogue of the CUDA speedup table:

```bash
./$BIN --path gpu-reverb-sdn --scenario static --in 32 --block 256 --blocks 400 --bench
# compare against WFS_SDN_NODE_PARALLEL=0
```

*(Cross-check: on the Mac, `gpu-reverb-sdn/*` hashes came out byte-identical to
the CPU `reverb-sdn/*` hashes — expected, since the kernel preserves the CPU
summation order. If Linux GPU and CPU agree on **every** reverb path including
FDN and IR, that is the signal something fell back to CPU; SDN alone agreeing is
fine.)*

## 5. Merge order — there is a real hazard here

The parent pins spatcore `58f7811`, and
`git -C spatcore branch -r --contains HEAD` returns **exactly one** branch:
`origin/feat/metal-sdn-node-parallel`. If spatcore's PR is squash- or
rebase-merged **and the branch deleted**, that SHA becomes unreachable and every
`submodules: recursive` checkout breaks — all parent CI jobs, the release
workflow, and every fresh clone.

```
1. Merge spatcore first (prefer --no-ff, or tag the SHA, so it stays reachable).
2. Re-pin the parent to the resulting spatcore/main SHA.
3. Re-run  python3 tools/validation/kernel_hashes.py   (contents shouldn't move,
   but confirm — don't assume).
4. Commit the re-pin, then merge the parent.
```

The de-facto gate list is what `tools/bump-spatcore.ps1` prints (app build,
kernel hashes + dep lint, GPU plugins + smoke, offline-render CPU/GPU check,
control-replay, spatcore standalone tests). Its step 6 —
`control-replay: session_roundtrip / osc_replay / mcp_replay` under
`tools/validation/control-replay/` — was **not** investigated for this handoff;
treat it as an unknown you still need to run.

## 6. Known-stale things you may trip over

- **`tools/windows/prebuilt/wfs_hip.dll`** (tracked in git) was last refreshed
  2026-07-20, but `spatcore/gpu/HipSdnBackend.cpp` changed 2026-07-25 with the
  node-parallel port. It is stale **relative to `main`** — this branch did not
  cause it, and it is not a gate for this merge. Refreshing it needs
  **Windows + ROCm**, not Ubuntu. See `tools/windows/prebuilt/README.md`.
- **`Documentation/Linux_GPU_Enablement.md:51`** still claims a
  `WFS_GPU_HIP=1` / ROCm-linked LINUX_MAKE config is "the current Linux variant".
  Two architecture generations out of date — the app is plugin-mode now.
- **`Documentation/Beta_Distribution_Checklist.md`** is stale (says version
  0.1.0, "zero distribution infrastructure", "ships CPU-only"). The live release
  procedure is `docs/CI_SETUP.md:62-78`.
- **`spatcore/examples/minimal-app/CMakeLists.txt:41`** pins JUCE **8.0.14**
  with the comment "pinned to the version the apps use" — the only live
  JUCE-version disagreement with the 9.0.0 submodule.
- **Exporter version drift:** the generated exporters carry
  `JUCE_APP_VERSION=1.0.0beta38` while the `.jucer` and `JuceHeader.h` say
  **beta39** — a stale Projucer re-save. This branch does not bump the version.
- **In-tree resources:** the Linux exporter has no postbuild step, so `lang/` and
  the MCP resources are *not* copied next to the binary and there is no Linux
  dev-tree fallback in the resource search. Expect untranslated UI when running
  from the build tree; the tarball staging is what puts them in place.

## 7. Packaging (release-time, not a merge gate)

`tools/linux/build-app-tarball.sh` produces
`WFS-DIY-Linux-x86_64-<version>.tar.gz` and, when ROCm/CUDA are present, bundles
the full GPU runtime closure (hipRTC + comgr + device bitcode, or NVRTC) so the
target needs no SDK. **It does not run `make` itself** — do a clean Release build
first. Runbook: `docs/Linux_AMD_Release_Tarball.md`. Nothing on this branch
invalidates a Linux packaging artifact, so leave this for the next release cut.

## Report back

- Did the JUCE 9 dep list fix the compile? If not, the exact errors.
- The four CI steps: pass/fail each.
- Smoke test output + exit code, and whether `WFS_SDN_TRACE=1` shows
  `chunk > 1` (i.e. the node-parallel mapping really ran).
- The two new baseline files, plus confirmation they re-check green twice.
- Anything in §6 that turned out to be worse than described.
