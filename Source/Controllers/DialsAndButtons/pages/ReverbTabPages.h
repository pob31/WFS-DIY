#pragma once

/**
 * ReverbTabPages — Stream Deck+ page definitions for the Reverb tab.
 *
 * Creates StreamDeckPage objects for the Reverb tab (main tab index 3).
 *
 * Subtabs:
 *   1: Pre-Processing   (4-band EQ + Pre-Compressor)
 *   3: Post-Processing  (4-band EQ + Post-Expander)
 */

#include "../StreamDeckPage.h"
#include "../../../Parameters/WFSValueTreeState.h"
#include "../../../Parameters/WFSParameterIDs.h"
#include "../../../Parameters/WFSParameterDefaults.h"
#include "../../../Localization/LocalizationManager.h"

namespace ReverbTabPages
{

/** Main tab index for Reverb (0-based position in the tab bar). */
static constexpr int REVERB_MAIN_TAB_INDEX = 3;

/** Band colours for 4-band EQ (subset of output EQ colours). */
inline juce::Colour getReverbEqBandColour (int band)
{
    static const juce::Colour colours[] = {
        juce::Colour (0xFFE74C3C),  // Band 1: Red
        juce::Colour (0xFFE67E22),  // Band 2: Orange
        juce::Colour (0xFFFFEB3B),  // Band 3: Yellow
        juce::Colour (0xFF2ECC71),  // Band 4: Green
    };
    return colours[juce::jlimit (0, 3, band)];
}

/**
 * Reverb EQ shape ↔ combo index mapping.
 * Reverb shapes: 0=OFF, 1=LowCut, 2=LowShelf, 3=Peak, 4=HighShelf, 5=HighCut, 6=BandPass
 * Combo order:   LowCut(1), LowShelf(2), Peak(3), BandPass(6), HighShelf(4), HighCut(5)
 */
static constexpr int reverbComboToShape[] = { 1, 2, 3, 6, 4, 5 };
static constexpr int reverbShapeToCombo[] = { 0, 0, 1, 2, 5, 4, 3 };  // index=shapeId → comboIdx

//==============================================================================
// Helpers for reverb channel parameter dials/buttons
//==============================================================================

inline ButtonBinding makeReverbToggleButton (const juce::String& label,
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
        return static_cast<int> (state.getReverbParameter (ch, paramId)) != 0;
    };

    btn.onPress = [&state, ch, paramId]()
    {
        int current = static_cast<int> (state.getReverbParameter (ch, paramId));
        state.setReverbParameter (ch, paramId, current != 0 ? 0 : 1);
    };

    return btn;
}

inline DialBinding makeReverbFloatDial (const juce::String& name,
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
        return static_cast<float> (state.getReverbParameter (ch, paramId));
    };

    dial.setValue = [&state, ch, paramId] (float v)
    {
        state.setReverbParameter (ch, paramId, v);
    };

    return dial;
}

inline DialBinding makeReverbIntDial (const juce::String& name,
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
        return static_cast<float> (static_cast<int> (state.getReverbParameter (ch, paramId)));
    };

    dial.setValue = [&state, ch, paramId] (float v)
    {
        state.setReverbParameter (ch, paramId, juce::roundToInt (v));
    };

    return dial;
}

//==============================================================================
// Subtab 0: Channel Parameters (Feed Params, Feed Orientation, Return, Solo/Mute)
//==============================================================================

inline StreamDeckPage createChannelParametersPage (
    WFSValueTreeState& state,
    int channelIndex,
    std::shared_ptr<bool> soloReverb,
    std::shared_ptr<bool> mutePre,
    std::shared_ptr<bool> mutePost,
    std::shared_ptr<bool> editOnMap,
    std::function<void (bool)> onSoloReverbChanged = nullptr,
    std::function<void (bool)> onMutePreChanged = nullptr,
    std::function<void (bool)> onMutePostChanged = nullptr,
    std::function<void (bool)> onEditOnMapChanged = nullptr)
{
    using namespace WFSParameterIDs;
    using namespace WFSParameterDefaults;

    int ch = channelIndex;
    StreamDeckPage page ("Reverb > Channel Parameters");

    const auto grey = juce::Colour (0xFF3A3A3A);

    //======================================================================
    // Section 0: Reverb Feed Parameters
    //======================================================================
    {
        auto& sec = page.sections[0];
        sec.sectionName   = LOC ("streamDeck.reverb.sections.feedParameters");
        sec.sectionColour = juce::Colour (0xFF4A90D9);  // steel blue

        // Button 0: unassigned

        // Button 1: Toggle Minimal Latency
        sec.buttons[1] = makeReverbToggleButton (LOC ("streamDeck.outputs.buttons.minimalLatency"),
                                                   grey, juce::Colour (0xFF4A90D9),
                                                   state, ch, reverbMiniLatencyEnable);

        // Button 2: Toggle Live Source
        sec.buttons[2] = makeReverbToggleButton (LOC ("streamDeck.outputs.buttons.liveSourceAtten"),
                                                   grey, juce::Colour (0xFF4A90D9),
                                                   state, ch, reverbLSenable);

        // Button 3: unassigned

        // Dial 0: Attenuation (-92 to 0 dB)
        sec.dials[0] = makeReverbFloatDial (LOC ("streamDeck.outputs.dials.attenuation"),
                                             LOC ("units.decibels"),
                                             reverbAttenuationMin, reverbAttenuationMax,
                                             0.5f, 0.1f, 1, false,
                                             state, ch, reverbAttenuation);

        // Dial 1: Delay/Latency (-100 to 100 ms) with dynamic name
        sec.dials[1] = makeReverbFloatDial (LOC ("streamDeck.outputs.dials.delay"),
                                             LOC ("units.milliseconds"),
                                             reverbDelayLatencyMin, reverbDelayLatencyMax,
                                             0.5f, 0.1f, 1, false,
                                             state, ch, reverbDelayLatency);

        sec.dials[1].getDynamicName = [&state, ch]()
        {
            float v = static_cast<float> (state.getReverbParameter (ch, reverbDelayLatency));
            return (v < 0.0f) ? LOC ("streamDeck.outputs.dials.latency")
                              : LOC ("streamDeck.outputs.dials.delay");
        };

        // Dial 2: Distance Attenuation % (0-200%)
        sec.dials[2] = makeReverbIntDial (LOC ("streamDeck.outputs.dials.distanceAttenuation"),
                                           LOC ("units.percent"),
                                           reverbDistanceAttenEnableMin, reverbDistanceAttenEnableMax,
                                           2, 1,
                                           state, ch, reverbDistanceAttenEnable);

        // Dial 3: HF Damping (-6 to 0 dB)
        sec.dials[3] = makeReverbFloatDial (LOC ("streamDeck.outputs.dials.hfDamping"),
                                             LOC ("units.decibels"),
                                             reverbHFdampingMin, reverbHFdampingMax,
                                             0.5f, 0.1f, 1, false,
                                             state, ch, reverbHFdamping);
    }

    //======================================================================
    // Section 1: Reverb Feed Orientation
    //======================================================================
    {
        auto& sec = page.sections[1];
        sec.sectionName   = LOC ("streamDeck.reverb.sections.feedOrientation");
        sec.sectionColour = juce::Colour (0xFF26A69A);  // teal

        // All 4 buttons unassigned

        // Dial 0: On Angle (1-180 degrees)
        sec.dials[0] = makeReverbIntDial (LOC ("streamDeck.outputs.dials.onAngle"),
                                           LOC ("units.degrees"),
                                           reverbAngleOnMin, reverbAngleOnMax,
                                           2, 1,
                                           state, ch, reverbAngleOn);

        // Dial 1: Off Angle (0-179 degrees)
        sec.dials[1] = makeReverbIntDial (LOC ("streamDeck.outputs.dials.offAngle"),
                                           LOC ("units.degrees"),
                                           reverbAngleOffMin, reverbAngleOffMax,
                                           2, 1,
                                           state, ch, reverbAngleOff);

        // Dial 2: Orientation (-179 to 180 degrees)
        sec.dials[2] = makeReverbIntDial (LOC ("streamDeck.outputs.dials.orientation"),
                                           LOC ("units.degrees"),
                                           reverbOrientationMin, reverbOrientationMax,
                                           5, 1,
                                           state, ch, reverbOrientation);

        // Dial 3: Pitch (-90 to 90 degrees)
        sec.dials[3] = makeReverbIntDial (LOC ("streamDeck.outputs.dials.pitch"),
                                           LOC ("units.degrees"),
                                           reverbPitchMin, reverbPitchMax,
                                           2, 1,
                                           state, ch, reverbPitch);
    }

    //======================================================================
    // Section 2: Reverb Return
    //======================================================================
    {
        auto& sec = page.sections[2];
        sec.sectionName   = LOC ("streamDeck.reverb.sections.reverbReturn");
        sec.sectionColour = juce::Colour (0xFFCC8800);  // warm orange

        // All 4 buttons unassigned

        // Dial 0: Distance Attenuation (-6 to 0 dB)
        sec.dials[0] = makeReverbFloatDial (LOC ("streamDeck.outputs.dials.distanceAttenuation"),
                                             LOC ("units.decibels"),
                                             reverbDistanceAttenuationMin, reverbDistanceAttenuationMax,
                                             0.5f, 0.1f, 1, false,
                                             state, ch, reverbDistanceAttenuation);

        // Dial 1: Common Attenuation (0-100%)
        sec.dials[1] = makeReverbIntDial (LOC ("streamDeck.reverb.dials.commonAttenuation"),
                                           LOC ("units.percent"),
                                           reverbCommonAttenMin, reverbCommonAttenMax,
                                           2, 1,
                                           state, ch, reverbCommonAtten);
    }

    //======================================================================
    // Section 3: Solo / Mute
    //======================================================================
    {
        auto& sec = page.sections[3];
        sec.sectionName   = LOC ("streamDeck.reverb.sections.soloMute");
        sec.sectionColour = juce::Colour (0xFFCC8800);  // warm orange

        // Button 0: Edit on Map (independent toggle)
        {
            auto& btn = sec.buttons[0];
            btn.label = LOC ("streamDeck.reverb.buttons.editOnMap");
            btn.colour = grey;
            btn.activeColour = juce::Colour (0xFFCC8800);
            btn.type = ButtonBinding::Toggle;

            btn.getState = [editOnMap]()
            {
                return *editOnMap;
            };

            btn.onPress = [editOnMap, onEditOnMapChanged]()
            {
                *editOnMap = !(*editOnMap);
                if (onEditOnMapChanged)
                    onEditOnMapChanged (*editOnMap);
            };
        }

        // Button 1: Solo Reverb (exclusive group)
        {
            auto& btn = sec.buttons[1];
            btn.label = LOC ("streamDeck.reverb.buttons.soloReverb");
            btn.colour = grey;
            btn.activeColour = juce::Colour (0xFFCC8800);
            btn.type = ButtonBinding::Toggle;

            btn.getState = [soloReverb]()
            {
                return *soloReverb;
            };

            btn.onPress = [soloReverb, mutePre, mutePost,
                           onSoloReverbChanged, onMutePreChanged, onMutePostChanged]()
            {
                *soloReverb = !(*soloReverb);
                if (*soloReverb)
                {
                    if (*mutePre)  { *mutePre = false;  if (onMutePreChanged)  onMutePreChanged (false); }
                    if (*mutePost) { *mutePost = false; if (onMutePostChanged) onMutePostChanged (false); }
                }
                if (onSoloReverbChanged)
                    onSoloReverbChanged (*soloReverb);
            };
        }

        // Button 2: Mute Pre (exclusive group)
        {
            auto& btn = sec.buttons[2];
            btn.label = LOC ("streamDeck.reverb.buttons.mutePre");
            btn.colour = grey;
            btn.activeColour = juce::Colour (0xFFCC8800);
            btn.type = ButtonBinding::Toggle;

            btn.getState = [mutePre]()
            {
                return *mutePre;
            };

            btn.onPress = [soloReverb, mutePre, mutePost,
                           onSoloReverbChanged, onMutePreChanged, onMutePostChanged]()
            {
                *mutePre = !(*mutePre);
                if (*mutePre)
                {
                    if (*soloReverb) { *soloReverb = false; if (onSoloReverbChanged) onSoloReverbChanged (false); }
                    if (*mutePost)   { *mutePost = false;   if (onMutePostChanged)   onMutePostChanged (false); }
                }
                if (onMutePreChanged)
                    onMutePreChanged (*mutePre);
            };
        }

        // Button 3: Mute Post (exclusive group)
        {
            auto& btn = sec.buttons[3];
            btn.label = LOC ("streamDeck.reverb.buttons.mutePost");
            btn.colour = grey;
            btn.activeColour = juce::Colour (0xFFCC8800);
            btn.type = ButtonBinding::Toggle;

            btn.getState = [mutePost]()
            {
                return *mutePost;
            };

            btn.onPress = [soloReverb, mutePre, mutePost,
                           onSoloReverbChanged, onMutePreChanged, onMutePostChanged]()
            {
                *mutePost = !(*mutePost);
                if (*mutePost)
                {
                    if (*soloReverb) { *soloReverb = false; if (onSoloReverbChanged) onSoloReverbChanged (false); }
                    if (*mutePre)    { *mutePre = false;    if (onMutePreChanged)    onMutePreChanged (false); }
                }
                if (onMutePostChanged)
                    onMutePostChanged (*mutePost);
            };
        }
    }

    page.numSections = 4;
    page.activeSectionIndex = 0;

    return page;
}

//==============================================================================
// Subtab 1: Pre-Processing (4-band EQ + Pre-Compressor)
//==============================================================================

inline StreamDeckPage createPreProcessingPage (WFSValueTreeState& state,
                                                 int channelIndex,
                                                 std::shared_ptr<int> selectedBand,
                                                 std::shared_ptr<bool> dynamicsMode,
                                                 std::function<void (int)> onBandSelectedInGui = nullptr)
{
    using namespace WFSParameterIDs;
    using namespace WFSParameterDefaults;

    int ch = channelIndex;
    int band = juce::jlimit (0, 3, *selectedBand);
    bool dynMode = *dynamicsMode;

    StreamDeckPage page ("Reverb > Pre-Processing");

    const auto grey = juce::Colour (0xFF3A3A3A);
    const auto eqColour = juce::Colour (0xFF4A90D9);       // steel blue
    const auto dynColour = juce::Colour (0xFF3498DB);       // compressor blue
    juce::Colour bandColour = getReverbEqBandColour (band);

    //======================================================================
    // Helper: create a band-selector ButtonBinding
    //======================================================================
    auto makeBandSelector = [&] (int targetBand) -> ButtonBinding
    {
        ButtonBinding btn;
        btn.label = LOC ("eq.labels.band") + " " + juce::String (targetBand + 1);
        btn.colour = grey;
        btn.activeColour = getReverbEqBandColour (targetBand);
        btn.type = ButtonBinding::Toggle;
        btn.requestsPageRebuild = true;

        btn.getState = [selectedBand, dynamicsMode, targetBand]()
        {
            return !(*dynamicsMode) && *selectedBand == targetBand;
        };

        btn.onPress = [selectedBand, dynamicsMode, targetBand, onBandSelectedInGui]()
        {
            *selectedBand = targetBand;
            *dynamicsMode = false;
            if (onBandSelectedInGui)
                onBandSelectedInGui (targetBand);
        };

        return btn;
    };

    //======================================================================
    // Top row: EQ on/off + Band 3/4 selectors + Dynamics mode
    //======================================================================

    // Button 0: EQ on/off toggle (per-channel)
    {
        auto& btn = page.topRowButtons[0];
        btn.label = LOC ("streamDeck.outputs.eq.buttons.eqOnOff");
        btn.colour = grey;
        btn.activeColour = eqColour;
        btn.type = ButtonBinding::Toggle;

        btn.getState = [&state, ch]()
        {
            auto eqSection = state.ensureReverbEQSection (ch);
            return static_cast<int> (eqSection.getProperty (reverbPreEQenable, reverbPreEQenableDefault)) != 0;
        };

        btn.onPress = [&state, ch]()
        {
            auto eqSection = state.ensureReverbEQSection (ch);
            int current = static_cast<int> (eqSection.getProperty (reverbPreEQenable, reverbPreEQenableDefault));
            eqSection.setProperty (reverbPreEQenable, current != 0 ? 0 : 1, nullptr);
        };

        btn.getDynamicLabel = [&state, ch]()
        {
            auto eqSection = state.ensureReverbEQSection (ch);
            bool on = static_cast<int> (eqSection.getProperty (reverbPreEQenable, reverbPreEQenableDefault)) != 0;
            return juce::String ("EQ\n") + (on ? "ON" : "OFF");
        };
    }

    // Buttons 1-2: Band 3, 4 selectors
    page.topRowButtons[1] = makeBandSelector (2);
    page.topRowButtons[2] = makeBandSelector (3);

    // Button 3: Dynamics mode selector (works like a band selector)
    {
        auto& btn = page.topRowButtons[3];
        btn.label = LOC ("reverbs.preProcessing.compressor");
        btn.colour = grey;
        btn.activeColour = dynColour;
        btn.type = ButtonBinding::Toggle;
        btn.requestsPageRebuild = true;

        btn.getState = [dynamicsMode]()
        {
            return *dynamicsMode;
        };

        btn.onPress = [dynamicsMode]()
        {
            *dynamicsMode = true;
        };
    }

    //======================================================================
    // Single section
    //======================================================================
    {
        auto& sec = page.sections[0];
        sec.sectionName   = dynMode ? LOC ("reverbs.preProcessing.compressor")
                                    : (LOC ("eq.labels.band") + " " + juce::String (band + 1));
        sec.sectionColour = dynMode ? dynColour : bandColour;

        //------------------------------------------------------------------
        // Bottom row buttons
        //------------------------------------------------------------------

        if (!dynMode)
        {
            // Button 0: Band on/off
            {
                auto& btn = sec.buttons[0];
                btn.label = LOC ("streamDeck.outputs.eq.buttons.bandOnOff");
                btn.colour = grey;
                btn.activeColour = bandColour;
                btn.type = ButtonBinding::Toggle;
                btn.requestsPageRebuild = true;

                btn.getState = [&state, ch, band]()
                {
                    auto bt = state.getReverbEQBand (ch, band);
                    return static_cast<int> (bt.getProperty (reverbPreEQshape, 0)) != 0;
                };

                btn.onPress = [&state, ch, band]()
                {
                    int numCh = state.getNumReverbChannels();
                    auto bt = state.getReverbEQBand (ch, band);
                    int shape = static_cast<int> (bt.getProperty (reverbPreEQshape, 0));
                    int newShape = (shape != 0) ? 0 : reverbPreEQBandComboDefaults[band];
                    for (int c = 0; c < numCh; ++c)
                    {
                        auto b = state.getReverbEQBand (c, band);
                        if (b.isValid())
                            b.setProperty (reverbPreEQshape, newShape, nullptr);
                    }
                };

                btn.getDynamicLabel = [&state, ch, band]()
                {
                    auto bt = state.getReverbEQBand (ch, band);
                    bool on = static_cast<int> (bt.getProperty (reverbPreEQshape, 0)) != 0;
                    return LOC ("eq.labels.band") + " " + juce::String (band + 1) + "\n" + (on ? "ON" : "OFF");
                };
            }

        }

        // Buttons 1-2: Band 1, 2 selectors (always visible)
        sec.buttons[1] = makeBandSelector (0);
        sec.buttons[2] = makeBandSelector (1);

        // Button 3: Dynamics enable/disable (compressor bypass)
        {
            auto& btn = sec.buttons[3];
            btn.label = LOC ("reverbs.preProcessing.compressorOff");
            btn.colour = grey;
            btn.activeColour = dynColour;
            btn.type = ButtonBinding::Toggle;

            btn.getState = [&state]()
            {
                auto preComp = state.ensureReverbPreCompSection();
                return static_cast<int> (preComp.getProperty (reverbPreCompBypass, reverbPreCompBypassDefault)) == 0;
            };

            btn.onPress = [&state]()
            {
                auto preComp = state.ensureReverbPreCompSection();
                int current = static_cast<int> (preComp.getProperty (reverbPreCompBypass, reverbPreCompBypassDefault));
                preComp.setProperty (reverbPreCompBypass, current != 0 ? 0 : 1, nullptr);
            };

            btn.getDynamicLabel = [&state]()
            {
                auto preComp = state.ensureReverbPreCompSection();
                int bypassed = static_cast<int> (preComp.getProperty (reverbPreCompBypass, reverbPreCompBypassDefault));
                return juce::String (LOC ("reverbs.preProcessing.compressor")) + "\n" + (bypassed != 0 ? "OFF" : "ON");
            };
        }

        //------------------------------------------------------------------
        // Dials: EQ mode or Dynamics mode
        //------------------------------------------------------------------

        if (!dynMode)
        {
            // Dial 0: Shape (ComboBox)
            {
                auto& dial = sec.dials[0];
                dial.paramName = LOC ("streamDeck.outputs.eq.dials.shape");
                dial.type = DialBinding::ComboBox;

                dial.comboOptions = {
                    LOC ("eq.filterTypes.lowCut"),
                    LOC ("eq.filterTypes.lowShelf"),
                    LOC ("eq.filterTypes.peakNotch"),
                    LOC ("eq.filterTypes.bandPass"),
                    LOC ("eq.filterTypes.highShelf"),
                    LOC ("eq.filterTypes.highCut")
                };

                dial.minValue = 0.0f;
                dial.maxValue = 5.0f;

                dial.getValue = [&state, ch, band]()
                {
                    auto bt = state.getReverbEQBand (ch, band);
                    int shape = static_cast<int> (bt.getProperty (reverbPreEQshape, 0));
                    if (shape <= 0 || shape > 6)
                        return static_cast<float> (reverbShapeToCombo[reverbPreEQBandComboDefaults[band]]);
                    return static_cast<float> (reverbShapeToCombo[shape]);
                };

                dial.setValue = [&state, ch, band] (float v)
                {
                    int comboIndex = juce::jlimit (0, 5, juce::roundToInt (v));
                    int newShape = reverbComboToShape[comboIndex];
                    int numCh = state.getNumReverbChannels();
                    for (int c = 0; c < numCh; ++c)
                    {
                        auto b = state.getReverbEQBand (c, band);
                        if (b.isValid())
                            b.setProperty (reverbPreEQshape, newShape, nullptr);
                    }
                };
            }

            // Dial 1: Frequency (20-20000 Hz, exponential)
            {
                auto& dial = sec.dials[1];
                dial.paramName = LOC ("streamDeck.outputs.eq.dials.frequency");
                dial.paramUnit = LOC ("units.hertz");
                dial.minValue = static_cast<float> (reverbPreEQfreqMin);
                dial.maxValue = static_cast<float> (reverbPreEQfreqMax);
                dial.step = 0.02f;
                dial.fineStep = 0.005f;
                dial.decimalPlaces = 0;
                dial.isExponential = true;
                dial.type = DialBinding::Float;

                dial.getValue = [&state, ch, band]()
                {
                    auto bt = state.getReverbEQBand (ch, band);
                    return static_cast<float> (static_cast<int> (bt.getProperty (reverbPreEQfreq, reverbPreEQfreqDefault)));
                };

                dial.setValue = [&state, ch, band] (float v)
                {
                    int freq = juce::roundToInt (v);
                    int numCh = state.getNumReverbChannels();
                    for (int c = 0; c < numCh; ++c)
                    {
                        auto b = state.getReverbEQBand (c, band);
                        if (b.isValid())
                            b.setProperty (reverbPreEQfreq, freq, nullptr);
                    }
                };
            }

            // Dial 2: Gain (-24 to 24 dB)
            {
                auto& dial = sec.dials[2];
                dial.paramName = LOC ("streamDeck.outputs.eq.dials.gain");
                dial.paramUnit = LOC ("units.decibels");
                dial.minValue = reverbPreEQgainMin;
                dial.maxValue = reverbPreEQgainMax;
                dial.step = 0.5f;
                dial.fineStep = 0.1f;
                dial.decimalPlaces = 1;
                dial.isExponential = false;
                dial.type = DialBinding::Float;

                dial.getValue = [&state, ch, band]()
                {
                    auto bt = state.getReverbEQBand (ch, band);
                    return static_cast<float> (bt.getProperty (reverbPreEQgain, reverbPreEQgainDefault));
                };

                dial.setValue = [&state, ch, band] (float v)
                {
                    int numCh = state.getNumReverbChannels();
                    for (int c = 0; c < numCh; ++c)
                    {
                        auto b = state.getReverbEQBand (c, band);
                        if (b.isValid())
                            b.setProperty (reverbPreEQgain, v, nullptr);
                    }
                };

                dial.getDynamicName = [&state, ch, band]()
                {
                    auto bt = state.getReverbEQBand (ch, band);
                    int shape = static_cast<int> (bt.getProperty (reverbPreEQshape, 0));
                    // LowCut(1), HighCut(5), BandPass(6) have no gain
                    if (shape == 1 || shape == 5 || shape == 6)
                        return juce::String (juce::CharPointer_UTF8 ("\xe2\x80\x94"));  // em dash
                    return LOC ("streamDeck.outputs.eq.dials.gain");
                };
            }

            // Dial 3: Q (0.1-20.0, exponential)
            {
                auto& dial = sec.dials[3];
                dial.paramName = LOC ("streamDeck.outputs.eq.dials.q");
                dial.minValue = reverbPreEQqMin;
                dial.maxValue = reverbPreEQqMax;
                dial.step = 0.02f;
                dial.fineStep = 0.005f;
                dial.decimalPlaces = 2;
                dial.isExponential = true;
                dial.type = DialBinding::Float;

                dial.getValue = [&state, ch, band]()
                {
                    auto bt = state.getReverbEQBand (ch, band);
                    return static_cast<float> (bt.getProperty (reverbPreEQq, reverbPreEQqDefault));
                };

                dial.setValue = [&state, ch, band] (float v)
                {
                    int numCh = state.getNumReverbChannels();
                    for (int c = 0; c < numCh; ++c)
                    {
                        auto b = state.getReverbEQBand (c, band);
                        if (b.isValid())
                            b.setProperty (reverbPreEQq, v, nullptr);
                    }
                };
            }
        }
        else
        {
            // Dynamics mode: Pre-Compressor dials

            // Dial 0: Threshold (-60 to 0 dB)
            {
                auto& dial = sec.dials[0];
                dial.paramName = LOC ("reverbs.preProcessing.threshold");
                dial.paramUnit = LOC ("units.decibels");
                dial.minValue = reverbPreCompThresholdMin;
                dial.maxValue = reverbPreCompThresholdMax;
                dial.step = 0.5f;
                dial.fineStep = 0.1f;
                dial.decimalPlaces = 1;
                dial.isExponential = false;
                dial.type = DialBinding::Float;

                dial.getValue = [&state]()
                {
                    auto preComp = state.ensureReverbPreCompSection();
                    return static_cast<float> (static_cast<double> (preComp.getProperty (reverbPreCompThreshold, reverbPreCompThresholdDefault)));
                };

                dial.setValue = [&state] (float v)
                {
                    auto preComp = state.ensureReverbPreCompSection();
                    preComp.setProperty (reverbPreCompThreshold, v, nullptr);
                };
            }

            // Dial 1: Ratio (1-20 :1)
            {
                auto& dial = sec.dials[1];
                dial.paramName = LOC ("reverbs.preProcessing.ratio");
                dial.paramUnit = ":1";
                dial.minValue = reverbPreCompRatioMin;
                dial.maxValue = reverbPreCompRatioMax;
                dial.step = 0.5f;
                dial.fineStep = 0.1f;
                dial.decimalPlaces = 1;
                dial.isExponential = false;
                dial.type = DialBinding::Float;

                dial.getValue = [&state]()
                {
                    auto preComp = state.ensureReverbPreCompSection();
                    return static_cast<float> (static_cast<double> (preComp.getProperty (reverbPreCompRatio, reverbPreCompRatioDefault)));
                };

                dial.setValue = [&state] (float v)
                {
                    auto preComp = state.ensureReverbPreCompSection();
                    preComp.setProperty (reverbPreCompRatio, v, nullptr);
                };
            }

            // Dial 2: Attack (0.1-100 ms, exponential)
            {
                auto& dial = sec.dials[2];
                dial.paramName = LOC ("reverbs.preProcessing.attack");
                dial.paramUnit = LOC ("units.milliseconds");
                dial.minValue = reverbPreCompAttackMin;
                dial.maxValue = reverbPreCompAttackMax;
                dial.step = 0.02f;
                dial.fineStep = 0.005f;
                dial.decimalPlaces = 1;
                dial.isExponential = true;
                dial.type = DialBinding::Float;

                dial.getValue = [&state]()
                {
                    auto preComp = state.ensureReverbPreCompSection();
                    return static_cast<float> (static_cast<double> (preComp.getProperty (reverbPreCompAttack, reverbPreCompAttackDefault)));
                };

                dial.setValue = [&state] (float v)
                {
                    auto preComp = state.ensureReverbPreCompSection();
                    preComp.setProperty (reverbPreCompAttack, v, nullptr);
                };
            }

            // Dial 3: Release (10-1000 ms, exponential)
            {
                auto& dial = sec.dials[3];
                dial.paramName = LOC ("reverbs.preProcessing.release");
                dial.paramUnit = LOC ("units.milliseconds");
                dial.minValue = reverbPreCompReleaseMin;
                dial.maxValue = reverbPreCompReleaseMax;
                dial.step = 0.02f;
                dial.fineStep = 0.005f;
                dial.decimalPlaces = 0;
                dial.isExponential = true;
                dial.type = DialBinding::Float;

                dial.getValue = [&state]()
                {
                    auto preComp = state.ensureReverbPreCompSection();
                    return static_cast<float> (static_cast<double> (preComp.getProperty (reverbPreCompRelease, reverbPreCompReleaseDefault)));
                };

                dial.setValue = [&state] (float v)
                {
                    auto preComp = state.ensureReverbPreCompSection();
                    preComp.setProperty (reverbPreCompRelease, v, nullptr);
                };
            }
        }
    }

    page.numSections = 1;
    page.activeSectionIndex = 0;

    return page;
}

//==============================================================================
// Subtab 3: Post-Processing (4-band EQ + Post-Expander)
//==============================================================================

inline StreamDeckPage createPostProcessingPage (WFSValueTreeState& state,
                                                  std::shared_ptr<int> selectedBand,
                                                  std::shared_ptr<bool> dynamicsMode,
                                                  std::function<void (int)> onBandSelectedInGui = nullptr)
{
    using namespace WFSParameterIDs;
    using namespace WFSParameterDefaults;

    int band = juce::jlimit (0, 3, *selectedBand);
    bool dynMode = *dynamicsMode;

    StreamDeckPage page ("Reverb > Post-Processing");

    const auto grey = juce::Colour (0xFF3A3A3A);
    const auto eqColour = juce::Colour (0xFF4A90D9);       // steel blue
    const auto dynColour = juce::Colour (0xFF9B59B6);       // expander purple
    juce::Colour bandColour = getReverbEqBandColour (band);

    //======================================================================
    // Helper: create a band-selector ButtonBinding
    //======================================================================
    auto makeBandSelector = [&] (int targetBand) -> ButtonBinding
    {
        ButtonBinding btn;
        btn.label = LOC ("eq.labels.band") + " " + juce::String (targetBand + 1);
        btn.colour = grey;
        btn.activeColour = getReverbEqBandColour (targetBand);
        btn.type = ButtonBinding::Toggle;
        btn.requestsPageRebuild = true;

        btn.getState = [selectedBand, dynamicsMode, targetBand]()
        {
            return !(*dynamicsMode) && *selectedBand == targetBand;
        };

        btn.onPress = [selectedBand, dynamicsMode, targetBand, onBandSelectedInGui]()
        {
            *selectedBand = targetBand;
            *dynamicsMode = false;
            if (onBandSelectedInGui)
                onBandSelectedInGui (targetBand);
        };

        return btn;
    };

    //======================================================================
    // Top row: EQ on/off + Band 3/4 selectors + Dynamics mode
    //======================================================================

    // Button 0: Post-EQ on/off toggle (global)
    {
        auto& btn = page.topRowButtons[0];
        btn.label = LOC ("streamDeck.outputs.eq.buttons.eqOnOff");
        btn.colour = grey;
        btn.activeColour = eqColour;
        btn.type = ButtonBinding::Toggle;

        btn.getState = [&state]()
        {
            auto eqSection = state.ensureReverbPostEQSection();
            return static_cast<int> (eqSection.getProperty (reverbPostEQenable, reverbPostEQenableDefault)) != 0;
        };

        btn.onPress = [&state]()
        {
            auto eqSection = state.ensureReverbPostEQSection();
            int current = static_cast<int> (eqSection.getProperty (reverbPostEQenable, reverbPostEQenableDefault));
            eqSection.setProperty (reverbPostEQenable, current != 0 ? 0 : 1, nullptr);
        };

        btn.getDynamicLabel = [&state]()
        {
            auto eqSection = state.ensureReverbPostEQSection();
            bool on = static_cast<int> (eqSection.getProperty (reverbPostEQenable, reverbPostEQenableDefault)) != 0;
            return juce::String ("EQ\n") + (on ? "ON" : "OFF");
        };
    }

    // Buttons 1-2: Band 3, 4 selectors
    page.topRowButtons[1] = makeBandSelector (2);
    page.topRowButtons[2] = makeBandSelector (3);

    // Button 3: Dynamics mode selector (works like a band selector)
    {
        auto& btn = page.topRowButtons[3];
        btn.label = LOC ("reverbs.postProcessing.expander");
        btn.colour = grey;
        btn.activeColour = dynColour;
        btn.type = ButtonBinding::Toggle;
        btn.requestsPageRebuild = true;

        btn.getState = [dynamicsMode]()
        {
            return *dynamicsMode;
        };

        btn.onPress = [dynamicsMode]()
        {
            *dynamicsMode = true;
        };
    }

    //======================================================================
    // Single section
    //======================================================================
    {
        auto& sec = page.sections[0];
        sec.sectionName   = dynMode ? LOC ("reverbs.postProcessing.expander")
                                    : (LOC ("eq.labels.band") + " " + juce::String (band + 1));
        sec.sectionColour = dynMode ? dynColour : bandColour;

        //------------------------------------------------------------------
        // Bottom row buttons
        //------------------------------------------------------------------

        if (!dynMode)
        {
            // Button 0: Band on/off
            {
                auto& btn = sec.buttons[0];
                btn.label = LOC ("streamDeck.outputs.eq.buttons.bandOnOff");
                btn.colour = grey;
                btn.activeColour = bandColour;
                btn.type = ButtonBinding::Toggle;
                btn.requestsPageRebuild = true;

                btn.getState = [&state, band]()
                {
                    auto bt = state.getReverbPostEQBand (band);
                    return static_cast<int> (bt.getProperty (reverbPostEQshape, 0)) != 0;
                };

                btn.onPress = [&state, band]()
                {
                    auto bt = state.getReverbPostEQBand (band);
                    int shape = static_cast<int> (bt.getProperty (reverbPostEQshape, 0));
                    int newShape = (shape != 0) ? 0 : reverbPostEQBandComboDefaults[band];
                    bt.setProperty (reverbPostEQshape, newShape, nullptr);
                };

                btn.getDynamicLabel = [&state, band]()
                {
                    auto bt = state.getReverbPostEQBand (band);
                    bool on = static_cast<int> (bt.getProperty (reverbPostEQshape, 0)) != 0;
                    return LOC ("eq.labels.band") + " " + juce::String (band + 1) + "\n" + (on ? "ON" : "OFF");
                };
            }

        }

        // Buttons 1-2: Band 1, 2 selectors (always visible)
        sec.buttons[1] = makeBandSelector (0);
        sec.buttons[2] = makeBandSelector (1);

        // Button 3: Dynamics enable/disable (expander bypass)
        {
            auto& btn = sec.buttons[3];
            btn.label = LOC ("reverbs.postProcessing.expanderOff");
            btn.colour = grey;
            btn.activeColour = dynColour;
            btn.type = ButtonBinding::Toggle;

            btn.getState = [&state]()
            {
                auto postExp = state.ensureReverbPostExpSection();
                return static_cast<int> (postExp.getProperty (reverbPostExpBypass, reverbPostExpBypassDefault)) == 0;
            };

            btn.onPress = [&state]()
            {
                auto postExp = state.ensureReverbPostExpSection();
                int current = static_cast<int> (postExp.getProperty (reverbPostExpBypass, reverbPostExpBypassDefault));
                postExp.setProperty (reverbPostExpBypass, current != 0 ? 0 : 1, nullptr);
            };

            btn.getDynamicLabel = [&state]()
            {
                auto postExp = state.ensureReverbPostExpSection();
                int bypassed = static_cast<int> (postExp.getProperty (reverbPostExpBypass, reverbPostExpBypassDefault));
                return juce::String (LOC ("reverbs.postProcessing.expander")) + "\n" + (bypassed != 0 ? "OFF" : "ON");
            };
        }

        //------------------------------------------------------------------
        // Dials: EQ mode or Dynamics mode
        //------------------------------------------------------------------

        if (!dynMode)
        {
            // Dial 0: Shape (ComboBox)
            {
                auto& dial = sec.dials[0];
                dial.paramName = LOC ("streamDeck.outputs.eq.dials.shape");
                dial.type = DialBinding::ComboBox;

                dial.comboOptions = {
                    LOC ("eq.filterTypes.lowCut"),
                    LOC ("eq.filterTypes.lowShelf"),
                    LOC ("eq.filterTypes.peakNotch"),
                    LOC ("eq.filterTypes.bandPass"),
                    LOC ("eq.filterTypes.highShelf"),
                    LOC ("eq.filterTypes.highCut")
                };

                dial.minValue = 0.0f;
                dial.maxValue = 5.0f;

                dial.getValue = [&state, band]()
                {
                    auto bt = state.getReverbPostEQBand (band);
                    int shape = static_cast<int> (bt.getProperty (reverbPostEQshape, 0));
                    if (shape <= 0 || shape > 6)
                        return static_cast<float> (reverbShapeToCombo[reverbPostEQBandComboDefaults[band]]);
                    return static_cast<float> (reverbShapeToCombo[shape]);
                };

                dial.setValue = [&state, band] (float v)
                {
                    int comboIndex = juce::jlimit (0, 5, juce::roundToInt (v));
                    int newShape = reverbComboToShape[comboIndex];
                    auto bt = state.getReverbPostEQBand (band);
                    if (bt.isValid())
                        bt.setProperty (reverbPostEQshape, newShape, nullptr);
                };
            }

            // Dial 1: Frequency (20-20000 Hz, exponential)
            {
                auto& dial = sec.dials[1];
                dial.paramName = LOC ("streamDeck.outputs.eq.dials.frequency");
                dial.paramUnit = LOC ("units.hertz");
                dial.minValue = static_cast<float> (reverbPostEQfreqMin);
                dial.maxValue = static_cast<float> (reverbPostEQfreqMax);
                dial.step = 0.02f;
                dial.fineStep = 0.005f;
                dial.decimalPlaces = 0;
                dial.isExponential = true;
                dial.type = DialBinding::Float;

                dial.getValue = [&state, band]()
                {
                    auto bt = state.getReverbPostEQBand (band);
                    return static_cast<float> (static_cast<int> (bt.getProperty (reverbPostEQfreq, reverbPostEQfreqDefault)));
                };

                dial.setValue = [&state, band] (float v)
                {
                    int freq = juce::roundToInt (v);
                    auto bt = state.getReverbPostEQBand (band);
                    if (bt.isValid())
                        bt.setProperty (reverbPostEQfreq, freq, nullptr);
                };
            }

            // Dial 2: Gain (-24 to 24 dB)
            {
                auto& dial = sec.dials[2];
                dial.paramName = LOC ("streamDeck.outputs.eq.dials.gain");
                dial.paramUnit = LOC ("units.decibels");
                dial.minValue = reverbPostEQgainMin;
                dial.maxValue = reverbPostEQgainMax;
                dial.step = 0.5f;
                dial.fineStep = 0.1f;
                dial.decimalPlaces = 1;
                dial.isExponential = false;
                dial.type = DialBinding::Float;

                dial.getValue = [&state, band]()
                {
                    auto bt = state.getReverbPostEQBand (band);
                    return static_cast<float> (bt.getProperty (reverbPostEQgain, reverbPostEQgainDefault));
                };

                dial.setValue = [&state, band] (float v)
                {
                    auto bt = state.getReverbPostEQBand (band);
                    if (bt.isValid())
                        bt.setProperty (reverbPostEQgain, v, nullptr);
                };

                dial.getDynamicName = [&state, band]()
                {
                    auto bt = state.getReverbPostEQBand (band);
                    int shape = static_cast<int> (bt.getProperty (reverbPostEQshape, 0));
                    // LowCut(1), HighCut(5), BandPass(6) have no gain
                    if (shape == 1 || shape == 5 || shape == 6)
                        return juce::String (juce::CharPointer_UTF8 ("\xe2\x80\x94"));  // em dash
                    return LOC ("streamDeck.outputs.eq.dials.gain");
                };
            }

            // Dial 3: Q (0.1-20.0, exponential)
            {
                auto& dial = sec.dials[3];
                dial.paramName = LOC ("streamDeck.outputs.eq.dials.q");
                dial.minValue = reverbPostEQqMin;
                dial.maxValue = reverbPostEQqMax;
                dial.step = 0.02f;
                dial.fineStep = 0.005f;
                dial.decimalPlaces = 2;
                dial.isExponential = true;
                dial.type = DialBinding::Float;

                dial.getValue = [&state, band]()
                {
                    auto bt = state.getReverbPostEQBand (band);
                    return static_cast<float> (bt.getProperty (reverbPostEQq, reverbPostEQqDefault));
                };

                dial.setValue = [&state, band] (float v)
                {
                    auto bt = state.getReverbPostEQBand (band);
                    if (bt.isValid())
                        bt.setProperty (reverbPostEQq, v, nullptr);
                };
            }
        }
        else
        {
            // Dynamics mode: Post-Expander dials

            // Dial 0: Threshold (-80 to -10 dB)
            {
                auto& dial = sec.dials[0];
                dial.paramName = LOC ("reverbs.postProcessing.threshold");
                dial.paramUnit = LOC ("units.decibels");
                dial.minValue = reverbPostExpThresholdMin;
                dial.maxValue = reverbPostExpThresholdMax;
                dial.step = 0.5f;
                dial.fineStep = 0.1f;
                dial.decimalPlaces = 1;
                dial.isExponential = false;
                dial.type = DialBinding::Float;

                dial.getValue = [&state]()
                {
                    auto postExp = state.ensureReverbPostExpSection();
                    return static_cast<float> (static_cast<double> (postExp.getProperty (reverbPostExpThreshold, reverbPostExpThresholdDefault)));
                };

                dial.setValue = [&state] (float v)
                {
                    auto postExp = state.ensureReverbPostExpSection();
                    postExp.setProperty (reverbPostExpThreshold, v, nullptr);
                };
            }

            // Dial 1: Ratio (1-8 1:N)
            {
                auto& dial = sec.dials[1];
                dial.paramName = LOC ("reverbs.postProcessing.ratio");
                dial.paramUnit = "1:";
                dial.minValue = reverbPostExpRatioMin;
                dial.maxValue = reverbPostExpRatioMax;
                dial.step = 0.1f;
                dial.fineStep = 0.05f;
                dial.decimalPlaces = 1;
                dial.isExponential = false;
                dial.type = DialBinding::Float;

                dial.getValue = [&state]()
                {
                    auto postExp = state.ensureReverbPostExpSection();
                    return static_cast<float> (static_cast<double> (postExp.getProperty (reverbPostExpRatio, reverbPostExpRatioDefault)));
                };

                dial.setValue = [&state] (float v)
                {
                    auto postExp = state.ensureReverbPostExpSection();
                    postExp.setProperty (reverbPostExpRatio, v, nullptr);
                };
            }

            // Dial 2: Attack (0.1-50 ms, exponential)
            {
                auto& dial = sec.dials[2];
                dial.paramName = LOC ("reverbs.postProcessing.attack");
                dial.paramUnit = LOC ("units.milliseconds");
                dial.minValue = reverbPostExpAttackMin;
                dial.maxValue = reverbPostExpAttackMax;
                dial.step = 0.02f;
                dial.fineStep = 0.005f;
                dial.decimalPlaces = 1;
                dial.isExponential = true;
                dial.type = DialBinding::Float;

                dial.getValue = [&state]()
                {
                    auto postExp = state.ensureReverbPostExpSection();
                    return static_cast<float> (static_cast<double> (postExp.getProperty (reverbPostExpAttack, reverbPostExpAttackDefault)));
                };

                dial.setValue = [&state] (float v)
                {
                    auto postExp = state.ensureReverbPostExpSection();
                    postExp.setProperty (reverbPostExpAttack, v, nullptr);
                };
            }

            // Dial 3: Release (50-2000 ms, exponential)
            {
                auto& dial = sec.dials[3];
                dial.paramName = LOC ("reverbs.postProcessing.release");
                dial.paramUnit = LOC ("units.milliseconds");
                dial.minValue = reverbPostExpReleaseMin;
                dial.maxValue = reverbPostExpReleaseMax;
                dial.step = 0.02f;
                dial.fineStep = 0.005f;
                dial.decimalPlaces = 0;
                dial.isExponential = true;
                dial.type = DialBinding::Float;

                dial.getValue = [&state]()
                {
                    auto postExp = state.ensureReverbPostExpSection();
                    return static_cast<float> (static_cast<double> (postExp.getProperty (reverbPostExpRelease, reverbPostExpReleaseDefault)));
                };

                dial.setValue = [&state] (float v)
                {
                    auto postExp = state.ensureReverbPostExpSection();
                    postExp.setProperty (reverbPostExpRelease, v, nullptr);
                };
            }
        }
    }

    page.numSections = 1;
    page.activeSectionIndex = 0;

    return page;
}

//==============================================================================
// Helpers for reverb algorithm (global) parameter dials
//==============================================================================

inline DialBinding makeAlgoFloatDial (const juce::String& name,
                                       const juce::String& unit,
                                       float minVal, float maxVal,
                                       float stepVal, float fineVal,
                                       int decimals, bool exponential,
                                       WFSValueTreeState& state,
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

    dial.getValue = [&state, paramId]()
    {
        auto section = state.getReverbAlgorithmSection();
        if (section.isValid())
            return static_cast<float> (section.getProperty (paramId));
        return 0.0f;
    };

    dial.setValue = [&state, paramId] (float v)
    {
        auto section = state.ensureReverbAlgorithmSection();
        section.setProperty (paramId, v, state.getUndoManager());
    };

    return dial;
}

//==============================================================================
// Subtab 2: Algorithm (SDN/FDN/IR selection + algorithm-specific params)
//==============================================================================

inline StreamDeckPage createAlgorithmPage (
    WFSValueTreeState& state,
    std::shared_ptr<int> algoSubMode,
    std::shared_ptr<float> irDuration,
    std::shared_ptr<bool> soloReverb,
    std::shared_ptr<bool> mutePre,
    std::shared_ptr<bool> mutePost,
    std::function<void (bool)> onSoloReverbChanged = nullptr,
    std::function<void (bool)> onMutePreChanged = nullptr,
    std::function<void (bool)> onMutePostChanged = nullptr)
{
    using namespace WFSParameterIDs;
    using namespace WFSParameterDefaults;

    StreamDeckPage page ("Reverb > Algorithm");

    const auto grey = juce::Colour (0xFF3A3A3A);

    // Read current algorithm type
    auto algoSection = state.getReverbAlgorithmSection();
    int algoType = algoSection.isValid()
                       ? static_cast<int> (algoSection.getProperty (reverbAlgoType, reverbAlgoTypeDefault))
                       : reverbAlgoTypeDefault;

    // For IR mode, force sub-mode to 0 (unless solo/mute)
    if (algoType == 2 && *algoSubMode == 1)
        *algoSubMode = 0;

    int subMode = *algoSubMode;

    //======================================================================
    // Top Row: Algorithm Selectors (buttons 0-2) + Solo/Mute (button 3)
    //======================================================================

    // Button 0: SDN
    {
        auto& btn = page.topRowButtons[0];
        btn.label = LOC ("streamDeck.reverb.algorithm.sdn");
        btn.colour = juce::Colour (0xFF00BCD4).withAlpha (0.3f);
        btn.activeColour = juce::Colour (0xFF00BCD4);
        btn.type = ButtonBinding::Toggle;
        btn.requestsPageRebuild = true;

        btn.getState = [&state]()
        {
            auto sec = state.getReverbAlgorithmSection();
            return sec.isValid() && static_cast<int> (sec.getProperty (reverbAlgoType, 0)) == 0;
        };

        btn.onPress = [&state, algoSubMode]()
        {
            auto sec = state.ensureReverbAlgorithmSection();
            sec.setProperty (reverbAlgoType, 0, state.getUndoManager());
            // Reset sub-mode when switching algorithm (unless in solo/mute)
            if (*algoSubMode != 2)
                *algoSubMode = 0;
        };
    }

    // Button 1: FDN
    {
        auto& btn = page.topRowButtons[1];
        btn.label = LOC ("streamDeck.reverb.algorithm.fdn");
        btn.colour = juce::Colour (0xFFFF9800).withAlpha (0.3f);
        btn.activeColour = juce::Colour (0xFFFF9800);
        btn.type = ButtonBinding::Toggle;
        btn.requestsPageRebuild = true;

        btn.getState = [&state]()
        {
            auto sec = state.getReverbAlgorithmSection();
            return sec.isValid() && static_cast<int> (sec.getProperty (reverbAlgoType, 0)) == 1;
        };

        btn.onPress = [&state, algoSubMode]()
        {
            auto sec = state.ensureReverbAlgorithmSection();
            sec.setProperty (reverbAlgoType, 1, state.getUndoManager());
            if (*algoSubMode != 2)
                *algoSubMode = 0;
        };
    }

    // Button 2: IR
    {
        auto& btn = page.topRowButtons[2];
        btn.label = LOC ("streamDeck.reverb.algorithm.ir");
        btn.colour = juce::Colour (0xFF9C27B0).withAlpha (0.3f);
        btn.activeColour = juce::Colour (0xFF9C27B0);
        btn.type = ButtonBinding::Toggle;
        btn.requestsPageRebuild = true;

        btn.getState = [&state]()
        {
            auto sec = state.getReverbAlgorithmSection();
            return sec.isValid() && static_cast<int> (sec.getProperty (reverbAlgoType, 0)) == 2;
        };

        btn.onPress = [&state, algoSubMode]()
        {
            auto sec = state.ensureReverbAlgorithmSection();
            sec.setProperty (reverbAlgoType, 2, state.getUndoManager());
            if (*algoSubMode != 2)
                *algoSubMode = 0;
        };
    }

    // Button 3: Solo/Mute
    {
        auto& btn = page.topRowButtons[3];
        btn.label = LOC ("streamDeck.reverb.algorithm.soloMute");
        btn.colour = grey;
        btn.activeColour = juce::Colour (0xFFCC8800);
        btn.type = ButtonBinding::Toggle;
        btn.requestsPageRebuild = true;

        btn.getState = [algoSubMode]()
        {
            return *algoSubMode == 2;
        };

        btn.onPress = [algoSubMode]()
        {
            *algoSubMode = (*algoSubMode == 2) ? 0 : 2;
        };
    }

    //======================================================================
    // Section 0: The single section (content depends on algoType + subMode)
    //======================================================================
    {
        auto& sec = page.sections[0];
        sec.sectionName = "Algorithm";
        sec.sectionColour = juce::Colour (0xFF4A90D9);

        if (subMode == 2)
        {
            //--------------------------------------------------------------
            // Solo/Mute mode — bottom buttons: Solo Reverb, Mute Pre, Mute Post
            //--------------------------------------------------------------
            // Button 0: Solo Reverb
            {
                auto& btn = sec.buttons[0];
                btn.label = LOC ("streamDeck.reverb.buttons.soloReverb");
                btn.colour = grey;
                btn.activeColour = juce::Colour (0xFFCC8800);
                btn.type = ButtonBinding::Toggle;

                btn.getState = [soloReverb]() { return *soloReverb; };

                btn.onPress = [soloReverb, mutePre, mutePost,
                               onSoloReverbChanged, onMutePreChanged, onMutePostChanged]()
                {
                    *soloReverb = !(*soloReverb);
                    if (*soloReverb)
                    {
                        if (*mutePre)  { *mutePre = false;  if (onMutePreChanged)  onMutePreChanged (false); }
                        if (*mutePost) { *mutePost = false; if (onMutePostChanged) onMutePostChanged (false); }
                    }
                    if (onSoloReverbChanged)
                        onSoloReverbChanged (*soloReverb);
                };
            }

            // Button 1: Mute Pre
            {
                auto& btn = sec.buttons[1];
                btn.label = LOC ("streamDeck.reverb.buttons.mutePre");
                btn.colour = grey;
                btn.activeColour = juce::Colour (0xFFCC8800);
                btn.type = ButtonBinding::Toggle;

                btn.getState = [mutePre]() { return *mutePre; };

                btn.onPress = [soloReverb, mutePre, mutePost,
                               onSoloReverbChanged, onMutePreChanged, onMutePostChanged]()
                {
                    *mutePre = !(*mutePre);
                    if (*mutePre)
                    {
                        if (*soloReverb) { *soloReverb = false; if (onSoloReverbChanged) onSoloReverbChanged (false); }
                        if (*mutePost)   { *mutePost = false;   if (onMutePostChanged)   onMutePostChanged (false); }
                    }
                    if (onMutePreChanged)
                        onMutePreChanged (*mutePre);
                };
            }

            // Button 2: Mute Post
            {
                auto& btn = sec.buttons[2];
                btn.label = LOC ("streamDeck.reverb.buttons.mutePost");
                btn.colour = grey;
                btn.activeColour = juce::Colour (0xFFCC8800);
                btn.type = ButtonBinding::Toggle;

                btn.getState = [mutePost]() { return *mutePost; };

                btn.onPress = [soloReverb, mutePre, mutePost,
                               onSoloReverbChanged, onMutePreChanged, onMutePostChanged]()
                {
                    *mutePost = !(*mutePost);
                    if (*mutePost)
                    {
                        if (*soloReverb) { *soloReverb = false; if (onSoloReverbChanged) onSoloReverbChanged (false); }
                        if (*mutePre)    { *mutePre = false;    if (onMutePreChanged)    onMutePreChanged (false); }
                    }
                    if (onMutePostChanged)
                        onMutePostChanged (*mutePost);
                };
            }

            // No dials in solo/mute mode
        }
        else if (algoType == 2)
        {
            //--------------------------------------------------------------
            // IR mode — no bottom buttons, dials: Trim, Length, empty, Wet Level
            //--------------------------------------------------------------
            {
                float irDur = (irDuration && *irDuration > 0.0f) ? *irDuration : reverbIRlengthDefault;
                float maxTrimMs = irDur * 1000.0f;

                sec.dials[0] = makeAlgoFloatDial (LOC ("streamDeck.reverb.algorithm.irTrim"),
                                                   LOC ("units.milliseconds"),
                                                   reverbIRtrimMin, maxTrimMs,
                                                   100.0f, 10.0f, 1, false,
                                                   state, reverbIRtrim);

                sec.dials[1] = makeAlgoFloatDial (LOC ("streamDeck.reverb.algorithm.irLength"),
                                                   LOC ("units.seconds"),
                                                   reverbIRlengthMin, irDur,
                                                   0.1f, 0.01f, 2, false,
                                                   state, reverbIRlength);
            }

            // Dial 2: unassigned

            sec.dials[3] = makeAlgoFloatDial (LOC ("streamDeck.reverb.algorithm.wetLevel"),
                                               LOC ("units.decibels"),
                                               reverbWetLevelMin, reverbWetLevelMax,
                                               0.5f, 0.1f, 1, false,
                                               state, reverbWetLevel);
        }
        else
        {
            //--------------------------------------------------------------
            // SDN/FDN mode — bottom buttons: RT60 / Cross Diff Scale(Size)
            //--------------------------------------------------------------
            bool isSDN = (algoType == 0);

            // Button 0: RT60 group selector
            {
                auto& btn = sec.buttons[0];
                btn.label = LOC ("streamDeck.reverb.algorithm.rt60Group");
                btn.colour = (subMode == 0) ? juce::Colour (0xFF4A90D9) : grey;
                btn.activeColour = juce::Colour (0xFF4A90D9);
                btn.type = ButtonBinding::Toggle;
                btn.requestsPageRebuild = true;

                btn.getState = [algoSubMode]() { return *algoSubMode == 0; };
                btn.onPress = [algoSubMode]() { *algoSubMode = 0; };
            }

            // Button 1: Crossover/Diffusion/Scale or Size group selector
            {
                auto& btn = sec.buttons[1];
                btn.label = isSDN ? LOC ("streamDeck.reverb.algorithm.crossDiffScale")
                                  : LOC ("streamDeck.reverb.algorithm.crossDiffSize");
                btn.colour = (subMode == 1) ? juce::Colour (0xFF4A90D9) : grey;
                btn.activeColour = juce::Colour (0xFF4A90D9);
                btn.type = ButtonBinding::Toggle;
                btn.requestsPageRebuild = true;

                btn.getState = [algoSubMode]() { return *algoSubMode == 1; };
                btn.onPress = [algoSubMode]() { *algoSubMode = 1; };
            }

            if (subMode == 0)
            {
                // RT60 group dials
                sec.dials[0] = makeAlgoFloatDial (LOC ("streamDeck.reverb.algorithm.rt60"),
                                                   LOC ("units.seconds"),
                                                   reverbRT60Min, reverbRT60Max,
                                                   0.02f, 0.005f, 2, true,
                                                   state, reverbRT60);

                sec.dials[1] = makeAlgoFloatDial (LOC ("streamDeck.reverb.algorithm.rt60High"),
                                                   "x",
                                                   reverbRT60HighMultMin, reverbRT60HighMultMax,
                                                   0.02f, 0.005f, 2, true,
                                                   state, reverbRT60HighMult);

                sec.dials[2] = makeAlgoFloatDial (LOC ("streamDeck.reverb.algorithm.rt60Low"),
                                                   "x",
                                                   reverbRT60LowMultMin, reverbRT60LowMultMax,
                                                   0.02f, 0.005f, 2, true,
                                                   state, reverbRT60LowMult);

                sec.dials[3] = makeAlgoFloatDial (LOC ("streamDeck.reverb.algorithm.wetLevel"),
                                                   LOC ("units.decibels"),
                                                   reverbWetLevelMin, reverbWetLevelMax,
                                                   0.5f, 0.1f, 1, false,
                                                   state, reverbWetLevel);
            }
            else
            {
                // Crossover/Diffusion/Scale(Size) group dials
                sec.dials[0] = makeAlgoFloatDial (LOC ("streamDeck.reverb.algorithm.crossoverHigh"),
                                                   LOC ("units.hertz"),
                                                   reverbCrossoverHighMin, reverbCrossoverHighMax,
                                                   0.02f, 0.005f, 0, true,
                                                   state, reverbCrossoverHigh);

                sec.dials[1] = makeAlgoFloatDial (LOC ("streamDeck.reverb.algorithm.crossoverLow"),
                                                   LOC ("units.hertz"),
                                                   reverbCrossoverLowMin, reverbCrossoverLowMax,
                                                   0.02f, 0.005f, 0, true,
                                                   state, reverbCrossoverLow);

                sec.dials[2] = makeAlgoFloatDial (LOC ("streamDeck.reverb.algorithm.diffusion"),
                                                   "%",
                                                   reverbDiffusionMin * 100.0f, reverbDiffusionMax * 100.0f,
                                                   1.0f, 0.5f, 0, false,
                                                   state, reverbDiffusion);
                // Diffusion is stored as 0-1 but displayed as 0-100%
                sec.dials[2].getValue = [&state]()
                {
                    auto section = state.getReverbAlgorithmSection();
                    if (section.isValid())
                        return static_cast<float> (section.getProperty (reverbDiffusion)) * 100.0f;
                    return 50.0f;
                };
                sec.dials[2].setValue = [&state] (float v)
                {
                    auto section = state.ensureReverbAlgorithmSection();
                    section.setProperty (reverbDiffusion, v / 100.0f, state.getUndoManager());
                };

                if (isSDN)
                {
                    sec.dials[3] = makeAlgoFloatDial (LOC ("streamDeck.reverb.algorithm.scale"),
                                                       "x",
                                                       reverbSDNscaleMin, reverbSDNscaleMax,
                                                       0.05f, 0.01f, 2, false,
                                                       state, reverbSDNscale);
                }
                else
                {
                    sec.dials[3] = makeAlgoFloatDial (LOC ("streamDeck.reverb.algorithm.size"),
                                                       "x",
                                                       reverbFDNsizeMin, reverbFDNsizeMax,
                                                       0.05f, 0.01f, 2, false,
                                                       state, reverbFDNsize);
                }
            }
        }
    }

    page.numSections = 1;
    page.activeSectionIndex = 0;

    return page;
}

//==============================================================================
// Factory
//==============================================================================

/** Create a Stream Deck page for the given Reverb subtab.
    @param subTabIndex           Subtab index (0 = Channel Params, 1 = Pre-Processing, 2 = Algorithm, 3 = Post-Processing)
    @param state                 The shared value tree state
    @param channelIndex          Reverb channel index (0-based)
    @param preEqBand             Shared pre-EQ band selection state (0-3)
    @param preDynMode            Shared pre-processing dynamics mode state
    @param postEqBand            Shared post-EQ band selection state (0-3)
    @param postDynMode           Shared post-processing dynamics mode state
    @param soloReverb            Shared solo reverb state
    @param mutePre               Shared mute pre state
    @param mutePost              Shared mute post state
    @param editOnMap             Shared edit-on-map state
    @param algoSubMode           Shared algorithm sub-mode state (0=RT60, 1=CrossDiff, 2=SoloMute)
    @param onPreEqBandSelected   Callback for GUI sync when pre-EQ band selected on Stream Deck
    @param onPostEqBandSelected  Callback for GUI sync when post-EQ band selected on Stream Deck
    @param onSoloReverbChanged   Callback when solo reverb toggled from Stream Deck
    @param onMutePreChanged      Callback when mute pre toggled from Stream Deck
    @param onMutePostChanged     Callback when mute post toggled from Stream Deck
    @param onEditOnMapChanged    Callback when edit-on-map toggled from Stream Deck
*/
inline StreamDeckPage createPage (int subTabIndex,
                                    WFSValueTreeState& state,
                                    int channelIndex,
                                    std::shared_ptr<int> preEqBand,
                                    std::shared_ptr<bool> preDynMode,
                                    std::shared_ptr<int> postEqBand,
                                    std::shared_ptr<bool> postDynMode,
                                    std::shared_ptr<bool> soloReverb = nullptr,
                                    std::shared_ptr<bool> mutePre = nullptr,
                                    std::shared_ptr<bool> mutePost = nullptr,
                                    std::shared_ptr<bool> editOnMap = nullptr,
                                    std::shared_ptr<int> algoSubMode = nullptr,
                                    std::shared_ptr<float> irDuration = nullptr,
                                    std::function<void (int)> onPreEqBandSelected = nullptr,
                                    std::function<void (int)> onPostEqBandSelected = nullptr,
                                    std::function<void (bool)> onSoloReverbChanged = nullptr,
                                    std::function<void (bool)> onMutePreChanged = nullptr,
                                    std::function<void (bool)> onMutePostChanged = nullptr,
                                    std::function<void (bool)> onEditOnMapChanged = nullptr)
{
    if (state.getNumReverbChannels() == 0)
    {
        StreamDeckPage page (LOC ("streamDeck.reverb.noChannels"));
        page.activeSectionIndex = -1;
        page.lcdMessage = LOC ("streamDeck.reverb.noChannels");
        return page;
    }

    switch (subTabIndex)
    {
        case 0:  return createChannelParametersPage (state, channelIndex,
                     soloReverb, mutePre, mutePost, editOnMap,
                     onSoloReverbChanged, onMutePreChanged, onMutePostChanged, onEditOnMapChanged);
        case 1:  return createPreProcessingPage (state, channelIndex, preEqBand, preDynMode, onPreEqBandSelected);
        case 2:  return createAlgorithmPage (state, algoSubMode, irDuration,
                     soloReverb, mutePre, mutePost,
                     onSoloReverbChanged, onMutePreChanged, onMutePostChanged);
        case 3:  return createPostProcessingPage (state, postEqBand, postDynMode, onPostEqBandSelected);
        default: return StreamDeckPage ("Reverb > Unknown");
    }
}

} // namespace ReverbTabPages
