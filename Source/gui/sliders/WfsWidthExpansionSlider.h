#pragma once

#include "WfsSliderBase.h"
#include <cmath>

class WfsWidthExpansionSlider : public WfsSliderBase
{
public:
    explicit WfsWidthExpansionSlider(Orientation direction = Orientation::horizontal)
        : WfsSliderBase(0.0f, 1.0f, direction)
    {
        setTrackColours(juce::Colour::fromRGB(28, 28, 28),
                        juce::Colour::fromRGB(0, 188, 212));
        setThumbColour(juce::Colours::white);
        // Track thickness is now set in base class to match Android design
    }

protected:
    float valueFromNormalized(float normalizedPos) const override
    {
        normalizedPos = juce::jlimit(0.0f, 1.0f, normalizedPos);
        return juce::jlimit(0.0f, 1.0f, 2.0f * std::abs(0.5f - normalizedPos));
    }

    float normalizedFromValue(float currentValue) const override
    {
        currentValue = juce::jlimit(0.0f, 1.0f, currentValue);
        if (currentValue <= 0.5f)
            return 0.5f - (currentValue * 0.5f);

        return 0.5f + (currentValue * 0.5f);
    }

    void paintSlider(juce::Graphics& g, juce::Rectangle<float> bounds) override
    {
        auto usable = getUsableBounds(bounds);
        auto track = getTrackBounds(usable);
        auto thumbPos = getThumbPosition(usable);

        const auto alpha = isEnabled() ? 1.0f : disabledAlpha;

        // Track background uses slider color with 0.24 alpha (matching Android app)
        g.setColour(trackForegroundColour.withAlpha(alpha * 0.24f));
        g.fillRect(track);

        const auto widthFraction = getValue();
        juce::Rectangle<float> active(track);
        if (getOrientation() == Orientation::horizontal)
        {
            const auto activeWidth = track.getWidth() * widthFraction;
            const auto startX = track.getCentreX() - activeWidth * 0.5f;
            active.setX(startX);
            active.setWidth(activeWidth);
        }
        else
        {
            const auto activeHeight = track.getHeight() * widthFraction;
            const auto startY = track.getCentreY() - activeHeight * 0.5f;
            active.setY(startY);
            active.setHeight(activeHeight);
        }

        // Brighten active track when hovering
        auto activeColour = isHovered ? trackForegroundColour.brighter(0.3f).withAlpha(alpha) : trackForegroundColour.withAlpha(alpha);
        g.setColour(activeColour);
        g.fillRect(active);

        // Draw thumbs on both ends of active track
        juce::Point<float> leftThumbPos, rightThumbPos;
        if (getOrientation() == Orientation::horizontal)
        {
            leftThumbPos = { active.getX(), track.getCentreY() };
            rightThumbPos = { active.getRight(), track.getCentreY() };
        }
        else
        {
            leftThumbPos = { track.getCentreX(), active.getY() };
            rightThumbPos = { track.getCentreX(), active.getBottom() };
        }
        
        drawThumbIndicator(g, track, leftThumbPos, alpha);
        drawThumbIndicator(g, track, rightThumbPos, alpha);
    }
};
