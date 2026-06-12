#pragma once

/*
    WfsFrHostState - host-side Floor Reflection state shared by the GPU WFS
    backends (Metal + CUDA).

    JUCE-free. Owns everything FR-related that runs on the CPU side of the
    GPU algorithm, mirroring InputBufferProcessor's per-input FR machinery:

      - Per-input FR pre-filter chain (optional LowCut + optional configurable
        HighShelf, WFSBiquadFilter reused verbatim so coefficient math, clamps
        and enable-freeze semantics match the CPU reference exactly). Runs on
        the pump thread over numInputs x blockSize samples per launch (tens of
        microseconds) and fills the frIn staging buffer the kernel appends to
        the FR ring.
      - Per-(in,out) diffusion jitter (the FIXED algorithm: draw a uniform
        target every 3rd 64-sample sub-step, one-pole 0.05 toward it each
        sub-step; per-input mt19937 seeded inputIndex*12345+67890 like the CPU
        path). Runs ceil(blockSize/64) sub-steps per launch to match the CPU's
        64-sample internal cadence regardless of host block size.
      - FR curr-matrix computation: absolute FR delay in samples with the
        pipeline latency pre-subtracted from the ABSOLUTE delay
        (direct + extra + jitter - L), preserving the FR-vs-direct offset.

    Threading contract: setFRFilterParams / setFRDiffusion may be called from
    the 50 Hz timer thread (atomics); everything else is called by the single
    pump thread between launches. The pump applies the param atomics to the
    filters at block start (WFSBiquadFilter recalculates only on change).
*/

#include "../WFSBiquadFilter.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <memory>
#include <random>
#include <vector>

class WfsFrHostState
{
public:
    WfsFrHostState() = default;

    void prepare (int numInputs, int numOutputs, double sampleRate)
    {
        numIn = std::max (1, numInputs);
        numOut = std::max (1, numOutputs);

        // unique_ptr elements: InputParams holds atomics (non-movable), so the
        // vector stores pointers to keep resize/relocation legal.
        params.clear();
        params.reserve ((size_t) numIn);
        for (int i = 0; i < numIn; ++i)
            params.push_back (std::make_unique<InputParams>());

        lowCutFilters.clear();
        highShelfFilters.clear();
        lowCutFilters.resize ((size_t) numIn);
        highShelfFilters.resize ((size_t) numIn);
        for (int i = 0; i < numIn; ++i)
        {
            auto& lc = lowCutFilters[(size_t) i];
            lc.prepare (sampleRate);
            lc.setType (WFSBiquadFilter::FilterType::LowCut);
            lc.setFrequency (100.0f);

            auto& hs = highShelfFilters[(size_t) i];
            hs.prepare (sampleRate);
            hs.setType (WFSBiquadFilter::FilterType::HighShelf);
            hs.setFrequency (3000.0f);
            hs.setGainDb (-2.0f);
            hs.setSlope (0.4f);
        }

        jitterStateMs.assign ((size_t) numIn * (size_t) numOut, 0.0f);
        jitterTargetMs.assign ((size_t) numIn * (size_t) numOut, 0.0f);
        jitterCounters.assign ((size_t) numIn, 0);
        jitterEngines.clear();
        for (int i = 0; i < numIn; ++i)
            jitterEngines.emplace_back ((unsigned int) (i * 12345 + 67890)); // same seeding as the CPU path
    }

    void reset()
    {
        for (auto& f : lowCutFilters)    f.reset();
        for (auto& f : highShelfFilters) f.reset();
        std::fill (jitterStateMs.begin(), jitterStateMs.end(), 0.0f);
        std::fill (jitterTargetMs.begin(), jitterTargetMs.end(), 0.0f);
        std::fill (jitterCounters.begin(), jitterCounters.end(), 0);
    }

    // ==== 50 Hz timer-thread setters (mirror InputBufferProcessor's) ====

    void setFRFilterParams (int inputIndex,
                            bool lowCutActive, float lowCutFreq,
                            bool highShelfActive, float highShelfFreq,
                            float highShelfGain, float highShelfSlope) noexcept
    {
        if (inputIndex < 0 || inputIndex >= (int) params.size())
            return;
        auto& p = *params[(size_t) inputIndex];
        p.lowCutActive.store (lowCutActive, std::memory_order_release);
        p.lowCutFreq.store (lowCutFreq, std::memory_order_release);
        p.highShelfActive.store (highShelfActive, std::memory_order_release);
        p.highShelfFreq.store (highShelfFreq, std::memory_order_release);
        p.highShelfGain.store (highShelfGain, std::memory_order_release);
        p.highShelfSlope.store (highShelfSlope, std::memory_order_release);
    }

    void setFRDiffusion (int inputIndex, float diffusionPercent) noexcept
    {
        if (inputIndex < 0 || inputIndex >= (int) params.size())
            return;
        // Max jitter is 5 ms at 100% diffusion (5 / 100 = 0.05), as on CPU.
        params[(size_t) inputIndex]->maxJitterMs.store (diffusionPercent * 0.05f,
                                                        std::memory_order_release);
    }

    // ==== Pump-thread per-launch steps ====

    /** Applies the param atomics to the per-input filters and runs the FR
        pre-filter chain over the launch block: frInFlat[in*blockSize + s] =
        highShelf(lowCut(input)) per active flags. Null input channels are
        processed as silence (keeps filter state evolution identical to a
        silent feed). The FR staging is ALWAYS written, filtered or not,
        mirroring the CPU's unconditional FR-ring write. */
    void filterBlock (const float* const* inputs, float* frInFlat, int blockSize)
    {
        for (int in = 0; in < numIn; ++in)
        {
            auto& p = *params[(size_t) in];
            const bool lcActive = p.lowCutActive.load (std::memory_order_acquire);
            const bool hsActive = p.highShelfActive.load (std::memory_order_acquire);

            auto& lc = lowCutFilters[(size_t) in];
            auto& hs = highShelfFilters[(size_t) in];

            // Apply params only while active, like the CPU setter (a disabled
            // filter's parameters stay frozen). Recalc happens only on change.
            if (lcActive)
                lc.setFrequency (p.lowCutFreq.load (std::memory_order_acquire));
            if (hsActive)
            {
                hs.setFrequency (p.highShelfFreq.load (std::memory_order_acquire));
                hs.setGainDb (p.highShelfGain.load (std::memory_order_acquire));
                hs.setSlope (p.highShelfSlope.load (std::memory_order_acquire));
            }

            const float* src = (inputs != nullptr) ? inputs[in] : nullptr;
            float* dst = frInFlat + (size_t) in * (size_t) blockSize;
            for (int s = 0; s < blockSize; ++s)
            {
                float v = (src != nullptr) ? src[s] : 0.0f;
                if (lcActive)
                    v = lc.processSample (v);
                if (hsActive)
                    v = hs.processSample (v);
                dst[s] = v;
            }
        }
    }

    /** Advances the diffusion jitter by ceil(blockSize/64) sub-steps, matching
        the CPU's 64-sample internal cadence (draw new per-output targets every
        3rd sub-step, one-pole 0.05 toward target each sub-step). */
    void advanceJitter (int blockSize)
    {
        const int subSteps = std::max (1, (blockSize + 63) / 64);
        const float smoothingFactor = 0.05f;

        for (int step = 0; step < subSteps; ++step)
        {
            for (int in = 0; in < numIn; ++in)
            {
                const float maxJitter = params[(size_t) in]->maxJitterMs.load (std::memory_order_acquire);

                auto& counter = jitterCounters[(size_t) in];
                ++counter;
                const bool drawNewTargets = (counter >= 3);
                if (drawNewTargets)
                    counter = 0;

                const size_t base = (size_t) in * (size_t) numOut;
                for (int out = 0; out < numOut; ++out)
                {
                    if (drawNewTargets)
                    {
                        std::uniform_real_distribution<float> dist (-maxJitter, maxJitter);
                        jitterTargetMs[base + (size_t) out] = dist (jitterEngines[(size_t) in]);
                    }
                    jitterStateMs[base + (size_t) out]
                        += (jitterTargetMs[base + (size_t) out] - jitterStateMs[base + (size_t) out])
                           * smoothingFactor;
                }
            }
        }
    }

    /** Fills the FR curr matrices for this launch (input-major [in*numOut+out]):
        frDelaysCurr in samples = clamp((directMs + extraMs + jitterMs - latencyMs)
        * srScale, 0, maxDelaySamples); frGainsCurr = frLevels (absolute linear).
        Null app pointers produce zeros (FR silent). */
    void computeFrCurr (const float* delaysMs, const float* frDelaysMs, const float* frLevels,
                        float latencyMs, float srScale, float maxDelaySamples,
                        float* frDelaysCurr, float* frGainsCurr) const noexcept
    {
        const size_t matrix = (size_t) numIn * (size_t) numOut;
        for (size_t m = 0; m < matrix; ++m)
        {
            const float g = (frLevels != nullptr) ? frLevels[m] : 0.0f;
            frGainsCurr[m] = g;

            float d = 0.0f;
            if (delaysMs != nullptr && frDelaysMs != nullptr)
                d = (delaysMs[m] + frDelaysMs[m] + jitterStateMs[m] - latencyMs) * srScale;
            frDelaysCurr[m] = std::clamp (d, 0.0f, maxDelaySamples);
        }
    }

private:
    struct InputParams
    {
        std::atomic<bool>  lowCutActive { false };
        std::atomic<float> lowCutFreq { 100.0f };
        std::atomic<bool>  highShelfActive { false };
        std::atomic<float> highShelfFreq { 3000.0f };
        std::atomic<float> highShelfGain { -2.0f };
        std::atomic<float> highShelfSlope { 0.4f };
        std::atomic<float> maxJitterMs { 0.0f };
    };

    int numIn = 0, numOut = 0;

    std::vector<std::unique_ptr<InputParams>> params; // per input (atomics: non-movable)
    std::vector<WFSBiquadFilter> lowCutFilters;      // per input, persistent state
    std::vector<WFSBiquadFilter> highShelfFilters;   // per input, persistent state

    std::vector<float> jitterStateMs;                // [in*numOut+out]
    std::vector<float> jitterTargetMs;               // [in*numOut+out]
    std::vector<int> jitterCounters;                 // per input
    std::vector<std::mt19937> jitterEngines;         // per input
};
