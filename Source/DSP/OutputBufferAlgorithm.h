#pragma once

#include "OutputBufferProcessor.h"
#include "SharedInputRingBuffer.h"
#include "LiveSourceLevelDetector.h"
#include "OutputLevelDetector.h"
#include <vector>
#include <memory>
#include <atomic>

//==============================================================================
/**
    Dedicated thread for input-side analysis (level detection, Live Source Tamer).
    Reads from shared input ring buffers independently of the audio callback.
*/
class InputAnalysisThread : public juce::Thread
{
public:
    InputAnalysisThread() : juce::Thread("InputAnalysis") {}

    ~InputAnalysisThread() override { stopThread(1000); }

    void prepare(const std::vector<std::unique_ptr<SharedInputRingBuffer>>& sharedBuffers,
                 const std::vector<std::unique_ptr<LiveSourceLevelDetector>>& detectors,
                 int blockSize)
    {
        sharedInputs.clear();
        lsDetectors.clear();

        for (auto& buf : sharedBuffers)
            sharedInputs.push_back(buf.get());
        for (auto& det : detectors)
            lsDetectors.push_back(det.get());

        readPositions.assign(sharedInputs.size(), 0);
        processingBlockSize = blockSize;
        inputBlock.setSize(1, blockSize);
    }

    void notifyInputAvailable(int available)
    {
        samplesAvailable.store(available, std::memory_order_release);
        notify();
    }

private:
    void run() override
    {
        while (!threadShouldExit())
        {
            if (samplesAvailable.load(std::memory_order_acquire) < processingBlockSize)
            {
                wait(10);  // Sleep longer when idle
                continue;
            }

            int numInputs = juce::jmin((int)sharedInputs.size(), (int)lsDetectors.size());

            for (int ch = 0; ch < numInputs; ++ch)
            {
                int read = sharedInputs[ch]->readWithPosition(
                    readPositions[ch], inputBlock.getWritePointer(0), processingBlockSize);

                if (read > 0 && lsDetectors[ch] != nullptr)
                {
                    auto* data = inputBlock.getReadPointer(0);
                    for (int i = 0; i < read; ++i)
                        lsDetectors[ch]->processSample(data[i]);
                }
            }

            // Update available from our read positions
            int minAvail = std::numeric_limits<int>::max();
            for (int i = 0; i < numInputs; ++i)
                minAvail = juce::jmin(minAvail, sharedInputs[i]->getAvailableAt(readPositions[i]));
            samplesAvailable.store(minAvail, std::memory_order_release);
        }
    }

    std::vector<SharedInputRingBuffer*> sharedInputs;
    std::vector<LiveSourceLevelDetector*> lsDetectors;
    std::vector<int> readPositions;
    std::atomic<int> samplesAvailable{0};
    int processingBlockSize = 64;
    juce::AudioBuffer<float> inputBlock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InputAnalysisThread)
};

//==============================================================================
/**
    Dedicated thread for output level metering.
    Reads from each output processor's metering ring buffer and runs detectors.
*/
class OutputMeteringThread : public juce::Thread
{
public:
    OutputMeteringThread() : juce::Thread("OutputMetering") {}
    ~OutputMeteringThread() override { stopThread(1000); }

    void prepare(const std::vector<std::unique_ptr<OutputBufferProcessor>>& processors,
                 const std::vector<std::unique_ptr<OutputLevelDetector>>& detectors,
                 int blockSize)
    {
        outputProcessors.clear();
        outputDetectors.clear();

        for (auto& proc : processors)
            outputProcessors.push_back(proc.get());
        for (auto& det : detectors)
            outputDetectors.push_back(det.get());

        processingBlockSize = blockSize;
        meterBlock.setSize(1, blockSize);
    }

private:
    void run() override
    {
        while (!threadShouldExit())
        {
            bool didWork = false;
            int numOutputs = juce::jmin((int)outputProcessors.size(), (int)outputDetectors.size());

            for (int ch = 0; ch < numOutputs; ++ch)
            {
                if (outputProcessors[ch]->getMeteringAvailable() >= processingBlockSize)
                {
                    int read = outputProcessors[ch]->readMeteringOutput(
                        meterBlock.getWritePointer(0), processingBlockSize);

                    if (read > 0 && outputDetectors[ch] != nullptr)
                    {
                        auto* data = meterBlock.getReadPointer(0);
                        for (int i = 0; i < read; ++i)
                            outputDetectors[ch]->processSample(data[i]);
                    }
                    didWork = true;
                }
            }

            if (!didWork)
                wait(10);  // Sleep longer when idle
        }
    }

    std::vector<OutputBufferProcessor*> outputProcessors;
    std::vector<OutputLevelDetector*> outputDetectors;
    int processingBlockSize = 64;
    juce::AudioBuffer<float> meterBlock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputMeteringThread)
};

//==============================================================================
/**
    Output-based WFS algorithm using write-time delays.

    Strategy:
    - One processing thread per output channel
    - Each thread receives all inputs and accumulates delayed contributions
    - Delay calculation happens at write time (when input arrives)

    This class manages a collection of OutputBufferProcessor instances.
*/
class OutputBufferAlgorithm
{
public:
    OutputBufferAlgorithm() = default;
    ~OutputBufferAlgorithm()
    {
        clear();
    }

    void prepare(int numInputs,
                int numOutputs,
                double sampleRate,
                int blockSize,
                const float* delayTimesPtr,
                const float* levelsPtr,
                bool processingEnabled,
                const float* hfAttenuationPtr = nullptr,
                const float* frDelayTimesPtr = nullptr,
                const float* frLevelsPtr = nullptr,
                const float* frHFAttenuationPtr = nullptr)
    {
        storedNumInputs = numInputs;
        storedNumOutputs = numOutputs;
        cachedSampleRate = sampleRate;

        // Create Live Source level detectors (one per input channel)
        lsDetectors.clear();
        for (int i = 0; i < numInputs; ++i)
        {
            auto detector = std::make_unique<LiveSourceLevelDetector>();
            detector->prepare(sampleRate);
            lsDetectors.push_back(std::move(detector));
        }

        // Create output level detectors (one per output channel)
        outputLevelDetectors.clear();
        for (int i = 0; i < numOutputs; ++i)
        {
            auto detector = std::make_unique<OutputLevelDetector>();
            detector->prepare(sampleRate);
            outputLevelDetectors.push_back(std::move(detector));
        }

        // Create shared input ring buffers (one per input channel, read by all output threads)
        sharedInputBuffers.clear();
        for (int i = 0; i < numInputs; ++i)
        {
            auto buf = std::make_unique<SharedInputRingBuffer>();
            buf->setSize(blockSize * 4);
            sharedInputBuffers.push_back(std::move(buf));
        }

        // Create output-based processors (one thread per output channel)
        for (int i = 0; i < numOutputs; ++i)
        {
            auto processor = std::make_unique<OutputBufferProcessor>(i, numInputs, numOutputs,
                                                                      delayTimesPtr,
                                                                      levelsPtr,
                                                                      hfAttenuationPtr,
                                                                      frDelayTimesPtr,
                                                                      frLevelsPtr,
                                                                      frHFAttenuationPtr);
            processor->prepare(sampleRate, blockSize);
            processor->setSharedInputBuffers(sharedInputBuffers);
            outputProcessors.push_back(std::move(processor));
        }

        // Start threads AFTER all processors are created and prepared
        for (auto& processor : outputProcessors)
        {
            processor->setProcessingEnabled(processingEnabled);
            processor->startThread(juce::Thread::Priority::high);
        }

        // Start input analysis thread (reads shared buffers, runs level detection)
        inputAnalysisThread = std::make_unique<InputAnalysisThread>();
        inputAnalysisThread->prepare(sharedInputBuffers, lsDetectors, blockSize);
        inputAnalysisThread->startThread(juce::Thread::Priority::normal);

        // Start output metering thread
        outputMeteringThread = std::make_unique<OutputMeteringThread>();
        outputMeteringThread->prepare(outputProcessors, outputLevelDetectors, blockSize);
        outputMeteringThread->startThread(juce::Thread::Priority::normal);
    }

    void reprepare(double sampleRate, int blockSize, bool processingEnabled)
    {
        cachedSampleRate = sampleRate;

        // Stop threads first
        if (outputMeteringThread)
            outputMeteringThread->stopThread(1000);
        if (inputAnalysisThread)
            inputAnalysisThread->stopThread(1000);
        for (auto& processor : outputProcessors)
            processor->stopThread(1000);

        // Resize shared input buffers
        for (auto& buf : sharedInputBuffers)
        {
            buf->reset();
            buf->setSize(blockSize * 4);
        }

        // Re-prepare and restart output processors
        for (auto& processor : outputProcessors)
        {
            processor->prepare(sampleRate, blockSize);
            processor->setSharedInputBuffers(sharedInputBuffers);
            processor->setProcessingEnabled(processingEnabled);
            processor->startThread(juce::Thread::Priority::high);
        }

        // Re-prepare input level detectors
        for (auto& detector : lsDetectors)
            detector->prepare(sampleRate);

        // Re-prepare output level detectors
        for (auto& detector : outputLevelDetectors)
            detector->prepare(sampleRate);

        // Restart input analysis thread
        if (inputAnalysisThread)
        {
            inputAnalysisThread->prepare(sharedInputBuffers, lsDetectors, blockSize);
            inputAnalysisThread->startThread(juce::Thread::Priority::normal);
        }

        // Restart output metering thread
        if (outputMeteringThread)
        {
            outputMeteringThread->prepare(outputProcessors, outputLevelDetectors, blockSize);
            outputMeteringThread->startThread(juce::Thread::Priority::normal);
        }
    }

    void processBlock(const juce::AudioSourceChannelInfo& bufferToFill,
                     int numInputChannels,
                     int numOutputChannels)
    {
        auto totalChannels = bufferToFill.buffer->getNumChannels();
        auto numSamples = bufferToFill.numSamples;

        if (outputProcessors.empty())
        {
            bufferToFill.clearActiveBufferRegion();
            return;
        }

        // Determine actual available channels
        auto numChannels = juce::jmin(numInputChannels, totalChannels);

        if (numChannels < numInputChannels)
            DBG("WARNING: OutputBufferAlgorithm processing " + juce::String(numChannels) +
                "/" + juce::String(numInputChannels) + " inputs (bufferCh=" +
                juce::String(totalChannels) + ", lsDetectors=" +
                juce::String((int)lsDetectors.size()) + ")");

        // Step 1: Write input data once to shared buffers (N writes, not N×M)
        for (int inChannel = 0; inChannel < numChannels && inChannel < (int)sharedInputBuffers.size(); ++inChannel)
        {
            auto* inputData = bufferToFill.buffer->getReadPointer(inChannel, bufferToFill.startSample);
            sharedInputBuffers[inChannel]->write(inputData, numSamples);
        }

        // Notify all output processor threads and the analysis thread
        for (auto& processor : outputProcessors)
            processor->notifyInputAvailable(numSamples);
        if (inputAnalysisThread)
            inputAnalysisThread->notifyInputAvailable(numSamples);

        // Clear output buffer
        bufferToFill.clearActiveBufferRegion();

        // Pull processed outputs from each output processor
        int numOutputs = juce::jmin(numOutputChannels, totalChannels, (int)outputProcessors.size());
        for (int outChannel = 0; outChannel < numOutputs; ++outChannel)
        {
            if (outputProcessors[outChannel] == nullptr)
                continue;

            auto* outputData = bufferToFill.buffer->getWritePointer(outChannel, bufferToFill.startSample);
            outputProcessors[outChannel]->pullOutput(outputData, numSamples);
        }
    }

    void setProcessingEnabled(bool enabled)
    {
        for (auto& processor : outputProcessors)
            processor->setProcessingEnabled(enabled);
    }

    void releaseResources()
    {
        if (outputMeteringThread)
            outputMeteringThread->stopThread(1000);
        if (inputAnalysisThread)
            inputAnalysisThread->stopThread(1000);

        for (auto& processor : outputProcessors)
        {
            processor->stopThread(1000);
            processor->reset();
        }
    }

    void clear()
    {
        outputMeteringThread.reset();
        inputAnalysisThread.reset();
        outputProcessors.clear();
        sharedInputBuffers.clear();
        lsDetectors.clear();
        outputLevelDetectors.clear();
    }

    bool isEmpty() const
    {
        return outputProcessors.empty();
    }

    size_t getNumProcessors() const
    {
        return outputProcessors.size();
    }

    float getCpuUsagePercent(size_t index) const
    {
        if (index < outputProcessors.size())
            return outputProcessors[index]->getCpuUsagePercent();
        return 0.0f;
    }

    float getProcessingTimeMicroseconds(size_t index) const
    {
        if (index < outputProcessors.size())
            return outputProcessors[index]->getProcessingTimeMicroseconds();
        return 0.0f;
    }

    // === Live Source Tamer accessors ===

    float getPeakGainReduction(size_t inputIndex) const
    {
        if (inputIndex < lsDetectors.size())
            return lsDetectors[inputIndex]->getPeakGainReduction();
        return 1.0f;
    }

    float getSlowGainReduction(size_t inputIndex) const
    {
        if (inputIndex < lsDetectors.size())
            return lsDetectors[inputIndex]->getSlowGainReduction();
        return 1.0f;
    }

    // Get short peak level in dB (5ms hold for AutomOtion triggering)
    float getShortPeakLevelDb(size_t inputIndex) const
    {
        if (inputIndex < lsDetectors.size())
            return lsDetectors[inputIndex]->getShortPeakLevelDb();
        return -200.0f;
    }

    // Get RMS level in dB (200ms window)
    float getRmsLevelDb(size_t inputIndex) const
    {
        if (inputIndex < lsDetectors.size())
            return lsDetectors[inputIndex]->getRmsLevelDb();
        return -200.0f;
    }

    void setLSParameters(size_t inputIndex, float peakThreshDb, float peakRatio,
                         float slowThreshDb, float slowRatio)
    {
        if (inputIndex < lsDetectors.size())
            lsDetectors[inputIndex]->setParameters(peakThreshDb, peakRatio,
                                                    slowThreshDb, slowRatio);
    }

    // === Floor Reflection parameter setters ===
    // Note: Each output processor handles all inputs, so FR params need to be
    // forwarded to all processors for the specific input index

    void setFRFilterParams(size_t inputIndex,
                           bool lowCutActive, float lowCutFreq,
                           bool highShelfActive, float highShelfFreq,
                           float highShelfGain, float highShelfSlope)
    {
        // Forward to all output processors (each handles all inputs)
        for (auto& processor : outputProcessors)
        {
            processor->setFRFilterParams(static_cast<int>(inputIndex),
                                          lowCutActive, lowCutFreq,
                                          highShelfActive, highShelfFreq,
                                          highShelfGain, highShelfSlope);
        }
    }

    void setFRDiffusion(size_t inputIndex, float diffusionPercent)
    {
        // Forward to all output processors (each handles all inputs)
        for (auto& processor : outputProcessors)
        {
            processor->setFRDiffusion(static_cast<int>(inputIndex), diffusionPercent);
        }
    }

    // === Output Level Metering ===

    void setOutputMeteringEnabled(bool enabled)
    {
        outputMeteringEnabled.store(enabled, std::memory_order_relaxed);
    }

    bool isOutputMeteringEnabled() const
    {
        return outputMeteringEnabled.load(std::memory_order_relaxed);
    }

    float getOutputPeakLevelDb(size_t outputIndex) const
    {
        if (outputIndex < outputLevelDetectors.size())
            return outputLevelDetectors[outputIndex]->getPeakLevelDb();
        return -200.0f;
    }

    float getOutputRmsLevelDb(size_t outputIndex) const
    {
        if (outputIndex < outputLevelDetectors.size())
            return outputLevelDetectors[outputIndex]->getRmsLevelDb();
        return -200.0f;
    }

    // Get input peak level in dB (for metering - delegates to lsDetectors)
    float getInputPeakLevelDb(size_t inputIndex) const
    {
        if (inputIndex < lsDetectors.size())
            return lsDetectors[inputIndex]->getPeakLevelDb();
        return -200.0f;
    }

    // Get input RMS level in dB (for metering - delegates to lsDetectors)
    float getInputRmsLevelDb(size_t inputIndex) const
    {
        if (inputIndex < lsDetectors.size())
            return lsDetectors[inputIndex]->getRmsLevelDb();
        return -200.0f;
    }

    size_t getNumOutputDetectors() const
    {
        return outputLevelDetectors.size();
    }

private:
    std::vector<std::unique_ptr<OutputBufferProcessor>> outputProcessors;

    // Shared input ring buffers (one per input, read by all output threads + analysis thread)
    std::vector<std::unique_ptr<SharedInputRingBuffer>> sharedInputBuffers;

    // Dedicated thread for input-side analysis (level detection, Live Source Tamer)
    std::unique_ptr<InputAnalysisThread> inputAnalysisThread;

    // Dedicated thread for output level metering
    std::unique_ptr<OutputMeteringThread> outputMeteringThread;

    // Live Source level detectors (one per input channel)
    std::vector<std::unique_ptr<LiveSourceLevelDetector>> lsDetectors;

    // Output level detectors (one per output channel)
    std::vector<std::unique_ptr<OutputLevelDetector>> outputLevelDetectors;
    std::atomic<bool> outputMeteringEnabled{false};

    int storedNumInputs = 0;
    int storedNumOutputs = 0;
    double cachedSampleRate = 48000.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputBufferAlgorithm)
};
