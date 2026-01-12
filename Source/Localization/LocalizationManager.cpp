#include "LocalizationManager.h"

//==============================================================================
// Singleton Implementation
//==============================================================================

LocalizationManager& LocalizationManager::getInstance()
{
    static LocalizationManager instance;
    return instance;
}

//==============================================================================
// Initialization
//==============================================================================

bool LocalizationManager::loadLanguage(const juce::String& locale)
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

bool LocalizationManager::loadFromString(const juce::String& jsonString, const juce::String& locale)
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

juce::StringArray LocalizationManager::getAvailableLanguages() const
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

//==============================================================================
// String Retrieval
//==============================================================================

juce::String LocalizationManager::get(const juce::String& keyPath) const
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

juce::String LocalizationManager::get(const juce::String& keyPath,
                                       const std::map<juce::String, juce::String>& params) const
{
    juce::String result = get(keyPath);

    for (const auto& [key, value] : params)
    {
        result = result.replace("{" + key + "}", value);
    }

    return result;
}

juce::String LocalizationManager::common(const juce::String& key) const
{
    return get("common." + key);
}

juce::String LocalizationManager::unit(const juce::String& key) const
{
    return get("units." + key);
}

bool LocalizationManager::hasKey(const juce::String& keyPath) const
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

//==============================================================================
// Resource Directory
//==============================================================================

void LocalizationManager::setResourceDirectory(const juce::File& dir)
{
    resourceDirectory = dir;
}

juce::File LocalizationManager::getResourceDirectory() const
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

void LocalizationManager::shutdown()
{
    stringsRoot = juce::var();  // Clear to avoid leak detector warnings
}
