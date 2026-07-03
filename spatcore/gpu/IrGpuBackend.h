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
  namespace spatcore::gpu { using IrGpuBackend = MetalIrBackend; }
#elif defined(WFS_GPU_HIP)
  #include "HipIrBackend.h"
  namespace spatcore::gpu { using IrGpuBackend = HipIrBackend; }
#else
  #include "CudaIrBackend.h"
  namespace spatcore::gpu { using IrGpuBackend = CudaIrBackend; }
#endif

// Extraction-compat alias — app code migrates to qualified names later.
using spatcore::gpu::IrGpuBackend;

#if WFS_GPU_NATIVE
#include "GpuBackendInterface.h"
#include <memory>
#if defined(WFS_GPU_PLUGINS) && ! defined(__APPLE__)
 #include "GpuBackendFactory.h"  // hoisted: an #include cannot sit inside the namespace
#endif

namespace spatcore::gpu {

// Thin adapter wrapping the compile-time-selected concrete IR backend behind
// IIrBackend (runtime-polymorphic). The concrete backend is untouched.
template <class B>
class IrBackendAdapter final : public IIrBackend
{
public:
    explicit IrBackendAdapter (int deviceIndex = 0) : impl (deviceIndex) {}

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
 inline std::unique_ptr<IIrBackend> makeIrBackend (const std::string& deviceId)
 {
     return GpuBackendFactory::instance().makeIr (deviceId);
 }
#else
 // In-process: bind a specific device of the compiled-in vendor (see WfsGpuBackend.h).
 inline std::unique_ptr<IIrBackend> makeIrBackend (int deviceIndex)
 {
     return std::make_unique<IrBackendAdapter<IrGpuBackend>> (deviceIndex);
 }
 inline std::unique_ptr<IIrBackend> makeIrBackend (const std::string& deviceId = {})
 {
     return makeIrBackend (deviceIndexFromId (deviceId));
 }
#endif

} // namespace spatcore::gpu

// Extraction-compat aliases — app code migrates to qualified names later.
using spatcore::gpu::IrBackendAdapter;
using spatcore::gpu::makeIrBackend;

#endif // WFS_GPU_NATIVE
