#pragma once

#include "WfsSliderBase.h"
#include <cmath>

class WfsAutoCenterSlider : public WfsSliderBase
{
public:
    explicit WfsAutoCenterSlider(Orientation direction = Orientation::horizontal)
        : WfsSliderBase(-1.0f, 1.0f, direction)
    {
        setTrackColours(juce::Colour::fromRGB(32, 32, 32),
                        juce::Colour::fromRGB(255, 152, 0));
        setThumbColour(juce::Colours::white);
        // Track thickness is now set in base class to match Android design
        // Initialize at center (0)
        setValue(0.0f);
    }

    void setCenterValue(float newCenter)
    {
        centerValue = juce::jlimit(minValue, maxValue, newCenter);
        repaint();
    }

    float getCenterValue() const noexcept { return centerValue; }

protected:
    void paintSlider(juce::Graphics& g, juce::Rectangle<float> bounds) override
    {
        auto usable = getUsableBounds(bounds);
        auto track = getTrackBounds(usable);
        auto thumbPos = getThumbPosition(usable);

        const auto alpha = isEnabled() ? 1.0f : disabledAlpha;

        // Track background uses slider color with 0.24 alpha (matching Android app)
        g.setColour(trackForegroundColour.withAlpha(alpha * 0.24f));
        g.fillRect(track);

        const auto centerNormalized = normalizedFromValue(centerValue);
        juce::Point<float> centerPoint;
        if (getOrientation() == Orientation::horizontal)
        {
            centerPoint = { track.getX() + centerNormalized * track.getWidth(), track.getCentreY() };
        }
        else
        {
            centerPoint = { track.getCentreX(), track.getBottom() - centerNormalized * track.getHeight() };
        }

        juce::Rectangle<float> active(track);
        if (getOrientation() == Orientation::horizontal)
        {
            const auto minX = juce::jmin(centerPoint.x, thumbPos.x);
            const auto width = juce::jmax(1.0f, std::abs(centerPoint.x - thumbPos.x));
            active.setX(minX);
            active.setWidth(width);
        }
        else
        {
            const auto minY = juce::jmin(centerPoint.y, thumbPos.y);
            const auto height = juce::jmax(1.0f, std::abs(centerPoint.y - thumbPos.y));
            active.setY(minY);
            active.setHeight(height);
        }

        // Brighten active track when hovering
        auto activeColour = isHovered ? trackForegroundColour.brighter(0.3f).withAlpha(alpha) : trackForegroundColour.withAlpha(alpha);
        g.setColour(activeColour);
        g.fillRect(active);

        // Centre marker
        g.setColour(trackForegroundColour.withMultipliedAlpha(0.35f));
        if (getOrientation() == Orientation::horizontal)
            g.fillRect(centerPoint.x - 1.0f, track.getY(), 2.0f, track.getHeight());
        else
            g.fillRect(track.getX(), centerPoint.y - 1.0f, track.getWidth(), 2.0f);

        drawThumbIndicator(g, track, thumbPos, alpha);
    }

private:
    void handleMouseUp() override
    {
        setValue(centerValue);
    }

    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override
    {
        // Auto-center sliders don't respond to scroll wheel
    }

    float centerValue = 0.0f;
};
