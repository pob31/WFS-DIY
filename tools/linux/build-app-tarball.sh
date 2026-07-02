#!/usr/bin/env bash
# Build a Linux release tarball for the WFS-DIY standalone app.
#
# Usage:
#   tools/linux/build-app-tarball.sh
#
# Produces:
#   Builds/LinuxMakefile/release/WFS-DIY-Linux-x86_64-<version>.tar.gz
#
# Env overrides (used by CI to match the release asset naming convention):
#   OUTPUT_DIR    where to write the tarball   (default: Builds/LinuxMakefile/release)
#   TARBALL_NAME  archive basename + top dir   (default: WFS-DIY-Linux-<arch>-<version>)
#   BUNDLE_GPU    auto|on|off  bundle GPU plugins + runtime compilers (default: auto)
#
# The tarball contains the binary, lang/ and MCP/resources/ runtime data,
# the udev rules for HID controllers + touchscreens, the JUCE patch + apply
# script, an install.sh and uninstall.sh, and a .desktop entry. Mirrors the
# macOS .pkg / Windows .exe release flow on Linux.
#
# GPU bundling (BUNDLE_GPU): when ROCm (/opt/rocm) and/or the CUDA toolkit
# (/usr/local/cuda) are present, this also builds the per-vendor GPU plugins
# (libwfs_hip.so / libwfs_cuda.so) and bundles their FULL runtime closure so the
# app can do GPU compute on a target that has NO ROCm/CUDA SDK installed:
#   - the GPU runtime libs (libamdhip64 / libcudart, ...);
#   - the RUNTIME KERNEL COMPILERS the backends call in prepare() — hipRTC (with
#     libamd_comgr + the amdgcn device bitcode) for AMD, and NVRTC (+ builtins)
#     for NVIDIA. These are what "the CUDA and HIP compiler binaries" means here:
#     kernels are compiled at runtime from string literals, not shipped prebuilt.
# What is NEVER bundled: the kernel-mode driver. The target still needs amdgpu/KFD
# (AMD) or the NVIDIA driver providing libcuda.so.1 (NVIDIA). Without that, the
# app loads fine and falls back to the CPU path.
#
# Pre-requisite: a clean Release build of the app must already be made
#   (cd Builds/LinuxMakefile && make clean && make CONFIG=Release -j$(nproc))
# This script does not invoke make itself; that's a separate concern, and
# keeping them split lets CI parallelise build vs. package. The `make clean`
# is part of the release flow specifically — incremental builds are fine for
# day-to-day dev work, but shipping artefacts get a clean rebuild.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="$REPO_ROOT/Builds/LinuxMakefile/build"
RELEASE_DIR="${OUTPUT_DIR:-$REPO_ROOT/Builds/LinuxMakefile/release}"
BUNDLE_GPU="${BUNDLE_GPU:-auto}"

BIN="$BUILD_DIR/WFS-DIY"
if [[ ! -x "$BIN" ]]; then
    echo "ERROR: $BIN not found or not executable." >&2
    echo "       Run 'cd Builds/LinuxMakefile && make clean && make CONFIG=Release -j\$(nproc)' first." >&2
    exit 1
fi

# Pull version from the JUCERPROJECT element's version attribute (single
# source of truth). The element spans multiple lines, so use perl in slurp
# mode to skip past the unrelated <?xml version="1.0"?> declaration.
VERSION="$(perl -0777 -ne '/<JUCERPROJECT\b[^>]*\sversion="([^"]+)"/s and print $1' "$REPO_ROOT/WFS-DIY.jucer")"
if [[ -z "$VERSION" ]]; then
    echo "ERROR: could not read version from WFS-DIY.jucer" >&2
    exit 1
fi

ARCH="$(uname -m)"
STAGE_NAME="${TARBALL_NAME:-WFS-DIY-Linux-${ARCH}-${VERSION}}"
STAGE_DIR="$RELEASE_DIR/$STAGE_NAME"
TARBALL="$RELEASE_DIR/$STAGE_NAME.tar.gz"

echo "==> Staging $STAGE_NAME"
rm -rf "$STAGE_DIR"
mkdir -p "$STAGE_DIR"

# Binary + runtime resources. We read lang/ and MCP/resources/ straight from
# the source-of-truth directories rather than the Makefile postbuild output —
# Projucer regens on macOS/Windows wipe the postbuild step, and we don't want
# the tarball to fall back to whatever stale lang/ files happened to be left
# next to the binary from a previous build.
cp "$BIN" "$STAGE_DIR/WFS-DIY"
mkdir -p "$STAGE_DIR/lang" "$STAGE_DIR/MCP/resources"
cp -R "$REPO_ROOT/Resources/lang/."                "$STAGE_DIR/lang/"
cp -R "$REPO_ROOT/Documentation/MCP/resources/."   "$STAGE_DIR/MCP/resources/"
# The generated tool surface (MCP server's list of AI-callable tools) --
# without this next to the binary, exeDir.getChildFile("MCP/generated_tools.json")
# finds nothing and MCP tool-calling silently comes up empty.
cp "$REPO_ROOT/Source/Network/MCP/generated_tools.json" "$STAGE_DIR/MCP/"

# ----------------------------------------------------------------------------
# GPU bundle (optional): per-vendor plugins + their full runtime closure so the
# app does GPU compute on a machine without a ROCm/CUDA SDK. See header comment.
# ----------------------------------------------------------------------------
GPU_BUNDLED=0
ROCM_DIR="${ROCM_PATH:-/opt/rocm}"
CUDA_DIR="${CUDA_PATH:-/usr/local/cuda}"

bundle_gpu() {
    local gpulib="$STAGE_DIR/gpu/lib"
    mkdir -p "$gpulib"

    # Build whichever plugins the host toolchains support, straight into the
    # staging root (the app's GpuBackendFactory loads libwfs_<vendor>.so from
    # beside the executable). build-gpu-plugins.sh no-ops cleanly per toolchain.
    echo "==> Building GPU plugins for the tarball"
    "$REPO_ROOT/tools/linux/build-gpu-plugins.sh" "$STAGE_DIR" || true

    # cp_lib <name-glob> <srcdir> : copy a lib + its soname/symlink farm (cp -a
    # keeps the symlinks so dlopen-by-any-name resolves), tolerating absence.
    cp_lib() { local g="$1" d="$2"; if compgen -G "$d/$g" >/dev/null; then cp -a "$d"/$g "$gpulib/"; else echo "   (skip $g — not in $d)"; fi; }

    # cp_lib_sys <soname> : resolve a system lib via ldconfig and copy it into
    # gpu/lib under that soname (for GPU deps not guaranteed on a minimal target).
    cp_lib_sys() {
        local n="$1" p
        p="$(ldconfig -p 2>/dev/null | awk -v n="$n" '$1==n && /x86-64/ {print $NF; exit}')"
        [[ -z "$p" ]] && p="$(ldconfig -p 2>/dev/null | awk -v n="$n" '$1==n {print $NF; exit}')"
        if [[ -n "$p" ]] && p="$(readlink -f "$p" 2>/dev/null)" && [[ -f "$p" ]]; then
            cp "$p" "$gpulib/$n"
        else echo "   (skip $n — not found via ldconfig)"; fi
    }

    # ---- AMD / HIP closure (verified via strace of the running plugin) -------
    if [[ -f "$STAGE_DIR/libwfs_hip.so" ]]; then
        echo "==> Bundling HIP runtime + hipRTC compiler closure"
        local rl="$ROCM_DIR/lib"
        # Base-name globs (version-agnostic across ROCm majors). libnuma/libelf
        # are system libs (handled by cp_lib_sys below). libhsa-amd-aqlprofile64
        # is deliberately NOT bundled: it ships under AMD's proprietary GPU-Pro
        # EULA (no redistribution) and is only a lazy profiler dlopen by
        # hsa-runtime — not needed for compute, which tolerates its absence.
        for g in libamdhip64.so. libhiprtc.so. libamd_comgr.so. \
                 libhsa-runtime64.so. librocprofiler-register.so.; do
            cp_lib "${g}*" "$rl"
        done
        for g in libdrm.so. libdrm_amdgpu.so.; do
            cp_lib "${g}*" "/opt/amdgpu/lib/x86_64-linux-gnu"
        done
        # Device bitcode (ocml/ockl/hip.bc/oclc_*) — comgr finds these RELATIVE to
        # its own .so: <root>/llvm/lib/clang/<N>/lib/amdgcn/bitcode where comgr is
        # at <root>/lib. Replicate that exact relative layout under gpu/.
        # comgr (at gpu/lib) finds the bitcode relative to itself, under
        # llvm/lib/clang/<N>/lib/amdgcn/bitcode — replicate that exact layout.
        # Pick the highest clang major numerically (sort -V), not lexically.
        local bitsrc rel
        bitsrc="$(ls -d "$ROCM_DIR"/llvm/lib/clang/*/lib/amdgcn/bitcode 2>/dev/null | sort -V | tail -1)"
        if [[ -n "$bitsrc" ]]; then
            rel="${bitsrc#"$ROCM_DIR"/}"                 # llvm/lib/clang/<N>/lib/amdgcn/bitcode
        else
            # Older ROCm layout: bitcode at $ROCM/amdgcn/bitcode, but comgr still
            # looks under the llvm-relative path — derive <N> from the clang tree.
            local cdir; cdir="$(ls -d "$ROCM_DIR"/llvm/lib/clang/* 2>/dev/null | sort -V | tail -1)"
            bitsrc="$(ls -d "$ROCM_DIR"/amdgcn/bitcode 2>/dev/null | head -1)"
            [[ -n "$cdir" && -n "$bitsrc" ]] && rel="llvm/lib/clang/$(basename "$cdir")/lib/amdgcn/bitcode"
        fi
        if [[ -n "$bitsrc" && -n "${rel:-}" ]] && compgen -G "$bitsrc/*.bc" >/dev/null; then
            mkdir -p "$STAGE_DIR/gpu/$rel"
            cp "$bitsrc"/*.bc "$STAGE_DIR/gpu/$rel/"
            echo "   device bitcode -> gpu/$rel ($(ls "$STAGE_DIR/gpu/$rel"/*.bc | wc -l) files)"
        else
            echo "   WARNING: amdgcn device bitcode not found — HIP kernel compile will fail on target." >&2
        fi
        GPU_BUNDLED=1
    fi

    # ---- NVIDIA / CUDA closure (NVRTC is self-contained: CUDA-C -> PTX) -------
    # libcuda.so.1 is the NVIDIA driver — intentionally NOT bundled. Untested
    # here (no NVIDIA GPU on the build box); manifest is the known NVRTC set.
    if [[ -f "$STAGE_DIR/libwfs_cuda.so" ]]; then
        echo "==> Bundling CUDA runtime + NVRTC compiler closure"
        local cl="$CUDA_DIR/lib64"
        # Base-name globs (version-agnostic across CUDA majors). nvrtc dlopens
        # libnvrtc-builtins by its major.minor soname, which the glob captures.
        for g in libcudart.so. libnvrtc.so. libnvrtc-builtins.so.; do
            cp_lib "${g}*" "$cl"
        done
        GPU_BUNDLED=1
    fi

    if [[ "$GPU_BUNDLED" != 1 ]]; then
        rm -rf "$STAGE_DIR/gpu"          # toolkit dir present but no plugin built
        return
    fi

    # System libs the GPU runtime needs that aren't guaranteed on a minimal
    # target: libamdhip64 needs numa/elf; comgr/LLVM needs z/zstd. (libcuda.so.1
    # is the NVIDIA *driver* and is deliberately never bundled.)
    for s in libnuma.so.1 libelf.so.1 libz.so.1 libzstd.so.1; do cp_lib_sys "$s"; done

    # Fail loudly on under-bundling (e.g. a soname the globs missed on a
    # different ROCm/CUDA major): every plugin's runtime deps must resolve within
    # gpu/lib — only libcuda.so.1 (the NVIDIA driver) may remain external.
    local p miss
    for p in "$STAGE_DIR"/libwfs_*.so; do
        miss="$(LD_LIBRARY_PATH="$gpulib" ldd "$p" 2>/dev/null | awk '/not found/ && $1!="libcuda.so.1"{print $1}')"
        if [[ -n "$miss" ]]; then
            echo "ERROR: $(basename "$p") has unbundled runtime deps:" $miss >&2
            echo "       (soname drift? widen the cp_lib globs in bundle_gpu.)" >&2
            exit 1
        fi
    done

    # Redistribution attribution: stage each bundled lib's license text. The AMD
    # ROCm compute libs are MIT/BSD/NCSA/Apache-with-LLVM-exception, the amdgcn
    # bitcode is NCSA, and CUDA libcudart/nvrtc are redistributable under the
    # CUDA EULA (Attachment A) — all permit bundling but require their notices.
    local licd="$STAGE_DIR/gpu/LICENSES"; mkdir -p "$licd"
    local d f
    for d in hip amd_comgr hsa-runtime64 rocprofiler-register ROCm-Device-Libs rocm-llvm rocm-core; do
        for f in "$ROCM_DIR"/share/doc/"$d"/LICENSE* "$ROCM_DIR"/share/doc/"$d"/NOTICES*; do
            [[ -f "$f" ]] && cp "$f" "$licd/${d}.$(basename "$f")"
        done
    done
    [[ -f "$CUDA_DIR/EULA.txt" ]] && cp "$CUDA_DIR/EULA.txt" "$licd/CUDA-EULA.txt"
    local c
    for c in libdrm2-amdgpu libdrm-amdgpu-amdgpu1 libdrm2 libnuma1 libelf1 libelf1t64 zlib1g libzstd1; do
        [[ -f "/usr/share/doc/$c/copyright" ]] && cp "/usr/share/doc/$c/copyright" "$licd/${c}.copyright"
    done
    rmdir "$licd" 2>/dev/null || true    # drop if nothing was found

    # Launcher: point the loader at the bundled GPU libs, then exec the real
    # binary so /proc/self/exe (used to find the plugins) is the binary, not the
    # script. Works both in-place and installed. LD_LIBRARY_PATH beats the
    # plugins' baked /opt/rocm rpath; comgr finds its bitcode relative to itself.
    mv "$STAGE_DIR/WFS-DIY" "$STAGE_DIR/WFS-DIY.bin"
    cat > "$STAGE_DIR/WFS-DIY" <<'LAUNCH_EOF'
#!/bin/sh
# WFS-DIY launcher — sets up the bundled GPU runtime, then runs the app.
HERE="$(cd "$(dirname "$(readlink -f "$0")")" && pwd)"
if   [ -f "$HERE/WFS-DIY.bin" ];                     then APP="$HERE"
elif [ -f "$HERE/../share/wfs-diy/WFS-DIY.bin" ];    then APP="$(cd "$HERE/../share/wfs-diy" && pwd)"
else APP="$HERE"; fi
[ -d "$APP/gpu/lib" ] && export LD_LIBRARY_PATH="$APP/gpu/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
exec "$APP/WFS-DIY.bin" "$@"
LAUNCH_EOF
    chmod +x "$STAGE_DIR/WFS-DIY"
    echo "==> GPU bundle: $(du -sh "$STAGE_DIR/gpu" | cut -f1) ($(ls "$STAGE_DIR"/libwfs_*.so 2>/dev/null | xargs -n1 basename | paste -sd, -))"
}

case "$BUNDLE_GPU" in
    off) echo "==> GPU bundling disabled (BUNDLE_GPU=off)";;
    on|auto)
        if [[ -d "$ROCM_DIR" || -d "$CUDA_DIR" ]]; then
            bundle_gpu
        elif [[ "$BUNDLE_GPU" == on ]]; then
            echo "ERROR: BUNDLE_GPU=on but neither $ROCM_DIR nor $CUDA_DIR found." >&2; exit 1
        else
            echo "==> No ROCm/CUDA toolkit found — building CPU-only tarball."
        fi;;
    *) echo "ERROR: BUNDLE_GPU must be auto|on|off (got '$BUNDLE_GPU')" >&2; exit 1;;
esac

# udev rules and the JUCE patch infrastructure.
mkdir -p "$STAGE_DIR/share"
cp "$REPO_ROOT/assets/linux/70-wfs-diy.rules" "$STAGE_DIR/share/"
cp "$REPO_ROOT/Resources/WFS-DIY_logo.png"     "$STAGE_DIR/share/"

# .desktop entry — installer rewrites Exec=/Icon= paths during install.
cat > "$STAGE_DIR/share/wfs-diy.desktop" <<'EOF'
[Desktop Entry]
Type=Application
Name=WFS-DIY
GenericName=Wave Field Synthesis Workstation
Comment=Live spatial audio app for Wave Field Synthesis rigs
Exec=__PREFIX__/bin/WFS-DIY
Icon=__PREFIX__/share/wfs-diy/WFS-DIY_logo.png
Terminal=false
Categories=AudioVideo;Audio;
StartupNotify=true
EOF

# Install script — supports per-user (~/.local) and system (/opt + /usr/local) layouts.
cat > "$STAGE_DIR/install.sh" <<'INSTALL_EOF'
#!/usr/bin/env bash
# WFS-DIY app installer.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"

MODE="${1:-}"
case "$MODE" in
    --user)   PREFIX="$HOME/.local"; SUDO="" ;;
    --system) PREFIX="/opt/wfs-diy"; SUDO="sudo" ;;
    *)
        echo "Usage: $0 [--user | --system]"
        echo
        echo "  --user    Install for current user only (~/.local). Default if stdin is a tty."
        echo "  --system  Install system-wide to /opt/wfs-diy (requires sudo)."
        if [[ -t 0 ]]; then
            read -r -p "Install for current user only? [Y/n] " ans
            case "${ans,,}" in n|no) MODE="--system"; PREFIX="/opt/wfs-diy"; SUDO="sudo" ;; *) MODE="--user"; PREFIX="$HOME/.local"; SUDO="" ;; esac
        else
            exit 1
        fi
        ;;
esac

BIN_DIR="$PREFIX/bin"
APP_DIR="$PREFIX/share/wfs-diy"
DESKTOP_DIR="$PREFIX/share/applications"

echo "==> Installing to $PREFIX (mode: $MODE)"
$SUDO mkdir -p "$BIN_DIR" "$APP_DIR" "$DESKTOP_DIR"

if [[ -f "$HERE/WFS-DIY.bin" ]]; then
    # GPU build: the real binary, the plugins and the GPU runtime live together
    # in APP_DIR (the plugins are found relative to the binary); bin/WFS-DIY is
    # the launcher that wires up the bundled GPU libs.
    $SUDO install -m 0755 "$HERE/WFS-DIY.bin"             "$APP_DIR/WFS-DIY.bin"
    $SUDO install -m 0755 "$HERE/WFS-DIY"                 "$BIN_DIR/WFS-DIY"
    for so in "$HERE"/libwfs_*.so; do [[ -e "$so" ]] && $SUDO install -m 0755 "$so" "$APP_DIR/"; done
    $SUDO cp -a          "$HERE/gpu"                      "$APP_DIR/gpu"
else
    $SUDO install -m 0755 "$HERE/WFS-DIY"                 "$BIN_DIR/WFS-DIY"
fi
$SUDO cp -R          "$HERE/lang"                      "$APP_DIR/lang"
$SUDO cp -R          "$HERE/MCP"                       "$APP_DIR/MCP"
$SUDO install -m 0644 "$HERE/share/WFS-DIY_logo.png"   "$APP_DIR/WFS-DIY_logo.png"

# Write a prefix-baked uninstaller into $APP_DIR so users can simply run it
# without remembering which prefix they installed against. The inner heredoc
# is unquoted: $PREFIX is expanded here at install time and baked in, while
# \$PREFIX / \$SUDO / \$HOME stay literal so they are evaluated when uninstall
# runs.
$SUDO tee "$APP_DIR/uninstall.sh" >/dev/null <<UNINSTALL_INNER
#!/usr/bin/env bash
set -euo pipefail
PREFIX="$PREFIX"
SUDO=""; [[ "\$PREFIX" != "\$HOME/"* ]] && SUDO="sudo"

\$SUDO rm -f  "\$PREFIX/bin/WFS-DIY"
\$SUDO rm -f  "\$PREFIX/share/applications/wfs-diy.desktop"
if [[ -f /etc/udev/rules.d/70-wfs-diy.rules ]]; then
    sudo rm -f /etc/udev/rules.d/70-wfs-diy.rules
    sudo udevadm control --reload-rules || true
fi
# Remove the share dir last — this deletes the running script too, but bash
# has already loaded it into memory so the rest of the script still executes.
\$SUDO rm -rf "\$PREFIX/share/wfs-diy"
echo "Uninstalled WFS-DIY from \$PREFIX."
UNINSTALL_INNER
$SUDO chmod +x "$APP_DIR/uninstall.sh"

# .desktop entry, with prefix substituted.
sed "s|__PREFIX__|$PREFIX|g" "$HERE/share/wfs-diy.desktop" \
    | $SUDO tee "$DESKTOP_DIR/wfs-diy.desktop" >/dev/null

if command -v update-desktop-database >/dev/null 2>&1; then
    $SUDO update-desktop-database "$DESKTOP_DIR" || true
fi

# udev rules — always system-wide; offer to install.
echo
if [[ -t 0 ]]; then
    read -r -p "Install udev rules for HID controllers + touchscreens (sudo)? [Y/n] " ans
    case "${ans,,}" in n|no) ;; *)
        sudo install -m 0644 "$HERE/share/70-wfs-diy.rules" /etc/udev/rules.d/70-wfs-diy.rules
        sudo udevadm control --reload-rules
        sudo udevadm trigger
        echo "    udev rules installed. Replug HID/touchscreen devices to apply."
        ;;
    esac
else
    echo "Stdin not a tty — skipping udev rules. Copy share/70-wfs-diy.rules to /etc/udev/rules.d/ manually if needed."
fi

echo
echo "Done. Launch from your application menu, or run: $BIN_DIR/WFS-DIY"
echo "To uninstall: ${SUDO:+$SUDO }$APP_DIR/uninstall.sh"
INSTALL_EOF
chmod +x "$STAGE_DIR/install.sh"

cat > "$STAGE_DIR/uninstall.sh" <<'UNINSTALL_EOF'
#!/usr/bin/env bash
set -euo pipefail
PREFIX="${1:-$HOME/.local}"
SUDO=""; [[ "$PREFIX" != "$HOME/"* ]] && SUDO="sudo"

$SUDO rm -f  "$PREFIX/bin/WFS-DIY"
$SUDO rm -rf "$PREFIX/share/wfs-diy"
$SUDO rm -f  "$PREFIX/share/applications/wfs-diy.desktop"
[[ -n "$SUDO" && -f /etc/udev/rules.d/70-wfs-diy.rules ]] \
    && sudo rm -f /etc/udev/rules.d/70-wfs-diy.rules \
    && sudo udevadm control --reload-rules || true

echo "Uninstalled WFS-DIY from $PREFIX."
UNINSTALL_EOF
chmod +x "$STAGE_DIR/uninstall.sh"

# README for the tarball
GPU_README=""
if [[ "$GPU_BUNDLED" == 1 ]]; then
    GPU_README="
GPU acceleration
    This build bundles the per-vendor GPU plugins (libwfs_*.so) and their full
    runtime + runtime-kernel-compiler closure under gpu/, so GPU compute works
    WITHOUT a ROCm/CUDA SDK installed. You still need the kernel-mode driver:
      - AMD:    the amdgpu/KFD kernel driver (in-tree is fine).
      - NVIDIA: the proprietary driver (provides libcuda.so.1).
    Without a supported GPU/driver the app simply runs on the CPU.
    Run ./WFS-DIY (the launcher wires up gpu/lib); WFS-DIY.bin is the raw binary.
"
fi
cat > "$STAGE_DIR/README.txt" <<EOF
WFS-DIY ${VERSION} — Linux ${ARCH}

Quick install:
    ./install.sh --user      # ~/.local (no sudo)
    ./install.sh --system    # /opt/wfs-diy (sudo)

Or just run in place:
    ./WFS-DIY

The app expects lang/ and MCP/ to sit next to the binary, which they do
in this archive. udev rules under share/70-wfs-diy.rules grant non-root
access to Stream Deck, Xencelabs Quick Keys, 3Dconnexion SpaceMouse and
USB touchscreens. The installer offers to drop them in /etc/udev/rules.d
on request.
${GPU_README}
Source: https://github.com/pob31/WFS-DIY
EOF

# Build the tarball.
cd "$RELEASE_DIR"
echo "==> Creating $TARBALL"
tar --owner=0 --group=0 -czf "$TARBALL" "$STAGE_NAME"

# Cleanup the staging dir after archiving (the tarball is the artefact).
rm -rf "$STAGE_DIR"

echo
ls -lh "$TARBALL"
echo "Done."
