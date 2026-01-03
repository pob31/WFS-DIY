#pragma once

#include <JuceHeader.h>
#include "JoystickUIComponents.h"
#include "SliderUIComponents.h"
#include "DialUIComponents.h"

class GuiPreviewComponent : public juce::Component
{
public:
    GuiPreviewComponent()
    {
        setOpaque(true);
        setWantsKeyboardFocus(false);
        setInterceptsMouseClicks(true, true);
        
        // Prevent components from taking keyboard focus to avoid grey backgrounds
        setFocusContainerType(FocusContainerType::none);

        configureLabel(joystickLabel, "Joystick");
        configureLabel(standardLabel, "Standard Slider");
        configureLabel(bidirectionalLabel, "Bidirectional Slider");
        configureLabel(autoCenterLabel, "Auto-Center Slider");
        configureLabel(widthExpansionLabel, "Width Expansion Slider");
        configureLabel(dialLabel, "Dial Variants");

        standardHorizontal.setLabel("Horizontal");
        standardVertical.setLabel("Vertical");
        standardVertical.setTrackColours(juce::Colour::fromRGB(30, 30, 30),
                                         juce::Colour::fromRGB(156, 39, 176));

        bidirectionalHorizontal.setTrackColours(juce::Colour::fromRGB(25, 25, 25),
                                                juce::Colour::fromRGB(76, 175, 80));
        bidirectionalVertical.setTrackColours(juce::Colour::fromRGB(25, 25, 25),
                                              juce::Colour::fromRGB(33, 150, 243));

        autoCenterHorizontal.setTrackColours(juce::Colour::fromRGB(28, 28, 28),
                                             juce::Colour::fromRGB(255, 152, 0));
        autoCenterVertical.setTrackColours(juce::Colour::fromRGB(28, 28, 28),
                                           juce::Colour::fromRGB(233, 30, 99));

        widthHorizontal.setTrackColours(juce::Colour::fromRGB(20, 20, 20),
                                        juce::Colour::fromRGB(0, 188, 212));
        widthVertical.setTrackColours(juce::Colour::fromRGB(20, 20, 20),
                                      juce::Colour::fromRGB(126, 87, 194));

        addAndMakeVisible(joystickLabel);
        addAndMakeVisible(standardLabel);
        addAndMakeVisible(bidirectionalLabel);
        addAndMakeVisible(autoCenterLabel);
        addAndMakeVisible(widthExpansionLabel);
        addAndMakeVisible(dialLabel);

        addAndMakeVisible(joystick);
        
        // Prevent all slider components from taking focus and showing hover indicators
        auto disableFocusForComponent = [](juce::Component& comp)
        {
            comp.setWantsKeyboardFocus(false);
            comp.setFocusContainerType(FocusContainerType::none);
            comp.setMouseClickGrabsKeyboardFocus(false);
            comp.setMouseCursor(juce::MouseCursor::NormalCursor);
        };
        
        disableFocusForComponent(joystick);
        disableFocusForComponent(standardHorizontal);
        disableFocusForComponent(standardVertical);
        disableFocusForComponent(bidirectionalHorizontal);
        disableFocusForComponent(bidirectionalVertical);
        disableFocusForComponent(autoCenterHorizontal);
        disableFocusForComponent(autoCenterVertical);
        disableFocusForComponent(widthHorizontal);
        disableFocusForComponent(widthVertical);
        disableFocusForComponent(basicDial);
        disableFocusForComponent(rotationDial);
        disableFocusForComponent(endlessDial);

        addAndMakeVisible(standardHorizontal);
        addAndMakeVisible(standardVertical);

        addAndMakeVisible(bidirectionalHorizontal);
        addAndMakeVisible(bidirectionalVertical);

        addAndMakeVisible(autoCenterHorizontal);
        addAndMakeVisible(autoCenterVertical);

        addAndMakeVisible(widthHorizontal);
        addAndMakeVisible(widthVertical);
        
        // Set initial values so sliders are visible in preview
        // Defer setting values to avoid paint issues during construction
        juce::MessageManager::callAsync([this]()
        {
            bidirectionalHorizontal.setValue(0.3f);
            bidirectionalVertical.setValue(-0.4f);
            // Auto-center sliders initialize at 0 (center) in their constructor
            widthHorizontal.setValue(0.7f);
            widthVertical.setValue(0.5f);
            standardHorizontal.setValue(0.4f);
            standardVertical.setValue(0.6f);
        });

        basicDial.setColours(juce::Colours::black, juce::Colours::white, juce::Colours::white);
        basicDial.setTrackColours(juce::Colour::fromRGB(50, 50, 50), juce::Colour::fromRGB(244, 67, 54));
        rotationDial.setColours(juce::Colours::black, juce::Colours::white, juce::Colours::grey);
        endlessDial.setColours(juce::Colours::black, juce::Colours::white, juce::Colours::grey);
        
        // Set initial values for dials so they're visible
        juce::MessageManager::callAsync([this]()
        {
            basicDial.setValue(0.5f);
            rotationDial.setAngle(45.0f); // Set angle in degrees
            endlessDial.setAngle(90.0f);  // Set angle in degrees
        });

        addAndMakeVisible(basicDial);
        addAndMakeVisible(rotationDial);
        addAndMakeVisible(endlessDial);
    }

    void paint(juce::Graphics& g) override
    {
        // Clear the entire area first to prevent artifacts
        g.fillAll(juce::Colours::black.withAlpha(0.85f));
        auto bounds = getLocalBounds().toFloat();
        g.setColour(juce::Colours::white.withAlpha(0.05f));
        g.drawRoundedRectangle(bounds.reduced(4.0f), 12.0f, 2.0f);
    }
    
    
    bool hitTest(int x, int y) override
    {
        // Allow hit testing but don't grant focus
        return Component::hitTest(x, y);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(12);

        auto configureSection = [&](juce::Label& label, auto&& layoutLambda)
        {
            if (area.getHeight() <= 0)
                return;

            auto section = area.removeFromTop(200);
            section.removeFromTop(9);
            label.setBounds(section.removeFromTop(24));
            layoutLambda(section);
            area.removeFromTop(18);
        };

        configureSection(joystickLabel, [&](juce::Rectangle<int> section)
        {
            auto size = juce::jmin(section.getWidth(), section.getHeight());
            size = juce::jmin(size, 240);
            auto joystickBounds = juce::Rectangle<int>(size, size).withCentre(section.getCentre());
            joystick.setBounds(joystickBounds);
        });

        auto layoutSliderPair = [&](WfsSliderBase& horizontal, WfsSliderBase& vertical, juce::Rectangle<int> section)
        {
            auto hArea = section.removeFromTop(100);
            horizontal.setBounds(hArea.reduced(12, 20));
            section.removeFromTop(10);
            auto vArea = section;
            auto verticalBounds = juce::Rectangle<int>(96, vArea.getHeight() - 20).withCentre(section.getCentre());
            vertical.setBounds(verticalBounds);
        };

        configureSection(standardLabel, [&](juce::Rectangle<int> section)
        {
            layoutSliderPair(standardHorizontal, standardVertical, section);
        });

        configureSection(bidirectionalLabel, [&](juce::Rectangle<int> section)
        {
            layoutSliderPair(bidirectionalHorizontal, bidirectionalVertical, section);
        });

        configureSection(autoCenterLabel, [&](juce::Rectangle<int> section)
        {
            layoutSliderPair(autoCenterHorizontal, autoCenterVertical, section);
        });

        configureSection(widthExpansionLabel, [&](juce::Rectangle<int> section)
        {
            layoutSliderPair(widthHorizontal, widthVertical, section);
        });

        configureSection(dialLabel, [&](juce::Rectangle<int> section)
        {
            auto size = juce::jmin(section.getWidth() / 3, section.getHeight() - 20);
            auto dialArea = juce::Rectangle<int>(size, size);
            auto spacing = 10;

            auto left = section.removeFromLeft(size);
            basicDial.setBounds(left.withSizeKeepingCentre(size, size));

            section.removeFromLeft(spacing);
            auto middle = section.removeFromLeft(size);
            rotationDial.setBounds(middle.withSizeKeepingCentre(size, size));

            section.removeFromLeft(spacing);
            auto right = section;
            endlessDial.setBounds(right.withSizeKeepingCentre(size, size));
        });
    }

private:
    void configureLabel(juce::Label& label, const juce::String& text)
    {
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultSansSerifFontName(), 16.0f, juce::Font::bold)));
    }

    juce::Label joystickLabel;
    juce::Label standardLabel;
    juce::Label bidirectionalLabel;
    juce::Label autoCenterLabel;
    juce::Label widthExpansionLabel;
    juce::Label dialLabel;

    WfsJoystickComponent joystick;
    WfsStandardSlider standardHorizontal { WfsSliderBase::Orientation::horizontal };
    WfsStandardSlider standardVertical { WfsSliderBase::Orientation::vertical };
    WfsBidirectionalSlider bidirectionalHorizontal { WfsSliderBase::Orientation::horizontal };
    WfsBidirectionalSlider bidirectionalVertical { WfsSliderBase::Orientation::vertical };
    WfsAutoCenterSlider autoCenterHorizontal { WfsSliderBase::Orientation::horizontal };
    WfsAutoCenterSlider autoCenterVertical { WfsSliderBase::Orientation::vertical };
    WfsWidthExpansionSlider widthHorizontal { WfsSliderBase::Orientation::horizontal };
    WfsWidthExpansionSlider widthVertical { WfsSliderBase::Orientation::vertical };

    WfsBasicDial basicDial;
    WfsRotationDial rotationDial;
    WfsEndlessDial endlessDial;
};
