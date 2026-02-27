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

/** Callbacks for AutomOtion transport controls, wired by MainComponent. */
struct MovementCallbacks
{
    std::function<void (int)> startMotion;    // channelIndex (0-based)
    std::function<void (int)> stopMotion;     // channelIndex (0-based)
    std::function<void (int)> pauseMotion;    // channelIndex (0-based)
    std::function<void (int)> resumeMotion;   // channelIndex (0-based)
    std::function<void()>     stopAll;
};

inline StreamDeckPage createMovementsPage (WFSValueTreeState& state,
                                            int ch,
                                            std::shared_ptr<int> lfoSubMode,
                                            MovementCallbacks movCB = {})
{
    using namespace WFSParameterIDs;
    using namespace WFSParameterDefaults;

    StreamDeckPage page ("Inputs > Movements");

    const auto offGrey = juce::Colour (0xFF3A3A3A);
    const auto onGreen = juce::Colour (0xFF5BBF68);

    //======================================================================
    // Section 0: LFO (with sub-mode switching via bottom buttons)
    //======================================================================
    {
        auto& sec = page.sections[0];
        sec.sectionName   = LOC ("streamDeck.inputs.sections.lfo");
        sec.sectionColour = juce::Colour (0xFF6A5ACD);

        int currentSubMode = lfoSubMode ? *lfoSubMode : 0;

        // --- Bottom buttons: sub-mode selectors (LFO & Jitter / X / Y / Z) ---
        const juce::String btnLocKeys[] = {
            "streamDeck.inputs.buttons.lfoAndJitter",
            "streamDeck.inputs.buttons.lfoX",
            "streamDeck.inputs.buttons.lfoY",
            "streamDeck.inputs.buttons.lfoZ"
        };

        for (int i = 0; i < 4; ++i)
        {
            auto& btn   = sec.buttons[i];
            btn.label   = LOC (btnLocKeys[i]);
            btn.type    = ButtonBinding::Toggle;
            btn.colour  = offGrey;
            btn.activeColour = juce::Colour (0xFF6A5ACD);
            btn.requestsPageRebuild = true;

            int idx = i;
            btn.getState = [lfoSubMode, idx]() { return lfoSubMode && *lfoSubMode == idx; };
            btn.onPress  = [lfoSubMode, idx]() { if (lfoSubMode) *lfoSubMode = idx; };
        }

        // --- Dials: depend on the active sub-mode ---
        if (currentSubMode == 0)
        {
            // Sub-mode 0: LFO & Jitter — Period, Phase, Gyrophone, Jitter
            sec.dials[0] = makeFloatDial (LOC ("streamDeck.inputs.dials.lfoPeriod"),
                                           LOC ("units.seconds"),
                                           inputLFOperiodMin, inputLFOperiodMax,
                                           0.02f, 0.005f, 2, true,
                                           state, ch, inputLFOperiod);

            sec.dials[1] = makeIntDial (LOC ("streamDeck.inputs.dials.lfoPhase"),
                                         LOC ("units.degrees"),
                                         inputLFOphaseMin, inputLFOphaseMax,
                                         5, 1, state, ch, inputLFOphase);

            // Gyrophone: ComboBox, param values -1/0/1 mapped to index 0/1/2
            {
                DialBinding gyroDial;
                gyroDial.paramName = LOC ("streamDeck.inputs.dials.gyrophone");
                gyroDial.type      = DialBinding::ComboBox;
                gyroDial.comboOptions = {
                    LOC ("inputs.lfo.gyrophone.antiClockwise"),
                    LOC ("inputs.lfo.gyrophone.off"),
                    LOC ("inputs.lfo.gyrophone.clockwise")
                };
                gyroDial.minValue = 0.0f;
                gyroDial.maxValue = 2.0f;

                gyroDial.getValue = [&state, ch]()
                {
                    int v = static_cast<int> (state.getInputParameter (ch, inputLFOgyrophone));
                    return static_cast<float> (v + 1);  // -1->0, 0->1, 1->2
                };
                gyroDial.setValue = [&state, ch] (float v)
                {
                    state.setInputParameter (ch, inputLFOgyrophone, juce::roundToInt (v) - 1);
                };
                sec.dials[2] = std::move (gyroDial);
            }

            sec.dials[3] = makeFloatDial (LOC ("streamDeck.inputs.dials.jitter"),
                                           LOC ("units.meters"),
                                           inputJitterMin, inputJitterMax,
                                           0.1f, 0.02f, 2, false,
                                           state, ch, inputJitter);
        }
        else
        {
            // Sub-modes 1/2/3: LFO X / Y / Z — Shape, Amplitude, Rate, Phase
            const juce::Identifier shapeIds[]     = { inputLFOshapeX,     inputLFOshapeY,     inputLFOshapeZ };
            const juce::Identifier amplitudeIds[] = { inputLFOamplitudeX, inputLFOamplitudeY, inputLFOamplitudeZ };
            const juce::Identifier rateIds[]      = { inputLFOrateX,      inputLFOrateY,      inputLFOrateZ };
            const juce::Identifier phaseIds[]     = { inputLFOphaseX,     inputLFOphaseY,     inputLFOphaseZ };
            int axis = currentSubMode - 1;  // 0=X, 1=Y, 2=Z

            // Shape ComboBox (9 options, index 0-8)
            {
                DialBinding shapeDial;
                shapeDial.paramName = LOC ("streamDeck.inputs.dials.lfoShape");
                shapeDial.type      = DialBinding::ComboBox;
                shapeDial.comboOptions = {
                    LOC ("inputs.lfo.shapes.off"),      LOC ("inputs.lfo.shapes.sine"),
                    LOC ("inputs.lfo.shapes.square"),   LOC ("inputs.lfo.shapes.sawtooth"),
                    LOC ("inputs.lfo.shapes.triangle"), LOC ("inputs.lfo.shapes.keystone"),
                    LOC ("inputs.lfo.shapes.log"),      LOC ("inputs.lfo.shapes.exp"),
                    LOC ("inputs.lfo.shapes.random")
                };
                shapeDial.minValue = static_cast<float> (inputLFOshapeMin);
                shapeDial.maxValue = static_cast<float> (inputLFOshapeMax);

                auto shapeId = shapeIds[axis];
                shapeDial.getValue = [&state, ch, shapeId]()
                {
                    return static_cast<float> (static_cast<int> (state.getInputParameter (ch, shapeId)));
                };
                shapeDial.setValue = [&state, ch, shapeId] (float v)
                {
                    state.setInputParameter (ch, shapeId, juce::roundToInt (v));
                };
                sec.dials[0] = std::move (shapeDial);
            }

            sec.dials[1] = makeFloatDial (LOC ("streamDeck.inputs.dials.lfoAmplitude"),
                                           LOC ("units.meters"),
                                           inputLFOamplitudeMin, inputLFOamplitudeMax,
                                           0.5f, 0.1f, 1, false,
                                           state, ch, amplitudeIds[axis]);

            sec.dials[2] = makeFloatDial (LOC ("streamDeck.inputs.dials.lfoRate"),
                                           "x",
                                           inputLFOrateMin, inputLFOrateMax,
                                           0.02f, 0.005f, 2, true,
                                           state, ch, rateIds[axis]);

            sec.dials[3] = makeIntDial (LOC ("streamDeck.inputs.dials.lfoAxisPhase"),
                                         LOC ("units.degrees"),
                                         inputLFOphaseMin, inputLFOphaseMax,
                                         5, 1, state, ch, phaseIds[axis]);
        }
    }

    //======================================================================
    // Section 1: AutomOtion Position
    //======================================================================
    {
        auto& sec = page.sections[1];
        sec.sectionName   = LOC ("streamDeck.inputs.sections.automOtionPosition");
        sec.sectionColour = juce::Colour (0xFF26A69A);

        const auto teal = juce::Colour (0xFF26A69A);

        // Button 0: Absolute/Relative toggle
        {
            auto& btn = sec.buttons[0];
            btn.label  = LOC ("streamDeck.inputs.buttons.absRel");
            btn.colour = offGrey;
            btn.activeColour = teal;
            btn.type   = ButtonBinding::Toggle;

            btn.getState = [&state, ch]()
            {
                return static_cast<int> (state.getInputParameter (ch, inputOtomoAbsoluteRelative)) != 0;
            };
            btn.onPress = [&state, ch]()
            {
                int cur = static_cast<int> (state.getInputParameter (ch, inputOtomoAbsoluteRelative));
                state.setInputParameter (ch, inputOtomoAbsoluteRelative, cur != 0 ? 0 : 1);
            };
        }

        // Button 1: Stay/Return toggle
        {
            auto& btn = sec.buttons[1];
            btn.label  = LOC ("streamDeck.inputs.buttons.stayReturn");
            btn.colour = offGrey;
            btn.activeColour = teal;
            btn.type   = ButtonBinding::Toggle;

            btn.getState = [&state, ch]()
            {
                return static_cast<int> (state.getInputParameter (ch, inputOtomoStayReturn)) != 0;
            };
            btn.onPress = [&state, ch]()
            {
                int cur = static_cast<int> (state.getInputParameter (ch, inputOtomoStayReturn));
                state.setInputParameter (ch, inputOtomoStayReturn, cur != 0 ? 0 : 1);
            };
        }

        // Buttons 2-3: unused

        // --- Dials 0-2: coordinate-dependent position ---
        int coordMode = static_cast<int> (state.getInputParameter (ch, inputOtomoCoordinateMode));

        // Dial 0: X / R(cyl) / R(sph)
        {
            const juce::Identifier pids[] = { inputOtomoX, inputOtomoR, inputOtomoRsph };
            const float mins[] = { inputOtomoMin, inputOtomoRMin, inputOtomoRsphMin };
            const float maxs[] = { inputOtomoMax, inputOtomoRMax, inputOtomoRsphMax };

            DialBinding dial;
            dial.type  = DialBinding::Float;
            dial.minValue = mins[coordMode];
            dial.maxValue = maxs[coordMode];
            dial.step      = 0.5f;
            dial.fineStep  = 0.1f;
            dial.decimalPlaces = 2;
            dial.paramUnit = LOC ("units.meters");

            auto pid = pids[coordMode];
            dial.getValue = [&state, ch, pid]() { return static_cast<float> (state.getInputParameter (ch, pid)); };
            dial.setValue = [&state, ch, pid] (float v) { state.setInputParameter (ch, pid, v); };

            dial.getDynamicName = [&state, ch]()
            {
                int m = static_cast<int> (state.getInputParameter (ch, inputOtomoCoordinateMode));
                return (m != 0) ? juce::String ("Radius") : juce::String ("Dest X");
            };
            sec.dials[0] = std::move (dial);
        }

        // Dial 1: Y / Theta(cyl+sph)
        {
            const juce::Identifier pids[] = { inputOtomoY, inputOtomoTheta, inputOtomoTheta };
            const float mins[] = { inputOtomoMin, inputOtomoThetaMin, inputOtomoThetaMin };
            const float maxs[] = { inputOtomoMax, inputOtomoThetaMax, inputOtomoThetaMax };

            DialBinding dial;
            dial.type  = DialBinding::Float;
            dial.minValue = mins[coordMode];
            dial.maxValue = maxs[coordMode];
            dial.step      = (coordMode == 0) ? 0.5f : 10.0f;
            dial.fineStep  = (coordMode == 0) ? 0.1f : 1.0f;
            dial.decimalPlaces = (coordMode == 0) ? 2 : 1;
            dial.paramUnit = (coordMode == 0) ? LOC ("units.meters") : LOC ("units.degrees");

            auto pid = pids[coordMode];
            dial.getValue = [&state, ch, pid]() { return static_cast<float> (state.getInputParameter (ch, pid)); };
            dial.setValue = [&state, ch, pid] (float v) { state.setInputParameter (ch, pid, v); };

            dial.getDynamicName = [&state, ch]()
            {
                int m = static_cast<int> (state.getInputParameter (ch, inputOtomoCoordinateMode));
                return (m != 0) ? juce::String ("Azimuth") : juce::String ("Dest Y");
            };
            sec.dials[1] = std::move (dial);
        }

        // Dial 2: Z(cart+cyl) / Phi(sph)
        {
            const juce::Identifier pids[] = { inputOtomoZ, inputOtomoZ, inputOtomoPhi };
            const float mins[] = { inputOtomoMin, inputOtomoMin, inputOtomoPhiMin };
            const float maxs[] = { inputOtomoMax, inputOtomoMax, inputOtomoPhiMax };

            DialBinding dial;
            dial.type  = DialBinding::Float;
            dial.minValue = mins[coordMode];
            dial.maxValue = maxs[coordMode];
            dial.step      = (coordMode == 2) ? 10.0f : 0.5f;
            dial.fineStep  = (coordMode == 2) ? 1.0f : 0.1f;
            dial.decimalPlaces = (coordMode == 2) ? 1 : 2;
            dial.paramUnit = (coordMode == 2) ? LOC ("units.degrees") : LOC ("units.meters");

            auto pid = pids[coordMode];
            dial.getValue = [&state, ch, pid]() { return static_cast<float> (state.getInputParameter (ch, pid)); };
            dial.setValue = [&state, ch, pid] (float v) { state.setInputParameter (ch, pid, v); };

            dial.getDynamicName = [&state, ch]()
            {
                int m = static_cast<int> (state.getInputParameter (ch, inputOtomoCoordinateMode));
                return (m == 2) ? juce::String ("Elevation") : juce::String ("Dest Z");
            };
            sec.dials[2] = std::move (dial);
        }

        // Dial 3: unused
    }

    //======================================================================
    // Section 2: AutomOtion (continued)
    //======================================================================
    {
        auto& sec = page.sections[2];
        sec.sectionName   = LOC ("streamDeck.inputs.sections.automOtionContinued");
        sec.sectionColour = juce::Colour (0xFFD4A843);

        // Button 3: Manual/Triggered toggle
        sec.buttons[3] = makeToggleButton (
            LOC ("streamDeck.inputs.buttons.manualTriggered"),
            offGrey, juce::Colour (0xFFD4A843), state, ch, inputOtomoTrigger);

        // Dial 0: Duration (exponential 0.1–3600 s)
        sec.dials[0] = makeFloatDial (LOC ("streamDeck.inputs.dials.duration"),
                                       LOC ("units.seconds"),
                                       inputOtomoDurationMin, inputOtomoDurationMax,
                                       0.02f, 0.005f, 1, true,
                                       state, ch, inputOtomoDuration);

        // Dial 1: Curve (-100 to 100 %)
        sec.dials[1] = makeIntDial (LOC ("streamDeck.inputs.dials.curve"),
                                     LOC ("units.percent"),
                                     inputOtomoCurveMin, inputOtomoCurveMax,
                                     2, 1, state, ch, inputOtomoCurve);

        // Dial 2: Speed Profile (0–100 %)
        sec.dials[2] = makeIntDial (LOC ("streamDeck.inputs.dials.speedProfile"),
                                     LOC ("units.percent"),
                                     inputOtomoSpeedProfileMin, inputOtomoSpeedProfileMax,
                                     2, 1, state, ch, inputOtomoSpeedProfile);

        // Dial 3: Trigger Threshold (primary) + Trigger Reset (altBinding on press)
        sec.dials[3] = makeFloatDial (LOC ("streamDeck.inputs.dials.triggerThreshold"),
                                       LOC ("units.decibels"),
                                       inputOtomoThresholdMin, inputOtomoThresholdMax,
                                       1.0f, 0.25f, 1, false,
                                       state, ch, inputOtomoThreshold);

        sec.dials[3].altBinding = std::make_unique<DialBinding> (
            makeFloatDial (LOC ("streamDeck.inputs.dials.triggerReset"),
                           LOC ("units.decibels"),
                           inputOtomoResetMin, inputOtomoResetMax,
                           1.0f, 0.25f, 1, false,
                           state, ch, inputOtomoReset));
    }

    //======================================================================
    // Section 3: AutomOtion Manual (transport controls)
    //======================================================================
    {
        auto& sec = page.sections[3];
        sec.sectionName   = LOC ("streamDeck.inputs.sections.automOtionManual");
        sec.sectionColour = juce::Colour (0xFFE05555);

        // Button 0: Play (Action — starts motion)
        {
            auto& btn = sec.buttons[0];
            btn.label    = juce::String::charToString (0x25B6);  // ▶
            btn.colour   = offGrey;
            btn.fontSize = 70.0f;
            btn.type     = ButtonBinding::Action;
            btn.onPress  = [movCB, ch]() { if (movCB.startMotion) movCB.startMotion (ch); };
        }

        // Button 1: Pause/Resume (Toggle — reads inputOtomoPauseResume)
        {
            auto& btn = sec.buttons[1];
            btn.label    = juce::String::charToString (0x275A) + juce::String::charToString (0x275A);  // ❚❚
            btn.colour   = offGrey;
            btn.fontSize = 70.0f;
            btn.activeColour = juce::Colour (0xFFD4A843);  // gold when paused
            btn.type     = ButtonBinding::Toggle;

            // State: true when PAUSED (param value 0 = paused, 1 = running)
            btn.getState = [&state, ch]()
            {
                return static_cast<int> (state.getInputParameter (ch, inputOtomoPauseResume)) == 0;
            };

            btn.onPress = [&state, ch, movCB]()
            {
                int current = static_cast<int> (state.getInputParameter (ch, inputOtomoPauseResume));
                if (current == 0)
                {
                    // Currently paused -> resume
                    state.setInputParameter (ch, inputOtomoPauseResume, 1);
                    if (movCB.resumeMotion) movCB.resumeMotion (ch);
                }
                else
                {
                    // Currently running -> pause
                    state.setInputParameter (ch, inputOtomoPauseResume, 0);
                    if (movCB.pauseMotion) movCB.pauseMotion (ch);
                }
            };
        }

        // Button 2: Stop (Action — stops motion for this channel)
        {
            auto& btn = sec.buttons[2];
            btn.label    = juce::String::charToString (0x25A0);  // ■
            btn.colour   = offGrey;
            btn.fontSize = 70.0f;
            btn.type     = ButtonBinding::Action;
            btn.onPress  = [movCB, ch]() { if (movCB.stopMotion) movCB.stopMotion (ch); };
        }

        // Button 3: Stop All (Action — stops all motions globally)
        {
            auto& btn = sec.buttons[3];
            btn.label  = LOC ("streamDeck.inputs.buttons.stopAll");
            btn.colour = juce::Colour (0xFFE05555);  // red background always
            btn.type   = ButtonBinding::Action;
            btn.onPress = [movCB]() { if (movCB.stopAll) movCB.stopAll(); };
        }

        // No dials in this section
    }

    page.numSections = 4;
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
    @param flipMode     Shared state for Constraint/Flip toggle (subtab 0 only)
    @param lfoSubMode   Shared state for LFO sub-mode selector (subtab 2 only)
    @param movementCB   Transport callbacks for AutomOtion (subtab 2 only) */
inline StreamDeckPage createPage (int subTabIndex,
                                   WFSValueTreeState& state,
                                   int channelIndex,
                                   std::shared_ptr<bool> flipMode = nullptr,
                                   std::shared_ptr<int> lfoSubMode = nullptr,
                                   MovementCallbacks movementCB = {})
{
    switch (subTabIndex)
    {
        case 0:  return createInputParametersPage (state, channelIndex, flipMode);
        case 1:  return createLiveSourcePage (state, channelIndex);
        case 2:  return createMovementsPage (state, channelIndex, lfoSubMode, movementCB);
        case 3:  return createVisualisationPage (state, channelIndex);
        default: return StreamDeckPage ("Inputs > Unknown");
    }
}

} // namespace InputsTabPages
