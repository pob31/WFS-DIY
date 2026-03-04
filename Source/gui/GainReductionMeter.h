#pragma once

#include <JuceHeader.h>
#include "ColorScheme.h"

/**
    GainReductionMeter

    A thin vertical bar showing gain reduction in dB.
    The bar extends downward from 0 dB at the top.

    Usage:
        meter.setColour (juce::Colour (0xFF3498DB));  // blue
        meter.setGainReductionDb (-6.0f);              // 6 dB of reduction
*/
class GainReductionMeter : public juce::Component
{
public:
    GainReductionMeter()
    {
        setOpaque (false);
    }

    void setMeterColour (juce::Colour c) { meterColour = c; }

    /** Set the current gain reduction in dB (0 = no reduction, negative = reduction). */
    void setGainReductionDb (float dB)
    {
        float clamped = juce::jlimit (minDb, 0.0f, dB);
        if (std::abs (clamped - targetDb) > 0.01f)
        {
            targetDb = clamped;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Smooth ballistics toward target
        float diff = targetDb - displayDb;
        if (std::abs (diff) < 0.1f)
            displayDb = targetDb;
        else
            displayDb += diff * 0.3f;  // Smooth decay

        // Background
        g.setColour (ColorScheme::get().chromeBackground.darker (0.3f));
        g.fillRoundedRectangle (bounds, 2.0f);

        // Meter bar — extends downward from top
        float normalisedGR = displayDb / minDb;  // 0 = no GR, 1 = max GR
        if (normalisedGR > 0.001f)
        {
            float barHeight = bounds.getHeight() * normalisedGR;
            auto barRect = bounds.removeFromTop (barHeight);
            g.setColour (meterColour);
            g.fillRoundedRectangle (barRect, 2.0f);
        }

        // dB value text at top
        if (displayDb < -0.5f)
        {
            g.setColour (juce::Colours::white.withAlpha (0.9f));
            float fontSize = juce::jmax (9.0f, getWidth() * 1.0f);
            g.setFont (juce::FontOptions (fontSize));
            int textHeight = juce::jmax (12, static_cast<int> (getHeight() * 0.15f));
            juce::String text = juce::String (displayDb, 1);
            g.drawText (text, getLocalBounds().removeFromTop (textHeight),
                        juce::Justification::centred, false);
        }

        // Schedule repaint if still animating
        if (std::abs (displayDb - targetDb) > 0.1f)
        {
            auto safeThis = juce::Component::SafePointer<GainReductionMeter> (this);
            juce::Timer::callAfterDelay (30, [safeThis]()
            {
                if (safeThis != nullptr)
                    safeThis->repaint();
            });
        }
    }

private:
    juce::Colour meterColour { 0xFF3498DB };
    float targetDb = 0.0f;
    float displayDb = 0.0f;
    static constexpr float minDb = -24.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainReductionMeter)
};
