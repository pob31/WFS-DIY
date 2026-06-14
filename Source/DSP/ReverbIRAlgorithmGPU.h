#pragma once
#if WFS_GPU_NATIVE

/*
    ReverbIRAlgorithmGPU — IR convolution reverb on the GPU, native edition.

    Drop-in counterpart to the CPU IRAlgorithm (same ReverbIRAlgorithmBase
    surface): numNodes mono channels, every node convolved with the same
    impulse response. Runs the native IrGpuBackend (Metal on macOS, CUDA on
    Windows — hybrid host-FFT / GPU frequency-domain MAC, see MetalIrBackend)
    through a GpuAsyncPipelineT pump so the reverb engine thread never waits
    on the GPU.

    Pipeline depth is derived from the engine block size: enough whole blocks
    to cover a ~20 ms cushion (4 at 256/48 k, 8 at 128, 15 at 64). The cushion
    is constant added latency on the WET PATH ONLY — perceptually a small
    extra pre-delay, absorbed by the reverb's nature — and is recomputed on
    every prepare(), so driver buffer-size changes track automatically.

    IR handling mirrors the CPU class: the full file buffer is cached, trim
    and length apply to the cache without re-reading disk. The final mono IR
    (trimmed, length-limited, resampled to the engine rate, energy-normalised
    like juce::dsp::Convolution::Normalise::yes) is staged to the backend,
    which transforms it progressively on the pump thread.

    GPU init failure leaves isReady() false; the ReverbEngine then swaps in
    the CPU IRAlgorithm and reports the fallback (same UX as the WFS GPU
    fallback).
*/

#include <JuceHeader.h>
#include "ReverbAlgorithm.h"
#include "ReverbDiagnostics.h"
#include "gpu/IrGpuBackend.h"
#include "gpu/GpuDeviceManager.h"
#include "gpu/GpuAsyncPipeline.h"

#include <atomic>
#include <cmath>
#include <memory>
#include <vector>

class ReverbIRAlgorithmGPU : public ReverbIRAlgorithmBase
{
public:
    static constexpr double kCushionMs = 20.0;       // wet-path latency target
    static constexpr double kMaxIrSeconds = 10.0;    // device allocation cap

    ReverbIRAlgorithmGPU() = default;

    ~ReverbIRAlgorithmGPU() override
    {
        pipeline.release();   // pump stops BEFORE the backend dies
        backend.reset();
    }

    //==========================================================================
    void prepare (double newSampleRate, int maxBlockSize, int newNumNodes) override
    {
        // The engine factory pre-prepares to test GPU readiness, then the
        // generic install path calls prepare() again — skip the expensive
        // re-arm when the geometry is unchanged.
        if (ready && newSampleRate == sampleRate
            && maxBlockSize == blockSize && newNumNodes == numNodes)
            return;

        ready = false;
        pipeline.release();
        backend = makeIrBackend (GpuDeviceManager::instance().firstGpuId());
        if (backend == nullptr)
        {
            lastError = "No GPU backend available (using CPU)";
            return;
        }

        sampleRate = newSampleRate;
        blockSize = juce::jmax (1, maxBlockSize);
        numNodes = juce::jmax (1, newNumNodes);

        if (! backend->prepare (numNodes, blockSize, sampleRate,
                                (int) (kMaxIrSeconds * sampleRate)))
        {
            lastError = backend->getLastError();
            DBG ("GPU IR reverb: backend init failed: " + lastError);
            return;
        }

        // Whole-block cushion covering ~20 ms at the current block size.
        const double blockMs = sampleRate > 0.0 ? 1000.0 * blockSize / sampleRate : 0.0;
        const int depth = blockMs > 0.0
            ? juce::jlimit (1, (int) decltype (pipeline)::kMaxDepthBlocks,
                            (int) std::ceil (kCushionMs / blockMs))
            : 4;

        if (! pipeline.prepare (backend.get(), numNodes, numNodes,
                                blockSize, sampleRate, depth))
        {
            lastError = pipeline.getLastError().toStdString();
            DBG ("GPU IR reverb: pipeline init failed: " + lastError);
            backend.reset();
            return;
        }

        // If an IR was already cached (backend switch / re-prepare), re-arm it.
        if (cachedIRBuffer.getNumSamples() > 0)
            stageIr();

        ready = true;
        juce::Logger::writeToLog ("GPU IR reverb active: " + juce::String (numNodes)
                                  + " nodes on " + getDeviceName()
                                  + " (pipeline +" + juce::String (pipeline.getLatencyMs(), 1)
                                  + " ms wet, depth " + juce::String (depth) + ")");
    }

    void reset() override
    {
        // Clears GPU input history + overlap tails at the next pump launch;
        // the loaded IR spectra survive.
        if (backend)
            backend->requestReset();
    }

    void processBlock (const juce::AudioBuffer<float>& nodeInputs,
                       juce::AudioBuffer<float>& nodeOutputs,
                       int numSamples) override
    {
        if (! ready || ! pipeline.isReady())
            return; // outputs were pre-cleared by the engine

        pipeline.pushInput (nodeInputs, numNodes, 0, numSamples);
        pipeline.popOutput (nodeOutputs, numNodes, 0, numSamples);

#if REVERB_DIAGNOSTICS
        if (diagPtr != nullptr)
        {
            const int cur = backend->getSegmentsLoaded() * blockSize;
            const int total = backend->getSegmentsTotal() * blockSize;
            diagPtr->irCurrentSize.store (cur, std::memory_order_relaxed);
            diagPtr->irExpectedSize.store (total, std::memory_order_relaxed);
            diagPtr->irFullyLoaded.store (total > 0 && cur == total, std::memory_order_relaxed);
        }
#endif
    }

    void setParameters (const AlgorithmParameters&) override
    {
        // Wet level is applied by the engine; trim/length arrive via
        // setIRParameters() like the CPU implementation.
    }

    void updateGeometry (const std::vector<NodePosition>&) override {}

    //==========================================================================
    // ReverbIRAlgorithmBase

    void loadIRFromBuffer (const juce::File& file,
                           juce::AudioBuffer<float>&& buf,
                           double fileSampleRate) override
    {
        currentIRFile = file;
        cachedIRBuffer = std::move (buf);
        cachedIRSampleRate = fileSampleRate > 0.0 ? fileSampleRate : sampleRate;
        irFileDurationSec = cachedIRSampleRate > 0.0
            ? (float) (cachedIRBuffer.getNumSamples() / cachedIRSampleRate)
            : 0.0f;
        stageIr();
    }

    void setIRParameters (float trimMs, float lengthSec) override
    {
        const bool changed = (trimMs != irTrimMs || lengthSec != irLengthSec);
        irTrimMs = trimMs;
        irLengthSec = lengthSec;
        if (changed && cachedIRBuffer.getNumSamples() > 0)
            stageIr();
    }

    const juce::File& getCurrentIRFile() const override { return currentIRFile; }
    float getIRFileDuration() const override { return irFileDurationSec; }

    //==========================================================================
    // Status (engine/UI)

    bool isReady() const noexcept { return ready && pipeline.isReady(); }
    juce::String getLastError() const { return juce::String (lastError); }
    juce::String getDeviceName() const { return backend ? juce::String (backend->getDeviceName()) : juce::String(); }
    double getPipelineLatencyMs() const noexcept { return pipeline.getLatencyMs(); }
    uint32_t getUnderrunCount() const noexcept { return pipeline.getUnderrunCount(); }
    float getAndResetPeakPumpMs() noexcept { return pipeline.getAndResetPeakPumpMs(); }
    bool hasPumpFailed() const noexcept { return pipeline.hasPumpFailed(); }

#if REVERB_DIAGNOSTICS
    void setDiagnostics (ReverbDiagnostics* d) { diagPtr = d; }
#endif

private:
    //==========================================================================
    // Builds the final mono IR (trim -> length -> resample -> normalise) and
    // stages it to the backend. Ported verbatim from the SDK prototype's
    // stageIrMessage() — the normalisation parity is the load-bearing part.
    void stageIr()
    {
        const int totalSamples = cachedIRBuffer.getNumSamples();
        if (totalSamples <= 0)
            return;

        const double srcRate = cachedIRSampleRate > 0.0 ? cachedIRSampleRate : sampleRate;

        const int startSample = juce::jlimit (0, totalSamples - 1,
                                              (int) (irTrimMs * 0.001 * srcRate));
        int numToUse = totalSamples - startSample;
        if (irLengthSec > 0.0f)
            numToUse = juce::jmin (numToUse, (int) (irLengthSec * srcRate));
        numToUse = juce::jmax (1, numToUse);

        // Mono source (channel 0, matching the CPU per-node mono convolution).
        const float* src = cachedIRBuffer.getReadPointer (0, startSample);

        // Resample to the engine rate when needed.
        std::vector<float> mono;
        if (std::abs (srcRate - sampleRate) > 1.0)
        {
            const double ratio = srcRate / sampleRate;
            const int outLen = juce::jmax (1, (int) (numToUse / ratio));
            mono.resize ((size_t) outLen);
            juce::LagrangeInterpolator resampler;
            resampler.process (ratio, src, mono.data(), outLen, numToUse, 0);
        }
        else
        {
            mono.assign (src, src + numToUse);
        }

        // Cap to the device allocation bound.
        const size_t maxSamples = (size_t) (kMaxIrSeconds * sampleRate);
        if (mono.size() > maxSamples)
            mono.resize (maxSamples);

        // Energy normalisation matching juce::dsp::Convolution::Normalise::yes
        // EXACTLY: factor = 0.125 / sqrt(sum of squares). The 0.125 (-18 dB)
        // is JUCE's constant; without it the GPU reverb runs 18 dB hot vs the
        // CPU path (caught by ear in the first A/B, 2026-06-11).
        double energy = 0.0;
        for (float v : mono)
            energy += (double) v * v;
        if (energy > 1.0e-8)
        {
            const float scale = (float) (0.125 / std::sqrt (energy));
            for (float& v : mono)
                v *= scale;
        }

        if (backend)
            backend->stageIr (mono.data(), (int) mono.size());
    }

    //==========================================================================
    std::unique_ptr<IIrBackend> backend;
    GpuAsyncPipelineT<IGpuBackend> pipeline;

    double sampleRate { 0.0 };
    int blockSize { 0 };
    int numNodes { 0 };
    bool ready { false };
    std::string lastError;

    // Cached IR (full file) + parameters, mirroring the CPU class
    juce::AudioBuffer<float> cachedIRBuffer;
    juce::File currentIRFile;
    double cachedIRSampleRate { 0.0 };
    float irTrimMs { 0.0f };
    float irLengthSec { 6.0f };
    float irFileDurationSec { 0.0f };

#if REVERB_DIAGNOSTICS
    ReverbDiagnostics* diagPtr = nullptr;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverbIRAlgorithmGPU)
};

#endif // WFS_GPU_NATIVE
