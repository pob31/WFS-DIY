#pragma once
#if WFS_GPU_NATIVE

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <vector>
#include <memory>
#include <cmath>

namespace spatcore::gpu {

/*
    GpuLevelMeters — lean host-side level metering for the native GPU WFS
    algorithms (NativeGpuWfs / NativeGpuOutputBuffer).

    The CPU InputBuffer/OutputBuffer paths run their LiveSourceLevelDetector /
    OutputLevelDetector instances on dedicated worker threads (InputBufferProcessor,
    InputAnalysisThread/OutputMeteringThread), never the audio callback. The GPU
    path's audio callback is deliberately near-idle (ring push/pop only), so this
    meter keeps the per-sample work transcendental-free: it tracks linear peak /
    short-peak / RMS-mean-square envelopes on the audio thread and defers the dB
    conversion (20*log10, sqrt) to the getters, which the 50 Hz timer thread calls.
    One Follower type serves both inputs and outputs (outputs simply ignore the
    short-peak envelope). Time constants match the CPU detectors: 100 ms peak
    release, 5 ms short-peak release (AutomOtion trigger), ~200 ms RMS window.

    Threading: meterInput/meterOutput/meterSilence run on the audio thread and
    mutate follower state; the owning algorithm only (re)builds the follower
    vectors inside its prepare()/releaseResources() under its procLock with
    ready==false, so the audio thread must hold that lock when calling these
    writers. The getters are lock-free (they read atomics) and are safe from the
    timer thread.

    Live Source Tamer: each input Follower also computes the per-input compressor
    gain reduction (peak + slow) reusing the peak/RMS envelopes it already tracks.
    Unlike the dB meters, the gain smoothing is a per-sample recursive filter so it
    runs inside the loop (only when that input's LS is active — gated to keep the
    log10/pow cost off the near-idle GPU callback thread for the common case). The
    GR is consumed at 50 Hz by LiveSourceTamerEngine, which bakes it into the WFS
    level matrix — the same matrix the GPU already reads — so no audio is gained
    here; we only supply the GR values, matching the CPU paths exactly. Note this
    runs the GR math on the GPU audio callback thread (the CPU paths use worker
    threads); acceptable because the callback is otherwise near-idle.
*/
class GpuLevelMeters
{
public:
    GpuLevelMeters() = default;

    /** Rebuild followers for the given channel counts. Call under the owning
        algorithm's procLock (audio thread is gated off via ready==false). */
    void prepare (int numIn, int numOut, double sampleRate)
    {
        const int ni = juce::jmax (0, numIn);
        const int no = juce::jmax (0, numOut);

        inputFollowers.clear();
        outputFollowers.clear();
        inputFollowers.reserve ((size_t) ni);
        outputFollowers.reserve ((size_t) no);

        for (int i = 0; i < ni; ++i)
        {
            auto f = std::make_unique<Follower>();
            f->prepare (sampleRate);
            inputFollowers.push_back (std::move (f));
        }
        for (int i = 0; i < no; ++i)
        {
            auto f = std::make_unique<Follower>();
            f->prepare (sampleRate);
            outputFollowers.push_back (std::move (f));
        }
    }

    void setOutputMeteringEnabled (bool enabled) noexcept
    {
        outputMeteringEnabled.store (enabled, std::memory_order_relaxed);
    }

    // === Audio-thread writers (call with the algorithm's procLock held) ===

    /** Always-on: input metering also feeds AutomOtion's audio trigger, which
        must work even when the meter window is closed (matches the CPU path
        where the input detector always runs). */
    void meterInput (const juce::AudioBuffer<float>& buffer, int numCh,
                     int startSample, int numSamples) noexcept
    {
        const int n = juce::jmin (numCh, (int) inputFollowers.size(), buffer.getNumChannels());
        for (int ch = 0; ch < n; ++ch)
            inputFollowers[(size_t) ch]->process (buffer.getReadPointer (ch, startSample), numSamples);
    }

    /** Gated by setOutputMeteringEnabled (meter window / map overlay). */
    void meterOutput (const juce::AudioBuffer<float>& buffer, int numCh,
                      int startSample, int numSamples) noexcept
    {
        if (! outputMeteringEnabled.load (std::memory_order_relaxed))
            return;

        const int n = juce::jmin (numCh, (int) outputFollowers.size(), buffer.getNumChannels());
        for (int ch = 0; ch < n; ++ch)
            outputFollowers[(size_t) ch]->process (buffer.getReadPointer (ch, startSample), numSamples);
    }

    /** Feed silence so the envelopes decay when a block is bypassed. */
    void meterSilence (int numSamples) noexcept
    {
        for (auto& f : inputFollowers)
            f->processSilence (numSamples);

        if (outputMeteringEnabled.load (std::memory_order_relaxed))
            for (auto& f : outputFollowers)
                f->processSilence (numSamples);
    }

    // === Timer-thread getters (lock-free; names mirror the CPU algorithms) ===

    // Input side — getShortPeakLevelDb/getRmsLevelDb feed AutomOtion;
    // getInputPeakLevelDb/getInputRmsLevelDb feed the input meters.
    float getShortPeakLevelDb (size_t i) const noexcept { return i < inputFollowers.size()  ? inputFollowers[i]->shortPeakDb() : -200.0f; }
    float getRmsLevelDb       (size_t i) const noexcept { return i < inputFollowers.size()  ? inputFollowers[i]->rmsDb()       : -200.0f; }
    float getInputPeakLevelDb (size_t i) const noexcept { return i < inputFollowers.size()  ? inputFollowers[i]->peakDb()      : -200.0f; }
    float getInputRmsLevelDb  (size_t i) const noexcept { return i < inputFollowers.size()  ? inputFollowers[i]->rmsDb()       : -200.0f; }

    // Output side
    float getOutputPeakLevelDb (size_t i) const noexcept { return i < outputFollowers.size() ? outputFollowers[i]->peakDb()    : -200.0f; }
    float getOutputRmsLevelDb  (size_t i) const noexcept { return i < outputFollowers.size() ? outputFollowers[i]->rmsDb()     : -200.0f; }

    // === Live Source Tamer (per-input compression gain reduction) ===
    // setLSParameters is pushed from the 50 Hz timer thread; the GR is computed on
    // the audio thread (gated by 'active') and read back via the getters. Mirrors
    // the CPU InputBufferAlgorithm/OutputBufferAlgorithm LS interface (plus the
    // 'active' gate). The GR is applied downstream via the WFS level matrix.
    void setLSParameters (size_t i, bool active,
                          float peakThresh, float peakRatio,
                          float slowThresh, float slowRatio) noexcept
    {
        if (i < inputFollowers.size())
            inputFollowers[i]->setLSParameters (active, peakThresh, peakRatio, slowThresh, slowRatio);
    }

    float getPeakGainReduction (size_t i) const noexcept { return i < inputFollowers.size() ? inputFollowers[i]->peakGainReduction() : 1.0f; }
    float getSlowGainReduction (size_t i) const noexcept { return i < inputFollowers.size() ? inputFollowers[i]->slowGainReduction() : 1.0f; }

private:
    struct Follower
    {
        void prepare (double sr)
        {
            sampleRate            = sr > 0.0 ? sr : 48000.0;
            peakReleaseCoeff      = (float) std::exp (-1.0 / (sampleRate * 0.1));    // 100 ms
            shortPeakReleaseCoeff = (float) std::exp (-1.0 / (sampleRate * 0.005));  // 5 ms
            rmsWindowSize         = juce::jmax (1, (int) (sampleRate / 5.0));        // ~200 ms

            rmsBuffer.assign ((size_t) rmsWindowSize, 0.0f);
            rmsWritePos       = 0;
            peakEnvelope      = 0.0f;
            shortPeakEnvelope = 0.0f;
            rmsSumSquared     = 0.0f;

            peakLin.store (0.0f, std::memory_order_relaxed);
            shortPeakLin.store (0.0f, std::memory_order_relaxed);
            rmsMeanSquared.store (0.0f, std::memory_order_relaxed);

            // LS gain-reduction smoothing — fixed sample counts, matching the CPU
            // LiveSourceLevelDetector verbatim (2 ms attack, 2 ms peak release,
            // 20 ms slow release at 48 kHz; sample-rate-independent like the
            // reference). lsActive stays false until setLSParameters re-pushes it.
            peakGainAttackCoeff  = (float) std::exp (-1.0 / 96.0);
            peakGainReleaseCoeff = (float) std::exp (-1.0 / 96.0);
            slowGainAttackCoeff  = (float) std::exp (-1.0 / 96.0);
            slowGainReleaseCoeff = (float) std::exp (-1.0 / 960.0);
            peakGainSmoothed = 1.0f;
            slowGainSmoothed = 1.0f;
            peakGR.store (1.0f, std::memory_order_relaxed);
            slowGR.store (1.0f, std::memory_order_relaxed);
        }

        // Metering per-sample work is transcendental-free; dB is computed on read.
        // LS gain reduction (when active) does run per-sample log10/pow/sqrt because
        // its gain smoother is a recursive filter that can't be deferred to read.
        void process (const float* data, int numSamples) noexcept
        {
            float pe  = peakEnvelope;
            float spe = shortPeakEnvelope;
            float sum = rmsSumSquared;
            int   pos = rmsWritePos;

            const bool ls = lsActive.load (std::memory_order_relaxed);
            float pgs = peakGainSmoothed, sgs = slowGainSmoothed;
            const float pThr = peakThreshold.load (std::memory_order_relaxed);
            const float pRat = peakRatio.load (std::memory_order_relaxed);
            const float sThr = slowThreshold.load (std::memory_order_relaxed);
            const float sRat = slowRatio.load (std::memory_order_relaxed);

            for (int i = 0; i < numSamples; ++i)
            {
                const float s = data[i];
                const float a = std::abs (s);
                pe  = a > pe  ? a : pe  * peakReleaseCoeff;
                spe = a > spe ? a : spe * shortPeakReleaseCoeff;

                const float sq = s * s;
                sum += sq - rmsBuffer[(size_t) pos];
                rmsBuffer[(size_t) pos] = sq;
                pos = (pos + 1) % rmsWindowSize;

                if (ls)
                    updateGainReduction (pe, sum, pgs, sgs, pThr, pRat, sThr, sRat);
            }

            store (pe, spe, sum, pos);
            if (ls)
                storeGains (pgs, sgs);
        }

        void processSilence (int numSamples) noexcept
        {
            float pe  = peakEnvelope;
            float spe = shortPeakEnvelope;
            float sum = rmsSumSquared;
            int   pos = rmsWritePos;

            const bool ls = lsActive.load (std::memory_order_relaxed);
            float pgs = peakGainSmoothed, sgs = slowGainSmoothed;
            const float pThr = peakThreshold.load (std::memory_order_relaxed);
            const float pRat = peakRatio.load (std::memory_order_relaxed);
            const float sThr = slowThreshold.load (std::memory_order_relaxed);
            const float sRat = slowRatio.load (std::memory_order_relaxed);

            for (int i = 0; i < numSamples; ++i)
            {
                pe  *= peakReleaseCoeff;
                spe *= shortPeakReleaseCoeff;
                sum -= rmsBuffer[(size_t) pos];
                rmsBuffer[(size_t) pos] = 0.0f;
                pos = (pos + 1) % rmsWindowSize;

                if (ls)
                    updateGainReduction (pe, sum, pgs, sgs, pThr, pRat, sThr, sRat);
            }

            store (pe, spe, sum, pos);
            if (ls)
                storeGains (pgs, sgs);
        }

        float peakDb()      const noexcept { return lin2db (peakLin.load (std::memory_order_relaxed)); }
        float shortPeakDb() const noexcept { return lin2db (shortPeakLin.load (std::memory_order_relaxed)); }
        float rmsDb()       const noexcept
        {
            const float ms = rmsMeanSquared.load (std::memory_order_relaxed);
            return lin2db (ms > 0.0f ? std::sqrt (ms) : 0.0f);
        }

        // === Live Source Tamer ===
        void setLSParameters (bool active, float peakThresh, float pRatio,
                              float slowThresh, float sRatio) noexcept
        {
            peakThreshold.store (peakThresh, std::memory_order_relaxed);
            peakRatio.store     (pRatio,     std::memory_order_relaxed);
            slowThreshold.store (slowThresh, std::memory_order_relaxed);
            slowRatio.store     (sRatio,     std::memory_order_relaxed);
            lsActive.store      (active,     std::memory_order_relaxed);
            if (! active)
            {
                // Publish no-compression so the GR meter and the tamer engine read
                // unity while inactive (the per-sample GR loop is skipped).
                peakGR.store (1.0f, std::memory_order_relaxed);
                slowGR.store (1.0f, std::memory_order_relaxed);
            }
        }

        float peakGainReduction() const noexcept { return peakGR.load (std::memory_order_relaxed); }
        float slowGainReduction() const noexcept { return slowGR.load (std::memory_order_relaxed); }

    private:
        void store (float pe, float spe, float sum, int pos) noexcept
        {
            peakEnvelope      = pe;
            shortPeakEnvelope = spe;
            rmsSumSquared     = sum < 0.0f ? 0.0f : sum;   // guard slow FP drift below zero
            rmsWritePos       = pos;

            peakLin.store (pe, std::memory_order_relaxed);
            shortPeakLin.store (spe, std::memory_order_relaxed);
            rmsMeanSquared.store (rmsSumSquared / (float) rmsWindowSize, std::memory_order_relaxed);
        }

        static float lin2db (float v) noexcept
        {
            return v > 1.0e-10f ? 20.0f * std::log10 (v) : -200.0f;
        }

        // Advance the peak + slow gain smoothers one sample from the current
        // peak envelope and RMS sum. Mirrors LiveSourceLevelDetector::processSample.
        void updateGainReduction (float pe, float sum, float& pgs, float& sgs,
                                  float pThr, float pRat, float sThr, float sRat) const noexcept
        {
            const float peakDb  = lin2db (pe);
            const float pTarget = calculateGainReduction (peakDb, pThr, pRat);
            pgs = (pTarget < pgs) ? pTarget + peakGainAttackCoeff  * (pgs - pTarget)
                                  : pTarget + peakGainReleaseCoeff * (pgs - pTarget);

            const float rms     = sum > 0.0f ? std::sqrt (sum / (float) rmsWindowSize) : 0.0f;
            const float rmsDb   = lin2db (rms);
            const float sTarget = calculateGainReduction (rmsDb, sThr, sRat);
            sgs = (sTarget < sgs) ? sTarget + slowGainAttackCoeff  * (sgs - sTarget)
                                  : sTarget + slowGainReleaseCoeff * (sgs - sTarget);
        }

        void storeGains (float pgs, float sgs) noexcept
        {
            peakGainSmoothed = pgs;
            slowGainSmoothed = sgs;
            peakGR.store (pgs, std::memory_order_relaxed);
            slowGR.store (sgs, std::memory_order_relaxed);
        }

        // Soft-knee compression, copied verbatim from LiveSourceLevelDetector for
        // bit-for-bit parity with the CPU paths. Returns a linear multiplier 0..1.
        static float calculateGainReduction (float levelDb, float threshold, float ratio) noexcept
        {
            if (ratio <= 1.0f)
                return 1.0f;

            if (levelDb > threshold + 10.0f)
            {
                const float gainDb = (threshold - levelDb) * (ratio - 1.0f) / ratio;
                return std::pow (10.0f, gainDb / 20.0f);
            }
            if (levelDb > threshold - 10.0f)
            {
                const float kneePosition = levelDb - threshold + 10.0f;
                const float kneeGainDb   = (1.0f / ratio - 1.0f) * kneePosition * kneePosition / 40.0f;
                return std::pow (10.0f, kneeGainDb / 20.0f);
            }
            return 1.0f;
        }

        double sampleRate = 48000.0;

        float peakEnvelope = 0.0f, shortPeakEnvelope = 0.0f;
        float peakReleaseCoeff = 0.0f, shortPeakReleaseCoeff = 0.0f;

        std::vector<float> rmsBuffer;
        int   rmsWindowSize = 9600;
        int   rmsWritePos   = 0;
        float rmsSumSquared = 0.0f;

        // Linear envelopes published to the timer thread; dB on read.
        std::atomic<float> peakLin        { 0.0f };
        std::atomic<float> shortPeakLin   { 0.0f };
        std::atomic<float> rmsMeanSquared { 0.0f };

        // Live Source Tamer gain reduction (peak + slow). State is audio-thread
        // owned; params/output use atomics for the 50 Hz timer thread.
        float peakGainSmoothed = 1.0f, slowGainSmoothed = 1.0f;
        float peakGainAttackCoeff = 0.0f, peakGainReleaseCoeff = 0.0f;
        float slowGainAttackCoeff = 0.0f, slowGainReleaseCoeff = 0.0f;
        std::atomic<bool>  lsActive      { false };
        std::atomic<float> peakThreshold { -20.0f };
        std::atomic<float> peakRatio     { 2.0f };
        std::atomic<float> slowThreshold { -20.0f };
        std::atomic<float> slowRatio     { 2.0f };
        std::atomic<float> peakGR        { 1.0f };
        std::atomic<float> slowGR        { 1.0f };
    };

    std::vector<std::unique_ptr<Follower>> inputFollowers;
    std::vector<std::unique_ptr<Follower>> outputFollowers;
    std::atomic<bool> outputMeteringEnabled { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GpuLevelMeters)
};

} // namespace spatcore::gpu

// Extraction-compat alias — app code migrates to qualified names later.
using spatcore::gpu::GpuLevelMeters;

#endif // WFS_GPU_NATIVE
