#pragma once

#include "WfsSliderBase.h"
#include "../WfsLookAndFeel.h"

class WfsStandardSlider : public WfsSliderBase
{
public:
    explicit WfsStandardSlider(Orientation direction = Orientation::horizontal)
        : WfsSliderBase(0.0f, 1.0f, direction)
    {
        setTrackColours(juce::Colour::fromRGB(45, 45, 45),
                        juce::Colour::fromRGB(255, 87, 34)); // Default deep orange
        setThumbColour(juce::Colours::white);
    }

    WfsStandardSlider(float minVal, float maxVal, Orientation direction = Orientation::horizontal)
        : WfsSliderBase(minVal, maxVal, direction)
    {
        setTrackColours(juce::Colour::fromRGB(45, 45, 45),
                        juce::Colour::fromRGB(255, 87, 34));
        setThumbColour(juce::Colours::white);
    }

    void setLabel(juce::String newLabel)
    {
        labelText = std::move(newLabel);
        repaint();
    }

    /** Enable inline mode: label left, track centre, value right — all on one row */
    void setInlineMode (bool enabled) { inlineMode = enabled; repaint(); }

    /** Custom value-to-string formatter (default: 1 decimal place) */
    void setValueToString (std::function<juce::String (float)> fn) { valueFormatter = std::move (fn); }

protected:
    juce::Rectangle<float> getPointerBounds() const override
    {
        if (inlineMode && labelText.isNotEmpty())
        {
            auto b = getLocalBounds().toFloat();
            float labelW = b.getWidth() * 0.30f;
            float valueW = b.getWidth() * 0.20f;
            b.removeFromLeft (labelW);
            b.removeFromRight (valueW);
            return b;
        }
        return getLocalBounds().toFloat();
    }

    void paintSlider(juce::Graphics& g, juce::Rectangle<float> bounds) override
    {
        const auto alpha = isEnabled() ? 1.0f : disabledAlpha;
        const auto foregroundColour = trackForegroundColour.withAlpha(alpha);

        if (inlineMode && labelText.isNotEmpty())
        {
            // ── Inline layout: [label 30%] [track 50%] [value 20%] ──
            float totalW = bounds.getWidth();
            float labelW = totalW * 0.30f;
            float valueW = totalW * 0.20f;

            auto labelArea = bounds.removeFromLeft (labelW);
            auto valueArea = bounds.removeFromRight (valueW);
            // bounds is now the track zone

            // Draw label
            g.setColour (juce::Colours::white.withAlpha (alpha));
            g.setFont (juce::FontOptions (juce::jmax (9.0f, 11.0f * WfsLookAndFeel::uiScale)));
            g.drawText (labelText, labelArea.toNearestInt(), juce::Justification::centredLeft, false);

            // Draw value
            juce::String valStr = valueFormatter ? valueFormatter (value) : juce::String (value, 1);
            g.drawText (valStr, valueArea.toNearestInt(), juce::Justification::centredRight, false);

            // Draw track within the middle zone
            auto usable = getUsableBounds (bounds);
            auto track = getTrackBounds (usable);
            auto thumbPos = getThumbPosition (usable);

            g.setColour (ColorScheme::get().sliderTrackBg.withAlpha (alpha));
            g.fillRect (track);

            juce::Rectangle<float> active (track);
            if (getOrientation() == Orientation::horizontal)
                active.setWidth (juce::jmax (1.0f, thumbPos.x - track.getX()));
            else
            {
                active.setY (thumbPos.y);
                active.setHeight (juce::jmax (1.0f, track.getBottom() - thumbPos.y));
            }

            auto activeColour = isHovered ? foregroundColour.brighter (0.3f) : foregroundColour;
            g.setColour (activeColour);
            g.fillRect (active);

            drawThumbIndicator (g, track, thumbPos, alpha);
            return;
        }

        // ── Default mode: label above track ──
        auto usable = getUsableBounds(bounds);
        auto track = getTrackBounds(usable);
        auto thumbPos = getThumbPosition(usable);

        // Track background uses neutral color from theme (black/dark grey/light grey)
        g.setColour(ColorScheme::get().sliderTrackBg.withAlpha(alpha));
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
            g.setFont(juce::jmax(10.0f, 14.0f * WfsLookAndFeel::uiScale));
            g.drawText(labelText,
                       getLocalBounds().withBottom(juce::roundToInt(usable.getY()) - 4),
                       juce::Justification::centredBottom,
                       false);
        }
    }

private:
    juce::String labelText;
    bool inlineMode = false;
    std::function<juce::String (float)> valueFormatter;
};

/**
 * Slider whose active track renders a greyscale gradient from black (or white
 * in light theme) to the current grey level, replacing separate swatch squares.
 */
class WfsGreyscaleSlider : public WfsSliderBase
{
public:
    explicit WfsGreyscaleSlider (Orientation direction = Orientation::horizontal)
        : WfsSliderBase (0.0f, 1.0f, direction)
    {
        setThumbColour (juce::Colours::white);
    }

    void setInvertForLightTheme (bool invert) { invertDisplay = invert; repaint(); }

protected:
    void paintSlider (juce::Graphics& g, juce::Rectangle<float> bounds) override
    {
        auto usable = getUsableBounds (bounds);
        auto track  = getTrackBounds (usable);
        auto thumbPos = getThumbPosition (usable);
        const auto alpha = isEnabled() ? 1.0f : disabledAlpha;

        // Track background
        g.setColour (ColorScheme::get().sliderTrackBg.withAlpha (alpha));
        g.fillRect (track);

        // Active portion
        juce::Rectangle<float> active (track);
        if (getOrientation() == Orientation::horizontal)
            active.setWidth (juce::jmax (1.0f, thumbPos.x - track.getX()));
        else
        {
            active.setY (thumbPos.y);
            active.setHeight (juce::jmax (1.0f, track.getBottom() - thumbPos.y));
        }

        // Greyscale gradient from 0 to current value
        float greyAtThumb = getNormalizedValue();
        if (invertDisplay)
            greyAtThumb = 1.0f - greyAtThumb;

        float startGrey = invertDisplay ? 1.0f : 0.0f;
        auto c0 = juce::Colour::greyLevel (startGrey).withAlpha (alpha);
        auto c1 = juce::Colour::greyLevel (greyAtThumb).withAlpha (alpha);

        if (isHovered)
        {
            c0 = c0.brighter (0.15f).withAlpha (alpha);
            c1 = c1.brighter (0.15f).withAlpha (alpha);
        }

        if (active.getWidth() < 2.0f)
        {
            // Too narrow for a gradient — solid fill avoids Direct2D crash
            g.setColour (c1);
            g.fillRect (active);
        }
        else
        {
            juce::ColourGradient grad (c0, active.getX(), active.getCentreY(),
                                       c1, active.getRight(), active.getCentreY(), false);
            g.setGradientFill (grad);
            g.fillRect (active);
        }

        drawThumbIndicator (g, track, thumbPos, alpha);
    }

private:
    bool invertDisplay = false;
};
