#pragma once

/**
 * StreamDeckManager — High-level orchestrator for Stream Deck+ integration.
 *
 * Owns the device driver, renderer, and page registry.
 * Routes UI navigation events (tab/subtab/channel changes) to page switches.
 * Routes device events (button press, dial rotation) to the active page bindings.
 * Handles bidirectional parameter sync and ComboBox dial interaction mode.
 */

#include <JuceHeader.h>
#include "StreamDeckDevice.h"
#include "StreamDeckPage.h"
#include "StreamDeckRenderer.h"

class StreamDeckManager : private juce::Timer
{
public:
    //==========================================================================
    // Construction / Destruction
    //==========================================================================

    StreamDeckManager()
    {
        // Wire device callbacks
        device.onButtonPressed = [this] (int btn) { handleButtonPressed (btn); };
        device.onButtonReleased = [this] (int btn) { handleButtonReleased (btn); };
        device.onDialRotated = [this] (int dial, int dir) { handleDialRotated (dial, dir); };
        device.onDialPressed = [this] (int dial) { handleDialPressed (dial); };
        device.onDialReleased = [this] (int dial) { handleDialReleased (dial); };
        device.onConnectionChanged = [this] (bool connected) { handleConnectionChanged (connected); };

        device.startMonitoring();

        // Start refresh timer for LCD value updates (10Hz)
        startTimer (100);
    }

    ~StreamDeckManager() override
    {
        stopTimer();
        device.stopMonitoring();
    }

    //==========================================================================
    // Page Registration
    //==========================================================================

    /** Register a page for a specific tab + subtab combination.
        @param mainTabIndex   Main tab index (0-based)
        @param subTabIndex    Subtab index within the main tab (0-based, use 0 for tabs without subtabs)
        @param page           The page definition (moved in)
    */
    void registerPage (int mainTabIndex, int subTabIndex, StreamDeckPage page)
    {
        auto key = makePageKey (mainTabIndex, subTabIndex);
        pages[key] = std::move (page);
    }

    /** Check if a page exists for a tab/subtab combination. */
    bool hasPage (int mainTabIndex, int subTabIndex) const
    {
        return pages.count (makePageKey (mainTabIndex, subTabIndex)) > 0;
    }

    //==========================================================================
    // Navigation (called by MainComponent / tab components)
    //==========================================================================

    /** Called when the main tab changes. */
    void setMainTab (int tabIndex)
    {
        if (currentMainTab == tabIndex)
            return;

        currentMainTab = tabIndex;
        exitComboMode();
        switchToCurrentPage();
    }

    /** Called when a subtab changes within the current main tab. */
    void setSubTab (int subTabIndex)
    {
        if (currentSubTab == subTabIndex)
            return;

        currentSubTab = subTabIndex;
        exitComboMode();
        switchToCurrentPage();
    }

    /** Called when the selected channel changes (e.g., input channel selection). */
    void setChannel (int channelIndex)
    {
        if (currentChannel == channelIndex)
            return;

        currentChannel = channelIndex;
        exitComboMode();
        refreshCurrentPage();
    }

    /** Get the current channel index. */
    int getChannel() const { return currentChannel; }

    //==========================================================================
    // Page Rebuilding (call when bindings need to update, e.g., after channel change)
    //==========================================================================

    /** Callback the owner can set to rebuild the current page's bindings.
        Called when the channel changes or when a page refresh is needed.
        The callback receives (mainTab, subTab, channel) and should update
        the registered page's getValue/setValue callbacks. */
    std::function<void (int mainTab, int subTab, int channel)> onPageNeedsRebuild;

    //==========================================================================
    // Direct Access
    //==========================================================================

    /** Get the device for direct image sending (advanced usage). */
    StreamDeckDevice& getDevice() { return device; }

    /** Get the renderer for customization. */
    StreamDeckRenderer& getRenderer() { return renderer; }

    /** Get the currently active page (nullptr if none). */
    StreamDeckPage* getCurrentPage()
    {
        auto key = makePageKey (currentMainTab, currentSubTab);
        auto it = pages.find (key);
        return (it != pages.end()) ? &it->second : nullptr;
    }

    /** Force a full visual refresh of the current page. */
    void refreshCurrentPage()
    {
        if (onPageNeedsRebuild)
            onPageNeedsRebuild (currentMainTab, currentSubTab, currentChannel);

        if (auto* page = getCurrentPage())
        {
            if (device.isConnected())
                renderer.renderAndSendFullPage (device, *page);
        }
    }

private:
    //==========================================================================
    // Event Handlers
    //==========================================================================

    void handleButtonPressed (int buttonIndex)
    {
        auto* page = getCurrentPage();
        if (page == nullptr)
            return;

        if (buttonIndex < 4)
        {
            // Top row: section selector
            if (buttonIndex < page->numSections)
            {
                exitComboMode();
                if (page->setActiveSection (buttonIndex))
                {
                    renderer.renderAndSendFullPage (device, *page);
                }
            }
        }
        else
        {
            // Bottom row: context button (index 4-7 → binding index 0-3)
            int bindingIndex = buttonIndex - 4;
            auto& binding = page->getActiveSection().buttons[bindingIndex];

            if (! binding.isValid())
                return;

            if (binding.type == ButtonBinding::Toggle && binding.getState)
            {
                binding.onPress();
                // Re-render just this button
                auto img = renderer.renderContextButton (binding);
                device.setButtonImage (buttonIndex, img);
            }
            else
            {
                binding.onPress();
            }
        }
    }

    void handleButtonReleased (int buttonIndex)
    {
        auto* page = getCurrentPage();
        if (page == nullptr || buttonIndex < 4)
            return;

        int bindingIndex = buttonIndex - 4;
        auto& binding = page->getActiveSection().buttons[bindingIndex];

        if (binding.isValid() && binding.type == ButtonBinding::Momentary && binding.onRelease)
        {
            binding.onRelease();
            auto img = renderer.renderContextButton (binding);
            device.setButtonImage (buttonIndex, img);
        }
    }

    void handleDialRotated (int dialIndex, int direction)
    {
        auto* page = getCurrentPage();
        if (page == nullptr || dialIndex < 0 || dialIndex >= 4)
            return;

        auto& binding = page->getActiveSection().dials[dialIndex];
        if (! binding.isValid())
            return;

        if (comboModeActive && comboDialIndex == dialIndex)
        {
            // ComboBox browse mode: rotate through options
            comboSelectedIndex += direction;
            comboSelectedIndex = juce::jlimit (0, binding.comboOptions.size() - 1, comboSelectedIndex);

            auto img = renderer.renderLcdZoneComboMode (binding, comboSelectedIndex);
            device.setLcdZoneImage (dialIndex, img);
            return;
        }

        // Normal mode: adjust parameter value
        isUpdatingFromController = true;
        float newVal = binding.applyStep (direction);
        binding.setValue (newVal);
        isUpdatingFromController = false;

        // Update LCD display
        auto img = renderer.renderLcdZone (binding);
        device.setLcdZoneImage (dialIndex, img);
    }

    void handleDialPressed (int dialIndex)
    {
        auto* page = getCurrentPage();
        if (page == nullptr || dialIndex < 0 || dialIndex >= 4)
            return;

        auto& binding = page->getActiveSection().dials[dialIndex];
        if (! binding.isValid())
            return;

        if (binding.type == DialBinding::ComboBox)
        {
            if (comboModeActive && comboDialIndex == dialIndex)
            {
                // Confirm selection and exit combo mode
                isUpdatingFromController = true;
                binding.setValue (static_cast<float> (comboSelectedIndex));
                isUpdatingFromController = false;
                exitComboMode();

                // Redraw normal LCD zone
                auto img = renderer.renderLcdZone (binding);
                device.setLcdZoneImage (dialIndex, img);
            }
            else
            {
                // Enter combo mode
                exitComboMode();
                comboModeActive = true;
                comboDialIndex = dialIndex;
                comboSelectedIndex = juce::roundToInt (binding.getValue());

                auto img = renderer.renderLcdZoneComboMode (binding, comboSelectedIndex);
                device.setLcdZoneImage (dialIndex, img);
            }
        }
    }

    void handleDialReleased (int /*dialIndex*/)
    {
        // Currently unused — could be used for momentary dial press behaviors
    }

    void handleConnectionChanged (bool connected)
    {
        DBG ("StreamDeckManager: connection " + juce::String (connected ? "established" : "lost"));

        if (connected)
        {
            device.setBrightness (80);
            refreshCurrentPage();
        }
    }

    //==========================================================================
    // Timer: Periodic LCD Refresh
    //==========================================================================

    void timerCallback() override
    {
        if (! device.isConnected() || isUpdatingFromController)
            return;

        // Refresh LCD zones with current parameter values (catches external changes)
        auto* page = getCurrentPage();
        if (page == nullptr)
            return;

        const auto& section = page->getActiveSection();
        for (int i = 0; i < 4; ++i)
        {
            if (comboModeActive && comboDialIndex == i)
                continue;  // Don't overwrite combo mode display

            if (section.dials[i].isValid())
            {
                auto img = renderer.renderLcdZone (section.dials[i]);
                device.setLcdZoneImage (i, img);
            }
        }
    }

    //==========================================================================
    // Page Switching
    //==========================================================================

    void switchToCurrentPage()
    {
        if (onPageNeedsRebuild)
            onPageNeedsRebuild (currentMainTab, currentSubTab, currentChannel);

        auto* page = getCurrentPage();
        if (page != nullptr && device.isConnected())
        {
            renderer.renderAndSendFullPage (device, *page);
        }
        else if (device.isConnected())
        {
            // No page registered for this tab/subtab — clear display
            device.clearAllButtons();
            device.clearLcdStrip();
        }
    }

    //==========================================================================
    // ComboBox Mode
    //==========================================================================

    void exitComboMode()
    {
        comboModeActive = false;
        comboDialIndex = -1;
        comboSelectedIndex = 0;
    }

    //==========================================================================
    // Helpers
    //==========================================================================

    static int makePageKey (int mainTab, int subTab)
    {
        return mainTab * 100 + subTab;
    }

    //==========================================================================
    // Member Data
    //==========================================================================

    StreamDeckDevice device;
    StreamDeckRenderer renderer;
    std::map<int, StreamDeckPage> pages;

    int currentMainTab = 0;
    int currentSubTab = 0;
    int currentChannel = 0;

    // ComboBox interaction state
    bool comboModeActive = false;
    int comboDialIndex = -1;
    int comboSelectedIndex = 0;

    // Guard flag to prevent feedback loops during controller→parameter updates
    bool isUpdatingFromController = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StreamDeckManager)
};
