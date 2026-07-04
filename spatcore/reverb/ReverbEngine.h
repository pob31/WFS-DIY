#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "../rt/LockFreeRingBuffer.h"
#include "ReverbAlgorithm.h"
#include "ReverbFDNAlgorithm.h"
#include "ReverbSDNAlgorithm.h"
#include "ReverbIRAlgorithm.h"
#if WFS_GPU_NATIVE
#include "ReverbIRAlgorithmGPU.h"
#include "ReverbFDNAlgorithmGPU.h"
#include "ReverbSDNAlgorithmGPU.h"
#endif
#include "../rt/ReverbDiagnostics.h"
#include "ReverbPreProcessor.h"
#include "ReverbPostProcessor.h"
#include "../rt/AudioParallelFor.h"
#include <atomic>
#include <chrono>   // always-on duty telemetry (M0); also used by REVERB_DIAGNOSTICS
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace spatcore::reverb {

//==============================================================================
/**
    ReverbEngine

    Thread-based reverb processor for the WFS system.
    Processes per-node reverb audio through a pre-processing → algorithm → post-processing chain.

    Integration pattern (mirrors BinauralProcessor):
    - Audio callback pushes per-node feed audio via pushNodeInput()
    - Audio callback pulls per-node wet output via pullNodeOutput()
    - Timer callback pushes parameters at 50Hz via setter methods
    - Engine runs on its own high-priority thread

    Supports three algorithms: SDN (0), FDN (1), IR (2).
    Algorithm switching creates a new instance and replaces the active one.
*/
class ReverbEngine : public juce::Thread
{
public:
    ReverbEngine()
        : juce::Thread ("ReverbEngine")
    {
    }

    ~ReverbEngine() override
    {
        stopThread (1000);
    }

    /** Optional audio workgroup, joined by the reverb thread and its fork-join pool.
        Set before prepareToPlay()/startProcessing(). */
    void setWorkgroupCoordinator (AudioWorkgroupCoordinator* c) { workgroupCoordinator = c; }

    //==========================================================================
    // Lifecycle
    //==========================================================================

    /**
        Prepare for playback. Allocate all ring buffers and working buffers.
        Must be called before startProcessing().
    */
    void prepareToPlay (double newSampleRate, int maxBlockSize, int numNodes)
    {
        sampleRate = newSampleRate;
        currentBlockSize = maxBlockSize;
        numReverbNodes = numNodes;

        // Run the reverb at >= 256-sample internal blocks regardless of the
        // sound-card buffer. The reverb is a wet send and not latency-critical,
        // so decoupling it from the (small) device block gives the GPU reverb a
        // full 256-sample budget (5.33 ms @ 48k) even when the card runs at 64 —
        // the rings below accumulate device blocks into internal blocks. Capped
        // at 1024 to bound the added wet latency at very large device buffers.
        internalBlockSize = juce::jlimit (256, 1024, maxBlockSize);

        // Per-node ring buffers: 32x the LARGER of the device/internal block, to
        // absorb convolution FFT spikes and the device<->internal size mismatch.
        int ringSize = juce::jmax (maxBlockSize, internalBlockSize) * 32;

        nodeInputBuffers.clear();
        nodeOutputBuffers.clear();

        for (int i = 0; i < numNodes; ++i)
        {
            nodeInputBuffers.push_back (std::make_unique<LockFreeRingBuffer>());
            nodeInputBuffers.back()->setSize (ringSize);

            nodeOutputBuffers.push_back (std::make_unique<LockFreeRingBuffer>());
            nodeOutputBuffers.back()->setSize (ringSize);
        }

        // Pre-fill output ring buffers with silence for an initial cushion that
        // absorbs engine-thread scheduling jitter. Bounded to ~16 ms (with a
        // floor of one full internal block) so it never balloons the wet-path
        // latency or overflows the ring as internalBlockSize grows — the floor
        // also guards the integer division from flooring to 0 at large blocks.
        {
            std::vector<float> silence (static_cast<size_t> (internalBlockSize), 0.0f);
            const int cushionSamples = juce::jmax (internalBlockSize,
                                                   static_cast<int> (sampleRate * 0.016));
            const int prefillBlocks = juce::jmax (1, cushionSamples / internalBlockSize);
            for (auto& buf : nodeOutputBuffers)
                for (int b = 0; b < prefillBlocks; ++b)
                    buf->write (silence.data(), internalBlockSize);
        }

        // Working buffers for internal processing
        nodeInputBlock.setSize (numNodes, internalBlockSize);
        nodeOutputBlock.setSize (numNodes, internalBlockSize);

        // Prepare pre/post processors
        preProcessor.prepare (sampleRate, internalBlockSize, numNodes);
        postProcessor.prepare (sampleRate, internalBlockSize, numNodes);
        sidechainLevels.assign (static_cast<size_t> (numNodes), 0.0f);

        // Prepare thread pool for parallel per-node processing
        {
            int hwThreads = static_cast<int> (std::thread::hardware_concurrency());
            int maxWorkers = juce::jmin (hwThreads - 2, numNodes - 1);
            maxWorkers = juce::jlimit (0, 7, maxWorkers);
            double blockMs = sampleRate > 0.0 ? (internalBlockSize / sampleRate) * 1000.0 : 0.0;
            parallelPool.prepare (maxWorkers, blockMs, blockMs, workgroupCoordinator);
        }

        // Prepare the active algorithm if one exists
        if (algorithm)
        {
            algorithm->prepare (sampleRate, internalBlockSize, numNodes);
            algorithm->setParallelFor (&parallelPool);
        }
    }

    /**
        Release all resources. Stops the thread first.
    */
    void releaseResources()
    {
        stopThread (1000);
        parallelPool.shutdown();
        nodeInputBuffers.clear();
        nodeOutputBuffers.clear();
    }

    /**
        Start the processing thread.
    */
    void startProcessing()
    {
        if (! isThreadRunning())
            startRealtimeThread (juce::Thread::RealtimeOptions{}
                                     .withApproximateAudioProcessingTime (internalBlockSize, sampleRate));
    }

    /**
        Stop the processing thread.
    */
    void stopProcessing()
    {
        stopThread (1000);
    }

    /**
        Reset all internal state to silence.
    */
    void reset()
    {
        for (auto& buf : nodeInputBuffers)
            buf->reset();
        for (auto& buf : nodeOutputBuffers)
            buf->reset();

        if (algorithm)
            algorithm->reset();

        preProcessor.reset();
        postProcessor.reset();
    }

    //==========================================================================
    // Audio Callback Interface (called from audio thread)
    //==========================================================================

    /**
        Push feed audio for a reverb node. Called from the audio callback.
        @param nodeIndex    Reverb node index (0-based)
        @param data         Input samples
        @param numSamples   Number of samples
    */
    void pushNodeInput (int nodeIndex, const float* data, int numSamples)
    {
        if (nodeIndex >= 0 && nodeIndex < (int) nodeInputBuffers.size())
            nodeInputBuffers[nodeIndex]->write (data, numSamples);
        notify();  // Wake reverb worker thread immediately (immune to timer coalescing)
    }

    /**
        Pull wet output for a reverb node. Called from the audio callback.
        Zero-pads if not enough data is available (prevents glitches on underrun).
        @param nodeIndex    Reverb node index (0-based)
        @param dest         Destination buffer
        @param numSamples   Number of samples
    */
    void pullNodeOutput (int nodeIndex, float* dest, int numSamples)
    {
        if (nodeIndex >= 0 && nodeIndex < (int) nodeOutputBuffers.size())
        {
            int samplesRead = nodeOutputBuffers[nodeIndex]->read (dest, numSamples);

            // Zero-pad if not enough data (underrun)
            if (samplesRead < numSamples)
            {
                juce::FloatVectorOperations::clear (dest + samplesRead, numSamples - samplesRead);
                dropoutCount.fetch_add (1, std::memory_order_relaxed);
            }
        }
        else
        {
            juce::FloatVectorOperations::clear (dest, numSamples);
        }
    }

    //==========================================================================
    // Parameter Setters (called from timer thread at 50Hz)
    //==========================================================================

    /** Set algorithm parameters (RT60, diffusion, size, wet level, etc.) */
    void setAlgorithmParameters (const AlgorithmParameters& params)
    {
        // Store locally for thread-safe access
        {
            juce::SpinLock::ScopedLockType lock (pendingParamsLock);
            pendingParams = params;
        }
        paramsChanged.store (true, std::memory_order_release);
    }

    /** Update node positions (for SDN geometry calculations) */
    void updateGeometry (const std::vector<NodePosition>& positions)
    {
        juce::SpinLock::ScopedLockType lock (geometryLock);
        pendingGeometry = positions;
        geometryChanged.store (true, std::memory_order_release);
    }

    /** Set the active algorithm instance. Ownership is transferred. */
    void setAlgorithm (std::unique_ptr<ReverbAlgorithm> newAlgorithm)
    {
        juce::SpinLock::ScopedLockType lock (algorithmLock);
        algorithm = std::move (newAlgorithm);

        if (algorithm && sampleRate > 0)
        {
            algorithm->prepare (sampleRate, internalBlockSize, numReverbNodes);
            algorithm->setParallelFor (&parallelPool);
            attachIRDiagnostics();
        }
    }

    /** Algorithm type contract — consumers map their algorithm parameter to these values. */
    enum AlgorithmType { SDN = 0, FDN = 1, IR = 2 };

    /**
        Set algorithm type by ID. Initiates a fade-out → switch → fade-in sequence.
        @param type  0=SDN, 1=FDN, 2=IR
    */
    void setAlgorithmType (int type)
    {
        if (type == currentAlgorithmType)
            return;

        // Store the pending type and initiate fade-out
        pendingAlgorithmType.store (type, std::memory_order_release);
        if (fadeState.load (std::memory_order_acquire) == FadeNone)
            fadeState.store (FadingOut, std::memory_order_release);
    }

    /** Get the current algorithm type. */
    int getAlgorithmType() const { return currentAlgorithmType; }

    //==========================================================================
    // Reverb GPU backend selection (per-algorithm GPU/CPU toggle)
    //==========================================================================

    /** GPU backend status for the active reverb algorithm, for the UI
        (see getReverbGpuStatus()). Algorithm-agnostic — IR and FDN both
        report through it (only one algorithm is active at a time). */
    struct ReverbGpuStatus
    {
        enum Mode { Cpu = 0, GpuActive, GpuFallback };
        Mode mode = Cpu;
        juce::String device;      // GPU device name when GpuActive
        juce::String error;       // failure reason when GpuFallback
        double latencyMs = 0.0;   // wet-path pipeline latency when GpuActive
    };

    /** Select the IR convolution backend (timer thread, 50 Hz). When the IR
        algorithm is active, switches via the same fade-to-silence path as an
        algorithm change; otherwise just records the choice for the next IR
        instantiation. If the GPU backend can't initialise, the engine falls
        back to CPU and reports it via getReverbGpuStatus(). */
    void setIRBackendDevice (const std::string& deviceId)
    {
        {
            const juce::SpinLock::ScopedLockType lock (deviceIdLock);
            if (irDeviceId == deviceId)
                return;
            irDeviceId = deviceId;
        }

        if (currentAlgorithmType == IR)
        {
            irBackendChangeRequested.store (true, std::memory_order_release);
            if (fadeState.load (std::memory_order_acquire) == FadeNone)
                fadeState.store (FadingOut, std::memory_order_release);
        }
    }

    /** Select the FDN backend (timer thread, 50 Hz). Mirrors setIRBackendGpu:
        when the FDN algorithm is active, switches via the fade-to-silence
        path; otherwise records the choice for the next FDN instantiation. */
    void setFDNBackendDevice (const std::string& deviceId)
    {
        {
            const juce::SpinLock::ScopedLockType lock (deviceIdLock);
            if (fdnDeviceId == deviceId)
                return;
            fdnDeviceId = deviceId;
        }

        if (currentAlgorithmType == FDN)
        {
            fdnBackendChangeRequested.store (true, std::memory_order_release);
            if (fadeState.load (std::memory_order_acquire) == FadeNone)
                fadeState.store (FadingOut, std::memory_order_release);
        }
    }

    /** Select the SDN backend (timer thread, 50 Hz). Mirrors setFDNBackendGpu:
        when the SDN algorithm is active, switches via the fade-to-silence
        path; otherwise records the choice for the next SDN instantiation. */
    void setSDNBackendDevice (const std::string& deviceId)
    {
        {
            const juce::SpinLock::ScopedLockType lock (deviceIdLock);
            if (sdnDeviceId == deviceId)
                return;
            sdnDeviceId = deviceId;
        }

        if (currentAlgorithmType == SDN)
        {
            sdnBackendChangeRequested.store (true, std::memory_order_release);
            if (fadeState.load (std::memory_order_acquire) == FadeNone)
                fadeState.store (FadingOut, std::memory_order_release);
        }
    }

    /** Current reverb GPU backend status (any thread). */
    ReverbGpuStatus getReverbGpuStatus() const
    {
        juce::SpinLock::ScopedLockType lock (reverbGpuStatusLock);
        return reverbGpuStatus;
    }

#if WFS_GPU_NATIVE
    /** Live pump telemetry for the active GPU reverb (message thread). The
        reverb runs its OWN async pipeline, independent of the direct-sound
        pump, so it needs its own readout. Fills underruns (cumulative) + the
        peak pump ms since the last call (resets it) + the per-block budget ms,
        and returns true only when a GPU reverb is actually live. */
    bool getReverbGpuPumpStats (uint32_t& underruns, float& peakPumpMs, float& budgetMs)
    {
        juce::SpinLock::ScopedLockType lock (algorithmLock);
        budgetMs = sampleRate > 0.0 ? (float) (internalBlockSize / sampleRate * 1000.0) : 0.0f;
        auto* a = algorithm.get();
        if (auto* s = dynamic_cast<ReverbSDNAlgorithmGPU*> (a))
        { if (! s->isReady()) return false; underruns = s->getUnderrunCount(); peakPumpMs = s->getAndResetPeakPumpMs(); return true; }
        if (auto* f = dynamic_cast<ReverbFDNAlgorithmGPU*> (a))
        { if (! f->isReady()) return false; underruns = f->getUnderrunCount(); peakPumpMs = f->getAndResetPeakPumpMs(); return true; }
        if (auto* i = dynamic_cast<ReverbIRAlgorithmGPU*> (a))
        { if (! i->isReady()) return false; underruns = i->getUnderrunCount(); peakPumpMs = i->getAndResetPeakPumpMs(); return true; }
        return false;
    }

    /** NON-destructive live sample of the active GPU reverb pump, for the
        20 Hz meter UI (message thread). Deliberately separate from
        getReverbGpuPumpStats(): that one RESETS the peak and stays exclusive
        to the 1/s underrun logger; this one only reads getLastPumpMs() (the
        meter keeps its own UI-side rolling peak). Uses tryEnter so the UI
        sampler can never block behind the engine thread's algorithm lock —
        returns false with `out` untouched when contended (caller keeps its
        previous sample). */
    struct ReverbGpuLiveSample
    {
        bool live = false;          // a GPU reverb is current and ready
        float lastPumpMs = 0.0f;    // last pump wall (non-destructive)
        float budgetMs = 0.0f;      // internal block budget (ms)
        uint32_t underruns = 0;     // cumulative pipeline underruns
        float latencyMs = 0.0f;     // pipeline cushion latency
        int depthBlocks = 0;        // pipeline depth
    };

    bool sampleReverbGpuLive (ReverbGpuLiveSample& out)
    {
        if (! algorithmLock.tryEnter())
            return false;

        ReverbGpuLiveSample s;
        s.budgetMs = sampleRate > 0.0 ? (float) (internalBlockSize / sampleRate * 1000.0) : 0.0f;

        auto fill = [&s] (auto* gpu)
        {
            if (! gpu->isReady())
                return;
            s.live = true;
            s.lastPumpMs = gpu->getLastPumpMs();
            s.underruns = gpu->getUnderrunCount();
            s.latencyMs = (float) gpu->getPipelineLatencyMs();
            s.depthBlocks = gpu->getPipelineDepthBlocks();
        };

        auto* a = algorithm.get();
        if (auto* p = dynamic_cast<ReverbSDNAlgorithmGPU*> (a))      fill (p);
        else if (auto* p = dynamic_cast<ReverbFDNAlgorithmGPU*> (a)) fill (p);
        else if (auto* p = dynamic_cast<ReverbIRAlgorithmGPU*> (a))  fill (p);

        algorithmLock.exit();
        out = s;
        return true;
    }
#endif

    //==========================================================================
    // Engine duty telemetry — always-on (GPU host-path optimization M0), i.e.
    // OUTSIDE the REVERB_DIAGNOSTICS fence which is compiled out in release
    // builds. One steady_clock pair per internal block (375/s at 96k) is
    // noise; relaxed atomics, non-destructive reads.
    //==========================================================================

    /** Microseconds spent in the last processBlock() (pre + algo + post). */
    float getLastEngineBlockUs() const noexcept
    {
        return lastEngineBlockUs.load (std::memory_order_relaxed);
    }

    /** Blocks processed since start (wraps; UI uses it as a liveness tick). */
    uint32_t getEngineBlockCount() const noexcept
    {
        return engineBlockCounter.load (std::memory_order_relaxed);
    }

    /** Per-internal-block budget in microseconds (0 before prepareToPlay). */
    float getEngineBudgetUs() const noexcept
    {
        return sampleRate > 0.0
            ? (float) (internalBlockSize / sampleRate * 1.0e6)
            : 0.0f;
    }

    /** Load an IR file via fade-to-silence transition.
        Reads the file outside any lock, stores the buffer as pending,
        and triggers a fade-out. The engine thread picks up the new IR
        at silence and fades back in — guaranteeing zero residual artifacts. */
    void loadIRFile (const juce::File& file)
    {
        if (! file.existsAsFile())
            return;

        // Read IR file (file I/O — outside any lock)
        juce::AudioFormatManager mgr;
        mgr.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader (mgr.createReaderFor (file));
        if (! reader)
            return;

        auto numSamples = static_cast<int> (reader->lengthInSamples);
        juce::AudioBuffer<float> buffer (1, numSamples);
        reader->read (&buffer, 0, numSamples, 0, true, false);
        double fileSR = reader->sampleRate;

        // Store the IR for the engine thread to pick up during fade. The
        // buffer is RETAINED (copied at use, not moved) so a later backend
        // switch can rebuild a fresh algorithm without a UI re-push.
        {
            std::lock_guard<std::mutex> lock (pendingIRMutex);
            pendingIRBuffer = std::move (buffer);
            pendingIRFile = file;
            pendingIRSampleRate = fileSR;
        }
        irChangeRequested.store (true, std::memory_order_release);

        // Trigger fade-out (only if not already fading)
        if (fadeState.load (std::memory_order_acquire) == FadeNone)
            fadeState.store (FadingOut, std::memory_order_release);
    }

    /** Set IR parameters (trim, length). Only effective for IR algorithm.
        Values are retained so rebuilt instances (backend switches) get them
        re-applied without a UI re-push. */
    void setIRParameters (float trimMs, float lengthSec)
    {
        lastIRTrimMs.store (trimMs, std::memory_order_relaxed);
        lastIRLengthSec.store (lengthSec, std::memory_order_relaxed);
        haveIRParams.store (true, std::memory_order_release);

        juce::SpinLock::ScopedLockType lock (algorithmLock);
        if (auto* ir = dynamic_cast<ReverbIRAlgorithmBase*> (algorithm.get()))
            ir->setIRParameters (trimMs, lengthSec);
    }

    /** Set pre-processor parameters (per-node EQ + global compressor). */
    void setPreProcessorParams (const ReverbPreProcessor::PreProcessorParams& params)
    {
        juce::SpinLock::ScopedLockType lock (prePostParamsLock);
        pendingPreParams = params;
        preParamsChanged.store (true, std::memory_order_release);
    }

    /** Set post-processor parameters (global EQ + sidechain-keyed expander). */
    void setPostProcessorParams (const ReverbPostProcessor::PostProcessorParams& params)
    {
        juce::SpinLock::ScopedLockType lock (prePostParamsLock);
        pendingPostParams = params;
        postParamsChanged.store (true, std::memory_order_release);
    }

    //==========================================================================
    // State Queries
    //==========================================================================

    /** Get compressor gain reduction in dB (max across all nodes). Thread-safe. */
    float getCompGainReductionDb() const { return preProcessor.getGainReductionDb(); }

    /** Get expander gain reduction in dB (max across all nodes). Thread-safe. */
    float getExpGainReductionDb() const { return postProcessor.getGainReductionDb(); }

#if REVERB_DIAGNOSTICS
    /** Get the shared diagnostics struct for external monitoring. */
    ReverbDiagnostics& getDiagnostics() { return diagnostics; }
    const ReverbDiagnostics& getDiagnostics() const { return diagnostics; }
#endif

    /** Get and reset the dropout counter (for UI warning). Always enabled. */
    uint64_t getAndResetDropoutCount() { return dropoutCount.exchange (0, std::memory_order_relaxed); }

    /** Check if the engine is actively processing. */
    bool isActive() const { return numReverbNodes > 0 && isThreadRunning(); }

    /** Get the current number of reverb nodes. */
    int getNumNodes() const { return numReverbNodes; }

    /** Update for changed node count. Stops/restarts the thread. */
    void setNumNodes (int numNodes)
    {
        if (numNodes != numReverbNodes && sampleRate > 0)
        {
            bool wasRunning = isThreadRunning();
            if (wasRunning)
                stopThread (1000);
            prepareToPlay (sampleRate, currentBlockSize, numNodes);
            if (wasRunning)
                startRealtimeThread (juce::Thread::RealtimeOptions{}
                                         .withApproximateAudioProcessingTime (internalBlockSize, sampleRate));
        }
    }

private:
    //==========================================================================
    // Thread Main Loop
    //==========================================================================

    void run() override
    {
        // Audio workgroup membership: token lives on (and is destroyed on) this thread.
        // This is also the fork-join calling thread for parallelPool, so the whole
        // reverb worker group ends up in the same workgroup.
        juce::WorkgroupToken wgToken;
        uint32_t wgSeenGeneration = 0;

        while (! threadShouldExit())
        {
            if (workgroupCoordinator != nullptr)
                workgroupCoordinator->joinIfChanged (wgToken, wgSeenGeneration);

            if (numReverbNodes > 0)
            {
                // Check if we have enough input data to process an internal block
                bool hasData = true;
                for (int i = 0; i < numReverbNodes && hasData; ++i)
                {
                    if (nodeInputBuffers[i]->getAvailableData() < internalBlockSize)
                        hasData = false;
                }

                if (hasData)
                {
                    processBlock();
                }
                else
                {
                    wait (1);
                }
            }
            else
            {
                wait (10);
            }
        }
    }

    //==========================================================================
    // Internal Processing
    //==========================================================================

    void processBlock()
    {
        // Always-on duty clock (outside the REVERB_DIAGNOSTICS fence, which is
        // compiled out in release builds — F5 of the GPU host-path plan).
        const auto engineBlockStart = std::chrono::steady_clock::now();
#if REVERB_DIAGNOSTICS
        auto blockStart = std::chrono::steady_clock::now();

        // Snapshot buffer levels before reading
        {
            int inMin = std::numeric_limits<int>::max(), inMax = 0;
            int outMin = std::numeric_limits<int>::max(), outMax = 0;
            for (int n = 0; n < numReverbNodes; ++n)
            {
                int inAvail = nodeInputBuffers[n]->getAvailableData();
                int outAvail = nodeOutputBuffers[n]->getAvailableData();
                inMin = juce::jmin (inMin, inAvail);
                inMax = juce::jmax (inMax, inAvail);
                outMin = juce::jmin (outMin, outAvail);
                outMax = juce::jmax (outMax, outAvail);
            }
            diagnostics.inputBufferMinLevel.store (inMin, std::memory_order_relaxed);
            diagnostics.inputBufferMaxLevel.store (inMax, std::memory_order_relaxed);
            diagnostics.outputBufferMinLevel.store (outMin, std::memory_order_relaxed);
            diagnostics.outputBufferMaxLevel.store (outMax, std::memory_order_relaxed);
        }

        // Aggregate ring buffer overflow/underrun counters
        {
            uint64_t totalOverflow = 0, totalOverflowEvts = 0;
            uint64_t totalUnderrun = 0, totalUnderrunEvts = 0;
            for (int n = 0; n < numReverbNodes; ++n)
            {
                totalOverflow += nodeInputBuffers[n]->overflowSamples.load (std::memory_order_relaxed);
                totalOverflowEvts += nodeInputBuffers[n]->overflowEvents.load (std::memory_order_relaxed);
                totalUnderrun += nodeOutputBuffers[n]->underrunSamples.load (std::memory_order_relaxed);
                totalUnderrunEvts += nodeOutputBuffers[n]->underrunEvents.load (std::memory_order_relaxed);
            }
            diagnostics.inputOverflowSamples.store (totalOverflow, std::memory_order_relaxed);
            diagnostics.inputOverflowEvents.store (totalOverflowEvts, std::memory_order_relaxed);
            diagnostics.outputUnderrunSamples.store (totalUnderrun, std::memory_order_relaxed);
            diagnostics.outputUnderrunEvents.store (totalUnderrunEvts, std::memory_order_relaxed);
        }

        diagnostics.fadeStateSnapshot.store (fadeState.load (std::memory_order_relaxed), std::memory_order_relaxed);
        diagnostics.algorithmType.store (currentAlgorithmType, std::memory_order_relaxed);
#endif

        int numSamples = internalBlockSize;

        // Read input from ring buffers into working buffer
        for (int n = 0; n < numReverbNodes; ++n)
        {
            nodeInputBuffers[n]->read (nodeInputBlock.getWritePointer (n), numSamples);
        }

        // Clear output buffer
        nodeOutputBlock.clear();

        // Apply pending algorithm parameter changes
        if (paramsChanged.load (std::memory_order_acquire))
        {
            AlgorithmParameters cp;
            {
                juce::SpinLock::ScopedLockType lock (pendingParamsLock);
                cp = pendingParams;
            }
            paramsChanged.store (false, std::memory_order_release);
            currentParams = cp;

            juce::SpinLock::ScopedLockType lock (algorithmLock);
            if (algorithm)
                algorithm->setParameters (currentParams);
        }

        // Apply pending pre/post processor parameter changes
        if (preParamsChanged.load (std::memory_order_acquire))
        {
            ReverbPreProcessor::PreProcessorParams pp;
            {
                juce::SpinLock::ScopedLockType lock (prePostParamsLock);
                pp = pendingPreParams;
            }
            preParamsChanged.store (false, std::memory_order_release);
            preProcessor.setParameters (pp);
        }

        if (postParamsChanged.load (std::memory_order_acquire))
        {
            ReverbPostProcessor::PostProcessorParams pp;
            {
                juce::SpinLock::ScopedLockType lock (prePostParamsLock);
                pp = pendingPostParams;
            }
            postParamsChanged.store (false, std::memory_order_release);
            postProcessor.setParameters (pp);
        }

        // Apply pending geometry changes
        if (geometryChanged.load (std::memory_order_acquire))
        {
            std::vector<NodePosition> geo;
            {
                juce::SpinLock::ScopedLockType lock (geometryLock);
                geo = pendingGeometry;
            }
            geometryChanged.store (false, std::memory_order_release);
            currentGeometry = geo;

            juce::SpinLock::ScopedLockType lock (algorithmLock);
            if (algorithm)
                algorithm->updateGeometry (geo);
        }

        // --- Pre-processing: per-node EQ + compressor + sidechain tap ---
        preProcessor.processBlock (nodeInputBlock, sidechainLevels, numSamples);

        // --- Algorithm processing ---
        {
            juce::SpinLock::ScopedLockType lock (algorithmLock);
            if (algorithm)
            {
                algorithm->processBlock (nodeInputBlock, nodeOutputBlock, numSamples);
            }
        }

        // --- Post-processing: global EQ + sidechain-keyed expander ---
        postProcessor.processBlock (nodeOutputBlock, sidechainLevels, numSamples);

        // Apply wet level
        float wetLevel = currentParams.wetLevel;
        if (wetLevel != 1.0f)
        {
            for (int n = 0; n < numReverbNodes; ++n)
                juce::FloatVectorOperations::multiply (nodeOutputBlock.getWritePointer (n),
                                                       wetLevel, numSamples);
        }

        // --- Signal validation (post-processing, pre-fade) ---
#if REVERB_DIAGNOSTICS
        {
            uint64_t nans = 0, clips = 0;
            float peak = 0.0f;
            for (int n = 0; n < numReverbNodes; ++n)
            {
                const float* data = nodeOutputBlock.getReadPointer (n);
                for (int s = 0; s < numSamples; ++s)
                {
                    float v = data[s];
                    float absV = std::abs (v);
                    if (std::isnan (v) || std::isinf (v))
                        ++nans;
                    if (absV > 10.0f)
                        ++clips;
                    if (absV > peak)
                        peak = absV;
                }
            }
            if (nans > 0)
                diagnostics.nanInfCount.fetch_add (nans, std::memory_order_relaxed);
            if (clips > 0)
                diagnostics.clippingCount.fetch_add (clips, std::memory_order_relaxed);
            float oldPeak = diagnostics.peakOutputLevel.load (std::memory_order_relaxed);
            while (peak > oldPeak)
            {
                if (diagnostics.peakOutputLevel.compare_exchange_weak (
                        oldPeak, peak, std::memory_order_relaxed))
                    break;
            }
        }
#endif

        // --- Algorithm switching fade ---
        int currentFade = fadeState.load (std::memory_order_acquire);
        if (currentFade != FadeNone)
        {
            applyFade (numSamples);
        }

        // Write output to ring buffers
        for (int n = 0; n < numReverbNodes; ++n)
        {
            nodeOutputBuffers[n]->write (nodeOutputBlock.getReadPointer (n), numSamples);
        }

#if REVERB_DIAGNOSTICS
        // Timing measurement
        auto blockEnd = std::chrono::steady_clock::now();
        float elapsedUs = std::chrono::duration<float, std::micro> (blockEnd - blockStart).count();

        diagnostics.lastProcessBlockUs.store (elapsedUs, std::memory_order_relaxed);

        float oldMax = diagnostics.maxProcessBlockUs.load (std::memory_order_relaxed);
        while (elapsedUs > oldMax)
        {
            if (diagnostics.maxProcessBlockUs.compare_exchange_weak (
                    oldMax, elapsedUs, std::memory_order_relaxed))
                break;
        }

        float oldAvg = diagnostics.avgProcessBlockUs.load (std::memory_order_relaxed);
        diagnostics.avgProcessBlockUs.store (oldAvg + 0.01f * (elapsedUs - oldAvg), std::memory_order_relaxed);

        float budget = static_cast<float> (internalBlockSize) / static_cast<float> (sampleRate) * 1e6f;
        diagnostics.budgetUs.store (budget, std::memory_order_relaxed);
        if (elapsedUs > budget)
            diagnostics.overbudgetCount.fetch_add (1, std::memory_order_relaxed);

        diagnostics.blocksProcessed.fetch_add (1, std::memory_order_relaxed);
#endif

        // Always-on duty telemetry (see getLastEngineBlockUs).
        lastEngineBlockUs.store (std::chrono::duration<float, std::micro> (
                                     std::chrono::steady_clock::now() - engineBlockStart).count(),
                                 std::memory_order_relaxed);
        engineBlockCounter.fetch_add (1, std::memory_order_relaxed);
    }

    //==========================================================================
    // Algorithm switching fade
    //==========================================================================

    void applyFade (int numSamples)
    {
        int currentFade = fadeState.load (std::memory_order_acquire);

        if (currentFade == FadingOut)
        {
            // Ramp gain down
            float fadeStep = static_cast<float> (numSamples) / fadeSamples;
            float startGain = fadeGain;
            float endGain = std::max (0.0f, fadeGain - fadeStep);

            for (int n = 0; n < numReverbNodes; ++n)
            {
                float* data = nodeOutputBlock.getWritePointer (n);
                float g = startGain;
                float gStep = (endGain - startGain) / static_cast<float> (numSamples);
                for (int s = 0; s < numSamples; ++s)
                {
                    data[s] *= g;
                    g += gStep;
                }
            }

            fadeGain = endGain;

            if (fadeGain <= 0.0f)
            {
                // Fade-out complete: determine what triggered the fade
                int newType = pendingAlgorithmType.load (std::memory_order_acquire);
                bool algoChange = (newType != currentAlgorithmType && newType >= 0);
                bool irChange = irChangeRequested.load (std::memory_order_acquire);
                const int effectiveType = algoChange ? newType : currentAlgorithmType;
                bool backendChange =
                    (irBackendChangeRequested.load (std::memory_order_acquire) && effectiveType == IR)
                    || (fdnBackendChangeRequested.load (std::memory_order_acquire) && effectiveType == FDN)
                    || (sdnBackendChangeRequested.load (std::memory_order_acquire) && effectiveType == SDN);

                if (algoChange)
                    currentAlgorithmType = newType;

                if (algoChange || irChange || backendChange)
                {
                    irChangeRequested.store (false, std::memory_order_release);
                    irBackendChangeRequested.store (false, std::memory_order_release);
                    fdnBackendChangeRequested.store (false, std::memory_order_release);
                    sdnBackendChangeRequested.store (false, std::memory_order_release);
                    installAlgorithm (currentAlgorithmType);
                }

                fadeGain = 0.0f;
                fadeState.store (FadingIn, std::memory_order_release);
            }
        }
        else if (currentFade == FadingIn)
        {
            // Ramp gain up
            float fadeStep = static_cast<float> (numSamples) / fadeSamples;
            float startGain = fadeGain;
            float endGain = std::min (1.0f, fadeGain + fadeStep);

            for (int n = 0; n < numReverbNodes; ++n)
            {
                float* data = nodeOutputBlock.getWritePointer (n);
                float g = startGain;
                float gStep = (endGain - startGain) / static_cast<float> (numSamples);
                for (int s = 0; s < numSamples; ++s)
                {
                    data[s] *= g;
                    g += gStep;
                }
            }

            fadeGain = endGain;

            if (fadeGain >= 1.0f)
            {
                fadeGain = 1.0f;

                // If an IR or backend change arrived while we were fading, re-trigger
                if (irChangeRequested.load (std::memory_order_acquire)
                    || irBackendChangeRequested.load (std::memory_order_acquire)
                    || fdnBackendChangeRequested.load (std::memory_order_acquire)
                    || sdnBackendChangeRequested.load (std::memory_order_acquire))
                    fadeState.store (FadingOut, std::memory_order_release);
                else
                    fadeState.store (FadeNone, std::memory_order_release);
            }
        }
    }

    //==========================================================================
    // Algorithm construction (engine thread, during fade silence)
    //==========================================================================

    /** Creates the IR algorithm honouring the GPU/CPU backend choice.
        The GPU instance is prepared here (outside the algorithm lock — the
        kernel compile takes ~10 ms) so readiness can be tested; if it fails,
        falls back to the CPU implementation and records the reason. */
    std::unique_ptr<ReverbAlgorithm> createIRAlgorithm()
    {
#if WFS_GPU_NATIVE
        std::string deviceId;
        { const juce::SpinLock::ScopedLockType lock (deviceIdLock); deviceId = irDeviceId; }
        if (! deviceId.empty() && deviceId != "cpu")
        {
            auto gpu = std::make_unique<ReverbIRAlgorithmGPU>();
            gpu->setDeviceId (deviceId);
            if (sampleRate > 0)
                gpu->prepare (sampleRate, internalBlockSize, numReverbNodes);

            if (gpu->isReady())
            {
                setReverbGpuStatusInternal ({ ReverbGpuStatus::GpuActive, gpu->getDeviceName(),
                                              {}, gpu->getPipelineLatencyMs() });
                return gpu;
            }

            setReverbGpuStatusInternal ({ ReverbGpuStatus::GpuFallback, {},
                                          gpu->getLastError(), 0.0 });
            juce::Logger::writeToLog ("GPU IR reverb unavailable, using CPU: "
                                      + gpu->getLastError());
        }
        else
        {
            setReverbGpuStatusInternal ({ ReverbGpuStatus::Cpu, {}, {}, 0.0 });
        }
#endif
        return std::make_unique<IRAlgorithm>();
    }

    /** Creates the FDN algorithm honouring the GPU/CPU backend choice. Mirrors
        createIRAlgorithm(): prepares the GPU instance to test readiness, falls
        back to the CPU FDNAlgorithm if it fails, and records the status. */
    std::unique_ptr<ReverbAlgorithm> createFDNAlgorithm()
    {
#if WFS_GPU_NATIVE
        std::string deviceId;
        { const juce::SpinLock::ScopedLockType lock (deviceIdLock); deviceId = fdnDeviceId; }
        if (! deviceId.empty() && deviceId != "cpu")
        {
            auto gpu = std::make_unique<ReverbFDNAlgorithmGPU>();
            gpu->setDeviceId (deviceId);
            if (sampleRate > 0)
                // prepare() with the GPU instance's DEFAULT fdnSize (1.0), exactly
                // like the CPU FDNAlgorithm whose prepare() runs before its own
                // setParameters — so the delay lengths match for A/B parity. The
                // generic install path below pushes the live params (coeffs only).
                gpu->prepare (sampleRate, internalBlockSize, numReverbNodes);

            if (gpu->isReady())
            {
                setReverbGpuStatusInternal ({ ReverbGpuStatus::GpuActive, gpu->getDeviceName(),
                                              {}, gpu->getPipelineLatencyMs() });
                return gpu;
            }

            setReverbGpuStatusInternal ({ ReverbGpuStatus::GpuFallback, {},
                                          gpu->getLastError(), 0.0 });
            juce::Logger::writeToLog ("GPU FDN reverb unavailable, using CPU: "
                                      + gpu->getLastError());
        }
        else
        {
            setReverbGpuStatusInternal ({ ReverbGpuStatus::Cpu, {}, {}, 0.0 });
        }
#endif
        return std::make_unique<FDNAlgorithm>();
    }

    /** Creates the SDN algorithm honouring the GPU/CPU backend choice. Mirrors
        createFDNAlgorithm(): prepares the GPU instance to test readiness, falls
        back to the CPU SDNAlgorithm if it fails, and records the status. The
        live geometry/params are pushed by the generic install path below. */
    std::unique_ptr<ReverbAlgorithm> createSDNAlgorithm()
    {
#if WFS_GPU_NATIVE
        std::string deviceId;
        { const juce::SpinLock::ScopedLockType lock (deviceIdLock); deviceId = sdnDeviceId; }
        if (! deviceId.empty() && deviceId != "cpu")
        {
            auto gpu = std::make_unique<ReverbSDNAlgorithmGPU>();
            gpu->setDeviceId (deviceId);
            if (sampleRate > 0)
                gpu->prepare (sampleRate, internalBlockSize, numReverbNodes);

            if (gpu->isReady())
            {
                setReverbGpuStatusInternal ({ ReverbGpuStatus::GpuActive, gpu->getDeviceName(),
                                              {}, gpu->getPipelineLatencyMs() });
                return gpu;
            }

            setReverbGpuStatusInternal ({ ReverbGpuStatus::GpuFallback, {},
                                          gpu->getLastError(), 0.0 });
            juce::Logger::writeToLog ("GPU SDN reverb unavailable, using CPU: "
                                      + gpu->getLastError());
        }
        else
        {
            setReverbGpuStatusInternal ({ ReverbGpuStatus::Cpu, {}, {}, 0.0 });
        }
#endif
        return std::make_unique<SDNAlgorithm>();
    }

    /** Creates, prepares and installs the algorithm for `type`, re-applying
        parameters, geometry and — for IR — the retained IR file/trim/length,
        so rebuilt instances need no UI re-push. Runs at fade silence. */
    void installAlgorithm (int type)
    {
        std::unique_ptr<ReverbAlgorithm> newAlgo;
        switch (type)
        {
            case SDN: newAlgo = createSDNAlgorithm();             break;
            case IR:  newAlgo = createIRAlgorithm();              break;
            case FDN: newAlgo = createFDNAlgorithm();             break;
            default:  newAlgo = std::make_unique<FDNAlgorithm>(); break;
        }

        // Arm the instance before it becomes visible to other threads (the
        // GPU instance's prepare() no-ops here — already done in the factory).
        if (sampleRate > 0)
        {
            newAlgo->prepare (sampleRate, internalBlockSize, numReverbNodes);
            newAlgo->setParallelFor (&parallelPool);
            newAlgo->setParameters (currentParams);
            newAlgo->updateGeometry (currentGeometry);
        }

        if (auto* ir = dynamic_cast<ReverbIRAlgorithmBase*> (newAlgo.get()))
        {
            if (haveIRParams.load (std::memory_order_acquire))
                ir->setIRParameters (lastIRTrimMs.load (std::memory_order_relaxed),
                                     lastIRLengthSec.load (std::memory_order_relaxed));

            // Reload the retained IR (copy — the original stays retained for
            // the next rebuild).
            juce::AudioBuffer<float> buf;
            juce::File file;
            double fileSR = 0.0;
            {
                std::lock_guard<std::mutex> lock (pendingIRMutex);
                buf = pendingIRBuffer;
                file = pendingIRFile;
                fileSR = pendingIRSampleRate;
            }
            if (buf.getNumSamples() > 0)
                ir->loadIRFromBuffer (file, std::move (buf), fileSR);
        }

        {
            juce::SpinLock::ScopedLockType lock (algorithmLock);
            algorithm = std::move (newAlgo);
            attachIRDiagnostics();
        }
    }

    void attachIRDiagnostics()
    {
#if REVERB_DIAGNOSTICS
        if (auto* ir = dynamic_cast<IRAlgorithm*> (algorithm.get()))
            ir->setDiagnostics (&diagnostics);
#if WFS_GPU_NATIVE
        else if (auto* irGpu = dynamic_cast<ReverbIRAlgorithmGPU*> (algorithm.get()))
            irGpu->setDiagnostics (&diagnostics);
#endif
#endif
    }

    void setReverbGpuStatusInternal (const ReverbGpuStatus& s)
    {
        juce::SpinLock::ScopedLockType lock (reverbGpuStatusLock);
        reverbGpuStatus = s;
    }

    //==========================================================================
    // State
    //==========================================================================

    double sampleRate = 0.0;
    int currentBlockSize = 512;
    int internalBlockSize = 256;
    int numReverbNodes = 0;

    // Per-node ring buffers for audio thread <-> engine thread
    std::vector<std::unique_ptr<LockFreeRingBuffer>> nodeInputBuffers;
    std::vector<std::unique_ptr<LockFreeRingBuffer>> nodeOutputBuffers;

    // Working buffers (numNodes channels x internalBlockSize samples)
    juce::AudioBuffer<float> nodeInputBlock;
    juce::AudioBuffer<float> nodeOutputBlock;

    // Active algorithm (nullptr = silence pass-through)
    std::unique_ptr<ReverbAlgorithm> algorithm;
    juce::SpinLock algorithmLock;
    int currentAlgorithmType = -1;  // -1 = none set yet

    // Thread-safe parameter passing
    AlgorithmParameters pendingParams;
    juce::SpinLock pendingParamsLock;
    std::atomic<bool> paramsChanged { false };
    AlgorithmParameters currentParams;

    // Thread-safe geometry passing
    std::vector<NodePosition> currentGeometry;
    std::vector<NodePosition> pendingGeometry;
    juce::SpinLock geometryLock;
    std::atomic<bool> geometryChanged { false };

    // Pre/post processors
    ReverbPreProcessor preProcessor;
    ReverbPostProcessor postProcessor;
    std::vector<float> sidechainLevels;

    // Thread-safe pre/post parameter passing
    ReverbPreProcessor::PreProcessorParams pendingPreParams;
    ReverbPostProcessor::PostProcessorParams pendingPostParams;
    juce::SpinLock prePostParamsLock;
    std::atomic<bool> preParamsChanged { false };
    std::atomic<bool> postParamsChanged { false };

    // Parallel per-node processing thread pool
    AudioParallelFor parallelPool;

    // Optional audio workgroup membership (macOS); null = disabled.
    AudioWorkgroupCoordinator* workgroupCoordinator = nullptr;

    // Algorithm switching fade state
    static constexpr int FadeNone = 0;
    static constexpr int FadingOut = 1;
    static constexpr int FadingIn = 2;
    std::atomic<int> fadeState { FadeNone };
    float fadeGain = 1.0f;
    float fadeSamples = 2400.0f;  // ~50ms at 48kHz
    std::atomic<int> pendingAlgorithmType { -1 };

    // Pending/retained IR change (fade-to-silence transition). The buffer is
    // retained after use so backend switches can rebuild without a UI re-push.
    std::mutex pendingIRMutex;
    juce::AudioBuffer<float> pendingIRBuffer;
    juce::File pendingIRFile;
    double pendingIRSampleRate = 0.0;
    std::atomic<bool> irChangeRequested { false };

    // Retained IR trim/length (re-applied to rebuilt IR instances)
    std::atomic<float> lastIRTrimMs { 0.0f };
    std::atomic<float> lastIRLengthSec { 6.0f };
    std::atomic<bool> haveIRParams { false };

    // Per-algorithm GPU device selection (deviceId; "cpu"/"" = CPU) + shared
    // status for the UI. The deviceId strings are guarded by deviceIdLock
    // (written by the 50 Hz timer thread, read at algorithm-swap time).
    juce::SpinLock deviceIdLock;
    std::string irDeviceId  { "cpu" };
    std::atomic<bool> irBackendChangeRequested { false };
    std::string fdnDeviceId { "cpu" };
    std::atomic<bool> fdnBackendChangeRequested { false };
    std::string sdnDeviceId { "cpu" };
    std::atomic<bool> sdnBackendChangeRequested { false };
    ReverbGpuStatus reverbGpuStatus;
    mutable juce::SpinLock reverbGpuStatusLock;

    // Always-on dropout counter (lightweight, for UI warning)
    std::atomic<uint64_t> dropoutCount { 0 };

    // Always-on engine duty telemetry (see getLastEngineBlockUs)
    std::atomic<float> lastEngineBlockUs { 0.0f };
    std::atomic<uint32_t> engineBlockCounter { 0 };

#if REVERB_DIAGNOSTICS
    ReverbDiagnostics diagnostics;
#endif
};

} // namespace spatcore::reverb

// Extraction-compat alias — app code migrates to qualified names later.
using spatcore::reverb::ReverbEngine;
