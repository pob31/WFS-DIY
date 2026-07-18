#pragma once

#include <JuceHeader.h>

#include "WFSValueTreeState.h"
#include "WFSParameterIDs.h"

/**
 * Array-bypass editing gate for output parameters.
 *
 * Single entry point for USER-originated per-output parameter writes (GUI
 * controls and Streamdeck dials/buttons). Without modifiers it degrades to
 * the normal array-propagating write, honouring each channel's
 * ABSOLUTE/RELATIVE/OFF apply mode. With Ctrl/Cmd held the write bypasses
 * array propagation entirely, so the adjustment lands on the edited channel
 * only — the apply-to-array setting itself is left untouched.
 *
 * The mirror image of ClusterParamEdit (inputs opt IN to cluster dispatch
 * with Shift; outputs opt OUT of array propagation with Ctrl/Cmd). Unlike
 * ClusterParamEdit no gesture state, baselines or throttling are needed:
 * a bypassed write touches a single channel, and each write re-polls the
 * modifier state, so releasing Ctrl mid-drag resumes propagation on the
 * next delta.
 *
 * Writes that do NOT come through this class (OSC arrayAdjust, MCP,
 * snapshots, config load) are unaffected by keyboard state.
 */
class ArrayParamEdit
{
public:
    explicit ArrayParamEdit (WFSValueTreeState& s) : state (s) {}

    /** Fired on the first bypassed write of a gesture (deduplicated per
        channel/param and by idle timeout), on the message thread. Only fires
        when propagation would actually have happened, i.e. the parameter is
        array-linked and the channel is an attached array member. */
    std::function<void (int arrayId)> onBypassStarted;

    /** Write a user edit to an output parameter. channelIndex is 0-based. */
    void write (int channelIndex, const juce::Identifier& paramId, const juce::var& value)
    {
        const bool bypass = bypassHeld();
        if (bypass)
            maybeNotify (channelIndex, paramId, WFSValueTreeState::isArrayLinkedParameter (paramId));
        state.setOutputParameterWithArrayPropagation (channelIndex, paramId, value, ! bypass);
    }

    /** Write a user edit to an output EQ band parameter. */
    void writeEQ (int channelIndex, int bandIndex, const juce::Identifier& paramId, const juce::var& value)
    {
        const bool bypass = bypassHeld();
        if (bypass)
            maybeNotify (channelIndex, paramId, WFSValueTreeState::isArrayLinkedEQParameter (paramId));
        state.setOutputEQBandParameterWithArrayPropagation (channelIndex, bandIndex, paramId, value, ! bypass);
    }

private:
    static constexpr juce::uint32 NOTIFY_AGAIN_MS = 800;

    static bool bypassHeld()
    {
        const auto mods = juce::ModifierKeys::getCurrentModifiersRealtime();
        return mods.isCommandDown() || mods.isCtrlDown();
    }

    void maybeNotify (int channelIndex, const juce::Identifier& paramId, bool arrayLinked)
    {
        if (! arrayLinked)
            return;

        using namespace WFSParameterIDs;
        const int arrayId = (int) state.getOutputParameter (channelIndex, outputArray);
        if (arrayId == 0)
            return;  // Single: nothing would have propagated anyway
        if ((int) state.getOutputParameter (channelIndex, outputApplyToArray) == 0)
            return;  // detached (OFF): ditto

        const auto now = juce::Time::getMillisecondCounter();
        if (channelIndex != lastChannel || paramId != lastParam
            || now - lastNotifyMs > NOTIFY_AGAIN_MS)
        {
            lastChannel  = channelIndex;
            lastParam    = paramId;
            if (onBypassStarted != nullptr)
                onBypassStarted (arrayId);
        }
        lastNotifyMs = now;
    }

    WFSValueTreeState& state;

    juce::uint32 lastNotifyMs = 0;
    int lastChannel = -1;
    juce::Identifier lastParam;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ArrayParamEdit)
};
