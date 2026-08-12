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

    /** Camera index for the webcam head tracker (machine-local, like all
        settings here — which camera exists is a property of this computer,
        not of the show file). No UI yet: edit WFS-DIY.settings directly. */
    static int getHeadtrackCameraIndex()
    {
        juce::PropertiesFile props (getOptions());
        return props.getIntValue ("headtrackCameraIndex", 0);
    }

    static void setHeadtrackCameraIndex (int index)
    {
        juce::PropertiesFile props (getOptions());
        props.setValue ("headtrackCameraIndex", index);
        props.saveIfNeeded();
    }

    /** USB head-tracker dongle. Machine-local like everything here: which
        hardware is attached is a property of this computer, not of the show
        file. No UI — edit WFS-DIY.settings directly.

        headtrackUsbEnabled     false skips all port probing, for machines that
                                will never have a dongle.
        headtrackUsbPort        explicit port ("COM7", "/dev/ttyACM0",
                                "/dev/cu.usbmodem1401"); empty = auto-discover.
        headtrackUsbReplayFile  path to an htgen capture. Non-empty replays it
                                instead of opening a port, which is how the
                                whole tracker path is exercised with no
                                hardware present.
        headtrackUsbSmoothingHz 1-Euro minimum cutoff for USB trackers; 0
                                bypasses filtering. A fused IMU is quiet enough
                                that heavy smoothing costs more in latency than
                                it buys in steadiness, so this is tunable
                                without a rebuild during bring-up. */
    static bool getHeadtrackUsbEnabled()
    {
        juce::PropertiesFile props (getOptions());
        return props.getBoolValue ("headtrackUsbEnabled", true);
    }

    static void setHeadtrackUsbEnabled (bool enabled)
    {
        juce::PropertiesFile props (getOptions());
        props.setValue ("headtrackUsbEnabled", enabled);
        props.saveIfNeeded();
    }

    static juce::String getHeadtrackUsbPort()
    {
        juce::PropertiesFile props (getOptions());
        return props.getValue ("headtrackUsbPort", "");
    }

    static void setHeadtrackUsbPort (const juce::String& port)
    {
        juce::PropertiesFile props (getOptions());
        props.setValue ("headtrackUsbPort", port);
        props.saveIfNeeded();
    }

    static juce::String getHeadtrackUsbReplayFile()
    {
        juce::PropertiesFile props (getOptions());
        return props.getValue ("headtrackUsbReplayFile", "");
    }

    static void setHeadtrackUsbReplayFile (const juce::String& path)
    {
        juce::PropertiesFile props (getOptions());
        props.setValue ("headtrackUsbReplayFile", path);
        props.saveIfNeeded();
    }

    static double getHeadtrackUsbSmoothingHz()
    {
        juce::PropertiesFile props (getOptions());
        return props.getDoubleValue ("headtrackUsbSmoothingHz", 10.0);
    }

    static void setHeadtrackUsbSmoothingHz (double hz)
    {
        juce::PropertiesFile props (getOptions());
        props.setValue ("headtrackUsbSmoothingHz", hz);
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
