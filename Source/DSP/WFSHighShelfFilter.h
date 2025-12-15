#pragma once

#include <cmath>

//==============================================================================
/**
    Simple high shelf biquad filter for WFS air absorption simulation.

    Fixed parameters: 800 Hz, Q = 0.3
    Variable: gain (dB, typically negative for attenuation)

    Uses Audio EQ Cookbook formulas.
    Designed for efficient per-sample processing with many filter instances.
*/
class WFSHighShelfFilter
{
public:
    WFSHighShelfFilter() = default;

    //==========================================================================
    /** Prepare filter with sample rate. Must be called before processing. */
    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        reset();
        recalculateCoefficients();
    }

    /** Reset filter state (clear delay elements) */
    void reset()
    {
        x1 = x2 = y1 = y2 = 0.0f;
    }

    //==========================================================================
    /** Set gain in dB (negative for attenuation). Coefficients recalculated. */
    void setGainDb (float newGainDb)
    {
        if (gainDb != newGainDb)
        {
            gainDb = newGainDb;
            recalculateCoefficients();
        }
    }

    /** Get current gain setting */
    float getGainDb() const { return gainDb; }

    //==========================================================================
    /** Process a single sample */
    float processSample (float input)
    {
        float output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;

        x2 = x1;
        x1 = input;
        y2 = y1;
        y1 = output;

        return output;
    }

    /** Process a block of samples in-place */
    void processBlock (float* samples, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
            samples[i] = processSample (samples[i]);
    }

private:
    void recalculateCoefficients()
    {
        // High shelf filter at 800 Hz with Q = 0.3
        // Using Audio EQ Cookbook formulas

        constexpr float frequency = 800.0f;
        constexpr float Q = 0.3f;

        if (sampleRate <= 0.0)
            return;

        float A = std::pow (10.0f, gainDb / 40.0f);  // sqrt(10^(dB/20))
        float w0 = 2.0f * 3.14159265358979f * frequency / static_cast<float> (sampleRate);
        float cosw0 = std::cos (w0);
        float sinw0 = std::sin (w0);

        // Alpha calculation for shelf filters
        // alpha = (sin(w0)/2) * sqrt( (A + 1/A) * (1/S - 1) + 2 )
        // where S is the shelf slope (we use Q as slope parameter)
        float alpha = (sinw0 / 2.0f) * std::sqrt ((A + 1.0f / A) * (1.0f / Q - 1.0f) + 2.0f);

        float a0_inv = 1.0f / ((A + 1.0f) - (A - 1.0f) * cosw0 + 2.0f * std::sqrt (A) * alpha);

        b0 = A * ((A + 1.0f) + (A - 1.0f) * cosw0 + 2.0f * std::sqrt (A) * alpha) * a0_inv;
        b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosw0) * a0_inv;
        b2 = A * ((A + 1.0f) + (A - 1.0f) * cosw0 - 2.0f * std::sqrt (A) * alpha) * a0_inv;
        a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cosw0) * a0_inv;
        a2 = ((A + 1.0f) - (A - 1.0f) * cosw0 - 2.0f * std::sqrt (A) * alpha) * a0_inv;
    }

    //==========================================================================
    // Biquad coefficients (normalized so a0 = 1)
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
    float a1 = 0.0f, a2 = 0.0f;

    // Filter state (delay elements)
    float x1 = 0.0f, x2 = 0.0f;  // Input history
    float y1 = 0.0f, y2 = 0.0f;  // Output history

    // Parameters
    float gainDb = 0.0f;
    double sampleRate = 44100.0;
};
