#pragma once

#include <JuceHeader.h>
#include <limits>

// Shared interactive base class for custom JUCE sliders that mimics
// the bespoke Compose sliders from the Android app. It handles hit
// testing, mouse interaction and provides helpers for drawing so
// derived classes can focus on visual styling logic. Implemented
// inline so projects that haven't regenerated exporter files still
// get the new behaviour without linker steps.
class WfsSliderBase : public juce::Component
{
public:
    enum class Orientation
    {
        horizontal,
        vertical
    };

    WfsSliderBase(float minValueIn, float maxValueIn, Orientation orientationIn)
        : minValue(minValueIn), maxValue(maxValueIn), orientation(orientationIn), value(minValueIn)
    {
        setRepaintsOnMouseActivity(false); // Disable to prevent hover effects - mouseDrag will repaint manually
        setWantsKeyboardFocus(false);
        setFocusContainerType(FocusContainerType::none);
        setOpaque(true); // Opaque to prevent JUCE from drawing default backgrounds
        setMouseClickGrabsKeyboardFocus(false);
    }
    
    void mouseEnter(const juce::MouseEvent&) override
    {
        isHovered = true;
        repaint(); // Repaint to show brighter track
    }
    
    void mouseExit(const juce::MouseEvent&) override
    {
        isHovered = false;
        repaint(); // Repaint to restore normal track
    }

    ~WfsSliderBase() override = default;

    void setValue(float newValue)
    {
        newValue = juce::jlimit(minValue, maxValue, newValue);
        if (!juce::approximatelyEqual(newValue, value))
        {
            value = newValue;
            valueChanged();
            repaint();
        }
    }

    float getValue() const noexcept { return value; }

    void setTrackThickness(float newThickness) noexcept { trackThickness = newThickness; }
    void setThumbRadius(float newRadius) noexcept { thumbRadius = newRadius; }

    void setTrackColours(juce::Colour backgroundColour, juce::Colour foregroundColour) noexcept
    {
        trackBackgroundColour = backgroundColour;
        trackForegroundColour = foregroundColour;
        repaint();
    }

    // Public callback for value changes
    std::function<void(float)> onValueChanged;

    void setThumbColour(juce::Colour newThumbColour) noexcept
    {
        thumbColour = newThumbColour;
        repaint();
    }
    void setDisabledAlpha(float alpha) noexcept { disabledAlpha = alpha; }

    Orientation getOrientation() const noexcept { return orientation; }

protected:
    float getNormalizedValue() const noexcept
    {
        return normalizedFromValue(value);
    }

    juce::Rectangle<float> getUsableBounds(const juce::Rectangle<float>& totalBounds) const noexcept
    {
        auto usable = totalBounds.reduced(thumbRadius * 0.75f);
        if (usable.isEmpty())
            return totalBounds;
        return usable;
    }

    juce::Rectangle<float> getTrackBounds(const juce::Rectangle<float>& usableBounds) const noexcept
    {
        if (orientation == Orientation::horizontal)
        {
            return juce::Rectangle<float>(
                usableBounds.getX(),
                usableBounds.getCentreY() - trackThickness * 0.5f,
                usableBounds.getWidth(),
                trackThickness);
        }

        return juce::Rectangle<float>(
            usableBounds.getCentreX() - trackThickness * 0.5f,
            usableBounds.getY(),
            trackThickness,
            usableBounds.getHeight());
    }

    juce::Point<float> getThumbPosition(const juce::Rectangle<float>& usableBounds) const noexcept
    {
        const auto normalized = getNormalizedValue();

        if (orientation == Orientation::horizontal)
        {
            const auto x = usableBounds.getX() + normalized * usableBounds.getWidth();
            return { x, usableBounds.getCentreY() };
        }

        const auto y = usableBounds.getBottom() - normalized * usableBounds.getHeight();
        return { usableBounds.getCentreX(), y };
    }

    virtual void paintSlider(juce::Graphics& g, juce::Rectangle<float> bounds) = 0;
    virtual void valueChanged()
    {
        if (onValueChanged)
            onValueChanged(value);
    }

    virtual float valueFromNormalized(float normalizedPos) const
    {
        normalizedPos = juce::jlimit(0.0f, 1.0f, normalizedPos);
        return minValue + (maxValue - minValue) * normalizedPos;
    }

    virtual float normalizedFromValue(float currentValue) const
    {
        if (juce::approximatelyEqual(maxValue, minValue))
            return 0.0f;

        currentValue = juce::jlimit(minValue, maxValue, currentValue);
        return (currentValue - minValue) / (maxValue - minValue);
    }

    void drawThumbIndicator(juce::Graphics& g,
                            const juce::Rectangle<float>& /* track */,
                            const juce::Point<float>& thumbPos,
                            float alpha) const
    {
        // Draw thin white line thumb matching Android app design
        auto colour = thumbColour.withAlpha(alpha);
        g.setColour(colour);
        
        // Thumb line thickness (stroke width along track) - kept thin for clean look
        const float lineThickness = 2.0f;
        
        if (orientation == Orientation::horizontal)
        {
            // For horizontal sliders: vertical line (perpendicular to track)
            // Thumb width across track is 80% of track thickness (matching Android)
            const float lineLength = trackThickness * 0.8f;
            g.drawLine(thumbPos.x,
                      thumbPos.y - lineLength * 0.5f,
                      thumbPos.x,
                      thumbPos.y + lineLength * 0.5f,
                      lineThickness);
        }
        else
        {
            // For vertical sliders: horizontal line (perpendicular to track)
            // Thumb width across track is 80% of track thickness (matching Android)
            const float lineLength = trackThickness * 0.8f;
            g.drawLine(thumbPos.x - lineLength * 0.5f,
                      thumbPos.y,
                      thumbPos.x + lineLength * 0.5f,
                      thumbPos.y,
                      lineThickness);
        }
    }

    juce::Colour trackBackgroundColour { juce::Colours::darkgrey };
    juce::Colour trackForegroundColour { juce::Colours::white };
    juce::Colour thumbColour { juce::Colours::white };
    float disabledAlpha = 0.38f;  // Match Material Design disabled alpha

    float minValue = 0.0f;
    float maxValue = 1.0f;

    // Track thickness: dimension perpendicular to slider displacement
    // Thumb width will be 80% of track thickness automatically
    float trackThickness = 20.0f;  // Track width perpendicular to displacement
    float thumbRadius = 8.0f;     // Thumb hit test radius (line is drawn separately)
    bool isHovered = false; // Track hover state for brightening active track

private:
    void paint(juce::Graphics& g) override
    {
        // Always fill with black background to prevent any hover background from showing
        g.fillAll(juce::Colours::black);
        auto bounds = getLocalBounds().toFloat();
        paintSlider(g, bounds);
    }
    
    void paintOverChildren(juce::Graphics& /* g */) override
    {
        // Override to prevent JUCE from drawing default focus indicators
    }
    
    void lookAndFeelChanged() override
    {
        // Prevent default focus indicator drawing
        repaint();
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        handlePointer(e.position);
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        handlePointer(e.position);
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        handleMouseUp();
    }

    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override
    {
        auto increment = (maxValue - minValue) * 0.01f; // 1% of range per step
        setValue(value + wheel.deltaY * increment);
    }

    virtual void handleMouseUp() {}

    void handlePointer(juce::Point<float> pos)
    {
        auto bounds = getUsableBounds(getLocalBounds().toFloat());
        auto normalized = 0.0f;

        if (orientation == Orientation::horizontal)
        {
            if (bounds.getWidth() <= std::numeric_limits<float>::epsilon())
                return;
            normalized = juce::jlimit(0.0f, 1.0f, (pos.x - bounds.getX()) / bounds.getWidth());
        }
        else
        {
            if (bounds.getHeight() <= std::numeric_limits<float>::epsilon())
                return;
            normalized = juce::jlimit(0.0f, 1.0f, (bounds.getBottom() - pos.y) / bounds.getHeight());
        }

        setValue(valueFromNormalized(normalized));
    }

protected:
    Orientation orientation = Orientation::horizontal;
    float value = 0.0f;
};
