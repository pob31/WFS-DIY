#pragma once

#include <JuceHeader.h>
#include "../MCPCompat.h"
#include "../../../Parameters/WFSValueTreeState.h"
#include "../../../Parameters/WFSParameterIDs.h"
#include "../../../Parameters/WFSFileManager.h"

namespace WFSNetwork::Tools::Session
{

/** Build a JSON object summarizing one channel: { id, name, x, y, z }. */
inline juce::var summarizeInputChannel (WFSValueTreeState& state, int channelIndex)
{
    auto obj = std::make_unique<juce::DynamicObject>();
    obj->setProperty ("id",   channelIndex + 1);  // 1-based for the AI
    obj->setProperty ("name", state.getInputParameter (channelIndex, WFSParameterIDs::inputName).toString());
    obj->setProperty ("x",    state.getInputParameter (channelIndex, WFSParameterIDs::inputPositionX));
    obj->setProperty ("y",    state.getInputParameter (channelIndex, WFSParameterIDs::inputPositionY));
    obj->setProperty ("z",    state.getInputParameter (channelIndex, WFSParameterIDs::inputPositionZ));
    return juce::var (obj.release());
}

inline juce::var summarizeOutputChannel (WFSValueTreeState& state, int channelIndex)
{
    auto obj = std::make_unique<juce::DynamicObject>();
    obj->setProperty ("id",            channelIndex + 1);
    obj->setProperty ("name",          state.getOutputParameter (channelIndex, WFSParameterIDs::outputName).toString());
    obj->setProperty ("x",             state.getOutputParameter (channelIndex, WFSParameterIDs::outputPositionX));
    obj->setProperty ("y",             state.getOutputParameter (channelIndex, WFSParameterIDs::outputPositionY));
    obj->setProperty ("z",             state.getOutputParameter (channelIndex, WFSParameterIDs::outputPositionZ));
    obj->setProperty ("array",         state.getOutputParameter (channelIndex, WFSParameterIDs::outputArray));
    return juce::var (obj.release());
}

inline juce::var summarizeReverbChannel (WFSValueTreeState& state, int channelIndex)
{
    auto obj = std::make_unique<juce::DynamicObject>();
    obj->setProperty ("id",   channelIndex + 1);
    obj->setProperty ("name", state.getReverbParameter (channelIndex, WFSParameterIDs::reverbName).toString());
    obj->setProperty ("x",    state.getReverbParameter (channelIndex, WFSParameterIDs::reverbPositionX));
    obj->setProperty ("y",    state.getReverbParameter (channelIndex, WFSParameterIDs::reverbPositionY));
    obj->setProperty ("z",    state.getReverbParameter (channelIndex, WFSParameterIDs::reverbPositionZ));
    return juce::var (obj.release());
}

/** session.get_state — read-only summary of inputs, outputs, reverbs, and
    channel counts. Read-only; does not produce a change record. */
inline ToolResult getState (WFSValueTreeState& state)
{
    juce::Array<juce::var> inputs;
    for (int i = 0; i < state.getNumInputChannels(); ++i)
        inputs.add (summarizeInputChannel (state, i));

    juce::Array<juce::var> outputs;
    for (int i = 0; i < state.getNumOutputChannels(); ++i)
        outputs.add (summarizeOutputChannel (state, i));

    juce::Array<juce::var> reverbs;
    for (int i = 0; i < state.getNumReverbChannels(); ++i)
        reverbs.add (summarizeReverbChannel (state, i));

    auto channelCounts = std::make_unique<juce::DynamicObject>();
    channelCounts->setProperty ("inputs",  state.getNumInputChannels());
    channelCounts->setProperty ("outputs", state.getNumOutputChannels());
    channelCounts->setProperty ("reverbs", state.getNumReverbChannels());

    auto root = std::make_unique<juce::DynamicObject>();
    root->setProperty ("channel_counts", juce::var (channelCounts.release()));
    root->setProperty ("inputs",  juce::var (inputs));
    root->setProperty ("outputs", juce::var (outputs));
    root->setProperty ("reverbs", juce::var (reverbs));

    return ToolResult::ok (juce::var (root.release()));
}

inline juce::var buildInputSchema()
{
    // session.get_state takes no arguments.
    auto schema = std::make_unique<juce::DynamicObject>();
    schema->setProperty ("type", "object");
    schema->setProperty ("properties", juce::var (new juce::DynamicObject()));
    schema->setProperty ("additionalProperties", false);
    return juce::var (schema.release());
}

inline ToolDescriptor describe (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "session_get_state";
    d.description = "Read-only summary of the current WFS-DIY session: "
                    "channel counts, plus per-channel id/name/position for "
                    "every input, output, and reverb. Use this to orient "
                    "before issuing modifying tool calls. For everything on "
                    "ONE channel (orientation, pitch, EQ bands, mutes, etc.) "
                    "call session_get_channel_full instead. For globals "
                    "(stage / origin / master / binaural / network) call "
                    "session_get_global_state.";
    d.inputSchema   = buildInputSchema();
    d.modifiesState = false;
    d.tier        = 1;  // read-only
    d.handler = [&state] (const juce::var&, ChangeRecord*) -> ToolResult
    {
        return getState (state);
    };
    return d;
}

/** session.save — persist the full session to the current project folder.
    Same code path as the SystemConfig tab's Save button
    (WFSFileManager::saveCompleteConfig → system/network/inputs/outputs/
    reverbs .xml, each with a rolling backup). Tier 2: it overwrites the
    operator's saved show on disk, so it needs the confirm round-trip. */
inline ToolResult save (WFSFileManager& fileManager)
{
    if (! fileManager.hasValidProjectFolder())
        return ToolResult::error ("no_project_folder",
                                  "No valid project folder is open - load or create a "
                                  "project before calling session_save.");

    if (! fileManager.saveCompleteConfig())
        return ToolResult::error ("save_failed", fileManager.getLastError());

    auto result = std::make_unique<juce::DynamicObject>();
    result->setProperty ("saved", true);
    result->setProperty ("project_folder", fileManager.getProjectFolder().getFullPathName());
    return ToolResult::ok (juce::var (result.release()));
}

inline ToolDescriptor describeSave (WFSFileManager& fileManager)
{
    ToolDescriptor d;
    d.name        = "session_save";
    d.description = "Save the complete current session (system, network, inputs, "
                    "outputs, reverbs) to the open project folder - identical to "
                    "the operator pressing Save in the System Config tab. "
                    "Overwrites the project's config files on disk (a rolling "
                    "backup of each file is kept in backups/). Fails if no "
                    "project folder is open."
                  + juce::String (kTier2DescriptionSuffix);
    d.inputSchema   = buildInputSchema();
    d.modifiesState = false;  // writes disk, not the in-memory session — nothing to undo
    d.tier        = 2;  // overwrites the saved show on disk
    d.handler = [&fileManager] (const juce::var&, ChangeRecord*) -> ToolResult
    {
        return save (fileManager);
    };
    return d;
}

} // namespace WFSNetwork::Tools::Session
