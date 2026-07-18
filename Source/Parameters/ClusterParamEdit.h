#pragma once

#include <JuceHeader.h>
#include <vector>

#include "WFSValueTreeState.h"
#include "WFSParameterIDs.h"
#include "../Network/OSCParameterBounds.h"
#include "../../spatcore/control/osc/OscTransportTypes.h"

/**
 * Cluster-wide parameter editing engine.
 *
 * Single entry point for USER-originated per-input parameter writes (GUI
 * controls and Streamdeck dials/buttons). Without modifiers it degrades to a
 * plain WFSValueTreeState::setInputParameter call. With modifiers held it
 * also propagates the edit to every other input of the same cluster:
 *
 *   - Shift            -> relative: members follow by the same delta
 *   - Ctrl/Cmd+Shift   -> absolute: members copy the edited value
 *   - toggles/combos   -> always absolute copy (no relative flip-flopping)
 *
 * Gestures are detected by timeout rather than explicit hooks: the first
 * modifier-write of a (channel, param) pair starts a gesture that captures
 * every member's baseline, and the gesture ends when the modifiers are
 * released or after an idle timeout. Relative targets are always computed as
 * clamp(baseline + delta), so members pinned at a bound recover their offset
 * when the edited value comes back.
 *
 * Member writes are throttled to one flush per THROTTLE_MS (~20 Hz) during
 * drags; the first write of a gesture flushes synchronously so single-shot
 * events (toggle click, combo pick, number-box commit, wheel notch) propagate
 * immediately. The edited channel itself is always written at full rate.
 *
 * Writes that do NOT come through this class (OSC, MCP, snapshots, tracking,
 * automation, LFO) never propagate — they call setInputParameter directly.
 */
class ClusterParamEdit : private juce::Timer
{
public:
    enum class Mode { None, Relative, Absolute };

    explicit ClusterParamEdit (WFSValueTreeState& s) : state (s) {}
    ~ClusterParamEdit() override { stopTimer(); }

    /** Fired once per modifier-gesture, on the message thread. */
    std::function<void (int clusterIndex, int memberCount, Mode mode)> onPropagationStarted;

    /** Write a user edit. channelIndex is 0-based. */
    void write (int channelIndex, const juce::Identifier& paramId, const juce::var& value)
    {
        // Baseline must be read before the edited channel is overwritten.
        const juce::var preValue = state.getInputParameter (channelIndex, paramId);

        const Mode mode = isExcluded (paramId) ? Mode::None : resolveMode (paramId, value);
        if (mode == Mode::None)
        {
            // Modifiers released mid-drag: finish the pending gesture cleanly.
            if (gestureActive)
                endGesture();
            state.setInputParameter (channelIndex, paramId, value);
            return;
        }

        if (gestureActive
            && (gestureChannel != channelIndex || gestureParam != paramId || gestureMode != mode))
            endGesture();

        // Gesture setup (member enumeration, undo transaction for hardware)
        // must happen BEFORE the edited channel's write so the whole gesture
        // lands in one undo step.
        const bool starting = ! gestureActive;
        if (starting && ! startGesture (channelIndex, paramId, mode, preValue))
        {
            state.setInputParameter (channelIndex, paramId, value);
            return;
        }

        state.setInputParameter (channelIndex, paramId, value);

        pendingValue = value;
        dirty = true;
        lastWriteMs = juce::Time::getMillisecondCounter();

        if (starting)
        {
            // First write flushes synchronously (inside the caller's
            // OriginTagScope) so single-shot events propagate at once.
            flushMembers();
            startTimer (TIMER_MS);
        }
    }

    /** Flush any pending member writes and end the current gesture. */
    void endGesture()
    {
        if (! gestureActive)
            return;

        if (dirty)
            flushMembers();

        stopTimer();
        gestureActive = false;
        gestureChannel = -1;
        gestureCluster = 0;
        gestureMode = Mode::None;
        members.clear();
        dirty = false;
    }

    /** Parameters that must never propagate to cluster members: position and
        offset (they have their own cluster-link logic in MapTab /
        propagateSharedClusterPosition), cluster bookkeeping, identity and
        per-channel-only settings, and transport-style actions. */
    static bool isExcluded (const juce::Identifier& paramId)
    {
        using namespace WFSParameterIDs;
        static const juce::Identifier list[] = {
            inputName,
            inputPositionX, inputPositionY, inputPositionZ,
            inputOffsetX,   inputOffsetY,   inputOffsetZ,
            inputCluster,   inputHiddenByCluster,
            inputTrackingID, inputAdmMapping,
            inputOtomoPauseResume
        };
        for (const auto& entry : list)
            if (entry == paramId)
                return true;
        return false;
    }

    /** Toggles and choice combos: relative deltas make no sense, both
        modifier combos copy the value absolutely. */
    static bool isAbsoluteOnly (const juce::Identifier& paramId)
    {
        using namespace WFSParameterIDs;
        static const juce::Identifier list[] = {
            inputMinimalLatency,
            inputConstraintX, inputConstraintY, inputConstraintZ,
            inputConstraintDistance,
            inputFlipX, inputFlipY, inputFlipZ,
            inputTrackingActive, inputMaxSpeedActive, inputPathModeActive,
            inputCoordinateMode,
            inputAttenuationLaw,
            inputLSactive, inputLSshape, inputLSpeakEnable, inputLSslowEnable,
            inputFRactive, inputFRlowCutActive, inputFRhighShelfActive,
            inputMuteReverbSends,
            inputLFOactive, inputLFOshapeX, inputLFOshapeY, inputLFOshapeZ,
            inputLFOgyrophone,
            inputOtomoAbsoluteRelative, inputOtomoStayReturn,
            inputOtomoTrigger, inputOtomoCoordinateMode,
            inputSidelinesActive,
            inputMutes, inputMuteMacro,
            inputMapLocked, inputMapVisible
        };
        for (const auto& entry : list)
            if (entry == paramId)
                return true;
        return false;
    }

private:
    static constexpr int TIMER_MS    = 25;   // tick resolution
    static constexpr int THROTTLE_MS = 50;   // min interval between member flushes (~20 Hz)
    static constexpr int IDLE_END_MS = 500;  // gesture ends after this much inactivity

    struct Member
    {
        int channel;
        double baseline;
    };

    Mode resolveMode (const juce::Identifier& paramId, const juce::var& value) const
    {
        const auto mods = juce::ModifierKeys::getCurrentModifiersRealtime();
        if (! mods.isShiftDown())
            return Mode::None;

        if (mods.isCommandDown() || mods.isCtrlDown())
            return Mode::Absolute;

        // Shift alone: relative, unless the parameter (or value type) only
        // supports copying.
        if (isAbsoluteOnly (paramId) || ! (value.isInt() || value.isInt64() || value.isDouble()))
            return Mode::Absolute;

        return Mode::Relative;
    }

    bool startGesture (int channelIndex, const juce::Identifier& paramId, Mode mode,
                       const juce::var& preValue)
    {
        const int cluster = (int) state.getInputParameter (channelIndex, WFSParameterIDs::inputCluster);
        if (cluster < 1)
            return false;

        members.clear();
        const int numChannels = state.getNumInputChannels();
        for (int i = 0; i < numChannels; ++i)
        {
            if (i == channelIndex)
                continue;
            if ((int) state.getInputParameter (i, WFSParameterIDs::inputCluster) == cluster)
                members.push_back ({ i, (double) state.getInputParameter (i, paramId) });
        }

        if (members.empty())
            return false;

        gestureActive  = true;
        gestureChannel = channelIndex;
        gestureCluster = cluster;
        gestureParam   = paramId;
        gestureMode    = mode;
        editedBaseline = (double) preValue;
        lastFlushMs    = 0;

        // The caller's OriginTagScope (UI / Hardware) is still open here;
        // remember it so throttled flushes from the timer keep the same
        // attribution.
        gestureOrigin = spatcore::control::osc::getCurrentOriginTag();

        // GUI drags already open an undo transaction in onGestureStart.
        // Streamdeck edits have none, so give the hardware gesture its own —
        // one Ctrl+Z then reverts the dial turn including all member writes.
        if (gestureOrigin == spatcore::control::osc::OriginTag::Hardware)
            state.beginUndoTransaction ("Cluster Edit: " + gestureParam.toString());

        if (onPropagationStarted != nullptr)
            onPropagationStarted (cluster, (int) members.size(), mode);

        return true;
    }

    void flushMembers()
    {
        spatcore::control::osc::OriginTagScope originScope { gestureOrigin };

        const auto bounds = WFSNetwork::getBounds (gestureParam);

        for (const auto& m : members)
        {
            juce::var target;
            if (gestureMode == Mode::Relative)
            {
                double v = m.baseline + ((double) pendingValue - editedBaseline);
                if (bounds.has_value())
                    v = juce::jlimit (bounds->min, bounds->max, v);
                target = (bounds.has_value() && bounds->isInt)
                             ? juce::var ((int) std::lround (v))
                             : juce::var (v);
            }
            else
            {
                target = pendingValue;
            }

            if (state.getInputParameter (m.channel, gestureParam) != target)
                state.setInputParameter (m.channel, gestureParam, target);
        }

        dirty = false;
        lastFlushMs = juce::Time::getMillisecondCounter();
    }

    void timerCallback() override
    {
        const auto now = juce::Time::getMillisecondCounter();

        // Mode change (modifier released / switched) ends the gesture; so
        // does idleness. endGesture() flushes anything still pending.
        if (resolveMode (gestureParam, pendingValue) != gestureMode
            || now - lastWriteMs > (juce::uint32) IDLE_END_MS)
        {
            endGesture();
            return;
        }

        if (dirty && now - lastFlushMs >= (juce::uint32) THROTTLE_MS)
            flushMembers();
    }

    WFSValueTreeState& state;

    bool gestureActive = false;
    int gestureChannel = -1;
    int gestureCluster = 0;
    juce::Identifier gestureParam;
    Mode gestureMode = Mode::None;
    double editedBaseline = 0.0;
    std::vector<Member> members;

    juce::var pendingValue;
    bool dirty = false;
    juce::uint32 lastWriteMs = 0, lastFlushMs = 0;
    spatcore::control::osc::OriginTag gestureOrigin = spatcore::control::osc::OriginTag::None;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClusterParamEdit)
};
