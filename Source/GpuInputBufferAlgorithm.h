#pragma once

#if defined(_WIN32) && !defined(NOMINMAX)
#define NOMINMAX 1
#endif

#include <algorithm>
#include <JuceHeader.h>
#include <atomic>
#include <engine_api/GraphLauncher.h>
#include <engine_api/Module.h>
#include <engine_api/ModuleInfo.h>
#include <engine_api/ProcessingGraph.h>
#include <engine_api/LauncherSpecification.h>
#include <gpu_audio_client/GpuAudioManager.h>
#include <gpu_audio_client/ProcessExecutorSync.h>
#include <gain_processor/GainSpecification.h>
#include <vector>

/**
    GPU-backed variant of the input-buffer approach.
    Initially runs a simple pass-through (gain = 1) on the GPU to validate
    CPU <-> GPU audio transfer. Processing topology mirrors the CPU path:
    audio arrives per-channel, is forwarded to the GPU, and the result is
    written back into the JUCE buffer.
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
                 bool processingEnabled);

    void reprepare(int numInputs,
                   int numOutputs,
                   double sampleRate,
                   int blockSize,
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
    bool loadGainModule();
    bool armProcessor();
    void disarmProcessor();
    void releaseResourcesUnlocked();
    void resetCachePointers(int channels);
    void clearOutputs(const juce::AudioSourceChannelInfo& bufferToFill, int numOutputChannels);

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
    GPUA::engine::v2::Module* gainModule {nullptr};
    ProcessExecutorConfig executorConfig {};
    ExecutorGuard executorGuard;

    GainConfig::Specification gainSpec {};

    int channelCount {0};
    int maxBlockSize {0};
    bool processingEnabledFlag {false};
    juce::String deviceName;
    std::atomic<bool> ready {false};

    std::vector<const float*> inputPtrs;
    std::vector<float*> outputPtrs;
    juce::AudioBuffer<float> scratchBuffer;
    std::atomic<float> lastGpuExecMs {0.0f};
    std::atomic<int> lastGpuLaunchSamples {0};
    std::atomic<bool> lastExecuteFailed {false};
    juce::SpinLock execLock;
};
