#pragma once

#include <JuceHeader.h>

/**
 * LocalizationManager - Centralized string localization manager
 *
 * Manages loading and retrieval of localized strings from JSON resource files.
 * Uses dot-notation key paths for hierarchical string access.
 *
 * JSON Structure:
 *   {
 *     "meta": { "language": "English", "locale": "en" },
 *     "common": { "ok": "OK", "cancel": "Cancel" },
 *     "tabs": { "inputs": "Inputs", "outputs": "Outputs" },
 *     "systemConfig": {
 *       "labels": { "showName": "Show Name" },
 *       "help": { "showName": "Name of the current show." }
 *     }
 *   }
 *
 * Usage:
 *   // Initialize at startup
 *   LocalizationManager::getInstance().loadLanguage("en");
 *
 *   // Get strings by key path
 *   auto text = LocalizationManager::getInstance().get("systemConfig.labels.showName");
 *
 *   // Convenience macro
 *   label.setText(LOC("tabs.inputs"), juce::dontSendNotification);
 */
class LocalizationManager
{
public:
    //==========================================================================
    // Translation tier
    //==========================================================================

    /** How much of the UI is translated for the selected (non-English) locale. */
    enum class TranslationTier
    {
        Minimal,  // Control surface English; only prose (help/messages/dialogs) translated.
        Full      // Full localization: labels + parameter names translated too.
    };

    /** Persisted string form of a tier ("minimal" / "full"). */
    static juce::String tierToString(TranslationTier tier)
    {
        return tier == TranslationTier::Full ? "full" : "minimal";
    }

    /** Parse a persisted tier string; unknown values default to Minimal. */
    static TranslationTier tierFromString(const juce::String& s)
    {
        return s == "full" ? TranslationTier::Full : TranslationTier::Minimal;
    }

    //==========================================================================
    // Singleton Access
    //==========================================================================

    static LocalizationManager& getInstance();

    //==========================================================================
    // Initialization
    //==========================================================================

    /**
     * Load strings from JSON file for the specified locale and tier.
     * Minimal tier reads Resources/lang/<locale>.json (prose only);
     * Full tier reads Resources/lang/full/<locale>.json (full localization).
     * en.json is always overlaid as the English fallback base. English ignores tier.
     *
     * @param locale Language code (e.g., "en", "fr", "de")
     * @param tier   How much of the UI to translate (default Minimal)
     * @return true if loaded successfully
     */
    bool loadLanguage(const juce::String& locale, TranslationTier tier = TranslationTier::Minimal);

    /**
     * Load strings from a JSON string (useful for embedded resources)
     */
    bool loadFromString(const juce::String& jsonString, const juce::String& locale);

    /** Get current language locale code */
    juce::String getCurrentLocale() const { return currentLocale; }

    /** Get the current translation tier. */
    TranslationTier getCurrentTier() const { return currentTier; }

    /** Check if a language is loaded */
    bool isLoaded() const { return stringsRoot.isObject(); }

    /**
     * Get list of available languages by scanning Resources/lang/ directory
     */
    juce::StringArray getAvailableLanguages() const;

    //==========================================================================
    // String Retrieval
    //==========================================================================

    /**
     * Get localized string by dot-separated key path.
     *
     * @param keyPath Dot-separated path (e.g., "systemConfig.labels.showName")
     * @return Localized string, or the key path itself if not found (for debugging)
     */
    juce::String get(const juce::String& keyPath) const;

    /**
     * Get localized string with parameter substitution.
     * Parameters are specified as {name} in the string.
     *
     * @param keyPath Dot-separated path
     * @param params Map of placeholder names to values (without braces)
     *
     * Example:
     *   // JSON: "greeting": "Hello, {name}!"
     *   get("greeting", {{"name", "World"}})  // Returns "Hello, World!"
     */
    juce::String get(const juce::String& keyPath,
                     const std::map<juce::String, juce::String>& params) const;

    /**
     * Get a string from the current locale's Full-tier translation, whatever
     * tier is selected, falling back to get() when that file lacks the key.
     *
     * For the strings that explain the tier choice itself: the Minimal tier
     * keeps control labels English, but someone who has not chosen a tier yet
     * must still be able to read what the choices mean in their own language.
     * Resolve it when the text is shown, not once at construction, so it
     * follows a language change without a restart.
     */
    juce::String getFullTier(const juce::String& keyPath) const;

    /** getFullTier() with {name} parameter substitution, as get(keyPath, params). */
    juce::String getFullTier(const juce::String& keyPath,
                             const std::map<juce::String, juce::String>& params) const;

    /**
     * Convenience method for common.* strings
     */
    juce::String common(const juce::String& key) const;

    /**
     * Convenience method for units.* strings
     */
    juce::String unit(const juce::String& key) const;

    /**
     * Check if a key path exists in the current language
     */
    bool hasKey(const juce::String& keyPath) const;

    //==========================================================================
    // Resource Directory
    //==========================================================================

    /** Set custom resource directory (defaults to app bundle Resources) */
    void setResourceDirectory(const juce::File& dir);

    /** Get resource directory */
    juce::File getResourceDirectory() const;

    /**
     * Clear all loaded resources. Call before app shutdown to avoid
     * JUCE leak detector warnings with static singleton.
     */
    void shutdown();

private:
    LocalizationManager() = default;
    ~LocalizationManager() = default;

    juce::String currentLocale = "en";
    TranslationTier currentTier = TranslationTier::Minimal;
    juce::var stringsRoot;
    juce::var fullTierRoot;  // full/<locale>.json alone; void for English or when absent
    juce::File resourceDirectory;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LocalizationManager)
};

//==============================================================================
// Convenience Macro
//==============================================================================

/**
 * Shorthand macro for getting localized strings.
 * Usage: LOC("systemConfig.labels.showName")
 */
#define LOC(key) LocalizationManager::getInstance().get(key)
