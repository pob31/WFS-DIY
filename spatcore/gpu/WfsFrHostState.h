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
      - Per-(in,out) diffusion grain (the shared zone-mapped model in
        FrDiffusionModel.h, identical to the CPU processors): one-pole
        low-passed hash noise per routing pair, stepped ONCE per launch with
        updateInterval = blockSize. The kernel's prev->curr per-sample
        interpolation reconstructs the same low-rate waveform the CPU paths
        produce per-sample, so the grain character matches across backends.
      - FR curr-matrix computation: absolute FR delay in samples with the
        pipeline latency pre-subtracted from the ABSOLUTE delay
        (direct + extra + jitter - L), preserving the FR-vs-direct offset.

    Threading contract: setFRFilterParams / setFRDiffusion may be called from
    the 50 Hz timer thread (atomics); everything else is called by the single
    pump thread between launches. The pump applies the param atomics to the
    filters at block start (WFSBiquadFilter recalculates only on change).
*/

#include "../dsp/WFSBiquadFilter.h"
#include "../dsp/FrDiffusionModel.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

namespace spatcore::gpu {

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

        srHz = (float) sampleRate;
        jitterLpState.assign ((size_t) numIn * (size_t) numOut, 0.0f);
        jitterSamples.assign ((size_t) numIn * (size_t) numOut, 0.0f);
        launchCounter = 0;

        // OutputBuffer (scatter) sub-step state.
        jitterLast.assign ((size_t) numIn * (size_t) numOut, 0.0f);
        baseCurrScratch.assign ((size_t) numIn * (size_t) numOut, 0.0f);
        subStepCounter = 0;
    }

    void reset()
    {
        for (auto& f : lowCutFilters)    f.reset();
        for (auto& f : highShelfFilters) f.reset();
        std::fill (jitterLpState.begin(), jitterLpState.end(), 0.0f);
        std::fill (jitterSamples.begin(), jitterSamples.end(), 0.0f);
        launchCounter = 0;
        std::fill (jitterLast.begin(), jitterLast.end(), 0.0f);
        subStepCounter = 0;
        haveBase = false;
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
        // Shared zone-mapped model (FrDiffusionModel.h); publish the fraction.
        params[(size_t) inputIndex]->diffusionAmount.store (
            std::min (1.0f, std::max (0.0f, diffusionPercent * 0.01f)),
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

    /** Advances the diffusion grain by ONE step per launch (shared zone-mapped
        model, updateInterval = blockSize - the amplitude normalisation in
        FrDiffusionModel keeps the excursion identical to the CPU's per-sample
        update). Fills jitterSamples[in*numOut+out] with the clamped per-pair
        jitter in audio samples, consumed by computeFrCurr(). */
    void advanceJitter (int blockSize)
    {
        for (int in = 0; in < numIn; ++in)
        {
            const float d = params[(size_t) in]->diffusionAmount.load (std::memory_order_acquire);
            const auto coeffs = FrDiffusion::computeCoeffs (d, srHz, (float) blockSize);

            const size_t base = (size_t) in * (size_t) numOut;
            if (coeffs.ampSamples <= 0.0f)
            {
                std::fill (jitterSamples.begin() + (long) base,
                           jitterSamples.begin() + (long) base + numOut, 0.0f);
                continue;
            }

            for (int out = 0; out < numOut; ++out)
                jitterSamples[base + (size_t) out] = FrDiffusion::step (
                    jitterLpState[base + (size_t) out], launchCounter,
                    FrDiffusion::makeKey (in, out), coeffs);
        }
        ++launchCounter;
    }

    /** Fills the FR curr matrices for this launch (input-major [in*numOut+out]):
        frDelaysCurr in samples = clamp((directMs + extraMs - latencyMs) * srScale
        + jitterSamples, 0, maxDelaySamples); frGainsCurr = frLevels (absolute
        linear). The diffusion grain is added POST-scale in samples - matching
        the CPU paths, which add it post-smoother at the read/write position.
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
                d = (delaysMs[m] + frDelaysMs[m] - latencyMs) * srScale + jitterSamples[m];
            frDelaysCurr[m] = std::clamp (d, 0.0f, maxDelaySamples);
        }
    }

    /** OutputBuffer (scatter) variant of the FR delay: fills a PER-SAMPLE absolute
        FR delay buffer frDelaysOut[pair*blockSize + s] (pair = in*numOut+out),
        sub-stepping the diffusion grain every `subBlock` samples and ramping
        within each sub-block. This MUST match the CPU OutputBufferProcessor, which
        steps the grain once per 64-sample processing block and ramps it across the
        block (write-time scatter is sensitive to a per-sample noise walk on the
        write position - it produced audible hiss, so the CPU block-steps it; a
        single per-launch step like advanceJitter() is too coarse for the scatter).

        The smoothed base FR delay (direct + extra - L, no jitter) is approximated
        by a prev->curr linear ramp across the launch (basePrev in/out), consistent
        with the rest of the GPU port's delay handling. Diffusion off => jitter 0 =>
        pure base ramp (then this equals the gather path's FR delay).

        Delays are clamped to a MINIMUM of 1 sample: the write-time scatter cannot
        represent d < 1 (the head cell was just read+cleared; a same-cell write
        only re-emerges when the head wraps ~1 s later), so pipeline-compensated
        delays below the latency floor clamp to 1 (the scatter analogue of the
        gather's "below L clamps to the floor" contract).

        frGainsPrev/frGainsCurr gate the per-sample fill: pairs whose FR gain is 0
        in both matrices are skipped (the kernel's doFr gate never reads their
        rows), which removes the O(pairs*blockSize) pump-thread cost for the
        common FR-sparse case. The jitter STATE still advances for every pair
        (cheap, one step per sub-block) so the noise stream phase matches the CPU,
        which steps diffusion regardless of levels. */
    void computeFrDelaysPerSample (const float* delaysMs, const float* frDelaysMs,
                                   const float* frGainsPrev, const float* frGainsCurr,
                                   float latencyMs, float srScale, float maxDelaySamples,
                                   int blockSize, int subBlock,
                                   std::vector<float>& basePrev,   // [pairs] in/out
                                   float* frDelaysOut) noexcept    // [pairs*blockSize]
    {
        const int pairs = numIn * numOut;
        if ((int) basePrev.size() != pairs)
            basePrev.assign ((size_t) pairs, 1.0f);

        baseCurrScratch.resize ((size_t) pairs);
        for (int m = 0; m < pairs; ++m)
        {
            float d = 0.0f;
            if (delaysMs != nullptr && frDelaysMs != nullptr)
                d = (delaysMs[m] + frDelaysMs[m] - latencyMs) * srScale;   // base, NO jitter
            baseCurrScratch[(size_t) m] = std::clamp (d, 1.0f, maxDelaySamples);
        }
        if (! haveBase)
        {
            basePrev = baseCurrScratch;
            haveBase = true;
        }

        // Hoist the per-input diffusion params out of the sub-block loop.
        diffusionScratch.resize ((size_t) numIn);
        for (int in = 0; in < numIn; ++in)
            diffusionScratch[(size_t) in] =
                params[(size_t) in]->diffusionAmount.load (std::memory_order_acquire);

        const int sub = std::max (1, subBlock);
        const float invLen = 1.0f / (float) std::max (1, blockSize);

        for (int a = 0; a < blockSize; a += sub)
        {
            const int b = std::min (a + sub, blockSize);
            const int subLen = b - a;
            ++subStepCounter;                          // one grain step per sub-block (CPU parity)
            const float invSub = 1.0f / (float) subLen;

            for (int in = 0; in < numIn; ++in)
            {
                const auto coeffs = FrDiffusion::computeCoeffs (diffusionScratch[(size_t) in],
                                                                srHz, (float) subLen);

                for (int out = 0; out < numOut; ++out)
                {
                    const int m = in * numOut + out;

                    // Advance the grain state for EVERY pair (CPU steps diffusion
                    // regardless of levels - keeps stream phase identical)...
                    float jCurr = 0.0f;
                    if (coeffs.ampSamples > 0.0f)
                        jCurr = FrDiffusion::step (jitterLpState[(size_t) m], subStepCounter,
                                                   FrDiffusion::makeKey (in, out), coeffs);
                    const float jPrev = jitterLast[(size_t) m];
                    jitterLast[(size_t) m] = jCurr;

                    // ...but only fill the per-sample row for FR-active pairs
                    // (the kernel's doFr gate never reads inactive rows).
                    if (frGainsPrev != nullptr && frGainsCurr != nullptr
                        && frGainsPrev[m] == 0.0f && frGainsCurr[m] == 0.0f)
                        continue;

                    float* dst = frDelaysOut + (size_t) m * (size_t) blockSize;
                    const float bp = basePrev[(size_t) m];
                    const float bc = baseCurrScratch[(size_t) m];
                    for (int s = a; s < b; ++s)
                    {
                        const float jit  = jPrev + (jCurr - jPrev) * ((float) (s - a + 1) * invSub);
                        const float base = bp + (bc - bp) * ((float) (s + 1) * invLen);
                        dst[(size_t) s] = std::clamp (base + jit, 1.0f, maxDelaySamples);
                    }
                }
            }
        }

        for (int m = 0; m < pairs; ++m)
            basePrev[(size_t) m] = baseCurrScratch[(size_t) m];
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
        std::atomic<float> diffusionAmount { 0.0f }; // Diffusion fraction 0..1
    };

    int numIn = 0, numOut = 0;

    std::vector<std::unique_ptr<InputParams>> params; // per input (atomics: non-movable)
    std::vector<WFSBiquadFilter> lowCutFilters;      // per input, persistent state
    std::vector<WFSBiquadFilter> highShelfFilters;   // per input, persistent state

    float srHz = 48000.0f;
    uint32_t launchCounter = 0;
    std::vector<float> jitterLpState;                // [in*numOut+out] one-pole LP state
    std::vector<float> jitterSamples;                // [in*numOut+out] clamped jitter (audio samples)

    // OutputBuffer (scatter) sub-step diffusion state (computeFrDelaysPerSample).
    std::vector<float> jitterLast;                   // [in*numOut+out] previous sub-block's jitter (ramp continuity)
    std::vector<float> baseCurrScratch;              // [in*numOut+out] this launch's base FR delay (no jitter)
    std::vector<float> diffusionScratch;             // [numIn] hoisted per-launch diffusion amounts
    uint32_t subStepCounter = 0;                     // 64-sample sub-block index (CPU frJitterBlockIndex parity)
    bool haveBase = false;                           // prev->curr base-delay ramp bootstrap
};

} // namespace spatcore::gpu

// Extraction-compat alias — app code migrates to qualified names later.
using spatcore::gpu::WfsFrHostState;
