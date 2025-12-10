#pragma once

#include <JuceHeader.h>
#include "../WfsParameters.h"
#include "StatusBar.h"
#include "SystemConfigTab.h"
#include "OutputsTab.h"
#include "InputsTab.h"
#include "ClustersTab.h"
#include "ReverbTab.h"
#include "MapTab.h"

/**
 * Main Config Tab Component with multiple sub-tabs
 * - System Configuration
 * - Outputs
 * - Inputs
 * - Clusters
 * - Reverb
 * - Map
 */
class ConfigTabComponent : public juce::Component
{
public:
    ConfigTabComponent(WfsParameters& params)
        : parameters(params)
    {
        // Create tabbed component
        addAndMakeVisible(tabbedComponent);
        tabbedComponent.setTabBarDepth(35);
        tabbedComponent.setOutline(0);

        // Create tabs
        systemConfigTab = new SystemConfigTab(parameters);
        outputsTab = new OutputsTab(parameters);
        inputsTab = new InputsTab(parameters);
        clustersTab = new ClustersTab(parameters);
        reverbTab = new ReverbTab(parameters);
        mapTab = new MapTab(parameters);

        // Add tabs to tabbed component
        tabbedComponent.addTab("System Configuration", juce::Colours::darkgrey, systemConfigTab, true);
        tabbedComponent.addTab("Outputs", juce::Colours::darkgrey, outputsTab, true);
        tabbedComponent.addTab("Inputs", juce::Colours::darkgrey, inputsTab, true);
        tabbedComponent.addTab("Clusters", juce::Colours::darkgrey, clustersTab, true);
        tabbedComponent.addTab("Reverb", juce::Colours::darkgrey, reverbTab, true);
        tabbedComponent.addTab("Map", juce::Colours::darkgrey, mapTab, true);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xFF1E1E1E));
    }

    void resized() override
    {
        tabbedComponent.setBounds(getLocalBounds());
    }

    void setStatusBar(StatusBar* bar)
    {
        statusBar = bar;
        // Pass status bar to system config tab
        systemConfigTab->setStatusBar(bar);
    }

    void setProcessingCallback(SystemConfigTab::ProcessingCallback callback)
    {
        systemConfigTab->setProcessingCallback(callback);
    }

    void setChannelCountCallback(SystemConfigTab::ChannelCountCallback callback)
    {
        systemConfigTab->setChannelCountCallback(callback);
    }

    void setAudioInterfaceCallback(SystemConfigTab::AudioInterfaceCallback callback)
    {
        systemConfigTab->setAudioInterfaceCallback(callback);
    }

private:
    WfsParameters& parameters;
    StatusBar* statusBar = nullptr;

    juce::TabbedComponent tabbedComponent { juce::TabbedButtonBar::TabsAtTop };

    // Tab components (owned by TabbedComponent)
    SystemConfigTab* systemConfigTab;
    OutputsTab* outputsTab;
    InputsTab* inputsTab;
    ClustersTab* clustersTab;
    ReverbTab* reverbTab;
    MapTab* mapTab;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConfigTabComponent)
};
