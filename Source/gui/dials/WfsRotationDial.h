#pragma once

#include <JuceHeader.h>

class WfsRotationDial : public juce::Component
{
public:
    WfsRotationDial()
    {
        setWantsKeyboardFocus(false);
        setFocusContainerType(FocusContainerType::none);
        setOpaque(true); // Opaque to prevent JUCE from drawing default backgrounds
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

private:
    void paint(juce::Graphics& g) override
    {
        // Always fill with black background to prevent any hover background from showing
        g.fillAll(juce::Colours::black);
        
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

        // Draw outer rim
        g.setColour(backgroundColour.brighter(0.2f));
        g.drawEllipse(circleBounds, 2.0f);

        // Draw indicator dot on the track (Android app style)
        auto angleRad = juce::degreesToRadians(angleDegrees - 90.0f);
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
    juce::Colour backgroundColour { juce::Colours::black };
    juce::Colour indicatorColour { juce::Colours::white };
    juce::Colour tickColour { juce::Colours::grey };

    float dragStartAngleDegrees = 0.0f;
    float dragStartAngle = 0.0f; // In radians
    float accumulatedAngleChange = 0.0f; // In degrees
};
