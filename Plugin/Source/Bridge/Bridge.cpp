#define WFS_BRIDGE_BUILDING 1
#include "../Shared/BridgeApi.h"

#include <atomic>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <set>

namespace
{
    struct TrackEntry
    {
        int inputId = 0;
        std::string variantTag;
        void* user = nullptr;
        WfsBridgeInboundFn onInbound = nullptr;
    };

    struct MasterEntry
    {
        void* user = nullptr;
        WfsBridgeOutboundFn       onOutbound  = nullptr;
        WfsBridgeTrackLifecycleFn onLifecycle = nullptr;
    };

    struct Registry
    {
        std::mutex lock;
        std::atomic<int> nextTrackId { 1 };
        std::unordered_map<int, TrackEntry>  tracks;
        std::unique_ptr<MasterEntry>         master;
    };

    Registry& getRegistry()
    {
        static Registry r;
        return r;
    }

    MasterEntry snapshotMaster (Registry& r)
    {
        std::lock_guard<std::mutex> sl (r.lock);
        if (r.master == nullptr)
            return {};
        return *r.master;
    }
}

struct WfsBridgeTrackHandle  { int id; };
struct WfsBridgeMasterHandle { int marker; };

extern "C" {

int wfs_bridge_abi_version()
{
    return wfs::plugin::kBridgeAbiVersion;
}

WfsBridgeMasterHandle* wfs_bridge_master_register (void* user,
                                                   WfsBridgeOutboundFn onOutbound,
                                                   WfsBridgeTrackLifecycleFn onLifecycle)
{
    auto& r = getRegistry();

    std::vector<TrackEntry> existingTracks;
    {
        std::lock_guard<std::mutex> sl (r.lock);
        if (r.master != nullptr)
            return nullptr;

        r.master = std::make_unique<MasterEntry>();
        r.master->user        = user;
        r.master->onOutbound  = onOutbound;
        r.master->onLifecycle = onLifecycle;

        for (auto& [id, entry] : r.tracks)
            existingTracks.push_back (entry);
    }

    if (onLifecycle)
        for (auto& entry : existingTracks)
            onLifecycle (user, entry.inputId, entry.variantTag.c_str(), 1);

    static WfsBridgeMasterHandle handle { 1 };
    return &handle;
}

void wfs_bridge_master_unregister (WfsBridgeMasterHandle* /*handle*/)
{
    auto& r = getRegistry();
    std::lock_guard<std::mutex> sl (r.lock);
    r.master.reset();
}

void wfs_bridge_master_dispatch_inbound (WfsBridgeMasterHandle* /*handle*/,
                                         int inputId,
                                         const char* oscPath,
                                         double value)
{
    auto& r = getRegistry();
    std::vector<TrackEntry> targets;
    {
        std::lock_guard<std::mutex> sl (r.lock);
        for (auto& [id, entry] : r.tracks)
            if (entry.inputId == inputId)
                targets.push_back (entry);
    }
    for (auto& entry : targets)
        if (entry.onInbound)
            entry.onInbound (entry.user, oscPath, inputId, value);
}

int wfs_bridge_master_snapshot_input_ids (WfsBridgeMasterHandle* /*handle*/,
                                          int* outIds,
                                          int maxIds)
{
    if (outIds == nullptr || maxIds <= 0)
        return 0;

    auto& r = getRegistry();
    std::set<int> unique;
    {
        std::lock_guard<std::mutex> sl (r.lock);
        for (auto& [id, entry] : r.tracks)
            unique.insert (entry.inputId);
    }
    int written = 0;
    for (int id : unique)
    {
        if (written >= maxIds) break;
        outIds[written++] = id;
    }
    return written;
}

WfsBridgeTrackHandle* wfs_bridge_track_register (int inputId,
                                                 const char* variantTag,
                                                 void* user,
                                                 WfsBridgeInboundFn onInbound)
{
    auto& r = getRegistry();
    MasterEntry masterSnapshot;
    bool notify = false;
    int id = 0;
    {
        std::lock_guard<std::mutex> sl (r.lock);
        id = r.nextTrackId.fetch_add (1);
        auto& entry = r.tracks[id];
        entry.inputId    = inputId;
        entry.variantTag = variantTag != nullptr ? variantTag : "";
        entry.user       = user;
        entry.onInbound  = onInbound;

        if (r.master != nullptr)
        {
            masterSnapshot = *r.master;
            notify = true;
        }
    }
    if (notify && masterSnapshot.onLifecycle)
        masterSnapshot.onLifecycle (masterSnapshot.user, inputId,
                                    variantTag != nullptr ? variantTag : "", 1);

    return new WfsBridgeTrackHandle { id };
}

void wfs_bridge_track_unregister (WfsBridgeTrackHandle* handle)
{
    if (handle == nullptr)
        return;
    auto& r = getRegistry();

    int lastInputId = 0;
    std::string lastVariant;
    bool stillReferenced = false;
    MasterEntry masterSnapshot;
    bool hasMaster = false;

    {
        std::lock_guard<std::mutex> sl (r.lock);
        auto it = r.tracks.find (handle->id);
        if (it != r.tracks.end())
        {
            lastInputId = it->second.inputId;
            lastVariant = it->second.variantTag;
            r.tracks.erase (it);

            for (auto& [oid, entry] : r.tracks)
                if (entry.inputId == lastInputId)
                {
                    stillReferenced = true;
                    break;
                }
        }
        if (r.master != nullptr)
        {
            masterSnapshot = *r.master;
            hasMaster = true;
        }
    }

    if (hasMaster && ! stillReferenced && masterSnapshot.onLifecycle)
        masterSnapshot.onLifecycle (masterSnapshot.user, lastInputId,
                                    lastVariant.c_str(), 0);

    delete handle;
}

void wfs_bridge_track_send_outbound (WfsBridgeTrackHandle* handle,
                                     const char* oscPath,
                                     int channelId,
                                     double value)
{
    if (handle == nullptr)
        return;
    auto& r = getRegistry();
    auto masterCopy = snapshotMaster (r);
    if (masterCopy.onOutbound)
        masterCopy.onOutbound (masterCopy.user, oscPath, channelId, value);
}

int wfs_bridge_track_count()
{
    auto& r = getRegistry();
    std::lock_guard<std::mutex> sl (r.lock);
    return static_cast<int> (r.tracks.size());
}

int wfs_bridge_has_master()
{
    auto& r = getRegistry();
    std::lock_guard<std::mutex> sl (r.lock);
    return r.master != nullptr ? 1 : 0;
}

}
