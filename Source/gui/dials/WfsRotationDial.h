#pragma once

#include <JuceHeader.h>

class WfsRotationDial : public juce::Component
{
public:
    WfsRotationDial() = default;

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

        g.setColour(backgroundColour.brighter(0.2f));
        g.drawEllipse(circleBounds, 2.0f);

        auto angleRad = juce::degreesToRadians(angleDegrees - 90.0f);
        auto indicatorLength = radius * 0.8f;
        juce::Point<float> indicatorEnd(
            centre.x + indicatorLength * std::cos(angleRad),
            centre.y + indicatorLength * std::sin(angleRad));

        g.setColour(indicatorColour);
        g.drawLine({ centre, indicatorEnd }, radius * 0.08f);
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        dragStartAngle = angleDegrees;
        dragStartPosition = event.position;
    }

    void mouseDrag(const juce::MouseEvent& event) override
    {
        auto delta = event.position - dragStartPosition;
        auto deltaAngle = (delta.x - delta.y);
        setAngle(dragStartAngle + deltaAngle);
    }

    float angleDegrees = 0.0f;
    juce::Colour backgroundColour { juce::Colours::black };
    juce::Colour indicatorColour { juce::Colours::white };
    juce::Colour tickColour { juce::Colours::grey };

    float dragStartAngle = 0.0f;
    juce::Point<float> dragStartPosition;
};
