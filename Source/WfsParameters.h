#pragma once

#include <JuceHeader.h>
#include "Parameters/WFSParameterIDs.h"
#include "Parameters/WFSParameterDefaults.h"
#include "Parameters/WFSValueTreeState.h"
#include "Parameters/WFSFileManager.h"
#include "Parameters/ParameterDirtyTracker.h"

/**
 * WFS Parameters - Backward Compatible Wrapper
 *
 * This class provides backward compatibility for existing GUI code
 * while using the new WFSValueTreeState system internally.
 *
 * For new code, prefer using WFSValueTreeState directly.
 */
class WfsParameters
{
public:
    WfsParameters()
        : fileManager (valueTreeState),
          dirtyTracker (valueTreeState.getState())
    {
    }

    //==============================================================================
    // Config Parameter Access (backward compatible API)
    //==============================================================================

    /** Get config parameter value */
    juce::var getConfigParam (const juce::String& paramName) const
    {
        // Map old parameter names to new identifiers
        auto id = mapParamNameToIdentifier (paramName);

        // Try to find in config sections
        auto config = valueTreeState.getConfigState();
        if (!config.isValid())
            return {};

        // Search through all config subsections
        for (int i = 0; i < config.getNumChildren(); ++i)
        {
            auto child = config.getChild (i);
            if (child.hasProperty (id))
                return child.getProperty (id);
        }

        return {};
    }

    /** Set config parameter value */
    void setConfigParam (const juce::String& paramName, const juce::var& value)
    {
        auto id = mapParamNameToIdentifier (paramName);

        auto config = valueTreeState.getConfigState();
        if (!config.isValid())
            return;

        // Search through all config subsections and set
        for (int i = 0; i < config.getNumChildren(); ++i)
        {
            auto child = config.getChild (i);
            if (child.hasProperty (id))
            {
                child.setProperty (id, value, valueTreeState.getUndoManager());
                return;
            }
        }

        // If not found, try setting directly on subsections based on param prefix
        setConfigParamBySection (paramName, id, value);
    }

    //==============================================================================
    // Input Parameter Access (backward compatible API)
    //==============================================================================

    /** Get input channel parameter */
    juce::var getInputParam (int channelIndex, const juce::String& paramName) const
    {
        auto id = mapParamNameToIdentifier (paramName);
        return valueTreeState.getInputParameter (channelIndex, id);
    }

    /** Set input channel parameter */
    void setInputParam (int channelIndex, const juce::String& paramName, const juce::var& value)
    {
        auto id = mapParamNameToIdentifier (paramName);
        valueTreeState.setInputParameter (channelIndex, id, value);
    }

    //==============================================================================
    // Output Parameter Access (backward compatible API)
    //==============================================================================

    /** Get output channel parameter */
    juce::var getOutputParam (int channelIndex, const juce::String& paramName) const
    {
        auto id = mapParamNameToIdentifier (paramName);
        return valueTreeState.getOutputParameter (channelIndex, id);
    }

    /** Set output channel parameter with array propagation
     *  If the output is part of an array and applyToArray is enabled,
     *  propagates the change to other array members.
     */
    void setOutputParam (int channelIndex, const juce::String& paramName, const juce::var& value)
    {
        auto id = mapParamNameToIdentifier (paramName);
        valueTreeState.setOutputParameterWithArrayPropagation (channelIndex, id, value);
    }

    /** Set output channel parameter without array propagation
     *  Use this when you explicitly want to set only this output's parameter.
     */
    void setOutputParamDirect (int channelIndex, const juce::String& paramName, const juce::var& value)
    {
        auto id = mapParamNameToIdentifier (paramName);
        valueTreeState.setOutputParameter (channelIndex, id, value);
    }

    /** Set output EQ band parameter with array propagation */
    void setOutputEQBandParam (int channelIndex, int bandIndex, const juce::String& paramName, const juce::var& value)
    {
        auto id = mapParamNameToIdentifier (paramName);
        valueTreeState.setOutputEQBandParameterWithArrayPropagation (channelIndex, bandIndex, id, value);
    }

    //==============================================================================
    // Reverb Parameter Access (backward compatible API)
    //==============================================================================

    /** Get reverb channel parameter */
    juce::var getReverbParam (int channelIndex, const juce::String& paramName) const
    {
        auto id = mapParamNameToIdentifier (paramName);
        return valueTreeState.getReverbParameter (channelIndex, id);
    }

    /** Set reverb channel parameter */
    void setReverbParam (int channelIndex, const juce::String& paramName, const juce::var& value)
    {
        auto id = mapParamNameToIdentifier (paramName);
        valueTreeState.setReverbParameter (channelIndex, id, value);
    }

    //==============================================================================
    // Channel Management (backward compatible API)
    //==============================================================================

    void setNumInputChannels (int numChannels)
    {
        valueTreeState.setNumInputChannels (numChannels);
    }

    void setNumOutputChannels (int numChannels)
    {
        valueTreeState.setNumOutputChannels (numChannels);
    }

    void setNumReverbChannels (int numChannels)
    {
        valueTreeState.setNumReverbChannels (numChannels);
    }

    int getNumInputChannels() const { return valueTreeState.getNumInputChannels(); }
    int getNumOutputChannels() const { return valueTreeState.getNumOutputChannels(); }
    int getNumReverbChannels() const { return valueTreeState.getNumReverbChannels(); }

    //==============================================================================
    // ValueTree Access (backward compatible API)
    //==============================================================================

    juce::ValueTree getRootTree() { return valueTreeState.getState(); }
    juce::ValueTree getConfigTree() { return valueTreeState.getConfigState(); }
    juce::ValueTree getInputTree() { return valueTreeState.getInputsState(); }
    juce::ValueTree getOutputTree() { return valueTreeState.getOutputsState(); }
    juce::ValueTree getReverbTree() { return valueTreeState.getReverbsState(); }

    //==============================================================================
    // Save/Load (backward compatible API)
    //==============================================================================

    bool saveCompleteConfig (const juce::File& file)
    {
        return fileManager.exportCompleteConfig (file);
    }

    bool loadCompleteConfig (const juce::File& file)
    {
        return fileManager.importCompleteConfig (file);
    }

    bool saveSystemConfig (const juce::File& file)
    {
        return fileManager.exportSystemConfig (file);
    }

    bool loadSystemConfig (const juce::File& file)
    {
        return fileManager.importSystemConfig (file);
    }

    bool saveSnapshot (const juce::File& file, bool includeInput, bool includeOutput, bool includeConfig)
    {
        WFSFileManager::SnapshotScope scope;
        // If includeConfig is false, we just save input/output
        if (!includeInput && !includeOutput && includeConfig)
        {
            return fileManager.exportSystemConfig (file);
        }
        if (includeInput)
        {
            return fileManager.exportInputConfig (file);
        }
        if (includeOutput)
        {
            return fileManager.exportOutputConfig (file);
        }
        return fileManager.exportCompleteConfig (file);
    }

    bool loadSnapshot (const juce::File& file, bool includeInput, bool includeOutput, bool includeConfig)
    {
        if (!includeInput && !includeOutput && includeConfig)
        {
            return fileManager.importSystemConfig (file);
        }
        if (includeInput && !includeOutput && !includeConfig)
        {
            return fileManager.importInputConfig (file);
        }
        if (includeOutput && !includeInput && !includeConfig)
        {
            return fileManager.importOutputConfig (file);
        }
        return fileManager.importCompleteConfig (file);
    }

    //==============================================================================
    // Direct Access to New System
    //==============================================================================

    /** Get direct access to the new WFSValueTreeState */
    WFSValueTreeState& getValueTreeState() { return valueTreeState; }
    const WFSValueTreeState& getValueTreeState() const { return valueTreeState; }

    /** Get direct access to the file manager */
    WFSFileManager& getFileManager() { return fileManager; }
    const WFSFileManager& getFileManager() const { return fileManager; }

    /** Get the parameter dirty tracker */
    ParameterDirtyTracker& getDirtyTracker() { return dirtyTracker; }
    const ParameterDirtyTracker& getDirtyTracker() const { return dirtyTracker; }

    /** Get undo manager for the active domain */
    juce::UndoManager* getUndoManager() { return valueTreeState.getUndoManager(); }

    /** Get undo manager for a specific domain */
    juce::UndoManager* getUndoManagerForDomain (UndoDomain domain) { return valueTreeState.getUndoManagerForDomain (domain); }

    /** Get active undo manager (alias) */
    juce::UndoManager* getActiveUndoManager() { return valueTreeState.getActiveUndoManager(); }

private:
    WFSValueTreeState valueTreeState;
    WFSFileManager fileManager;
    ParameterDirtyTracker dirtyTracker;

    //==============================================================================
    // Parameter Name Mapping
    //==============================================================================

    /** Map old-style parameter names to new Identifiers */
    static juce::Identifier mapParamNameToIdentifier (const juce::String& paramName)
    {
        // Direct mapping - most names are the same
        // Handle special cases for old naming conventions

        // Show section
        if (paramName == "ShowName") return WFSParameterIDs::showName;
        if (paramName == "ShowLocation") return WFSParameterIDs::showLocation;

        // I/O section
        if (paramName == "InputChannels") return WFSParameterIDs::inputChannels;
        if (paramName == "OutputChannels") return WFSParameterIDs::outputChannels;
        if (paramName == "ReverbChannels") return WFSParameterIDs::reverbChannels;
        if (paramName == "ProcessingEnabled") return WFSParameterIDs::runDSP;
        if (paramName == "ProcessingAlgorithm") return WFSParameterIDs::algorithmDSP;

        // Stage section
        if (paramName == "StageShape") return WFSParameterIDs::stageShape;
        if (paramName == "StageWidth") return WFSParameterIDs::stageWidth;
        if (paramName == "StageDepth") return WFSParameterIDs::stageDepth;
        if (paramName == "StageHeight") return WFSParameterIDs::stageHeight;
        if (paramName == "StageDiameter") return WFSParameterIDs::stageDiameter;
        if (paramName == "DomeElevation") return WFSParameterIDs::domeElevation;
        if (paramName == "StageOriginWidth") return WFSParameterIDs::originWidth;
        if (paramName == "StageOriginDepth") return WFSParameterIDs::originDepth;
        if (paramName == "StageOriginHeight") return WFSParameterIDs::originHeight;
        if (paramName == "SpeedOfSound") return WFSParameterIDs::speedOfSound;
        if (paramName == "Temperature") return WFSParameterIDs::temperature;

        // Master section
        if (paramName == "MasterLevel") return WFSParameterIDs::masterLevel;
        if (paramName == "SystemLatency") return WFSParameterIDs::systemLatency;
        if (paramName == "HaasEffect") return WFSParameterIDs::haasEffect;

        // Network section
        if (paramName == "CurrentIPv4" || paramName == "NetworkCurrentIP") return WFSParameterIDs::networkCurrentIP;
        if (paramName == "UdpPort" || paramName == "NetworkRxUDPport") return WFSParameterIDs::networkRxUDPport;
        if (paramName == "TcpPort" || paramName == "NetworkRxTCPport") return WFSParameterIDs::networkRxTCPport;
        if (paramName == "NetworkInterface") return WFSParameterIDs::networkInterface;

        // ADM-OSC section
        if (paramName == "AdmOscOffsetX") return WFSParameterIDs::admOscOffsetX;
        if (paramName == "AdmOscOffsetY") return WFSParameterIDs::admOscOffsetY;
        if (paramName == "AdmOscOffsetZ") return WFSParameterIDs::admOscOffsetZ;
        if (paramName == "AdmOscScaleX") return WFSParameterIDs::admOscScaleX;
        if (paramName == "AdmOscScaleY") return WFSParameterIDs::admOscScaleY;
        if (paramName == "AdmOscScaleZ") return WFSParameterIDs::admOscScaleZ;
        if (paramName == "AdmOscFlipX") return WFSParameterIDs::admOscFlipX;
        if (paramName == "AdmOscFlipY") return WFSParameterIDs::admOscFlipY;
        if (paramName == "AdmOscFlipZ") return WFSParameterIDs::admOscFlipZ;

        // Tracking section
        if (paramName == "TrackingEnabled") return WFSParameterIDs::trackingEnabled;
        if (paramName == "TrackingProtocol") return WFSParameterIDs::trackingProtocol;
        if (paramName == "TrackingPort") return WFSParameterIDs::trackingPort;
        if (paramName == "TrackingOffsetX") return WFSParameterIDs::trackingOffsetX;
        if (paramName == "TrackingOffsetY") return WFSParameterIDs::trackingOffsetY;
        if (paramName == "TrackingOffsetZ") return WFSParameterIDs::trackingOffsetZ;
        if (paramName == "TrackingScaleX") return WFSParameterIDs::trackingScaleX;
        if (paramName == "TrackingScaleY") return WFSParameterIDs::trackingScaleY;
        if (paramName == "TrackingScaleZ") return WFSParameterIDs::trackingScaleZ;
        if (paramName == "TrackingFlipX") return WFSParameterIDs::trackingFlipX;
        if (paramName == "TrackingFlipY") return WFSParameterIDs::trackingFlipY;
        if (paramName == "TrackingFlipZ") return WFSParameterIDs::trackingFlipZ;

        // Default: use the parameter name directly as identifier
        return juce::Identifier (paramName);
    }

    /** Set config parameter by determining section from param name */
    void setConfigParamBySection (const juce::String& paramName, const juce::Identifier& id, const juce::var& value)
    {
        auto config = valueTreeState.getConfigState();
        if (!config.isValid())
            return;

        // Determine which section based on param name prefix or known params
        juce::ValueTree targetSection;

        if (paramName.startsWith ("Show") || id == WFSParameterIDs::showName || id == WFSParameterIDs::showLocation)
            targetSection = config.getChildWithName (WFSParameterIDs::Show);
        else if (paramName.startsWith ("Stage") || paramName.startsWith ("Origin") ||
                 paramName == "DomeElevation" || paramName == "SpeedOfSound" || paramName == "Temperature")
            targetSection = config.getChildWithName (WFSParameterIDs::Stage);
        else if (paramName.startsWith ("Master") || paramName == "SystemLatency" || paramName == "HaasEffect")
            targetSection = config.getChildWithName (WFSParameterIDs::Master);
        else if (paramName.startsWith ("Network") || paramName.startsWith ("Current") ||
                 paramName.contains ("Port") || paramName.contains ("Udp") || paramName.contains ("Tcp"))
            targetSection = config.getChildWithName (WFSParameterIDs::Network);
        else if (paramName.startsWith ("AdmOsc"))
            targetSection = config.getChildWithName (WFSParameterIDs::ADMOSC);
        else if (paramName.startsWith ("Tracking"))
            targetSection = config.getChildWithName (WFSParameterIDs::Tracking);
        else
            targetSection = config.getChildWithName (WFSParameterIDs::IO);

        if (targetSection.isValid())
            targetSection.setProperty (id, value, valueTreeState.getUndoManager());
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WfsParameters)
};
