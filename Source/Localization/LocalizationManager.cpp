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

namespace
{
    /**
     * Recursively merge `overlay` into `base`. Where both hold an object at the
     * same key, recurse; otherwise the overlay value wins. Keys present only in
     * `base` are left untouched. Used to overlay a locale on top of the English
     * base so any key a locale omits falls back to English rather than a raw key.
     */
    void mergeInto(juce::var& base, const juce::var& overlay)
    {
        auto* overlayObj = overlay.getDynamicObject();
        auto* baseObj = base.getDynamicObject();

        if (overlayObj == nullptr || baseObj == nullptr)
        {
            base = overlay;  // not both objects -> overlay replaces base wholesale
            return;
        }

        for (const auto& prop : overlayObj->getProperties())
        {
            const auto& name = prop.name;
            const juce::var& overlayVal = prop.value;

            if (baseObj->hasProperty(name))
            {
                juce::var childBase = baseObj->getProperty(name);
                if (childBase.getDynamicObject() != nullptr
                    && overlayVal.getDynamicObject() != nullptr)
                {
                    mergeInto(childBase, overlayVal);  // both objects -> recurse
                    baseObj->setProperty(name, childBase);
                    continue;
                }
            }

            baseObj->setProperty(name, overlayVal);
        }
    }

    /** Walk a dot-separated key path from `root`; void when a step is missing. */
    juce::var resolveKey(const juce::var& root, const juce::String& keyPath)
    {
        juce::StringArray pathComponents;
        pathComponents.addTokens(keyPath, ".", "");

        juce::var current = root;

        for (const auto& component : pathComponents)
        {
            if (!current.isObject())
                return {};  // Path doesn't exist

            current = current[juce::Identifier(component)];
        }

        return current;
    }
}

bool LocalizationManager::loadLanguage(const juce::String& locale, TranslationTier tier)
{
    auto langDir = getResourceDirectory().getChildFile("lang");

    // Tier selects which locale file to overlay. The "Full" tier lives in a
    // lang/full/ subfolder (labels + parameter names translated); the default
    // "Minimal" tier is the flat lang/<locale>.json (prose only, control surface
    // English). English ignores the tier (it is the overlay base in both cases).
    juce::File langFile;
    if (locale != "en" && tier == TranslationTier::Full)
    {
        langFile = langDir.getChildFile("full").getChildFile(locale + ".json");
        if (!langFile.existsAsFile())
        {
            // Defensive: a missing full-tier file falls back to the minimal locale
            // rather than failing, so the UI never breaks on a partial install.
            DBG("LocalizationManager: Full-tier file missing, falling back to minimal: " + langFile.getFullPathName());
            langFile = langDir.getChildFile(locale + ".json");
        }
    }
    else
    {
        langFile = langDir.getChildFile(locale + ".json");
    }

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

    // English is the source of truth. For any other locale, load en.json as a
    // base and overlay the locale on top, so labels/keys the locale intentionally
    // omits (or hasn't translated yet) resolve to English instead of a raw key.
    // The en base is always the flat superset en.json, for both tiers.
    // The Full tier stacks en.json, then the minimal <locale>.json, then
    // full/<locale>.json: a message the full file has not caught up with yet
    // then shows in the minimal tier's translation instead of in English.
    if (locale != "en")
    {
        auto enJson = juce::JSON::parse(langDir.getChildFile("en.json"));
        if (enJson.isObject())
        {
            if (tier == TranslationTier::Full && langFile != langDir.getChildFile(locale + ".json"))
            {
                auto minimalJson = juce::JSON::parse(langDir.getChildFile(locale + ".json"));
                if (minimalJson.isObject())
                    mergeInto(enJson, minimalJson);
            }

            mergeInto(enJson, json);
            json = enJson;
        }
    }

    // getFullTier() reads full/<locale>.json on its own, with no en base, so a
    // key it lacks falls through to get(). The Full tier already has it in
    // stringsRoot, so only the Minimal tier needs the file parsed separately.
    fullTierRoot = juce::var();
    if (locale != "en" && tier == TranslationTier::Minimal)
    {
        auto fullFile = langDir.getChildFile("full").getChildFile(locale + ".json");
        if (fullFile.existsAsFile())
            fullTierRoot = juce::JSON::parse(fullFile);
    }

    stringsRoot = json;
    currentLocale = locale;
    currentTier = tier;

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
    fullTierRoot = juce::var();
    currentLocale = locale;
    return true;
}

juce::StringArray LocalizationManager::getAvailableLanguages() const
{
    juce::StringArray languages;
    auto langDir = getResourceDirectory().getChildFile("lang");

    if (langDir.isDirectory())
    {
        // Non-recursive on purpose: the lang/full/ subfolder (Full-tier files)
        // must NOT appear here as fake languages. Keep the 'false' (no recursion).
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
    auto value = resolveKey(stringsRoot, keyPath);

    if (value.isString())
        return value.toString();

    return keyPath;  // Missing or not a string: return the key as fallback
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

juce::String LocalizationManager::getFullTier(const juce::String& keyPath) const
{
    auto value = resolveKey(fullTierRoot, keyPath);

    if (value.isString())
        return value.toString();

    return get(keyPath);
}

juce::String LocalizationManager::getFullTier(const juce::String& keyPath,
                                               const std::map<juce::String, juce::String>& params) const
{
    juce::String result = getFullTier(keyPath);

    for (const auto& [key, value] : params)
        result = result.replace("{" + key + "}", value);

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
    return resolveKey(stringsRoot, keyPath).isString();
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
    fullTierRoot = juce::var();
}
