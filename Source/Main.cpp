/*
  ==============================================================================

    This file contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "MainComponent.h"
#include "Parameters/WFSFileManager.h"
#include "gui/WindowUtils.h"
#include "gui/ColorScheme.h"
#include "WFSLogger.h"
#include "UpdateChecker.h"
#include "AppSettings.h"

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
    bool moreThanOneInstanceAllowed() override             { return false; }

    //==============================================================================
    void initialise (const juce::String& commandLine) override
    {
        // Start session logger before anything else
        WFSLogger::getInstance().initialise();

        WFSLogger::getInstance().logInfo ("Session started - " + getApplicationName()
                                          + " v" + getApplicationVersion());
        WFSLogger::getInstance().logInfo ("OS: " + juce::SystemStats::getOperatingSystemName());
        WFSLogger::getInstance().logInfo ("CPU: " + juce::SystemStats::getCpuModel()
                                          + " (" + juce::String (juce::SystemStats::getNumCpus()) + " cores)");

        // Parse command-line for .wfs project file
        auto pendingProjectFolder = parseWfsCommandLine (commandLine);

        mainWindow.reset (new MainWindow (getApplicationName()));

        // Mark session as dirty until clean shutdown (crash detection).
        // Must be AFTER MainWindow creation so SystemConfigTab can read the previous state.
        AppSettings::setCleanShutdown (false);

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

        // Deferred project open: MainComponent constructor has finished but we need
        // the message loop running before loading configs
        if (pendingProjectFolder.isDirectory())
        {
            auto folder = pendingProjectFolder;
            juce::MessageManager::callAsync ([this, folder]()
            {
                if (auto* mainComp = dynamic_cast<MainComponent*> (mainWindow->getContentComponent()))
                    mainComp->openProjectFromFile (folder);
            });
        }

        // Check for updates on GitHub (non-blocking background thread)
        updateChecker = std::make_unique<UpdateChecker> (
            [this] (const juce::String& version, const juce::String& url)
            {
                if (auto* mainComp = dynamic_cast<MainComponent*> (mainWindow->getContentComponent()))
                    mainComp->showUpdateBanner (version, url);
            });
        updateChecker->checkNow();
    }

    void shutdown() override
    {
        AppSettings::setCleanShutdown (true);

#if JUCE_WINDOWS
        // Clear global pointer before window destruction
        g_audioDeviceManager = nullptr;

        // Release 1ms timer resolution requested in initialise()
        timeEndPeriod(1);
#endif

        updateChecker.reset();
        mainWindow = nullptr; // (deletes our window)

        // Shutdown logger last so all other destructors can still log
        WFSLogger::shutdown();
    }

    //==============================================================================
    void systemRequestedQuit() override
    {
        auto* mainComp = dynamic_cast<MainComponent*> (mainWindow->getContentComponent());

        if (mainComp != nullptr && mainComp->isProcessingActive())
        {
            auto options = juce::MessageBoxOptions()
                .withIconType (juce::MessageBoxIconType::WarningIcon)
                .withTitle ("Audio Processing Active")
                .withMessage ("Audio processing is currently running.\n\n"
                              "Are you sure you want to quit?")
                .withButton ("Quit")
                .withButton ("Cancel");

            juce::AlertWindow::showAsync (options, [] (int result)
            {
                if (result == 1)
                    JUCEApplication::getInstance()->quit();
            });
        }
        else
        {
            quit();
        }
    }

    void anotherInstanceStarted (const juce::String& commandLine) override
    {
        auto folder = parseWfsCommandLine (commandLine);
        if (folder.isDirectory())
        {
            if (auto* mainComp = dynamic_cast<MainComponent*> (mainWindow->getContentComponent()))
                mainComp->openProjectFromFile (folder);

            mainWindow->toFront (true);
        }
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
    std::unique_ptr<UpdateChecker> updateChecker;

    /** Parse a .wfs file path from command-line arguments and resolve to project folder */
    static juce::File parseWfsCommandLine (const juce::String& commandLine)
    {
        auto path = commandLine.trim().unquoted().trim();
        if (path.isEmpty())
            return {};

        juce::File wfsFile (path);
        return WFSFileManager::getProjectFolderFromManifest (wfsFile);
    }
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION (WFSDIYApplication)
