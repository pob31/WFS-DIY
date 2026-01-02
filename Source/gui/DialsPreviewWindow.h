#pragma once

#include <JuceHeader.h>
#include "DialUIComponents.h"
#include "WindowUtils.h"

class DialsPreviewComponent : public juce::Component
{
public:
    DialsPreviewComponent()
    {
        setOpaque(true);
        setWantsKeyboardFocus(false);
        setInterceptsMouseClicks(true, true);
        
        // Prevent components from taking keyboard focus
        setFocusContainerType(FocusContainerType::none);

        configureLabel(dialLabel, "Dial Variants");
        
        // Configure dials
        basicDial.setColours(juce::Colours::black, juce::Colours::white, juce::Colours::white);
        basicDial.setTrackColours(juce::Colour::fromRGB(50, 50, 50), juce::Colour::fromRGB(244, 67, 54));
        rotationDial.setColours(juce::Colours::black, juce::Colours::white, juce::Colours::grey);
        endlessDial.setColours(juce::Colours::black, juce::Colours::white, juce::Colours::grey);
        
        // Prevent dials from taking focus
        auto disableFocusForComponent = [](juce::Component& comp)
        {
            comp.setWantsKeyboardFocus(false);
            comp.setFocusContainerType(FocusContainerType::none);
            comp.setMouseClickGrabsKeyboardFocus(false);
        };
        
        disableFocusForComponent(basicDial);
        disableFocusForComponent(rotationDial);
        disableFocusForComponent(endlessDial);
        
        addAndMakeVisible(dialLabel);
        addAndMakeVisible(basicDial);
        addAndMakeVisible(rotationDial);
        addAndMakeVisible(endlessDial);
        
        // Set initial values for dials so they're visible
        juce::MessageManager::callAsync([this]()
        {
            basicDial.setValue(0.5f);
            rotationDial.setAngle(45.0f);
            endlessDial.setAngle(90.0f);
        });
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black.withAlpha(0.85f));
        auto bounds = getLocalBounds().toFloat();
        g.setColour(juce::Colours::white.withAlpha(0.05f));
        g.drawRoundedRectangle(bounds.reduced(4.0f), 12.0f, 2.0f);
    }
    
    bool hitTest(int x, int y) override
    {
        return Component::hitTest(x, y);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(12);

        // Label at top
        auto labelArea = area.removeFromTop(30);
        dialLabel.setBounds(labelArea);
        area.removeFromTop(10);
        
        // Arrange dials horizontally
        auto size = juce::jmin(area.getWidth() / 3, area.getHeight() - 20, 200);
        auto spacing = 20;
        
        auto left = area.removeFromLeft(size);
        basicDial.setBounds(left.withSizeKeepingCentre(size, size));
        
        area.removeFromLeft(spacing);
        auto middle = area.removeFromLeft(size);
        rotationDial.setBounds(middle.withSizeKeepingCentre(size, size));
        
        area.removeFromLeft(spacing);
        auto right = area.removeFromLeft(size);
        endlessDial.setBounds(right.withSizeKeepingCentre(size, size));
    }

private:
    void configureLabel(juce::Label& label, const juce::String& text)
    {
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, juce::Colours::white);
        label.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultSansSerifFontName(), 16.0f, juce::Font::bold)));
    }

    juce::Label dialLabel;
    WfsBasicDial basicDial;
    WfsRotationDial rotationDial;
    WfsEndlessDial endlessDial;
};

class DialsPreviewRootComponent : public juce::Component
{
public:
    DialsPreviewRootComponent()
    {
        setOpaque(true);
        setWantsKeyboardFocus(false);
        
        previewComponent = std::make_unique<DialsPreviewComponent>();
        previewComponent->setSize(800, 300);
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

private:
    std::unique_ptr<DialsPreviewComponent> previewComponent;
};

class DialsPreviewWindow : public juce::DocumentWindow
{
public:
    DialsPreviewWindow()
        : DocumentWindow("WFS Dials Preview",
                         juce::Colours::black,
                         DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar(true);
        setResizable(true, true);
        
        setContentOwned(new DialsPreviewRootComponent(), true);

        const int preferredWidth = 850;
        const int preferredHeight = 350;

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

    ~DialsPreviewWindow() override
    {
        // Cleanup handled by unique_ptr
    }

};

