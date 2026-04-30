#pragma once

#include <JuceHeader.h>
#include "../MCPToolRegistry.h"
#include "../MCPChangeRecords.h"
#include "../../../Parameters/WFSValueTreeState.h"
#include "../../../Parameters/WFSParameterIDs.h"

namespace WFSNetwork::Tools::Reverb
{

namespace detail
{
    inline int resolveChannelIndex (WFSValueTreeState& state, int reverbId)
    {
        if (reverbId < 1 || reverbId > state.getNumReverbChannels())
            return -1;
        return reverbId - 1;
    }

    inline juce::var positionRangeSchema()
    {
        auto v = std::make_unique<juce::DynamicObject>();
        v->setProperty ("type", "number");
        v->setProperty ("minimum", -50.0);
        v->setProperty ("maximum",  50.0);
        return juce::var (v.release());
    }
}

//==============================================================================
// reverb.position.set_cartesian — set X, Y, Z in one call
//==============================================================================

inline juce::var buildSetCartesianSchema()
{
    auto reverbId = std::make_unique<juce::DynamicObject>();
    reverbId->setProperty ("type", "integer");
    reverbId->setProperty ("minimum", 1);
    reverbId->setProperty ("maximum", 16);
    reverbId->setProperty ("description", "Reverb channel number (1-based).");

    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("reverb_id", juce::var (reverbId.release()));
    props->setProperty ("x", detail::positionRangeSchema());
    props->setProperty ("y", detail::positionRangeSchema());
    props->setProperty ("z", detail::positionRangeSchema());

    auto required = juce::Array<juce::var>();
    required.add ("reverb_id");
    required.add ("x");
    required.add ("y");
    required.add ("z");

    auto schema = std::make_unique<juce::DynamicObject>();
    schema->setProperty ("type", "object");
    schema->setProperty ("properties", juce::var (props.release()));
    schema->setProperty ("required", juce::var (required));
    schema->setProperty ("additionalProperties", false);
    return juce::var (schema.release());
}

inline ToolResult setCartesian (WFSValueTreeState& state, const juce::var& args, ChangeRecord* record)
{
    if (! args.isObject())
        return ToolResult::error ("invalid_args", "Arguments must be a JSON object");

    auto* obj = args.getDynamicObject();
    int reverbId = static_cast<int> (obj->getProperty ("reverb_id"));
    int channelIndex = detail::resolveChannelIndex (state, reverbId);
    if (channelIndex < 0)
        return ToolResult::error ("invalid_args", "reverb_id out of range: " + juce::String (reverbId));

    constexpr float kStageMax = 50.0f;
    float x = juce::jlimit (-kStageMax, kStageMax, static_cast<float> (obj->getProperty ("x")));
    float y = juce::jlimit (-kStageMax, kStageMax, static_cast<float> (obj->getProperty ("y")));
    float z = juce::jlimit (-kStageMax, kStageMax, static_cast<float> (obj->getProperty ("z")));

    float oldX = static_cast<float> (state.getReverbParameter (channelIndex, WFSParameterIDs::reverbPositionX));
    float oldY = static_cast<float> (state.getReverbParameter (channelIndex, WFSParameterIDs::reverbPositionY));
    float oldZ = static_cast<float> (state.getReverbParameter (channelIndex, WFSParameterIDs::reverbPositionZ));

    state.setReverbParameter (channelIndex, WFSParameterIDs::reverbPositionX, x);
    state.setReverbParameter (channelIndex, WFSParameterIDs::reverbPositionY, y);
    state.setReverbParameter (channelIndex, WFSParameterIDs::reverbPositionZ, z);

    juce::String displayName =
        state.getReverbParameter (channelIndex, WFSParameterIDs::reverbName).toString();
    juce::String pretty = displayName.isNotEmpty()
                            ? " (" + displayName + ")"
                            : juce::String();

    if (record != nullptr)
    {
        record->operatorDescription = "Moved reverb " + juce::String (reverbId) + pretty
                                       + " to x=" + juce::String (x, 2)
                                       + ", y="   + juce::String (y, 2)
                                       + ", z="   + juce::String (z, 2);
        record->affectedParameters.add ("reverbPositionX");
        record->affectedParameters.add ("reverbPositionY");
        record->affectedParameters.add ("reverbPositionZ");
        record->affectedGroups.push_back ({ reverbId, "Position" });

        auto before = std::make_unique<juce::DynamicObject>();
        before->setProperty ("reverbPositionX", oldX);
        before->setProperty ("reverbPositionY", oldY);
        before->setProperty ("reverbPositionZ", oldZ);
        record->beforeState = juce::var (before.release());

        auto after = std::make_unique<juce::DynamicObject>();
        after->setProperty ("reverbPositionX", x);
        after->setProperty ("reverbPositionY", y);
        after->setProperty ("reverbPositionZ", z);
        record->afterState = juce::var (after.release());
    }

    auto result = std::make_unique<juce::DynamicObject>();
    result->setProperty ("reverb_id", reverbId);
    result->setProperty ("x", x);
    result->setProperty ("y", y);
    result->setProperty ("z", z);
    return ToolResult::ok (juce::var (result.release()));
}

inline ToolDescriptor describeSetCartesian (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "reverb_position_set_cartesian";
    d.description = "Move a reverb channel to absolute Cartesian coordinates "
                    "(meters) in one call. Coordinate convention: +X = stage "
                    "right, +Y = upstage (away from audience), +Z = up. "
                    "Out-of-range values are clamped to the +/-50 m stage "
                    "envelope. Use `session_get_state()` first if unsure "
                    "which reverb channels exist.";
    d.inputSchema   = buildSetCartesianSchema();
    d.modifiesState = true;
    d.tier        = 1;
    d.handler = [&state] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        return setCartesian (state, args, record);
    };
    return d;
}

} // namespace WFSNetwork::Tools::Reverb
