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

**Current binary**: built 2026-07-26 from spatcore `d1e6f66` (v0.1.1-18) with
ROCm 7.1 clang++ against the MSVC dynamic UCRT. Adds the **node-parallel SDN port**
(`c12f3b6`) and the **`WFS_SDN_TRACE` mapping log** (`3da516a`) over the previous
snapshot; still carries the SDN N-invariant output gain (`c7dad5c`) and the GPU
host-path work (M1 blocking event waits, M2 upload diet, M3 GpuHostWorkPool).
Update this line whenever you recommit the DLL.

Validated on gfx1103 (Radeon 780M) / ROCm 7.1 the same day: `test-gpu-plugin.exe`
7/7 PASS exit 0, SDN peak `0.0633` matching the Linux gfx1103 reference, and both
mapping lines present under `WFS_SDN_TRACE=1` (`lockstep minDelay=1` +
`node-parallel minDelay=349`). Node-parallel launch 1.199 ms vs lockstep 2.666 ms,
identical peak — see `docs/Linux_Branch_Completion_Handoff.md` step 4b.

> **The stale-DLL trap.** The plugin ABI is the seven `extern "C"` entry points and
> they rarely change, so a DLL older than the spatcore it is paired with **loads
> happily and silently runs the older kernel** — no error, no warning. Zero `[sdn]`
> lines under `WFS_SDN_TRACE=1` means this DLL is stale, *not* that the port failed.
> Rebuild before trusting any HIP-vs-CUDA/Metal comparison.

### Rebuild + recommit when the HIP backend changes

This DLL is a binary snapshot of `spatcore/gpu/Hip*Backend.cpp` +
`plugin/GpuVendorPlugin.cpp`. Whenever that source changes, rebuild and recommit:

1. On a Windows box with the AMD HIP SDK **and** the MSVC toolchain, from a
   Developer PowerShell (so `cl.exe` and `hipcc` are on `PATH`):
   `tools\windows\build-gpu-plugins.ps1`
   This builds both plugins into `Builds\VisualStudio2022\x64\Release\App\`.
2. Copy the fresh `wfs_hip.dll` over the one here and commit it:
   `Copy-Item Builds\VisualStudio2022\x64\Release\App\wfs_hip.dll tools\windows\prebuilt\ -Force`

### Runtime dependencies (not bundled)

`wfs_hip.dll` loads `amdhip64_7.dll` and `hiprtc0701.dll` from the user's AMD HIP
runtime at load time (`hiprtc0701.dll` in turn pulls `hiprtc-builtins0701.dll`).
Those are **not** shipped in the installer (they come from the AMD driver / HIP
install). On a machine without them, `GpuDeviceManager` simply enumerates no AMD
device and the plugin is never loaded — a clean no-op.

**This binary requires a ROCm 7.x HIP SDK.** Those import names are version-pinned
at link time, so the committed DLL will not resolve against a ROCm 6.x install even
though the *source* supports 6.x and 7.x alike (see `docs/AMD_Windows_HIP_Validation.md`).
Whoever rebuilds decides the floor: build on ROCm 6.x and the imports come out
`amdhip64_6.dll` / `hiprtc*` instead. Restate the version here when that changes.

Note this is a different mechanism from the app's own device enumeration, which
`dlopen`s `amdhip64.dll` with versioned fallbacks and is therefore version-flexible.
