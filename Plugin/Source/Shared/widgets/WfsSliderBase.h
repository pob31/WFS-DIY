#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <limits>
#include "../PluginLookAndFeel.h"

// Copied from Source/gui/sliders/WfsSliderBase.h with surgery for the plugin:
//   * removed `JuceHeader.h` include
//   * removed TTSManager dependency (accessibility announcements are a
//     main-app concern)
//   * replaced `ColorScheme::get()` with `wfs::plugin::DarkPalette`
// Kept in the global namespace to match the main app so subclass copies
// remain structurally identical.
class WfsSliderBase : public juce::Component,
                      public juce::SettableTooltipClient
{
public:
    enum class Orientation
    {
        horizontal,
        vertical
    };

    WfsSliderBase (float minValueIn, float maxValueIn, Orientation orientationIn)
        : minValue (minValueIn), maxValue (maxValueIn), orientation (orientationIn), value (minValueIn)
    {
        setRepaintsOnMouseActivity (false);
        setWantsKeyboardFocus (false);
        setFocusContainerType (FocusContainerType::none);
        setOpaque (false);
        setMouseClickGrabsKeyboardFocus (false);
    }

    void mouseEnter (const juce::MouseEvent&) override { isHovered = true;  repaint(); }
    void mouseExit  (const juce::MouseEvent&) override { isHovered = false; repaint(); }

    ~WfsSliderBase() override = default;

    void setValue (float newValue)
    {
        newValue = juce::jlimit (minValue, maxValue, newValue);
        if (! juce::approximatelyEqual (newValue, value))
        {
            value = newValue;
            valueChanged();
            repaint();
        }
    }

    float getValue() const noexcept { return value; }

    void setTrackThickness (float newThickness) noexcept { trackThickness = newThickness; }
    void setThumbRadius    (float newRadius)    noexcept { thumbRadius    = newRadius;    }

    void setTrackColours (juce::Colour backgroundColour, juce::Colour foregroundColour) noexcept
    {
        trackBackgroundColour = backgroundColour;
        trackForegroundColour = foregroundColour;
        repaint();
    }

    std::function<void(float)> onValueChanged;
    std::function<void()>      onGestureStart;
    std::function<void()>      onGestureEnd;

    void setThumbColour    (juce::Colour newThumbColour) noexcept { thumbColour = newThumbColour; repaint(); }
    void setDisabledAlpha  (float alpha) noexcept                 { disabledAlpha = alpha; }

    Orientation getOrientation() const noexcept { return orientation; }

    void resized() override
    {
        float ref = (orientation == Orientation::horizontal)
                    ? static_cast<float> (getHeight())
                    : static_cast<float> (getWidth());
        if (ref > 0.0f)
        {
            trackThickness = ref * 0.6f;
            thumbRadius    = ref * 0.2f;
        }
    }

protected:
    float getNormalizedValue() const noexcept { return normalizedFromValue (value); }

    juce::Rectangle<float> getUsableBounds (const juce::Rectangle<float>& totalBounds) const noexcept
    {
        auto usable = totalBounds.reduced (thumbRadius * 0.75f);
        return usable.isEmpty() ? totalBounds : usable;
    }

    juce::Rectangle<float> getTrackBounds (const juce::Rectangle<float>& usableBounds) const noexcept
    {
        if (orientation == Orientation::horizontal)
            return { usableBounds.getX(),
                     usableBounds.getCentreY() - trackThickness * 0.5f,
                     usableBounds.getWidth(),
                     trackThickness };
        return { usableBounds.getCentreX() - trackThickness * 0.5f,
                 usableBounds.getY(),
                 trackThickness,
                 usableBounds.getHeight() };
    }

    juce::Point<float> getThumbPosition (const juce::Rectangle<float>& usableBounds) const noexcept
    {
        const auto normalized = getNormalizedValue();
        if (orientation == Orientation::horizontal)
            return { usableBounds.getX() + normalized * usableBounds.getWidth(),
                     usableBounds.getCentreY() };
        return { usableBounds.getCentreX(),
                 usableBounds.getBottom() - normalized * usableBounds.getHeight() };
    }

    virtual void paintSlider (juce::Graphics& g, juce::Rectangle<float> bounds) = 0;

    virtual void valueChanged()
    {
        if (onValueChanged)
            onValueChanged (value);
    }

    virtual float valueFromNormalized (float normalizedPos) const
    {
        normalizedPos = juce::jlimit (0.0f, 1.0f, normalizedPos);
        return minValue + (maxValue - minValue) * normalizedPos;
    }

    virtual float normalizedFromValue (float currentValue) const
    {
        if (juce::approximatelyEqual (maxValue, minValue))
            return 0.0f;
        currentValue = juce::jlimit (minValue, maxValue, currentValue);
        return (currentValue - minValue) / (maxValue - minValue);
    }

    void drawThumbIndicator (juce::Graphics& g,
                             const juce::Rectangle<float>&,
                             const juce::Point<float>& thumbPos,
                             float alpha) const
    {
        const auto colour = juce::Colour (wfs::plugin::DarkPalette::sliderThumb).withAlpha (alpha);
        g.setColour (colour);
        const float lineThickness = juce::jmax (1.0f, trackThickness * 0.1f);
        const float lineLength    = trackThickness * 0.8f;

        if (orientation == Orientation::horizontal)
            g.drawLine (thumbPos.x, thumbPos.y - lineLength * 0.5f,
                        thumbPos.x, thumbPos.y + lineLength * 0.5f, lineThickness);
        else
            g.drawLine (thumbPos.x - lineLength * 0.5f, thumbPos.y,
                        thumbPos.x + lineLength * 0.5f, thumbPos.y, lineThickness);
    }

    juce::Colour trackBackgroundColour { juce::Colours::darkgrey };
    juce::Colour trackForegroundColour { juce::Colours::white };
    juce::Colour thumbColour            { juce::Colours::white };
    float disabledAlpha = 0.38f;

    float minValue = 0.0f;
    float maxValue = 1.0f;

    float trackThickness = 20.0f;
    float thumbRadius    = 8.0f;
    bool  isHovered      = false;

    virtual juce::Rectangle<float> getPointerBounds() const
    {
        return getLocalBounds().toFloat();
    }

    void handlePointer (juce::Point<float> pos)
    {
        auto bounds = getUsableBounds (getPointerBounds());
        float normalized = 0.0f;

        if (orientation == Orientation::horizontal)
        {
            if (bounds.getWidth() <= std::numeric_limits<float>::epsilon()) return;
            normalized = juce::jlimit (0.0f, 1.0f, (pos.x - bounds.getX()) / bounds.getWidth());
        }
        else
        {
            if (bounds.getHeight() <= std::numeric_limits<float>::epsilon()) return;
            normalized = juce::jlimit (0.0f, 1.0f, (bounds.getBottom() - pos.y) / bounds.getHeight());
        }

        if (normalized < 0.02f)      normalized = 0.0f;
        else if (normalized > 0.98f) normalized = 1.0f;

        setValue (valueFromNormalized (normalized));
    }

private:
    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        paintSlider (g, bounds);
    }

    void paintOverChildren (juce::Graphics&) override {}
    void lookAndFeelChanged() override { repaint(); }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (onGestureStart) onGestureStart();
        handlePointer (e.position);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        handlePointer (e.position);
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        handleMouseUp();
        if (onGestureEnd) onGestureEnd();
    }

    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override
    {
        if (onGestureStart) onGestureStart();
        auto increment = (maxValue - minValue) * 0.01f;
        setValue (value + wheel.deltaY * increment);
        if (onGestureEnd) onGestureEnd();
    }

    virtual void handleMouseUp() {}

protected:
    Orientation orientation = Orientation::horizontal;
    float value = 0.0f;
};
