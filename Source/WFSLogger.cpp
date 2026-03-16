#include "WFSLogger.h"

//==============================================================================
WFSLogger::WFSLogger() {}

WFSLogger::~WFSLogger()
{
    stopTimer();

    // Drain any remaining audio thread messages
    int rp = audioReadPos.load (std::memory_order_relaxed);
    int wp = audioWritePos.load (std::memory_order_acquire);
    while (rp != wp)
    {
        writeToFile (Level::Info, "[AUDIO] " + audioMessageQueue[rp]);
        audioMessageQueue[rp] = juce::String();
        rp = (rp + 1) % audioQueueSize;
    }
    audioReadPos.store (rp, std::memory_order_release);

    if (fileLogger)
    {
        writeToFile (Level::Info, "Session ended");
        fileLogger.reset();
    }
}

//==============================================================================
void WFSLogger::initialise()
{
    auto logDir = getLogDirectory();
    if (!logDir.exists())
        logDir.createDirectory();

    // Rotate old log files
    rotateLogFiles();

    // Create session log file with timestamp
    auto now = juce::Time::getCurrentTime();
    auto filename = "WFS-DIY_" + now.formatted ("%Y-%m-%d_%H-%M-%S") + ".log";
    currentLogFile = logDir.getChildFile (filename);

    fileLogger = std::make_unique<juce::FileLogger> (currentLogFile, juce::String(), 0);
    currentFileSize = 0;
    fileSizeLimitReached = false;

    // Start drain timer (100ms)
    startTimer (100);
}

//==============================================================================
juce::File WFSLogger::getLogDirectory() const
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("WFS-DIY")
               .getChildFile ("logs");
}

//==============================================================================
void WFSLogger::rotateLogFiles()
{
    auto logDir = getLogDirectory();
    if (!logDir.exists())
        return;

    auto logFiles = logDir.findChildFiles (juce::File::findFiles, false, "WFS-DIY_*.log");

    // Sort by modification time, newest first
    logFiles.sort();
    std::sort (logFiles.begin(), logFiles.end(),
               [](const juce::File& a, const juce::File& b)
               {
                   return a.getLastModificationTime() > b.getLastModificationTime();
               });

    // Delete files beyond the limit (keep maxLogFiles - 1 to leave room for new session)
    for (int i = maxLogFiles - 1; i < logFiles.size(); ++i)
        logFiles[i].deleteFile();
}

//==============================================================================
void WFSLogger::log (Level level, const juce::String& message)
{
    const juce::ScopedLock sl (writeLock);
    writeToFile (level, message);
}

//==============================================================================
void WFSLogger::logFromAudioThread (const juce::String& message)
{
    // Lock-free SPSC enqueue
    int wp = audioWritePos.load (std::memory_order_relaxed);
    int nextWp = (wp + 1) % audioQueueSize;

    // If queue full, silently drop
    if (nextWp == audioReadPos.load (std::memory_order_acquire))
        return;

    audioMessageQueue[wp] = message;
    audioWritePos.store (nextWp, std::memory_order_release);
}

//==============================================================================
void WFSLogger::timerCallback()
{
    // Drain audio thread queue
    int rp = audioReadPos.load (std::memory_order_relaxed);
    int wp = audioWritePos.load (std::memory_order_acquire);

    if (rp == wp)
        return;

    const juce::ScopedLock sl (writeLock);

    while (rp != wp)
    {
        writeToFile (Level::Info, "[AUDIO] " + audioMessageQueue[rp]);
        audioMessageQueue[rp] = juce::String();
        rp = (rp + 1) % audioQueueSize;
    }

    audioReadPos.store (rp, std::memory_order_release);
}

//==============================================================================
void WFSLogger::writeToFile (Level level, const juce::String& message)
{
    if (fileLogger == nullptr)
        return;

    if (fileSizeLimitReached)
        return;

    auto line = "[" + formatTimestamp() + "] [" + levelToString (level) + "] " + message;

    // Check file size limit
    auto lineSize = static_cast<int64_t> (line.getNumBytesAsUTF8() + 1); // +1 for newline
    auto newSize = currentFileSize.load (std::memory_order_relaxed) + lineSize;

    if (newSize > maxFileSize)
    {
        fileLogger->logMessage ("[" + formatTimestamp() + "] [WARNING] Log file size limit reached (10 MB) - truncating");
        fileSizeLimitReached = true;
        return;
    }

    fileLogger->logMessage (line);
    currentFileSize.store (newSize, std::memory_order_relaxed);
}

//==============================================================================
void WFSLogger::logStateSnapshot (const juce::String& context)
{
    const juce::ScopedLock sl (writeLock);

    writeToFile (Level::Info, "=== State Snapshot: " + context + " ===");
    writeToFile (Level::Info, "OS: " + juce::SystemStats::getOperatingSystemName());
    writeToFile (Level::Info, "CPU: " + juce::SystemStats::getCpuModel()
                              + " (" + juce::String (juce::SystemStats::getNumCpus()) + " cores)");
    writeToFile (Level::Info, "Memory: " + juce::String (juce::SystemStats::getMemorySizeInMegabytes()) + " MB");
    writeToFile (Level::Info, "Stack trace:\n" + juce::SystemStats::getStackBacktrace());
    writeToFile (Level::Info, "=== End State Snapshot ===");
}

//==============================================================================
bool WFSLogger::exportLogs (const juce::File& destinationDir)
{
    auto exportDir = destinationDir.getChildFile ("WFS-DIY-logs");
    if (!exportDir.createDirectory())
        return false;

    auto logDir = getLogDirectory();
    if (!logDir.exists())
        return false;

    // Collect all log files, sort by modification time (newest first)
    auto logFiles = logDir.findChildFiles (juce::File::findFiles, false, "WFS-DIY_*.log");
    std::sort (logFiles.begin(), logFiles.end(),
               [](const juce::File& a, const juce::File& b)
               {
                   return a.getLastModificationTime() > b.getLastModificationTime();
               });

    // Copy up to 6 most recent logs (current session + last 5)
    int copied = 0;
    for (int i = 0; i < juce::jmin (6, logFiles.size()); ++i)
    {
        if (logFiles[i].copyFileTo (exportDir.getChildFile (logFiles[i].getFileName())))
            ++copied;
    }

    // Copy settings file
    auto settingsDir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                           .getChildFile ("WFS-DIY");
    auto settingsFile = settingsDir.getChildFile ("WFS-DIY.settings");
    if (settingsFile.existsAsFile())
        settingsFile.copyFileTo (exportDir.getChildFile (settingsFile.getFileName()));

    return copied > 0;
}

//==============================================================================
juce::String WFSLogger::formatTimestamp()
{
    auto now = juce::Time::getCurrentTime();
    return now.formatted ("%Y-%m-%d %H:%M:%S") + "." +
           juce::String (now.getMilliseconds()).paddedLeft ('0', 3);
}

const char* WFSLogger::levelToString (Level level)
{
    switch (level)
    {
        case Level::Info:    return "INFO";
        case Level::Warning: return "WARNING";
        case Level::Error:   return "ERROR";
        default:             return "INFO";
    }
}
