#pragma once

#include <JuceHeader.h>
#include <atomic>
#include "WFSFileManager.h"
#include "EffectsSnapshotScope.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Network/OSCProtocolTypes.h"

/**
 * Parameter Dirty Tracker
 *
 * Tracks which input AND effect parameters have been modified by the user (UI or
 * Remote app) since the last reset event. Used by the snapshot scope window to
 * offer auto-preselection of modified parameters, speeding up cuelist authoring.
 *
 * Dirty state uses the same key format as ExtendedSnapshotScope: "itemId_channelIndex".
 * One key set serves both families: every effects item id starts with "fx" and
 * no input item id does, so "fxEq2_1" (effect 2's EQ 2) can never be read as an
 * input key. An input key's channel is the input SLOT, an effect key's the DENSE
 * effect index - each grid asks with its own.
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
    // Non-user write suppression (AutomOtion playback, tracking receivers, …)
    //==========================================================================

    /** RAII guard: suppresses dirty marking for writes performed inside its scope.
     *  Unlike beginSuppression(), this does NOT clear existing dirty state —
     *  it only prevents new writes from being flagged. Safe to use across threads. */
    struct ScopedInternalWrite
    {
        explicit ScopedInternalWrite (ParameterDirtyTracker* t) noexcept : tracker (t)
        {
            if (tracker != nullptr)
                tracker->nonUserWriteDepth.fetch_add (1, std::memory_order_relaxed);
        }
        ~ScopedInternalWrite() noexcept
        {
            if (tracker != nullptr)
                tracker->nonUserWriteDepth.fetch_sub (1, std::memory_order_relaxed);
        }
        ParameterDirtyTracker* tracker;
        JUCE_DECLARE_NON_COPYABLE (ScopedInternalWrite)
    };

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
        // Resolve the scope item ID - and the channel - for this property change
        juce::String itemId;
        int channelIndex = -1;

        if (isInputParameterTree (tree))
        {
            itemId = resolveItemId (tree, property);
            channelIndex = extractChannelIndex (tree);
        }
        else if (auto child = directChildOfEffect (tree); child.isValid())
        {
            // The node the property sits on decides the item for a module (a
            // band or a tap reports its module), the property for a flat node.
            // effectSolo / effectOtomoPauseResume / effectName resolve to no
            // item and so never dirty anything, like inputSolo.
            itemId = EffectsSnapshotScope::itemIdFor (child.getType(), property);
            channelIndex = extractEffectChannelIndex (child.getParent());
        }

        if (itemId.isEmpty())
            return;

        // Skip during snapshot loading or non-user writes (AutomOtion, tracking, …)
        if (suppressTracking || nonUserWriteDepth.load (std::memory_order_relaxed) > 0)
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
            markDirtyKey (itemId, channelIndex);
        }
    }

    void valueTreeChildAdded (juce::ValueTree& parent, juce::ValueTree& child) override
    {
        handleSubtreeChildChange (parent, child);
    }

    void valueTreeChildRemoved (juce::ValueTree& parent, juce::ValueTree& child, int) override
    {
        handleSubtreeChildChange (parent, child);
    }

    void valueTreeChildOrderChanged (juce::ValueTree&, int, int) override {}
    void valueTreeParentChanged (juce::ValueTree&) override {}

private:
    juce::ValueTree state;

    // Threading invariant: dirtyKeys is mutated only from the message thread.
    // ValueTree listeners fire on whichever thread calls setProperty; the
    // only background-thread writers (AutomOtion playback, tracking receivers)
    // wrap their writes in ScopedInternalWrite, which short-circuits before
    // reaching markDirty(). If a future contributor adds a background writer
    // that is NOT suppressed, this set must be guarded (juce::CriticalSection).
    std::set<juce::String> dirtyKeys;
    bool suppressTracking = false;
    std::atomic<int> nonUserWriteDepth {0};

    /** Reverse lookup: paramId string -> scope itemId */
    std::unordered_map<juce::String, juce::String> paramToItemMap;

    /** Extract 0-based channel SLOT from a ValueTree node.
     *  Walks up to the Input node and returns its child index — NOT id - 1:
     *  the id is the permanent channel number, and with gaps/reordering it no
     *  longer coincides with the slot the scope machinery works in. */
    int extractChannelIndex (const juce::ValueTree& tree) const
    {
        auto node = tree;
        while (node.isValid())
        {
            if (node.getType() == WFSParameterIDs::Input)
            {
                auto parent = node.getParent();
                return parent.isValid() ? parent.indexOf (node) : -1;
            }
            node = node.getParent();
        }
        return -1;
    }

    /** The direct child of an <Effect> (under <Effects>) that `tree` is or lives
        under - its flat node or its module node - or an invalid tree when
        `tree` is not inside an effect channel's parameters. */
    juce::ValueTree directChildOfEffect (const juce::ValueTree& tree) const
    {
        juce::ValueTree child;
        for (auto node = tree; node.isValid(); node = node.getParent())
        {
            if (node.getType() == WFSParameterIDs::Effect)
                return node.getParent().hasType (WFSParameterIDs::Effects) ? child : juce::ValueTree();
            child = node;
        }
        return {};
    }

    /** The DENSE index of an <Effect>: its ordinal among the <Effect>-typed
        children of <Effects>, the count-by-type walk every effects accessor uses
        (WFSValueTreeState::getEffectState) - never id - 1, never indexOf. */
    int extractEffectChannelIndex (const juce::ValueTree& effect) const
    {
        auto parent = effect.getParent();
        if (! parent.isValid())
            return -1;

        int dense = 0;
        for (int i = 0; i < parent.getNumChildren(); ++i)
        {
            const auto child = parent.getChild (i);
            if (child == effect)
                return dense;
            if (child.hasType (WFSParameterIDs::Effect))
                ++dense;
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

    /** Walk up from the given tree until a GradientLayer ancestor is found. */
    juce::ValueTree findEnclosingGradientLayer (juce::ValueTree tree) const
    {
        while (tree.isValid())
        {
            if (tree.getType() == WFSParameterIDs::GradientLayer)
                return tree;
            tree = tree.getParent();
        }
        return {};
    }

    /** Produce the gmLayer{1,2,3} scope itemId for a tree inside a GradientLayer.
     *  Returns empty string if not inside a layer or the layer id is invalid. */
    juce::String getGradientLayerItemId (const juce::ValueTree& tree) const
    {
        auto layer = findEnclosingGradientLayer (tree);
        if (!layer.isValid())
            return {};
        int idx = static_cast<int> (layer.getProperty (WFSParameterIDs::id, -1));
        if (idx < 0 || idx > 2)
            return {};
        return "gmLayer" + juce::String (idx + 1);
    }

    /** Resolve which scope item a property change on `tree` belongs to. */
    juce::String resolveItemId (const juce::ValueTree& tree,
                                 const juce::Identifier& property) const
    {
        // Gradient layer params are shared across gmLayer1/2/3 — disambiguate via layer id
        if (auto gm = getGradientLayerItemId (tree); gm.isNotEmpty())
            return gm;

        // Per-channel sampler on/off lives on the Channel subtree, not the Sampler
        // subtree, and isn't in paramToItemMap — handle it explicitly.
        if (property == WFSParameterIDs::inputSamplerActive)
            return "sampler";

        if (auto it = paramToItemMap.find (property.toString()); it != paramToItemMap.end())
            return it->second;

        if (isSamplerSubtree (tree))
            return "sampler";

        return {};
    }

    /** Thread-safe notification: posts to message thread via AsyncUpdater */
    void notifyDirtyStateChanged()
    {
        triggerAsyncUpdate();
    }

    /** Mark a scope item as dirty for the channel that owns the given tree node */
    void markDirty (const juce::String& itemId, const juce::ValueTree& tree)
    {
        markDirtyKey (itemId, extractChannelIndex (tree));
    }

    /** Mark a scope item as dirty for a channel already resolved */
    void markDirtyKey (const juce::String& itemId, int channelIndex)
    {
        if (channelIndex >= 0)
        {
            auto key = ExtendedScope::makeKey (itemId, channelIndex);
            auto [pos, inserted] = dirtyKeys.insert (key);
            if (inserted)
                notifyDirtyStateChanged();
        }
    }

    /** Handle subtree-based scope items (sampler cells/sets, gradient layer shapes). */
    void handleSubtreeChildChange (juce::ValueTree& parent, juce::ValueTree& child)
    {
        if (!isInputParameterTree (parent))
            return;

        // Resolve itemId based on which subtree the change belongs to
        juce::String itemId;
        if (isSamplerSubtree (parent) || isSamplerSubtree (child))
            itemId = "sampler";
        else if (auto gm = getGradientLayerItemId (child); gm.isNotEmpty())
            itemId = gm;
        else if (auto gm = getGradientLayerItemId (parent); gm.isNotEmpty())
            itemId = gm;
        else
            return;

        if (suppressTracking || nonUserWriteDepth.load (std::memory_order_relaxed) > 0)
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
            markDirty (itemId, parent);
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterDirtyTracker)
};
