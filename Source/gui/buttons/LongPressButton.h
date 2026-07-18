#pragma once

#include <JuceHeader.h>
#include "../ColorScheme.h"
#include "../WfsLookAndFeel.h"

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
        hasCustomBaseColour = colour.getAlpha() > 0;
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

protected:
    int getEffectiveDuration() const
    {
        return juce::jmax(1, static_cast<int>(longPressDurationMs * durationMultiplier));
    }

private:
    void timerCallback() override
    {
        if (isLongPressActive && !thresholdReached)
        {
            auto elapsed = (juce::Time::getCurrentTime() - pressStartTime).inMilliseconds();
            if (elapsed >= getEffectiveDuration())
            {
                thresholdReached = true;
                stopTimer();
            }
        }
        repaint();
    }

    void paintButton(juce::Graphics& g, bool shouldHighlight, bool shouldBeDown) override
    {
        auto bounds = getLocalBounds().toFloat();
        bounds.removeFromLeft(6.0f);
        bounds.removeFromRight(6.0f);

        // Background — always use theme button colors (slightly darker for footer buttons)
        juce::Colour bgColour;
        if (!isEnabled())
            bgColour = ColorScheme::get().buttonNormal.withAlpha(0.4f);
        else if (shouldBeDown)
            bgColour = hasCustomBaseColour ? ColorScheme::get().buttonPressed.darker(0.15f) : ColorScheme::get().buttonPressed;
        else if (shouldHighlight)
            bgColour = hasCustomBaseColour ? ColorScheme::get().buttonHover.darker(0.15f) : ColorScheme::get().buttonHover;
        else
            bgColour = hasCustomBaseColour ? ColorScheme::get().buttonNormal.darker(0.15f) : ColorScheme::get().buttonNormal;

        g.setColour(bgColour);
        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(ColorScheme::get().buttonBorder);
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

        // Long-press affordance: a right-pointing triangle on the left edge — a
        // lighter shade on dark buttons, a darker shade on light buttons — so a
        // long-press button reads as distinct from an ordinary one at a glance.
        {
            bool darkBg = bgColour.getBrightness() < 0.5f;
            auto triColour = darkBg ? bgColour.brighter(0.5f) : bgColour.darker(0.35f);
            g.setColour(isEnabled() ? triColour : triColour.withAlpha(0.4f));

            float triH = juce::jmin(9.0f, bounds.getHeight() * 0.4f);
            float triW = triH * 0.72f;
            float cx = bounds.getX() + 7.0f;
            float cy = bounds.getCentreY();
            juce::Path tri;
            tri.startNewSubPath(cx, cy - triH * 0.5f);
            tri.lineTo(cx + triW, cy);
            tri.lineTo(cx, cy + triH * 0.5f);
            tri.closeSubPath();
            g.fillPath(tri);
        }

        // Progress indicator during long press (fills from left to right)
        if (isLongPressActive && !thresholdReached)
        {
            auto elapsed = (juce::Time::getCurrentTime() - pressStartTime).inMilliseconds();
            float progress = juce::jlimit(0.0f, 1.0f,
                static_cast<float>(elapsed) / static_cast<float>(getEffectiveDuration()));

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

        // Text — use accent colour when set, brightened/darkened for theme contrast
        if (!isEnabled())
            g.setColour(ColorScheme::get().textDisabled);
        else if (hasCustomBaseColour)
        {
            bool isDarkTheme = ColorScheme::get().background.getBrightness() < 0.5f;
            g.setColour(isDarkTheme ? customBaseColour.brighter(0.5f) : customBaseColour.darker(0.2f));
        }
        else
            g.setColour(ColorScheme::get().textPrimary);
        auto fontSize = hasCustomBaseColour ? juce::jmax(12.0f, 16.0f * WfsLookAndFeel::uiScale)
                                            : juce::jmax(10.0f, 14.0f * WfsLookAndFeel::uiScale);
        auto font = juce::Font(juce::FontOptions(fontSize));
        g.setFont(hasCustomBaseColour ? font.boldened() : font);
        g.drawText(getButtonText(), bounds, juce::Justification::centred);
    }

    juce::Colour customBaseColour;
    bool hasCustomBaseColour = false;

    static inline float durationMultiplier = 1.0f;

protected:
    int longPressDurationMs;
    juce::Time pressStartTime;
    bool isLongPressActive = false;
    bool thresholdReached = false;

public:
    static void setShortMode(bool enabled) { durationMultiplier = enabled ? 0.5f : 1.0f; }
    static bool isShortMode() { return durationMultiplier < 1.0f; }
};
