#pragma once

#include <JuceHeader.h>

/**
 * TTSManager - Centralized Text-to-Speech manager for accessibility
 *
 * Provides screen reader integration via JUCE's AccessibilityHandler.
 * Supports immediate announcements on hover and delayed full descriptions.
 *
 * Behavior:
 * - On component hover: Immediately announce parameter name and current value
 * - After 3.5 seconds of static stay: Announce full help text description
 * - Rate limiting prevents speech overlap (max 2 announcements/second)
 *
 * Usage:
 *   TTSManager::getInstance().setEnabled(true);
 *   TTSManager::getInstance().onComponentEnter("X Position", "2.5 m", "Object position in Width...");
 *   TTSManager::getInstance().onComponentExit();
 */
class TTSManager : private juce::Timer
{
public:
    //==========================================================================
    // Singleton Access
    //==========================================================================

    static TTSManager& getInstance()
    {
        static TTSManager instance;
        return instance;
    }

    //==========================================================================
    // Configuration
    //==========================================================================

    /** Enable or disable TTS globally (disabled by default) */
    void setEnabled(bool enabled)
    {
        juce::ScopedLock sl(lock);
        ttsEnabled = enabled;

        if (!enabled)
        {
            stopTimer();
            pendingHelpText.clear();
            currentComponentName.clear();
        }
    }

    bool isEnabled() const
    {
        juce::ScopedLock sl(lock);
        return ttsEnabled;
    }

    /** Set delay before announcing full help text (default: 3500ms) */
    void setHelpTextDelay(int delayMs)
    {
        juce::ScopedLock sl(lock);
        helpTextDelayMs = juce::jmax(500, delayMs);
    }

    int getHelpTextDelay() const
    {
        juce::ScopedLock sl(lock);
        return helpTextDelayMs;
    }

    /** Minimum interval between announcements to prevent overlap (default: 500ms) */
    void setMinAnnouncementInterval(int intervalMs)
    {
        juce::ScopedLock sl(lock);
        minAnnouncementIntervalMs = juce::jmax(100, intervalMs);
    }

    int getMinAnnouncementInterval() const
    {
        juce::ScopedLock sl(lock);
        return minAnnouncementIntervalMs;
    }

    //==========================================================================
    // Announcement API
    //==========================================================================

    /**
     * Called on mouseEnter - announces parameter name and value immediately,
     * then schedules full help text for delayed announcement.
     *
     * @param componentName User-readable name (e.g., "X Position")
     * @param currentValue Formatted value string (e.g., "2.5 m") - can be empty
     * @param helpText Full help description for delayed announcement
     */
    void onComponentEnter(const juce::String& componentName,
                          const juce::String& currentValue,
                          const juce::String& helpText)
    {
        juce::ScopedLock sl(lock);
        if (!ttsEnabled) return;

        // Store state for delayed help text
        currentComponentName = componentName;
        pendingHelpText = helpText;
        helpTextAnnounced = false;
        componentEnteredTime = juce::Time::currentTimeMillis();

        // Build immediate announcement: "Parameter Name: Value" or just "Parameter Name"
        juce::String immediateText = componentName;
        if (currentValue.isNotEmpty())
            immediateText += ": " + currentValue;

        // Announce immediately (respecting rate limit)
        doAnnouncement(immediateText, juce::AccessibilityHandler::AnnouncementPriority::medium);

        // Start timer for delayed help text
        if (helpText.isNotEmpty())
            startTimer(helpTextDelayMs);
    }

    /**
     * Called on mouseExit - cancels pending delayed announcement
     */
    void onComponentExit()
    {
        juce::ScopedLock sl(lock);
        stopTimer();
        pendingHelpText.clear();
        currentComponentName.clear();
        helpTextAnnounced = false;
    }

    /**
     * Force immediate announcement (e.g., for important state changes)
     * Bypasses rate limiting for high priority announcements.
     */
    void announceImmediate(const juce::String& text,
                           juce::AccessibilityHandler::AnnouncementPriority priority =
                               juce::AccessibilityHandler::AnnouncementPriority::medium)
    {
        juce::ScopedLock sl(lock);
        if (!ttsEnabled || text.isEmpty()) return;

        // High priority bypasses rate limiting
        if (priority == juce::AccessibilityHandler::AnnouncementPriority::high)
        {
            juce::AccessibilityHandler::postAnnouncement(text, priority);
            lastAnnouncementTime = juce::Time::currentTimeMillis();
        }
        else
        {
            doAnnouncement(text, priority);
        }
    }

    /**
     * Announce value change during interaction (rate-limited)
     * Use this when a parameter value changes while user is interacting with a control.
     */
    void announceValueChange(const juce::String& componentName,
                             const juce::String& newValue)
    {
        juce::ScopedLock sl(lock);
        if (!ttsEnabled) return;

        juce::String text = componentName + ": " + newValue;
        doAnnouncement(text, juce::AccessibilityHandler::AnnouncementPriority::medium);
    }

    //==========================================================================
    // Settings Persistence
    //==========================================================================

    void saveSettings()
    {
        auto settingsFile = getSettingsFile();
        auto settingsDir = settingsFile.getParentDirectory();

        if (!settingsDir.exists())
            settingsDir.createDirectory();

        juce::var settings = juce::var(new juce::DynamicObject());
        settings.getDynamicObject()->setProperty("enabled", ttsEnabled);
        settings.getDynamicObject()->setProperty("helpTextDelayMs", helpTextDelayMs);
        settings.getDynamicObject()->setProperty("minAnnouncementIntervalMs", minAnnouncementIntervalMs);

        settingsFile.replaceWithText(juce::JSON::toString(settings));
    }

    void loadSettings()
    {
        auto settingsFile = getSettingsFile();

        if (settingsFile.existsAsFile())
        {
            auto json = juce::JSON::parse(settingsFile);
            if (json.isObject())
            {
                juce::ScopedLock sl(lock);
                if (json.hasProperty("enabled"))
                    ttsEnabled = json["enabled"];
                if (json.hasProperty("helpTextDelayMs"))
                    helpTextDelayMs = json["helpTextDelayMs"];
                if (json.hasProperty("minAnnouncementIntervalMs"))
                    minAnnouncementIntervalMs = json["minAnnouncementIntervalMs"];
            }
        }
    }

private:
    TTSManager()
    {
        loadSettings();
    }

    ~TTSManager() override
    {
        stopTimer();
    }

    void timerCallback() override
    {
        juce::ScopedLock sl(lock);
        stopTimer();

        // Announce pending help text if we haven't already
        if (!helpTextAnnounced && pendingHelpText.isNotEmpty())
        {
            helpTextAnnounced = true;
            doAnnouncement(pendingHelpText, juce::AccessibilityHandler::AnnouncementPriority::low);
        }
    }

    bool canAnnounce() const
    {
        auto now = juce::Time::currentTimeMillis();
        return (now - lastAnnouncementTime) >= minAnnouncementIntervalMs;
    }

    void doAnnouncement(const juce::String& text,
                        juce::AccessibilityHandler::AnnouncementPriority priority)
    {
        if (text.isEmpty()) return;

        // Check rate limiting
        if (!canAnnounce())
            return;

        juce::AccessibilityHandler::postAnnouncement(text, priority);
        lastAnnouncementTime = juce::Time::currentTimeMillis();
    }

    juce::File getSettingsFile() const
    {
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("WFS-DIY")
            .getChildFile("tts_settings.json");
    }

    // Configuration
    bool ttsEnabled = true;  // On by default - postAnnouncement() is a no-op when no screen reader is active
    int helpTextDelayMs = 3500;  // 3.5 seconds for full help
    int minAnnouncementIntervalMs = 500;  // Rate limiting (2 per second max)

    // Current hover state
    juce::String pendingHelpText;
    juce::String currentComponentName;
    juce::int64 componentEnteredTime = 0;
    juce::int64 lastAnnouncementTime = 0;
    bool helpTextAnnounced = false;

    // Thread safety
    mutable juce::CriticalSection lock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TTSManager)
};
