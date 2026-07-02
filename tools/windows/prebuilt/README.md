# Prebuilt GPU plugins (Windows)

This directory holds prebuilt GPU vendor plugins that the Windows CI cannot build
itself, so they can still be bundled into the released installer.

## `wfs_hip.dll` — AMD / HIP plugin

The GitHub Windows runner has the CUDA toolkit (so CI builds `wfs_cuda.dll` fresh),
but **no AMD HIP SDK**, and installing it in CI is multi-gigabyte. So `wfs_hip.dll`
is built locally on a machine with the AMD HIP SDK and committed here; the release
workflow copies it next to the exe before packaging.

It is force-tracked via a `!` rule in the repo `.gitignore` (the global `*.dll`
rule would otherwise ignore it).

### Rebuild + recommit when the HIP backend changes

This DLL is a binary snapshot of `Source/DSP/gpu/Hip*Backend.cpp` +
`plugin/GpuVendorPlugin.cpp`. Whenever that source changes, rebuild and recommit:

1. On a Windows box with the AMD HIP SDK **and** the MSVC toolchain, from a
   Developer PowerShell (so `cl.exe` and `hipcc` are on `PATH`):
   `tools\windows\build-gpu-plugins.ps1`
   This builds both plugins into `Builds\VisualStudio2022\x64\Release\App\`.
2. Copy the fresh `wfs_hip.dll` over the one here and commit it:
   `Copy-Item Builds\VisualStudio2022\x64\Release\App\wfs_hip.dll tools\windows\prebuilt\ -Force`

### Runtime dependencies (not bundled)

`wfs_hip.dll` loads `amdhip64_6.dll` / `hiprtc*.dll` from the user's AMD HIP
runtime at load time. Those are **not** shipped in the installer (they come from
the AMD driver / HIP install). On a machine without them, `GpuDeviceManager`
simply enumerates no AMD device and the plugin is never loaded — a clean no-op.
