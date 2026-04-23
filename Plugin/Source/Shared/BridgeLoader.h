#pragma once

#include "BridgeApi.h"
#include <juce_core/juce_core.h>

namespace wfs::plugin
{
    class BridgeLoader
    {
    public:
        static BridgeLoader& getInstance();

        bool ensureLoaded();
        bool isLoaded() const { return dll != nullptr; }

        decltype(&wfs_bridge_abi_version)                abiVersion          = nullptr;
        decltype(&wfs_bridge_master_register)            masterRegister      = nullptr;
        decltype(&wfs_bridge_master_unregister)          masterUnregister    = nullptr;
        decltype(&wfs_bridge_master_dispatch_inbound)    masterDispatch      = nullptr;
        decltype(&wfs_bridge_master_snapshot_input_ids)  masterSnapshotIds   = nullptr;
        decltype(&wfs_bridge_track_register)             trackRegister       = nullptr;
        decltype(&wfs_bridge_track_unregister)           trackUnregister     = nullptr;
        decltype(&wfs_bridge_track_send_outbound)        trackSendOutbound   = nullptr;
        decltype(&wfs_bridge_track_count)                trackCount          = nullptr;
        decltype(&wfs_bridge_has_master)                 hasMaster           = nullptr;

        decltype(&wfs_bridge_master_set_outbound_3f)       masterSetOutbound3f = nullptr;
        decltype(&wfs_bridge_master_dispatch_inbound_3f)   masterDispatch3f    = nullptr;
        decltype(&wfs_bridge_track_set_inbound_3f)         trackSetInbound3f   = nullptr;
        decltype(&wfs_bridge_track_send_outbound_3f)       trackSendOutbound3f = nullptr;

    private:
        BridgeLoader() = default;
        ~BridgeLoader();

        bool resolveSymbols();
        juce::File locateBridgeLibrary() const;

        std::unique_ptr<juce::DynamicLibrary> dll;
    };
}
