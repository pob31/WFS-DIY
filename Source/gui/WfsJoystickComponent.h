#pragma once

#include <JuceHeader.h>
#include "ColorScheme.h"

class WfsJoystickComponent : public juce::Component,
                             private juce::Timer
{
public:
    WfsJoystickComponent()
    {
        setRepaintsOnMouseActivity(false); // Disable to prevent hover background drawing
        setReportingIntervalHz(reportingIntervalHz);
        setWantsKeyboardFocus(false);
        setFocusContainerType(FocusContainerType::none);
        setOpaque(false); // Transparent background
        setMouseClickGrabsKeyboardFocus(false);
    }
    
    void mouseEnter(const juce::MouseEvent&) override
    {
        // Override to prevent hover effects - do nothing
    }
    
    void mouseExit(const juce::MouseEvent&) override
    {
        // Override to prevent hover effects - do nothing
    }

    ~WfsJoystickComponent() override
    {
        stopTimer();
    }

    void setOuterColour(juce::Colour colour) noexcept { outerColour = colour; repaint(); }
    void setThumbColour(juce::Colour colour) noexcept { thumbColour = colour; repaint(); }

    void setReportingIntervalHz(double intervalHz)
    {
        reportingIntervalHz = juce::jlimit(1.0, 60.0, intervalHz);
        const auto intervalMs = juce::roundToInt(1000.0 / reportingIntervalHz);
        startTimer(intervalMs);
    }

    void setOnPositionChanged(std::function<void(float, float)> callback)
    {
        onPositionChanged = std::move(callback);
    }

    std::pair<float, float> getCurrentPosition() const noexcept { return { normalizedPosition.x, normalizedPosition.y }; }

private:
    void paint(juce::Graphics& g) override
    {
        // Background is transparent - no fill

        static constexpr float thumbRatio = 0.33f;

        auto bounds = getLocalBounds().toFloat();
        const auto diameter = juce::jmin(bounds.getWidth(), bounds.getHeight());
        const auto radius = diameter * 0.5f;
        const auto center = bounds.getCentre();

        const auto circleBounds = juce::Rectangle<float>(
            center.x - radius,
            center.y - radius,
            diameter,
            diameter);

        // Outer circle fill and border - use theme colors
        auto borderColor = ColorScheme::get().buttonBorder;
        g.setColour(borderColor.darker(0.3f));
        g.fillEllipse(circleBounds);
        g.setColour(borderColor);
        g.drawEllipse(circleBounds, 2.0f);

        // Crosshairs - use theme color with transparency
        g.setColour(borderColor.withMultipliedAlpha(0.5f));
        g.drawLine(center.x, circleBounds.getY() + 6.0f, center.x, circleBounds.getBottom() - 6.0f, 1.0f);
        g.drawLine(circleBounds.getX() + 6.0f, center.y, circleBounds.getRight() - 6.0f, center.y, 1.0f);

        const auto thumbRadius = radius * thumbRatio;
        const auto thumbCentre = center + thumbOffset;

        // Thumb keeps its set color (typically accent orange)
        g.setColour(thumbColour.brighter(0.2f));
        g.fillEllipse({ thumbCentre.x - thumbRadius, thumbCentre.y - thumbRadius, thumbRadius * 2.0f, thumbRadius * 2.0f });
        g.setColour(thumbColour.darker(0.2f));
        g.drawEllipse({ thumbCentre.x - thumbRadius, thumbCentre.y - thumbRadius, thumbRadius * 2.0f, thumbRadius * 2.0f }, 1.5f);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        updateFromPointer(e.position);
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        updateFromPointer(e.position);
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        resetToCenter();
    }

    void timerCallback() override
    {
        if (onPositionChanged != nullptr)
            onPositionChanged(normalizedPosition.x, normalizedPosition.y);
    }

    void updateFromPointer(juce::Point<float> position)
    {
        static constexpr float thumbRatio = 0.33f;

        auto bounds = getLocalBounds().toFloat();
        const auto diameter = juce::jmin(bounds.getWidth(), bounds.getHeight());
        const auto radius = diameter * 0.5f;
        const auto thumbRadius = radius * thumbRatio;
        const auto maxThumbDistance = juce::jmax(0.0f, radius - thumbRadius);
        const auto centre = bounds.getCentre();

        auto relative = position - centre;
        const auto distance = relative.getDistanceFromOrigin();

        if (distance > maxThumbDistance && distance > 0.0f)
            relative = relative / distance * maxThumbDistance;

        thumbOffset = relative;

        if (maxThumbDistance > 0.0f)
        {
            normalizedPosition.x = juce::jlimit(-1.0f, 1.0f, relative.x / maxThumbDistance);
            normalizedPosition.y = juce::jlimit(-1.0f, 1.0f, -relative.y / maxThumbDistance);
        }
        else
        {
            normalizedPosition = { 0.0f, 0.0f };
        }

        repaint();
    }

    void resetToCenter()
    {
        thumbOffset = { 0.0f, 0.0f };
        normalizedPosition = { 0.0f, 0.0f };
        repaint();
    }
    
    void paintOverChildren(juce::Graphics&) override
    {
        // Prevent JUCE from drawing default focus indicators
    }

    juce::Colour outerColour { juce::Colours::lightgrey };
    juce::Colour thumbColour { juce::Colours::darkgrey };
    juce::Point<float> thumbOffset;
    juce::Point<float> normalizedPosition { 0.0f, 0.0f };
    std::function<void(float, float)> onPositionChanged;
    double reportingIntervalHz = 10.0; // ~100ms
};
