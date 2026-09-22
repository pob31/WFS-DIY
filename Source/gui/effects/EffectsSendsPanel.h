#pragma once

#include <JuceHeader.h>
#include "EffectsTabContext.h"
#include "../ColorScheme.h"
#include "../../../spatcore/ui/sends/SendMatrixConfig.h"
#include "../../../spatcore/ui/sends/SendMatrixComponent.h"
#include "../../Parameters/WFSParameterIDs.h"
#include "../../Parameters/WFSParameterDefaults.h"
#include "../../Localization/LocalizationManager.h"

/** The app's binding of the shared send matrix to the effects send rows.
    Defined in EffectsSendMatrixShim.cpp, the one file in Source/gui/effects
    that names both the widget and the app's schema. */
spatcore::ui::sends::SendMatrixConfig makeEffectsSendMatrixConfig (EffectsTabContext& ctx,
                                                                   const std::function<bool (int column)>& columnInCycle);

/**
    The Post-Processing sub-tab: the sends matrix, which is where this
    effect's output goes next.

    THE WHOLE MATRIX, NOT ONE ROW. Levels 1 and 2 of the effects family are
    one grid - every input and every effect return as a source row, every
    effect as a destination column - and a bunch is a shape in that grid: an
    entry column fed by inputs, the others fed by each other. Showing only
    the selected channel's column would hide the shape that makes it a bunch.
    The selected channel's column is highlighted instead, and the two
    "all sends" buttons act on that column.

    THE WIDGET OWNS NOTHING. It asks the tab context for every cell through
    the config's providers and is told to refresh() when a <Sends> node
    changed under it - by anything other than this panel, whose own writes
    are already on screen. Cycle membership comes from the calculation
    engine through setCycleMask(); the entry badge is derived here from the
    rows, because the entry role is emergent by decision.
*/
class EffectsSendsPanel : public juce::Component,
                          private juce::ValueTree::Listener
{
public:
    explicit EffectsSendsPanel (EffectsTabContext& context)
        : ctx (context),
          matrix (makeEffectsSendMatrixConfig (context, [this] (int column) { return inCycle (column); }))
    {
        addAndMakeVisible (matrix);

        matrix.onGestureStart = [this] (const juce::String& name) { ctx.beginGesture (name); };

        matrix.onCellToggled = [this] (int row, int column, bool on)
        {
            const juce::ScopedValueSetter<bool> selfWriteScope (ctx.isSelfWriting, true);
            auto& vts = ctx.parameters.getValueTreeState();
            const int numInputs = ctx.parameters.getNumInputChannels();

            if (row < numInputs)
                vts.setEffectSendOnFromInput (column, vts.getInputChannelNumber (row), on);
            else
                vts.setEffectFxSendOnFromEffect (column, row - numInputs, on);
        };

        matrix.onCellLevelChanged = [this] (int row, int column, float levelDb)
        {
            const juce::ScopedValueSetter<bool> selfWriteScope (ctx.isSelfWriting, true);
            auto& vts = ctx.parameters.getValueTreeState();
            const int numInputs = ctx.parameters.getNumInputChannels();

            if (row < numInputs)
                vts.setEffectSendLevelFromInput (column, vts.getInputChannelNumber (row), levelDb);
            else
                vts.setEffectFxSendLevelFromEffect (column, row - numInputs, levelDb);
        };

        matrix.onStatusMessage = [this] (const juce::String& m) { ctx.showStatusMessage (m); };

        // "All on / all off for this effect": every send INTO the selected
        // column, in one undo transaction each. Written per cell through the
        // typed accessors so the diagonal and the width rules hold.
        addAndMakeVisible (allOnButton);
        allOnButton.setButtonText (LOC ("effects.sends.allOn"));
        allOnButton.onClick = [this] { setAllSends (true); };

        addAndMakeVisible (allOffButton);
        allOffButton.setButtonText (LOC ("effects.sends.allOff"));
        allOffButton.onClick = [this] { setAllSends (false); };

        addAndMakeVisible (hintLabel);
        hintLabel.setText (LOC ("effects.sends.hint"), juce::dontSendNotification);
        hintLabel.setColour (juce::Label::textColourId, ColorScheme::get().textSecondary);
        hintLabel.setJustificationType (juce::Justification::centredLeft);

        attachListeners();
    }

    ~EffectsSendsPanel() override
    {
        detachListeners();
    }

    /** The tab's channel changed, or a load replaced the trees. */
    void refresh()
    {
        detachListeners();
        attachListeners();
        matrix.refresh();
    }

    /** The calculation engine's feedback-cycle bitmask, one bit per effect. */
    void setCycleMask (juce::uint32 mask)
    {
        if (cycleMask == mask)
            return;

        cycleMask = mask;
        matrix.repaint();
    }

    spatcore::ui::sends::SendMatrixComponent& getMatrix() noexcept { return matrix; }

    void resized() override
    {
        auto area = getLocalBounds().reduced (scaled (6));

        auto top = area.removeFromTop (scaled (28));
        allOnButton.setBounds (top.removeFromLeft (scaled (170)));
        top.removeFromLeft (scaled (6));
        allOffButton.setBounds (top.removeFromLeft (scaled (170)));
        top.removeFromLeft (scaled (12));
        hintLabel.setBounds (top);
        area.removeFromTop (scaled (6));

        matrix.setBounds (area);
    }

private:
    bool inCycle (int column) const
    {
        return column >= 0 && column < 32 && (cycleMask & (1u << column)) != 0;
    }

    void setAllSends (bool on)
    {
        auto& vts = ctx.parameters.getValueTreeState();
        const int fx = ctx.slot();
        const int numInputs = ctx.parameters.getNumInputChannels();
        const int numEffects = ctx.parameters.getNumEffectChannels();

        ctx.beginGesture (on ? "Effect Sends All On" : "Effect Sends All Off");

        const juce::ScopedValueSetter<bool> selfWriteScope (ctx.isSelfWriting, true);
        for (int slot = 0; slot < numInputs; ++slot)
            vts.setEffectSendOnFromInput (fx, vts.getInputChannelNumber (slot), on);
        for (int src = 0; src < numEffects; ++src)
            if (src != fx)
                vts.setEffectFxSendOnFromEffect (fx, src, on);

        matrix.refresh();
    }

    //==========================================================================
    // Listening: any <Sends> node of any effect, and the input list (a channel
    // added or removed changes the row count).

    void attachListeners()
    {
        auto& vts = ctx.parameters.getValueTreeState();
        effectsTree = vts.getEffectsState();
        inputsTree = vts.getInputsState();

        if (effectsTree.isValid())
            effectsTree.addListener (this);
        if (inputsTree.isValid())
            inputsTree.addListener (this);
    }

    void detachListeners()
    {
        if (effectsTree.isValid())
            effectsTree.removeListener (this);
        if (inputsTree.isValid())
            inputsTree.removeListener (this);
    }

    void valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& property) override
    {
        if (ctx.isSelfWriting)
            return;                        // our own write is already on screen

        using namespace WFSParameterIDs;
        const bool sendsRow = property == effectSendLevels || property == effectSendOns
                           || property == effectFxSendLevels || property == effectFxSendOns;
        const bool grouping = property == effectLinkGroup || property == effectName || property == inputName;

        if (sendsRow || grouping || tree.hasType (Sends))
            scheduleRefresh();
    }

    void valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&) override        { scheduleRefresh(); }
    void valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree&, int) override { scheduleRefresh(); }
    void valueTreeChildOrderChanged (juce::ValueTree&, int, int) override        { scheduleRefresh(); }
    void valueTreeParentChanged (juce::ValueTree&) override {}

    void scheduleRefresh()
    {
        if (refreshPending)
            return;

        refreshPending = true;
        juce::MessageManager::callAsync ([this]
        {
            refreshPending = false;
            matrix.refresh();
        });
    }

    int scaled (int ref) const
    {
        const float s = static_cast<float> (getHeight()) / 700.0f;
        return juce::jmax (static_cast<int> (ref * 0.65f), static_cast<int> (ref * s));
    }

    EffectsTabContext& ctx;
    spatcore::ui::sends::SendMatrixComponent matrix;
    juce::TextButton allOnButton, allOffButton;
    juce::Label hintLabel;
    juce::ValueTree effectsTree, inputsTree;
    juce::uint32 cycleMask = 0;
    bool refreshPending = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectsSendsPanel)
};
