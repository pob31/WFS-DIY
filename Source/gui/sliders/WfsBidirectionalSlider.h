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
        // Track thickness is now set in base class to match Android design
    }

protected:
    void paintSlider(juce::Graphics& g, juce::Rectangle<float> bounds) override
    {
        auto usable = getUsableBounds(bounds);
        auto track = getTrackBounds(usable);
        auto thumbPos = getThumbPosition(usable);

        const auto alpha = isEnabled() ? 1.0f : disabledAlpha;

        // Track background uses neutral color from theme (black/dark grey/light grey)
        g.setColour(ColorScheme::get().sliderTrackBg.withAlpha(alpha));
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

        // Brighten active track when hovering
        auto activeColour = isHovered ? trackForegroundColour.brighter(0.3f).withAlpha(alpha) : trackForegroundColour.withAlpha(alpha);
        g.setColour(activeColour);
        g.fillRect(active);

        // zero marker
        auto zeroRect = track;
        const float markerW = juce::jmax(1.0f, trackThickness * 0.1f);
        if (getOrientation() == Orientation::horizontal)
        {
            zeroRect.setX(track.getCentreX() - markerW * 0.5f);
            zeroRect.setWidth(markerW);
        }
        else
        {
            zeroRect.setY(track.getCentreY() - markerW * 0.5f);
            zeroRect.setHeight(markerW);
        }
        g.setColour(trackForegroundColour.withMultipliedAlpha(0.35f));
        g.fillRect(zeroRect);

        drawThumbIndicator(g, track, thumbPos, alpha);
    }
};
