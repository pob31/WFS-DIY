#pragma once

/**
 * ClustersTabPages — Stream Deck+ page definitions for the Clusters tab.
 *
 * Creates a StreamDeckPage for cluster LFO control with 7 sub-modes:
 *   Top row:    LFO (main) | LFO X | LFO Y | LFO Z
 *   Bottom row: Preset | Rotation | Scale | On/Off (toggle)
 */

#include "../StreamDeckPage.h"
#include "../../Parameters/WFSValueTreeState.h"
#include "../../Parameters/WFSParameterIDs.h"
#include "../../Parameters/WFSParameterDefaults.h"
#include "../../Localization/LocalizationManager.h"

namespace ClustersTabPages
{

static constexpr int CLUSTERS_MAIN_TAB_INDEX = 5;

//==============================================================================
// Callbacks struct — actions that must go through the GUI
//==============================================================================

struct ClusterLFOCallbacks
{
    std::function<void()>    stopAllClusterLFOs;
    std::function<void (int)> storePreset;       // 0-15
    std::function<void (int)> recallPreset;       // 0-15
    std::function<void (int)> recallAndStart;     // 0-15
    std::function<void (int)> highlightPreset;    // 0-15, -1 = clear
};

//==============================================================================
// Helper: cluster float dial (same pattern as InputsTabPages but for clusters)
//==============================================================================

inline DialBinding makeClusterFloatDial (const juce::String& name,
                                          const juce::String& unit,
                                          float minVal, float maxVal,
                                          float stepVal, float fineVal,
                                          int decimals,
                                          bool exponential,
                                          WFSValueTreeState& state,
                                          int cluster,
                                          const juce::Identifier& paramId)
{
    DialBinding dial;
    dial.paramName      = name;
    dial.paramUnit      = unit;
    dial.minValue       = minVal;
    dial.maxValue       = maxVal;
    dial.step           = stepVal;
    dial.fineStep       = fineVal;
    dial.decimalPlaces  = decimals;
    dial.isExponential  = exponential;
    dial.type           = DialBinding::Float;

    dial.getValue = [&state, cluster, paramId]()
    {
        return static_cast<float> (state.getClusterParameter (cluster, paramId));
    };
    dial.setValue = [&state, cluster, paramId] (float v)
    {
        state.setClusterParameter (cluster, paramId, v);
    };
    return dial;
}

//==============================================================================
// Helper: cluster integer dial
//==============================================================================

inline DialBinding makeClusterIntDial (const juce::String& name,
                                        const juce::String& unit,
                                        int minVal, int maxVal,
                                        int stepVal, int fineVal,
                                        WFSValueTreeState& state,
                                        int cluster,
                                        const juce::Identifier& paramId)
{
    DialBinding dial;
    dial.paramName      = name;
    dial.paramUnit      = unit;
    dial.minValue       = static_cast<float> (minVal);
    dial.maxValue       = static_cast<float> (maxVal);
    dial.step           = static_cast<float> (stepVal);
    dial.fineStep       = static_cast<float> (fineVal);
    dial.decimalPlaces  = 0;
    dial.isExponential  = false;
    dial.type           = DialBinding::Int;

    dial.getValue = [&state, cluster, paramId]()
    {
        return static_cast<float> (static_cast<int> (state.getClusterParameter (cluster, paramId)));
    };
    dial.setValue = [&state, cluster, paramId] (float v)
    {
        state.setClusterParameter (cluster, paramId, juce::roundToInt (v));
    };
    return dial;
}

//==============================================================================
// Helper: shape ComboBox dial for a given axis param
//==============================================================================

inline DialBinding makeShapeDial (WFSValueTreeState& state, int cluster,
                                   const juce::Identifier& shapeId)
{
    DialBinding d;
    d.paramName = LOC ("streamDeck.clusters.dials.shape");
    d.type      = DialBinding::ComboBox;
    d.comboOptions = {
        LOC ("inputs.lfo.shapes.off"),      LOC ("inputs.lfo.shapes.sine"),
        LOC ("inputs.lfo.shapes.square"),   LOC ("inputs.lfo.shapes.sawtooth"),
        LOC ("inputs.lfo.shapes.triangle"), LOC ("inputs.lfo.shapes.keystone"),
        LOC ("inputs.lfo.shapes.log"),      LOC ("inputs.lfo.shapes.exp"),
        LOC ("inputs.lfo.shapes.random")
    };
    d.minValue = 0.0f;
    d.maxValue = 8.0f;

    d.getValue = [&state, cluster, shapeId]()
    {
        return static_cast<float> (static_cast<int> (state.getClusterParameter (cluster, shapeId)));
    };
    d.setValue = [&state, cluster, shapeId] (float v)
    {
        state.setClusterParameter (cluster, shapeId, juce::roundToInt (v));
    };
    return d;
}

//==============================================================================
// Page factory
//==============================================================================

inline StreamDeckPage createPage (int /*subTab*/,
                                   WFSValueTreeState& state,
                                   int clusterNum,
                                   std::shared_ptr<int> lfoSubMode,
                                   std::shared_ptr<int> presetCol,
                                   std::shared_ptr<int> presetRow,
                                   ClusterLFOCallbacks cb)
{
    using namespace WFSParameterIDs;
    using namespace WFSParameterDefaults;

    StreamDeckPage page ("Clusters > LFO");

    const auto offGrey = juce::Colour (0xFF3A3A3A);
    const auto violet  = juce::Colour (0xFF6A5ACD);

    int currentSubMode = lfoSubMode ? *lfoSubMode : 0;

    //======================================================================
    // Top row buttons (0-3): LFO | X | Y | Z  — sub-mode selectors
    //======================================================================
    {
        const juce::String btnLocKeys[] = {
            "streamDeck.clusters.buttons.lfo",
            "streamDeck.clusters.buttons.lfoX",
            "streamDeck.clusters.buttons.lfoY",
            "streamDeck.clusters.buttons.lfoZ"
        };

        for (int i = 0; i < 4; ++i)
        {
            auto& btn = page.topRowButtons[i];
            btn.label   = LOC (btnLocKeys[i]);
            btn.type    = ButtonBinding::Toggle;
            btn.colour  = offGrey;
            btn.activeColour = violet;
            btn.requestsPageRebuild = true;

            int idx = i;
            btn.getState = [lfoSubMode, idx]() { return lfoSubMode && *lfoSubMode == idx; };
            btn.onPress  = [lfoSubMode, idx, cb]()
            {
                if (lfoSubMode) *lfoSubMode = idx;
                if (idx != 6 && cb.highlightPreset) cb.highlightPreset (-1);
            };
        }
    }

    //======================================================================
    // Single section (section 0) — bottom buttons + dials
    //======================================================================
    auto& sec = page.sections[0];
    sec.sectionName   = LOC ("streamDeck.clusters.buttons.lfo");
    sec.sectionColour = violet;

    //----------------------------------------------------------------------
    // Bottom buttons (4-7): Preset | Rotation | Scale | On/Off
    //----------------------------------------------------------------------
    {
        // Button 0 — Preset
        auto& btnPreset = sec.buttons[0];
        btnPreset.label  = LOC ("streamDeck.clusters.buttons.preset");
        btnPreset.type   = ButtonBinding::Toggle;
        btnPreset.colour = offGrey;
        btnPreset.activeColour = violet;
        btnPreset.requestsPageRebuild = true;

        btnPreset.getState = [lfoSubMode]() { return lfoSubMode && *lfoSubMode == 6; };
        btnPreset.onPress  = [lfoSubMode, presetCol, presetRow, cb]()
        {
            if (lfoSubMode) *lfoSubMode = 6;
            if (cb.highlightPreset && presetCol && presetRow)
                cb.highlightPreset (*presetRow * 4 + *presetCol);
        };
    }
    {
        // Button 1 — Rotation
        auto& btnRot = sec.buttons[1];
        btnRot.label  = LOC ("streamDeck.clusters.buttons.rotation");
        btnRot.type   = ButtonBinding::Toggle;
        btnRot.colour = offGrey;
        btnRot.activeColour = violet;
        btnRot.requestsPageRebuild = true;

        btnRot.getState = [lfoSubMode]() { return lfoSubMode && *lfoSubMode == 4; };
        btnRot.onPress  = [lfoSubMode, cb]()
        {
            if (lfoSubMode) *lfoSubMode = 4;
            if (cb.highlightPreset) cb.highlightPreset (-1);
        };
    }
    {
        // Button 2 — Scale
        auto& btnScale = sec.buttons[2];
        btnScale.label  = LOC ("streamDeck.clusters.buttons.scale");
        btnScale.type   = ButtonBinding::Toggle;
        btnScale.colour = offGrey;
        btnScale.activeColour = violet;
        btnScale.requestsPageRebuild = true;

        btnScale.getState = [lfoSubMode]() { return lfoSubMode && *lfoSubMode == 5; };
        btnScale.onPress  = [lfoSubMode, cb]()
        {
            if (lfoSubMode) *lfoSubMode = 5;
            if (cb.highlightPreset) cb.highlightPreset (-1);
        };
    }
    {
        // Button 3 — On/Off toggle (does NOT change sub-mode)
        auto& btnOnOff = sec.buttons[3];
        btnOnOff.type    = ButtonBinding::Toggle;
        btnOnOff.colour  = offGrey;
        btnOnOff.activeColour = juce::Colour (0xFF26A69A);  // teal
        btnOnOff.requestsPageRebuild = false;

        btnOnOff.getDynamicLabel = [&state, clusterNum]()
        {
            int active = static_cast<int> (state.getClusterParameter (clusterNum, clusterLFOactive));
            return active != 0 ? LOC ("streamDeck.clusters.buttons.lfoOn")
                               : LOC ("streamDeck.clusters.buttons.lfoOff");
        };
        btnOnOff.getState = [&state, clusterNum]()
        {
            return static_cast<int> (state.getClusterParameter (clusterNum, clusterLFOactive)) != 0;
        };
        btnOnOff.onPress = [&state, clusterNum]()
        {
            int current = static_cast<int> (state.getClusterParameter (clusterNum, clusterLFOactive));
            state.setClusterParameter (clusterNum, clusterLFOactive, current != 0 ? 0 : 1);
        };
    }

    //----------------------------------------------------------------------
    // Dials — depend on currentSubMode
    //----------------------------------------------------------------------

    if (currentSubMode == 0)
    {
        //==================================================================
        // Sub-mode 0: LFO Main — [Stop All] [—] [Period] [Phase]
        //==================================================================

        // Dial 0: Stop All LFO (press only, shows active count)
        {
            DialBinding d;
            d.paramName     = LOC ("streamDeck.clusters.dials.stopAll");
            d.type          = DialBinding::Int;
            d.minValue      = 0.0f;
            d.maxValue      = 10.0f;
            d.step          = 0.0f;
            d.fineStep      = 0.0f;
            d.decimalPlaces = 0;

            d.getValue = [&state]()
            {
                int activeCount = 0;
                for (int c = 1; c <= 10; ++c)
                    if (static_cast<int> (state.getClusterParameter (c, clusterLFOactive)) != 0)
                        ++activeCount;
                return static_cast<float> (activeCount);
            };
            d.setValue = [] (float) {};  // turning does nothing
            d.onPress = [cb]() { if (cb.stopAllClusterLFOs) cb.stopAllClusterLFOs(); };

            sec.dials[0] = std::move (d);
        }

        // Dial 1: empty (not assigned)

        // Dial 2: Period
        sec.dials[2] = makeClusterFloatDial (LOC ("streamDeck.clusters.dials.period"),
                                              LOC ("units.seconds"),
                                              clusterLFOperiodMin, clusterLFOperiodMax,
                                              0.02f, 0.005f, 2, true,
                                              state, clusterNum, clusterLFOperiod);

        // Dial 3: Phase
        sec.dials[3] = makeClusterIntDial (LOC ("streamDeck.clusters.dials.phase"),
                                            LOC ("units.degrees"),
                                            clusterLFOphaseMin, clusterLFOphaseMax,
                                            5, 1, state, clusterNum, clusterLFOphase);
    }
    else if (currentSubMode >= 1 && currentSubMode <= 3)
    {
        //==================================================================
        // Sub-modes 1-3: X / Y / Z — [Shape] [Amplitude] [Rate] [Phase]
        //==================================================================
        const juce::Identifier shapeIds[]  = { clusterLFOshapeX,      clusterLFOshapeY,      clusterLFOshapeZ };
        const juce::Identifier ampIds[]    = { clusterLFOamplitudeX,  clusterLFOamplitudeY,  clusterLFOamplitudeZ };
        const juce::Identifier rateIds[]   = { clusterLFOrateX,       clusterLFOrateY,       clusterLFOrateZ };
        const juce::Identifier phaseIds[]  = { clusterLFOphaseX,      clusterLFOphaseY,      clusterLFOphaseZ };
        int axis = currentSubMode - 1;

        sec.dials[0] = makeShapeDial (state, clusterNum, shapeIds[axis]);

        sec.dials[1] = makeClusterFloatDial (LOC ("streamDeck.clusters.dials.amplitude"),
                                              LOC ("units.meters"),
                                              clusterLFOamplitudeXYZMin, clusterLFOamplitudeXYZMax,
                                              0.5f, 0.1f, 1, false,
                                              state, clusterNum, ampIds[axis]);

        sec.dials[2] = makeClusterFloatDial (LOC ("streamDeck.clusters.dials.rate"),
                                              "x",
                                              clusterLFOrateMin, clusterLFOrateMax,
                                              0.02f, 0.005f, 2, true,
                                              state, clusterNum, rateIds[axis]);

        sec.dials[3] = makeClusterIntDial (LOC ("streamDeck.clusters.dials.axisPhase"),
                                            LOC ("units.degrees"),
                                            clusterLFOphaseMin, clusterLFOphaseMax,
                                            5, 1, state, clusterNum, phaseIds[axis]);
    }
    else if (currentSubMode == 4)
    {
        //==================================================================
        // Sub-mode 4: Rotation — [Shape] [Angle] [Rate] [Phase]
        //==================================================================
        sec.dials[0] = makeShapeDial (state, clusterNum, clusterLFOshapeRot);

        sec.dials[1] = makeClusterIntDial (LOC ("streamDeck.clusters.dials.angle"),
                                            LOC ("units.degrees"),
                                            clusterLFOamplitudeRotMin, clusterLFOamplitudeRotMax,
                                            5, 1, state, clusterNum, clusterLFOamplitudeRot);

        sec.dials[2] = makeClusterFloatDial (LOC ("streamDeck.clusters.dials.rate"),
                                              "x",
                                              clusterLFOrateMin, clusterLFOrateMax,
                                              0.02f, 0.005f, 2, true,
                                              state, clusterNum, clusterLFOrateRot);

        sec.dials[3] = makeClusterIntDial (LOC ("streamDeck.clusters.dials.axisPhase"),
                                            LOC ("units.degrees"),
                                            clusterLFOphaseMin, clusterLFOphaseMax,
                                            5, 1, state, clusterNum, clusterLFOphaseRot);
    }
    else if (currentSubMode == 5)
    {
        //==================================================================
        // Sub-mode 5: Scale — [Shape] [Ratio] [Rate] [Phase]
        //==================================================================
        sec.dials[0] = makeShapeDial (state, clusterNum, clusterLFOshapeScale);

        sec.dials[1] = makeClusterFloatDial (LOC ("streamDeck.clusters.dials.ratio"),
                                              "x",
                                              clusterLFOamplitudeScaleMin, clusterLFOamplitudeScaleMax,
                                              0.02f, 0.005f, 2, true,
                                              state, clusterNum, clusterLFOamplitudeScale);

        sec.dials[2] = makeClusterFloatDial (LOC ("streamDeck.clusters.dials.rate"),
                                              "x",
                                              clusterLFOrateMin, clusterLFOrateMax,
                                              0.02f, 0.005f, 2, true,
                                              state, clusterNum, clusterLFOrateScale);

        sec.dials[3] = makeClusterIntDial (LOC ("streamDeck.clusters.dials.axisPhase"),
                                            LOC ("units.degrees"),
                                            clusterLFOphaseMin, clusterLFOphaseMax,
                                            5, 1, state, clusterNum, clusterLFOphaseScale);
    }
    else if (currentSubMode == 6)
    {
        //==================================================================
        // Sub-mode 6: Preset — [Navigate] [Store] [Recall] [Recall&Start]
        //==================================================================

        // Dial 0: Navigator — turn scrolls 1-16 wrapping, press+turn jumps rows
        {
            DialBinding d;
            d.paramName     = LOC ("streamDeck.clusters.dials.presetNav");
            d.type          = DialBinding::Int;
            d.minValue      = 1.0f;
            d.maxValue      = 16.0f;
            d.step          = 1.0f;
            d.fineStep      = 0.0f;
            d.decimalPlaces = 0;

            d.getDynamicName = [presetCol, presetRow]()
            {
                if (!presetCol || !presetRow) return juce::String ("Preset");
                int num = *presetRow * 4 + *presetCol + 1;
                return LOC ("streamDeck.clusters.dials.presetN").replace ("%d", juce::String (num));
            };

            d.getValue = [presetCol, presetRow]()
            {
                if (!presetCol || !presetRow) return 1.0f;
                return static_cast<float> (*presetRow * 4 + *presetCol + 1);
            };

            d.setValue = [presetCol, presetRow, cb] (float v)
            {
                if (!presetCol || !presetRow) return;
                int idx = juce::jlimit (1, 16, juce::roundToInt (v));
                // Wrap: if value went beyond range, wrap around
                if (juce::roundToInt (v) < 1)  idx = 16;
                if (juce::roundToInt (v) > 16) idx = 1;
                *presetRow = (idx - 1) / 4;
                *presetCol = (idx - 1) % 4;
                if (cb.highlightPreset) cb.highlightPreset (idx - 1);
            };

            // Alt-binding: press+turn jumps rows directly
            auto alt = std::make_unique<DialBinding>();
            alt->paramName     = LOC ("streamDeck.clusters.dials.presetRow");
            alt->type          = DialBinding::Int;
            alt->minValue      = 1.0f;
            alt->maxValue      = 4.0f;
            alt->step          = 1.0f;
            alt->fineStep      = 0.0f;
            alt->decimalPlaces = 0;

            alt->getValue = [presetRow]()
            {
                return presetRow ? static_cast<float> (*presetRow + 1) : 1.0f;
            };
            alt->setValue = [presetCol, presetRow, cb] (float v)
            {
                if (!presetRow || !presetCol) return;
                *presetRow = juce::jlimit (0, 3, juce::roundToInt (v) - 1);
                if (cb.highlightPreset) cb.highlightPreset (*presetRow * 4 + *presetCol);
            };

            d.altBinding = std::move (alt);
            sec.dials[0] = std::move (d);
        }

        // Dial 1: Store (press only)
        {
            DialBinding d;
            d.paramName     = LOC ("streamDeck.clusters.dials.store");
            d.type          = DialBinding::Int;
            d.minValue      = 1.0f;
            d.maxValue      = 16.0f;
            d.step          = 0.0f;
            d.fineStep      = 0.0f;
            d.decimalPlaces = 0;

            d.getValue = [presetCol, presetRow]()
            {
                if (!presetCol || !presetRow) return 1.0f;
                return static_cast<float> (*presetRow * 4 + *presetCol + 1);
            };
            d.setValue = [] (float) {};
            d.onPress = [presetCol, presetRow, cb]()
            {
                if (!presetCol || !presetRow || !cb.storePreset) return;
                cb.storePreset (*presetRow * 4 + *presetCol);
            };

            sec.dials[1] = std::move (d);
        }

        // Dial 2: Recall (press only)
        {
            DialBinding d;
            d.paramName     = LOC ("streamDeck.clusters.dials.recall");
            d.type          = DialBinding::Int;
            d.minValue      = 1.0f;
            d.maxValue      = 16.0f;
            d.step          = 0.0f;
            d.fineStep      = 0.0f;
            d.decimalPlaces = 0;

            d.getValue = [presetCol, presetRow]()
            {
                if (!presetCol || !presetRow) return 1.0f;
                return static_cast<float> (*presetRow * 4 + *presetCol + 1);
            };
            d.setValue = [] (float) {};
            d.onPress = [presetCol, presetRow, cb]()
            {
                if (!presetCol || !presetRow || !cb.recallPreset) return;
                cb.recallPreset (*presetRow * 4 + *presetCol);
            };

            sec.dials[2] = std::move (d);
        }

        // Dial 3: Recall & Start (press only)
        {
            DialBinding d;
            d.paramName     = LOC ("streamDeck.clusters.dials.recallStart");
            d.type          = DialBinding::Int;
            d.minValue      = 1.0f;
            d.maxValue      = 16.0f;
            d.step          = 0.0f;
            d.fineStep      = 0.0f;
            d.decimalPlaces = 0;

            d.getValue = [presetCol, presetRow]()
            {
                if (!presetCol || !presetRow) return 1.0f;
                return static_cast<float> (*presetRow * 4 + *presetCol + 1);
            };
            d.setValue = [] (float) {};
            d.onPress = [presetCol, presetRow, cb]()
            {
                if (!presetCol || !presetRow || !cb.recallAndStart) return;
                cb.recallAndStart (*presetRow * 4 + *presetCol);
            };

            sec.dials[3] = std::move (d);
        }
    }

    page.activeSectionIndex = 0;
    return page;
}

} // namespace ClustersTabPages
