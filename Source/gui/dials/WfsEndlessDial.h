#pragma once

#include <JuceHeader.h>
#include <cmath>

class WfsEndlessDial : public juce::Component
{
public:
    WfsEndlessDial()
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
        // Normalize to -180 to 180 range: ((x+180) % 360) - 180
        degrees = std::fmod(degrees + 180.0f, 360.0f);
        if (degrees < 0.0f) degrees += 360.0f;
        degrees -= 180.0f;

        if (!juce::approximatelyEqual(degrees, angleDegrees))
        {
            angleDegrees = degrees;
            if (onAngleChanged)
                onAngleChanged(angleDegrees);
            repaint();
        }
    }
    float getAngle() const noexcept { return angleDegrees; }

    void setSensitivity(float degreesPerPixel) { dragSensitivity = juce::jmax(1.0f, degreesPerPixel); }

    void setColours(juce::Colour background, juce::Colour indicator, juce::Colour /*unusedTickColour*/ = juce::Colours::transparentWhite)
    {
        backgroundColour = background;
        indicatorColour = indicator;
        repaint();
    }

    std::function<void(float)> onAngleChanged;

private:
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
        auto centre = bounds.getCentre();
        auto radius = size * 0.5f;

        juce::Rectangle<float> circleBounds(
            centre.x - radius,
            centre.y - radius,
            radius * 2.0f,
            radius * 2.0f);

        g.setColour(backgroundColour.darker(0.7f));
        g.fillEllipse(circleBounds);

        // Draw full circle track
        auto trackRadius = radius * 0.8f;
        auto trackWidth = radius * 0.12f;
        g.setColour(backgroundColour.brighter(0.2f));
        g.drawEllipse(juce::Rectangle<float>(
            centre.x - trackRadius, centre.y - trackRadius,
            trackRadius * 2.0f, trackRadius * 2.0f), trackWidth);

        // Draw indicator dot on the track (Android app style)
        // +90 offset so 0° is at the bottom
        auto angleRad = juce::degreesToRadians(angleDegrees + 90.0f);
        auto dotRadius = trackWidth * 0.8f;
        juce::Point<float> dotPosition(
            centre.x + trackRadius * std::cos(angleRad),
            centre.y + trackRadius * std::sin(angleRad));

        g.setColour(indicatorColour);
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
        
        // Accumulate angle change (convert radians to degrees, apply sensitivity)
        accumulatedAngleChange += juce::radiansToDegrees(angleDelta) * dragSensitivity;
        dragStartAngle = currentAngle; // Update for next drag
        
        setAngle(dragStartAngleDegrees + accumulatedAngleChange);
    }

    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override
    {
        auto increment = 5.0f * dragSensitivity; // 5 degrees per step (scaled by sensitivity)
        setAngle(angleDegrees + wheel.deltaY * increment);
    }
    
    void paintOverChildren(juce::Graphics&) override
    {
        // Prevent JUCE from drawing default focus indicators
    }

    float angleDegrees = 0.0f;
    float dragSensitivity = 1.0f;
    juce::Colour backgroundColour { juce::Colours::black };
    juce::Colour indicatorColour { juce::Colours::white };

    float dragStartAngleDegrees = 0.0f;
    float dragStartAngle = 0.0f; // In radians
    float accumulatedAngleChange = 0.0f; // In degrees
};
