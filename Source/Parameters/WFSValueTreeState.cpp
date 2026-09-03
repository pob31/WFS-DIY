#include "WFSValueTreeState.h"
#include "../Network/OSCParameterBounds.h"
#include "../WFSLogger.h"
#include "../Sampler/SamplerData.h"

#include <algorithm>
#include <vector>

using namespace WFSParameterIDs;

namespace
{
    /** Find the child of `parent` with `type` whose `property` equals `wanted`.
        ADM identity is carried in a property, never in child position: the
        mappings of both kinds are siblings under <ADMOSC>, and the GUI's own
        lookups match on the id for the same reason. */
    juce::ValueTree findChildByIntProperty (const juce::ValueTree& parent,
                                            const juce::Identifier& type,
                                            const juce::Identifier& property,
                                            int wanted)
    {
        for (int i = 0; i < parent.getNumChildren(); ++i)
        {
            auto child = parent.getChild (i);
            if (child.hasType (type) && static_cast<int> (child.getProperty (property, -1)) == wanted)
                return child;
        }
        return {};
    }
}

using namespace WFSParameterDefaults;

//==============================================================================
// Construction / Destruction
//==============================================================================

WFSValueTreeState::WFSValueTreeState()
    : TreeParameterStore (static_cast<int> (UndoDomain::COUNT),
                          { "Input", "Output", "Reverb", "Map", "Config", "Clusters" })
{
    // WRITE-INTERCEPTOR (control Q6a): numeric-bounds hardening at the store
    // choke point, using the same bounds table OSC ingress and the MCP
    // escape-hatch validate against (both of those REJECT out-of-range before
    // the store; this clamp only ever fires for paths that used to bypass
    // validation). In-range numeric writes and all non-numeric writes return
    // the proposed var UNTOUCHED — same object, same type — so every
    // already-validated caller produces byte-identical results.
    setWriteInterceptor ([] (const juce::Identifier& property, const juce::var& proposed,
                             const juce::ValueTree&) -> juce::var
    {
        if (proposed.isDouble() || proposed.isInt() || proposed.isInt64())
        {
            // LFO phases are circular: wrap into the canonical [-180, 180]
            // instead of clamping, so legacy 0..360 values (accepted by the
            // gates' compat window) land on the equivalent angle.
            if (WFSNetwork::isLFOPhaseParam (property))
            {
                const double d = static_cast<double> (proposed);
                if (d < -180.0 || d > 180.0)
                    return juce::var (WFSParameterDefaults::wrapPhaseDegrees (juce::roundToInt (d)));
                return proposed;
            }

            if (const auto bounds = WFSNetwork::getBounds (property))
            {
                const double d = static_cast<double> (proposed);
                if (d < bounds->min || d > bounds->max)
                {
                    const double clamped = juce::jlimit (bounds->min, bounds->max, d);
                    return bounds->isInt ? juce::var (juce::roundToInt (clamped))
                                         : juce::var (clamped);
                }
            }
        }
        return proposed;
    });

    initializeDefaultState();
    state.addListener (this);
}

WFSValueTreeState::~WFSValueTreeState()
{
    state.removeListener (this);
}

//==============================================================================
// State Access
//==============================================================================

juce::ValueTree WFSValueTreeState::getConfigState()
{
    return state.getChildWithName (Config);
}

juce::ValueTree WFSValueTreeState::getConfigState() const
{
    return state.getChildWithName (Config);
}

juce::ValueTree WFSValueTreeState::getShowState()
{
    return getConfigState().getChildWithName (Show);
}

juce::ValueTree WFSValueTreeState::getIOState()
{
    return getConfigState().getChildWithName (IO);
}

juce::ValueTree WFSValueTreeState::getStageState()
{
    return getConfigState().getChildWithName (Stage);
}

juce::ValueTree WFSValueTreeState::getMasterState()
{
    return getConfigState().getChildWithName (Master);
}

juce::ValueTree WFSValueTreeState::getNetworkState()
{
    return getConfigState().getChildWithName (Network);
}

juce::ValueTree WFSValueTreeState::getUIState()
{
    return getConfigState().getChildWithName (UI);
}

juce::ValueTree WFSValueTreeState::getADMOSCState()
{
    return getConfigState().getChildWithName (ADMOSC);
}

juce::ValueTree WFSValueTreeState::getTrackingState()
{
    return getConfigState().getChildWithName (Tracking);
}

juce::ValueTree WFSValueTreeState::getClustersState()
{
    return getConfigState().getChildWithName (Clusters);
}

juce::ValueTree WFSValueTreeState::getClustersState() const
{
    return getConfigState().getChildWithName (Clusters);
}

juce::ValueTree WFSValueTreeState::getBinauralState()
{
    return getConfigState().getChildWithName (Binaural);
}

juce::ValueTree WFSValueTreeState::getBinauralState() const
{
    return getConfigState().getChildWithName (Binaural);
}

juce::ValueTree WFSValueTreeState::getClusterState (int clusterIndex)
{
    auto clusters = getClustersState();
    // clusterIndex is 1-based (1-10), convert to 0-based for array access
    int idx = clusterIndex - 1;
    if (idx >= 0 && idx < clusters.getNumChildren())
        return clusters.getChild (idx);
    return {};
}

juce::ValueTree WFSValueTreeState::getClusterLFOSection (int clusterIndex)
{
    auto cluster = getClusterState (clusterIndex);
    if (! cluster.isValid())
        return {};

    auto lfoSection = cluster.getChildWithName (ClusterLFO);
    if (! lfoSection.isValid())
    {
        // Migration: create with defaults if missing (old file loaded)
        lfoSection = juce::ValueTree (ClusterLFO);
        lfoSection.setProperty (clusterLFOactive,         clusterLFOactiveDefault,          nullptr);
        lfoSection.setProperty (clusterLFOperiod,         clusterLFOperiodDefault,          nullptr);
        lfoSection.setProperty (clusterLFOphase,          clusterLFOphaseDefault,           nullptr);
        lfoSection.setProperty (clusterLFOshapeX,         clusterLFOshapeDefault,           nullptr);
        lfoSection.setProperty (clusterLFOshapeY,         clusterLFOshapeDefault,           nullptr);
        lfoSection.setProperty (clusterLFOshapeZ,         clusterLFOshapeDefault,           nullptr);
        lfoSection.setProperty (clusterLFOshapeRot,       clusterLFOshapeDefault,           nullptr);
        lfoSection.setProperty (clusterLFOshapeScale,     clusterLFOshapeDefault,           nullptr);
        lfoSection.setProperty (clusterLFOrateX,          clusterLFOrateDefault,            nullptr);
        lfoSection.setProperty (clusterLFOrateY,          clusterLFOrateDefault,            nullptr);
        lfoSection.setProperty (clusterLFOrateZ,          clusterLFOrateDefault,            nullptr);
        lfoSection.setProperty (clusterLFOrateRot,        clusterLFOrateDefault,            nullptr);
        lfoSection.setProperty (clusterLFOrateScale,      clusterLFOrateDefault,            nullptr);
        lfoSection.setProperty (clusterLFOamplitudeX,     clusterLFOamplitudeXYZDefault,    nullptr);
        lfoSection.setProperty (clusterLFOamplitudeY,     clusterLFOamplitudeXYZDefault,    nullptr);
        lfoSection.setProperty (clusterLFOamplitudeZ,     clusterLFOamplitudeXYZDefault,    nullptr);
        lfoSection.setProperty (clusterLFOamplitudeRot,   clusterLFOamplitudeRotDefault,    nullptr);
        lfoSection.setProperty (clusterLFOamplitudeScale, clusterLFOamplitudeScaleDefault,  nullptr);
        lfoSection.setProperty (clusterLFOphaseX,         clusterLFOphaseDefault,           nullptr);
        lfoSection.setProperty (clusterLFOphaseY,         clusterLFOphaseDefault,           nullptr);
        lfoSection.setProperty (clusterLFOphaseZ,         clusterLFOphaseDefault,           nullptr);
        lfoSection.setProperty (clusterLFOphaseRot,       clusterLFOphaseDefault,           nullptr);
        lfoSection.setProperty (clusterLFOphaseScale,     clusterLFOphaseDefault,           nullptr);
        cluster.appendChild (lfoSection, nullptr);
    }
    return lfoSection;
}

juce::ValueTree WFSValueTreeState::getClusterLFOPresetsSection()
{
    auto config = getConfigState();
    auto presets = config.getChildWithName (ClusterLFOPresets);
    if (! presets.isValid())
    {
        // Migration: create with 16 empty preset slots
        presets = juce::ValueTree (ClusterLFOPresets);
        for (int i = 0; i < maxClusterLFOPresets; ++i)
        {
            juce::ValueTree preset (ClusterLFOPreset);
            preset.setProperty (clusterLFOPresetName,      juce::String(),                  nullptr);
            preset.setProperty (clusterLFOperiod,          clusterLFOperiodDefault,          nullptr);
            preset.setProperty (clusterLFOphase,           clusterLFOphaseDefault,           nullptr);
            preset.setProperty (clusterLFOshapeX,          clusterLFOshapeDefault,           nullptr);
            preset.setProperty (clusterLFOshapeY,          clusterLFOshapeDefault,           nullptr);
            preset.setProperty (clusterLFOshapeZ,          clusterLFOshapeDefault,           nullptr);
            preset.setProperty (clusterLFOshapeRot,        clusterLFOshapeDefault,           nullptr);
            preset.setProperty (clusterLFOshapeScale,      clusterLFOshapeDefault,           nullptr);
            preset.setProperty (clusterLFOrateX,           clusterLFOrateDefault,            nullptr);
            preset.setProperty (clusterLFOrateY,           clusterLFOrateDefault,            nullptr);
            preset.setProperty (clusterLFOrateZ,           clusterLFOrateDefault,            nullptr);
            preset.setProperty (clusterLFOrateRot,         clusterLFOrateDefault,            nullptr);
            preset.setProperty (clusterLFOrateScale,       clusterLFOrateDefault,            nullptr);
            preset.setProperty (clusterLFOamplitudeX,      clusterLFOamplitudeXYZDefault,    nullptr);
            preset.setProperty (clusterLFOamplitudeY,      clusterLFOamplitudeXYZDefault,    nullptr);
            preset.setProperty (clusterLFOamplitudeZ,      clusterLFOamplitudeXYZDefault,    nullptr);
            preset.setProperty (clusterLFOamplitudeRot,    clusterLFOamplitudeRotDefault,    nullptr);
            preset.setProperty (clusterLFOamplitudeScale,  clusterLFOamplitudeScaleDefault,  nullptr);
            preset.setProperty (clusterLFOphaseX,          clusterLFOphaseDefault,           nullptr);
            preset.setProperty (clusterLFOphaseY,          clusterLFOphaseDefault,           nullptr);
            preset.setProperty (clusterLFOphaseZ,          clusterLFOphaseDefault,           nullptr);
            preset.setProperty (clusterLFOphaseRot,        clusterLFOphaseDefault,           nullptr);
            preset.setProperty (clusterLFOphaseScale,      clusterLFOphaseDefault,           nullptr);
            presets.appendChild (preset, nullptr);
        }
        config.appendChild (presets, nullptr);
    }
    return presets;
}

juce::ValueTree WFSValueTreeState::ensureClusterLFOPreset (int presetIndex)
{
    auto presets = getClusterLFOPresetsSection();
    if (presetIndex >= 0 && presetIndex < presets.getNumChildren())
        return presets.getChild (presetIndex);
    return {};
}

void WFSValueTreeState::recallClusterLFOPreset (int clusterId, int presetIndex)
{
    auto preset = ensureClusterLFOPreset (presetIndex);
    if (! preset.isValid()) return;

    auto lfoSection = getClusterLFOSection (clusterId);
    if (! lfoSection.isValid()) return;

    // The 22 preset properties (same order as ClustersTab::getLFOPresetPropId)
    static const juce::Identifier ids[] = {
        clusterLFOperiod, clusterLFOphase,
        clusterLFOshapeX, clusterLFOshapeY, clusterLFOshapeZ, clusterLFOshapeRot, clusterLFOshapeScale,
        clusterLFOrateX, clusterLFOrateY, clusterLFOrateZ, clusterLFOrateRot, clusterLFOrateScale,
        clusterLFOamplitudeX, clusterLFOamplitudeY, clusterLFOamplitudeZ, clusterLFOamplitudeRot, clusterLFOamplitudeScale,
        clusterLFOphaseX, clusterLFOphaseY, clusterLFOphaseZ, clusterLFOphaseRot, clusterLFOphaseScale
    };

    // Apply non-shape properties first (amplitude, rate, phase, period)
    for (const auto& propId : ids)
    {
        if (propId == clusterLFOshapeX || propId == clusterLFOshapeY || propId == clusterLFOshapeZ ||
            propId == clusterLFOshapeRot || propId == clusterLFOshapeScale)
            continue;
        if (preset.hasProperty (propId))
            lfoSection.setProperty (propId, preset.getProperty (propId), nullptr);
    }

    // Then apply shapes (triggers fade dip if shape changed)
    lfoSection.setProperty (clusterLFOshapeX,     preset.getProperty (clusterLFOshapeX), nullptr);
    lfoSection.setProperty (clusterLFOshapeY,     preset.getProperty (clusterLFOshapeY), nullptr);
    lfoSection.setProperty (clusterLFOshapeZ,     preset.getProperty (clusterLFOshapeZ), nullptr);
    lfoSection.setProperty (clusterLFOshapeRot,   preset.getProperty (clusterLFOshapeRot), nullptr);
    lfoSection.setProperty (clusterLFOshapeScale, preset.getProperty (clusterLFOshapeScale), nullptr);
}

juce::ValueTree WFSValueTreeState::getInputsState()
{
    return state.getChildWithName (Inputs);
}

juce::ValueTree WFSValueTreeState::getInputsState() const
{
    return state.getChildWithName (Inputs);
}

juce::ValueTree WFSValueTreeState::getInputState (int channelIndex)
{
    auto inputs = getInputsState();
    if (channelIndex >= 0 && channelIndex < inputs.getNumChildren())
        return inputs.getChild (channelIndex);
    return {};
}

juce::ValueTree WFSValueTreeState::getOutputsState()
{
    return state.getChildWithName (Outputs);
}

juce::ValueTree WFSValueTreeState::getOutputsState() const
{
    return state.getChildWithName (Outputs);
}

juce::ValueTree WFSValueTreeState::getOutputState (int channelIndex)
{
    auto outputs = getOutputsState();
    if (channelIndex >= 0 && channelIndex < outputs.getNumChildren())
        return outputs.getChild (channelIndex);
    return {};
}

juce::ValueTree WFSValueTreeState::getReverbsState()
{
    return state.getChildWithName (Reverbs);
}

juce::ValueTree WFSValueTreeState::getReverbsState() const
{
    return state.getChildWithName (Reverbs);
}

juce::ValueTree WFSValueTreeState::getReverbState (int channelIndex)
{
    auto reverbs = getReverbsState();
    int reverbCount = 0;
    for (int i = 0; i < reverbs.getNumChildren(); ++i)
    {
        auto child = reverbs.getChild (i);
        if (child.hasType (Reverb))
        {
            if (reverbCount == channelIndex)
                return child;
            ++reverbCount;
        }
    }
    return {};
}

juce::ValueTree WFSValueTreeState::getAudioPatchState()
{
    return state.getChildWithName (AudioPatch);
}

//==============================================================================
// Parameter Access - Type Safe
//==============================================================================
// Typed getters live in TreeParameterStore (resolved through the
// getTreeForParameter override below). The setters stay here because the
// channel-count writes need schema-structural routing before the generic
// core write.

void WFSValueTreeState::setParameter (const juce::Identifier& paramId, const juce::var& value, int channelIndex)
{
    // Special case: writes to channel-count parameters must go through the
    // setNumXChannels helpers so the channel subtrees actually grow/shrink
    // alongside the count property. A bare setProperty here updates the
    // property but leaves the array stale — the GUI editor calls
    // setNumXChannels directly, but OSC ingress / MCP set_parameter / file
    // load round-trips would silently land here and desync. Cast to int via
    // juce::var so a numeric-string ("12") still routes correctly.
    if (paramId == inputChannels)
    {
        setNumInputChannels (static_cast<int> (value));
        return;
    }
    if (paramId == outputChannels)
    {
        setNumOutputChannels (static_cast<int> (value));
        return;
    }
    if (paramId == reverbChannels)
    {
        setNumReverbChannels (static_cast<int> (value));
        return;
    }
    if (paramId == stereoInputChannels)
    {
        // Obsolete under the stable-number model: a channel's type lives on
        // the channel (inputChannelType) and changes through
        // setInputChannelType. Ignore so a stale file value or remote write
        // cannot resurrect the count-based tail semantics.
        juce::Logger::writeToLog ("Ignoring write to obsolete stereoInputChannels parameter");
        return;
    }

    TreeParameterStore::setParameter (paramId, value, channelIndex);
}

bool WFSValueTreeState::isProcessingEnabled() const
{
    // Read through the tree rather than getIOState(), which is non-const.
    // Absent means a tree that has not been built yet, which is not "running".
    const auto io = state.getChildWithName (Config).getChildWithName (IO);
    return io.isValid() && static_cast<bool> (io.getProperty (runDSP, false));
}

bool WFSValueTreeState::canWriteParameter (const juce::Identifier& paramId, int channelIndex) const
{
    // The three live channel counts never reach getTreeForParameter: setParameter
    // re-routes them to the setNumXChannels helpers above. They resolve to nothing
    // and are writable anyway, so answer for the routing, not for the tree.
    if (paramId == inputChannels || paramId == outputChannels || paramId == reverbChannels)
        return true;

    // Deliberately dropped — see the comment in setParameter. Saying so here is the
    // whole point: a caller that asks gets told, instead of being handed a success.
    if (paramId == stereoInputChannels)
        return false;

    return getTreeForParameter (paramId, channelIndex).isValid();
}

void WFSValueTreeState::setParameterWithoutUndo (const juce::Identifier& paramId, const juce::var& value, int channelIndex)
{
    // Same channel-count routing as setParameter — but setNumXChannels
    // always uses getActiveUndoManager(). When the caller asked for "no
    // undo", the count subtree resize still needs to happen; we just
    // accept the slightly redundant undo bookkeeping for these
    // structural writes (the alternative is duplicating ~80 lines of
    // setNumXChannels with `nullptr` undo, which isn't worth it).
    if (paramId == inputChannels)  { setNumInputChannels  (static_cast<int> (value)); return; }
    if (paramId == outputChannels) { setNumOutputChannels (static_cast<int> (value)); return; }
    if (paramId == reverbChannels) { setNumReverbChannels (static_cast<int> (value)); return; }
    if (paramId == stereoInputChannels) { return; }  // obsolete — see setParameter

    TreeParameterStore::setParameterWithoutUndo (paramId, value, channelIndex);
}

//==============================================================================
// Input Channel Access
//==============================================================================

juce::var WFSValueTreeState::getInputParameter (int channelIndex, const juce::Identifier& paramId) const
{
    auto input = const_cast<WFSValueTreeState*>(this)->getInputState (channelIndex);
    if (!input.isValid())
        return {};

    // Search through all subsections
    for (int i = 0; i < input.getNumChildren(); ++i)
    {
        auto child = input.getChild (i);
        if (child.hasProperty (paramId))
            return child.getProperty (paramId);
    }
    return {};
}

void WFSValueTreeState::setInputParameter (int channelIndex, const juce::Identifier& paramId, const juce::var& value)
{
    auto input = getInputState (channelIndex);
    if (!input.isValid())
        return;

    // Search through all subsections
    for (int i = 0; i < input.getNumChildren(); ++i)
    {
        auto child = input.getChild (i);
        if (child.hasProperty (paramId))
        {
            writeProperty (child, paramId, value, getActiveUndoManager());

            // Maintain the Shared-Position cluster invariant: any write to
            // inputPositionX/Y/Z on a shared-mode cluster member propagates
            // to every other member. No-op for inputs not in Shared mode.
            if (paramId == inputPositionX || paramId == inputPositionY || paramId == inputPositionZ)
            {
                propagateSharedClusterPosition (channelIndex);

                // A position write through the user-facing API (UI tabs, Map
                // tab, OSC, MCP) means the user is positioning things: latch
                // ownership so auto-placement never fights them. The engine's
                // own layout helpers write via setProperty and bypass this.
                markPositionsUserOwned();
            }

            return;
        }
    }

    // Property not found - add it to the appropriate section if we know where it belongs
    // This handles old config files that may be missing newer properties
    if (paramId == inputCoordinateMode)
    {
        auto position = getInputPositionSection (channelIndex);
        if (position.isValid())
            writeProperty (position, paramId, value, getActiveUndoManager());
    }
    else if (paramId == inputMuteReverbSends)
    {
        auto mutes = getInputMutesSection (channelIndex);
        if (mutes.isValid())
            writeProperty (mutes, paramId, value, getActiveUndoManager());
    }
    // Note: inputAttenuation always exists in the Channel section (created by
    // createInputChannelSection), so the search loop above always finds it - no
    // fallback needed. The calc engine reads it from the Channel section too.
}

juce::ValueTree WFSValueTreeState::getInputChannelSection (int channelIndex)
{
    return getInputState (channelIndex).getChildWithName (Channel);
}

juce::ValueTree WFSValueTreeState::getInputPositionSection (int channelIndex)
{
    return getInputState (channelIndex).getChildWithName (Position);
}

juce::ValueTree WFSValueTreeState::getInputAttenuationSection (int channelIndex)
{
    return getInputState (channelIndex).getChildWithName (Attenuation);
}

juce::ValueTree WFSValueTreeState::getInputDirectivitySection (int channelIndex)
{
    return getInputState (channelIndex).getChildWithName (Directivity);
}

juce::ValueTree WFSValueTreeState::getInputLiveSourceSection (int channelIndex)
{
    return getInputState (channelIndex).getChildWithName (LiveSourceTamer);
}

juce::ValueTree WFSValueTreeState::getInputHackousticsSection (int channelIndex)
{
    return getInputState (channelIndex).getChildWithName (Hackoustics);
}

juce::ValueTree WFSValueTreeState::getInputLFOSection (int channelIndex)
{
    return getInputState (channelIndex).getChildWithName (LFO);
}

juce::ValueTree WFSValueTreeState::getInputAutoMotionSection (int channelIndex)
{
    return getInputState (channelIndex).getChildWithName (AutomOtion);
}

juce::ValueTree WFSValueTreeState::getInputMutesSection (int channelIndex)
{
    return getInputState (channelIndex).getChildWithName (Mutes);
}

juce::ValueTree WFSValueTreeState::getInputGradientMapsSection (int channelIndex)
{
    return getInputState (channelIndex).getChildWithName (GradientMaps);
}

juce::ValueTree WFSValueTreeState::getInputSamplerCell (int channelIndex, int cellIndex)
{
    auto sampler = getInputSamplerSection (channelIndex);
    if (! sampler.isValid() || cellIndex < 0)
        return {};
    return findChildByIntProperty (sampler, SamplerCell, id, cellIndex);
}

juce::ValueTree WFSValueTreeState::getInputSamplerSet (int channelIndex, int setIndex)
{
    auto sampler = getInputSamplerSection (channelIndex);
    if (! sampler.isValid() || setIndex < 0)
        return {};

    // Ordinal, not id: SamplerSubTab::onDeleteSet removes a child without
    // renumbering the rest, so the id property goes stale on the survivors.
    int seen = 0;
    for (int i = 0; i < sampler.getNumChildren(); ++i)
    {
        auto child = sampler.getChild (i);
        if (! child.hasType (SamplerSet))
            continue;
        if (seen == setIndex)
            return child;
        ++seen;
    }
    return {};
}

int WFSValueTreeState::getNumInputSamplerSets (int channelIndex)
{
    auto sampler = getInputSamplerSection (channelIndex);
    if (! sampler.isValid())
        return 0;

    int setCount = 0;
    for (int i = 0; i < sampler.getNumChildren(); ++i)
        if (sampler.getChild (i).hasType (SamplerSet))
            ++setCount;
    return setCount;
}

juce::ValueTree WFSValueTreeState::addInputSamplerSet (int channelIndex, const juce::String& setName)
{
    auto sampler = ensureInputSamplerSection (channelIndex);
    if (! sampler.isValid())
        return {};

    const int existing = getNumInputSamplerSets (channelIndex);
    if (existing >= maxSamplerSets)
        return {};

    // Defaults read off a default-constructed SamplerSet rather than written out
    // here, so this and SamplerSubTab::saveSetToValueTree cannot disagree about
    // what a new set looks like.
    const SamplerData::SamplerSet defaults;

    juce::ValueTree set (SamplerSet);
    set.setProperty (id, existing, nullptr);
    set.setProperty (samplerSetName, setName.isNotEmpty() ? setName
                                                          : "Set " + juce::String (existing + 1), nullptr);
    set.setProperty (samplerSetPlayMode, defaults.playMode, nullptr);
    set.setProperty (samplerSetCells,    juce::String(), nullptr);
    set.setProperty (samplerSetPosX,     defaults.posX, nullptr);
    set.setProperty (samplerSetPosY,     defaults.posY, nullptr);
    set.setProperty (samplerSetPosZ,     defaults.posZ, nullptr);
    set.setProperty (samplerSetLevel,    defaults.level, nullptr);

    set.setProperty (samplerSetPressLevelEnabled, defaults.pressLevel.enabled ? 1 : 0, nullptr);
    set.setProperty (samplerSetPressLevelDir,     defaults.pressLevel.direction, nullptr);
    set.setProperty (samplerSetPressLevelCurve,   defaults.pressLevel.curve, nullptr);

    set.setProperty (samplerSetPressZEnabled, defaults.pressZ.enabled ? 1 : 0, nullptr);
    set.setProperty (samplerSetPressZDir,     defaults.pressZ.direction, nullptr);
    set.setProperty (samplerSetPressZCurve,   defaults.pressZ.curve, nullptr);

    set.setProperty (samplerSetPressHFEnabled, defaults.pressHF.enabled ? 1 : 0, nullptr);
    set.setProperty (samplerSetPressHFDir,     defaults.pressHF.direction, nullptr);
    set.setProperty (samplerSetPressHFCurve,   defaults.pressHF.curve, nullptr);

    set.setProperty (samplerSetPressXYEnabled, defaults.pressXYEnabled ? 1 : 0, nullptr);
    set.setProperty (samplerSetPressXYScale,   defaults.pressXYScale, nullptr);

    sampler.appendChild (set, nullptr);
    return set;
}

bool WFSValueTreeState::removeInputSamplerSet (int channelIndex, int setIndex)
{
    auto set = getInputSamplerSet (channelIndex, setIndex);
    if (! set.isValid())
        return false;

    auto sampler = getInputSamplerSection (channelIndex);
    sampler.removeChild (set, nullptr);
    return true;
}

juce::ValueTree WFSValueTreeState::getInputGradientShape (int channelIndex, int layerIndex, int shapeIndex)
{
    auto layer = getInputGradientLayer (channelIndex, layerIndex);
    if (! layer.isValid() || shapeIndex < 0)
        return {};

    // Shapes are identified by position, not by an id property: GradientShape
    // carries no id, and the editor re-sorts by gmShapeOrder when it loads a
    // layer. Count only GradientShape children, so a layer that ever grows
    // another kind of child does not shift the numbering.
    int seen = 0;
    for (int i = 0; i < layer.getNumChildren(); ++i)
    {
        auto child = layer.getChild (i);
        if (! child.hasType (GradientShape))
            continue;
        if (seen == shapeIndex)
            return child;
        ++seen;
    }
    return {};
}

juce::ValueTree WFSValueTreeState::getInputGradientLayer (int channelIndex, int layerIndex)
{
    auto gm = getInputGradientMapsSection (channelIndex);
    if (! gm.isValid() || layerIndex < 0 || layerIndex >= gm.getNumChildren())
        return {};

    return gm.getChild (layerIndex);
}

juce::ValueTree WFSValueTreeState::ensureInputGradientMapsSection (int channelIndex)
{
    auto input = getInputState (channelIndex);
    if (! input.isValid())
        return {};

    auto gm = input.getChildWithName (GradientMaps);
    if (! gm.isValid())
    {
        gm = createInputGradientMapsSection();
        input.appendChild (gm, nullptr);
    }
    return gm;
}

juce::ValueTree WFSValueTreeState::getInputSamplerSection (int channelIndex)
{
    return getInputState (channelIndex).getChildWithName (Sampler);
}

juce::ValueTree WFSValueTreeState::ensureInputSamplerSection (int channelIndex)
{
    auto input = getInputState (channelIndex);
    if (! input.isValid())
        return {};

    auto sampler = input.getChildWithName (Sampler);
    if (! sampler.isValid())
    {
        sampler = createInputSamplerSection();
        input.appendChild (sampler, nullptr);
    }
    return sampler;
}

//==============================================================================
// Output Channel Access
//==============================================================================

juce::var WFSValueTreeState::getOutputParameter (int channelIndex, const juce::Identifier& paramId) const
{
    auto output = const_cast<WFSValueTreeState*>(this)->getOutputState (channelIndex);
    if (!output.isValid())
        return {};

    // Search through all subsections
    for (int i = 0; i < output.getNumChildren(); ++i)
    {
        auto child = output.getChild (i);
        if (child.hasProperty (paramId))
            return child.getProperty (paramId);
    }
    return {};
}

void WFSValueTreeState::setOutputParameter (int channelIndex, const juce::Identifier& paramId, const juce::var& value)
{
    auto output = getOutputState (channelIndex);
    if (!output.isValid())
        return;

    // Search through all subsections
    for (int i = 0; i < output.getNumChildren(); ++i)
    {
        auto child = output.getChild (i);
        if (child.hasProperty (paramId))
        {
            writeProperty (child, paramId, value, getActiveUndoManager());

            // Speaker positions are never auto-placed, but editing one still
            // signals the user is positioning things (see the input twin).
            if (paramId == outputPositionX || paramId == outputPositionY || paramId == outputPositionZ)
                markPositionsUserOwned();

            return;
        }
    }

    // Property not found - add it to the appropriate section if we know where it belongs
    // This handles old config files that may be missing newer properties
    if (paramId == outputCoordinateMode)
    {
        auto position = getOutputPositionSection (channelIndex);
        if (position.isValid())
            writeProperty (position, paramId, value, getActiveUndoManager());
    }
}

void WFSValueTreeState::setOutputParameterDirect (int channelIndex, const juce::Identifier& paramId, const juce::var& value)
{
    auto output = getOutputState (channelIndex);
    if (!output.isValid())
        return;

    // Search through all subsections (but skip EQ bands)
    for (int i = 0; i < output.getNumChildren(); ++i)
    {
        auto child = output.getChild (i);
        if (child.hasProperty (paramId))
        {
            writeProperty (child, paramId, value, getActiveUndoManager());
            return;
        }
    }
}

void WFSValueTreeState::setOutputEQBandParameterDirect (int channelIndex, int bandIndex, const juce::Identifier& paramId, const juce::var& value)
{
    auto band = getOutputEQBand (channelIndex, bandIndex);
    if (band.isValid())
        writeProperty (band, paramId, value, getActiveUndoManager());
}

bool WFSValueTreeState::isArrayLinkedParameter (const juce::Identifier& paramId)
{
    // Parameters that should propagate to array members
    return paramId == outputAttenuation ||
           paramId == outputDelayLatency ||
           paramId == outputOrientation ||
           paramId == outputAngleOn ||
           paramId == outputAngleOff ||
           paramId == outputPitch ||
           paramId == outputHFdamping ||
           paramId == outputMiniLatencyEnable ||
           paramId == outputLSattenEnable ||
           paramId == outputFRenable ||
           paramId == outputDistanceAttenPercent ||
           paramId == outputHparallax ||
           paramId == outputVparallax ||
           paramId == outputEQenabled;
}

bool WFSValueTreeState::isArrayLinkedEQParameter (const juce::Identifier& paramId)
{
    // EQ band parameters that should propagate to array members
    return paramId == eqShape ||
           paramId == eqFrequency ||
           paramId == eqGain ||
           paramId == eqQ ||
           paramId == eqSlope;
}

bool WFSValueTreeState::isBooleanOutputParameter (const juce::Identifier& paramId)
{
    // On/off toggles: a "relative" delta is meaningless for these, so array
    // propagation must always share the absolute state (see propagation below).
    return paramId == outputMiniLatencyEnable ||
           paramId == outputLSattenEnable ||
           paramId == outputFRenable ||
           paramId == outputEQenabled;
}

float WFSValueTreeState::clampOutputParamToRange (const juce::Identifier& paramId, float value)
{
    using namespace WFSParameterDefaults;

    if (paramId == outputAttenuation)
        return juce::jlimit (outputAttenuationMin, outputAttenuationMax, value);
    if (paramId == outputDelayLatency)
        return juce::jlimit (outputDelayLatencyMin, outputDelayLatencyMax, value);
    if (paramId == outputOrientation)
        return juce::jlimit (static_cast<float> (outputOrientationMin), static_cast<float> (outputOrientationMax), value);
    if (paramId == outputAngleOn)
        return juce::jlimit (static_cast<float> (outputAngleOnMin), static_cast<float> (outputAngleOnMax), value);
    if (paramId == outputAngleOff)
        return juce::jlimit (static_cast<float> (outputAngleOffMin), static_cast<float> (outputAngleOffMax), value);
    if (paramId == outputPitch)
        return juce::jlimit (static_cast<float> (outputPitchMin), static_cast<float> (outputPitchMax), value);
    if (paramId == outputHFdamping)
        return juce::jlimit (outputHFdampingMin, outputHFdampingMax, value);
    if (paramId == outputDistanceAttenPercent)
        return juce::jlimit (static_cast<float> (outputDistanceAttenPercentMin), static_cast<float> (outputDistanceAttenPercentMax), value);
    if (paramId == outputHparallax)
        return juce::jlimit (outputHparallaxMin, outputHparallaxMax, value);
    if (paramId == outputVparallax)
        return juce::jlimit (outputVparallaxMin, outputVparallaxMax, value);

    // EQ parameters
    if (paramId == eqFrequency)
        return juce::jlimit (eqFrequencyMin, eqFrequencyMax, value);
    if (paramId == eqGain)
        return juce::jlimit (eqGainMin, eqGainMax, value);
    if (paramId == eqQ)
        return juce::jlimit (eqQMin, eqQMax, value);
    if (paramId == eqSlope)
        return juce::jlimit (eqSlopeMin, eqSlopeMax, value);
    if (paramId == eqShape)
        return juce::jlimit (static_cast<float> (eqShapeMin), static_cast<float> (eqShapeMax), value);

    // Boolean/toggle parameters (0 or 1)
    if (paramId == outputMiniLatencyEnable || paramId == outputLSattenEnable ||
        paramId == outputFRenable || paramId == outputEQenabled)
        return value != 0.0f ? 1.0f : 0.0f;

    return value;
}

void WFSValueTreeState::setOutputParameterWithArrayPropagation (int channelIndex,
                                                                 const juce::Identifier& paramId,
                                                                 const juce::var& value,
                                                                 bool propagateToArray)
{
    // Check if this is an array-linked parameter
    if (!propagateToArray || !isArrayLinkedParameter (paramId))
    {
        setOutputParameter (channelIndex, paramId, value);
        return;
    }

    // Get array assignment for this output
    int arrayId = static_cast<int> (getOutputParameter (channelIndex, outputArray));
    if (arrayId == 0)  // Single, not in array
    {
        setOutputParameter (channelIndex, paramId, value);
        return;
    }

    // Get apply mode for this output
    int applyMode = static_cast<int> (getOutputParameter (channelIndex, outputApplyToArray));
    if (applyMode == 0)  // OFF
    {
        setOutputParameter (channelIndex, paramId, value);
        return;
    }

    // Get old value for RELATIVE mode delta calculation
    auto oldValue = getOutputParameter (channelIndex, paramId);
    float oldFloat = static_cast<float> (oldValue);
    float newFloat = static_cast<float> (value);
    float delta = newFloat - oldFloat;

    // Set the originating channel
    setOutputParameter (channelIndex, paramId, value);

    // Propagate to array members
    int numOutputs = getNumOutputChannels();
    for (int i = 0; i < numOutputs; ++i)
    {
        if (i == channelIndex)
            continue;  // Skip originating channel

        // Check if this output is in the same array
        int memberArray = static_cast<int> (getOutputParameter (i, outputArray));
        if (memberArray != arrayId)
            continue;

        // Check member's apply mode (per-output unlinking)
        int memberApplyMode = static_cast<int> (getOutputParameter (i, outputApplyToArray));
        if (memberApplyMode == 0)  // This member is unlinked (OFF)
            continue;

        // Absolute value when BOTH source and member are ABSOLUTE, or for on/off
        // toggles in any mode — a toggle has no meaningful relative offset, so a
        // delta would invert already-matching members instead of sharing the state.
        // Otherwise apply the delta (RELATIVE source, or ABSOLUTE source → RELATIVE member).
        if ((applyMode == 1 && memberApplyMode == 1) || isBooleanOutputParameter (paramId))
        {
            setOutputParameterDirect (i, paramId, value);
        }
        else
        {
            float memberCurrent = static_cast<float> (getOutputParameter (i, paramId));
            float memberNew = clampOutputParamToRange (paramId, memberCurrent + delta);

            // For int parameters, round the result (toggles never reach this branch)
            if (paramId == outputOrientation || paramId == outputAngleOn ||
                paramId == outputAngleOff || paramId == outputPitch ||
                paramId == outputDistanceAttenPercent)
            {
                setOutputParameterDirect (i, paramId, static_cast<int> (std::round (memberNew)));
            }
            else
            {
                setOutputParameterDirect (i, paramId, memberNew);
            }
        }
    }
}

void WFSValueTreeState::setOutputEQBandParameterWithArrayPropagation (int channelIndex,
                                                                       int bandIndex,
                                                                       const juce::Identifier& paramId,
                                                                       const juce::var& value,
                                                                       bool propagateToArray)
{
    // Check if this is an array-linked EQ parameter
    if (!propagateToArray || !isArrayLinkedEQParameter (paramId))
    {
        auto band = getOutputEQBand (channelIndex, bandIndex);
        if (band.isValid())
            writeProperty (band, paramId, value, getActiveUndoManager());
        return;
    }

    // Get array assignment for this output
    int arrayId = static_cast<int> (getOutputParameter (channelIndex, outputArray));
    if (arrayId == 0)  // Single, not in array
    {
        auto band = getOutputEQBand (channelIndex, bandIndex);
        if (band.isValid())
            writeProperty (band, paramId, value, getActiveUndoManager());
        return;
    }

    // Get apply mode for this output
    int applyMode = static_cast<int> (getOutputParameter (channelIndex, outputApplyToArray));
    if (applyMode == 0)  // OFF
    {
        auto band = getOutputEQBand (channelIndex, bandIndex);
        if (band.isValid())
            writeProperty (band, paramId, value, getActiveUndoManager());
        return;
    }

    // Get old value for RELATIVE mode delta calculation
    auto band = getOutputEQBand (channelIndex, bandIndex);
    if (!band.isValid())
        return;

    float oldFloat = static_cast<float> (band.getProperty (paramId));
    float newFloat = static_cast<float> (value);
    float delta = newFloat - oldFloat;

    // Set the originating channel's band
    writeProperty (band, paramId, value, getActiveUndoManager());

    // Propagate to array members
    int numOutputs = getNumOutputChannels();
    for (int i = 0; i < numOutputs; ++i)
    {
        if (i == channelIndex)
            continue;

        int memberArray = static_cast<int> (getOutputParameter (i, outputArray));
        if (memberArray != arrayId)
            continue;

        int memberApplyMode = static_cast<int> (getOutputParameter (i, outputApplyToArray));
        if (memberApplyMode == 0)
            continue;

        auto memberBand = getOutputEQBand (i, bandIndex);
        if (!memberBand.isValid())
            continue;

        // Absolute value only when BOTH source and member are ABSOLUTE;
        // otherwise apply the delta
        if (applyMode == 1 && memberApplyMode == 1)
        {
            setOutputEQBandParameterDirect (i, bandIndex, paramId, value);
        }
        else
        {
            float memberCurrent = static_cast<float> (memberBand.getProperty (paramId));
            float memberNew = clampOutputParamToRange (paramId, memberCurrent + delta);

            // For eqShape (int), round the result
            if (paramId == eqShape)
                setOutputEQBandParameterDirect (i, bandIndex, paramId, static_cast<int> (std::round (memberNew)));
            else
                setOutputEQBandParameterDirect (i, bandIndex, paramId, memberNew);
        }
    }
}

juce::ValueTree WFSValueTreeState::getOutputChannelSection (int channelIndex)
{
    return getOutputState (channelIndex).getChildWithName (Channel);
}

juce::ValueTree WFSValueTreeState::getOutputPositionSection (int channelIndex)
{
    return getOutputState (channelIndex).getChildWithName (Position);
}

juce::ValueTree WFSValueTreeState::getOutputOptionsSection (int channelIndex)
{
    return getOutputState (channelIndex).getChildWithName (Options);
}

juce::ValueTree WFSValueTreeState::getOutputEQSection (int channelIndex)
{
    return getOutputState (channelIndex).getChildWithName (EQ);
}

juce::ValueTree WFSValueTreeState::getOutputEQBand (int channelIndex, int bandIndex)
{
    auto eq = getOutputEQSection (channelIndex);
    if (eq.isValid() && bandIndex >= 0 && bandIndex < eq.getNumChildren())
        return eq.getChild (bandIndex);
    return {};
}

//==============================================================================
// Reverb Channel Access
//==============================================================================

juce::var WFSValueTreeState::getReverbParameter (int channelIndex, const juce::Identifier& paramId) const
{
    auto reverb = const_cast<WFSValueTreeState*>(this)->getReverbState (channelIndex);
    if (!reverb.isValid())
        return {};

    // Search through all subsections
    for (int i = 0; i < reverb.getNumChildren(); ++i)
    {
        auto child = reverb.getChild (i);
        if (child.hasProperty (paramId))
            return child.getProperty (paramId);

        // Check EQ bands
        if (child.getType() == EQ)
        {
            for (int j = 0; j < child.getNumChildren(); ++j)
            {
                auto band = child.getChild (j);
                if (band.hasProperty (paramId))
                    return band.getProperty (paramId);
            }
        }
    }
    return {};
}

void WFSValueTreeState::setReverbParameter (int channelIndex, const juce::Identifier& paramId, const juce::var& value)
{
    auto reverb = getReverbState (channelIndex);
    if (!reverb.isValid())
        return;

    // Search through all subsections
    for (int i = 0; i < reverb.getNumChildren(); ++i)
    {
        auto child = reverb.getChild (i);
        if (child.hasProperty (paramId))
        {
            writeProperty (child, paramId, value, getActiveUndoManager());

            // Node or return position edits latch user ownership of positions
            // (see the input twin). Return offsets count: they are where the
            // reverb is perceived, edited from the same tab.
            if (paramId == reverbPositionX     || paramId == reverbPositionY     || paramId == reverbPositionZ
             || paramId == reverbReturnOffsetX || paramId == reverbReturnOffsetY || paramId == reverbReturnOffsetZ)
                markPositionsUserOwned();

            return;
        }

        // Check EQ bands
        if (child.getType() == EQ)
        {
            for (int j = 0; j < child.getNumChildren(); ++j)
            {
                auto band = child.getChild (j);
                if (band.hasProperty (paramId))
                {
                    writeProperty (band, paramId, value, getActiveUndoManager());
                    return;
                }
            }
        }
    }

    // Property not found - add it to the appropriate section if we know where it belongs
    // This handles old config files that may be missing newer properties
    if (paramId == reverbCoordinateMode)
    {
        auto position = getReverbPositionSection (channelIndex);
        if (position.isValid())
            writeProperty (position, paramId, value, getActiveUndoManager());
    }
}

juce::ValueTree WFSValueTreeState::getReverbChannelSection (int channelIndex)
{
    return getReverbState (channelIndex).getChildWithName (Channel);
}

juce::ValueTree WFSValueTreeState::getReverbPositionSection (int channelIndex)
{
    return getReverbState (channelIndex).getChildWithName (Position);
}

juce::ValueTree WFSValueTreeState::getReverbFeedSection (int channelIndex)
{
    return getReverbState (channelIndex).getChildWithName (Feed);
}

juce::ValueTree WFSValueTreeState::getReverbEQSection (int channelIndex)
{
    return getReverbState (channelIndex).getChildWithName (EQ);
}

juce::ValueTree WFSValueTreeState::ensureReverbEQSection (int channelIndex)
{
    auto reverb = getReverbState (channelIndex);
    if (! reverb.isValid())
        return {};

    auto eq = reverb.getChildWithName (EQ);
    if (! eq.isValid())
    {
        // Create the EQ section if it doesn't exist (e.g., loading old config)
        eq = createReverbEQSection();
        reverb.appendChild (eq, nullptr);
    }
    else
    {
        // Migrate old property names: reverbEQ* -> reverbPreEQ*
        using namespace WFSParameterIDs;
        using namespace WFSParameterDefaults;
        static const juce::Identifier oldEQenable  ("reverbEQenable");
        static const juce::Identifier oldEQshape   ("reverbEQshape");
        static const juce::Identifier oldEQfreq    ("reverbEQfreq");
        static const juce::Identifier oldEQgain    ("reverbEQgain");
        static const juce::Identifier oldEQq       ("reverbEQq");
        static const juce::Identifier oldEQslope   ("reverbEQslope");

        if (eq.hasProperty (oldEQenable))
        {
            eq.setProperty (reverbPreEQenable, eq.getProperty (oldEQenable), nullptr);
            eq.removeProperty (oldEQenable, nullptr);
        }

        for (int i = 0; i < eq.getNumChildren(); ++i)
        {
            auto band = eq.getChild (i);
            if (band.hasProperty (oldEQshape))
            {
                band.setProperty (reverbPreEQshape, band.getProperty (oldEQshape), nullptr);
                band.removeProperty (oldEQshape, nullptr);
            }
            if (band.hasProperty (oldEQfreq))
            {
                band.setProperty (reverbPreEQfreq, band.getProperty (oldEQfreq), nullptr);
                band.removeProperty (oldEQfreq, nullptr);
            }
            if (band.hasProperty (oldEQgain))
            {
                band.setProperty (reverbPreEQgain, band.getProperty (oldEQgain), nullptr);
                band.removeProperty (oldEQgain, nullptr);
            }
            if (band.hasProperty (oldEQq))
            {
                band.setProperty (reverbPreEQq, band.getProperty (oldEQq), nullptr);
                band.removeProperty (oldEQq, nullptr);
            }
            if (band.hasProperty (oldEQslope))
            {
                band.setProperty (reverbPreEQslope, band.getProperty (oldEQslope), nullptr);
                band.removeProperty (oldEQslope, nullptr);
            }
        }
    }
    return eq;
}

juce::ValueTree WFSValueTreeState::getReverbEQBand (int channelIndex, int bandIndex)
{
    auto eq = getReverbEQSection (channelIndex);
    if (eq.isValid() && bandIndex >= 0 && bandIndex < eq.getNumChildren())
        return eq.getChild (bandIndex);
    return {};
}

juce::ValueTree WFSValueTreeState::getReverbReturnSection (int channelIndex)
{
    return getReverbState (channelIndex).getChildWithName (ReverbReturn);
}

juce::ValueTree WFSValueTreeState::getReverbAlgorithmSection()
{
    return getReverbsState().getChildWithName (ReverbAlgorithm);
}

juce::ValueTree WFSValueTreeState::ensureReverbAlgorithmSection()
{
    auto reverbs = getReverbsState();
    if (! reverbs.isValid())
        return {};

    auto algo = reverbs.getChildWithName (ReverbAlgorithm);
    if (! algo.isValid())
    {
        // Create the algorithm section if it doesn't exist (e.g., loading old config)
        algo = createReverbAlgorithmSection();
        reverbs.appendChild (algo, nullptr);
    }
    return algo;
}

juce::ValueTree WFSValueTreeState::getReverbPostEQSection()
{
    return getReverbsState().getChildWithName (ReverbPostEQ);
}

juce::ValueTree WFSValueTreeState::ensureReverbPostEQSection()
{
    auto reverbs = getReverbsState();
    if (! reverbs.isValid())
        return {};

    auto postEQ = reverbs.getChildWithName (ReverbPostEQ);
    if (! postEQ.isValid())
    {
        // Create the post-EQ section if it doesn't exist (e.g., loading old config)
        postEQ = createReverbPostEQSection();
        reverbs.appendChild (postEQ, nullptr);
    }
    return postEQ;
}

juce::ValueTree WFSValueTreeState::getReverbPostEQBand (int bandIndex)
{
    auto postEQ = getReverbPostEQSection();
    if (postEQ.isValid() && bandIndex >= 0 && bandIndex < postEQ.getNumChildren())
        return postEQ.getChild (bandIndex);
    return {};
}

juce::ValueTree WFSValueTreeState::getReverbPreCompSection()
{
    return getReverbsState().getChildWithName (ReverbPreComp);
}

juce::ValueTree WFSValueTreeState::ensureReverbPreCompSection()
{
    auto reverbs = getReverbsState();
    if (! reverbs.isValid())
        return {};

    auto preComp = reverbs.getChildWithName (ReverbPreComp);
    if (! preComp.isValid())
    {
        preComp = createReverbPreCompSection();
        reverbs.appendChild (preComp, nullptr);
    }
    return preComp;
}

juce::ValueTree WFSValueTreeState::getReverbPostExpSection()
{
    return getReverbsState().getChildWithName (ReverbPostExp);
}

juce::ValueTree WFSValueTreeState::ensureReverbPostExpSection()
{
    auto reverbs = getReverbsState();
    if (! reverbs.isValid())
        return {};

    auto postExp = reverbs.getChildWithName (ReverbPostExp);
    if (! postExp.isValid())
    {
        postExp = createReverbPostExpSection();
        reverbs.appendChild (postExp, nullptr);
    }
    return postExp;
}

//==============================================================================
// Cluster Access
//==============================================================================

juce::var WFSValueTreeState::getClusterParameter (int clusterIndex, const juce::Identifier& paramId) const
{
    auto cluster = const_cast<WFSValueTreeState*>(this)->getClusterState (clusterIndex);
    if (cluster.isValid() && cluster.hasProperty (paramId))
        return cluster.getProperty (paramId);
    return {};
}

void WFSValueTreeState::setClusterParameter (int clusterIndex, const juce::Identifier& paramId, const juce::var& value)
{
    auto cluster = getClusterState (clusterIndex);
    if (cluster.isValid())
        writeProperty (cluster, paramId, value, getActiveUndoManager());
}

//==============================================================================
// Binaural Enable/Solo Access
//==============================================================================

bool WFSValueTreeState::getBinauralEnabled() const
{
    JUCE_ASSERT_MESSAGE_THREAD  // ValueTree reads are message-thread only; RT threads use BinauralCalculationEngine's RtParams snapshot
    auto binaural = getBinauralState();
    if (binaural.isValid())
        return (bool) binaural.getProperty (binauralEnabled, binauralEnabledDefault);
    return binauralEnabledDefault;
}

void WFSValueTreeState::setBinauralEnabled (bool isEnabled)
{
    auto binaural = getBinauralState();
    if (binaural.isValid())
        binaural.setProperty (binauralEnabled, isEnabled, getActiveUndoManager());
}

int WFSValueTreeState::getBinauralSoloMode() const
{
    JUCE_ASSERT_MESSAGE_THREAD  // see getBinauralEnabled()
    auto binaural = getBinauralState();
    if (binaural.isValid())
        return (int) binaural.getProperty (binauralSoloMode, binauralSoloModeDefault);
    return binauralSoloModeDefault;
}

void WFSValueTreeState::setBinauralSoloMode (int mode)
{
    auto binaural = getBinauralState();
    if (binaural.isValid())
        binaural.setProperty (binauralSoloMode, mode, getActiveUndoManager());
}

// Solo lives ON the channel (Channel.inputSolo) so it travels with the node:
// the legacy Binaural.inputSoloStates csv was positional and silently followed
// the wrong channels across insertions/reorders/deletions.

bool WFSValueTreeState::isInputSoloed (int inputIndex) const
{
    JUCE_ASSERT_MESSAGE_THREAD  // ValueTree read — message thread only
    auto input = const_cast<WFSValueTreeState*> (this)->getInputState (inputIndex);
    auto channel = input.getChildWithName (Channel);
    return channel.isValid() && static_cast<int> (channel.getProperty (inputSolo, 0)) != 0;
}

void WFSValueTreeState::setInputSoloed (int inputIndex, bool soloed)
{
    auto input = getInputState (inputIndex);
    auto channel = input.getChildWithName (Channel);
    if (! channel.isValid())
        return;

    // In Single mode, clear all other solos first
    if (soloed && getBinauralSoloMode() == 0)
        clearAllSoloStates();

    channel.setProperty (inputSolo, soloed ? 1 : 0, getActiveUndoManager());
}

void WFSValueTreeState::clearAllSoloStates()
{
    auto inputs = getInputsState();
    for (int i = 0; i < inputs.getNumChildren(); ++i)
    {
        auto channel = inputs.getChild (i).getChildWithName (Channel);
        if (channel.isValid()
            && static_cast<int> (channel.getProperty (inputSolo, 0)) != 0)
            channel.setProperty (inputSolo, 0, getActiveUndoManager());
    }
}

int WFSValueTreeState::getNumSoloedInputs() const
{
    JUCE_ASSERT_MESSAGE_THREAD  // ValueTree read — message thread only
    int soloCount = 0;
    for (int i = 0; i < getNumInputChannels(); ++i)
        if (isInputSoloed (i))
            ++soloCount;
    return soloCount;
}

int WFSValueTreeState::getBinauralOutputChannel() const
{
    JUCE_ASSERT_MESSAGE_THREAD  // audio callback uses BinauralCalculationEngine::getBinauralOutputChannel() (atomic) instead
    auto binaural = getBinauralState();
    if (binaural.isValid())
        return (int) binaural.getProperty (binauralOutputChannel, binauralOutputChannelDefault);
    return binauralOutputChannelDefault;
}

void WFSValueTreeState::setBinauralOutputChannel (int channel)
{
    auto binaural = getBinauralState();
    if (binaural.isValid())
        binaural.setProperty (binauralOutputChannel, channel, getActiveUndoManager());
}

//==============================================================================
// Network Target Access
//==============================================================================

int WFSValueTreeState::getNumNetworkTargets() const
{
    auto network = const_cast<WFSValueTreeState*>(this)->getNetworkState();
    return network.getNumChildren();
}

void WFSValueTreeState::addNetworkTarget()
{
    auto network = getNetworkState();
    if (network.getNumChildren() < maxNetworkTargets)
    {
        auto target = createDefaultNetworkTarget (network.getNumChildren());
        network.appendChild (target, getActiveUndoManager());
    }
}

void WFSValueTreeState::removeNetworkTarget (int targetIndex)
{
    auto network = getNetworkState();
    if (targetIndex >= 0 && targetIndex < network.getNumChildren())
        network.removeChild (targetIndex, getActiveUndoManager());
}

juce::ValueTree WFSValueTreeState::getNetworkTargetState (int targetIndex)
{
    auto network = getNetworkState();
    if (targetIndex >= 0 && targetIndex < network.getNumChildren())
        return network.getChild (targetIndex);
    return {};
}

//==============================================================================
// Channel Management
//==============================================================================

int WFSValueTreeState::getNumInputChannels() const
{
    return const_cast<WFSValueTreeState*>(this)->getInputsState().getNumChildren();
}

int WFSValueTreeState::getNumStereoInputChannels() const
{
    // Derived from the per-channel type — the legacy IO property is read only
    // by migration (and removed from the tree afterwards).
    auto inputs = getInputsState();
    int stereo = 0;
    for (int i = 0; i < inputs.getNumChildren(); ++i)
        if (isInputChannelStereo (i))
            ++stereo;
    return stereo;
}

int WFSValueTreeState::getInputChannelNumber (int slot) const
{
    auto inputs = getInputsState();
    if (slot < 0 || slot >= inputs.getNumChildren())
        return 0;
    return static_cast<int> (inputs.getChild (slot).getProperty (id, 0));
}

int WFSValueTreeState::getSlotForChannelNumber (int number) const
{
    auto inputs = getInputsState();
    const int n = inputs.getNumChildren();
    if (number <= 0 || n == 0)
        return -1;

    // Fast path: dense, un-reordered list — slot is number - 1.
    if (number <= n
        && static_cast<int> (inputs.getChild (number - 1).getProperty (id, 0)) == number)
        return number - 1;

    // Tree order is the user's DISPLAY order (drag-to-reorder moves nodes),
    // so numbers carry no ordering guarantee — linear scan (n <= 64; runs on
    // the OSC/tracking ingress path, still trivially cheap).
    for (int i = 0; i < n; ++i)
        if (static_cast<int> (inputs.getChild (i).getProperty (id, 0)) == number)
            return i;
    return -1;
}

bool WFSValueTreeState::isInputChannelStereo (int slot) const
{
    auto inputs = getInputsState();
    if (slot < 0 || slot >= inputs.getNumChildren())
        return false;
    return inputs.getChild (slot).getProperty (inputChannelType).toString()
             == inputChannelTypeStereo;
}

int WFSValueTreeState::getHighestChannelNumber() const
{
    // Tree order is display order (drag-to-reorder), so scan for the max.
    auto inputs = getInputsState();
    int highest = 0;
    for (int i = 0; i < inputs.getNumChildren(); ++i)
        highest = juce::jmax (highest,
                              static_cast<int> (inputs.getChild (i).getProperty (id, 0)));
    return highest;
}

int WFSValueTreeState::getNextChannelNumber() const
{
    return getHighestChannelNumber() + 1;
}

void WFSValueTreeState::stampChannelTypesFromLegacySplit (juce::UndoManager* um, int stereoCountOverride)
{
    // Migration-only: stamps the pre-rework tail split ("the LAST N channels
    // are stereo") onto the per-channel property. The caller reads N from the
    // legacy IO property — getNumStereoInputChannels() is derived from the
    // types and cannot be used here.
    jassert (stereoCountOverride >= 0);

    auto inputs = getInputsState();
    const int total  = inputs.getNumChildren();
    const int stereo = juce::jmin (juce::jmax (0, stereoCountOverride), total);

    for (int i = 0; i < total; ++i)
    {
        auto input = inputs.getChild (i);
        const juce::String wanted = (i >= total - stereo) ? inputChannelTypeStereo
                                                          : inputChannelTypeMono;
        if (input.getProperty (inputChannelType).toString() != wanted)
            input.setProperty (inputChannelType, wanted, um);
    }
}

void WFSValueTreeState::migrateInputChannelModel()
{
    auto inputs = getInputsState();
    if (! inputs.isValid())
        return;

    const int total = inputs.getNumChildren();
    if (total == 0)
        return;

    // 1. Repair ids: every channel must carry a unique positive number.
    //    Well-formed files are dense 1..N in order; a hand-edited file with
    //    missing or duplicate ids gets one dense renumber here — the last
    //    renumbering that can ever happen to it.
    {
        juce::SortedSet<int> seen;
        bool idsValid = true;
        for (int i = 0; i < total && idsValid; ++i)
        {
            const int number = static_cast<int> (inputs.getChild (i).getProperty (id, 0));
            if (number <= 0 || seen.contains (number))
                idsValid = false;
            else
                seen.add (number);
        }
        if (! idsValid)
            for (int i = 0; i < total; ++i)
                inputs.getChild (i).setProperty (id, i + 1, nullptr);
    }

    // (Tree order is preserved as-is: it is the user's saved display order —
    // drag-to-reorder moves nodes, numbers stay put.)

    // 2. Stamp types for files that predate the per-channel property — but
    //    only when the WHOLE list lacks it: a partially-typed list is
    //    post-rework data whose missing entries default to mono.
    bool anyTyped = false;
    for (int i = 0; i < total && ! anyTyped; ++i)
        anyTyped = inputs.getChild (i).hasProperty (inputChannelType);

    auto io = getIOState();
    if (! anyTyped)
    {
        const int legacyStereo = io.isValid()
            ? juce::jlimit (0, WFSParameterDefaults::maxStereoChannels,
                            (int) io.getProperty (WFSParameterIDs::stereoInputChannels, 0))
            : 0;
        stampChannelTypesFromLegacySplit (nullptr, legacyStereo);
    }
    else
    {
        for (int i = 0; i < total; ++i)
        {
            auto input = inputs.getChild (i);
            if (! input.hasProperty (inputChannelType))
                input.setProperty (inputChannelType, inputChannelTypeMono, nullptr);
        }
    }

    // 3. The legacy tail-split property is consumed; drop it so nothing can
    //    resurrect the count-based semantics from a stale file value.
    if (io.isValid() && io.hasProperty (WFSParameterIDs::stereoInputChannels))
        io.removeProperty (WFSParameterIDs::stereoInputChannels, nullptr);

    // 4. Solo: the legacy Binaural.inputSoloStates csv is positional (index =
    //    slot); convert to the per-channel Channel.inputSolo property so the
    //    state travels with the node, then drop the csv.
    auto binaural = getBinauralState();
    if (binaural.isValid() && binaural.hasProperty (WFSParameterIDs::inputSoloStates))
    {
        juce::StringArray states;
        states.addTokens (binaural.getProperty (WFSParameterIDs::inputSoloStates, "").toString(), ",", "");
        for (int i = 0; i < juce::jmin (states.size(), total); ++i)
        {
            auto channel = inputs.getChild (i).getChildWithName (Channel);
            if (channel.isValid() && states[i] == "1")
                channel.setProperty (inputSolo, 1, nullptr);
        }
        binaural.removeProperty (WFSParameterIDs::inputSoloStates, nullptr);
    }
}

void WFSValueTreeState::compactChannelNumbersToDisplayOrder()
{
    auto inputs = getInputsState();
    if (! inputs.isValid())
        return;

    // Ascending, id written BEFORE the tracking id: mid-walk the list can
    // briefly hold the same number twice (slot s takes s + 1 while the node that
    // still owns s + 1 has not been visited yet). Harmless —
    // getSlotForChannelNumber's fast path resolves a NEW number to the node just
    // renumbered, so the synchronous valueTreePropertyChanged fired by the
    // tracking-id write below still reports the right slot.
    const int total = inputs.getNumChildren();
    for (int slot = 0; slot < total; ++slot)
    {
        if (getInputChannelNumber (slot) != slot + 1)
            setInputChannelNumberAtSlot (slot, slot + 1);
    }
}

void WFSValueTreeState::setInputChannelNumberAtSlot (int slot, int newNumber)
{
    auto input = getInputsState().getChild (slot);
    if (! input.isValid())
        return;
    const int oldNumber = getInputChannelNumber (slot);
    if (oldNumber == newNumber)
        return;

    // Direct setProperty throughout, never setParameter/setInputParam: a
    // renumber is bookkeeping, and those wrappers carry undo entries,
    // dirty-tracking and ownership latches that must not fire for it.
    input.setProperty (id, newNumber, nullptr);

    // The tracking id, which createDefaultInputChannel stamps from the
    // number, follows it only while it still matches — a tracker mapping the
    // user has pointed elsewhere stays put. (The name is NOT the number's to
    // move: resequenceDefaultInputNames owns it.)
    auto position = input.getChildWithName (Position);
    if (position.isValid()
        && static_cast<int> (position.getProperty (inputTrackingID, 0)) == oldNumber)
        position.setProperty (inputTrackingID, newNumber, nullptr);
}

namespace
{
    // 1-based ordinal a default input name of the given shape carries ("Mono 3"
    // against "Mono" -> 3), or 0 when the name is not that shape. "Mono 0" and
    // "Mono 007" read as 0 and 7: the ordinal is what the name says, and only a
    // positive one is a name the app could have stamped.
    int defaultInputNameOrdinal (const juce::String& candidate, juce::StringRef word)
    {
        const int space = candidate.lastIndexOfChar (' ');
        if (space <= 0 || candidate.substring (0, space) != word)
            return 0;

        const juce::String tail = candidate.substring (space + 1);
        if (tail.isEmpty() || ! tail.containsOnly ("0123456789"))
            return 0;
        return tail.getIntValue();
    }
}

void WFSValueTreeState::resequenceDefaultInputNames()
{
    auto inputs = getInputsState();
    if (! inputs.isValid())
        return;

    // Any n, not the one this channel happens to carry now: by the time this
    // runs the ordinals have already shifted under the names, so testing
    // against the single name a channel *would* have had would freeze every
    // default the reorder displaced.
    auto isDefaultName = [] (const juce::String& candidate) -> bool
    {
        return defaultInputNameOrdinal (candidate, "Input") > 0
            || defaultInputNameOrdinal (candidate, "Mono") > 0
            || defaultInputNameOrdinal (candidate, "Stereo") > 0;
    };

    int monoCount = 0, stereoCount = 0;
    const int total = inputs.getNumChildren();
    for (int slot = 0; slot < total; ++slot)
    {
        const bool stereo = isInputChannelStereo (slot);
        const int ordinal = stereo ? ++stereoCount : ++monoCount;

        auto channel = inputs.getChild (slot).getChildWithName (Channel);
        if (! channel.isValid())
            continue;

        const juce::String current = channel.getProperty (inputName).toString();
        if (! isDefaultName (current))
            continue;

        // Direct setProperty, never setInputParam: the app renaming its own
        // defaults must not push an undo entry, mark the project dirty or trip
        // an ownership latch.
        const juce::String renamed = getDefaultInputNameForType (stereo, ordinal);
        if (current != renamed)
            channel.setProperty (inputName, renamed, nullptr);
    }
}

void WFSValueTreeState::setInputChannelCounts (int numMono, int numStereo)
{
    // Two-count entry point (System Config fields, load shim). Under the
    // stable-number model it is a thin loop over the structural ops: additions
    // APPEND after the last channel — numbers never shift and mono and stereo
    // channels may interleave — and reductions remove the LAST channel of that
    // type in DISPLAY order. Not "the highest-numbered": the lambda walks slots
    // descending, and on a latched list that has been dragged those are
    // different channels. The bottom of the Arrange list is what the operator
    // can see, so it is the rule; predictInputChannelReduction mirrors it and
    // the dialog shows exactly that. Not undoable (the ops clear the undo
    // history; a half-undone tree/patch pair would silently desync).
    clampInputChannelCounts (numMono, numStereo);

    auto removeLastOfTypeInDisplayOrder = [this] (bool stereo) -> bool
    {
        auto inputs = getInputsState();
        for (int i = inputs.getNumChildren(); --i >= 0;)
            if (isInputChannelStereo (i) == stereo)
                return removeInputChannel (getInputChannelNumber (i)).wasOk();
        return false;
    };

    // Reductions first so the 64-live budget is free before additions.
    while (getNumStereoInputChannels() > numStereo)
        if (! removeLastOfTypeInDisplayOrder (true)) break;
    while (getNumInputChannels() - getNumStereoInputChannels() > numMono)
        if (! removeLastOfTypeInDisplayOrder (false)) break;
    while (getNumStereoInputChannels() < numStereo)
        if (! addInputChannel (true).wasOk()) break;
    while (getNumInputChannels() - getNumStereoInputChannels() < numMono)
        if (! addInputChannel (false).wasOk()) break;
}

int WFSValueTreeState::getLowestFreeChannelNumber() const
{
    for (int n = 1; n <= WFSParameterDefaults::maxInputChannels; ++n)
        if (getSlotForChannelNumber (n) < 0)
            return n;
    return 0;
}

juce::Result WFSValueTreeState::addInputChannel (bool stereo, int explicitNumber)
{
    auto inputs = getInputsState();
    const int total = inputs.getNumChildren();

    if (total >= WFSParameterDefaults::maxInputChannels)
        return juce::Result::fail ("input list is full ("
                                   + juce::String (WFSParameterDefaults::maxInputChannels)
                                   + " live channels)");
    if (stereo && getNumStereoInputChannels() >= WFSParameterDefaults::maxStereoChannels)
        return juce::Result::fail ("stereo budget reached ("
                                   + juce::String (WFSParameterDefaults::maxStereoChannels)
                                   + " stereo channels)");

    int number = explicitNumber;
    if (number <= 0)
    {
        // Append-only: the next number is highest + 1. Once 64 has been used,
        // the caller must explicitly pick a free (retired) number — the UI
        // confirms with the user first, because snapshots/cues addressed to
        // that number will affect the new channel. (An unlatched session never
        // reaches exhaustion with gaps: the tail renumber keeps the list dense.)
        number = getNextChannelNumber();
        if (number > WFSParameterDefaults::maxInputChannels)
            return juce::Result::fail ("channel number space exhausted; reuse a free number (lowest free: "
                                       + juce::String (getLowestFreeChannelNumber()) + ")");
    }
    else
    {
        if (number > WFSParameterDefaults::maxInputChannels)
            return juce::Result::fail ("channel number above "
                                       + juce::String (WFSParameterDefaults::maxInputChannels));
        if (getSlotForChannelNumber (number) >= 0)
            return juce::Result::fail ("channel number " + juce::String (number) + " is already live");
    }

    // New channels always land at the END of the display order (append-only);
    // the user drags them into place afterwards. This holds for gap-reuse
    // creation too — the recycled NUMBER does not dictate a position.
    // (Unlatched, the tail renumber then gives it the number of that display
    // position.)
    const int slot = total;

    auto node = createDefaultInputChannel (slot, total + 1, number);
    node.setProperty (inputChannelType,
                      stereo ? juce::String (inputChannelTypeStereo)
                             : juce::String (inputChannelTypeMono), nullptr);

    // The default name can only be built once the real type is known:
    // createDefaultInputChannel stamps mono. The ordinal is the highest one any
    // live name of that shape already claims, never the count of channels of
    // that type: a latched session never resequences, so after a delete — or
    // after a setInputChannelType flip, which renames nothing — count + 1 would
    // hand out a name a live channel still carries, permanently. On a dense list
    // of untouched defaults the two are the same value. Shape, not type, drives
    // the scan: a flipped channel keeps the name of the type it was born as.
    const juce::StringRef word = stereo ? "Stereo" : "Mono";
    int ordinal = stereo ? getNumStereoInputChannels()
                         : total - getNumStereoInputChannels();
    for (int i = 0; i < total; ++i)
    {
        auto channel = inputs.getChild (i).getChildWithName (Channel);
        if (channel.isValid())
            ordinal = juce::jmax (ordinal,
                                  defaultInputNameOrdinal (channel.getProperty (inputName).toString(),
                                                           word));
    }

    auto newChannel = node.getChildWithName (Channel);
    if (newChannel.isValid())
        newChannel.setProperty (inputName,
                                getDefaultInputNameForType (stereo, ordinal + 1), nullptr);

    inputs.addChild (node, slot, nullptr);

    insertInputPatchRow (slot, stereo);

    auto io = getIOState();
    if (io.isValid())
        io.setProperty (inputChannels, inputs.getNumChildren(), nullptr);
    inputs.setProperty (count, inputs.getNumChildren(), nullptr);

    if (! arePositionsUserOwned())
        redistributeAllInputPositions();

    // Fresh session: nothing outside the app can reference these numbers yet,
    // so the appended channel must read as its display position, not as
    // highest + 1 past an earlier gap. The patch has to read the same way — a
    // gapless diagonal in DISPLAY order, which the diagonal-continue row
    // inserted above cannot give: it appends past the globally highest patched
    // column, i.e. in creation order.
    //
    // The ORDER of the two calls does not matter; do not "fix" it later. The
    // patch re-flow is slot-keyed and reads only tree SHAPE
    // (getNumInputChannels, isInputChannelStereo), writing only patchData/rows;
    // the number compaction writes only `id` and `inputTrackingID` on <Input>
    // nodes and moves no child. They commute.
    if (! areChannelNumbersUserOwned())
    {
        compactChannelNumbersToDisplayOrder();
        compactInputPatchToDisplayOrder();
    }

    // Structural edits are not undoable: the channel node and its patch row
    // must live and die together, and ValueTree undo cannot span the flat
    // patchData string edit safely.
    clearAllUndoHistories();
    return juce::Result::ok();
}

juce::Result WFSValueTreeState::removeInputChannel (int channelNumber)
{
    const int slot = getSlotForChannelNumber (channelNumber);
    if (slot < 0)
        return juce::Result::fail ("channel " + juce::String (channelNumber) + " is not live");

    auto inputs = getInputsState();
    if (inputs.getNumChildren() <= 1)
        return juce::Result::fail ("at least one input channel is required");

    inputs.removeChild (slot, nullptr);
    removeInputPatchRow (slot);

    // Slot-keyed side state: drop the deleted slot, shift the ones above
    remapClusterInputOrders ([slot] (int s)
    {
        return s == slot ? -1 : (s > slot ? s - 1 : s);
    });

    auto io = getIOState();
    if (io.isValid())
        io.setProperty (inputChannels, inputs.getNumChildren(), nullptr);
    inputs.setProperty (count, inputs.getNumChildren(), nullptr);

    if (! arePositionsUserOwned())
        redistributeAllInputPositions();

    // Fresh session: close the gap the removal just opened, so the list keeps
    // reading 1..N. The columns the deleted row held are handed back out by the
    // re-flow, so the diagonal closes up instead of leaving a hole no later
    // channel can ever reach.
    if (! areChannelNumbersUserOwned())
    {
        compactChannelNumbersToDisplayOrder();
        compactInputPatchToDisplayOrder();
    }

    clearAllUndoHistories();
    return juce::Result::ok();
}

juce::Result WFSValueTreeState::setInputChannelType (int channelNumber, bool stereo)
{
    const int slot = getSlotForChannelNumber (channelNumber);
    if (slot < 0)
        return juce::Result::fail ("channel " + juce::String (channelNumber) + " is not live");
    if (isInputChannelStereo (slot) == stereo)
        return juce::Result::ok();
    if (stereo && getNumStereoInputChannels() >= WFSParameterDefaults::maxStereoChannels)
        return juce::Result::fail ("stereo budget reached ("
                                   + juce::String (WFSParameterDefaults::maxStereoChannels)
                                   + " stereo channels)");

    // Latched, the patch row keeps its columns here and the caller's
    // reconfiguration pass (sanitizeMonoPatchRows / autoPatchStereoRightColumns)
    // drops the R column on stereo→mono and auto-assigns a free R on
    // mono→stereo. Unlatched, the re-flow below replaces both.
    getInputsState().getChild (slot).setProperty (
        inputChannelType,
        stereo ? juce::String (inputChannelTypeStereo)
               : juce::String (inputChannelTypeMono), nullptr);

    // PATCH only, deliberately NOT the numbers — the asymmetry with the other
    // three structural ops is the point, not an oversight. A type flip moves no
    // channel's display position, so compactChannelNumbersToDisplayOrder() here
    // would be a provable no-op. The patch is NOT invariant under it: the row's
    // capacity changes by one column, which shifts every column after it.
    // Today's substitute is the caller's autoPatchStereoRightColumns, a
    // heuristic that refuses when leftCol + 1 is already claimed — and under
    // strict packing the next column is ALWAYS claimed by the following row, so
    // mono→stereo would essentially never get its R. The "already this type"
    // early return above sits before this, so a no-op flip does not re-flow.
    if (! areChannelNumbersUserOwned())
        compactInputPatchToDisplayOrder();

    clearAllUndoHistories();
    return juce::Result::ok();
}

juce::ValueTree WFSValueTreeState::buildInputChannelInventory() const
{
    // Display order, because that is the half of the model `inputChannels`
    // cannot express: the sum says 22, it does not say that slots 0 and 21 are
    // the stereo pairs. Patch rows are positional, so losing this ordering
    // silently re-patches the show.
    juce::ValueTree inventory (InputChannelList);

    const int total = getNumInputChannels();
    for (int slot = 0; slot < total; ++slot)
    {
        const int number = getInputChannelNumber (slot);
        if (number <= 0)
            continue;   // migrateInputChannelModel repairs these; never write one out

        juce::ValueTree ch (Ch);
        ch.setProperty (chNumber, number, nullptr);
        ch.setProperty (chType, isInputChannelStereo (slot)
                                    ? juce::String (inputChannelTypeStereo)
                                    : juce::String (inputChannelTypeMono), nullptr);
        inventory.appendChild (ch, nullptr);
    }

    return inventory;
}

void WFSValueTreeState::applyInputChannelInventory (const juce::ValueTree& inventory)
{
    if (! inventory.isValid())
        return;

    // Parse first, act second: a malformed entry must not leave the list
    // half-reconciled. First occurrence of a number wins — a file listing one
    // twice is corrupt, and picking one reading beats creating a duplicate.
    struct Entry { int number; bool stereo; };
    std::vector<Entry> wanted;
    for (int i = 0; i < inventory.getNumChildren(); ++i)
    {
        auto ch = inventory.getChild (i);
        if (! ch.hasType (Ch))
            continue;

        const int number = static_cast<int> (ch.getProperty (chNumber, 0));
        if (number <= 0 || number > WFSParameterDefaults::maxInputChannels)
            continue;

        const bool duplicate = std::any_of (wanted.begin(), wanted.end(),
                                            [number] (const Entry& e) { return e.number == number; });
        if (duplicate)
        {
            juce::Logger::writeToLog ("Channel inventory lists channel " + juce::String (number)
                                      + " twice; keeping the first");
            continue;
        }

        wanted.push_back ({ number,
                            ch.getProperty (chType).toString() == inputChannelTypeStereo });
    }

    if (wanted.empty())
        return;   // nothing usable — the caller's sum fallback is the better answer

    auto isWanted = [&wanted] (int number)
    {
        return std::any_of (wanted.begin(), wanted.end(),
                            [number] (const Entry& e) { return e.number == number; });
    };

    // Extras first, so both the 64-channel and the 8-stereo budgets are free
    // before anything is created — the same ordering setInputChannelCounts uses.
    // removeInputChannel refuses to empty the list, so a wholesale replacement
    // needs a second pass after the additions have raised the count; that is
    // what the `again` sweep below is for, not a retry loop.
    auto removeExtras = [this, &isWanted]
    {
        auto inputs = getInputsState();
        for (int slot = inputs.getNumChildren(); --slot >= 0;)
        {
            const int number = getInputChannelNumber (slot);
            if (number > 0 && ! isWanted (number))
                removeInputChannel (number);   // may legitimately refuse on the last channel
        }
    };
    removeExtras();

    // Create what the file lists and the tree lacks, with the recorded type, so
    // no channel is ever born mono and flipped afterwards (a flip would have to
    // find a free patch column, which strict packing rarely leaves).
    for (const auto& e : wanted)
    {
        if (getSlotForChannelNumber (e.number) >= 0)
            continue;

        const auto r = addInputChannel (e.stereo, e.number);
        if (r.failed())
            juce::Logger::writeToLog ("Channel inventory: could not create channel "
                                      + juce::String (e.number) + " - " + r.getErrorMessage());
    }

    removeExtras();   // the channel the "at least one" floor protected, if any

    // Type corrections for channels that already existed. Stereo->mono first:
    // it frees stereo budget that a mono->stereo correction in the same pass
    // may need, and the reverse order would fail on a full-budget swap.
    for (int pass = 0; pass < 2; ++pass)
    {
        const bool toStereo = (pass == 1);
        for (const auto& e : wanted)
        {
            if (e.stereo != toStereo)
                continue;

            const int slot = getSlotForChannelNumber (e.number);
            if (slot < 0 || isInputChannelStereo (slot) == e.stereo)
                continue;

            const auto r = setInputChannelType (e.number, e.stereo);
            if (r.failed())
                juce::Logger::writeToLog ("Channel inventory: could not set channel "
                                          + juce::String (e.number) + " to "
                                          + (e.stereo ? "stereo" : "mono")
                                          + " - " + r.getErrorMessage());
        }
    }

    // Display order last. Moved directly rather than through moveInputChannel:
    // that one drags the patch row with it, which is right for a user drag but
    // wrong here — the rows are about to be overwritten wholesale by the file's
    // own patchData, and moving them first would shuffle rows that are already
    // in the file's order. Resolve the source slot against the live tree on
    // every iteration, since earlier moves shift the ones after them.
    auto inputs = getInputsState();
    int target = 0;
    for (const auto& e : wanted)
    {
        const int from = getSlotForChannelNumber (e.number);
        if (from < 0)
            continue;   // creation failed above; already logged

        if (from != target)
            inputs.moveChild (from, target, nullptr);
        ++target;
    }

    clearAllUndoHistories();
}

void WFSValueTreeState::remapClusterInputOrders (const std::function<int (int)>& oldToNew)
{
    // clusterInputOrder is a csv of 0-based SLOT indices in memory; structural
    // edits (delete/reorder) shift slots, so every cluster's order must be
    // remapped in the same operation or the ordering silently migrates to the
    // wrong channels.
    //
    // The shape — split, map each token, drop on negative, rejoin, write only if
    // changed — is also exactly what the FILE boundary needs, so the two
    // converters below reuse it with slot<->number lambdas. Hence the parameter
    // is named for the transform, not for slots: it is not always slot->slot.
    auto clusters = getClustersState();
    for (int c = 0; c < clusters.getNumChildren(); ++c)
    {
        auto cluster = clusters.getChild (c);
        const juce::String order = cluster.getProperty (clusterInputOrder, "").toString();
        if (order.isEmpty())
            continue;

        juce::StringArray tokens;
        tokens.addTokens (order, ",", "");
        juce::StringArray remapped;
        for (const auto& tok : tokens)
        {
            const int newSlot = oldToNew (tok.trim().getIntValue());
            if (newSlot >= 0)
                remapped.add (juce::String (newSlot));
        }

        const juce::String newOrder = remapped.joinIntoString (",");
        if (newOrder != order)
            cluster.setProperty (clusterInputOrder, newOrder, nullptr);
    }
}

void WFSValueTreeState::convertClusterOrdersSlotsToNumbers()
{
    // Save direction. NOTE the guard: getInputChannelNumber returns 0 — not -1 —
    // for an out-of-range slot, and remapClusterInputOrders keeps any token >= 0,
    // so returning it raw would write a bogus "0" into the file. 0 is never a
    // valid channel number, and it is exactly the token a reader might mistake
    // for a legacy slot, so it must never be emitted.
    remapClusterInputOrders ([this] (int slot)
    {
        const int number = getInputChannelNumber (slot);
        return number > 0 ? number : -1;
    });
}

void WFSValueTreeState::convertClusterOrdersNumbersToSlots()
{
    // Load direction. getSlotForChannelNumber returns -1 for a number with no
    // live channel, which remapClusterInputOrders drops — the same
    // drop-what-no-longer-exists semantics deserializeExtendedScope uses for
    // snapshot scope entries.
    remapClusterInputOrders ([this] (int number)
    {
        return getSlotForChannelNumber (number);
    });
}

void WFSValueTreeState::moveInputChannelNodeAndRow (int fromSlot, int toSlot)
{
    auto inputs = getInputsState();
    inputs.moveChild (fromSlot, toSlot, nullptr);
    moveInputPatchRow (fromSlot, toSlot);

    // Slot-keyed side state follows the move
    const int from = fromSlot, to = toSlot;
    remapClusterInputOrders ([from, to] (int s)
    {
        if (s == from) return to;
        if (from < to) return (s > from && s <= to) ? s - 1 : s;
        return (s >= to && s < from) ? s + 1 : s;
    });
}

juce::Result WFSValueTreeState::moveInputChannel (int channelNumber, int targetSlot)
{
    auto inputs = getInputsState();
    const int fromSlot = getSlotForChannelNumber (channelNumber);
    if (fromSlot < 0)
        return juce::Result::fail ("channel " + juce::String (channelNumber) + " is not live");

    targetSlot = juce::jlimit (0, inputs.getNumChildren() - 1, targetSlot);
    if (targetSlot == fromSlot)
        return juce::Result::ok();

    // Drag-to-reorder: the channel node and its patch row move TOGETHER, so
    // the patch matrix, map, picker and engine slots all follow the new
    // display order while the permanent number (and every external
    // reference) stays put once the session is latched; before that the
    // renumber below makes the numbers follow the display order.
    moveInputChannelNodeAndRow (fromSlot, targetSlot);

    // Fresh session: the dragged channel takes the number of its new display
    // position — the whole point of the unlatched regime. moveInputPatchRow
    // above carries the columns WITH the row, which is the latched behaviour and
    // is exactly what decouples the patch from display order; unlatched, the
    // re-flow overwrites it.
    if (! areChannelNumbersUserOwned())
    {
        compactChannelNumbersToDisplayOrder();
        compactInputPatchToDisplayOrder();
    }

    clearAllUndoHistories();
    return juce::Result::ok();
}

void WFSValueTreeState::clampInputChannelCounts (int& numMono, int& numStereo)
{
    numStereo = juce::jlimit (0, WFSParameterDefaults::maxStereoChannels, numStereo);
    numMono   = juce::jlimit (1, WFSParameterDefaults::maxInputChannels - numStereo, numMono);
}

std::vector<InputChannelRef> WFSValueTreeState::predictInputChannelReduction (int numMono, int numStereo) const
{
    clampInputChannelCounts (numMono, numStereo);

    const int total = getNumInputChannels();
    int stereoLeft  = getNumStereoInputChannels();
    int monoLeft    = total - stereoLeft;

    // Same walk, same order, as setInputChannelCounts: stereo victims first,
    // each type from the bottom of the display order up. Everything about a
    // victim is captured HERE, before any removal, because an unlatched session
    // renumbers the survivors after each one.
    std::vector<InputChannelRef> victims;
    auto capture = [this] (int slot)
    {
        InputChannelRef ref;
        ref.slot     = slot;
        ref.number   = getInputChannelNumber (slot);
        ref.stereo   = isInputChannelStereo (slot);
        ref.hwInputs = getInputPatchHardwareInputs (slot);
        auto channel = getInputsState().getChild (slot).getChildWithName (Channel);
        if (channel.isValid())
            ref.name = channel.getProperty (inputName).toString();
        return ref;
    };

    for (int slot = total; --slot >= 0 && stereoLeft > numStereo;)
        if (isInputChannelStereo (slot)) { victims.push_back (capture (slot)); --stereoLeft; }
    for (int slot = total; --slot >= 0 && monoLeft > numMono;)
        if (! isInputChannelStereo (slot)) { victims.push_back (capture (slot)); --monoLeft; }

    return victims;
}

std::vector<int> WFSValueTreeState::getInputPatchHardwareInputs (int slot) const
{
    std::vector<int> out;
    // getAudioPatchState has no const overload; this only reads. Same precedent
    // as WFSFileManager's const_cast around getInputState.
    auto patch = const_cast<WFSValueTreeState*> (this)->getAudioPatchState().getChildWithName (InputPatch);
    if (! patch.isValid() || slot < 0)
        return out;

    juce::StringArray rowStrings = juce::StringArray::fromTokens (patch.getProperty (patchData).toString(), ";", "");
    if (slot >= rowStrings.size())
        return out;

    juce::StringArray colStrings = juce::StringArray::fromTokens (rowStrings[slot], ",", "");
    for (int c = 0; c < colStrings.size(); ++c)
        if (colStrings[c].getIntValue() == 1)
            out.push_back (c + 1);   // 1-based, as the operator sees it
    return out;
}

InputChannelIdentity WFSValueTreeState::getInputChannelIdentity() const
{
    InputChannelIdentity live;
    auto inputs = getInputsState();
    const int total = inputs.getNumChildren();
    for (int slot = 0; slot < total; ++slot)
    {
        InputChannelRef ref;
        ref.slot     = slot;
        ref.number   = getInputChannelNumber (slot);
        ref.stereo   = isInputChannelStereo (slot);
        ref.hwInputs = getInputPatchHardwareInputs (slot);
        auto channel = inputs.getChild (slot).getChildWithName (Channel);
        if (channel.isValid())
            ref.name = channel.getProperty (inputName).toString();
        live.slots.push_back (std::move (ref));
    }
    if (! live.slots.empty())
    {
        live.source     = InputChannelIdentity::Source::liveTree;
        live.typesKnown = true;
        live.orderKnown = true;
        live.hwKnown    = true;
    }
    return live;
}

juce::Result WFSValueTreeState::assignInputChannelNumbersBySlot (const std::vector<int>& numbersBySlot,
                                                                 const juce::String& reason)
{
    const int total = getNumInputChannels();
    if ((int) numbersBySlot.size() != total)
        return juce::Result::fail ("relabel: " + juce::String ((int) numbersBySlot.size())
                                   + " numbers for " + juce::String (total) + " channels");

    // Everything validated before the first write, so a bad list changes nothing.
    std::vector<int> seen;
    for (int n : numbersBySlot)
    {
        if (n <= 0 || n > WFSParameterDefaults::maxInputChannels)
            return juce::Result::fail ("relabel: channel number " + juce::String (n) + " is out of range");
        if (std::find (seen.begin(), seen.end(), n) != seen.end())
            return juce::Result::fail ("relabel: channel number " + juce::String (n) + " appears twice");
        seen.push_back (n);
    }

    juce::StringArray changes;
    for (int slot = 0; slot < total; ++slot)
    {
        const int oldNumber = getInputChannelNumber (slot);
        const int newNumber = numbersBySlot[(size_t) slot];
        if (oldNumber != newNumber)
        {
            changes.add ("#" + juce::String (oldNumber) + "->#" + juce::String (newNumber));
            setInputChannelNumberAtSlot (slot, newNumber);
        }
    }

    // The numbers are now an external contract, whatever they were before.
    markChannelNumbersUserOwned (reason);
    clearAllUndoHistories();

    WFSLogger::getInstance().logInfo ("Channel numbers relabelled (" + reason + "): "
                                      + (changes.isEmpty() ? juce::String ("no change")
                                                           : changes.joinIntoString (", ")));
    return juce::Result::ok();
}

juce::Result WFSValueTreeState::reorderInputChannelsToNumbers (const std::vector<int>& numbersInDisplayOrder,
                                                               const juce::String& reason)
{
    // FIRST, before any move: unlatched, moveInputChannel's tail recompacts the
    // numbers to display order, and the targets would drift under the loop.
    markChannelNumbersUserOwned (reason);

    int target = 0;
    for (int number : numbersInDisplayOrder)
    {
        const int from = getSlotForChannelNumber (number);
        if (from < 0)
            continue;
        if (from != target)
        {
            const auto r = moveInputChannel (number, target);
            if (r.failed())
                return r;
        }
        ++target;
    }

    WFSLogger::getInstance().logInfo ("Channel order rearranged (" + reason + ")");
    return juce::Result::ok();
}

void WFSValueTreeState::moveInputPatchRow (int fromSlot, int toSlot)
{
    auto patch = getAudioPatchState().getChildWithName (InputPatch);
    if (! patch.isValid())
        return;

    juce::StringArray rowsArr = juce::StringArray::fromTokens (
        patch.getProperty (patchData).toString(), ";", "");
    if (fromSlot < 0 || fromSlot >= rowsArr.size()
        || toSlot < 0 || toSlot >= rowsArr.size())
        return;

    rowsArr.move (fromSlot, toSlot);   // same semantics as ValueTree::moveChild
    patch.setProperty (patchData, rowsArr.joinIntoString (";"), nullptr);
}

void WFSValueTreeState::insertInputPatchRow (int slot, bool stereo)
{
    auto patch = getAudioPatchState().getChildWithName (InputPatch);
    if (! patch.isValid())
        return;

    juce::StringArray rowsArr = juce::StringArray::fromTokens (
        patch.getProperty (patchData).toString(), ";", "");

    // Diagonal-continue: the new row takes the next hardware column(s) past
    // everything already patched (two consecutive for stereo, lower = L).
    int cursor = 0;
    for (int r = 0; r < rowsArr.size(); ++r)
    {
        juce::StringArray rowCols = juce::StringArray::fromTokens (rowsArr[r], ",", "");
        for (int c = rowCols.size(); --c >= 0;)
        {
            if (rowCols[c].getIntValue() == 1)
            {
                cursor = juce::jmax (cursor, c + 1);
                break;
            }
        }
    }

    const int capacity = stereo ? 2 : 1;
    const int lastCol  = juce::jmin (maxHardwarePatchChannels - 1, cursor + capacity - 1);
    const int hwCols   = static_cast<int> (patch.getProperty (cols, 64));
    const int rowLen   = juce::jmax (hwCols, lastCol + 1);

    juce::StringArray rowCols;
    for (int c = 0; c < rowLen; ++c)
        rowCols.add (c >= cursor && c <= lastCol ? "1" : "0");

    rowsArr.insert (slot, rowCols.joinIntoString (","));

    patch.setProperty (patchData, rowsArr.joinIntoString (";"), nullptr);
    patch.setProperty (rows, rowsArr.size(), nullptr);
    recomputePatchCols();
}

void WFSValueTreeState::removeInputPatchRow (int slot)
{
    auto patch = getAudioPatchState().getChildWithName (InputPatch);
    if (! patch.isValid())
        return;

    juce::StringArray rowsArr = juce::StringArray::fromTokens (
        patch.getProperty (patchData).toString(), ";", "");
    if (slot < 0 || slot >= rowsArr.size())
        return;

    rowsArr.remove (slot);
    patch.setProperty (patchData, rowsArr.joinIntoString (";"), nullptr);
    patch.setProperty (rows, rowsArr.size(), nullptr);
    recomputePatchCols();
}

void WFSValueTreeState::normalizeInputPatchRows()
{
    // Config load rewrites patchData wholesale, so the stored rows may not
    // match the channel list: truncate extras, append diagonal-continue rows
    // (capacity from the channel's type) for the missing tail. Idempotent.
    auto patch = getAudioPatchState().getChildWithName (InputPatch);
    if (! patch.isValid())
        return;

    const int total = getNumInputChannels();
    juce::StringArray rowsArr = juce::StringArray::fromTokens (
        patch.getProperty (patchData).toString(), ";", "");

    if (rowsArr.size() > total)
    {
        rowsArr.removeRange (total, rowsArr.size() - total);
        patch.setProperty (patchData, rowsArr.joinIntoString (";"), nullptr);
        patch.setProperty (rows, rowsArr.size(), nullptr);
        recomputePatchCols();
    }

    while (rowsArr.size() < total)
    {
        insertInputPatchRow (rowsArr.size(), isInputChannelStereo (rowsArr.size()));
        rowsArr = juce::StringArray::fromTokens (
            patch.getProperty (patchData).toString(), ";", "");
    }
}

void WFSValueTreeState::compactInputPatchToDisplayOrder()
{
    auto patch = getAudioPatchState().getChildWithName (InputPatch);
    if (! patch.isValid())
        return;

    const int total = getNumInputChannels();
    if (total <= 0)
        return;

    // Rebuilt from the channel list, discarding the stored rows wholesale. That
    // is safe on a call-graph property, not on anything in this code: while the
    // numbers are unlatched, no operator click and no wire message has ever
    // reached patchData. The only interactive writer is
    // PatchMatrixComponent::savePatchesToValueTree, reachable only through
    // MainComponent::openAudioInterfaceWindow(), which calls
    // markChannelNumbersUserOwned() BEFORE it constructs the window; every load
    // path latches on success; MCP lists patchData under ignored_parameters and
    // OSC has no patch address. WARNING: anything future that lets a patch be
    // authored must latch first, or this will eat it.
    //
    // Rebuilding is also what makes the re-flow self-repairing: it drops a stale
    // row count and the ragged row lengths insertInputPatchRow leaves behind (it
    // sizes only the row it inserts), and it makes the result a pure function of
    // the channel list — hence idempotent by construction.
    //
    // Strict packing: consecutive columns, no gaps, and NO alignment to the
    // interface's odd/even input pairs, so N mono + M stereo always fit in
    // N + 2M hardware inputs. A stereo pair may therefore legitimately start on
    // hardware input 11.
    const int demand = total + getNumStereoInputChannels();
    const int hwCols = static_cast<int> (patch.getProperty (cols, 64));
    const int rowLen = juce::jmax (hwCols, juce::jmin (maxHardwarePatchChannels, demand));

    juce::StringArray rowsArr;
    int cursor = 0;
    for (int slot = 0; slot < total; ++slot)
    {
        const int capacity = isInputChannelStereo (slot) ? 2 : 1;
        const int first    = cursor;

        // The clamp mirrors insertInputPatchRow's literally. At the exact
        // boundary a stereo row would come out with a single column — 64 live
        // channels with at most 8 stereo demand 72 of 512 columns, so that is
        // structurally unreachable. It is a guard, not a policy: nobody should
        // read a rule out of it and "unify" the two functions on its strength.
        const int last = juce::jmin (maxHardwarePatchChannels - 1, cursor + capacity - 1);
        cursor += capacity;

        juce::StringArray rowCols;
        for (int c = 0; c < rowLen; ++c)
            rowCols.add (c >= first && c <= last ? "1" : "0");

        rowsArr.add (rowCols.joinIntoString (","));
    }

    // Direct setProperty throughout, never setParameter: re-flowing the app's
    // own default patch must not push an undo entry, mark the project dirty or
    // trip an ownership latch. Same rule compactChannelNumbersToDisplayOrder and
    // resequenceDefaultInputNames follow.
    patch.setProperty (patchData, rowsArr.joinIntoString (";"), nullptr);
    patch.setProperty (rows, rowsArr.size(), nullptr);
    recomputePatchCols();
}

int WFSValueTreeState::getNumOutputChannels() const
{
    return const_cast<WFSValueTreeState*>(this)->getOutputsState().getNumChildren();
}

int WFSValueTreeState::getNumReverbChannels() const
{
    // Count actual `Reverb`-typed children only; the Reverbs subtree also
    // hosts global siblings (ReverbAlgorithm, ReverbPreComp, ReverbPostEQ,
    // ReverbPostExp) that must NOT count toward channel total. Reading
    // the reverbChannels property directly used to do this job, but the
    // property could drift from the actual children when a writer
    // bypassed setNumReverbChannels — which then made
    // session_get_channel_full(reverb, ...) refuse valid IDs because the
    // child lookup ran out of Reverb-typed siblings before the ID range.
    auto reverbs = const_cast<WFSValueTreeState*>(this)->getReverbsState();
    int reverbCount = 0;
    for (int i = 0; i < reverbs.getNumChildren(); ++i)
        if (reverbs.getChild (i).hasType (Reverb))
            ++reverbCount;
    return reverbCount;
}

void WFSValueTreeState::setNumInputChannels (int numChannels)
{
    // Blunt count entry point (config-load sync, OSC/MCP inputChannels
    // writes). Under the stable-number model: growth APPENDS default mono
    // channels after the last channel (numbers never shift), reduction
    // removes the HIGHEST-NUMBERED channels — nothing in the middle ever
    // moves. Patch rows are mirrored by the ops. Not undoable (the ops clear
    // the undo history).
    numChannels = juce::jlimit (1, maxInputChannels, numChannels);
    auto inputs = getInputsState();

    while (inputs.getNumChildren() > numChannels)
        if (! removeInputChannel (getHighestChannelNumber()).wasOk()) break;
    while (inputs.getNumChildren() < numChannels)
        if (! addInputChannel (false).wasOk()) break;

    // Ensure GradientMaps and Sampler sections exist for all inputs (migration for old configs)
    for (int i = 0; i < inputs.getNumChildren(); ++i)
    {
        ensureInputGradientMapsSection (i);
        ensureInputSamplerSection (i);
    }

    // Keep the count properties honest even when no structural change was
    // needed (the ops already stamp them on every add/remove). Direct writes,
    // NOT via setParameter — its routing would recurse into us.
    auto io = getIOState();
    if (io.isValid())
        io.setProperty (inputChannels, inputs.getNumChildren(), nullptr);
    inputs.setProperty (count, inputs.getNumChildren(), nullptr);
}

void WFSValueTreeState::setNumOutputChannels (int numChannels)
{
    numChannels = juce::jlimit (1, maxOutputChannels, numChannels);
    auto outputs = getOutputsState();
    int currentCount = outputs.getNumChildren();

    beginUndoTransaction ("Set Output Channel Count");

    if (numChannels > currentCount)
    {
        // Add new channels
        for (int i = currentCount; i < numChannels; ++i)
            outputs.appendChild (createDefaultOutputChannel (i), getActiveUndoManager());
    }
    else if (numChannels < currentCount)
    {
        // Remove excess channels
        while (outputs.getNumChildren() > numChannels)
            outputs.removeChild (outputs.getNumChildren() - 1, getActiveUndoManager());
    }

    // Update the count property directly (NOT via setParameter) so the
    // setParameter -> setNumOutputChannels routing in setParameter doesn't
    // recurse into us.
    {
        auto io = getIOState();
        if (io.isValid())
            io.setProperty (outputChannels, numChannels, getActiveUndoManager());
    }
    outputs.setProperty (count, numChannels, getActiveUndoManager());

    // Update input mute arrays
    auto inputs = getInputsState();
    for (int i = 0; i < inputs.getNumChildren(); ++i)
    {
        auto mutesTree = getInputMutesSection (i);
        if (mutesTree.isValid())
        {
            juce::String mutesStr = mutesTree.getProperty (inputMutes).toString();
            juce::StringArray mutesArray;
            mutesArray.addTokens (mutesStr, ",", "");

            while (mutesArray.size() < numChannels)
                mutesArray.add ("0");
            while (mutesArray.size() > numChannels)
                mutesArray.remove (mutesArray.size() - 1);

            mutesTree.setProperty (inputMutes, mutesArray.joinIntoString (","), getActiveUndoManager());
        }
    }
}

void WFSValueTreeState::setNumReverbChannels (int numChannels)
{
    numChannels = juce::jlimit (0, maxReverbChannels, numChannels);
    auto reverbs = getReverbsState();

    // Create Reverbs section if it doesn't exist
    if (!reverbs.isValid())
    {
        createReverbsSection();
        reverbs = getReverbsState();
    }

    // Count only Reverb channel children (not ReverbAlgorithm or other global sections)
    int currentCount = 0;
    for (int i = 0; i < reverbs.getNumChildren(); ++i)
        if (reverbs.getChild (i).hasType (Reverb))
            ++currentCount;
    const int originalCount = currentCount;   // the removal loop below mutates currentCount

    beginUndoTransaction ("Set Reverb Channel Count");

    if (numChannels > currentCount)
    {
        // Add new channels
        for (int i = currentCount; i < numChannels; ++i)
            reverbs.appendChild (createDefaultReverbChannel (i, numChannels), getActiveUndoManager());
    }
    else if (numChannels < currentCount)
    {
        // Remove excess Reverb channels (not global sections like ReverbAlgorithm)
        for (int i = reverbs.getNumChildren() - 1; i >= 0 && currentCount > numChannels; --i)
        {
            if (reverbs.getChild (i).hasType (Reverb))
            {
                reverbs.removeChild (i, getActiveUndoManager());
                --currentCount;
            }
        }
    }

    // Ensure all existing reverb channels have EQ sections (handles old configs without EQ)
    for (int i = 0; i < numChannels; ++i)
        ensureReverbEQSection (i);

    // Ensure global algorithm section exists (handles old configs)
    ensureReverbAlgorithmSection();

    // Ensure global pre-compressor section exists (handles old configs)
    ensureReverbPreCompSection();

    // Ensure global post-processing EQ section exists (handles old configs)
    ensureReverbPostEQSection();

    // Ensure global post-expander section exists (handles old configs)
    ensureReverbPostExpSection();

    // Update the count property directly (NOT via setParameter) so the
    // setParameter -> setNumReverbChannels routing in setParameter doesn't
    // recurse into us.
    {
        auto io = getIOState();
        if (io.isValid())
            io.setProperty (reverbChannels, numChannels, getActiveUndoManager());
    }
    reverbs.setProperty (count, numChannels, getActiveUndoManager());

    // The arc layout depends on the total node count, so channels created under
    // an earlier count sit on the wrong arc — re-lay the whole set. Gated on
    // ownership: once the user has positioned things, the new nodes land on the
    // default arc and everything else stays put (same rule as the input grid).
    if (originalCount != numChannels && ! arePositionsUserOwned())
        redistributeAllReverbPositions();
}

namespace
{
    int computeHighestPatchedHardwareChannel (const juce::String& patchDataStr)
    {
        int highest = -1;
        auto rowTokens = juce::StringArray::fromTokens (patchDataStr, ";", "");
        for (auto& rowStr : rowTokens)
        {
            auto colTokens = juce::StringArray::fromTokens (rowStr, ",", "");
            for (int c = 0; c < colTokens.size(); ++c)
                if (colTokens[c].getIntValue() == 1 && c > highest)
                    highest = c;
        }
        return highest;
    }

    void applyColsPolicy (juce::ValueTree& patchTree, int deviceChannels)
    {
        if (! patchTree.isValid())
            return;

        constexpr int minCols = 64;
        constexpr int maxCols = WFSValueTreeState::maxHardwarePatchChannels;

        auto patchDataStr = patchTree.getProperty (WFSParameterIDs::patchData).toString();
        int highestPatched = computeHighestPatchedHardwareChannel (patchDataStr);

        int target = juce::jmax (minCols, deviceChannels, highestPatched + 1);
        int newCols = juce::jlimit (minCols, maxCols, target);

        int currentCols = patchTree.getProperty (WFSParameterIDs::cols, minCols);
        if (newCols != currentCols)
            patchTree.setProperty (WFSParameterIDs::cols, newCols, nullptr);
    }
}

void WFSValueTreeState::updateHardwareChannelCount (int hwInputs, int hwOutputs)
{
    auto audioPatch = state.getChildWithName (AudioPatch);
    if (! audioPatch.isValid())
        return;

    auto inputPatchTree = audioPatch.getChildWithName (InputPatch);
    if (inputPatchTree.isValid())
    {
        inputPatchTree.setProperty (WFSParameterIDs::activeHardwareInputs, hwInputs, nullptr);
        applyColsPolicy (inputPatchTree, hwInputs);
    }

    auto outputPatchTree = audioPatch.getChildWithName (OutputPatch);
    if (outputPatchTree.isValid())
    {
        outputPatchTree.setProperty (WFSParameterIDs::activeHardwareOutputs, hwOutputs, nullptr);
        applyColsPolicy (outputPatchTree, hwOutputs);
    }
}

void WFSValueTreeState::recomputePatchCols()
{
    auto audioPatch = state.getChildWithName (AudioPatch);
    if (! audioPatch.isValid())
        return;

    auto inputPatchTree = audioPatch.getChildWithName (InputPatch);
    if (inputPatchTree.isValid())
    {
        int stored = inputPatchTree.getProperty (WFSParameterIDs::activeHardwareInputs, 0);
        applyColsPolicy (inputPatchTree, stored);
    }

    auto outputPatchTree = audioPatch.getChildWithName (OutputPatch);
    if (outputPatchTree.isValid())
    {
        int stored = outputPatchTree.getProperty (WFSParameterIDs::activeHardwareOutputs, 0);
        applyColsPolicy (outputPatchTree, stored);
    }
}

//==============================================================================
// Undo / Redo + Listener Management — moved to TreeParameterStore.
// (Per-domain UndoManager array, MCP-origin undo suppression, parameter
// listener registry and ValueTree listener add/remove all live in the core;
// the UndoDomain-typed wrappers are inline in the header.)
//==============================================================================

//==============================================================================
// State Management
//==============================================================================

void WFSValueTreeState::resetToDefaults()
{
    state.removeListener (this);
    initializeDefaultState();
    state.addListener (this);
    clearAllUndoHistories();
}

void WFSValueTreeState::resetInputToDefaults (int channelIndex)
{
    auto input = getInputState (channelIndex);
    if (input.isValid())
    {
        beginUndoTransaction ("Reset Input " + juce::String (channelIndex + 1));
        auto newInput = createDefaultInputChannel (channelIndex);
        input.copyPropertiesAndChildrenFrom (newInput, getActiveUndoManager());
    }
}

void WFSValueTreeState::resetOutputToDefaults (int channelIndex)
{
    auto output = getOutputState (channelIndex);
    if (output.isValid())
    {
        beginUndoTransaction ("Reset Output " + juce::String (channelIndex + 1));
        auto newOutput = createDefaultOutputChannel (channelIndex);
        output.copyPropertiesAndChildrenFrom (newOutput, getActiveUndoManager());
    }
}

void WFSValueTreeState::resetReverbToDefaults (int channelIndex)
{
    auto reverb = getReverbState (channelIndex);
    if (reverb.isValid())
    {
        beginUndoTransaction ("Reset Reverb " + juce::String (channelIndex + 1));
        auto newReverb = createDefaultReverbChannel (channelIndex, getNumReverbChannels());
        reverb.copyPropertiesAndChildrenFrom (newReverb, getActiveUndoManager());
    }
}

void WFSValueTreeState::redistributeAllInputPositions()
{
    auto inputs = getInputsState();
    int numInputs = inputs.getNumChildren();
    if (numInputs == 0) return;

    auto stageTree = getStageState();
    float sw = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (stageWidth))  : stageWidthDefault;
    float sd = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (stageDepth))  : stageDepthDefault;
    float sh = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (stageHeight)) : stageHeightDefault;
    float ow = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (originWidth)) : originWidthDefault;
    float od = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (originDepth)) : originDepthDefault;
    float oh = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (originHeight)) : originHeightDefault;

    beginUndoTransaction ("Redistribute Input Positions");

    for (int i = 0; i < numInputs; ++i)
    {
        auto input = inputs.getChild (i);
        auto pos = input.getChildWithName (Position);
        if (! pos.isValid()) continue;

        float x, y, z;
        getDefaultInputPosition (i, numInputs, sw, sd, sh, ow, od, oh, x, y, z);
        pos.setProperty (inputPositionX, x, getActiveUndoManager());
        pos.setProperty (inputPositionY, y, getActiveUndoManager());
        pos.setProperty (inputPositionZ, z, getActiveUndoManager());
    }
}

void WFSValueTreeState::scaleAllInputPositions (float oldW, float oldD, float oldH,
                                                 float oldOW, float oldOD, float oldOH)
{
    auto inputs = getInputsState();
    int numInputs = inputs.getNumChildren();
    if (numInputs == 0) return;

    auto stageTree = getStageState();
    float newW  = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (stageWidth))  : stageWidthDefault;
    float newD  = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (stageDepth))  : stageDepthDefault;
    float newH  = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (stageHeight)) : stageHeightDefault;
    float newOW = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (originWidth)) : originWidthDefault;
    float newOD = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (originDepth)) : originDepthDefault;
    float newOH = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (originHeight)) : originHeightDefault;

    // Old and new stage bounds (origin-relative)
    float oldMinX = -oldW / 2.0f - oldOW;
    float oldMinY = -oldD / 2.0f - oldOD;
    float oldMinZ = -oldOH;
    float newMinX = -newW / 2.0f - newOW;
    float newMinY = -newD / 2.0f - newOD;
    float newMinZ = -newOH;

    beginUndoTransaction ("Scale Input Positions");

    for (int i = 0; i < numInputs; ++i)
    {
        auto input = inputs.getChild (i);
        auto pos = input.getChildWithName (Position);
        if (! pos.isValid()) continue;

        float x = static_cast<float> (pos.getProperty (inputPositionX));
        float y = static_cast<float> (pos.getProperty (inputPositionY));
        float z = static_cast<float> (pos.getProperty (inputPositionZ));

        // Map from old bounds to normalized [0..1], then to new bounds
        if (oldW > 0.0f)
            x = newMinX + ((x - oldMinX) / oldW) * newW;
        if (oldD > 0.0f)
            y = newMinY + ((y - oldMinY) / oldD) * newD;
        if (oldH > 0.0f)
            z = newMinZ + ((z - oldMinZ) / oldH) * newH;

        pos.setProperty (inputPositionX, x, getActiveUndoManager());
        pos.setProperty (inputPositionY, y, getActiveUndoManager());
        pos.setProperty (inputPositionZ, z, getActiveUndoManager());
    }
}

void WFSValueTreeState::fitAllInputPositionsToStage()
{
    auto inputs = getInputsState();
    int numInputs = inputs.getNumChildren();
    if (numInputs == 0) return;

    auto stageTree = getStageState();
    float sw = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (stageWidth))  : stageWidthDefault;
    float sd = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (stageDepth))  : stageDepthDefault;
    float sh = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (stageHeight)) : stageHeightDefault;
    float ow = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (originWidth)) : originWidthDefault;
    float od = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (originDepth)) : originDepthDefault;
    float oh = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (originHeight)) : originHeightDefault;

    float minX = -sw / 2.0f - ow;
    float maxX =  sw / 2.0f - ow;
    float minY = -sd / 2.0f - od;
    float maxY =  sd / 2.0f - od;
    float minZ = -oh;
    float maxZ =  sh - oh;

    beginUndoTransaction ("Fit Input Positions to Stage");

    for (int i = 0; i < numInputs; ++i)
    {
        auto input = inputs.getChild (i);
        auto pos = input.getChildWithName (Position);
        if (! pos.isValid()) continue;

        float x = static_cast<float> (pos.getProperty (inputPositionX));
        float y = static_cast<float> (pos.getProperty (inputPositionY));
        float z = static_cast<float> (pos.getProperty (inputPositionZ));

        float cx = juce::jlimit (minX, maxX, x);
        float cy = juce::jlimit (minY, maxY, y);
        float cz = juce::jlimit (minZ, maxZ, z);

        // Only write if changed (avoid unnecessary undo entries)
        if (cx != x) pos.setProperty (inputPositionX, cx, getActiveUndoManager());
        if (cy != y) pos.setProperty (inputPositionY, cy, getActiveUndoManager());
        if (cz != z) pos.setProperty (inputPositionZ, cz, getActiveUndoManager());
    }
}

void WFSValueTreeState::shiftAllInputPositions (float dx, float dy, float dz)
{
    auto inputs = getInputsState();
    int n = inputs.getNumChildren();
    if (n == 0) return;

    beginUndoTransaction ("Shift Input Positions");
    for (int i = 0; i < n; ++i)
    {
        auto pos = inputs.getChild (i).getChildWithName (Position);
        if (! pos.isValid()) continue;
        pos.setProperty (inputPositionX, static_cast<float> (pos.getProperty (inputPositionX)) + dx, getActiveUndoManager());
        pos.setProperty (inputPositionY, static_cast<float> (pos.getProperty (inputPositionY)) + dy, getActiveUndoManager());
        pos.setProperty (inputPositionZ, static_cast<float> (pos.getProperty (inputPositionZ)) + dz, getActiveUndoManager());
    }
}

void WFSValueTreeState::shiftAllOutputPositions (float dx, float dy, float dz)
{
    auto outputs = getOutputsState();
    int n = outputs.getNumChildren();
    if (n == 0) return;

    beginUndoTransaction ("Shift Output Positions");
    for (int i = 0; i < n; ++i)
    {
        auto pos = outputs.getChild (i).getChildWithName (Position);
        if (! pos.isValid()) continue;
        pos.setProperty (outputPositionX, static_cast<float> (pos.getProperty (outputPositionX)) + dx, getActiveUndoManager());
        pos.setProperty (outputPositionY, static_cast<float> (pos.getProperty (outputPositionY)) + dy, getActiveUndoManager());
        pos.setProperty (outputPositionZ, static_cast<float> (pos.getProperty (outputPositionZ)) + dz, getActiveUndoManager());
    }
}

void WFSValueTreeState::shiftAllReverbPositions (float dx, float dy, float dz)
{
    auto reverbs = getReverbsState();
    int n = reverbs.getNumChildren();
    if (n == 0) return;

    beginUndoTransaction ("Shift Reverb Positions");
    for (int i = 0; i < n; ++i)
    {
        auto child = reverbs.getChild (i);
        if (child.getType() != Reverb) continue;  // Skip ReverbAlgorithm
        auto pos = child.getChildWithName (Position);
        if (! pos.isValid()) continue;
        pos.setProperty (reverbPositionX, static_cast<float> (pos.getProperty (reverbPositionX)) + dx, getActiveUndoManager());
        pos.setProperty (reverbPositionY, static_cast<float> (pos.getProperty (reverbPositionY)) + dy, getActiveUndoManager());
        pos.setProperty (reverbPositionZ, static_cast<float> (pos.getProperty (reverbPositionZ)) + dz, getActiveUndoManager());
    }
}

void WFSValueTreeState::replaceState (const juce::ValueTree& newState)
{
    if (validateState (newState))
    {
        state.copyPropertiesAndChildrenFrom (newState, nullptr);
        migrateADMOSCSection();
        ensureInputAdmMappingProperty();
        // Stable-number model migration MUST precede ensureCompleteSchema: the
        // schema template stamps inputChannelType=mono, which would otherwise
        // preempt the legacy tail-split stamp for pre-rework files.
        migrateInputChannelModel();
        // Strays that older files carry in <IO> because nothing stamped them
        // anywhere. Must precede ensureCompleteSchema, or the backfill stamps a
        // default in the proper section and the real value stays orphaned.
        migrateStrayConfigProperties();
        stripObsoleteReverbProperties();
        // Back-fill anything the loaded state omitted (incomplete / scope-filtered
        // files) so no parameter is left absent on this wholesale-replace path.
        ensureCompleteSchema();
        // A wholesale replace IS a project load: the numbers in the file are
        // already in use (cues, snapshots, plug-in automation, external
        // controllers), and a file written before this latch existed carries no
        // property at all. Both must land on the permanent-number regime — this
        // is the whole backward-compatibility story, and it must run after
        // ensureCompleteSchema so the IO node exists to hold the flag.
        markChannelNumbersUserOwned ("project load (state replace)");

        // A wholesale replace brings <Clusters> and <Inputs> in together, so a
        // slot-keyed clusterInputOrder is internally consistent and needs nothing
        // — which is why exportCompleteConfig, writing the live tree verbatim,
        // stays slot-keyed and carries no marker. Honour the marker anyway if one
        // is present, so a number-keyed file loaded through this path is not
        // silently read as slots.
        if (auto clusters = getClustersState();
            clusters.isValid() && clusters.getProperty (inputOrderKey).toString() == inputOrderKeyNumber)
        {
            convertClusterOrdersNumbersToSlots();
            clusters.removeProperty (inputOrderKey, nullptr);   // file artifact, not runtime state
        }

        clearAllUndoHistories();
    }
}

namespace
{
    // Recursively add to `target` any property or child present in `tmpl` but
    // missing from `target`. Never overwrites an existing value and never removes
    // anything. Children are matched by id (when the template child carries one),
    // otherwise by type name - mirroring WFSFileManager::mergeTreeRecursive.
    void backfillFromTemplate (juce::ValueTree& target, const juce::ValueTree& tmpl,
                               juce::UndoManager* um)
    {
        for (int i = 0; i < tmpl.getNumProperties(); ++i)
        {
            const auto propName = tmpl.getPropertyName (i);
            if (! target.hasProperty (propName))
                target.setProperty (propName, tmpl.getProperty (propName), um);
        }

        for (int i = 0; i < tmpl.getNumChildren(); ++i)
        {
            const auto tmplChild = tmpl.getChild (i);
            juce::ValueTree match;

            if (tmplChild.hasProperty (id))
            {
                match = target.getChildWithProperty (id, tmplChild.getProperty (id));
                if (match.isValid() && match.getType() != tmplChild.getType())
                    match = {};
            }
            else
            {
                match = target.getChildWithName (tmplChild.getType());
            }

            if (match.isValid())
                backfillFromTemplate (match, tmplChild, um);
            else
                target.appendChild (tmplChild.createCopy(), um);
        }
    }
}

void WFSValueTreeState::ensureCompleteSchema()
{
    juce::UndoManager* um = nullptr;  // schema back-fill is not an undoable user edit

    // --- Config (singleton subsections) ---
    auto config = state.getChildWithName (Config);
    if (config.isValid())
    {
        juce::ValueTree defaultConfig (Config);
        createShowSection (defaultConfig);
        createIOSection (defaultConfig);
        createStageSection (defaultConfig);
        createMasterSection (defaultConfig);
        createNetworkSection (defaultConfig);
        createADMOSCSection (defaultConfig);
        createTrackingSection (defaultConfig);
        createClustersSection (defaultConfig);
        createBinauralSection (defaultConfig);
        createUISection (defaultConfig);
        backfillFromTemplate (config, defaultConfig, um);
    }

    // --- Inputs (per channel; Config is back-filled first so IO counts exist) ---
    auto inputs = state.getChildWithName (Inputs);
    if (inputs.isValid())
    {
        for (int i = 0; i < inputs.getNumChildren(); ++i)
        {
            auto child = inputs.getChild (i);
            if (! child.hasType (Input))
                continue;
            auto tmpl = createDefaultInputChannel (i);
            backfillFromTemplate (child, tmpl, um);
        }
    }

    // --- Outputs (per channel) ---
    auto outputs = state.getChildWithName (Outputs);
    if (outputs.isValid())
    {
        for (int i = 0; i < outputs.getNumChildren(); ++i)
        {
            auto child = outputs.getChild (i);
            if (! child.hasType (Output))
                continue;
            auto tmpl = createDefaultOutputChannel (i);
            backfillFromTemplate (child, tmpl, um);
        }
    }

    // --- Reverbs (per channel + global sibling sections) ---
    auto reverbs = state.getChildWithName (Reverbs);
    if (reverbs.isValid())
    {
        int reverbCount = 0;
        for (int i = 0; i < reverbs.getNumChildren(); ++i)
            if (reverbs.getChild (i).hasType (Reverb))
                ++reverbCount;

        int revIdx = 0;
        for (int i = 0; i < reverbs.getNumChildren(); ++i)
        {
            auto child = reverbs.getChild (i);
            if (! child.hasType (Reverb))
                continue;
            auto tmpl = createDefaultReverbChannel (revIdx++, reverbCount);
            backfillFromTemplate (child, tmpl, um);
        }

        // Global sibling sections (ReverbAlgorithm / PreComp / PostEQ / PostExp):
        // match the template's own type so we never have to name them here.
        auto backfillGlobal = [um, &reverbs] (juce::ValueTree tmpl)
        {
            auto existing = reverbs.getChildWithName (tmpl.getType());
            if (existing.isValid())
                backfillFromTemplate (existing, tmpl, um);
            else
                reverbs.appendChild (tmpl.createCopy(), um);
        };
        backfillGlobal (createReverbAlgorithmSection());
        backfillGlobal (createReverbPreCompSection());
        backfillGlobal (createReverbPostEQSection());
        backfillGlobal (createReverbPostExpSection());
    }
}

void WFSValueTreeState::migrateADMOSCSection()
{
    auto config = state.getChildWithName (Config);
    if (!config.isValid()) return;

    auto admosc = config.getChildWithName (ADMOSC);
    if (!admosc.isValid())
    {
        // No ADMOSC section at all — create fresh
        createADMOSCSection (config);
        return;
    }

    // Detect old-style flat ADMOSC (has admOscOffsetX property)
    if (!admosc.hasProperty (admOscOffsetX)) return;  // Already new format

    // Read old values
    float oldOffsetX = static_cast<float> (admosc.getProperty (admOscOffsetX, 0.0f));
    float oldOffsetY = static_cast<float> (admosc.getProperty (juce::Identifier ("admOscOffsetY"), 0.0f));
    float oldOffsetZ = static_cast<float> (admosc.getProperty (juce::Identifier ("admOscOffsetZ"), 0.0f));
    float oldScaleX  = static_cast<float> (admosc.getProperty (admOscScaleX, 1.0f));
    float oldScaleY  = static_cast<float> (admosc.getProperty (juce::Identifier ("admOscScaleY"), 1.0f));
    float oldScaleZ  = static_cast<float> (admosc.getProperty (juce::Identifier ("admOscScaleZ"), 1.0f));
    int oldFlipX     = static_cast<int>   (admosc.getProperty (admOscFlipX, 0));
    int oldFlipY     = static_cast<int>   (admosc.getProperty (juce::Identifier ("admOscFlipY"), 0));
    int oldFlipZ     = static_cast<int>   (admosc.getProperty (juce::Identifier ("admOscFlipZ"), 0));

    // Remove old ADMOSC node
    config.removeChild (admosc, nullptr);

    // Create new structure
    createADMOSCSection (config);

    // Apply old values to Cart mapping 0
    auto newAdmosc = config.getChildWithName (ADMOSC);
    auto cartMapping0 = newAdmosc.getChildWithName (ADMCartMapping);
    if (!cartMapping0.isValid()) return;

    float oldOffsets[3] = { oldOffsetX, oldOffsetY, oldOffsetZ };
    float oldScales[3]  = { oldScaleX,  oldScaleY,  oldScaleZ };
    int   oldFlips[3]   = { oldFlipX,   oldFlipY,   oldFlipZ };

    for (int a = 0; a < 3; ++a)
    {
        auto axis = cartMapping0.getChild (a);
        if (!axis.isValid()) continue;

        axis.setProperty (admCartCenterOffset,  oldOffsets[a], nullptr);
        axis.setProperty (admCartSignFlip,      oldFlips[a], nullptr);
        // Convert scale to half-widths: old mapping was offset + v * scale
        // New piecewise: at breakpoint 0.5, inner = scale*0.5, outer = scale*0.5
        float hw = std::abs (oldScales[a]) * 0.5f;
        if (hw < admCartWidthMin) hw = admCartWidthDefault;
        axis.setProperty (admCartPosInnerWidth, hw, nullptr);
        axis.setProperty (admCartPosOuterWidth, hw, nullptr);
        axis.setProperty (admCartNegInnerWidth, hw, nullptr);
        axis.setProperty (admCartNegOuterWidth, hw, nullptr);
    }
}

void WFSValueTreeState::ensureInputAdmMappingProperty()
{
    auto inputs = state.getChildWithName (Inputs);
    if (!inputs.isValid()) return;

    for (int i = 0; i < inputs.getNumChildren(); ++i)
    {
        auto input = inputs.getChild (i);
        auto position = input.getChildWithName (Position);
        if (position.isValid() && !position.hasProperty (inputAdmMapping))
            position.setProperty (inputAdmMapping, inputAdmMappingDefault, nullptr);
    }
}

bool WFSValueTreeState::validateState (const juce::ValueTree& stateToValidate) const
{
    // Check root type
    if (stateToValidate.getType() != WFSProcessor)
        return false;

    // Check for required sections
    if (!stateToValidate.getChildWithName (Config).isValid())
        return false;
    if (!stateToValidate.getChildWithName (Inputs).isValid())
        return false;
    if (!stateToValidate.getChildWithName (Outputs).isValid())
        return false;

    return true;
}

void WFSValueTreeState::copyStateFrom (const WFSValueTreeState& other)
{
    replaceState (other.state);
}

//==============================================================================
// TreeParameterStore seams (change-notification dispatch hooks)
//==============================================================================
// The ValueTree::Listener plumbing lives in TreeParameterStore. Its
// valueTreePropertyChanged calls resolveChannelIndex, then the POST-WRITE
// HOOK (handlePostWrite — the WFS semantic invariants below), then
// notifyParameterListeners — the same order as the pre-split monolith.

int WFSValueTreeState::resolveChannelIndex (const juce::ValueTree& changedNode) const
{
    // Determine channel index if this is an input/output/reverb parameter.
    // The notified index is the SLOT (dense child index): for inputs the id
    // is the permanent channel number and the list may have gaps, so it must
    // go through the number->slot lookup — id - 1 would point at the wrong
    // channel. Outputs/reverbs stay dense (id == index + 1).
    auto slotOf = [this] (const juce::ValueTree& node) -> int
    {
        if (node.getType() == Input)
            return getSlotForChannelNumber (static_cast<int> (node.getProperty (id)));
        return static_cast<int> (node.getProperty (id)) - 1;
    };

    int channelIndex = -1;
    auto parent = changedNode.getParent();

    if (parent.isValid())
    {
        if (parent.getType() == Input || parent.getType() == Output || parent.getType() == Reverb)
            channelIndex = slotOf (parent);
        else if (parent.getParent().isValid() &&
                 (parent.getParent().getType() == Input || parent.getParent().getType() == Output ||
                  parent.getParent().getType() == Reverb))
            channelIndex = slotOf (parent.getParent());
    }

    return channelIndex;
}

void WFSValueTreeState::handlePostWrite (juce::ValueTree& changedNode, const juce::Identifier& property,
                                         const juce::var& value, int channelIndex)
{
    // Enforce tracking constraint: only one tracked input per cluster
    // This catches changes from OSC, file loading, and any other source
    if (property == inputTrackingActive && channelIndex >= 0)
    {
        enforceClusterTrackingConstraint (channelIndex);
    }
    else if (property == inputCluster && channelIndex >= 0)
    {
        // When cluster assignment changes, also check constraint
        enforceClusterTrackingConstraint (channelIndex);

        // And enforce the Shared-Position invariant on the new cluster so
        // a freshly assigned input snaps to the cluster's shared position.
        int newCluster = static_cast<int> (value);
        if (newCluster >= 1)
            enforceSharedClusterInvariant (newCluster);
    }
    else if (property == clusterReferenceMode &&
             changedNode.getType() == Cluster)
    {
        // Reference mode flipped (from OSC, MCP, file load, etc.). If the
        // cluster just entered Shared Position, snap all its members to the
        // first-ordered member's position. Idempotent for non-shared modes.
        int clusterIdx = static_cast<int> (changedNode.getProperty (id));
        if (clusterIdx >= 1 && static_cast<int> (value) == 2)
            enforceSharedClusterInvariant (clusterIdx);
    }
}

//==============================================================================
// Initialization
//==============================================================================

void WFSValueTreeState::initializeDefaultState()
{
    state = juce::ValueTree (WFSProcessor);
    state.setProperty (version, "1.0", nullptr);

    createConfigSection();
    createInputsSection();
    createOutputsSection();
    createReverbsSection();
    createAudioPatchSection();
}

void WFSValueTreeState::createConfigSection()
{
    juce::ValueTree config (Config);

    createShowSection (config);
    createIOSection (config);
    createStageSection (config);
    createMasterSection (config);
    createNetworkSection (config);
    createADMOSCSection (config);
    createTrackingSection (config);
    createClustersSection (config);
    createBinauralSection (config);
    createUISection (config);

    state.appendChild (config, nullptr);
}

void WFSValueTreeState::createShowSection (juce::ValueTree& config)
{
    juce::ValueTree show (Show);
    show.setProperty (showName, showNameDefault, nullptr);
    show.setProperty (showLocation, showLocationDefault, nullptr);
    show.setProperty (autoPreselectDirty, false, nullptr);
    config.appendChild (show, nullptr);
}

void WFSValueTreeState::createIOSection (juce::ValueTree& config)
{
    juce::ValueTree io (IO);
    io.setProperty (inputChannels, inputChannelsDefault, nullptr);
    io.setProperty (outputChannels, outputChannelsDefault, nullptr);
    io.setProperty (reverbChannels, reverbChannelsDefault, nullptr);
    io.setProperty (algorithmDSP, algorithmDSPDefault, nullptr);
    io.setProperty (runDSP, runDSPDefault, nullptr);
    config.appendChild (io, nullptr);
}

void WFSValueTreeState::createStageSection (juce::ValueTree& config)
{
    juce::ValueTree stage (Stage);
    stage.setProperty (stageShape, stageShapeDefault, nullptr);
    stage.setProperty (stageWidth, stageWidthDefault, nullptr);
    stage.setProperty (stageDepth, stageDepthDefault, nullptr);
    stage.setProperty (stageHeight, stageHeightDefault, nullptr);
    stage.setProperty (stageDiameter, stageDiameterDefault, nullptr);
    stage.setProperty (domeElevation, domeElevationDefault, nullptr);
    stage.setProperty (originWidth, originWidthDefault, nullptr);
    stage.setProperty (originDepth, originDepthDefault, nullptr);
    stage.setProperty (originHeight, originHeightDefault, nullptr);
    stage.setProperty (speedOfSound, speedOfSoundDefault, nullptr);
    stage.setProperty (temperature, temperatureDefault, nullptr);
    config.appendChild (stage, nullptr);
}

void WFSValueTreeState::createMasterSection (juce::ValueTree& config)
{
    juce::ValueTree master (Master);
    master.setProperty (masterLevel, masterLevelDefault, nullptr);
    master.setProperty (systemLatency, systemLatencyDefault, nullptr);
    master.setProperty (haasEffect, haasEffectDefault, nullptr);
    master.setProperty (reverbsMapVisible, 1, nullptr);  // Default: visible
    config.appendChild (master, nullptr);
}

void WFSValueTreeState::migrateStrayConfigProperties()
{
    // A handful of config parameters were never stamped by any createXSection.
    // They came into being the first time the GUI wrote one, and
    // WfsParameters::setConfigParamBySection put them wherever its prefix tests
    // led - which for a lowercase name like "trackingMqttHost" is none of them,
    // so they landed in <IO>, the fallback. They worked, because getConfigParam
    // searches every section and takes the first hit.
    //
    // They are stamped in their proper sections now, which means a file written
    // before this change would carry the real value in <IO> AND a fresh default
    // in the right section. Both lookups take the first hit in the same order, so
    // nothing would break - but the duplicate is the kind of thing that bites
    // later, when someone edits the one nobody reads. Move the value across and
    // drop the stray.
    //
    // Runs BEFORE ensureCompleteSchema, so the value is already in place when the
    // backfill looks, and no default is stamped over it.
    auto config = getConfigState();
    if (! config.isValid())
        return;

    auto io = config.getChildWithName (IO);
    if (! io.isValid())
        return;

    struct Stray { juce::Identifier property; juce::Identifier section; };
    static const Stray strays[] = {
        { networkOscQueryEnabled, Network },  { networkOscQueryPort,  Network },
        { networkOscSourceFilter, Network },  { trackingOscPath,      Tracking },
        { trackingPsnInterface,   Tracking }, { trackingMqttHost,     Tracking },
        { trackingMqttTopic,      Tracking }, { trackingMqttTagIds,   Tracking },
        { trackingMqttJsonX,      Tracking }, { trackingMqttJsonY,    Tracking },
        { trackingMqttJsonZ,      Tracking }, { trackingMqttJsonQ,    Tracking },
        { samplerControllerMode,  UI },
    };

    for (const auto& stray : strays)
    {
        if (! io.hasProperty (stray.property))
            continue;

        auto target = config.getChildWithName (stray.section);
        if (! target.isValid())
            continue;   // section missing: leave the value where it is rather than lose it

        target.setProperty (stray.property, io.getProperty (stray.property), nullptr);
        io.removeProperty (stray.property, nullptr);
    }

    // colorScheme is the same story with a rename on top: the GUI wrote it under
    // the unmapped name "ColorScheme", so older files carry that spelling on <IO>
    // while the MCP surface has always advertised "colorScheme". Carry the value
    // across to the real identifier so an operator's chosen theme survives.
    static const juce::Identifier legacyColorScheme ("ColorScheme");
    if (io.hasProperty (legacyColorScheme))
    {
        auto ui = config.getChildWithName (UI);
        if (ui.isValid())
            ui.setProperty (colorScheme, io.getProperty (legacyColorScheme), nullptr);
        io.removeProperty (legacyColorScheme, nullptr);
    }
}

void WFSValueTreeState::stripObsoleteReverbProperties()
{
    // reverbLSenable (Live Source attenuation on reverb feeds) was removed on
    // 2026-08-28 without ever having been wired: the Tamer computes gains per
    // (input, speaker) and the feed matrix never read them. Files written
    // before that carry the attribute on every <Feed>; mergeTreeRecursive
    // never removes a property, so without this it would ride along in the
    // live tree and be re-saved forever. Same shape as legacyColorScheme
    // above: a local identifier, because the real one no longer exists.
    static const juce::Identifier legacyReverbLSenable ("reverbLSenable");

    const int n = getNumReverbChannels();
    for (int i = 0; i < n; ++i)
    {
        auto feed = getReverbFeedSection (i);
        if (feed.isValid() && feed.hasProperty (legacyReverbLSenable))
            feed.removeProperty (legacyReverbLSenable, nullptr);
    }
}

void WFSValueTreeState::createNetworkSection (juce::ValueTree& config)
{
    juce::ValueTree network (Network);
    network.setProperty (networkInterface, "", nullptr);
    network.setProperty (networkCurrentIP, networkCurrentIPDefault, nullptr);
    network.setProperty (networkRxUDPport, networkRxUDPportDefault, nullptr);
    network.setProperty (networkRxTCPport, networkRxTCPportDefault, nullptr);
    network.setProperty (findDevicePassword, findDevicePasswordDefault, nullptr);
    // Stamped so they exist from the start. Until now these came into being only
    // when the GUI first wrote one, which left every generic reader and writer -
    // MCP among them - addressing a property that was not there yet.
    network.setProperty (networkOscQueryEnabled, 0,    nullptr);
    network.setProperty (networkOscQueryPort,    5005, nullptr);
    network.setProperty (networkOscSourceFilter, 0,    nullptr);
    config.appendChild (network, nullptr);
}

void WFSValueTreeState::createADMOSCSection (juce::ValueTree& config)
{
    juce::ValueTree admosc (ADMOSC);

    // Create 4 Cartesian mappings
    for (int m = 0; m < admCartMappingCount; ++m)
    {
        juce::ValueTree mapping (ADMCartMapping);
        mapping.setProperty (id, m, nullptr);

        for (int a = 0; a < 3; ++a)
        {
            juce::ValueTree axis (ADMCartAxis);
            axis.setProperty (admCartAxisId,        a, nullptr);
            axis.setProperty (admCartAxisSwap,      a, nullptr);  // identity: X→X, Y→Y, Z→Z

            // ADM-OSC y=forward vs WFS y=upstage are opposite conventions,
            // so the Y axis needs a sign flip by default to get audience-side
            // sources to land on the audience side of the stage.
            const int signFlip = (a == 1) ? 1 : admCartSignFlipDefault;
            axis.setProperty (admCartSignFlip,      signFlip, nullptr);
            axis.setProperty (admCartCenterOffset,  admCartCenterOffsetDefault, nullptr);
            axis.setProperty (admCartBreakpoint,    admCartBreakpointDefault, nullptr);
            axis.setProperty (admCartPosInnerWidth, admCartWidthDefault, nullptr);
            axis.setProperty (admCartPosOuterWidth, admCartWidthDefault, nullptr);
            axis.setProperty (admCartNegInnerWidth, admCartWidthDefault, nullptr);
            axis.setProperty (admCartNegOuterWidth, admCartWidthDefault, nullptr);
            mapping.appendChild (axis, nullptr);
        }
        admosc.appendChild (mapping, nullptr);
    }

    // Create 4 Polar mappings
    for (int m = 0; m < admPolarMappingCount; ++m)
    {
        juce::ValueTree mapping (ADMPolarMapping);
        mapping.setProperty (id,                    m, nullptr);
        mapping.setProperty (admPolarAzimuthOffset,  admPolarAzimuthOffsetDefault, nullptr);
        mapping.setProperty (admPolarAzimuthFlip,    admPolarAzimuthFlipDefault, nullptr);
        mapping.setProperty (admPolarElevationFlip,  admPolarElevationFlipDefault, nullptr);
        mapping.setProperty (admPolarDistMin,        admPolarDistMinDefault, nullptr);
        mapping.setProperty (admPolarDistMax,        admPolarDistMaxDefault, nullptr);
        admosc.appendChild (mapping, nullptr);
    }

    config.appendChild (admosc, nullptr);
}

void WFSValueTreeState::createTrackingSection (juce::ValueTree& config)
{
    juce::ValueTree tracking (Tracking);
    tracking.setProperty (trackingEnabled, trackingEnabledDefault, nullptr);
    tracking.setProperty (trackingProtocol, trackingProtocolDefault, nullptr);
    tracking.setProperty (trackingPort, trackingPortDefault, nullptr);
    tracking.setProperty (trackingOffsetX, trackingOffsetDefault, nullptr);
    tracking.setProperty (trackingOffsetY, trackingOffsetDefault, nullptr);
    tracking.setProperty (trackingOffsetZ, trackingOffsetDefault, nullptr);
    tracking.setProperty (trackingScaleX, trackingScaleDefault, nullptr);
    tracking.setProperty (trackingScaleY, trackingScaleDefault, nullptr);
    tracking.setProperty (trackingScaleZ, trackingScaleDefault, nullptr);
    tracking.setProperty (trackingFlipX, trackingFlipDefault, nullptr);
    tracking.setProperty (trackingFlipY, trackingFlipDefault, nullptr);
    tracking.setProperty (trackingFlipZ, trackingFlipDefault, nullptr);
    // Same story as the OSCQuery trio above. Defaults match what the GUI falls
    // back to when the property is absent, so stamping them changes nothing an
    // operator can see.
    tracking.setProperty (trackingOscPath,      "/wfs/tracking <ID> <x> <y> <z>", nullptr);
    tracking.setProperty (trackingPsnInterface, "",                               nullptr);
    tracking.setProperty (trackingMqttHost,     "192.168.1.1",                    nullptr);
    tracking.setProperty (trackingMqttTopic,    "dwm/node/+/uplink/location",     nullptr);
    tracking.setProperty (trackingMqttTagIds,   "",                               nullptr);
    tracking.setProperty (trackingMqttJsonX,    "x",                              nullptr);
    tracking.setProperty (trackingMqttJsonY,    "y",                              nullptr);
    tracking.setProperty (trackingMqttJsonZ,    "z",                              nullptr);
    tracking.setProperty (trackingMqttJsonQ,    "quality",                        nullptr);
    config.appendChild (tracking, nullptr);
}

void WFSValueTreeState::createClustersSection (juce::ValueTree& config)
{
    juce::ValueTree clusters (Clusters);
    clusters.setProperty (count, maxClusters, nullptr);

    // Create 10 cluster entries
    for (int i = 0; i < maxClusters; ++i)
    {
        juce::ValueTree cluster (Cluster);
        cluster.setProperty (id, i + 1, nullptr);
        cluster.setProperty (clusterReferenceMode, clusterReferenceModeDefault, nullptr);
        cluster.setProperty (clusterInputOrder, clusterInputOrderDefault, nullptr);
        cluster.setProperty (clusterInputsVisible, clusterInputsVisibleDefault, nullptr);

        // Cluster LFO section
        juce::ValueTree lfo (ClusterLFO);
        lfo.setProperty (clusterLFOactive,         clusterLFOactiveDefault,          nullptr);
        lfo.setProperty (clusterLFOperiod,         clusterLFOperiodDefault,          nullptr);
        lfo.setProperty (clusterLFOphase,          clusterLFOphaseDefault,           nullptr);
        lfo.setProperty (clusterLFOshapeX,         clusterLFOshapeDefault,           nullptr);
        lfo.setProperty (clusterLFOshapeY,         clusterLFOshapeDefault,           nullptr);
        lfo.setProperty (clusterLFOshapeZ,         clusterLFOshapeDefault,           nullptr);
        lfo.setProperty (clusterLFOshapeRot,       clusterLFOshapeDefault,           nullptr);
        lfo.setProperty (clusterLFOshapeScale,     clusterLFOshapeDefault,           nullptr);
        lfo.setProperty (clusterLFOrateX,          clusterLFOrateDefault,            nullptr);
        lfo.setProperty (clusterLFOrateY,          clusterLFOrateDefault,            nullptr);
        lfo.setProperty (clusterLFOrateZ,          clusterLFOrateDefault,            nullptr);
        lfo.setProperty (clusterLFOrateRot,        clusterLFOrateDefault,            nullptr);
        lfo.setProperty (clusterLFOrateScale,      clusterLFOrateDefault,            nullptr);
        lfo.setProperty (clusterLFOamplitudeX,     clusterLFOamplitudeXYZDefault,    nullptr);
        lfo.setProperty (clusterLFOamplitudeY,     clusterLFOamplitudeXYZDefault,    nullptr);
        lfo.setProperty (clusterLFOamplitudeZ,     clusterLFOamplitudeXYZDefault,    nullptr);
        lfo.setProperty (clusterLFOamplitudeRot,   clusterLFOamplitudeRotDefault,    nullptr);
        lfo.setProperty (clusterLFOamplitudeScale, clusterLFOamplitudeScaleDefault,  nullptr);
        lfo.setProperty (clusterLFOphaseX,         clusterLFOphaseDefault,           nullptr);
        lfo.setProperty (clusterLFOphaseY,         clusterLFOphaseDefault,           nullptr);
        lfo.setProperty (clusterLFOphaseZ,         clusterLFOphaseDefault,           nullptr);
        lfo.setProperty (clusterLFOphaseRot,       clusterLFOphaseDefault,           nullptr);
        lfo.setProperty (clusterLFOphaseScale,     clusterLFOphaseDefault,           nullptr);
        cluster.appendChild (lfo, nullptr);

        clusters.appendChild (cluster, nullptr);
    }

    config.appendChild (clusters, nullptr);
}

void WFSValueTreeState::createBinauralSection (juce::ValueTree& config)
{
    juce::ValueTree binaural (Binaural);
    binaural.setProperty (binauralEnabled, binauralEnabledDefault, nullptr);
    binaural.setProperty (binauralSoloMode, binauralSoloModeDefault, nullptr);
    binaural.setProperty (binauralOutputChannel, binauralOutputChannelDefault, nullptr);
    binaural.setProperty (binauralListenerDistance, binauralListenerDistanceDefault, nullptr);
    binaural.setProperty (binauralListenerAngle, binauralListenerAngleDefault, nullptr);
    binaural.setProperty (binauralAttenuation, binauralAttenuationDefault, nullptr);
    binaural.setProperty (binauralDelay, binauralDelayDefault, nullptr);
    // (solo is per-channel: Channel.inputSolo — no csv here anymore)
    config.appendChild (binaural, nullptr);
}

void WFSValueTreeState::createUISection (juce::ValueTree& config)
{
    juce::ValueTree ui (WFSParameterIDs::UI);
    ui.setProperty (streamDeckEnabled, streamDeckEnabledDefault, nullptr);
    ui.setProperty (samplerEnabled, samplerEnabledDefault, nullptr);
    ui.setProperty (samplerBlockSerial, "", nullptr);
    ui.setProperty (lightpadPad0Split, lightpadSplitDefault, nullptr);
    ui.setProperty (lightpadPad1Split, lightpadSplitDefault, nullptr);
    ui.setProperty (lightpadPad2Split, lightpadSplitDefault, nullptr);
    ui.setProperty (lightpadPad0DeviceId, "", nullptr);
    ui.setProperty (lightpadPad1DeviceId, "", nullptr);
    ui.setProperty (lightpadPad2DeviceId, "", nullptr);
    ui.setProperty (lightpadSensitivity, lightpadSensitivityDefault, nullptr);
    ui.setProperty (samplerControllerMode, 0, nullptr);
    ui.setProperty (colorScheme, 0, nullptr);
    config.appendChild (ui, nullptr);
}

void WFSValueTreeState::createInputsSection()
{
    juce::ValueTree inputs (Inputs);
    inputs.setProperty (count, inputChannelsDefault, nullptr);

    for (int i = 0; i < inputChannelsDefault; ++i)
        inputs.appendChild (createDefaultInputChannel (i), nullptr);

    state.appendChild (inputs, nullptr);
}

void WFSValueTreeState::createOutputsSection()
{
    juce::ValueTree outputs (Outputs);
    outputs.setProperty (count, outputChannelsDefault, nullptr);

    for (int i = 0; i < outputChannelsDefault; ++i)
        outputs.appendChild (createDefaultOutputChannel (i), nullptr);

    state.appendChild (outputs, nullptr);
}

void WFSValueTreeState::createReverbsSection()
{
    juce::ValueTree reverbs (Reverbs);
    reverbs.setProperty (count, reverbChannelsDefault, nullptr);

    // Create reverb channels based on default count (typically 0)
    for (int i = 0; i < reverbChannelsDefault; ++i)
        reverbs.appendChild (createDefaultReverbChannel (i, reverbChannelsDefault), nullptr);

    // Create global algorithm section
    reverbs.appendChild (createReverbAlgorithmSection(), nullptr);

    // Create global pre-compressor section
    reverbs.appendChild (createReverbPreCompSection(), nullptr);

    // Create global post-processing EQ section
    reverbs.appendChild (createReverbPostEQSection(), nullptr);

    // Create global post-expander section
    reverbs.appendChild (createReverbPostExpSection(), nullptr);

    state.appendChild (reverbs, nullptr);
}

void WFSValueTreeState::createAudioPatchSection()
{
    juce::ValueTree audioPatch (AudioPatch);
    audioPatch.setProperty (driverMode, driverModeDefault, nullptr);
    audioPatch.setProperty (audioInterface, audioInterfaceDefault, nullptr);

    // Create input patch matrix (diagonal by default)
    juce::ValueTree inputPatchTree (InputPatch);
    inputPatchTree.setProperty (rows, inputChannelsDefault, nullptr);
    inputPatchTree.setProperty (cols, maxInputChannels, nullptr);

    juce::StringArray inputPatchData;
    for (int r = 0; r < inputChannelsDefault; ++r)
    {
        juce::StringArray row;
        for (int c = 0; c < maxInputChannels; ++c)
            row.add (r == c ? "1" : "0");
        inputPatchData.add (row.joinIntoString (","));
    }
    inputPatchTree.setProperty (patchData, inputPatchData.joinIntoString (";"), nullptr);
    audioPatch.appendChild (inputPatchTree, nullptr);

    // Create output patch matrix (diagonal by default)
    juce::ValueTree outputPatchTree (OutputPatch);
    outputPatchTree.setProperty (rows, outputChannelsDefault, nullptr);
    outputPatchTree.setProperty (cols, maxOutputChannels, nullptr);

    juce::StringArray outputPatchData;
    for (int r = 0; r < outputChannelsDefault; ++r)
    {
        juce::StringArray row;
        for (int c = 0; c < maxOutputChannels; ++c)
            row.add (r == c ? "1" : "0");
        outputPatchData.add (row.joinIntoString (","));
    }
    outputPatchTree.setProperty (patchData, outputPatchData.joinIntoString (";"), nullptr);
    audioPatch.appendChild (outputPatchTree, nullptr);

    state.appendChild (audioPatch, nullptr);
}

juce::ValueTree WFSValueTreeState::createDefaultInputChannel (int index, int totalInputsIn, int channelNumber)
{
    // The caller must pass the TARGET count when it is growing the channel
    // list. setNumInputChannels creates the new channels first and only writes
    // the new count afterwards, so reading it from the tree here yielded the
    // OLD count: growing 8 -> 64 gave every new channel numRows = 1, and
    // getDefaultInputPosition's fracY = (row+1)/(numRows+1) then ran past 1 —
    // index 63 landed at fracY = 4, i.e. four stage-depths off the front edge.
    int totalInputs = totalInputsIn;
    if (totalInputs <= 0)
    {
        totalInputs = inputChannelsDefault;
        auto io = getIOState();
        if (io.isValid())
            totalInputs = static_cast<int> (io.getProperty (inputChannels));
    }
    totalInputs = juce::jmax (1, totalInputs, index + 1);

    // The permanent channel number defaults to index + 1 (dense creation);
    // addInputChannel passes it explicitly, since with gaps in the list the
    // number and the slot no longer coincide. Id and tracking id follow the
    // NUMBER; the position default follows the SLOT (grid layout).
    const int number = channelNumber > 0 ? channelNumber : index + 1;

    juce::ValueTree input (Input);
    input.setProperty (id, number, nullptr);
    input.setProperty (inputChannelType, inputChannelTypeMono, nullptr);

    // Born mono, so the number doubles as the mono ordinal — exact for the dense
    // all-mono list initializeDefaultState builds. A caller creating anything
    // else (addInputChannel) overwrites the type and the name together.
    input.appendChild (createInputChannelSection (false, number), nullptr);
    input.appendChild (createInputPositionSection (index, totalInputs), nullptr);
    input.appendChild (createInputAttenuationSection(), nullptr);
    input.appendChild (createInputDirectivitySection(), nullptr);
    input.appendChild (createInputLiveSourceSection(), nullptr);
    input.appendChild (createInputHackousticsSection(), nullptr);
    input.appendChild (createInputLFOSection(), nullptr);
    input.appendChild (createInputAutoMotionSection(), nullptr);
    input.appendChild (createInputMutesSection (getNumOutputChannels()), nullptr);
    input.appendChild (createInputGradientMapsSection(), nullptr);
    input.appendChild (createInputSamplerSection(), nullptr);

    // Tracking id follows the permanent number, not the slot (the position
    // section builder only knows the slot).
    auto position = input.getChildWithName (Position);
    if (position.isValid())
        position.setProperty (inputTrackingID, number, nullptr);

    return input;
}

juce::ValueTree WFSValueTreeState::createInputChannelSection (bool stereo, int ordinal)
{
    juce::ValueTree channel (Channel);
    channel.setProperty (inputName, getDefaultInputNameForType (stereo, ordinal), nullptr);
    channel.setProperty (inputColour, inputColourDefault, nullptr);   // Default: -1 = auto
    channel.setProperty (inputSolo, 0, nullptr);
    channel.setProperty (inputStereoWidth, inputStereoWidthDefault, nullptr);
    channel.setProperty (inputStereoAxisOffset, inputStereoAxisOffsetDefault, nullptr);
    channel.setProperty (inputStereoAxisLock, inputStereoAxisLockDefault, nullptr);
    channel.setProperty (inputAttenuation, inputAttenuationDefault, nullptr);
    channel.setProperty (inputDelayLatency, inputDelayLatencyDefault, nullptr);
    channel.setProperty (inputMinimalLatency, inputMinimalLatencyDefault, nullptr);
    channel.setProperty (inputMapLocked, 0, nullptr);    // Default: unlocked
    channel.setProperty (inputMapVisible, 1, nullptr);   // Default: visible
    channel.setProperty (inputHiddenByCluster, 0, nullptr);  // Default: not hidden by a cluster toggle
    channel.setProperty (inputSamplerActive, inputSamplerActiveDefault, nullptr);
    channel.setProperty (samplerMidiZoneQuadrant, samplerMidiZoneQuadrantDefault, nullptr);
    channel.setProperty (lightpadZoneId, lightpadZoneIdDefault, nullptr);
    return channel;
}

juce::ValueTree WFSValueTreeState::createInputPositionSection (int index, int totalInputs)
{
    juce::ValueTree position (Position);

    // Calculate default position
    float x, y, z;
    auto stageTree = getStageState();
    float sw = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (stageWidth)) : stageWidthDefault;
    float sd = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (stageDepth)) : stageDepthDefault;
    float sh = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (stageHeight)) : stageHeightDefault;
    float ow = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (originWidth)) : originWidthDefault;
    float od = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (originDepth)) : originDepthDefault;
    float oh = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (originHeight)) : originHeightDefault;

    getDefaultInputPosition (index, totalInputs, sw, sd, sh, ow, od, oh, x, y, z);

    position.setProperty (inputPositionX, x, nullptr);
    position.setProperty (inputPositionY, y, nullptr);
    position.setProperty (inputPositionZ, z, nullptr);
    position.setProperty (inputOffsetX, inputOffsetDefault, nullptr);
    position.setProperty (inputOffsetY, inputOffsetDefault, nullptr);
    position.setProperty (inputOffsetZ, inputOffsetDefault, nullptr);
    position.setProperty (inputConstraintX, inputConstraintDefault, nullptr);
    position.setProperty (inputConstraintY, inputConstraintDefault, nullptr);
    position.setProperty (inputConstraintZ, inputConstraintDefault, nullptr);
    position.setProperty (inputConstraintDistance, inputConstraintDistanceDefault, nullptr);
    position.setProperty (inputConstraintDistanceMin, inputConstraintDistanceMinDefault, nullptr);
    position.setProperty (inputConstraintDistanceMax, inputConstraintDistanceMaxDefault, nullptr);
    position.setProperty (inputFlipX, inputFlipDefault, nullptr);
    position.setProperty (inputFlipY, inputFlipDefault, nullptr);
    position.setProperty (inputFlipZ, inputFlipDefault, nullptr);
    position.setProperty (inputCluster, inputClusterDefault, nullptr);
    position.setProperty (inputTrackingActive, inputTrackingActiveDefault, nullptr);
    position.setProperty (inputTrackingID, index + 1, nullptr);  // Default to channel index
    position.setProperty (inputTrackingSmooth, inputTrackingSmoothDefault, nullptr);
    position.setProperty (inputMaxSpeedActive, inputMaxSpeedActiveDefault, nullptr);
    position.setProperty (inputMaxSpeed, inputMaxSpeedDefault, nullptr);
    position.setProperty (inputPathModeActive, inputPathModeActiveDefault, nullptr);
    position.setProperty (inputHeightFactor, inputHeightFactorDefault, nullptr);
    position.setProperty (inputCoordinateMode, inputCoordinateModeDefault, nullptr);
    position.setProperty (inputAdmMapping, inputAdmMappingDefault, nullptr);
    position.setProperty (inputJitter, inputJitterDefault, nullptr);

    return position;
}

juce::ValueTree WFSValueTreeState::createInputAttenuationSection()
{
    juce::ValueTree attenuation (Attenuation);
    // Note: inputAttenuation itself lives in the Channel section (that is where the
    // GUI/OSC/snapshot system and the calc engine read+write it). The Attenuation
    // section only holds the distance law, ratio and common-attenuation parameters.
    attenuation.setProperty (inputAttenuationLaw, inputAttenuationLawDefault, nullptr);
    attenuation.setProperty (inputDistanceAttenuation, inputDistanceAttenuationDefault, nullptr);
    attenuation.setProperty (inputDistanceRatio, inputDistanceRatioDefault, nullptr);
    attenuation.setProperty (inputCommonAtten, inputCommonAttenDefault, nullptr);
    return attenuation;
}

juce::ValueTree WFSValueTreeState::createInputDirectivitySection()
{
    juce::ValueTree directivity (Directivity);
    directivity.setProperty (inputDirectivity, inputDirectivityDefault, nullptr);
    directivity.setProperty (inputRotation, inputRotationDefault, nullptr);
    directivity.setProperty (inputTilt, inputTiltDefault, nullptr);
    directivity.setProperty (inputHFshelf, inputHFshelfDefault, nullptr);
    return directivity;
}

juce::ValueTree WFSValueTreeState::createInputLiveSourceSection()
{
    juce::ValueTree liveSource (LiveSourceTamer);
    liveSource.setProperty (inputLSactive, inputLSactiveDefault, nullptr);
    liveSource.setProperty (inputLSradius, inputLSradiusDefault, nullptr);
    liveSource.setProperty (inputLSshape, inputLSshapeDefault, nullptr);
    liveSource.setProperty (inputLSattenuation, inputLSattenuationDefault, nullptr);
    liveSource.setProperty (inputLSpeakEnable, inputLSpeakEnableDefault, nullptr);
    liveSource.setProperty (inputLSpeakThreshold, inputLSpeakThresholdDefault, nullptr);
    liveSource.setProperty (inputLSpeakRatio, inputLSpeakRatioDefault, nullptr);
    liveSource.setProperty (inputLSslowEnable, inputLSslowEnableDefault, nullptr);
    liveSource.setProperty (inputLSslowThreshold, inputLSslowThresholdDefault, nullptr);
    liveSource.setProperty (inputLSslowRatio, inputLSslowRatioDefault, nullptr);
    return liveSource;
}

juce::ValueTree WFSValueTreeState::createInputHackousticsSection()
{
    juce::ValueTree hackoustics (Hackoustics);
    hackoustics.setProperty (inputFRactive, inputFRactiveDefault, nullptr);
    hackoustics.setProperty (inputFRattenuation, inputFRattenuationDefault, nullptr);
    hackoustics.setProperty (inputFRlowCutActive, inputFRlowCutActiveDefault, nullptr);
    hackoustics.setProperty (inputFRlowCutFreq, inputFRlowCutFreqDefault, nullptr);
    hackoustics.setProperty (inputFRhighShelfActive, inputFRhighShelfActiveDefault, nullptr);
    hackoustics.setProperty (inputFRhighShelfFreq, inputFRhighShelfFreqDefault, nullptr);
    hackoustics.setProperty (inputFRhighShelfGain, inputFRhighShelfGainDefault, nullptr);
    hackoustics.setProperty (inputFRhighShelfSlope, inputFRhighShelfSlopeDefault, nullptr);
    hackoustics.setProperty (inputFRdiffusion, inputFRdiffusionDefault, nullptr);
    return hackoustics;
}

juce::ValueTree WFSValueTreeState::createInputLFOSection()
{
    juce::ValueTree lfo (LFO);
    lfo.setProperty (inputLFOactive, inputLFOactiveDefault, nullptr);
    lfo.setProperty (inputLFOperiod, inputLFOperiodDefault, nullptr);
    lfo.setProperty (inputLFOphase, inputLFOphaseDefault, nullptr);
    lfo.setProperty (inputLFOshapeX, inputLFOshapeDefault, nullptr);
    lfo.setProperty (inputLFOshapeY, inputLFOshapeDefault, nullptr);
    lfo.setProperty (inputLFOshapeZ, inputLFOshapeDefault, nullptr);
    lfo.setProperty (inputLFOrateX, inputLFOrateDefault, nullptr);
    lfo.setProperty (inputLFOrateY, inputLFOrateDefault, nullptr);
    lfo.setProperty (inputLFOrateZ, inputLFOrateDefault, nullptr);
    lfo.setProperty (inputLFOamplitudeX, inputLFOamplitudeDefault, nullptr);
    lfo.setProperty (inputLFOamplitudeY, inputLFOamplitudeDefault, nullptr);
    lfo.setProperty (inputLFOamplitudeZ, inputLFOamplitudeDefault, nullptr);
    lfo.setProperty (inputLFOphaseX, inputLFOphaseDefault, nullptr);
    lfo.setProperty (inputLFOphaseY, inputLFOphaseDefault, nullptr);
    lfo.setProperty (inputLFOphaseZ, inputLFOphaseDefault, nullptr);
    lfo.setProperty (inputLFOgyrophone, inputLFOgyrophoneDefault, nullptr);
    return lfo;
}

juce::ValueTree WFSValueTreeState::createInputAutoMotionSection()
{
    juce::ValueTree automOtion (AutomOtion);
    automOtion.setProperty (inputOtomoX, inputOtomoDefault, nullptr);
    automOtion.setProperty (inputOtomoY, inputOtomoDefault, nullptr);
    automOtion.setProperty (inputOtomoZ, inputOtomoDefault, nullptr);
    automOtion.setProperty (inputOtomoAbsoluteRelative, inputOtomoAbsoluteRelativeDefault, nullptr);
    automOtion.setProperty (inputOtomoStayReturn, inputOtomoStayReturnDefault, nullptr);
    automOtion.setProperty (inputOtomoSpeedProfile, inputOtomoSpeedProfileDefault, nullptr);
    automOtion.setProperty (inputOtomoDuration, inputOtomoDurationDefault, nullptr);
    automOtion.setProperty (inputOtomoCurve, inputOtomoCurveDefault, nullptr);
    automOtion.setProperty (inputOtomoTrigger, inputOtomoTriggerDefault, nullptr);
    automOtion.setProperty (inputOtomoThreshold, inputOtomoThresholdDefault, nullptr);
    automOtion.setProperty (inputOtomoReset, inputOtomoResetDefault, nullptr);
    automOtion.setProperty (inputOtomoPauseResume, inputOtomoPauseResumeDefault, nullptr);

    // Polar coordinate parameters
    automOtion.setProperty (inputOtomoCoordinateMode, inputOtomoCoordinateModeDefault, nullptr);
    automOtion.setProperty (inputOtomoR, inputOtomoRDefault, nullptr);
    automOtion.setProperty (inputOtomoTheta, inputOtomoThetaDefault, nullptr);
    automOtion.setProperty (inputOtomoRsph, inputOtomoRsphDefault, nullptr);
    automOtion.setProperty (inputOtomoPhi, inputOtomoPhiDefault, nullptr);

    return automOtion;
}

juce::ValueTree WFSValueTreeState::createInputMutesSection (int numOutputs)
{
    juce::ValueTree mutes (Mutes);

    // Create comma-separated string of zeros
    juce::StringArray muteArray;
    for (int i = 0; i < numOutputs; ++i)
        muteArray.add ("0");
    mutes.setProperty (inputMutes, muteArray.joinIntoString (","), nullptr);

    // Sidelines (auto-mute at stage edges)
    mutes.setProperty (inputSidelinesActive, inputSidelinesActiveDefault, nullptr);
    mutes.setProperty (inputSidelinesFringe, inputSidelinesFringeDefault, nullptr);

    // Mute reverb sends (default 0 = sends active)
    mutes.setProperty (inputMuteReverbSends, 0, nullptr);

    // Array attenuation (per-array level control, 0 dB default)
    mutes.setProperty (inputArrayAtten1, inputArrayAttenDefault, nullptr);
    mutes.setProperty (inputArrayAtten2, inputArrayAttenDefault, nullptr);
    mutes.setProperty (inputArrayAtten3, inputArrayAttenDefault, nullptr);
    mutes.setProperty (inputArrayAtten4, inputArrayAttenDefault, nullptr);
    mutes.setProperty (inputArrayAtten5, inputArrayAttenDefault, nullptr);
    mutes.setProperty (inputArrayAtten6, inputArrayAttenDefault, nullptr);
    mutes.setProperty (inputArrayAtten7, inputArrayAttenDefault, nullptr);
    mutes.setProperty (inputArrayAtten8, inputArrayAttenDefault, nullptr);
    mutes.setProperty (inputArrayAtten9, inputArrayAttenDefault, nullptr);
    mutes.setProperty (inputArrayAtten10, inputArrayAttenDefault, nullptr);

    return mutes;
}

juce::ValueTree WFSValueTreeState::createInputGradientMapsSection()
{
    using namespace WFSParameterIDs;
    using namespace WFSParameterDefaults;

    juce::ValueTree gm (GradientMaps);

    // Create 3 layers with default parameter assignments
    const int defaultParams[3] = { 0, 1, 2 };  // Attenuation, Height, HF Shelf

    for (int i = 0; i < maxGradientLayers; ++i)
    {
        juce::ValueTree layer (GradientLayer);
        layer.setProperty (WFSParameterIDs::id, i, nullptr);
        layer.setProperty (gmLayerEnabled,  gmLayerEnabledDefault, nullptr);
        layer.setProperty (gmLayerParam,    defaultParams[i], nullptr);
        layer.setProperty (gmLayerWhite,    gmLayerWhiteDefault, nullptr);
        layer.setProperty (gmLayerBlack,    gmLayerBlackDefault, nullptr);
        layer.setProperty (gmLayerCurve,    gmLayerCurveDefault, nullptr);
        layer.setProperty (gmLayerVisible,  gmLayerVisibleDefault, nullptr);
        gm.appendChild (layer, nullptr);
    }

    return gm;
}

juce::ValueTree WFSValueTreeState::getADMCartMapping (int mappingIndex)
{
    return findChildByIntProperty (getADMOSCState(), ADMCartMapping, id, mappingIndex);
}

/** NOTE: admPolarDistBreakpoint/Inner/Outer/Center are deliberately NOT stamped by
    createADMOSCSection, unlike the stray config properties fixed elsewhere in this
    series. ADMOSCMapping::loadPolarConfig uses the ABSENCE of admPolarDistInner to
    decide whether to migrate a legacy admPolarDistMin/Max pair; stamping defaults
    would send every pre-existing file down the new-style branch with default
    widths and quietly discard its real polar mapping. A write through this
    accessor creates the property, which is exactly the intended transition. */
juce::ValueTree WFSValueTreeState::getADMPolarMapping (int mappingIndex)
{
    return findChildByIntProperty (getADMOSCState(), ADMPolarMapping, id, mappingIndex);
}

juce::ValueTree WFSValueTreeState::getADMCartAxis (int mappingIndex, int axisIndex)
{
    auto mapping = getADMCartMapping (mappingIndex);
    if (! mapping.isValid())
        return {};
    return findChildByIntProperty (mapping, ADMCartAxis, admCartAxisId, axisIndex);
}

juce::ValueTree WFSValueTreeState::createInputSamplerSection()
{
    using namespace WFSParameterIDs;
    using namespace WFSParameterDefaults;

    juce::ValueTree sampler (Sampler);

    // Pre-create 36 cells (6x6 grid) with empty defaults
    for (int i = 0; i < samplerGridCells; ++i)
    {
        juce::ValueTree cell (SamplerCell);
        cell.setProperty (WFSParameterIDs::id, i, nullptr);
        cell.setProperty (samplerCellName, "", nullptr);
        cell.setProperty (samplerCellFile, "", nullptr);
        cell.setProperty (samplerCellInTime, samplerCellInTimeDefault, nullptr);
        cell.setProperty (samplerCellOutTime, samplerCellOutTimeDefault, nullptr);
        cell.setProperty (samplerCellOffsetX, samplerCellOffsetDefault, nullptr);
        cell.setProperty (samplerCellOffsetY, samplerCellOffsetDefault, nullptr);
        cell.setProperty (samplerCellOffsetZ, samplerCellOffsetDefault, nullptr);
        cell.setProperty (samplerCellAttenuation, samplerCellAttenuationDefault, nullptr);
        sampler.appendChild (cell, nullptr);
    }

    // No sets by default — user creates them dynamically
    sampler.setProperty (inputSamplerActiveSet, inputSamplerActiveSetDefault, nullptr);

    return sampler;
}

juce::ValueTree WFSValueTreeState::createDefaultOutputChannel (int index)
{
    juce::ValueTree output (Output);
    output.setProperty (id, index + 1, nullptr);

    output.appendChild (createOutputChannelSection (index), nullptr);
    output.appendChild (createOutputPositionSection(), nullptr);
    output.appendChild (createOutputOptionsSection(), nullptr);
    output.appendChild (createOutputEQSection(), nullptr);

    return output;
}

juce::ValueTree WFSValueTreeState::createOutputChannelSection (int index)
{
    juce::ValueTree channel (Channel);
    channel.setProperty (outputName, getDefaultOutputName (index), nullptr);
    channel.setProperty (outputArray, outputArrayDefault, nullptr);
    channel.setProperty (outputApplyToArray, outputApplyToArrayDefault, nullptr);
    channel.setProperty (outputAttenuation, outputAttenuationDefault, nullptr);
    channel.setProperty (outputDelayLatency, outputDelayLatencyDefault, nullptr);
    channel.setProperty (outputMapVisible, 1, nullptr);       // Default: visible
    channel.setProperty (outputArrayMapVisible, 1, nullptr);  // Default: visible
    return channel;
}

juce::ValueTree WFSValueTreeState::createOutputPositionSection()
{
    juce::ValueTree position (Position);
    position.setProperty (outputPositionX, outputPositionDefault, nullptr);
    position.setProperty (outputPositionY, outputPositionDefault, nullptr);
    position.setProperty (outputPositionZ, outputPositionDefault, nullptr);
    position.setProperty (outputOrientation, outputOrientationDefault, nullptr);
    position.setProperty (outputAngleOn, outputAngleOnDefault, nullptr);
    position.setProperty (outputAngleOff, outputAngleOffDefault, nullptr);
    position.setProperty (outputPitch, outputPitchDefault, nullptr);
    position.setProperty (outputHFdamping, outputHFdampingDefault, nullptr);
    position.setProperty (outputCoordinateMode, outputCoordinateModeDefault, nullptr);
    return position;
}

juce::ValueTree WFSValueTreeState::createOutputOptionsSection()
{
    juce::ValueTree options (Options);
    options.setProperty (outputMiniLatencyEnable, outputMiniLatencyEnableDefault, nullptr);
    options.setProperty (outputLSattenEnable, outputLSattenEnableDefault, nullptr);
    options.setProperty (outputFRenable, outputFRenableDefault, nullptr);
    options.setProperty (outputDistanceAttenPercent, outputDistanceAttenPercentDefault, nullptr);
    options.setProperty (outputHparallax, outputHparallaxDefault, nullptr);
    options.setProperty (outputVparallax, outputVparallaxDefault, nullptr);
    return options;
}

juce::ValueTree WFSValueTreeState::createOutputEQSection()
{
    juce::ValueTree eq (EQ);
    eq.setProperty (outputEQenabled, outputEQenabledDefault, nullptr);

    for (int i = 0; i < numEQBands; ++i)
    {
        juce::ValueTree band (Band);
        band.setProperty (id, i + 1, nullptr);
        band.setProperty (eqShape, eqBandShapes[i], nullptr);
        band.setProperty (eqFrequency, eqBandFrequencies[i], nullptr);
        band.setProperty (eqGain, eqGainDefault, nullptr);
        band.setProperty (eqQ, eqQDefault, nullptr);
        band.setProperty (eqSlope, eqSlopeDefault, nullptr);
        eq.appendChild (band, nullptr);
    }

    return eq;
}

juce::ValueTree WFSValueTreeState::createDefaultReverbChannel (int index, int totalCount)
{
    juce::ValueTree reverb (Reverb);
    reverb.setProperty (id, index + 1, nullptr);

    reverb.appendChild (createReverbChannelSection (index), nullptr);
    reverb.appendChild (createReverbPositionSection (index, totalCount), nullptr);
    reverb.appendChild (createReverbFeedSection(), nullptr);
    reverb.appendChild (createReverbEQSection(), nullptr);
    reverb.appendChild (createReverbReturnSection (getNumOutputChannels()), nullptr);

    return reverb;
}

bool WFSValueTreeState::arePositionsUserOwned()
{
    auto stageTree = getStageState();
    return stageTree.isValid()
        && (bool) stageTree.getProperty (positionsUserOwned, false);
}

void WFSValueTreeState::markPositionsUserOwned()
{
    auto stageTree = getStageState();
    if (stageTree.isValid() && ! (bool) stageTree.getProperty (positionsUserOwned, false))
        stageTree.setProperty (positionsUserOwned, true, nullptr);   // no undo: see header
}

bool WFSValueTreeState::areChannelNumbersUserOwned()
{
    // An invalid IO tree reads as OWNED — the opposite fallback to
    // arePositionsUserOwned. Being "unowned" here licenses a rewrite of every
    // channel id, so malformed or half-built state must land on the permanent
    // regime; a wrong "fresh" verdict would renumber a real show.
    auto io = getIOState();
    return (! io.isValid())
        || (bool) io.getProperty (channelNumbersUserOwned, false);
}

void WFSValueTreeState::markChannelNumbersUserOwned (const juce::String& reason)
{
    auto io = getIOState();
    if (io.isValid() && ! (bool) io.getProperty (channelNumbersUserOwned, false))
    {
        io.setProperty (channelNumbersUserOwned, true, nullptr);   // no undo: see header
        WFSLogger::getInstance().logInfo ("Channel numbers latched: " + reason
                                          + " (structural edits keep permanent numbers from here on)");
    }
}

void WFSValueTreeState::redistributeAllReverbPositions()
{
    const int n = getNumReverbChannels();
    if (n <= 0)
        return;

    const auto nodes = ReverbNodePlacement::layout (getStageForPlacement(), n);

    beginUndoTransaction ("Redistribute Reverb Positions");

    // Writes go through setProperty directly, like redistributeAllInputPositions:
    // the engine re-laying its own nodes must not trip the user-ownership latch
    // that setReverbParameter carries.
    for (int i = 0; i < n; ++i)
    {
        auto pos = getReverbPositionSection (i);
        if (! pos.isValid()) continue;
        const auto& node = nodes[(size_t) i];
        pos.setProperty (reverbPositionX, node.x, getActiveUndoManager());
        pos.setProperty (reverbPositionY, node.y, getActiveUndoManager());
        pos.setProperty (reverbPositionZ, node.z, getActiveUndoManager());
    }
}

ReverbNodePlacement::Stage WFSValueTreeState::getStageForPlacement()
{
    ReverbNodePlacement::Stage s;
    auto stageTree = getStageState();
    if (! stageTree.isValid())
        return s;   // helper falls back to a nominal extent

    s.shape    = static_cast<int>   (stageTree.getProperty (stageShape,    0));
    s.width    = static_cast<float> (stageTree.getProperty (stageWidth,    stageWidthDefault));
    s.depth    = static_cast<float> (stageTree.getProperty (stageDepth,    stageDepthDefault));
    s.height   = static_cast<float> (stageTree.getProperty (stageHeight,   stageHeightDefault));
    s.diameter = static_cast<float> (stageTree.getProperty (stageDiameter, 0.0f));
    s.originW  = static_cast<float> (stageTree.getProperty (originWidth,  originWidthDefault));
    s.originD  = static_cast<float> (stageTree.getProperty (originDepth,  originDepthDefault));
    return s;
}

juce::ValueTree WFSValueTreeState::createReverbChannelSection (int index)
{
    juce::ValueTree channel (Channel);
    channel.setProperty (reverbName, getDefaultReverbName (index), nullptr);
    channel.setProperty (reverbAttenuation, reverbAttenuationDefault, nullptr);
    channel.setProperty (reverbDelayLatency, reverbDelayLatencyDefault, nullptr);
    return channel;
}

juce::ValueTree WFSValueTreeState::createReverbPositionSection (int index, int totalCount)
{
    juce::ValueTree position (Position);

    // Default layout: a semi-ellipse (box) or ring (cylinder/dome) at 1.5x the
    // stage, 2 m high, jittered to break symmetry and de-crowded in Z. The old
    // default put every node on a straight line along X at 1 m spacing with
    // Z = 0 — collinear, on the floor, and mirror-symmetrical, which also gave
    // the SDN inter-node delays of a few samples. See ReverbNodePlacement.h.
    //
    // Recomputes the whole layout per channel rather than threading it through
    // the callers: this is a setup path and the node count is <= 32.
    const auto nodes = ReverbNodePlacement::layout (getStageForPlacement(),
                                                    juce::jmax (1, totalCount));
    const auto& n = nodes[(size_t) juce::jlimit (0, (int) nodes.size() - 1, index)];

    position.setProperty (reverbPositionX, n.x, nullptr);
    position.setProperty (reverbPositionY, n.y, nullptr);
    position.setProperty (reverbPositionZ, n.z, nullptr);
    position.setProperty (reverbReturnOffsetX, reverbReturnOffsetDefault, nullptr);
    position.setProperty (reverbReturnOffsetY, reverbReturnOffsetDefault, nullptr);
    position.setProperty (reverbReturnOffsetZ, reverbReturnOffsetDefault, nullptr);
    position.setProperty (reverbCoordinateMode, reverbCoordinateModeDefault, nullptr);
    return position;
}

juce::ValueTree WFSValueTreeState::createReverbFeedSection()
{
    juce::ValueTree feed (Feed);
    feed.setProperty (reverbOrientation, reverbOrientationDefault, nullptr);
    feed.setProperty (reverbAngleOn, reverbAngleOnDefault, nullptr);
    feed.setProperty (reverbAngleOff, reverbAngleOffDefault, nullptr);
    feed.setProperty (reverbPitch, reverbPitchDefault, nullptr);
    feed.setProperty (reverbHFdamping, reverbHFdampingDefault, nullptr);
    feed.setProperty (reverbMiniLatencyEnable, reverbMiniLatencyEnableDefault, nullptr);
    feed.setProperty (reverbDistanceAttenEnable, reverbDistanceAttenEnableDefault, nullptr);
    return feed;
}

juce::ValueTree WFSValueTreeState::createReverbEQSection()
{
    juce::ValueTree eq (EQ);
    eq.setProperty (reverbPreEQenable, reverbPreEQenableDefault, nullptr);

    for (int i = 0; i < numReverbPreEQBands; ++i)
    {
        juce::ValueTree band (Band);
        band.setProperty (id, i + 1, nullptr);
        band.setProperty (reverbPreEQshape, reverbPreEQBandShapes[i], nullptr);
        band.setProperty (reverbPreEQfreq, reverbPreEQBandFrequencies[i], nullptr);
        band.setProperty (reverbPreEQgain, reverbPreEQgainDefault, nullptr);
        band.setProperty (reverbPreEQq, reverbPreEQqDefault, nullptr);
        band.setProperty (reverbPreEQslope, reverbPreEQslopeDefault, nullptr);
        eq.appendChild (band, nullptr);
    }

    return eq;
}

juce::ValueTree WFSValueTreeState::createReverbReturnSection (int numOutputs)
{
    juce::ValueTree returnSection (ReverbReturn);
    returnSection.setProperty (reverbDistanceAttenuation, reverbDistanceAttenuationDefault, nullptr);
    returnSection.setProperty (reverbCommonAtten, reverbCommonAttenDefault, nullptr);

    // Create comma-separated string of zeros for mutes
    juce::StringArray muteArray;
    int outputCount = numOutputs > 0 ? numOutputs : outputChannelsDefault;
    for (int i = 0; i < outputCount; ++i)
        muteArray.add ("0");
    returnSection.setProperty (reverbMutes, muteArray.joinIntoString (","), nullptr);

    returnSection.setProperty (reverbMuteMacro, reverbMuteMacroDefault, nullptr);
    return returnSection;
}

juce::ValueTree WFSValueTreeState::createReverbAlgorithmSection()
{
    juce::ValueTree algo (ReverbAlgorithm);
    algo.setProperty (reverbAlgoType,        reverbAlgoTypeDefault, nullptr);
    algo.setProperty (reverbRT60,            reverbRT60Default, nullptr);
    algo.setProperty (reverbRT60LowMult,     reverbRT60LowMultDefault, nullptr);
    algo.setProperty (reverbRT60HighMult,    reverbRT60HighMultDefault, nullptr);
    algo.setProperty (reverbCrossoverLow,    reverbCrossoverLowDefault, nullptr);
    algo.setProperty (reverbCrossoverHigh,   reverbCrossoverHighDefault, nullptr);
    algo.setProperty (reverbDiffusion,       reverbDiffusionDefault, nullptr);
    algo.setProperty (reverbSDNscale,        reverbSDNscaleDefault, nullptr);
    algo.setProperty (reverbFDNsize,         reverbFDNsizeDefault, nullptr);
    algo.setProperty (reverbIRfile,          "", nullptr);
    algo.setProperty (reverbIRtrim,          reverbIRtrimDefault, nullptr);
    algo.setProperty (reverbIRlength,        reverbIRlengthDefault, nullptr);
    algo.setProperty (reverbPerNodeIR,       reverbPerNodeIRDefault, nullptr);
    algo.setProperty (reverbIRGpuDevice,     reverbIRGpuDeviceDefault, nullptr);
    algo.setProperty (reverbFDNGpuDevice,    reverbFDNGpuDeviceDefault, nullptr);
    algo.setProperty (reverbSDNGpuDevice,    reverbSDNGpuDeviceDefault, nullptr);
    algo.setProperty (reverbWetLevel,        reverbWetLevelDefault, nullptr);
    return algo;
}

juce::ValueTree WFSValueTreeState::createReverbPostEQSection()
{
    juce::ValueTree postEQ (ReverbPostEQ);
    postEQ.setProperty (reverbPostEQenable, reverbPostEQenableDefault, nullptr);

    for (int i = 0; i < numReverbPostEQBands; ++i)
    {
        juce::ValueTree band (PostEQBand);
        band.setProperty (id, i + 1, nullptr);
        band.setProperty (reverbPostEQshape, reverbPostEQBandShapes[i], nullptr);
        band.setProperty (reverbPostEQfreq, reverbPostEQBandFrequencies[i], nullptr);
        band.setProperty (reverbPostEQgain, reverbPostEQgainDefault, nullptr);
        band.setProperty (reverbPostEQq, reverbPostEQqDefault, nullptr);
        band.setProperty (reverbPostEQslope, reverbPostEQslopeDefault, nullptr);
        postEQ.appendChild (band, nullptr);
    }

    return postEQ;
}

juce::ValueTree WFSValueTreeState::createReverbPreCompSection()
{
    using namespace WFSParameterIDs;
    using namespace WFSParameterDefaults;
    juce::ValueTree preComp (ReverbPreComp);
    preComp.setProperty (reverbPreCompBypass,    reverbPreCompBypassDefault, nullptr);
    preComp.setProperty (reverbPreCompThreshold, reverbPreCompThresholdDefault, nullptr);
    preComp.setProperty (reverbPreCompRatio,     reverbPreCompRatioDefault, nullptr);
    preComp.setProperty (reverbPreCompAttack,    reverbPreCompAttackDefault, nullptr);
    preComp.setProperty (reverbPreCompRelease,   reverbPreCompReleaseDefault, nullptr);
    return preComp;
}

juce::ValueTree WFSValueTreeState::createReverbPostExpSection()
{
    using namespace WFSParameterIDs;
    using namespace WFSParameterDefaults;
    juce::ValueTree postExp (ReverbPostExp);
    postExp.setProperty (reverbPostExpBypass,    reverbPostExpBypassDefault, nullptr);
    postExp.setProperty (reverbPostExpThreshold, reverbPostExpThresholdDefault, nullptr);
    postExp.setProperty (reverbPostExpRatio,     reverbPostExpRatioDefault, nullptr);
    postExp.setProperty (reverbPostExpAttack,    reverbPostExpAttackDefault, nullptr);
    postExp.setProperty (reverbPostExpRelease,   reverbPostExpReleaseDefault, nullptr);
    return postExp;
}

juce::ValueTree WFSValueTreeState::createDefaultNetworkTarget (int index)
{
    juce::ValueTree target (NetworkTarget);
    target.setProperty (id, index + 1, nullptr);
    target.setProperty (networkTSname, networkTSnameDefault + " " + juce::String (index + 1), nullptr);
    target.setProperty (networkTSdataMode, networkTSdataModeDefault, nullptr);
    target.setProperty (networkTSip, networkTSipDefault, nullptr);
    target.setProperty (networkTSport, networkTSportDefault + index, nullptr);
    target.setProperty (networkTSrxEnable, networkTSrxEnableDefault, nullptr);
    target.setProperty (networkTStxEnable, networkTStxEnableDefault, nullptr);
    target.setProperty (networkTSProtocol, networkTSProtocolDefault, nullptr);
    return target;
}

//==============================================================================
// Helper Methods
//==============================================================================

juce::ValueTree WFSValueTreeState::getTreeForParameter (const juce::Identifier& paramId, int channelIndex) const
{
    auto scope = getParameterScope (paramId);
    auto& mutableState = const_cast<juce::ValueTree&> (state);

    switch (scope)
    {
        case ParameterScope::Config:
        {
            // Check each config subsection
            auto config = mutableState.getChildWithName (Config);
            if (!config.isValid())
                return {};

            // Show section
            auto show = config.getChildWithName (Show);
            if (show.hasProperty (paramId))
                return show;

            // IO section
            auto io = config.getChildWithName (IO);
            if (io.hasProperty (paramId))
                return io;

            // Stage section
            auto stage = config.getChildWithName (Stage);
            if (stage.hasProperty (paramId))
                return stage;

            // Master section
            auto master = config.getChildWithName (Master);
            if (master.hasProperty (paramId))
                return master;

            // Network section
            auto network = config.getChildWithName (Network);
            if (network.hasProperty (paramId))
                return network;

            // ADM-OSC section
            auto admosc = config.getChildWithName (ADMOSC);
            if (admosc.hasProperty (paramId))
                return admosc;

            // Tracking section
            auto tracking = config.getChildWithName (Tracking);
            if (tracking.hasProperty (paramId))
                return tracking;

            // Binaural section. Its seven parameters have their own typed setters,
            // which the GUI uses; every generic caller — MCP, and config-scope OSC
            // ingress — landed here and fell off the end of this list, so those
            // writes were dropped in silence.
            auto binaural = config.getChildWithName (Binaural);
            if (binaural.hasProperty (paramId))
                return binaural;

            // UI section (Stream Deck / Sampler / Lightpad toggles).
            auto ui = config.getChildWithName (UI);
            if (ui.hasProperty (paramId))
                return ui;

            return {};
        }

        case ParameterScope::Cluster:
        {
            if (channelIndex < 0)
                return {};

            // These accessors are non-const and may materialise a missing node
            // (old files). Same const_cast idiom this function already uses for
            // the tree itself, a few lines up.
            auto* self = const_cast<WFSValueTreeState*> (this);

            // Preset names are the ONE property unique to the preset list. Every
            // other clusterLFO* name appears on both a live cluster's <ClusterLFO>
            // and on each stored preset, so order matters here: resolve the live
            // cluster first, or a period write would land in a preset slot.
            if (paramId == clusterLFOPresetName)
            {
                auto presets = self->getClusterLFOPresetsSection();
                if (! presets.isValid() || channelIndex >= presets.getNumChildren())
                    return {};
                return presets.getChild (channelIndex);
            }

            // getClusterState and getClusterLFOSection take a ONE-based cluster
            // index (they subtract 1 internally), while channelIndex arrives
            // zero-based — resolveChannelSlot turns cluster_id into displayId - 1.
            // Convert here rather than at either end, so both conventions stay
            // true where they are documented.
            const int oneBasedCluster = channelIndex + 1;

            auto cluster = self->getClusterState (oneBasedCluster);
            if (cluster.isValid() && cluster.hasProperty (paramId))
                return cluster;

            auto lfo = self->getClusterLFOSection (oneBasedCluster);
            if (lfo.isValid() && lfo.hasProperty (paramId))
                return lfo;

            return {};
        }

        case ParameterScope::Input:
        {
            if (channelIndex < 0)
                return {};

            auto inputs = mutableState.getChildWithName (Inputs);
            if (!inputs.isValid() || channelIndex >= inputs.getNumChildren())
                return {};

            auto input = inputs.getChild (channelIndex);

            // Search subsections
            for (int i = 0; i < input.getNumChildren(); ++i)
            {
                auto child = input.getChild (i);
                if (child.hasProperty (paramId))
                    return child;
            }
            return {};
        }

        case ParameterScope::Output:
        {
            if (channelIndex < 0)
                return {};

            auto outputs = mutableState.getChildWithName (Outputs);
            if (!outputs.isValid() || channelIndex >= outputs.getNumChildren())
                return {};

            auto output = outputs.getChild (channelIndex);

            // Search subsections
            for (int i = 0; i < output.getNumChildren(); ++i)
            {
                auto child = output.getChild (i);
                if (child.hasProperty (paramId))
                    return child;

                // Check EQ bands
                if (child.getType() == EQ)
                {
                    for (int j = 0; j < child.getNumChildren(); ++j)
                    {
                        auto band = child.getChild (j);
                        if (band.hasProperty (paramId))
                            return band;
                    }
                }
            }
            return {};
        }

        case ParameterScope::Reverb:
        {
            auto reverbs = mutableState.getChildWithName (Reverbs);
            if (! reverbs.isValid())
                return {};

            // Algo-level (global) subtrees first. ReverbAlgorithm /
            // ReverbPreComp / ReverbPostExp / ReverbPostEQ are siblings
            // of per-channel Reverb children inside Reverbs and carry
            // unique properties — a hasProperty hit here is unambiguous
            // regardless of the channelIndex argument. PostEQ band props
            // (reverbPostEQshape/freq/gain/q/slope) live on PostEQBand
            // children and are routed through the EQ-band path in the
            // MCP dispatcher; they need a band index this signature
            // doesn't carry.
            auto algo = reverbs.getChildWithName (ReverbAlgorithm);
            if (algo.hasProperty (paramId))
                return algo;
            auto preComp = reverbs.getChildWithName (ReverbPreComp);
            if (preComp.hasProperty (paramId))
                return preComp;
            auto postExp = reverbs.getChildWithName (ReverbPostExp);
            if (postExp.hasProperty (paramId))
                return postExp;
            auto postEQ = reverbs.getChildWithName (ReverbPostEQ);
            if (postEQ.hasProperty (paramId))
                return postEQ;

            // Per-channel: walk Reverb-typed children only, skipping
            // algo subtree siblings, and pick the nth one (zero-based).
            // Reverbs.getChild(channelIndex) is unsafe here because the
            // algo subtrees share the same parent and may be ordered
            // before or after the per-channel children depending on
            // when they were appended.
            if (channelIndex < 0)
                return {};
            int nth = 0;
            for (int i = 0; i < reverbs.getNumChildren(); ++i)
            {
                auto child = reverbs.getChild (i);
                if (child.getType() != Reverb)
                    continue;
                if (nth == channelIndex)
                {
                    for (int j = 0; j < child.getNumChildren(); ++j)
                    {
                        auto sub = child.getChild (j);
                        if (sub.hasProperty (paramId))
                            return sub;
                        if (sub.getType() == EQ)
                        {
                            for (int k = 0; k < sub.getNumChildren(); ++k)
                            {
                                auto band = sub.getChild (k);
                                if (band.hasProperty (paramId))
                                    return band;
                            }
                        }
                    }
                    return {};
                }
                ++nth;
            }
            return {};
        }

        case ParameterScope::AudioPatch:
        {
            auto audioPatch = mutableState.getChildWithName (AudioPatch);
            if (audioPatch.hasProperty (paramId))
                return audioPatch;
            return {};
        }

        default:
            return {};
    }
}

void WFSValueTreeState::enforceClusterTrackingConstraint (int changedInputIndex)
{
    // Get tracking state for the changed input
    auto changedInput = getInputState (changedInputIndex);
    if (!changedInput.isValid())
        return;

    auto posSection = changedInput.getChildWithName (Position);
    if (!posSection.isValid())
        return;

    int clusterIdx = static_cast<int> (posSection.getProperty (inputCluster));
    bool trackingActive = static_cast<int> (posSection.getProperty (inputTrackingActive)) != 0;

    // Only check if this input is in a cluster (not "Single" which is 0)
    // and has tracking enabled
    if (clusterIdx < 1 || !trackingActive)
        return;

    // Check global tracking state - constraints only matter when global tracking is active
    auto trackingSection = getTrackingState();
    bool globalEnabled = trackingSection.isValid() &&
                         static_cast<int> (trackingSection.getProperty (trackingEnabled)) != 0;
    int protocol = trackingSection.isValid() ?
                   static_cast<int> (trackingSection.getProperty (trackingProtocol)) : 0;

    if (!globalEnabled || protocol == 0)
        return;  // Global tracking not active, constraint doesn't apply

    // Find all other inputs in the same cluster with tracking enabled
    auto inputs = getInputsState();
    int numInputs = inputs.getNumChildren();

    for (int i = 0; i < numInputs; ++i)
    {
        if (i == changedInputIndex)
            continue;  // Skip the changed input

        auto input = inputs.getChild (i);
        auto pos = input.getChildWithName (Position);
        if (!pos.isValid())
            continue;

        int otherCluster = static_cast<int> (pos.getProperty (inputCluster));
        bool otherTracking = static_cast<int> (pos.getProperty (inputTrackingActive)) != 0;

        if (otherCluster == clusterIdx && otherTracking)
        {
            // Found another input in same cluster with tracking enabled
            // Disable tracking on the OTHER input (keep the one that was just changed)
            pos.setProperty (inputTrackingActive, 0, nullptr);
        }
    }
}

// Find the first-ordered member of a cluster, honouring clusterInputOrder
// when present, otherwise falling back to the lowest-index member. Returns
// -1 if the cluster has no members. Used by the Shared-Position invariant.
static int findFirstOrderedClusterMember (juce::ValueTree inputs,
                                          const juce::Identifier& inputClusterId,
                                          const juce::Identifier& positionId,
                                          juce::String inputOrder,
                                          int clusterIndex)
{
    const int numInputs = inputs.getNumChildren();

    auto isMember = [&] (int idx) -> bool
    {
        if (idx < 0 || idx >= numInputs) return false;
        auto input = inputs.getChild (idx);
        auto pos = input.getChildWithName (positionId);
        if (! pos.isValid()) return false;
        return static_cast<int> (pos.getProperty (inputClusterId)) == clusterIndex;
    };

    if (inputOrder.isNotEmpty())
    {
        juce::StringArray tokens;
        tokens.addTokens (inputOrder, ",", "");
        for (const auto& tok : tokens)
        {
            int candidate = tok.trim().getIntValue();
            if (isMember (candidate))
                return candidate;
        }
    }

    for (int i = 0; i < numInputs; ++i)
        if (isMember (i))
            return i;

    return -1;
}

void WFSValueTreeState::propagateSharedClusterPosition (int sourceInputIndex)
{
    auto sourceInput = getInputState (sourceInputIndex);
    if (! sourceInput.isValid())
        return;

    auto sourcePos = sourceInput.getChildWithName (Position);
    if (! sourcePos.isValid())
        return;

    const int clusterIdx = static_cast<int> (sourcePos.getProperty (inputCluster));
    if (clusterIdx < 1)
        return;

    const int mode = static_cast<int> (getClusterParameter (clusterIdx, clusterReferenceMode));
    if (mode != 2)
        return;

    const float x = static_cast<float> (sourcePos.getProperty (inputPositionX));
    const float y = static_cast<float> (sourcePos.getProperty (inputPositionY));
    const float z = static_cast<float> (sourcePos.getProperty (inputPositionZ));

    auto inputs = getInputsState();
    const int numInputs = inputs.getNumChildren();

    for (int i = 0; i < numInputs; ++i)
    {
        if (i == sourceInputIndex) continue;

        auto input = inputs.getChild (i);
        auto pos = input.getChildWithName (Position);
        if (! pos.isValid()) continue;

        if (static_cast<int> (pos.getProperty (inputCluster)) != clusterIdx)
            continue;

        // Skip the write when it's already correct — avoids reentry loops if
        // any listener calls back into propagation.
        if (static_cast<float> (pos.getProperty (inputPositionX)) != x)
            pos.setProperty (inputPositionX, x, nullptr);
        if (static_cast<float> (pos.getProperty (inputPositionY)) != y)
            pos.setProperty (inputPositionY, y, nullptr);
        if (static_cast<float> (pos.getProperty (inputPositionZ)) != z)
            pos.setProperty (inputPositionZ, z, nullptr);
    }
}

void WFSValueTreeState::enforceSharedClusterInvariant (int clusterIndex)
{
    if (clusterIndex < 1)
        return;

    const int mode = static_cast<int> (getClusterParameter (clusterIndex, clusterReferenceMode));
    if (mode != 2)
        return;

    auto inputs = getInputsState();
    const juce::String order = getClusterParameter (clusterIndex, clusterInputOrder).toString();
    const int refIdx = findFirstOrderedClusterMember (inputs, inputCluster, Position, order, clusterIndex);
    if (refIdx < 0)
        return;

    auto refInput = inputs.getChild (refIdx);
    auto refPos = refInput.getChildWithName (Position);
    if (! refPos.isValid())
        return;

    const float x = static_cast<float> (refPos.getProperty (inputPositionX));
    const float y = static_cast<float> (refPos.getProperty (inputPositionY));
    const float z = static_cast<float> (refPos.getProperty (inputPositionZ));

    const int numInputs = inputs.getNumChildren();
    for (int i = 0; i < numInputs; ++i)
    {
        if (i == refIdx) continue;

        auto input = inputs.getChild (i);
        auto pos = input.getChildWithName (Position);
        if (! pos.isValid()) continue;

        if (static_cast<int> (pos.getProperty (inputCluster)) != clusterIndex)
            continue;

        if (static_cast<float> (pos.getProperty (inputPositionX)) != x)
            pos.setProperty (inputPositionX, x, nullptr);
        if (static_cast<float> (pos.getProperty (inputPositionY)) != y)
            pos.setProperty (inputPositionY, y, nullptr);
        if (static_cast<float> (pos.getProperty (inputPositionZ)) != z)
            pos.setProperty (inputPositionZ, z, nullptr);
    }
}

void WFSValueTreeState::enforceAllSharedClusterInvariants()
{
    for (int c = 1; c <= 10; ++c)
        enforceSharedClusterInvariant (c);
}

WFSValueTreeState::ParameterScope WFSValueTreeState::getParameterScope (const juce::Identifier& paramId) const
{
    // Check for config-level parameters that might have misleading prefixes
    // inputChannels, outputChannels, reverbChannels are stored in Config/IO,
    // not in their respective channel sections
    if (paramId == inputChannels || paramId == outputChannels || paramId == reverbChannels)
        return ParameterScope::Config;

    // reverbsMapVisible is a Master-section display toggle, not a per-reverb
    // parameter. It only LOOKS like one: the prefix test below would send it to
    // the Reverb branch, which searches <Reverb> channel nodes and never finds
    // it, so the write vanished. Named here for the same reason the channel
    // counts are — the prefix lies about where the property lives.
    if (paramId == reverbsMapVisible)
        return ParameterScope::Config;

    // Check if it's an input parameter
    juce::String paramName = paramId.toString();
    if (paramName.startsWith ("input"))
        return ParameterScope::Input;

    // Sampler parameters hang off an <Input>, but are named for the feature
    // rather than the scope, so they never matched the "input" prefix and
    // defaulted to Config — where nothing could find them. The ones on <Channel>
    // (a direct child) resolve from here; cell and set properties live on
    // grandchildren and still need the sub-index path.
    if (paramName.startsWith ("sampler"))
        return ParameterScope::Input;

    // Cluster parameters carry their index as cluster_id, which arrives as the
    // channelIndex argument — the plumbing was always there, the scope was not.
    if (paramName.startsWith ("cluster"))
        return ParameterScope::Cluster;

    // Check if it's a reverb parameter
    if (paramName.startsWith ("reverb"))
        return ParameterScope::Reverb;

    // Check if it's an output parameter
    if (paramName.startsWith ("output") || paramName.startsWith ("eq"))
        return ParameterScope::Output;

    // Check if it's an audio patch parameter
    if (paramId == driverMode || paramId == audioInterface ||
        paramId == inputMatrixMode || paramId == outputMatrixMode ||
        paramId == testTone || paramId == sineFrequency || paramId == testToneLevel ||
        paramId == patchData)
        return ParameterScope::AudioPatch;

    // Default to config
    return ParameterScope::Config;
}
