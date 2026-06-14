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

#if WFS_GPU_NATIVE
#include "GpuBackendInterface.h"
#include <memory>

// Thin adapter wrapping the compile-time-selected concrete IR backend behind
// IIrBackend (runtime-polymorphic). The concrete backend is untouched.
template <class B>
class IrBackendAdapter final : public IIrBackend
{
public:
    bool prepare (int nn, int bs, double sr, int maxIr) override { return impl.prepare (nn, bs, sr, maxIr); }
    void stageIr (const float* ir, int n) override { impl.stageIr (ir, n); }
    void requestReset() noexcept override { impl.requestReset(); }
    int getSegmentsLoaded() const noexcept override { return impl.getSegmentsLoaded(); }
    int getSegmentsTotal() const noexcept override { return impl.getSegmentsTotal(); }
    bool processBlock (const float* const* in, float* const* out) override { return impl.processBlock (in, out); }
    void release() noexcept override { impl.release(); }
    bool isReady() const noexcept override { return impl.isReady(); }
    const std::string& getLastError() const noexcept override { return impl.getLastError(); }
    double getLastLaunchMs() const noexcept override { return impl.getLastLaunchMs(); }
    const std::string& getDeviceName() const noexcept override { return impl.getDeviceName(); }
private:
    B impl;
};

#if defined(WFS_GPU_PLUGINS) && ! defined(__APPLE__)
 #include "GpuBackendFactory.h"
 inline std::unique_ptr<IIrBackend> makeIrBackend (const std::string& deviceId)
 {
     return GpuBackendFactory::instance().makeIr (deviceId);
 }
#else
 inline std::unique_ptr<IIrBackend> makeIrBackend (const std::string& deviceId = {})
 {
     (void) deviceId;
     return std::make_unique<IrBackendAdapter<IrGpuBackend>>();
 }
#endif
#endif // WFS_GPU_NATIVE
