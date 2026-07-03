#pragma once

#include <JuceHeader.h>
#include "../MCPCompat.h"
#include "../../../Parameters/WFSValueTreeState.h"
#include "../../../Parameters/WFSParameterIDs.h"

namespace WFSNetwork::Tools::ChannelLifecycle
{

/** Per-channel-kind config. Mirrors the OSC bounds + the visible names
    the rest of the system uses, so tier-2 wrappers stay in sync with
    the underlying tier-3 channel-count parameters. */
struct ChannelKindConfig
{
    juce::String        kindLabel;       // "input" / "output" / "reverb"
    juce::Identifier    countParamId;    // WFSParameterIDs::*Channels
    int                 hardMax;         // per-kind max channel count (from WFSParameterDefaults)
};

inline const ChannelKindConfig& configFor (const juce::String& kind)
{
    static const ChannelKindConfig inputCfg  { "input",  WFSParameterIDs::inputChannels,  WFSParameterDefaults::maxInputChannels };
    static const ChannelKindConfig outputCfg { "output", WFSParameterIDs::outputChannels, WFSParameterDefaults::maxOutputChannels };
    static const ChannelKindConfig reverbCfg { "reverb", WFSParameterIDs::reverbChannels, WFSParameterDefaults::maxReverbChannels };
    if (kind == "input")  return inputCfg;
    if (kind == "output") return outputCfg;
    return reverbCfg;
}

/** Schema for the delete tools: just an optional confirm token. */
inline juce::var emptyObjectSchema()
{
    auto props = std::make_unique<juce::DynamicObject>();

    auto confirm = std::make_unique<juce::DynamicObject>();
    confirm->setProperty ("type", "string");
    confirm->setProperty ("description",
        "Confirmation token from the previous call's tier_enforcement "
        "envelope. Re-call with confirm set to this token within 30s. "
        "Tier-2 auto-confirm (Network tab) skips the handshake.");
    props->setProperty ("confirm", juce::var (confirm.release()));

    auto schema = std::make_unique<juce::DynamicObject>();
    schema->setProperty ("type", "object");
    schema->setProperty ("properties", juce::var (props.release()));
    schema->setProperty ("additionalProperties", false);
    return juce::var (schema.release());
}

/** Schema for the create tools: optional count (default 1, capped at hardMax)
    plus the usual tier-2 confirm token. */
inline juce::var createSchema (int hardMax)
{
    auto props = std::make_unique<juce::DynamicObject>();

    auto count = std::make_unique<juce::DynamicObject>();
    count->setProperty ("type",    "integer");
    count->setProperty ("minimum", 1);
    count->setProperty ("maximum", hardMax);
    count->setProperty ("default", 1);
    count->setProperty ("description",
        "How many channels to create in one call. Default 1. The new "
        "channels are appended to the end (ids = currentCount+1 .. "
        "currentCount+count). Refused with at_capacity if "
        "currentCount + count > hardMax (32 reverbs / 64 inputs / 128 outputs). "
        "One tier-2 handshake covers the whole batch; the result is a single "
        "undoable entry.");
    props->setProperty ("count", juce::var (count.release()));

    auto confirm = std::make_unique<juce::DynamicObject>();
    confirm->setProperty ("type", "string");
    confirm->setProperty ("description",
        "Confirmation token from the previous call's tier_enforcement "
        "envelope. Re-call with confirm set to this token within 30s. "
        "Tier-2 auto-confirm (Network tab) skips the handshake.");
    props->setProperty ("confirm", juce::var (confirm.release()));

    auto schema = std::make_unique<juce::DynamicObject>();
    schema->setProperty ("type", "object");
    schema->setProperty ("properties", juce::var (props.release()));
    schema->setProperty ("additionalProperties", false);
    return juce::var (schema.release());
}

inline ToolResult create (WFSValueTreeState& state,
                            const ChannelKindConfig& cfg,
                            int count,
                            ChangeRecord* record)
{
    if (count < 1)
        return ToolResult::error ("invalid_args",
            "count must be >= 1 (got " + juce::String (count) + ").");

    const int currentCount = static_cast<int> (state.getParameter (cfg.countParamId, -1));
    if (currentCount + count > cfg.hardMax)
        return ToolResult::error ("at_capacity",
            cfg.kindLabel + " count " + juce::String (currentCount)
            + " + requested " + juce::String (count)
            + " would exceed hard max " + juce::String (cfg.hardMax) + ".");

    const int newCount = currentCount + count;
    state.setParameter (cfg.countParamId, juce::var (newCount), -1);

    if (record != nullptr)
    {
        record->affectedParameters.add (cfg.countParamId.toString());
        record->affectedGroups.push_back ({ 0, juce::String ("I/O") });
        // One (channelId, "Channel") entry per newly created channel so
        // later per-channel writes by the same actor chain in undo.
        for (int i = 0; i < count; ++i)
            record->affectedGroups.push_back ({ currentCount + 1 + i,
                                                juce::String ("Channel") });

        auto before = std::make_unique<juce::DynamicObject>();
        before->setProperty (cfg.countParamId, currentCount);
        record->beforeState = juce::var (before.release());

        auto after = std::make_unique<juce::DynamicObject>();
        after->setProperty (cfg.countParamId, newCount);
        record->afterState = juce::var (after.release());

        if (count == 1)
        {
            record->operatorDescription = "Created " + cfg.kindLabel + " channel "
                                           + juce::String (newCount)
                                           + " (" + cfg.kindLabel + " count "
                                           + juce::String (currentCount) + " -> "
                                           + juce::String (newCount) + ")";
        }
        else
        {
            record->operatorDescription = "Created " + cfg.kindLabel + " channels "
                                           + juce::String (currentCount + 1) + ".."
                                           + juce::String (newCount)
                                           + " (" + cfg.kindLabel + " count "
                                           + juce::String (currentCount) + " -> "
                                           + juce::String (newCount) + ")";
        }
    }

    juce::Array<juce::var> idsArr;
    for (int i = 0; i < count; ++i)
        idsArr.add (currentCount + 1 + i);

    auto result = std::make_unique<juce::DynamicObject>();
    result->setProperty ("created_channel_ids", juce::var (idsArr));
    result->setProperty ("first_channel_id",    currentCount + 1);
    result->setProperty ("last_channel_id",     newCount);
    result->setProperty ("created_count",       count);
    result->setProperty ("total",               newCount);
    result->setProperty ("kind",                cfg.kindLabel);
    // Back-compat alias: callers that read `channel_id` (singular) still get
    // the last-created id. Deprecated in favor of `created_channel_ids`.
    result->setProperty ("channel_id",          newCount);
    return ToolResult::ok (juce::var (result.release()));
}

inline ToolResult del (WFSValueTreeState& state,
                         const ChannelKindConfig& cfg,
                         ChangeRecord* record)
{
    const int currentCount = static_cast<int> (state.getParameter (cfg.countParamId, -1));
    if (currentCount <= 0)
        return ToolResult::error ("empty",
            cfg.kindLabel + " channel count is already 0.");

    const int newCount = currentCount - 1;
    const int deletedId = currentCount;  // we drop the highest-numbered channel
    state.setParameter (cfg.countParamId, juce::var (newCount), -1);

    if (record != nullptr)
    {
        record->affectedParameters.add (cfg.countParamId.toString());
        record->affectedGroups.push_back ({ 0, juce::String ("I/O") });

        auto before = std::make_unique<juce::DynamicObject>();
        before->setProperty (cfg.countParamId, currentCount);
        record->beforeState = juce::var (before.release());

        auto after = std::make_unique<juce::DynamicObject>();
        after->setProperty (cfg.countParamId, newCount);
        record->afterState = juce::var (after.release());

        record->operatorDescription = "Deleted " + cfg.kindLabel + " channel "
                                       + juce::String (deletedId)
                                       + " (" + cfg.kindLabel + " count "
                                       + juce::String (currentCount) + " -> "
                                       + juce::String (newCount) + ")";
    }

    auto result = std::make_unique<juce::DynamicObject>();
    result->setProperty ("deleted_channel_id", deletedId);
    result->setProperty ("total",              newCount);
    result->setProperty ("kind",               cfg.kindLabel);
    return ToolResult::ok (juce::var (result.release()));
}

inline ToolDescriptor describeCreate (WFSValueTreeState& state,
                                        const juce::String& kindLabel)
{
    const auto& cfg = configFor (kindLabel);
    ToolDescriptor d;
    d.name        = kindLabel + "_create";
    d.description = "Add one or more " + kindLabel + " channels by bumping "
                    + cfg.countParamId.toString() + ". With the default count=1 "
                    "this creates a single channel; pass count=N (up to "
                    + juce::String (cfg.hardMax) + ") to create N at once with "
                    "a single tier-2 handshake and a single undoable entry. "
                    "Returns `created_channel_ids` (array of new 1-based ids), "
                    "`first_channel_id`, `last_channel_id`, `created_count`, "
                    "`total`, `kind`. Also returns `channel_id` (deprecated "
                    "alias for last_channel_id) for back-compat. Refused with "
                    "at_capacity when currentCount + count > hardMax ("
                    + juce::String (cfg.hardMax) + "). Triggers a DSP restart, "
                    "so plan accordingly."
                  + juce::String (kTier2DescriptionSuffix);
    d.inputSchema   = createSchema (cfg.hardMax);
    d.modifiesState = true;
    d.tier        = 2;
    d.handler = [&state, &cfg] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        int count = 1;
        if (args.isObject())
        {
            if (auto* obj = args.getDynamicObject())
            {
                if (obj->hasProperty ("count"))
                    count = static_cast<int> (obj->getProperty ("count"));
            }
        }
        return create (state, cfg, count, record);
    };
    return d;
}

inline ToolDescriptor describeDelete (WFSValueTreeState& state,
                                        const juce::String& kindLabel)
{
    const auto& cfg = configFor (kindLabel);
    ToolDescriptor d;
    d.name        = kindLabel + "_delete";
    d.description = "Remove the highest-numbered " + kindLabel + " channel by "
                    "decrementing " + cfg.countParamId.toString() + " by 1. "
                    "The deleted channel's parameters are dropped. Refused "
                    "with `empty` when the count is already 0. Triggers a DSP "
                    "restart."
                  + juce::String (kTier2DescriptionSuffix);
    d.inputSchema   = emptyObjectSchema();
    d.modifiesState = true;
    d.tier        = 2;
    d.handler = [&state, &cfg] (const juce::var&, ChangeRecord* record) -> ToolResult
    {
        return del (state, cfg, record);
    };
    return d;
}

} // namespace WFSNetwork::Tools::ChannelLifecycle
