/*
  ==============================================================================
    LightpadManager.h
    Orchestrates 1-3 ROLI Lightpad Blocks.
    Handles topology changes, zone-to-input mapping, position integration,
    and LED state management.

    Arrangement is auto-detected from the physical topology using
    Block::getBlockAreaWithinLayout() — no manual configuration needed.
  ==============================================================================
*/

#pragma once

#include <roli_blocks_basics/roli_blocks_basics.h>
#include "LightpadDevice.h"
#include "LightpadTypes.h"
#include "../../Parameters/WFSParameterIDs.h"
#include "../../Parameters/WFSParameterDefaults.h"
#include "../../Network/OSCProtocolTypes.h"

class WfsParameters;  // forward declare

class LightpadManager : public roli::TopologySource::Listener,
                         private juce::Timer
{
public:
    //==============================================================================
    static constexpr int maxPads = 3;

    struct Callbacks
    {
        std::function<void (int inputIndex, float dx, float dy)> moveInputDelta;
        std::function<void (int inputIndex, float pressure)>     applyPressure;
        std::function<void (int inputIndex, float pressure)>     onTouchStart;
        std::function<void (int inputIndex)>                     onTouchEnd;
    };

    Callbacks callbacks;

    // Topology change notification — fires on message thread with current pad layout
    std::function<void (const std::vector<PadLayoutInfo>&)> onTopologyChanged;

    //==============================================================================
    explicit LightpadManager (WfsParameters& params)
        : parameters (params)
    {
        topologySource.addListener (this);
    }

    ~LightpadManager() override
    {
        onTopologyChanged = nullptr;  // prevent callbacks during teardown
        callbacks = {};
        stopTimer();
        topologySource.removeListener (this);
        devices.clear();
    }

    //==============================================================================
    // Lifecycle
    //==============================================================================
    void start()
    {
        topologySource.setActive (true);
        startTimerHz (50);  // 50 Hz position integration
    }

    void stop()
    {
        stopTimer();
        topologySource.setActive (false);
    }

    //==============================================================================
    // Configuration (called from GUI)
    //==============================================================================
    void setPadSplit (int padIndex, bool split)
    {
        if (padIndex >= 0 && padIndex < maxPads)
        {
            padSplit[padIndex] = split;

            for (auto& dev : devices)
                if (dev->getPadIndex() == padIndex)
                    dev->setSplit (split);

            // Update cached layout info
            for (auto& pl : padLayouts)
                if (pl.padIndex == padIndex)
                    pl.isSplit = split;

            updateZoneColours();
        }
    }

    bool getPadSplit (int padIndex) const
    {
        return padIndex >= 0 && padIndex < maxPads ? padSplit[padIndex] : false;
    }

    void setSensitivity (float sens) { sensitivity = sens; }
    float getSensitivity() const     { return sensitivity; }

    //==============================================================================
    // Pad layout queries
    //==============================================================================
    int getNumDetectedPads() const { return numDetectedPads.load(); }

    std::vector<PadLayoutInfo> getPadLayouts() const { return padLayouts; }

    //==============================================================================
    // Zone assignment
    //==============================================================================
    void assignZoneToInput (int zoneId, int inputChannel)
    {
        // Remove any previous assignment for this zone
        unassignZone (zoneId);

        // Remove any previous zone assignment for this input
        for (auto it = zoneToInput.begin(); it != zoneToInput.end(); )
        {
            if (it->second == inputChannel)
                it = zoneToInput.erase (it);
            else
                ++it;
        }

        if (zoneId >= 0)
            zoneToInput[zoneId] = inputChannel;

        updateZoneColours();
    }

    void unassignZone (int zoneId)
    {
        zoneToInput.erase (zoneId);
        updateZoneColours();
    }

    int getInputForZone (int zoneId) const
    {
        auto it = zoneToInput.find (zoneId);
        return it != zoneToInput.end() ? it->second : -1;
    }

    int getZoneForInput (int inputChannel) const
    {
        for (auto& [zone, input] : zoneToInput)
            if (input == inputChannel)
                return zone;

        return -1;
    }

    std::set<int> getAssignedZoneIds() const
    {
        std::set<int> result;
        for (auto& [zone, input] : zoneToInput)
            result.insert (zone);
        return result;
    }

    std::map<int, int> getAssignedZonesMap() const
    {
        return zoneToInput;  // zoneId → inputChannel (0-based)
    }

    std::vector<int> getActiveZoneIdsList() const
    {
        return getActiveZoneIds (numDetectedPads.load(), padSplit);
    }

    //==============================================================================
    // Zone display helpers
    //==============================================================================
    juce::String getZoneDisplayName (int zoneId) const
    {
        return ::getZoneDisplayName (zoneId, padSplit);
    }

    std::vector<std::pair<int, juce::String>> getAllZonesWithNames() const
    {
        auto ids = getActiveZoneIdsList();
        std::vector<std::pair<int, juce::String>> result;

        for (int id : ids)
            result.push_back ({ id, getZoneDisplayName (id) });

        return result;
    }

    //==============================================================================
    // LED zone number display (for combobox interaction)
    //==============================================================================
    void showZoneNumbersOnLeds (bool show)
    {
        for (auto& dev : devices)
            dev->setShowZoneNumbers (show);
    }

    //==============================================================================
    // Connected device queries
    //==============================================================================
    int getNumConnectedDevices() const { return static_cast<int> (devices.size()); }

    struct ConnectedDeviceInfo
    {
        roli::Block::UID uid;
        juce::String name;
        juce::String serialNumber;
        int assignedPadIndex;
    };

    std::vector<ConnectedDeviceInfo> getConnectedDevices() const
    {
        std::vector<ConnectedDeviceInfo> result;

        for (auto& dev : devices)
        {
            auto block = dev->getBlock();
            result.push_back ({ block->uid, block->name, block->serialNumber,
                                dev->getPadIndex() });
        }

        return result;
    }

    //==============================================================================
    // TopologySource::Listener — auto-detect arrangement from spatial positions
    //==============================================================================
    void topologyChanged() override
    {
        auto topology = topologySource.getCurrentTopology();

        // Collect all Lightpad blocks with their spatial positions
        struct BlockInfo
        {
            roli::Block::Ptr block;
            int layoutX, layoutY;
            bool isMaster;
        };

        std::vector<BlockInfo> lightpads;

        for (auto& block : topology.blocks)
        {
            if (block->getType() != roli::Block::lightPadBlock)
                continue;

            auto area = block->getBlockAreaWithinLayout();
            lightpads.push_back ({ block, area.x, area.y, block->isMasterBlock() });
        }

        // Sort by (y, x) — top-to-bottom, left-to-right
        std::sort (lightpads.begin(), lightpads.end(),
            [] (const BlockInfo& a, const BlockInfo& b)
            {
                if (a.layoutY != b.layoutY) return a.layoutY < b.layoutY;
                return a.layoutX < b.layoutX;
            });

        // Clamp to maxPads
        if (lightpads.size() > static_cast<size_t> (maxPads))
            lightpads.resize (maxPads);

        // Remove devices whose blocks are no longer in the topology
        devices.erase (
            std::remove_if (devices.begin(), devices.end(),
                [&topology] (const std::unique_ptr<LightpadDevice>& dev)
                {
                    for (auto& block : topology.blocks)
                        if (block->uid == dev->getBlockUID())
                            return false;
                    return true;  // block gone, remove device
                }),
            devices.end());

        // Assign pad indices based on sorted spatial position
        for (int i = 0; i < static_cast<int> (lightpads.size()); ++i)
        {
            auto& info = lightpads[i];
            int padIdx = i;

            // Check if device already tracked
            bool found = false;
            for (auto& dev : devices)
            {
                if (dev->getBlockUID() == info.block->uid)
                {
                    dev->setPadIndex (padIdx);
                    dev->setSplit (padSplit[padIdx]);
                    found = true;
                    break;
                }
            }

            if (! found)
            {
                auto device = std::make_unique<LightpadDevice> (info.block, padIdx);
                device->setSplit (padSplit[padIdx]);

                // Wire touch callback
                device->onZoneTouch = [this] (int zoneId, float dx, float dy,
                                              float pressure, bool isStart, bool isEnd)
                {
                    handleZoneTouch (zoneId, dx, dy, pressure, isStart, isEnd);
                };

                devices.push_back (std::move (device));
            }
        }

        // Update detected pad count and layout info
        numDetectedPads.store (static_cast<int> (lightpads.size()));

        padLayouts.clear();
        for (int i = 0; i < static_cast<int> (lightpads.size()); ++i)
        {
            padLayouts.push_back ({ i, lightpads[i].layoutX, lightpads[i].layoutY,
                                     lightpads[i].isMaster, padSplit[i] });
        }

        updateZoneColours();

        // Notify GUI about topology change (on message thread)
        if (onTopologyChanged)
        {
            auto layoutsCopy = padLayouts;
            juce::MessageManager::callAsync ([cb = onTopologyChanged, layoutsCopy]()
            {
                cb (layoutsCopy);
            });
        }
    }

private:
    //==============================================================================
    WfsParameters& parameters;

    roli::PhysicalTopologySource topologySource;
    std::vector<std::unique_ptr<LightpadDevice>> devices;

    std::atomic<int> numDetectedPads { 0 };
    std::vector<PadLayoutInfo> padLayouts;  // current topology layout
    bool padSplit[maxPads] = { false, false, false };
    float sensitivity = WFSParameterDefaults::lightpadSensitivityDefault;

    std::map<int, int> zoneToInput;  // zoneId → inputChannel (0-based)

    // Accumulated deflections per zone (for 50Hz integration)
    struct ZoneTouchAccum
    {
        float dx = 0.0f;
        float dy = 0.0f;
        float pressure = 0.0f;
        bool active = false;
    };

    std::map<int, ZoneTouchAccum> zoneTouchState;

    //==============================================================================
    void handleZoneTouch (int zoneId, float dx, float dy, float pressure,
                          bool isStart, bool isEnd)
    {
        // Phase 7: tag onTouchStart / onTouchEnd as hardware-origin so any
        // ValueTree side effects (e.g. sample triggering) are correctly
        // credited.
        WFSNetwork::OriginTagScope originScope { WFSNetwork::OriginTag::Hardware };

        // Resolve zone → input channel for start/end callbacks
        auto it = zoneToInput.find (zoneId);
        int inputIdx = (it != zoneToInput.end()) ? it->second : -1;

        if (isEnd)
        {
            zoneTouchState.erase (zoneId);
            if (inputIdx >= 0 && callbacks.onTouchEnd)
                callbacks.onTouchEnd (inputIdx);
            return;
        }

        if (isStart && inputIdx >= 0 && callbacks.onTouchStart)
            callbacks.onTouchStart (inputIdx, pressure);

        auto& state = zoneTouchState[zoneId];
        state.dx = dx;
        state.dy = dy;
        state.pressure = pressure;
        state.active = true;
    }

    //==============================================================================
    // 50 Hz timer for position integration
    //==============================================================================
    void timerCallback() override
    {
        // Phase 7: every per-tick callback below performs a hardware-driven
        // ValueTree write. Wrap once at the chokepoint so change records and
        // cross-actor notifications credit the Lightpad.
        WFSNetwork::OriginTagScope originScope { WFSNetwork::OriginTag::Hardware };

        for (auto& [zoneId, state] : zoneTouchState)
        {
            if (! state.active) continue;

            auto it = zoneToInput.find (zoneId);
            if (it == zoneToInput.end()) continue;

            int inputIdx = it->second;

            float scaledDx = state.dx * sensitivity;
            float scaledDy = -state.dy * sensitivity;  // Negate: Lightpad Y increases downward, WFS Y increases forward

            if (callbacks.moveInputDelta)
                callbacks.moveInputDelta (inputIdx, scaledDx, scaledDy);

            if (callbacks.applyPressure)
                callbacks.applyPressure (inputIdx, state.pressure);
        }
    }

    //==============================================================================
    void updateZoneColours()
    {
        auto activeIds = getActiveZoneIdsList();
        auto assigned  = getAssignedZoneIds();

        for (auto& dev : devices)
        {
            int pi = dev->getPadIndex();
            auto padColour = LightpadColours::getPadColour (pi);
            juce::Colour colours[4];

            for (int q = 0; q < 4; ++q)
            {
                int zid = encodeZoneId (pi, q);

                if (assigned.count (zid) > 0)
                    colours[q] = padColour;
                else
                    colours[q] = padColour.withAlpha (0.3f);
            }

            dev->setAllZoneColours (colours);
        }
    }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LightpadManager)
};
