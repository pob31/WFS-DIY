#pragma once

#include <JuceHeader.h>
#include "WFSFileManager.h"
#include "../Network/OSCProtocolTypes.h"

/**
 * Parameter Dirty Tracker
 *
 * Tracks which input parameters have been modified by the user (UI or Remote app)
 * since the last reset event. Used by the snapshot scope window to offer
 * auto-preselection of modified parameters, speeding up cuelist authoring.
 *
 * Dirty state uses the same key format as ExtendedSnapshotScope: "itemId_channelIndex".
 *
 * Reset events:
 *   - Any DAW OSC received (Protocol::OSC / ADMOSC) -> clear all
 *   - Snapshot recall -> clear all (via beginSuppression/endSuppressionAndClear)
 *   - Snapshot store/update -> clear all (via clearAll)
 *   - Config file load/import -> clear all (via beginSuppression/endSuppressionAndClear)
 */
class ParameterDirtyTracker : public juce::ValueTree::Listener,
                               public juce::AsyncUpdater
{
public:
    using ExtendedScope = WFSFileManager::ExtendedSnapshotScope;
    using ScopeItem = WFSFileManager::ScopeItem;

    explicit ParameterDirtyTracker (juce::ValueTree rootState)
        : state (rootState)
    {
        // Build reverse lookup: paramId -> itemId
        for (const auto& item : ExtendedScope::getScopeItems())
        {
            for (const auto& paramId : item.parameterIds)
                paramToItemMap[paramId.toString()] = item.itemId;
        }

        state.addListener (this);
    }

    ~ParameterDirtyTracker() override
    {
        state.removeListener (this);
    }

    //==========================================================================
    // Source detection delegate
    //==========================================================================

    /** Returns the current incoming protocol. Set by MainComponent after
     *  OSCManager is created. When null/unset, assumes Protocol::Disabled (UI). */
    std::function<WFSNetwork::Protocol()> getIncomingProtocol;

    //==========================================================================
    // Query methods
    //==========================================================================

    /** Check if a scope item is dirty for a specific channel */
    bool isDirty (const juce::String& itemId, int channelIndex) const
    {
        return dirtyKeys.find (ExtendedScope::makeKey (itemId, channelIndex)) != dirtyKeys.end();
    }

    /** Check if any parameters are dirty at all */
    bool hasAnyDirty() const
    {
        return !dirtyKeys.empty();
    }

    /** Get all dirty keys for bulk operations (e.g., copying to scope) */
    const std::set<juce::String>& getDirtyKeys() const
    {
        return dirtyKeys;
    }

    //==========================================================================
    // Clear / suppression methods
    //==========================================================================

    /** Clear ALL dirty flags. Called after snapshot store/update or DAW OSC. */
    void clearAll()
    {
        if (!dirtyKeys.empty())
        {
            dirtyKeys.clear();
            notifyDirtyStateChanged();
        }
    }

    /** Begin suppression — dirty tracking is paused (e.g., during snapshot recall).
     *  Call endSuppressionAndClear() when done. */
    void beginSuppression()
    {
        suppressTracking = true;
    }

    /** End suppression and clear all dirty flags. */
    void endSuppressionAndClear()
    {
        suppressTracking = false;
        clearAll();
    }

    //==========================================================================
    // Listener notification
    //==========================================================================

    /** Callback when dirty state changes (for UI repaint). Called on message thread. */
    std::function<void()> onDirtyStateChanged;

    //==========================================================================
    // AsyncUpdater — marshal notifications to the message thread
    //==========================================================================

    void handleAsyncUpdate() override
    {
        if (onDirtyStateChanged)
            onDirtyStateChanged();
    }

    //==========================================================================
    // ValueTree::Listener
    //==========================================================================

    void valueTreePropertyChanged (juce::ValueTree& tree,
                                    const juce::Identifier& property) override
    {
        // Only track input parameters
        if (!isInputParameterTree (tree))
            return;

        // Resolve the scope item ID for this property change
        juce::String itemId;
        auto it = paramToItemMap.find (property.toString());
        if (it != paramToItemMap.end())
        {
            itemId = it->second;
        }
        else if (isSamplerSubtree (tree))
        {
            itemId = "sampler";
        }
        else
        {
            return;
        }

        // Skip during snapshot loading
        if (suppressTracking)
            return;

        // Determine the source of this change
        auto protocol = getIncomingProtocol ? getIncomingProtocol()
                                            : WFSNetwork::Protocol::Disabled;

        // DAW OSC or ADM-OSC -> clear ALL dirty flags
        if (protocol == WFSNetwork::Protocol::OSC ||
            protocol == WFSNetwork::Protocol::ADMOSC)
        {
            clearAll();
            return;
        }

        // UI (Disabled) or Remote -> mark dirty
        if (protocol == WFSNetwork::Protocol::Disabled ||
            protocol == WFSNetwork::Protocol::Remote)
        {
            markDirty (itemId, tree);
        }
    }

    void valueTreeChildAdded (juce::ValueTree& parent, juce::ValueTree& child) override
    {
        handleSamplerChildChange (parent, child);
    }

    void valueTreeChildRemoved (juce::ValueTree& parent, juce::ValueTree& child, int) override
    {
        handleSamplerChildChange (parent, child);
    }

    void valueTreeChildOrderChanged (juce::ValueTree&, int, int) override {}
    void valueTreeParentChanged (juce::ValueTree&) override {}

private:
    juce::ValueTree state;
    std::set<juce::String> dirtyKeys;
    bool suppressTracking = false;

    /** Reverse lookup: paramId string -> scope itemId */
    std::unordered_map<juce::String, juce::String> paramToItemMap;

    /** Extract 0-based channel index from a ValueTree node.
     *  Walks up the tree to find the Input node and reads its "id" property (1-based). */
    int extractChannelIndex (const juce::ValueTree& tree) const
    {
        auto node = tree;
        while (node.isValid())
        {
            if (node.getType() == WFSParameterIDs::Input)
                return static_cast<int> (node.getProperty (WFSParameterIDs::id)) - 1;
            node = node.getParent();
        }
        return -1;
    }

    /** Check if a tree node is inside the Inputs hierarchy */
    bool isInputParameterTree (const juce::ValueTree& tree) const
    {
        auto node = tree;
        while (node.isValid())
        {
            if (node.getType() == WFSParameterIDs::Inputs)
                return true;
            node = node.getParent();
        }
        return false;
    }

    /** Check if a tree node is part of the Sampler subtree (Sampler, SamplerCell, or SamplerSet) */
    bool isSamplerSubtree (const juce::ValueTree& tree) const
    {
        auto type = tree.getType();
        return type == WFSParameterIDs::Sampler
            || type == WFSParameterIDs::SamplerCell
            || type == WFSParameterIDs::SamplerSet;
    }

    /** Thread-safe notification: posts to message thread via AsyncUpdater */
    void notifyDirtyStateChanged()
    {
        triggerAsyncUpdate();
    }

    /** Mark a scope item as dirty for the channel that owns the given tree node */
    void markDirty (const juce::String& itemId, const juce::ValueTree& tree)
    {
        int channelIndex = extractChannelIndex (tree);
        if (channelIndex >= 0)
        {
            auto key = ExtendedScope::makeKey (itemId, channelIndex);
            auto [pos, inserted] = dirtyKeys.insert (key);
            if (inserted)
                notifyDirtyStateChanged();
        }
    }

    /** Handle sampler child add/remove — mark sampler dirty for that channel */
    void handleSamplerChildChange (juce::ValueTree& parent, juce::ValueTree& child)
    {
        if (!isInputParameterTree (parent))
            return;
        if (!isSamplerSubtree (parent) && !isSamplerSubtree (child))
            return;
        if (suppressTracking)
            return;

        auto protocol = getIncomingProtocol ? getIncomingProtocol()
                                            : WFSNetwork::Protocol::Disabled;
        if (protocol == WFSNetwork::Protocol::OSC ||
            protocol == WFSNetwork::Protocol::ADMOSC)
        {
            clearAll();
            return;
        }

        if (protocol == WFSNetwork::Protocol::Disabled ||
            protocol == WFSNetwork::Protocol::Remote)
        {
            markDirty ("sampler", parent);
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterDirtyTracker)
};
