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

inline StreamDeckPage createLiveSourcePage (WFSValueTreeState& state,
                                             int channelIndex)
{
    using namespace WFSParameterIDs;
    using namespace WFSParameterDefaults;
    int ch = channelIndex;

    StreamDeckPage page ("Inputs > Live Source");

    const auto offGrey   = juce::Colour (0xFF555555);
    const auto onGreen   = juce::Colour (0xFF4CAF50);

    //======================================================================
    // Section 0: Live Source Basics
    //======================================================================
    {
        auto& sec = page.sections[0];
        sec.sectionName   = LOC ("streamDeck.inputs.sections.liveSourceBasics");
        sec.sectionColour = juce::Colour (0xFFE07B39);

        // Button 0 — Toggle Live Source Tamer
        sec.buttons[0] = makeToggleButton (
            LOC ("streamDeck.inputs.buttons.liveSourceTamer"),
            offGrey, onGreen, state, ch, inputLSactive);

        // Dial 0 — Shape (ComboBox)
        {
            DialBinding dial;
            dial.paramName = LOC ("streamDeck.inputs.dials.shape");
            dial.type = DialBinding::ComboBox;
            dial.comboOptions = {
                LOC ("inputs.liveSource.linear"),
                LOC ("inputs.liveSource.log"),
                LOC ("streamDeck.inputs.comboOptions.square"),
                LOC ("inputs.liveSource.sine")
            };
            dial.minValue = static_cast<float> (inputLSshapeMin);
            dial.maxValue = static_cast<float> (inputLSshapeMax);

            dial.getValue = [&state, ch]()
            {
                return static_cast<float> (static_cast<int> (state.getInputParameter (ch, inputLSshape)));
            };

            dial.setValue = [&state, ch] (float v)
            {
                state.setInputParameter (ch, inputLSshape, juce::roundToInt (v));
            };

            sec.dials[0] = dial;
        }

        // Dial 1 — Radius
        sec.dials[1] = makeFloatDial (
            LOC ("streamDeck.inputs.dials.radius"),
            LOC ("units.meters"),
            inputLSradiusMin, inputLSradiusMax,
            1.0f, 0.25f, 1, false,
            state, ch, inputLSradius);

        // Dial 2 — Fixed Attenuation
        sec.dials[2] = makeFloatDial (
            LOC ("streamDeck.inputs.dials.fixedAttenuation"),
            LOC ("units.decibels"),
            inputLSattenuationMin, inputLSattenuationMax,
            1.0f, 0.25f, 1, false,
            state, ch, inputLSattenuation);
    }

    //======================================================================
    // Section 1: Live Source Compression
    //======================================================================
    {
        auto& sec = page.sections[1];
        sec.sectionName   = LOC ("streamDeck.inputs.sections.liveSourceCompression");
        sec.sectionColour = juce::Colour (0xFFD94A6B);

        // Dial 0 — Peak Threshold
        sec.dials[0] = makeFloatDial (
            LOC ("streamDeck.inputs.dials.peakThreshold"),
            LOC ("units.decibels"),
            inputLSpeakThresholdMin, inputLSpeakThresholdMax,
            1.0f, 0.25f, 1, false,
            state, ch, inputLSpeakThreshold);

        // Dial 1 — Peak Ratio
        sec.dials[1] = makeFloatDial (
            LOC ("streamDeck.inputs.dials.peakRatio"),
            "",
            inputLSpeakRatioMin, inputLSpeakRatioMax,
            0.5f, 0.1f, 1, false,
            state, ch, inputLSpeakRatio);

        // Dial 2 — Slow Threshold
        sec.dials[2] = makeFloatDial (
            LOC ("streamDeck.inputs.dials.slowThreshold"),
            LOC ("units.decibels"),
            inputLSslowThresholdMin, inputLSslowThresholdMax,
            1.0f, 0.25f, 1, false,
            state, ch, inputLSslowThreshold);

        // Dial 3 — Slow Ratio
        sec.dials[3] = makeFloatDial (
            LOC ("streamDeck.inputs.dials.slowRatio"),
            "",
            inputLSslowRatioMin, inputLSslowRatioMax,
            0.5f, 0.1f, 1, false,
            state, ch, inputLSslowRatio);
    }

    //======================================================================
    // Section 2: Hackoustics
    //======================================================================
    {
        auto& sec = page.sections[2];
        sec.sectionName   = LOC ("streamDeck.inputs.sections.hackoustics");
        sec.sectionColour = juce::Colour (0xFF5BBF68);

        // Button 0 — Toggle Floor Reflections
        sec.buttons[0] = makeToggleButton (
            LOC ("streamDeck.inputs.buttons.floorReflections"),
            offGrey, onGreen, state, ch, inputFRactive);

        // Button 2 — Toggle Low Cut Filter
        sec.buttons[2] = makeToggleButton (
            LOC ("streamDeck.inputs.buttons.lowCutFilter"),
            offGrey, onGreen, state, ch, inputFRlowCutActive);

        // Dial 0 — FR Attenuation
        sec.dials[0] = makeFloatDial (
            LOC ("streamDeck.inputs.dials.frAttenuation"),
            LOC ("units.decibels"),
            inputFRattenuationMin, inputFRattenuationMax,
            1.0f, 0.25f, 1, false,
            state, ch, inputFRattenuation);

        // Dial 1 — FR Diffusion
        sec.dials[1] = makeIntDial (
            LOC ("streamDeck.inputs.dials.frDiffusion"),
            LOC ("units.percent"),
            inputFRdiffusionMin, inputFRdiffusionMax,
            5, 1,
            state, ch, inputFRdiffusion);

        // Dial 2 — Low Cut Frequency (exponential 20–20000 Hz)
        {
            DialBinding dial;
            dial.paramName = LOC ("streamDeck.inputs.dials.lowCutFrequency");
            dial.paramUnit = LOC ("units.hertz");
            dial.minValue = static_cast<float> (inputFRfreqMin);
            dial.maxValue = static_cast<float> (inputFRfreqMax);
            dial.step = 0.02f;
            dial.fineStep = 0.005f;
            dial.decimalPlaces = 0;
            dial.isExponential = true;
            dial.type = DialBinding::Int;

            dial.getValue = [&state, ch]()
            {
                return static_cast<float> (static_cast<int> (state.getInputParameter (ch, inputFRlowCutFreq)));
            };

            dial.setValue = [&state, ch] (float v)
            {
                state.setInputParameter (ch, inputFRlowCutFreq, juce::roundToInt (v));
            };

            sec.dials[2] = dial;
        }
    }

    //======================================================================
    // Section 3: Hackoustics (continued)
    //======================================================================
    {
        auto& sec = page.sections[3];
        sec.sectionName   = LOC ("streamDeck.inputs.sections.hackousticsContinued");
        sec.sectionColour = juce::Colour (0xFF4ABFAD);

        // Button 0 — Toggle High Shelf Filter
        sec.buttons[0] = makeToggleButton (
            LOC ("streamDeck.inputs.buttons.highShelfFilter"),
            offGrey, onGreen, state, ch, inputFRhighShelfActive);

        // Button 3 — Toggle Send to Reverb (inverted: ON = value 0 = not muted)
        {
            ButtonBinding btn;
            btn.label = LOC ("streamDeck.inputs.buttons.sendToReverb");
            btn.colour = offGrey;
            btn.activeColour = onGreen;
            btn.type = ButtonBinding::Toggle;

            btn.getState = [&state, ch]()
            {
                return static_cast<int> (state.getInputParameter (ch, inputMuteReverbSends)) == 0;
            };

            btn.onPress = [&state, ch]()
            {
                int current = static_cast<int> (state.getInputParameter (ch, inputMuteReverbSends));
                state.setInputParameter (ch, inputMuteReverbSends, current != 0 ? 0 : 1);
            };

            sec.buttons[3] = btn;
        }

        // Dial 0 — High Shelf Frequency (exponential 20–20000 Hz)
        {
            DialBinding dial;
            dial.paramName = LOC ("streamDeck.inputs.dials.highShelfFrequency");
            dial.paramUnit = LOC ("units.hertz");
            dial.minValue = static_cast<float> (inputFRfreqMin);
            dial.maxValue = static_cast<float> (inputFRfreqMax);
            dial.step = 0.02f;
            dial.fineStep = 0.005f;
            dial.decimalPlaces = 0;
            dial.isExponential = true;
            dial.type = DialBinding::Int;

            dial.getValue = [&state, ch]()
            {
                return static_cast<float> (static_cast<int> (state.getInputParameter (ch, inputFRhighShelfFreq)));
            };

            dial.setValue = [&state, ch] (float v)
            {
                state.setInputParameter (ch, inputFRhighShelfFreq, juce::roundToInt (v));
            };

            sec.dials[0] = dial;
        }

        // Dial 1 — High Shelf Gain
        sec.dials[1] = makeFloatDial (
            LOC ("streamDeck.inputs.dials.highShelfGain"),
            LOC ("units.decibels"),
            inputFRhighShelfGainMin, inputFRhighShelfGainMax,
            1.0f, 0.25f, 1, false,
            state, ch, inputFRhighShelfGain);

        // Dial 2 — High Shelf Slope
        sec.dials[2] = makeFloatDial (
            LOC ("streamDeck.inputs.dials.highShelfSlope"),
            "",
            inputFRhighShelfSlopeMin, inputFRhighShelfSlopeMax,
            0.1f, 0.01f, 2, false,
            state, ch, inputFRhighShelfSlope);
    }

    page.numSections = 4;
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
