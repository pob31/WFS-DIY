<#
    build-gpu-plugins.ps1 — build the per-vendor GPU plugin DLLs on Windows
    (Phase 3, the Windows twin of tools/linux/build-gpu-plugins.sh).

    The CPU-safe main app (built with WFS_GPU_PLUGINS) LoadLibrary's these at
    runtime per the user-selected device; the app itself links no GPU runtime.

    Builds whichever toolchains are present:
        wfs_cuda.dll   (NVIDIA/CUDA)  — needs the CUDA Toolkit (cl.exe + CUDA_PATH)
        wfs_hip.dll    (AMD/HIP)      — needs the AMD HIP SDK for Windows (hipcc)

    Besides the plugin DLLs, this stages the CUDA runtime DLLs the cuda plugin
    loads at runtime (CUDA Runtime + NVRTC) next to it, so a machine with only the
    NVIDIA driver (no CUDA toolkit) still gets GPU acceleration. The app/installer
    then ship the plugins + those runtime DLLs from this one dir.

    Usage (from a "x64 Native Tools Command Prompt for VS" / Developer PowerShell):
        tools\windows\build-gpu-plugins.ps1 [-OutDir <dir>]
    OutDir defaults to Builds\VisualStudio2022\x64\Release\App (beside the .exe,
    where GpuBackendFactory dlopens the plugins and the installer collects them).

    NOTE: untested in CI — validate on a real Windows + toolkit setup.
#>
param([string]$OutDir = "")

$ErrorActionPreference = "Stop"
$Root = (Resolve-Path "$PSScriptRoot\..\..").Path
$Src  = Join-Path $Root "Source\DSP\gpu"
if ([string]::IsNullOrEmpty($OutDir)) { $OutDir = Join-Path $Root "Builds\VisualStudio2022\x64\Release\App" }
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$plugin    = Join-Path $Src "plugin\GpuVendorPlugin.cpp"
$cudaSrc   = @("Wfs","Ob","Ir","Fdn","Sdn") | ForEach-Object { Join-Path $Src "Cuda$($_)Backend.cpp" }
$hipSrc    = @("Wfs","Ob","Ir","Fdn","Sdn") | ForEach-Object { Join-Path $Src "Hip$($_)Backend.cpp" }
$built = $false

# /MD on BOTH plugins is load-bearing, not cosmetic: the main app (JUCE VS2022
# Release) uses the dynamic CRT, and GpuBackendFactory destroys plugin-allocated
# backends with an app-side `delete`. That cross-DLL free is safe only if the app
# and the plugin share one heap — i.e. both link the dynamic UCRT (/MD), not /MT.
# cl.exe /LD defaults to /MT, and clang/hipcc defaults to /MT too, so force /MD.

# ---- NVIDIA / CUDA (cl.exe + CUDA headers; kernels are runtime NVRTC strings) ----
# NOTE: the CUDA libs are ALSO #pragma comment(lib,...)'d in Cuda*Backend.cpp under
# _MSC_VER, so the explicit cudart/nvrtc/cuda.lib below are redundant — but
# /LIBPATH:"$cuda\lib\x64" is required either way so those pragma libs resolve.
$cuda = $env:CUDA_PATH
if ($cuda -and (Test-Path (Join-Path $cuda "include\cuda.h")) -and (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    Write-Host "Building wfs_cuda.dll ..."
    & cl.exe /nologo /LD /MD /std:c++17 /EHsc /O2 /DWFS_GPU_NATIVE=1 `
        /I"$Src" /I"$Root\Source\DSP" /I"$cuda\include" `
        @cudaSrc $plugin `
        /Fe:"$OutDir\wfs_cuda.dll" `
        /link /LIBPATH:"$cuda\lib\x64" cudart.lib nvrtc.lib cuda.lib
    Write-Host "  -> $OutDir\wfs_cuda.dll"

    # Stage the CUDA runtime DLLs wfs_cuda.dll links (CUDA Runtime + NVRTC) next
    # to it so the plugin loads on a driver-only machine (no CUDA toolkit). Skip
    # the ~85 MB nvrtc *.alt.dll forward-compat twin (not needed at runtime).
    # nvcuda.dll (the driver API) ships with the NVIDIA driver and is NOT copied.
    $cudaBin = Join-Path $cuda "bin"
    Get-ChildItem $cudaBin -Filter "cudart64_*.dll"          | Copy-Item -Destination $OutDir -Force
    Get-ChildItem $cudaBin -Filter "nvrtc64_*.dll"           | Where-Object { $_.Name -notlike "*.alt.dll" } | Copy-Item -Destination $OutDir -Force
    Get-ChildItem $cudaBin -Filter "nvrtc-builtins64_*.dll"  | Copy-Item -Destination $OutDir -Force
    Write-Host "  staged CUDA runtime DLLs (cudart/nvrtc/nvrtc-builtins) -> $OutDir"
    $built = $true
} else {
    Write-Host "skip wfs_cuda.dll (no CUDA_PATH/cl.exe)"
}

# ---- AMD / HIP (host-mode clang++ from the HIP SDK for Windows) ----
# The Hip*Backend.cpp files are HOST-ONLY C++: the kernels are hiprtc runtime-
# compiled strings (CudaWfsKernels.h et al.), so there is NO device code to
# compile here -- exactly like the CUDA branch above builds host code with
# cl.exe, not nvcc. Do NOT use hipcc: it forces `-x hip` (device) compilation,
# and on Windows that pulls HIP's device-math headers (__clang_hip_cmath.h)
# whose isgreater/isless/... overloads collide with MSVC <cmath>'s
# _CLANG_BUILTIN2(...) and fail to build. (On Linux hipcc works because
# libstdc++'s <cmath> doesn't clash.) So compile as host C++ with the SDK's
# clang++ (-x c++), define __HIP_PLATFORM_AMD__ so <hip/hip_runtime.h> exposes
# the host API, and link amdhip64 (HIP runtime) + hiprtc (runtime kernel
# compiler). -fms-runtime-lib=dll is the clang spelling of /MD (shared dynamic
# CRT, see above). Resolve the SDK root from HIP_PATH (set by the HIP SDK shell)
# or from hipcc's own location; the app is expected to run from a Developer
# shell so clang++ finds the MSVC toolchain + Windows SDK via INCLUDE/LIB.
if (Get-Command hipcc -ErrorAction SilentlyContinue) {
    Write-Host "Building wfs_hip.dll ..."
    $hipRoot  = if ($env:HIP_PATH) { $env:HIP_PATH } else { Split-Path (Split-Path (Get-Command hipcc).Source) }
    $hipClang = Join-Path $hipRoot "bin\clang++.exe"
    & $hipClang -shared -std=c++17 -O2 -fms-runtime-lib=dll -x c++ `
        -D__HIP_PLATFORM_AMD__=1 -DWFS_GPU_NATIVE=1 -DWFS_GPU_HIP=1 `
        -isystem "$hipRoot\include" `
        -I"$Src" -I"$Root\Source\DSP" `
        @hipSrc $plugin `
        -fuse-ld=lld --ld-path="$hipRoot\bin\lld-link.exe" `
        -L"$hipRoot\lib" -lamdhip64 -lhiprtc `
        -o "$OutDir\wfs_hip.dll"
    Write-Host "  -> $OutDir\wfs_hip.dll"

    # NOTE: unlike the CUDA runtime above, the HIP runtime is deliberately NOT
    # staged next to the plugin. amdhip64_*.dll loads if copied but then fails
    # device init (hipErrorNoDevice: "no ROCm-capable device is detected"),
    # because it locates its ROCm support files relative to its own directory --
    # a relocated copy can't find them. So the runtime must load IN PLACE from
    # the ROCm install; the app adds that install's bin to its DLL search path at
    # startup (PlatformDynLib.h ensureVendorRuntimeSearchPath), which resolves
    # both the amdhip64 enumeration and the plugin's amdhip64/hiprtc imports.
    # hipRTC ships only with the HIP SDK (not the consumer Adrenalin driver), so
    # an installed HIP SDK / ROCm is required for AMD GPU acceleration.
    $built = $true
} else {
    Write-Host "skip wfs_hip.dll (hipcc not found)"
}

if (-not $built) { Write-Error "No GPU toolchain found — nothing built." }

# Sanity-check the export table: a DLL that loads but exports no wfs_plugin_*
# symbols is the silent-CPU-fallback trap (Windows exports nothing by default).
if (Get-Command dumpbin.exe -ErrorAction SilentlyContinue) {
    foreach ($dll in @("wfs_cuda.dll","wfs_hip.dll")) {
        $path = Join-Path $OutDir $dll
        if (Test-Path $path) {
            $exports = & dumpbin.exe /EXPORTS $path 2>$null
            if ($exports -match "wfs_plugin_create_wfs") {
                Write-Host "  exports OK: $dll"
            } else {
                Write-Warning "$dll exports no wfs_plugin_* symbols — the app will fall back to CPU. Check __declspec(dllexport) in GpuVendorPlugin.cpp."
            }
        }
    }
}
Write-Host "Done."
