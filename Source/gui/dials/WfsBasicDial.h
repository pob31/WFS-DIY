#pragma once

#include <JuceHeader.h>
#include "../ColorScheme.h"

class WfsBasicDial : public juce::Component
{
public:
    WfsBasicDial()
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

    void setValue(float newValue)
    {
        newValue = juce::jlimit(minValue, maxValue, newValue);
        if (!juce::approximatelyEqual(newValue, value))
        {
            value = newValue;
            if (onValueChanged)
                onValueChanged(value);
            repaint();
        }
    }
    float getValue() const noexcept { return value; }

    void setRange(float newMin, float newMax)
    {
        if (newMax > newMin)
        {
            minValue = newMin;
            maxValue = newMax;
            setValue(value);
        }
    }

    void setColours(juce::Colour background, juce::Colour indicator, juce::Colour text)
    {
        backgroundColour = background;
        indicatorColour = indicator;
        textColour = text;
        repaint();
    }

    void setTrackColours(juce::Colour inactive, juce::Colour active)
    {
        inactiveTrackColour = inactive;
        activeTrackColour = active;
        repaint();
    }

    std::function<void(float)> onValueChanged;

private:
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
        auto centre = bounds.getCentre();
        auto radius = size * 0.5f;

        // Background is transparent - no fill

        // Needle angle parameters: 315° range
        const float needleAngleRange = juce::degreesToRadians(315.0f);
        const float needleStartAngle = juce::degreesToRadians(112.5f);  // Needle starts at 7:30 position

        // Track arc parameters: rotated 90° clockwise from needle so dead zone is at bottom
        const float trackAngleRange = juce::degreesToRadians(315.0f);
        const float trackStartAngle = juce::degreesToRadians(202.5f);  // Track starts at 4:30 position
        const float trackEndAngle = trackStartAngle + trackAngleRange; // Track ends at 7:30 position

        // Draw inactive track (full range) - use theme color
        auto trackRadius = radius * 0.8f;
        auto trackWidth = radius * 0.12f;
        juce::Path inactiveTrack;
        inactiveTrack.addCentredArc(centre.x, centre.y, trackRadius, trackRadius,
                                     0.0f, trackStartAngle, trackEndAngle, true);
        g.setColour(ColorScheme::get().buttonBorder);
        g.strokePath(inactiveTrack, juce::PathStrokeType(trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Calculate current needle angle
        const float normalizedValue = (juce::jlimit(minValue, maxValue, value) - minValue) / (maxValue - minValue);
        const float currentNeedleAngle = needleStartAngle + needleAngleRange * normalizedValue;

        // Calculate corresponding track angle (90° offset from needle)
        const float currentTrackAngle = trackStartAngle + trackAngleRange * normalizedValue;

        // Draw active track (from start to current value)
        juce::Path activeTrack;
        activeTrack.addCentredArc(centre.x, centre.y, trackRadius, trackRadius,
                                   0.0f, trackStartAngle, currentTrackAngle, true);
        g.setColour(activeTrackColour);
        g.strokePath(activeTrack, juce::PathStrokeType(trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Draw indicator dot on the track (Android app style) - use theme color
        auto dotRadius = trackWidth * 0.8f; // Dot slightly smaller than track width
        juce::Point<float> dotPosition(
            centre.x + trackRadius * std::cos(currentNeedleAngle),
            centre.y + trackRadius * std::sin(currentNeedleAngle));

        g.setColour(ColorScheme::get().sliderThumb);
        g.fillEllipse(dotPosition.x - dotRadius, dotPosition.y - dotRadius,
                      dotRadius * 2.0f, dotRadius * 2.0f);
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        dragStartValue = value;
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
        
        // Accumulate angle change
        accumulatedAngleChange += angleDelta;
        dragStartAngle = currentAngle; // Update for next drag
        
        // Convert accumulated angular change to value change
        // Needle angle range is 315 degrees (7/8 of full circle)
        const float needleAngleRange = juce::degreesToRadians(315.0f);
        auto normalizedDelta = accumulatedAngleChange / needleAngleRange;
        auto deltaValue = normalizedDelta * (maxValue - minValue);
        
        setValue(dragStartValue + deltaValue);
    }

    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override
    {
        auto increment = (maxValue - minValue) * 0.01f; // 1% of range per step
        setValue(value + wheel.deltaY * increment);
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        dragStartValue = value;
    }
    
    void paintOverChildren(juce::Graphics&) override
    {
        // Prevent JUCE from drawing default focus indicators
    }

    float value = 0.0f;
    float minValue = 0.0f;
    float maxValue = 1.0f;

    juce::Colour backgroundColour { juce::Colours::black };
    juce::Colour indicatorColour { juce::Colours::white };
    juce::Colour textColour { juce::Colours::white };
    juce::Colour inactiveTrackColour { juce::Colour::fromRGB(50, 50, 50) };
    juce::Colour activeTrackColour { juce::Colour::fromRGB(0, 150, 255) };

    float dragStartValue = 0.0f;
    float dragStartAngle = 0.0f;
    float accumulatedAngleChange = 0.0f;
};
