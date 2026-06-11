#pragma once
#if WFS_GPU_NATIVE

/*
    NativeGpuWfsAlgorithm — the WFS delay-and-sum input-buffer algorithm on
    native Metal. Drop-in third algorithm beside the CPU InputBuffer and
    OutputBuffer implementations: same prepare/processBlock/release protocol,
    same matrices, same audible behavior (delays, gains, per-sample ramps).

    Composition:
      MetalWfsBackend  — owns the Metal objects, persistent delay rings,
                         prev->curr matrix snapshots, -L latency compensation
      GpuAsyncPipeline — pump thread + lock-free rings; the audio callback
                         never waits on the GPU

    Limitations vs the CPU path (same as the SDK-era GPU mode): no per-pair
    HF air-absorption shelf and no Floor Reflections yet (kernel parity is a
    planned phase); the CPU path remains the reference implementation.
*/

#include <JuceHeader.h>
#include <atomic>

#include "MetalWfsBackend.h"
#include "GpuAsyncPipeline.h"

class NativeGpuWfsAlgorithm
{
public:
    static constexpr int kDefaultDepthBlocks = 4;

    NativeGpuWfsAlgorithm() = default;
    ~NativeGpuWfsAlgorithm() { clear(); }

    bool prepare (int numInputs,
                  int numOutputs,
                  double sampleRate,
                  int blockSize,
                  const float* delayTimesMsPtr,
                  const float* levelsPtr,
                  bool processingEnabled,
                  int pipelineDepthBlocks = kDefaultDepthBlocks)
    {
        const juce::SpinLock::ScopedLockType lock (procLock);
        ready.store (false, std::memory_order_release);
        lastError.clear();
        processingEnabledFlag = processingEnabled;

        pipeline.release();
        backend.release();

        inputChannelCount = juce::jmax (1, numInputs);
        outputChannelCount = juce::jmax (1, numOutputs);

        const int depth = juce::jlimit (GpuAsyncPipeline::kMinDepthBlocks,
                                        GpuAsyncPipeline::kMaxDepthBlocks,
                                        pipelineDepthBlocks);
        const double latencyMs = depth * 1000.0 * blockSize / sampleRate;

        if (! backend.prepare (inputChannelCount, outputChannelCount,
                               blockSize, sampleRate, latencyMs))
        {
            lastError = juce::String (backend.getLastError());
            DBG ("Native GPU WFS: backend init failed: " + lastError);
            return false;
        }
        backend.setMatrixPointers (delayTimesMsPtr, levelsPtr);

        if (! pipeline.prepare (&backend, inputChannelCount, outputChannelCount,
                                blockSize, sampleRate, depth))
        {
            lastError = pipeline.getLastError();
            DBG ("Native GPU WFS: pipeline init failed: " + lastError);
            backend.release();
            return false;
        }

        ready.store (true, std::memory_order_release);
        return true;
    }

    void processBlock (const juce::AudioSourceChannelInfo& bufferToFill,
                       const juce::AudioBuffer<float>& inputBuffer,
                       int numInputChannels,
                       int numOutputChannels)
    {
        if (! ready.load (std::memory_order_acquire))
        {
            bufferToFill.clearActiveBufferRegion();
            return;
        }

        const juce::SpinLock::ScopedTryLockType lock (procLock);
        if (! lock.isLocked() || bufferToFill.buffer == nullptr || ! processingEnabledFlag
            || ! pipeline.isReady())
        {
            bufferToFill.clearActiveBufferRegion();
            if (pipeline.hasPumpFailed())
                ready.store (false, std::memory_order_release);
            return;
        }

        auto* buffer = bufferToFill.buffer;
        const int availableInputs = juce::jmin (inputBuffer.getNumChannels(),
                                                numInputChannels, inputChannelCount);
        const int availableOutputs = juce::jmin (buffer->getNumChannels(),
                                                 numOutputChannels, outputChannelCount);

        pipeline.pushInput (inputBuffer, availableInputs, bufferToFill.startSample, bufferToFill.numSamples);
        pipeline.popOutput (*buffer, availableOutputs, bufferToFill.startSample, bufferToFill.numSamples);

        for (int ch = availableOutputs; ch < buffer->getNumChannels(); ++ch)
            buffer->clear (ch, bufferToFill.startSample, bufferToFill.numSamples);
    }

    void setProcessingEnabled (bool enabled)   { processingEnabledFlag = enabled; }

    void releaseResources()
    {
        const juce::SpinLock::ScopedLockType lock (procLock);
        ready.store (false, std::memory_order_release);
        pipeline.release();
        backend.release();
    }

    void clear()
    {
        releaseResources();
        inputChannelCount = outputChannelCount = 0;
        lastError.clear();
    }

    bool isReady() const noexcept              { return ready.load (std::memory_order_acquire); }
    juce::String getLastError() const          { return lastError; }
    juce::String getDeviceName() const         { return "Apple Silicon (Metal)"; }
    float getLastGpuExecMs() const noexcept    { return pipeline.getLastPumpMs(); }
    float getAndResetPeakGpuExecMs() noexcept  { return pipeline.getAndResetPeakPumpMs(); }
    uint32_t getUnderrunCount() const noexcept { return pipeline.getUnderrunCount(); }
    double getPipelineLatencyMs() const noexcept { return pipeline.getLatencyMs(); }
    int getPipelineDepthBlocks() const noexcept  { return pipeline.getDepthBlocks(); }

private:
    MetalWfsBackend backend;
    GpuAsyncPipeline pipeline;

    int inputChannelCount { 0 };
    int outputChannelCount { 0 };
    bool processingEnabledFlag { false };
    juce::String lastError;
    std::atomic<bool> ready { false };
    juce::SpinLock procLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NativeGpuWfsAlgorithm)
};

#endif // WFS_GPU_NATIVE
