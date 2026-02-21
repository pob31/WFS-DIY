#pragma once

#include "WfsSliderBase.h"
#include <cmath>

class WfsAutoCenterSlider : public WfsSliderBase,
                            private juce::Timer
{
public:
    explicit WfsAutoCenterSlider(Orientation direction = Orientation::horizontal)
        : WfsSliderBase(-1.0f, 1.0f, direction)
    {
        setTrackColours(juce::Colour::fromRGB(32, 32, 32),
                        juce::Colour::fromRGB(255, 152, 0));
        setThumbColour(juce::Colours::white);
        // Track thickness is now set in base class to match Android design
        // Initialize at center (0)
        setValue(0.0f);
    }

    ~WfsAutoCenterSlider() override
    {
        stopTimer();
    }

    void setCenterValue(float newCenter)
    {
        centerValue = juce::jlimit(minValue, maxValue, newCenter);
        repaint();
    }

    float getCenterValue() const noexcept { return centerValue; }

    // Set the reporting interval for continuous polling (like joystick)
    void setReportingIntervalHz(double intervalHz)
    {
        reportingIntervalHz = juce::jlimit(1.0, 60.0, intervalHz);
    }

    // Callback for continuous position reporting (fires at reportingIntervalHz while dragging)
    std::function<void(float)> onPositionPolled;

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

        const auto centerNormalized = normalizedFromValue(centerValue);
        juce::Point<float> centerPoint;
        if (getOrientation() == Orientation::horizontal)
        {
            centerPoint = { track.getX() + centerNormalized * track.getWidth(), track.getCentreY() };
        }
        else
        {
            centerPoint = { track.getCentreX(), track.getBottom() - centerNormalized * track.getHeight() };
        }

        juce::Rectangle<float> active(track);
        if (getOrientation() == Orientation::horizontal)
        {
            const auto minX = juce::jmin(centerPoint.x, thumbPos.x);
            const auto width = juce::jmax(1.0f, std::abs(centerPoint.x - thumbPos.x));
            active.setX(minX);
            active.setWidth(width);
        }
        else
        {
            const auto minY = juce::jmin(centerPoint.y, thumbPos.y);
            const auto height = juce::jmax(1.0f, std::abs(centerPoint.y - thumbPos.y));
            active.setY(minY);
            active.setHeight(height);
        }

        // Brighten active track when hovering
        auto activeColour = isHovered ? trackForegroundColour.brighter(0.3f).withAlpha(alpha) : trackForegroundColour.withAlpha(alpha);
        g.setColour(activeColour);
        g.fillRect(active);

        // Centre marker
        g.setColour(trackForegroundColour.withMultipliedAlpha(0.35f));
        if (getOrientation() == Orientation::horizontal)
            g.fillRect(centerPoint.x - 1.0f, track.getY(), 2.0f, track.getHeight());
        else
            g.fillRect(track.getX(), centerPoint.y - 1.0f, track.getWidth(), 2.0f);

        drawThumbIndicator(g, track, thumbPos, alpha);
    }

private:
    void mouseDown(const juce::MouseEvent& e) override
    {
        handlePointer(e.position);  // Same as base class mouseDown
        // Start timer for continuous polling while dragging
        if (onPositionPolled != nullptr)
        {
            const auto intervalMs = juce::roundToInt(1000.0 / reportingIntervalHz);
            startTimer(intervalMs);
        }
    }

    void handleMouseUp() override
    {
        stopTimer();
        setValue(centerValue);
    }

    void timerCallback() override
    {
        if (onPositionPolled != nullptr)
            onPositionPolled(getValue());
    }

    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override
    {
        // Auto-center sliders don't respond to scroll wheel
    }

    float centerValue = 0.0f;
    double reportingIntervalHz = 50.0;  // Default 50Hz like joystick
};
