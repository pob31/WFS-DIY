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

extern "C" {

// Plugin self-identification (matches GpuDevice::Vendor labels, lower-case).
const char* wfs_plugin_vendor()
{
#if defined(WFS_GPU_HIP)
    return "hip";
#elif defined(__APPLE__)
    return "metal";
#else
    return "cuda";
#endif
}

// deviceIndex selects which GPU of this vendor (reserved for multi-GPU binding;
// the backends currently bind device 0 — single-GPU machines are unaffected).
IWfsBackend* wfs_plugin_create_wfs (int /*deviceIndex*/) { return makeWfsBackend().release(); }
IObBackend*  wfs_plugin_create_ob  (int /*deviceIndex*/) { return makeObBackend().release();  }
IIrBackend*  wfs_plugin_create_ir  (int /*deviceIndex*/) { return makeIrBackend().release();  }
IFdnBackend* wfs_plugin_create_fdn (int /*deviceIndex*/) { return makeFdnBackend().release(); }
ISdnBackend* wfs_plugin_create_sdn (int /*deviceIndex*/) { return makeSdnBackend().release(); }

// Destroy any backend created above (virtual dtor via IGpuBackend). The app and
// plugin share one toolchain/stdlib, so app-side delete is also safe; this entry
// point keeps allocation + free symmetric inside the plugin.
void wfs_plugin_destroy (IGpuBackend* p) { delete p; }

} // extern "C"
