#pragma once

#include <JuceHeader.h>

#if JUCE_WINDOWS
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#endif

#if JUCE_MAC
// Forward declaration for Objective-C helper
void enableDarkTitleBarMac(void* nsWindow);
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
}
