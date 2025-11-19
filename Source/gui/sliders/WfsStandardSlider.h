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
        setTrackThickness(40.0f);
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

        g.setColour(backgroundColour);
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

        g.setColour(foregroundColour);
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
