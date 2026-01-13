#pragma once

#include <JuceHeader.h>
#include "InputBufferAlgorithm.h"
#include "OutputBufferAlgorithm.h"
#include <vector>
#include <atomic>
#include <cmath>

/**
 * LevelMeteringManager
 *
 * Central coordinator for audio level metering data.
 * Manages enable/disable state and provides thread-safe level access for UI.
 *
 * Features:
 * - Enable/disable metering from map overlay or meter window
 * - Collect input/output levels from algorithms
 * - Thread performance data access
 * - Visual solo support (per-input contribution tracking)
 */
class LevelMeteringManager
{
public:
    // Processing algorithm enum (matches MainComponent)
    enum class ProcessingAlgorithm
    {
        InputBuffer,
        OutputBuffer
    };

    struct LevelData
    {
        float peakDb = -200.0f;
        float rmsDb = -200.0f;
    };

    struct ThreadPerformance
    {
        float cpuPercent = 0.0f;
        float microsecondsPerBlock = 0.0f;
    };

    LevelMeteringManager(int numInputs, int numOutputs)
        : numInputChannels(numInputs)
        , numOutputChannels(numOutputs)
    {
        inputLevels.resize(numInputs);
        outputLevels.resize(numOutputs);
        threadPerformance.resize(juce::jmax(numInputs, numOutputs));
    }

    // === Enable/Disable Control ===

    void setMapOverlayEnabled(bool enabled)
    {
        mapOverlayEnabled.store(enabled, std::memory_order_relaxed);
        updateAlgorithmMeteringFlags();
    }

    void setMeterWindowEnabled(bool enabled)
    {
        meterWindowEnabled.store(enabled, std::memory_order_relaxed);
        updateAlgorithmMeteringFlags();
    }

    bool isMapOverlayEnabled() const
    {
        return mapOverlayEnabled.load(std::memory_order_relaxed);
    }

    bool isMeterWindowEnabled() const
    {
        return meterWindowEnabled.load(std::memory_order_relaxed);
    }

    bool isMeteringActive() const
    {
        return mapOverlayEnabled.load(std::memory_order_relaxed) ||
               meterWindowEnabled.load(std::memory_order_relaxed);
    }

    // === Algorithm References ===
    // Call these after algorithms are prepared

    void setAlgorithms(InputBufferAlgorithm* inputAlg, OutputBufferAlgorithm* outputAlg)
    {
        inputAlgorithm = inputAlg;
        outputAlgorithm = outputAlg;
        updateAlgorithmMeteringFlags();
    }

    void setCurrentAlgorithm(ProcessingAlgorithm alg)
    {
        currentAlgorithm = alg;
    }

    ProcessingAlgorithm getCurrentAlgorithm() const
    {
        return currentAlgorithm;
    }

    // === Level Updates ===
    // Call this from MainComponent::timerCallback at 20Hz

    void updateLevels()
    {
        if (!isMeteringActive())
            return;

        if (currentAlgorithm == ProcessingAlgorithm::InputBuffer && inputAlgorithm != nullptr)
        {
            // Get input levels from InputBufferAlgorithm
            for (int i = 0; i < numInputChannels && i < (int)inputLevels.size(); ++i)
            {
                inputLevels[i].peakDb = inputAlgorithm->getInputPeakLevelDb(i);
                inputLevels[i].rmsDb = inputAlgorithm->getInputRmsLevelDb(i);
            }

            // Get output levels from InputBufferAlgorithm
            for (int i = 0; i < numOutputChannels && i < (int)outputLevels.size(); ++i)
            {
                outputLevels[i].peakDb = inputAlgorithm->getOutputPeakLevelDb(i);
                outputLevels[i].rmsDb = inputAlgorithm->getOutputRmsLevelDb(i);
            }

            // Get thread performance (one per input in InputBuffer mode)
            for (int i = 0; i < numInputChannels && i < (int)threadPerformance.size(); ++i)
            {
                threadPerformance[i].cpuPercent = inputAlgorithm->getCpuUsagePercent(i);
                threadPerformance[i].microsecondsPerBlock = inputAlgorithm->getProcessingTimeMicroseconds(i);
            }
        }
        else if (currentAlgorithm == ProcessingAlgorithm::OutputBuffer && outputAlgorithm != nullptr)
        {
            // Get input levels from OutputBufferAlgorithm
            for (int i = 0; i < numInputChannels && i < (int)inputLevels.size(); ++i)
            {
                inputLevels[i].peakDb = outputAlgorithm->getInputPeakLevelDb(i);
                inputLevels[i].rmsDb = outputAlgorithm->getInputRmsLevelDb(i);
            }

            // Get output levels from OutputBufferAlgorithm
            for (int i = 0; i < numOutputChannels && i < (int)outputLevels.size(); ++i)
            {
                outputLevels[i].peakDb = outputAlgorithm->getOutputPeakLevelDb(i);
                outputLevels[i].rmsDb = outputAlgorithm->getOutputRmsLevelDb(i);
            }

            // Get thread performance (one per output in OutputBuffer mode)
            for (int i = 0; i < numOutputChannels && i < (int)threadPerformance.size(); ++i)
            {
                threadPerformance[i].cpuPercent = outputAlgorithm->getCpuUsagePercent(i);
                threadPerformance[i].microsecondsPerBlock = outputAlgorithm->getProcessingTimeMicroseconds(i);
            }
        }
    }

    // === Level Accessors ===

    LevelData getInputLevel(int index) const
    {
        if (index >= 0 && index < (int)inputLevels.size())
            return inputLevels[index];
        return LevelData{};
    }

    LevelData getOutputLevel(int index) const
    {
        if (index >= 0 && index < (int)outputLevels.size())
            return outputLevels[index];
        return LevelData{};
    }

    int getNumInputChannels() const { return numInputChannels; }
    int getNumOutputChannels() const { return numOutputChannels; }

    // === Thread Performance Accessors ===

    ThreadPerformance getThreadPerformance(int index) const
    {
        if (index >= 0 && index < (int)threadPerformance.size())
            return threadPerformance[index];
        return ThreadPerformance{};
    }

    int getNumThreads() const
    {
        if (currentAlgorithm == ProcessingAlgorithm::InputBuffer)
            return numInputChannels;
        else
            return numOutputChannels;
    }

    // === Visual Solo ===

    void setVisualSoloInput(int inputIndex)
    {
        visualSoloInput.store(inputIndex, std::memory_order_relaxed);
    }

    int getVisualSoloInput() const
    {
        return visualSoloInput.load(std::memory_order_relaxed);
    }

    /**
     * Get estimated contribution of an input to an output.
     * This is an approximation based on input level and routing level.
     *
     * @param inputIndex Input channel index
     * @param outputIndex Output channel index
     * @param routingLevel Linear routing level from WFSCalculationEngine (0-1)
     * @return Estimated contribution in dB
     */
    float getInputContributionToOutput(int inputIndex, int outputIndex, float routingLevel) const
    {
        juce::ignoreUnused(outputIndex);  // For future use with per-output routing analysis

        if (inputIndex < 0 || inputIndex >= (int)inputLevels.size())
            return -200.0f;

        float inputPeakDb = inputLevels[inputIndex].peakDb;

        // Convert routing level to dB and add to input level
        float routingLevelDb = (routingLevel > 1e-10f)
            ? 20.0f * std::log10(routingLevel)
            : -200.0f;

        return inputPeakDb + routingLevelDb;
    }

    // === Channel Count Updates ===

    void setChannelCounts(int inputs, int outputs)
    {
        numInputChannels = inputs;
        numOutputChannels = outputs;
        inputLevels.resize(inputs);
        outputLevels.resize(outputs);
        threadPerformance.resize(juce::jmax(inputs, outputs));
    }

private:
    void updateAlgorithmMeteringFlags()
    {
        bool active = isMeteringActive();

        if (inputAlgorithm != nullptr)
            inputAlgorithm->setOutputMeteringEnabled(active);

        if (outputAlgorithm != nullptr)
            outputAlgorithm->setOutputMeteringEnabled(active);
    }

    // Algorithm references (not owned)
    InputBufferAlgorithm* inputAlgorithm = nullptr;
    OutputBufferAlgorithm* outputAlgorithm = nullptr;
    ProcessingAlgorithm currentAlgorithm = ProcessingAlgorithm::InputBuffer;

    // Channel counts
    int numInputChannels = 0;
    int numOutputChannels = 0;

    // Enable flags
    std::atomic<bool> mapOverlayEnabled{false};
    std::atomic<bool> meterWindowEnabled{false};

    // Cached level data (updated at 20Hz from timer thread)
    std::vector<LevelData> inputLevels;
    std::vector<LevelData> outputLevels;
    std::vector<ThreadPerformance> threadPerformance;

    // Visual solo
    std::atomic<int> visualSoloInput{-1};
};
