#pragma once

#include <JuceHeader.h>
#include "../../WfsParameters.h"
#include "../../Parameters/WFSParameterIDs.h"
#include "../StatusBar.h"

/**
    What every Effects panel shares with the tab that owns it.

    WHY THIS EXISTS AT ALL. ReverbTab is one 5859-line class, so its reload
    guards, its help-text map and its write helpers can all live on one `this`.
    The Effects tab is deliberately split across a header and four panels
    (`Documentation/effects-channels-plan.md` 6.10), which would otherwise mean
    five copies of the guards - and a guard that exists five times is a guard
    that is set in four of them.

    THE GUARDS ARE THE POINT. `isLoadingParameters` stops a control's own
    callback writing the tree while the panel is pushing values INTO that
    control. `isSelfWriting` stops `valueTreePropertyChanged` scheduling a
    channel reload for a write this tab just made - one reload per drag event
    floods the message queue, and a reload mid-drag fights the operator's hand.
    Both belong to the TAB, not to a panel: a write made in the Chain panel
    must suppress the reload the header would otherwise schedule.

    EVERY USER EDIT GOES THROUGH THE FUNNEL. The write helpers below call
    EffectParamEdit, never the state directly, so a link group propagates and a
    held Ctrl/Cmd detaches the gesture. They also open nothing: the widget's
    own onGestureStart calls beginGesture() first, exactly as every existing
    tab does, so a drag coalesces into one undo step across every member the
    edit reaches.
*/
struct EffectsTabContext
{
    explicit EffectsTabContext (WfsParameters& p) : parameters (p) {}

    WfsParameters& parameters;

    /** 1-based, as the selector and every wire protocol report it. */
    int currentChannel = 1;

    /** The dense 0-based index every state accessor takes. */
    int slot() const noexcept { return currentChannel - 1; }

    bool hasChannels() const { return parameters.getNumEffectChannels() > 0; }

    bool isLoadingParameters = false;
    bool isSelfWriting = false;

    StatusBar* statusBar = nullptr;

    /** Hover text and OSC addresses, keyed by the component the pointer is
        over. The tab owns the maps; each panel fills in its own rows. */
    std::map<juce::Component*, juce::String> helpTextMap;
    std::map<juce::Component*, juce::String> oscMethodMap;

    /** Open one undo transaction for the gesture that is starting. Call from
        a widget's onGestureStart, never per delta. */
    void beginGesture (const juce::String& name)
    {
        parameters.getValueTreeState().beginUndoTransaction (name);
    }

    /** A plain per-channel parameter. */
    void write (const juce::Identifier& paramId, const juce::var& value)
    {
        if (isLoadingParameters)
            return;

        const juce::ScopedValueSetter<bool> selfWriteScope (isSelfWriting, true);
        parameters.getEffectEdit().write (slot(), paramId, value);
    }

    /** A parameter on one module node, named by its node type - the only way
        to tell FxEq1 from FxEq2 and FxDyn1 from FxDyn2. */
    void writeModule (const juce::Identifier& moduleType,
                      const juce::Identifier& paramId, const juce::var& value)
    {
        if (isLoadingParameters)
            return;

        const juce::ScopedValueSetter<bool> selfWriteScope (isSelfWriting, true);
        parameters.getEffectEdit().writeModule (slot(), moduleType, paramId, value);
    }

    /** One band of one EQ instance, both 0-based. */
    void writeBand (int eqInstance, int bandIndex,
                    const juce::Identifier& paramId, const juce::var& value)
    {
        if (isLoadingParameters)
            return;

        const juce::ScopedValueSetter<bool> selfWriteScope (isSelfWriting, true);
        parameters.getEffectEdit().writeBand (slot(), eqInstance, bandIndex, paramId, value);
    }

    /** One delay tap, 0-based. */
    void writeTap (int tapIndex, const juce::Identifier& paramId, const juce::var& value)
    {
        if (isLoadingParameters)
            return;

        const juce::ScopedValueSetter<bool> selfWriteScope (isSelfWriting, true);
        parameters.getEffectEdit().writeTap (slot(), tapIndex, paramId, value);
    }

    /** A write that must NOT propagate to the link group and must not be
        mistaken for a user gesture: a macro's result, a re-layout, anything
        the operator did not aim at this one channel. */
    void writeDirect (int channelSlot, const juce::Identifier& paramId, const juce::var& value)
    {
        const juce::ScopedValueSetter<bool> selfWriteScope (isSelfWriting, true);
        parameters.getValueTreeState().setEffectParameter (channelSlot, paramId, value);
    }

    juce::var read (const juce::Identifier& paramId) const
    {
        return parameters.getValueTreeState().getEffectParameter (slot(), paramId);
    }

    float readFloat (const juce::Identifier& paramId, float fallback = 0.0f) const
    {
        const auto v = read (paramId);
        return v.isVoid() ? fallback : static_cast<float> (static_cast<double> (v));
    }

    int readInt (const juce::Identifier& paramId, int fallback = 0) const
    {
        const auto v = read (paramId);
        return v.isVoid() ? fallback : static_cast<int> (v);
    }

    void showStatusMessage (const juce::String& message) const
    {
        if (statusBar != nullptr)
            statusBar->showTemporaryMessage (message);
    }
};
