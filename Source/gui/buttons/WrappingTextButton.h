#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/** TextButton that wraps its label onto a second line when the
    available width is too narrow for one, instead of horizontally
    squeezing the glyphs to fit.

    JUCE's default drawButtonText already allows two lines but uses
    a default minimumHorizontalScale of ~0.7, meaning it will shrink
    the text to 70% of its natural width before considering a line
    break. For toggle labels like "Constraint X: ON" / "Beschränkung
    X: AUS" this produces a visually squashed single line at narrow
    button widths. Passing minimumHorizontalScale = 1.0 disables that
    fallback so the engine wraps to two lines instead. */
class WrappingTextButton : public juce::TextButton
{
public:
    WrappingTextButton() = default;

    void paintButton (juce::Graphics& g,
                      bool shouldDrawAsHighlighted,
                      bool shouldDrawAsDown) override
    {
        auto& lf = getLookAndFeel();

        lf.drawButtonBackground (g, *this,
            findColour (getToggleState() ? buttonOnColourId : buttonColourId),
            shouldDrawAsHighlighted, shouldDrawAsDown);

        // Mirrors LookAndFeel_V2::drawButtonText but with
        // minimumHorizontalScale = 1.0 so wrapping is preferred over
        // shrinking when the button is narrower than the label.
        juce::Font font (lf.getTextButtonFont (*this, getHeight()));
        g.setFont (font);
        g.setColour (findColour (getToggleState() ? textColourOnId : textColourOffId)
                       .withMultipliedAlpha (isEnabled() ? 1.0f : 0.5f));

        const int yIndent     = juce::jmin (4, proportionOfHeight (0.3f));
        const int cornerSize  = juce::jmin (getHeight(), getWidth()) / 2;
        const int fontHeight  = juce::roundToInt (font.getHeight() * 0.6f);
        const int leftIndent  = juce::jmin (fontHeight, 2 + cornerSize / (isConnectedOnLeft()  ? 4 : 2));
        const int rightIndent = juce::jmin (fontHeight, 2 + cornerSize / (isConnectedOnRight() ? 4 : 2));
        const int textWidth   = getWidth() - leftIndent - rightIndent;

        if (textWidth > 0)
            g.drawFittedText (getButtonText(),
                              leftIndent, yIndent,
                              textWidth, getHeight() - yIndent * 2,
                              juce::Justification::centred,
                              2,        // max two lines
                              1.0f);    // no horizontal squeezing
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WrappingTextButton)
};
