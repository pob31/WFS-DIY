#pragma once

#include <cmath>

//==============================================================================
/**
    Parametric biquad filter for the reverb pre/post EQ.

    Supports 6 filter shapes matching reverbPreEQshape / reverbPostEQshape:
      0 = OFF (pass-through)
      1 = LowCut (2nd-order high-pass)
      2 = LowShelf
      3 = Peak / Notch
      4 = HighShelf
      5 = HighCut (2nd-order low-pass)

    Uses Audio EQ Cookbook formulas (Robert Bristow-Johnson).
    Designed for per-sample processing in the reverb engine thread.
*/
class ReverbBiquadFilter
{
public:
    ReverbBiquadFilter() = default;

    //==========================================================================
    // Lifecycle
    //==========================================================================

    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        reset();
        recalculate();
    }

    void reset()
    {
        x1 = x2 = y1 = y2 = 0.0f;
    }

    //==========================================================================
    // Parameter setters — recalculates only when something changed
    //==========================================================================

    void setParameters (int newShape, float newFreq, float newGainDb,
                        float newQ, float newSlope)
    {
        newFreq = std::max (20.0f, std::min (newFreq, 20000.0f));
        newQ    = std::max (0.1f, std::min (newQ, 20.0f));
        newSlope = std::max (0.1f, std::min (newSlope, 20.0f));
        newGainDb = std::max (-24.0f, std::min (newGainDb, 24.0f));
        newShape = std::max (0, std::min (newShape, 5));

        if (shape != newShape || freq != newFreq || gainDb != newGainDb
            || q != newQ || slope != newSlope)
        {
            shape   = newShape;
            freq    = newFreq;
            gainDb  = newGainDb;
            q       = newQ;
            slope   = newSlope;
            recalculate();
        }
    }

    int getShape() const { return shape; }
    bool isActive() const { return shape != 0; }

    //==========================================================================
    // Processing
    //==========================================================================

    float processSample (float input)
    {
        if (shape == 0)
            return input;

        float output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        x2 = x1;
        x1 = input;
        y2 = y1;
        y1 = output;
        return output;
    }

    void processBlock (float* samples, int numSamples)
    {
        if (shape == 0)
            return;

        for (int i = 0; i < numSamples; ++i)
            samples[i] = processSample (samples[i]);
    }

private:
    //==========================================================================
    void recalculate()
    {
        if (sampleRate <= 0.0 || shape == 0)
        {
            b0 = 1.0f; b1 = 0.0f; b2 = 0.0f;
            a1 = 0.0f; a2 = 0.0f;
            return;
        }

        constexpr float pi = 3.14159265358979f;
        float w0 = 2.0f * pi * freq / static_cast<float> (sampleRate);
        float cosw0 = std::cos (w0);
        float sinw0 = std::sin (w0);

        float a0_inv = 1.0f;

        switch (shape)
        {
            case 1: // LowCut (high-pass)
            {
                float alpha = sinw0 / (2.0f * q);
                a0_inv = 1.0f / (1.0f + alpha);
                b0 = ((1.0f + cosw0) / 2.0f) * a0_inv;
                b1 = -(1.0f + cosw0) * a0_inv;
                b2 = ((1.0f + cosw0) / 2.0f) * a0_inv;
                a1 = (-2.0f * cosw0) * a0_inv;
                a2 = (1.0f - alpha) * a0_inv;
                break;
            }

            case 2: // LowShelf
            {
                float A = std::pow (10.0f, gainDb / 40.0f);
                float alpha = (sinw0 / 2.0f) * std::sqrt (
                    (A + 1.0f / A) * (1.0f / slope - 1.0f) + 2.0f);
                float sqrtA2alpha = 2.0f * std::sqrt (A) * alpha;

                a0_inv = 1.0f / ((A + 1.0f) + (A - 1.0f) * cosw0 + sqrtA2alpha);
                b0 = A * ((A + 1.0f) - (A - 1.0f) * cosw0 + sqrtA2alpha) * a0_inv;
                b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosw0) * a0_inv;
                b2 = A * ((A + 1.0f) - (A - 1.0f) * cosw0 - sqrtA2alpha) * a0_inv;
                a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cosw0) * a0_inv;
                a2 = ((A + 1.0f) + (A - 1.0f) * cosw0 - sqrtA2alpha) * a0_inv;
                break;
            }

            case 3: // Peak / Notch
            {
                float A = std::pow (10.0f, gainDb / 40.0f);
                float alpha = sinw0 / (2.0f * q);

                a0_inv = 1.0f / (1.0f + alpha / A);
                b0 = (1.0f + alpha * A) * a0_inv;
                b1 = (-2.0f * cosw0) * a0_inv;
                b2 = (1.0f - alpha * A) * a0_inv;
                a1 = (-2.0f * cosw0) * a0_inv;
                a2 = (1.0f - alpha / A) * a0_inv;
                break;
            }

            case 4: // HighShelf
            {
                float A = std::pow (10.0f, gainDb / 40.0f);
                float alpha = (sinw0 / 2.0f) * std::sqrt (
                    (A + 1.0f / A) * (1.0f / slope - 1.0f) + 2.0f);
                float sqrtA2alpha = 2.0f * std::sqrt (A) * alpha;

                a0_inv = 1.0f / ((A + 1.0f) - (A - 1.0f) * cosw0 + sqrtA2alpha);
                b0 = A * ((A + 1.0f) + (A - 1.0f) * cosw0 + sqrtA2alpha) * a0_inv;
                b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosw0) * a0_inv;
                b2 = A * ((A + 1.0f) + (A - 1.0f) * cosw0 - sqrtA2alpha) * a0_inv;
                a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cosw0) * a0_inv;
                a2 = ((A + 1.0f) - (A - 1.0f) * cosw0 - sqrtA2alpha) * a0_inv;
                break;
            }

            case 5: // HighCut (low-pass)
            {
                float alpha = sinw0 / (2.0f * q);
                a0_inv = 1.0f / (1.0f + alpha);
                b0 = ((1.0f - cosw0) / 2.0f) * a0_inv;
                b1 = (1.0f - cosw0) * a0_inv;
                b2 = ((1.0f - cosw0) / 2.0f) * a0_inv;
                a1 = (-2.0f * cosw0) * a0_inv;
                a2 = (1.0f - alpha) * a0_inv;
                break;
            }

            default:
                b0 = 1.0f; b1 = 0.0f; b2 = 0.0f;
                a1 = 0.0f; a2 = 0.0f;
                break;
        }
    }

    //==========================================================================
    // State
    //==========================================================================

    int shape = 0;          // 0=OFF, 1=LowCut, 2=LowShelf, 3=Peak, 4=HighShelf, 5=HighCut
    float freq = 1000.0f;
    float gainDb = 0.0f;
    float q = 0.7f;
    float slope = 0.7f;
    double sampleRate = 48000.0;

    // Biquad coefficients (normalized, a0 = 1)
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
    float a1 = 0.0f, a2 = 0.0f;

    // Delay elements
    float x1 = 0.0f, x2 = 0.0f;
    float y1 = 0.0f, y2 = 0.0f;
};
