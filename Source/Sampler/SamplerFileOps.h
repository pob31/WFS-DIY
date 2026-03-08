#pragma once

#include <JuceHeader.h>

/**
 * Utility for loading audio samples and managing the project samples/ folder.
 * Supports WAV, AIFF, FLAC, OGG formats via JUCE AudioFormatManager.
 */
class SamplerFileOps
{
public:
    SamplerFileOps()
    {
        formatManager.registerBasicFormats();
    }

    /**
     * Load an audio file into a shared AudioBuffer.
     * Returns nullptr on failure.
     */
    std::shared_ptr<juce::AudioBuffer<float>> loadAudioFile (const juce::File& file,
                                                              double& outSampleRate,
                                                              int& outNumSamples)
    {
        outSampleRate = 0.0;
        outNumSamples = 0;

        if (! file.existsAsFile())
            return nullptr;

        std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
        if (reader == nullptr)
            return nullptr;

        auto numChannels = static_cast<int> (reader->numChannels);
        auto numSamples  = static_cast<int> (reader->lengthInSamples);

        if (numSamples <= 0)
            return nullptr;

        // Always store as mono (mix down if stereo+)
        auto buffer = std::make_shared<juce::AudioBuffer<float>> (1, numSamples);

        if (numChannels == 1)
        {
            reader->read (buffer.get(), 0, numSamples, 0, true, false);
        }
        else
        {
            // Read all channels into temp buffer, then mix to mono
            juce::AudioBuffer<float> temp (numChannels, numSamples);
            reader->read (&temp, 0, numSamples, 0, true, true);

            auto* mono = buffer->getWritePointer (0);
            const float scale = 1.0f / static_cast<float> (numChannels);

            for (int s = 0; s < numSamples; ++s)
            {
                float sum = 0.0f;
                for (int ch = 0; ch < numChannels; ++ch)
                    sum += temp.getSample (ch, s);
                mono[s] = sum * scale;
            }
        }

        outSampleRate  = reader->sampleRate;
        outNumSamples  = numSamples;
        return buffer;
    }

    /**
     * Copy an external audio file into the project samples/ folder.
     * Returns the relative path (filename only) within the samples folder,
     * or empty string on failure.
     */
    juce::String importSampleToProject (const juce::File& sourceFile,
                                         const juce::File& samplesFolder)
    {
        if (! sourceFile.existsAsFile() || ! samplesFolder.isDirectory())
            return {};

        juce::File destFile = samplesFolder.getChildFile (sourceFile.getFileName());

        // Handle name collision: append _1, _2, etc.
        if (destFile.existsAsFile())
        {
            auto baseName = sourceFile.getFileNameWithoutExtension();
            auto extension = sourceFile.getFileExtension();
            int counter = 1;
            do
            {
                destFile = samplesFolder.getChildFile (baseName + "_" + juce::String (counter) + extension);
                counter++;
            } while (destFile.existsAsFile() && counter < 1000);
        }

        if (! sourceFile.copyFileTo (destFile))
            return {};

        return destFile.getFileName();
    }

    /**
     * Load a sample from the project samples/ folder by relative filename.
     */
    std::shared_ptr<juce::AudioBuffer<float>> loadFromProject (const juce::File& samplesFolder,
                                                                const juce::String& relativeFilePath,
                                                                double& outSampleRate,
                                                                int& outNumSamples)
    {
        if (relativeFilePath.isEmpty() || ! samplesFolder.isDirectory())
            return nullptr;

        juce::File file = samplesFolder.getChildFile (relativeFilePath);
        return loadAudioFile (file, outSampleRate, outNumSamples);
    }

    /** Get supported file extensions as a wildcard pattern for file choosers */
    juce::String getWildcardFilter() const
    {
        return formatManager.getWildcardForAllFormats();
    }

private:
    juce::AudioFormatManager formatManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplerFileOps)
};
