#pragma once

#include <JuceHeader.h>

/**
 * TTSManager - Centralized Text-to-Speech manager for accessibility
 *
 * Provides screen reader integration via JUCE's AccessibilityHandler.
 * Always active - postAnnouncement() is a no-op when no screen reader is running.
 *
 * Behavior:
 * - On component hover: Immediately announce parameter name and current value
 * - After 3.5 seconds of static stay: Announce full help text description
 * - Rate limiting prevents speech overlap (max 2 announcements/second)
 * - Debounced announcements wait for pointer to settle before speaking
 *
 * Usage:
 *   TTSManager::getInstance().onComponentEnter("X Position", "2.5 m", "Object position in Width...");
 *   TTSManager::getInstance().onComponentExit();
 *   TTSManager::getInstance().announceDebounced("Cell info"); // For rapid hover updates
 */

// Helper class for debounce timer (separate from main help text timer)
class TTSDebounceTimer : public juce::Timer
{
public:
    std::function<void()> callback;
    void timerCallback() override
    {
        stopTimer();
        if (callback)
            callback();
    }
};

class TTSManager : private juce::Timer
{
public:
    //==========================================================================
    // Singleton Access
    //==========================================================================

    static TTSManager& getInstance()
    {
        // Create instance on first use
        if (instance == nullptr)
            instance = new TTSManager();
        return *instance;
    }

    /**
     * Call from MainComponent destructor before JUCE shuts down.
     * Destroys the singleton so Timer base class destructor runs while JUCE is still alive.
     */
    static void shutdown()
    {
        if (instance != nullptr)
        {
            delete instance;
            instance = nullptr;
        }
    }

    //==========================================================================
    // Configuration
    //==========================================================================

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
        if (text.isEmpty()) return;

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
        juce::String text = componentName + ": " + newValue;
        doAnnouncement(text, juce::AccessibilityHandler::AnnouncementPriority::medium);
    }

    /**
     * Debounced announcement for rapid hover updates (e.g., patch matrix cells).
     * Waits for pointer to settle before announcing, cancels stale announcements.
     * Bypasses rate limiting since debounce already prevents announcement spam.
     *
     * @param text Text to announce
     * @param debounceMs Delay before announcing (default 300ms - long enough to skip intermediate cells)
     */
    void announceDebounced(const juce::String& text, int debounceMs = 300)
    {
        juce::ScopedLock sl(lock);
        if (text.isEmpty()) return;

        // Cancel any pending debounced announcement
        debounceTimer.stopTimer();

        // Store the pending text
        debouncedText = text;

        // Set up callback and start timer
        // Debounced announcements bypass rate limiting since the debounce mechanism
        // already prevents spam - only the final position is announced
        debounceTimer.callback = [this]()
        {
            juce::ScopedLock sl2(lock);
            if (debouncedText.isNotEmpty())
            {
                juce::AccessibilityHandler::postAnnouncement(debouncedText,
                    juce::AccessibilityHandler::AnnouncementPriority::medium);
                lastAnnouncementTime = juce::Time::currentTimeMillis();
                debouncedText.clear();
            }
        };
        debounceTimer.startTimer(debounceMs);
    }

    /**
     * Cancel any pending debounced announcement.
     * Call this on mouseExit to prevent stale announcements.
     */
    void cancelDebouncedAnnouncement()
    {
        juce::ScopedLock sl(lock);
        debounceTimer.stopTimer();
        debouncedText.clear();
    }

    //==========================================================================
    // Component Value Extraction Helpers
    //==========================================================================

    /**
     * Get the current value from a component as a string.
     * Handles common JUCE component types: Slider, ComboBox, TextEditor, Button, Label
     */
    static juce::String getComponentValue(juce::Component* component)
    {
        if (component == nullptr)
            return {};

        // Slider (including custom dial/slider subclasses)
        if (auto* slider = dynamic_cast<juce::Slider*>(component))
            return juce::String(slider->getValue(), 2);

        // ComboBox
        if (auto* combo = dynamic_cast<juce::ComboBox*>(component))
            return combo->getText();

        // TextEditor
        if (auto* editor = dynamic_cast<juce::TextEditor*>(component))
            return editor->getText();

        // ToggleButton or TextButton
        if (auto* button = dynamic_cast<juce::Button*>(component))
        {
            if (button->isToggleable())
                return button->getToggleState() ? "On" : "Off";
            return button->getButtonText();
        }

        // Label (display only)
        if (auto* label = dynamic_cast<juce::Label*>(component))
            return label->getText();

        return {};
    }

    /**
     * Extract a short parameter name from help text.
     * Takes the first sentence or up to the first period.
     */
    static juce::String extractParameterName(const juce::String& helpText)
    {
        if (helpText.isEmpty())
            return {};

        // Find first period, comma, or parenthesis
        int endPos = helpText.length();
        for (int i = 0; i < helpText.length(); ++i)
        {
            juce::juce_wchar c = helpText[i];
            if (c == '.' || c == '(' || c == ',')
            {
                endPos = i;
                break;
            }
        }

        return helpText.substring(0, endPos).trim();
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
        debounceTimer.stopTimer();
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
    int helpTextDelayMs = 3500;  // 3.5 seconds for full help
    int minAnnouncementIntervalMs = 500;  // Rate limiting (2 per second max)

    // Current hover state
    juce::String pendingHelpText;
    juce::String currentComponentName;
    juce::int64 componentEnteredTime = 0;
    juce::int64 lastAnnouncementTime = 0;
    bool helpTextAnnounced = false;

    // Debounce state for rapid hover updates
    TTSDebounceTimer debounceTimer;
    juce::String debouncedText;

    // Thread safety
    mutable juce::CriticalSection lock;

    // Singleton instance (pointer-based so we can destroy before JUCE shuts down)
    static inline TTSManager* instance = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TTSManager)
};
