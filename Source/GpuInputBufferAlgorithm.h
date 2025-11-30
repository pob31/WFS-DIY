#pragma once

#if defined(_WIN32) && !defined(NOMINMAX)
#define NOMINMAX 1
#endif

#include <algorithm>
#include <JuceHeader.h>
#include <atomic>
#include <cstdint>
#include <engine_api/GraphLauncher.h>
#include <engine_api/Module.h>
#include <engine_api/ModuleInfo.h>
#include <engine_api/ProcessingGraph.h>
#include <engine_api/LauncherSpecification.h>
#include <gpu_audio_client/GpuAudioManager.h>
#include <gpu_audio_client/ProcessExecutorSync.h>
#include <vector>

namespace WfsInputConfig
{
    struct Specification
    {
        static constexpr uint32_t ConstructionType = 0x57534649; // 'WFSI'
        uint32_t ThisType {ConstructionType};

        uint32_t numInputs {0};
        uint32_t numOutputs {0};
        uint32_t maxSamplesPerChannel {0};
        uint32_t maxDelaySamples {0};
    };

    struct RoutingMessage
    {
        static constexpr uint32_t RoutingType = 0x57534652; // 'WFSR'
        uint32_t ThisMessage {RoutingType};
        uint32_t numInputs {0};
        uint32_t numOutputs {0};
        // Followed in memory by `numInputs * numOutputs` floats of delay (samples),
        // then the same number of floats for gains.
    };
} // namespace WfsInputConfig

/**
    GPU-backed variant of the input-buffer approach.
    Sends the current delay/gain matrix to a custom GPU Audio processor
    (module id: "wfs_input_buffer") each block. Processing topology mirrors
    the CPU InputBufferAlgorithm: per-input delays feeding multiple outputs.
*/
class GpuInputBufferAlgorithm
{
public:
    GpuInputBufferAlgorithm() = default;
    ~GpuInputBufferAlgorithm();

    bool prepare(int numInputs,
                 int numOutputs,
                 double sampleRate,
                 int blockSize,
                 const float* delayTimesPtr,
                 const float* levelsPtr,
                 bool processingEnabled);

    void reprepare(int numInputs,
                   int numOutputs,
                   double sampleRate,
                   int blockSize,
                   const float* delayTimesPtr,
                   const float* levelsPtr,
                   bool processingEnabled);

    void processBlock(const juce::AudioSourceChannelInfo& bufferToFill,
                      int numInputChannels,
                      int numOutputChannels);

    void setProcessingEnabled(bool enabled);
    void releaseResources();
    void clear();

    bool isReady() const noexcept { return executorGuard.executor != nullptr; }
    juce::String getDeviceName() const noexcept { return deviceName; }
    float getLastGpuExecMs() const noexcept { return lastGpuExecMs.load(); }
    int getLastGpuLaunchSamples() const noexcept { return lastGpuLaunchSamples.load(); }
    bool getLastExecuteFailed() const noexcept { return lastExecuteFailed.load(); }

private:
    bool initialiseLauncher();
    bool createGraph();
    bool loadWfsModule();
    bool armProcessor();
    void disarmProcessor();
    void releaseResourcesUnlocked();
    void resetCachePointers(int channels);
    void clearOutputs(const juce::AudioSourceChannelInfo& bufferToFill, int numOutputChannels);
    bool buildRoutingMessage();

    struct ExecutorGuard
    {
        GPUA::engine::v2::Module* module {nullptr};
        GPUA::engine::v2::Processor* processor {nullptr};
        std::unique_ptr<ProcessExecutor<ExecutionMode::eSync>> executor;
        std::vector<GPUA::engine::v2::Processor*> processorList;

        void reset()
        {
            executor.reset();
            if (module != nullptr && processor != nullptr)
                module->DeleteProcessor(processor);
            processor = nullptr;
            processorList.clear();
        }

        void clearModule()
        {
            module = nullptr;
        }
    };

    GPUA::engine::v2::GraphLauncher* launcher {nullptr};
    GPUA::engine::v2::ProcessingGraph* graph {nullptr};
    GPUA::engine::v2::Module* processorModule {nullptr};
    ProcessExecutorConfig executorConfig {};
    ExecutorGuard executorGuard;

    WfsInputConfig::Specification wfsSpec {};

    int inputChannelCount {0};
    int outputChannelCount {0};
    int maxBlockSize {0};
    double currentSampleRate {0.0};
    bool processingEnabledFlag {false};
    juce::String deviceName;
    std::atomic<bool> ready {false};

    const float* delayTimes {nullptr};
    const float* levels {nullptr};
    size_t routingMatrixSize {0};

    std::vector<const float*> inputPtrs;
    std::vector<float*> outputPtrs;
    std::vector<uint8_t> routingMessage;
    juce::AudioBuffer<float> scratchBuffer;
    std::atomic<float> lastGpuExecMs {0.0f};
    std::atomic<int> lastGpuLaunchSamples {0};
    std::atomic<bool> lastExecuteFailed {false};
    juce::SpinLock execLock;
};
