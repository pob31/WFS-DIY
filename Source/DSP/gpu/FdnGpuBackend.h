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
  using FdnGpuBackend = MetalFdnBackend;
#elif defined(WFS_GPU_HIP)
  #include "HipFdnBackend.h"
  using FdnGpuBackend = HipFdnBackend;
#else
  #include "CudaFdnBackend.h"
  using FdnGpuBackend = CudaFdnBackend;
#endif

#if WFS_GPU_NATIVE
#include "GpuBackendInterface.h"
#include <memory>

// Thin adapter wrapping the compile-time-selected concrete FDN backend behind
// IFdnBackend (runtime-polymorphic). The concrete backend is untouched.
template <class B>
class FdnBackendAdapter final : public IFdnBackend
{
public:
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
 #include "GpuBackendFactory.h"
 inline std::unique_ptr<IFdnBackend> makeFdnBackend (const std::string& deviceId)
 {
     return GpuBackendFactory::instance().makeFdn (deviceId);
 }
#else
 inline std::unique_ptr<IFdnBackend> makeFdnBackend (const std::string& deviceId = {})
 {
     (void) deviceId;
     return std::make_unique<FdnBackendAdapter<FdnGpuBackend>>();
 }
#endif
#endif // WFS_GPU_NATIVE
