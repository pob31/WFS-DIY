# Linux AMD (ROCm/HIP) release tarball — handoff runbook

**Run this on the Ubuntu + AMD-GPU box, once per release, after the GitHub
Release is published.** It produces the supplementary Linux tarball that carries
the AMD GPU plugin (`libwfs_hip.so`) plus its full ROCm runtime closure, and
attaches it to the release.

## Why this is a manual step (and why *this* box)

Release CI builds the Linux tarball on a shared GitHub runner. It installs the
CUDA toolkit (so the NVIDIA plugin `libwfs_cuda.so` + its runtime closure get
bundled), but it does **not** install ROCm — a full ROCm install is multi-GB and
slow/flaky on a shared runner, and the Linux HIP runtime closure is far too large
to commit as a prebuilt blob the way Windows commits `wfs_hip.dll`. So the
CI Linux tarball serves CPU and NVIDIA users; AMD-on-Linux users fall back to CPU.

This box closes that gap, and it is the *only* machine that can — not just because
it has ROCm installed, but because it has a real AMD GPU. That lets it do the one
thing neither CI nor the Windows dev box can: **functionally run the HIP kernels
on AMD hardware** and confirm the plugin works before it ships (step 4 below).

Result: each published release ends up with two Linux assets —

| Asset | Built by | GPU acceleration for |
|---|---|---|
| `WFS-DIY-<tag>-linux-x86_64.tar.gz` | CI (automatic) | CPU + **NVIDIA/CUDA** |
| `WFS-DIY-<tag>-linux-x86_64-rocm.tar.gz` | this runbook (manual) | CPU + **AMD/HIP** |

The `-rocm` tarball is purely additive: if you skip it, nothing breaks — AMD
users just fall back to CPU as they do today.

---

## One-time box setup

- **ROCm** installed at `/opt/rocm` (or export `ROCM_PATH`) with `hipcc` on `PATH`:
  ```bash
  hipcc --version && rocminfo | grep -m1 gfx    # confirm the toolchain + a GPU
  ```
  Validated toolchains so far: gfx1103 (Radeon 780M iGPU) on ROCm 6.4/7.1. Any
  ROCm the box runs is fine — the tarball bundles whatever runtime it built against.
- **JUCE build dependencies** (same set CI installs):
  ```bash
  sudo apt-get update && sudo apt-get install -y \
    build-essential pkg-config \
    libasound2-dev libjack-jackd2-dev \
    libfreetype-dev libfontconfig1-dev \
    libgl1-mesa-dev libcurl4-openssl-dev \
    libgtk-3-dev libwebkit2gtk-4.1-dev libudev-dev libxi-dev
  ```
- **GitHub CLI** for attaching the asset:
  ```bash
  gh auth status || gh auth login      # needs write access to pob31/WFS-DIY
  ```

> **Do NOT install the CUDA toolkit on this box** unless you deliberately want a
> universal tarball. With both ROCm *and* CUDA present, `BUNDLE_GPU=auto` builds
> *both* plugins and the resulting tarball serves both vendors — see the last
> section. The default AMD-only flow below assumes ROCm only.

---

## Release steps

Set the tag you are building once; every block below reuses it:

```bash
export TAG=v1.0.0beta27          # the published release tag, exactly
cd ~/dev/WFS-DIY                 # your checkout of pob31/WFS-DIY
```

### 1. Sync to the exact release tag (with submodules)

This is load-bearing: the tarball's version comes from `WFS-DIY.jucer` at this
commit, and the GPU kernels come from the **spatcore submodule pinned in the
tag's tree** — checking out the tag with submodules guarantees you build the
exact sources the release was cut from.

```bash
git fetch --tags origin
git checkout "$TAG"
git submodule update --init --recursive
```

(Tags before the JUCE 9 migration additionally need
`tools/apply-linux-juce-patches.sh` from that tag's tree — since JUCE 9,
touch input is native and the submodule builds unpatched.)

### 2. Clean Release build of the app

```bash
cd Builds/LinuxMakefile
make clean && make CONFIG=Release -j"$(nproc)"
cd ../..
```

### 3. Build the AMD plugin standalone (for the smoke test in step 4)

```bash
tools/linux/build-gpu-plugins.sh
# -> Builds/LinuxMakefile/build/libwfs_hip.so   (skips CUDA cleanly if absent)
```

### 4. Functionally verify the plugin on the AMD GPU — the reason this runs here

Build the smoke-test tool and run it against the freshly built `.so`. It drives
all five kernel families (WFS, OutputBuffer, IR, FDN, SDN) on the real device.
**Exit 0 = the AMD GPU path works.** This is the on-hardware confirmation CI
cannot give.

```bash
( cd tools && g++ -std=c++17 -DWFS_GPU_NATIVE=1 -I../spatcore/gpu \
      test-gpu-plugin.cpp -ldl -o test-gpu-plugin )
tools/test-gpu-plugin Builds/LinuxMakefile/build/libwfs_hip.so ; echo "exit=$?"
```

- Expect `exit=0`. If it exits non-zero with a "no ROCm-capable device" message,
  the GPU/driver isn't visible to this shell — fix that before shipping (a
  tarball that bundles a plugin you never ran on hardware defeats the purpose).
- If the library fails to *load* (unresolved `hiprtc`/`amdhip64` symbols),
  prepend the runtime dir: `LD_LIBRARY_PATH=/opt/rocm/lib tools/test-gpu-plugin …`.

### 5. Build the release tarball with the AMD plugin + runtime closure

`build-app-tarball.sh` rebuilds the plugin into a staging tree and bundles its
**full** runtime closure — the HIP runtime, the hipRTC kernel compiler,
`amd_comgr`, and the amdgcn device bitcode — so the app does GPU compute on a
target that has *no* ROCm SDK installed (only the amdgpu/KFD kernel driver is
needed). The script fails loudly if any runtime dependency is left unbundled.

```bash
OUTPUT_DIR="$PWD/dist" \
TARBALL_NAME="WFS-DIY-${TAG}-linux-x86_64-rocm" \
  tools/linux/build-app-tarball.sh
```

Sanity-check that the AMD plugin actually made it in:

```bash
tar tzf "dist/WFS-DIY-${TAG}-linux-x86_64-rocm.tar.gz" | grep -E 'libwfs_hip\.so|gpu/lib/' | head
```

### 6. Checksum and attach to the release

```bash
TAR="dist/WFS-DIY-${TAG}-linux-x86_64-rocm.tar.gz"
sha256sum "$TAR" | awk '{print $1}' > "$TAR.sha256"
gh release upload "$TAG" "$TAR" "$TAR.sha256" --clobber
```

`--clobber` lets you re-run the upload if you rebuild. Done — the release now has
both Linux tarballs.

---

## Troubleshooting

- **`skip libwfs_hip.so (hipcc not found)`** then `No GPU toolchain found` — `hipcc`
  isn't on `PATH`. `source /opt/rocm/bin/env` or add `/opt/rocm/bin` to `PATH`.
- **`ERROR: … has unbundled runtime deps:`** from the tarball script — a ROCm
  major bump moved/renamed a runtime `.so` the bundler's globs didn't catch. Widen
  the `cp_lib` globs in `bundle_gpu()` inside `tools/linux/build-app-tarball.sh`
  (it names the missing soname).
- **`WARNING: amdgcn device bitcode not found`** — the ROCm device-libs aren't where
  the bundler looks (`$ROCM/llvm/lib/clang/*/lib/amdgcn/bitcode` or the older
  `$ROCM/amdgcn/bitcode`). Without the bitcode, hipRTC can't compile kernels on the
  target; install `rocm-device-libs` (or the ROCm `llvm` package) on this box.
- **Version mismatch in the tarball name vs the app** — you didn't check out `$TAG`
  before building (step 1). The internal version is read from `WFS-DIY.jucer`.

---

## Optional: one universal Linux tarball instead of two

If this box *also* has the CUDA toolkit at `/usr/local/cuda`, `BUNDLE_GPU=auto`
builds **both** `libwfs_cuda.so` and `libwfs_hip.so` and bundles both runtime
closures. That single tarball serves NVIDIA *and* AMD users, and you could name it
`WFS-DIY-${TAG}-linux-x86_64` (no suffix) and upload it with `--clobber` to
*replace* the CI-built NVIDIA-only tarball. Trade-off: the release's Linux asset
then depends on this manual step every time, instead of CI producing a usable
(NVIDIA+CPU) tarball automatically. Prefer the two-asset default unless you have a
reason to consolidate.
