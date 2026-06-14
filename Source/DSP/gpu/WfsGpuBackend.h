#pragma once

/*
    WfsGpuBackend — compile-time selection of the native GPU WFS backend.

    The Metal and CUDA backends never coexist in a single binary (one targets
    Apple, the other Windows/NVIDIA) and they expose the identical method
    surface (prepare / setMatrixPointers / processBlock / reset / release /
    isReady / getLastError / getLastLaunchMs / getDeviceName). A plain type
    alias is therefore enough to let GpuAsyncPipeline and NativeGpuWfsAlgorithm
    stay backend-agnostic with zero #if scatter.
*/

#if defined(__APPLE__)
  #include "MetalWfsBackend.h"
  using WfsGpuBackend = MetalWfsBackend;
#else
  #include "CudaWfsBackend.h"
  using WfsGpuBackend = CudaWfsBackend;
#endif
