#pragma once

#include "WfsSliderBase.h"
#include <cmath>

// Copied from Source/gui/sliders/WfsBidirectionalSlider.h with one addition:
// the (min, max) constructor, so we can drive the slider directly from a
// symmetric real-value parameter (e.g. tilt -90..+90°) rather than forcing
// -1..+1 normalized.
class WfsBidirectionalSlider : public WfsSliderBase
{
public:
    explicit WfsBidirectionalSlider (Orientation direction = Orientation::horizontal)
        : WfsSliderBase (-1.0f, 1.0f, direction)
    {
        applyDefaultColours();
    }

    WfsBidirectionalSlider (float minVal, float maxVal,
                            Orientation direction = Orientation::horizontal)
        : WfsSliderBase (minVal, maxVal, direction)
    {
        applyDefaultColours();
    }

protected:
    void paintSlider (juce::Graphics& g, juce::Rectangle<float> bounds) override
    {
        auto usable   = getUsableBounds (bounds);
        auto track    = getTrackBounds (usable);
        auto thumbPos = getThumbPosition (usable);

        const auto alpha = isEnabled() ? 1.0f : disabledAlpha;

        g.setColour (juce::Colour (wfs::plugin::DarkPalette::sliderTrackBg).withAlpha (alpha));
        g.fillRect (track);

        const auto centrePoint = juce::Point<float> (track.getCentreX(), track.getCentreY());

        juce::Rectangle<float> active (track);
        if (getOrientation() == Orientation::horizontal)
        {
            const auto startX = juce::jmin (thumbPos.x, centrePoint.x);
            const auto width  = juce::jmax (1.0f, std::abs (thumbPos.x - centrePoint.x));
            active.setX (startX);
            active.setWidth (width);
        }
        else
        {
            const auto startY = juce::jmin (thumbPos.y, centrePoint.y);
            const auto height = juce::jmax (1.0f, std::abs (thumbPos.y - centrePoint.y));
            active.setY (startY);
            active.setHeight (height);
        }

        const auto activeColour = isHovered ? trackForegroundColour.brighter (0.3f).withAlpha (alpha)
                                            : trackForegroundColour.withAlpha (alpha);
        g.setColour (activeColour);
        g.fillRect (active);

        auto zeroRect = track;
        const float markerW = juce::jmax (1.0f, trackThickness * 0.1f);
        if (getOrientation() == Orientation::horizontal)
        {
            zeroRect.setX (track.getCentreX() - markerW * 0.5f);
            zeroRect.setWidth (markerW);
        }
        else
        {
            zeroRect.setY (track.getCentreY() - markerW * 0.5f);
            zeroRect.setHeight (markerW);
        }
        g.setColour (trackForegroundColour.withMultipliedAlpha (0.35f));
        g.fillRect (zeroRect);

        drawThumbIndicator (g, track, thumbPos, alpha);
    }

private:
    void applyDefaultColours()
    {
        setTrackColours (juce::Colour (wfs::plugin::DarkPalette::sliderTrackBg),
                         juce::Colour (0xFF26A69A)); // teal — matches directivity/tilt in InputsTab
        setThumbColour  (juce::Colour (wfs::plugin::DarkPalette::sliderThumb));
    }
};
