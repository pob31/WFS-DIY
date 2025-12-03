#pragma once

#include <JuceHeader.h>

/**
 * WFS Parameter Management System
 *
 * Manages all application parameters using JUCE ValueTree for:
 * - Hierarchical parameter organization
 * - Automatic undo/redo support
 * - Efficient save/load with scope filtering
 * - Thread-safe parameter access
 *
 * Parameter Scopes:
 * - Config: System configuration (I/O, stage, network, etc.)
 * - Input: Input channel settings (per-channel parameters)
 * - Output: Output channel settings (per-channel parameters)
 * - Snapshot: Scene snapshots with scope-based recall filtering
 */
class WfsParameters
{
public:
    WfsParameters()
    {
        initializeParameterTree();
    }

    // Parameter Scopes for save/load filtering
    enum class Scope
    {
        Config,      // System configuration
        Input,       // Input channel parameters
        Output,      // Output channel parameters
        Snapshot     // Scene snapshots
    };

    //==============================================================================
    // Parameter Access

    /** Get config parameter value */
    juce::var getConfigParam(const juce::String& paramName) const
    {
        return configParams.getProperty(paramName);
    }

    /** Set config parameter value */
    void setConfigParam(const juce::String& paramName, const juce::var& value)
    {
        configParams.setProperty(paramName, value, nullptr);
    }

    /** Get input channel parameter */
    juce::var getInputParam(int channelIndex, const juce::String& paramName) const
    {
        auto channel = inputParams.getChild(channelIndex);
        return channel.isValid() ? channel.getProperty(paramName) : juce::var();
    }

    /** Set input channel parameter */
    void setInputParam(int channelIndex, const juce::String& paramName, const juce::var& value)
    {
        auto channel = inputParams.getChild(channelIndex);
        if (channel.isValid())
            channel.setProperty(paramName, value, nullptr);
    }

    /** Get output channel parameter */
    juce::var getOutputParam(int channelIndex, const juce::String& paramName) const
    {
        auto channel = outputParams.getChild(channelIndex);
        return channel.isValid() ? channel.getProperty(paramName) : juce::var();
    }

    /** Set output channel parameter */
    void setOutputParam(int channelIndex, const juce::String& paramName, const juce::var& value)
    {
        auto channel = outputParams.getChild(channelIndex);
        if (channel.isValid())
            channel.setProperty(paramName, value, nullptr);
    }

    //==============================================================================
    // Channel Management

    /** Set number of input channels and initialize their parameters */
    void setNumInputChannels(int numChannels)
    {
        inputParams.removeAllChildren(nullptr);
        for (int i = 0; i < numChannels; ++i)
        {
            juce::ValueTree channel("InputChannel");
            initializeInputChannelParams(channel, i);
            inputParams.appendChild(channel, nullptr);
        }
    }

    /** Set number of output channels and initialize their parameters */
    void setNumOutputChannels(int numChannels)
    {
        outputParams.removeAllChildren(nullptr);
        for (int i = 0; i < numChannels; ++i)
        {
            juce::ValueTree channel("OutputChannel");
            initializeOutputChannelParams(channel, i);
            outputParams.appendChild(channel, nullptr);
        }
    }

    int getNumInputChannels() const { return inputParams.getNumChildren(); }
    int getNumOutputChannels() const { return outputParams.getNumChildren(); }

    //==============================================================================
    // Save/Load

    /** Save complete configuration to XML file */
    bool saveCompleteConfig(const juce::File& file)
    {
        auto xml = rootParams.createXml();
        if (xml != nullptr)
            return xml->writeTo(file);
        return false;
    }

    /** Load complete configuration from XML file */
    bool loadCompleteConfig(const juce::File& file)
    {
        auto xml = juce::parseXML(file);
        if (xml != nullptr)
        {
            auto loadedTree = juce::ValueTree::fromXml(*xml);
            if (loadedTree.isValid())
            {
                rootParams.copyPropertiesAndChildrenFrom(loadedTree, nullptr);
                return true;
            }
        }
        return false;
    }

    /** Save only system configuration (not channel data) */
    bool saveSystemConfig(const juce::File& file)
    {
        auto xml = configParams.createXml();
        if (xml != nullptr)
            return xml->writeTo(file);
        return false;
    }

    /** Load only system configuration */
    bool loadSystemConfig(const juce::File& file)
    {
        auto xml = juce::parseXML(file);
        if (xml != nullptr)
        {
            auto loadedTree = juce::ValueTree::fromXml(*xml);
            if (loadedTree.isValid())
            {
                configParams.copyPropertiesAndChildrenFrom(loadedTree, nullptr);
                return true;
            }
        }
        return false;
    }

    /** Save snapshot with scope filtering */
    bool saveSnapshot(const juce::File& file, bool includeInput, bool includeOutput, bool includeConfig)
    {
        juce::ValueTree snapshot("Snapshot");

        if (includeConfig)
            snapshot.appendChild(configParams.createCopy(), nullptr);
        if (includeInput)
            snapshot.appendChild(inputParams.createCopy(), nullptr);
        if (includeOutput)
            snapshot.appendChild(outputParams.createCopy(), nullptr);

        auto xml = snapshot.createXml();
        if (xml != nullptr)
            return xml->writeTo(file);
        return false;
    }

    /** Load snapshot with scope filtering */
    bool loadSnapshot(const juce::File& file, bool includeInput, bool includeOutput, bool includeConfig)
    {
        auto xml = juce::parseXML(file);
        if (xml != nullptr)
        {
            auto snapshot = juce::ValueTree::fromXml(*xml);
            if (snapshot.isValid())
            {
                if (includeConfig)
                {
                    auto config = snapshot.getChildWithName("ConfigParams");
                    if (config.isValid())
                        configParams.copyPropertiesAndChildrenFrom(config, nullptr);
                }
                if (includeInput)
                {
                    auto input = snapshot.getChildWithName("InputParams");
                    if (input.isValid())
                        inputParams.copyPropertiesAndChildrenFrom(input, nullptr);
                }
                if (includeOutput)
                {
                    auto output = snapshot.getChildWithName("OutputParams");
                    if (output.isValid())
                        outputParams.copyPropertiesAndChildrenFrom(output, nullptr);
                }
                return true;
            }
        }
        return false;
    }

    //==============================================================================
    // ValueTree Access (for listeners)

    juce::ValueTree& getRootTree() { return rootParams; }
    juce::ValueTree& getConfigTree() { return configParams; }
    juce::ValueTree& getInputTree() { return inputParams; }
    juce::ValueTree& getOutputTree() { return outputParams; }

private:
    void initializeParameterTree()
    {
        rootParams = juce::ValueTree("WfsParameters");
        configParams = juce::ValueTree("ConfigParams");
        inputParams = juce::ValueTree("InputParams");
        outputParams = juce::ValueTree("OutputParams");

        rootParams.appendChild(configParams, nullptr);
        rootParams.appendChild(inputParams, nullptr);
        rootParams.appendChild(outputParams, nullptr);

        initializeConfigParams();
    }

    void initializeConfigParams()
    {
        // Show Section
        configParams.setProperty("ShowName", "My Show", nullptr);
        configParams.setProperty("ShowLocation", "", nullptr);

        // I/O Section
        configParams.setProperty("InputChannels", 8, nullptr);
        configParams.setProperty("OutputChannels", 16, nullptr);
        configParams.setProperty("ReverbChannels", 0, nullptr);
        configParams.setProperty("ProcessingEnabled", false, nullptr);

        // Stage Section
        configParams.setProperty("StageWidth", 20.0f, nullptr);
        configParams.setProperty("StageDepth", 10.0f, nullptr);
        configParams.setProperty("StageHeight", 8.0f, nullptr);
        configParams.setProperty("StageOriginWidth", 0.0f, nullptr);
        configParams.setProperty("StageOriginDepth", 0.0f, nullptr);
        configParams.setProperty("StageOriginHeight", 0.0f, nullptr);
        configParams.setProperty("SpeedOfSound", 343.0f, nullptr);
        configParams.setProperty("Temperature", 20.0f, nullptr);

        // Master Section
        configParams.setProperty("MasterLevel", 0.0f, nullptr);
        configParams.setProperty("SystemLatency", 0.0f, nullptr);
        configParams.setProperty("HaasEffect", 0.0f, nullptr);

        // Network Section
        configParams.setProperty("CurrentIPv4", "127.0.0.1", nullptr);
        configParams.setProperty("UdpPort", 9000, nullptr);
        configParams.setProperty("TcpPort", 9001, nullptr);

        // ADM-OSC Section (to be added)
        // Tracking Section (to be added)
    }

    void initializeInputChannelParams(juce::ValueTree& channel, int index)
    {
        channel.setProperty("ChannelIndex", index, nullptr);
        channel.setProperty("Name", "Input " + juce::String(index + 1), nullptr);
        channel.setProperty("Enabled", true, nullptr);
        channel.setProperty("Gain", 0.0f, nullptr);
        channel.setProperty("Pan", 0.0f, nullptr);
        // Additional input channel parameters to be added
    }

    void initializeOutputChannelParams(juce::ValueTree& channel, int index)
    {
        channel.setProperty("ChannelIndex", index, nullptr);
        channel.setProperty("Name", "Output " + juce::String(index + 1), nullptr);
        channel.setProperty("Enabled", true, nullptr);
        channel.setProperty("Gain", 0.0f, nullptr);
        // Additional output channel parameters to be added
    }

    juce::ValueTree rootParams;
    juce::ValueTree configParams;
    juce::ValueTree inputParams;
    juce::ValueTree outputParams;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WfsParameters)
};
