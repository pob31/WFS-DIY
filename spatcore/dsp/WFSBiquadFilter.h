#pragma once

#include <cmath>

//==============================================================================
/**
    Configurable biquad filter for WFS floor reflections.

    Supports two filter types:
    - LowCut (2nd-order high-pass)
    - HighShelf (parametric high-shelf)

    Uses Audio EQ Cookbook formulas.
    Designed for efficient per-sample processing with many filter instances.
*/
class WFSBiquadFilter
{
public:
    enum class FilterType
    {
        LowCut,     // 2nd-order high-pass (Butterworth-style)
        HighShelf   // Parametric high-shelf with gain and slope
    };

    WFSBiquadFilter() = default;

    //==========================================================================
    /** Prepare filter with sample rate. Must be called before processing. */
    void prepare(double newSampleRate)
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
    /** Set filter type. Coefficients recalculated. */
    void setType(FilterType newType)
    {
        if (filterType != newType)
        {
            filterType = newType;
            recalculateCoefficients();
        }
    }

    /** Get current filter type */
    FilterType getType() const { return filterType; }

    /** Set frequency in Hz. Coefficients recalculated. */
    void setFrequency(float newFrequency)
    {
        newFrequency = std::max(20.0f, std::min(newFrequency, 20000.0f));
        if (frequency != newFrequency)
        {
            frequency = newFrequency;
            recalculateCoefficients();
        }
    }

    /** Get current frequency */
    float getFrequency() const { return frequency; }

    /** Set gain in dB (for high-shelf only). Coefficients recalculated. */
    void setGainDb(float newGainDb)
    {
        newGainDb = std::max(-24.0f, std::min(newGainDb, 12.0f));
        if (gainDb != newGainDb)
        {
            gainDb = newGainDb;
            if (filterType == FilterType::HighShelf)
                recalculateCoefficients();
        }
    }

    /** Get current gain setting */
    float getGainDb() const { return gainDb; }

    /** Set slope (0.1-0.9, for high-shelf). Coefficients recalculated. */
    void setSlope(float newSlope)
    {
        newSlope = std::max(0.1f, std::min(newSlope, 0.9f));
        if (slope != newSlope)
        {
            slope = newSlope;
            if (filterType == FilterType::HighShelf)
                recalculateCoefficients();
        }
    }

    /** Get current slope setting */
    float getSlope() const { return slope; }

    //==========================================================================
    /** Process a single sample */
    float processSample(float input)
    {
        float output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;

        x2 = x1;
        x1 = input;
        y2 = y1;
        y1 = output;

        return output;
    }

    /** Process a block of samples in-place */
    void processBlock(float* samples, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
            samples[i] = processSample(samples[i]);
    }

private:
    void recalculateCoefficients()
    {
        if (sampleRate <= 0.0)
            return;

        constexpr float pi = 3.14159265358979f;
        float w0 = 2.0f * pi * frequency / static_cast<float>(sampleRate);
        float cosw0 = std::cos(w0);
        float sinw0 = std::sin(w0);

        if (filterType == FilterType::LowCut)
        {
            // 2nd-order high-pass (Butterworth Q = 0.7071)
            constexpr float Q = 0.7071f;
            float alpha = sinw0 / (2.0f * Q);

            float a0_inv = 1.0f / (1.0f + alpha);

            b0 = ((1.0f + cosw0) / 2.0f) * a0_inv;
            b1 = -(1.0f + cosw0) * a0_inv;
            b2 = ((1.0f + cosw0) / 2.0f) * a0_inv;
            a1 = (-2.0f * cosw0) * a0_inv;
            a2 = (1.0f - alpha) * a0_inv;
        }
        else // HighShelf
        {
            // High shelf filter using Audio EQ Cookbook formulas
            float A = std::pow(10.0f, gainDb / 40.0f);  // sqrt(10^(dB/20))

            // Use slope parameter as S (shelf slope), converted from 0.1-0.9 range
            float S = slope;
            float alpha = (sinw0 / 2.0f) * std::sqrt((A + 1.0f / A) * (1.0f / S - 1.0f) + 2.0f);

            float a0_inv = 1.0f / ((A + 1.0f) - (A - 1.0f) * cosw0 + 2.0f * std::sqrt(A) * alpha);

            b0 = A * ((A + 1.0f) + (A - 1.0f) * cosw0 + 2.0f * std::sqrt(A) * alpha) * a0_inv;
            b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosw0) * a0_inv;
            b2 = A * ((A + 1.0f) + (A - 1.0f) * cosw0 - 2.0f * std::sqrt(A) * alpha) * a0_inv;
            a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cosw0) * a0_inv;
            a2 = ((A + 1.0f) - (A - 1.0f) * cosw0 - 2.0f * std::sqrt(A) * alpha) * a0_inv;
        }
    }

    //==========================================================================
    // Filter type
    FilterType filterType = FilterType::LowCut;

    // Biquad coefficients (normalized so a0 = 1)
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
    float a1 = 0.0f, a2 = 0.0f;

    // Filter state (delay elements)
    float x1 = 0.0f, x2 = 0.0f;  // Input history
    float y1 = 0.0f, y2 = 0.0f;  // Output history

    // Parameters
    float frequency = 100.0f;    // Hz
    float gainDb = 0.0f;         // dB (for high-shelf)
    float slope = 0.4f;          // Shelf slope (0.1-0.9)
    double sampleRate = 44100.0;
};
