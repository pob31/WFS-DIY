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
    // Singleton Access
    //==========================================================================

    static LocalizationManager& getInstance()
    {
        static LocalizationManager instance;
        return instance;
    }

    //==========================================================================
    // Initialization
    //==========================================================================

    /**
     * Load strings from JSON file for the specified locale.
     * Looks for file in Resources/lang/<locale>.json
     *
     * @param locale Language code (e.g., "en", "fr", "de")
     * @return true if loaded successfully
     */
    bool loadLanguage(const juce::String& locale)
    {
        auto langFile = getResourceDirectory().getChildFile("lang").getChildFile(locale + ".json");

        if (!langFile.existsAsFile())
        {
            DBG("LocalizationManager: Language file not found: " + langFile.getFullPathName());
            return false;
        }

        auto json = juce::JSON::parse(langFile);
        if (!json.isObject())
        {
            DBG("LocalizationManager: Failed to parse language file: " + langFile.getFullPathName());
            return false;
        }

        stringsRoot = json;
        currentLocale = locale;

        DBG("LocalizationManager: Loaded language '" + locale + "' from " + langFile.getFullPathName());
        return true;
    }

    /**
     * Load strings from a JSON string (useful for embedded resources)
     */
    bool loadFromString(const juce::String& jsonString, const juce::String& locale)
    {
        auto json = juce::JSON::parse(jsonString);
        if (!json.isObject())
        {
            DBG("LocalizationManager: Failed to parse JSON string");
            return false;
        }

        stringsRoot = json;
        currentLocale = locale;
        return true;
    }

    /** Get current language locale code */
    juce::String getCurrentLocale() const { return currentLocale; }

    /** Check if a language is loaded */
    bool isLoaded() const { return stringsRoot.isObject(); }

    /**
     * Get list of available languages by scanning Resources/lang/ directory
     */
    juce::StringArray getAvailableLanguages() const
    {
        juce::StringArray languages;
        auto langDir = getResourceDirectory().getChildFile("lang");

        if (langDir.isDirectory())
        {
            for (const auto& file : langDir.findChildFiles(juce::File::findFiles, false, "*.json"))
            {
                languages.add(file.getFileNameWithoutExtension());
            }
        }

        return languages;
    }

    //==========================================================================
    // String Retrieval
    //==========================================================================

    /**
     * Get localized string by dot-separated key path.
     *
     * @param keyPath Dot-separated path (e.g., "systemConfig.labels.showName")
     * @return Localized string, or the key path itself if not found (for debugging)
     */
    juce::String get(const juce::String& keyPath) const
    {
        if (!stringsRoot.isObject())
            return keyPath;  // Return key as fallback

        juce::StringArray pathComponents;
        pathComponents.addTokens(keyPath, ".", "");

        juce::var current = stringsRoot;

        for (const auto& component : pathComponents)
        {
            if (!current.isObject())
                return keyPath;  // Path doesn't exist

            current = current[juce::Identifier(component)];
        }

        if (current.isString())
            return current.toString();

        return keyPath;  // Not a string value
    }

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
                     const std::map<juce::String, juce::String>& params) const
    {
        juce::String result = get(keyPath);

        for (const auto& [key, value] : params)
        {
            result = result.replace("{" + key + "}", value);
        }

        return result;
    }

    /**
     * Convenience method for common.* strings
     */
    juce::String common(const juce::String& key) const
    {
        return get("common." + key);
    }

    /**
     * Convenience method for units.* strings
     */
    juce::String unit(const juce::String& key) const
    {
        return get("units." + key);
    }

    /**
     * Check if a key path exists in the current language
     */
    bool hasKey(const juce::String& keyPath) const
    {
        if (!stringsRoot.isObject())
            return false;

        juce::StringArray pathComponents;
        pathComponents.addTokens(keyPath, ".", "");

        juce::var current = stringsRoot;

        for (const auto& component : pathComponents)
        {
            if (!current.isObject())
                return false;

            current = current[juce::Identifier(component)];

            if (current.isVoid())
                return false;
        }

        return current.isString();
    }

    //==========================================================================
    // Resource Directory
    //==========================================================================

    /** Set custom resource directory (defaults to app bundle Resources) */
    void setResourceDirectory(const juce::File& dir)
    {
        resourceDirectory = dir;
    }

    /** Get resource directory */
    juce::File getResourceDirectory() const
    {
        if (resourceDirectory.exists())
            return resourceDirectory;

        // Default locations based on platform
#if JUCE_MAC
        return juce::File::getSpecialLocation(juce::File::currentApplicationFile)
            .getChildFile("Contents/Resources");
#elif JUCE_WINDOWS
        return juce::File::getSpecialLocation(juce::File::currentExecutableFile)
            .getParentDirectory()
            .getChildFile("Resources");
#else
        return juce::File::getSpecialLocation(juce::File::currentExecutableFile)
            .getParentDirectory()
            .getChildFile("Resources");
#endif
    }

private:
    LocalizationManager() = default;
    ~LocalizationManager() = default;

    juce::String currentLocale = "en";
    juce::var stringsRoot;
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
