#pragma once

#include <JuceHeader.h>
#include "../MCPCompat.h"
#include "../../../Parameters/WFSValueTreeState.h"
#include "../../../Parameters/WFSParameterIDs.h"
#include "../../../Parameters/WFSParameterDefaults.h"
#include "ToolArgs.h"

/*
    The Clusters tab's 16 LFO preset slots over MCP.

    The generated cluster_lfo_presets_set_lfo_preset_name took cluster_id 1-10
    and used it as the PRESET slot (the resolver indexes the preset list with
    the channel index), so it claimed to name a cluster, named a preset, and
    could not reach slots 11-16. These address the slot by its own number.
*/
namespace WFSNetwork::Tools::ClusterPresets
{

inline juce::var presetArg()
{
    return Args::integerArg ("Preset slot, 1-16 (the Clusters tab's 4x4 tiles, read in rows).",
                             1, WFSParameterDefaults::maxClusterLFOPresets);
}

inline ToolDescriptor describeList (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "cluster_lfo_presets_list";
    d.description = "List the Clusters tab's 16 LFO preset slots with their names (empty = unnamed). "
                    "Read-only.";
    d.inputSchema   = Args::schemaOf (std::make_unique<juce::DynamicObject>(), {});
    d.modifiesState = false;
    d.tier          = 1;
    d.handler = [&state] (const juce::var&, ChangeRecord*) -> ToolResult
    {
        auto presets = state.getClusterLFOPresetsSection();
        juce::Array<juce::var> out;
        for (int i = 0; i < presets.getNumChildren(); ++i)
        {
            auto p = std::make_unique<juce::DynamicObject>();
            p->setProperty ("preset", i + 1);
            p->setProperty ("name", presets.getChild (i).getProperty (WFSParameterIDs::clusterLFOPresetName).toString());
            out.add (juce::var (p.release()));
        }
        auto result = std::make_unique<juce::DynamicObject>();
        result->setProperty ("presets", juce::var (out));
        return ToolResult::ok (juce::var (result.release()));
    };
    return d;
}

inline ToolDescriptor describeSetName (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "cluster_lfo_preset_set_name";
    d.description = "Rename one of the 16 cluster LFO preset slots, as clicking its tile's name does. "
                    "Not undoable: rename it back instead.";
    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("preset", presetArg());
    props->setProperty ("name", Args::stringArg ("The new name (empty clears it)."));
    d.inputSchema   = Args::schemaOf (std::move (props), { "preset", "name" });
    d.modifiesState = true;
    d.tier          = 1;
    d.handler = [&state] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        auto* obj = args.getDynamicObject();
        if (obj == nullptr || Args::missing (*obj, "preset") || ! obj->hasProperty ("name"))
            return ToolResult::error ("invalid_args", "Missing required arg: preset and name");
        juce::String code, message;
        const auto slot = Args::wholeNumber (obj->getProperty ("preset"), "preset", 1,
                                             WFSParameterDefaults::maxClusterLFOPresets, code, message);
        if (! slot.has_value())
            return ToolResult::error (code, message);

        auto preset = state.ensureClusterLFOPreset (*slot - 1);
        if (! preset.isValid())
            return ToolResult::error ("internal_error", "Preset slot " + juce::String (*slot) + " does not exist");

        const auto before = preset.getProperty (WFSParameterIDs::clusterLFOPresetName).toString();
        const auto name   = obj->getProperty ("name").toString();
        preset.setProperty (WFSParameterIDs::clusterLFOPresetName, name, nullptr);   // as the tile does

        if (record != nullptr)
        {
            record->operatorDescription = "Renamed cluster LFO preset " + juce::String (*slot) + " to \""
                                        + name + "\" (was \"" + before + "\"; not undoable)";
            record->affectedParameters.add (WFSParameterIDs::clusterLFOPresetName.toString());
            record->affectedGroups.push_back ({ *slot, juce::String ("LFO Presets") });
            record->undoable = false;
        }
        auto result = std::make_unique<juce::DynamicObject>();
        result->setProperty ("preset", *slot);
        result->setProperty ("name", name);
        return ToolResult::ok (juce::var (result.release()));
    };
    return d;
}

/** The 22 properties a preset holds - the same list, in the same order, as
    WFSValueTreeState::recallClusterLFOPreset and the tab's storePreset. */
inline const std::vector<juce::Identifier>& presetProperties()
{
    using namespace WFSParameterIDs;
    static const std::vector<juce::Identifier> ids {
        clusterLFOperiod, clusterLFOphase,
        clusterLFOshapeX, clusterLFOshapeY, clusterLFOshapeZ, clusterLFOshapeRot, clusterLFOshapeScale,
        clusterLFOrateX, clusterLFOrateY, clusterLFOrateZ, clusterLFOrateRot, clusterLFOrateScale,
        clusterLFOamplitudeX, clusterLFOamplitudeY, clusterLFOamplitudeZ, clusterLFOamplitudeRot, clusterLFOamplitudeScale,
        clusterLFOphaseX, clusterLFOphaseY, clusterLFOphaseZ, clusterLFOphaseRot, clusterLFOphaseScale
    };
    return ids;
}

inline ToolDescriptor describeGet (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "cluster_lfo_preset_get";
    d.description = "Read one cluster LFO preset slot: its name, whether it holds a movement (any axis "
                    "shape not Off), and its 22 values (period, phase, and shape / rate / amplitude / phase "
                    "for X, Y, Z, rotation and scale), named as the cluster_lfo_* parameters. Read-only. "
                    "Recalling a preset onto a cluster is the operator's (a timed action).";
    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("preset", presetArg());
    d.inputSchema   = Args::schemaOf (std::move (props), { "preset" });
    d.modifiesState = false;
    d.tier          = 1;
    d.handler = [&state] (const juce::var& args, ChangeRecord*) -> ToolResult
    {
        auto* obj = args.getDynamicObject();
        juce::String code, message;
        if (obj == nullptr || Args::missing (*obj, "preset"))
            return ToolResult::error ("invalid_args", "Missing required arg: preset");
        const auto slot = Args::wholeNumber (obj->getProperty ("preset"), "preset", 1,
                                             WFSParameterDefaults::maxClusterLFOPresets, code, message);
        if (! slot.has_value())
            return ToolResult::error (code, message);
        auto preset = state.ensureClusterLFOPreset (*slot - 1);
        if (! preset.isValid())
            return ToolResult::error ("internal_error", "Preset slot " + juce::String (*slot) + " does not exist");

        auto values = std::make_unique<juce::DynamicObject>();
        bool populated = false;
        for (const auto& id : presetProperties())
        {
            values->setProperty (id, preset.getProperty (id));
            if (id.toString().startsWith ("clusterLFOshape") && static_cast<int> (preset.getProperty (id)) != 0)
                populated = true;
        }
        auto result = std::make_unique<juce::DynamicObject>();
        result->setProperty ("preset", *slot);
        result->setProperty ("name", preset.getProperty (WFSParameterIDs::clusterLFOPresetName).toString());
        result->setProperty ("populated", populated);
        result->setProperty ("values", juce::var (values.release()));
        return ToolResult::ok (juce::var (result.release()));
    };
    return d;
}

inline ToolDescriptor describeStore (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "cluster_lfo_preset_store";
    d.description = juce::String (
        "Store one cluster's current LFO settings into a preset slot (1-16), as the Clusters tab's "
        "store on a tile; the slot's name is kept. Set the LFO first with the cluster_lfo_* parameters. "
        "Recalling it is the operator's. Not undoable: store again instead.")
        + juce::String (kTier2DescriptionSuffix);
    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("cluster_id", Args::integerArg ("Cluster whose LFO is stored (1-10).", 1, 10));
    props->setProperty ("preset", presetArg());
    props->setProperty ("confirm", Args::confirmArg());
    d.inputSchema   = Args::schemaOf (std::move (props), { "cluster_id", "preset" });
    d.modifiesState = true;
    d.tier          = 2;
    d.handler = [&state] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        auto* obj = args.getDynamicObject();
        juce::String code, message;
        if (obj == nullptr || Args::missing (*obj, "cluster_id") || Args::missing (*obj, "preset"))
            return ToolResult::error ("invalid_args", "Missing required arg: cluster_id and preset");
        const auto cluster = Args::wholeNumber (obj->getProperty ("cluster_id"), "cluster_id", 1, 10, code, message);
        if (! cluster.has_value()) return ToolResult::error (code, message);
        const auto slot = Args::wholeNumber (obj->getProperty ("preset"), "preset", 1,
                                             WFSParameterDefaults::maxClusterLFOPresets, code, message);
        if (! slot.has_value()) return ToolResult::error (code, message);

        auto lfo = state.getClusterLFOSection (*cluster);
        auto preset = state.ensureClusterLFOPreset (*slot - 1);
        if (! lfo.isValid() || ! preset.isValid())
            return ToolResult::error ("internal_error", "Cluster " + juce::String (*cluster) + " or preset "
                                                          + juce::String (*slot) + " has no state");
        for (const auto& id : presetProperties())
            preset.setProperty (id, lfo.getProperty (id), nullptr);   // as the tab's storePreset

        if (record != nullptr)
        {
            record->operatorDescription = "Stored cluster " + juce::String (*cluster) + "'s LFO into preset "
                                        + juce::String (*slot) + " (not undoable)";
            record->affectedParameters.add ("clusterLFOPreset");
            record->affectedGroups.push_back ({ *slot, juce::String ("LFO Presets") });
            record->undoable = false;
        }
        auto result = std::make_unique<juce::DynamicObject>();
        result->setProperty ("cluster_id", *cluster);
        result->setProperty ("preset", *slot);
        return ToolResult::ok (juce::var (result.release()));
    };
    return d;
}

} // namespace WFSNetwork::Tools::ClusterPresets
