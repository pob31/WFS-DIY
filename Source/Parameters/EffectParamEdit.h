#pragma once

#include <JuceHeader.h>
#include "WFSValueTreeState.h"
#include "WFSParameterIDs.h"

/**
    The write funnel for GUI edits to an effects channel.

    THE MIRROR OF ArrayParamEdit, NOT OF ClusterParamEdit (R5-6). Effects link
    groups have membership (effectLinkGroup) AND a mode on every member
    (effectLinkMode), which is the output-array shape; clusters have membership
    with no per-member mode, so cloning that funnel would have inherited exactly
    the gap effectLinkMode closes. What is borrowed from the cluster file is
    nothing but its "a bypassed write is still a write" discipline.

    Without modifiers this degrades to the normal group-propagating write,
    honouring each channel's ABSOLUTE / RELATIVE / OFF mode on both sides. With
    Ctrl/Cmd held the write reaches the edited channel alone and the group is
    left entirely untouched - the link mode itself is not altered, so releasing
    the key resumes propagation on the next delta.

    Like ArrayParamEdit this needs no gesture state, no baselines and no
    throttling: a bypassed write touches a single channel, and every write
    re-polls the modifier state, so letting go of Ctrl mid-drag resumes
    propagation immediately. It opens no undo transaction of its own either -
    the widget's onGestureStart does that, exactly as every existing tab does,
    and the member writes then land in the same transaction as the source.

    Writes that do NOT come through this class - OSC, MCP, snapshot recall,
    file loads, the calculation engine - are unaffected by keyboard state and
    never propagate. That is the same rule ClusterParamEdit records, and it is
    what keeps a cue-driven show from reaching channels the cue did not name.
*/
class EffectParamEdit
{
public:
    explicit EffectParamEdit (WFSValueTreeState& s) : state (s) {}

    /** Fired on the first bypassed write of a gesture (de-duplicated per
        channel/parameter and by idle timeout), on the message thread. Only
        fires when propagation would actually have happened - the parameter
        propagates, the channel is in a group, and neither it nor the group is
        detached - so the GUI can say "this edit stayed on channel N" without
        saying it for every write that was never going to travel. */
    std::function<void (int group)> onBypassStarted;

    /** A plain per-channel parameter: anything on <Channel>, <Position>,
        <Feed>, <Return>, <Chain>, <AutomOtion> or a single-instance module
        node. channelIndex is 0-based and dense. */
    void write (int channelIndex, const juce::Identifier& paramId, const juce::var& value)
    {
        const bool bypass = bypassHeld();
        if (bypass)
            maybeNotify (channelIndex, paramId);

        state.setEffectParameterWithLinkPropagation (channelIndex, paramId, value, ! bypass);
    }

    /** A parameter on one module node, named by its node type. This is the
        only way to address FxEq1 vs FxEq2 and FxDyn1 vs FxDyn2, whose property
        names are identical - the generic path deliberately skips them rather
        than answering instance 1 and reporting success. */
    void writeModule (int channelIndex, const juce::Identifier& moduleType,
                      const juce::Identifier& paramId, const juce::var& value)
    {
        const bool bypass = bypassHeld();
        if (bypass)
            maybeNotify (channelIndex, paramId);

        state.setEffectModuleParameterWithLinkPropagation (channelIndex, moduleType, paramId,
                                                           value, ! bypass);
    }

    /** One band of one EQ instance. eqInstance and bandIndex are 0-based. */
    void writeBand (int channelIndex, int eqInstance, int bandIndex,
                    const juce::Identifier& paramId, const juce::var& value)
    {
        const bool bypass = bypassHeld();
        if (bypass)
            maybeNotify (channelIndex, paramId);

        state.setEffectEQBandParameterWithLinkPropagation (channelIndex, eqInstance, bandIndex,
                                                           paramId, value, ! bypass);
    }

    /** One delay tap. tapIndex is 0-based. */
    void writeTap (int channelIndex, int tapIndex,
                   const juce::Identifier& paramId, const juce::var& value)
    {
        const bool bypass = bypassHeld();
        if (bypass)
            maybeNotify (channelIndex, paramId);

        state.setEffectDelayTapParameterWithLinkPropagation (channelIndex, tapIndex,
                                                              paramId, value, ! bypass);
    }

private:
    static constexpr juce::uint32 NOTIFY_AGAIN_MS = 800;

    static bool bypassHeld()
    {
        // Polled per write, never latched, so releasing the key mid-drag
        // resumes propagation on the very next delta. Ctrl/Cmd is the OUTPUT
        // family's gesture (opt OUT of propagation); inputs opt IN with Shift.
        // An effects channel is grouped like an output array, so it follows
        // the output convention.
        const auto mods = juce::ModifierKeys::getCurrentModifiersRealtime();
        return mods.isCommandDown() || mods.isCtrlDown();
    }

    void maybeNotify (int channelIndex, const juce::Identifier& paramId)
    {
        if (WFSValueTreeState::isEffectLinkExcluded (paramId))
            return;                 // it was never going to travel

        const int group = state.getEffectLinkGroup (channelIndex);
        if (group == 0)
            return;                 // unlinked: nothing would have propagated anyway

        if (state.getEffectLinkMode (channelIndex) == 0)
            return;                 // already detached: ditto

        const auto now = juce::Time::getMillisecondCounter();
        if (channelIndex != lastChannel || paramId != lastParam
            || now - lastNotifyMs > NOTIFY_AGAIN_MS)
        {
            lastChannel = channelIndex;
            lastParam = paramId;

            if (onBypassStarted != nullptr)
                onBypassStarted (group);
        }

        lastNotifyMs = now;
    }

    WFSValueTreeState& state;
    juce::uint32 lastNotifyMs = 0;
    int lastChannel = -1;
    juce::Identifier lastParam;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectParamEdit)
};
