/*
  ==============================================================================

    This file contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "MainComponent.h"
#include "gui/WindowUtils.h"
#include "gui/ColorScheme.h"
#include "WFSLogger.h"

//==============================================================================
// Windows crash handler to release ASIO driver on unhandled exceptions
//==============================================================================
#if JUCE_WINDOWS
#include <Windows.h>
#include <timeapi.h>    // timeBeginPeriod / timeEndPeriod
#pragma comment(lib, "winmm.lib")

// Global pointer for emergency audio cleanup
static juce::AudioDeviceManager* g_audioDeviceManager = nullptr;

LONG WINAPI asioCleanupCrashHandler(EXCEPTION_POINTERS* exceptionInfo)
{
    juce::ignoreUnused(exceptionInfo);

    // Best-effort crash logging
    try
    {
        WFSLogger::getInstance().logError ("CRASH: Unhandled exception");
        WFSLogger::getInstance().logStateSnapshot ("Crash");
    }
    catch (...) {}

    // Attempt emergency audio device shutdown to release ASIO driver
    if (g_audioDeviceManager != nullptr)
    {
        try
        {
            g_audioDeviceManager->closeAudioDevice();
        }
        catch (...) { /* Best effort - ignore any exceptions during cleanup */ }
    }

    // Let Windows handle the crash (show error dialog, generate dump, etc.)
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif

//==============================================================================
class WFSDIYApplication  : public juce::JUCEApplication
{
public:
    //==============================================================================
    WFSDIYApplication() {}

    const juce::String getApplicationName() override       { return ProjectInfo::projectName; }
    const juce::String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override             { return true; }

    //==============================================================================
    void initialise (const juce::String& commandLine) override
    {
        juce::ignoreUnused(commandLine);

        // Start session logger before anything else
        WFSLogger::getInstance().initialise();
        WFSLogger::getInstance().logInfo ("Session started - " + getApplicationName()
                                          + " v" + getApplicationVersion());
        WFSLogger::getInstance().logInfo ("OS: " + juce::SystemStats::getOperatingSystemName());
        WFSLogger::getInstance().logInfo ("CPU: " + juce::SystemStats::getCpuModel()
                                          + " (" + juce::String (juce::SystemStats::getNumCpus()) + " cores)");

        mainWindow.reset (new MainWindow (getApplicationName()));

#if JUCE_WINDOWS
        // Real-time audio: raise process priority so Windows won't deprioritize
        // our worker threads when the window is minimized.
        SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

        // Opt out of EcoQoS / Efficiency Mode power throttling.
        PROCESS_POWER_THROTTLING_STATE throttlingState = {};
        throttlingState.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
        throttlingState.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;
        throttlingState.StateMask = 0;  // 0 = disable throttling
        SetProcessInformation(GetCurrentProcess(), ProcessPowerThrottling,
                              &throttlingState, sizeof(throttlingState));

        // Request 1ms timer/wait resolution for audio worker thread polling.
        timeBeginPeriod(1);

        // Register crash handler to release ASIO driver on unhandled exceptions
        if (auto* mainComp = dynamic_cast<MainComponent*>(mainWindow->getContentComponent()))
        {
            g_audioDeviceManager = &mainComp->getDeviceManager();
            SetUnhandledExceptionFilter(asioCleanupCrashHandler);
        }
#endif
    }

    void shutdown() override
    {
#if JUCE_WINDOWS
        // Clear global pointer before window destruction
        g_audioDeviceManager = nullptr;

        // Release 1ms timer resolution requested in initialise()
        timeEndPeriod(1);
#endif

        mainWindow = nullptr; // (deletes our window)

        // Shutdown logger last so all other destructors can still log
        WFSLogger::shutdown();
    }

    //==============================================================================
    void systemRequestedQuit() override
    {
        // This is called when the app is being asked to quit: you can ignore this
        // request and let the app carry on running, or call quit() to allow the app to close.
        quit();
    }

    void anotherInstanceStarted (const juce::String& commandLine) override
    {
        juce::ignoreUnused(commandLine);
        // When another instance of the app is launched while this one is running,
        // this method is invoked, and the commandLine parameter tells you what
        // the other instance's command-line arguments were.
    }

    //==============================================================================
    /*
        This class implements the desktop window that contains an instance of
        our MainComponent class.
    */
    class MainWindow    : public juce::DocumentWindow
    {
    public:
        MainWindow (juce::String name)
            : DocumentWindow (name,
                              ColorScheme::get().background,  // Dark background to avoid white flash
                              DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (new MainComponent(), true);

           #if JUCE_IOS || JUCE_ANDROID
            setFullScreen (true);
           #else
            setResizable (true, true);
            centreWithSize (getWidth(), getHeight());
           #endif

            setVisible (true);
            WindowUtils::enableDarkTitleBar (this);
        }

        void closeButtonPressed() override
        {
            // This is called when the user tries to close this window. Here, we'll just
            // ask the app to quit when this happens, but you can change this to do
            // whatever you need.
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

        /* Note: Be careful if you override any DocumentWindow methods - the base
           class uses a lot of them, so by overriding you might break its functionality.
           It's best to do all your work in your content component instead, but if
           you really have to override any DocumentWindow methods, make sure your
           subclass also calls the superclass's method.
        */

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION (WFSDIYApplication)
