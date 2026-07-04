#pragma once

#include <JuceHeader.h>
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

        for (auto& a : hardwareInputPeakDb)
            a.store(-200.0f, std::memory_order_relaxed);
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

    // === Level Updates ===
    // Call this from MainComponent::timerCallback at 20Hz

    void updateLevels()
    {
        if (!isMeteringActive())
            return;

        if (currentAlgorithm == ProcessingAlgorithm::InputBuffer && inputAlgorithm != nullptr)
        {
            // Get input levels from InputBufferAlgorithm
            for (int i = 0; i < numInputChannels && i < (int)inputLevels.size(); ++i)
            {
                inputLevels[i].peakDb = inputAlgorithm->getInputPeakLevelDb(i);
                inputLevels[i].rmsDb = inputAlgorithm->getInputRmsLevelDb(i);
            }

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
            // Get input levels from OutputBufferAlgorithm
            for (int i = 0; i < numInputChannels && i < (int)inputLevels.size(); ++i)
            {
                inputLevels[i].peakDb = outputAlgorithm->getInputPeakLevelDb(i);
                inputLevels[i].rmsDb = outputAlgorithm->getInputRmsLevelDb(i);
            }

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
     *  input. Instantaneous rise, ~150 ms exponential release. Lock-free. */
    void pushHardwareInputBlockPeak(int ch, float blockMaxAbs,
                                    int numSamples, double sampleRate) noexcept
    {
        if (ch < 0 || ch >= MaxHardwareInputs || numSamples <= 0 || sampleRate <= 0.0)
            return;

        const float newDb = (blockMaxAbs > 1.0e-10f)
                                ? 20.0f * std::log10(blockMaxAbs)
                                : -200.0f;

        const float cur = hardwareInputPeakDb[ch].load(std::memory_order_relaxed);
        float out;
        if (newDb >= cur)
        {
            out = newDb;
        }
        else
        {
            const double releaseSeconds = 0.150;
            const float coef = (float) std::exp(-(double) numSamples / (sampleRate * releaseSeconds));
            out = newDb + (cur - newDb) * coef;
        }

        hardwareInputPeakDb[ch].store(out, std::memory_order_relaxed);
    }

    /** GUI-thread reader. Returns -200 dB when channel is out of range. */
    float getHardwareInputPeakDb(int ch) const noexcept
    {
        if (ch < 0 || ch >= MaxHardwareInputs)
            return -200.0f;
        return hardwareInputPeakDb[ch].load(std::memory_order_relaxed);
    }

private:
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

#if WFS_GPU_NATIVE
    // Both native GPU algorithms expose the same host-side metering interface
    // (getInputPeakLevelDb / getInputRmsLevelDb / getOutputPeakLevelDb /
    // getOutputRmsLevelDb), so a small template serves both. The GPU paths run a
    // single pump thread, so per-channel thread-performance data is not meaningful.
    template <typename GpuAlgorithm>
    void updateLevelsFromGpu(const GpuAlgorithm& algorithm)
    {
        for (int i = 0; i < numInputChannels && i < (int)inputLevels.size(); ++i)
        {
            inputLevels[i].peakDb = algorithm.getInputPeakLevelDb(static_cast<size_t>(i));
            inputLevels[i].rmsDb  = algorithm.getInputRmsLevelDb(static_cast<size_t>(i));
        }

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
        if (!isGpuAlgorithmCurrent())
        {
            gpuStats = GpuPipelineStats{};   // strip hidden; don't show stale numbers
            return;
        }

#if WFS_GPU_NATIVE
        const double nowMs = juce::Time::getMillisecondCounterHiRes();
        GpuPipelineStats s = gpuStats;   // rev* fields carry over when the
                                         // engine's try-lock sample is skipped
        // Direct-sound pump: whichever GPU algorithm is current. Budget is
        // recovered exactly from the pipeline's own numbers
        // (latency = depth * blockMs => blockMs = latency / depth).
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

        gpuStats = s;
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
    std::vector<LevelData> outputLevels;
    std::vector<ThreadPerformance> threadPerformance;

    // Visual solo
    std::atomic<int> visualSoloInput{-1};

    // Per-hardware-input peak dB, written by audio thread pre-DSP-gate,
    // read by Input Patch header paint. Fixed capacity avoids any resize
    // (std::atomic is not movable).
    std::array<std::atomic<float>, MaxHardwareInputs> hardwareInputPeakDb;
};
