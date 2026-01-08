#pragma once

#include <JuceHeader.h>
#include <cmath>
#include <algorithm>
#include "../ColorScheme.h"

/**
 * WfsRangeSlider - Double-thumbed horizontal slider for min/max range selection.
 *
 * Features:
 * - Two independently movable thumbs that can cross each other
 * - Inactive track color outside thumbs, active track color between thumbs
 * - getMinValue() returns lower value, getMaxValue() returns upper value (auto-swapped)
 * - Follows existing slider patterns with hover effects and ColorScheme integration
 */
class WfsRangeSlider : public juce::Component
{
public:
    WfsRangeSlider (float minRangeValue = 0.0f, float maxRangeValue = 50.0f)
        : rangeMin (minRangeValue),
          rangeMax (maxRangeValue),
          thumbValue1 (minRangeValue),
          thumbValue2 (maxRangeValue)
    {
        setRepaintsOnMouseActivity (false);
        setWantsKeyboardFocus (false);
        setFocusContainerType (FocusContainerType::none);
        setOpaque (false);
        setMouseClickGrabsKeyboardFocus (false);
    }

    // Get lower/upper values (auto-swapped)
    float getMinValue() const noexcept { return juce::jmin (thumbValue1, thumbValue2); }
    float getMaxValue() const noexcept { return juce::jmax (thumbValue1, thumbValue2); }

    // Get raw thumb values (not swapped)
    float getThumb1Value() const noexcept { return thumbValue1; }
    float getThumb2Value() const noexcept { return thumbValue2; }

    // Set values directly (thumb1 = left/min concept, thumb2 = right/max concept)
    void setValues (float val1, float val2)
    {
        thumbValue1 = juce::jlimit (rangeMin, rangeMax, val1);
        thumbValue2 = juce::jlimit (rangeMin, rangeMax, val2);
        repaint();
    }

    // Set range bounds
    void setRange (float newMin, float newMax)
    {
        rangeMin = newMin;
        rangeMax = newMax;
        thumbValue1 = juce::jlimit (rangeMin, rangeMax, thumbValue1);
        thumbValue2 = juce::jlimit (rangeMin, rangeMax, thumbValue2);
        repaint();
    }

    // Callback for value changes - provides min and max (auto-swapped)
    std::function<void (float, float)> onValuesChanged;

    // Track colors
    void setTrackColours (juce::Colour inactive, juce::Colour active)
    {
        trackInactiveColour = inactive;
        trackActiveColour = active;
        repaint();
    }

    void setTrackThickness (float thickness) noexcept
    {
        trackThickness = thickness;
        repaint();
    }

protected:
    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        auto usable = bounds.reduced (thumbRadius * 0.75f);

        // Track bounds
        auto track = juce::Rectangle<float> (usable.getX(),
                                             usable.getCentreY() - trackThickness * 0.5f,
                                             usable.getWidth(),
                                             trackThickness);

        // Calculate thumb positions
        float thumb1X = getThumbX (thumbValue1, usable);
        float thumb2X = getThumbX (thumbValue2, usable);
        float leftX = juce::jmin (thumb1X, thumb2X);
        float rightX = juce::jmax (thumb1X, thumb2X);

        const float alpha = isEnabled() ? 1.0f : 0.38f;

        // Draw inactive track (full length, behind active)
        g.setColour (trackActiveColour.withAlpha (alpha * 0.24f));
        g.fillRect (track);

        // Draw active track (between thumbs)
        auto activeTrack = track.withX (leftX).withWidth (rightX - leftX);
        auto activeColour = isHovered ? trackActiveColour.brighter (0.3f) : trackActiveColour;
        g.setColour (activeColour.withAlpha (alpha));
        g.fillRect (activeTrack);

        // Draw thumb indicators
        drawThumbIndicator (g, track, { thumb1X, track.getCentreY() }, alpha);
        drawThumbIndicator (g, track, { thumb2X, track.getCentreY() }, alpha);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        auto usable = getLocalBounds().toFloat().reduced (thumbRadius * 0.75f);
        draggedThumb = getClosestThumb (e.position.x, usable);
        updateThumbValue (e.position.x, usable);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (draggedThumb >= 0)
        {
            auto usable = getLocalBounds().toFloat().reduced (thumbRadius * 0.75f);
            updateThumbValue (e.position.x, usable);
        }
    }

    void mouseUp (const juce::MouseEvent&) override { draggedThumb = -1; }

    void mouseEnter (const juce::MouseEvent&) override
    {
        isHovered = true;
        repaint();
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        isHovered = false;
        repaint();
    }

    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override
    {
        auto usable = getLocalBounds().toFloat().reduced (thumbRadius * 0.75f);
        int thumb = getClosestThumb (e.position.x, usable);
        float increment = (rangeMax - rangeMin) * 0.01f;  // 1% of range per step

        if (thumb == 0)
        {
            thumbValue1 = juce::jlimit (rangeMin, rangeMax, thumbValue1 + wheel.deltaY * increment);
        }
        else
        {
            thumbValue2 = juce::jlimit (rangeMin, rangeMax, thumbValue2 + wheel.deltaY * increment);
        }

        repaint();
        if (onValuesChanged)
            onValuesChanged (getMinValue(), getMaxValue());
    }

private:
    float rangeMin, rangeMax;
    float thumbValue1, thumbValue2;  // Raw thumb positions (can cross)
    int draggedThumb = -1;           // -1=none, 0=thumb1, 1=thumb2
    bool isHovered = false;

    float trackThickness = 20.0f;
    float thumbRadius = 8.0f;

    juce::Colour trackInactiveColour { 0xFF1C1C1C };
    juce::Colour trackActiveColour { 0xFF00BCD4 };

    float getThumbX (float value, const juce::Rectangle<float>& usable) const
    {
        float normalized = (value - rangeMin) / (rangeMax - rangeMin);
        normalized = juce::jlimit (0.0f, 1.0f, normalized);
        return usable.getX() + normalized * usable.getWidth();
    }

    float valueFromX (float x, const juce::Rectangle<float>& usable) const
    {
        if (usable.getWidth() <= 0.0f)
            return rangeMin;

        float normalized = (x - usable.getX()) / usable.getWidth();
        normalized = juce::jlimit (0.0f, 1.0f, normalized);
        return rangeMin + normalized * (rangeMax - rangeMin);
    }

    int getClosestThumb (float x, const juce::Rectangle<float>& usable) const
    {
        float thumb1X = getThumbX (thumbValue1, usable);
        float thumb2X = getThumbX (thumbValue2, usable);

        float dist1 = std::abs (x - thumb1X);
        float dist2 = std::abs (x - thumb2X);

        return (dist1 <= dist2) ? 0 : 1;
    }

    void updateThumbValue (float x, const juce::Rectangle<float>& usable)
    {
        float newValue = valueFromX (x, usable);

        if (draggedThumb == 0)
            thumbValue1 = newValue;
        else if (draggedThumb == 1)
            thumbValue2 = newValue;

        repaint();
        if (onValuesChanged)
            onValuesChanged (getMinValue(), getMaxValue());
    }

    void drawThumbIndicator (juce::Graphics& g,
                             const juce::Rectangle<float>& /* track */,
                             const juce::Point<float>& thumbPos,
                             float alpha) const
    {
        // Draw thin line thumb matching Android app design - uses ColorScheme for theming
        auto colour = ColorScheme::get().sliderThumb.withAlpha (alpha);
        g.setColour (colour);

        // Thumb line thickness (stroke width along track) - kept thin for clean look
        const float lineThickness = 2.0f;

        // For horizontal sliders: vertical line (perpendicular to track)
        // Thumb width across track is 80% of track thickness (matching Android)
        const float lineLength = trackThickness * 0.8f;
        g.drawLine (thumbPos.x,
                    thumbPos.y - lineLength * 0.5f,
                    thumbPos.x,
                    thumbPos.y + lineLength * 0.5f,
                    lineThickness);
    }
};
