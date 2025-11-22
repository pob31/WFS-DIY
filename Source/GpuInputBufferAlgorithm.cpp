#include "GpuInputBufferAlgorithm.h"

#include <cwchar>

namespace
{
    // GPU Audio module identifier for the built-in gain processor
    constexpr const wchar_t* kGainProcessorId = L"gain";

#if JUCE_WINDOWS
    // Execute GPU call under SEH guard to avoid process termination on driver faults.
    static bool executeGpuSafely(ProcessExecutor<ExecutionMode::eSync>* exec,
                                 uint32_t launchSamples,
                                 const float* const* inputs,
                                 float* const* outputs)
    {
        __try
        {
            exec->Execute<AudioDataLayout::eChannelsIndividual>(launchSamples, inputs, outputs);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }
#else
    static bool executeGpuSafely(ProcessExecutor<ExecutionMode::eSync>* exec,
                                 uint32_t launchSamples,
                                 const float* const* inputs,
                                 float* const* outputs)
    {
        try
        {
            exec->Execute<AudioDataLayout::eChannelsIndividual>(launchSamples, inputs, outputs);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }
#endif
}

GpuInputBufferAlgorithm::~GpuInputBufferAlgorithm()
{
    clear();
}

bool GpuInputBufferAlgorithm::prepare(int numInputs,
                                      int numOutputs,
                                      double sampleRate,
                                      int blockSize,
                                      bool processingEnabled)
{
    juce::ignoreUnused(sampleRate);

    const juce::SpinLock::ScopedLockType lock(execLock);
    ready.store(false, std::memory_order_release);
    processingEnabledFlag = processingEnabled;

    // Tear down any previous GPU state
    releaseResourcesUnlocked();

    channelCount = juce::jmax(1, juce::jmin(numInputs, numOutputs));
    maxBlockSize = blockSize;

    executorConfig = {};
    executorConfig.nchannels_in = static_cast<uint32_t>(channelCount);
    executorConfig.nchannels_out = static_cast<uint32_t>(channelCount);
    executorConfig.max_samples_per_channel = static_cast<uint32_t>(blockSize);

    scratchBuffer.setSize(channelCount, blockSize);
    scratchBuffer.clear();

    if (!initialiseLauncher())
    {
        releaseResourcesUnlocked();
        return false;
    }

    if (!createGraph())
    {
        releaseResourcesUnlocked();
        return false;
    }

    gainSpec = {};
    gainSpec.params.gain_value = 1.0f; // unity pass-through

    if (!loadGainModule())
    {
        releaseResourcesUnlocked();
        return false;
    }

    if (!armProcessor())
    {
        releaseResourcesUnlocked();
        return false;
    }

    resetCachePointers(channelCount);
    ready.store(true, std::memory_order_release);
    return true;
}

void GpuInputBufferAlgorithm::reprepare(int numInputs,
                                        int numOutputs,
                                        double sampleRate,
                                        int blockSize,
                                        bool processingEnabled)
{
    juce::ignoreUnused(sampleRate);
    prepare(numInputs, numOutputs, sampleRate, blockSize, processingEnabled);
}

void GpuInputBufferAlgorithm::processBlock(const juce::AudioSourceChannelInfo& bufferToFill,
                                           int numInputChannels,
                                           int numOutputChannels)
{
    if (!ready.load(std::memory_order_acquire))
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    juce::SpinLock::ScopedTryLockType lock(execLock);
    if (! lock.isLocked())
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    auto* buffer = bufferToFill.buffer;
    if (buffer == nullptr || !executorGuard.executor || channelCount <= 0 || bufferToFill.numSamples <= 0)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    if (!processingEnabledFlag)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    const int availableChannels = juce::jmin(buffer->getNumChannels(),
                                             numInputChannels,
                                             numOutputChannels,
                                             channelCount);

    scratchBuffer.clear();

    uint32_t remainingSamples = static_cast<uint32_t>(bufferToFill.numSamples);
    int startSample = bufferToFill.startSample;
    const uint32_t chunkSize = executorConfig.max_samples_per_channel;

    while (remainingSamples > 0)
    {
        const uint32_t launchSamples = juce::jmin(chunkSize, remainingSamples);
        resetCachePointers(channelCount);

        for (int ch = 0; ch < channelCount; ++ch)
        {
            if (ch < availableChannels)
            {
                inputPtrs[static_cast<size_t>(ch)] = buffer->getReadPointer(ch, startSample);
                outputPtrs[static_cast<size_t>(ch)] = buffer->getWritePointer(ch, startSample);
            }
            else
            {
                inputPtrs[static_cast<size_t>(ch)] = scratchBuffer.getReadPointer(ch);
                outputPtrs[static_cast<size_t>(ch)] = scratchBuffer.getWritePointer(ch);
            }
        }

        const auto gpuStartMs = juce::Time::getMillisecondCounterHiRes();
        const bool ok = (executorGuard.executor != nullptr) &&
                        executeGpuSafely(executorGuard.executor.get(), launchSamples,
                                         inputPtrs.data(), outputPtrs.data());
        const auto gpuEndMs = juce::Time::getMillisecondCounterHiRes();

        lastExecuteFailed.store(!ok, std::memory_order_release);
        if (!ok)
        {
            DBG("GPU Audio: Execute failed (driver/SDK error). Disabling GPU path.");
            ready.store(false, std::memory_order_release);
            return;
        }
        lastGpuExecMs.store((float)(gpuEndMs - gpuStartMs), std::memory_order_release);
        lastGpuLaunchSamples.store((int)launchSamples, std::memory_order_release);

        remainingSamples -= launchSamples;
        startSample += static_cast<int>(launchSamples);
    }

    // Clear any channels we did not process (device outputs beyond our GPU routed channels)
    if (availableChannels < buffer->getNumChannels())
    {
        for (int ch = availableChannels; ch < buffer->getNumChannels(); ++ch)
        {
            buffer->clear(ch, bufferToFill.startSample, bufferToFill.numSamples);
        }
    }
}

void GpuInputBufferAlgorithm::setProcessingEnabled(bool enabled)
{
    processingEnabledFlag = enabled;
}

void GpuInputBufferAlgorithm::releaseResources()
{
    const juce::SpinLock::ScopedLockType lock(execLock);
    ready.store(false, std::memory_order_release);
    releaseResourcesUnlocked();
}

void GpuInputBufferAlgorithm::releaseResourcesUnlocked()
{
    disarmProcessor();

    if (graph && launcher)
    {
        launcher->DeleteProcessingGraph(graph);
        graph = nullptr;
    }

    if (launcher)
    {
        if (auto* gpuAudio = GpuAudioManager::GetGpuAudio())
            gpuAudio->DeleteLauncher(launcher);
        launcher = nullptr;
    }

    executorGuard.reset();
}

void GpuInputBufferAlgorithm::clear()
{
    const juce::SpinLock::ScopedLockType lock(execLock);
    ready.store(false, std::memory_order_release);

    releaseResourcesUnlocked();

    gainModule = nullptr;
    executorGuard.clearModule();
    executorGuard.processor = nullptr;
    channelCount = 0;
    maxBlockSize = 0;
    processingEnabledFlag = false;
    deviceName.clear();
    inputPtrs.clear();
    outputPtrs.clear();
    scratchBuffer.setSize(0, 0);
    lastGpuExecMs.store(0.0f, std::memory_order_release);
    lastGpuLaunchSamples.store(0, std::memory_order_release);
    lastExecuteFailed.store(false, std::memory_order_release);
}

bool GpuInputBufferAlgorithm::initialiseLauncher()
{
    if (launcher != nullptr)
        return true;

    auto* gpuAudio = GpuAudioManager::GetGpuAudio();
    if (gpuAudio == nullptr)
    {
        DBG("GPU Audio: No GPU audio engine available. Check GPUAUDIO_PATH and GPU drivers.");
        return false;
    }

    // Diagnostics: log environment paths and available devices
    DBG("GPU Audio: GPUAUDIO_PATH=" + juce::String(::getenv("GPUAUDIO_PATH")));
    DBG("GPU Audio: GPUAUDIO_PROCESSOR_PATH=" + juce::String(::getenv("GPUAUDIO_PROCESSOR_PATH")));
    const auto& deviceInfoProvider = gpuAudio->GetDeviceInfoProvider();
    const auto deviceCount = deviceInfoProvider.GetDeviceCount();
    DBG("GPU Audio: device count = " + juce::String((int) deviceCount));
    for (uint32_t i = 0; i < deviceCount; ++i)
    {
        const GPUA::engine::v2::DeviceInfo* devInfo = nullptr;
        const auto errDev = deviceInfoProvider.GetDeviceInfo(i, devInfo);
        if (errDev == GPUA::engine::v2::ErrorCode::eSuccess && devInfo != nullptr && devInfo->name != nullptr)
            DBG("GPU Audio: device[" + juce::String((int) i) + "] name=" + juce::String(devInfo->name) +
                " id=" + (devInfo->device_id ? juce::String(devInfo->device_id) : juce::String("<null>")) +
                " platforms=" + (devInfo->platforms ? juce::String(devInfo->platforms) : juce::String("<null>")));
        else
            DBG("GPU Audio: device[" + juce::String((int) i) + "] query failed, code=" + juce::String((int) errDev));
    }

    GPUA::engine::v2::LauncherSpecification launcherSpec {};
    const auto deviceIndex = GpuAudioManager::GetDeviceIndex();
    const GPUA::engine::v2::DeviceInfo* deviceInfo = nullptr;
    const auto errInfo = deviceInfoProvider.GetDeviceInfo(deviceIndex, deviceInfo);

    if (errInfo != GPUA::engine::v2::ErrorCode::eSuccess || deviceInfo == nullptr)
    {
        DBG("GPU Audio: Failed to query device info for launcher.");
        deviceName.clear();
        return false;
    }

    deviceName = deviceInfo->name != nullptr ? juce::String(deviceInfo->name) : juce::String();

    launcherSpec.device_info = deviceInfo;

    const auto errCreate = gpuAudio->CreateLauncher(launcherSpec, launcher);
    if (errCreate != GPUA::engine::v2::ErrorCode::eSuccess || launcher == nullptr)
    {
        DBG("GPU Audio: Failed to create launcher.");
        launcher = nullptr;
        deviceName.clear();
        return false;
    }

    return true;
}

bool GpuInputBufferAlgorithm::createGraph()
{
    if (launcher == nullptr)
        return false;

    if (graph != nullptr)
        return true;

    if (launcher->CreateProcessingGraph(graph) != GPUA::engine::v2::ErrorCode::eSuccess || graph == nullptr)
    {
        DBG("GPU Audio: Failed to create processing graph.");
        graph = nullptr;
        return false;
    }

    return true;
}

bool GpuInputBufferAlgorithm::loadGainModule()
{
    if (launcher == nullptr)
        return false;

    auto& moduleProvider = launcher->GetModuleProvider();
    const auto moduleCount = moduleProvider.GetModulesCount();

    GPUA::engine::v2::ModuleInfo info {};
    bool found = false;
    DBG("GPU Audio: module count = " + juce::String((int) moduleCount));
    for (uint32_t i = 0; i < moduleCount; ++i)
    {
        if (moduleProvider.GetModuleInfo(i, info) == GPUA::engine::v2::ErrorCode::eSuccess &&
            info.id != nullptr &&
            std::wcscmp(info.id, kGainProcessorId) == 0)
        {
            found = true;
            break;
        }
        else if (info.id != nullptr)
        {
            DBG("GPU Audio: module[" + juce::String((int) i) + "] id=" + juce::String(info.id));
        }
        else
        {
            DBG("GPU Audio: module[" + juce::String((int) i) + "] has null id");
        }
    }

    if (!found)
    {
        DBG("GPU Audio: Gain processor module not found. Ensure GPUAUDIO_PROCESSOR_PATH points to the built processors.");
        return false;
    }

    if (moduleProvider.GetModule(info, gainModule) != GPUA::engine::v2::ErrorCode::eSuccess || gainModule == nullptr)
    {
        DBG("GPU Audio: Failed to load gain module.");
        gainModule = nullptr;
        return false;
    }

    executorGuard.module = gainModule;
    return true;
}

bool GpuInputBufferAlgorithm::armProcessor()
{
    disarmProcessor();

    if (gainModule == nullptr || graph == nullptr || launcher == nullptr)
        return false;

    if (gainModule->CreateProcessor(graph, &gainSpec, sizeof(gainSpec), executorGuard.processor) != GPUA::engine::v2::ErrorCode::eSuccess ||
        executorGuard.processor == nullptr)
    {
        DBG("GPU Audio: Failed to create gain processor instance.");
        executorGuard.processor = nullptr;
        return false;
    }

    executorGuard.processorList.clear();
    executorGuard.processorList.push_back(executorGuard.processor);

    try
    {
        executorGuard.executor = std::make_unique<ProcessExecutor<ExecutionMode::eSync>>(launcher,
                                                                                         graph,
                                                                                         1,
                                                                                         executorGuard.processorList.data(),
                                                                                         executorConfig);
    }
    catch (const std::exception& e)
    {
        DBG("GPU Audio: Executor creation failed: " + juce::String(e.what()));
        gainModule->DeleteProcessor(executorGuard.processor);
        executorGuard.processor = nullptr;
        executorGuard.processorList.clear();
        executorGuard.executor.reset();
        return false;
    }

    return true;
}

void GpuInputBufferAlgorithm::disarmProcessor()
{
    // Order matters: destroy the executor first (it may hold references to the processor),
    // then delete the processor instance.
    executorGuard.executor.reset();

    if (gainModule != nullptr && executorGuard.processor != nullptr)
    {
        gainModule->DeleteProcessor(executorGuard.processor);
        executorGuard.processor = nullptr;
    }

    executorGuard.processorList.clear();
}

void GpuInputBufferAlgorithm::resetCachePointers(int channels)
{
    inputPtrs.resize(static_cast<size_t>(channels));
    outputPtrs.resize(static_cast<size_t>(channels));
}

void GpuInputBufferAlgorithm::clearOutputs(const juce::AudioSourceChannelInfo& bufferToFill,
                                           int numOutputChannels)
{
    if (bufferToFill.buffer == nullptr)
        return;

    const int totalChannels = bufferToFill.buffer->getNumChannels();
    const int channelsToClear = juce::jmin(numOutputChannels, totalChannels);
    for (int ch = 0; ch < channelsToClear; ++ch)
        bufferToFill.buffer->clear(ch, bufferToFill.startSample, bufferToFill.numSamples);
}
