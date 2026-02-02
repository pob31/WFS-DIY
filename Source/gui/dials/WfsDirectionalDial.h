#pragma once

#include <JuceHeader.h>
#include <cmath>
#include "../ColorScheme.h"
#include "../../Accessibility/TTSManager.h"

/**
 * WfsDirectionalDial
 *
 * A combined dial showing:
 * - Orientation (white needle)
 * - Angle On (green sector on opposite side - amplification zone)
 * - Angle Off (red sector on same side - no-amplification zone)
 * - Transition zone (orange area between)
 *
 * Mouse interaction:
 * - Drag: Change orientation
 * - Shift+Drag: Change Angle Off (red sector)
 * - Alt/Option+Drag: Change Angle On (green sector)
 * - Mouse wheel: Change orientation (5° increments)
 */
class WfsDirectionalDial : public juce::Component
{
public:
    WfsDirectionalDial()
    {
        setWantsKeyboardFocus(false);
        setFocusContainerType(FocusContainerType::none);
        setOpaque(false);
        setMouseClickGrabsKeyboardFocus(false);
    }

    // Setters
    void setOrientation(float degrees)
    {
        // Normalize to -180 to 180 range
        degrees = std::fmod(degrees + 180.0f, 360.0f);
        if (degrees < 0.0f) degrees += 360.0f;
        degrees -= 180.0f;

        if (!juce::approximatelyEqual(degrees, orientationDegrees))
        {
            orientationDegrees = degrees;
            if (onOrientationChanged)
                onOrientationChanged(orientationDegrees);
            repaint();
        }
    }

    void setAngleOn(int degrees)
    {
        degrees = juce::jlimit(1, 180, degrees);
        if (degrees != angleOnDegrees)
        {
            angleOnDegrees = degrees;
            if (onAngleOnChanged)
                onAngleOnChanged(angleOnDegrees);
            repaint();
        }
    }

    void setAngleOff(int degrees)
    {
        degrees = juce::jlimit(0, 179, degrees);
        if (degrees != angleOffDegrees)
        {
            angleOffDegrees = degrees;
            if (onAngleOffChanged)
                onAngleOffChanged(angleOffDegrees);
            repaint();
        }
    }

    // Getters
    float getOrientation() const noexcept { return orientationDegrees; }
    int getAngleOn() const noexcept { return angleOnDegrees; }
    int getAngleOff() const noexcept { return angleOffDegrees; }

    // Callbacks
    std::function<void(float)> onOrientationChanged;
    std::function<void(int)> onAngleOnChanged;
    std::function<void(int)> onAngleOffChanged;

    // TTS accessibility
    void setTTSParameterName(const juce::String& name) { ttsParameterName = name; }

private:
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);
        auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
        auto centre = bounds.getCentre();
        auto radius = size * 0.5f;

        // Create a square bounds centered in the component
        auto dialBounds = juce::Rectangle<float>(size, size).withCentre(centre);

        // Convert orientation to radians (0° = down, positive = clockwise)
        float orientationRad = juce::degreesToRadians(orientationDegrees);
        float angleOnRad = juce::degreesToRadians(static_cast<float>(angleOnDegrees));
        float angleOffRad = juce::degreesToRadians(static_cast<float>(angleOffDegrees));

        // For JUCE paths: 0 radians = 3 o'clock, positive angles go clockwise (screen coords)
        // Our visual: 0° = 6 o'clock (bottom), positive = clockwise
        // Red sector (off) = where needle points (speaker front)
        // Green sector (on) = opposite of needle (behind speaker, where sound transmits)
        // Negate angle to match rotation direction, no offset needed
        float juceOrientationRad = -orientationRad;

        // Staggered radii for visual distinction:
        // - Green (angle on): 100% = radius * 0.9
        // - Orange (transition) & Red (angle off): 90% of green = radius * 0.81
        auto greenBounds = dialBounds.reduced(radius * 0.1f);    // 100%
        auto innerBounds = dialBounds.reduced(radius * 0.19f);   // 90% of green (orange & red)

        // 1. Background circle (orange/transition zone)
        g.setColour(transitionColour);
        g.fillEllipse(innerBounds);

        // 2. Angle On sector (green) - centered on orientation direction (front of speaker)
        if (angleOnDegrees > 0)
        {
            juce::Path onPath;
            float onStart = juceOrientationRad - angleOnRad;
            float onEnd = juceOrientationRad + angleOnRad;
            onPath.addPieSegment(greenBounds, onStart, onEnd, 0.0f);
            g.setColour(angleOnColour);
            g.fillPath(onPath);
        }

        // 3. Angle Off sector (red) - centered on opposite of orientation (back of speaker)
        if (angleOffDegrees > 0)
        {
            juce::Path offPath;
            float backDirection = juceOrientationRad + juce::MathConstants<float>::pi;
            float offStart = backDirection - angleOffRad;
            float offEnd = backDirection + angleOffRad;
            offPath.addPieSegment(innerBounds, offStart, offEnd, 0.0f);
            g.setColour(angleOffColour);
            g.fillPath(offPath);
        }

        // 4. Center circle (dark, to hide pie segment centers)
        float innerRadius = radius * 0.15f;
        g.setColour(ColorScheme::get().background);
        g.fillEllipse(centre.x - innerRadius, centre.y - innerRadius,
                      innerRadius * 2.0f, innerRadius * 2.0f);

        // 5. Orientation needle (white line) - from center outward
        float needleLength = radius * 0.78f;
        // 0° = down (south), positive = clockwise (pointing right)
        float needleX = centre.x + needleLength * std::sin(orientationRad);
        float needleY = centre.y + needleLength * std::cos(orientationRad);

        g.setColour(ColorScheme::get().textPrimary);  // White in dark mode, black in light mode
        g.drawLine(centre.x, centre.y, needleX, needleY, 2.0f);

        // Draw a small circle at the tip of the needle
        float tipRadius = 3.0f;
        g.fillEllipse(needleX - tipRadius, needleY - tipRadius,
                      tipRadius * 2.0f, tipRadius * 2.0f);

        // 6. Outer ring/border
        g.setColour(ColorScheme::get().buttonBorder);
        g.drawEllipse(dialBounds.reduced(radius * 0.1f), 1.5f);
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        auto centre = getLocalBounds().toFloat().getCentre();
        auto delta = event.position - centre;
        dragStartMouseAngle = std::atan2(delta.x, delta.y); // 0 at bottom, clockwise positive

        // Determine which parameter we're adjusting based on modifiers
        isAdjustingAngleOff = event.mods.isShiftDown();
        isAdjustingAngleOn = event.mods.isAltDown();

        if (isAdjustingAngleOff)
            dragStartValue = static_cast<float>(angleOffDegrees);
        else if (isAdjustingAngleOn)
            dragStartValue = static_cast<float>(angleOnDegrees);
        else
            dragStartValue = orientationDegrees;

        accumulatedChange = 0.0f;
    }

    void mouseDrag(const juce::MouseEvent& event) override
    {
        auto centre = getLocalBounds().toFloat().getCentre();
        auto delta = event.position - centre;
        float currentMouseAngle = std::atan2(delta.x, delta.y);

        // Calculate angular change (handle wrap-around)
        float angleDelta = currentMouseAngle - dragStartMouseAngle;
        if (angleDelta > juce::MathConstants<float>::pi)
            angleDelta -= 2.0f * juce::MathConstants<float>::pi;
        else if (angleDelta < -juce::MathConstants<float>::pi)
            angleDelta += 2.0f * juce::MathConstants<float>::pi;

        accumulatedChange += juce::radiansToDegrees(angleDelta);
        dragStartMouseAngle = currentMouseAngle;

        if (isAdjustingAngleOff)
        {
            // For angle off, we use distance from center to control size
            float distance = delta.getDistanceFromOrigin();
            float maxDistance = juce::jmin(getWidth(), getHeight()) * 0.5f;
            int newAngleOff = static_cast<int>((distance / maxDistance) * 179.0f);
            setAngleOff(newAngleOff);
        }
        else if (isAdjustingAngleOn)
        {
            // For angle on, we use distance from center to control size
            float distance = delta.getDistanceFromOrigin();
            float maxDistance = juce::jmin(getWidth(), getHeight()) * 0.5f;
            int newAngleOn = static_cast<int>((distance / maxDistance) * 180.0f);
            if (newAngleOn < 1) newAngleOn = 1;
            setAngleOn(newAngleOn);
        }
        else
        {
            // Normal orientation drag
            setOrientation(dragStartValue + accumulatedChange);
        }
    }

    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override
    {
        float increment = 5.0f; // 5 degrees per step

        if (event.mods.isShiftDown())
        {
            int newAngleOff = angleOffDegrees + static_cast<int>(wheel.deltaY * increment);
            setAngleOff(newAngleOff);
        }
        else if (event.mods.isAltDown())
        {
            int newAngleOn = angleOnDegrees + static_cast<int>(wheel.deltaY * increment);
            setAngleOn(newAngleOn);
        }
        else
        {
            setOrientation(orientationDegrees + wheel.deltaY * increment);
        }
    }

    void mouseEnter(const juce::MouseEvent&) override {}
    void mouseExit(const juce::MouseEvent&) override {}
    void paintOverChildren(juce::Graphics&) override {}

    // State
    float orientationDegrees = 0.0f;   // -180 to +180, 0 = pointing down (toward audience)
    int angleOnDegrees = 86;           // 1 to 180
    int angleOffDegrees = 90;          // 0 to 179

    // Drag state
    float dragStartMouseAngle = 0.0f;
    float dragStartValue = 0.0f;
    float accumulatedChange = 0.0f;
    bool isAdjustingAngleOff = false;
    bool isAdjustingAngleOn = false;

    // Colors
    juce::Colour needleColour { juce::Colours::white };  // Not used - paint() uses ColorScheme directly
    juce::Colour angleOnColour { 0xFF4CAF50 };      // Green
    juce::Colour angleOffColour { 0xFFE53935 };     // Red
    juce::Colour transitionColour { 0xFFFF9800 };   // Orange

    // TTS
    juce::String ttsParameterName;
};
