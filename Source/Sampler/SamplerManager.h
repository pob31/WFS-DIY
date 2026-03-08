#pragma once

#include <JuceHeader.h>
#include "SamplerEngine.h"
#include "SamplerFileOps.h"
#include "SamplerData.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/WFSParameterDefaults.h"

/**
 * Owns per-channel SamplerEngine instances.
 * Manages sample loading, engine lifecycle, and position override output.
 *
 * Thread safety:
 *   - Audio thread calls processChannel()
 *   - Message thread calls load/configure methods
 *   - BLOCKS/MIDI thread calls pushTouchEvent()
 */
class SamplerManager
{
public:
    SamplerManager() = default;

    //==========================================================================
    /** Prepare all engines for audio processing */
    void prepare (double sampleRate, int maxBlockSize, int numChannels)
    {
        currentSampleRate = sampleRate;
        currentBlockSize = maxBlockSize;

        engines.resize (static_cast<size_t> (numChannels));
        channelActive.resize (static_cast<size_t> (numChannels), false);

        for (auto& engine : engines)
        {
            if (engine == nullptr)
                engine = std::make_unique<SamplerEngine>();
            engine->prepare (sampleRate, maxBlockSize);
        }
    }

    /** Reset all engines */
    void reset()
    {
        for (auto& engine : engines)
            if (engine != nullptr)
                engine->reset();
    }

    //==========================================================================
    /**
     * Process a single channel's audio. Overwrites the buffer data for this channel.
     * Called from the audio thread ONLY.
     *
     * @param channelIndex  WFS input channel index (0-based)
     * @param buffer        The main audio buffer
     * @param startSample   Start offset in buffer
     * @param numSamples    Number of samples to process
     */
    void processChannel (int channelIndex, juce::AudioBuffer<float>& buffer,
                         int startSample, int numSamples)
    {
        auto idx = static_cast<size_t> (channelIndex);
        if (idx >= engines.size() || engines[idx] == nullptr)
            return;

        if (! channelActive[idx])
            return;

        float* data = buffer.getWritePointer (channelIndex, startSample);
        engines[idx]->processBlock (data, numSamples);
    }

    //==========================================================================
    /** Set whether sampler is active for a given channel */
    void setChannelActive (int channelIndex, bool active)
    {
        auto idx = static_cast<size_t> (channelIndex);
        if (idx < channelActive.size())
            channelActive[idx] = active;
    }

    /** Check if sampler is active for a given channel */
    bool isChannelActive (int channelIndex) const
    {
        auto idx = static_cast<size_t> (channelIndex);
        return idx < channelActive.size() && channelActive[idx];
    }

    //==========================================================================
    /** Load cell data for a channel from ValueTree */
    void loadChannelCells (int channelIndex, const juce::ValueTree& samplerNode)
    {
        auto idx = static_cast<size_t> (channelIndex);
        if (idx >= engines.size() || engines[idx] == nullptr)
            return;

        std::vector<SamplerData::SampleCell> cells;
        cells.resize (WFSParameterDefaults::samplerGridCells);

        for (int i = 0; i < samplerNode.getNumChildren(); ++i)
        {
            auto child = samplerNode.getChild (i);
            if (child.hasType (WFSParameterIDs::SamplerCell))
            {
                int id = child.getProperty ("id", -1);
                if (id >= 0 && id < WFSParameterDefaults::samplerGridCells)
                    cells[static_cast<size_t> (id)].loadFromValueTree (child);
            }
        }

        engines[idx]->loadCells (cells);
    }

    /** Load a set configuration for a channel */
    void loadChannelSet (int channelIndex, const SamplerData::SamplerSet& set)
    {
        auto idx = static_cast<size_t> (channelIndex);
        if (idx >= engines.size() || engines[idx] == nullptr)
            return;

        engines[idx]->loadSet (set);
    }

    //==========================================================================
    /** Load audio data for a specific cell on a specific channel */
    void loadCellAudio (int channelIndex, int cellIndex,
                        const juce::File& samplesFolder, const juce::String& relativeFilePath)
    {
        auto idx = static_cast<size_t> (channelIndex);
        if (idx >= engines.size() || engines[idx] == nullptr)
            return;

        if (relativeFilePath.isEmpty())
            return;

        double sr = 0.0;
        int numSamples = 0;
        auto buffer = fileOps.loadFromProject (samplesFolder, relativeFilePath, sr, numSamples);

        if (buffer != nullptr)
        {
            // Resample if needed (simple case: just store with original rate info)
            // The engine plays at whatever rate the sample was recorded.
            // TODO: Add resampling if sample rate != engine sample rate

            // Update the cell's audio data in the engine's cell array
            juce::SpinLock::ScopedLockType lock (cellAudioLock);
            pendingAudioLoads.push_back ({ channelIndex, cellIndex, buffer, sr, numSamples });
        }
    }

    /** Apply any pending audio loads (call from message thread periodically) */
    void applyPendingAudioLoads()
    {
        juce::SpinLock::ScopedLockType lock (cellAudioLock);

        for (auto& load : pendingAudioLoads)
        {
            auto idx = static_cast<size_t> (load.channelIndex);
            if (idx >= engines.size() || engines[idx] == nullptr)
                continue;

            // The engine needs updated cell data with the new audio buffer
            // This is a simplified approach — in practice, loadChannelCells should be called
            // after updating the cell's audioBuffer in the data layer
        }

        pendingAudioLoads.clear();
    }

    //==========================================================================
    /** Push a touch event for a channel (called from BLOCKS/MIDI thread) */
    bool pushTouchEvent (int channelIndex, const SamplerEngine::TouchEvent& event)
    {
        auto idx = static_cast<size_t> (channelIndex);
        if (idx >= engines.size() || engines[idx] == nullptr)
            return false;

        return engines[idx]->pushEvent (event);
    }

    //==========================================================================
    /** Get position override for a channel (called from 50Hz message-thread timer) */
    bool getPositionOverride (int channelIndex, float& outX, float& outY, float& outZ) const
    {
        auto idx = static_cast<size_t> (channelIndex);
        if (idx >= engines.size() || engines[idx] == nullptr)
            return false;

        if (! engines[idx]->hasPositionOverride())
            return false;

        outX = engines[idx]->getPositionX();
        outY = engines[idx]->getPositionY();
        outZ = engines[idx]->getPositionZ();
        return true;
    }

    /** Check if any channel has an active sampler */
    bool hasAnyActiveChannel() const
    {
        for (size_t i = 0; i < channelActive.size(); ++i)
            if (channelActive[i])
                return true;
        return false;
    }

    /** Get number of configured channels */
    int getNumChannels() const { return static_cast<int> (engines.size()); }

    /** Access file ops for import dialogs */
    SamplerFileOps& getFileOps() { return fileOps; }

    /** Get engine for a channel (for direct access, e.g., isPlaying check) */
    SamplerEngine* getEngine (int channelIndex)
    {
        auto idx = static_cast<size_t> (channelIndex);
        if (idx >= engines.size())
            return nullptr;
        return engines[idx].get();
    }

private:
    struct PendingAudioLoad
    {
        int channelIndex;
        int cellIndex;
        std::shared_ptr<juce::AudioBuffer<float>> buffer;
        double sampleRate;
        int numSamples;
    };

    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    std::vector<std::unique_ptr<SamplerEngine>> engines;
    std::vector<bool> channelActive;

    SamplerFileOps fileOps;

    juce::SpinLock cellAudioLock;
    std::vector<PendingAudioLoad> pendingAudioLoads;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplerManager)
};
