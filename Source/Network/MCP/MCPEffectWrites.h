#pragma once

#include <JuceHeader.h>
#include "../../Parameters/WFSValueTreeState.h"
#include "../../Parameters/WFSParameterIDs.h"

/*
    How an MCP write reaches an effect channel - one place, shared by the
    generated tools, wfs_set_parameter / batch / nudge and the undo engine.

    The generic path (state.setParameter) finds an effect property by walking
    the channel's children for the first node that carries the name. That is
    right for the flat sections and the single-instance modules, but it skips
    the rules a write from outside the GUI must keep: a reverb TYPE expands
    into its preset, a real edit to a preset-owned value makes the reverb
    Custom first, a tap time edited in Pattern mode takes over from the
    pattern, and a position write latches position ownership. OSC goes through
    applyExternalEffectEdit for exactly that (OSCManager::applyEffectUpdate),
    and so does MCP, through these functions. Like OSC, no link-group
    propagation: a remote write addresses the channel it names.

    Writes land in the Effects tab's undo domain, as the OSC ones do.
*/
namespace WFSNetwork::MCPEffect
{

/** A per-channel property of an <Effect> (not an effectsGlobal* or the
    effectChannels / effectsMapVisible config values). */
inline bool isChannelParameter (const juce::Identifier& id)
{
    const auto s = id.toString();
    return s.startsWith ("effect") && ! s.startsWith ("effects")
        && id != WFSParameterIDs::effectChannels;
}

/** A Settings global of the effects engine (stored on <Config>). */
inline bool isGlobalParameter (const juce::Identifier& id)
{
    return id.toString().startsWith ("effectsGlobal");
}

/** A packed row: written whole through setEffectParameter, which refuses a
    value that is not a well-formed row and keeps the stored one. */
inline bool isRow (const juce::Identifier& id)
{
    return id == WFSParameterIDs::effectMutes
        || id == WFSParameterIDs::effectChainOrder
        || id == WFSParameterIDs::effectSendLevels
        || id == WFSParameterIDs::effectSendOns
        || id == WFSParameterIDs::effectFxSendLevels
        || id == WFSParameterIDs::effectFxSendOns;
}

/** Properties that exist more than once per channel (two EQ and two dynamics
    modules, six bands, eight taps): a channel index alone cannot name them,
    so only the generated tool with its instance / effect_band / tap argument
    can write them. */
inline bool needsSubIndex (const juce::Identifier& id)
{
    const auto s = id.toString();
    return s.startsWith ("effectEQ") || s.startsWith ("effectDyn")
        || id == WFSParameterIDs::effectDelayTapTime
        || id == WFSParameterIDs::effectDelayTapLevel;
}

inline bool isLive (WFSValueTreeState& state, int channelIndex)
{
    return channelIndex >= 0 && channelIndex < state.getNumEffectChannels()
        && state.getEffectState (channelIndex).isValid();
}

inline juce::String notLiveMessage (WFSValueTreeState& state, int effectId)
{
    return "effect_id " + juce::String (effectId) + " is not a live effect channel (this session has "
         + juce::String (state.getNumEffectChannels()) + "; ids are dense and start at 1)";
}

/** Write one flat per-channel effect value the way OSC does. */
inline void writeChannelValue (WFSValueTreeState& state, int channelIndex,
                               const juce::Identifier& id, const juce::var& value)
{
    WFSValueTreeState::ScopedUndoDomain scope (state, UndoDomain::Effects);
    if (isRow (id))
        state.setEffectParameter (channelIndex, id, value);
    else
        state.applyExternalEffectEdit (channelIndex, id, value);
}

/** Write one value into an effect module node (an EQ / dynamics instance, a
    band or a tap). A tap goes through prepareEffectSectionWrite first, so a
    tap time edited in Pattern mode takes over from the pattern. */
inline void writeSectionValue (WFSValueTreeState& state, juce::ValueTree node,
                               const juce::Identifier& id, const juce::var& value)
{
    WFSValueTreeState::ScopedUndoDomain scope (state, UndoDomain::Effects);
    if (node.hasType (WFSParameterIDs::Tap) || node.hasType (WFSParameterIDs::FxDelay))
        state.prepareEffectSectionWrite (node, id, value);
    state.writeSubTreeProperty (node, id, value, state.getActiveUndoManager());
}

/** The general write route for tools addressing a parameter by (name,
    channelIndex) - wfs_set_parameter, its batch, the undo engine: effect
    values the OSC way, everything else setParameter as before. */
inline void routeWrite (WFSValueTreeState& state, const juce::Identifier& id,
                        const juce::var& value, int channelIndex)
{
    if (isChannelParameter (id) && channelIndex >= 0)
    {
        writeChannelValue (state, channelIndex, id, value);
        return;
    }
    if (isGlobalParameter (id))
    {
        WFSValueTreeState::ScopedUndoDomain scope (state, UndoDomain::Effects);
        state.setParameter (id, value, channelIndex);
        return;
    }
    state.setParameter (id, value, channelIndex);
}

/** For a (variable, channel_id) write to an effect-scope parameter: empty when
    it can go ahead, else why not. `toolName` is the generated tool that can
    write it when a module instance, band or tap is needed. */
inline juce::String refuseRegistryTarget (WFSValueTreeState& state, const juce::Identifier& id,
                                          int effectId, const juce::String& toolName)
{
    if (needsSubIndex (id))
        return "'" + id.toString() + "' exists once per EQ/dynamics module, band or delay tap, "
               "which channel_id alone cannot name. Call `" + toolName + "` with its "
               "instance / effect_band / tap argument instead.";
    if (! isLive (state, effectId - 1))
        return notLiveMessage (state, effectId);
    return {};
}

} // namespace WFSNetwork::MCPEffect
