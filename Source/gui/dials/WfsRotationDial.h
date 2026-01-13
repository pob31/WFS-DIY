#pragma once

#include <JuceHeader.h>
#include "../ColorScheme.h"
#include "../../Accessibility/TTSManager.h"

class WfsRotationDial : public juce::Component
{
public:
    WfsRotationDial()
    {
        setWantsKeyboardFocus(false);
        setFocusContainerType(FocusContainerType::none);
        setOpaque(false); // Transparent background
        setMouseClickGrabsKeyboardFocus(false);
    }
    
    void mouseEnter(const juce::MouseEvent&) override
    {
        // Override to prevent hover effects - do nothing
    }
    
    void mouseExit(const juce::MouseEvent&) override
    {
        // Override to prevent hover effects - do nothing
    }

    void setAngle(float degrees)
    {
        degrees = juce::jlimit(-180.0f, 180.0f, degrees);
        if (!juce::approximatelyEqual(degrees, angleDegrees))
        {
            angleDegrees = degrees;
            if (onAngleChanged)
                onAngleChanged(angleDegrees);

            // TTS: Announce angle change for accessibility
            if (ttsParameterName.isNotEmpty())
            {
                juce::String valueStr = juce::String(static_cast<int>(angleDegrees)) + " degrees";
                TTSManager::getInstance().announceValueChange(ttsParameterName, valueStr);
            }

            repaint();
        }
    }
    float getAngle() const noexcept { return angleDegrees; }

    void setColours(juce::Colour background, juce::Colour indicator, juce::Colour tick)
    {
        backgroundColour = background;
        indicatorColour = indicator;
        tickColour = tick;
        repaint();
    }

    std::function<void(float)> onAngleChanged;

    /** Set parameter name for TTS announcements (e.g., "Rotation") */
    void setTTSParameterName(const juce::String& name) { ttsParameterName = name; }

    /** Configure TTS - unit is automatically "degrees" for rotation dials */
    void setTTSInfo(const juce::String& name) { ttsParameterName = name; }

    /** Set alpha for disabled state (used for visual dimming) */
    void setDisabledAlpha(float alpha) noexcept { disabledAlpha = juce::jlimit(0.0f, 1.0f, alpha); }

private:
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
        auto centre = bounds.getCentre();
        auto radius = size * 0.5f;

        // Background is transparent - no fill

        // Draw full circle track - use theme color with disabled alpha
        auto trackRadius = radius * 0.8f;
        auto trackWidth = radius * 0.12f;
        g.setColour(ColorScheme::get().buttonBorder.withAlpha(disabledAlpha));
        g.drawEllipse(juce::Rectangle<float>(
            centre.x - trackRadius, centre.y - trackRadius,
            trackRadius * 2.0f, trackRadius * 2.0f), trackWidth);

        // Draw indicator dot on the track (Android app style) - use theme color with disabled alpha
        auto angleRad = juce::degreesToRadians(angleDegrees - 90.0f);
        auto dotRadius = trackWidth * 0.8f;
        juce::Point<float> dotPosition(
            centre.x + trackRadius * std::cos(angleRad),
            centre.y + trackRadius * std::sin(angleRad));

        g.setColour(ColorScheme::get().sliderThumb.withAlpha(disabledAlpha));
        g.fillEllipse(dotPosition.x - dotRadius, dotPosition.y - dotRadius,
                      dotRadius * 2.0f, dotRadius * 2.0f);
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        dragStartAngleDegrees = angleDegrees;
        auto bounds = getLocalBounds().toFloat();
        auto centre = bounds.getCentre();
        auto deltaFromCentre = event.position - centre;
        dragStartAngle = std::atan2(deltaFromCentre.y, deltaFromCentre.x);
        accumulatedAngleChange = 0.0f;
    }

    void mouseDrag(const juce::MouseEvent& event) override
    {
        auto bounds = getLocalBounds().toFloat();
        auto centre = bounds.getCentre();
        auto deltaFromCentre = event.position - centre;
        auto currentAngle = std::atan2(deltaFromCentre.y, deltaFromCentre.x);
        
        // Calculate angular change (handle wrap-around)
        auto angleDelta = currentAngle - dragStartAngle;
        if (angleDelta > juce::MathConstants<float>::pi)
            angleDelta -= 2.0f * juce::MathConstants<float>::pi;
        else if (angleDelta < -juce::MathConstants<float>::pi)
            angleDelta += 2.0f * juce::MathConstants<float>::pi;
        
        // Accumulate angle change (convert radians to degrees)
        accumulatedAngleChange += juce::radiansToDegrees(angleDelta);
        dragStartAngle = currentAngle; // Update for next drag
        
        setAngle(dragStartAngleDegrees + accumulatedAngleChange);
    }

    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override
    {
        auto increment = 5.0f; // 5 degrees per step
        setAngle(angleDegrees + wheel.deltaY * increment);
    }
    
    void paintOverChildren(juce::Graphics&) override
    {
        // Prevent JUCE from drawing default focus indicators
    }

    float angleDegrees = 0.0f;
    float disabledAlpha = 1.0f;  // Alpha for disabled state visual dimming

    // TTS accessibility
    juce::String ttsParameterName;

    juce::Colour backgroundColour { juce::Colours::black };
    juce::Colour indicatorColour { juce::Colours::white };
    juce::Colour tickColour { juce::Colours::grey };

    float dragStartAngleDegrees = 0.0f;
    float dragStartAngle = 0.0f; // In radians
    float accumulatedAngleChange = 0.0f; // In degrees
};
