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
     * Keeps a window's maximum size to the user area of the display it is on,
     * keeping its minimum. Call from the window's moved(): the windows set
     * their limits from the main display when they open, so on a taller
     * second display (a 16:10 touch screen beside a 1080p main one) a zoomed
     * window stopped short of the bottom - and a cap taken from the largest
     * display would let a window on a smaller one spill past its edge.
     * A window dragged onto a smaller display shrinks to fit it.
     */
    inline void fitResizeLimitsToDisplay(juce::ResizableWindow& window)
    {
        const auto* display = juce::Desktop::getInstance().getDisplays().getDisplayForRect(window.getScreenBounds());
        auto* constrainer = window.getConstrainer();
        if (display == nullptr || constrainer == nullptr)
            return;

        const auto area = display->userArea;
        if (constrainer->getMaximumWidth() == area.getWidth() && constrainer->getMaximumHeight() == area.getHeight())
            return;   // same display: nothing to do (and no re-constraining on every step of a drag)

        window.setResizeLimits(constrainer->getMinimumWidth(), constrainer->getMinimumHeight(),
                               area.getWidth(), area.getHeight());
    }

    /**
     * Preserve raw multi-touch on Windows under JUCE 9, which delivers OS
     * gestures (pinch → mouseMagnify) by default instead of per-finger events.
     * The app's hand-rolled pinch/multi-drag (EQDisplay, MapTab, PatchMatrix)
     * needs the raw points, so every window hosting them opts back in; doing so
     * disables OS gestures/mouseMagnify() on that window on Windows only —
     * macOS trackpad mouseMagnify is unaffected. No-op under JUCE 8 and on
     * other OSes. Call from the window constructor.
     */
    inline void enableRawMultiTouch(juce::TopLevelWindow* window)
    {
#if JUCE_WINDOWS && (JUCE_MAJOR_VERSION >= 9)
        window->setUsingWindowsMultiTouch(true);
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
