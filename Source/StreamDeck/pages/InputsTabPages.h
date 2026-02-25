#pragma once

/**
 * InputsTabPages — Stream Deck+ page definitions for the Inputs tab.
 *
 * Creates StreamDeckPage objects for each subtab of the Inputs tab.
 * The section/button/dial assignments are stubs to be filled in by the user's
 * mapping document. Each factory function demonstrates the binding pattern.
 *
 * Subtabs:
 *   0: Input Parameters  (Position, Sound, etc.)
 *   1: Live Source & Hackoustics
 *   2: Movements (LFO, AutomOtion)
 *   3: Visualisation (display-only, minimal controls)
 */

#include "../StreamDeckPage.h"
#include "../../Parameters/WFSValueTreeState.h"
#include "../../Parameters/WFSParameterIDs.h"

namespace InputsTabPages
{

//==============================================================================
// Subtab 0: Input Parameters
//==============================================================================

inline StreamDeckPage createInputParametersPage (WFSValueTreeState& /*state*/,
                                                  int /*channelIndex*/)
{
    StreamDeckPage page ("Inputs > Parameters");

    // --- Section 0: TBD ---
    page.sections[0].sectionName = "Section 1";
    page.sections[0].sectionColour = juce::Colour (0xFF4A90D9);  // Blue

    // --- Section 1: TBD ---
    page.sections[1].sectionName = "Section 2";
    page.sections[1].sectionColour = juce::Colour (0xFF5BBFBA);  // Teal

    // --- Section 2: TBD ---
    page.sections[2].sectionName = "Section 3";
    page.sections[2].sectionColour = juce::Colour (0xFFC9A94E);  // Yellow

    // --- Section 3: TBD ---
    page.sections[3].sectionName = "Section 4";
    page.sections[3].sectionColour = juce::Colour (0xFF9B6FC3);  // Violet

    page.numSections = 4;
    page.activeSectionIndex = 0;

    return page;
}

//==============================================================================
// Subtab 1: Live Source & Hackoustics
//==============================================================================

inline StreamDeckPage createLiveSourcePage (WFSValueTreeState& /*state*/,
                                             int /*channelIndex*/)
{
    StreamDeckPage page ("Inputs > Live Source");

    page.sections[0].sectionName = "Section 1";
    page.sections[0].sectionColour = juce::Colour (0xFF4A90D9);

    page.sections[1].sectionName = "Section 2";
    page.sections[1].sectionColour = juce::Colour (0xFF9B6FC3);

    page.numSections = 2;
    page.activeSectionIndex = 0;

    return page;
}

//==============================================================================
// Subtab 2: Movements (LFO + AutomOtion)
//==============================================================================

inline StreamDeckPage createMovementsPage (WFSValueTreeState& /*state*/,
                                            int /*channelIndex*/)
{
    StreamDeckPage page ("Inputs > Movements");

    page.sections[0].sectionName = "Section 1";
    page.sections[0].sectionColour = juce::Colour (0xFFC9A94E);  // Yellow

    page.sections[1].sectionName = "Section 2";
    page.sections[1].sectionColour = juce::Colour (0xFFC9A94E);

    page.numSections = 2;
    page.activeSectionIndex = 0;

    return page;
}

//==============================================================================
// Subtab 3: Visualisation (display-only — minimal/no controls)
//==============================================================================

inline StreamDeckPage createVisualisationPage (WFSValueTreeState& /*state*/,
                                                int /*channelIndex*/)
{
    StreamDeckPage page ("Inputs > Visualisation");

    page.sections[0].sectionName = "View";
    page.sections[0].sectionColour = juce::Colour (0xFF4A90D9);

    page.numSections = 1;
    page.activeSectionIndex = 0;

    return page;
}

//==============================================================================
// Factory: Create and register all Inputs tab pages
//==============================================================================

/** Main tab index for Inputs (0-based position in the tab bar).
    Tab order: 0=SystemConfig, 1=Network, 2=Outputs, 3=Reverb, 4=Inputs, 5=Clusters, 6=Map */
static constexpr int INPUTS_MAIN_TAB_INDEX = 4;

/** Build the page for a given subtab and register it with the manager.
    Call this whenever the channel changes to rebind getValue/setValue callbacks. */
inline StreamDeckPage createPage (int subTabIndex,
                                   WFSValueTreeState& state,
                                   int channelIndex)
{
    switch (subTabIndex)
    {
        case 0:  return createInputParametersPage (state, channelIndex);
        case 1:  return createLiveSourcePage (state, channelIndex);
        case 2:  return createMovementsPage (state, channelIndex);
        case 3:  return createVisualisationPage (state, channelIndex);
        default: return StreamDeckPage ("Inputs > Unknown");
    }
}

} // namespace InputsTabPages
