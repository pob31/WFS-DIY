#pragma once

#include <JuceHeader.h>
#include "../Parameters/WFSParameterDefaults.h"
#include "../../spatcore/wfs/InputBufferAlgorithm.h"
#include "../../spatcore/wfs/OutputBufferAlgorithm.h"
#if WFS_GPU_NATIVE
 #include "../../spatcore/wfs/NativeGpuWfsAlgorithm.h"
 #include "../../spatcore/wfs/NativeGpuOutputBufferAlgorithm.h"
#endif
#include "../../spatcore/reverb/ReverbEngine.h"
#include "../../spatcore/reverb/ReverbFeedThread.h"
#include <vector>
#include <array>
#include <atomic>
#include <cmath>

/**
 * LevelMeteringManager
 *
 * Central coordinator for audio level metering data.
 * Manages enable/disable state and provides thread-safe level access for UI.
 *
 * Features:
 * - Enable/disable metering from map overlay or meter window
 * - Collect input/output levels from algorithms
 * - Thread performance data access
 * - Visual solo support (per-input contribution tracking)
 */
class LevelMeteringManager
{
public:
    // Processing algorithm enum (matches MainComponent)
    enum class ProcessingAlgorithm
    {
        InputBuffer,
        OutputBuffer
#if WFS_GPU_NATIVE
        , NativeGpuWfs
        , NativeGpuOutputBuffer
#endif
    };

    struct LevelData
    {
        float peakDb = -200.0f;
        float rmsDb = -200.0f;
    };

    struct ThreadPerformance
    {
        float cpuPercent = 0.0f;
        float microsecondsPerBlock = 0.0f;
    };

    /**
     * GPU pipeline telemetry snapshot (GPU host-path optimization M0),
     * sampled in updateLevels() on the message thread and read by the
     * LevelMeterWindow GPU strip at 20 Hz. All values come from
     * NON-destructive accessors — the destructive peak accessors
     * (getAndResetPeak*) stay exclusive to the 1/s underrun logger in
     * MainComponent::timerCallback (F4). The ui-peak fields are a rolling
     * ~3 s max over the sampled values: a sampled floor for the true
     * per-block peak (true peaks still reach the log on underrun).
     */
    struct GpuPipelineStats
    {
        // Direct-sound GPU pump (WFS gather or OB scatter, whichever is current)
        bool wfsLive = false;
        float wfsLastMs = 0.0f, wfsBudgetMs = 0.0f, wfsUiPeakMs = 0.0f;
        uint32_t wfsUnderruns = 0;
        int wfsDepthBlocks = 0;
        float wfsLatencyMs = 0.0f;

        // GPU reverb pump (own async pipeline; SDN/FDN/IR wrapper)
        bool revLive = false;
        float revLastMs = 0.0f, revBudgetMs = 0.0f, revUiPeakMs = 0.0f;
        uint32_t revUnderruns = 0;
        int revDepthBlocks = 0;
        float revLatencyMs = 0.0f;

        // ReverbFeedThread duty (per device block) + ReverbEngine duty
        // (per internal block) — always-on atomics, CPU-side threads.
        bool feedLive = false;
        float feedLastMs = 0.0f, feedBudgetMs = 0.0f, feedUiPeakMs = 0.0f, feedPct = 0.0f;
        bool engineLive = false;
        float engineLastMs = 0.0f, engineBudgetMs = 0.0f, engineUiPeakMs = 0.0f, enginePct = 0.0f;
    };

    LevelMeteringManager(int numInputs, int numOutputs)
        : numInputChannels(numInputs)
        , numOutputChannels(numOutputs)
    {
        inputLevels.resize(numInputs);
        outputLevels.resize(numOutputs);
        threadPerformance.resize(juce::jmax(numInputs, numOutputs));

        for (auto& a : hardwareInputPeakLin)
            a.store(0.0f, std::memory_order_relaxed);
        for (auto& a : renderSourcePeakLin)
            a.store(0.0f, std::memory_order_relaxed);
        for (auto& a : renderSourceMeanSq)
            a.store(0.0f, std::memory_order_relaxed);
    }

    // === Enable/Disable Control ===

    void setMapOverlayEnabled(bool enabled)
    {
        mapOverlayEnabled.store(enabled, std::memory_order_relaxed);
        updateAlgorithmMeteringFlags();
    }

    void setMeterWindowEnabled(bool enabled)
    {
        meterWindowEnabled.store(enabled, std::memory_order_relaxed);
        updateAlgorithmMeteringFlags();
    }

    bool isMapOverlayEnabled() const
    {
        return mapOverlayEnabled.load(std::memory_order_relaxed);
    }

    bool isMeterWindowEnabled() const
    {
        return meterWindowEnabled.load(std::memory_order_relaxed);
    }

    bool isMeteringActive() const
    {
        return mapOverlayEnabled.load(std::memory_order_relaxed) ||
               meterWindowEnabled.load(std::memory_order_relaxed);
    }

    // === Algorithm References ===
    // Call these after algorithms are prepared

    void setAlgorithms(InputBufferAlgorithm* inputAlg, OutputBufferAlgorithm* outputAlg)
    {
        inputAlgorithm = inputAlg;
        outputAlgorithm = outputAlg;
        updateAlgorithmMeteringFlags();
    }

#if WFS_GPU_NATIVE
    void setGpuAlgorithms(NativeGpuWfsAlgorithm* gpuWfsAlg,
                          NativeGpuOutputBufferAlgorithm* gpuObAlg)
    {
        gpuWfsAlgorithm = gpuWfsAlg;
        gpuObAlgorithm = gpuObAlg;
        updateAlgorithmMeteringFlags();
    }
#endif

    /**
     * Wire the reverb-side telemetry sources for the GPU pipeline strip.
     * Called by MainComponent after setupSharedInputFeed() (and re-called on
     * every feed-thread rebuild / teardown — either pointer may be null).
     * feedBudgetMsIn is the feed thread's per-batch budget (device block ms);
     * MainComponent owns block size + sample rate at the wiring site.
     * Message thread only (same thread as updateLevels()).
     */
    void setReverbSources(ReverbEngine* engine, ReverbFeedThread* feedThread,
                          float feedBudgetMsIn)
    {
        reverbEngine = engine;
        reverbFeedThread = feedThread;
        feedBudgetMs = feedBudgetMsIn;
    }

    void setCurrentAlgorithm(ProcessingAlgorithm alg)
    {
        currentAlgorithm = alg;
    }

    ProcessingAlgorithm getCurrentAlgorithm() const
    {
        return currentAlgorithm;
    }

    /** True when a native GPU direct-sound algorithm is current (drives the
        GPU pipeline strip visibility in the level meter window). */
    bool isGpuAlgorithmCurrent() const
    {
#if WFS_GPU_NATIVE
        return currentAlgorithm == ProcessingAlgorithm::NativeGpuWfs
            || currentAlgorithm == ProcessingAlgorithm::NativeGpuOutputBuffer;
#else
        return false;
#endif
    }

    /** True when ANY pipeline is on a GPU — the direct path, the reverb, or
        both. This, not isGpuAlgorithmCurrent(), is what governs whether the GPU
        pipeline strip has anything to say: the two backends are chosen
        independently, so GPU reverb with a CPU direct path is a normal (and on
        a second card, recommended) configuration.

        Reads the last sampled revLive, refreshed by updateGpuPipelineStats() at
        the metering tick rate. isGpuAlgorithmCurrent() remains the right test
        for whether the PER-CHANNEL CPU thread bars are meaningless. */
    bool isGpuStripRelevant() const
    {
        return isGpuAlgorithmCurrent() || gpuStats.revLive;
    }

    // === Level Updates ===
    // Call this from MainComponent::timerCallback at 20Hz

    void updateLevels()
    {
        if (!isMeteringActive())
            return;

        // NOTE: input levels are NOT read here. They come from the render-source
        // meter (refreshInputLevels), because the branches below are selected by
        // which algorithm is CONFIGURED, never by whether one is running — so
        // sourcing input levels from them made the meters die, or freeze at
        // their last reading, whenever the WFS engine was stopped.
        //
        // Outputs and thread performance genuinely ARE per-algorithm, so they
        // keep the switch — but they need the same not-running guard, or they
        // hold their last pre-stop reading for the same reason.
        if (! wfsProcessingActive)
        {
            for (auto& o : outputLevels)
                o = LevelData{};
            for (auto& t : threadPerformance)
                t = ThreadPerformance{};

            updateGpuPipelineStats();
            return;
        }

        if (currentAlgorithm == ProcessingAlgorithm::InputBuffer && inputAlgorithm != nullptr)
        {
            // Get output levels from InputBufferAlgorithm
            for (int i = 0; i < numOutputChannels && i < (int)outputLevels.size(); ++i)
            {
                outputLevels[i].peakDb = inputAlgorithm->getOutputPeakLevelDb(i);
                outputLevels[i].rmsDb = inputAlgorithm->getOutputRmsLevelDb(i);
            }

            // Get thread performance (one per input in InputBuffer mode)
            for (int i = 0; i < numInputChannels && i < (int)threadPerformance.size(); ++i)
            {
                threadPerformance[i].cpuPercent = inputAlgorithm->getCpuUsagePercent(i);
                threadPerformance[i].microsecondsPerBlock = inputAlgorithm->getProcessingTimeMicroseconds(i);
            }
        }
        else if (currentAlgorithm == ProcessingAlgorithm::OutputBuffer && outputAlgorithm != nullptr)
        {
            // Get output levels from OutputBufferAlgorithm
            for (int i = 0; i < numOutputChannels && i < (int)outputLevels.size(); ++i)
            {
                outputLevels[i].peakDb = outputAlgorithm->getOutputPeakLevelDb(i);
                outputLevels[i].rmsDb = outputAlgorithm->getOutputRmsLevelDb(i);
            }

            // Get thread performance (one per output in OutputBuffer mode)
            for (int i = 0; i < numOutputChannels && i < (int)threadPerformance.size(); ++i)
            {
                threadPerformance[i].cpuPercent = outputAlgorithm->getCpuUsagePercent(i);
                threadPerformance[i].microsecondsPerBlock = outputAlgorithm->getProcessingTimeMicroseconds(i);
            }
        }
#if WFS_GPU_NATIVE
        else if (currentAlgorithm == ProcessingAlgorithm::NativeGpuWfs && gpuWfsAlgorithm != nullptr)
        {
            updateLevelsFromGpu(*gpuWfsAlgorithm);
        }
        else if (currentAlgorithm == ProcessingAlgorithm::NativeGpuOutputBuffer && gpuObAlgorithm != nullptr)
        {
            updateLevelsFromGpu(*gpuObAlgorithm);
        }
#endif

        updateGpuPipelineStats();
    }

    /** Latest GPU pipeline telemetry (message thread; refreshed by
        updateLevels() while metering is active and a GPU algorithm is
        current — zeroed otherwise). */
    GpuPipelineStats getGpuPipelineStats() const
    {
        return gpuStats;
    }

    // === Level Accessors ===

    LevelData getInputLevel(int index) const
    {
        if (index >= 0 && index < (int)inputLevels.size())
            return inputLevels[index];
        return LevelData{};
    }

    LevelData getOutputLevel(int index) const
    {
        if (index >= 0 && index < (int)outputLevels.size())
            return outputLevels[index];
        return LevelData{};
    }

    int getNumInputChannels() const { return numInputChannels; }
    int getNumOutputChannels() const { return numOutputChannels; }

    // === Thread Performance Accessors ===

    ThreadPerformance getThreadPerformance(int index) const
    {
        if (index >= 0 && index < (int)threadPerformance.size())
            return threadPerformance[index];
        return ThreadPerformance{};
    }

    int getNumThreads() const
    {
        if (currentAlgorithm == ProcessingAlgorithm::InputBuffer)
            return numInputChannels;
#if WFS_GPU_NATIVE
        if (currentAlgorithm == ProcessingAlgorithm::NativeGpuWfs
            || currentAlgorithm == ProcessingAlgorithm::NativeGpuOutputBuffer)
            return 1;  // single GPU pump thread
#endif
        return numOutputChannels;
    }

    // === Visual Solo ===

    void setVisualSoloInput(int inputIndex)
    {
        visualSoloInput.store(inputIndex, std::memory_order_relaxed);
    }

    int getVisualSoloInput() const
    {
        return visualSoloInput.load(std::memory_order_relaxed);
    }

    /**
     * Get estimated contribution of an input to an output.
     * This is an approximation based on input level and routing level.
     *
     * @param inputIndex Input channel index
     * @param outputIndex Output channel index
     * @param routingLevel Linear routing level from WFSCalculationEngine (0-1)
     * @return Estimated contribution in dB
     */
    float getInputContributionToOutput(int inputIndex, int outputIndex, float routingLevel) const
    {
        juce::ignoreUnused(outputIndex);  // For future use with per-output routing analysis

        if (inputIndex < 0 || inputIndex >= (int)inputLevels.size())
            return -200.0f;

        float inputPeakDb = inputLevels[inputIndex].peakDb;

        // Convert routing level to dB and add to input level
        float routingLevelDb = (routingLevel > 1e-10f)
            ? 20.0f * std::log10(routingLevel)
            : -200.0f;

        return inputPeakDb + routingLevelDb;
    }

    // === Channel Count Updates ===

    /** Render sources feeding each visible channel's meter (index = channel,
        values = render-source indices). Absent or single-entry rows read the
        channel's own index directly. A stereo-pair channel lists its primary
        slot plus its 5 derived slice slots so the meter shows the whole
        channel, not slice 0. Message thread, config-time. */
    void setSourceMap(std::vector<std::vector<int>> aggregation)
    {
        sourceAggregation = std::move(aggregation);
    }

    void setChannelCounts(int inputs, int outputs)
    {
        numInputChannels = inputs;
        numOutputChannels = outputs;
        inputLevels.resize(inputs);
        outputLevels.resize(outputs);
        threadPerformance.resize(juce::jmax(inputs, outputs));
    }

    // === Hardware Input Signal-Presence Meter ===
    // Fed from the audio thread before the DSP gate so it works even when
    // WFS and binaural are both stopped. Read from the GUI thread for the
    // Input Patch header tint.

    // Upper bound for hardware input metering. Kept well above the WFS logical
    // input count (max 64) so large interfaces like the RME AoX-D (up to 512
    // channels) can meter and patch beyond 64 hardware inputs.
    static constexpr int MaxHardwareInputs = 512;

    /** Audio-thread writer. Pushes one block's peak for a single hardware
     *  input. Instantaneous rise, exponential release by `decayCoef`.
     *  Lock-free.
     *
     *  `decayCoef` is passed in rather than derived here: this runs once per
     *  hardware channel per block, and on a 512-channel interface computing it
     *  inline meant 512 std::exp plus 512 std::log10 every block for a value
     *  that depends only on the block size. Now: one exp in the caller, and the
     *  dB conversion deferred to the reader. */
    void pushHardwareInputBlockPeak(int ch, float blockMaxAbs, float decayCoef) noexcept
    {
        if (ch < 0 || ch >= MaxHardwareInputs)
            return;

        const float cur = hardwareInputPeakLin[ch].load(std::memory_order_relaxed);
        hardwareInputPeakLin[ch].store(
            blockMaxAbs >= cur ? blockMaxAbs
                               : blockMaxAbs + (cur - blockMaxAbs) * decayCoef,
            std::memory_order_relaxed);
    }

    /** Audio thread, once per block after the hardware loop. */
    void markHardwareMeterBlock() noexcept
    {
        hardwareMeterMs.store(juce::Time::getMillisecondCounter(),
                              std::memory_order_release);
    }

    /** GUI-thread reader. Returns -200 dB when channel is out of range.
     *
     *  Reports silence once the feed goes stale. Nothing ever reset these
     *  atomics, and only the audio callback writes them — so closing the audio
     *  device used to leave the Input Patch header tint lit at whatever the
     *  last block held, permanently. */
    float getHardwareInputPeakDb(int ch) const noexcept
    {
        if (ch < 0 || ch >= MaxHardwareInputs || ! isMeterFresh(hardwareMeterMs))
            return -200.0f;
        return linearToDb(hardwareInputPeakLin[ch].load(std::memory_order_relaxed));
    }

    // === Render-Source Input Meter ===
    // The display meter for the WFS inputs, fed from the audio thread at the
    // one point both engine paths share.
    //
    // Input levels used to come from whichever WFS ALGORITHM was configured,
    // which meant they died whenever that algorithm was not running — most
    // visibly in binaural-only monitoring, where audio is plainly flowing but
    // no LiveSourceLevelDetector ever sees a sample. Worse, the algorithm's
    // detectors kept their last pre-stop reading, so the meters froze rather
    // than falling, and silence and no-feed looked identical.
    //
    // An input level is a property of the INPUT, not of anything downstream, so
    // it is measured on patchedInputBuffer — post-patch, post-sampler,
    // post-stereo-decomposition — which is populated whether the WFS engine,
    // the binaural renderer, or both are running. Same samples the algorithms'
    // detectors see, one stage earlier; what differs is the envelope, since
    // those detectors exist for the Live Source Tamer and carry a compressor's
    // ballistics rather than a display meter's.

    static constexpr int MaxRenderSources = WFSParameterDefaults::maxRenderSources;

    /** Coefficient for one block of exponential decay toward a target.
     *  Depends only on (numSamples, sampleRate, tau) — compute it ONCE per
     *  block in the caller, not once per channel: this is the only
     *  transcendental on the metering path. */
    static float blockDecayCoef(int numSamples, double sampleRate, double tauSeconds) noexcept
    {
        if (numSamples <= 0 || sampleRate <= 0.0 || tauSeconds <= 0.0)
            return 0.0f;
        return (float) std::exp(-(double) numSamples / (sampleRate * tauSeconds));
    }

    /** Time constant for both input-meter paths, chosen to match what the
     *  algorithms' detectors used to show:
     *   - peak had a 100 ms release, so the peak path uses the same;
     *   - RMS was a true 200 ms sliding rectangular window. A one-pole matches
     *     a rectangular window of length T in smoothing at tau = T/2, and
     *     converges to the identical steady-state value, so 100 ms it is. */
    static constexpr double kInputMeterTauSeconds = 0.100;

    /** Audio-thread writer, once per render source per block.
     *
     *  Publishes LINEAR peak and LINEAR mean-square; dB conversion happens on
     *  the message thread in refreshInputLevels(). Same split GpuLevelMeters
     *  uses, and for the same reason: no logarithms on the audio thread.
     *
     *  Peak rises instantly and decays by `decayCoef` — a meter's ballistics.
     *  Mean-square uses the SAME coefficient in BOTH directions. That symmetry
     *  is the point: an instant-attack RMS would leap to near the peak value on
     *  every transient (a snare inside a 64-sample block can measure 20 dB above
     *  its 200 ms window RMS), which is exactly the calm-RMS regression this is
     *  replacing a sliding window to avoid. */
    void pushRenderSourceBlockLevels(int src, float blockMaxAbs, float blockMeanSquare,
                                     float decayCoef) noexcept
    {
        if (src < 0 || src >= MaxRenderSources)
            return;

        const float curPeak = renderSourcePeakLin[src].load(std::memory_order_relaxed);
        renderSourcePeakLin[src].store(
            blockMaxAbs >= curPeak ? blockMaxAbs
                                   : blockMaxAbs + (curPeak - blockMaxAbs) * decayCoef,
            std::memory_order_relaxed);

        const float curMs = renderSourceMeanSq[src].load(std::memory_order_relaxed);
        renderSourceMeanSq[src].store(
            blockMeanSquare + (curMs - blockMeanSquare) * decayCoef,
            std::memory_order_relaxed);
    }

    /** Audio thread, once per block AFTER the render-source loop. One stamp for
     *  the whole tap — the loop writes every source together, so per-source
     *  stamps would carry no extra information and cost 104 more stores. */
    void markRenderSourceMeterBlock() noexcept
    {
        renderSourceMeterMs.store(juce::Time::getMillisecondCounter(),
                                  std::memory_order_release);
    }

    /** Message thread, every tick: collapse the render-source meters onto the
     *  visible channels.
     *
     *  Deliberately NOT gated on isMeteringActive(): AutomOtion's audio trigger
     *  reads these levels too, and it has to keep working with every meter
     *  closed. The work is a handful of atomic loads per channel. */
    void refreshInputLevels()
    {
        // Nothing has pushed recently: no audio device, or nothing rendering.
        // Report silence rather than holding the last reading — a frozen meter
        // is what made a stopped engine look like a running one, and it is the
        // half of this bug that does not announce itself.
        //
        // Substituted at READ time, never by decaying the atomics from here:
        // the audio thread owns that state and a second writer could swallow an
        // attack or resurrect a stale value.
        if (! isMeterFresh(renderSourceMeterMs))
        {
            for (auto& l : inputLevels)
                l = LevelData{};
            return;
        }

        fillAggregatedInputLevels(
            [this](int s) { return s >= 0 && s < MaxRenderSources
                ? linearToDb(renderSourcePeakLin[s].load(std::memory_order_relaxed))
                : -200.0f; },
            [this](int s) { return s >= 0 && s < MaxRenderSources
                ? meanSquareToDb(renderSourceMeanSq[s].load(std::memory_order_relaxed))
                : -200.0f; });
    }

    /** Message thread: is the WFS engine actually rendering?
     *
     *  Only OUTPUT levels and thread performance need this. Input levels do
     *  not — they come from the render-source tap, which runs on both audio
     *  paths. Outputs genuinely have no source when no algorithm runs, and
     *  without this they would freeze at their last pre-stop reading. */
    void setWfsProcessingActive(bool active) noexcept
    {
        wfsProcessingActive = active;
    }

private:
    // Long enough that even an 8192-frame buffer at 44.1 kHz (186 ms) never
    // trips it, short enough that a stopped engine reads as silence before
    // anyone notices the meters sticking.
    static constexpr juce::uint32 kMeterStaleMs = 250;

    bool isMeterFresh(const std::atomic<juce::uint32>& stampMs) const noexcept
    {
        return juce::Time::getMillisecondCounter()
                 - stampMs.load(std::memory_order_acquire) < kMeterStaleMs;
    }

    /** dB from a linear amplitude, with a floor instead of -inf. */
    static float linearToDb(float amplitude) noexcept
    {
        return amplitude > 1.0e-10f ? 20.0f * std::log10(amplitude) : -200.0f;
    }

    /** dB from a mean of squares. 10*log10(ms) == 20*log10(sqrt(ms)) without
     *  the square root. */
    static float meanSquareToDb(float meanSquare) noexcept
    {
        return meanSquare > 1.0e-20f ? 10.0f * std::log10(meanSquare) : -200.0f;
    }

    void updateAlgorithmMeteringFlags()
    {
        bool active = isMeteringActive();

        if (inputAlgorithm != nullptr)
            inputAlgorithm->setOutputMeteringEnabled(active);

        if (outputAlgorithm != nullptr)
            outputAlgorithm->setOutputMeteringEnabled(active);

#if WFS_GPU_NATIVE
        if (gpuWfsAlgorithm != nullptr)
            gpuWfsAlgorithm->setOutputMeteringEnabled(active);

        if (gpuObAlgorithm != nullptr)
            gpuObAlgorithm->setOutputMeteringEnabled(active);
#endif
    }

    // Fill inputLevels per visible channel, aggregating across the channel's
    // render sources when it has more than one (max peak, energy-sum RMS).
    template <typename PeakFn, typename RmsFn>
    void fillAggregatedInputLevels(PeakFn&& getPeakDb, RmsFn&& getRmsDb)
    {
        for (int ch = 0; ch < numInputChannels && ch < (int)inputLevels.size(); ++ch)
        {
            if (ch < (int)sourceAggregation.size() && sourceAggregation[ch].size() > 1)
            {
                // -200, not -100: the single-source branch below passes the
                // accessor's own floor straight through, and a stereo channel
                // that could never read below -100 while a mono one reads -200
                // is two different scales under one glyph.
                float peakDb = -200.0f;
                double rmsEnergy = 0.0;
                for (int src : sourceAggregation[ch])
                {
                    peakDb = juce::jmax(peakDb, getPeakDb(src));
                    rmsEnergy += std::pow(10.0, (double)getRmsDb(src) / 10.0);
                }
                inputLevels[ch].peakDb = peakDb;
                inputLevels[ch].rmsDb = rmsEnergy > 0.0
                    ? (float)(10.0 * std::log10(rmsEnergy)) : -200.0f;
            }
            else
            {
                inputLevels[ch].peakDb = getPeakDb(ch);
                inputLevels[ch].rmsDb = getRmsDb(ch);
            }
        }
    }

#if WFS_GPU_NATIVE
    // Both native GPU algorithms expose the same host-side metering interface
    // (getInputPeakLevelDb / getInputRmsLevelDb / getOutputPeakLevelDb /
    // getOutputRmsLevelDb), so a small template serves both. The GPU paths run a
    // single pump thread, so per-channel thread-performance data is not meaningful.
    template <typename GpuAlgorithm>
    void updateLevelsFromGpu(const GpuAlgorithm& algorithm)
    {
        // Outputs only — input levels come from the render-source meter.
        for (int i = 0; i < numOutputChannels && i < (int)outputLevels.size(); ++i)
        {
            outputLevels[i].peakDb = algorithm.getOutputPeakLevelDb(static_cast<size_t>(i));
            outputLevels[i].rmsDb  = algorithm.getOutputRmsLevelDb(static_cast<size_t>(i));
        }

        // Single GPU pump thread — clear the (CPU-oriented) per-thread display.
        if (! threadPerformance.empty())
            threadPerformance[0] = ThreadPerformance{};
    }
#endif

    //==========================================================================
    // GPU pipeline telemetry sampling (message thread, from updateLevels()).
    //==========================================================================

    /** Rolling ~3 s peak-hold over sampled values (documented as a sampled
        floor of the true per-block peak — see GpuPipelineStats). */
    struct UiPeakHold
    {
        static constexpr double kHoldMs = 3000.0;
        float peak = 0.0f;
        double heldAtMs = 0.0;

        float update(float v, double nowMs) noexcept
        {
            if (v >= peak || nowMs - heldAtMs > kHoldMs)
            {
                peak = v;
                heldAtMs = nowMs;
            }
            return peak;
        }
    };

    void updateGpuPipelineStats()
    {
#if WFS_GPU_NATIVE
        const double nowMs = juce::Time::getMillisecondCounterHiRes();
        GpuPipelineStats s = gpuStats;   // rev* fields carry over when the
                                         // engine's try-lock sample is skipped

        // The direct path and the reverb select their backends INDEPENDENTLY:
        // GPU reverb alongside a CPU direct path is a normal configuration (and
        // a recommended one on a second card). So the reverb pump is sampled
        // unconditionally below, and only the direct-pump sampling is gated on
        // the direct path actually being on the GPU. Gating the whole function
        // on isGpuAlgorithmCurrent() is what used to hide GPU reverb telemetry
        // entirely whenever the main algorithm ran on the CPU.
        s.wfsLive = false;
        if (currentAlgorithm == ProcessingAlgorithm::NativeGpuWfs && gpuWfsAlgorithm != nullptr)
            sampleDirectPump(*gpuWfsAlgorithm, s, nowMs);
        else if (currentAlgorithm == ProcessingAlgorithm::NativeGpuOutputBuffer && gpuObAlgorithm != nullptr)
            sampleDirectPump(*gpuObAlgorithm, s, nowMs);

        // Reverb pump: non-destructive engine sample (try-lock — on contention
        // keep the previous rev* values for this tick).
        if (reverbEngine != nullptr)
        {
            ReverbEngine::ReverbGpuLiveSample rs;
            if (reverbEngine->sampleReverbGpuLive(rs))
            {
                s.revLive = rs.live;
                s.revLastMs = rs.lastPumpMs;
                s.revBudgetMs = rs.budgetMs;
                s.revUnderruns = rs.underruns;
                s.revDepthBlocks = rs.depthBlocks;
                s.revLatencyMs = rs.latencyMs;
            }
            if (s.revLive)
                s.revUiPeakMs = revUiPeak.update(s.revLastMs, nowMs);

            // Engine duty (always-on relaxed atomics)
            s.engineLive = reverbEngine->isActive();
            s.engineLastMs = reverbEngine->getLastEngineBlockUs() * 0.001f;
            s.engineBudgetMs = reverbEngine->getEngineBudgetUs() * 0.001f;
            s.enginePct = s.engineBudgetMs > 0.0f
                              ? 100.0f * s.engineLastMs / s.engineBudgetMs : 0.0f;
            s.engineUiPeakMs = engineUiPeak.update(s.engineLastMs, nowMs);
        }
        else
        {
            s.revLive = false;
            s.engineLive = false;
        }

        // Feed thread duty (always-on relaxed atomics; budget wired by
        // MainComponent = device block ms).
        if (reverbFeedThread != nullptr)
        {
            s.feedLive = true;
            s.feedLastMs = reverbFeedThread->getLastBatchUs() * 0.001f;
            s.feedBudgetMs = feedBudgetMs;
            s.feedPct = feedBudgetMs > 0.0f ? 100.0f * s.feedLastMs / feedBudgetMs : 0.0f;
            s.feedUiPeakMs = feedUiPeak.update(s.feedLastMs, nowMs);
        }
        else
        {
            s.feedLive = false;
        }

        // Nothing on a GPU at all: clear, so the strip never shows stale numbers
        // after a switch back to a fully-CPU configuration.
        if (! isGpuAlgorithmCurrent() && ! s.revLive)
        {
            gpuStats = GpuPipelineStats{};
            return;
        }

        gpuStats = s;
#else
        gpuStats = GpuPipelineStats{};
#endif
    }

#if WFS_GPU_NATIVE
    // NativeGpuWfsAlgorithm and NativeGpuOutputBufferAlgorithm expose the same
    // pump telemetry surface but are unrelated types (same pattern as
    // updateLevelsFromGpu above).
    template <typename GpuAlgorithm>
    void sampleDirectPump(const GpuAlgorithm& alg, GpuPipelineStats& s, double nowMs)
    {
        if (!alg.isReady())
            return;
        s.wfsLive = true;
        s.wfsLastMs = alg.getLastGpuExecMs();          // = pipeline lastPumpMs, non-destructive
        s.wfsUnderruns = alg.getUnderrunCount();
        s.wfsDepthBlocks = alg.getPipelineDepthBlocks();
        s.wfsLatencyMs = (float)alg.getPipelineLatencyMs();
        s.wfsBudgetMs = s.wfsDepthBlocks > 0 ? s.wfsLatencyMs / (float)s.wfsDepthBlocks : 0.0f;
        s.wfsUiPeakMs = wfsUiPeak.update(s.wfsLastMs, nowMs);
    }
#endif

    // Algorithm references (not owned)
    InputBufferAlgorithm* inputAlgorithm = nullptr;
    OutputBufferAlgorithm* outputAlgorithm = nullptr;
#if WFS_GPU_NATIVE
    NativeGpuWfsAlgorithm* gpuWfsAlgorithm = nullptr;
    NativeGpuOutputBufferAlgorithm* gpuObAlgorithm = nullptr;
#endif
    ProcessingAlgorithm currentAlgorithm = ProcessingAlgorithm::InputBuffer;

    // Reverb telemetry sources (not owned; message-thread wiring — see
    // setReverbSources)
    ReverbEngine* reverbEngine = nullptr;
    ReverbFeedThread* reverbFeedThread = nullptr;
    float feedBudgetMs = 0.0f;

    // GPU pipeline strip state (message thread only)
    GpuPipelineStats gpuStats;
    UiPeakHold wfsUiPeak, revUiPeak, feedUiPeak, engineUiPeak;

    // Channel counts
    int numInputChannels = 0;
    int numOutputChannels = 0;

    // Enable flags
    std::atomic<bool> mapOverlayEnabled{false};
    std::atomic<bool> meterWindowEnabled{false};

    // Cached level data (updated at 20Hz from timer thread)
    std::vector<LevelData> inputLevels;
    std::vector<std::vector<int>> sourceAggregation;  // per-channel render sources (see setSourceMap)
    std::vector<LevelData> outputLevels;
    std::vector<ThreadPerformance> threadPerformance;

    // Visual solo
    std::atomic<int> visualSoloInput{-1};

    // Per-hardware-input LINEAR peak, written by audio thread pre-DSP-gate,
    // read (and converted to dB) by Input Patch header paint. Fixed capacity
    // avoids any resize (std::atomic is not movable).
    std::array<std::atomic<float>, MaxHardwareInputs> hardwareInputPeakLin;

    // Per-render-source LINEAR peak and mean-square — the input meters proper.
    // Written by the audio thread from patchedInputBuffer, converted to dB and
    // collapsed onto channels by refreshInputLevels(). Same no-resize reasoning
    // as above.
    std::array<std::atomic<float>, MaxRenderSources> renderSourcePeakLin;
    std::array<std::atomic<float>, MaxRenderSources> renderSourceMeanSq;

    // Freshness stamps, one per tap, written once per block by the audio thread.
    std::atomic<juce::uint32> renderSourceMeterMs { 0 };
    std::atomic<juce::uint32> hardwareMeterMs { 0 };

    // Message thread only: gates OUTPUT levels and thread performance, which
    // have no source when no algorithm is running.
    bool wfsProcessingActive = false;
};
