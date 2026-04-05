#pragma once

#include <JuceHeader.h>
#include "MapTab.h"
#include "WindowUtils.h"
#include "ColorScheme.h"
#include "../Localization/LocalizationManager.h"

/**
 * MapTabWindow
 *
 * Floating window that hosts the detached MapTab for dual-screen setups.
 * The MapTab is displayed without ownership transfer (setContentNonOwned),
 * so it remains owned by MainComponent throughout its lifetime.
 *
 * Closing this window triggers the onWindowClosed callback, which re-attaches
 * the map back into the main tabbed component.
 */
class MapTabWindow : public juce::DocumentWindow
{
public:
    MapTabWindow(MapTab* mapTabComponent)
        : DocumentWindow(juce::String("WFS-DIY ") + juce::String::charToString(0x2014) + " " + LOC("tabs.map"),
                         ColorScheme::get().background,
                         DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(true);
        setResizable(true, true);

        setContentNonOwned(mapTabComponent, false);

        // Window size — scale with display resolution
        auto& displays = juce::Desktop::getInstance().getDisplays();
        const auto* displayPtr = displays.getPrimaryDisplay();
        juce::Rectangle<int> userArea = (displayPtr != nullptr && !displayPtr->userArea.isEmpty())
            ? displayPtr->userArea
            : displays.getTotalBounds(true);

        float ds = static_cast<float>(userArea.getHeight()) / 1080.0f;
        auto dsc = [ds](int ref) { return juce::jmax(static_cast<int>(ref * 0.65f), static_cast<int>(ref * ds)); };

        const int preferredWidth  = dsc(1200);
        const int preferredHeight = dsc(900);

        const int margin = static_cast<int>(40.0f * ds);
        const int windowWidth  = juce::jmin(preferredWidth,  userArea.getWidth()  - margin);
        const int windowHeight = juce::jmin(preferredHeight, userArea.getHeight() - margin);

        setResizeLimits(dsc(600), dsc(400), userArea.getWidth(), userArea.getHeight());

        centreWithSize(windowWidth, windowHeight);
        setVisible(true);
        WindowUtils::enableDarkTitleBar(this);
    }

    ~MapTabWindow() override = default;

    void closeButtonPressed() override
    {
        if (onWindowClosed)
            onWindowClosed();
    }

    void activeWindowStatusChanged() override
    {
        if (isActiveWindow())
        {
            if (onWindowFocused)
                onWindowFocused();
        }
        else
        {
            if (onWindowUnfocused)
                onWindowUnfocused();
        }
    }

    /** Callback when this window gains focus (for Stream Deck override). */
    std::function<void()> onWindowFocused;

    /** Callback when this window loses focus. */
    std::function<void()> onWindowUnfocused;

    /** Callback when window is closed via the close button. */
    std::function<void()> onWindowClosed;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MapTabWindow)
};
