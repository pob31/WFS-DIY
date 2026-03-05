#if GPU_AUDIO_ENABLED
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
    lastError.clear();
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
    static int diagCounter = 0;
    const bool diagLog = (diagCounter < 30);

    if (!ready.load(std::memory_order_acquire))
    {
        if (diagLog) { DBG("GPU diag: not ready"); ++diagCounter; }
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    juce::SpinLock::ScopedTryLockType lock(execLock);
    if (! lock.isLocked())
    {
        if (diagLog) { DBG("GPU diag: lock failed"); ++diagCounter; }
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    auto* buffer = bufferToFill.buffer;
    if (buffer == nullptr || !executorGuard.executor ||
        inputChannelCount <= 0 || outputChannelCount <= 0 ||
        bufferToFill.numSamples <= 0)
    {
        if (diagLog) { DBG("GPU diag: null/zero guard (buf=" + juce::String((int)(buffer != nullptr))
            + " exec=" + juce::String((int)(executorGuard.executor != nullptr))
            + " in=" + juce::String(inputChannelCount)
            + " out=" + juce::String(outputChannelCount)
            + " samples=" + juce::String(bufferToFill.numSamples) + ")"); ++diagCounter; }
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    if (!processingEnabledFlag)
    {
        if (diagLog) { DBG("GPU diag: processingEnabledFlag=false"); ++diagCounter; }
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    if (!buildRoutingMessage())
    {
        if (diagLog) { DBG("GPU diag: buildRoutingMessage failed (matrixSize=" + juce::String((int)routingMatrixSize)
            + " delays=" + juce::String((int)(delayTimes != nullptr)) + " levels=" + juce::String((int)(levels != nullptr)) + ")"); ++diagCounter; }
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

        // Fill outputPtrs for all channels the GPU port may report
        // (defensively covers max of input/output to prevent out-of-bounds in SDK callback)
        for (int ch = 0; ch < scratchChannels; ++ch)
        {
            if (ch < availableOutputs)
            {
                outputPtrs[static_cast<size_t>(ch)] = buffer->getWritePointer(ch, startSample);
            }
            else
            {
                outputPtrs[static_cast<size_t>(ch)] = scratchBuffer.getWritePointer(ch % scratchBuffer.getNumChannels());
            }
        }

        // Measure input BEFORE execute (SyncOutputCallback will overwrite the buffer)
        float preMaxIn = 0.0f;
        float rawMaxAny = 0.0f;
        int rawMaxCh = -1;
        if (diagLog)
        {
            for (int ch = 0; ch < juce::jmin(availableInputs, 2); ++ch)
                for (uint32_t s = 0; s < juce::jmin(launchSamples, 64u); ++s)
                    preMaxIn = juce::jmax(preMaxIn, std::abs(inputPtrs[ch][s]));

            // Scan ALL buffer channels for any audio (find where audio actually lives)
            for (int ch = 0; ch < buffer->getNumChannels(); ++ch)
            {
                const float* data = buffer->getReadPointer(ch, startSample);
                for (uint32_t s = 0; s < juce::jmin(launchSamples, 16u); ++s)
                {
                    float v = std::abs(data[s]);
                    if (v > rawMaxAny)
                    {
                        rawMaxAny = v;
                        rawMaxCh = ch;
                    }
                }
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

        if (diagLog)
        {
            // Check gain and delay values in routing message
            float maxGain = 0.0f;
            float maxDelay = 0.0f;
            float minDelay = 0.0f;
            if (!routingMessage.empty())
            {
                auto* hdr = reinterpret_cast<const WfsInputConfig::RoutingMessage*>(routingMessage.data());
                const float* payload = reinterpret_cast<const float*>(hdr + 1);
                const uint32_t mSize = hdr->numInputs * hdr->numOutputs;
                for (uint32_t g = 0; g < mSize; ++g)
                    maxGain = juce::jmax(maxGain, std::abs(payload[mSize + g])); // gains after delays
                for (uint32_t d = 0; d < mSize; ++d)
                {
                    maxDelay = juce::jmax(maxDelay, payload[d]);
                    if (d == 0) minDelay = payload[d];
                    else minDelay = juce::jmin(minDelay, payload[d]);
                }
            }

            // Check output max amplitude (post-execute, buffer may be overwritten by callback)
            float maxOut = 0.0f;
            for (int ch = 0; ch < juce::jmin(availableOutputs, 2); ++ch)
                for (uint32_t s = 0; s < juce::jmin(launchSamples, 64u); ++s)
                    maxOut = juce::jmax(maxOut, std::abs(outputPtrs[ch][s]));

            DBG("GPU diag: exec OK, launch=" + juce::String(launchSamples)
                + " maxGain=" + juce::String(maxGain, 6)
                + " preMaxIn=" + juce::String(preMaxIn, 6)
                + " maxOut=" + juce::String(maxOut, 6)
                + " delay=" + juce::String(minDelay, 1) + "-" + juce::String(maxDelay, 1)
                + " rawMaxAny=" + juce::String(rawMaxAny, 6)
                + " availIn=" + juce::String(availableInputs)
                + " availOut=" + juce::String(availableOutputs));
            ++diagCounter;
        }

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
        lastError = "No GPU Audio engine available. Check GPU Audio Platform installation and GPU drivers.";
        DBG("GPU Audio: " + lastError);
        return false;
    }

    const auto& deviceInfoProvider = gpuAudio->GetDeviceInfoProvider();

    GPUA::engine::v2::LauncherSpecification launcherSpec {};
    const auto deviceIndex = GpuAudioManager::GetDeviceIndex();
    const GPUA::engine::v2::DeviceInfo* deviceInfo = nullptr;
    const auto errInfo = deviceInfoProvider.GetDeviceInfo(deviceIndex, deviceInfo);

    if (errInfo != GPUA::engine::v2::ErrorCode::eSuccess || deviceInfo == nullptr)
    {
        lastError = "Failed to query GPU device info for launcher.";
        DBG("GPU Audio: " + lastError);
        deviceName.clear();
        return false;
    }

    deviceName = deviceInfo->name != nullptr ? juce::String(deviceInfo->name) : juce::String();

    launcherSpec.device_info = deviceInfo;

    const auto errCreate = gpuAudio->CreateLauncher(launcherSpec, launcher);
    if (errCreate != GPUA::engine::v2::ErrorCode::eSuccess || launcher == nullptr)
    {
        lastError = "Failed to create GPU Audio launcher.";
        DBG("GPU Audio: " + lastError);
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
        lastError = "Failed to create GPU Audio processing graph.";
        DBG("GPU Audio: " + lastError);
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
    for (uint32_t i = 0; i < moduleCount; ++i)
    {
        if (moduleProvider.GetModuleInfo(i, info) == GPUA::engine::v2::ErrorCode::eSuccess &&
            info.id != nullptr &&
            std::wcscmp(info.id, kWfsProcessorId) == 0)
        {
            found = true;
            break;
        }
    }

    if (!found)
    {
        juce::String moduleList;
        for (uint32_t i = 0; i < moduleCount; ++i)
        {
            GPUA::engine::v2::ModuleInfo mi {};
            if (moduleProvider.GetModuleInfo(i, mi) == GPUA::engine::v2::ErrorCode::eSuccess && mi.id != nullptr)
                moduleList += "\n  - " + juce::String(mi.id);
        }
        lastError = "WFS module 'wfs_input' not found (" + juce::String(moduleCount)
                    + " modules available):" + moduleList
                    + "\nCheck GPUAUDIO_PROCESSOR_PATH env var.";
        DBG("GPU Audio: " + lastError);
        return false;
    }

    if (moduleProvider.GetModule(info, processorModule) != GPUA::engine::v2::ErrorCode::eSuccess || processorModule == nullptr)
    {
        lastError = "Failed to load WFS input module.";
        DBG("GPU Audio: " + lastError);
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
        lastError = "Failed to create WFS input processor instance.";
        DBG("GPU Audio: " + lastError);
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
        lastError = "Executor creation failed: " + juce::String(e.what());
        DBG("GPU Audio: " + lastError);
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

    // Subtract minimum delay so all delays are relative (keeps them within buffer size)
    float minDelay = std::numeric_limits<float>::max();
    for (size_t i = 0; i < routingMatrixSize; ++i)
        minDelay = std::min(minDelay, payload[i]);
    if (minDelay > 0.0f)
    {
        for (size_t i = 0; i < routingMatrixSize; ++i)
            payload[i] -= minDelay;
    }

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
#endif // GPU_AUDIO_ENABLED
