#pragma once

#include <JuceHeader.h>
#include "../../spatcore/reverb/ReverbEngine.h"
#include "../WFSLogger.h"
#include <mutex>

/**
 * ReverbIRLoader - reads the convolution reverb's IR file off the message thread.
 *
 * ReverbEngine::loadIRFile reads the whole file before it hands the buffer to
 * the engine thread. It used to run inside the 200 Hz timer, so a long IR
 * stalled the UI for as long as the read took, and a file on an unreachable
 * share for the SMB timeout (re-audit 2026-09-29, B4). The read now runs here,
 * one request at a time, the latest one winning.
 *
 * Before any sample is read the header is checked: a reader must open, the
 * sample rate must be positive and the length must fit maxIRSeconds. A WAV
 * header can claim any length, and the engine converted it to an int and
 * allocated that many floats.
 *
 * Lifetime: holds a reference to the ReverbEngine, which MainComponent creates
 * once and destroys after this (declared after it; stop() runs first in
 * ~MainComponent).
 */
class ReverbIRLoader final : private juce::Thread
{
public:
    /** 60 s covers every legal trim (30 s) plus length (30 s) setting. */
    static constexpr double maxIRSeconds = 60.0;

    /** And never more than this many samples, whatever the rate claims: about
        46 MB of float, the most a legal IR needs at 192 kHz. */
    static constexpr juce::int64 maxIRSamples = static_cast<juce::int64> (maxIRSeconds * 192000.0);

    /** An empty string when `file` may be read as an IR, else why not. Opens
        the header only. */
    static juce::String whyUnusable (const juce::File& file)
    {
        if (! file.existsAsFile())
            return "the file does not exist";

        juce::AudioFormatManager formats;
        formats.registerBasicFormats();
        const std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));

        if (reader == nullptr)
            return "it is not an audio file this app reads";

        if (! (reader->sampleRate > 0.0) || reader->lengthInSamples <= 0)
            return "its header has no sample rate or no samples";

        if (reader->lengthInSamples > maxIRSamples
            || static_cast<double> (reader->lengthInSamples) / reader->sampleRate > maxIRSeconds)
            return "it is longer than " + juce::String (maxIRSeconds, 0) + " s";

        return {};
    }

    explicit ReverbIRLoader (ReverbEngine& engineToFeed)
        : juce::Thread ("WFS-DIY IR loader"), engine (engineToFeed)
    {
        startThread();
    }

    ~ReverbIRLoader() override { stop(); }

    /** Joins the worker; later requests are dropped. Message thread. */
    void stop()
    {
        signalThreadShouldExit();
        notify();
        stopThread (10000);
    }

    /** Read `file` as the IR, replacing any request not started yet. */
    void request (const juce::File& file)
    {
        {
            const std::lock_guard<std::mutex> lock (requestLock);
            pending = file;
            hasPending = true;
        }
        notify();
    }

private:
    void run() override
    {
        while (! threadShouldExit())
        {
            juce::File file;
            {
                const std::lock_guard<std::mutex> lock (requestLock);
                if (hasPending)
                {
                    file = pending;
                    hasPending = false;
                }
            }

            if (file == juce::File())
            {
                wait (-1);
                continue;
            }

            if (const auto why = whyUnusable (file); why.isNotEmpty())
            {
                WFSLogger::getInstance().logWarning ("Reverb IR " + file.getFileName().quoted()
                                                     + " not loaded: " + why);
                continue;
            }

            engine.loadIRFile (file);
        }
    }

    ReverbEngine& engine;
    std::mutex requestLock;
    juce::File pending;
    bool hasPending = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverbIRLoader)
};
