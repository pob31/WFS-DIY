#pragma once

/*
    FdnGpuBackend — compile-time selection of the native GPU FDN reverb backend.

    Mirrors WfsGpuBackend.h / IrGpuBackend.h: the Metal and CUDA FDN backends
    never coexist in one binary and expose the identical method surface
    (prepare / setParameters / requestReset / processBlock / release / isReady /
    getLastError / getLastLaunchMs / getDeviceName), so a type alias keeps
    ReverbFDNAlgorithmGPU and GpuAsyncPipelineT backend-agnostic.
*/

#if defined(__APPLE__)
  #include "MetalFdnBackend.h"
  using FdnGpuBackend = MetalFdnBackend;
#else
  #include "CudaFdnBackend.h"
  using FdnGpuBackend = CudaFdnBackend;
#endif
