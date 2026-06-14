#pragma once

/*
    SdnGpuBackend — compile-time selection of the native GPU SDN reverb backend.

    Mirrors FdnGpuBackend.h / IrGpuBackend.h: the Metal and CUDA SDN backends
    never coexist in one binary and expose the identical method surface (prepare /
    setGeometry / setParameters / requestReset / processBlock / release / isReady /
    getLastError / getLastLaunchMs / getDeviceName), so a type alias keeps
    ReverbSDNAlgorithmGPU and GpuAsyncPipelineT backend-agnostic.
*/

#if defined(__APPLE__)
  #include "MetalSdnBackend.h"
  using SdnGpuBackend = MetalSdnBackend;
#elif defined(WFS_GPU_HIP)
  #include "HipSdnBackend.h"
  using SdnGpuBackend = HipSdnBackend;
#else
  #include "CudaSdnBackend.h"
  using SdnGpuBackend = CudaSdnBackend;
#endif
