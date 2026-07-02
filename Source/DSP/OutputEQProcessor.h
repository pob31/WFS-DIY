#pragma once

#include "../../spatcore/dsp/OutputEQBiquadFilter.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <vector>

//==============================================================================
/**
    Per-output 6-band parametric EQ processor.

    Each output channel has its own bank of 6 biquads in series. Filter state
    is independent per output; coefficient values come from per-channel
    parameter entries (typically kept in sync across the array by the GUI's
    array-propagation, but treated as independent here).

    Threading model mirrors ReverbPostProcessor:
      - prepare() / setParameters() are called from a non-audio thread (timer
        callback or prepareToPlay).
      - processBlock() runs on the audio thread.
      - The biquad's setParameters short-circuits on no-change, so pushing
        params unconditionally every tick is cheap when nothing moved.
*/
class OutputEQProcessor
{
public:
    static constexpr int NUM_EQ_BANDS = 6;

    //==========================================================================
    struct EQBandParams
    {
        int   shape = 0;
        float freq  = 1000.0f;
        float gain  = 0.0f;
        float q     = 0.7f;
        float slope = 0.7f;
    };

    struct OutputChannelParams
    {
        bool enabled = false;
        std::array<EQBandParams, NUM_EQ_BANDS> bands {};
    };

    struct Params
    {
        std::vector<OutputChannelParams> channels;
    };

    //==========================================================================
    void prepare (double newSampleRate, int /*maxBlockSize*/, int numOutputs)
    {
        sampleRate = newSampleRate;
        numActiveChannels = std::max (0, numOutputs);

        filters.resize (static_cast<size_t> (numActiveChannels));
        channelEnabled.assign (static_cast<size_t> (numActiveChannels), false);

        for (auto& bandArray : filters)
            for (auto& f : bandArray)
                f.prepare (sampleRate);
    }

    void reset()
    {
        for (auto& bandArray : filters)
            for (auto& f : bandArray)
                f.reset();
    }

    //==========================================================================
    void setParameters (const Params& newParams)
    {
        int n = std::min (static_cast<int> (newParams.channels.size()), numActiveChannels);

        for (int c = 0; c < n; ++c)
        {
            const auto& cp = newParams.channels[static_cast<size_t> (c)];
            channelEnabled[static_cast<size_t> (c)] = cp.enabled;

            auto& bandArray = filters[static_cast<size_t> (c)];
            for (int b = 0; b < NUM_EQ_BANDS; ++b)
            {
                const auto& bp = cp.bands[static_cast<size_t> (b)];
                int effectiveShape = cp.enabled ? bp.shape : 0;
                bandArray[static_cast<size_t> (b)]
                    .setParameters (effectiveShape, bp.freq, bp.gain, bp.q, bp.slope);
            }
        }
    }

    //==========================================================================
    /** Process EQ on the WFS output buffer slice. */
    void processBlock (juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
    {
        const int numCh = std::min (numActiveChannels, buffer.getNumChannels());

        for (int c = 0; c < numCh; ++c)
        {
            if (! channelEnabled[static_cast<size_t> (c)])
                continue;

            float* data = buffer.getWritePointer (c) + startSample;
            auto& bandArray = filters[static_cast<size_t> (c)];

            for (int b = 0; b < NUM_EQ_BANDS; ++b)
                bandArray[static_cast<size_t> (b)].processBlock (data, numSamples);
        }
    }

private:
    double sampleRate = 48000.0;
    int    numActiveChannels = 0;

    std::vector<std::array<OutputEQBiquadFilter, NUM_EQ_BANDS>> filters;
    std::vector<uint8_t> channelEnabled;  // uint8_t avoids vector<bool> proxy
};
