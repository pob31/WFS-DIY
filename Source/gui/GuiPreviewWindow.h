#pragma once

#include <JuceHeader.h>
#include "GuiPreviewComponent.h"
#include "WindowUtils.h"

class GuiPreviewRootComponent : public juce::Component
{
public:
    GuiPreviewRootComponent()
    {
        setOpaque(true);
        setWantsKeyboardFocus(false);
        
        // Add preview component directly (viewport causes assertion issues - scrolling will be added later)
        previewComponent = std::make_unique<GuiPreviewComponent>();
        previewComponent->setSize(defaultContentWidth, defaultContentHeight);
        addAndMakeVisible(previewComponent.get());
    }
    
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black);
    }

    void resized() override
    {
        if (previewComponent != nullptr)
        {
            previewComponent->setBounds(getLocalBounds());
        }
    }

    static constexpr int defaultContentWidth = 860;
    static constexpr int defaultContentHeight = 1400;

private:
    std::unique_ptr<GuiPreviewComponent> previewComponent;
};

class GuiPreviewWindow : public juce::DocumentWindow
{
public:
    GuiPreviewWindow()
        : DocumentWindow("WFS Control UI Preview",
                         juce::Colours::black,
                         DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar(true);
        setResizable(true, true);
        
        setContentOwned(new GuiPreviewRootComponent(), true);

        const int preferredWidth = 900;
        const int preferredHeight = 1000;

        auto& displays = juce::Desktop::getInstance().getDisplays();
        const auto* displayPtr = displays.getPrimaryDisplay();
        juce::Rectangle<int> userArea = (displayPtr != nullptr && !displayPtr->userArea.isEmpty())
            ? displayPtr->userArea
            : displays.getTotalBounds(true);

        auto getSafeDimension = [](int preferred, int availableMinusMargin, int available)
        {
            if (availableMinusMargin > 0)
                return juce::jmin(preferred, availableMinusMargin);
            if (available > 0)
                return juce::jmin(preferred, available);
            return preferred;
        };

        const int margin = 40;
        const int windowWidth = getSafeDimension(preferredWidth,
                                                 userArea.getWidth() - margin,
                                                 userArea.getWidth());
        const int windowHeight = getSafeDimension(preferredHeight,
                                                  userArea.getHeight() - margin,
                                                  userArea.getHeight());

        const int maxWidth = userArea.getWidth() > 0 ? userArea.getWidth() : preferredWidth;
        const int maxHeight = userArea.getHeight() > 0 ? userArea.getHeight() : preferredHeight;

        const int minWidth = (windowWidth >= 200) ? 200 : juce::jmax(100, windowWidth);
        const int minHeight = (windowHeight >= 200) ? 200 : juce::jmax(100, windowHeight);

        setResizeLimits(minWidth, minHeight,
                        juce::jmax(minWidth, maxWidth),
                        juce::jmax(minHeight, maxHeight));

        centreWithSize(windowWidth, windowHeight);
        setVisible(true);
        WindowUtils::enableDarkTitleBar(this);
    }

    void closeButtonPressed() override
    {
        setVisible(false);
    }

    ~GuiPreviewWindow() override
    {
        // Cleanup handled by unique_ptr
    }

};
