#pragma once

/**
 * NetworkTabPages — Stream Deck+ page definitions for the Network tab.
 *
 * Creates a StreamDeckPage for the Network tab (tab index 1).
 * Top row: navigation buttons to Outputs, Reverb, Inputs, Map.
 * Bottom row: OSC filter toggle, Open Log Window, (unassigned), Tracking toggle.
 * No dials — the Network tab has no dial-appropriate parameters.
 */

#include "../StreamDeckPage.h"
#include "../../Parameters/WFSValueTreeState.h"
#include "../../Parameters/WFSParameterIDs.h"
#include "../../Localization/LocalizationManager.h"

namespace NetworkTabPages
{

static constexpr int NETWORK_MAIN_TAB_INDEX = 1;

//==============================================================================
// Callbacks struct — actions that must go through the GUI
//==============================================================================

struct NetworkCallbacks
{
    std::function<void()> toggleOscFilter;
    std::function<void()> toggleTracking;
    std::function<void()> openLogWindow;
};

//==============================================================================
// Helper: read a config param from the ValueTree
//==============================================================================

inline juce::var getConfigParam (WFSValueTreeState& state, const juce::Identifier& paramId)
{
    auto config = state.getConfigState();
    for (int i = 0; i < config.getNumChildren(); ++i)
    {
        auto child = config.getChild (i);
        if (child.hasProperty (paramId))
            return child.getProperty (paramId);
    }
    return {};
}

//==============================================================================
// Network page (single page, no subtabs)
//==============================================================================

inline StreamDeckPage createNetworkPage (WFSValueTreeState& state,
                                          const NetworkCallbacks& callbacks)
{
    using namespace WFSParameterIDs;

    StreamDeckPage page ("Network");

    const auto grey = juce::Colour (0xFF3A3A3A);

    //======================================================================
    // Top row: navigation buttons to other tabs
    //======================================================================

    // Button 0: → Outputs (tab 2)
    page.topRowNavigateToTab[0]     = 2;
    page.topRowOverrideLabel[0]     = LOC ("tabs.outputs");
    page.topRowOverrideColour[0]    = juce::Colour (0xFF4A90D9);   // blue

    // Button 1: → Reverb (tab 3)
    page.topRowNavigateToTab[1]     = 3;
    page.topRowOverrideLabel[1]     = LOC ("tabs.reverb");
    page.topRowOverrideColour[1]    = juce::Colour (0xFF9B6FC3);   // purple

    // Button 2: → Inputs (tab 4)
    page.topRowNavigateToTab[2]     = 4;
    page.topRowOverrideLabel[2]     = LOC ("tabs.inputs");
    page.topRowOverrideColour[2]    = juce::Colour (0xFF26A69A);   // teal

    // Button 3: → Map (tab 6)
    page.topRowNavigateToTab[3]     = 6;
    page.topRowOverrideLabel[3]     = LOC ("tabs.map");
    page.topRowOverrideColour[3]    = juce::Colour (0xFF7B68EE);   // medium slate blue

    //======================================================================
    // Single section: Network utility controls
    //======================================================================
    {
        auto& sec = page.sections[0];
        sec.sectionName   = LOC ("tabs.network");
        sec.sectionColour = juce::Colour (0xFF4A90D9);

        //------------------------------------------------------------------
        // Button 0: OSC Filter toggle (dynamic label)
        //------------------------------------------------------------------
        {
            auto& btn = sec.buttons[0];
            btn.colour       = grey;
            btn.activeColour = juce::Colour (0xFF4A90D9);
            btn.type         = ButtonBinding::Toggle;

            btn.getState = [&state]()
            {
                return static_cast<int> (getConfigParam (state, networkOscSourceFilter)) != 0;
            };

            btn.getDynamicLabel = [&state]()
            {
                bool filtering = static_cast<int> (getConfigParam (state, networkOscSourceFilter)) != 0;
                return filtering ? LOC ("streamDeck.network.buttons.oscFilterRegistered")
                                : LOC ("streamDeck.network.buttons.oscFilterAll");
            };

            btn.onPress = [callbacks]()
            {
                if (callbacks.toggleOscFilter)
                    callbacks.toggleOscFilter();
            };
        }

        //------------------------------------------------------------------
        // Button 1: Open Log Window (action)
        //------------------------------------------------------------------
        {
            auto& btn = sec.buttons[1];
            btn.label        = LOC ("streamDeck.network.buttons.openLog");
            btn.colour       = grey;
            btn.type         = ButtonBinding::Action;

            btn.onPress = [callbacks]()
            {
                if (callbacks.openLogWindow)
                    callbacks.openLogWindow();
            };
        }

        //------------------------------------------------------------------
        // Button 2: unassigned (leave default / invalid)
        //------------------------------------------------------------------

        //------------------------------------------------------------------
        // Button 3: Tracking toggle (dynamic label)
        //------------------------------------------------------------------
        {
            auto& btn = sec.buttons[3];
            btn.colour       = grey;
            btn.activeColour = juce::Colour (0xFF2ECC71);
            btn.type         = ButtonBinding::Toggle;

            btn.getState = [&state]()
            {
                return static_cast<int> (getConfigParam (state, trackingEnabled)) != 0;
            };

            btn.getDynamicLabel = [&state]()
            {
                bool isOn = static_cast<int> (getConfigParam (state, trackingEnabled)) != 0;
                return isOn ? LOC ("streamDeck.network.buttons.trackingOn")
                            : LOC ("streamDeck.network.buttons.trackingOff");
            };

            btn.onPress = [callbacks]()
            {
                if (callbacks.toggleTracking)
                    callbacks.toggleTracking();
            };
        }

        //------------------------------------------------------------------
        // No dials on this page
        //------------------------------------------------------------------
    }

    page.numSections = 1;
    page.activeSectionIndex = 0;

    return page;
}

//==============================================================================
// Factory
//==============================================================================

inline StreamDeckPage createPage (int /*subTabIndex*/,
                                   WFSValueTreeState& state,
                                   const NetworkCallbacks& callbacks)
{
    return createNetworkPage (state, callbacks);
}

} // namespace NetworkTabPages
