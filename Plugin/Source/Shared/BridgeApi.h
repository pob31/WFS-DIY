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
}

namespace wfs::plugin
{
    static constexpr int kBridgeAbiVersion = 1;
}
