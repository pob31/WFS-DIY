#pragma once

/**
 * QuickKeysManager — High-level orchestrator for Xencelabs Quick Keys integration.
 *
 * Owns the device driver and page registry.
 * Routes UI navigation events (tab/subtab changes) to page switches.
 * Implements traverse/adjust wheel modes:
 *   - Traverse: wheel cycles through parameters, OLED shows name
 *   - Adjust:   wheel changes the focused parameter, OLED shows value
 *
 * Wheel button press toggles between modes.
 */

#include <JuceHeader.h>
#include "../../Network/OSCProtocolTypes.h"
#include "XencelabsDevice.h"
#include "QuickKeysPage.h"

class QuickKeysManager : private juce::Timer
{
public:
    //==========================================================================
    // Construction / Destruction
    //==========================================================================

    QuickKeysManager()
    {
        // Wire device callbacks — but do NOT start monitoring.
        // The device is only started when setEnabled(true) is called.
        // Each input event runs inside an OriginTagScope { Hardware } so
        // page-binding writes are credited to the controller in change
        // records and cross-actor notifications.
        device.onButtonPressed  = [this] (int btn) {
            WFSNetwork::OriginTagScope s { WFSNetwork::OriginTag::Hardware };
            handleButtonPressed (btn);
        };
        device.onButtonReleased = [this] (int btn) {
            WFSNetwork::OriginTagScope s { WFSNetwork::OriginTag::Hardware };
            handleButtonReleased (btn);
        };
        device.onWheelRotated   = [this] (int dir) {
            WFSNetwork::OriginTagScope s { WFSNetwork::OriginTag::Hardware };
            handleWheelRotated (dir);
        };
        device.onConnectionChanged = [this] (bool connected) { handleConnectionChanged (connected); };
    }

    ~QuickKeysManager() override
    {
        stopTimer();
        device.stopMonitoring();
    }

    //==========================================================================
    // Enable / Disable
    //==========================================================================

    void setEnabled (bool shouldBeEnabled)
    {
        if (shouldBeEnabled)
        {
            device.startMonitoring();
            startTimer (100);
        }
        else
        {
            stopTimer();
            device.stopMonitoring();
            isAdjustMode = false;
        }
    }

    //==========================================================================
    // Page Registration
    //==========================================================================

    void registerPage (int mainTabIndex, int subTabIndex, QuickKeysPage page)
    {
        auto key = makePageKey (mainTabIndex, subTabIndex);
        pages[key] = std::move (page);
    }

    bool hasPage (int mainTabIndex, int subTabIndex) const
    {
        return pages.count (makePageKey (mainTabIndex, subTabIndex)) > 0;
    }

    //==========================================================================
    // Navigation
    //==========================================================================

    /** Called when the main tab or subtab changes in the GUI. */
    void setActivePage (int mainTabIndex, int subTabIndex)
    {
        auto key = makePageKey (mainTabIndex, subTabIndex);
        if (key == currentPageKey)
            return;

        currentPageKey = key;
        isAdjustMode = false;

        updateDisplay();
    }

    //==========================================================================
    // Callbacks (set by MainComponent)
    //==========================================================================

    /** Request main tab switch (e.g., from button press). */
    std::function<void (int tabIndex)> onRequestMainTabChange;

    /** Request page rebuild (e.g., when channel/subtab changes). */
    std::function<void (int mainTab, int subTab)> onPageNeedsRebuild;

    //==========================================================================
    // Constants
    //==========================================================================

    /** Wheel button index — the Quick Keys wheel press is reported as bit 9
        in the 16-bit bitmask (byte 2 bits 0-7 = side keys, byte 3 bit 1 = wheel). */
    static constexpr int WHEEL_BUTTON_INDEX = 9;

private:
    //==========================================================================
    // Event Handlers
    //==========================================================================

    void handleButtonPressed (int buttonIndex)
    {
        if (buttonIndex == WHEEL_BUTTON_INDEX)
        {
            // Toggle traverse ↔ adjust mode
            isAdjustMode = ! isAdjustMode;
            traverseAccumulator = 0;
            updateDisplay();
            return;
        }

        // 8 side buttons — reserved for future assignment
        // buttonIndex 0-7
    }

    void handleButtonReleased (int /*buttonIndex*/)
    {
        // Reserved for future assignment
    }

    void handleWheelRotated (int direction)
    {
        auto* page = getActivePage();
        if (page == nullptr)
            return;

        if (isAdjustMode)
        {
            // Adjust the focused parameter
            auto* binding = page->getFocused();
            if (binding != nullptr && binding->dial.isValid())
            {
                float newVal = binding->dial.applyStep (direction);
                binding->dial.setValue (newVal);
                updateDisplay();
            }
        }
        else
        {
            // Traverse: accumulate wheel clicks, advance after threshold
            traverseAccumulator += direction;

            if (std::abs (traverseAccumulator) >= traverseThreshold)
            {
                if (traverseAccumulator > 0)
                    page->focusNext();
                else
                    page->focusPrev();

                traverseAccumulator = 0;
                updateDisplay();
            }
        }
    }

    void handleConnectionChanged (bool connected)
    {
        DBG ("QuickKeys " + juce::String (connected ? "connected" : "disconnected"));

        if (connected)
        {
            // All writes are queued and processed on the background thread,
            // so these calls are non-blocking.
            device.setDisplayOrientation (XencelabsDevice::Rotate0);
            device.setDisplayBrightness (XencelabsDevice::BrightnessMedium);
            device.setWheelSpeed (XencelabsDevice::Slower);
            updateDisplay();
        }
    }

    //==========================================================================
    // Display
    //==========================================================================

    void updateDisplay()
    {
        if (! device.isConnected())
            return;

        auto* page = getActivePage();
        if (page == nullptr)
        {
            DBG ("QuickKeys updateDisplay: no active page (key=" + juce::String (currentPageKey) + ")");
            device.setWheelColor (40, 40, 40);
            return;
        }

        auto* binding = page->getFocused();
        if (binding == nullptr)
        {
            device.setWheelColor (40, 40, 40);
            return;
        }

        // LED ring: full brightness in adjust mode, dim in traverse mode
        auto c = binding->ledColour;
        if (isAdjustMode)
        {
            device.setWheelColor (c.getRed(), c.getGreen(), c.getBlue());
            lastDisplayedValue = binding->dial.getValue();
        }
        else
        {
            device.setWheelColor (
                static_cast<uint8_t> (c.getRed()   * 0.3f),
                static_cast<uint8_t> (c.getGreen() * 0.3f),
                static_cast<uint8_t> (c.getBlue()  * 0.3f));
            lastDisplayedValue = -999.0f;
        }

        // Display using key text labels across both rows for centered appearance
        // Top row:    [tab]  [section]  [param]  [value]
        // Bottom row: cleared
        device.setKeyText (0, page->tabName);
        device.setKeyText (1, page->sectionName);
        device.setKeyText (2, binding->dial.getDisplayName());
        device.setKeyText (3, isAdjustMode ? binding->dial.formatValueWithUnit() : "");
        // Clear bottom row
        device.setKeyText (4, "");
        device.setKeyText (5, "");
        device.setKeyText (6, "");
        device.setKeyText (7, "");
    }

    //==========================================================================
    // Timer: Refresh OLED in adjust mode
    //==========================================================================

    void timerCallback() override
    {
        // Only check for external value changes in adjust mode (no HID writes from timer in traverse)
        if (! isAdjustMode || ! device.isConnected())
            return;

        auto* page = getActivePage();
        if (page == nullptr) return;

        auto* binding = page->getFocused();
        if (binding == nullptr || ! binding->dial.isValid()) return;

        float currentVal = binding->dial.getValue();
        if (std::abs (currentVal - lastDisplayedValue) > 0.001f)
        {
            lastDisplayedValue = currentVal;
            updateDisplay();
        }
    }

    //==========================================================================
    // Utility
    //==========================================================================

    static int makePageKey (int mainTab, int subTab)
    {
        return mainTab * 100 + subTab;
    }

    QuickKeysPage* getActivePage()
    {
        auto it = pages.find (currentPageKey);
        return (it != pages.end()) ? &it->second : nullptr;
    }

    //==========================================================================
    // Member Data
    //==========================================================================

    XencelabsDevice device;
    std::map<int, QuickKeysPage> pages;

    int currentPageKey = -1;
    bool isAdjustMode = false;

    int traverseAccumulator = 0;
    static constexpr int traverseThreshold = 2;  // Number of wheel clicks needed to advance
    float lastDisplayedValue = -999.0f;           // Track for dirty-check in timer

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (QuickKeysManager)
};
