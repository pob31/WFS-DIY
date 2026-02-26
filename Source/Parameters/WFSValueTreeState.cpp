#include "WFSValueTreeState.h"

using namespace WFSParameterIDs;
using namespace WFSParameterDefaults;

//==============================================================================
// Construction / Destruction
//==============================================================================

WFSValueTreeState::WFSValueTreeState()
{
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

float WFSValueTreeState::getFloatParameter (const juce::Identifier& paramId, int channelIndex) const
{
    auto tree = getTreeForParameter (paramId, channelIndex);
    if (tree.isValid() && tree.hasProperty (paramId))
        return static_cast<float> (tree.getProperty (paramId));
    return 0.0f;
}

int WFSValueTreeState::getIntParameter (const juce::Identifier& paramId, int channelIndex) const
{
    auto tree = getTreeForParameter (paramId, channelIndex);
    if (tree.isValid() && tree.hasProperty (paramId))
        return static_cast<int> (tree.getProperty (paramId));
    return 0;
}

juce::String WFSValueTreeState::getStringParameter (const juce::Identifier& paramId, int channelIndex) const
{
    auto tree = getTreeForParameter (paramId, channelIndex);
    if (tree.isValid() && tree.hasProperty (paramId))
        return tree.getProperty (paramId).toString();
    return {};
}

juce::var WFSValueTreeState::getParameter (const juce::Identifier& paramId, int channelIndex) const
{
    auto tree = getTreeForParameter (paramId, channelIndex);
    if (tree.isValid())
        return tree.getProperty (paramId);
    return {};
}

void WFSValueTreeState::setParameter (const juce::Identifier& paramId, const juce::var& value, int channelIndex)
{
    auto tree = getTreeForParameter (paramId, channelIndex);
    if (tree.isValid())
        tree.setProperty (paramId, value, getActiveUndoManager());
}

void WFSValueTreeState::setParameterWithoutUndo (const juce::Identifier& paramId, const juce::var& value, int channelIndex)
{
    auto tree = getTreeForParameter (paramId, channelIndex);
    if (tree.isValid())
        tree.setProperty (paramId, value, nullptr);
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
            child.setProperty (paramId, value, getActiveUndoManager());
            return;
        }
    }

    // Property not found - add it to the appropriate section if we know where it belongs
    // This handles old config files that may be missing newer properties
    if (paramId == inputCoordinateMode)
    {
        auto position = getInputPositionSection (channelIndex);
        if (position.isValid())
            position.setProperty (paramId, value, getActiveUndoManager());
    }
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
            child.setProperty (paramId, value, getActiveUndoManager());
            return;
        }
    }

    // Property not found - add it to the appropriate section if we know where it belongs
    // This handles old config files that may be missing newer properties
    if (paramId == outputCoordinateMode)
    {
        auto position = getOutputPositionSection (channelIndex);
        if (position.isValid())
            position.setProperty (paramId, value, getActiveUndoManager());
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
            child.setProperty (paramId, value, getActiveUndoManager());
            return;
        }
    }
}

void WFSValueTreeState::setOutputEQBandParameterDirect (int channelIndex, int bandIndex, const juce::Identifier& paramId, const juce::var& value)
{
    auto band = getOutputEQBand (channelIndex, bandIndex);
    if (band.isValid())
        band.setProperty (paramId, value, getActiveUndoManager());
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
    if (paramId == outputHparallax || paramId == outputVparallax)
        return juce::jlimit (outputParallaxMin, outputParallaxMax, value);

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

        if (applyMode == 1)  // ABSOLUTE
        {
            setOutputParameterDirect (i, paramId, value);
        }
        else  // RELATIVE (applyMode == 2)
        {
            float memberCurrent = static_cast<float> (getOutputParameter (i, paramId));
            float memberNew = clampOutputParamToRange (paramId, memberCurrent + delta);

            // For int parameters, round the result
            if (paramId == outputOrientation || paramId == outputAngleOn ||
                paramId == outputAngleOff || paramId == outputPitch ||
                paramId == outputDistanceAttenPercent ||
                paramId == outputMiniLatencyEnable || paramId == outputLSattenEnable ||
                paramId == outputFRenable || paramId == outputEQenabled)
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
                                                                       const juce::var& value)
{
    // Check if this is an array-linked EQ parameter
    if (!isArrayLinkedEQParameter (paramId))
    {
        auto band = getOutputEQBand (channelIndex, bandIndex);
        if (band.isValid())
            band.setProperty (paramId, value, getActiveUndoManager());
        return;
    }

    // Get array assignment for this output
    int arrayId = static_cast<int> (getOutputParameter (channelIndex, outputArray));
    if (arrayId == 0)  // Single, not in array
    {
        auto band = getOutputEQBand (channelIndex, bandIndex);
        if (band.isValid())
            band.setProperty (paramId, value, getActiveUndoManager());
        return;
    }

    // Get apply mode for this output
    int applyMode = static_cast<int> (getOutputParameter (channelIndex, outputApplyToArray));
    if (applyMode == 0)  // OFF
    {
        auto band = getOutputEQBand (channelIndex, bandIndex);
        if (band.isValid())
            band.setProperty (paramId, value, getActiveUndoManager());
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
    band.setProperty (paramId, value, getActiveUndoManager());

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

        if (applyMode == 1)  // ABSOLUTE
        {
            setOutputEQBandParameterDirect (i, bandIndex, paramId, value);
        }
        else  // RELATIVE
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
            child.setProperty (paramId, value, getActiveUndoManager());
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
                    band.setProperty (paramId, value, getActiveUndoManager());
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
            position.setProperty (paramId, value, getActiveUndoManager());
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
        cluster.setProperty (paramId, value, getActiveUndoManager());
}

//==============================================================================
// Binaural Enable/Solo Access
//==============================================================================

bool WFSValueTreeState::getBinauralEnabled() const
{
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

bool WFSValueTreeState::isInputSoloed (int inputIndex) const
{
    auto binaural = getBinauralState();
    if (!binaural.isValid())
        return false;

    juce::String soloStates = binaural.getProperty (inputSoloStates, "").toString();
    if (soloStates.isEmpty())
        return false;

    juce::StringArray states;
    states.addTokens (soloStates, ",", "");

    if (inputIndex >= 0 && inputIndex < states.size())
        return states[inputIndex] == "1";

    return false;
}

void WFSValueTreeState::setInputSoloed (int inputIndex, bool soloed)
{
    auto binaural = getBinauralState();
    if (!binaural.isValid() || inputIndex < 0)
        return;

    int numInputs = getNumInputChannels();
    if (inputIndex >= numInputs)
        return;

    // Get current solo states
    juce::String soloStates = binaural.getProperty (inputSoloStates, "").toString();
    juce::StringArray states;
    states.addTokens (soloStates, ",", "");

    // Ensure array is large enough
    while (states.size() < numInputs)
        states.add ("0");

    // In Single mode, clear all other solos first
    if (soloed && getBinauralSoloMode() == 0)
    {
        for (int i = 0; i < states.size(); ++i)
            states.set (i, "0");
    }

    // Set the requested input's solo state
    states.set (inputIndex, soloed ? "1" : "0");

    // Save back
    binaural.setProperty (inputSoloStates, states.joinIntoString (","), getActiveUndoManager());
}

void WFSValueTreeState::clearAllSoloStates()
{
    auto binaural = getBinauralState();
    if (binaural.isValid())
        binaural.setProperty (inputSoloStates, "", getActiveUndoManager());
}

int WFSValueTreeState::getNumSoloedInputs() const
{
    auto binaural = getBinauralState();
    if (!binaural.isValid())
        return 0;

    juce::String soloStates = binaural.getProperty (inputSoloStates, "").toString();
    if (soloStates.isEmpty())
        return 0;

    juce::StringArray states;
    states.addTokens (soloStates, ",", "");

    int soloCount = 0;
    for (const auto& s : states)
        if (s == "1")
            ++soloCount;

    return soloCount;
}

int WFSValueTreeState::getBinauralOutputChannel() const
{
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

int WFSValueTreeState::getNumOutputChannels() const
{
    return const_cast<WFSValueTreeState*>(this)->getOutputsState().getNumChildren();
}

int WFSValueTreeState::getNumReverbChannels() const
{
    return getIntParameter (reverbChannels);
}

void WFSValueTreeState::setNumInputChannels (int numChannels)
{
    numChannels = juce::jlimit (1, maxInputChannels, numChannels);
    auto inputs = getInputsState();
    int currentCount = inputs.getNumChildren();

    beginUndoTransaction ("Set Input Channel Count");

    if (numChannels > currentCount)
    {
        // Add new channels
        for (int i = currentCount; i < numChannels; ++i)
            inputs.appendChild (createDefaultInputChannel (i), getActiveUndoManager());
    }
    else if (numChannels < currentCount)
    {
        // Remove excess channels
        while (inputs.getNumChildren() > numChannels)
            inputs.removeChild (inputs.getNumChildren() - 1, getActiveUndoManager());
    }

    // Update the count in config
    setParameter (inputChannels, numChannels);
    inputs.setProperty (count, numChannels, getActiveUndoManager());
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

    // Update the count in config
    setParameter (outputChannels, numChannels);
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

    beginUndoTransaction ("Set Reverb Channel Count");

    if (numChannels > currentCount)
    {
        // Add new channels
        for (int i = currentCount; i < numChannels; ++i)
            reverbs.appendChild (createDefaultReverbChannel (i), getActiveUndoManager());
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

    // Update the count in config
    setParameter (reverbChannels, numChannels);
    reverbs.setProperty (count, numChannels, getActiveUndoManager());
}

//==============================================================================
// Undo / Redo  (per-domain)
//==============================================================================

void WFSValueTreeState::setActiveDomain (UndoDomain domain)
{
    activeDomain = domain;
}

UndoDomain WFSValueTreeState::getActiveDomain() const
{
    return activeDomain;
}

juce::UndoManager* WFSValueTreeState::getUndoManagerForDomain (UndoDomain domain)
{
    auto idx = static_cast<int> (domain);
    jassert (idx >= 0 && idx < static_cast<int> (UndoDomain::COUNT));
    return &undoManagers[idx];
}

juce::UndoManager* WFSValueTreeState::getActiveUndoManager()
{
    return getUndoManagerForDomain (activeDomain);
}

bool WFSValueTreeState::undo()
{
    return getActiveUndoManager()->undo();
}

bool WFSValueTreeState::redo()
{
    return getActiveUndoManager()->redo();
}

bool WFSValueTreeState::canUndo() const
{
    return undoManagers[static_cast<int> (activeDomain)].canUndo();
}

bool WFSValueTreeState::canRedo() const
{
    return undoManagers[static_cast<int> (activeDomain)].canRedo();
}

void WFSValueTreeState::beginUndoTransaction (const juce::String& transactionName)
{
    getActiveUndoManager()->beginNewTransaction (transactionName);
}

void WFSValueTreeState::clearUndoHistory()
{
    getActiveUndoManager()->clearUndoHistory();
}

void WFSValueTreeState::clearAllUndoHistories()
{
    for (int i = 0; i < static_cast<int> (UndoDomain::COUNT); ++i)
        undoManagers[i].clearUndoHistory();
}

//==============================================================================
// Listener Management
//==============================================================================

void WFSValueTreeState::addParameterListener (const juce::Identifier& paramId,
                                               ParameterCallback callback,
                                               int channelIndex)
{
    juce::ScopedLock lock (listenerLock);
    parameterListeners.push_back ({ paramId, channelIndex, std::move (callback) });
}

void WFSValueTreeState::removeParameterListeners (const juce::Identifier& paramId, int channelIndex)
{
    juce::ScopedLock lock (listenerLock);
    parameterListeners.erase (
        std::remove_if (parameterListeners.begin(), parameterListeners.end(),
            [&] (const ListenerEntry& entry)
            {
                return entry.parameterId == paramId && entry.channelIndex == channelIndex;
            }),
        parameterListeners.end());
}

void WFSValueTreeState::addListener (juce::ValueTree::Listener* listener)
{
    state.addListener (listener);
}

void WFSValueTreeState::removeListener (juce::ValueTree::Listener* listener)
{
    state.removeListener (listener);
}

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
        auto newReverb = createDefaultReverbChannel (channelIndex);
        reverb.copyPropertiesAndChildrenFrom (newReverb, getActiveUndoManager());
    }
}

void WFSValueTreeState::replaceState (const juce::ValueTree& newState)
{
    if (validateState (newState))
    {
        state.copyPropertiesAndChildrenFrom (newState, nullptr);
        clearAllUndoHistories();
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
// ValueTree::Listener Implementation
//==============================================================================

void WFSValueTreeState::valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged,
                                                   const juce::Identifier& property)
{
    // Determine channel index if this is an input/output/reverb parameter
    int channelIndex = -1;
    auto parent = treeWhosePropertyHasChanged.getParent();

    if (parent.isValid())
    {
        if (parent.getType() == Input || parent.getType() == Output || parent.getType() == Reverb)
            channelIndex = static_cast<int> (parent.getProperty (id)) - 1;
        else if (parent.getParent().isValid() &&
                 (parent.getParent().getType() == Input || parent.getParent().getType() == Output ||
                  parent.getParent().getType() == Reverb))
            channelIndex = static_cast<int> (parent.getParent().getProperty (id)) - 1;
    }

    auto value = treeWhosePropertyHasChanged.getProperty (property);

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
    }

    notifyParameterListeners (property, value, channelIndex);
}

void WFSValueTreeState::valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&)
{
    // Could notify listeners of structural changes if needed
}

void WFSValueTreeState::valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree&, int)
{
    // Could notify listeners of structural changes if needed
}

void WFSValueTreeState::valueTreeChildOrderChanged (juce::ValueTree&, int, int)
{
    // Not typically needed for parameters
}

void WFSValueTreeState::valueTreeParentChanged (juce::ValueTree&)
{
    // Not typically needed for parameters
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

void WFSValueTreeState::createNetworkSection (juce::ValueTree& config)
{
    juce::ValueTree network (Network);
    network.setProperty (networkInterface, "", nullptr);
    network.setProperty (networkCurrentIP, networkCurrentIPDefault, nullptr);
    network.setProperty (networkRxUDPport, networkRxUDPportDefault, nullptr);
    network.setProperty (networkRxTCPport, networkRxTCPportDefault, nullptr);
    network.setProperty (findDevicePassword, findDevicePasswordDefault, nullptr);
    config.appendChild (network, nullptr);
}

void WFSValueTreeState::createADMOSCSection (juce::ValueTree& config)
{
    juce::ValueTree admosc (ADMOSC);
    admosc.setProperty (admOscOffsetX, admOscOffsetDefault, nullptr);
    admosc.setProperty (admOscOffsetY, admOscOffsetDefault, nullptr);
    admosc.setProperty (admOscOffsetZ, admOscOffsetDefault, nullptr);
    admosc.setProperty (admOscScaleX, admOscScaleDefault, nullptr);
    admosc.setProperty (admOscScaleY, admOscScaleDefault, nullptr);
    admosc.setProperty (admOscScaleZ, admOscScaleDefault, nullptr);
    admosc.setProperty (admOscFlipX, admOscFlipDefault, nullptr);
    admosc.setProperty (admOscFlipY, admOscFlipDefault, nullptr);
    admosc.setProperty (admOscFlipZ, admOscFlipDefault, nullptr);
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
    binaural.setProperty (inputSoloStates, "", nullptr);  // Empty = no solos
    config.appendChild (binaural, nullptr);
}

void WFSValueTreeState::createUISection (juce::ValueTree& config)
{
    juce::ValueTree ui (UI);
    ui.setProperty (streamDeckEnabled, streamDeckEnabledDefault, nullptr);
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
        reverbs.appendChild (createDefaultReverbChannel (i), nullptr);

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

juce::ValueTree WFSValueTreeState::createDefaultInputChannel (int index)
{
    int totalInputs = inputChannelsDefault;
    auto io = getIOState();
    if (io.isValid())
        totalInputs = static_cast<int> (io.getProperty (inputChannels));

    juce::ValueTree input (Input);
    input.setProperty (id, index + 1, nullptr);

    input.appendChild (createInputChannelSection (index), nullptr);
    input.appendChild (createInputPositionSection (index, totalInputs), nullptr);
    input.appendChild (createInputAttenuationSection(), nullptr);
    input.appendChild (createInputDirectivitySection(), nullptr);
    input.appendChild (createInputLiveSourceSection(), nullptr);
    input.appendChild (createInputHackousticsSection(), nullptr);
    input.appendChild (createInputLFOSection(), nullptr);
    input.appendChild (createInputAutoMotionSection(), nullptr);
    input.appendChild (createInputMutesSection (getNumOutputChannels()), nullptr);

    return input;
}

juce::ValueTree WFSValueTreeState::createInputChannelSection (int index)
{
    juce::ValueTree channel (Channel);
    channel.setProperty (inputName, getDefaultInputName (index), nullptr);
    channel.setProperty (inputAttenuation, inputAttenuationDefault, nullptr);
    channel.setProperty (inputDelayLatency, inputDelayLatencyDefault, nullptr);
    channel.setProperty (inputMinimalLatency, inputMinimalLatencyDefault, nullptr);
    channel.setProperty (inputMapLocked, 0, nullptr);    // Default: unlocked
    channel.setProperty (inputMapVisible, 1, nullptr);   // Default: visible
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
    position.setProperty (inputJitter, inputJitterDefault, nullptr);

    return position;
}

juce::ValueTree WFSValueTreeState::createInputAttenuationSection()
{
    juce::ValueTree attenuation (Attenuation);
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
    liveSource.setProperty (inputLSpeakThreshold, inputLSpeakThresholdDefault, nullptr);
    liveSource.setProperty (inputLSpeakRatio, inputLSpeakRatioDefault, nullptr);
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
    options.setProperty (outputHparallax, outputParallaxDefault, nullptr);
    options.setProperty (outputVparallax, outputParallaxDefault, nullptr);
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

juce::ValueTree WFSValueTreeState::createDefaultReverbChannel (int index)
{
    juce::ValueTree reverb (Reverb);
    reverb.setProperty (id, index + 1, nullptr);

    reverb.appendChild (createReverbChannelSection (index), nullptr);
    reverb.appendChild (createReverbPositionSection(), nullptr);
    reverb.appendChild (createReverbFeedSection(), nullptr);
    reverb.appendChild (createReverbEQSection(), nullptr);
    reverb.appendChild (createReverbReturnSection (getNumOutputChannels()), nullptr);

    return reverb;
}

juce::ValueTree WFSValueTreeState::createReverbChannelSection (int index)
{
    juce::ValueTree channel (Channel);
    channel.setProperty (reverbName, getDefaultReverbName (index), nullptr);
    channel.setProperty (reverbAttenuation, reverbAttenuationDefault, nullptr);
    channel.setProperty (reverbDelayLatency, reverbDelayLatencyDefault, nullptr);
    return channel;
}

juce::ValueTree WFSValueTreeState::createReverbPositionSection()
{
    juce::ValueTree position (Position);
    position.setProperty (reverbPositionX, reverbPositionDefault, nullptr);
    position.setProperty (reverbPositionY, reverbPositionDefault, nullptr);
    position.setProperty (reverbPositionZ, reverbPositionDefault, nullptr);
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
    feed.setProperty (reverbLSenable, reverbLSenableDefault, nullptr);
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
            if (channelIndex < 0)
                return {};

            auto reverbs = mutableState.getChildWithName (Reverbs);
            if (!reverbs.isValid() || channelIndex >= reverbs.getNumChildren())
                return {};

            auto reverb = reverbs.getChild (channelIndex);

            // Search subsections
            for (int i = 0; i < reverb.getNumChildren(); ++i)
            {
                auto child = reverb.getChild (i);
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

void WFSValueTreeState::notifyParameterListeners (const juce::Identifier& paramId,
                                                   const juce::var& value,
                                                   int channelIndex)
{
    juce::ScopedLock lock (listenerLock);

    for (const auto& entry : parameterListeners)
    {
        if (entry.parameterId == paramId &&
            (entry.channelIndex == -1 || entry.channelIndex == channelIndex))
        {
            entry.callback (value);
        }
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
            DBG ("WFSValueTreeState: Disabled tracking on Input " << (i + 1)
                 << " due to cluster constraint (Input " << (changedInputIndex + 1)
                 << " now tracked in cluster " << clusterIdx << ")");
        }
    }
}

WFSValueTreeState::ParameterScope WFSValueTreeState::getParameterScope (const juce::Identifier& paramId) const
{
    // Check for config-level parameters that might have misleading prefixes
    // inputChannels, outputChannels, reverbChannels are stored in Config/IO,
    // not in their respective channel sections
    if (paramId == inputChannels || paramId == outputChannels || paramId == reverbChannels)
        return ParameterScope::Config;

    // Check if it's an input parameter
    juce::String paramName = paramId.toString();
    if (paramName.startsWith ("input"))
        return ParameterScope::Input;

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
