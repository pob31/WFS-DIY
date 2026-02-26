#pragma once

/**
 * OutputsTabPages — Stream Deck+ page definitions for the Outputs tab.
 *
 * Creates StreamDeckPage objects for the Outputs tab (main tab index 2).
 *
 * Subtabs:
 *   0: Output Parameters  (Parameters + Orientation, with nav to EQ and Map)
 *   1: Output EQ          (6-band parametric EQ with band selection)
 */

#include "../StreamDeckPage.h"
#include "../../Parameters/WFSValueTreeState.h"
#include "../../Parameters/WFSParameterIDs.h"
#include "../../Parameters/WFSParameterDefaults.h"
#include "../../Localization/LocalizationManager.h"

namespace OutputsTabPages
{

//==============================================================================
// Helper: create a toggle button binding for an output integer parameter (0/1)
//==============================================================================

inline ButtonBinding makeOutputToggleButton (const juce::String& label,
                                              juce::Colour offColour,
                                              juce::Colour onColour,
                                              WFSValueTreeState& state,
                                              int ch,
                                              const juce::Identifier& paramId)
{
    ButtonBinding btn;
    btn.label = label;
    btn.colour = offColour;
    btn.activeColour = onColour;
    btn.type = ButtonBinding::Toggle;

    btn.getState = [&state, ch, paramId]()
    {
        return static_cast<int> (state.getOutputParameter (ch, paramId)) != 0;
    };

    btn.onPress = [&state, ch, paramId]()
    {
        int current = static_cast<int> (state.getOutputParameter (ch, paramId));
        state.setOutputParameter (ch, paramId, current != 0 ? 0 : 1);
    };

    return btn;
}

//==============================================================================
// Helper: create a float dial binding for an output parameter
//==============================================================================

inline DialBinding makeOutputFloatDial (const juce::String& name,
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
        return static_cast<float> (state.getOutputParameter (ch, paramId));
    };

    dial.setValue = [&state, ch, paramId] (float v)
    {
        state.setOutputParameter (ch, paramId, v);
    };

    return dial;
}

//==============================================================================
// Helper: create an integer dial binding for an output parameter
//==============================================================================

inline DialBinding makeOutputIntDial (const juce::String& name,
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
        return static_cast<float> (static_cast<int> (state.getOutputParameter (ch, paramId)));
    };

    dial.setValue = [&state, ch, paramId] (float v)
    {
        state.setOutputParameter (ch, paramId, juce::roundToInt (v));
    };

    return dial;
}

//==============================================================================
// Subtab 0: Output Parameters + Orientation
//==============================================================================

inline StreamDeckPage createOutputParametersPage (WFSValueTreeState& state,
                                                    int channelIndex)
{
    using namespace WFSParameterIDs;
    using namespace WFSParameterDefaults;

    int ch = channelIndex;
    StreamDeckPage page ("Outputs > Parameters");

    const auto grey = juce::Colour (0xFF3A3A3A);

    //======================================================================
    // Section 0: Output Parameters
    //======================================================================
    {
        auto& sec = page.sections[0];
        sec.sectionName   = LOC ("streamDeck.outputs.sections.outputParameters");
        sec.sectionColour = juce::Colour (0xFF4A90D9);  // steel blue

        // Button 0: unassigned

        // Button 1: Toggle Minimal Latency
        sec.buttons[1] = makeOutputToggleButton (LOC ("streamDeck.outputs.buttons.minimalLatency"),
                                                  grey, juce::Colour (0xFF4A90D9),
                                                  state, ch, outputMiniLatencyEnable);

        // Button 2: Toggle Live Source Attenuation
        sec.buttons[2] = makeOutputToggleButton (LOC ("streamDeck.outputs.buttons.liveSourceAtten"),
                                                  grey, juce::Colour (0xFF4A90D9),
                                                  state, ch, outputLSattenEnable);

        // Button 3: Toggle Floor Reflections
        sec.buttons[3] = makeOutputToggleButton (LOC ("streamDeck.outputs.buttons.floorReflections"),
                                                  grey, juce::Colour (0xFF4A90D9),
                                                  state, ch, outputFRenable);

        // Dial 0: Attenuation (-92 to 12 dB)
        sec.dials[0] = makeOutputFloatDial (LOC ("streamDeck.outputs.dials.attenuation"),
                                             LOC ("units.decibels"),
                                             outputAttenuationMin, outputAttenuationMax,
                                             0.5f, 0.1f, 1, false,
                                             state, ch, outputAttenuation);

        // Dial 1: Delay/Latency (-100 to 100 ms) with dynamic name
        sec.dials[1] = makeOutputFloatDial (LOC ("streamDeck.outputs.dials.delay"),
                                             LOC ("units.milliseconds"),
                                             outputDelayLatencyMin, outputDelayLatencyMax,
                                             0.5f, 0.1f, 1, false,
                                             state, ch, outputDelayLatency);

        sec.dials[1].getDynamicName = [&state, ch]()
        {
            float v = static_cast<float> (state.getOutputParameter (ch, outputDelayLatency));
            return (v < 0.0f) ? LOC ("streamDeck.outputs.dials.latency")
                              : LOC ("streamDeck.outputs.dials.delay");
        };

        // Dial 2: Distance Attenuation % (0-100%)
        sec.dials[2] = makeOutputIntDial (LOC ("streamDeck.outputs.dials.distanceAttenuation"),
                                           LOC ("units.percent"),
                                           outputDistanceAttenPercentMin, outputDistanceAttenPercentMax,
                                           2, 1,
                                           state, ch, outputDistanceAttenPercent);

        // Dial 3: HF Damping (-24 to 0 dB)
        sec.dials[3] = makeOutputFloatDial (LOC ("streamDeck.outputs.dials.hfDamping"),
                                             LOC ("units.decibels"),
                                             outputHFdampingMin, outputHFdampingMax,
                                             0.5f, 0.1f, 1, false,
                                             state, ch, outputHFdamping);
    }

    //======================================================================
    // Section 1: Output Orientation
    //======================================================================
    {
        auto& sec = page.sections[1];
        sec.sectionName   = LOC ("streamDeck.outputs.sections.outputOrientation");
        sec.sectionColour = juce::Colour (0xFF26A69A);  // teal

        // All 4 buttons unassigned

        // Dial 0: On Angle (0-90 degrees)
        sec.dials[0] = makeOutputIntDial (LOC ("streamDeck.outputs.dials.onAngle"),
                                           LOC ("units.degrees"),
                                           outputAngleOnMin, outputAngleOnMax,
                                           2, 1,
                                           state, ch, outputAngleOn);

        // Dial 1: Off Angle (0-180 degrees)
        sec.dials[1] = makeOutputIntDial (LOC ("streamDeck.outputs.dials.offAngle"),
                                           LOC ("units.degrees"),
                                           outputAngleOffMin, outputAngleOffMax,
                                           2, 1,
                                           state, ch, outputAngleOff);

        // Dial 2: Orientation (-180 to 180 degrees)
        sec.dials[2] = makeOutputIntDial (LOC ("streamDeck.outputs.dials.orientation"),
                                           LOC ("units.degrees"),
                                           outputOrientationMin, outputOrientationMax,
                                           5, 1,
                                           state, ch, outputOrientation);

        // Dial 3: Pitch (-90 to 90 degrees)
        sec.dials[3] = makeOutputIntDial (LOC ("streamDeck.outputs.dials.pitch"),
                                           LOC ("units.degrees"),
                                           outputPitchMin, outputPitchMax,
                                           2, 1,
                                           state, ch, outputPitch);
    }

    page.numSections = 2;
    page.activeSectionIndex = 0;

    // Button 2 (top row): navigate to Output EQ subtab
    page.topRowNavigateToTab[2] = 2;       // same main tab (Outputs)
    page.topRowNavigateToSubTab[2] = 1;    // subtab 1 = EQ
    page.topRowOverrideLabel[2] = LOC ("outputs.tabs.eq");
    page.topRowOverrideColour[2] = juce::Colour (0xFFD4A843);  // gold

    // Button 3 (top row): navigate to Map tab
    page.topRowNavigateToTab[3] = 6;
    page.topRowOverrideLabel[3] = LOC ("tabs.map");
    page.topRowOverrideColour[3] = juce::Colour (0xFF7B68EE);  // medium slate blue

    return page;
}

//==============================================================================
// Subtab 1: Output EQ (6-band parametric)
//==============================================================================

/** Band colours matching EQDisplayComponent::getBandColour (0-5). */
inline juce::Colour getEqBandColour (int band)
{
    static const juce::Colour colours[] = {
        juce::Colour (0xFFE74C3C),  // Band 1: Red
        juce::Colour (0xFFE67E22),  // Band 2: Orange
        juce::Colour (0xFFFFEB3B),  // Band 3: Yellow
        juce::Colour (0xFF2ECC71),  // Band 4: Green
        juce::Colour (0xFF3498DB),  // Band 5: Blue
        juce::Colour (0xFF9B59B6),  // Band 6: Purple
    };
    return colours[juce::jlimit (0, 5, band)];
}

/** Shape combo ↔ shape ID mapping tables.
    GUI combo order: LowCut=1, LowShelf=2, Peak=3, BandPass=4, AllPass=7, HighShelf=5, HighCut=6. */
static constexpr int comboToShape[] = { 1, 2, 3, 4, 7, 5, 6 };
static constexpr int shapeToCombo[] = { 0, 0, 1, 2, 3, 5, 6, 4 };

inline StreamDeckPage createOutputEQPage (WFSValueTreeState& state,
                                            int channelIndex,
                                            std::shared_ptr<int> selectedBand,
                                            std::function<void (int)> onBandSelectedInGui = nullptr)
{
    using namespace WFSParameterIDs;
    using namespace WFSParameterDefaults;

    int ch = channelIndex;
    int band = juce::jlimit (0, 5, *selectedBand);

    StreamDeckPage page ("Outputs > EQ");

    const auto grey = juce::Colour (0xFF3A3A3A);
    juce::Colour bandColour = getEqBandColour (band);

    //======================================================================
    // Helper: create a band-selector ButtonBinding
    //======================================================================
    auto makeBandSelector = [&] (int targetBand) -> ButtonBinding
    {
        ButtonBinding btn;
        btn.label = LOC ("eq.labels.band") + " " + juce::String (targetBand + 1);
        btn.colour = grey;
        btn.activeColour = getEqBandColour (targetBand);
        btn.type = ButtonBinding::Toggle;
        btn.requestsPageRebuild = true;

        btn.getState = [selectedBand, targetBand]()
        {
            return *selectedBand == targetBand;
        };

        btn.onPress = [selectedBand, targetBand, onBandSelectedInGui]()
        {
            *selectedBand = targetBand;
            if (onBandSelectedInGui)
                onBandSelectedInGui (targetBand);
        };

        return btn;
    };

    //======================================================================
    // Top row: EQ toggle + Band 4/5/6 selectors
    //======================================================================

    // Button 0: Global EQ on/off toggle
    {
        auto& btn = page.topRowButtons[0];
        btn.label = LOC ("streamDeck.outputs.eq.buttons.eqOnOff");
        btn.colour = grey;
        btn.activeColour = juce::Colour (0xFF4A90D9);
        btn.type = ButtonBinding::Toggle;

        btn.getState = [&state, ch]()
        {
            return static_cast<int> (state.getOutputParameter (ch, outputEQenabled)) != 0;
        };

        btn.onPress = [&state, ch]()
        {
            int current = static_cast<int> (state.getOutputParameter (ch, outputEQenabled));
            state.setOutputParameter (ch, outputEQenabled, current != 0 ? 0 : 1);
        };
    }

    // Buttons 1-3: Band 4, 5, 6 selectors
    page.topRowButtons[1] = makeBandSelector (3);
    page.topRowButtons[2] = makeBandSelector (4);
    page.topRowButtons[3] = makeBandSelector (5);

    //======================================================================
    // Single section: EQ controls for selected band
    //======================================================================
    {
        auto& sec = page.sections[0];
        sec.sectionName   = LOC ("eq.labels.band") + " " + juce::String (band + 1);
        sec.sectionColour = bandColour;

        //------------------------------------------------------------------
        // Button 0: Toggle selected band on/off
        //------------------------------------------------------------------
        {
            auto& btn = sec.buttons[0];
            btn.label = LOC ("streamDeck.outputs.eq.buttons.bandOnOff");
            btn.colour = grey;
            btn.activeColour = bandColour;
            btn.type = ButtonBinding::Toggle;
            btn.requestsPageRebuild = true;

            btn.getState = [&state, ch, band]()
            {
                auto bt = state.getOutputEQBand (ch, band);
                return static_cast<int> (bt.getProperty (eqShape, 0)) != 0;
            };

            btn.onPress = [&state, ch, band]()
            {
                auto bt = state.getOutputEQBand (ch, band);
                int shape = static_cast<int> (bt.getProperty (eqShape, 0));
                if (shape != 0)
                    state.setOutputEQBandParameterWithArrayPropagation (ch, band, eqShape, 0);
                else
                    state.setOutputEQBandParameterWithArrayPropagation (ch, band, eqShape, eqBandComboDefaults[band]);
            };
        }

        // Buttons 1-3: Band 1, 2, 3 selectors
        sec.buttons[1] = makeBandSelector (0);
        sec.buttons[2] = makeBandSelector (1);
        sec.buttons[3] = makeBandSelector (2);

        //------------------------------------------------------------------
        // Dial 0: Band shape (ComboBox)
        //------------------------------------------------------------------
        {
            auto& dial = sec.dials[0];
            dial.paramName = LOC ("streamDeck.outputs.eq.dials.shape");
            dial.type = DialBinding::ComboBox;

            // 7 shape options in GUI combo order
            dial.comboOptions = {
                LOC ("eq.filterTypes.lowCut"),
                LOC ("eq.filterTypes.lowShelf"),
                LOC ("eq.filterTypes.peakNotch"),
                LOC ("eq.filterTypes.bandPass"),
                LOC ("eq.filterTypes.allPass"),
                LOC ("eq.filterTypes.highShelf"),
                LOC ("eq.filterTypes.highCut")
            };

            dial.minValue = 0.0f;
            dial.maxValue = 6.0f;

            dial.getValue = [&state, ch, band]()
            {
                auto bt = state.getOutputEQBand (ch, band);
                int shape = static_cast<int> (bt.getProperty (eqShape, 0));
                if (shape <= 0 || shape > 7)
                    return static_cast<float> (shapeToCombo[eqBandComboDefaults[band]]);
                return static_cast<float> (shapeToCombo[shape]);
            };

            dial.setValue = [&state, ch, band] (float v)
            {
                int comboIndex = juce::jlimit (0, 6, juce::roundToInt (v));
                state.setOutputEQBandParameterWithArrayPropagation (ch, band, eqShape, comboToShape[comboIndex]);
            };
        }

        //------------------------------------------------------------------
        // Dial 1: Frequency (20-20000 Hz, exponential)
        //------------------------------------------------------------------
        {
            auto& dial = sec.dials[1];
            dial.paramName = LOC ("streamDeck.outputs.eq.dials.frequency");
            dial.paramUnit = LOC ("units.hertz");
            dial.minValue = eqFrequencyMin;
            dial.maxValue = eqFrequencyMax;
            dial.step = 0.02f;
            dial.fineStep = 0.005f;
            dial.decimalPlaces = 0;
            dial.isExponential = true;
            dial.type = DialBinding::Float;

            dial.getValue = [&state, ch, band]()
            {
                auto bt = state.getOutputEQBand (ch, band);
                return static_cast<float> (bt.getProperty (eqFrequency, eqFrequencyDefault));
            };

            dial.setValue = [&state, ch, band] (float v)
            {
                state.setOutputEQBandParameterWithArrayPropagation (ch, band, eqFrequency, v);
            };
        }

        //------------------------------------------------------------------
        // Dial 2: Gain (-24 to 24 dB)
        // getDynamicName returns "—" for shapes without gain control
        //------------------------------------------------------------------
        {
            auto& dial = sec.dials[2];
            dial.paramName = LOC ("streamDeck.outputs.eq.dials.gain");
            dial.paramUnit = LOC ("units.decibels");
            dial.minValue = eqGainMin;
            dial.maxValue = eqGainMax;
            dial.step = 0.5f;
            dial.fineStep = 0.1f;
            dial.decimalPlaces = 1;
            dial.isExponential = false;
            dial.type = DialBinding::Float;

            dial.getValue = [&state, ch, band]()
            {
                auto bt = state.getOutputEQBand (ch, band);
                return static_cast<float> (bt.getProperty (eqGain, eqGainDefault));
            };

            dial.setValue = [&state, ch, band] (float v)
            {
                state.setOutputEQBandParameterWithArrayPropagation (ch, band, eqGain, v);
            };

            // Dynamic name: show "—" for LowCut(1), BandPass(4), HighCut(6), AllPass(7)
            dial.getDynamicName = [&state, ch, band]()
            {
                auto bt = state.getOutputEQBand (ch, band);
                int shape = static_cast<int> (bt.getProperty (eqShape, 0));
                if (shape == 1 || shape == 4 || shape == 6 || shape == 7)
                    return juce::String (juce::CharPointer_UTF8 ("\xe2\x80\x94"));  // em dash
                return LOC ("streamDeck.outputs.eq.dials.gain");
            };
        }

        //------------------------------------------------------------------
        // Dial 3: Q (0.1-10.0, exponential)
        //------------------------------------------------------------------
        {
            auto& dial = sec.dials[3];
            dial.paramName = LOC ("streamDeck.outputs.eq.dials.q");
            dial.minValue = eqQMin;
            dial.maxValue = eqQMax;
            dial.step = 0.02f;
            dial.fineStep = 0.005f;
            dial.decimalPlaces = 2;
            dial.isExponential = true;
            dial.type = DialBinding::Float;

            dial.getValue = [&state, ch, band]()
            {
                auto bt = state.getOutputEQBand (ch, band);
                return static_cast<float> (bt.getProperty (eqQ, eqQDefault));
            };

            dial.setValue = [&state, ch, band] (float v)
            {
                state.setOutputEQBandParameterWithArrayPropagation (ch, band, eqQ, v);
            };
        }
    }

    page.numSections = 1;
    page.activeSectionIndex = 0;

    return page;
}

//==============================================================================
// Factory
//==============================================================================

/** Main tab index for Outputs (0-based position in the tab bar). */
static constexpr int OUTPUTS_MAIN_TAB_INDEX = 2;

/** Create a Stream Deck page for the given Outputs subtab.
    @param subTabIndex   Subtab index (0 = Parameters, 1 = EQ)
    @param state         The shared value tree state
    @param channelIndex  Output channel index (0-based)
    @param selectedBand  Shared EQ band selection state (0-5)
*/
inline StreamDeckPage createPage (int subTabIndex,
                                   WFSValueTreeState& state,
                                   int channelIndex,
                                   std::shared_ptr<int> selectedBand = nullptr,
                                   std::function<void (int)> onBandSelectedInGui = nullptr)
{
    switch (subTabIndex)
    {
        case 0:  return createOutputParametersPage (state, channelIndex);
        case 1:  if (selectedBand != nullptr)
                     return createOutputEQPage (state, channelIndex, selectedBand, onBandSelectedInGui);
                 return StreamDeckPage ("Outputs > EQ");
        default: return StreamDeckPage ("Outputs > Unknown");
    }
}

} // namespace OutputsTabPages
