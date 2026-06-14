#pragma once

/*
    SdnGpuBackend — compile-time selection of the native GPU SDN reverb backend.

    Mirrors FdnGpuBackend.h / IrGpuBackend.h: the Metal, CUDA, and HIP SDN backends
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

#if WFS_GPU_NATIVE
#include "GpuBackendInterface.h"
#include <memory>

// Thin adapter wrapping the compile-time-selected concrete SDN backend behind
// ISdnBackend (runtime-polymorphic). The concrete backend is untouched.
template <class B>
class SdnBackendAdapter final : public ISdnBackend
{
public:
    bool prepare (int nn, int bs, double sr) override { return impl.prepare (nn, bs, sr); }
    void setGeometry (const float* xyz, int count) noexcept override { impl.setGeometry (xyz, count); }
    void setParameters (float a, float b, float c, float d, float e, float f, float g) noexcept override
        { impl.setParameters (a, b, c, d, e, f, g); }
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

inline std::unique_ptr<ISdnBackend> makeSdnBackend()
{
    return std::make_unique<SdnBackendAdapter<SdnGpuBackend>>();
}
#endif // WFS_GPU_NATIVE
