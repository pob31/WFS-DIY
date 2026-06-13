#pragma once
#if WFS_GPU_NATIVE

#include <JuceHeader.h>
#include <atomic>
#include <vector>
#include <memory>
#include <cmath>

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
        }

        // Per-sample work is transcendental-free; dB is computed on read.
        void process (const float* data, int numSamples) noexcept
        {
            float pe  = peakEnvelope;
            float spe = shortPeakEnvelope;
            float sum = rmsSumSquared;
            int   pos = rmsWritePos;

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
            }

            store (pe, spe, sum, pos);
        }

        void processSilence (int numSamples) noexcept
        {
            float pe  = peakEnvelope;
            float spe = shortPeakEnvelope;
            float sum = rmsSumSquared;
            int   pos = rmsWritePos;

            for (int i = 0; i < numSamples; ++i)
            {
                pe  *= peakReleaseCoeff;
                spe *= shortPeakReleaseCoeff;
                sum -= rmsBuffer[(size_t) pos];
                rmsBuffer[(size_t) pos] = 0.0f;
                pos = (pos + 1) % rmsWindowSize;
            }

            store (pe, spe, sum, pos);
        }

        float peakDb()      const noexcept { return lin2db (peakLin.load (std::memory_order_relaxed)); }
        float shortPeakDb() const noexcept { return lin2db (shortPeakLin.load (std::memory_order_relaxed)); }
        float rmsDb()       const noexcept
        {
            const float ms = rmsMeanSquared.load (std::memory_order_relaxed);
            return lin2db (ms > 0.0f ? std::sqrt (ms) : 0.0f);
        }

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
    };

    std::vector<std::unique_ptr<Follower>> inputFollowers;
    std::vector<std::unique_ptr<Follower>> outputFollowers;
    std::atomic<bool> outputMeteringEnabled { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GpuLevelMeters)
};

#endif // WFS_GPU_NATIVE
