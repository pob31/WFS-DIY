#pragma once

#include <JuceHeader.h>
#include "ConfigTabComponent.h"

class ConfigTabPreviewWindow : public juce::DocumentWindow
{
public:
    ConfigTabPreviewWindow()
        : DocumentWindow("Config Tab Preview",
                         juce::Colours::black,
                         DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar(true);
        setResizable(true, true);

        configTab = std::make_unique<ConfigTabComponent>();
        setContentOwned(configTab.release(), true);

        const int preferredWidth = 1440;
        const int preferredHeight = 740;

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

private:
    std::unique_ptr<ConfigTabComponent> configTab;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConfigTabPreviewWindow)
};
