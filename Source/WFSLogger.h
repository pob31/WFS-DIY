#pragma once

#include <JuceHeader.h>

/**
 * WFSLogger - Session logging system for diagnostics and error reporting.
 *
 * Captures session events and exceptions to a per-session log file.
 * Thread-safe for message thread, OSC threads, and timer threads via CriticalSection.
 * Audio thread uses a lock-free SPSC ring buffer with timer-based drain.
 *
 * Usage:
 *   WFSLogger::getInstance().initialise();
 *   WFSLogger::getInstance().logInfo("Something happened");
 *   WFSLogger::getInstance().logFromAudioThread("xrun detected");
 *   WFSLogger::shutdown();
 */
class WFSLogger : private juce::Timer
{
public:
    enum class Level { Info, Warning, Error };

    //==========================================================================
    // Singleton
    //==========================================================================

    static WFSLogger& getInstance()
    {
        if (instance == nullptr)
            instance = new WFSLogger();
        return *instance;
    }

    static void shutdown()
    {
        if (instance != nullptr)
        {
            delete instance;
            instance = nullptr;
        }
    }

    //==========================================================================
    // Initialisation
    //==========================================================================

    /** Call once at application startup (before MainWindow creation). */
    void initialise();

    //==========================================================================
    // Logging API (thread-safe, NOT for audio thread)
    //==========================================================================

    void log (Level level, const juce::String& message);
    void logInfo (const juce::String& message)    { log (Level::Info, message); }
    void logWarning (const juce::String& message)  { log (Level::Warning, message); }
    void logError (const juce::String& message)    { log (Level::Error, message); }

    //==========================================================================
    // Audio thread logging (lock-free, deferred)
    //==========================================================================

    /** Enqueue a message from the audio callback. Never blocks. */
    void logFromAudioThread (const juce::String& message);

    //==========================================================================
    // State snapshot
    //==========================================================================

    /** Log a snapshot of current application state (device, channels, algorithm, CPU). */
    void logStateSnapshot (const juce::String& context);

    //==========================================================================
    // Export
    //==========================================================================

    /** Copy recent logs + config files to a destination directory for support.
        Returns true on success. */
    bool exportLogs (const juce::File& destinationDir);

    //==========================================================================
    // Accessors
    //==========================================================================

    /** Get the log file directory (~/.WFS-DIY/logs/). */
    juce::File getLogDirectory() const;

    /** Get the current session log file. */
    juce::File getCurrentLogFile() const { return currentLogFile; }

private:
    WFSLogger();
    ~WFSLogger() override;

    // Timer callback drains audio thread queue
    void timerCallback() override;

    // Log rotation: keep last maxLogFiles sessions
    void rotateLogFiles();

    // Internal write (under lock)
    void writeToFile (Level level, const juce::String& message);

    // Format a timestamp string
    static juce::String formatTimestamp();

    // Level to string
    static const char* levelToString (Level level);

    //==========================================================================
    // Members
    //==========================================================================
    std::unique_ptr<juce::FileLogger> fileLogger;
    juce::CriticalSection writeLock;
    juce::File currentLogFile;

    // Lock-free SPSC ring buffer for audio thread messages
    static constexpr int audioQueueSize = 256;
    juce::String audioMessageQueue[audioQueueSize];
    std::atomic<int> audioWritePos { 0 };
    std::atomic<int> audioReadPos { 0 };

    // File size tracking
    std::atomic<int64_t> currentFileSize { 0 };
    static constexpr int64_t maxFileSize = 10 * 1024 * 1024; // 10 MB
    bool fileSizeLimitReached = false;

    // Rotation
    static constexpr int maxLogFiles = 20;

    static inline WFSLogger* instance = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WFSLogger)
};
