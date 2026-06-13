#pragma once
#if WFS_GPU_NATIVE

/*
    NativeGpuWfsAlgorithm — the WFS delay-and-sum input-buffer algorithm on
    native Metal. Drop-in third algorithm beside the CPU InputBuffer and
    OutputBuffer implementations: same prepare/processBlock/release protocol,
    same matrices, same audible behavior (delays, gains, per-sample ramps).

    Composition:
      WfsGpuBackend    — Metal or CUDA backend (compile-time alias): owns the
                         GPU objects, persistent delay rings (direct + FR),
                         per-pair HF shelf states, prev->curr matrix
                         snapshots, -L latency compensation
      GpuAsyncPipeline — pump thread + lock-free rings; the audio callback
                         never waits on the GPU

    DSP parity with the CPU InputBuffer path: per-pair 800 Hz HF
    air-absorption shelf on both taps and Floor Reflections (per-input
    pre-filter chain, diffusion jitter, image-source delay/level matrices).
    FR gains/delays ramp per sample where the CPU steps them at 50 Hz. The
    CPU path remains the reference implementation.
*/

#include <JuceHeader.h>
#include <atomic>

#include "WfsGpuBackend.h"
#include "GpuAsyncPipeline.h"
#include "GpuLevelMeters.h"

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
                  const float* hfAttenuationPtr = nullptr,
                  const float* frDelayTimesPtr = nullptr,
                  const float* frLevelsPtr = nullptr,
                  const float* frHFAttenuationPtr = nullptr,
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
        backend.setMatrixPointers (delayTimesMsPtr, levelsPtr,
                                   hfAttenuationPtr, frDelayTimesPtr,
                                   frLevelsPtr, frHFAttenuationPtr);

        if (! pipeline.prepare (&backend, inputChannelCount, outputChannelCount,
                                blockSize, sampleRate, depth))
        {
            lastError = pipeline.getLastError();
            DBG ("Native GPU WFS: pipeline init failed: " + lastError);
            backend.release();
            return false;
        }

        // Host-side level meters track the same channel counts; rebuilt here
        // under procLock so the audio thread (gated by ready==false) never
        // touches a half-built follower vector.
        meters.prepare (inputChannelCount, outputChannelCount, sampleRate);

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
        if (! lock.isLocked())
        {
            // procLock contended by prepare()/releaseResources(): skip metering
            // entirely (the follower vectors may be mid-rebuild).
            bufferToFill.clearActiveBufferRegion();
            return;
        }

        if (bufferToFill.buffer == nullptr || ! processingEnabledFlag || ! pipeline.isReady())
        {
            // Bypassed but lock held: let the meters decay toward silence.
            bufferToFill.clearActiveBufferRegion();
            meters.meterSilence (bufferToFill.numSamples);
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
        meters.meterInput (inputBuffer, availableInputs, bufferToFill.startSample, bufferToFill.numSamples);

        pipeline.popOutput (*buffer, availableOutputs, bufferToFill.startSample, bufferToFill.numSamples);
        meters.meterOutput (*buffer, availableOutputs, bufferToFill.startSample, bufferToFill.numSamples);

        for (int ch = availableOutputs; ch < buffer->getNumChannels(); ++ch)
            buffer->clear (ch, bufferToFill.startSample, bufferToFill.numSamples);
    }

    void setProcessingEnabled (bool enabled)   { processingEnabledFlag = enabled; }

    // === Level metering (host-side; mirrors the CPU algorithm interface) ===
    void  setOutputMeteringEnabled (bool enabled) noexcept { meters.setOutputMeteringEnabled (enabled); }
    float getShortPeakLevelDb (size_t i) const noexcept { return meters.getShortPeakLevelDb (i); }
    float getRmsLevelDb       (size_t i) const noexcept { return meters.getRmsLevelDb (i); }
    float getInputPeakLevelDb (size_t i) const noexcept { return meters.getInputPeakLevelDb (i); }
    float getInputRmsLevelDb  (size_t i) const noexcept { return meters.getInputRmsLevelDb (i); }
    float getOutputPeakLevelDb (size_t i) const noexcept { return meters.getOutputPeakLevelDb (i); }
    float getOutputRmsLevelDb  (size_t i) const noexcept { return meters.getOutputRmsLevelDb (i); }

    // === Floor Reflection parameter setters (50 Hz timer thread) ===
    // Same signatures as InputBufferAlgorithm; forwarded to the backend's
    // host-side FR state (atomics; safe before prepare - bounds-checked).

    void setFRFilterParams (size_t inputIndex,
                            bool lowCutActive, float lowCutFreq,
                            bool highShelfActive, float highShelfFreq,
                            float highShelfGain, float highShelfSlope)
    {
        backend.setFRFilterParams ((int) inputIndex, lowCutActive, lowCutFreq,
                                   highShelfActive, highShelfFreq,
                                   highShelfGain, highShelfSlope);
    }

    void setFRDiffusion (size_t inputIndex, float diffusionPercent)
    {
        backend.setFRDiffusion ((int) inputIndex, diffusionPercent);
    }

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
    juce::String getDeviceName() const         { return juce::String (backend.getDeviceName()); }
    float getLastGpuExecMs() const noexcept    { return pipeline.getLastPumpMs(); }
    float getAndResetPeakGpuExecMs() noexcept  { return pipeline.getAndResetPeakPumpMs(); }
    uint32_t getUnderrunCount() const noexcept { return pipeline.getUnderrunCount(); }
    double getPipelineLatencyMs() const noexcept { return pipeline.getLatencyMs(); }
    int getPipelineDepthBlocks() const noexcept  { return pipeline.getDepthBlocks(); }

private:
    WfsGpuBackend backend;
    GpuAsyncPipeline pipeline;
    GpuLevelMeters meters;

    int inputChannelCount { 0 };
    int outputChannelCount { 0 };
    bool processingEnabledFlag { false };
    juce::String lastError;
    std::atomic<bool> ready { false };
    juce::SpinLock procLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NativeGpuWfsAlgorithm)
};

#endif // WFS_GPU_NATIVE
