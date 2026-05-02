#pragma once

#include <JuceHeader.h>
#include "../MCPToolRegistry.h"
#include "../MCPChangeRecords.h"
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
    int                 hardMax;         // 64 / 64 / 16
};

inline const ChannelKindConfig& configFor (const juce::String& kind)
{
    static const ChannelKindConfig inputCfg  { "input",  WFSParameterIDs::inputChannels,  64 };
    static const ChannelKindConfig outputCfg { "output", WFSParameterIDs::outputChannels, 64 };
    static const ChannelKindConfig reverbCfg { "reverb", WFSParameterIDs::reverbChannels, 16 };
    if (kind == "input")  return inputCfg;
    if (kind == "output") return outputCfg;
    return reverbCfg;
}

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

inline ToolResult create (WFSValueTreeState& state,
                            const ChannelKindConfig& cfg,
                            ChangeRecord* record)
{
    const int currentCount = static_cast<int> (state.getParameter (cfg.countParamId, -1));
    if (currentCount >= cfg.hardMax)
        return ToolResult::error ("at_capacity",
            cfg.kindLabel + " channel count already at maximum ("
            + juce::String (cfg.hardMax) + ").");

    const int newCount = currentCount + 1;
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

        record->operatorDescription = "Created " + cfg.kindLabel + " channel "
                                       + juce::String (newCount)
                                       + " (" + cfg.kindLabel + " count "
                                       + juce::String (currentCount) + " → "
                                       + juce::String (newCount) + ")";
    }

    auto result = std::make_unique<juce::DynamicObject>();
    result->setProperty ("channel_id", newCount);  // 1-based id of the new channel
    result->setProperty ("total",      newCount);
    result->setProperty ("kind",       cfg.kindLabel);
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
                                       + juce::String (currentCount) + " → "
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
    d.description = "Add one " + kindLabel + " channel by bumping "
                    + cfg.countParamId.toString() + " by 1. Returns the new "
                    "channel's 1-based id (always == new total). Refused with "
                    "at_capacity when count is already at the hard maximum ("
                    + juce::String (cfg.hardMax) + "). Triggers a DSP restart, "
                    "so plan accordingly."
                  + juce::String (kTier2DescriptionSuffix);
    d.inputSchema   = emptyObjectSchema();
    d.modifiesState = true;
    d.tier        = 2;
    d.handler = [&state, &cfg] (const juce::var&, ChangeRecord* record) -> ToolResult
    {
        return create (state, cfg, record);
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
