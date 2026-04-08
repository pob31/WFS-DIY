#pragma once
#include <JuceHeader.h>

struct AppSettings
{
    static juce::File getLastFolder (const juce::String& key,
                                     const juce::File& defaultDir = juce::File::getSpecialLocation (juce::File::userHomeDirectory))
    {
        juce::PropertiesFile props (getOptions());
        juce::String path = props.getValue (key, "");
        if (path.isNotEmpty())
        {
            juce::File f (path);
            if (f.isDirectory())
                return f;
        }
        return defaultDir;
    }

    static void setLastFolder (const juce::String& key, const juce::File& folder)
    {
        juce::PropertiesFile props (getOptions());
        props.setValue (key, folder.getFullPathName());
        props.saveIfNeeded();
    }

    static int getStreamDeckBrightness()
    {
        juce::PropertiesFile props (getOptions());
        return props.getIntValue ("streamDeckBrightness", 100);
    }

    static void setStreamDeckBrightness (int percent)
    {
        juce::PropertiesFile props (getOptions());
        props.setValue ("streamDeckBrightness", percent);
        props.saveIfNeeded();
    }

    static juce::String getUpdateDismissedVersion()
    {
        juce::PropertiesFile props (getOptions());
        return props.getValue ("updateDismissedVersion", "");
    }

    static void setUpdateDismissedVersion (const juce::String& version)
    {
        juce::PropertiesFile props (getOptions());
        props.setValue ("updateDismissedVersion", version);
        props.saveIfNeeded();
    }

    static bool getCleanShutdown()
    {
        juce::PropertiesFile props (getOptions());
        return props.getBoolValue ("cleanShutdown", true);
    }

    static void setCleanShutdown (bool clean)
    {
        juce::PropertiesFile props (getOptions());
        props.setValue ("cleanShutdown", clean);
        props.saveIfNeeded();
    }

private:
    static juce::PropertiesFile::Options getOptions()
    {
        juce::PropertiesFile::Options options;
        options.applicationName = "WFS-DIY";
        options.filenameSuffix = ".settings";
        options.osxLibrarySubFolder = "Application Support";
        options.folderName = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                                .getChildFile ("WFS-DIY").getFullPathName();
        return options;
    }
};
