#pragma once

#include "WfsSliderBase.h"
#include <cmath>

class WfsBidirectionalSlider : public WfsSliderBase
{
public:
    explicit WfsBidirectionalSlider(Orientation direction = Orientation::horizontal)
        : WfsSliderBase(-1.0f, 1.0f, direction)
    {
        setTrackColours(juce::Colour::fromRGB(30, 30, 30),
                        juce::Colour::fromRGB(76, 175, 80));
        setThumbColour(juce::Colours::white);
        setTrackThickness(40.0f);
    }

protected:
    void paintSlider(juce::Graphics& g, juce::Rectangle<float> bounds) override
    {
        auto usable = getUsableBounds(bounds);
        auto track = getTrackBounds(usable);
        auto thumbPos = getThumbPosition(usable);

        const auto alpha = isEnabled() ? 1.0f : disabledAlpha;

        g.setColour(trackBackgroundColour.withAlpha(alpha));
        g.fillRect(track);

        const auto centrePoint = (getOrientation() == Orientation::horizontal)
                                     ? juce::Point<float>(track.getCentreX(), track.getCentreY())
                                     : juce::Point<float>(track.getCentreX(), track.getCentreY());

        juce::Rectangle<float> active(track);
        if (getOrientation() == Orientation::horizontal)
        {
            const auto startX = juce::jmin(thumbPos.x, centrePoint.x);
            const auto width = juce::jmax(1.0f, std::abs(thumbPos.x - centrePoint.x));
            active.setX(startX);
            active.setWidth(width);
        }
        else
        {
            const auto thumbY = thumbPos.y;
            const auto centreY = centrePoint.y;
            const auto height = juce::jmax(1.0f, std::abs(thumbY - centreY));
            active.setY(juce::jmin(thumbY, centreY));
            active.setHeight(height);
        }

        g.setColour(trackForegroundColour.withAlpha(alpha));
        g.fillRect(active);

        // zero marker
        auto zeroRect = track;
        if (getOrientation() == Orientation::horizontal)
        {
            zeroRect.setX(track.getCentreX() - 1.0f);
            zeroRect.setWidth(2.0f);
        }
        else
        {
            zeroRect.setY(track.getCentreY() - 1.0f);
            zeroRect.setHeight(2.0f);
        }
        g.setColour(trackForegroundColour.withMultipliedAlpha(0.35f));
        g.fillRect(zeroRect);

        drawThumbIndicator(g, track, thumbPos, alpha);
    }
};
