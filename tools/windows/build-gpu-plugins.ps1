# Thin wrapper — the script moved to spatcore/tools/gpu/ (Phase 2 of the
# spatcore extraction). Kept so existing docs/CI invocations keep working.
& "$PSScriptRoot\..\..\spatcore\tools\gpu\build-gpu-plugins.ps1" @args
exit $LASTEXITCODE
