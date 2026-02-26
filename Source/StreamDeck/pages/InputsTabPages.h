#pragma once

/**
 * InputsTabPages — Stream Deck+ page definitions for the Inputs tab.
 *
 * Creates StreamDeckPage objects for each subtab of the Inputs tab.
 *
 * Subtabs:
 *   0: Input Parameters  (Attenuation & Delay, Position & Directivity, Position Advanced)
 *   1: Live Source & Hackoustics
 *   2: Movements (LFO, AutomOtion)
 *   3: Visualisation (display-only, minimal controls)
 */

#include "../StreamDeckPage.h"
#include "../../Parameters/WFSValueTreeState.h"
#include "../../Parameters/WFSParameterIDs.h"
#include "../../Parameters/WFSParameterDefaults.h"
#include "../../Localization/LocalizationManager.h"

namespace InputsTabPages
{

//==============================================================================
// Helper: create a toggle button binding for an integer parameter (0/1)
//==============================================================================

inline ButtonBinding makeToggleButton (const juce::String& label,
                                        juce::Colour offColour,
                                        juce::Colour onColour,
                                        WFSValueTreeState& state,
                                        int ch,
                                        const juce::Identifier& paramId,
                                        bool rebuildsPage = false)
{
    ButtonBinding btn;
    btn.label = label;
    btn.colour = offColour;
    btn.activeColour = onColour;
    btn.type = ButtonBinding::Toggle;
    btn.requestsPageRebuild = rebuildsPage;

    btn.getState = [&state, ch, paramId]()
    {
        return static_cast<int> (state.getInputParameter (ch, paramId)) != 0;
    };

    btn.onPress = [&state, ch, paramId]()
    {
        int current = static_cast<int> (state.getInputParameter (ch, paramId));
        state.setInputParameter (ch, paramId, current != 0 ? 0 : 1);
    };

    return btn;
}

//==============================================================================
// Helper: create a float dial binding
//==============================================================================

inline DialBinding makeFloatDial (const juce::String& name,
                                   const juce::String& unit,
                                   float minVal, float maxVal,
                                   float stepVal, float fineVal,
                                   int decimals,
                                   bool exponential,
                                   WFSValueTreeState& state,
                                   int ch,
                                   const juce::Identifier& paramId)
{
    DialBinding dial;
    dial.paramName = name;
    dial.paramUnit = unit;
    dial.minValue = minVal;
    dial.maxValue = maxVal;
    dial.step = stepVal;
    dial.fineStep = fineVal;
    dial.decimalPlaces = decimals;
    dial.isExponential = exponential;
    dial.type = DialBinding::Float;

    dial.getValue = [&state, ch, paramId]()
    {
        return static_cast<float> (state.getInputParameter (ch, paramId));
    };

    dial.setValue = [&state, ch, paramId] (float v)
    {
        state.setInputParameter (ch, paramId, v);
    };

    return dial;
}

//==============================================================================
// Helper: create an integer dial binding
//==============================================================================

inline DialBinding makeIntDial (const juce::String& name,
                                 const juce::String& unit,
                                 int minVal, int maxVal,
                                 int stepVal, int fineVal,
                                 WFSValueTreeState& state,
                                 int ch,
                                 const juce::Identifier& paramId)
{
    DialBinding dial;
    dial.paramName = name;
    dial.paramUnit = unit;
    dial.minValue = static_cast<float> (minVal);
    dial.maxValue = static_cast<float> (maxVal);
    dial.step = static_cast<float> (stepVal);
    dial.fineStep = static_cast<float> (fineVal);
    dial.decimalPlaces = 0;
    dial.isExponential = false;
    dial.type = DialBinding::Int;

    dial.getValue = [&state, ch, paramId]()
    {
        return static_cast<float> (static_cast<int> (state.getInputParameter (ch, paramId)));
    };

    dial.setValue = [&state, ch, paramId] (float v)
    {
        state.setInputParameter (ch, paramId, juce::roundToInt (v));
    };

    return dial;
}

//==============================================================================
// Subtab 0: Input Parameters
//==============================================================================

inline StreamDeckPage createInputParametersPage (WFSValueTreeState& state,
                                                  int ch,
                                                  std::shared_ptr<bool> flipMode)
{
    using namespace WFSParameterIDs;
    using namespace WFSParameterDefaults;

    StreamDeckPage page ("Inputs > Parameters");

    // --- Section 0: Attenuation & Delay ---
    {
        auto& sec = page.sections[0];
        sec.sectionName = LOC ("streamDeck.inputs.sections.attenuationAndDelay");
        sec.sectionColour = juce::Colour (0xFF4A90D9);  // Blue

        // Bottom buttons: [empty] | Minimal Delay | Atten Law | [empty]
        sec.buttons[1] = makeToggleButton (LOC ("streamDeck.inputs.buttons.minimalDelay"),
                                            juce::Colour (0xFF3A3A3A), juce::Colour (0xFF4A90D9),
                                            state, ch, inputMinimalLatency);

        sec.buttons[2] = makeToggleButton (LOC ("streamDeck.inputs.buttons.attenuationLaw"),
                                            juce::Colour (0xFF3A3A3A), juce::Colour (0xFFC9A94E),
                                            state, ch, inputAttenuationLaw, true);  // rebuilds page

        // Dials: Attenuation | Delay/Latency | DistAtten or Ratio | Common Atten
        sec.dials[0] = makeFloatDial (LOC ("streamDeck.inputs.dials.attenuation"), LOC ("units.decibels"),
                                       inputAttenuationMin, inputAttenuationMax,
                                       1.0f, 0.25f, 1, false,
                                       state, ch, inputAttenuation);

        sec.dials[1] = makeFloatDial (LOC ("streamDeck.inputs.dials.delay"), LOC ("units.milliseconds"),
                                       inputDelayLatencyMin, inputDelayLatencyMax,
                                       2.0f, 0.5f, 1, false,
                                       state, ch, inputDelayLatency);

        // Dynamic label: "Delay" for values >= 0, "Latency" for values < 0
        sec.dials[1].getDynamicName = [&state, ch]()
        {
            float v = static_cast<float> (state.getInputParameter (ch, WFSParameterIDs::inputDelayLatency));
            return v >= 0.0f ? LOC ("streamDeck.inputs.dials.delay") : LOC ("streamDeck.inputs.dials.latency");
        };

        // Dial 2: depends on attenuation law
        bool is1OverD = static_cast<int> (state.getInputParameter (ch, inputAttenuationLaw)) != 0;
        if (is1OverD)
        {
            sec.dials[2] = makeFloatDial (LOC ("streamDeck.inputs.dials.ratio"), "x",
                                           inputDistanceRatioMin, inputDistanceRatioMax,
                                           0.02f, 0.005f, 2, true,
                                           state, ch, inputDistanceRatio);
        }
        else
        {
            sec.dials[2] = makeFloatDial (LOC ("streamDeck.inputs.dials.distanceAttenuation"), LOC ("units.decibelPerMeter"),
                                           inputDistanceAttenuationMin, inputDistanceAttenuationMax,
                                           0.1f, 0.02f, 2, false,
                                           state, ch, inputDistanceAttenuation);
        }

        sec.dials[3] = makeIntDial (LOC ("streamDeck.inputs.dials.commonAttenuation"), LOC ("units.percent"),
                                     inputCommonAttenMin, inputCommonAttenMax,
                                     2, 1,
                                     state, ch, inputCommonAtten);
    }

    // --- Section 1: Position & Directivity ---
    {
        auto& sec = page.sections[1];
        sec.sectionName = LOC ("streamDeck.inputs.sections.positionAndDirectivity");
        sec.sectionColour = juce::Colour (0xFF5BBFBA);  // Teal

        bool isFlip = flipMode && *flipMode;

        // Bottom buttons: Constraint/Flip meta | X | Y | Z
        {
            auto& btn = sec.buttons[0];
            btn.label = isFlip ? LOC ("streamDeck.inputs.buttons.flip") : LOC ("streamDeck.inputs.buttons.constraint");
            btn.colour = juce::Colour (0xFF3A3A3A);
            btn.activeColour = juce::Colour (0xFF9B6FC3);  // Violet
            btn.type = ButtonBinding::Toggle;
            btn.requestsPageRebuild = true;

            btn.getState = [flipMode]() { return flipMode && *flipMode; };
            btn.onPress = [flipMode]()
            {
                if (flipMode)
                    *flipMode = ! *flipMode;
            };
        }

        if (isFlip)
        {
            sec.buttons[1] = makeToggleButton (LOC ("streamDeck.inputs.buttons.flipX"),
                                                juce::Colour (0xFF3A3A3A), juce::Colour (0xFF9B6FC3),
                                                state, ch, inputFlipX);
            sec.buttons[2] = makeToggleButton (LOC ("streamDeck.inputs.buttons.flipY"),
                                                juce::Colour (0xFF3A3A3A), juce::Colour (0xFF9B6FC3),
                                                state, ch, inputFlipY);
            sec.buttons[3] = makeToggleButton (LOC ("streamDeck.inputs.buttons.flipZ"),
                                                juce::Colour (0xFF3A3A3A), juce::Colour (0xFF9B6FC3),
                                                state, ch, inputFlipZ);
        }
        else
        {
            sec.buttons[1] = makeToggleButton (LOC ("streamDeck.inputs.buttons.constraintX"),
                                                juce::Colour (0xFF3A3A3A), juce::Colour (0xFF5BBFBA),
                                                state, ch, inputConstraintX);
            sec.buttons[2] = makeToggleButton (LOC ("streamDeck.inputs.buttons.constraintY"),
                                                juce::Colour (0xFF3A3A3A), juce::Colour (0xFF5BBFBA),
                                                state, ch, inputConstraintY);
            sec.buttons[3] = makeToggleButton (LOC ("streamDeck.inputs.buttons.constraintZ"),
                                                juce::Colour (0xFF3A3A3A), juce::Colour (0xFF5BBFBA),
                                                state, ch, inputConstraintZ);
        }

        // Dials: Directivity | Rotation | Tilt | HF Shelf
        sec.dials[0] = makeIntDial (LOC ("streamDeck.inputs.dials.directivity"), "deg",
                                     inputDirectivityMin, inputDirectivityMax,
                                     5, 1,
                                     state, ch, inputDirectivity);

        sec.dials[1] = makeIntDial (LOC ("streamDeck.inputs.dials.rotation"), "deg",
                                     inputRotationMin, inputRotationMax,
                                     5, 1,
                                     state, ch, inputRotation);

        sec.dials[2] = makeIntDial (LOC ("streamDeck.inputs.dials.tilt"), "deg",
                                     inputTiltMin, inputTiltMax,
                                     2, 1,
                                     state, ch, inputTilt);

        sec.dials[3] = makeFloatDial (LOC ("streamDeck.inputs.dials.hfShelf"), LOC ("units.decibels"),
                                       inputHFshelfMin, inputHFshelfMax,
                                       0.5f, 0.1f, 1, false,
                                       state, ch, inputHFshelf);
    }

    // --- Section 2: Position Advanced ---
    {
        auto& sec = page.sections[2];
        sec.sectionName = LOC ("streamDeck.inputs.sections.positionAdvanced");
        sec.sectionColour = juce::Colour (0xFFC9A94E);  // Yellow

        // Bottom buttons: Sideline | Tracking | Max Speed | [empty]
        sec.buttons[0] = makeToggleButton (LOC ("streamDeck.inputs.buttons.sideline"),
                                            juce::Colour (0xFF3A3A3A), juce::Colour (0xFFC9A94E),
                                            state, ch, inputSidelinesActive);

        sec.buttons[1] = makeToggleButton (LOC ("streamDeck.inputs.buttons.tracking"),
                                            juce::Colour (0xFF3A3A3A), juce::Colour (0xFFC9A94E),
                                            state, ch, inputTrackingActive);

        sec.buttons[2] = makeToggleButton (LOC ("streamDeck.inputs.buttons.maxSpeed"),
                                            juce::Colour (0xFF3A3A3A), juce::Colour (0xFFC9A94E),
                                            state, ch, inputMaxSpeedActive);

        // Dials: Fringe | Tracking Smooth | Max Speed | Height Factor
        sec.dials[0] = makeFloatDial (LOC ("streamDeck.inputs.dials.fringe"), LOC ("units.meters"),
                                       inputSidelinesFringeMin, inputSidelinesFringeMax,
                                       0.02f, 0.005f, 2, true,
                                       state, ch, inputSidelinesFringe);

        sec.dials[1] = makeIntDial (LOC ("streamDeck.inputs.dials.trackingSmooth"), LOC ("units.percent"),
                                     inputTrackingSmoothMin, inputTrackingSmoothMax,
                                     2, 1,
                                     state, ch, inputTrackingSmooth);

        sec.dials[2] = makeFloatDial (LOC ("streamDeck.inputs.dials.maxSpeed"), LOC ("units.metersPerSecond"),
                                       inputMaxSpeedMin, inputMaxSpeedMax,
                                       0.02f, 0.005f, 2, true,
                                       state, ch, inputMaxSpeed);

        sec.dials[3] = makeIntDial (LOC ("streamDeck.inputs.dials.heightFactor"), LOC ("units.percent"),
                                     inputHeightFactorMin, inputHeightFactorMax,
                                     2, 1,
                                     state, ch, inputHeightFactor);
    }

    page.numSections = 3;
    page.activeSectionIndex = 0;

    // Button 3 (top row): navigate to Map tab (index 6)
    page.topRowNavigateToTab[3] = 6;
    page.topRowOverrideLabel[3] = LOC ("tabs.map");
    page.topRowOverrideColour[3] = juce::Colour (0xFF7B68EE);  // Medium slate blue

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
    Call this whenever the channel changes to rebind getValue/setValue callbacks.
    @param flipMode  Shared state for Constraint/Flip toggle (subtab 0 only) */
inline StreamDeckPage createPage (int subTabIndex,
                                   WFSValueTreeState& state,
                                   int channelIndex,
                                   std::shared_ptr<bool> flipMode = nullptr)
{
    switch (subTabIndex)
    {
        case 0:  return createInputParametersPage (state, channelIndex, flipMode);
        case 1:  return createLiveSourcePage (state, channelIndex);
        case 2:  return createMovementsPage (state, channelIndex);
        case 3:  return createVisualisationPage (state, channelIndex);
        default: return StreamDeckPage ("Inputs > Unknown");
    }
}

} // namespace InputsTabPages
