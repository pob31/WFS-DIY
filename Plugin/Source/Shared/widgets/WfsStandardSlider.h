#pragma once

#include "WfsSliderBase.h"

// Copied from Source/gui/sliders/WfsStandardSlider.h with surgery:
//   * removed WfsLookAndFeel::uiScale dependency (use 1.0f)
//   * replaced ColorScheme::get() calls with plugin DarkPalette
class WfsStandardSlider : public WfsSliderBase
{
public:
    explicit WfsStandardSlider (Orientation direction = Orientation::horizontal)
        : WfsSliderBase (0.0f, 1.0f, direction)
    {
        setTrackColours (juce::Colour (wfs::plugin::DarkPalette::sliderTrackBg),
                         juce::Colour (wfs::plugin::DarkPalette::accentBlueBright));
        setThumbColour  (juce::Colour (wfs::plugin::DarkPalette::sliderThumb));
    }

    WfsStandardSlider (float minVal, float maxVal, Orientation direction = Orientation::horizontal)
        : WfsSliderBase (minVal, maxVal, direction)
    {
        setTrackColours (juce::Colour (wfs::plugin::DarkPalette::sliderTrackBg),
                         juce::Colour (wfs::plugin::DarkPalette::accentBlueBright));
        setThumbColour  (juce::Colour (wfs::plugin::DarkPalette::sliderThumb));
    }

    void setLabel (juce::String newLabel) { labelText = std::move (newLabel); repaint(); }
    void setInlineMode (bool enabled)     { inlineMode = enabled; repaint(); }
    void setValueToString (std::function<juce::String (float)> fn) { valueFormatter = std::move (fn); }

protected:
    juce::Rectangle<float> getPointerBounds() const override
    {
        if (inlineMode && labelText.isNotEmpty())
        {
            auto b = getLocalBounds().toFloat();
            const float labelW = b.getWidth() * 0.30f;
            const float valueW = b.getWidth() * 0.20f;
            b.removeFromLeft  (labelW);
            b.removeFromRight (valueW);
            return b;
        }
        return getLocalBounds().toFloat();
    }

    void paintSlider (juce::Graphics& g, juce::Rectangle<float> bounds) override
    {
        const auto alpha            = isEnabled() ? 1.0f : disabledAlpha;
        const auto foregroundColour = trackForegroundColour.withAlpha (alpha);

        if (inlineMode && labelText.isNotEmpty())
        {
            const float totalW = bounds.getWidth();
            const float labelW = totalW * 0.30f;
            const float valueW = totalW * 0.20f;

            auto labelArea = bounds.removeFromLeft  (labelW);
            auto valueArea = bounds.removeFromRight (valueW);

            g.setColour (juce::Colour (wfs::plugin::DarkPalette::textPrimary).withAlpha (alpha));
            g.setFont   (juce::FontOptions (11.0f));
            g.drawText (labelText, labelArea.toNearestInt(), juce::Justification::centredLeft, false);

            const juce::String valStr = valueFormatter ? valueFormatter (value) : juce::String (value, 1);
            g.drawText (valStr, valueArea.toNearestInt(), juce::Justification::centredRight, false);

            auto usable   = getUsableBounds (bounds);
            auto track    = getTrackBounds (usable);
            auto thumbPos = getThumbPosition (usable);

            g.setColour (juce::Colour (wfs::plugin::DarkPalette::sliderTrackBg).withAlpha (alpha));
            g.fillRect (track);

            juce::Rectangle<float> active (track);
            if (getOrientation() == Orientation::horizontal)
                active.setWidth (juce::jmax (1.0f, thumbPos.x - track.getX()));
            else
            {
                active.setY (thumbPos.y);
                active.setHeight (juce::jmax (1.0f, track.getBottom() - thumbPos.y));
            }

            g.setColour (isHovered ? foregroundColour.brighter (0.3f) : foregroundColour);
            g.fillRect (active);

            drawThumbIndicator (g, track, thumbPos, alpha);
            return;
        }

        auto usable   = getUsableBounds (bounds);
        auto track    = getTrackBounds (usable);
        auto thumbPos = getThumbPosition (usable);

        g.setColour (juce::Colour (wfs::plugin::DarkPalette::sliderTrackBg).withAlpha (alpha));
        g.fillRect (track);

        juce::Rectangle<float> active (track);
        if (getOrientation() == Orientation::horizontal)
            active.setWidth (juce::jmax (1.0f, thumbPos.x - track.getX()));
        else
        {
            active.setY (thumbPos.y);
            active.setHeight (juce::jmax (1.0f, track.getBottom() - thumbPos.y));
        }

        g.setColour (isHovered ? foregroundColour.brighter (0.3f) : foregroundColour);
        g.fillRect (active);

        drawThumbIndicator (g, track, thumbPos, alpha);

        if (labelText.isNotEmpty())
        {
            g.setColour (juce::Colour (wfs::plugin::DarkPalette::textPrimary).withAlpha (alpha));
            g.setFont   (juce::FontOptions (14.0f));
            g.drawText (labelText,
                        getLocalBounds().withBottom (juce::roundToInt (usable.getY()) - 4),
                        juce::Justification::centredBottom,
                        false);
        }
    }

private:
    juce::String labelText;
    bool         inlineMode = false;
    std::function<juce::String (float)> valueFormatter;
};
