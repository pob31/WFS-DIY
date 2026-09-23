#pragma once

#include <JuceHeader.h>
#include "EffectsTabContext.h"
#include "EffectsModuleDescriptors.h"
#include "../ColorScheme.h"
#include "../EQDisplayComponent.h"
#include "../GainReductionMeter.h"
#include "../buttons/EQBandToggle.h"
#include "../buttons/LongPressButton.h"
#include "../dials/WfsBasicDial.h"
#include "../dials/WfsRotationDial.h"
#include "../sliders/WfsStandardSlider.h"
#include "../sliders/WfsBidirectionalSlider.h"
#include "../../Accessibility/TTSManager.h"
#include "../../Parameters/WFSParameterIDs.h"
#include "../../Parameters/WFSParameterDefaults.h"
#include "../../Localization/LocalizationManager.h"
#include "../../../spatcore/effects/EffectsTypes.h"
#include "../../../spatcore/effects/EffectPresets.h"

/**
    One module's parameters, for one declared chain slot.

    DRIVEN BY THE CSV. Every ordinary control - slider, log slider, dial,
    combo, toggle, bypass - is built from the generated descriptor of its CSV
    row (EffectsModuleDescriptors.h): kind, range, default, unit and enum,
    with the label and the hover text under the same key in en.json. Adding
    a row to the CSV and re-running the generator adds the control; nothing
    here names a parameter of the ordinary kind.

    FOUR MODULES HAVE SOMETHING THE CSV CANNOT DESCRIBE, and only those are
    special-cased: the two EQ instances (the interactive display, the six
    band strips), the two Dynamics instances (the gain-reduction meter), the
    delay (its eight tap rows) and the reverb (its presets: selecting a type
    other than Custom WRITES the preset's eight values through the funnel,
    because the engine applies no preset itself, and editing one of those
    eight afterwards flips the type back to Custom).

    EVERY WRITE GOES THROUGH THE CONTEXT, so a linked group receives it
    according to each member's mode and Ctrl-drag edits this channel alone.
*/
class EffectsModulePanel : public juce::Component
{
public:
    static constexpr int numEqBands = WFSParameterDefaults::numEffectEQBands;
    static constexpr int numTaps    = WFSParameterDefaults::numEffectDelayTaps;

    EffectsModulePanel (EffectsTabContext& context, int slotIndex)
        : ctx (context), slot (slotIndex),
          node (WFSValueTreeState::getEffectModuleType (slotIndex)),
          token (spatcore::effects::kSlots[static_cast<size_t> (slotIndex)].token)
    {
        const auto list = EffectsUi::controlsForSlot (slot);
        for (int i = 0; i < list.count; ++i)
            addRow (list.controls[i]);

        if (isEq())    setupEq();
        if (isDyn())   setupDyn();
        if (isDelay()) setupDelay();
    }

    int getSlot() const noexcept { return slot; }
    bool isEq()    const noexcept { return slot == 1 || slot == 2; }
    bool isDyn()   const noexcept { return slot == 3 || slot == 4; }
    bool isDelay() const noexcept { return slot == 9; }
    bool isReverb() const noexcept { return slot == 8; }
    int eqInstance() const noexcept { return slot == 2 ? 1 : 0; }

    /** Re-read every control for the context's channel. */
    void loadParameters()
    {
        for (auto& row : rows)
            loadRow (*row);

        if (isEq())    loadEq();
        if (isDelay()) refreshTapDimming();
        if (isReverb()) refreshPresetDimming();
    }

    /** The engine's per-slot meter (gain reduction for Dynamics, output peak otherwise). */
    void setMeterDb (float db)
    {
        // Below -60 dB is the "no engine" sentinel, not 60 dB of reduction
        if (grMeter != nullptr)
            grMeter->setGainReductionDb (db < -60.0f ? 0.0f : juce::jmin (0.0f, db));
    }

    void resized() override
    {
        layoutScale = static_cast<float> (getHeight()) / 640.0f;
        auto area = getLocalBounds().reduced (scaled (8), scaled (6));

        if (isEq())
        {
            layoutEq (area);
            return;
        }

        if (isDyn())
        {
            auto meterArea = area.removeFromRight (scaled (56));
            meterArea.removeFromTop (scaled (18));
            grLabel.setBounds (meterArea.removeFromTop (scaled (18)));
            grMeter->setBounds (meterArea.reduced (scaled (14), scaled (6)));
            area.removeFromRight (scaled (10));
        }

        if (isDelay())
        {
            // The tap table takes the lower part; the module rows the upper.
            const int tapRow = scaled (24);
            auto tapArea = area.removeFromBottom (tapRow * (numTaps + 1) + scaled (8));
            layoutTaps (tapArea);
            area.removeFromBottom (scaled (8));
        }

        layoutRows (area);
    }

private:
    //==========================================================================
    // Ordinary controls, one row each
    //==========================================================================

    struct Row
    {
        const EffectsUi::ControlDesc* desc = nullptr;
        juce::Label label, value;
        std::unique_ptr<juce::Component> control;
        WfsSliderBase*    slider   = nullptr;
        WfsBasicDial*     dial     = nullptr;
        juce::ComboBox*   combo    = nullptr;
        juce::TextButton* button   = nullptr;
        WfsRotationDial*  rotation = nullptr;
        bool presetOwned = false;      // reverb: one of the eight a preset writes
    };

    void addRow (const EffectsUi::ControlDesc& d)
    {
        using EffectsUi::Kind;
        auto row = std::make_unique<Row>();
        row->desc = &d;
        const juce::String key (d.key);

        addAndMakeVisible (row->label);
        row->label.setText (LOC ("effects.labels." + key), juce::dontSendNotification);
        row->label.setJustificationType (juce::Justification::centredLeft);

        auto* r = row.get();
        switch (d.kind)
        {
            case Kind::Bypass:
            {
                auto* b = new juce::TextButton();
                row->control.reset (b);
                row->button = b;
                b->onClick = [this, r]
                {
                    // The stored value is the BYPASS (1 = module off); the
                    // button shows and toggles the module's ON state.
                    const bool currentlyOn = readInt (*r->desc) == 0;
                    ctx.writeModule (node, r->desc->id, currentlyOn ? 1 : 0);
                    loadRow (*r);
                };
                break;
            }
            case Kind::Toggle:
            {
                auto* b = new juce::TextButton();
                row->control.reset (b);
                row->button = b;
                b->onClick = [this, r]
                {
                    const auto& items = r->desc->items;
                    const int current = readInt (*r->desc);
                    const int next = items.size() >= 2 && current == items[0].value ? items[1].value : items[0].value;
                    presetGuard (*r);
                    ctx.writeModule (node, r->desc->id, next);
                    loadRow (*r);
                };
                break;
            }
            case Kind::Combo:
            {
                auto* c = new juce::ComboBox();
                row->control.reset (c);
                row->combo = c;
                for (const auto& item : d.items)
                    c->addItem (LOC (juce::String (d.enumPrefix) + item.slug), item.value + 1);
                c->onChange = [this, r]
                {
                    if (ctx.isLoadingParameters) return;
                    const int value = r->combo->getSelectedId() - 1;
                    if (isReverb() && r->desc->id == WFSParameterIDs::effectReverbType)
                    {
                        applyReverbPreset (value);
                        return;
                    }
                    presetGuard (*r);
                    ctx.writeModule (node, r->desc->id, value);
                    TTSManager::getInstance().announceValueChange (r->label.getText(), r->combo->getText());
                };
                break;
            }
            case Kind::Rotation:
            {
                auto* dial = new WfsRotationDial();
                row->control.reset (dial);
                row->rotation = dial;
                dial->setColours (juce::Colour (0xFF3A3A3A), juce::Colour (0xFFE6B422), juce::Colours::white);
                dial->onGestureStart = [this, r] { ctx.beginGesture ("Effect " + r->label.getText()); };
                dial->onAngleChanged = [this, r] (float angle)
                {
                    const float deg = angle < 0.0f ? angle + 360.0f : angle;     // stored 0..360
                    r->value.setText (formatValue (deg, r->desc->unit), juce::dontSendNotification);
                    ctx.writeModule (node, r->desc->id, deg);
                };
                break;
            }
            case Kind::Dial:
            {
                auto* dial = new WfsBasicDial();
                row->control.reset (dial);
                row->dial = dial;
                dial->setColours (juce::Colours::black, moduleColour(), juce::Colours::grey);
                dial->setTrackColours (ColorScheme::get().sliderTrackBg, moduleColour());
                if (d.min < 0.0f && d.max > 0.0f)
                    dial->setBipolar (true);
                dial->onGestureStart = [this, r] { ctx.beginGesture ("Effect " + r->label.getText()); };
                dial->onValueChanged = [this, r] (float v) { commitNormalised (*r, v); };
                break;
            }
            case Kind::BiSlider:
            {
                auto* s = new WfsBidirectionalSlider();
                row->control.reset (s);
                row->slider = s;
                s->setTrackColours (juce::Colour (0xFF1E1E1E), moduleColour());
                s->onGestureStart = [this, r] { ctx.beginGesture ("Effect " + r->label.getText()); };
                s->onValueChanged = [this, r] (float v) { commitNormalised (*r, v); };
                break;
            }
            case Kind::Slider:
            case Kind::LogSlider:
            {
                auto* s = new WfsStandardSlider();
                row->control.reset (s);
                row->slider = s;
                s->setTrackColours (juce::Colour (0xFF1E1E1E), moduleColour());
                s->onGestureStart = [this, r] { ctx.beginGesture ("Effect " + r->label.getText()); };
                s->onValueChanged = [this, r] (float v) { commitNormalised (*r, v); };
                break;
            }
        }

        addAndMakeVisible (*row->control);
        addAndMakeVisible (row->value);
        row->value.setJustificationType (juce::Justification::centredRight);
        ctx.helpTextMap[row->control.get()] = LOC ("effects.help." + key);

        if (isReverb())
            row->presetOwned = isPresetOwned (d.id);

        rows.push_back (std::move (row));
    }

    //--------------------------------------------------------------------------
    // Value laws: the CSV's formula column, min+(max-min)*x or min*(max/min)^x
    static float realFromNormalised (const EffectsUi::ControlDesc& d, float x)
    {
        using EffectsUi::Kind;
        switch (d.kind)
        {
            case Kind::LogSlider: return d.min * std::pow (d.max / d.min, juce::jlimit (0.0f, 1.0f, x));
            case Kind::BiSlider:  return (d.min + d.max) * 0.5f + juce::jlimit (-1.0f, 1.0f, x) * (d.max - d.min) * 0.5f;
            default:              return d.min + (d.max - d.min) * juce::jlimit (0.0f, 1.0f, x);
        }
    }

    static float normalisedFromReal (const EffectsUi::ControlDesc& d, float v)
    {
        using EffectsUi::Kind;
        v = juce::jlimit (d.min, d.max, v);
        switch (d.kind)
        {
            case Kind::LogSlider: return d.max > d.min && d.min > 0.0f ? std::log (v / d.min) / std::log (d.max / d.min) : 0.0f;
            case Kind::BiSlider:  return d.max > d.min ? (v - (d.min + d.max) * 0.5f) / ((d.max - d.min) * 0.5f) : 0.0f;
            default:              return d.max > d.min ? (v - d.min) / (d.max - d.min) : 0.0f;
        }
    }

    static juce::String formatValue (float v, const juce::String& unit)
    {
        if (unit == "Hz")
            return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " kHz" : juce::String (v, v < 100.0f ? 1 : 0) + " Hz";
        if (unit == "ms")   return juce::String (v, v < 10.0f ? 2 : v < 100.0f ? 1 : 0) + " ms";
        if (unit == "dB")   return juce::String (v, 1) + " dB";
        if (unit == "%")    return juce::String (juce::roundToInt (v)) + " %";
        if (unit == "s")    return juce::String (v, 2) + " s";
        if (unit == ":1")   return juce::String (v, 1) + ":1";
        if (unit == "x")    return juce::String (v, 2) + "x";
        if (unit == "oct")  return juce::String (v, 2) + " oct";
        if (unit == "bits") return juce::String (v, 1) + " bits";
        if (unit == juce::String::fromUTF8 ("\xc2\xb0")) return juce::String (juce::roundToInt (v)) + juce::String::charToString (0x00B0);
        return juce::String (v, 2);
    }

    void commitNormalised (Row& r, float x)
    {
        const float real = realFromNormalised (*r.desc, x);
        r.value.setText (formatValue (real, r.desc->unit), juce::dontSendNotification);
        if (ctx.isLoadingParameters)
            return;
        presetGuard (r);
        ctx.writeModule (node, r.desc->id, real);
    }

    juce::var readVar (const EffectsUi::ControlDesc& d) const
    {
        return ctx.readModule (node, d.id);
    }

    int readInt (const EffectsUi::ControlDesc& d) const
    {
        const auto v = readVar (d);
        return v.isVoid() ? juce::roundToInt (d.def) : static_cast<int> (v);
    }

    float readFloat (const EffectsUi::ControlDesc& d) const
    {
        const auto v = readVar (d);
        return v.isVoid() ? d.def : static_cast<float> (static_cast<double> (v));
    }

    void loadRow (Row& r)
    {
        using EffectsUi::Kind;
        const auto& d = *r.desc;

        switch (d.kind)
        {
            case Kind::Bypass:
            {
                const bool on = readInt (d) == 0;
                r.button->setButtonText (moduleName() + ": " + LOC (on ? "effects.chain.moduleOn" : "effects.chain.moduleOff"));
                r.button->setColour (juce::TextButton::buttonColourId,
                                     on ? juce::Colour (0xFF26A69A) : ColorScheme::get().buttonNormal);
                r.label.setText (juce::String(), juce::dontSendNotification);
                break;
            }
            case Kind::Toggle:
            {
                const int value = readInt (d);
                juce::String name;
                for (const auto& item : d.items)
                    if (item.value == value)
                        name = LOC (juce::String (d.enumPrefix) + item.slug);
                r.button->setButtonText (name);
                const bool second = d.items.size() >= 2 && value == d.items[1].value;
                r.button->setColour (juce::TextButton::buttonColourId,
                                     second ? juce::Colour (0xFF3A6EA5) : ColorScheme::get().buttonNormal);
                break;
            }
            case Kind::Combo:
                r.combo->setSelectedId (readInt (d) + 1, juce::dontSendNotification);
                break;
            case Kind::Rotation:
            {
                const float deg = readFloat (d);
                r.rotation->setAngle (deg > 180.0f ? deg - 360.0f : deg);
                r.value.setText (formatValue (deg, d.unit), juce::dontSendNotification);
                break;
            }
            case Kind::Dial:
            {
                const float real = readFloat (d);
                r.dial->setValue (normalisedFromReal (d, real));
                r.value.setText (formatValue (real, d.unit), juce::dontSendNotification);
                break;
            }
            case Kind::BiSlider:
            case Kind::Slider:
            case Kind::LogSlider:
            {
                const float real = readFloat (d);
                r.slider->setValue (normalisedFromReal (d, real));
                r.value.setText (formatValue (real, d.unit), juce::dontSendNotification);
                break;
            }
        }
    }

    void layoutRows (juce::Rectangle<int> area)
    {
        const int rowH = scaled (30);
        const int dialRowH = scaled (54);
        const int gap = scaled (4);
        const int labelW = scaled (170);
        const int valueW = scaled (78);

        // Two columns, the rows split by count
        const int n = static_cast<int> (rows.size());
        const int firstColumn = (n + 1) / 2;
        auto left = area.removeFromLeft (area.getWidth() / 2).reduced (scaled (6), 0);
        auto right = area.reduced (scaled (6), 0);

        for (int i = 0; i < n; ++i)
        {
            auto& col = i < firstColumn ? left : right;
            auto& r = *rows[static_cast<size_t> (i)];
            const bool dialRow = r.dial != nullptr || r.rotation != nullptr;
            auto line = col.removeFromTop (dialRow ? dialRowH : rowH);
            col.removeFromTop (gap);

            r.label.setBounds (line.removeFromLeft (labelW));

            if (r.button != nullptr)
            {
                r.button->setBounds (line.removeFromLeft (juce::jmin (line.getWidth(), scaled (200))));
                r.value.setBounds ({});
            }
            else if (r.combo != nullptr)
            {
                r.combo->setBounds (line.removeFromLeft (juce::jmin (line.getWidth(), scaled (200))));
                r.value.setBounds ({});
            }
            else if (dialRow)
            {
                const int d = dialRowH - scaled (4);
                r.control->setBounds (line.removeFromLeft (d).withSizeKeepingCentre (d, d));
                line.removeFromLeft (gap);
                r.value.setBounds (line.removeFromLeft (valueW));
            }
            else
            {
                r.value.setBounds (line.removeFromRight (valueW));
                r.control->setBounds (line.reduced (0, scaled (4)));
            }
        }
    }

    //==========================================================================
    // Reverb presets
    //==========================================================================

    static bool isPresetOwned (const juce::Identifier& id)
    {
        using namespace WFSParameterIDs;
        return id == effectReverbRT60 || id == effectReverbRT60LowMult || id == effectReverbRT60HighMult
            || id == effectReverbCrossoverLow || id == effectReverbCrossoverHigh || id == effectReverbDiffusion
            || id == effectReverbSize || id == effectReverbPredelay;
    }

    /** An edit to a preset-owned value makes the reverb Custom first, so the
        combo never claims a preset the values no longer match. */
    void presetGuard (Row& r)
    {
        if (! isReverb() || ! r.presetOwned)
            return;

        const int custom = static_cast<int> (spatcore::effects::ReverbType::Custom);
        if (ctx.readInt (WFSParameterIDs::effectReverbType, 0) != custom)
        {
            ctx.writeModule (node, WFSParameterIDs::effectReverbType, custom);
            for (auto& row : rows)
                if (row->desc->id == WFSParameterIDs::effectReverbType)
                    loadRow (*row);
            refreshPresetDimming();
        }
    }

    void applyReverbPreset (int type)
    {
        using namespace WFSParameterIDs;
        namespace fx = spatcore::effects;

        ctx.beginGesture ("Effect Reverb Preset");
        ctx.writeModule (node, effectReverbType, type);

        if (const auto* p = fx::findReverbPreset (type))
        {
            ctx.writeModule (node, effectReverbModel, static_cast<int> (p->model));
            ctx.writeModule (node, effectReverbRT60, p->rt60);
            ctx.writeModule (node, effectReverbRT60LowMult, p->rt60LowMult);
            ctx.writeModule (node, effectReverbRT60HighMult, p->rt60HighMult);
            ctx.writeModule (node, effectReverbCrossoverLow, p->crossoverLow);
            ctx.writeModule (node, effectReverbCrossoverHigh, p->crossoverHigh);
            ctx.writeModule (node, effectReverbDiffusion, p->diffusion);
            ctx.writeModule (node, effectReverbSize, p->size);
            ctx.writeModule (node, effectReverbPredelay, p->predelayMs);
            ctx.showStatusMessage (LOC ("effects.chain.presetApplied").replace ("{name}", p->name));
        }

        const juce::ScopedValueSetter<bool> loadingScope (ctx.isLoadingParameters, true);
        for (auto& row : rows)
            loadRow (*row);
        refreshPresetDimming();
    }

    void refreshPresetDimming()
    {
        if (! isReverb())
            return;

        const bool custom = ctx.readInt (WFSParameterIDs::effectReverbType, 0)
                            == static_cast<int> (spatcore::effects::ReverbType::Custom);
        for (auto& row : rows)
            if (row->presetOwned)
            {
                const float alpha = custom ? 1.0f : 0.55f;
                row->label.setAlpha (alpha);
                row->control->setAlpha (alpha);
                row->value.setAlpha (alpha);
            }
    }

    //==========================================================================
    // Dynamics: the gain-reduction meter
    //==========================================================================

    void setupDyn()
    {
        grMeter = std::make_unique<GainReductionMeter>();
        addAndMakeVisible (*grMeter);
        addAndMakeVisible (grLabel);
        grLabel.setText (LOC ("effects.chain.grMeter"), juce::dontSendNotification);
        grLabel.setJustificationType (juce::Justification::centred);
        grLabel.setColour (juce::Label::textColourId, ColorScheme::get().textSecondary);
        ctx.helpTextMap[grMeter.get()] = LOC ("effects.help.grMeter");
    }

    //==========================================================================
    // Delay: the tap table
    //==========================================================================

    struct TapRow
    {
        juce::Label name, timeValue, levelValue;
        WfsStandardSlider time, level;
    };

    void setupDelay()
    {
        addAndMakeVisible (tapsHeader);
        tapsHeader.setText (LOC ("effects.chain.taps"), juce::dontSendNotification);
        tapsHeader.setFont (juce::FontOptions (14.0f).withStyle ("Bold"));
        addAndMakeVisible (tapTimeHeader);
        tapTimeHeader.setText (LOC ("effects.chain.tapTime"), juce::dontSendNotification);
        tapTimeHeader.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (tapLevelHeader);
        tapLevelHeader.setText (LOC ("effects.chain.tapLevel"), juce::dontSendNotification);
        tapLevelHeader.setJustificationType (juce::Justification::centred);

        for (int t = 0; t < numTaps; ++t)
        {
            auto& row = taps[static_cast<size_t> (t)];
            addAndMakeVisible (row.name);
            row.name.setText (LOC ("effects.chain.tap").replace ("{n}", juce::String (t + 1)), juce::dontSendNotification);

            row.time.setTrackColours (juce::Colour (0xFF1E1E1E), moduleColour());
            row.time.onGestureStart = [this, t] { ctx.beginGesture ("Effect Delay Tap " + juce::String (t + 1) + " Time"); };
            row.time.onValueChanged = [this, t] (float v)
            {
                const auto& d = EffectsUi::descDelayTapTime();
                const float real = realFromNormalised (d, v);
                taps[static_cast<size_t> (t)].timeValue.setText (formatValue (real, d.unit), juce::dontSendNotification);
                if (! ctx.isLoadingParameters)
                    ctx.writeTap (t, d.id, real);
            };
            addAndMakeVisible (row.time);
            addAndMakeVisible (row.timeValue);
            row.timeValue.setJustificationType (juce::Justification::centredRight);

            row.level.setTrackColours (juce::Colour (0xFF1E1E1E), juce::Colour (0xFF4A90D9));
            row.level.onGestureStart = [this, t] { ctx.beginGesture ("Effect Delay Tap " + juce::String (t + 1) + " Level"); };
            row.level.onValueChanged = [this, t] (float v)
            {
                const auto& d = EffectsUi::descDelayTapLevel();
                const float real = realFromNormalised (d, v);
                taps[static_cast<size_t> (t)].levelValue.setText (formatValue (real, d.unit), juce::dontSendNotification);
                if (! ctx.isLoadingParameters)
                    ctx.writeTap (t, d.id, real);
            };
            addAndMakeVisible (row.level);
            addAndMakeVisible (row.levelValue);
            row.levelValue.setJustificationType (juce::Justification::centredRight);

            ctx.helpTextMap[&row.time]  = LOC ("effects.help.tapTime");
            ctx.helpTextMap[&row.level] = LOC ("effects.help.tapLevel");
        }
    }

    void loadTaps()
    {
        const auto& dt = EffectsUi::descDelayTapTime();
        const auto& dl = EffectsUi::descDelayTapLevel();

        for (int t = 0; t < numTaps; ++t)
        {
            auto& row = taps[static_cast<size_t> (t)];
            const auto tv = ctx.readTap (t, dt.id);
            const auto lv = ctx.readTap (t, dl.id);
            const float time  = tv.isVoid() ? dt.def : static_cast<float> (static_cast<double> (tv));
            const float level = lv.isVoid() ? dl.def : static_cast<float> (static_cast<double> (lv));
            row.time.setValue (normalisedFromReal (dt, time));
            row.timeValue.setText (formatValue (time, dt.unit), juce::dontSendNotification);
            row.level.setValue (normalisedFromReal (dl, level));
            row.levelValue.setText (formatValue (level, dl.unit), juce::dontSendNotification);
        }
    }

    void refreshTapDimming()
    {
        if (! isDelay())
            return;

        loadTaps();

        // Rows beyond the tap count are dormant; in Pattern mode the pattern
        // sets the times and the time sliders dim (levels stay live).
        const int active = juce::jlimit (1, numTaps, ctx.readInt (WFSParameterIDs::effectDelayTaps, 3));
        const bool pattern = ctx.readInt (WFSParameterIDs::effectDelayTapMode, 1) != 0;

        for (int t = 0; t < numTaps; ++t)
        {
            auto& row = taps[static_cast<size_t> (t)];
            const float rowAlpha = t < active ? 1.0f : 0.35f;
            row.name.setAlpha (rowAlpha);
            row.level.setAlpha (rowAlpha);
            row.levelValue.setAlpha (rowAlpha);
            const float timeAlpha = t < active && ! pattern ? 1.0f : 0.35f;
            row.time.setAlpha (timeAlpha);
            row.timeValue.setAlpha (timeAlpha);
        }
    }

    void layoutTaps (juce::Rectangle<int> area)
    {
        const int rowH = scaled (24);
        const int nameW = scaled (60);
        const int valueW = scaled (78);
        const int gap = scaled (10);

        auto header = area.removeFromTop (rowH);
        tapsHeader.setBounds (header.removeFromLeft (nameW));
        auto half = header.getWidth() / 2;
        tapTimeHeader.setBounds (header.removeFromLeft (half));
        tapLevelHeader.setBounds (header);

        for (int t = 0; t < numTaps; ++t)
        {
            auto& row = taps[static_cast<size_t> (t)];
            auto line = area.removeFromTop (rowH);
            row.name.setBounds (line.removeFromLeft (nameW));
            auto timeArea = line.removeFromLeft (line.getWidth() / 2);
            timeArea.removeFromRight (gap);
            row.timeValue.setBounds (timeArea.removeFromRight (valueW));
            row.time.setBounds (timeArea.reduced (0, scaled (3)));
            row.levelValue.setBounds (line.removeFromRight (valueW));
            row.level.setBounds (line.reduced (0, scaled (3)));
        }
    }

    //==========================================================================
    // EQ: the display and the six band strips
    //==========================================================================

    struct Band
    {
        juce::Label name, freqLabel, freqValue, gainLabel, gainValue, qLabel, qValue, slopeLabel, slopeValue;
        EQBandToggle toggle;
        juce::ComboBox shape;
        LongPressButton reset { 800 };
        WfsStandardSlider freq;
        WfsBasicDial gain, q, slope;
    };

    void setupEq()
    {
        addAndMakeVisible (eqFlattenButton);
        eqFlattenButton.setButtonText (LOC ("eq.buttons.flattenEQ"));
        eqFlattenButton.onLongPress = [this]
        {
            ctx.beginGesture ("Effect EQ Flatten");
            for (int b = 0; b < numEqBands; ++b)
                resetBand (b);
        };
        ctx.helpTextMap[&eqFlattenButton] = LOC ("effects.help.eqFlatten");

        for (int b = 0; b < numEqBands; ++b)
        {
            auto& band = bands[static_cast<size_t> (b)];
            const auto colour = EQDisplayComponent::getBandColour (b);

            addAndMakeVisible (band.name);
            band.name.setText (LOC ("effects.chain.band").replace ("{n}", juce::String (b + 1)), juce::dontSendNotification);
            band.name.setColour (juce::Label::textColourId, colour);
            band.name.setJustificationType (juce::Justification::centred);

            addAndMakeVisible (band.toggle);
            band.toggle.setBandColour (colour);
            band.toggle.onClick = [this, b]
            {
                auto& bd = bands[static_cast<size_t> (b)];
                const int shape = bd.toggle.getToggleState() ? bd.shape.getSelectedId() : 0;
                ctx.beginGesture ("Effect EQ Band " + juce::String (b + 1) + " On/Off");
                ctx.writeBand (eqInstance(), b, WFSParameterIDs::effectEQshape, shape);
                updateBandAppearance (b);
            };
            ctx.helpTextMap[&band.toggle] = LOC ("effects.help.eqBandToggle");

            // The OUTPUT EQ's numbering (the CSV's): Off 0, Low Cut 1, Low Shelf 2,
            // Peak 3, Band Pass 4, High Shelf 5, High Cut 6, All Pass 7; the
            // toggle owns Off.
            addAndMakeVisible (band.shape);
            band.shape.addItem (LOC ("eq.filterTypes.lowCut"), 1);
            band.shape.addItem (LOC ("eq.filterTypes.lowShelf"), 2);
            band.shape.addItem (LOC ("eq.filterTypes.peakNotch"), 3);
            band.shape.addItem (LOC ("eq.filterTypes.bandPass"), 4);
            band.shape.addItem (LOC ("eq.filterTypes.highShelf"), 5);
            band.shape.addItem (LOC ("eq.filterTypes.highCut"), 6);
            band.shape.addItem (LOC ("eq.filterTypes.allPass"), 7);
            band.shape.setSelectedId (WFSParameterDefaults::effectEQBandShapes[b], juce::dontSendNotification);
            band.shape.onChange = [this, b]
            {
                if (ctx.isLoadingParameters) return;
                auto& bd = bands[static_cast<size_t> (b)];
                if (bd.toggle.getToggleState())
                    ctx.writeBand (eqInstance(), b, WFSParameterIDs::effectEQshape, bd.shape.getSelectedId());
                updateBandAppearance (b);
                TTSManager::getInstance().announceValueChange ("EQ Band " + juce::String (b + 1) + " Shape", bd.shape.getText());
            };
            ctx.helpTextMap[&band.shape] = LOC ("effects.help.eqShape");

            addAndMakeVisible (band.reset);
            band.reset.setButtonText (LOC ("eq.buttons.resetBand"));
            band.reset.onLongPress = [this, b] { ctx.beginGesture ("Effect EQ Band Reset"); resetBand (b); };
            ctx.helpTextMap[&band.reset] = LOC ("effects.help.eqBandReset");

            addAndMakeVisible (band.freqLabel);
            band.freqLabel.setText (LOC ("eq.labels.freq"), juce::dontSendNotification);
            band.freqLabel.setColour (juce::Label::textColourId, ColorScheme::get().textSecondary);
            band.freq.setTrackColours (ColorScheme::get().sliderTrackBg, colour);
            band.freq.onGestureStart = [this, b] { ctx.beginGesture ("Effect EQ Band " + juce::String (b + 1) + " Freq"); };
            band.freq.onValueChanged = [this, b] (float v)
            {
                const int freq = static_cast<int> (20.0f * std::pow (10.0f, 3.0f * v));
                bands[static_cast<size_t> (b)].freqValue.setText (formatValue ((float) freq, "Hz"), juce::dontSendNotification);
                if (! ctx.isLoadingParameters)
                    ctx.writeBand (eqInstance(), b, WFSParameterIDs::effectEQfreq, freq);
            };
            addAndMakeVisible (band.freq);
            addAndMakeVisible (band.freqValue);
            band.freqValue.setJustificationType (juce::Justification::centred);
            ctx.helpTextMap[&band.freq] = LOC ("effects.help.eqFreq");

            setupBandDial (band.gainLabel, band.gain, band.gainValue, LOC ("eq.labels.gain"), colour, b,
                           WFSParameterIDs::effectEQgain, EffectsUi::descEQgain(), "Gain");
            setupBandDial (band.qLabel, band.q, band.qValue, LOC ("eq.labels.q"), colour, b,
                           WFSParameterIDs::effectEQq, EffectsUi::descEQq(), "Q");
            setupBandDial (band.slopeLabel, band.slope, band.slopeValue, LOC ("effects.labels.eqSlope"), colour, b,
                           WFSParameterIDs::effectEQslope, EffectsUi::descEQslope(), "Slope");
            band.gain.setBipolar (true);
        }
    }

    void setupBandDial (juce::Label& label, WfsBasicDial& dial, juce::Label& value, const juce::String& text,
                        juce::Colour colour, int b, const juce::Identifier& id,
                        const EffectsUi::ControlDesc& desc, const juce::String& gestureName)
    {
        addAndMakeVisible (label);
        label.setText (text.trimCharactersAtEnd (":"), juce::dontSendNotification);
        label.setColour (juce::Label::textColourId, ColorScheme::get().textSecondary);
        label.setJustificationType (juce::Justification::centred);

        dial.setTrackColours (ColorScheme::get().sliderTrackBg, colour);
        dial.onGestureStart = [this, b, gestureName] { ctx.beginGesture ("Effect EQ Band " + juce::String (b + 1) + " " + gestureName); };
        dial.onValueChanged = [this, b, &value, &id, &desc] (float v)
        {
            // Gain is linear over +-24; Q and slope follow the CSV's laws
            const float real = id == WFSParameterIDs::effectEQgain ? (v * 48.0f) - 24.0f
                             : id == WFSParameterIDs::effectEQq    ? desc.min * std::pow (desc.max / desc.min, v)
                                                                   : desc.min + (desc.max - desc.min) * v;
            value.setText (id == WFSParameterIDs::effectEQgain ? formatValue (real, "dB") : juce::String (real, 2),
                           juce::dontSendNotification);
            if (! ctx.isLoadingParameters)
                ctx.writeBand (eqInstance(), b, id, real);
        };
        addAndMakeVisible (dial);
        addAndMakeVisible (value);
        value.setJustificationType (juce::Justification::centred);
        ctx.helpTextMap[&dial] = LOC ("effects.help." + juce::String (desc.key));
    }

    void loadEq()
    {
        // The display binds to the <FxEqN> node of ONE channel, so it is
        // rebuilt when the channel changes and never mid-drag (OutputsTab's guard).
        auto& vts = ctx.parameters.getValueTreeState();
        auto eqTree = vts.getEffectEQSection (ctx.slot(), eqInstance());

        if (eqTree.isValid() && (eqDisplay == nullptr || eqDisplayChannel != ctx.slot()))
        {
            eqDisplay = std::make_unique<EQDisplayComponent> (eqTree, numEqBands, EQDisplayConfig::forEffectEQ());
            addAndMakeVisible (*eqDisplay);
            eqDisplay->ownerPersistsValue = true;
            eqDisplay->onParameterChanged = [this] (int band, const juce::Identifier& id, const juce::var& v)
            {
                if (ctx.isLoadingParameters) return;
                ctx.writeBand (eqInstance(), band, id, v);
                refreshBand (band);
            };
            ctx.helpTextMap[eqDisplay.get()] = LOC ("effects.help.eqDisplay");
            eqDisplayChannel = ctx.slot();
            resized();
        }

        for (int b = 0; b < numEqBands; ++b)
            refreshBand (b);

        if (eqDisplay != nullptr)
            eqDisplay->setEQEnabled (ctx.readModule (node, WFSParameterIDs::effectEQBypass).isVoid()
                                         ? false
                                         : static_cast<int> (ctx.readModule (node, WFSParameterIDs::effectEQBypass)) == 0);
    }

    void refreshBand (int b)
    {
        const juce::ScopedValueSetter<bool> loadingScope (ctx.isLoadingParameters, true);
        auto& band = bands[static_cast<size_t> (b)];
        using namespace WFSParameterIDs;

        const int shape = static_cast<int> (ctx.readBand (eqInstance(), b, effectEQshape));
        const bool on = shape != 0;
        band.toggle.setToggleState (on, juce::dontSendNotification);
        if (on)
            band.shape.setSelectedId (shape, juce::dontSendNotification);

        const auto fv = ctx.readBand (eqInstance(), b, effectEQfreq);
        const int freq = fv.isVoid() ? WFSParameterDefaults::effectEQBandFrequencies[b] : static_cast<int> (fv);
        band.freq.setValue (juce::jlimit (0.0f, 1.0f, std::log10 (freq / 20.0f) / 3.0f));
        band.freqValue.setText (formatValue ((float) freq, "Hz"), juce::dontSendNotification);

        const float gain = readBandFloat (b, effectEQgain, 0.0f);
        band.gain.setValue ((gain + 24.0f) / 48.0f);
        band.gainValue.setText (formatValue (gain, "dB"), juce::dontSendNotification);

        const auto& dq = EffectsUi::descEQq();
        const float q = readBandFloat (b, effectEQq, dq.def);
        band.q.setValue (juce::jlimit (0.0f, 1.0f, std::log (q / dq.min) / std::log (dq.max / dq.min)));
        band.qValue.setText (juce::String (q, 2), juce::dontSendNotification);

        const auto& ds = EffectsUi::descEQslope();
        const float slope = readBandFloat (b, effectEQslope, ds.def);
        band.slope.setValue (juce::jlimit (0.0f, 1.0f, (slope - ds.min) / (ds.max - ds.min)));
        band.slopeValue.setText (juce::String (slope, 2), juce::dontSendNotification);

        updateBandAppearance (b);
    }

    float readBandFloat (int b, const juce::Identifier& id, float fallback) const
    {
        const auto v = ctx.readBand (eqInstance(), b, id);
        return v.isVoid() ? fallback : static_cast<float> (static_cast<double> (v));
    }

    void updateBandAppearance (int b)
    {
        auto& band = bands[static_cast<size_t> (b)];
        const bool on = band.toggle.getToggleState();
        const int shape = band.shape.getSelectedId();
        const bool cutOrPass = shape == 1 || shape == 4 || shape == 6 || shape == 7;
        const bool shelf = shape == 2 || shape == 5;
        const float alpha = on ? 1.0f : 0.4f;

        band.shape.setAlpha (alpha);
        band.freqLabel.setAlpha (alpha);
        band.freq.setAlpha (alpha);
        band.freqValue.setAlpha (alpha);
        for (auto* c : { static_cast<juce::Component*> (&band.gainLabel), static_cast<juce::Component*> (&band.gain), static_cast<juce::Component*> (&band.gainValue) })
            c->setAlpha (on && ! cutOrPass ? 1.0f : 0.4f);
        for (auto* c : { static_cast<juce::Component*> (&band.qLabel), static_cast<juce::Component*> (&band.q), static_cast<juce::Component*> (&band.qValue) })
            c->setAlpha (on && ! shelf ? 1.0f : 0.4f);
        for (auto* c : { static_cast<juce::Component*> (&band.slopeLabel), static_cast<juce::Component*> (&band.slope), static_cast<juce::Component*> (&band.slopeValue) })
            c->setAlpha (on && shelf ? 1.0f : 0.4f);
    }

    void resetBand (int b)
    {
        using namespace WFSParameterIDs;
        namespace D = WFSParameterDefaults;
        ctx.writeBand (eqInstance(), b, effectEQshape, D::effectEQBandShapes[b]);
        ctx.writeBand (eqInstance(), b, effectEQfreq,  D::effectEQBandFrequencies[b]);
        ctx.writeBand (eqInstance(), b, effectEQgain,  D::effectEQgainDefault);
        ctx.writeBand (eqInstance(), b, effectEQq,     D::effectEQqDefault);
        ctx.writeBand (eqInstance(), b, effectEQslope, D::effectEQslopeDefault);
        refreshBand (b);
    }

    void layoutEq (juce::Rectangle<int> area)
    {
        const int rowH = scaled (30);
        const int labelH = scaled (18);
        const int sliderH = scaled (32);
        const int gap = scaled (5);
        const int toggleSize = scaled (18);
        const int dialSize = juce::jmax (36, static_cast<int> (52.0f * layoutScale));

        // Top row: the bypass (the one descriptor row) left, Flatten right
        auto top = area.removeFromTop (rowH);
        if (! rows.empty())
        {
            rows[0]->label.setBounds ({});
            rows[0]->control->setBounds (top.removeFromLeft (scaled (160)));
            rows[0]->value.setBounds ({});
        }
        eqFlattenButton.setBounds (top.removeFromRight (scaled (120)));
        area.removeFromTop (gap * 2);

        if (eqDisplay != nullptr)
        {
            eqDisplay->setBounds (area.removeFromTop (juce::jmax (140, area.getHeight() * 38 / 100)));
            area.removeFromTop (gap);
        }

        const int bandW = area.getWidth() / numEqBands;
        for (int b = 0; b < numEqBands; ++b)
        {
            auto& band = bands[static_cast<size_t> (b)];
            auto col = area.removeFromLeft (bandW).reduced (scaled (4), 0);

            band.name.setBounds (col.removeFromTop (labelH));
            auto shapeRow = col.removeFromTop (rowH);
            band.toggle.setBounds (shapeRow.removeFromLeft (toggleSize).withSizeKeepingCentre (toggleSize, toggleSize));
            shapeRow.removeFromLeft (scaled (4));
            band.reset.setBounds (shapeRow.removeFromRight (scaled (50)));
            band.shape.setBounds (shapeRow);
            col.removeFromTop (gap);

            band.freqLabel.setBounds (col.removeFromTop (labelH));
            band.freq.setBounds (col.removeFromTop (sliderH));
            band.freqValue.setBounds (col.removeFromTop (labelH));
            col.removeFromTop (gap);

            auto dialRow = col.removeFromTop (dialSize + labelH * 2);
            const int cellW = dialRow.getWidth() / 3;
            auto cell = [&] (juce::Label& l, WfsBasicDial& d, juce::Label& v)
            {
                auto c = dialRow.removeFromLeft (cellW);
                l.setBounds (c.removeFromTop (labelH));
                d.setBounds (c.removeFromTop (dialSize).withSizeKeepingCentre (dialSize, dialSize));
                v.setBounds (c.removeFromTop (labelH));
            };
            cell (band.gainLabel, band.gain, band.gainValue);
            cell (band.qLabel, band.q, band.qValue);
            cell (band.slopeLabel, band.slope, band.slopeValue);
        }
    }

    //==========================================================================
    juce::String moduleName() const { return LOC ("effects.modules." + juce::String (token)); }

    juce::Colour moduleColour() const
    {
        // One hue per module family, so a row reads as its module's at a glance
        static const juce::Colour hues[] = {
            juce::Colour (0xFFE57373), juce::Colour (0xFF4A90D9), juce::Colour (0xFF4A90D9),
            juce::Colour (0xFF2E7D32), juce::Colour (0xFF2E7D32), juce::Colour (0xFF9B59B6),
            juce::Colour (0xFF00ACC1), juce::Colour (0xFFD4A017), juce::Colour (0xFF26A69A),
            juce::Colour (0xFFFF8F00), juce::Colour (0xFFCDDC39) };
        return hues[static_cast<size_t> (juce::jlimit (0, 10, slot))];
    }

    int scaled (int ref) const
    {
        return juce::jmax (static_cast<int> (ref * 0.65f), static_cast<int> (ref * layoutScale));
    }

    EffectsTabContext& ctx;
    const int slot;
    const juce::Identifier node;
    const char* const token;
    float layoutScale = 1.0f;

    std::vector<std::unique_ptr<Row>> rows;

    // Dynamics
    std::unique_ptr<GainReductionMeter> grMeter;
    juce::Label grLabel;

    // Delay
    juce::Label tapsHeader, tapTimeHeader, tapLevelHeader;
    std::array<TapRow, static_cast<size_t> (WFSParameterDefaults::numEffectDelayTaps)> taps;

    // EQ
    LongPressButton eqFlattenButton { 800 };
    std::array<Band, static_cast<size_t> (WFSParameterDefaults::numEffectEQBands)> bands;
    std::unique_ptr<EQDisplayComponent> eqDisplay;
    int eqDisplayChannel = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectsModulePanel)
};
