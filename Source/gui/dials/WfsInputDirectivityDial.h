#pragma once

#include <JuceHeader.h>
#include <cmath>
#include "../ColorScheme.h"

/**
 * WfsInputDirectivityDial
 *
 * A dial showing:
 * - Rotation (red needle pointing in source direction)
 * - Directivity (white sector showing coverage area)
 * - Remainder (grey sector at smaller radius, with optional HF shelf modulation)
 *
 * Coordinate system:
 * - 0° = pointing down (toward audience)
 * - Positive angles = clockwise
 * - 90° = pointing left, 180° = pointing up
 *
 * Mouse interaction:
 * - Drag: Change rotation
 * - Mouse wheel: Change rotation (5° increments)
 */
class WfsInputDirectivityDial : public juce::Component
{
public:
    WfsInputDirectivityDial()
    {
        setWantsKeyboardFocus(false);
        setFocusContainerType(FocusContainerType::none);
        setOpaque(false);
        setMouseClickGrabsKeyboardFocus(false);
    }

    // Setters
    void setRotation(float degrees)
    {
        // Normalize to -180 to 180 range
        degrees = std::fmod(degrees + 180.0f, 360.0f);
        if (degrees < 0.0f) degrees += 360.0f;
        degrees -= 180.0f;

        if (!juce::approximatelyEqual(degrees, rotationDegrees))
        {
            rotationDegrees = degrees;
            if (onRotationChanged)
                onRotationChanged(rotationDegrees);
            repaint();
        }
    }

    void setDirectivity(float degrees)
    {
        degrees = juce::jlimit(0.0f, 360.0f, degrees);
        if (!juce::approximatelyEqual(degrees, directivityDegrees))
        {
            directivityDegrees = degrees;
            repaint();
        }
    }

    void setHfShelf(float dB)
    {
        dB = juce::jlimit(-24.0f, 6.0f, dB);
        if (!juce::approximatelyEqual(dB, hfShelfDb))
        {
            hfShelfDb = dB;
            repaint();
        }
    }

    // Getters
    float getRotation() const noexcept { return rotationDegrees; }
    float getDirectivity() const noexcept { return directivityDegrees; }
    float getHfShelf() const noexcept { return hfShelfDb; }

    // Callbacks
    std::function<void(float)> onRotationChanged;

private:
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);
        auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
        auto centre = bounds.getCentre();
        auto radius = size * 0.5f;

        auto dialBounds = juce::Rectangle<float>(size, size).withCentre(centre);

        // Convert to radians
        float rotationRad = juce::degreesToRadians(rotationDegrees);
        float directivityRad = juce::degreesToRadians(directivityDegrees * 0.5f); // Half-angle

        // The directivity sector is centered on the rotation direction
        // In our coordinate system: 0° = down, positive = clockwise
        // For JUCE paths: we use the same conversion as WfsDirectionalDial
        float juceRotationRad = rotationRad;

        // Calculate radii
        float outerRadius = radius * 0.9f;
        float whiteRadius = outerRadius;

        // HF shelf modulation: 0 dB = same as white radius, -24 dB = half the white radius
        // Map from [-24, 0] to [0.5, 1.0] factor (clamp positive values to 1.0)
        float hfFactor = juce::jmap(juce::jlimit(-24.0f, 0.0f, hfShelfDb), -24.0f, 0.0f, 0.5f, 1.0f);
        float minGreyRadius = whiteRadius * hfFactor;

        // 1. Draw outer ring/border first
        g.setColour(ColorScheme::get().buttonBorder);
        g.drawEllipse(dialBounds.reduced(radius * 0.1f), 1.5f);

        // 2. Fill background with grey (the "off" zone)
        // Draw grey with variable radius using a custom path for the cosine curve effect
        if (directivityDegrees < 360.0f)
        {
            // Draw grey sector (remainder) with cosine-modulated radius
            juce::Path greyPath;

            // The grey sector covers the area NOT covered by directivity
            float greyHalfAngle = juce::MathConstants<float>::pi - directivityRad;
            float greyCenter = juceRotationRad + juce::MathConstants<float>::pi; // Opposite to rotation

            if (greyHalfAngle > 0.01f)
            {
                // Create path with varying radius following cosine curve
                int numSegments = 60;
                float startAngle = greyCenter - greyHalfAngle;
                float endAngle = greyCenter + greyHalfAngle;
                float angleStep = (endAngle - startAngle) / numSegments;

                greyPath.startNewSubPath(centre);

                for (int i = 0; i <= numSegments; ++i)
                {
                    float angle = startAngle + i * angleStep;

                    // Calculate distance from edge of white sector (0 at edge, 1 at center of grey)
                    float distFromEdge = std::abs(angle - greyCenter) / greyHalfAngle;
                    distFromEdge = 1.0f - distFromEdge; // 0 at edge, 1 at center

                    // Cosine curve: radius varies from whiteRadius at edges to minGreyRadius at center
                    float cosineWeight = 0.5f * (1.0f - std::cos(distFromEdge * juce::MathConstants<float>::pi));
                    float currentRadius = whiteRadius - (whiteRadius - minGreyRadius) * cosineWeight;

                    // Calculate position (using same coordinate system as needle)
                    // Positive angles point right (clockwise from above)
                    float x = centre.x + currentRadius * std::sin(angle);
                    float y = centre.y + currentRadius * std::cos(angle);

                    greyPath.lineTo(x, y);
                }

                greyPath.closeSubPath();

                g.setColour(greyColour);
                g.fillPath(greyPath);
            }
        }

        // 3. Draw white directivity sector
        if (directivityDegrees > 0.0f)
        {
            juce::Path whitePath;

            if (directivityDegrees >= 360.0f)
            {
                // Full circle
                whitePath.addEllipse(dialBounds.reduced(radius * 0.1f));
            }
            else
            {
                // Sector centered on rotation direction
                int numSegments = 60;
                float startAngle = juceRotationRad - directivityRad;
                float endAngle = juceRotationRad + directivityRad;
                float angleStep = (endAngle - startAngle) / numSegments;

                whitePath.startNewSubPath(centre);

                for (int i = 0; i <= numSegments; ++i)
                {
                    float angle = startAngle + i * angleStep;
                    float x = centre.x + whiteRadius * std::sin(angle);
                    float y = centre.y + whiteRadius * std::cos(angle);
                    whitePath.lineTo(x, y);
                }

                whitePath.closeSubPath();
            }

            // Use white in dark mode, light blue in light mode (so it's visible against white background)
            auto bgLuminance = ColorScheme::get().background.getBrightness();
            auto sectorColour = (bgLuminance > 0.5f) ? juce::Colour(0xFFE3F2FD) : whiteColour;  // Light blue vs white
            g.setColour(sectorColour);
            g.fillPath(whitePath);
        }

        // 4. Center circle (dark)
        float innerRadius = radius * 0.12f;
        g.setColour(ColorScheme::get().background);
        g.fillEllipse(centre.x - innerRadius, centre.y - innerRadius,
                      innerRadius * 2.0f, innerRadius * 2.0f);

        // 5. Rotation needle (red line) - pointing in rotation direction
        // Positive angles point right (clockwise from above)
        float needleLength = radius * 0.85f;
        float needleX = centre.x + needleLength * std::sin(rotationRad);
        float needleY = centre.y + needleLength * std::cos(rotationRad);

        g.setColour(needleColour);
        g.drawLine(centre.x, centre.y, needleX, needleY, 2.5f);

        // Draw a small circle at the tip of the needle
        float tipRadius = 4.0f;
        g.fillEllipse(needleX - tipRadius, needleY - tipRadius,
                      tipRadius * 2.0f, tipRadius * 2.0f);
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        auto centre = getLocalBounds().toFloat().getCentre();
        auto delta = event.position - centre;
        dragStartMouseAngle = std::atan2(delta.x, delta.y); // 0 at bottom, clockwise positive
        dragStartValue = rotationDegrees;
        accumulatedChange = 0.0f;
    }

    void mouseDrag(const juce::MouseEvent& event) override
    {
        auto centre = getLocalBounds().toFloat().getCentre();
        auto delta = event.position - centre;
        float currentMouseAngle = std::atan2(delta.x, delta.y);

        float angleDelta = currentMouseAngle - dragStartMouseAngle;
        if (angleDelta > juce::MathConstants<float>::pi)
            angleDelta -= 2.0f * juce::MathConstants<float>::pi;
        else if (angleDelta < -juce::MathConstants<float>::pi)
            angleDelta += 2.0f * juce::MathConstants<float>::pi;

        accumulatedChange += juce::radiansToDegrees(angleDelta);
        dragStartMouseAngle = currentMouseAngle;

        setRotation(dragStartValue + accumulatedChange);
    }

    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override
    {
        float increment = 5.0f;
        setRotation(rotationDegrees + wheel.deltaY * increment);
    }

    void mouseEnter(const juce::MouseEvent&) override {}
    void mouseExit(const juce::MouseEvent&) override {}

    // State
    float rotationDegrees = 0.0f;      // -180 to +180
    float directivityDegrees = 360.0f; // 0 to 360
    float hfShelfDb = 0.0f;            // -24 to +6 dB

    // Drag state
    float dragStartMouseAngle = 0.0f;
    float dragStartValue = 0.0f;
    float accumulatedChange = 0.0f;

    // Colors
    juce::Colour needleColour { 0xFFE53935 };     // Red
    juce::Colour whiteColour { 0xFFFFFFFF };      // White (directivity zone)
    juce::Colour greyColour { 0xFF707070 };       // Grey (off zone)
};
