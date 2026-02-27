#pragma once

/**
 * SystemConfigTabPages — Stream Deck+ page definitions for the System Config tab.
 *
 * Creates a StreamDeckPage for the System Config tab (tab index 0).
 * Top row: navigation buttons to Outputs, Reverb, Inputs, Map.
 * Bottom row: Audio Patch window, Processing toggle, Binaural toggle, (unassigned).
 * Dials: Listener Distance, Listener Angle, Binaural Level, Binaural Delay.
 */

#include "../StreamDeckPage.h"
#include "../../Parameters/WFSValueTreeState.h"
#include "../../Parameters/WFSParameterIDs.h"
#include "../../Parameters/WFSParameterDefaults.h"
#include "../../Localization/LocalizationManager.h"

// Reuse config-param helper from NetworkTabPages
#include "NetworkTabPages.h"

namespace SystemConfigTabPages
{

static constexpr int SYSCONFIG_MAIN_TAB_INDEX = 0;

//==============================================================================
// Callbacks struct — actions that must go through the GUI
//==============================================================================

struct SysConfigCallbacks
{
    std::function<void()> openAudioPatchWindow;
    std::function<void()> startProcessing;   // start only — never stop from Stream Deck
    std::function<void()> startBinaural;     // start only — never stop from Stream Deck
};

//==============================================================================
// Helper: make a binaural float dial binding
//==============================================================================

inline DialBinding makeBinauralFloatDial (const juce::String& name,
                                           const juce::String& unit,
                                           float minVal, float maxVal,
                                           float stepVal, float fineVal,
                                           int decimals,
                                           WFSValueTreeState& state,
                                           const juce::Identifier& paramId)
{
    DialBinding d;
    d.paramName     = name;
    d.paramUnit     = unit;
    d.minValue      = minVal;
    d.maxValue      = maxVal;
    d.step          = stepVal;
    d.fineStep      = fineVal;
    d.decimalPlaces = decimals;
    d.type          = DialBinding::Float;

    d.getValue = [&state, paramId, minVal]()
    {
        auto bState = state.getBinauralState();
        return static_cast<float> (bState.getProperty (paramId, minVal));
    };

    d.setValue = [&state, paramId] (float v)
    {
        auto bState = state.getBinauralState();
        bState.setProperty (paramId, v, nullptr);
    };

    return d;
}

//==============================================================================
// Helper: make a binaural int dial binding
//==============================================================================

inline DialBinding makeBinauralIntDial (const juce::String& name,
                                         const juce::String& unit,
                                         int minVal, int maxVal,
                                         int stepVal, int fineVal,
                                         WFSValueTreeState& state,
                                         const juce::Identifier& paramId)
{
    DialBinding d;
    d.paramName     = name;
    d.paramUnit     = unit;
    d.minValue      = static_cast<float> (minVal);
    d.maxValue      = static_cast<float> (maxVal);
    d.step          = static_cast<float> (stepVal);
    d.fineStep      = static_cast<float> (fineVal);
    d.decimalPlaces = 0;
    d.type          = DialBinding::Int;

    d.getValue = [&state, paramId, minVal]()
    {
        auto bState = state.getBinauralState();
        return static_cast<float> (static_cast<int> (bState.getProperty (paramId, minVal)));
    };

    d.setValue = [&state, paramId] (float v)
    {
        auto bState = state.getBinauralState();
        bState.setProperty (paramId, juce::roundToInt (v), nullptr);
    };

    return d;
}

//==============================================================================
// System Config page (single page, no subtabs)
//==============================================================================

inline StreamDeckPage createSysConfigPage (WFSValueTreeState& state,
                                            const SysConfigCallbacks& callbacks)
{
    using namespace WFSParameterIDs;
    using namespace WFSParameterDefaults;

    StreamDeckPage page ("System Config");

    const auto grey = juce::Colour (0xFF3A3A3A);

    //======================================================================
    // Top row: navigation buttons to other tabs
    //======================================================================

    // Button 0: → Outputs (tab 2)
    page.topRowNavigateToTab[0]     = 2;
    page.topRowOverrideLabel[0]     = LOC ("tabs.outputs");
    page.topRowOverrideColour[0]    = juce::Colour (0xFF4A90D9);

    // Button 1: → Reverb (tab 3)
    page.topRowNavigateToTab[1]     = 3;
    page.topRowOverrideLabel[1]     = LOC ("tabs.reverb");
    page.topRowOverrideColour[1]    = juce::Colour (0xFF9B6FC3);

    // Button 2: → Inputs (tab 4)
    page.topRowNavigateToTab[2]     = 4;
    page.topRowOverrideLabel[2]     = LOC ("tabs.inputs");
    page.topRowOverrideColour[2]    = juce::Colour (0xFF26A69A);

    // Button 3: → Map (tab 6)
    page.topRowNavigateToTab[3]     = 6;
    page.topRowOverrideLabel[3]     = LOC ("tabs.map");
    page.topRowOverrideColour[3]    = juce::Colour (0xFF7B68EE);

    //======================================================================
    // Single section: System Config controls
    //======================================================================
    {
        auto& sec = page.sections[0];
        sec.sectionName   = LOC ("tabs.systemConfig");
        sec.sectionColour = juce::Colour (0xFF4A90D9);

        //------------------------------------------------------------------
        // Button 0: Open Audio Interface & Patch window
        //------------------------------------------------------------------
        {
            auto& btn = sec.buttons[0];
            btn.label  = LOC ("streamDeck.systemConfig.buttons.audioPatch");
            btn.colour = grey;
            btn.type   = ButtonBinding::Action;

            btn.onPress = [callbacks]()
            {
                if (callbacks.openAudioPatchWindow)
                    callbacks.openAudioPatchWindow();
            };
        }

        //------------------------------------------------------------------
        // Button 1: Start Processing (only visible when OFF)
        //------------------------------------------------------------------
        {
            bool processingOn = static_cast<int> (NetworkTabPages::getConfigParam (state, runDSP)) != 0;
            if (! processingOn)
            {
                auto& btn = sec.buttons[1];
                btn.label  = LOC ("streamDeck.systemConfig.buttons.processingOff");
                btn.colour = grey;
                btn.type   = ButtonBinding::Action;
                btn.requestsPageRebuild = true;

                btn.onPress = [callbacks]()
                {
                    if (callbacks.startProcessing)
                        callbacks.startProcessing();
                };
            }
        }

        //------------------------------------------------------------------
        // Button 2: Start Binaural Renderer (only visible when OFF)
        //------------------------------------------------------------------
        {
            bool binauralOn = state.getBinauralEnabled();
            if (! binauralOn)
            {
                auto& btn = sec.buttons[2];
                btn.label  = LOC ("streamDeck.systemConfig.buttons.binauralOff");
                btn.colour = grey;
                btn.type   = ButtonBinding::Action;
                btn.requestsPageRebuild = true;

                btn.onPress = [callbacks]()
                {
                    if (callbacks.startBinaural)
                        callbacks.startBinaural();
                };
            }
        }

        //------------------------------------------------------------------
        // Button 3: unassigned
        //------------------------------------------------------------------

        //------------------------------------------------------------------
        // Dial 0: Listener Distance (0–10 m)
        //------------------------------------------------------------------
        sec.dials[0] = makeBinauralFloatDial (
            LOC ("streamDeck.systemConfig.dials.listenerDistance"),
            LOC ("units.meters"),
            binauralListenerDistanceMin, binauralListenerDistanceMax,
            0.1f, 0.01f, 2,
            state, binauralListenerDistance);

        //------------------------------------------------------------------
        // Dial 1: Listener Angle (-180–180 degrees)
        //------------------------------------------------------------------
        sec.dials[1] = makeBinauralIntDial (
            LOC ("streamDeck.systemConfig.dials.listenerAngle"),
            LOC ("units.degrees"),
            binauralListenerAngleMin, binauralListenerAngleMax,
            5, 1,
            state, binauralListenerAngle);

        //------------------------------------------------------------------
        // Dial 2: Binaural Level (-40–0 dB)
        //------------------------------------------------------------------
        sec.dials[2] = makeBinauralFloatDial (
            LOC ("streamDeck.systemConfig.dials.binauralLevel"),
            LOC ("units.decibels"),
            binauralAttenuationMin, binauralAttenuationMax,
            0.5f, 0.1f, 1,
            state, binauralAttenuation);

        //------------------------------------------------------------------
        // Dial 3: Binaural Delay (0–100 ms)
        //------------------------------------------------------------------
        sec.dials[3] = makeBinauralFloatDial (
            LOC ("streamDeck.systemConfig.dials.binauralDelay"),
            LOC ("units.milliseconds"),
            binauralDelayMin, binauralDelayMax,
            1.0f, 0.1f, 1,
            state, binauralDelay);
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
                                   const SysConfigCallbacks& callbacks)
{
    return createSysConfigPage (state, callbacks);
}

} // namespace SystemConfigTabPages
