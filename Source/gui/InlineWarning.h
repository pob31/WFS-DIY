#pragma once

#include <JuceHeader.h>
#include "ColorScheme.h"
#include "WfsLookAndFeel.h"

/**
 * InlineWarning
 *
 * A compact amber warning shown next to a control when a feature is enabled on
 * an input but cannot take effect given the current system state (e.g. no
 * speaker supports it). Draws a warning triangle plus a short caption; the full
 * explanation of what to correct is exposed as the tooltip.
 *
 * It is a SettableTooltipClient so hovering surfaces the actionable detail via
 * the app's existing TooltipWindow.
 */
class InlineWarning : public juce::Component,
                      public juce::SettableTooltipClient
{
public:
    InlineWarning()
    {
        setInterceptsMouseClicks(true, false);  // needed so the tooltip fires on hover
    }

    /** shortText: the inline caption (may be empty for icon-only).
        detail:    the full explanation shown as the tooltip. */
    void setWarning(const juce::String& shortText, const juce::String& detail)
    {
        caption = shortText;
        setTooltip(detail);
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        const float h = juce::jmin(bounds.getHeight(), 18.0f * WfsLookAndFeel::uiScale);
        const juce::Colour amber(0xFFE0A020);

        // Warning triangle on the left
        const float triH = h;
        const float triW = triH * 1.12f;
        const float x = bounds.getX();
        const float cy = bounds.getCentreY();

        juce::Path tri;
        tri.addTriangle(x,               cy + triH * 0.5f,
                        x + triW,        cy + triH * 0.5f,
                        x + triW * 0.5f, cy - triH * 0.5f);
        g.setColour(amber);
        g.fillPath(tri);

        // Exclamation mark, sitting in the lower two-thirds of the triangle
        g.setColour(ColorScheme::get().background);
        g.setFont(juce::Font(juce::FontOptions(triH * 0.62f).withStyle("Bold")));
        g.drawText("!", juce::Rectangle<float>(x, cy - triH * 0.28f, triW, triH * 0.72f),
                   juce::Justification::centred);

        // Caption
        if (caption.isNotEmpty())
        {
            g.setColour(amber);
            g.setFont(juce::Font(juce::FontOptions(juce::jmax(10.0f, 12.0f * WfsLookAndFeel::uiScale))));
            g.drawText(caption, bounds.withTrimmedLeft(static_cast<int>(triW + 5.0f)),
                       juce::Justification::centredLeft, true /* use ellipsis */);
        }
    }

private:
    juce::String caption;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InlineWarning)
};
