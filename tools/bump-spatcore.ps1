<#
    bump-spatcore.ps1 — update the spatcore submodule pin to a new tag/SHA and
    stage the bump commit, reminding you of the gate ritual before it merges.

    Usage (from the repo root, any PowerShell):
        tools\bump-spatcore.ps1 v0.2.0          # a tag
        tools\bump-spatcore.ps1 3f9c1ab         # or a SHA
        tools\bump-spatcore.ps1                 # no arg = latest origin/main

    What it does: fetch inside the submodule, checkout the requested ref,
    stage the gitlink, and print the gate checklist. It does NOT commit or
    push — a submodule bump is a normal PR through the same gates as any
    other change (offline render baselines, control replays, kernel hashes,
    dependency lint, spatcore standalone tests, app build).
#>
param([string]$Ref = "")

$ErrorActionPreference = "Stop"
$Root = (Resolve-Path "$PSScriptRoot\..").Path
$Sub  = Join-Path $Root "spatcore"

if (-not (Test-Path (Join-Path $Sub ".git"))) {
    Write-Error "spatcore submodule not initialised - run: git submodule update --init spatcore"
}

git -C $Sub fetch --tags origin
if ([string]::IsNullOrEmpty($Ref)) { $Ref = "origin/main" }
git -C $Sub checkout --quiet $Ref
$sha  = (git -C $Sub rev-parse --short HEAD).Trim()
$desc = (git -C $Sub describe --tags --always).Trim()
git -C $Root add spatcore

Write-Host ""
Write-Host "spatcore pinned at $desc ($sha) and staged."
Write-Host "Before committing the bump, run the gate ritual:"
Write-Host "  1. App build (MSBuild Release, -p:PlatformToolset=v145)"
Write-Host "  2. python tools/validation/kernel_hashes.py  +  spatcore_dep_lint.py"
Write-Host "  3. Rebuild GPU plugins (tools/windows/build-gpu-plugins.ps1) + smoke test"
Write-Host "  4. offline-render --path cpu/gpu --check (bit-identical unless the bump"
Write-Host "     is declared value-affecting - then re-baseline DELIBERATELY)"
Write-Host "  5. control-replay: session_roundtrip / osc_replay / mcp_replay"
Write-Host "  6. spatcore standalone tests (reconfigure if CMake changed)"
Write-Host ""
Write-Host "Then:  git commit -m `"bump spatcore to $desc`""
