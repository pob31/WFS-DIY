#pragma once

#include <JuceHeader.h>

/**
 * Compatible screen rendering: System Config's Screen Rendering, Accelerated or
 * Compatible (Windows only, lasts for the session).
 *
 * JUCE 9 draws every window with Direct2D, presented through a DirectComposition
 * swap chain that never paints the window's GDI surface. Capture that reads a
 * single window through GDI - Zoom's "share a window" - therefore keeps showing
 * the first frame. This mode switches every JUCE window to the Software Renderer,
 * which paints through WM_PAINT, and opens menus inside their window so a
 * one-window share includes them. Drawing moves from the GPU to the CPU, so the
 * mode is off at every launch.
 *
 * A new top-level window calls apply() before its first setVisible (true), and a
 * dialog is built with LaunchOptions::create() rather than launchAsync(), which
 * shows it before returning. The Watcher switches any window that missed that
 * within a quarter of a second.
 *
 * JUCE APIs only: no <windows.h> here, WfsLookAndFeel.h includes this.
 */
namespace ScreenShareRendering
{
    class Watcher;

    namespace detail
    {
        inline bool& onFlag()                { static bool on = false;          return on; }
        inline int& defaultEngine()          { static int engine = -1;          return engine; }
        inline Watcher*& watcherInstance()   { static Watcher* watcher = nullptr; return watcher; }
    }

    /** Only Windows has a Direct2D renderer that single-window capture misses. */
    constexpr bool isSupported()
    {
       #if JUCE_WINDOWS
        return true;
       #else
        return false;
       #endif
    }

    inline bool isOn()   { return detail::onFlag(); }

    /** Puts a window on the engine the mode wants. Does nothing for a component
        that is not a window of its own: getPeer() would return its window's peer. */
    inline void apply (juce::Component& c)
    {
       #if JUCE_WINDOWS
        if (! c.isOnDesktop())
            return;

        auto* peer = c.getPeer();

        if (peer == nullptr)
            return;

        // The first window seen is on JUCE's own choice, which "off" restores
        auto& defaultEngine = detail::defaultEngine();

        if (defaultEngine < 0)
            defaultEngine = peer->getCurrentRenderingEngine();

        const auto wanted = isOn() ? peer->getAvailableRenderingEngines().indexOf ("Software Renderer")
                                   : defaultEngine;

        if (wanted >= 0 && peer->getCurrentRenderingEngine() != wanted)
            peer->setCurrentRenderingEngine (wanted);
       #else
        juce::ignoreUnused (c);
       #endif
    }

    /** Desktop components, not ComponentPeer::getPeer(): a peer joins that list
        before its renderer exists, and asking it for its engine then crashes. */
    inline void applyToAllWindows()
    {
        auto& desktop = juce::Desktop::getInstance();

        for (int i = desktop.getNumComponents(); --i >= 0;)
            if (auto* c = desktop.getComponent (i))
                apply (*c);
    }

    /** Where a menu opens while the mode is on: inside the app window it was
        opened from, so a one-window share shows it. nullptr keeps the usual
        choice - an explicit parent, a submenu's inherited parent, or a desktop
        window. Dialogs and alerts keep desktop menus: they are too small to
        hold a long list. */
    inline juce::Component* menuParentFor (const juce::PopupMenu::Options& options)
    {
        if (! isOn() || options.getParentComponent() != nullptr)
            return nullptr;

        auto* target = options.getTargetComponent();

        if (target == nullptr)
            return nullptr;

        auto* window = dynamic_cast<juce::DocumentWindow*> (target->getTopLevelComponent());

        if (window == nullptr || dynamic_cast<juce::DialogWindow*> (window) != nullptr)
            return nullptr;

        if (! window->isOnDesktop() || ! window->isShowing())
            return nullptr;

        // The window itself rather than its content: the menu then sits above
        // the content's always-on-top overlays (help cards, wizard, pickers).
        return window;
    }

    /** Switches, while the mode is on, any window created without apply().
        Owned by the application for its lifetime. */
    class Watcher : private juce::Timer
    {
    public:
        Watcher()             { detail::watcherInstance() = this; sync(); }
        ~Watcher() override   { stopTimer(); detail::watcherInstance() = nullptr; }

        void sync()
        {
            if (isOn())
                startTimerHz (4);
            else
                stopTimer();
        }

    private:
        void timerCallback() override   { applyToAllWindows(); }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Watcher)
    };

    /** Switches the mode and every open window. The windows switch on the next
        message-loop turn: the toggle's click runs inside a window's own event. */
    inline void setOn (bool shouldBeOn)
    {
        if (! isSupported() || shouldBeOn == isOn())
            return;

        detail::onFlag() = shouldBeOn;

        if (auto* watcher = detail::watcherInstance())
            watcher->sync();

        juce::MessageManager::callAsync ([] { applyToAllWindows(); });
    }
}
