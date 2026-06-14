#pragma once

/*
    IrGpuBackend — compile-time selection of the native GPU IR-reverb backend.

    Mirrors WfsGpuBackend.h: the Metal, CUDA, and HIP IR backends never coexist in
    one binary and expose the identical method surface (prepare / stageIr /
    requestReset / processBlock / release / isReady / getLastError /
    getLastLaunchMs / getDeviceName / getSegmentsLoaded / getSegmentsTotal),
    so a type alias keeps ReverbIRAlgorithmGPU and GpuAsyncPipelineT
    backend-agnostic with zero #if scatter.
*/

#if defined(__APPLE__)
  #include "MetalIrBackend.h"
  using IrGpuBackend = MetalIrBackend;
#elif defined(WFS_GPU_HIP)
  #include "HipIrBackend.h"
  using IrGpuBackend = HipIrBackend;
#else
  #include "CudaIrBackend.h"
  using IrGpuBackend = CudaIrBackend;
#endif
