# Build the wfs_headtrack webcam head-tracking plugin (Windows) and stage it
# next to the app, following the GPU-plugin convention (the script writes into
# the build output dir directly; nothing in the app build depends on this).
#
# OpenCV: uses the official prebuilt (opencv_world). Pass -OpenCVDir to point
# at an existing <opencv>/build; otherwise the script downloads and caches the
# 4.10.0 Windows package under tools/headtrack/.opencv (~180 MB download, once).
#
# Usage:
#   tools\headtrack\build-headtrack-plugin.ps1 [-Config Release] [-OpenCVDir <path>]
#                                              [-StageDir <appOutputDir>]

param(
    [string] $Config = "Release",
    [string] $OpenCVDir = "",
    [string] $StageDir = ""
)

$ErrorActionPreference = "Stop"
$Root = Resolve-Path "$PSScriptRoot\..\.."
$Src  = "$PSScriptRoot"
$Build = "$PSScriptRoot\build"

# ---------------------------------------------------------------- OpenCV
if ($OpenCVDir -eq "") {
    $cache = "$PSScriptRoot\.opencv"
    $OpenCVDir = "$cache\opencv\build"
    if (-not (Test-Path "$OpenCVDir\OpenCVConfig.cmake")) {
        $url = "https://github.com/opencv/opencv/releases/download/4.10.0/opencv-4.10.0-windows.exe"
        $exe = "$cache\opencv-4.10.0-windows.exe"
        New-Item -ItemType Directory -Force $cache | Out-Null
        Write-Host "Downloading OpenCV 4.10.0 prebuilt (once, cached in tools/headtrack/.opencv)..."
        Invoke-WebRequest -Uri $url -OutFile $exe -UseBasicParsing
        # The release .exe is a self-extracting archive; -o sets the target, -y overwrites.
        Start-Process -FilePath $exe -ArgumentList "-o`"$cache`" -y" -Wait -NoNewWindow
    }
}
if (-not (Test-Path "$OpenCVDir\OpenCVConfig.cmake")) {
    throw "OpenCV not found at $OpenCVDir"
}

# ---------------------------------------------------------------- configure + build
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if ($null -eq $cmake) {
    $vsCmake = "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    if (Test-Path $vsCmake) { $cmake = @{ Source = $vsCmake } } else { throw "cmake not found" }
}
$cmakeExe = $cmake.Source

# OpenCV_RUNTIME=vc16 pinned explicitly: the 4.10 prebuilt's config script does
# not recognise the VS2026 compiler version and would set OpenCV_FOUND=FALSE.
# Safe because only the plugin's C ABI crosses to the app.
& $cmakeExe -S $Src -B $Build -A x64 `
    -DOpenCV_DIR="$OpenCVDir" -DOpenCV_ARCH=x64 -DOpenCV_RUNTIME=vc16
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }

& $cmakeExe --build $Build --config $Config
if ($LASTEXITCODE -ne 0) { throw "Build failed" }

$out = "$Build\$Config"

# ---------------------------------------------------------------- export self-check
$dumpbin = Get-Command dumpbin -ErrorAction SilentlyContinue
if ($null -ne $dumpbin) {
    $exports = & dumpbin /EXPORTS "$out\wfs_headtrack.dll" | Out-String
    if ($exports -notmatch "wfs_headtrack_abi_version") {
        Write-Warning "wfs_headtrack.dll is missing its exports (WFS_HEADTRACK_API marker?) - the app would not load it"
    }
}

# ---------------------------------------------------------------- stage runtime deps
# opencv_world beside the plugin (like the CUDA runtime beside wfs_cuda.dll).
# The ffmpeg videoio DLL is for video FILES, not cameras - deliberately not staged.
Copy-Item "$OpenCVDir\x64\vc16\bin\opencv_world4100.dll" $out -Force
Copy-Item "$OpenCVDir\x64\vc16\bin\opencv_videoio_msmf4100_64.dll" $out -Force -ErrorAction SilentlyContinue

# ---------------------------------------------------------------- stage next to the app
if ($StageDir -eq "") {
    $StageDir = "$Root\Builds\VisualStudio2022\x64\$Config\App"
}
if (Test-Path $StageDir) {
    foreach ($f in @("wfs_headtrack.dll", "opencv_world4100.dll",
                     "opencv_videoio_msmf4100_64.dll", "face_detection_yunet_2023mar.onnx")) {
        if (Test-Path "$out\$f") { Copy-Item "$out\$f" $StageDir -Force }
    }
    Write-Host "Staged wfs_headtrack.dll + runtime into $StageDir"
} else {
    Write-Host "App output dir not found ($StageDir) - plugin left in $out"
}

Write-Host "OK: $out\wfs_headtrack.dll"
