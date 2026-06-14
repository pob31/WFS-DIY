#pragma once

/*
    ObGpuBackend — compile-time selection of the native GPU OutputBuffer
    (scatter / write-time) backend. The scatter twin of WfsGpuBackend.

    The Metal, CUDA, and HIP backends never coexist in a single binary (Apple -> Metal, AMD/Linux -> HIP, else -> CUDA) and expose the identical method surface
    (prepare / setMatrixPointers / setFRFilterParams / setFRDiffusion /
    processBlock / reset / release / isReady / getLastError / getLastLaunchMs /
    getDeviceName). A plain type alias lets GpuAsyncPipeline and
    NativeGpuOutputBufferAlgorithm stay backend-agnostic with zero #if scatter.
*/

#if defined(__APPLE__)
  #include "MetalObBackend.h"
  using ObGpuBackend = MetalObBackend;
#elif defined(WFS_GPU_HIP)
  #include "HipObBackend.h"
  using ObGpuBackend = HipObBackend;
#else
  #include "CudaObBackend.h"
  using ObGpuBackend = CudaObBackend;
#endif
