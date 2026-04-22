#pragma once

#include "WfsSliderBase.h"
#include <cmath>

// Copied from Source/gui/sliders/WfsWidthExpansionSlider.h with surgery:
//   * replaced ColorScheme::get() with plugin DarkPalette
//   * default track colours taken from the main app's "directivity" tint
class WfsWidthExpansionSlider : public WfsSliderBase
{
public:
    explicit WfsWidthExpansionSlider (Orientation direction = Orientation::horizontal)
        : WfsSliderBase (0.0f, 1.0f, direction)
    {
        setTrackColours (juce::Colour (wfs::plugin::DarkPalette::sliderTrackBg),
                         juce::Colour (0xFF26A69A));  // teal matches main app
        setThumbColour  (juce::Colour (wfs::plugin::DarkPalette::sliderThumb));
    }

protected:
    float valueFromNormalized (float normalizedPos) const override
    {
        normalizedPos = juce::jlimit (0.0f, 1.0f, normalizedPos);
        return juce::jlimit (0.0f, 1.0f, 2.0f * std::abs (0.5f - normalizedPos));
    }

    float normalizedFromValue (float currentValue) const override
    {
        currentValue = juce::jlimit (0.0f, 1.0f, currentValue);
        if (currentValue <= 0.5f)
            return 0.5f - (currentValue * 0.5f);
        return 0.5f + (currentValue * 0.5f);
    }

    void paintSlider (juce::Graphics& g, juce::Rectangle<float> bounds) override
    {
        auto usable = getUsableBounds (bounds);
        auto track  = getTrackBounds (usable);

        const auto alpha = isEnabled() ? 1.0f : disabledAlpha;

        g.setColour (juce::Colour (wfs::plugin::DarkPalette::sliderTrackBg).withAlpha (alpha));
        g.fillRect (track);

        const auto widthFraction = getValue();
        juce::Rectangle<float> active (track);
        if (getOrientation() == Orientation::horizontal)
        {
            const auto w = track.getWidth() * widthFraction;
            active.setX (track.getCentreX() - w * 0.5f);
            active.setWidth (w);
        }
        else
        {
            const auto h = track.getHeight() * widthFraction;
            active.setY (track.getCentreY() - h * 0.5f);
            active.setHeight (h);
        }

        const auto activeColour = isHovered ? trackForegroundColour.brighter (0.3f).withAlpha (alpha)
                                            : trackForegroundColour.withAlpha (alpha);
        g.setColour (activeColour);
        g.fillRect (active);

        juce::Point<float> leftThumb, rightThumb;
        if (getOrientation() == Orientation::horizontal)
        {
            leftThumb  = { active.getX(),     track.getCentreY() };
            rightThumb = { active.getRight(), track.getCentreY() };
        }
        else
        {
            leftThumb  = { track.getCentreX(), active.getY() };
            rightThumb = { track.getCentreX(), active.getBottom() };
        }
        drawThumbIndicator (g, track, leftThumb,  alpha);
        drawThumbIndicator (g, track, rightThumb, alpha);
    }
};
