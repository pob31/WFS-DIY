#pragma once
#if WFS_GPU_NATIVE

/*
    ReverbFDNAlgorithmGPU — Feedback Delay Network reverb on the GPU, native
    edition. Drop-in counterpart to the CPU FDNAlgorithm: one independent
    16-line FDN per node, no inter-node coupling. Runs the native FdnGpuBackend
    (Metal on macOS, CUDA on Windows) through a GpuAsyncPipelineT pump so the
    reverb engine thread never waits on the GPU.

    Like the GPU IR reverb, the pipeline depth is derived from the engine block
    size to cover a ~20 ms cushion — constant added latency on the WET path
    only (a small extra pre-delay, inaudible for reverb), recomputed on every
    prepare() so driver buffer-size changes track automatically.

    Parameters are applied stepwise (the CPU FDN does not ramp), forwarded to
    the backend which recomputes its coefficients on the pump thread. fdnSize
    is captured at prepare() only — matching the CPU, whose delay lengths are
    fixed in prepareNode() and never resized for a runtime fdnSize change.

    GPU init failure leaves isReady() false; the ReverbEngine then swaps in the
    CPU FDNAlgorithm and reports the fallback (same UX as the GPU IR fallback).
*/

#include <JuceHeader.h>
#include "ReverbAlgorithm.h"
#include "ReverbDiagnostics.h"
#include "gpu/FdnGpuBackend.h"
#include "gpu/GpuAsyncPipeline.h"

#include <cmath>
#include <string>

class ReverbFDNAlgorithmGPU : public ReverbAlgorithm
{
public:
    static constexpr double kCushionMs = 20.0;   // wet-path latency target

    ReverbFDNAlgorithmGPU() = default;

    ~ReverbFDNAlgorithmGPU() override
    {
        pipeline.release();   // pump stops BEFORE the backend dies
        backend.release();
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
        backend.release();

        sampleRate = newSampleRate;
        blockSize = juce::jmax (1, maxBlockSize);
        numNodes = juce::jmax (1, newNumNodes);

        if (! backend.prepare (numNodes, blockSize, sampleRate, currentParams.fdnSize))
        {
            lastError = backend.getLastError();
            DBG ("GPU FDN reverb: backend init failed: " + lastError);
            return;
        }

        // Whole-block cushion covering ~20 ms at the current block size.
        const double blockMs = sampleRate > 0.0 ? 1000.0 * blockSize / sampleRate : 0.0;
        const int depth = blockMs > 0.0
            ? juce::jlimit (1, (int) decltype (pipeline)::kMaxDepthBlocks,
                            (int) std::ceil (kCushionMs / blockMs))
            : 4;

        if (! pipeline.prepare (&backend, numNodes, numNodes,
                                blockSize, sampleRate, depth))
        {
            lastError = pipeline.getLastError().toStdString();
            DBG ("GPU FDN reverb: pipeline init failed: " + lastError);
            backend.release();
            return;
        }

        // Re-apply the current parameters to the fresh backend.
        pushParameters();

        ready = true;
        juce::Logger::writeToLog ("GPU FDN reverb active: " + juce::String (numNodes)
                                  + " nodes on " + getDeviceName()
                                  + " (pipeline +" + juce::String (pipeline.getLatencyMs(), 1)
                                  + " ms wet, depth " + juce::String (depth) + ")");
    }

    void reset() override
    {
        backend.requestReset();
    }

    void processBlock (const juce::AudioBuffer<float>& nodeInputs,
                       juce::AudioBuffer<float>& nodeOutputs,
                       int numSamples) override
    {
        if (! ready || ! pipeline.isReady())
            return; // outputs were pre-cleared by the engine

        pipeline.pushInput (nodeInputs, numNodes, 0, numSamples);
        pipeline.popOutput (nodeOutputs, numNodes, 0, numSamples);
    }

    void setParameters (const AlgorithmParameters& params) override
    {
        currentParams = params;
        if (ready)
            pushParameters();
    }

    void updateGeometry (const std::vector<NodePosition>&) override {}

    //==========================================================================
    // Status (engine/UI)

    bool isReady() const noexcept { return ready && pipeline.isReady(); }
    juce::String getLastError() const { return juce::String (lastError); }
    juce::String getDeviceName() const { return juce::String (backend.getDeviceName()); }
    double getPipelineLatencyMs() const noexcept { return pipeline.getLatencyMs(); }
    uint32_t getUnderrunCount() const noexcept { return pipeline.getUnderrunCount(); }
    bool hasPumpFailed() const noexcept { return pipeline.hasPumpFailed(); }

#if REVERB_DIAGNOSTICS
    void setDiagnostics (ReverbDiagnostics*) {}
#endif

private:
    void pushParameters()
    {
        backend.setParameters (currentParams.rt60, currentParams.rt60LowMult,
                               currentParams.rt60HighMult, currentParams.crossoverLow,
                               currentParams.crossoverHigh, currentParams.diffusion);
    }

    FdnGpuBackend backend;
    GpuAsyncPipelineT<FdnGpuBackend> pipeline;

    double sampleRate { 0.0 };
    int blockSize { 0 };
    int numNodes { 0 };
    bool ready { false };
    std::string lastError;
    AlgorithmParameters currentParams;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverbFDNAlgorithmGPU)
};

#endif // WFS_GPU_NATIVE
