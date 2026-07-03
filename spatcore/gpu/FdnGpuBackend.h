#pragma once

/*
    FdnGpuBackend — compile-time selection of the native GPU FDN reverb backend.

    Mirrors WfsGpuBackend.h / IrGpuBackend.h: the Metal, CUDA, and HIP FDN backends
    never coexist in one binary and expose the identical method surface
    (prepare / setParameters / requestReset / processBlock / release / isReady /
    getLastError / getLastLaunchMs / getDeviceName), so a type alias keeps
    ReverbFDNAlgorithmGPU and GpuAsyncPipelineT backend-agnostic.
*/

#if defined(__APPLE__)
  #include "MetalFdnBackend.h"
  namespace spatcore::gpu { using FdnGpuBackend = MetalFdnBackend; }
#elif defined(WFS_GPU_HIP)
  #include "HipFdnBackend.h"
  namespace spatcore::gpu { using FdnGpuBackend = HipFdnBackend; }
#else
  #include "CudaFdnBackend.h"
  namespace spatcore::gpu { using FdnGpuBackend = CudaFdnBackend; }
#endif

// Extraction-compat alias — app code migrates to qualified names later.
using spatcore::gpu::FdnGpuBackend;

#if WFS_GPU_NATIVE
#include "GpuBackendInterface.h"
#include <memory>
#if defined(WFS_GPU_PLUGINS) && ! defined(__APPLE__)
 #include "GpuBackendFactory.h"  // hoisted: an #include cannot sit inside the namespace
#endif

namespace spatcore::gpu {

// Thin adapter wrapping the compile-time-selected concrete FDN backend behind
// IFdnBackend (runtime-polymorphic). The concrete backend is untouched.
template <class B>
class FdnBackendAdapter final : public IFdnBackend
{
public:
    explicit FdnBackendAdapter (int deviceIndex = 0) : impl (deviceIndex) {}

    bool prepare (int nn, int bs, double sr, float sz) override { return impl.prepare (nn, bs, sr, sz); }
    void setParameters (float a, float b, float c, float d, float e, float f) noexcept override
        { impl.setParameters (a, b, c, d, e, f); }
    void requestReset() noexcept override { impl.requestReset(); }
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
 inline std::unique_ptr<IFdnBackend> makeFdnBackend (const std::string& deviceId)
 {
     return GpuBackendFactory::instance().makeFdn (deviceId);
 }
#else
 // In-process: bind a specific device of the compiled-in vendor (see WfsGpuBackend.h).
 inline std::unique_ptr<IFdnBackend> makeFdnBackend (int deviceIndex)
 {
     return std::make_unique<FdnBackendAdapter<FdnGpuBackend>> (deviceIndex);
 }
 inline std::unique_ptr<IFdnBackend> makeFdnBackend (const std::string& deviceId = {})
 {
     return makeFdnBackend (deviceIndexFromId (deviceId));
 }
#endif

} // namespace spatcore::gpu

// Extraction-compat aliases — app code migrates to qualified names later.
using spatcore::gpu::FdnBackendAdapter;
using spatcore::gpu::makeFdnBackend;

#endif // WFS_GPU_NATIVE
