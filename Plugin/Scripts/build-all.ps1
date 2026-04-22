# WFS-DIY Plugin Suite - Windows build orchestration
#
# Usage (from repo root or anywhere):
#   pwsh Plugin/Scripts/build-all.ps1
#
# Runs: cmake configure -> cmake build Release -> Inno Setup compile.
# Produces Plugin/Installer/Output/WFS-DIY-Plugins-Setup-0.0.1.exe

param(
    [string]$Configuration = "Release",
    [switch]$SkipInstaller
)

$ErrorActionPreference = "Stop"
$ScriptDir  = Split-Path -Parent $MyInvocation.MyCommand.Path
$PluginDir  = Split-Path -Parent $ScriptDir
$RepoRoot   = Split-Path -Parent $PluginDir
$BuildDir   = Join-Path $PluginDir "Builds/CMake-Windows"

Write-Host "==> Configuring CMake (Ninja, $Configuration)"
if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

$Generators = @("Ninja", "Visual Studio 17 2022")
$Configured = $false
foreach ($gen in $Generators) {
    try {
        if ($gen -eq "Ninja") {
            cmake -S $PluginDir -B $BuildDir -G "Ninja" -DCMAKE_BUILD_TYPE=$Configuration
        } else {
            cmake -S $PluginDir -B $BuildDir -G "Visual Studio 17 2022" -A x64
        }
        if ($LASTEXITCODE -eq 0) { $Configured = $true; break }
    } catch {
        Write-Host "  $gen failed, trying next."
    }
}
if (-not $Configured) {
    throw "CMake configuration failed for all generators."
}

Write-Host "==> Building WFSPluginsAll target"
cmake --build $BuildDir --config $Configuration --target WFSPluginsAll --parallel
if ($LASTEXITCODE -ne 0) { throw "CMake build failed." }

if ($SkipInstaller) {
    Write-Host "Build complete. Installer skipped."
    exit 0
}

$InnoSetup = "C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
if (-not (Test-Path $InnoSetup)) {
    $InnoSetup = "C:\Program Files\Inno Setup 6\ISCC.exe"
}
if (-not (Test-Path $InnoSetup)) {
    Write-Warning "ISCC.exe not found. Install Inno Setup 6 to build the installer."
    exit 0
}

Write-Host "==> Running Inno Setup compiler"
$IssFile = Join-Path $PluginDir "Installer/WFS-DIY-Plugins.iss"
& $InnoSetup $IssFile
if ($LASTEXITCODE -ne 0) { throw "Inno Setup compilation failed." }

Write-Host "==> Done. Installer in Plugin/Installer/Output/"
