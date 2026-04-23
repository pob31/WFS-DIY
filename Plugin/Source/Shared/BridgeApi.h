#pragma once

#if defined (_WIN32)
  #define WFS_BRIDGE_EXPORT __declspec(dllexport)
  #define WFS_BRIDGE_IMPORT __declspec(dllimport)
#else
  #define WFS_BRIDGE_EXPORT __attribute__((visibility("default")))
  #define WFS_BRIDGE_IMPORT __attribute__((visibility("default")))
#endif

#if defined (WFS_BRIDGE_BUILDING)
  #define WFS_BRIDGE_API WFS_BRIDGE_EXPORT
#else
  #define WFS_BRIDGE_API WFS_BRIDGE_IMPORT
#endif

extern "C" {

    struct WfsBridgeTrackHandle;
    struct WfsBridgeMasterHandle;

    typedef void (*WfsBridgeInboundFn)(void* trackUser,
                                       const char* oscPath,
                                       int channelId,
                                       double value);

    typedef void (*WfsBridgeOutboundFn)(void* masterUser,
                                        const char* oscPath,
                                        int channelId,
                                        double value);

    typedef void (*WfsBridgeTrackLifecycleFn)(void* masterUser,
                                              int inputId,
                                              const char* variantTag,
                                              int isRegister);

    // Three-float outbound/inbound variants for ADM-OSC where the message
    // carries three payload floats (e.g. /adm/obj/N/xyz or /aed) and the
    // channel ID is embedded in the address path rather than a leading int arg.
    typedef void (*WfsBridgeOutbound3fFn)(void* masterUser,
                                          const char* oscPath,
                                          double v1, double v2, double v3);

    typedef void (*WfsBridgeInbound3fFn)(void* trackUser,
                                         const char* oscPath,
                                         int channelId,
                                         double v1, double v2, double v3);

    WFS_BRIDGE_API int                     wfs_bridge_abi_version();

    WFS_BRIDGE_API WfsBridgeMasterHandle*  wfs_bridge_master_register (void* user,
                                                                      WfsBridgeOutboundFn onOutbound,
                                                                      WfsBridgeTrackLifecycleFn onLifecycle);
    WFS_BRIDGE_API void                    wfs_bridge_master_unregister (WfsBridgeMasterHandle* handle);
    WFS_BRIDGE_API void                    wfs_bridge_master_dispatch_inbound (WfsBridgeMasterHandle* handle,
                                                                               int inputId,
                                                                               const char* oscPath,
                                                                               double value);
    WFS_BRIDGE_API int                     wfs_bridge_master_snapshot_input_ids (WfsBridgeMasterHandle* handle,
                                                                                 int* outIds,
                                                                                 int maxIds);

    WFS_BRIDGE_API WfsBridgeTrackHandle*   wfs_bridge_track_register (int inputId,
                                                                     const char* variantTag,
                                                                     void* user,
                                                                     WfsBridgeInboundFn onInbound);
    WFS_BRIDGE_API void                    wfs_bridge_track_unregister (WfsBridgeTrackHandle* handle);
    WFS_BRIDGE_API void                    wfs_bridge_track_send_outbound (WfsBridgeTrackHandle* handle,
                                                                          const char* oscPath,
                                                                          int channelId,
                                                                          double value);

    WFS_BRIDGE_API int                     wfs_bridge_track_count();
    WFS_BRIDGE_API int                     wfs_bridge_has_master();

    // ── Three-float variants (ADM-OSC) ──
    WFS_BRIDGE_API void                    wfs_bridge_master_set_outbound_3f (WfsBridgeMasterHandle* handle,
                                                                              WfsBridgeOutbound3fFn onOutbound3f);
    WFS_BRIDGE_API void                    wfs_bridge_master_dispatch_inbound_3f (WfsBridgeMasterHandle* handle,
                                                                                   int inputId,
                                                                                   const char* oscPath,
                                                                                   double v1, double v2, double v3);
    WFS_BRIDGE_API void                    wfs_bridge_track_set_inbound_3f (WfsBridgeTrackHandle* handle,
                                                                             WfsBridgeInbound3fFn onInbound3f);
    WFS_BRIDGE_API void                    wfs_bridge_track_send_outbound_3f (WfsBridgeTrackHandle* handle,
                                                                               const char* oscPath,
                                                                               double v1, double v2, double v3);
}

namespace wfs::plugin
{
    static constexpr int kBridgeAbiVersion = 1;
}
