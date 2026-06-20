/*
    GpuVendorPlugin — the per-vendor GPU plugin entry points (Phase 3 Stage 3).

    Compiled ONCE PER VENDOR into libwfs_<vendor>.so (libwfs_hip.so links the HIP
    runtime; libwfs_cuda.so links the CUDA runtime). The CPU-safe main app links
    NEITHER runtime; it dlopens the plugin matching the user-selected device and
    obtains backends through these C entry points (see GpuBackendFactory.h).

    Which concrete backend compiles in is the existing compile-time selection
    (WFS_GPU_HIP -> Hip*, else -> Cuda*), so each plugin carries exactly one
    vendor's backends. The returned objects implement the shared IXBackend
    interfaces from GpuBackendInterface.h (ABI-stable across the .so boundary:
    app + plugins build from one tree with one toolchain).
*/

#include "../WfsGpuBackend.h"
#include "../ObGpuBackend.h"
#include "../IrGpuBackend.h"
#include "../FdnGpuBackend.h"
#include "../SdnGpuBackend.h"

#if ! WFS_GPU_NATIVE
 #error "GpuVendorPlugin.cpp must be built with WFS_GPU_NATIVE=1"
#endif

// A Windows DLL exports NOTHING by default (unlike a Linux ELF .so, whose default
// visibility exports every extern symbol). Without an explicit export the host's
// GetProcAddress("wfs_plugin_create_*") returns null, wrap() hits its null guard,
// and every GPU device silently falls back to CPU. So mark each entry point.
// POSIX needs no marker here (the plugin build sets no -fvisibility=hidden).
#if defined(_WIN32)
 #define WFS_PLUGIN_API __declspec(dllexport)
#else
 #define WFS_PLUGIN_API
#endif

extern "C" {

// Plugin self-identification (matches GpuDevice::Vendor labels, lower-case).
WFS_PLUGIN_API const char* wfs_plugin_vendor()
{
#if defined(WFS_GPU_HIP)
    return "hip";
#elif defined(__APPLE__)
    return "metal";
#else
    return "cuda";
#endif
}

// deviceIndex selects which GPU of this vendor to bind (the factory resolves it
// from the device id, e.g. "cuda:1" -> 1). Forwarded to the concrete backend's
// constructor; single-GPU machines pass 0 and are unaffected.
WFS_PLUGIN_API IWfsBackend* wfs_plugin_create_wfs (int deviceIndex) { return makeWfsBackend (deviceIndex).release(); }
WFS_PLUGIN_API IObBackend*  wfs_plugin_create_ob  (int deviceIndex) { return makeObBackend  (deviceIndex).release(); }
WFS_PLUGIN_API IIrBackend*  wfs_plugin_create_ir  (int deviceIndex) { return makeIrBackend  (deviceIndex).release(); }
WFS_PLUGIN_API IFdnBackend* wfs_plugin_create_fdn (int deviceIndex) { return makeFdnBackend (deviceIndex).release(); }
WFS_PLUGIN_API ISdnBackend* wfs_plugin_create_sdn (int deviceIndex) { return makeSdnBackend (deviceIndex).release(); }

// Destroy any backend created above (virtual dtor via IGpuBackend). The factory
// wraps the returned pointer in a default-deleter unique_ptr, so the host module
// runs `delete`. That is safe ONLY while the host and the plugin share one heap:
//   Linux  — both built with g++ → one libstdc++ → one heap (validated).
//   Windows— both must use the DYNAMIC CRT (/MD; the JUCE VS2022 default and what
//            tools/windows/build-gpu-plugins.ps1 forces) → one shared ucrtbase
//            heap. If a future split breaks that invariant, route the factory's
//            deleter through this exported symbol instead of app-side delete.
WFS_PLUGIN_API void wfs_plugin_destroy (IGpuBackend* p) { delete p; }

} // extern "C"
