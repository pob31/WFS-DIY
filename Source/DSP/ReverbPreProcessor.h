#pragma once

#include "ReverbBiquadFilter.h"
#include <array>
#include <vector>
#include <cmath>

//==============================================================================
/**
    Reverb Pre-Processor: per-node 4-band EQ → sidechain tap → global compressor.

    Processing chain per node:
    1. 4-band parametric EQ (per-channel settings, each band independently shaped)
    2. Sidechain tap — captures post-EQ RMS level for the post-expander
    3. Feed-forward compressor (global settings, applied per-node)
*/
class ReverbPreProcessor
{
public:
    static constexpr int MAX_NODES = 16;
    static constexpr int NUM_EQ_BANDS = 4;

    //==========================================================================
    // Parameter structs
    //==========================================================================

    struct EQBandParams
    {
        int   shape = 0;       // 0=OFF, 1-5 = filter shapes
        float freq  = 1000.0f;
        float gain  = 0.0f;
        float q     = 0.7f;
        float slope = 0.7f;
    };

    struct PreProcessorParams
    {
        // Per-node EQ (up to MAX_NODES × NUM_EQ_BANDS)
        std::array<std::array<EQBandParams, NUM_EQ_BANDS>, MAX_NODES> eqBands {};
        std::array<bool, MAX_NODES> eqEnabled {};  // per-node EQ master enable

        // Global compressor
        bool  compBypass    = true;
        float compThreshold = -12.0f;  // dB
        float compRatio     = 2.0f;    // :1
        float compAttack    = 10.0f;   // ms
        float compRelease   = 100.0f;  // ms

        PreProcessorParams()
        {
            eqEnabled.fill (true);
        }
    };

    //==========================================================================
    // Lifecycle
    //==========================================================================

    void prepare (double newSampleRate, int /*maxBlockSize*/, int numNodes)
    {
        sr = newSampleRate;
        numActiveNodes = std::min (numNodes, MAX_NODES);

        // Prepare per-node EQ filters
        for (int n = 0; n < numActiveNodes; ++n)
            for (int b = 0; b < NUM_EQ_BANDS; ++b)
                eqFilters[static_cast<size_t> (n)][static_cast<size_t> (b)].prepare (sr);

        // Reset compressor envelopes
        compEnvelopes.assign (static_cast<size_t> (numActiveNodes), 0.0f);

        // Reset sidechain levels
        sidechainLevels.assign (static_cast<size_t> (numActiveNodes), 0.0f);

        updateCompressorCoeffs();
    }

    void reset()
    {
        for (int n = 0; n < MAX_NODES; ++n)
            for (int b = 0; b < NUM_EQ_BANDS; ++b)
                eqFilters[static_cast<size_t> (n)][static_cast<size_t> (b)].reset();

        std::fill (compEnvelopes.begin(), compEnvelopes.end(), 0.0f);
        std::fill (sidechainLevels.begin(), sidechainLevels.end(), 0.0f);
    }

    //==========================================================================
    // Parameter update (called from engine thread after pending params arrive)
    //==========================================================================

    void setParameters (const PreProcessorParams& newParams)
    {
        bool compChanged = (newParams.compBypass != params.compBypass
                           || newParams.compThreshold != params.compThreshold
                           || newParams.compRatio != params.compRatio
                           || newParams.compAttack != params.compAttack
                           || newParams.compRelease != params.compRelease);

        params = newParams;

        // Update EQ filter coefficients
        for (int n = 0; n < numActiveNodes; ++n)
        {
            for (int b = 0; b < NUM_EQ_BANDS; ++b)
            {
                auto& bp = params.eqBands[static_cast<size_t> (n)][static_cast<size_t> (b)];
                int effectiveShape = params.eqEnabled[static_cast<size_t> (n)] ? bp.shape : 0;
                eqFilters[static_cast<size_t> (n)][static_cast<size_t> (b)]
                    .setParameters (effectiveShape, bp.freq, bp.gain, bp.q, bp.slope);
            }
        }

        if (compChanged)
            updateCompressorCoeffs();
    }

    //==========================================================================
    // Processing
    //==========================================================================

    /**
        Process a block of audio through per-node EQ and global compressor.
        Also computes sidechain levels (post-EQ RMS per node) for the post-expander.

        @param buffer           Audio buffer (numNodes channels × numSamples)
        @param outSidechain     Output: per-node sidechain RMS levels [numActiveNodes]
        @param numSamples       Number of samples to process
    */
    void processBlock (juce::AudioBuffer<float>& buffer,
                       std::vector<float>& outSidechain,
                       int numSamples)
    {
        outSidechain.resize (static_cast<size_t> (numActiveNodes));

        for (int n = 0; n < numActiveNodes; ++n)
        {
            float* data = buffer.getWritePointer (n);

            // 1. Per-node 4-band EQ
            for (int b = 0; b < NUM_EQ_BANDS; ++b)
                eqFilters[static_cast<size_t> (n)][static_cast<size_t> (b)]
                    .processBlock (data, numSamples);

            // 2. Sidechain tap: compute RMS of post-EQ signal
            float sumSq = 0.0f;
            for (int s = 0; s < numSamples; ++s)
                sumSq += data[s] * data[s];
            float rms = std::sqrt (sumSq / static_cast<float> (std::max (1, numSamples)));
            outSidechain[static_cast<size_t> (n)] = rms;

            // 3. Compressor (global settings, per-node envelope)
            if (! params.compBypass)
                processCompressor (data, numSamples, n);
        }
    }

private:
    //==========================================================================
    // Compressor implementation
    //==========================================================================

    void processCompressor (float* data, int numSamples, int nodeIndex)
    {
        float& envelope = compEnvelopes[static_cast<size_t> (nodeIndex)];

        for (int s = 0; s < numSamples; ++s)
        {
            // Input level in dB
            float absLevel = std::abs (data[s]);
            float levelDb = (absLevel > 1e-10f)
                ? 20.0f * std::log10 (absLevel)
                : -200.0f;

            // Feed-forward gain computation
            float gainDb = 0.0f;
            if (levelDb > compThresholdDb)
            {
                // Above threshold: compress
                float overshoot = levelDb - compThresholdDb;
                gainDb = overshoot * (1.0f / compRatioVal - 1.0f);
            }

            // Envelope follower (smooth the gain)
            float coeff = (gainDb < envelope) ? compAttackCoeff : compReleaseCoeff;
            envelope += coeff * (gainDb - envelope);

            // Apply gain
            float gain = std::pow (10.0f, envelope / 20.0f);
            data[s] *= gain;
        }
    }

    void updateCompressorCoeffs()
    {
        if (sr <= 0.0)
            return;

        compThresholdDb = params.compThreshold;
        compRatioVal = std::max (1.0f, params.compRatio);

        float attackSec  = params.compAttack * 0.001f;
        float releaseSec = params.compRelease * 0.001f;

        // Time constant: coeff = 1 - exp(-1 / (sr * time))
        compAttackCoeff  = 1.0f - std::exp (-1.0f / (static_cast<float> (sr) * std::max (0.0001f, attackSec)));
        compReleaseCoeff = 1.0f - std::exp (-1.0f / (static_cast<float> (sr) * std::max (0.001f, releaseSec)));
    }

    //==========================================================================
    // State
    //==========================================================================

    double sr = 48000.0;
    int numActiveNodes = 0;

    PreProcessorParams params;

    // Per-node EQ: MAX_NODES × NUM_EQ_BANDS filters
    std::array<std::array<ReverbBiquadFilter, NUM_EQ_BANDS>, MAX_NODES> eqFilters;

    // Per-node compressor envelope (dB domain)
    std::vector<float> compEnvelopes;

    // Per-node sidechain levels (post-EQ RMS)
    std::vector<float> sidechainLevels;

    // Compressor cached coefficients
    float compThresholdDb = -12.0f;
    float compRatioVal    = 2.0f;
    float compAttackCoeff  = 0.01f;
    float compReleaseCoeff = 0.001f;
};
