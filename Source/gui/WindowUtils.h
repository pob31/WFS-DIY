#pragma once

#include <JuceHeader.h>

#if JUCE_WINDOWS
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#endif

#if JUCE_MAC
// Forward declarations for Objective-C helpers
void enableDarkTitleBarMac(void* nsWindow);
void beginRealtimeAudioActivityMac();
void endRealtimeAudioActivityMac();
#endif

namespace WindowUtils
{
    /**
     * Enable dark mode title bar on Windows 10/11 and macOS.
     * Call this after the window is made visible.
     */
    inline void enableDarkTitleBar(juce::Component* window)
    {
#if JUCE_WINDOWS
        if (auto* peer = window->getPeer())
        {
            if (auto* handle = static_cast<HWND>(peer->getNativeHandle()))
            {
                // DWMWA_USE_IMMERSIVE_DARK_MODE = 20 (Windows 10 20H1+)
                BOOL darkMode = TRUE;
                ::DwmSetWindowAttribute(handle, 20, &darkMode, sizeof(darkMode));
            }
        }
#elif JUCE_MAC
        if (auto* peer = window->getPeer())
        {
            enableDarkTitleBarMac(peer->getNativeHandle());
        }
#else
        juce::ignoreUnused(window);
#endif
    }

    /**
     * Real-time audio: opt the process out of macOS App Nap and timer coalescing
     * for the app's lifetime, so the DSP worker threads stay on the performance
     * cores even when the window is not frontmost. macOS counterpart to the
     * Windows EcoQoS / HIGH_PRIORITY_CLASS opt-out in Main.cpp. No-op elsewhere.
     * Call once at startup and pair with endRealtimeAudioActivity() at shutdown.
     */
    inline void beginRealtimeAudioActivity()
    {
#if JUCE_MAC
        beginRealtimeAudioActivityMac();
#endif
    }

    /** Ends the activity started by beginRealtimeAudioActivity(). No-op elsewhere. */
    inline void endRealtimeAudioActivity()
    {
#if JUCE_MAC
        endRealtimeAudioActivityMac();
#endif
    }
}
