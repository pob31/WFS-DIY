#!/usr/bin/env bash
#
# build-gpu-plugins.sh — build the per-vendor GPU plugin shared libraries
# (Phase 3 Stage 3). The CPU-safe main app dlopens these at runtime per the
# user-selected device; the app itself links no GPU runtime.
#
# Builds whichever toolchains are present:
#   libwfs_hip.so   (AMD/ROCm)   — if hipcc / /opt/rocm is available
#   libwfs_cuda.so  (NVIDIA/CUDA) — if a CUDA toolkit is available
#
# Usage: tools/linux/build-gpu-plugins.sh [OUTPUT_DIR]
#   OUTPUT_DIR defaults to Builds/LinuxMakefile/build (next to the app binary).

set -euo pipefail

HERE="$(cd "$(dirname "$0")/../../.." && pwd)"       # repo root
SRC="$HERE/spatcore/gpu"
OUT="${1:-$HERE/Builds/LinuxMakefile/build}"
mkdir -p "$OUT"

PLUGIN="$SRC/plugin/GpuVendorPlugin.cpp"
HIP_SOURCES=("$SRC"/Hip{Wfs,Ob,Ir,Fdn,Sdn}Backend.cpp)
CUDA_SOURCES=("$SRC"/Cuda{Wfs,Ob,Ir,Fdn,Sdn}Backend.cpp)

built=0

# ---- AMD / HIP ----
ROCM="${ROCM_PATH:-/opt/rocm}"
if command -v hipcc >/dev/null 2>&1; then
    echo "Building libwfs_hip.so (hipcc) ..."
    # hipcc auto-links the HIP runtime (amdhip64) but NOT hiprtc, the runtime
    # kernel-compiler the backends call (hiprtcCreateProgram/Compile/GetCode/...).
    # A .so links with undefined symbols allowed, so this is silent at build time,
    # but the app dlopens with RTLD_NOW -> the unresolved hiprtc symbols would make
    # the load fail and fall back to CPU. Link hiprtc explicitly (the CUDA branch
    # below already links -lnvrtc); rpath the ROCm lib dir so it resolves at runtime.
    hipcc -fPIC -shared -std=c++17 -O2 \
        -DWFS_GPU_NATIVE=1 -DWFS_GPU_HIP=1 \
        -I"$SRC" \
        "${HIP_SOURCES[@]}" "$PLUGIN" \
        -L"$ROCM/lib" -lhiprtc -Wl,-rpath,"$ROCM/lib" \
        -o "$OUT/libwfs_hip.so"
    echo "  -> $OUT/libwfs_hip.so"
    built=1
else
    echo "skip libwfs_hip.so (hipcc not found)"
fi

# ---- NVIDIA / CUDA ----
CUDA="${CUDA_PATH:-/usr/local/cuda}"
if [ -d "$CUDA/include" ]; then
    echo "Building libwfs_cuda.so (g++ + CUDA $CUDA) ..."
    g++ -fPIC -shared -std=c++17 -O2 \
        -DWFS_GPU_NATIVE=1 -D__HIP_PLATFORM_NONE__ \
        -I"$SRC" -I"$CUDA/include" \
        "${CUDA_SOURCES[@]}" "$PLUGIN" \
        -L"$CUDA/lib64" -L"$CUDA/lib64/stubs" -lcudart -lnvrtc -lcuda \
        -Wl,-rpath,"$CUDA/lib64" \
        -o "$OUT/libwfs_cuda.so"
    echo "  -> $OUT/libwfs_cuda.so"
    built=1
else
    echo "skip libwfs_cuda.so (no CUDA toolkit at $CUDA)"
fi

[ "$built" = 1 ] && echo "Done." || { echo "No GPU toolchain found — nothing built." >&2; exit 1; }
