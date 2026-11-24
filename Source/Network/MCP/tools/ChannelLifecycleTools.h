#pragma once

#include <JuceHeader.h>
#include "../MCPCompat.h"
#include "../../../Parameters/WFSValueTreeState.h"
#include "../../../Parameters/WFSParameterIDs.h"
#include "../../../Parameters/InputChannelDescription.h"
#include "ToolArgs.h"

namespace WFSNetwork::Tools::ChannelLifecycle
{

/** Per-channel-kind config. Mirrors the OSC bounds + the visible names
    the rest of the system uses, so tier-2 wrappers stay in sync with
    the underlying tier-3 channel-count parameters. */
struct ChannelKindConfig
{
    juce::String        kindLabel;       // "input" / "output" / "reverb" / "effect"
    juce::Identifier    countParamId;    // WFSParameterIDs::*Channels
    int                 hardMax;         // per-kind max channel count (from WFSParameterDefaults)
};

inline const ChannelKindConfig& configFor (const juce::String& kind)
{
    static const ChannelKindConfig inputCfg  { "input",  WFSParameterIDs::inputChannels,  WFSParameterDefaults::maxInputChannels };
    static const ChannelKindConfig outputCfg { "output", WFSParameterIDs::outputChannels, WFSParameterDefaults::maxOutputChannels };
    static const ChannelKindConfig reverbCfg { "reverb", WFSParameterIDs::reverbChannels, WFSParameterDefaults::maxReverbChannels };
    static const ChannelKindConfig effectCfg { "effect", WFSParameterIDs::effectChannels, WFSParameterDefaults::maxEffectChannels };
    if (kind == "input")  return inputCfg;
    if (kind == "output") return outputCfg;
    if (kind == "effect") return effectCfg;
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

/** A channel create, delete or recomposition is filed in the history but cannot
    be undone (re-audit 2026-09-29, M3). Undo replayed the count into the tree
    with state.setParameter: no processing refusal, no topology callback, so
    the engine kept its old shape until the next snapshot recall saw the
    difference and stopped processing mid-show. Undoing a delete could not
    bring the channel back either - it appended a new default mono channel.
    The opposite tool is the way back. */
inline void markStructural (ChangeRecord& record)
{
    record.undoable = false;
    record.operatorDescription += " (structural change - not undoable)";
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
        state.markChannelNumbersUserOwned ("MCP channel create");
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
        markStructural (*record);
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
        state.markChannelNumbersUserOwned ("MCP channel delete");

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
        markStructural (*record);
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

/** Structural channel edits are stopped-only. The GUI enforces that by greying its
    I/O controls while processing runs (SystemConfigTab::updateIOControlsEnabledState);
    nothing enforced it for any other caller, so MCP could restructure the channel
    list mid-show — resizing routing matrices and releasing the algorithm underneath
    a live audio callback. Same flag the GUI reads, so the two cannot disagree.

    Deliberately at this layer and not inside WFSValueTreeState: the config-load path
    calls setNumInputChannels directly, and a guard down there would refuse to load a
    project whenever the engine happened to be running. */
inline bool refuseWhileProcessing (WFSValueTreeState& state,
                                   const juce::String& what,
                                   ToolResult& out)
{
    if (! state.isProcessingEnabled())
        return false;

    out = ToolResult::error ("engine_running",
                             juce::String ("Cannot ") + what
                               + " while the audio engine is running. Channel structure "
                                 "is stopped-only: stop processing first (System Config "
                                 "> Run DSP), then retry.");
    return true;
}

//==============================================================================
// Input channel COUNTS
//
// These two shadow the generated system_i_o_set_input_channels /
// system_i_o_set_stereo_input_channels by name (hand-written tools are registered
// after the generated set and overwrite by name - see MCPServer). They exist
// because the generated pair could not do what they said:
//
//   * the stereo one wrote `stereoInputChannels`, which setParameter has ignored
//     outright since the stable-number rework made a channel's type per-channel.
//     It burned the operator's safety gate and changed nothing.
//   * the mono one is LABELLED "Mono Inputs" but routed to setNumInputChannels,
//     which sets the TOTAL and removes by highest NUMBER - while the field of the
//     same name in System Config means the mono count and removes the last channel
//     of that type in DISPLAY order. On a dragged list those are different channels.
//   * neither fired the topology callback, so the tree changed and the renderer,
//     routing matrices, patch rows and meters were never rebuilt.
//
// Both now do exactly what the two GUI count fields do: clamp identically, call
// setInputChannelCounts, and re-prepare the engine.
//==============================================================================

/** Schema for the two count tools: the new count, plus the tier-3 confirm token. */
inline juce::var countSchema (int maxValue, const juce::String& description)
{
    auto props = std::make_unique<juce::DynamicObject>();

    auto value = std::make_unique<juce::DynamicObject>();
    value->setProperty ("type",        "integer");
    value->setProperty ("minimum",     0);
    value->setProperty ("maximum",     maxValue);
    value->setProperty ("description", description);
    props->setProperty ("value", juce::var (value.release()));

    auto confirm = std::make_unique<juce::DynamicObject>();
    confirm->setProperty ("type", "string");
    confirm->setProperty ("description",
        "Confirmation token from the previous call's tier_enforcement envelope.");
    props->setProperty ("confirm", juce::var (confirm.release()));

    auto schema = std::make_unique<juce::DynamicObject>();
    schema->setProperty ("type", "object");
    schema->setProperty ("properties", juce::var (props.release()));
    schema->setProperty ("required", juce::var (juce::Array<juce::var> { juce::var ("value") }));
    schema->setProperty ("additionalProperties", false);
    return juce::var (schema.release());
}

/** Apply one axis of the mono/stereo composition, leaving the other as it is. */
inline ToolResult setCounts (WFSValueTreeState& state,
                             bool settingStereo,
                             int requested,
                             ChangeRecord* record)
{
    const int liveTotal  = state.getNumInputChannels();
    const int liveStereo = state.getNumStereoInputChannels();
    const int liveMono   = liveTotal - liveStereo;

    int wantMono   = settingStereo ? liveMono  : requested;
    int wantStereo = settingStereo ? requested : liveStereo;

    // The same clamp the GUI applies, from the same function, so a value MCP
    // accepts and a value typed into the field land on identical compositions.
    WFSValueTreeState::clampInputChannelCounts (wantMono, wantStereo);

    if (wantMono == liveMono && wantStereo == liveStereo)
    {
        auto unchanged = std::make_unique<juce::DynamicObject>();
        unchanged->setProperty ("mono",    liveMono);
        unchanged->setProperty ("stereo",  liveStereo);
        unchanged->setProperty ("total",   liveTotal);
        unchanged->setProperty ("changed", false);
        return ToolResult::ok (juce::var (unchanged.release()));
    }

    // Captured BEFORE the mutation: removeInputChannel renumbers on an unlatched
    // session, so asking afterwards describes channels that no longer exist under
    // the numbers they had. This is the same list the GUI confirmation dialog
    // shows, from the same prediction.
    juce::Array<juce::var> removedDescriptions;
    for (const auto& victim : state.predictInputChannelReduction (wantMono, wantStereo))
        removedDescriptions.add (juce::var (describeInputChannel (victim)));

    state.setInputChannelCounts (wantMono, wantStereo);

    const int newTotal  = state.getNumInputChannels();
    const int newStereo = state.getNumStereoInputChannels();
    const int newMono   = newTotal - newStereo;

    if (record != nullptr)
    {
        record->affectedParameters.add (WFSParameterIDs::inputChannels.toString());
        record->affectedGroups.push_back ({ 0, juce::String ("I/O") });

        auto before = std::make_unique<juce::DynamicObject>();
        before->setProperty ("mono",   liveMono);
        before->setProperty ("stereo", liveStereo);
        record->beforeState = juce::var (before.release());

        auto after = std::make_unique<juce::DynamicObject>();
        after->setProperty ("mono",   newMono);
        after->setProperty ("stereo", newStereo);
        record->afterState = juce::var (after.release());

        record->operatorDescription =
            juce::String ("Set input channels to ") + juce::String (newMono)
            + " mono + " + juce::String (newStereo) + " stereo (was "
            + juce::String (liveMono) + " + " + juce::String (liveStereo) + ")";
        markStructural (*record);
    }

    auto result = std::make_unique<juce::DynamicObject>();
    result->setProperty ("mono",    newMono);
    result->setProperty ("stereo",  newStereo);
    result->setProperty ("total",   newTotal);
    result->setProperty ("changed", true);
    if (requested != (settingStereo ? newStereo : newMono))
        result->setProperty ("requested", requested);   // clamped: say so
    if (! removedDescriptions.isEmpty())
        result->setProperty ("removed_channels", juce::var (removedDescriptions));
    return ToolResult::ok (juce::var (result.release()));
}

inline ToolDescriptor describeSetCount (WFSValueTreeState& state,
                                        bool settingStereo,
                                        const std::function<void()>* topologyChanged)
{
    ToolDescriptor d;
    d.name = settingStereo ? "system_i_o_set_stereo_input_channels"
                           : "system_i_o_set_input_channels";
    d.description = (settingStereo
        ? juce::String (
            "Set how many input channels are STEREO pairs, leaving the mono count as "
            "it is. Raising it appends stereo channels after the last channel; "
            "lowering it removes the last stereo channel(s) in DISPLAY order - the "
            "bottom of the Arrange list - and retires their numbers as permanent "
            "gaps. Each stereo channel claims two hardware inputs in the patch. "
            "Returns the resulting mono/stereo/total and, on a reduction, "
            "removed_channels naming exactly what went. Clears every tab undo "
            "history and re-prepares the engine.")
        : juce::String (
            "Set how many input channels are MONO, leaving the stereo count as it is. "
            "Raising it appends mono channels after the last channel; lowering it "
            "removes the last mono channel(s) in DISPLAY order - the bottom of the "
            "Arrange list - and retires their numbers as permanent gaps. This is the "
            "mono count, NOT the total: the total is mono + stereo. Returns the "
            "resulting mono/stereo/total and, on a reduction, removed_channels "
            "naming exactly what went. Clears every tab undo history and "
            "re-prepares the engine."))
        + " Refused while the audio engine is running. Not undoable: set the count back instead."
        + juce::String (kTier3DescriptionSuffix);
    d.inputSchema = countSchema (settingStereo ? WFSParameterDefaults::maxStereoChannels
                                               : WFSParameterDefaults::maxInputChannels,
                                 settingStereo ? "Number of stereo pair input channels."
                                               : "Number of mono input channels.");
    d.modifiesState = true;
    d.tier          = 3;   // removes channels; same gate the generated pair carried
    d.handler = [&state, settingStereo, topologyChanged] (const juce::var& args,
                                                          ChangeRecord* record) -> ToolResult
    {
        auto* obj = args.getDynamicObject();
        if (obj == nullptr || ! obj->hasProperty ("value"))
            return ToolResult::error ("invalid_args", "Missing required arg: value");

        ToolResult refusal;
        if (refuseWhileProcessing (state, "change the input channel composition", refusal))
            return refusal;

        auto result = setCounts (state, settingStereo,
                                 static_cast<int> (obj->getProperty ("value")), record);
        if (result.success)
            notifyTopology (topologyChanged);
        return result;
    };
    return d;
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
                    "restart, so plan accordingly. Not undoable: delete the channel "
                    "instead.")
                  + juce::String (kTier2DescriptionSuffix)
                  : "Add one or more " + kindLabel + " channels by bumping "
                    + cfg.countParamId.toString() + ". With the default count=1 "
                    "this creates a single channel; pass count=N (up to "
                    + juce::String (cfg.hardMax) + ") to create N at once with "
                    "a single tier-2 handshake and a single history entry. "
                    "Returns `created_channel_ids` (array of new 1-based ids), "
                    "`first_channel_id`, `last_channel_id`, `created_count`, "
                    "`total`, `kind`. Also returns `channel_id` (deprecated "
                    "alias for last_channel_id) for back-compat. Refused with "
                    "at_capacity when currentCount + count > hardMax ("
                    + juce::String (cfg.hardMax) + "). Triggers a DSP restart, "
                    "so plan accordingly. Not undoable: delete the channels "
                    "instead."
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
        ToolResult refusal;
        if (refuseWhileProcessing (state, "create a " + cfg.kindLabel + " channel", refusal))
            return refusal;

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
                    "DSP restart. Not undoable: create a channel instead (its "
                    "parameters are not restored).")
                  + juce::String (kTier2DescriptionSuffix)
                  : "Remove the highest-numbered " + kindLabel + " channel by "
                    "decrementing " + cfg.countParamId.toString() + " by 1. "
                    "The deleted channel's parameters are dropped. Refused "
                    "with `empty` when the count is already 0. Triggers a DSP "
                    "restart. Not undoable: create a channel instead (its "
                    "parameters are not restored)."
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
        ToolResult refusal;
        if (refuseWhileProcessing (state, "delete a " + cfg.kindLabel + " channel", refusal))
            return refusal;

        auto result = del (state, cfg, requestedNumber, record);
        if (result.success)
            notifyTopology (topologyChanged);
        return result;
    };
    return d;
}

//==============================================================================
// Sampler SETS
//
// The 17 generated input_sampler_set_set_* tools address a set by number, but
// there was no way to make one: onAddSet is a GUI button, so on a fresh input
// every one of those tools failed with "no set at that index" and stayed failed.
// A capability the operator has and MCP does not is the same class of gap as a
// tool that silently does nothing - it just fails honestly instead.
//==============================================================================

inline juce::var samplerSetCreateSchema()
{
    auto props = std::make_unique<juce::DynamicObject>();

    auto inputId = std::make_unique<juce::DynamicObject>();
    inputId->setProperty ("type",    "integer");
    inputId->setProperty ("minimum", 1);
    inputId->setProperty ("maximum", WFSParameterDefaults::maxInputChannels);
    inputId->setProperty ("description",
        "Permanent number of the input channel to add the set to.");
    props->setProperty ("input_id", juce::var (inputId.release()));

    auto name = std::make_unique<juce::DynamicObject>();
    name->setProperty ("type", "string");
    name->setProperty ("description",
        "Optional name. Omitted: \"Set N\" for the next number.");
    props->setProperty ("name", juce::var (name.release()));

    auto confirm = std::make_unique<juce::DynamicObject>();
    confirm->setProperty ("type", "string");
    confirm->setProperty ("description",
        "Confirmation token from the previous call's tier_enforcement envelope.");
    props->setProperty ("confirm", juce::var (confirm.release()));

    auto schema = std::make_unique<juce::DynamicObject>();
    schema->setProperty ("type", "object");
    schema->setProperty ("properties", juce::var (props.release()));
    schema->setProperty ("required", juce::var (juce::Array<juce::var> { juce::var ("input_id") }));
    schema->setProperty ("additionalProperties", false);
    return juce::var (schema.release());
}

inline juce::var samplerSetDeleteSchema()
{
    auto props = std::make_unique<juce::DynamicObject>();

    auto inputId = std::make_unique<juce::DynamicObject>();
    inputId->setProperty ("type",    "integer");
    inputId->setProperty ("minimum", 1);
    inputId->setProperty ("maximum", WFSParameterDefaults::maxInputChannels);
    inputId->setProperty ("description", "Permanent number of the input channel.");
    props->setProperty ("input_id", juce::var (inputId.release()));

    auto setId = std::make_unique<juce::DynamicObject>();
    setId->setProperty ("type",    "integer");
    setId->setProperty ("minimum", 1);
    setId->setProperty ("maximum", WFSParameterDefaults::maxSamplerSets);
    setId->setProperty ("description",
        "Set number to remove (1-based, as the Sampler tab numbers them). "
        "Sets after it KEEP their positions, so the numbers of later sets shift "
        "down by one - the same as deleting from the tab.");
    props->setProperty ("set_id", juce::var (setId.release()));

    auto confirm = std::make_unique<juce::DynamicObject>();
    confirm->setProperty ("type", "string");
    confirm->setProperty ("description",
        "Confirmation token from the previous call's tier_enforcement envelope.");
    props->setProperty ("confirm", juce::var (confirm.release()));

    auto schema = std::make_unique<juce::DynamicObject>();
    schema->setProperty ("type", "object");
    schema->setProperty ("properties", juce::var (props.release()));
    schema->setProperty ("required",
                         juce::var (juce::Array<juce::var> { juce::var ("input_id"),
                                                             juce::var ("set_id") }));
    schema->setProperty ("additionalProperties", false);
    return juce::var (schema.release());
}

inline ToolDescriptor describeSamplerSetCreate (WFSValueTreeState& state,
                                                const std::function<void (int)>* onSamplerChanged)
{
    ToolDescriptor d;
    d.name        = "input_sampler_create_set";
    d.description = juce::String (
        "Add a sampler set to an input channel and return its 1-based number. "
        "A set groups cells for playback and carries its own position, level and "
        "pressure mappings; an input starts with none, which is why every "
        "input_sampler_set_set_* tool fails until one exists. Capped at "
        + juce::String (WFSParameterDefaults::maxSamplerSets) + " sets per input.")
        + juce::String (kTier2DescriptionSuffix);
    d.inputSchema   = samplerSetCreateSchema();
    d.modifiesState = true;
    d.tier          = 2;
    d.handler = [&state, onSamplerChanged] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        auto* obj = args.getDynamicObject();
        if (obj == nullptr || ! obj->hasProperty ("input_id"))
            return ToolResult::error ("invalid_args", "Missing required arg: input_id");

        const int number = static_cast<int> (obj->getProperty ("input_id"));
        const int slot   = state.getSlotForChannelNumber (number);
        if (slot < 0)
            return ToolResult::error ("invalid_args",
                                      "input_id not a live channel: " + juce::String (number));

        const juce::String name = obj->hasProperty ("name")
                                    ? obj->getProperty ("name").toString() : juce::String();

        auto created = state.addInputSamplerSet (slot, name);
        if (! created.isValid())
            return ToolResult::error ("at_capacity",
                                      "That input already has the maximum of "
                                        + juce::String (WFSParameterDefaults::maxSamplerSets)
                                        + " sampler sets.");

        const int total = state.getNumInputSamplerSets (slot);

        if (record != nullptr)
        {
            record->affectedParameters.add (WFSParameterIDs::samplerSetName.toString());
            record->affectedGroups.push_back ({ number, juce::String ("Sampler") });
            record->operatorDescription = "Created sampler set " + juce::String (total)
                                            + " on input " + juce::String (number);
        }

        if (onSamplerChanged != nullptr && *onSamplerChanged)
            (*onSamplerChanged) (slot);

        auto result = std::make_unique<juce::DynamicObject>();
        result->setProperty ("input_id", number);
        result->setProperty ("set_id",   total);   // 1-based: the new set is the last
        result->setProperty ("name",     created.getProperty (WFSParameterIDs::samplerSetName));
        result->setProperty ("total",    total);
        return ToolResult::ok (juce::var (result.release()));
    };
    return d;
}

inline ToolDescriptor describeSamplerSetDelete (WFSValueTreeState& state,
                                                const std::function<void (int)>* onSamplerChanged)
{
    ToolDescriptor d;
    d.name        = "input_sampler_delete_set";
    d.description = juce::String (
        "Remove a sampler set from an input channel by its 1-based number. Later "
        "sets shift down by one, exactly as they do when a set is deleted from the "
        "Sampler tab - their stored ids were never reliable for this, which is why "
        "every consumer counts positions instead.")
        + juce::String (kTier2DescriptionSuffix);
    d.inputSchema   = samplerSetDeleteSchema();
    d.modifiesState = true;
    d.tier          = 2;
    d.handler = [&state, onSamplerChanged] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        auto* obj = args.getDynamicObject();
        if (obj == nullptr || ! obj->hasProperty ("input_id") || ! obj->hasProperty ("set_id"))
            return ToolResult::error ("invalid_args", "Missing required arg: input_id and set_id");

        const int number = static_cast<int> (obj->getProperty ("input_id"));
        const int slot   = state.getSlotForChannelNumber (number);
        if (slot < 0)
            return ToolResult::error ("invalid_args",
                                      "input_id not a live channel: " + juce::String (number));

        const int oneBased = static_cast<int> (obj->getProperty ("set_id"));
        if (! state.removeInputSamplerSet (slot, oneBased - 1))
            return ToolResult::error ("invalid_args",
                                      "No sampler set " + juce::String (oneBased)
                                        + " on that input (sets are numbered from 1).");

        const int total = state.getNumInputSamplerSets (slot);

        if (record != nullptr)
        {
            record->affectedParameters.add (WFSParameterIDs::samplerSetName.toString());
            record->affectedGroups.push_back ({ number, juce::String ("Sampler") });
            record->operatorDescription = "Deleted sampler set " + juce::String (oneBased)
                                            + " on input " + juce::String (number);
        }

        if (onSamplerChanged != nullptr && *onSamplerChanged)
            (*onSamplerChanged) (slot);

        auto result = std::make_unique<juce::DynamicObject>();
        result->setProperty ("input_id",       number);
        result->setProperty ("deleted_set_id", oneBased);
        result->setProperty ("total",          total);
        return ToolResult::ok (juce::var (result.release()));
    };
    return d;
}

//==============================================================================
// Input ORDER and NUMBERS
//
// The display order (the Arrange list, top to bottom) and the permanent numbers
// are two different things: a drag moves a channel, its patch row and its
// cluster positions while its number stays; a relabel changes numbers while
// every channel stays where it is. Both were GUI-only. These three tools call
// the same WFSValueTreeState functions the Arrange list and the project-load
// identity dialog call, and end with the same reconfiguration pass.
//
// All three are stopped-only and clear every tab undo history (the state
// functions do that themselves): a channel's slot is the key of every undo
// entry, and a move invalidates them all. Filed, not undoable - the way back
// is another move.
//==============================================================================

/** Every live input in display order: { position, id, name, type }. */
inline juce::var inputDisplayOrder (WFSValueTreeState& state)
{
    juce::Array<juce::var> out;
    for (int slot = 0; slot < state.getNumInputChannels(); ++slot)
    {
        auto obj = std::make_unique<juce::DynamicObject>();
        obj->setProperty ("position", slot + 1);
        obj->setProperty ("id",       state.getInputChannelNumber (slot));
        obj->setProperty ("name",     state.getInputParameter (slot, WFSParameterIDs::inputName).toString());
        obj->setProperty ("type",     state.isInputChannelStereo (slot) ? "stereo" : "mono");
        out.add (juce::var (obj.release()));
    }
    return juce::var (out);
}

inline std::vector<int> liveNumbersInDisplayOrder (WFSValueTreeState& state)
{
    std::vector<int> out;
    for (int slot = 0; slot < state.getNumInputChannels(); ++slot)
        out.push_back (state.getInputChannelNumber (slot));
    return out;
}

inline juce::String numberList (const std::vector<int>& numbers)
{
    juce::StringArray parts;
    for (int n : numbers)
        parts.add (juce::String (n));
    return parts.joinIntoString (", ");
}

inline ToolResult moveInput (WFSValueTreeState& state, const juce::var& args, ChangeRecord* record)
{
    auto* obj = args.getDynamicObject();
    if (obj == nullptr)
        return ToolResult::error ("invalid_args", "Arguments must be a JSON object");
    for (const char* arg : { "input_id", "position" })
        if (Args::missing (*obj, arg))
            return ToolResult::error ("invalid_args", juce::String ("Missing required arg: ") + arg);

    juce::String code, message;
    const auto number = Args::wholeNumber (obj->getProperty ("input_id"), "input_id", 1,
                                           WFSParameterDefaults::maxInputChannels, code, message);
    if (! number.has_value())
        return ToolResult::error (code, message);
    const int total = state.getNumInputChannels();
    const auto position = Args::wholeNumber (obj->getProperty ("position"), "position", 1,
                                             juce::jmax (1, total), code, message);
    if (! position.has_value())
        return ToolResult::error (code, message);

    const int fromSlot = state.getSlotForChannelNumber (*number);
    if (fromSlot < 0)
        return ToolResult::error ("invalid_args", "input_id is not a live input channel: " + juce::String (*number));

    // Latch first: unlatched, a move renumbers every channel to its new display
    // position, which would change the very number the client just quoted.
    state.markChannelNumbersUserOwned ("MCP input move");

    auto res = state.moveInputChannel (*number, *position - 1);
    if (res.failed())
        return ToolResult::error ("invalid_args", res.getErrorMessage());

    if (record != nullptr)
    {
        record->affectedParameters.add (WFSParameterIDs::inputChannels.toString());
        record->affectedGroups.push_back ({ *number, juce::String ("Channel") });
        record->operatorDescription = "Moved input " + juce::String (*number) + " from position "
                                    + juce::String (fromSlot + 1) + " to " + juce::String (*position);
        markStructural (*record);
    }

    auto result = std::make_unique<juce::DynamicObject>();
    result->setProperty ("input_id",      *number);
    result->setProperty ("from_position", fromSlot + 1);
    result->setProperty ("position",      state.getSlotForChannelNumber (*number) + 1);
    result->setProperty ("order",         inputDisplayOrder (state));
    return ToolResult::ok (juce::var (result.release()));
}

inline ToolResult reorderInputs (WFSValueTreeState& state, const juce::var& args, ChangeRecord* record)
{
    auto* obj = args.getDynamicObject();
    if (obj == nullptr || Args::missing (*obj, "order"))
        return ToolResult::error ("invalid_args", "Missing required arg: order");

    juce::String code, message;
    const auto order = Args::wholeNumberArray (obj->getProperty ("order"), "order", 1,
                                               WFSParameterDefaults::maxInputChannels, code, message);
    if (! order.has_value())
        return ToolResult::error (code, message);
    if (order->empty())
        return ToolResult::error ("invalid_args", "order must list at least one input number");

    std::vector<int> seen, unknown;
    for (int n : *order)
    {
        if (std::find (seen.begin(), seen.end(), n) != seen.end())
            return ToolResult::error ("invalid_args", "input " + juce::String (n) + " appears twice in order");
        seen.push_back (n);
        if (state.getSlotForChannelNumber (n) < 0)
            unknown.push_back (n);
    }
    if (unknown.size() == order->size())
        return ToolResult::error ("invalid_args", "none of the listed numbers is a live input channel");

    const auto before = liveNumbersInDisplayOrder (state);

    // Latches the numbers itself, before the first move.
    auto res = state.reorderInputChannelsToNumbers (*order, "MCP input reorder");
    if (res.failed())
        return ToolResult::error ("internal_error", res.getErrorMessage());

    const auto after = liveNumbersInDisplayOrder (state);

    if (record != nullptr)
    {
        record->affectedParameters.add (WFSParameterIDs::inputChannels.toString());
        record->affectedGroups.push_back ({ 0, juce::String ("I/O") });
        record->operatorDescription = "Rearranged the input order to " + numberList (after)
                                    + " (was " + numberList (before) + ")";
        markStructural (*record);
    }

    auto result = std::make_unique<juce::DynamicObject>();
    result->setProperty ("changed", before != after);
    result->setProperty ("order",   inputDisplayOrder (state));
    if (! unknown.empty())
    {
        juce::Array<juce::var> skipped;
        for (int n : unknown)
            skipped.add (n);
        result->setProperty ("skipped_unknown_ids", juce::var (skipped));
    }
    return ToolResult::ok (juce::var (result.release()));
}

inline ToolResult renumberInputs (WFSValueTreeState& state, const juce::var& args, ChangeRecord* record)
{
    auto* obj = args.getDynamicObject();
    if (obj == nullptr || Args::missing (*obj, "numbers"))
        return ToolResult::error ("invalid_args", "Missing required arg: numbers");

    juce::String code, message;
    const auto numbers = Args::wholeNumberArray (obj->getProperty ("numbers"), "numbers", 1,
                                                 WFSParameterDefaults::maxInputChannels, code, message);
    if (! numbers.has_value())
        return ToolResult::error (code, message);

    const auto before = liveNumbersInDisplayOrder (state);

    // Validates the whole list (length, range, duplicates) before the first
    // write, so a refused list changes nothing. Latches the numbers.
    auto res = state.assignInputChannelNumbersBySlot (*numbers, "MCP input renumber");
    if (res.failed())
        return ToolResult::error ("invalid_args", res.getErrorMessage());

    if (record != nullptr)
    {
        record->affectedParameters.add (WFSParameterIDs::inputChannels.toString());
        record->affectedGroups.push_back ({ 0, juce::String ("I/O") });
        record->operatorDescription = "Renumbered the inputs to " + numberList (*numbers)
                                    + " (was " + numberList (before) + ")";
        markStructural (*record);
    }

    auto result = std::make_unique<juce::DynamicObject>();
    result->setProperty ("changed", before != *numbers);
    result->setProperty ("order",   inputDisplayOrder (state));
    return ToolResult::ok (juce::var (result.release()));
}

inline ToolDescriptor describeInputMove (WFSValueTreeState& state, const std::function<void()>* topologyChanged)
{
    ToolDescriptor d;
    d.name        = "input_move";
    d.description = juce::String (
        "Move one input channel to another place in the DISPLAY order (the Arrange list, "
        "top to bottom), as dragging it there does. The channel keeps its permanent "
        "number, settings and hardware inputs; its patch row and cluster positions "
        "move with it. position is 1-based (1 = top). Returns the new order. "
        "session_get_state lists the current order. Refused while the audio engine "
        "is running. Clears every tab undo history. Not undoable: move it back instead.")
        + juce::String (kTier2DescriptionSuffix);

    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("input_id", Args::integerArg ("Permanent number of the input to move.", 1,
                                                      WFSParameterDefaults::maxInputChannels));
    props->setProperty ("position", Args::integerArg ("Target place in the display order, 1 = top.", 1,
                                                      WFSParameterDefaults::maxInputChannels));
    props->setProperty ("confirm", Args::confirmArg());
    d.inputSchema   = Args::schemaOf (std::move (props), { "input_id", "position" });
    d.modifiesState = true;
    d.tier          = 2;
    d.handler = [&state, topologyChanged] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        ToolResult refusal;
        if (refuseWhileProcessing (state, "move an input channel", refusal))
            return refusal;
        auto result = moveInput (state, args, record);
        if (result.success)
            notifyTopology (topologyChanged);
        return result;
    };
    return d;
}

inline ToolDescriptor describeInputReorder (WFSValueTreeState& state, const std::function<void()>* topologyChanged)
{
    ToolDescriptor d;
    d.name        = "input_reorder";
    d.description = juce::String (
        "Rearrange the input channels so their permanent numbers read `order` from the "
        "top of the display order down, e.g. order=[5,1,2] puts input 5 first. Numbers, "
        "settings and hardware inputs travel with their channel; nothing is renumbered "
        "(input_renumber does that). Channels not listed keep their relative order "
        "after the listed ones. Numbers that are not live are skipped and reported in "
        "skipped_unknown_ids. Returns the new order. Refused while the audio engine is "
        "running. Clears every tab undo history. Not undoable: reorder again instead.")
        + juce::String (kTier2DescriptionSuffix);

    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("order", Args::integerArrayArg ("Permanent input numbers in the wanted display "
                                                        "order, top first. No duplicates.",
                                                        1, WFSParameterDefaults::maxInputChannels));
    props->setProperty ("confirm", Args::confirmArg());
    d.inputSchema   = Args::schemaOf (std::move (props), { "order" });
    d.modifiesState = true;
    d.tier          = 2;
    d.handler = [&state, topologyChanged] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        ToolResult refusal;
        if (refuseWhileProcessing (state, "reorder the input channels", refusal))
            return refusal;
        auto result = reorderInputs (state, args, record);
        if (result.success)
            notifyTopology (topologyChanged);
        return result;
    };
    return d;
}

inline ToolDescriptor describeInputRenumber (WFSValueTreeState& state, const std::function<void()>* topologyChanged)
{
    ToolDescriptor d;
    d.name        = "input_renumber";
    d.description = juce::String (
        "Give the input channels new permanent numbers WITHOUT moving them: numbers[i] "
        "becomes the number of the channel at display position i+1. Every channel keeps "
        "its place, settings, name and hardware inputs, and its effect sends follow it. "
        "The list must name every live input exactly once (same length as the input "
        "count, no duplicates, 1-64); a bad list changes nothing. CAUTION: the numbers "
        "are what snapshots, QLab/Go.dot cues, OSC and DAW mappings address, so those "
        "reach different channels afterwards - this is for the one-time alignment of a "
        "session with the numbers an external system expects. Refused while the audio "
        "engine is running. Clears every tab undo history. Not undoable.")
        + juce::String (kTier3DescriptionSuffix);

    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("numbers", Args::integerArrayArg ("New permanent number for each display position, "
                                                          "top first.",
                                                          1, WFSParameterDefaults::maxInputChannels));
    props->setProperty ("confirm", Args::confirmArg());
    d.inputSchema   = Args::schemaOf (std::move (props), { "numbers" });
    d.modifiesState = true;
    d.tier          = 3;
    d.handler = [&state, topologyChanged] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        ToolResult refusal;
        if (refuseWhileProcessing (state, "renumber the input channels", refusal))
            return refusal;
        auto result = renumberInputs (state, args, record);
        if (result.success)
            notifyTopology (topologyChanged);
        return result;
    };
    return d;
}

} // namespace WFSNetwork::Tools::ChannelLifecycle
