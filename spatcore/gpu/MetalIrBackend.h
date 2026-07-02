#pragma once

/*
    MetalIrBackend — native Metal execution of the GPU IR-reverb convolution.

    JUCE-free, SDK-free, structured exactly like MetalWfsBackend: pImpl over
    the Metal objects, MSL kernel string compiled at prepare() time, and a
    synchronous (commit + wait) processBlock driven by the GpuAsyncPipelineT
    pump, which provides the deadline isolation above it.

    Hybrid convolution (see IrConvHostState / IrHostFft): the host FFTs each
    node's input block into a per-node ring of frequency-domain segments held
    in a shared-storage MTLBuffer, one GPU kernel (ir_fdl_mac) accumulates
    ring x IR-spectra over all loaded segments, and the host inverse-FFTs and
    overlap-adds the result. The IR is staged from the engine thread and
    transformed progressively (budgeted segments per launch), mirroring the
    background-loading behaviour of juce::dsp::Convolution.

    Threading contract: single caller thread for processBlock() (the pump);
    stageIr()/requestReset() may be called from the engine thread between
    blocks (lock-free handoff inside IrConvHostState).
*/

#include <cstdint>
#include <memory>
#include <string>

class MetalIrBackend
{
public:
    explicit MetalIrBackend (int deviceIndex = 0);
    ~MetalIrBackend();

    MetalIrBackend (const MetalIrBackend&) = delete;
    MetalIrBackend& operator= (const MetalIrBackend&) = delete;

    /** Creates device objects and the persistent spectra buffers.
        blockSize must be a power of two in [4, 1024] (the engine's internal
        reverb block; non-conforming driver buffers decline here and the
        engine falls back to the CPU IR algorithm).
        maxIrSamples bounds the device allocation (10 s * sr typical).
        Returns false with getLastError() set on failure. */
    bool prepare (int numNodes,
                  int blockSize,
                  double sampleRate,
                  int maxIrSamples);

    /** Hands a final mono IR (already trimmed/resampled/normalised by the
        caller) to the pump thread. Callable any time after prepare(). */
    void stageIr (const float* monoIr, int numSamples);

    /** Clears input history + overlap tails at the next launch (loaded IR
        spectra survive). Callable from any thread. */
    void requestReset() noexcept;

    /** Processes one block synchronously on the GPU (pump thread).
        inputs/outputs: numNodes channel pointers, blockSize samples each.
        Returns false on device error. */
    bool processBlock (const float* const* inputs, float* const* outputs);

    void release() noexcept;

    bool isReady() const noexcept { return ready; }
    const std::string& getLastError() const noexcept { return lastError; }
    double getLastLaunchMs() const noexcept { return lastLaunchMs; }
    const std::string& getDeviceName() const noexcept { return deviceName; }

    /** Progressive IR loader status (any thread, for diagnostics/UI). */
    int getSegmentsLoaded() const noexcept;
    int getSegmentsTotal() const noexcept;

private:
    struct Impl;                 // Objective-C++ internals (Metal objects)
    std::unique_ptr<Impl> impl;

    bool ready { false };
    std::string lastError;
    std::string deviceName { "Apple Silicon (Metal)" };
    double lastLaunchMs { 0.0 };
};
