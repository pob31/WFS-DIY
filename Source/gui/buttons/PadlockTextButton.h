#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/** TextButton that draws a padlock in front of its label: open and
    yellow when unlocked, closed and red when locked, so a locked
    channel stands out at a glance.

    The padlock is drawn as a path rather than the 🔒/🔓 emoji because
    colour emoji glyphs ignore the text colour and always come out in
    the emoji font's own yellow.

    The lock state doubles as the button's toggle state (toggleable but
    not click-toggling — the owner decides) so screen readers still
    announce locked/unlocked now that the emoji is gone from the text. */
class PadlockTextButton : public juce::TextButton
{
public:
    PadlockTextButton() { setToggleable (true); }

    void setLocked (bool shouldBeLocked)
    {
        setToggleState (shouldBeLocked, juce::dontSendNotification);
    }

    bool isLocked() const { return getToggleState(); }

    void paintButton (juce::Graphics& g,
                      bool shouldDrawAsHighlighted,
                      bool shouldDrawAsDown) override
    {
        auto& lf = getLookAndFeel();

        // The lock state is shown by the padlock alone, so the background
        // stays the normal button colour in both states.
        lf.drawButtonBackground (g, *this, findColour (buttonColourId),
                                 shouldDrawAsHighlighted, shouldDrawAsDown);

        const float alpha = isEnabled() ? 1.0f : 0.5f;
        juce::Font font (lf.getTextButtonFont (*this, getHeight()));

        // WfsLookAndFeel insets the background 6 px each side; keep a little
        // padding inside that.
        auto area = getLocalBounds().toFloat().reduced (10.0f, 2.0f);
        const float iconSize = juce::jmin (font.getHeight() * 1.1f, area.getHeight());
        const float gap = iconSize * 0.35f;

        // Centre icon + label as one group when the label fits on a line;
        // otherwise pin the icon left and let the label wrap beside it.
        const float textWidth = juce::GlyphArrangement::getStringWidth (font, getButtonText());
        const float groupWidth = iconSize + gap + textWidth;
        if (groupWidth < area.getWidth())
            area = area.withSizeKeepingCentre (groupWidth, area.getHeight());

        auto iconArea = area.removeFromLeft (iconSize)
                            .withSizeKeepingCentre (iconSize, iconSize);
        area.removeFromLeft (gap);

        drawPadlock (g, iconArea, isLocked(), alpha);

        g.setFont (font);
        g.setColour (findColour (textColourOffId).withMultipliedAlpha (alpha));
        g.drawFittedText (getButtonText(), area.toNearestInt(),
                          juce::Justification::centredLeft, 2);
    }

private:
    static void drawPadlock (juce::Graphics& g, juce::Rectangle<float> r,
                             bool locked, float alpha)
    {
        const auto colour = (locked ? juce::Colour (0xFFE53935)    // red
                                    : juce::Colour (0xFFF2C230))   // yellow
                                .withMultipliedAlpha (alpha);
        const float s = r.getWidth();

        // Body: lower half of the icon.
        auto body = juce::Rectangle<float> (r.getX() + s * 0.12f, r.getY() + s * 0.46f,
                                            s * 0.76f, s * 0.54f);

        // Shackle: an arch over the body. Unlocked, it is raised and its
        // right leg stops short of the body.
        const float thickness = s * 0.12f;
        const float radius    = s * 0.24f;
        const float lift      = locked ? 0.0f : s * 0.14f;
        const float cx        = r.getCentreX();
        const float arcY      = r.getY() + radius + thickness * 0.5f + (locked ? s * 0.06f : 0.0f);
        const float leftEnd   = body.getY() + thickness * 0.5f;
        const float rightEnd  = locked ? leftEnd : body.getY() - lift;

        juce::Path shackle;
        shackle.startNewSubPath (cx - radius, leftEnd);
        shackle.lineTo (cx - radius, arcY);
        shackle.addCentredArc (cx, arcY, radius, radius, 0.0f,
                               -juce::MathConstants<float>::halfPi,
                                juce::MathConstants<float>::halfPi);
        shackle.lineTo (cx + radius, rightEnd);

        g.setColour (colour);
        g.strokePath (shackle, juce::PathStrokeType (thickness,
                                                     juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::butt));
        g.fillRoundedRectangle (body, s * 0.1f);

        // Keyhole
        g.setColour (juce::Colours::black.withAlpha (0.45f * alpha));
        const float holeR = s * 0.07f;
        g.fillEllipse (cx - holeR, body.getY() + body.getHeight() * 0.32f, holeR * 2.0f, holeR * 2.0f);
        g.fillRect (cx - holeR * 0.45f, body.getY() + body.getHeight() * 0.45f,
                    holeR * 0.9f, body.getHeight() * 0.3f);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PadlockTextButton)
};
