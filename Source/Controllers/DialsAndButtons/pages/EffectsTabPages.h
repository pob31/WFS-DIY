#pragma once

/**
    Stream Deck pages for the Effects tab (TabIndex::Effects), one per sub-tab:
    0 Channel Parameters, 1 Chain, 2 Post-Processing (the sends matrix),
    3 Movements, 4 Settings.

    THE FUNNEL, NOT THE STATE. Every per-channel write goes through
    EffectParamEdit, exactly as the Outputs pages write through ArrayParamEdit:
    a hardware edit propagates to the link group by each member's mode, and a
    detached channel stays detached. The reads come from the state. The nine
    globals on the Settings page are config, so they use the generic
    setParameter, as the Settings sub-tab does.

    THE CHAIN PAGE FOLLOWS THE SELECTED MODULE. Its dials are the selected
    module's ordinary controls, taken from the same CSV-generated descriptors
    the GUI panel is built from, so a control added to the CSV reaches the
    deck without touching this file. The module is chosen with the Prev /
    Next buttons (a shared slot index, rebuild on change), which walk the
    channel's chain order as the GUI's tile strip shows it, and the choice
    is mirrored to that strip. Twelve dials is not every module's
    count, so the controls come in BANKS of twelve with a Page button on
    every module section; the bank goes back to the first on a module change.
    The reverb's controls are the ones its model uses (the CSV's Models
    column), so a deck edit that changes the model - itself, or through a
    preset - asks for the page to be laid out again.
*/

#include "../../../../spatcore/controllers/streamdeck/StreamDeckPage.h"
#include "../../../../spatcore/effects/EffectsTypes.h"
#include "../../../Parameters/WFSValueTreeState.h"
#include "../../../Parameters/WFSParameterIDs.h"
#include "../../../Parameters/WFSParameterDefaults.h"
#include "../../../Parameters/EffectParamEdit.h"
#include "../../../Localization/LocalizationManager.h"
#include "../../../gui/TabIndex.h"
#include "../../../gui/effects/EffectsModuleDescriptors.h"

namespace EffectsTabPages
{

static constexpr int EFFECTS_MAIN_TAB_INDEX = TabIndex::Effects;
static constexpr int kNumSlots = spatcore::effects::kNumModuleSlots;

/** What the deck reaches beyond the tree: the engine, the map, the movement
    processor, the sends widget and the GUI's chain selection. */
struct EffectsCallbacks
{
    std::function<void (bool)>  onSoloEffectsChanged;
    std::function<void (bool)>  onEditOnMapChanged;
    std::function<void (int)>   onClear;               // fx, -1 = all
    std::function<void (int)>   onChainSlotSelected;   // mirror to the GUI strip
    std::function<void()>       onRelayout;
    std::function<void()>       onModuleLayoutChanged; // a deck edit moved the reverb's model: rebuild, deferred

    std::function<void (int)>   startMotion;
    std::function<void (int)>   stopMotion;
    std::function<void (int)>   pauseMotion;
    std::function<void (int)>   resumeMotion;
    std::function<void()>       stopAll;

    std::function<void (int, int)> sendsMove;          // dx, dy
    std::function<void()>          sendsToggle;
    std::function<float()>         sendsLevelDb;
    std::function<void (float)>    sendsSetLevelDb;
    std::function<void (bool)>     sendsSetAll;
};

//==============================================================================
// Binding helpers: read the state, write the funnel
//==============================================================================

inline ButtonBinding makeToggleButton (const juce::String& label, juce::Colour offColour, juce::Colour onColour,
                                       WFSValueTreeState& state, EffectParamEdit& edit, int ch,
                                       const juce::Identifier& paramId)
{
    ButtonBinding btn;
    btn.label = label;
    btn.colour = offColour;
    btn.activeColour = onColour;
    btn.type = ButtonBinding::Toggle;
    btn.getState = [&state, ch, paramId]() { return static_cast<int> (state.getEffectParameter (ch, paramId)) != 0; };
    btn.onPress  = [&state, &edit, ch, paramId]()
    {
        const int current = static_cast<int> (state.getEffectParameter (ch, paramId));
        edit.write (ch, paramId, current != 0 ? 0 : 1);
    };
    return btn;
}

inline DialBinding makeFloatDial (const juce::String& name, const juce::String& unit,
                                  float minVal, float maxVal, float stepVal, float fineVal,
                                  int decimals, bool exponential,
                                  WFSValueTreeState& state, EffectParamEdit& edit, int ch,
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
    dial.getValue = [&state, ch, paramId]() { return static_cast<float> (state.getEffectParameter (ch, paramId)); };
    dial.setValue = [&edit, ch, paramId] (float v) { edit.write (ch, paramId, v); };
    return dial;
}

inline DialBinding makeIntDial (const juce::String& name, const juce::String& unit,
                                int minVal, int maxVal, int stepVal, int fineVal,
                                WFSValueTreeState& state, EffectParamEdit& edit, int ch,
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
    dial.getValue = [&state, ch, paramId]() { return static_cast<float> (static_cast<int> (state.getEffectParameter (ch, paramId))); };
    dial.setValue = [&edit, ch, paramId] (float v) { edit.write (ch, paramId, juce::roundToInt (v)); };
    return dial;
}

/** A module control, from its CSV descriptor: the read goes to the module
    NODE (the only way to tell FxEq1 from FxEq2), the write to writeModule. */
inline DialBinding makeModuleDial (const EffectsUi::ControlDesc& d, const juce::Identifier& moduleType,
                                   WFSValueTreeState& state, EffectParamEdit& edit, int ch)
{
    using EffectsUi::Kind;
    DialBinding dial;
    dial.paramName = LOC ("effects.labels." + juce::String (d.key)).trimCharactersAtEnd (":");
    dial.paramUnit = juce::String (d.unit);
    dial.minValue = d.min;
    dial.maxValue = d.max;

    auto readVar = [&state, ch, moduleType, id = &d.id]() { return state.getEffectModuleSection (ch, moduleType).getProperty (*id); };

    switch (d.kind)
    {
        case Kind::Bypass:
        case Kind::Toggle:
        case Kind::Combo:
        {
            dial.type = DialBinding::ComboBox;
            std::vector<int> values;
            if (d.kind == Kind::Bypass)
            {
                dial.comboOptions = { LOC ("effects.chain.moduleOff"), LOC ("effects.chain.moduleOn") };
                values = { 1, 0 };        // the stored value is the bypass; the deck shows ON/OFF
            }
            else
            {
                for (const auto& item : d.items)
                {
                    dial.comboOptions.add (LOC (juce::String (d.enumPrefix) + item.slug));
                    values.push_back (item.value);
                }
            }
            dial.minValue = 0.0f;
            dial.maxValue = static_cast<float> (juce::jmax (0, static_cast<int> (values.size()) - 1));
            dial.getValue = [readVar, values, def = d.def]()
            {
                const auto v = readVar();
                const int stored = v.isVoid() ? juce::roundToInt (def) : static_cast<int> (v);
                for (size_t i = 0; i < values.size(); ++i)
                    if (values[i] == stored) return static_cast<float> (i);
                return 0.0f;
            };
            dial.setValue = [&edit, ch, moduleType, id = &d.id, values] (float v)
            {
                const int index = juce::jlimit (0, static_cast<int> (values.size()) - 1, juce::roundToInt (v));
                edit.writeModule (ch, moduleType, *id, values[static_cast<size_t> (index)]);
            };
            break;
        }
        case Kind::Rotation:
        case Kind::Dial:
        case Kind::Slider:
        case Kind::LogSlider:
        case Kind::BiSlider:
        {
            dial.type = DialBinding::Float;
            dial.isExponential = d.kind == Kind::LogSlider;
            const float span = d.max - d.min;
            dial.step = dial.isExponential ? 0.02f : span / 100.0f;
            dial.fineStep = dial.isExponential ? 0.005f : span / 500.0f;
            dial.decimalPlaces = span > 200.0f ? 0 : span > 20.0f ? 1 : 2;
            dial.getValue = [readVar, def = d.def]()
            {
                const auto v = readVar();
                return v.isVoid() ? def : static_cast<float> (static_cast<double> (v));
            };
            dial.setValue = [&edit, ch, moduleType, id = &d.id] (float v) { edit.writeModule (ch, moduleType, *id, v); };
            break;
        }
    }
    return dial;
}

//==============================================================================
// Sub-tab 0: Channel Parameters
//==============================================================================

inline StreamDeckPage createChannelParametersPage (WFSValueTreeState& state, EffectParamEdit& edit, int ch,
                                                   std::shared_ptr<bool> soloEffects,
                                                   std::shared_ptr<bool> editOnMap,
                                                   const EffectsCallbacks& cb)
{
    using namespace WFSParameterIDs;
    using namespace WFSParameterDefaults;

    StreamDeckPage page ("Effects > Channel Parameters");
    const auto grey = juce::Colour (0xFF3A3A3A);

    // Section 0: Feed parameters (the Reverb page's shape, effect ids)
    {
        auto& sec = page.sections[0];
        sec.sectionName = LOC ("streamDeck.effects.sections.feedParameters");
        sec.sectionColour = juce::Colour (0xFF4A90D9);

        sec.buttons[1] = makeToggleButton (LOC ("streamDeck.outputs.buttons.minimalLatency"), grey, juce::Colour (0xFF4A90D9),
                                           state, edit, ch, effectMinimalLatency);
        sec.buttons[2] = makeToggleButton (LOC ("streamDeck.effects.buttons.feedMiniLatency"), grey, juce::Colour (0xFF4A90D9),
                                           state, edit, ch, effectFeedMiniLatency);

        sec.dials[0] = makeFloatDial (LOC ("streamDeck.outputs.dials.attenuation"), LOC ("units.decibels"),
                                      effectAttenuationMin, effectAttenuationMax, 0.5f, 0.1f, 1, false,
                                      state, edit, ch, effectAttenuation);
        sec.dials[0].barColour = juce::Colour (0xFF4A90D9);

        sec.dials[1] = makeFloatDial (LOC ("streamDeck.outputs.dials.delay"), LOC ("units.milliseconds"),
                                      effectDelayLatencyMin, effectDelayLatencyMax, 0.5f, 0.1f, 1, false,
                                      state, edit, ch, effectDelayLatency);
        sec.dials[1].barColour = juce::Colour (0xFFD4A017);
        sec.dials[1].getDynamicName = [&state, ch]()
        {
            const float v = static_cast<float> (state.getEffectParameter (ch, effectDelayLatency));
            return v < 0.0f ? LOC ("streamDeck.outputs.dials.latency") : LOC ("streamDeck.outputs.dials.delay");
        };

        sec.dials[2] = makeIntDial (LOC ("streamDeck.outputs.dials.distanceAttenuation"), LOC ("units.percent"),
                                    effectDistanceAttenPercentMin, effectDistanceAttenPercentMax, 2, 1,
                                    state, edit, ch, effectDistanceAttenPercent);
        sec.dials[2].barColour = juce::Colour (0xFF4A90D9);

        sec.dials[3] = makeFloatDial (LOC ("streamDeck.outputs.dials.hfDamping"), LOC ("units.decibels"),
                                      effectHFdampingMin, effectHFdampingMax, 0.5f, 0.1f, 1, false,
                                      state, edit, ch, effectHFdamping);
        sec.dials[3].barColour = juce::Colour (0xFFE07878);
    }

    // Section 1: Feed orientation
    {
        auto& sec = page.sections[1];
        sec.sectionName = LOC ("streamDeck.effects.sections.feedOrientation");
        sec.sectionColour = juce::Colour (0xFF26A69A);

        sec.dials[0] = makeIntDial (LOC ("streamDeck.outputs.dials.onAngle"), LOC ("units.degrees"),
                                    effectAngleOnMin, effectAngleOnMax, 2, 1, state, edit, ch, effectAngleOn);
        sec.dials[0].barColour = juce::Colour (0xFF4CAF50);
        sec.dials[1] = makeIntDial (LOC ("streamDeck.outputs.dials.offAngle"), LOC ("units.degrees"),
                                    effectAngleOffMin, effectAngleOffMax, 2, 1, state, edit, ch, effectAngleOff);
        sec.dials[1].barColour = juce::Colour (0xFFE53935);
        sec.dials[2] = makeIntDial (LOC ("streamDeck.outputs.dials.orientation"), LOC ("units.degrees"),
                                    effectOrientationMin, effectOrientationMax, 5, 1, state, edit, ch, effectOrientation);
        sec.dials[2].barColour = juce::Colour (0xFF26A69A);
        sec.dials[3] = makeIntDial (LOC ("streamDeck.outputs.dials.pitch"), LOC ("units.degrees"),
                                    effectPitchMin, effectPitchMax, 2, 1, state, edit, ch, effectPitch);
        sec.dials[3].barColour = juce::Colour (0xFF26A69A);
    }

    // Section 2: Return
    {
        auto& sec = page.sections[2];
        sec.sectionName = LOC ("streamDeck.effects.sections.effectReturn");
        sec.sectionColour = juce::Colour (0xFFCC8800);

        sec.buttons[0] = makeToggleButton (LOC ("streamDeck.effects.buttons.attenuationLaw"), grey, juce::Colour (0xFFCC8800),
                                           state, edit, ch, effectAttenuationLaw);

        sec.dials[0] = makeFloatDial (LOC ("streamDeck.outputs.dials.distanceAttenuation"), LOC ("units.decibels"),
                                      effectDistanceAttenuationMin, effectDistanceAttenuationMax, 0.5f, 0.1f, 1, false,
                                      state, edit, ch, effectDistanceAttenuation);
        sec.dials[0].barColour = juce::Colour (0xFF4A90D9);
        sec.dials[1] = makeFloatDial (LOC ("streamDeck.effects.dials.distanceRatio"), "x",
                                      effectDistanceRatioMin, effectDistanceRatioMax, 0.1f, 0.01f, 2, true,
                                      state, edit, ch, effectDistanceRatio);
        sec.dials[1].barColour = juce::Colour (0xFF4A90D9);
        sec.dials[2] = makeIntDial (LOC ("streamDeck.reverb.dials.commonAttenuation"), LOC ("units.percent"),
                                    effectCommonAttenMin, effectCommonAttenMax, 2, 1, state, edit, ch, effectCommonAtten);
        sec.dials[2].barColour = juce::Colour (0xFF4A90D9);
        sec.dials[3] = makeFloatDial (LOC ("streamDeck.effects.dials.hfShelf"), LOC ("units.decibels"),
                                      effectHFshelfMin, effectHFshelfMax, 0.5f, 0.1f, 1, false,
                                      state, edit, ch, effectHFshelf);
        sec.dials[3].barColour = juce::Colour (0xFFE07878);
    }

    // Section 3: Solo / Mute / Map
    {
        auto& sec = page.sections[3];
        sec.sectionName = LOC ("streamDeck.effects.sections.soloMute");
        sec.sectionColour = juce::Colour (0xFFCC8800);

        sec.buttons[0] = makeToggleButton (LOC ("streamDeck.effects.buttons.mute"), grey, juce::Colour (0xFFCC8800),
                                           state, edit, ch, effectMute);
        sec.buttons[1] = makeToggleButton (LOC ("streamDeck.effects.buttons.solo"), grey, juce::Colour (0xFFCC8800),
                                           state, edit, ch, effectSolo);

        {
            auto& btn = sec.buttons[2];
            btn.label = LOC ("streamDeck.effects.buttons.soloEffects");
            btn.colour = grey;
            btn.activeColour = juce::Colour (0xFFCC8800);
            btn.type = ButtonBinding::Toggle;
            btn.getState = [soloEffects]() { return soloEffects && *soloEffects; };
            btn.onPress = [soloEffects, cb]()
            {
                if (! soloEffects) return;
                *soloEffects = ! *soloEffects;
                if (cb.onSoloEffectsChanged) cb.onSoloEffectsChanged (*soloEffects);
            };
        }
        {
            auto& btn = sec.buttons[3];
            btn.label = LOC ("streamDeck.effects.buttons.editOnMap");
            btn.colour = grey;
            btn.activeColour = juce::Colour (0xFFCC8800);
            btn.type = ButtonBinding::Toggle;
            btn.getState = [editOnMap]() { return editOnMap && *editOnMap; };
            btn.onPress = [editOnMap, cb]()
            {
                if (! editOnMap) return;
                *editOnMap = ! *editOnMap;
                if (cb.onEditOnMapChanged) cb.onEditOnMapChanged (*editOnMap);
            };
        }
    }

    page.topRowNavigateToTab[3] = TabIndex::Map;
    page.topRowOverrideLabel[3] = LOC ("tabs.map");
    page.topRowOverrideColour[3] = juce::Colour (0xFF7B68EE);

    page.numSections = 4;
    page.activeSectionIndex = 0;
    return page;
}

//==============================================================================
// Sub-tab 1: Chain
//==============================================================================

/** The controls a Chain page shows for a slot, after the bypass: all of
    them, or for the reverb the ones its stored model uses. */
inline std::vector<const EffectsUi::ControlDesc*> chainPageControls (WFSValueTreeState& state, int ch, int slot)
{
    const auto controls = EffectsUi::controlsForSlot (slot);
    const bool reverb = slot == 8;
    const int model = reverb ? spatcore::effects::resolveReverbModel (static_cast<int> (
                                   state.getEffectModuleSection (ch, WFSParameterIDs::FxReverb)
                                        .getProperty (WFSParameterIDs::effectReverbModel, 0)))
                             : -1;

    std::vector<const EffectsUi::ControlDesc*> shown;
    for (int k = 1; k < controls.count; ++k)
        if (! reverb || EffectsUi::isVisibleForModel (controls.controls[k], model))
            shown.push_back (&controls.controls[k]);
    return shown;
}

/** A channel's processing order - the GUI's tile strip, left to right. An
    unreadable order falls back to the order a new channel is given. */
inline spatcore::effects::ChainOrder chainOrderOf (WFSValueTreeState& state, int ch)
{
    namespace fx = spatcore::effects;
    fx::ChainOrder order = fx::kDefaultOrder;
    const auto csv = state.getEffectChainSection (ch).getProperty (WFSParameterIDs::effectChainOrder).toString();
    if (! fx::parseChainOrder (csv.toRawUTF8(), order))
        fx::parseChainOrder (WFSParameterDefaults::effectChainOrderDefault.toRawUTF8(), order);
    return order;
}

/** The module `direction` places along the chain from `slot` (+1 next, -1
    previous), wrapping at the ends. */
inline int chainNeighbour (WFSValueTreeState& state, int ch, int slot, int direction)
{
    const auto order = chainOrderOf (state, ch);
    int position = 0;
    for (int p = 0; p < kNumSlots; ++p)
        if (order[static_cast<size_t> (p)] == slot)
            position = p;
    return order[static_cast<size_t> (((position + direction) % kNumSlots + kNumSlots) % kNumSlots)];
}

inline StreamDeckPage createChainPage (WFSValueTreeState& state, EffectParamEdit& edit, int ch,
                                       std::shared_ptr<int> chainSlot, std::shared_ptr<int> chainBank,
                                       const EffectsCallbacks& cb)
{
    using namespace WFSParameterIDs;

    StreamDeckPage page ("Effects > Chain");
    const auto grey = juce::Colour (0xFF3A3A3A);
    const int slot = juce::jlimit (0, kNumSlots - 1, chainSlot ? *chainSlot : 0);
    const auto& moduleType = WFSValueTreeState::getEffectModuleType (slot);
    const auto moduleName = LOC ("effects.modules." + juce::String (spatcore::effects::kSlots[static_cast<size_t> (slot)].token));
    const auto controls = EffectsUi::controlsForSlot (slot);
    const auto shown = chainPageControls (state, ch, slot);

    constexpr int perBank = 12;
    const int numBanks = juce::jmax (1, (static_cast<int> (shown.size()) + perBank - 1) / perBank);
    const int bank = juce::jlimit (0, numBanks - 1, chainBank ? *chainBank : 0);
    if (chainBank)
        *chainBank = bank;

    // Section 0: the chain - which module, the chain bypass, Clear
    {
        auto& sec = page.sections[0];
        sec.sectionName = LOC ("streamDeck.effects.sections.chain");
        sec.sectionColour = juce::Colour (0xFF9B59B6);

        auto selectSlot = [chainSlot, chainBank, cb] (int next)
        {
            if (! chainSlot) return;
            *chainSlot = juce::jlimit (0, kNumSlots - 1, next);
            if (chainBank) *chainBank = 0;
            if (cb.onChainSlotSelected) cb.onChainSlotSelected (*chainSlot);
        };

        {
            auto& btn = sec.buttons[0];
            btn.label = LOC ("streamDeck.effects.buttons.prevModule");
            btn.colour = grey;
            btn.type = ButtonBinding::Action;
            btn.requestsPageRebuild = true;
            // Along the chain as the strip shows it, read at the press, so a
            // reorder made while this page is up is already followed
            btn.onPress = [selectSlot, &state, ch, slot]() { selectSlot (chainNeighbour (state, ch, slot, -1)); };
        }
        {
            auto& btn = sec.buttons[1];
            btn.label = LOC ("streamDeck.effects.buttons.nextModule");
            btn.colour = grey;
            btn.type = ButtonBinding::Action;
            btn.requestsPageRebuild = true;
            btn.onPress = [selectSlot, &state, ch, slot]() { selectSlot (chainNeighbour (state, ch, slot, +1)); };
        }
        sec.buttons[2] = makeToggleButton (LOC ("streamDeck.effects.buttons.chainBypass"), grey, juce::Colour (0xFFCC8800),
                                           state, edit, ch, effectChainBypass);
        {
            auto& btn = sec.buttons[3];
            btn.label = LOC ("streamDeck.effects.buttons.clear");
            btn.colour = juce::Colour (0xFFE05555);
            btn.type = ButtonBinding::Action;
            btn.onPress = [cb, ch]() { if (cb.onClear) cb.onClear (ch); };
        }

        // Dial 0: the selected module's ON/OFF (its bypass, shown as the
        // module's state); dial 1: the module name, read-only
        if (controls.count > 0 && controls.controls[0].kind == EffectsUi::Kind::Bypass)
        {
            sec.dials[0] = makeModuleDial (controls.controls[0], moduleType, state, edit, ch);
            sec.dials[0].paramName = moduleName;
            sec.dials[0].barColour = juce::Colour (0xFF4CAF50);
        }
    }

    // Sections 1-3: the module's ordinary controls, four per section, in CSV
    // order after the bypass - this bank's twelve of them.
    const int resolvedModel = slot == 8
        ? spatcore::effects::resolveReverbModel (static_cast<int> (state.getEffectModuleSection (ch, FxReverb)
                                                                       .getProperty (effectReverbModel, 0)))
        : -1;
    int next = bank * perBank;
    for (int s = 1; s < 4; ++s)
    {
        auto& sec = page.sections[s];
        sec.sectionName = moduleName + "\n" + LOC (s == 1 ? "streamDeck.effects.sections.module"
                                                : s == 2 ? "streamDeck.effects.sections.moduleCont"
                                                         : "streamDeck.effects.sections.moduleCont2");
        sec.sectionColour = juce::Colour (0xFF9B59B6);

        for (int d = 0; d < 4 && next < static_cast<int> (shown.size()); ++d, ++next)
        {
            const auto& desc = *shown[static_cast<size_t> (next)];
            sec.dials[d] = makeModuleDial (desc, moduleType, state, edit, ch);
            sec.dials[d].barColour = juce::Colour (0xFF9B59B6);

            // The reverb's model, directly or through a preset, decides which
            // controls this page shows: when a deck turn moves it, the page is
            // laid out again - deferred, never from inside the dial's callback.
            if (slot == 8 && (desc.id == effectReverbModel || desc.id == effectReverbType))
            {
                auto write = sec.dials[d].setValue;
                sec.dials[d].setValue = [write, &state, ch, resolvedModel, cb] (float v)
                {
                    write (v);
                    const int now = spatcore::effects::resolveReverbModel (static_cast<int> (
                        state.getEffectModuleSection (ch, FxReverb).getProperty (effectReverbModel, 0)));
                    if (now != resolvedModel && cb.onModuleLayoutChanged)
                        cb.onModuleLayoutChanged();
                };
            }
        }

        if (numBanks > 1)
        {
            auto& btn = sec.buttons[3];
            btn.label = LOC ("streamDeck.effects.buttons.bank")
                            .replace ("{n}", juce::String (bank + 1))
                            .replace ("{count}", juce::String (numBanks));
            btn.colour = juce::Colour (0xFF9B59B6);
            btn.type = ButtonBinding::Action;
            btn.requestsPageRebuild = true;
            btn.onPress = [chainBank, bank, numBanks]() { if (chainBank) *chainBank = (bank + 1) % numBanks; };
        }
    }

    page.numSections = 4;
    page.activeSectionIndex = 0;
    return page;
}

//==============================================================================
// Sub-tab 2: Post-Processing (the sends matrix)
//==============================================================================

inline StreamDeckPage createSendsPage (const EffectsCallbacks& cb)
{
    StreamDeckPage page ("Effects > Post-Processing");
    const auto grey = juce::Colour (0xFF3A3A3A);

    // Section 0: move the selected cell
    {
        auto& sec = page.sections[0];
        sec.sectionName = LOC ("streamDeck.effects.sections.sends");
        sec.sectionColour = juce::Colour (0xFF26A69A);

        const juce::juce_wchar glyphs[4] = { 0x25B2, 0x25BC, 0x25C0, 0x25B6 };   // up down left right
        const int dx[4] = { 0, 0, -1, 1 };
        const int dy[4] = { -1, 1, 0, 0 };
        for (int i = 0; i < 4; ++i)
        {
            auto& btn = sec.buttons[i];
            btn.label = juce::String::charToString (glyphs[i]);
            btn.fontSize = 60.0f;
            btn.colour = grey;
            btn.type = ButtonBinding::Action;
            const int mx = dx[i], my = dy[i];
            btn.onPress = [cb, mx, my]() { if (cb.sendsMove) cb.sendsMove (mx, my); };
        }

        DialBinding level;
        level.paramName = LOC ("streamDeck.effects.dials.sendLevel");
        level.paramUnit = LOC ("units.decibels");
        level.minValue = WFSParameterDefaults::effectSendLevelMin;
        level.maxValue = WFSParameterDefaults::effectSendLevelMax;
        level.step = 1.0f;
        level.fineStep = 0.1f;
        level.decimalPlaces = 1;
        level.type = DialBinding::Float;
        level.getValue = [cb]() { return cb.sendsLevelDb ? cb.sendsLevelDb() : 0.0f; };
        level.setValue = [cb] (float v) { if (cb.sendsSetLevelDb) cb.sendsSetLevelDb (v); };
        level.onPress = [cb]() { if (cb.sendsToggle) cb.sendsToggle(); };
        level.barColour = juce::Colour (0xFF26A69A);
        sec.dials[0] = std::move (level);
    }

    // Section 1: the switch, and the whole column
    {
        auto& sec = page.sections[1];
        sec.sectionName = LOC ("streamDeck.effects.sections.sendsSwitch");
        sec.sectionColour = juce::Colour (0xFF26A69A);

        {
            auto& btn = sec.buttons[0];
            btn.label = LOC ("streamDeck.effects.buttons.sendToggle");
            btn.colour = grey;
            btn.type = ButtonBinding::Action;
            btn.onPress = [cb]() { if (cb.sendsToggle) cb.sendsToggle(); };
        }
        {
            auto& btn = sec.buttons[2];
            btn.label = LOC ("streamDeck.effects.buttons.sendsAllOn");
            btn.colour = grey;
            btn.type = ButtonBinding::Action;
            btn.onPress = [cb]() { if (cb.sendsSetAll) cb.sendsSetAll (true); };
        }
        {
            auto& btn = sec.buttons[3];
            btn.label = LOC ("streamDeck.effects.buttons.sendsAllOff");
            btn.colour = grey;
            btn.type = ButtonBinding::Action;
            btn.onPress = [cb]() { if (cb.sendsSetAll) cb.sendsSetAll (false); };
        }
    }

    page.numSections = 2;
    page.activeSectionIndex = 0;
    return page;
}

//==============================================================================
// Sub-tab 3: Movements (the Inputs Movements page minus gyrophone and Stay)
//==============================================================================

inline StreamDeckPage createMovementsPage (WFSValueTreeState& state, EffectParamEdit& edit, int ch,
                                           std::shared_ptr<int> lfoSubMode, const EffectsCallbacks& cb)
{
    using namespace WFSParameterIDs;
    using namespace WFSParameterDefaults;

    StreamDeckPage page ("Effects > Movements");
    const auto offGrey = juce::Colour (0xFF3A3A3A);
    const int currentSubMode = lfoSubMode ? *lfoSubMode : 0;

    // Section 0: LFO, with the X/Y/Z sub-modes on the buttons
    {
        auto& sec = page.sections[0];
        sec.sectionName = LOC ("streamDeck.inputs.sections.lfo");
        sec.sectionColour = juce::Colour (0xFF6A5ACD);

        const juce::String btnKeys[] = { "streamDeck.inputs.buttons.lfo", "streamDeck.inputs.buttons.lfoX",
                                         "streamDeck.inputs.buttons.lfoY", "streamDeck.inputs.buttons.lfoZ" };
        for (int i = 0; i < 4; ++i)
        {
            auto& btn = sec.buttons[i];
            btn.label = LOC (btnKeys[i]);
            btn.type = ButtonBinding::Toggle;
            btn.colour = offGrey;
            btn.activeColour = juce::Colour (0xFF6A5ACD);
            btn.requestsPageRebuild = true;
            const int idx = i;
            btn.getState = [lfoSubMode, idx]() { return lfoSubMode && *lfoSubMode == idx; };
            btn.onPress = [lfoSubMode, idx]() { if (lfoSubMode) *lfoSubMode = idx; };
        }

        if (currentSubMode == 0)
        {
            DialBinding d;
            d.paramName = LOC ("streamDeck.inputs.dials.lfoToggle");
            d.type = DialBinding::ComboBox;
            d.comboOptions = { "OFF", "ON" };
            d.minValue = 0.0f;
            d.maxValue = 1.0f;
            d.getValue = [&state, ch]() { return static_cast<float> (static_cast<int> (state.getEffectParameter (ch, effectLFOactive))); };
            d.setValue = [&edit, ch] (float v) { edit.write (ch, effectLFOactive, juce::roundToInt (v)); };
            d.onPress = [&state, &edit, ch]()
            {
                const int current = static_cast<int> (state.getEffectParameter (ch, effectLFOactive));
                edit.write (ch, effectLFOactive, current != 0 ? 0 : 1);
            };
            sec.dials[0] = std::move (d);

            sec.dials[1] = makeFloatDial (LOC ("streamDeck.inputs.dials.lfoPeriod"), LOC ("units.seconds"),
                                          effectLFOperiodMin, effectLFOperiodMax, 0.02f, 0.005f, 2, true,
                                          state, edit, ch, effectLFOperiod);
            sec.dials[1].barColour = juce::Colour (0xFFD4A017);
            sec.dials[2] = makeIntDial (LOC ("streamDeck.inputs.dials.lfoPhase"), LOC ("units.degrees"),
                                        effectLFOphaseMin, effectLFOphaseMax, 5, 1, state, edit, ch, effectLFOphase);
            sec.dials[2].barColour = juce::Colour (0xFF9B59B6);
        }
        else
        {
            const juce::Identifier shapeIds[]     = { effectLFOshapeX, effectLFOshapeY, effectLFOshapeZ };
            const juce::Identifier amplitudeIds[] = { effectLFOamplitudeX, effectLFOamplitudeY, effectLFOamplitudeZ };
            const juce::Identifier rateIds[]      = { effectLFOrateX, effectLFOrateY, effectLFOrateZ };
            const juce::Identifier phaseIds[]     = { effectLFOphaseX, effectLFOphaseY, effectLFOphaseZ };
            const int axis = juce::jlimit (0, 2, currentSubMode - 1);

            DialBinding shape;
            shape.paramName = LOC ("streamDeck.inputs.dials.lfoShape");
            shape.type = DialBinding::ComboBox;
            for (const char* key : { "off", "sine", "square", "sawtooth", "triangle", "keystone", "log", "exp", "random" })
                shape.comboOptions.add (LOC (juce::String ("effects.lfo.shapes.") + key));
            shape.minValue = static_cast<float> (effectLFOshapeMin);
            shape.maxValue = static_cast<float> (effectLFOshapeMax);
            const auto shapeId = shapeIds[axis];
            shape.getValue = [&state, ch, shapeId]() { return static_cast<float> (static_cast<int> (state.getEffectParameter (ch, shapeId))); };
            shape.setValue = [&edit, ch, shapeId] (float v) { edit.write (ch, shapeId, juce::roundToInt (v)); };
            sec.dials[0] = std::move (shape);

            sec.dials[1] = makeFloatDial (LOC ("streamDeck.inputs.dials.lfoAmplitude"), LOC ("units.meters"),
                                          effectLFOamplitudeMin, effectLFOamplitudeMax, 0.5f, 0.1f, 1, false,
                                          state, edit, ch, amplitudeIds[axis]);
            sec.dials[1].barColour = juce::Colour (0xFF26A69A);
            sec.dials[2] = makeFloatDial (LOC ("streamDeck.inputs.dials.lfoRate"), "x",
                                          effectLFOrateMin, effectLFOrateMax, 0.02f, 0.005f, 2, true,
                                          state, edit, ch, rateIds[axis]);
            sec.dials[2].barColour = juce::Colour (0xFFD4A017);
            sec.dials[3] = makeIntDial (LOC ("streamDeck.inputs.dials.lfoAxisPhase"), LOC ("units.degrees"),
                                        effectLFOphaseMin, effectLFOphaseMax, 5, 1, state, edit, ch, phaseIds[axis]);
            sec.dials[3].barColour = juce::Colour (0xFF9B59B6);
        }
    }

    // Section 1: AutomOtion destination
    {
        auto& sec = page.sections[1];
        sec.sectionName = LOC ("streamDeck.inputs.sections.automOtionPosition");
        sec.sectionColour = juce::Colour (0xFF26A69A);

        sec.buttons[0] = makeToggleButton (LOC ("streamDeck.inputs.buttons.absRel"), offGrey, juce::Colour (0xFF26A69A),
                                           state, edit, ch, effectOtomoAbsoluteRelative);

        const int coordMode = juce::jlimit (0, 2, static_cast<int> (state.getEffectParameter (ch, effectOtomoCoordinateMode)));
        const juce::Identifier pids0[] = { effectOtomoX, effectOtomoR, effectOtomoRsph };
        const juce::Identifier pids1[] = { effectOtomoY, effectOtomoTheta, effectOtomoTheta };
        const juce::Identifier pids2[] = { effectOtomoZ, effectOtomoZ, effectOtomoPhi };
        const float mins0[] = { effectOtomoMin, effectOtomoRMin, effectOtomoRsphMin };
        const float maxs0[] = { effectOtomoMax, effectOtomoRMax, effectOtomoRsphMax };
        const float mins1[] = { effectOtomoMin, effectOtomoThetaMin, effectOtomoThetaMin };
        const float maxs1[] = { effectOtomoMax, effectOtomoThetaMax, effectOtomoThetaMax };
        const float mins2[] = { effectOtomoMin, effectOtomoMin, effectOtomoPhiMin };
        const float maxs2[] = { effectOtomoMax, effectOtomoMax, effectOtomoPhiMax };
        const bool angular1 = coordMode != 0, angular2 = coordMode == 2;

        auto dest = [&] (int d, const juce::Identifier& pid, float mn, float mx, bool angular, const juce::String& name)
        {
            DialBinding dial;
            dial.type = DialBinding::Float;
            dial.minValue = mn;
            dial.maxValue = mx;
            dial.step = angular ? 10.0f : 0.5f;
            dial.fineStep = angular ? 1.0f : 0.1f;
            dial.decimalPlaces = angular ? 1 : 2;
            dial.paramUnit = angular ? LOC ("units.degrees") : LOC ("units.meters");
            dial.paramName = name;
            dial.getValue = [&state, ch, pid]() { return static_cast<float> (state.getEffectParameter (ch, pid)); };
            dial.setValue = [&edit, ch, pid] (float v) { edit.write (ch, pid, v); };
            dial.barColour = juce::Colour (0xFF4CAF50);
            sec.dials[d] = std::move (dial);
        };
        dest (0, pids0[coordMode], mins0[coordMode], maxs0[coordMode], false, coordMode != 0 ? juce::String ("Radius") : juce::String ("Dest X"));
        dest (1, pids1[coordMode], mins1[coordMode], maxs1[coordMode], angular1, angular1 ? juce::String ("Azimuth") : juce::String ("Dest Y"));
        dest (2, pids2[coordMode], mins2[coordMode], maxs2[coordMode], angular2, angular2 ? juce::String ("Elevation") : juce::String ("Dest Z"));
    }

    // Section 2: AutomOtion continued
    {
        auto& sec = page.sections[2];
        sec.sectionName = LOC ("streamDeck.inputs.sections.automOtionContinued");
        sec.sectionColour = juce::Colour (0xFFD4A843);

        sec.buttons[3] = makeToggleButton (LOC ("streamDeck.inputs.buttons.manualTriggered"), offGrey, juce::Colour (0xFFD4A843),
                                           state, edit, ch, effectOtomoTrigger);

        sec.dials[0] = makeFloatDial (LOC ("streamDeck.inputs.dials.duration"), LOC ("units.seconds"),
                                      effectOtomoDurationMin, effectOtomoDurationMax, 0.02f, 0.005f, 1, true,
                                      state, edit, ch, effectOtomoDuration);
        sec.dials[0].barColour = juce::Colour (0xFFD4A017);
        sec.dials[1] = makeIntDial (LOC ("streamDeck.inputs.dials.curve"), LOC ("units.percent"),
                                    effectOtomoCurveMin, effectOtomoCurveMax, 2, 1, state, edit, ch, effectOtomoCurve);
        sec.dials[1].barColour = juce::Colour (0xFF00ACC1);
        sec.dials[2] = makeIntDial (LOC ("streamDeck.inputs.dials.speedProfile"), LOC ("units.percent"),
                                    effectOtomoSpeedProfileMin, effectOtomoSpeedProfileMax, 2, 1, state, edit, ch, effectOtomoSpeedProfile);
        sec.dials[2].barColour = juce::Colour (0xFF00ACC1);
        sec.dials[3] = makeFloatDial (LOC ("streamDeck.inputs.dials.triggerThreshold"), LOC ("units.decibels"),
                                      effectOtomoThresholdMin, effectOtomoThresholdMax, 1.0f, 0.25f, 1, false,
                                      state, edit, ch, effectOtomoThreshold);
        sec.dials[3].barColour = juce::Colour (0xFF4A90D9);
        sec.dials[3].altBinding = std::make_unique<DialBinding> (
            makeFloatDial (LOC ("streamDeck.inputs.dials.triggerReset"), LOC ("units.decibels"),
                           effectOtomoResetMin, effectOtomoResetMax, 1.0f, 0.25f, 1, false,
                           state, edit, ch, effectOtomoReset));
        sec.dials[3].altBinding->barColour = juce::Colour (0xFF4A90D9);
    }

    // Section 3: transport
    {
        auto& sec = page.sections[3];
        sec.sectionName = LOC ("streamDeck.inputs.sections.automOtionManual");
        sec.sectionColour = juce::Colour (0xFFE05555);

        {
            auto& btn = sec.buttons[0];
            btn.label = juce::String::charToString (0x25B6);
            btn.colour = offGrey;
            btn.fontSize = 70.0f;
            btn.type = ButtonBinding::Action;
            btn.onPress = [cb, ch]() { if (cb.startMotion) cb.startMotion (ch); };
        }
        {
            auto& btn = sec.buttons[1];
            btn.label = juce::String::charToString (0x275A) + juce::String::charToString (0x275A);
            btn.colour = offGrey;
            btn.fontSize = 70.0f;
            btn.activeColour = juce::Colour (0xFFD4A843);
            btn.type = ButtonBinding::Toggle;
            btn.getState = [&state, ch]() { return static_cast<int> (state.getEffectParameter (ch, effectOtomoPauseResume)) == 0; };
            btn.onPress = [&state, &edit, ch, cb]()
            {
                const int current = static_cast<int> (state.getEffectParameter (ch, effectOtomoPauseResume));
                if (current == 0)
                {
                    edit.write (ch, effectOtomoPauseResume, 1);
                    if (cb.resumeMotion) cb.resumeMotion (ch);
                }
                else
                {
                    edit.write (ch, effectOtomoPauseResume, 0);
                    if (cb.pauseMotion) cb.pauseMotion (ch);
                }
            };
        }
        {
            auto& btn = sec.buttons[2];
            btn.label = juce::String::charToString (0x25A0);
            btn.colour = offGrey;
            btn.fontSize = 70.0f;
            btn.type = ButtonBinding::Action;
            btn.onPress = [cb, ch]() { if (cb.stopMotion) cb.stopMotion (ch); };
        }
        {
            auto& btn = sec.buttons[3];
            btn.label = LOC ("streamDeck.inputs.buttons.stopAll");
            btn.colour = juce::Colour (0xFFE05555);
            btn.type = ButtonBinding::Action;
            btn.onPress = [cb]() { if (cb.stopAll) cb.stopAll(); };
        }
    }

    page.numSections = 4;
    page.activeSectionIndex = 0;
    return page;
}

//==============================================================================
// Sub-tab 4: Settings (the nine globals; config, so the generic setter)
//==============================================================================

inline StreamDeckPage createSettingsPage (WFSValueTreeState& state, const EffectsCallbacks& cb)
{
    using namespace WFSParameterIDs;
    using namespace WFSParameterDefaults;

    StreamDeckPage page ("Effects > Settings");
    const auto grey = juce::Colour (0xFF3A3A3A);

    auto readGlobal = [&state] (const juce::Identifier& id, float fallback)
    {
        const auto v = state.getEffectsGlobalSection().getProperty (id);
        return v.isVoid() ? fallback : static_cast<float> (static_cast<double> (v));
    };

    auto globalToggle = [&] (const juce::String& label, const juce::Identifier& id, int def, juce::Colour on)
    {
        ButtonBinding btn;
        btn.label = label;
        btn.colour = grey;
        btn.activeColour = on;
        btn.type = ButtonBinding::Toggle;
        btn.getState = [&state, id, def]() { return static_cast<int> (state.getEffectsGlobalSection().getProperty (id, def)) != 0; };
        btn.onPress = [&state, id, def]()
        {
            const int current = static_cast<int> (state.getEffectsGlobalSection().getProperty (id, def));
            state.setParameter (id, current != 0 ? 0 : 1);
        };
        return btn;
    };

    auto globalIntDial = [&] (const juce::String& name, const juce::String& unit, int mn, int mx, const juce::Identifier& id, int def)
    {
        DialBinding dial;
        dial.paramName = name;
        dial.paramUnit = unit;
        dial.minValue = static_cast<float> (mn);
        dial.maxValue = static_cast<float> (mx);
        dial.step = 1.0f;
        dial.fineStep = 1.0f;
        dial.type = DialBinding::Int;
        dial.getValue = [readGlobal, id, def]() { return readGlobal (id, static_cast<float> (def)); };
        dial.setValue = [&state, id] (float v) { state.setParameter (id, juce::roundToInt (v)); };
        return dial;
    };

    // Section 0: link and guard
    {
        auto& sec = page.sections[0];
        sec.sectionName = LOC ("streamDeck.effects.sections.settings");
        sec.sectionColour = juce::Colour (0xFF3A6EA5);

        sec.buttons[0] = globalToggle (LOC ("streamDeck.effects.buttons.loopGuard"), effectsGlobalLoopGuard,
                                       effectsGlobalLoopGuardDefault, juce::Colour (0xFF26A69A));
        sec.buttons[1] = globalToggle (LOC ("streamDeck.effects.buttons.fxFeed"), effectsGlobalFxFeedGeometric,
                                       effectsGlobalFxFeedGeometricDefault, juce::Colour (0xFF26A69A));
        {
            auto& btn = sec.buttons[3];
            btn.label = LOC ("streamDeck.effects.buttons.relayout");
            btn.colour = juce::Colour (0xFF3A6EA5);
            btn.type = ButtonBinding::Action;
            btn.onPress = [cb]() { if (cb.onRelayout) cb.onRelayout(); };
        }

        DialBinding mode;
        mode.paramName = LOC ("streamDeck.effects.dials.linkMode");
        mode.type = DialBinding::ComboBox;
        mode.comboOptions = { LOC ("effects.link.modeOff"), LOC ("effects.link.modeAbsolute"), LOC ("effects.link.modeRelative") };
        mode.minValue = 0.0f;
        mode.maxValue = 2.0f;
        mode.getValue = [readGlobal]() { return readGlobal (effectsGlobalLinkMode, static_cast<float> (effectsGlobalLinkModeDefault)); };
        mode.setValue = [&state] (float v) { state.setParameter (effectsGlobalLinkMode, juce::jlimit (0, 2, juce::roundToInt (v))); };
        sec.dials[0] = std::move (mode);

        DialBinding ceiling;
        ceiling.paramName = LOC ("streamDeck.effects.dials.loopCeiling");
        ceiling.paramUnit = LOC ("units.decibels");
        ceiling.minValue = effectsGlobalLoopGuardCeilingMin;
        ceiling.maxValue = effectsGlobalLoopGuardCeilingMax;
        ceiling.step = 0.5f;
        ceiling.fineStep = 0.1f;
        ceiling.decimalPlaces = 1;
        ceiling.type = DialBinding::Float;
        ceiling.getValue = [readGlobal]() { return readGlobal (effectsGlobalLoopGuardCeiling, effectsGlobalLoopGuardCeilingDefault); };
        ceiling.setValue = [&state] (float v) { state.setParameter (effectsGlobalLoopGuardCeiling, v); };
        ceiling.barColour = juce::Colour (0xFFFF5722);
        sec.dials[1] = std::move (ceiling);

        sec.dials[2] = globalIntDial (LOC ("streamDeck.effects.dials.maxDelay"), LOC ("units.seconds"),
                                      effectsGlobalMaxDelaySecondsMin, effectsGlobalMaxDelaySecondsMax,
                                      effectsGlobalMaxDelaySeconds, effectsGlobalMaxDelaySecondsDefault);
        sec.dials[2].barColour = juce::Colour (0xFFD4A017);
    }

    // Section 1: engine
    {
        auto& sec = page.sections[1];
        sec.sectionName = LOC ("streamDeck.effects.sections.engine");
        sec.sectionColour = juce::Colour (0xFF3A6EA5);

        sec.dials[0] = globalIntDial (LOC ("streamDeck.effects.dials.workerThreads"), juce::String(),
                                      effectsGlobalWorkerThreadsMin, effectsGlobalWorkerThreadsMax,
                                      effectsGlobalWorkerThreads, effectsGlobalWorkerThreadsDefault);

        DialBinding cushion;
        cushion.paramName = LOC ("streamDeck.effects.dials.returnCushion");
        cushion.type = DialBinding::ComboBox;
        cushion.comboOptions = { LOC ("effects.settings.cushion.auto"), LOC ("effects.settings.cushion.n1"),
                                 LOC ("effects.settings.cushion.n2"), LOC ("effects.settings.cushion.n3") };
        cushion.minValue = 0.0f;
        cushion.maxValue = 3.0f;
        cushion.getValue = [readGlobal]() { return readGlobal (effectsGlobalReturnCushion, static_cast<float> (effectsGlobalReturnCushionDefault)); };
        cushion.setValue = [&state] (float v) { state.setParameter (effectsGlobalReturnCushion, juce::jlimit (0, 3, juce::roundToInt (v))); };
        sec.dials[1] = std::move (cushion);
    }

    page.numSections = 2;
    page.activeSectionIndex = 0;
    return page;
}

//==============================================================================
inline StreamDeckPage createPage (int subTabIndex,
                                  WFSValueTreeState& state,
                                  EffectParamEdit& edit,
                                  int channelIndex,
                                  std::shared_ptr<bool> soloEffects,
                                  std::shared_ptr<bool> editOnMap,
                                  std::shared_ptr<int> lfoSubMode,
                                  std::shared_ptr<int> chainSlot,
                                  std::shared_ptr<int> chainBank,
                                  const EffectsCallbacks& cb = {})
{
    if (state.getNumEffectChannels() == 0)
    {
        StreamDeckPage page (LOC ("streamDeck.effects.noChannels"));
        page.activeSectionIndex = -1;
        page.lcdMessage = LOC ("streamDeck.effects.noChannels");
        return page;
    }

    const int ch = juce::jlimit (0, state.getNumEffectChannels() - 1, channelIndex);

    switch (subTabIndex)
    {
        case 0:  return createChannelParametersPage (state, edit, ch, soloEffects, editOnMap, cb);
        case 1:  return createChainPage (state, edit, ch, chainSlot, chainBank, cb);
        case 2:  return createSendsPage (cb);
        case 3:  return createMovementsPage (state, edit, ch, lfoSubMode, cb);
        case 4:  return createSettingsPage (state, cb);
        default: return StreamDeckPage ("Effects > Unknown");
    }
}

} // namespace EffectsTabPages
