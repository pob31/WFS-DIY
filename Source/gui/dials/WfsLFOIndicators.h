#pragma once

#include <JuceHeader.h>
#include "../sliders/WfsBidirectionalSlider.h"

/**
 * Read-only bidirectional slider for displaying LFO output values.
 * Shows a value from -1 to +1 with the indicator moving from center.
 * Does not respond to mouse input.
 */
class WfsLFOOutputSlider : public juce::Component
{
public:
    WfsLFOOutputSlider()
    {
        slider.setEnabled (false);
        slider.setTrackColours (juce::Colour (0xFF1E1E1E), juce::Colour (0xFF00BCD4));
        slider.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (slider);
    }

    void setValue (float newValue)
    {
        slider.setValue (juce::jlimit (-1.0f, 1.0f, newValue));
    }

    float getValue() const noexcept
    {
        return slider.getValue();
    }

    void setTrackColour (juce::Colour colour)
    {
        slider.setTrackColours (juce::Colour (0xFF1E1E1E), colour);
    }

    void resized() override
    {
        slider.setBounds (getLocalBounds());
    }

private:
    WfsBidirectionalSlider slider;
};

/**
 * LFO Progress Indicator - shows cycle progress as a rotating dot.
 * Read-only, updated externally via setProgress().
 */
class WfsLFOProgressDial : public juce::Component
{
public:
    WfsLFOProgressDial()
    {
        setOpaque (false);
        setInterceptsMouseClicks (false, false);
    }

    /**
     * Set progress value (0.0 to 1.0)
     * 0.0 = dot at bottom, progresses clockwise
     */
    void setProgress (float newProgress)
    {
        newProgress = juce::jlimit (0.0f, 1.0f, newProgress);
        if (! juce::approximatelyEqual (newProgress, progress))
        {
            progress = newProgress;
            repaint();
        }
    }

    float getProgress() const noexcept { return progress; }

    void setColours (juce::Colour bg, juce::Colour indicator)
    {
        backgroundColour = bg;
        indicatorColour = indicator;
        repaint();
    }

    void setActive (bool shouldBeActive)
    {
        if (isActive != shouldBeActive)
        {
            isActive = shouldBeActive;
            repaint();
        }
    }

private:
    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        auto size = juce::jmin (bounds.getWidth(), bounds.getHeight());
        auto centre = bounds.getCentre();
        auto radius = size * 0.5f;

        // Background circle
        juce::Rectangle<float> circleBounds (
            centre.x - radius,
            centre.y - radius,
            radius * 2.0f,
            radius * 2.0f);

        g.setColour (backgroundColour.darker (0.7f));
        g.fillEllipse (circleBounds);

        // Draw track circle
        auto trackRadius = radius * 0.75f;
        auto trackWidth = radius * 0.15f;
        g.setColour (backgroundColour.brighter (0.2f));
        g.drawEllipse (
            centre.x - trackRadius,
            centre.y - trackRadius,
            trackRadius * 2.0f,
            trackRadius * 2.0f,
            trackWidth);

        if (isActive)
        {
            // Calculate position on track (from bottom, clockwise)
            // Angle: 0 = bottom (90°), progress goes clockwise
            auto startAngle = juce::MathConstants<float>::halfPi;  // 90° = bottom
            auto currentAngle = startAngle + progress * juce::MathConstants<float>::twoPi;

            // Draw indicator dot at current position (no active track arc)
            auto dotRadius = trackWidth * 0.6f;
            juce::Point<float> dotPosition (
                centre.x + trackRadius * std::cos (currentAngle),
                centre.y + trackRadius * std::sin (currentAngle));

            g.setColour (indicatorColour.brighter (0.3f));
            g.fillEllipse (
                dotPosition.x - dotRadius,
                dotPosition.y - dotRadius,
                dotRadius * 2.0f,
                dotRadius * 2.0f);
        }
    }

    float progress = 0.0f;
    bool isActive = false;
    juce::Colour backgroundColour { juce::Colours::black };
    juce::Colour indicatorColour { juce::Colour (0xFF00BCD4) };  // Cyan
};
