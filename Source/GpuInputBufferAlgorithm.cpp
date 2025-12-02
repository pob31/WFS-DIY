#include "GpuInputBufferAlgorithm.h"

#include <cwchar>
#include <cstring>

namespace
{
    // GPU Audio module identifier for the custom WFS input-buffer processor
    constexpr const wchar_t* kWfsProcessorId = L"wfs_input_buffer";

#if JUCE_WINDOWS
    // Execute GPU call under SEH guard to avoid process termination on driver faults.
    static bool executeGpuSafely(ProcessExecutor<ExecutionMode::eSync>* exec,
                                 uint32_t launchSamples,
                                 const float* const* inputs,
                                 float* const* outputs,
                                 void* appData,
                                 uint32_t appDataSize)
    {
        __try
        {
            exec->Execute<AudioDataLayout::eChannelsIndividual>(launchSamples, inputs, outputs,
                                                                appData, appDataSize);
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
                                 float* const* outputs,
                                 void* appData,
                                 uint32_t appDataSize)
    {
        try
        {
            exec->Execute<AudioDataLayout::eChannelsIndividual>(launchSamples, inputs, outputs,
                                                                appData, appDataSize);
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
                                      const float* delayTimesPtr,
                                      const float* levelsPtr,
                                      bool processingEnabled)
{
    const juce::SpinLock::ScopedLockType lock(execLock);
    ready.store(false, std::memory_order_release);
    processingEnabledFlag = processingEnabled;
    currentSampleRate = sampleRate;

    // Tear down any previous GPU state
    releaseResourcesUnlocked();

    inputChannelCount = juce::jmax(1, numInputs);
    outputChannelCount = juce::jmax(1, numOutputs);
    routingMatrixSize = static_cast<size_t>(inputChannelCount * outputChannelCount);
    maxBlockSize = blockSize;
    delayTimes = delayTimesPtr;
    levels = levelsPtr;

    executorConfig = {};
    executorConfig.nchannels_in = static_cast<uint32_t>(inputChannelCount);
    executorConfig.nchannels_out = static_cast<uint32_t>(outputChannelCount);
    executorConfig.max_samples_per_channel = static_cast<uint32_t>(blockSize);

    const int scratchChannels = juce::jmax(inputChannelCount, outputChannelCount);
    scratchBuffer.setSize(scratchChannels, blockSize);
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

    // Configure processor construction parameters
    wfsSpec = {};
    wfsSpec.numInputs = static_cast<uint32_t>(inputChannelCount);
    wfsSpec.numOutputs = static_cast<uint32_t>(outputChannelCount);
    wfsSpec.maxSamplesPerChannel = static_cast<uint32_t>(blockSize);
    // Mirror CPU path: 1 second of delay line capacity
    wfsSpec.maxDelaySamples = static_cast<uint32_t>(juce::roundToInt(sampleRate));

    if (!loadWfsModule())
    {
        releaseResourcesUnlocked();
        return false;
    }

    if (!armProcessor())
    {
        releaseResourcesUnlocked();
        return false;
    }

    resetCachePointers(scratchChannels);
    ready.store(true, std::memory_order_release);
    return true;
}

void GpuInputBufferAlgorithm::reprepare(int numInputs,
                                        int numOutputs,
                                        double sampleRate,
                                        int blockSize,
                                        const float* delayTimesPtr,
                                        const float* levelsPtr,
                                        bool processingEnabled)
{
    prepare(numInputs, numOutputs, sampleRate, blockSize, delayTimesPtr, levelsPtr, processingEnabled);
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
    if (buffer == nullptr || !executorGuard.executor ||
        inputChannelCount <= 0 || outputChannelCount <= 0 ||
        bufferToFill.numSamples <= 0)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    if (!processingEnabledFlag)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    if (!buildRoutingMessage())
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    const auto* appDataPtr = routingMessage.empty() ? nullptr : routingMessage.data();
    const auto appDataSize = routingMessage.empty() ? 0u : static_cast<uint32_t>(routingMessage.size());

    const int availableInputs = juce::jmin(buffer->getNumChannels(),
                                           numInputChannels,
                                           inputChannelCount);
    const int availableOutputs = juce::jmin(buffer->getNumChannels(),
                                            numOutputChannels,
                                            outputChannelCount);

    scratchBuffer.clear();

    uint32_t remainingSamples = static_cast<uint32_t>(bufferToFill.numSamples);
    int startSample = bufferToFill.startSample;
    const uint32_t chunkSize = executorConfig.max_samples_per_channel;
    const int scratchChannels = juce::jmax(inputChannelCount, outputChannelCount);

    while (remainingSamples > 0)
    {
        const uint32_t launchSamples = juce::jmin(chunkSize, remainingSamples);
        resetCachePointers(scratchChannels);

        for (int ch = 0; ch < inputChannelCount; ++ch)
        {
            if (ch < availableInputs)
            {
                inputPtrs[static_cast<size_t>(ch)] = buffer->getReadPointer(ch, startSample);
            }
            else
            {
                inputPtrs[static_cast<size_t>(ch)] = scratchBuffer.getReadPointer(ch);
            }
        }

        for (int ch = 0; ch < outputChannelCount; ++ch)
        {
            if (ch < availableOutputs)
            {
                outputPtrs[static_cast<size_t>(ch)] = buffer->getWritePointer(ch, startSample);
            }
            else
            {
                outputPtrs[static_cast<size_t>(ch)] = scratchBuffer.getWritePointer(ch);
            }
        }

        const auto gpuStartMs = juce::Time::getMillisecondCounterHiRes();
        const bool ok = (executorGuard.executor != nullptr) &&
                        executeGpuSafely(executorGuard.executor.get(), launchSamples,
                                         inputPtrs.data(), outputPtrs.data(),
                                         const_cast<void*>(static_cast<const void*>(appDataPtr)),
                                         appDataSize);
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
    const int channelsToClear = buffer->getNumChannels();
    if (outputChannelCount < channelsToClear)
    {
        for (int ch = outputChannelCount; ch < channelsToClear; ++ch)
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

    processorModule = nullptr;
    executorGuard.clearModule();
    executorGuard.processor = nullptr;
    inputChannelCount = 0;
    outputChannelCount = 0;
    routingMatrixSize = 0;
    delayTimes = nullptr;
    levels = nullptr;
    maxBlockSize = 0;
    currentSampleRate = 0.0;
    processingEnabledFlag = false;
    deviceName.clear();
    inputPtrs.clear();
    outputPtrs.clear();
    routingMessage.clear();
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

bool GpuInputBufferAlgorithm::loadWfsModule()
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
            std::wcscmp(info.id, kWfsProcessorId) == 0)
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
        DBG("GPU Audio: WFS input processor module not found. Ensure GPUAUDIO_PROCESSOR_PATH points to the built processors.");
        return false;
    }

    if (moduleProvider.GetModule(info, processorModule) != GPUA::engine::v2::ErrorCode::eSuccess || processorModule == nullptr)
    {
        DBG("GPU Audio: Failed to load WFS input module.");
        processorModule = nullptr;
        return false;
    }

    executorGuard.module = processorModule;
    return true;
}

bool GpuInputBufferAlgorithm::armProcessor()
{
    disarmProcessor();

    if (processorModule == nullptr || graph == nullptr || launcher == nullptr)
        return false;

    if (processorModule->CreateProcessor(graph, &wfsSpec, sizeof(wfsSpec), executorGuard.processor) != GPUA::engine::v2::ErrorCode::eSuccess ||
        executorGuard.processor == nullptr)
    {
        DBG("GPU Audio: Failed to create WFS input processor instance.");
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
        processorModule->DeleteProcessor(executorGuard.processor);
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

    if (processorModule != nullptr && executorGuard.processor != nullptr)
    {
        processorModule->DeleteProcessor(executorGuard.processor);
        executorGuard.processor = nullptr;
    }

    executorGuard.processorList.clear();
}

bool GpuInputBufferAlgorithm::buildRoutingMessage()
{
    if (routingMatrixSize == 0 || delayTimes == nullptr || levels == nullptr)
        return false;

    const size_t matrixBytes = routingMatrixSize * sizeof(float);
    const size_t headerBytes = sizeof(WfsInputConfig::RoutingMessage);
    const size_t totalBytes = headerBytes + (matrixBytes * 2);

    routingMessage.resize(totalBytes);

    auto* header = reinterpret_cast<WfsInputConfig::RoutingMessage*>(routingMessage.data());
    header->ThisMessage = WfsInputConfig::RoutingMessage::RoutingType;
    header->numInputs = static_cast<uint32_t>(inputChannelCount);
    header->numOutputs = static_cast<uint32_t>(outputChannelCount);

    // Payload layout: [delays (samples) | gains], both flattened input-major.
    auto* payload = reinterpret_cast<float*>(header + 1);
    const float sampleScale = static_cast<float>(currentSampleRate / 1000.0);

    for (size_t i = 0; i < routingMatrixSize; ++i)
        payload[i] = delayTimes[i] * sampleScale; // convert ms to samples

    std::memcpy(payload + routingMatrixSize, levels, matrixBytes);
    return true;
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
