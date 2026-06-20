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

# ---- AMD / HIP (hipcc from the HIP SDK for Windows) ----
# Run from the "HIP SDK" environment so `hipcc` (hipcc.bat/.pl) resolves on PATH;
# -fms-runtime-lib=dll is the clang spelling of /MD (shared dynamic CRT, see above).
# hipcc's --hip-link auto-links the HIP runtime (amdhip64) but NOT hiprtc, the
# runtime kernel-compiler the backends call (hiprtcCreateProgram/Compile/GetCode/
# ...). Link hiprtc.lib explicitly, mirroring the Linux .jucer's -lhiprtc. hipcc
# re-splits its command line on spaces, so a "-L<dir with spaces>" (e.g. the SDK's
# default "C:\Program Files\AMD\ROCm\..\lib") would be mangled; put the SDK lib dir
# on the linker's %LIB% (lld-link honours it) instead. Resolve the SDK root from
# HIP_PATH (set by the HIP SDK shell) or from hipcc's own location.
if (Get-Command hipcc -ErrorAction SilentlyContinue) {
    Write-Host "Building wfs_hip.dll ..."
    $hipRoot = if ($env:HIP_PATH) { $env:HIP_PATH } else { Split-Path (Split-Path (Get-Command hipcc).Source) }
    $env:LIB = (Join-Path $hipRoot "lib") + ";" + $env:LIB
    & hipcc -shared -std=c++17 -O2 -fms-runtime-lib=dll -DWFS_GPU_NATIVE=1 -DWFS_GPU_HIP=1 `
        -I"$Src" -I"$Root\Source\DSP" `
        @hipSrc $plugin `
        -lhiprtc `
        -o "$OutDir\wfs_hip.dll"
    Write-Host "  -> $OutDir\wfs_hip.dll"
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
