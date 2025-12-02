#pragma once

#include "WfsSliderBase.h"

class WfsStandardSlider : public WfsSliderBase
{
public:
    explicit WfsStandardSlider(Orientation direction = Orientation::horizontal)
        : WfsSliderBase(0.0f, 1.0f, direction)
    {
        setTrackColours(juce::Colour::fromRGB(45, 45, 45),
                        juce::Colour::fromRGB(255, 87, 34)); // Default deep orange
        setThumbColour(juce::Colours::white);
        // Track thickness is now set in base class to match Android design
    }

    void setLabel(juce::String newLabel)
    {
        labelText = std::move(newLabel);
        repaint();
    }

protected:
    void paintSlider(juce::Graphics& g, juce::Rectangle<float> bounds) override
    {
        auto usable = getUsableBounds(bounds);
        auto track = getTrackBounds(usable);
        auto thumbPos = getThumbPosition(usable);

        const auto alpha = isEnabled() ? 1.0f : disabledAlpha;
        const auto backgroundColour = trackBackgroundColour.withAlpha(alpha);
        const auto foregroundColour = trackForegroundColour.withAlpha(alpha);

        // Track background uses slider color with 0.24 alpha (matching Android app)
        g.setColour(foregroundColour.withAlpha(alpha * 0.24f));
        g.fillRect(track);

        juce::Rectangle<float> active(track);
        if (getOrientation() == Orientation::horizontal)
        {
            active.setWidth(juce::jmax(1.0f, thumbPos.x - track.getX()));
        }
        else
        {
            active.setY(thumbPos.y);
            active.setHeight(juce::jmax(1.0f, track.getBottom() - thumbPos.y));
        }

        // Brighten active track when hovering
        auto activeColour = isHovered ? foregroundColour.brighter(0.3f) : foregroundColour;
        g.setColour(activeColour);
        g.fillRect(active);

        drawThumbIndicator(g, track, thumbPos, alpha);

        if (labelText.isNotEmpty())
        {
            g.setColour(juce::Colours::white.withAlpha(alpha));
            g.setFont(14.0f);
            g.drawText(labelText,
                       getLocalBounds().withBottom(juce::roundToInt(usable.getY()) - 4),
                       juce::Justification::centredBottom,
                       false);
        }
    }

private:
    juce::String labelText;
};
