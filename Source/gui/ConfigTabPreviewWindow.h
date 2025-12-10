#pragma once

#include <JuceHeader.h>
#include "ConfigTabComponent.h"
#include "StatusBar.h"

class ConfigTabPreviewWindow : public juce::DocumentWindow
{
public:
    // Container component that handles layout
    class ContainerComponent : public juce::Component
    {
    public:
        ContainerComponent(ConfigTabComponent* config, StatusBar* status)
            : configTab(config), statusBar(status)
        {
            addAndMakeVisible(configTab);
            addAndMakeVisible(statusBar);
        }

        void resized() override
        {
            auto area = getLocalBounds();
            const int statusBarHeight = 30;

            // Status bar at bottom, full width
            statusBar->setBounds(area.removeFromBottom(statusBarHeight));

            // Config tab takes remaining space
            configTab->setBounds(area);
        }

    private:
        ConfigTabComponent* configTab;
        StatusBar* statusBar;
    };

    ConfigTabPreviewWindow(WfsParameters& params)
        : DocumentWindow("Config Tab Preview",
                         juce::Colours::black,
                         DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar(true);
        setResizable(true, true);

        configTab = std::make_unique<ConfigTabComponent>(params);
        statusBar = std::make_unique<StatusBar>();

        // Create container that handles layout
        auto* container = new ContainerComponent(configTab.get(), statusBar.get());

        // Pass status bar reference to config tab
        configTab->setStatusBar(statusBar.get());

        setContentOwned(container, true);

        const int preferredWidth = 1440;
        const int preferredHeight = 770;  // Increased to accommodate status bar

        auto& displays = juce::Desktop::getInstance().getDisplays();
        const auto* displayPtr = displays.getPrimaryDisplay();
        juce::Rectangle<int> userArea = (displayPtr != nullptr && !displayPtr->userArea.isEmpty())
            ? displayPtr->userArea
            : displays.getTotalBounds(true);

        const int margin = 40;
        const int windowWidth = juce::jmin(preferredWidth, userArea.getWidth() - margin);
        const int windowHeight = juce::jmin(preferredHeight, userArea.getHeight() - margin);

        setResizeLimits(400, 400, userArea.getWidth(), userArea.getHeight());

        centreWithSize(windowWidth, windowHeight);
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        setVisible(false);
    }

    void setProcessingCallback(SystemConfigTab::ProcessingCallback callback)
    {
        configTab->setProcessingCallback(callback);
    }

    void setChannelCountCallback(SystemConfigTab::ChannelCountCallback callback)
    {
        configTab->setChannelCountCallback(callback);
    }

    void setAudioInterfaceCallback(SystemConfigTab::AudioInterfaceCallback callback)
    {
        configTab->setAudioInterfaceCallback(callback);
    }

private:
    std::unique_ptr<ConfigTabComponent> configTab;
    std::unique_ptr<StatusBar> statusBar;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConfigTabPreviewWindow)
};
