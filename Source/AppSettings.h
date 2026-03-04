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
