#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/** Toggle TextButton with an optional "depleting fill" overlay that
    visualises a time-remaining countdown. When `setCountdown(remaining,
    total, colour)` is called with non-zero values, the right edge of
    the button is overpainted with the LookAndFeel's normal background
    colour so the toggled-on tint appears to drain from right to left
    as `remaining` decreases. The text is repainted on top so it stays
    readable through the overlay.

    Used for the AI critical-actions toggle on the Network tab — when
    the operator allows critical actions, the 60 s auto-block window is
    shown as a red fill that drains away. */
class CountdownTextButton : public juce::TextButton
{
public:
    CountdownTextButton() = default;

    /** `remainingSeconds` and `totalSeconds` together set the depletion
        ratio (remaining / total). Pass 0 / 0 to clear the overlay. The
        `drainColour` is what the overpaint uses to "consume" the
        toggled-on tint — typically the same as the button's normal
        (off-state) background. */
    void setCountdown (int remainingSeconds, int totalSeconds, juce::Colour drainColour)
    {
        countdownRemaining = juce::jmax (0, remainingSeconds);
        countdownTotal     = juce::jmax (0, totalSeconds);
        drainColourCache   = drainColour;
        repaint();
    }

    /** Convenience: hide the overlay. */
    void clearCountdown()
    {
        setCountdown (0, 0, juce::Colours::transparentBlack);
    }

    void paintButton (juce::Graphics& g,
                      bool shouldDrawAsHighlighted,
                      bool shouldDrawAsDown) override
    {
        // Standard button render (background + text).
        juce::TextButton::paintButton (g, shouldDrawAsHighlighted, shouldDrawAsDown);

        if (countdownTotal <= 0 || ! getToggleState())
            return;

        // Cap the ratio at [0, 1]; remaining > total can happen briefly
        // on a fresh open() if the timer fires between state changes.
        const float ratio = juce::jlimit (0.0f, 1.0f,
                                          static_cast<float> (countdownRemaining)
                                            / static_cast<float> (countdownTotal));

        // WfsLookAndFeel insets the button background by `kCornerInset`
        // on the left AND right and fills a rounded rectangle inside
        // those bounds. We need to match that inset exactly, otherwise
        // our flat overlay paints into the gutter outside the visual
        // button — which looks like a "progress bar that overshoots
        // the button". Clipping to a rounded path also keeps the right-
        // side corners consistent with the rest of the button.
        auto visibleBounds = getLocalBounds().toFloat();
        visibleBounds.removeFromLeft  (kCornerInset);
        visibleBounds.removeFromRight (kCornerInset);

        const float drainWidth = visibleBounds.getWidth() * (1.0f - ratio);
        if (drainWidth <= 0.5f)
            return;

        juce::Path roundedClip;
        roundedClip.addRoundedRectangle (visibleBounds, kCornerInset);

        juce::Graphics::ScopedSaveState saved (g);
        g.reduceClipRegion (roundedClip);

        const auto drainArea = visibleBounds.removeFromRight (drainWidth);
        g.setColour (drainColourCache);
        g.fillRect (drainArea);

        // Re-render text on top so the overlay doesn't bury it. JUCE's
        // TextButton draws via the LookAndFeel; easiest path is to
        // re-call drawButtonText with the same arguments.
        getLookAndFeel().drawButtonText (g, *this,
                                          shouldDrawAsHighlighted,
                                          shouldDrawAsDown);
    }

private:
    int countdownRemaining = 0;
    int countdownTotal     = 0;
    juce::Colour drainColourCache;

    // Matches WfsLookAndFeel::drawButtonBackground's left/right gutter
    // and corner radius (both 6 px). Keep these in sync if the L&F
    // changes its button shape.
    static constexpr float kCornerInset = 6.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CountdownTextButton)
};
