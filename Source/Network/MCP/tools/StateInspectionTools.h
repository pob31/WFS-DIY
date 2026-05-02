#pragma once

#include <JuceHeader.h>
#include "../MCPToolRegistry.h"
#include "../../../Parameters/WFSValueTreeState.h"

namespace WFSNetwork::Tools::StateInspection
{

namespace detail
{
    /** Append every property on `tree` (and any descendant subtrees) into
        `out`, using the property identifier as the JSON key. Property
        names are unique across the WFS-DIY ValueTree by construction —
        the subtree shape exists only for code organisation, not for
        namespacing — so flattening loses no information and gives the AI
        a flat dictionary that mirrors the param-IDs in WFSParameterIDs.h.

        Sections that are themselves arrays (e.g. EQ bands as
        `OutputEQBand` children of `OutputEQ`) get expanded into a nested
        `juce::Array<juce::var>` keyed by the section name. */
    inline void flattenProperties (const juce::ValueTree& tree,
                                    juce::DynamicObject& out);

    inline juce::var subtreeToVar (const juce::ValueTree& tree)
    {
        auto obj = std::make_unique<juce::DynamicObject>();
        flattenProperties (tree, *obj);
        return juce::var (obj.release());
    }

    inline void flattenProperties (const juce::ValueTree& tree,
                                    juce::DynamicObject& out)
    {
        if (! tree.isValid()) return;

        for (int i = 0; i < tree.getNumProperties(); ++i)
        {
            const auto name = tree.getPropertyName (i);
            out.setProperty (name, tree.getProperty (name));
        }

        // Group siblings of the same type into an array (e.g. multiple
        // OutputEQBand children become a single `OutputEQBand: [...]`
        // entry). Single-child sections become an object value.
        std::map<juce::String, juce::Array<juce::var>> grouped;
        for (int i = 0; i < tree.getNumChildren(); ++i)
        {
            const auto child = tree.getChild (i);
            const auto type = child.getType().toString();
            grouped[type].add (subtreeToVar (child));
        }
        for (auto& [type, arr] : grouped)
        {
            if (arr.size() == 1)
                out.setProperty (type, arr[0]);
            else
                out.setProperty (type, juce::var (arr));
        }
    }
} // namespace detail

//==============================================================================
// session_get_global_state — non-channel parameters
//==============================================================================

/** All section keys this tool knows about. The first 7 are the
    "everyday" defaults; clusters and admosc are off by default because
    they expand into ~10 cluster x ~25 LFO fields and 4 mapping x 3 axis
    x 9 field tables respectively (the verbosity the testing AI flagged).
    Pass them explicitly via `sections` to opt in. */
static const juce::StringArray& kGlobalStateAllSections()
{
    static const juce::StringArray all {
        "stage", "master", "io", "show", "binaural",
        "network", "tracking",
        "admosc", "clusters"
    };
    return all;
}

static const juce::StringArray& kGlobalStateDefaultSections()
{
    static const juce::StringArray def {
        "stage", "master", "io", "show", "binaural",
        "network", "tracking"
    };
    return def;
}

inline juce::var globalStateSchema()
{
    auto sectionsArg = std::make_unique<juce::DynamicObject>();
    sectionsArg->setProperty ("type", "array");
    auto items = std::make_unique<juce::DynamicObject>();
    items->setProperty ("type", "string");
    juce::Array<juce::var> enumArr;
    for (const auto& s : kGlobalStateAllSections())
        enumArr.add (s);
    items->setProperty ("enum", juce::var (enumArr));
    sectionsArg->setProperty ("items", juce::var (items.release()));
    sectionsArg->setProperty ("description",
        "Optional list of section keys to include. Default is the everyday "
        "set (stage, master, io, show, binaural, network, tracking). Pass "
        "`admosc` to add the 4+4 ADM-OSC mapping tables and `clusters` to "
        "add the per-cluster LFO state - both are voluminous and excluded "
        "by default. Channel counts are always included.");

    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("sections", juce::var (sectionsArg.release()));

    auto schema = std::make_unique<juce::DynamicObject>();
    schema->setProperty ("type", "object");
    schema->setProperty ("properties", juce::var (props.release()));
    schema->setProperty ("additionalProperties", false);
    return juce::var (schema.release());
}

inline ToolResult getGlobalState (WFSValueTreeState& state, const juce::var& args)
{
    // Resolve which sections to include. Empty/missing -> default set.
    juce::StringArray wanted;
    if (auto* obj = args.getDynamicObject())
    {
        const auto sectionsVar = obj->getProperty ("sections");
        if (sectionsVar.isArray())
        {
            for (const auto& s : *sectionsVar.getArray())
            {
                const auto key = s.toString();
                if (kGlobalStateAllSections().contains (key))
                    wanted.addIfNotAlreadyThere (key);
            }
        }
    }
    if (wanted.isEmpty())
        wanted = kGlobalStateDefaultSections();

    auto root = std::make_unique<juce::DynamicObject>();

    auto addIfWanted = [&] (const juce::String& key, juce::ValueTree section)
    {
        if (! wanted.contains (key)) return;
        if (section.isValid())
            root->setProperty (key, detail::subtreeToVar (section));
    };

    addIfWanted ("stage",    state.getStageState());
    addIfWanted ("master",   state.getMasterState());
    addIfWanted ("io",       state.getIOState());
    addIfWanted ("show",     state.getShowState());
    addIfWanted ("binaural", state.getBinauralState());
    addIfWanted ("network",  state.getNetworkState());
    addIfWanted ("tracking", state.getTrackingState());
    addIfWanted ("admosc",   state.getADMOSCState());
    addIfWanted ("clusters", state.getClustersState());

    auto counts = std::make_unique<juce::DynamicObject>();
    counts->setProperty ("inputs",   state.getNumInputChannels());
    counts->setProperty ("outputs",  state.getNumOutputChannels());
    counts->setProperty ("reverbs",  state.getNumReverbChannels());
    root->setProperty ("channel_counts", juce::var (counts.release()));

    juce::Array<juce::var> includedArr;
    for (const auto& s : wanted) includedArr.add (s);
    root->setProperty ("included_sections", juce::var (includedArr));

    return ToolResult::ok (juce::var (root.release()));
}

inline ToolDescriptor describeGlobalState (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "session_get_global_state";
    d.description = "Read-only dump of NON-channel parameters: stage shape and "
                    "dimensions, origin offsets (originWidth/Depth/Height), "
                    "master level / system latency / Haas, IO + show metadata, "
                    "binaural settings, network globals, tracking globals. "
                    "Defaults to the everyday set; opt in to ADM-OSC mappings "
                    "or per-cluster LFO state via the `sections` arg "
                    "(verbose). Channel counts are always included. "
                    "Complements session_get_state (per-channel positions) and "
                    "session_get_channel_full (one channel, every parameter).";
    d.inputSchema   = globalStateSchema();
    d.modifiesState = false;
    d.tier        = 1;
    d.handler = [&state] (const juce::var& args, ChangeRecord*) -> ToolResult
    {
        return getGlobalState (state, args);
    };
    return d;
}

//==============================================================================
// session_get_channel_full — every parameter for one channel
//==============================================================================

inline juce::var channelFullSchema()
{
    auto channelType = std::make_unique<juce::DynamicObject>();
    channelType->setProperty ("type", "string");
    juce::Array<juce::var> typeEnum;
    typeEnum.add ("input");
    typeEnum.add ("output");
    typeEnum.add ("reverb");
    channelType->setProperty ("enum", typeEnum);
    channelType->setProperty ("description",
        "Channel kind: input / output / reverb. Cluster channels live in "
        "session_get_global_state under the cluster section.");

    auto channelId = std::make_unique<juce::DynamicObject>();
    channelId->setProperty ("type", "integer");
    channelId->setProperty ("minimum", 1);
    channelId->setProperty ("description",
        "Channel number (1-based). Use session_get_state to discover ranges.");

    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("channel_type", juce::var (channelType.release()));
    props->setProperty ("channel_id",   juce::var (channelId.release()));

    auto required = juce::Array<juce::var>();
    required.add ("channel_type");
    required.add ("channel_id");

    auto schema = std::make_unique<juce::DynamicObject>();
    schema->setProperty ("type", "object");
    schema->setProperty ("properties", juce::var (props.release()));
    schema->setProperty ("required",   juce::var (required));
    schema->setProperty ("additionalProperties", false);
    return juce::var (schema.release());
}

inline ToolResult getChannelFull (WFSValueTreeState& state, const juce::var& args)
{
    if (! args.isObject())
        return ToolResult::error ("invalid_args", "Arguments must be a JSON object");

    auto* obj = args.getDynamicObject();
    const juce::String channelType = obj->getProperty ("channel_type").toString();
    if (! obj->hasProperty ("channel_id"))
        return ToolResult::error ("invalid_args", "Missing required arg: channel_id");
    const int displayId = static_cast<int> (obj->getProperty ("channel_id"));
    const int channelIndex = displayId - 1;
    if (channelIndex < 0)
        return ToolResult::error ("invalid_args", "channel_id must be >= 1");

    juce::ValueTree section;
    int maxIndex = 0;
    if (channelType == "input")
    {
        maxIndex = state.getNumInputChannels();
        section  = state.getInputState (channelIndex);
    }
    else if (channelType == "output")
    {
        maxIndex = state.getNumOutputChannels();
        section  = state.getOutputState (channelIndex);
    }
    else if (channelType == "reverb")
    {
        maxIndex = state.getNumReverbChannels();
        section  = state.getReverbState (channelIndex);
    }
    else
    {
        return ToolResult::error ("invalid_args",
            "channel_type must be one of: input, output, reverb");
    }

    if (channelIndex >= maxIndex || ! section.isValid())
        return ToolResult::error ("invalid_args",
            "channel_id " + juce::String (displayId) + " out of range for "
            + channelType + " (1.." + juce::String (maxIndex) + ")");

    auto root = std::make_unique<juce::DynamicObject>();
    root->setProperty ("channel_type", channelType);
    root->setProperty ("channel_id",   displayId);
    root->setProperty ("parameters",   detail::subtreeToVar (section));
    return ToolResult::ok (juce::var (root.release()));
}

inline ToolDescriptor describeChannelFull (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "session_get_channel_full";
    d.description = "Read-only dump of every parameter on one channel - every "
                    "property in the channel's ValueTree subtree, including "
                    "child sections (Position / Attenuation / Directivity / "
                    "LiveSource / LFO / AutomOtion / Mutes / Sampler for inputs; "
                    "Position / Options / OutputEQBand[*] for outputs; etc). "
                    "Use this to verify what was actually written, or to "
                    "snapshot a channel's state before bulk edits.";
    d.inputSchema   = channelFullSchema();
    d.modifiesState = false;
    d.tier        = 1;
    d.handler = [&state] (const juce::var& args, ChangeRecord*) -> ToolResult
    {
        return getChannelFull (state, args);
    };
    return d;
}

} // namespace WFSNetwork::Tools::StateInspection
