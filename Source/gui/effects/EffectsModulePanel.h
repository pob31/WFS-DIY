#pragma once

#include <JuceHeader.h>
#include "EffectsTabContext.h"
#include "EffectsModuleDescriptors.h"
#include "EffectsFieldEditing.h"
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
    delay (its eight tap rows) and the reverb (its presets). A preset is not
    the panel's to apply: the type goes through the funnel like any other
    write, and WFSValueTreeState expands it - the same action the Stream Deck
    and OSC reach - and flips the type to Custom when a value it owns is
    really edited. The panel only has to show what that did: every row after
    a preset, the preset combo after an owned edit. The reverb also shows only
    the rows its MODEL uses (the CSV's Models column), resolved the way the
    engine resolves it, so a reserved id shows the FDN it runs.

    EVERY WRITE GOES THROUGH THE CONTEXT, so a linked group receives it
    according to each member's mode and Ctrl-drag edits this channel alone.

    A MODULE THAT IS OFF STILL TAKES EDITS. Its controls grey out (alpha
    only, never setEnabled) so a sound can be prepared before it is switched
    in; the same holds for the Dynamics compressor and expander sections.
    Every other dimming here (preset-owned rows, dormant taps, off EQ bands)
    records its own alpha through setOwnAlpha, and the module's state
    multiplies it, so the two never overwrite each other.
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

        setupFieldEditing();

        // A click on an empty patch of the module closes an open field
        // (EffectsFieldEditing says why the panel has to take the focus)
        setWantsKeyboardFocus (true);
    }

    std::unique_ptr<juce::ComponentTraverser> createKeyboardFocusTraverser() override
    {
        return fields.createTraverser();
    }

    EffectsFieldEditing& getFieldsForTest() { return fields; }

    int getSlot() const noexcept { return slot; }
    bool isEq()    const noexcept { return slot == 1 || slot == 2; }
    bool isDyn()   const noexcept { return slot == 3 || slot == 4; }
    bool isDelay() const noexcept { return slot == 9; }
    bool isReverb() const noexcept { return slot == 8; }
    int eqInstance() const noexcept { return slot == 2 ? 1 : 0; }

    /** One hue per module, in declared slot order (spatcore::effects::kSlots),
        shared by the chain tiles and the panel so the tile picked and the
        panel it opens read as the same thing. The doubled modules differ by
        shade: EQ 2 and Dynamics 2 are the lighter ones. */
    static juce::Colour slotColour (int slotIndex)
    {
        static const juce::Colour hues[] = {
            juce::Colour (0xFFE57373),     // dist    red
            juce::Colour (0xFF4A90D9),     // eq1     blue
            juce::Colour (0xFF90CAF9),     // eq2     light blue
            juce::Colour (0xFF43A047),     // dyn1    green
            juce::Colour (0xFF81C784),     // dyn2    light green
            juce::Colour (0xFF9B59B6),     // mod     purple
            juce::Colour (0xFF00ACC1),     // phaser  cyan
            juce::Colour (0xFFD4A017),     // trem    gold
            juce::Colour (0xFF26A69A),     // reverb  teal
            juce::Colour (0xFFFF8F00),     // delay   orange
            juce::Colour (0xFFEC407A) };   // crush   pink
        return hues[static_cast<size_t> (juce::jlimit (0, 10, slotIndex))];
    }

    /** A reverb control by identifier, whatever its row in the CSV. */
    static const EffectsUi::ControlDesc& reverbControl (const juce::Identifier& id)
    {
        const auto list = EffectsUi::controlsForReverb();
        for (int i = 0; i < list.count; ++i)
            if (list.controls[i].id == id)
                return list.controls[i];
        jassertfalse;
        return list.controls[0];
    }

    /** Re-read every control for the context's channel. */
    void loadParameters()
    {
        for (auto& row : rows)
            loadRow (*row);

        if (isEq())    loadEq();
        if (isDelay()) refreshTapDimming();
        if (isReverb())
        {
            refreshReverbState();
            refreshPresetDimming();
        }
        refreshSectionDimming();
        refreshModuleOn();
    }

    /** Fired when this panel's ON button switches the module - the chain
        strip's tile shows the new state. */
    std::function<void()> onModuleOnChanged;

    bool isModuleOn() const noexcept { return moduleOn; }

    void paint (juce::Graphics& g) override
    {
        // A faint card in the module's hue, edged in it: the panel belongs
        // to the tile of the same colour above it
        const auto hue = slotColour (slot);
        auto r = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (ColorScheme::get().surfaceCard.interpolatedWith (hue, moduleOn ? 0.07f : 0.03f));
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (hue.withAlpha (moduleOn ? 0.75f : 0.3f));
        g.drawRoundedRectangle (r, 6.0f, 1.5f);
    }

    /** Fired when the reverb's rows follow a different model - the Stream
        Deck lays its page out again on it. Argument: the resolved model. */
    std::function<void (int resolvedModel)> onReverbModelShown;

    /** The rows on show, in order - the self-test holds the Models column to it. */
    std::vector<juce::Identifier> getShownRowIds() const
    {
        std::vector<juce::Identifier> ids;
        for (const auto& row : rows)
            if (row->shown)
                ids.push_back (row->desc->id);
        return ids;
    }

    /** Every shown row's bounds, label to value, for the no-overlap check. */
    std::vector<juce::Rectangle<int>> getShownRowBounds() const
    {
        std::vector<juce::Rectangle<int>> bounds;
        for (const auto& row : rows)
            if (row->shown)
                bounds.push_back (row->label.getBounds().getUnion (row->control->getBounds())
                                                        .getUnion (row->value.getBounds()));
        return bounds;
    }

    /** A combo row's selected id (its item value + 1; 0 for none) - the
        self-test reads the reverb's model as the menu shows it. */
    int getComboSelectedId (const juce::Identifier& id) const
    {
        for (const auto& row : rows)
            if (row->desc->id == id && row->combo != nullptr)
                return row->combo->getSelectedId();
        return 0;
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
        auto area = getLocalBounds().reduced (scaled (14), scaled (12));

        if (isEq())
        {
            layoutEq (area);
            updateCircuits();
            return;
        }

        // The header line, the same in every module: the ON button left,
        // Mix (dry/wet) where the right column starts. It spans the whole
        // panel, so neither the delay's taps nor the GR meter move it.
        layoutHeader (area.removeFromTop (scaled (40)));
        area.removeFromTop (scaled (16));

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
            // At the standard slider height the eight taps no longer fit
            // under the module rows: they take a column of their own, right.
            auto tapArea = area.removeFromRight (area.getWidth() * 2 / 5);
            area.removeFromRight (scaled (16));
            layoutTaps (tapArea);
        }

        layoutRows (area);
        updateCircuits();
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
        bool presetOwned = false;      // reverb: one of the fifteen a preset writes
        bool shown = true;             // reverb: used by the model on show
        bool inHeader = false;         // the ON button and Mix: the header line, not a column
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
                    // The tab does not reload for its own writes, so every
                    // view of this state follows here: the EQ display's
                    // overlay, the greyed controls, the chain tile
                    refreshModuleOn();
                };
                bypassControl = b;
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
                    ctx.writeModule (node, r->desc->id, next);
                    loadRow (*r);
                    afterReverbEdit (*r);
                    refreshSectionDimming();
                };
                break;
            }
            case Kind::Combo:
            {
                auto* c = new juce::ComboBox();
                row->control.reset (c);
                row->combo = c;
                if (isReverb() && d.id == WFSParameterIDs::effectReverbType)
                    addReverbPresetItems (*c, d);
                else
                    for (const auto& item : d.items)
                        c->addItem (LOC (juce::String (d.enumPrefix) + item.slug), item.value + 1);
                c->onChange = [this, r]
                {
                    if (ctx.isLoadingParameters) return;
                    const int value = r->combo->getSelectedId() - 1;
                    if (isReverb() && r->desc->id == WFSParameterIDs::effectReverbType)
                    {
                        selectReverbPreset (value);
                        return;
                    }
                    ctx.writeModule (node, r->desc->id, value);
                    afterReverbEdit (*r);
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
                dial->setColours (juce::Colours::black, slotColour (slot), juce::Colours::grey);
                dial->setTrackColours (ColorScheme::get().sliderTrackBg, slotColour (slot));
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
                s->setTrackColours (juce::Colour (0xFF1E1E1E), slotColour (slot));
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
                s->setTrackColours (juce::Colour (0xFF1E1E1E), slotColour (slot));
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
            row->presetOwned = WFSValueTreeState::isEffectReverbPresetOwned (d.id);

        // Every module's dry/wet row is its "<module>Mix"
        row->inHeader = d.kind == Kind::Bypass || key.endsWith ("Mix");

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
        ctx.writeModule (node, r.desc->id, real);
        afterReverbEdit (r);
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
                const auto hue = slotColour (slot);
                r.button->setButtonText (moduleName() + ": " + LOC (on ? "effects.chain.moduleOn" : "effects.chain.moduleOff"));
                r.button->setColour (juce::TextButton::buttonColourId, on ? hue : ColorScheme::get().buttonNormal);
                if (on)
                    r.button->setColour (juce::TextButton::textColourOffId,
                                         hue.getPerceivedBrightness() > 0.6f ? juce::Colours::black : juce::Colours::white);
                else
                    r.button->removeColour (juce::TextButton::textColourOffId);
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
            {
                int v = readInt (d);
                if (isReverb() && d.id == WFSParameterIDs::effectReverbModel)
                    v = spatcore::effects::resolveReverbModel (v);      // 2 and 3 run the FDN: say so
                r.combo->setSelectedId (v + 1, juce::dontSendNotification);
                break;
            }
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

    int columnGap() const { return scaled (28); }

    /** The ON button at the panel's left edge, Mix where a full-width right
        column starts - the same two places in every module. */
    void layoutHeader (juce::Rectangle<int> line)
    {
        auto left = line.removeFromLeft ((line.getWidth() - columnGap()) / 2);
        line.removeFromLeft (columnGap());

        for (auto& row : rows)
        {
            if (! row->inHeader)
                continue;

            if (row->desc->kind == EffectsUi::Kind::Bypass)
            {
                row->label.setBounds ({});
                row->control->setBounds (left.withWidth (juce::jmin (left.getWidth(), scaled (220)))
                                             .reduced (0, juce::jmax (1, left.getHeight() / 14)));
                row->value.setBounds ({});
            }
            else
            {
                placeRow (*row, line);
            }
        }
    }

    /** One row on one line: label, then the control, then its value. */
    void placeRow (Row& r, juce::Rectangle<int> line)
    {
        const int labelW = scaled (170);
        const int valueW = scaled (78);

        r.label.setBounds (line.removeFromLeft (labelW));

        if (r.button != nullptr || r.combo != nullptr)
        {
            r.control->setBounds (line.removeFromLeft (juce::jmin (line.getWidth(), scaled (220)))
                                      .reduced (0, juce::jmax (1, line.getHeight() / 14)));
            r.value.setBounds ({});
        }
        else if (r.dial != nullptr || r.rotation != nullptr)
        {
            const int d = line.getHeight() - scaled (2);
            r.control->setBounds (line.removeFromLeft (d).withSizeKeepingCentre (d, d));
            line.removeFromLeft (scaled (8));
            r.value.setBounds (line.removeFromLeft (valueW));
        }
        else
        {
            r.value.setBounds (line.removeFromRight (valueW));
            line.removeFromRight (scaled (6));
            r.control->setBounds (line);
        }
    }

    void layoutRows (juce::Rectangle<int> area)
    {
        // Touch sizes: every line is the app's standard slider height (the
        // Inputs, Outputs and Reverb tabs' 40), with room between lines.
        // Two columns, the rows on show split by count; a hidden row takes no
        // room, so the reverb's columns close up round the model's own.
        std::vector<Row*> shownRows;
        for (auto& row : rows)
            if (row->shown && ! row->inHeader)
                shownRows.push_back (row.get());

        const int n = static_cast<int> (shownRows.size());
        const int firstColumn = (n + 1) / 2;
        const auto isDialRow = [] (const Row& r) { return r.dial != nullptr || r.rotation != nullptr; };

        // The padding gives way first when a module is too tall for the
        // panel, then the lines themselves - never an overlap or a clip
        float lineH = static_cast<float> (scaled (40));
        float dialH = static_cast<float> (scaled (58));
        int gap = scaled (12);
        const int minGap = scaled (4);
        for (int c = 0; c < 2; ++c)
        {
            const int from = c == 0 ? 0 : firstColumn, to = c == 0 ? firstColumn : n;
            if (to - from < 1)
                continue;
            int lines = 0, dials = 0;
            for (int i = from; i < to; ++i)
                (isDialRow (*shownRows[static_cast<size_t> (i)]) ? dials : lines)++;
            const float content = lines * lineH + dials * dialH;
            const int gaps = to - from - 1;
            if (gaps > 0 && content + gap * gaps > area.getHeight())
                gap = juce::jmax (minGap, static_cast<int> ((area.getHeight() - content) / gaps));
            if (content + gap * gaps > area.getHeight())
            {
                const float shrink = juce::jmax (0.5f, (area.getHeight() - gap * gaps) / content);
                lineH *= shrink;
                dialH *= shrink;
            }
        }

        auto left = area.removeFromLeft ((area.getWidth() - columnGap()) / 2);
        area.removeFromLeft (columnGap());
        auto right = area;

        for (int i = 0; i < n; ++i)
        {
            auto& col = i < firstColumn ? left : right;
            auto& r = *shownRows[static_cast<size_t> (i)];
            placeRow (r, col.removeFromTop (juce::roundToInt (isDialRow (r) ? dialH : lineH)));
            col.removeFromTop (gap);
        }
    }

    //==========================================================================
    // Typed values and Tab sections
    //==========================================================================

    static bool isNumericRow (const Row& r)
    {
        return r.slider != nullptr || r.dial != nullptr || r.rotation != nullptr;
    }

    /** Every value a row, tap or band shows takes a typed number, clamped to
        the CSV's range and written through the control, as a drag would. */
    void setupFieldEditing()
    {
        for (auto& row : rows)
        {
            if (! isNumericRow (*row))
                continue;

            auto* r = row.get();
            fields.makeEditable (r->value, "Effect " + r->label.getText(), [this, r] (float typed)
            {
                const auto& d = *r->desc;
                if (r->rotation != nullptr)
                {
                    float deg = std::fmod (typed, 360.0f);          // stored 0..360
                    if (deg < 0.0f)
                        deg += 360.0f;
                    r->rotation->setAngle (deg > 180.0f ? deg - 360.0f : deg);
                    r->value.setText (formatValue (deg, d.unit), juce::dontSendNotification);
                    return;
                }

                const float real = juce::jlimit (d.min, d.max, typed);
                if (r->dial != nullptr)
                    r->dial->setValue (normalisedFromReal (d, real));
                else
                    r->slider->setValue (normalisedFromReal (d, real));
                r->value.setText (formatValue (real, d.unit), juce::dontSendNotification);
            });
        }

        if (isDelay())
        {
            for (int t = 0; t < numTaps; ++t)
            {
                const auto tapName = "Effect Delay Tap " + juce::String (t + 1);
                fields.makeEditable (taps[static_cast<size_t> (t)].timeValue, tapName + " Time", [this, t] (float typed)
                {
                    const auto& d = EffectsUi::descDelayTapTime();
                    const float real = juce::jlimit (d.min, d.max, typed);
                    auto& row = taps[static_cast<size_t> (t)];
                    row.time.setValue (normalisedFromReal (d, real));
                    row.timeValue.setText (formatValue (real, d.unit), juce::dontSendNotification);
                });
                fields.makeEditable (taps[static_cast<size_t> (t)].levelValue, tapName + " Level", [this, t] (float typed)
                {
                    const auto& d = EffectsUi::descDelayTapLevel();
                    const float real = juce::jlimit (d.min, d.max, typed);
                    auto& row = taps[static_cast<size_t> (t)];
                    row.level.setValue (normalisedFromReal (d, real));
                    row.levelValue.setText (formatValue (real, d.unit), juce::dontSendNotification);
                });
            }
        }

        if (isEq())
        {
            for (int b = 0; b < numEqBands; ++b)
            {
                auto& band = bands[static_cast<size_t> (b)];
                const auto bandName = "Effect EQ Band " + juce::String (b + 1);

                fields.makeEditable (band.freqValue, bandName + " Freq", [this, b] (float typed)
                {
                    auto& bd = bands[static_cast<size_t> (b)];
                    const int freq = juce::jlimit (20, 20000, juce::roundToInt (typed));
                    bd.freq.setValue (juce::jlimit (0.0f, 1.0f, std::log10 (freq / 20.0f) / 3.0f));
                    // The slider's own law truncates (1000 Hz comes back as
                    // 999), so the typed frequency is written as typed
                    ctx.writeBand (eqInstance(), b, WFSParameterIDs::effectEQfreq, freq);
                    bd.freqValue.setText (formatValue (static_cast<float> (freq), "Hz"), juce::dontSendNotification);
                });

                fields.makeEditable (band.gainValue, bandName + " Gain", [this, b] (float typed)
                {
                    auto& bd = bands[static_cast<size_t> (b)];
                    const float gain = juce::jlimit (-24.0f, 24.0f, typed);
                    bd.gain.setValue ((gain + 24.0f) / 48.0f);
                    bd.gainValue.setText (formatValue (gain, "dB"), juce::dontSendNotification);
                });

                fields.makeEditable (band.qValue, bandName + " Q", [this, b] (float typed)
                {
                    auto& bd = bands[static_cast<size_t> (b)];
                    const auto& dq = EffectsUi::descEQq();
                    const float q = juce::jlimit (dq.min, dq.max, typed);
                    bd.q.setValue (juce::jlimit (0.0f, 1.0f, std::log (q / dq.min) / std::log (dq.max / dq.min)));
                    bd.qValue.setText (juce::String (q, 2), juce::dontSendNotification);
                });

                fields.makeEditable (band.slopeValue, bandName + " Slope", [this, b] (float typed)
                {
                    auto& bd = bands[static_cast<size_t> (b)];
                    const auto& ds = EffectsUi::descEQslope();
                    const float slope = juce::jlimit (ds.min, ds.max, typed);
                    bd.slope.setValue (juce::jlimit (0.0f, 1.0f, (slope - ds.min) / (ds.max - ds.min)));
                    bd.slopeValue.setText (juce::String (slope, 2), juce::dontSendNotification);
                });
            }
        }
    }

    /** Tab keeps to one column on screen: the two row columns as layoutRows
        splits them (Mix heads the right one), the tap times, the tap levels,
        and each EQ band. Rebuilt on every layout, because the reverb's model
        decides which rows are on show. */
    void updateCircuits()
    {
        std::vector<std::vector<juce::Component*>> circuits;
        std::vector<juce::Component*> left, right;

        for (auto& row : rows)
            if (row->inHeader && isNumericRow (*row))
                right.push_back (&row->value);

        std::vector<Row*> shownRows;
        for (auto& row : rows)
            if (row->shown && ! row->inHeader)
                shownRows.push_back (row.get());

        const int firstColumn = (static_cast<int> (shownRows.size()) + 1) / 2;
        for (int i = 0; i < static_cast<int> (shownRows.size()); ++i)
            if (isNumericRow (*shownRows[static_cast<size_t> (i)]))
                (i < firstColumn ? left : right).push_back (&shownRows[static_cast<size_t> (i)]->value);

        for (auto* column : { &left, &right })
            if (! column->empty())
                circuits.push_back (*column);

        if (isDelay())
        {
            std::vector<juce::Component*> times, levels;
            for (auto& tap : taps)
            {
                times.push_back (&tap.timeValue);
                levels.push_back (&tap.levelValue);
            }
            circuits.push_back (times);
            circuits.push_back (levels);
        }

        if (isEq())
            for (auto& band : bands)
                circuits.push_back ({ &band.freqValue, &band.gainValue, &band.qValue, &band.slopeValue });

        fields.setCircuits (std::move (circuits));
    }

    //==========================================================================
    // Reverb presets
    //==========================================================================

    /** A preset: one write of the type, which the state expands (and runs on
        every linked member). Then the panel shows the result - all of it. */
    void selectReverbPreset (int type)
    {
        ctx.writeModule (node, WFSParameterIDs::effectReverbType, type);

        if (const auto* p = spatcore::effects::findReverbPreset (type))
            ctx.showStatusMessage (LOC ("effects.chain.presetApplied").replace ("{name}", p->name));

        const juce::ScopedValueSetter<bool> loadingScope (ctx.isLoadingParameters, true);
        for (auto& row : rows)
            loadRow (*row);
        refreshReverbState();
        refreshPresetDimming();
    }

    /** After an edit to a value a preset owns, the state may have made the
        reverb Custom: show the preset combo as it now is - and, for the
        model itself, the rows the new model uses. */
    void afterReverbEdit (Row& r)
    {
        if (! isReverb() || ! r.presetOwned)
            return;

        const juce::ScopedValueSetter<bool> loadingScope (ctx.isLoadingParameters, true);
        for (auto& row : rows)
            if (row->desc->id == WFSParameterIDs::effectReverbType)
                loadRow (*row);
        if (r.desc->id == WFSParameterIDs::effectReverbModel)
            refreshReverbState();
        refreshPresetDimming();
    }

    /** The rows the stored model uses, resolved the way the engine resolves
        it; a relayout when that set changes, and word to the deck when the
        model on show does. */
    void refreshReverbState()
    {
        if (! isReverb())
            return;

        const int model = spatcore::effects::resolveReverbModel (ctx.readInt (WFSParameterIDs::effectReverbModel, 0));
        bool changed = false;

        for (auto& row : rows)
        {
            const bool show = EffectsUi::isVisibleForModel (*row->desc, model);
            if (row->shown != show)
            {
                row->shown = show;
                changed = true;
            }
            row->label.setVisible (show);
            row->control->setVisible (show);
            row->value.setVisible (show);
        }

        if (changed)
            resized();

        if (model != shownModel)
        {
            shownModel = model;
            if (onReverbModelShown != nullptr)
                onReverbModelShown (model);
        }
    }

    /** The Preset combo in the menu's own order: each model's presets under
        its name, then Custom - the one id with no row - last. */
    static void addReverbPresetItems (juce::ComboBox& c, const EffectsUi::ControlDesc& d)
    {
        namespace fx = spatcore::effects;
        const auto& models = EffectsUi::controlsForReverb().controls[1];    // the Model row: its names

        for (const auto& model : models.items)
        {
            bool heading = false;
            for (const auto& item : d.items)
            {
                const auto* row = fx::findReverbPreset (item.value);
                if (row == nullptr || static_cast<int> (row->model) != model.value)
                    continue;
                if (! heading)
                {
                    c.addSectionHeading (LOC (juce::String (models.enumPrefix) + model.slug));
                    heading = true;
                }
                c.addItem (LOC (juce::String (d.enumPrefix) + item.slug), item.value + 1);
            }
        }

        c.addSeparator();
        for (const auto& item : d.items)
            if (fx::findReverbPreset (item.value) == nullptr)
                c.addItem (LOC (juce::String (d.enumPrefix) + item.slug), item.value + 1);
    }

    void refreshPresetDimming()
    {
        if (! isReverb())
            return;

        const bool custom = ctx.readInt (WFSParameterIDs::effectReverbType, 0)
                            == static_cast<int> (spatcore::effects::ReverbType::Custom);
        for (auto& row : rows)
            if (row->presetOwned)
                setRowAlpha (*row, custom ? 1.0f : 0.55f);
    }

    //==========================================================================
    // Greying out: the module's state times each control's own alpha
    //==========================================================================

    static constexpr float offModuleAlpha = 0.4f;

    static const juce::Identifier& ownAlphaKey()
    {
        static const juce::Identifier id ("fxOwnAlpha");
        return id;
    }

    /** The alpha a control would have with the module on; the module's
        state is applied on top, here and in refreshModuleOn. */
    void setOwnAlpha (juce::Component& c, float alpha)
    {
        c.getProperties().set (ownAlphaKey(), alpha);
        c.setAlpha (alpha * moduleFactorFor (c));
    }

    void setRowAlpha (Row& r, float alpha)
    {
        setOwnAlpha (r.label, alpha);
        setOwnAlpha (*r.control, alpha);
        setOwnAlpha (r.value, alpha);
    }

    float moduleFactorFor (const juce::Component& c) const
    {
        // The ON button stays readable, and the EQ display says EQ OFF itself
        const bool exempt = &c == bypassControl || &c == eqDisplay.get();
        return moduleOn || exempt ? 1.0f : offModuleAlpha;
    }

    bool readModuleOn()
    {
        for (const auto& row : rows)
            if (row->desc->kind == EffectsUi::Kind::Bypass)
                return readInt (*row->desc) == 0;
        return true;
    }

    /** The module's ON state, everywhere this panel shows it. */
    void refreshModuleOn()
    {
        moduleOn = readModuleOn();

        for (auto* c : getChildren())
            c->setAlpha (static_cast<float> (c->getProperties().getWithDefault (ownAlphaKey(), 1.0f))
                         * moduleFactorFor (*c));

        if (eqDisplay != nullptr)
            eqDisplay->setEQEnabled (moduleOn);

        repaint();

        if (onModuleOnChanged != nullptr)
            onModuleOnChanged();
    }

    /** Dynamics: the compressor's rows grey out while it is off, and the
        expander's likewise. Their On toggles stay bright. */
    void refreshSectionDimming()
    {
        if (! isDyn())
            return;

        const auto sectionOn = [this] (const juce::Identifier& id, int fallback)
        {
            const auto v = ctx.readModule (node, id);
            return (v.isVoid() ? fallback : static_cast<int> (v)) != 0;
        };
        const bool compOn = sectionOn (WFSParameterIDs::effectDynCompOn, WFSParameterDefaults::effectDynCompOnDefault);
        const bool expOn  = sectionOn (WFSParameterIDs::effectDynExpOn,  WFSParameterDefaults::effectDynExpOnDefault);

        for (auto& row : rows)
        {
            const juce::String key (row->desc->key);
            if (row->desc->id == WFSParameterIDs::effectDynCompOn || row->desc->id == WFSParameterIDs::effectDynExpOn)
                continue;
            if (key.startsWith ("dynComp"))
                setRowAlpha (*row, compOn ? 1.0f : 0.45f);
            else if (key.startsWith ("dynExp"))
                setRowAlpha (*row, expOn ? 1.0f : 0.45f);
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

            row.time.setTrackColours (juce::Colour (0xFF1E1E1E), slotColour (slot));
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
            setOwnAlpha (row.name, rowAlpha);
            setOwnAlpha (row.level, rowAlpha);
            setOwnAlpha (row.levelValue, rowAlpha);
            const float timeAlpha = t < active && ! pattern ? 1.0f : 0.35f;
            setOwnAlpha (row.time, timeAlpha);
            setOwnAlpha (row.timeValue, timeAlpha);
        }
    }

    void layoutTaps (juce::Rectangle<int> area)
    {
        const int headerH = scaled (30);
        const int nameW = scaled (60);
        const int valueW = scaled (70);
        const int gap = scaled (12);

        auto header = area.removeFromTop (headerH);
        tapsHeader.setBounds (header.removeFromLeft (nameW));
        auto half = header.getWidth() / 2;
        tapTimeHeader.setBounds (header.removeFromLeft (half));
        tapLevelHeader.setBounds (header);
        area.removeFromTop (scaled (4));

        // The standard slider height when the column allows it; the padding
        // between taps is what gives way first
        const int pitch = area.getHeight() / numTaps;
        const int rowH = juce::jmin (scaled (40), pitch - scaled (4));

        for (int t = 0; t < numTaps; ++t)
        {
            auto& row = taps[static_cast<size_t> (t)];
            auto line = area.removeFromTop (pitch).withSizeKeepingCentre (area.getWidth(), rowH);
            row.name.setBounds (line.removeFromLeft (nameW));
            auto timeArea = line.removeFromLeft (line.getWidth() / 2);
            timeArea.removeFromRight (gap);
            row.timeValue.setBounds (timeArea.removeFromRight (valueW));
            row.time.setBounds (timeArea);
            row.levelValue.setBounds (line.removeFromRight (valueW));
            row.level.setBounds (line);
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

        setOwnAlpha (band.shape, alpha);
        setOwnAlpha (band.freqLabel, alpha);
        setOwnAlpha (band.freq, alpha);
        setOwnAlpha (band.freqValue, alpha);
        for (auto* c : { static_cast<juce::Component*> (&band.gainLabel), static_cast<juce::Component*> (&band.gain), static_cast<juce::Component*> (&band.gainValue) })
            setOwnAlpha (*c, on && ! cutOrPass ? 1.0f : 0.4f);
        for (auto* c : { static_cast<juce::Component*> (&band.qLabel), static_cast<juce::Component*> (&band.q), static_cast<juce::Component*> (&band.qValue) })
            setOwnAlpha (*c, on && ! shelf ? 1.0f : 0.4f);
        for (auto* c : { static_cast<juce::Component*> (&band.slopeLabel), static_cast<juce::Component*> (&band.slope), static_cast<juce::Component*> (&band.slopeValue) })
            setOwnAlpha (*c, on && shelf ? 1.0f : 0.4f);
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
        const int rowH = scaled (34);
        const int labelH = scaled (18);
        const int sliderH = scaled (40);        // the app's standard slider height
        const int gap = scaled (8);
        const int toggleSize = scaled (22);
        const int dialSize = juce::jmax (36, static_cast<int> (52.0f * layoutScale));

        // The header line as in every other module: the ON button (the one
        // descriptor row) at the left edge; Flatten right, as EQ has no Mix
        auto top = area.removeFromTop (scaled (40));
        const int inset = juce::jmax (1, top.getHeight() / 14);
        if (! rows.empty())
        {
            rows[0]->label.setBounds ({});
            rows[0]->control->setBounds (top.withWidth (juce::jmin (top.getWidth(), scaled (220))).reduced (0, inset));
            rows[0]->value.setBounds ({});
        }
        eqFlattenButton.setBounds (top.removeFromRight (scaled (140)).reduced (0, inset));
        area.removeFromTop (scaled (16));

        // The band strips take what they need; the display takes the rest
        const int bandsH = labelH + rowH + gap + labelH + sliderH + labelH + gap + dialSize + labelH * 2;
        if (eqDisplay != nullptr)
        {
            eqDisplay->setBounds (area.removeFromTop (juce::jmax (140, area.getHeight() - bandsH - gap * 2)));
            area.removeFromTop (gap * 2);
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

    int scaled (int ref) const
    {
        return juce::jmax (static_cast<int> (ref * 0.65f), static_cast<int> (ref * layoutScale));
    }

    EffectsTabContext& ctx;
    EffectsFieldEditing fields { ctx, *this };
    const int slot;
    const juce::Identifier node;
    const char* const token;
    float layoutScale = 1.0f;

    std::vector<std::unique_ptr<Row>> rows;
    int shownModel = -1;                // reverb: the resolved model the rows follow
    bool moduleOn = true;               // the stored bypass, as last shown
    juce::Component* bypassControl = nullptr;

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
