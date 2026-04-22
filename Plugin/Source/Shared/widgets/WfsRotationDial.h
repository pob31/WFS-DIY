#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>
#include "../PluginLookAndFeel.h"

// Copied from Source/gui/dials/WfsRotationDial.h with surgery:
//   * removed JuceHeader.h include
//   * removed TTSManager dependency
//   * replaced ColorScheme::get() calls with plugin DarkPalette
class WfsRotationDial : public juce::Component,
                        public juce::SettableTooltipClient
{
public:
    WfsRotationDial()
    {
        setWantsKeyboardFocus (false);
        setFocusContainerType (FocusContainerType::none);
        setOpaque (false);
        setMouseClickGrabsKeyboardFocus (false);
    }

    void mouseEnter (const juce::MouseEvent&) override {}
    void mouseExit  (const juce::MouseEvent&) override {}

    void setAngle (float degrees)
    {
        degrees = juce::jlimit (-180.0f, 180.0f, degrees);
        if (! juce::approximatelyEqual (degrees, angleDegrees))
        {
            angleDegrees = degrees;
            if (onAngleChanged)
                onAngleChanged (angleDegrees);
            repaint();
        }
    }
    float getAngle() const noexcept { return angleDegrees; }

    void setColours (juce::Colour background, juce::Colour indicator, juce::Colour tick)
    {
        backgroundColour = background;
        indicatorColour  = indicator;
        tickColour       = tick;
        repaint();
    }

    std::function<void(float)> onAngleChanged;
    std::function<void()>      onGestureStart;
    std::function<void()>      onGestureEnd;

    void setDisabledAlpha (float alpha) noexcept
    {
        disabledAlpha = juce::jlimit (0.0f, 1.0f, alpha);
    }

private:
    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        auto size   = juce::jmin (bounds.getWidth(), bounds.getHeight());
        auto centre = bounds.getCentre();
        auto radius = size * 0.5f;

        auto trackRadius = radius * 0.8f;
        auto trackWidth  = radius * 0.12f;
        g.setColour (juce::Colour (wfs::plugin::DarkPalette::buttonBorder).withAlpha (disabledAlpha));
        g.drawEllipse (juce::Rectangle<float> (centre.x - trackRadius, centre.y - trackRadius,
                                               trackRadius * 2.0f, trackRadius * 2.0f),
                       trackWidth);

        const auto angleRad = juce::degreesToRadians (angleDegrees - 90.0f);
        const auto dotRadius = trackWidth * 0.8f;
        const juce::Point<float> dotPosition (centre.x + trackRadius * std::cos (angleRad),
                                              centre.y + trackRadius * std::sin (angleRad));

        g.setColour (juce::Colour (wfs::plugin::DarkPalette::sliderThumb).withAlpha (disabledAlpha));
        g.fillEllipse (dotPosition.x - dotRadius, dotPosition.y - dotRadius,
                       dotRadius * 2.0f, dotRadius * 2.0f);
    }

    void mouseDown (const juce::MouseEvent& event) override
    {
        if (onGestureStart) onGestureStart();
        dragStartAngleDegrees = angleDegrees;
        const auto centre = getLocalBounds().toFloat().getCentre();
        const auto delta  = event.position - centre;
        dragStartAngle = std::atan2 (delta.y, delta.x);
        accumulatedAngleChange = 0.0f;
    }

    void mouseDrag (const juce::MouseEvent& event) override
    {
        const auto centre = getLocalBounds().toFloat().getCentre();
        const auto delta  = event.position - centre;
        const auto currentAngle = std::atan2 (delta.y, delta.x);

        auto angleDelta = currentAngle - dragStartAngle;
        if      (angleDelta >  juce::MathConstants<float>::pi) angleDelta -= 2.0f * juce::MathConstants<float>::pi;
        else if (angleDelta < -juce::MathConstants<float>::pi) angleDelta += 2.0f * juce::MathConstants<float>::pi;

        accumulatedAngleChange += juce::radiansToDegrees (angleDelta);
        dragStartAngle = currentAngle;

        setAngle (dragStartAngleDegrees + accumulatedAngleChange);
    }

    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override
    {
        if (onGestureStart) onGestureStart();
        const auto increment = 5.0f;
        setAngle (angleDegrees + wheel.deltaY * increment);
        if (onGestureEnd) onGestureEnd();
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (onGestureEnd) onGestureEnd();
    }

    void paintOverChildren (juce::Graphics&) override {}

    float angleDegrees    = 0.0f;
    float disabledAlpha   = 1.0f;

    juce::Colour backgroundColour { juce::Colours::black };
    juce::Colour indicatorColour  { juce::Colours::white };
    juce::Colour tickColour       { juce::Colours::grey };

    float dragStartAngleDegrees = 0.0f;
    float dragStartAngle        = 0.0f;
    float accumulatedAngleChange = 0.0f;
};
