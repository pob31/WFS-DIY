#pragma once

#include <JuceHeader.h>
#include "../ColorScheme.h"

/**
 * LongPressButton
 *
 * A TextButton that requires a sustained press to trigger its action.
 * Visual feedback: blue progress bar fills left-to-right, turns green when ready.
 * Short taps are ignored — prevents accidental destructive actions.
 */
class LongPressButton : public juce::TextButton,
                        private juce::Timer
{
public:
    explicit LongPressButton(int durationMs = 1000)
        : longPressDurationMs(durationMs)
    {
    }

    void setBaseColour(juce::Colour colour)
    {
        customBaseColour = colour;
        hasCustomBaseColour = true;
        repaint();
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (!isEnabled()) { TextButton::mouseDown(e); return; }

        if (e.mods.isLeftButtonDown())
        {
            pressStartTime = juce::Time::getCurrentTime();
            isLongPressActive = true;
            thresholdReached = false;
            startTimer(50);
        }
        TextButton::mouseDown(e);
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        stopTimer();

        if (thresholdReached && isLongPressActive && contains(e.getPosition()))
        {
            if (onLongPress)
                onLongPress();
        }

        isLongPressActive = false;
        thresholdReached = false;
        repaint();
        TextButton::mouseUp(e);
    }

    void mouseExit(const juce::MouseEvent& e) override
    {
        if (isLongPressActive)
        {
            stopTimer();
            isLongPressActive = false;
            thresholdReached = false;
            repaint();
        }
        TextButton::mouseExit(e);
    }

    std::function<void()> onLongPress;

private:
    void timerCallback() override
    {
        if (isLongPressActive && !thresholdReached)
        {
            auto elapsed = (juce::Time::getCurrentTime() - pressStartTime).inMilliseconds();
            if (elapsed >= longPressDurationMs)
            {
                thresholdReached = true;
                stopTimer();
            }
        }
        repaint();
    }

    void paintButton(juce::Graphics& g, bool shouldHighlight, bool shouldBeDown) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(1.0f);

        // Background
        if (!isEnabled())
        {
            auto baseCol = hasCustomBaseColour ? customBaseColour : ColorScheme::get().buttonNormal;
            g.setColour(baseCol.withAlpha(0.4f));
        }
        else if (shouldBeDown)
            g.setColour(hasCustomBaseColour ? customBaseColour.darker(0.2f) : ColorScheme::get().buttonPressed);
        else if (shouldHighlight)
            g.setColour(hasCustomBaseColour ? customBaseColour.brighter(0.1f) : ColorScheme::get().buttonHover);
        else
            g.setColour(hasCustomBaseColour ? customBaseColour : ColorScheme::get().buttonNormal);

        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(ColorScheme::get().buttonBorder);
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

        // Progress indicator during long press (fills from left to right)
        if (isLongPressActive && !thresholdReached)
        {
            auto elapsed = (juce::Time::getCurrentTime() - pressStartTime).inMilliseconds();
            float progress = juce::jlimit(0.0f, 1.0f,
                static_cast<float>(elapsed) / static_cast<float>(longPressDurationMs));

            g.setColour(ColorScheme::get().accentBlue.withAlpha(0.5f));
            auto progressBounds = bounds;
            progressBounds = progressBounds.removeFromLeft(bounds.getWidth() * progress);
            g.fillRoundedRectangle(progressBounds, 4.0f);
        }

        // Show green when threshold reached (ready to release)
        if (thresholdReached && isLongPressActive)
        {
            g.setColour(ColorScheme::get().accentGreen.withAlpha(0.5f));
            g.fillRoundedRectangle(bounds, 4.0f);
        }

        // Text
        g.setColour(isEnabled() ? ColorScheme::get().textPrimary : ColorScheme::get().textDisabled);
        g.setFont(juce::FontOptions(14.0f));
        g.drawText(getButtonText(), bounds, juce::Justification::centred);
    }

    const int longPressDurationMs;
    juce::Time pressStartTime;
    bool isLongPressActive = false;
    bool thresholdReached = false;
    juce::Colour customBaseColour;
    bool hasCustomBaseColour = false;
};
