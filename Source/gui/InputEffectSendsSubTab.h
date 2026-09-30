#pragma once

#include <JuceHeader.h>
#include "../WfsParameters.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/WFSParameterDefaults.h"
#include "../Localization/LocalizationManager.h"
#include "../Accessibility/TTSManager.h"
#include "ColorScheme.h"
#include "WfsLookAndFeel.h"
#include "sliders/WfsStandardSlider.h"
#include "StatusBar.h"
#include "HelpCard.h"

/**
    The Inputs tab's "Effect Sends" sub-tab: the selected input's sends into
    every effect channel, one strip per effect.

    THE SAME CELLS AS THE EFFECTS TAB'S MATRIX, SEEN FROM THE INPUT. The
    Post-Processing sub-tab of the Effects tab shows the whole grid - every
    source as a row, every effect as a column. This is one row of it turned
    into a mixer bank: a fader for the level and a switch for the on/off,
    so an operator working on an input reaches its sends without leaving the
    tab, and can mute a send and bring it back at the level it had.

    THE STRIPS OWN NOTHING. Every value is read through the typed accessors
    (getEffectSendLevelFromInput and friends), which key the packed rows by
    the input's PERMANENT number, and every write goes back through their
    setters, so what lands is the same whether it came from here, from the
    matrix or from OSC. The <Sends> nodes are listened to for everyone else's
    writes; our own are already on screen (isSelfWriting).

    THE LEVEL LAW is the one the array-attenuation dials and the matrix cells
    use (WFS-UI_effects.csv, effectSendLevel): linear = min + v² (1 - min),
    in dB, so the top of the fader is fine and the bottom sweeps fast.
*/
class InputEffectSendsSubTab : public juce::Component,
                               public ColorScheme::Manager::Listener,
                               private juce::ValueTree::Listener
{
public:
    explicit InputEffectSendsSubTab (WfsParameters& params)
        : parameters (params)
    {
        setOpaque (false);
        ColorScheme::Manager::getInstance().addListener (this);

        addAndMakeVisible (allOnButton);
        allOnButton.setButtonText (LOC ("inputs.effectSends.allOn"));
        allOnButton.onClick = [this] { setAllSends (true); };

        addAndMakeVisible (allOffButton);
        allOffButton.setButtonText (LOC ("inputs.effectSends.allOff"));
        allOffButton.onClick = [this] { setAllSends (false); };

        addAndMakeVisible (hintLabel);
        hintLabel.setText (LOC ("inputs.effectSends.hint"), juce::dontSendNotification);
        hintLabel.setJustificationType (juce::Justification::centredLeft);
        hintLabel.setMinimumHorizontalScale (0.8f);

        addChildComponent (emptyLabel);
        emptyLabel.setText (LOC ("inputs.effectSends.none"), juce::dontSendNotification);
        emptyLabel.setJustificationType (juce::Justification::centred);

        viewport.setViewedComponent (&stripsContent, false);
        viewport.setScrollBarsShown (false, true, false, true);
        addAndMakeVisible (viewport);

        addAndMakeVisible (helpButton);
        addChildComponent (helpCard);
        helpCard.setContent (LOC ("help.inputEffectSends.title"), LOC ("help.inputEffectSends.body"));
        helpButton.setCard (&helpCard);

        applyColours();
        attachListeners();
    }

    ~InputEffectSendsSubTab() override
    {
        detachListeners();
        ColorScheme::Manager::getInstance().removeListener (this);
    }

    //==========================================================================
    // Public API, the shape SamplerSubTab has

    /** The input shown, by PERMANENT number (what the send rows are keyed by).
        0 or a dead number shows nothing. Always reloads: the Inputs tab calls
        this after every load, and a load may have replaced the trees. */
    void setCurrentChannel (int inputPermanentNumber)
    {
        currentInput = inputPermanentNumber;
        refresh();
    }

    /** Re-read every strip from the tree, re-attaching to the trees a load
        may have replaced, and rebuilding the bank when the effect count moved. */
    void refresh()
    {
        detachListeners();
        attachListeners();
        rebuildStripsIfNeeded();
        loadValues();
    }

    void setStatusBar (StatusBar* bar)
    {
        statusBar = bar;
        setupHelpText();
    }

    HelpCardButton& getHelpButton() { return helpButton; }

    /** The strips the Stream Deck's Effect Sends page holds: `firstEffect`
        (dense, 0-based) and `count` of them, outlined and scrolled into view.
        A negative first clears the mark (no deck, or the page went away). */
    void setDeckWindow (int firstEffect, int count)
    {
        deckFirst = firstEffect;
        deckCount = firstEffect < 0 ? 0 : count;
        for (auto* strip : strips)
        {
            const bool held = strip->effectIndex >= deckFirst && strip->effectIndex < deckFirst + deckCount;
            if (strip->onDeck != held)
            {
                strip->onDeck = held;
                strip->repaint();
            }
        }
        scrollDeckWindowIntoView();
    }

    //==========================================================================
    void resized() override
    {
        layoutScale = static_cast<float> (getHeight()) / 700.0f;

        auto area = getLocalBounds().reduced (scaled (6));

        auto top = area.removeFromTop (scaled (28));
        const int btnSize = scaled (22);
        helpButton.setBounds (top.removeFromRight (btnSize).withSizeKeepingCentre (btnSize, btnSize));
        top.removeFromRight (scaled (8));
        allOnButton.setBounds (top.removeFromLeft (scaled (170)));
        top.removeFromLeft (scaled (6));
        allOffButton.setBounds (top.removeFromLeft (scaled (170)));
        top.removeFromLeft (scaled (12));
        hintLabel.setBounds (top);
        hintLabel.setFont (juce::FontOptions (juce::jmax (10.0f, 13.0f * layoutScale)));
        area.removeFromTop (scaled (6));

        bankArea = area;
        viewport.setBounds (area);
        emptyLabel.setBounds (area);
        emptyLabel.setFont (juce::FontOptions (juce::jmax (12.0f, 16.0f * layoutScale)));

        layoutStrips();

        // Help card, centred over the bank.
        const int cardW = juce::jmin (area.getWidth() - 40, 700);
        const int cardH = helpCard.getIdealHeight (cardW);
        helpCard.setBounds (area.getX() + (area.getWidth() - cardW) / 2, area.getY() + 40, cardW, cardH);
    }

    void colorSchemeChanged() override
    {
        applyColours();
        repaint();
    }

private:
    //==========================================================================
    // One effect's strip: header, fader, readout, switch.

    struct Strip : public juce::Component
    {
        Strip()
            : fader (0.0f, 1.0f, WfsSliderBase::Orientation::vertical)
        {
            addAndMakeVisible (header);
            header.setJustificationType (juce::Justification::centred);
            header.setMinimumHorizontalScale (0.7f);

            addAndMakeVisible (fader);
            fader.setThumbRadius (7.0f);

            addAndMakeVisible (valueLabel);
            valueLabel.setJustificationType (juce::Justification::centred);

            addAndMakeVisible (onButton);
            onButton.setClickingTogglesState (false);   // the state is the tree's, not the button's
        }

        void paint (juce::Graphics& g) override
        {
            g.setColour (ColorScheme::get().surfaceCard);
            g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);

            // Held by the Stream Deck's Effect Sends page: one of its four.
            if (onDeck)
            {
                g.setColour (kEffectColour.withAlpha (0.9f));
                g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 6.0f, 2.0f);
            }
        }

        void resized() override
        {
            auto r = getLocalBounds().reduced (pad);
            header.setBounds (r.removeFromTop (headerH));
            onButton.setBounds (r.removeFromBottom (buttonH));
            r.removeFromBottom (pad);
            valueLabel.setBounds (r.removeFromBottom (valueH));
            r.removeFromBottom (pad / 2);
            fader.setBounds (r.reduced (juce::jmax (0, (r.getWidth() - faderW) / 2), 0));
        }

        int effectIndex = 0;       // dense, 0-based
        bool onDeck = false;       // held by the Stream Deck page
        int pad = 4, headerH = 40, valueH = 20, buttonH = 26, faderW = 28;

        juce::Label header;
        WfsStandardSlider fader;
        juce::Label valueLabel;
        juce::TextButton onButton;
    };

    //==========================================================================
    // The level law (see the class comment). Shared with the array dials.

    static float minLinear() noexcept
    {
        return std::pow (10.0f, WFSParameterDefaults::effectSendLevelMin / 20.0f);
    }

    static float faderToDb (float v) noexcept
    {
        const float linear = minLinear() + v * v * (1.0f - minLinear());
        return juce::jlimit (WFSParameterDefaults::effectSendLevelMin,
                             WFSParameterDefaults::effectSendLevelMax,
                             20.0f * std::log10 (linear));
    }

    static float dbToFader (float dB) noexcept
    {
        const float linear = std::pow (10.0f, juce::jlimit (WFSParameterDefaults::effectSendLevelMin,
                                                            WFSParameterDefaults::effectSendLevelMax, dB) / 20.0f);
        return std::sqrt (juce::jlimit (0.0f, 1.0f, (linear - minLinear()) / (1.0f - minLinear())));
    }

    static juce::String formatDb (float dB)
    {
        return juce::String (dB, 1) + " dB";
    }

    //==========================================================================
    int scaled (int ref) const
    {
        return juce::jmax (static_cast<int> (ref * 0.65f), static_cast<int> (ref * layoutScale));
    }

    bool hasInput() const
    {
        return currentInput > 0 && parameters.getValueTreeState().getSlotForChannelNumber (currentInput) >= 0;
    }

    juce::String effectLabel (int fx) const
    {
        const auto name = parameters.getValueTreeState().getEffectParameter (fx, WFSParameterIDs::effectName).toString();
        return LOC ("effects.sends.effectPrefix") + " " + juce::String (fx + 1)
             + (name.isNotEmpty() ? "\n" + name : juce::String());
    }

    //==========================================================================
    // Building and loading

    void rebuildStripsIfNeeded()
    {
        const int numEffects = parameters.getNumEffectChannels();
        if (strips.size() == numEffects)
            return;

        strips.clear();
        for (int fx = 0; fx < numEffects; ++fx)
        {
            auto* strip = strips.add (new Strip());
            strip->effectIndex = fx;
            strip->onDeck = fx >= deckFirst && fx < deckFirst + deckCount;
            stripsContent.addAndMakeVisible (strip);

            strip->fader.onGestureStart = [this]
            {
                parameters.getValueTreeState().beginUndoTransaction (LOC ("effects.sends.gesture.level"));
            };
            strip->fader.onValueChanged = [this, strip] (float v)
            {
                if (isLoading || ! hasInput())
                    return;

                const float dB = faderToDb (v);
                strip->valueLabel.setText (formatDb (dB), juce::dontSendNotification);

                const juce::ScopedValueSetter<bool> selfWriteScope (isSelfWriting, true);
                parameters.getValueTreeState().setEffectSendLevelFromInput (strip->effectIndex, currentInput, dB);
            };
            strip->onButton.onClick = [this, strip]
            {
                if (! hasInput())
                    return;

                auto& vts = parameters.getValueTreeState();
                const bool on = ! vts.getEffectSendOnFromInput (strip->effectIndex, currentInput);
                vts.beginUndoTransaction (LOC ("effects.sends.gesture.toggle"));
                {
                    const juce::ScopedValueSetter<bool> selfWriteScope (isSelfWriting, true);
                    vts.setEffectSendOnFromInput (strip->effectIndex, currentInput, on);
                }
                showSwitch (*strip, on);
                TTSManager::getInstance().announceImmediate (
                    effectLabel (strip->effectIndex).replaceCharacter ('\n', ' ') + " "
                        + LOC (on ? "effects.sends.announce.on" : "effects.sends.announce.off"),
                    juce::AccessibilityHandler::AnnouncementPriority::medium);
            };
        }

        applyColours();
        setupHelpText();
        layoutStrips();
    }

    void loadValues()
    {
        const juce::ScopedValueSetter<bool> loadingScope (isLoading, true);
        auto& vts = parameters.getValueTreeState();
        const bool live = hasInput();

        for (auto* strip : strips)
        {
            strip->header.setText (effectLabel (strip->effectIndex), juce::dontSendNotification);
            strip->fader.setTTSInfo (LOC ("effects.sends.gesture.level") + " "
                                         + effectLabel (strip->effectIndex).replaceCharacter ('\n', ' '),
                                     LOC ("units.decibels"));

            const float dB = live ? vts.getEffectSendLevelFromInput (strip->effectIndex, currentInput)
                                  : WFSParameterDefaults::effectSendLevelDefault;
            strip->fader.setValue (dbToFader (dB));
            strip->valueLabel.setText (formatDb (dB), juce::dontSendNotification);
            showSwitch (*strip, live && vts.getEffectSendOnFromInput (strip->effectIndex, currentInput));
            strip->setEnabled (live);
        }

        const bool any = strips.size() > 0;
        emptyLabel.setVisible (! any);
        viewport.setVisible (any);
        allOnButton.setEnabled (any && live);
        allOffButton.setEnabled (any && live);
    }

    void showSwitch (Strip& strip, bool on)
    {
        strip.onButton.setToggleState (on, juce::dontSendNotification);
        strip.onButton.setButtonText (LOC (on ? "inputs.effectSends.on" : "inputs.effectSends.off"));
        strip.fader.setAlpha (on ? 1.0f : 0.55f);
        strip.valueLabel.setAlpha (on ? 1.0f : 0.55f);
    }

    /** Every send of this input, on or off, in one undo step. Written per
        cell through the typed setters so the row rules hold. */
    void setAllSends (bool on)
    {
        if (! hasInput())
            return;

        auto& vts = parameters.getValueTreeState();
        vts.beginUndoTransaction (LOC (on ? "inputs.effectSends.gesture.allOn" : "inputs.effectSends.gesture.allOff"));

        const juce::ScopedValueSetter<bool> selfWriteScope (isSelfWriting, true);
        for (auto* strip : strips)
        {
            vts.setEffectSendOnFromInput (strip->effectIndex, currentInput, on);
            showSwitch (*strip, on);
        }
    }

    //==========================================================================
    // Layout and colours

    void layoutStrips()
    {
        const int stripW = scaled (84);
        const int gap = scaled (6);
        const int stripH = juce::jmax (scaled (200), bankArea.getHeight() - (strips.size() * stripW + gap > bankArea.getWidth() ? scaled (14) : 0));

        for (auto* strip : strips)
        {
            strip->pad = scaled (4);
            strip->headerH = scaled (40);
            strip->valueH = scaled (20);
            strip->buttonH = scaled (26);
            strip->faderW = scaled (28);
            strip->header.setFont (juce::FontOptions (juce::jmax (9.0f, 12.0f * layoutScale)));
            strip->valueLabel.setFont (juce::FontOptions (juce::jmax (9.0f, 12.0f * layoutScale)));
        }

        stripsContent.setSize (juce::jmax (bankArea.getWidth(), strips.size() * (stripW + gap)), stripH);
        int x = 0;
        for (auto* strip : strips)
        {
            strip->setBounds (x, 0, stripW, stripH);
            x += stripW + gap;
        }
        scrollDeckWindowIntoView();
    }

    /** Scroll the bank so the deck's four strips are on screen, moving as
        little as possible; nothing to do when there is no deck window. */
    void scrollDeckWindowIntoView()
    {
        if (deckFirst < 0 || deckCount <= 0 || strips.isEmpty() || viewport.getWidth() <= 0)
            return;

        const int firstIndex = juce::jlimit (0, strips.size() - 1, deckFirst);
        const int lastIndex  = juce::jlimit (0, strips.size() - 1, deckFirst + deckCount - 1);
        const int left  = strips[firstIndex]->getX();
        const int right = strips[lastIndex]->getRight();

        auto pos = viewport.getViewPosition();
        if (left < pos.x)
            pos.x = left;
        else if (right > pos.x + viewport.getViewWidth())
            pos.x = juce::jmax (0, right - viewport.getViewWidth());
        viewport.setViewPosition (pos);
    }

    void applyColours()
    {
        const auto& scheme = ColorScheme::get();
        hintLabel.setColour (juce::Label::textColourId, scheme.textSecondary);
        emptyLabel.setColour (juce::Label::textColourId, scheme.textSecondary);

        for (auto* strip : strips)
        {
            strip->header.setColour (juce::Label::textColourId, kEffectColour);
            strip->valueLabel.setColour (juce::Label::textColourId, scheme.textPrimary);
            strip->fader.setTrackColours (scheme.sliderTrackBg, kEffectColour);
            strip->onButton.setColour (juce::TextButton::buttonOnColourId, kEffectColour);
        }
    }

    //==========================================================================
    // Listening: any <Sends> node of any effect, an effect renamed, an effect
    // added or removed (the bank is one strip per effect).

    void attachListeners()
    {
        effectsTree = parameters.getValueTreeState().getEffectsState();
        if (effectsTree.isValid())
            effectsTree.addListener (this);
    }

    void detachListeners()
    {
        if (effectsTree.isValid())
            effectsTree.removeListener (this);
    }

    void valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& property) override
    {
        if (isSelfWriting)
            return;                        // our own write is already on screen

        using namespace WFSParameterIDs;
        if (property == effectSendLevels || property == effectSendOns
            || property == effectName || tree.hasType (Sends))
            scheduleRefresh();
    }

    void valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&) override        { scheduleRefresh(); }
    void valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree&, int) override { scheduleRefresh(); }
    void valueTreeChildOrderChanged (juce::ValueTree&, int, int) override        { scheduleRefresh(); }
    void valueTreeParentChanged (juce::ValueTree&) override                      { scheduleRefresh(); }
    void valueTreeRedirected (juce::ValueTree&) override                         { scheduleRefresh(); }

    void scheduleRefresh()
    {
        if (refreshPending)
            return;

        refreshPending = true;
        juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<InputEffectSendsSubTab> (this)]
        {
            if (safe == nullptr)
                return;
            safe->refreshPending = false;
            safe->refresh();
        });
    }

    //==========================================================================
    // Status bar: hover help and the OSC address a strip stands for.

    void setupHelpText()
    {
        helpTextMap.clear();
        oscMethodMap.clear();

        helpTextMap[&allOnButton]  = LOC ("inputs.help.effectSendsAllOn");
        helpTextMap[&allOffButton] = LOC ("inputs.help.effectSendsAllOff");
        helpTextMap[&helpButton]   = LOC ("help.inputEffectSends.title");

        for (auto* strip : strips)
        {
            const juce::String fx (strip->effectIndex + 1);
            helpTextMap[&strip->fader]      = LOC ("inputs.help.effectSendLevel");
            helpTextMap[&strip->valueLabel] = LOC ("inputs.help.effectSendLevel");
            helpTextMap[&strip->onButton]   = LOC ("inputs.help.effectSendToggle");
            oscMethodMap[&strip->fader]      = "/wfs/effect/sendLevel " + fx + " <ID> <value>";
            oscMethodMap[&strip->valueLabel] = "/wfs/effect/sendLevel " + fx + " <ID> <value>";
            oscMethodMap[&strip->onButton]   = "/wfs/effect/sendOn " + fx + " <ID> <0|1>";
        }

        for (auto& [comp, text] : helpTextMap)
            comp->addMouseListener (this, false);
    }

    void mouseEnter (const juce::MouseEvent& e) override
    {
        if (statusBar == nullptr)
            return;

        for (auto* comp = e.eventComponent; comp != nullptr; comp = comp->getParentComponent())
        {
            const auto it = helpTextMap.find (comp);
            if (it == helpTextMap.end())
                continue;

            statusBar->setHelpText (it->second);
            const auto osc = oscMethodMap.find (comp);
            if (osc != oscMethodMap.end())
                statusBar->setOscMethod (osc->second);
            return;
        }
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        if (statusBar != nullptr)
            statusBar->clearText();
    }

    //==========================================================================
    // Effect returns on every map and grid: teal, as the binaural controls.
    static inline const juce::Colour kEffectColour { 0xFF26A69A };

    WfsParameters& parameters;
    int currentInput = 0;              // permanent number; 0 = none
    int deckFirst = -1, deckCount = 0; // the Stream Deck page's window; -1 = none
    float layoutScale = 1.0f;
    bool isLoading = false;
    bool isSelfWriting = false;
    bool refreshPending = false;

    juce::TextButton allOnButton, allOffButton;
    juce::Label hintLabel, emptyLabel;
    juce::Viewport viewport;
    juce::Component stripsContent;
    juce::OwnedArray<Strip> strips;
    juce::Rectangle<int> bankArea;

    HelpCardButton helpButton;
    HelpCard helpCard;

    StatusBar* statusBar = nullptr;
    std::map<juce::Component*, juce::String> helpTextMap;
    std::map<juce::Component*, juce::String> oscMethodMap;

    juce::ValueTree effectsTree;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InputEffectSendsSubTab)
};
