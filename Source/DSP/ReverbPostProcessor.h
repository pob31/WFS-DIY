#pragma once

#include "ReverbBiquadFilter.h"
#include <array>
#include <vector>
#include <cmath>

//==============================================================================
/**
    Reverb Post-Processor: global 4-band EQ → sidechain-keyed expander.

    Processing chain per node:
    1. 4-band parametric EQ (global settings, independent state per node)
    2. Expander keyed on sidechain signal from the pre-processor
       (post-pre-EQ dry level — ducks reverb tail when source goes quiet)
*/
class ReverbPostProcessor
{
public:
    static constexpr int MAX_NODES = 16;
    static constexpr int NUM_EQ_BANDS = 4;

    //==========================================================================
    // Parameter structs
    //==========================================================================

    struct EQBandParams
    {
        int   shape = 0;
        float freq  = 1000.0f;
        float gain  = 0.0f;
        float q     = 0.7f;
        float slope = 0.7f;
    };

    struct PostProcessorParams
    {
        // Global EQ (4 bands, same settings for all nodes)
        std::array<EQBandParams, NUM_EQ_BANDS> eqBands {};
        bool eqEnabled = true;

        // Global expander (sidechain-keyed)
        bool  expBypass    = true;
        float expThreshold = -40.0f;  // dB
        float expRatio     = 2.0f;    // 1:N expansion ratio
        float expAttack    = 1.0f;    // ms
        float expRelease   = 200.0f;  // ms
    };

    //==========================================================================
    // Lifecycle
    //==========================================================================

    void prepare (double newSampleRate, int /*maxBlockSize*/, int numNodes)
    {
        sr = newSampleRate;
        numActiveNodes = std::min (numNodes, MAX_NODES);

        // Prepare per-node EQ filters (same global settings, independent state)
        for (int n = 0; n < numActiveNodes; ++n)
            for (int b = 0; b < NUM_EQ_BANDS; ++b)
                eqFilters[static_cast<size_t> (n)][static_cast<size_t> (b)].prepare (sr);

        // Reset expander envelopes
        expEnvelopes.assign (static_cast<size_t> (numActiveNodes), -200.0f);

        updateExpanderCoeffs();
    }

    void reset()
    {
        for (int n = 0; n < MAX_NODES; ++n)
            for (int b = 0; b < NUM_EQ_BANDS; ++b)
                eqFilters[static_cast<size_t> (n)][static_cast<size_t> (b)].reset();

        std::fill (expEnvelopes.begin(), expEnvelopes.end(), -200.0f);
    }

    //==========================================================================
    // Parameter update
    //==========================================================================

    void setParameters (const PostProcessorParams& newParams)
    {
        bool expChanged = (newParams.expBypass != params.expBypass
                          || newParams.expThreshold != params.expThreshold
                          || newParams.expRatio != params.expRatio
                          || newParams.expAttack != params.expAttack
                          || newParams.expRelease != params.expRelease);

        params = newParams;

        // Update EQ filters for all nodes (same settings, independent state)
        for (int n = 0; n < numActiveNodes; ++n)
        {
            for (int b = 0; b < NUM_EQ_BANDS; ++b)
            {
                auto& bp = params.eqBands[static_cast<size_t> (b)];
                int effectiveShape = params.eqEnabled ? bp.shape : 0;
                eqFilters[static_cast<size_t> (n)][static_cast<size_t> (b)]
                    .setParameters (effectiveShape, bp.freq, bp.gain, bp.q, bp.slope);
            }
        }

        if (expChanged)
            updateExpanderCoeffs();
    }

    //==========================================================================
    // Processing
    //==========================================================================

    /**
        Process a block of reverb output through global EQ and sidechain-keyed expander.

        @param buffer           Audio buffer (numNodes channels × numSamples)
        @param sidechainLevels  Per-node sidechain RMS levels from pre-processor [numActiveNodes]
        @param numSamples       Number of samples to process
    */
    void processBlock (juce::AudioBuffer<float>& buffer,
                       const std::vector<float>& sidechainLevels,
                       int numSamples)
    {
        for (int n = 0; n < numActiveNodes; ++n)
        {
            float* data = buffer.getWritePointer (n);

            // 1. Per-node EQ (global settings, independent filter state)
            for (int b = 0; b < NUM_EQ_BANDS; ++b)
                eqFilters[static_cast<size_t> (n)][static_cast<size_t> (b)]
                    .processBlock (data, numSamples);

            // 2. Sidechain-keyed expander
            if (! params.expBypass && static_cast<size_t> (n) < sidechainLevels.size())
                processExpander (data, numSamples, n, sidechainLevels[static_cast<size_t> (n)]);
        }
    }

private:
    //==========================================================================
    // Expander implementation
    //==========================================================================

    void processExpander (float* data, int numSamples, int nodeIndex, float sidechainRms)
    {
        float& envelope = expEnvelopes[static_cast<size_t> (nodeIndex)];

        // Convert sidechain RMS to dB (this is the key signal level)
        float keyLevelDb = (sidechainRms > 1e-10f)
            ? 20.0f * std::log10 (sidechainRms)
            : -200.0f;

        // Compute expansion gain from sidechain key
        float targetGainDb = 0.0f;
        if (keyLevelDb < expThresholdDb)
        {
            // Below threshold: expand (reduce gain)
            float undershoot = expThresholdDb - keyLevelDb;
            targetGainDb = -undershoot * (expRatioVal - 1.0f);
        }

        // Envelope follower on the gain (smooth to avoid clicks)
        // Attack = key goes above threshold (gain recovery)
        // Release = key drops below threshold (gain reduction)
        float coeff = (targetGainDb > envelope) ? expAttackCoeff : expReleaseCoeff;
        envelope += coeff * (targetGainDb - envelope);

        // Apply the computed gain uniformly across the block
        // (sidechain is block-level RMS, so gain is constant per block)
        float gain = std::pow (10.0f, envelope / 20.0f);
        for (int s = 0; s < numSamples; ++s)
            data[s] *= gain;
    }

    void updateExpanderCoeffs()
    {
        if (sr <= 0.0)
            return;

        expThresholdDb = params.expThreshold;
        expRatioVal = std::max (1.0f, params.expRatio);

        float attackSec  = params.expAttack * 0.001f;
        float releaseSec = params.expRelease * 0.001f;

        // For expander: attack = how fast gain recovers when key goes above threshold
        //               release = how fast gain reduces when key drops below threshold
        expAttackCoeff  = 1.0f - std::exp (-1.0f / (static_cast<float> (sr) * std::max (0.0001f, attackSec)));
        expReleaseCoeff = 1.0f - std::exp (-1.0f / (static_cast<float> (sr) * std::max (0.001f, releaseSec)));

        // Scale coefficients for block-rate updates (256 samples at a time)
        // Since we apply gain uniformly per block, we want the envelope to
        // move at a rate appropriate for block-level updates
        float blockDuration = 256.0f / static_cast<float> (sr);
        expAttackCoeff  = 1.0f - std::exp (-blockDuration / std::max (0.0001f, attackSec));
        expReleaseCoeff = 1.0f - std::exp (-blockDuration / std::max (0.001f, releaseSec));
    }

    //==========================================================================
    // State
    //==========================================================================

    double sr = 48000.0;
    int numActiveNodes = 0;

    PostProcessorParams params;

    // Per-node EQ: MAX_NODES × NUM_EQ_BANDS filters (global settings, independent state)
    std::array<std::array<ReverbBiquadFilter, NUM_EQ_BANDS>, MAX_NODES> eqFilters;

    // Per-node expander envelope (dB domain)
    std::vector<float> expEnvelopes;

    // Expander cached coefficients
    float expThresholdDb = -40.0f;
    float expRatioVal    = 2.0f;
    float expAttackCoeff  = 0.1f;
    float expReleaseCoeff = 0.01f;
};
