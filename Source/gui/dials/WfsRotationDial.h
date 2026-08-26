#pragma once

#include <JuceHeader.h>
#include "../ColorScheme.h"
#include "../../Accessibility/TTSManager.h"

class WfsRotationDial : public juce::Component,
                        public juce::SettableTooltipClient
{
public:
    WfsRotationDial()
    {
        setWantsKeyboardFocus(false);
        setFocusContainerType(FocusContainerType::none);
        setOpaque(false); // Transparent background
        setMouseClickGrabsKeyboardFocus(false);
    }
    
    /** juce::Component::setEnabled(false) does NOT stop a plain Component from
        receiving mouse events — Component::hitTest never consults isEnabled()
        — and a Component with no LookAndFeel drawing does not dim either. So a
        disabled dial used to stay bright and fully draggable, which is worse
        than no disabling at all: the operator drags the one control that still
        looks live in a dead section and nothing happens. Wire both here so
        setEnabled() means what every caller already assumed it meant. */
    void enablementChanged() override
    {
        setInterceptsMouseClicks(isEnabled(), false);
        repaint();
    }

    void mouseEnter(const juce::MouseEvent&) override
    {
        // Override to prevent hover effects - do nothing
    }
    
    void mouseExit(const juce::MouseEvent&) override
    {
        // Override to prevent hover effects - do nothing
    }

    void setAngle(float degrees)
    {
        // Wrap into [-180, 180]: angles are circular, so out-of-range values
        // (typed or dragged past the ends) continue around instead of saturating.
        if (degrees > 180.0f || degrees < -180.0f)
        {
            degrees = std::fmod(degrees + 180.0f, 360.0f);
            if (degrees < 0.0f)
                degrees += 360.0f;
            degrees -= 180.0f;
        }
        if (!juce::approximatelyEqual(degrees, angleDegrees))
        {
            angleDegrees = degrees;
            if (onAngleChanged)
                onAngleChanged(angleDegrees);

            // TTS: Announce angle change for accessibility
            if (ttsParameterName.isNotEmpty())
            {
                juce::String valueStr = juce::String(static_cast<int>(angleDegrees)) + " degrees";
                TTSManager::getInstance().announceValueChange(ttsParameterName, valueStr);
            }

            repaint();
        }
    }
    float getAngle() const noexcept { return angleDegrees; }

    void setColours(juce::Colour background, juce::Colour indicator, juce::Colour tick)
    {
        backgroundColour = background;
        indicatorColour = indicator;
        tickColour = tick;
        repaint();
    }

    std::function<void(float)> onAngleChanged;

    // Gesture callbacks for undo transaction boundaries
    std::function<void()> onGestureStart;
    std::function<void()> onGestureEnd;

    /** Set parameter name for TTS announcements (e.g., "Rotation") */
    void setTTSParameterName(const juce::String& name) { ttsParameterName = name; }

    /** Configure TTS - unit is automatically "degrees" for rotation dials */
    void setTTSInfo(const juce::String& name) { ttsParameterName = name; }

    /** Alpha the dial paints at once setEnabled(false) has been called.
        Defaults to a visible dim; callers rarely need to change it. */
    void setDisabledAlpha(float alpha) noexcept
    {
        disabledAlpha = juce::jlimit(0.0f, 1.0f, alpha);
        repaint();
    }

    /** Opt in to the Map's plan-view reading: 0 degrees at the BOTTOM, positive
        counter-clockwise, so the indicator sits where the thing it controls
        sits on the Map.

        The default (0 at the top, positive clockwise) is what the phase and
        axis dials want — a phase dial has no plan view to agree with, and
        several of them reason about the default in their own layout code. Only
        a dial whose value IS a position on the Map should turn this on. */
    void setPlanViewMapping(bool shouldUsePlanView) noexcept
    {
        if (planViewMapping != shouldUsePlanView)
        {
            planViewMapping = shouldUsePlanView;
            repaint();
        }
    }

private:
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
        auto centre = bounds.getCentre();
        auto radius = size * 0.5f;

        // Background is transparent - no fill

        // Draw full circle track - use theme color with disabled alpha
        auto trackRadius = radius * 0.8f;
        auto trackWidth = radius * 0.12f;
        const float alpha = isEnabled() ? 1.0f : disabledAlpha;
        g.setColour(ColorScheme::get().buttonBorder.withAlpha(alpha));
        g.drawEllipse(juce::Rectangle<float>(
            centre.x - trackRadius, centre.y - trackRadius,
            trackRadius * 2.0f, trackRadius * 2.0f), trackWidth);

        // Draw indicator dot on the track (Android app style) - use theme color with disabled alpha
        auto dotRadius = trackWidth * 0.8f;

        // Plan view: (sin, +cos) with screen y growing downward puts 0 at the
        // bottom and +90 to the right — the same place the Map's stageToScreen
        // puts a bearing of 0 and 90 (audience at the bottom, stage right to
        // the right). Default: (cos, sin) of (angle - 90), i.e. 0 at the top
        // running clockwise.
        const auto t = juce::degreesToRadians(angleDegrees);
        juce::Point<float> dotPosition = planViewMapping
            ? juce::Point<float>(centre.x + trackRadius * std::sin(t),
                                 centre.y + trackRadius * std::cos(t))
            : juce::Point<float>(centre.x + trackRadius * std::cos(t - juce::MathConstants<float>::halfPi),
                                 centre.y + trackRadius * std::sin(t - juce::MathConstants<float>::halfPi));

        g.setColour(ColorScheme::get().sliderThumb.withAlpha(alpha));
        g.fillEllipse(dotPosition.x - dotRadius, dotPosition.y - dotRadius,
                      dotRadius * 2.0f, dotRadius * 2.0f);
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        if (onGestureStart) onGestureStart();
        dragStartAngleDegrees = angleDegrees;
        auto bounds = getLocalBounds().toFloat();
        auto centre = bounds.getCentre();
        auto deltaFromCentre = event.position - centre;
        dragStartAngle = std::atan2(deltaFromCentre.y, deltaFromCentre.x);
        accumulatedAngleChange = 0.0f;
    }

    void mouseDrag(const juce::MouseEvent& event) override
    {
        auto bounds = getLocalBounds().toFloat();
        auto centre = bounds.getCentre();
        auto deltaFromCentre = event.position - centre;
        auto currentAngle = std::atan2(deltaFromCentre.y, deltaFromCentre.x);
        
        // Calculate angular change (handle wrap-around)
        auto angleDelta = currentAngle - dragStartAngle;
        if (angleDelta > juce::MathConstants<float>::pi)
            angleDelta -= 2.0f * juce::MathConstants<float>::pi;
        else if (angleDelta < -juce::MathConstants<float>::pi)
            angleDelta += 2.0f * juce::MathConstants<float>::pi;
        
        // Accumulate angle change (convert radians to degrees). Screen atan2
        // grows clockwise; in plan view a growing ANGLE moves the dot
        // counter-clockwise, so the sign flips for the dot to keep following
        // the pointer.
        accumulatedAngleChange += (planViewMapping ? -1.0f : 1.0f) * juce::radiansToDegrees(angleDelta);
        dragStartAngle = currentAngle; // Update for next drag
        
        setAngle(dragStartAngleDegrees + accumulatedAngleChange);
    }

    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override
    {
        if (onGestureStart) onGestureStart();
        auto increment = 5.0f; // 5 degrees per step
        setAngle(angleDegrees + wheel.deltaY * increment);
        if (onGestureEnd) onGestureEnd();
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        if (onGestureEnd) onGestureEnd();
    }

    void paintOverChildren(juce::Graphics&) override
    {
        // Prevent JUCE from drawing default focus indicators
    }

    float angleDegrees = 0.0f;
    float disabledAlpha = 0.45f;  // painted alpha while !isEnabled()
    bool planViewMapping = false;  // see setPlanViewMapping

    // TTS accessibility
    juce::String ttsParameterName;

    juce::Colour backgroundColour { juce::Colours::black };
    juce::Colour indicatorColour { juce::Colours::white };
    juce::Colour tickColour { juce::Colours::grey };

    float dragStartAngleDegrees = 0.0f;
    float dragStartAngle = 0.0f; // In radians
    float accumulatedAngleChange = 0.0f; // In degrees
};
