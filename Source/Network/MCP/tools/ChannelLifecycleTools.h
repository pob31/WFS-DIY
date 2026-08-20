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

/** Schema for the create tools: optional count (default 1, capped at hardMax),
    for inputs an optional mono/stereo type, plus the tier-2 confirm token. */
inline juce::var createSchema (int hardMax, bool withInputType)
{
    auto props = std::make_unique<juce::DynamicObject>();

    auto count = std::make_unique<juce::DynamicObject>();
    count->setProperty ("type",    "integer");
    count->setProperty ("minimum", 1);
    count->setProperty ("maximum", hardMax);
    count->setProperty ("default", 1);
    count->setProperty ("description",
        withInputType
            ? juce::String ("How many channels to create in one call. Default 1. New "
                            "channels are APPENDED after the last channel with permanent "
                            "numbers highest+1..highest+count — numbers of existing "
                            "channels never shift, and deleted numbers are NOT reused. "
                            "Refused with at_capacity when the live count or the number "
                            "space would exceed 64 (or the stereo budget of 8).")
            : juce::String ("How many channels to create in one call. Default 1. The new "
                            "channels are appended to the end (ids = currentCount+1 .. "
                            "currentCount+count). Refused with at_capacity if "
                            "currentCount + count > hardMax. One tier-2 handshake covers "
                            "the whole batch."));
    props->setProperty ("count", juce::var (count.release()));

    if (withInputType)
    {
        auto type = std::make_unique<juce::DynamicObject>();
        type->setProperty ("type", "string");
        juce::Array<juce::var> values { juce::var ("mono"), juce::var ("stereo") };
        type->setProperty ("enum", juce::var (values));
        type->setProperty ("default", "mono");
        type->setProperty ("description",
            "Channel type for every channel created by this call. A stereo "
            "channel keeps ONE channel number and claims two hardware inputs "
            "(L/R) in the patch. At most 8 stereo channels.");
        props->setProperty ("type", juce::var (type.release()));
    }

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

/** Schema for input_delete: optional input_id (default: highest number). */
inline juce::var inputDeleteSchema()
{
    auto props = std::make_unique<juce::DynamicObject>();

    auto inputId = std::make_unique<juce::DynamicObject>();
    inputId->setProperty ("type",    "integer");
    inputId->setProperty ("minimum", 1);
    inputId->setProperty ("maximum", WFSParameterDefaults::maxInputChannels);
    inputId->setProperty ("description",
        "Permanent number of the LIVE channel to remove. Omitted: the "
        "highest-numbered channel. The number is retired (permanent gap); "
        "every other channel keeps its number, slot and patch columns.");
    props->setProperty ("input_id", juce::var (inputId.release()));

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
                            bool stereoType,
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

    // Inputs: structural ops with permanent numbers (append-only; mono and
    // stereo interleave). The created ids are the channels' NUMBERS.
    juce::Array<juce::var> createdNumbers;
    if (cfg.kindLabel == "input")
    {
        for (int i = 0; i < count; ++i)
        {
            auto res = state.addInputChannel (stereoType);
            if (res.failed())
            {
                if (createdNumbers.isEmpty())
                    return ToolResult::error ("at_capacity", res.getErrorMessage());
                break;  // partial batch: report what was created
            }
            createdNumbers.add (state.getHighestChannelNumber());
        }

        // The numbers about to leave in created_channel_ids are what an
        // external agent will quote in every later call, so they stop being
        // reflowable the moment they are handed out: without this, a human
        // reordering the list afterwards renumbers the very channels the
        // client is holding. This is the non-obvious trigger — creating a
        // channel does not itself read a number, but publishing one commits
        // to it. Output and reverb ids are dense slot positions and stay
        // outside this branch on purpose.
        state.markChannelNumbersUserOwned();
    }
    else
    {
        state.setParameter (cfg.countParamId, juce::var (currentCount + count), -1);
        for (int i = 0; i < count; ++i)
            createdNumbers.add (currentCount + 1 + i);
    }

    const int created  = createdNumbers.size();
    const int newCount = currentCount + created;

    const int firstId = created > 0 ? static_cast<int> (createdNumbers.getFirst()) : 0;
    const int lastId  = created > 0 ? static_cast<int> (createdNumbers.getLast())  : 0;

    if (record != nullptr)
    {
        record->affectedParameters.add (cfg.countParamId.toString());
        record->affectedGroups.push_back ({ 0, juce::String ("I/O") });
        // One (channelId, "Channel") entry per newly created channel so
        // later per-channel writes by the same actor chain in undo.
        for (const auto& idVar : createdNumbers)
            record->affectedGroups.push_back ({ static_cast<int> (idVar),
                                                juce::String ("Channel") });

        auto before = std::make_unique<juce::DynamicObject>();
        before->setProperty (cfg.countParamId, currentCount);
        record->beforeState = juce::var (before.release());

        auto after = std::make_unique<juce::DynamicObject>();
        after->setProperty (cfg.countParamId, newCount);
        record->afterState = juce::var (after.release());

        const juce::String typeSuffix = cfg.kindLabel == "input"
            ? juce::String (stereoType ? " (stereo)" : " (mono)") : juce::String();
        if (created == 1)
        {
            record->operatorDescription = "Created " + cfg.kindLabel + " channel "
                                           + juce::String (lastId) + typeSuffix
                                           + " (" + cfg.kindLabel + " count "
                                           + juce::String (currentCount) + " -> "
                                           + juce::String (newCount) + ")";
        }
        else
        {
            record->operatorDescription = "Created " + cfg.kindLabel + " channels "
                                           + juce::String (firstId) + ".."
                                           + juce::String (lastId) + typeSuffix
                                           + " (" + cfg.kindLabel + " count "
                                           + juce::String (currentCount) + " -> "
                                           + juce::String (newCount) + ")";
        }
    }

    auto result = std::make_unique<juce::DynamicObject>();
    result->setProperty ("created_channel_ids", juce::var (createdNumbers));
    result->setProperty ("first_channel_id",    firstId);
    result->setProperty ("last_channel_id",     lastId);
    result->setProperty ("created_count",       created);
    result->setProperty ("total",               newCount);
    result->setProperty ("kind",                cfg.kindLabel);
    // Back-compat alias: callers that read `channel_id` (singular) still get
    // the last-created id. Deprecated in favor of `created_channel_ids`.
    result->setProperty ("channel_id",          lastId);
    return ToolResult::ok (juce::var (result.release()));
}

inline ToolResult del (WFSValueTreeState& state,
                         const ChannelKindConfig& cfg,
                         int requestedNumber,   // inputs only; <= 0 = highest
                         ChangeRecord* record)
{
    const int currentCount = static_cast<int> (state.getParameter (cfg.countParamId, -1));
    if (currentCount <= 0)
        return ToolResult::error ("empty",
            cfg.kindLabel + " channel count is already 0.");

    const int newCount = currentCount - 1;
    int deletedId = currentCount;  // dense kinds drop the highest-numbered channel

    if (cfg.kindLabel == "input")
    {
        // Freeze BEFORE the removal, not after: while unlatched
        // removeInputChannel reflows every number to slot+1, which would
        // renumber the channels the client still holds and break this tool's
        // own promise that the deleted number is retired as a permanent gap
        // while everything else keeps its number, slot and patch columns.
        // Output and reverb deletes are dense-count decrements and must not
        // freeze input numbering.
        state.markChannelNumbersUserOwned();

        // Delete by permanent number; the number is retired (gap), everything
        // else keeps its number, slot and patch columns.
        deletedId = requestedNumber > 0 ? requestedNumber
                                        : state.getHighestChannelNumber();
        auto res = state.removeInputChannel (deletedId);
        if (res.failed())
            return ToolResult::error ("invalid_args", res.getErrorMessage());
    }
    else
    {
        state.setParameter (cfg.countParamId, juce::var (newCount), -1);
    }

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

inline void notifyTopology (const std::function<void()>* topologyChanged)
{
    if (topologyChanged != nullptr && *topologyChanged)
        (*topologyChanged)();
}

inline ToolDescriptor describeCreate (WFSValueTreeState& state,
                                        const juce::String& kindLabel,
                                        const std::function<void()>* topologyChanged)
{
    const auto& cfg = configFor (kindLabel);
    const bool isInput = kindLabel == "input";
    ToolDescriptor d;
    d.name        = kindLabel + "_create";
    d.description = isInput
                  ? juce::String ("Add one or more input channels (optional type: mono/stereo, "
                    "default mono). Channels are APPENDED after the last channel with "
                    "permanent numbers highest+1..: numbers of existing channels never "
                    "shift, and mono/stereo channels may interleave. Returns "
                    "`created_channel_ids` (permanent numbers), `first_channel_id`, "
                    "`last_channel_id`, `created_count`, `total`, `kind` (+ deprecated "
                    "`channel_id` alias). Refused with at_capacity at 64 live channels, "
                    "number-space exhaustion, or the 8-stereo budget. Triggers a DSP "
                    "restart, so plan accordingly.")
                  + juce::String (kTier2DescriptionSuffix)
                  : "Add one or more " + kindLabel + " channels by bumping "
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
    d.inputSchema   = createSchema (cfg.hardMax, isInput);
    d.modifiesState = true;
    d.tier        = 2;
    d.handler = [&state, &cfg, topologyChanged] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        int count = 1;
        bool stereoType = false;
        if (args.isObject())
        {
            if (auto* obj = args.getDynamicObject())
            {
                if (obj->hasProperty ("count"))
                    count = static_cast<int> (obj->getProperty ("count"));
                if (obj->hasProperty ("type"))
                    stereoType = obj->getProperty ("type").toString() == "stereo";
            }
        }
        auto result = create (state, cfg, count, stereoType, record);
        if (result.success)
            notifyTopology (topologyChanged);
        return result;
    };
    return d;
}

inline ToolDescriptor describeDelete (WFSValueTreeState& state,
                                        const juce::String& kindLabel,
                                        const std::function<void()>* topologyChanged)
{
    const auto& cfg = configFor (kindLabel);
    const bool isInput = kindLabel == "input";
    ToolDescriptor d;
    d.name        = kindLabel + "_delete";
    d.description = isInput
                  ? juce::String ("Remove a LIVE input channel by its permanent number "
                    "(input_id; omitted = highest number). The number is RETIRED — a "
                    "permanent gap: every other channel keeps its number, slot and "
                    "patch columns, so snapshots, QLab cues and DAW mappings stay "
                    "valid. The deleted channel's parameters are dropped. Triggers a "
                    "DSP restart.")
                  + juce::String (kTier2DescriptionSuffix)
                  : "Remove the highest-numbered " + kindLabel + " channel by "
                    "decrementing " + cfg.countParamId.toString() + " by 1. "
                    "The deleted channel's parameters are dropped. Refused "
                    "with `empty` when the count is already 0. Triggers a DSP "
                    "restart."
                  + juce::String (kTier2DescriptionSuffix);
    d.inputSchema   = isInput ? inputDeleteSchema() : emptyObjectSchema();
    d.modifiesState = true;
    d.tier        = 2;
    d.handler = [&state, &cfg, topologyChanged] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        int requestedNumber = 0;
        if (args.isObject())
            if (auto* obj = args.getDynamicObject())
                if (obj->hasProperty ("input_id"))
                    requestedNumber = static_cast<int> (obj->getProperty ("input_id"));
        auto result = del (state, cfg, requestedNumber, record);
        if (result.success)
            notifyTopology (topologyChanged);
        return result;
    };
    return d;
}

} // namespace WFSNetwork::Tools::ChannelLifecycle
