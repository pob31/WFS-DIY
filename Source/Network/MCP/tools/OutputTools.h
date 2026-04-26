#pragma once

#include <JuceHeader.h>
#include "../MCPToolRegistry.h"
#include "../MCPChangeRecords.h"
#include "../../../Parameters/WFSValueTreeState.h"
#include "../../../Parameters/WFSParameterIDs.h"

namespace WFSNetwork::Tools::Output
{

namespace detail
{
    inline int resolveChannelIndex (WFSValueTreeState& state, int outputId)
    {
        if (outputId < 1 || outputId > state.getNumOutputChannels())
            return -1;
        return outputId - 1;
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
// output.position.set_cartesian — set X, Y, Z in one call
//==============================================================================

inline juce::var buildSetCartesianSchema()
{
    auto outputId = std::make_unique<juce::DynamicObject>();
    outputId->setProperty ("type", "integer");
    outputId->setProperty ("minimum", 1);
    outputId->setProperty ("maximum", 64);
    outputId->setProperty ("description", "Output channel number (1-based).");

    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("output_id", juce::var (outputId.release()));
    props->setProperty ("x", detail::positionRangeSchema());
    props->setProperty ("y", detail::positionRangeSchema());
    props->setProperty ("z", detail::positionRangeSchema());

    auto required = juce::Array<juce::var>();
    required.add ("output_id");
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
    int outputId = static_cast<int> (obj->getProperty ("output_id"));
    int channelIndex = detail::resolveChannelIndex (state, outputId);
    if (channelIndex < 0)
        return ToolResult::error ("invalid_args", "output_id out of range: " + juce::String (outputId));

    constexpr float kStageMax = 50.0f;
    float x = juce::jlimit (-kStageMax, kStageMax, static_cast<float> (obj->getProperty ("x")));
    float y = juce::jlimit (-kStageMax, kStageMax, static_cast<float> (obj->getProperty ("y")));
    float z = juce::jlimit (-kStageMax, kStageMax, static_cast<float> (obj->getProperty ("z")));

    float oldX = static_cast<float> (state.getOutputParameter (channelIndex, WFSParameterIDs::outputPositionX));
    float oldY = static_cast<float> (state.getOutputParameter (channelIndex, WFSParameterIDs::outputPositionY));
    float oldZ = static_cast<float> (state.getOutputParameter (channelIndex, WFSParameterIDs::outputPositionZ));

    state.setOutputParameter (channelIndex, WFSParameterIDs::outputPositionX, x);
    state.setOutputParameter (channelIndex, WFSParameterIDs::outputPositionY, y);
    state.setOutputParameter (channelIndex, WFSParameterIDs::outputPositionZ, z);

    juce::String displayName =
        state.getOutputParameter (channelIndex, WFSParameterIDs::outputName).toString();
    juce::String pretty = displayName.isNotEmpty()
                            ? " (" + displayName + ")"
                            : juce::String();

    if (record != nullptr)
    {
        record->operatorDescription = "Moved output " + juce::String (outputId) + pretty
                                       + " to x=" + juce::String (x, 2)
                                       + ", y="   + juce::String (y, 2)
                                       + ", z="   + juce::String (z, 2);
        record->affectedParameters.add ("outputPositionX");
        record->affectedParameters.add ("outputPositionY");
        record->affectedParameters.add ("outputPositionZ");
        record->affectedGroups.push_back ({ outputId, "Position" });

        auto before = std::make_unique<juce::DynamicObject>();
        before->setProperty ("outputPositionX", oldX);
        before->setProperty ("outputPositionY", oldY);
        before->setProperty ("outputPositionZ", oldZ);
        record->beforeState = juce::var (before.release());

        auto after = std::make_unique<juce::DynamicObject>();
        after->setProperty ("outputPositionX", x);
        after->setProperty ("outputPositionY", y);
        after->setProperty ("outputPositionZ", z);
        record->afterState = juce::var (after.release());
    }

    auto result = std::make_unique<juce::DynamicObject>();
    result->setProperty ("output_id", outputId);
    result->setProperty ("x", x);
    result->setProperty ("y", y);
    result->setProperty ("z", z);
    return ToolResult::ok (juce::var (result.release()));
}

inline ToolDescriptor describeSetCartesian (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "output_position_set_cartesian";
    d.description = "Move an output speaker to absolute Cartesian coordinates "
                    "(meters) in one call. Coordinate convention: +X = stage "
                    "right, +Y = upstage (away from audience), +Z = up. "
                    "Out-of-range values are clamped to the +/-50 m stage "
                    "envelope. Use `session_get_state()` first if unsure "
                    "which output channels exist.";
    d.inputSchema   = buildSetCartesianSchema();
    d.modifiesState = true;
    d.handler = [&state] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        return setCartesian (state, args, record);
    };
    return d;
}

} // namespace WFSNetwork::Tools::Output
