#pragma once

#include <JuceHeader.h>
#include "WFSParameterIDs.h"
#include "WFSFileManager.h"
#include "WFSValueTreeState.h"

//==============================================================================
/**
    The effects half of a snapshot: what an <Effect> entry carries, and how it is
    captured, applied and trimmed.

    ONE SNAPSHOT FILE CARRIES BOTH FAMILIES (plan revision 8). The input half keeps
    its own loops in WFSFileManager; this is the effects half, called from the same
    save / load / scope-update functions, over the effects scope matrix.

    WHY THE EFFECTS HALF CANNOT REUSE THE INPUT LOOPS. The input loops find a
    parameter's node by `hasProperty`, which is exact there because no input
    property lives on two <Input> child nodes. On an effect that holds for the
    eight FLAT nodes (Channel, Position, Feed, Return, AutomOtion, LFO, Chain,
    Sends - every name on them is unique) and fails for the eleven MODULE nodes:
    FxEq1 and FxEq2 carry identical names, so do FxDyn1 and FxDyn2, and the six
    <Band>s and eight <Tap>s repeat theirs by index (WFSValueTreeState.h, the
    "WHAT THE EXCEPTION COSTS" note). So the scope is a hybrid:
      - property items over the flat nodes, discriminated by hasProperty exactly
        as the input half does;
      - one WHOLE-NODE item per module, keyed by the module's node TYPE, copied as
        a subtree and applied property by property onto the live node, children
        matched by type and id.

    APPLY WRITES ONLY WHAT THE LIVE NODE ALREADY HAS. Nothing may stamp a property
    onto an <Effect> subtree that the channel builder does not stamp
    (stripObsoleteEffectProperties evicts it on the next load), and nothing here
    ever adds or removes a child. A snapshot from an older schema therefore
    restores what still exists and leaves the rest alone.

    THE FIVE PACKED ROWS go through setEffectParameter, never a raw setProperty:
    the store's write interceptor fits effectMutes to the live outputs and keeps
    the four send rows at their fixed widths with the fx diagonal forced off. A raw
    write would skip all of that. Every other value is written raw, as the input
    half writes its own, so a recall neither clamps nor propagates through a link
    group (EffectParamEdit is the GUI's funnel, never a recall's).
*/
namespace EffectsSnapshotScope
{
    using ScopeItem      = WFSFileManager::ScopeItem;
    using ScopeItemTable = WFSFileManager::ScopeItemTable;
    using ScopeMatrix    = WFSFileManager::ScopeMatrix;

    inline const ScopeItemTable& table() { return WFSFileManager::effectScopeTable(); }

    /** The eight flat child nodes of an <Effect>, walked as property sections. */
    inline bool isFlatNode (const juce::Identifier& type)
    {
        using namespace WFSParameterIDs;
        return type == Channel || type == Position || type == Feed || type == ReverbReturn
            || type == AutomOtion || type == LFO || type == Chain || type == Sends;
    }

    /** The module item a node type is the whole of, or empty when it is none. */
    inline juce::String moduleItemId (const juce::Identifier& nodeType)
    {
        if (! nodeType.isValid())
            return {};

        for (const auto& item : table().items)
            if (item.nodeType == nodeType)
                return item.itemId;

        return {};
    }

    /** The scope item a property belongs to, given the DIRECT child of <Effect>
        it lives under (for a <Band> or a <Tap>, that is its module node).
        Empty when no item carries it - effectName, effectSolo,
        effectOtomoPauseResume, or anything the table does not know. */
    inline juce::String itemIdFor (const juce::Identifier& childOfEffect, const juce::Identifier& property)
    {
        if (auto module = moduleItemId (childOfEffect); module.isNotEmpty())
            return module;

        if (! isFlatNode (childOfEffect))
            return {};

        for (const auto& item : table().items)
            if (! item.nodeType.isValid())
                for (const auto& p : item.parameterIds)
                    if (p == property)
                        return item.itemId;

        return {};
    }

    /** The five packed rows: written through setEffectParameter so the
        interceptor sees them. */
    inline bool isPackedRow (const juce::Identifier& property)
    {
        using namespace WFSParameterIDs;
        return property == effectMutes
            || property == effectSendLevels   || property == effectSendOns
            || property == effectFxSendLevels || property == effectFxSendOns;
    }

    /** True when a snapshot carries `property` of the node `childOfEffect`
        through some scope item. The coverage self-test (phase Q) asks this of
        every property a live channel has: an omission here would be symmetric
        between store and recall, so no round trip could ever see it. */
    inline bool isEffectPropertyCovered (const juce::Identifier& childOfEffect, const juce::Identifier& property)
    {
        return itemIdFor (childOfEffect, property).isNotEmpty();
    }

    /** Properties no scope item carries, on purpose. Phase Q asserts both ways:
        everything else is covered, and none of these is. */
    struct Excluded { const char* name; const char* why; };

    inline const std::vector<Excluded>& notSnapshotted()
    {
        static const std::vector<Excluded> list = {
            { "effectName",             "always captured and always applied, outside the scope system "
                                        "(the <Channel> rule the inputs follow for inputName)" },
            { "effectSolo",             "transient monitoring state, not show state; unlike inputSolo it is "
                                        "persisted in the tree, so it is kept out of the snapshot by omission" },
            { "effectOtomoPauseResume", "run-state of a motion in flight, not a destination" },
        };
        return list;
    }

    //==========================================================================
    // Capture / apply / trim
    //==========================================================================

    /** The live channel `fx` (dense, 0-based) as a snapshot entry
        <Effect id="fx + 1">, carrying what `matrix` includes for it. effectName
        is always carried. */
    inline juce::ValueTree extractEffect (WFSValueTreeState& vts, int fx, const ScopeMatrix& matrix)
    {
        using namespace WFSParameterIDs;

        juce::ValueTree entry (Effect);
        entry.setProperty (id, fx + 1, nullptr);

        auto effect = vts.getEffectState (fx);
        if (! effect.isValid())
            return entry;

        for (int i = 0; i < effect.getNumChildren(); ++i)
        {
            const auto node = effect.getChild (i);
            const auto type = node.getType();

            if (const auto module = moduleItemId (type); module.isNotEmpty())
            {
                if (matrix.isIncluded (module, fx))
                    entry.appendChild (node.createCopy(), nullptr);
                continue;
            }

            if (! isFlatNode (type))
                continue;   // an unknown node is not show state

            juce::ValueTree copy (type);

            if (type == Channel && node.hasProperty (effectName))
                copy.setProperty (effectName, node.getProperty (effectName), nullptr);

            for (const auto& item : table().items)
            {
                if (item.nodeType.isValid() || ! matrix.isIncluded (item.itemId, fx))
                    continue;

                for (const auto& p : item.parameterIds)
                    if (node.hasProperty (p))
                        copy.setProperty (p, node.getProperty (p), nullptr);
            }

            if (copy.getNumProperties() > 0)
                entry.appendChild (copy, nullptr);
        }

        return entry;
    }

    /** Copy onto `target` every property of `source` that `target` already has
        (never `id`), then recurse into children matched by type AND id - or, for
        an id-less child, by type and ordinal among its id-less siblings. Never
        adds or removes a child. Returns the number of properties written. */
    inline int applyNodeRecursive (juce::ValueTree target, const juce::ValueTree& source, juce::UndoManager* um)
    {
        using namespace WFSParameterIDs;
        int written = 0;

        for (int p = 0; p < source.getNumProperties(); ++p)
        {
            const auto property = source.getPropertyName (p);
            if (property == id || ! target.hasProperty (property))
                continue;

            target.setProperty (property, source.getProperty (property), um);
            ++written;
        }

        for (int c = 0; c < source.getNumChildren(); ++c)
        {
            const auto child = source.getChild (c);
            juce::ValueTree match;

            if (child.hasProperty (id))
            {
                for (int t = 0; t < target.getNumChildren() && ! match.isValid(); ++t)
                {
                    auto candidate = target.getChild (t);
                    if (candidate.hasType (child.getType())
                        && candidate.getProperty (id).toString() == child.getProperty (id).toString())
                        match = candidate;
                }
            }
            else
            {
                int ordinal = 0;
                for (int s = 0; s < c; ++s)
                    if (source.getChild (s).hasType (child.getType()) && ! source.getChild (s).hasProperty (id))
                        ++ordinal;

                for (int t = 0; t < target.getNumChildren() && ! match.isValid(); ++t)
                {
                    auto candidate = target.getChild (t);
                    if (candidate.hasType (child.getType()) && ! candidate.hasProperty (id) && ordinal-- == 0)
                        match = candidate;
                }
            }

            if (match.isValid())
                written += applyNodeRecursive (match, child, um);
        }

        return written;
    }

    /** Apply a snapshot entry to the live channel `fx` over `matrix`. Only what the
        entry carries AND the live node has is written; returns how many values. */
    inline int applyEffect (WFSValueTreeState& vts, int fx, const juce::ValueTree& entry,
                            const ScopeMatrix& matrix, juce::UndoManager* um)
    {
        using namespace WFSParameterIDs;

        auto effect = vts.getEffectState (fx);
        if (! effect.isValid())
            return 0;

        int written = 0;

        for (int i = 0; i < entry.getNumChildren(); ++i)
        {
            const auto source = entry.getChild (i);
            const auto type = source.getType();

            if (const auto module = moduleItemId (type); module.isNotEmpty())
            {
                if (! matrix.isIncluded (module, fx))
                    continue;

                auto target = effect.getChildWithName (type);
                if (target.isValid())
                    written += applyNodeRecursive (target, source, um);
                continue;
            }

            if (! isFlatNode (type))
                continue;

            auto target = effect.getChildWithName (type);
            if (! target.isValid())
                continue;

            if (type == Channel && source.hasProperty (effectName) && target.hasProperty (effectName))
            {
                target.setProperty (effectName, source.getProperty (effectName), um);
                ++written;
            }

            for (const auto& item : table().items)
            {
                if (item.nodeType.isValid() || ! matrix.isIncluded (item.itemId, fx))
                    continue;

                for (const auto& p : item.parameterIds)
                {
                    if (! source.hasProperty (p) || ! target.hasProperty (p))
                        continue;

                    // setEffectParameter is void and silently writes nothing when
                    // no child has the property - hence the hasProperty test above.
                    if (isPackedRow (p))
                        vts.setEffectParameter (fx, p, source.getProperty (p));
                    else
                        target.setProperty (p, source.getProperty (p), um);

                    ++written;
                }
            }
        }

        return written;
    }

    /** Remove what `matrix` excludes for live channel `fx` from a STORED entry, in
        place: excluded property items leave their flat node (effectName stays),
        excluded modules leave whole, and a flat node left empty is dropped.
        Removal only - it never adds data. */
    inline void trimEffectToScope (juce::ValueTree entry, const ScopeMatrix& matrix, int fx)
    {
        using namespace WFSParameterIDs;

        for (int i = entry.getNumChildren(); --i >= 0;)
        {
            auto node = entry.getChild (i);
            const auto type = node.getType();

            if (const auto module = moduleItemId (type); module.isNotEmpty())
            {
                if (! matrix.isIncluded (module, fx))
                    entry.removeChild (i, nullptr);
                continue;
            }

            if (! isFlatNode (type))
                continue;

            for (const auto& item : table().items)
                if (! item.nodeType.isValid() && ! matrix.isIncluded (item.itemId, fx))
                    for (const auto& p : item.parameterIds)
                        node.removeProperty (p, nullptr);

            if (node.getNumProperties() == 0 && node.getNumChildren() == 0)
                entry.removeChild (i, nullptr);
        }
    }
}
