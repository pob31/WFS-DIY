#pragma once

#include <JuceHeader.h>
#include "../ColorScheme.h"

/**
    The three transport buttons of an AutomOtion block: Play, Stop, Pause,
    drawn as icons in the theme's button colours. They were private to the
    Inputs tab until the Effects tab's Movements sub-tab needed the same three;
    moved here unchanged so both tabs draw the same transport.
*/

//==============================================================================
// Custom Transport Button - Play (right-pointing triangle)
class PlayButton : public juce::Button
{
public:
    PlayButton() : juce::Button("Play") {}

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);

        // Background - use theme colors
        if (shouldDrawButtonAsDown)
            g.setColour(ColorScheme::get().buttonPressed);
        else if (shouldDrawButtonAsHighlighted)
            g.setColour(ColorScheme::get().buttonHover);
        else
            g.setColour(ColorScheme::get().buttonNormal);

        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(ColorScheme::get().buttonBorder);
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

        // Draw play triangle
        auto iconBounds = bounds.reduced(10.0f);
        juce::Path triangle;
        triangle.addTriangle(
            iconBounds.getX(), iconBounds.getY(),
            iconBounds.getX(), iconBounds.getBottom(),
            iconBounds.getRight(), iconBounds.getCentreY());

        g.setColour(ColorScheme::get().textPrimary);
        g.fillPath(triangle);
    }
};

//==============================================================================
// Custom Transport Button - Stop (square)
class StopButton : public juce::Button
{
public:
    StopButton() : juce::Button("Stop") {}

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);

        // Background - use theme colors
        if (shouldDrawButtonAsDown)
            g.setColour(ColorScheme::get().buttonPressed);
        else if (shouldDrawButtonAsHighlighted)
            g.setColour(ColorScheme::get().buttonHover);
        else
            g.setColour(ColorScheme::get().buttonNormal);

        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(ColorScheme::get().buttonBorder);
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

        // Draw stop square
        auto iconBounds = bounds.reduced(10.0f);
        g.setColour(ColorScheme::get().textPrimary);
        g.fillRect(iconBounds);
    }
};

//==============================================================================
// Custom Transport Button - Pause (two vertical bars)
class PauseButton : public juce::Button
{
public:
    PauseButton() : juce::Button("Pause") {}

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);

        // Background - use theme colors, toggle state affects color
        if (shouldDrawButtonAsDown || getToggleState())
            g.setColour(ColorScheme::get().buttonPressed);
        else if (shouldDrawButtonAsHighlighted)
            g.setColour(ColorScheme::get().buttonHover);
        else
            g.setColour(ColorScheme::get().buttonNormal);

        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(ColorScheme::get().buttonBorder);
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

        // Draw pause bars (two vertical rectangles)
        auto iconBounds = bounds.reduced(10.0f);
        float barWidth = iconBounds.getWidth() * 0.3f;
        float gap = iconBounds.getWidth() * 0.4f;

        g.setColour(ColorScheme::get().textPrimary);
        g.fillRect(iconBounds.getX(), iconBounds.getY(), barWidth, iconBounds.getHeight());
        g.fillRect(iconBounds.getX() + barWidth + gap, iconBounds.getY(), barWidth, iconBounds.getHeight());
    }
};
