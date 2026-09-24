#pragma once

#include <JuceHeader.h>
#include <array>
#include "EffectsTabContext.h"
#include "EffectsModulePanel.h"
#include "../ColorScheme.h"
#include "../../Parameters/WFSParameterIDs.h"
#include "../../Parameters/WFSParameterDefaults.h"
#include "../../Localization/LocalizationManager.h"
#include "../../../spatcore/effects/EffectsTypes.h"

/**
    The Chain sub-tab: what this channel's eleven modules are, in what order,
    and the parameters of the one selected.

    THE TOP ROW SAYS WHO ELSE HEARS AN EDIT. An effect channel is not a
    reverb: it can sit in a link group, and then its chain order, its
    bypasses and every module parameter are shared with the other members
    according to each member's link mode, while its movements, sends, mutes
    and position never are. The badge names the group, the mode and how
    many channels an edit reaches, or says the chain is this channel's
    alone. Ctrl-drag bypasses the group for one gesture (the funnel's rule).

    THE STRIP IS THE ORDER. Eleven tiles in effectChainOrder order; click
    selects, drag reorders and writes a new order string (a validated
    permutation, absolute-only through a group). Each tile carries the
    module's ON dot and its meter from the engine: output peak, or gain
    reduction for Dynamics.
*/
class EffectsChainPanel : public juce::Component
{
public:
    static constexpr int numSlots = spatcore::effects::kNumModuleSlots;

    explicit EffectsChainPanel (EffectsTabContext& context) : ctx (context)
    {
        addAndMakeVisible (linkBadge);
        linkBadge.setJustificationType (juce::Justification::centredLeft);
        linkBadge.setColour (juce::Label::textColourId, ColorScheme::get().textSecondary);
        ctx.helpTextMap[&linkBadge] = LOC ("effects.help.linkBadge");

        addAndMakeVisible (latencyLabel);
        latencyLabel.setJustificationType (juce::Justification::centredRight);
        latencyLabel.setColour (juce::Label::textColourId, ColorScheme::get().textSecondary);
        latencyLabel.setText (LOC ("effects.chain.latencyIdle"), juce::dontSendNotification);
        ctx.helpTextMap[&latencyLabel] = LOC ("effects.help.chainLatency");

        addAndMakeVisible (chainBypassButton);
        chainBypassButton.onClick = [this]
        {
            const bool bypassed = ctx.readInt (WFSParameterIDs::effectChainBypass, 0) != 0;
            ctx.beginGesture ("Effect Chain Bypass");
            ctx.write (WFSParameterIDs::effectChainBypass, bypassed ? 0 : 1);
            refreshChainBypass();
        };
        ctx.helpTextMap[&chainBypassButton] = LOC ("effects.help.chainBypass");

        addAndMakeVisible (hintLabel);
        hintLabel.setText (LOC ("effects.chain.reorderHint"), juce::dontSendNotification);
        hintLabel.setColour (juce::Label::textColourId, ColorScheme::get().textDisabled);
        hintLabel.setJustificationType (juce::Justification::centredLeft);

        for (int s = 0; s < numSlots; ++s)
        {
            tiles[static_cast<size_t> (s)] = std::make_unique<Tile> (*this, s);
            addAndMakeVisible (*tiles[static_cast<size_t> (s)]);
            ctx.helpTextMap[tiles[static_cast<size_t> (s)].get()] = LOC ("effects.help.chainTile");

            panels[static_cast<size_t> (s)] = std::make_unique<EffectsModulePanel> (ctx, s);
            addChildComponent (*panels[static_cast<size_t> (s)]);

            // The panel's ON button is the one place a module is switched
            // here, and the tab does not reload for its own writes
            panels[static_cast<size_t> (s)]->onModuleOnChanged = [this, s] { tiles[static_cast<size_t> (s)]->refresh(); };
        }

        // The reverb panel's rows follow its model; whoever lays out a view of
        // those rows elsewhere (the Stream Deck) hears when they change.
        panels[8]->onReverbModelShown = [this] (int model) { if (onReverbModelShown) onReverbModelShown (model); };

        order = defaultOrder();
        selectedSlot = order[0];
        showSelected();
    }

    /** The channel changed or the tree was reloaded: order, bypass, badge, every panel. */
    void loadParameters()
    {
        readOrder();
        refreshChainBypass();
        refreshLinkBadge();

        for (auto& p : panels)
            p->loadParameters();

        for (auto& t : tiles)
            t->refresh();

        resized();
    }

    /** 50 Hz, for the channel shown. latencySamples < 0 means "no engine". */
    void setLiveState (int latencySamples, double sampleRate, const std::array<float, numSlots>& slotMetersDb)
    {
        if (latencySamples < 0)
        {
            latencyLabel.setText (LOC ("effects.chain.latencyIdle"), juce::dontSendNotification);
        }
        else if (latencySamples != lastLatencySamples)
        {
            lastLatencySamples = latencySamples;
            const double ms = sampleRate > 0.0 ? 1000.0 * latencySamples / sampleRate : 0.0;
            latencyLabel.setText (LOC ("effects.chain.latency")
                                      .replace ("{samples}", juce::String (latencySamples))
                                      .replace ("{ms}", juce::String (ms, 2)),
                                  juce::dontSendNotification);
        }

        for (int s = 0; s < numSlots; ++s)
        {
            tiles[static_cast<size_t> (s)]->setMeterDb (slotMetersDb[static_cast<size_t> (s)]);
            panels[static_cast<size_t> (s)]->setMeterDb (slotMetersDb[static_cast<size_t> (s)]);
        }
    }

    int getSelectedSlot() const noexcept { return selectedSlot; }

    /** Fired when the reverb panel starts following another model. */
    std::function<void (int resolvedModel)> onReverbModelShown;

    /** One slot's panel, for the offscreen render. */
    EffectsModulePanel& getModulePanel (int slot) { return *panels[static_cast<size_t> (juce::jlimit (0, numSlots - 1, slot))]; }

    /** Fired when the selection changes, from a click, a drop or selectSlot. */
    std::function<void (int slot)> onSlotSelected;

    void selectSlot (int slot)
    {
        if (slot < 0 || slot >= numSlots || slot == selectedSlot)
            return;
        selectedSlot = slot;
        showSelected();
        for (auto& t : tiles)
            t->refresh();
        if (onSlotSelected)
            onSlotSelected (selectedSlot);
    }

    /** The order string with the tile at position `from` moved to position
        `to`; pure, so the self-test can hold it to account. An unparsable
        input comes back unchanged. */
    static juce::String movedOrder (const juce::String& csv, int from, int to)
    {
        namespace fx = spatcore::effects;
        fx::ChainOrder o = fx::kDefaultOrder;
        if (! fx::parseChainOrder (csv.toRawUTF8(), o))
            return csv;

        if (from < 0 || from >= numSlots || to < 0 || to >= numSlots || from == to)
            return orderToString (o);

        const auto moving = o[static_cast<size_t> (from)];
        if (from < to)
            for (int i = from; i < to; ++i) o[static_cast<size_t> (i)] = o[static_cast<size_t> (i + 1)];
        else
            for (int i = from; i > to; --i) o[static_cast<size_t> (i)] = o[static_cast<size_t> (i - 1)];
        o[static_cast<size_t> (to)] = moving;
        return orderToString (o);
    }

    /** The order a new channel is given (WFSParameterDefaults), which is not
        the engine's declared slot order. */
    static spatcore::effects::ChainOrder defaultOrder()
    {
        auto o = spatcore::effects::kDefaultOrder;
        spatcore::effects::parseChainOrder (WFSParameterDefaults::effectChainOrderDefault.toRawUTF8(), o);
        return o;
    }

    static juce::String orderToString (const spatcore::effects::ChainOrder& o)
    {
        juce::StringArray tokens;
        for (auto slot : o)
            tokens.add (spatcore::effects::kSlots[static_cast<size_t> (slot)].token);
        return tokens.joinIntoString (",");
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (ColorScheme::get().surfaceCard);
        g.fillRoundedRectangle (stripBounds.toFloat(), 4.0f);
    }

    void resized() override
    {
        layoutScale = static_cast<float> (getHeight()) / 780.0f;
        auto area = getLocalBounds().reduced (scaled (10), scaled (6));
        const int rowH = scaled (28);
        const int gap = scaled (6);

        auto top = area.removeFromTop (rowH);
        chainBypassButton.setBounds (top.removeFromRight (scaled (160)));
        top.removeFromRight (gap * 2);
        latencyLabel.setBounds (top.removeFromRight (scaled (260)));
        linkBadge.setBounds (top);
        area.removeFromTop (gap);

        hintLabel.setBounds (area.removeFromTop (scaled (18)));
        area.removeFromTop (gap / 2);

        stripBounds = area.removeFromTop (scaled (64));
        layoutTiles();
        area.removeFromTop (gap * 2);

        for (auto& p : panels)
            p->setBounds (area);
    }

private:
    //==========================================================================
    struct Tile : public juce::Component
    {
        Tile (EffectsChainPanel& o, int s) : owner (o), slot (s) {}

        void refresh()
        {
            on = static_cast<int> (owner.ctx.readModule (WFSValueTreeState::getEffectModuleType (slot),
                                                          bypassIdFor (slot))) == 0;
            repaint();
        }

        void setMeterDb (float db)
        {
            if (std::abs (db - meterDb) > 0.5f)
            {
                meterDb = db;
                repaint();
            }
        }

        void paint (juce::Graphics& g) override
        {
            const bool selected = owner.selectedSlot == slot;
            const auto& cs = ColorScheme::get();
            const auto hue = EffectsModulePanel::slotColour (slot);
            auto r = getLocalBounds().toFloat().reduced (2.0f);

            // Tinted in the module's hue - the panel below wears the same -
            // stronger when on or selected, nearly grey when off
            const float tint = on ? (selected ? 0.45f : 0.26f) : (selected ? 0.2f : 0.08f);
            auto fill = cs.buttonNormal.interpolatedWith (hue, tint);
            g.setColour (dragging ? fill.brighter (0.2f) : fill);
            g.fillRoundedRectangle (r, 4.0f);

            // The hue band across the top names the module before the text does
            g.setColour (on ? hue : hue.withAlpha (0.35f));
            g.fillRoundedRectangle (r.reduced (4.0f, 0.0f).withHeight (4.0f).translated (0.0f, 2.0f), 2.0f);

            g.setColour (selected ? cs.textPrimary : hue.withAlpha (on ? 0.6f : 0.25f));
            g.drawRoundedRectangle (r, 4.0f, selected ? 2.0f : 1.0f);

            // The ON dot
            const float d = 8.0f;
            auto dot = juce::Rectangle<float> (r.getX() + 5.0f, r.getY() + 9.0f, d, d);
            g.setColour (on ? juce::Colour (0xFF4CAF50) : ColorScheme::get().textDisabled.withAlpha (0.35f));
            g.fillEllipse (dot);

            // The meter: a thin bar down the right edge. Dynamics report gain
            // reduction (0 dB = none, drawn downwards); everything else its
            // output peak (drawn upwards from -60 dB).
            auto bar = juce::Rectangle<float> (r.getRight() - 7.0f, r.getY() + 9.0f, 3.0f, r.getHeight() - 14.0f);
            g.setColour (ColorScheme::get().sliderTrackBg);
            g.fillRect (bar);
            const bool isDyn = slot == 3 || slot == 4;
            if (isDyn)
            {
                // Below -60 dB is the "no engine" sentinel, not 60 dB of reduction
                const float f = meterDb < -60.0f ? 0.0f : juce::jlimit (0.0f, 1.0f, -meterDb / 24.0f);
                g.setColour (juce::Colour (0xFF3498DB));
                g.fillRect (bar.withHeight (bar.getHeight() * f));
            }
            else
            {
                const float f = juce::jlimit (0.0f, 1.0f, (meterDb + 60.0f) / 60.0f);
                g.setColour (juce::Colour (0xFF26A69A));
                g.fillRect (bar.withTop (bar.getBottom() - bar.getHeight() * f));
            }

            g.setColour (on ? ColorScheme::get().textPrimary : ColorScheme::get().textSecondary);
            g.setFont (juce::FontOptions (juce::jmax (11.0f, 13.0f * owner.layoutScale)));
            g.drawFittedText (LOC ("effects.modules." + juce::String (spatcore::effects::kSlots[static_cast<size_t> (slot)].token)),
                              getLocalBounds().reduced (10, 4), juce::Justification::centred, 2);
        }

        void mouseDown (const juce::MouseEvent& e) override
        {
            dragStartX = e.getScreenX();
            dragging = false;
            homeX = getX();
        }

        void mouseDrag (const juce::MouseEvent& e) override
        {
            const int dx = e.getScreenX() - dragStartX;
            if (! dragging && std::abs (dx) > 6)
            {
                dragging = true;
                toFront (false);
            }
            if (dragging)
            {
                setTopLeftPosition (homeX + dx, getY());
                repaint();
            }
        }

        void mouseUp (const juce::MouseEvent&) override
        {
            if (dragging)
            {
                dragging = false;
                owner.tileDropped (slot, getBounds().getCentreX());
            }
            else
            {
                owner.selectSlot (slot);
            }
        }

        EffectsChainPanel& owner;
        const int slot;
        bool on = true;
        bool dragging = false;
        float meterDb = -120.0f;
        int dragStartX = 0, homeX = 0;
    };

    static const juce::Identifier& bypassIdFor (int slot)
    {
        using namespace WFSParameterIDs;
        switch (slot)
        {
            case 0:  return effectDistBypass;
            case 1:  case 2: return effectEQBypass;
            case 3:  case 4: return effectDynBypass;
            case 5:  return effectModBypass;
            case 6:  return effectPhaserBypass;
            case 7:  return effectTremBypass;
            case 8:  return effectReverbBypass;
            case 9:  return effectDelayBypass;
            default: return effectCrushBypass;
        }
    }

    //==========================================================================
    void readOrder()
    {
        namespace fx = spatcore::effects;
        fx::ChainOrder o = fx::kDefaultOrder;
        const auto csv = ctx.read (WFSParameterIDs::effectChainOrder).toString();
        if (fx::parseChainOrder (csv.toRawUTF8(), o))
            order = o;
        else
            order = defaultOrder();
    }

    int positionOf (int slot) const
    {
        for (int p = 0; p < numSlots; ++p)
            if (order[static_cast<size_t> (p)] == slot)
                return p;
        return 0;
    }

    void layoutTiles()
    {
        const int gap = scaled (6);
        auto strip = stripBounds.reduced (gap, gap);
        const int tileW = (strip.getWidth() - gap * (numSlots - 1)) / numSlots;

        for (int p = 0; p < numSlots; ++p)
        {
            auto& tile = *tiles[static_cast<size_t> (order[static_cast<size_t> (p)])];
            tile.setBounds (strip.getX() + p * (tileW + gap), strip.getY(), tileW, strip.getHeight());
        }
    }

    void tileDropped (int slot, int centreX)
    {
        const int gap = scaled (6);
        auto strip = stripBounds.reduced (gap, gap);
        const int tileW = juce::jmax (1, (strip.getWidth() - gap * (numSlots - 1)) / numSlots);
        const int to = juce::jlimit (0, numSlots - 1, (centreX - strip.getX()) / (tileW + gap));
        const int from = positionOf (slot);

        if (to != from)
        {
            const auto next = movedOrder (orderToString (order), from, to);
            ctx.beginGesture ("Effect Chain Order");
            ctx.write (WFSParameterIDs::effectChainOrder, next);
            readOrder();
        }

        layoutTiles();
        selectSlot (slot);
    }

    void showSelected()
    {
        for (int s = 0; s < numSlots; ++s)
            panels[static_cast<size_t> (s)]->setVisible (s == selectedSlot);
    }

    void refreshChainBypass()
    {
        const bool bypassed = ctx.readInt (WFSParameterIDs::effectChainBypass, 0) != 0;
        chainBypassButton.setButtonText (LOC (bypassed ? "effects.chain.bypassOff" : "effects.chain.bypassOn"));
        chainBypassButton.setColour (juce::TextButton::buttonColourId,
                                     bypassed ? juce::Colour (0xFFCC8800) : ColorScheme::get().buttonNormal);
    }

    void refreshLinkBadge()
    {
        auto& vts = ctx.parameters.getValueTreeState();
        const int group = vts.getEffectLinkGroup (ctx.slot());
        const int mode = vts.getEffectLinkMode (ctx.slot());

        if (group <= 0)
        {
            linkBadge.setText (LOC ("effects.chain.unlinked"), juce::dontSendNotification);
            return;
        }

        int others = 0;
        const int numEffects = ctx.parameters.getNumEffectChannels();
        for (int i = 0; i < numEffects; ++i)
            if (i != ctx.slot() && vts.getEffectLinkGroup (i) == group)
                ++others;

        juce::StringArray names;
        names.addTokens (ctx.parameters.getConfigParam ("effectsGlobalLinkNames").toString(), ",", "");
        const auto custom = names[group - 1].trim();
        const auto groupName = custom.isNotEmpty() ? custom : LOC ("effects.link.group") + " " + juce::String (group);
        const auto modeName = LOC (mode == 0 ? "effects.link.modeOff" : mode == 1 ? "effects.link.modeAbsolute" : "effects.link.modeRelative");

        const char* key = mode == 0 ? "effects.chain.linkedOff" : others == 0 ? "effects.chain.linkedAlone" : "effects.chain.linked";
        linkBadge.setText (LOC (key).replace ("{group}", groupName)
                                    .replace ("{mode}", modeName)
                                    .replace ("{count}", juce::String (others)),
                           juce::dontSendNotification);
    }

    int scaled (int ref) const
    {
        return juce::jmax (static_cast<int> (ref * 0.65f), static_cast<int> (ref * layoutScale));
    }

    //==========================================================================
    EffectsTabContext& ctx;
    float layoutScale = 1.0f;
    juce::Rectangle<int> stripBounds;
    spatcore::effects::ChainOrder order {};
    int selectedSlot = 0;
    int lastLatencySamples = -1;

    juce::Label linkBadge, latencyLabel, hintLabel;
    juce::TextButton chainBypassButton;
    std::array<std::unique_ptr<Tile>, static_cast<size_t> (numSlots)> tiles;
    std::array<std::unique_ptr<EffectsModulePanel>, static_cast<size_t> (numSlots)> panels;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectsChainPanel)
};
