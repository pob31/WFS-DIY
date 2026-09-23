#include "WFSValueTreeState.h"
#include "../Network/OSCParameterBounds.h"
#include "../WFSLogger.h"
#include "../Sampler/SamplerData.h"
#include "VarCoercion.h"

#include <algorithm>
#include <cmath>
#include <vector>

using namespace WFSParameterIDs;

namespace
{
    /** Find the child of `parent` with `type` whose `property` equals `wanted`.
        ADM identity is carried in a property, never in child position: the
        mappings of both kinds are siblings under <ADMOSC>, and the GUI's own
        lookups match on the id for the same reason. */
    juce::ValueTree findChildByIntProperty (const juce::ValueTree& parent,
                                            const juce::Identifier& type,
                                            const juce::Identifier& property,
                                            int wanted)
    {
        for (int i = 0; i < parent.getNumChildren(); ++i)
        {
            auto child = parent.getChild (i);
            if (child.hasType (type) && static_cast<int> (child.getProperty (property, -1)) == wanted)
                return child;
        }
        return {};
    }

    /** The nth child of `parent` that has `type`, counting ONLY that type.

        ONE RULE, ONE PLACE, because this file got it wrong at three different
        depths. A container built holding a single node type is addressed by its
        callers as if the nth child WERE the nth of that type - <EQ> holds six
        <Band>s, <FxDelay> eight <Tap>s, <ReverbPostEQ> its <PostEQBand>s - and
        for everything this application writes that is true. A FILE is not this
        application: mergeTreeRecursive appends an unmatched source child
        verbatim, so one unrecognised node anywhere in the list shifts every
        sibling above it by one, and a straight getChild (n) then answers with
        the wrong node - or with nothing, for the last band, which has been
        pushed off the end.

        That is not a lookup miss, it is a SILENT WRITE TO NOWHERE. The caller
        gets back a valid tree, setProperty succeeds, canWriteParameter says yes
        and the remote surface reports success - while the value lands on a node
        no reader ever descends into. Worse, it ROUND-TRIPS: the property is
        saved onto that node and read back off it, so a check that writes and
        reads through the same accessor passes, and the operator only ever sees
        an EQ band or a delay tap that does not do anything.

        The unknown child is left exactly where it is. An unrecognised node is
        evidence of nothing, and deleting it on load would be a second silent
        data loss rather than a fix for the first - the rule getEffectState and
        stripObsoleteEffectProperties already follow at their own granularities.

        Returns an invalid tree when there is no nth child of that type, which is
        what every caller already handles for an out-of-range index. */
    juce::ValueTree nthChildOfType (const juce::ValueTree& parent,
                                    const juce::Identifier& type,
                                    int n)
    {
        if (n < 0)
            return {};

        int seen = 0;
        for (int i = 0; i < parent.getNumChildren(); ++i)
        {
            auto child = parent.getChild (i);
            if (! child.hasType (type))
                continue;
            if (seen == n)
                return child;
            ++seen;
        }
        return {};
    }

    /** Is this token a number a packed LEVEL row may keep exactly as written?

        A level normaliser that re-spells every token would rewrite an operator's
        whole row on any touch and make a file diff where no value moved, so a
        token that is already a well-formed number in range is kept verbatim and
        only the rest are rewritten. That is only safe if "well-formed" is tested
        rather than assumed: getFloatValue() answers 0 for "abc", and 0 dB is a
        legal level, so trusting the parse alone would preserve junk as unity
        gain.

        The GRAMMAR IS WALKED rather than sampled, because "at least one digit and
        nothing outside the characters a number uses" is not the same test and
        misses this function's own target: "--5" and "-6.5.3" both pass it, both
        answer 0 from getFloatValue(), and 0 dB is UNITY - precisely the outcome
        the paragraph above says this exists to prevent. The overflow form
        "1e400" passes it too and parses to infinity, which the level clamp then
        pins to the MAXIMUM (unity again) rather than to the row default. So: an
        optional sign, digits with at most one point, an optional exponent,
        nothing after it, and a value that is actually finite.

        Spelled out here rather than handed to strtod, whose decimal point
        follows the C locale while juce::String::getFloatValue never does - this
        app runs in French on the dev box, and a locale-sensitive test would
        re-default every level of every row the first time it ran there. */
    bool isNumericToken (const juce::String& token)
    {
        auto p = token.getCharPointer();

        auto readDigits = [&p]
        {
            int n = 0;
            while (*p >= '0' && *p <= '9') { ++p; ++n; }
            return n;
        };

        if (*p == '+' || *p == '-')
            ++p;

        int digits = readDigits();
        if (*p == '.')
        {
            ++p;
            digits += readDigits();
        }
        if (digits == 0)
            return false;

        if (*p == 'e' || *p == 'E')
        {
            ++p;
            if (*p == '+' || *p == '-')
                ++p;
            if (readDigits() == 0)
                return false;
        }

        return p.isEmpty() && std::isfinite (token.getFloatValue());
    }

    /** A write refused because it was not a row, said out loud.

        A guard that silently keeps the old value turns a DESTRUCTIVE write into
        one that appears to succeed and does nothing, which is the shape of bug
        the cell setters refuse to be (they return false rather than report a
        write they cannot make). The interceptor cannot return false - it returns
        the value to store - so the one place that can see the refusal names it.

        Rare by construction: every writer inside this app sends a full row, and
        the OSC per-output form is parsed into setInputOutputMute long before it
        gets here. What reaches this line is an MCP enum, a mistyped OSC string
        or a cue carrying a scalar - which is precisely what a reader trying to
        work out why a mute did nothing needs to be told. */
    void logRefusedRowWrite (const juce::Identifier& property, const juce::var& proposed)
    {
        const juce::String text = proposed.toString();
        WFSLogger::getInstance().logWarning (
            "Refused a write to " + property.toString() + " that is not a row: \""
            + (text.length() > 64 ? text.substring (0, 64) + "..." : text)
            + "\" - the stored row was kept. A row is one value per column, comma-separated.");
    }

    /** The <Input> child a property belongs in when no child carries it yet. A
        channel the merge could not match by number is appended whole from the
        file, with no backfill, so one from an inputs.xml older than a property has
        none of it, and a write that only searches the children finds no section
        and is dropped. Only properties younger than existing files need an entry;
        everything older is on every node. */
    juce::Identifier sectionForMissingInputProperty (const juce::Identifier& paramId)
    {
        if (paramId == inputCoordinateMode)
            return Position;

        static const juce::Identifier mutesIds[] = {
            inputMuteReverbSends,
            inputArrayAtten1, inputArrayAtten2, inputArrayAtten3, inputArrayAtten4, inputArrayAtten5,
            inputArrayAtten6, inputArrayAtten7, inputArrayAtten8, inputArrayAtten9, inputArrayAtten10
        };
        for (const auto& mutesId : mutesIds)
            if (paramId == mutesId)
                return Mutes;

        return {};
    }
}

using namespace WFSParameterDefaults;

//==============================================================================
// Construction / Destruction
//==============================================================================

WFSValueTreeState::WFSValueTreeState()
    : TreeParameterStore (static_cast<int> (UndoDomain::COUNT),
                          { "Input", "Output", "Reverb", "Map", "Config", "Clusters", "Effects" })
{
    // WRITE-INTERCEPTOR (control Q6a): numeric-bounds hardening at the store
    // choke point, using the same bounds table OSC ingress and the MCP
    // escape-hatch validate against (both of those REJECT out-of-range before
    // the store; this clamp only ever fires for paths that used to bypass
    // validation). In-range numeric writes and all non-numeric writes return
    // the proposed var UNTOUCHED — same object, same type — so every
    // already-validated caller produces byte-identical results. The exceptions
    // are the SEVEN packed CSV rows, handled first below: inputMutes,
    // reverbMutes, effectMutes and the four <Sends> rows. Each is a whole row of
    // columns in one string property, none of them has a bounds entry (they are
    // not numbers), and so the generic clause below passes a bare number through
    // untouched and the row becomes that number. That is how input mutes were
    // lost to a QLab cue, an OSC scalar and an MCP enum; the clause that closed
    // it for inputMutes is the model for the other six.
    //
    // WHAT IS TESTED IS THE SHAPE OF THE WRITE, NOT ITS TYPE. A guard that only
    // asked "is this a string?" would hand every string to the normaliser, and
    // one junk token normalises into a full row of defaults - a quieter loss
    // than the bare number that started this, not a smaller one, because a row
    // of zeros is well-formed and reads as a deliberate unmute-all. The MCP
    // surface advertises reverb_set_mutes with its value as the string enum
    // "unmute" / "MUTE" (Source/Network/MCP/generated_tools.json) and the OSC
    // list form takes any non-numeric string, so that write is reachable today.
    setWriteInterceptor ([this] (const juce::Identifier& property, const juce::var& proposed,
                                 const juce::ValueTree& node) -> juce::var
    {
        // THE PER-OUTPUT MUTE ROW OF ALL THREE FAMILIES. One string holding
        // the whole row, and a bare number is never a row: it is what a QLab
        // cue, an OSC scalar or an MCP enum used to write over inputMutes, which
        // unmuted every output but the first and was then saved like that. Keep
        // the row as it is instead, and fit a real one to the live outputs.
        //
        // reverbMutes and effectMutes join it here, and reverbMutes was
        // destructible by that exact route until this clause: /wfs/reverb/mutes
        // <id> <out> <v> parses the SECOND argument as the value, reverbMutes has
        // no bounds entry so valueWithinBounds waves it through, and the row
        // became "2". All three share ONE rule because a column really is the
        // same thing in all three - an output index - so the row follows the live
        // output count. That is precisely what separates them from the four send
        // rows below, whose columns are input numbers and effect indexes.
        if (property == inputMutes || property == reverbMutes || property == effectMutes)
        {
            if (isPackedRowWrite (proposed, getNumOutputChannels()))
            {
                // MERGED onto the stored row, never padded over it, and fitted to
                // a width that can only grow. A writer speaks for the columns it
                // names: a tablet showing 16 outputs of a 32-output rig writes 16
                // tokens, and a snapshot recalled before the output count caught
                // up writes more than the rig has - the first must not clear the
                // top half, the second must not be cut. Same rule as the refit in
                // setNumOutputChannels, and for the same reason.
                const juce::String row = mergePackedRow (node.getProperty (property), proposed);
                return juce::var (normaliseMuteList (row, perOutputRowWidth (row, getNumOutputChannels())));
            }

            logRefusedRowWrite (property, proposed);
            return node.hasProperty (property)
                       ? node.getProperty (property)
                       : juce::var (normaliseMuteList ({}, getNumOutputChannels()));
        }

        // THE FOUR SEND ROWS. Same protection, DIFFERENT SHAPE, and the
        // difference is the whole reason canonicalEffectSendRow exists rather
        // than another normaliseMuteList call:
        //
        //   - their width is FIXED (maxInputChannels / maxEffectChannels) and
        //     has nothing to do with the live output count. A column is an input
        //     PERMANENT NUMBER, which can be anything up to the maximum whatever
        //     the live count is, so fitting one of these to the outputs would
        //     drop the columns of channels that still exist;
        //   - two of them hold dB, not flags. normaliseMuteList coerces every
        //     token to 0 or 1, which on a level row silently sets every send to
        //     unity or to nothing;
        //   - the fx rows have a diagonal that must stay off, and this is one of
        //     the places it is forced: a whole-row write is exactly how a self-
        //     send would otherwise arrive.
        if (property == effectSendLevels || property == effectSendOns
         || property == effectFxSendLevels || property == effectFxSendOns)
        {
            const int selfIndex = denseEffectIndexOfNode (node);
            const int columns   = (property == effectSendLevels || property == effectSendOns)
                                      ? maxInputChannels : maxEffectChannels;

            // The same shape test as the mute rows above. None of these four is
            // on the MCP surface yet, so none can be reached by the string enum
            // that reaches reverbMutes - but they inherit that hole the day the
            // grid tools land, and a one-token string wiped a routed row here
            // exactly as it did there. The width is FIXED, so that is what the
            // shape is judged against rather than any live count.
            if (isPackedRowWrite (proposed, columns))
                return juce::var (canonicalEffectSendRow (property,
                                                          mergePackedRow (node.getProperty (property), proposed),
                                                          selfIndex));

            logRefusedRowWrite (property, proposed);
            return node.hasProperty (property)
                       ? node.getProperty (property)
                       : juce::var (canonicalEffectSendRow (property, {}, selfIndex));
        }

        if (proposed.isDouble() || proposed.isInt() || proposed.isInt64())
        {
            // LFO phases are circular: wrap into the canonical [-180, 180]
            // instead of clamping, so legacy 0..360 values (accepted by the
            // gates' compat window) land on the equivalent angle.
            if (WFSNetwork::isLFOPhaseParam (property))
            {
                const double d = static_cast<double> (proposed);
                if (d < -180.0 || d > 180.0)
                    return juce::var (WFSParameterDefaults::wrapPhaseDegrees (juce::roundToInt (d)));
                return proposed;
            }

            if (const auto bounds = WFSNetwork::getBounds (property))
            {
                const double d = static_cast<double> (proposed);
                if (d < bounds->min || d > bounds->max)
                {
                    const double clamped = juce::jlimit (bounds->min, bounds->max, d);
                    return bounds->isInt ? juce::var (juce::roundToInt (clamped))
                                         : juce::var (clamped);
                }
            }
        }
        return proposed;
    });

    initializeDefaultState();
    state.addListener (this);
}

WFSValueTreeState::~WFSValueTreeState()
{
    state.removeListener (this);
}

//==============================================================================
// State Access
//==============================================================================

juce::ValueTree WFSValueTreeState::getConfigState()
{
    return state.getChildWithName (Config);
}

juce::ValueTree WFSValueTreeState::getConfigState() const
{
    return state.getChildWithName (Config);
}

juce::ValueTree WFSValueTreeState::getShowState()
{
    return getConfigState().getChildWithName (Show);
}

juce::ValueTree WFSValueTreeState::getIOState()
{
    return getConfigState().getChildWithName (IO);
}

juce::ValueTree WFSValueTreeState::getStageState()
{
    return getConfigState().getChildWithName (Stage);
}

juce::ValueTree WFSValueTreeState::getMasterState()
{
    return getConfigState().getChildWithName (Master);
}

juce::ValueTree WFSValueTreeState::getNetworkState()
{
    return getConfigState().getChildWithName (Network);
}

juce::ValueTree WFSValueTreeState::getUIState()
{
    return getConfigState().getChildWithName (UI);
}

juce::ValueTree WFSValueTreeState::getADMOSCState()
{
    return getConfigState().getChildWithName (ADMOSC);
}

juce::ValueTree WFSValueTreeState::getTrackingState()
{
    return getConfigState().getChildWithName (Tracking);
}

juce::ValueTree WFSValueTreeState::getClustersState()
{
    return getConfigState().getChildWithName (Clusters);
}

juce::ValueTree WFSValueTreeState::getClustersState() const
{
    return getConfigState().getChildWithName (Clusters);
}

juce::ValueTree WFSValueTreeState::getBinauralState()
{
    return getConfigState().getChildWithName (Binaural);
}

juce::ValueTree WFSValueTreeState::getBinauralState() const
{
    return getConfigState().getChildWithName (Binaural);
}

juce::ValueTree WFSValueTreeState::getClusterState (int clusterIndex)
{
    auto clusters = getClustersState();
    // clusterIndex is 1-based (1-10), convert to 0-based for array access
    int idx = clusterIndex - 1;
    if (idx >= 0 && idx < clusters.getNumChildren())
        return clusters.getChild (idx);
    return {};
}

juce::ValueTree WFSValueTreeState::getClusterLFOSection (int clusterIndex)
{
    auto cluster = getClusterState (clusterIndex);
    if (! cluster.isValid())
        return {};

    auto lfoSection = cluster.getChildWithName (ClusterLFO);
    if (! lfoSection.isValid())
    {
        // Migration: create with defaults if missing (old file loaded)
        lfoSection = juce::ValueTree (ClusterLFO);
        lfoSection.setProperty (clusterLFOactive,         clusterLFOactiveDefault,          nullptr);
        lfoSection.setProperty (clusterLFOperiod,         clusterLFOperiodDefault,          nullptr);
        lfoSection.setProperty (clusterLFOphase,          clusterLFOphaseDefault,           nullptr);
        lfoSection.setProperty (clusterLFOshapeX,         clusterLFOshapeDefault,           nullptr);
        lfoSection.setProperty (clusterLFOshapeY,         clusterLFOshapeDefault,           nullptr);
        lfoSection.setProperty (clusterLFOshapeZ,         clusterLFOshapeDefault,           nullptr);
        lfoSection.setProperty (clusterLFOshapeRot,       clusterLFOshapeDefault,           nullptr);
        lfoSection.setProperty (clusterLFOshapeScale,     clusterLFOshapeDefault,           nullptr);
        lfoSection.setProperty (clusterLFOrateX,          clusterLFOrateDefault,            nullptr);
        lfoSection.setProperty (clusterLFOrateY,          clusterLFOrateDefault,            nullptr);
        lfoSection.setProperty (clusterLFOrateZ,          clusterLFOrateDefault,            nullptr);
        lfoSection.setProperty (clusterLFOrateRot,        clusterLFOrateDefault,            nullptr);
        lfoSection.setProperty (clusterLFOrateScale,      clusterLFOrateDefault,            nullptr);
        lfoSection.setProperty (clusterLFOamplitudeX,     clusterLFOamplitudeXYZDefault,    nullptr);
        lfoSection.setProperty (clusterLFOamplitudeY,     clusterLFOamplitudeXYZDefault,    nullptr);
        lfoSection.setProperty (clusterLFOamplitudeZ,     clusterLFOamplitudeXYZDefault,    nullptr);
        lfoSection.setProperty (clusterLFOamplitudeRot,   clusterLFOamplitudeRotDefault,    nullptr);
        lfoSection.setProperty (clusterLFOamplitudeScale, clusterLFOamplitudeScaleDefault,  nullptr);
        lfoSection.setProperty (clusterLFOphaseX,         clusterLFOphaseDefault,           nullptr);
        lfoSection.setProperty (clusterLFOphaseY,         clusterLFOphaseDefault,           nullptr);
        lfoSection.setProperty (clusterLFOphaseZ,         clusterLFOphaseDefault,           nullptr);
        lfoSection.setProperty (clusterLFOphaseRot,       clusterLFOphaseDefault,           nullptr);
        lfoSection.setProperty (clusterLFOphaseScale,     clusterLFOphaseDefault,           nullptr);
        cluster.appendChild (lfoSection, nullptr);
    }
    return lfoSection;
}

juce::ValueTree WFSValueTreeState::getClusterLFOPresetsSection()
{
    auto config = getConfigState();
    auto presets = config.getChildWithName (ClusterLFOPresets);
    if (! presets.isValid())
    {
        // Migration: create with 16 empty preset slots
        presets = juce::ValueTree (ClusterLFOPresets);
        for (int i = 0; i < maxClusterLFOPresets; ++i)
        {
            juce::ValueTree preset (ClusterLFOPreset);
            preset.setProperty (clusterLFOPresetName,      juce::String(),                  nullptr);
            preset.setProperty (clusterLFOperiod,          clusterLFOperiodDefault,          nullptr);
            preset.setProperty (clusterLFOphase,           clusterLFOphaseDefault,           nullptr);
            preset.setProperty (clusterLFOshapeX,          clusterLFOshapeDefault,           nullptr);
            preset.setProperty (clusterLFOshapeY,          clusterLFOshapeDefault,           nullptr);
            preset.setProperty (clusterLFOshapeZ,          clusterLFOshapeDefault,           nullptr);
            preset.setProperty (clusterLFOshapeRot,        clusterLFOshapeDefault,           nullptr);
            preset.setProperty (clusterLFOshapeScale,      clusterLFOshapeDefault,           nullptr);
            preset.setProperty (clusterLFOrateX,           clusterLFOrateDefault,            nullptr);
            preset.setProperty (clusterLFOrateY,           clusterLFOrateDefault,            nullptr);
            preset.setProperty (clusterLFOrateZ,           clusterLFOrateDefault,            nullptr);
            preset.setProperty (clusterLFOrateRot,         clusterLFOrateDefault,            nullptr);
            preset.setProperty (clusterLFOrateScale,       clusterLFOrateDefault,            nullptr);
            preset.setProperty (clusterLFOamplitudeX,      clusterLFOamplitudeXYZDefault,    nullptr);
            preset.setProperty (clusterLFOamplitudeY,      clusterLFOamplitudeXYZDefault,    nullptr);
            preset.setProperty (clusterLFOamplitudeZ,      clusterLFOamplitudeXYZDefault,    nullptr);
            preset.setProperty (clusterLFOamplitudeRot,    clusterLFOamplitudeRotDefault,    nullptr);
            preset.setProperty (clusterLFOamplitudeScale,  clusterLFOamplitudeScaleDefault,  nullptr);
            preset.setProperty (clusterLFOphaseX,          clusterLFOphaseDefault,           nullptr);
            preset.setProperty (clusterLFOphaseY,          clusterLFOphaseDefault,           nullptr);
            preset.setProperty (clusterLFOphaseZ,          clusterLFOphaseDefault,           nullptr);
            preset.setProperty (clusterLFOphaseRot,        clusterLFOphaseDefault,           nullptr);
            preset.setProperty (clusterLFOphaseScale,      clusterLFOphaseDefault,           nullptr);
            presets.appendChild (preset, nullptr);
        }
        config.appendChild (presets, nullptr);
    }
    return presets;
}

juce::ValueTree WFSValueTreeState::ensureClusterLFOPreset (int presetIndex)
{
    auto presets = getClusterLFOPresetsSection();
    if (presetIndex >= 0 && presetIndex < presets.getNumChildren())
        return presets.getChild (presetIndex);
    return {};
}

void WFSValueTreeState::recallClusterLFOPreset (int clusterId, int presetIndex)
{
    auto preset = ensureClusterLFOPreset (presetIndex);
    if (! preset.isValid()) return;

    auto lfoSection = getClusterLFOSection (clusterId);
    if (! lfoSection.isValid()) return;

    // The 22 preset properties (same order as ClustersTab::getLFOPresetPropId)
    static const juce::Identifier ids[] = {
        clusterLFOperiod, clusterLFOphase,
        clusterLFOshapeX, clusterLFOshapeY, clusterLFOshapeZ, clusterLFOshapeRot, clusterLFOshapeScale,
        clusterLFOrateX, clusterLFOrateY, clusterLFOrateZ, clusterLFOrateRot, clusterLFOrateScale,
        clusterLFOamplitudeX, clusterLFOamplitudeY, clusterLFOamplitudeZ, clusterLFOamplitudeRot, clusterLFOamplitudeScale,
        clusterLFOphaseX, clusterLFOphaseY, clusterLFOphaseZ, clusterLFOphaseRot, clusterLFOphaseScale
    };

    // Apply non-shape properties first (amplitude, rate, phase, period)
    for (const auto& propId : ids)
    {
        if (propId == clusterLFOshapeX || propId == clusterLFOshapeY || propId == clusterLFOshapeZ ||
            propId == clusterLFOshapeRot || propId == clusterLFOshapeScale)
            continue;
        if (preset.hasProperty (propId))
            lfoSection.setProperty (propId, preset.getProperty (propId), nullptr);
    }

    // Then apply shapes (triggers fade dip if shape changed)
    lfoSection.setProperty (clusterLFOshapeX,     preset.getProperty (clusterLFOshapeX), nullptr);
    lfoSection.setProperty (clusterLFOshapeY,     preset.getProperty (clusterLFOshapeY), nullptr);
    lfoSection.setProperty (clusterLFOshapeZ,     preset.getProperty (clusterLFOshapeZ), nullptr);
    lfoSection.setProperty (clusterLFOshapeRot,   preset.getProperty (clusterLFOshapeRot), nullptr);
    lfoSection.setProperty (clusterLFOshapeScale, preset.getProperty (clusterLFOshapeScale), nullptr);
}

juce::ValueTree WFSValueTreeState::getInputsState()
{
    return state.getChildWithName (Inputs);
}

juce::ValueTree WFSValueTreeState::getInputsState() const
{
    return state.getChildWithName (Inputs);
}

juce::ValueTree WFSValueTreeState::getInputState (int channelIndex)
{
    auto inputs = getInputsState();
    if (channelIndex >= 0 && channelIndex < inputs.getNumChildren())
        return inputs.getChild (channelIndex);
    return {};
}

juce::ValueTree WFSValueTreeState::getOutputsState()
{
    return state.getChildWithName (Outputs);
}

juce::ValueTree WFSValueTreeState::getOutputsState() const
{
    return state.getChildWithName (Outputs);
}

juce::ValueTree WFSValueTreeState::getOutputState (int channelIndex)
{
    auto outputs = getOutputsState();
    if (channelIndex >= 0 && channelIndex < outputs.getNumChildren())
        return outputs.getChild (channelIndex);
    return {};
}

juce::ValueTree WFSValueTreeState::getReverbsState()
{
    return state.getChildWithName (Reverbs);
}

juce::ValueTree WFSValueTreeState::getReverbsState() const
{
    return state.getChildWithName (Reverbs);
}

juce::ValueTree WFSValueTreeState::getReverbState (int channelIndex)
{
    auto reverbs = getReverbsState();
    int reverbCount = 0;
    for (int i = 0; i < reverbs.getNumChildren(); ++i)
    {
        auto child = reverbs.getChild (i);
        if (child.hasType (Reverb))
        {
            if (reverbCount == channelIndex)
                return child;
            ++reverbCount;
        }
    }
    return {};
}

juce::ValueTree WFSValueTreeState::getEffectsState()
{
    return state.getChildWithName (Effects);
}

juce::ValueTree WFSValueTreeState::getEffectsState() const
{
    return state.getChildWithName (Effects);
}

juce::ValueTree WFSValueTreeState::getEffectsGlobalSection() const
{
    return getConfigState().getChildWithName (EffectsGlobal);
}

juce::ValueTree WFSValueTreeState::getEffectState (int channelIndex)
{
    // THE SAME COUNT-BY-TYPE WALK getReverbState does, and for the same reason
    // - not because <Effects> is meant to hold anything but <Effect> children,
    // but because this must AGREE WITH getNumEffectChannels whatever the file
    // turned out to hold.
    //
    // This used to index straight into the child list and treat the type test as
    // a guard on the container's invariant. The invariant is real and nothing in
    // this app breaks it; a FILE can. mergeTreeRecursive appends any unmatched
    // source child verbatim, and WFSFileManager::applyEffectsSection is exactly
    // the path a hand-edited or foreign effects.xml takes, so <Effects> holding
    // <Effect id="1"/>, <Foo/>, <Effect id="2"/> was reachable. The count walks
    // by type and said two; this indexed and returned an invalid tree for
    // channel 2, and every accessor built on it - Channel, Position, Feed,
    // Return, AutomOtion, Chain, Sends, the eleven modules - plus
    // redistributeAllEffectPositions went with it. setNumEffectChannels then
    // wrote that same two into <Effects count> and Config/IO/effectChannels, so
    // a channel the whole application agreed existed could not be addressed.
    //
    // The unknown child is left where it is rather than deleted on load: an
    // unrecognised node is evidence of nothing (see stripObsoleteEffectProperties
    // for the same rule at property granularity), and dropping it would be the
    // second silent data loss on this path, not a fix for the first.
    auto effects = getEffectsState();
    if (channelIndex < 0)
        return {};

    int effectCount = 0;
    for (int i = 0; i < effects.getNumChildren(); ++i)
    {
        auto child = effects.getChild (i);
        if (child.hasType (Effect))
        {
            if (effectCount == channelIndex)
                return child;
            ++effectCount;
        }
    }
    return {};
}

juce::ValueTree WFSValueTreeState::getAudioPatchState()
{
    return state.getChildWithName (AudioPatch);
}

//==============================================================================
// Parameter Access - Type Safe
//==============================================================================
// Typed getters live in TreeParameterStore (resolved through the
// getTreeForParameter override below). The setters stay here because the
// channel-count writes need schema-structural routing before the generic
// core write.

void WFSValueTreeState::setParameter (const juce::Identifier& paramId, const juce::var& value, int channelIndex)
{
    // Special case: writes to channel-count parameters must go through the
    // setNumXChannels helpers so the channel subtrees actually grow/shrink
    // alongside the count property. A bare setProperty here updates the
    // property but leaves the array stale — the GUI editor calls
    // setNumXChannels directly, but OSC ingress / MCP set_parameter / file
    // load round-trips would silently land here and desync. Cast to int via
    // juce::var so a numeric-string ("12") still routes correctly.
    if (paramId == inputChannels)
    {
        setNumInputChannels (static_cast<int> (value));
        return;
    }
    if (paramId == outputChannels)
    {
        setNumOutputChannels (static_cast<int> (value));
        return;
    }
    if (paramId == reverbChannels)
    {
        setNumReverbChannels (static_cast<int> (value));
        return;
    }
    if (paramId == effectChannels)
    {
        setNumEffectChannels (static_cast<int> (value));
        return;
    }
    if (paramId == stereoInputChannels)
    {
        // Obsolete under the stable-number model: a channel's type lives on
        // the channel (inputChannelType) and changes through
        // setInputChannelType. Ignore so a stale file value or remote write
        // cannot resurrect the count-based tail semantics.
        juce::Logger::writeToLog ("Ignoring write to obsolete stereoInputChannels parameter");
        return;
    }

    TreeParameterStore::setParameter (paramId, value, channelIndex);
}

bool WFSValueTreeState::isProcessingEnabled() const
{
    // Read through the tree rather than getIOState(), which is non-const.
    // Absent means a tree that has not been built yet, which is not "running".
    const auto io = state.getChildWithName (Config).getChildWithName (IO);
    return io.isValid() && static_cast<bool> (io.getProperty (runDSP, false));
}

bool WFSValueTreeState::canWriteParameter (const juce::Identifier& paramId, int channelIndex) const
{
    // The three live channel counts never reach getTreeForParameter: setParameter
    // re-routes them to the setNumXChannels helpers above. They resolve to nothing
    // and are writable anyway, so answer for the routing, not for the tree.
    if (paramId == inputChannels || paramId == outputChannels || paramId == reverbChannels
        || paramId == effectChannels)
        return true;

    // Deliberately dropped — see the comment in setParameter. Saying so here is the
    // whole point: a caller that asks gets told, instead of being handed a success.
    if (paramId == stereoInputChannels)
        return false;

    return getTreeForParameter (paramId, channelIndex).isValid();
}

void WFSValueTreeState::setParameterWithoutUndo (const juce::Identifier& paramId, const juce::var& value, int channelIndex)
{
    // Same channel-count routing as setParameter — but setNumXChannels
    // always uses getActiveUndoManager(). When the caller asked for "no
    // undo", the count subtree resize still needs to happen; we just
    // accept the slightly redundant undo bookkeeping for these
    // structural writes (the alternative is duplicating ~80 lines of
    // setNumXChannels with `nullptr` undo, which isn't worth it).
    if (paramId == inputChannels)  { setNumInputChannels  (static_cast<int> (value)); return; }
    if (paramId == outputChannels) { setNumOutputChannels (static_cast<int> (value)); return; }
    if (paramId == reverbChannels) { setNumReverbChannels (static_cast<int> (value)); return; }
    if (paramId == effectChannels) { setNumEffectChannels (static_cast<int> (value)); return; }
    if (paramId == stereoInputChannels) { return; }  // obsolete — see setParameter

    TreeParameterStore::setParameterWithoutUndo (paramId, value, channelIndex);
}

//==============================================================================
// Input Channel Access
//==============================================================================

juce::var WFSValueTreeState::getInputParameter (int channelIndex, const juce::Identifier& paramId) const
{
    auto input = const_cast<WFSValueTreeState*>(this)->getInputState (channelIndex);
    if (!input.isValid())
        return {};

    // Search through all subsections
    for (int i = 0; i < input.getNumChildren(); ++i)
    {
        auto child = input.getChild (i);
        if (child.hasProperty (paramId))
            return child.getProperty (paramId);
    }
    return {};
}

void WFSValueTreeState::setInputParameter (int channelIndex, const juce::Identifier& paramId, const juce::var& value)
{
    auto input = getInputState (channelIndex);
    if (!input.isValid())
        return;

    // Search through all subsections
    for (int i = 0; i < input.getNumChildren(); ++i)
    {
        auto child = input.getChild (i);
        if (child.hasProperty (paramId))
        {
            writeProperty (child, paramId, value, getActiveUndoManager());

            // Maintain the Shared-Position cluster invariant: any write to
            // inputPositionX/Y/Z on a shared-mode cluster member propagates
            // to every other member. No-op for inputs not in Shared mode.
            if (paramId == inputPositionX || paramId == inputPositionY || paramId == inputPositionZ)
            {
                propagateSharedClusterPosition (channelIndex);

                // A position write through the user-facing API (UI tabs, Map
                // tab, OSC, MCP) means the user is positioning things: latch
                // ownership so auto-placement never fights them. The engine's
                // own layout helpers write via setProperty and bypass this.
                markPositionsUserOwned();
            }

            return;
        }
    }

    // Property not found - add it to the appropriate section if we know where it belongs
    // This handles old config files that may be missing newer properties
    if (const auto sectionId = sectionForMissingInputProperty (paramId); sectionId.isValid())
    {
        auto section = input.getChildWithName (sectionId);
        if (section.isValid())
            writeProperty (section, paramId, value, getActiveUndoManager());
    }
    // Note: inputAttenuation always exists in the Channel section (created by
    // createInputChannelSection), so the search loop above always finds it - no
    // fallback needed. The calc engine reads it from the Channel section too.
}

juce::ValueTree WFSValueTreeState::getInputChannelSection (int channelIndex)
{
    return getInputState (channelIndex).getChildWithName (Channel);
}

juce::ValueTree WFSValueTreeState::getInputPositionSection (int channelIndex)
{
    return getInputState (channelIndex).getChildWithName (Position);
}

juce::ValueTree WFSValueTreeState::getInputAttenuationSection (int channelIndex)
{
    return getInputState (channelIndex).getChildWithName (Attenuation);
}

juce::ValueTree WFSValueTreeState::getInputDirectivitySection (int channelIndex)
{
    return getInputState (channelIndex).getChildWithName (Directivity);
}

juce::ValueTree WFSValueTreeState::getInputLiveSourceSection (int channelIndex)
{
    return getInputState (channelIndex).getChildWithName (LiveSourceTamer);
}

juce::ValueTree WFSValueTreeState::getInputHackousticsSection (int channelIndex)
{
    return getInputState (channelIndex).getChildWithName (Hackoustics);
}

juce::ValueTree WFSValueTreeState::getInputLFOSection (int channelIndex)
{
    return getInputState (channelIndex).getChildWithName (LFO);
}

juce::ValueTree WFSValueTreeState::getInputAutoMotionSection (int channelIndex)
{
    return getInputState (channelIndex).getChildWithName (AutomOtion);
}

juce::ValueTree WFSValueTreeState::getInputMutesSection (int channelIndex)
{
    return getInputState (channelIndex).getChildWithName (Mutes);
}

juce::String WFSValueTreeState::normaliseMuteList (const juce::var& list, int numOutputs, int keepTokens)
{
    juce::StringArray tokens;
    tokens.addTokens (list.toString(), ",", "");

    for (int i = 0; i < tokens.size(); ++i)
        tokens.set (i, (i < keepTokens && tokens[i].trim().getIntValue() != 0) ? "1" : "0");

    if (numOutputs > 0)
    {
        while (tokens.size() < numOutputs)
            tokens.add ("0");
        tokens.removeRange (numOutputs, tokens.size() - numOutputs);
    }

    return tokens.joinIntoString (",");
}

int WFSValueTreeState::perOutputRowWidth (const juce::var& row, int numOutputs)
{
    juce::StringArray tokens;
    tokens.addTokens (row.toString(), ",", "");
    return juce::jmax (numOutputs, tokens.size());
}

bool WFSValueTreeState::isPackedRowWrite (const juce::var& proposed, int expectedColumns)
{
    if (! proposed.isString())
        return false;   // a bare number is a scalar, and always was

    juce::StringArray tokens;
    tokens.addTokens (proposed.toString(), ",", "");

    if (tokens.isEmpty())
        return false;   // "" names no column at all

    // ONE TOKEN IS A SCALAR whatever it spells - unless the row really does have
    // one column, which is why the width is asked for rather than assumed.
    if (tokens.size() == 1 && expectedColumns != 1)
        return false;

    // A ROW IS NUMBERS. A string with a token that is not one is refused WHOLE
    // rather than repaired token by token: the normalisers repair what a FILE
    // carries, because a file has no writer left to refuse, but a live write
    // that cannot spell its own row is not a row that lost a column.
    for (const auto& token : tokens)
        if (! isNumericToken (token.trim()))
            return false;

    return true;
}

juce::String WFSValueTreeState::mergePackedRow (const juce::var& existing, const juce::var& proposed)
{
    juce::StringArray row;
    row.addTokens (existing.toString(), ",", "");

    juce::StringArray tokens;
    tokens.addTokens (proposed.toString(), ",", "");

    for (int i = 0; i < tokens.size(); ++i)
    {
        if (i < row.size())
            row.set (i, tokens[i]);
        else
            row.add (tokens[i]);
    }

    return row.joinIntoString (",");
}

bool WFSValueTreeState::setInputOutputMute (int channelIndex, int outputIndex, bool muted)
{
    const int numOutputs = getNumOutputChannels();
    if (! getInputMutesSection (channelIndex).isValid() || outputIndex < 0 || outputIndex >= numOutputs)
        return false;

    juce::StringArray tokens;
    tokens.addTokens (normaliseMuteList (getInputParameter (channelIndex, inputMutes), numOutputs), ",", "");
    tokens.set (outputIndex, muted ? "1" : "0");
    setInputParameter (channelIndex, inputMutes, tokens.joinIntoString (","));
    return true;
}

juce::ValueTree WFSValueTreeState::getInputGradientMapsSection (int channelIndex)
{
    return getInputState (channelIndex).getChildWithName (GradientMaps);
}

juce::ValueTree WFSValueTreeState::getInputSamplerCell (int channelIndex, int cellIndex)
{
    auto sampler = getInputSamplerSection (channelIndex);
    if (! sampler.isValid() || cellIndex < 0)
        return {};
    return findChildByIntProperty (sampler, SamplerCell, id, cellIndex);
}

juce::ValueTree WFSValueTreeState::getInputSamplerSet (int channelIndex, int setIndex)
{
    auto sampler = getInputSamplerSection (channelIndex);
    if (! sampler.isValid() || setIndex < 0)
        return {};

    // Ordinal, not id: SamplerSubTab::onDeleteSet removes a child without
    // renumbering the rest, so the id property goes stale on the survivors.
    int seen = 0;
    for (int i = 0; i < sampler.getNumChildren(); ++i)
    {
        auto child = sampler.getChild (i);
        if (! child.hasType (SamplerSet))
            continue;
        if (seen == setIndex)
            return child;
        ++seen;
    }
    return {};
}

int WFSValueTreeState::getNumInputSamplerSets (int channelIndex)
{
    auto sampler = getInputSamplerSection (channelIndex);
    if (! sampler.isValid())
        return 0;

    int setCount = 0;
    for (int i = 0; i < sampler.getNumChildren(); ++i)
        if (sampler.getChild (i).hasType (SamplerSet))
            ++setCount;
    return setCount;
}

juce::ValueTree WFSValueTreeState::addInputSamplerSet (int channelIndex, const juce::String& setName)
{
    auto sampler = ensureInputSamplerSection (channelIndex);
    if (! sampler.isValid())
        return {};

    const int existing = getNumInputSamplerSets (channelIndex);
    if (existing >= maxSamplerSets)
        return {};

    // Defaults read off a default-constructed SamplerSet rather than written out
    // here, so this and SamplerSubTab::saveSetToValueTree cannot disagree about
    // what a new set looks like.
    const SamplerData::SamplerSet defaults;

    juce::ValueTree set (SamplerSet);
    set.setProperty (id, existing, nullptr);
    set.setProperty (samplerSetName, setName.isNotEmpty() ? setName
                                                          : "Set " + juce::String (existing + 1), nullptr);
    set.setProperty (samplerSetPlayMode, defaults.playMode, nullptr);
    set.setProperty (samplerSetCells,    juce::String(), nullptr);
    set.setProperty (samplerSetPosX,     defaults.posX, nullptr);
    set.setProperty (samplerSetPosY,     defaults.posY, nullptr);
    set.setProperty (samplerSetPosZ,     defaults.posZ, nullptr);
    set.setProperty (samplerSetLevel,    defaults.level, nullptr);

    set.setProperty (samplerSetPressLevelEnabled, defaults.pressLevel.enabled ? 1 : 0, nullptr);
    set.setProperty (samplerSetPressLevelDir,     defaults.pressLevel.direction, nullptr);
    set.setProperty (samplerSetPressLevelCurve,   defaults.pressLevel.curve, nullptr);

    set.setProperty (samplerSetPressZEnabled, defaults.pressZ.enabled ? 1 : 0, nullptr);
    set.setProperty (samplerSetPressZDir,     defaults.pressZ.direction, nullptr);
    set.setProperty (samplerSetPressZCurve,   defaults.pressZ.curve, nullptr);

    set.setProperty (samplerSetPressHFEnabled, defaults.pressHF.enabled ? 1 : 0, nullptr);
    set.setProperty (samplerSetPressHFDir,     defaults.pressHF.direction, nullptr);
    set.setProperty (samplerSetPressHFCurve,   defaults.pressHF.curve, nullptr);

    set.setProperty (samplerSetPressXYEnabled, defaults.pressXYEnabled ? 1 : 0, nullptr);
    set.setProperty (samplerSetPressXYScale,   defaults.pressXYScale, nullptr);

    sampler.appendChild (set, nullptr);
    return set;
}

bool WFSValueTreeState::removeInputSamplerSet (int channelIndex, int setIndex)
{
    auto set = getInputSamplerSet (channelIndex, setIndex);
    if (! set.isValid())
        return false;

    auto sampler = getInputSamplerSection (channelIndex);
    sampler.removeChild (set, nullptr);
    return true;
}

juce::ValueTree WFSValueTreeState::getInputGradientShape (int channelIndex, int layerIndex, int shapeIndex)
{
    auto layer = getInputGradientLayer (channelIndex, layerIndex);
    if (! layer.isValid() || shapeIndex < 0)
        return {};

    // Shapes are identified by position, not by an id property: GradientShape
    // carries no id, and the editor re-sorts by gmShapeOrder when it loads a
    // layer. Count only GradientShape children, so a layer that ever grows
    // another kind of child does not shift the numbering.
    int seen = 0;
    for (int i = 0; i < layer.getNumChildren(); ++i)
    {
        auto child = layer.getChild (i);
        if (! child.hasType (GradientShape))
            continue;
        if (seen == shapeIndex)
            return child;
        ++seen;
    }
    return {};
}

juce::ValueTree WFSValueTreeState::getInputGradientLayer (int channelIndex, int layerIndex)
{
    auto gm = getInputGradientMapsSection (channelIndex);
    if (! gm.isValid() || layerIndex < 0 || layerIndex >= gm.getNumChildren())
        return {};

    return gm.getChild (layerIndex);
}

juce::ValueTree WFSValueTreeState::ensureInputGradientMapsSection (int channelIndex)
{
    auto input = getInputState (channelIndex);
    if (! input.isValid())
        return {};

    auto gm = input.getChildWithName (GradientMaps);
    if (! gm.isValid())
    {
        gm = createInputGradientMapsSection();
        input.appendChild (gm, nullptr);
    }
    return gm;
}

juce::ValueTree WFSValueTreeState::getInputSamplerSection (int channelIndex)
{
    return getInputState (channelIndex).getChildWithName (Sampler);
}

juce::ValueTree WFSValueTreeState::ensureInputSamplerSection (int channelIndex)
{
    auto input = getInputState (channelIndex);
    if (! input.isValid())
        return {};

    auto sampler = input.getChildWithName (Sampler);
    if (! sampler.isValid())
    {
        sampler = createInputSamplerSection();
        input.appendChild (sampler, nullptr);
    }
    return sampler;
}

//==============================================================================
// Output Channel Access
//==============================================================================

juce::var WFSValueTreeState::getOutputParameter (int channelIndex, const juce::Identifier& paramId) const
{
    auto output = const_cast<WFSValueTreeState*>(this)->getOutputState (channelIndex);
    if (!output.isValid())
        return {};

    // Search through all subsections
    for (int i = 0; i < output.getNumChildren(); ++i)
    {
        auto child = output.getChild (i);
        if (child.hasProperty (paramId))
            return child.getProperty (paramId);
    }
    return {};
}

void WFSValueTreeState::setOutputParameter (int channelIndex, const juce::Identifier& paramId, const juce::var& value)
{
    auto output = getOutputState (channelIndex);
    if (!output.isValid())
        return;

    // Search through all subsections
    for (int i = 0; i < output.getNumChildren(); ++i)
    {
        auto child = output.getChild (i);
        if (child.hasProperty (paramId))
        {
            writeProperty (child, paramId, value, getActiveUndoManager());

            // Speaker positions are never auto-placed, but editing one still
            // signals the user is positioning things (see the input twin).
            if (paramId == outputPositionX || paramId == outputPositionY || paramId == outputPositionZ)
                markPositionsUserOwned();

            return;
        }
    }

    // Property not found - add it to the appropriate section if we know where it belongs
    // This handles old config files that may be missing newer properties
    if (paramId == outputCoordinateMode)
    {
        auto position = getOutputPositionSection (channelIndex);
        if (position.isValid())
            writeProperty (position, paramId, value, getActiveUndoManager());
    }
}

void WFSValueTreeState::setOutputParameterDirect (int channelIndex, const juce::Identifier& paramId, const juce::var& value)
{
    auto output = getOutputState (channelIndex);
    if (!output.isValid())
        return;

    // Search through all subsections (but skip EQ bands)
    for (int i = 0; i < output.getNumChildren(); ++i)
    {
        auto child = output.getChild (i);
        if (child.hasProperty (paramId))
        {
            writeProperty (child, paramId, value, getActiveUndoManager());
            return;
        }
    }
}

void WFSValueTreeState::setOutputEQBandParameterDirect (int channelIndex, int bandIndex, const juce::Identifier& paramId, const juce::var& value)
{
    auto band = getOutputEQBand (channelIndex, bandIndex);
    if (band.isValid())
        writeProperty (band, paramId, value, getActiveUndoManager());
}

bool WFSValueTreeState::isArrayLinkedParameter (const juce::Identifier& paramId)
{
    // Parameters that should propagate to array members
    return paramId == outputAttenuation ||
           paramId == outputDelayLatency ||
           paramId == outputOrientation ||
           paramId == outputAngleOn ||
           paramId == outputAngleOff ||
           paramId == outputPitch ||
           paramId == outputHFdamping ||
           paramId == outputMiniLatencyEnable ||
           paramId == outputLSattenEnable ||
           paramId == outputFRenable ||
           paramId == outputDistanceAttenPercent ||
           paramId == outputHparallax ||
           paramId == outputVparallax ||
           paramId == outputEQenabled;
}

bool WFSValueTreeState::isArrayLinkedEQParameter (const juce::Identifier& paramId)
{
    // EQ band parameters that should propagate to array members
    return paramId == eqShape ||
           paramId == eqFrequency ||
           paramId == eqGain ||
           paramId == eqQ ||
           paramId == eqSlope;
}

bool WFSValueTreeState::isBooleanOutputParameter (const juce::Identifier& paramId)
{
    // On/off toggles: a "relative" delta is meaningless for these, so array
    // propagation must always share the absolute state (see propagation below).
    return paramId == outputMiniLatencyEnable ||
           paramId == outputLSattenEnable ||
           paramId == outputFRenable ||
           paramId == outputEQenabled;
}

float WFSValueTreeState::clampOutputParamToRange (const juce::Identifier& paramId, float value)
{
    using namespace WFSParameterDefaults;

    if (paramId == outputAttenuation)
        return juce::jlimit (outputAttenuationMin, outputAttenuationMax, value);
    if (paramId == outputDelayLatency)
        return juce::jlimit (outputDelayLatencyMin, outputDelayLatencyMax, value);
    if (paramId == outputOrientation)
        return juce::jlimit (static_cast<float> (outputOrientationMin), static_cast<float> (outputOrientationMax), value);
    if (paramId == outputAngleOn)
        return juce::jlimit (static_cast<float> (outputAngleOnMin), static_cast<float> (outputAngleOnMax), value);
    if (paramId == outputAngleOff)
        return juce::jlimit (static_cast<float> (outputAngleOffMin), static_cast<float> (outputAngleOffMax), value);
    if (paramId == outputPitch)
        return juce::jlimit (static_cast<float> (outputPitchMin), static_cast<float> (outputPitchMax), value);
    if (paramId == outputHFdamping)
        return juce::jlimit (outputHFdampingMin, outputHFdampingMax, value);
    if (paramId == outputDistanceAttenPercent)
        return juce::jlimit (static_cast<float> (outputDistanceAttenPercentMin), static_cast<float> (outputDistanceAttenPercentMax), value);
    if (paramId == outputHparallax)
        return juce::jlimit (outputHparallaxMin, outputHparallaxMax, value);
    if (paramId == outputVparallax)
        return juce::jlimit (outputVparallaxMin, outputVparallaxMax, value);

    // EQ parameters
    if (paramId == eqFrequency)
        return juce::jlimit (eqFrequencyMin, eqFrequencyMax, value);
    if (paramId == eqGain)
        return juce::jlimit (eqGainMin, eqGainMax, value);
    if (paramId == eqQ)
        return juce::jlimit (eqQMin, eqQMax, value);
    if (paramId == eqSlope)
        return juce::jlimit (eqSlopeMin, eqSlopeMax, value);
    if (paramId == eqShape)
        return juce::jlimit (static_cast<float> (eqShapeMin), static_cast<float> (eqShapeMax), value);

    // Boolean/toggle parameters (0 or 1)
    if (paramId == outputMiniLatencyEnable || paramId == outputLSattenEnable ||
        paramId == outputFRenable || paramId == outputEQenabled)
        return value != 0.0f ? 1.0f : 0.0f;

    return value;
}

void WFSValueTreeState::setOutputParameterWithArrayPropagation (int channelIndex,
                                                                 const juce::Identifier& paramId,
                                                                 const juce::var& value,
                                                                 bool propagateToArray)
{
    // Check if this is an array-linked parameter
    if (!propagateToArray || !isArrayLinkedParameter (paramId))
    {
        setOutputParameter (channelIndex, paramId, value);
        return;
    }

    // Get array assignment for this output
    int arrayId = static_cast<int> (getOutputParameter (channelIndex, outputArray));
    if (arrayId == 0)  // Single, not in array
    {
        setOutputParameter (channelIndex, paramId, value);
        return;
    }

    // Get apply mode for this output
    int applyMode = static_cast<int> (getOutputParameter (channelIndex, outputApplyToArray));
    if (applyMode == 0)  // OFF
    {
        setOutputParameter (channelIndex, paramId, value);
        return;
    }

    // Get old value for RELATIVE mode delta calculation
    auto oldValue = getOutputParameter (channelIndex, paramId);
    float oldFloat = static_cast<float> (oldValue);
    float newFloat = static_cast<float> (value);
    float delta = newFloat - oldFloat;

    // Set the originating channel
    setOutputParameter (channelIndex, paramId, value);

    // Propagate to array members
    int numOutputs = getNumOutputChannels();
    for (int i = 0; i < numOutputs; ++i)
    {
        if (i == channelIndex)
            continue;  // Skip originating channel

        // Check if this output is in the same array
        int memberArray = static_cast<int> (getOutputParameter (i, outputArray));
        if (memberArray != arrayId)
            continue;

        // Check member's apply mode (per-output unlinking)
        int memberApplyMode = static_cast<int> (getOutputParameter (i, outputApplyToArray));
        if (memberApplyMode == 0)  // This member is unlinked (OFF)
            continue;

        // Absolute value when BOTH source and member are ABSOLUTE, or for on/off
        // toggles in any mode — a toggle has no meaningful relative offset, so a
        // delta would invert already-matching members instead of sharing the state.
        // Otherwise apply the delta (RELATIVE source, or ABSOLUTE source → RELATIVE member).
        if ((applyMode == 1 && memberApplyMode == 1) || isBooleanOutputParameter (paramId))
        {
            setOutputParameterDirect (i, paramId, value);
        }
        else
        {
            float memberCurrent = static_cast<float> (getOutputParameter (i, paramId));
            float memberNew = clampOutputParamToRange (paramId, memberCurrent + delta);

            // For int parameters, round the result (toggles never reach this branch)
            if (paramId == outputOrientation || paramId == outputAngleOn ||
                paramId == outputAngleOff || paramId == outputPitch ||
                paramId == outputDistanceAttenPercent)
            {
                setOutputParameterDirect (i, paramId, static_cast<int> (std::round (memberNew)));
            }
            else
            {
                setOutputParameterDirect (i, paramId, memberNew);
            }
        }
    }
}

void WFSValueTreeState::setOutputEQBandParameterWithArrayPropagation (int channelIndex,
                                                                       int bandIndex,
                                                                       const juce::Identifier& paramId,
                                                                       const juce::var& value,
                                                                       bool propagateToArray)
{
    // Check if this is an array-linked EQ parameter
    if (!propagateToArray || !isArrayLinkedEQParameter (paramId))
    {
        auto band = getOutputEQBand (channelIndex, bandIndex);
        if (band.isValid())
            writeProperty (band, paramId, value, getActiveUndoManager());
        return;
    }

    // Get array assignment for this output
    int arrayId = static_cast<int> (getOutputParameter (channelIndex, outputArray));
    if (arrayId == 0)  // Single, not in array
    {
        auto band = getOutputEQBand (channelIndex, bandIndex);
        if (band.isValid())
            writeProperty (band, paramId, value, getActiveUndoManager());
        return;
    }

    // Get apply mode for this output
    int applyMode = static_cast<int> (getOutputParameter (channelIndex, outputApplyToArray));
    if (applyMode == 0)  // OFF
    {
        auto band = getOutputEQBand (channelIndex, bandIndex);
        if (band.isValid())
            writeProperty (band, paramId, value, getActiveUndoManager());
        return;
    }

    // Get old value for RELATIVE mode delta calculation
    auto band = getOutputEQBand (channelIndex, bandIndex);
    if (!band.isValid())
        return;

    float oldFloat = static_cast<float> (band.getProperty (paramId));
    float newFloat = static_cast<float> (value);
    float delta = newFloat - oldFloat;

    // Set the originating channel's band
    writeProperty (band, paramId, value, getActiveUndoManager());

    // Propagate to array members
    int numOutputs = getNumOutputChannels();
    for (int i = 0; i < numOutputs; ++i)
    {
        if (i == channelIndex)
            continue;

        int memberArray = static_cast<int> (getOutputParameter (i, outputArray));
        if (memberArray != arrayId)
            continue;

        int memberApplyMode = static_cast<int> (getOutputParameter (i, outputApplyToArray));
        if (memberApplyMode == 0)
            continue;

        auto memberBand = getOutputEQBand (i, bandIndex);
        if (!memberBand.isValid())
            continue;

        // Absolute value only when BOTH source and member are ABSOLUTE;
        // otherwise apply the delta
        if (applyMode == 1 && memberApplyMode == 1)
        {
            setOutputEQBandParameterDirect (i, bandIndex, paramId, value);
        }
        else
        {
            float memberCurrent = static_cast<float> (memberBand.getProperty (paramId));
            float memberNew = clampOutputParamToRange (paramId, memberCurrent + delta);

            // For eqShape (int), round the result
            if (paramId == eqShape)
                setOutputEQBandParameterDirect (i, bandIndex, paramId, static_cast<int> (std::round (memberNew)));
            else
                setOutputEQBandParameterDirect (i, bandIndex, paramId, memberNew);
        }
    }
}

juce::ValueTree WFSValueTreeState::getOutputChannelSection (int channelIndex)
{
    return getOutputState (channelIndex).getChildWithName (Channel);
}

juce::ValueTree WFSValueTreeState::getOutputPositionSection (int channelIndex)
{
    return getOutputState (channelIndex).getChildWithName (Position);
}

juce::ValueTree WFSValueTreeState::getOutputOptionsSection (int channelIndex)
{
    return getOutputState (channelIndex).getChildWithName (Options);
}

juce::ValueTree WFSValueTreeState::getOutputEQSection (int channelIndex)
{
    return getOutputState (channelIndex).getChildWithName (EQ);
}

juce::ValueTree WFSValueTreeState::getOutputEQBand (int channelIndex, int bandIndex)
{
    // By TYPE, never by position - see nthChildOfType. <EQ> holds <Band> children
    // and nothing else, and a loaded outputs.xml is exactly what can make that
    // untrue. This accessor has live remote callers (the MCP EQ-band dispatcher,
    // the generic get/set tools, the dials pages), so a foreign child in here
    // used to make a write to band 2 land on it and report success.
    return nthChildOfType (getOutputEQSection (channelIndex), Band, bandIndex);
}

//==============================================================================
// Reverb Channel Access
//==============================================================================

juce::var WFSValueTreeState::getReverbParameter (int channelIndex, const juce::Identifier& paramId) const
{
    auto reverb = const_cast<WFSValueTreeState*>(this)->getReverbState (channelIndex);
    if (!reverb.isValid())
        return {};

    // Search through all subsections
    for (int i = 0; i < reverb.getNumChildren(); ++i)
    {
        auto child = reverb.getChild (i);
        if (child.hasProperty (paramId))
            return child.getProperty (paramId);

        // Check EQ bands
        if (child.getType() == EQ)
        {
            for (int j = 0; j < child.getNumChildren(); ++j)
            {
                auto band = child.getChild (j);
                if (band.hasProperty (paramId))
                    return band.getProperty (paramId);
            }
        }
    }
    return {};
}

void WFSValueTreeState::setReverbParameter (int channelIndex, const juce::Identifier& paramId, const juce::var& value)
{
    auto reverb = getReverbState (channelIndex);
    if (!reverb.isValid())
        return;

    // Search through all subsections
    for (int i = 0; i < reverb.getNumChildren(); ++i)
    {
        auto child = reverb.getChild (i);
        if (child.hasProperty (paramId))
        {
            writeProperty (child, paramId, value, getActiveUndoManager());

            // Node or return position edits latch user ownership of positions
            // (see the input twin). Return offsets count: they are where the
            // reverb is perceived, edited from the same tab.
            if (paramId == reverbPositionX     || paramId == reverbPositionY     || paramId == reverbPositionZ
             || paramId == reverbReturnOffsetX || paramId == reverbReturnOffsetY || paramId == reverbReturnOffsetZ)
                markPositionsUserOwned();

            return;
        }

        // Check EQ bands
        if (child.getType() == EQ)
        {
            for (int j = 0; j < child.getNumChildren(); ++j)
            {
                auto band = child.getChild (j);
                if (band.hasProperty (paramId))
                {
                    writeProperty (band, paramId, value, getActiveUndoManager());
                    return;
                }
            }
        }
    }

    // Property not found - add it to the appropriate section if we know where it belongs
    // This handles old config files that may be missing newer properties
    if (paramId == reverbCoordinateMode)
    {
        auto position = getReverbPositionSection (channelIndex);
        if (position.isValid())
            writeProperty (position, paramId, value, getActiveUndoManager());
    }
}

juce::ValueTree WFSValueTreeState::getReverbChannelSection (int channelIndex)
{
    return getReverbState (channelIndex).getChildWithName (Channel);
}

juce::ValueTree WFSValueTreeState::getReverbPositionSection (int channelIndex)
{
    return getReverbState (channelIndex).getChildWithName (Position);
}

juce::ValueTree WFSValueTreeState::getReverbFeedSection (int channelIndex)
{
    return getReverbState (channelIndex).getChildWithName (Feed);
}

juce::ValueTree WFSValueTreeState::getReverbEQSection (int channelIndex)
{
    return getReverbState (channelIndex).getChildWithName (EQ);
}

juce::ValueTree WFSValueTreeState::ensureReverbEQSection (int channelIndex)
{
    auto reverb = getReverbState (channelIndex);
    if (! reverb.isValid())
        return {};

    auto eq = reverb.getChildWithName (EQ);
    if (! eq.isValid())
    {
        // Create the EQ section if it doesn't exist (e.g., loading old config)
        eq = createReverbEQSection();
        reverb.appendChild (eq, nullptr);
    }
    else
    {
        // Migrate old property names: reverbEQ* -> reverbPreEQ*
        using namespace WFSParameterIDs;
        using namespace WFSParameterDefaults;
        static const juce::Identifier oldEQenable  ("reverbEQenable");
        static const juce::Identifier oldEQshape   ("reverbEQshape");
        static const juce::Identifier oldEQfreq    ("reverbEQfreq");
        static const juce::Identifier oldEQgain    ("reverbEQgain");
        static const juce::Identifier oldEQq       ("reverbEQq");
        static const juce::Identifier oldEQslope   ("reverbEQslope");

        if (eq.hasProperty (oldEQenable))
        {
            eq.setProperty (reverbPreEQenable, eq.getProperty (oldEQenable), nullptr);
            eq.removeProperty (oldEQenable, nullptr);
        }

        for (int i = 0; i < eq.getNumChildren(); ++i)
        {
            auto band = eq.getChild (i);
            if (band.hasProperty (oldEQshape))
            {
                band.setProperty (reverbPreEQshape, band.getProperty (oldEQshape), nullptr);
                band.removeProperty (oldEQshape, nullptr);
            }
            if (band.hasProperty (oldEQfreq))
            {
                band.setProperty (reverbPreEQfreq, band.getProperty (oldEQfreq), nullptr);
                band.removeProperty (oldEQfreq, nullptr);
            }
            if (band.hasProperty (oldEQgain))
            {
                band.setProperty (reverbPreEQgain, band.getProperty (oldEQgain), nullptr);
                band.removeProperty (oldEQgain, nullptr);
            }
            if (band.hasProperty (oldEQq))
            {
                band.setProperty (reverbPreEQq, band.getProperty (oldEQq), nullptr);
                band.removeProperty (oldEQq, nullptr);
            }
            if (band.hasProperty (oldEQslope))
            {
                band.setProperty (reverbPreEQslope, band.getProperty (oldEQslope), nullptr);
                band.removeProperty (oldEQslope, nullptr);
            }
        }
    }
    return eq;
}

juce::ValueTree WFSValueTreeState::getReverbEQBand (int channelIndex, int bandIndex)
{
    // By TYPE - see nthChildOfType, and note getReverbEQSection above already
    // MIGRATES old band property names in place, so this is a child list a FILE
    // has demonstrably written. The most exposed of the five:
    // /wfs/reverb/n/eq/b/gain resolves through here (OSCManager), as do the MCP
    // band tools and the GUI tab, and an unknown child inside <EQ> used to send
    // every one of those writes onto it with a success reply.
    return nthChildOfType (getReverbEQSection (channelIndex), Band, bandIndex);
}

juce::ValueTree WFSValueTreeState::getReverbReturnSection (int channelIndex)
{
    return getReverbState (channelIndex).getChildWithName (ReverbReturn);
}

juce::ValueTree WFSValueTreeState::getReverbAlgorithmSection()
{
    return getReverbsState().getChildWithName (ReverbAlgorithm);
}

juce::ValueTree WFSValueTreeState::ensureReverbAlgorithmSection()
{
    auto reverbs = getReverbsState();
    if (! reverbs.isValid())
        return {};

    auto algo = reverbs.getChildWithName (ReverbAlgorithm);
    if (! algo.isValid())
    {
        // Create the algorithm section if it doesn't exist (e.g., loading old config)
        algo = createReverbAlgorithmSection();
        reverbs.appendChild (algo, nullptr);
    }
    return algo;
}

juce::ValueTree WFSValueTreeState::getReverbPostEQSection()
{
    return getReverbsState().getChildWithName (ReverbPostEQ);
}

juce::ValueTree WFSValueTreeState::ensureReverbPostEQSection()
{
    auto reverbs = getReverbsState();
    if (! reverbs.isValid())
        return {};

    auto postEQ = reverbs.getChildWithName (ReverbPostEQ);
    if (! postEQ.isValid())
    {
        // Create the post-EQ section if it doesn't exist (e.g., loading old config)
        postEQ = createReverbPostEQSection();
        reverbs.appendChild (postEQ, nullptr);
    }
    return postEQ;
}

juce::ValueTree WFSValueTreeState::getReverbPostEQBand (int bandIndex)
{
    // By TYPE - see nthChildOfType. <ReverbPostEQ> holds <PostEQBand> children,
    // NOT <Band>: the post EQ is a global sibling of the reverb channels and its
    // bands carry their own node type, which is what keeps the two id namespaces
    // apart. Naming the wrong type here would resolve nothing at all, which is
    // the loud failure rather than the silent one.
    return nthChildOfType (getReverbPostEQSection(), PostEQBand, bandIndex);
}

juce::ValueTree WFSValueTreeState::getReverbPreCompSection()
{
    return getReverbsState().getChildWithName (ReverbPreComp);
}

juce::ValueTree WFSValueTreeState::ensureReverbPreCompSection()
{
    auto reverbs = getReverbsState();
    if (! reverbs.isValid())
        return {};

    auto preComp = reverbs.getChildWithName (ReverbPreComp);
    if (! preComp.isValid())
    {
        preComp = createReverbPreCompSection();
        reverbs.appendChild (preComp, nullptr);
    }
    return preComp;
}

juce::ValueTree WFSValueTreeState::getReverbPostExpSection()
{
    return getReverbsState().getChildWithName (ReverbPostExp);
}

juce::ValueTree WFSValueTreeState::ensureReverbPostExpSection()
{
    auto reverbs = getReverbsState();
    if (! reverbs.isValid())
        return {};

    auto postExp = reverbs.getChildWithName (ReverbPostExp);
    if (! postExp.isValid())
    {
        postExp = createReverbPostExpSection();
        reverbs.appendChild (postExp, nullptr);
    }
    return postExp;
}

//==============================================================================
// Effects Channel Access
//==============================================================================

namespace
{
    /** The eleven chain slots in their declared order - the same order and the
        same tokens as spatcore::effects::kSlots (dist, eq1, eq2, dyn1, dyn2,
        mod, phaser, trem, reverb, delay, crush). Each slot is its own id-less
        node TYPE, so the tree is a 1:1 transcription of that table and
        getChildWithName addresses a slot with no sub-index at all. */
    const juce::Identifier* const effectModuleTypeTable[] = {
        &FxDist, &FxEq1, &FxEq2, &FxDyn1, &FxDyn2, &FxMod,
        &FxPhaser, &FxTrem, &FxReverb, &FxDelay, &FxCrush
    };

    static_assert ((int) (sizeof (effectModuleTypeTable) / sizeof (effectModuleTypeTable[0]))
                       == WFSParameterDefaults::numEffectModuleSlots,
                   "the effect module type table and numEffectModuleSlots must agree");

    /** How much further out than the reverb arc the effect ring sits.

        The two families are the same kind of object - a virtual source outside
        the stage whose feed bearing and inter-node spacing both matter - so
        they share the placement helper. They must not share the SPOT: a session
        with four reverbs and four effects would otherwise put eight nodes on
        four positions, which reads as four nodes on the map and gives the
        angular attenuation two feeds pointing identically. */
    constexpr float kEffectRingExpansion = 1.25f;
}

const juce::Identifier& WFSValueTreeState::getEffectModuleType (int slotIndex)
{
    static const juce::Identifier none;
    if (slotIndex < 0 || slotIndex >= numEffectModuleSlots)
        return none;
    return *effectModuleTypeTable[(size_t) slotIndex];
}

bool WFSValueTreeState::isInstancedEffectModuleType (const juce::Identifier& nodeType)
{
    return nodeType == FxEq1 || nodeType == FxEq2 || nodeType == FxDyn1 || nodeType == FxDyn2;
}

juce::var WFSValueTreeState::getEffectParameter (int channelIndex, const juce::Identifier& paramId) const
{
    auto effect = const_cast<WFSValueTreeState*>(this)->getEffectState (channelIndex);
    if (! effect.isValid())
        return {};

    for (int i = 0; i < effect.getNumChildren(); ++i)
    {
        auto child = effect.getChild (i);

        // FxEq1/FxEq2 and FxDyn1/FxDyn2 carry identical property names, and the
        // <Band> / <Tap> grandchildren repeat theirs across siblings. A by-name
        // hit on any of those could only ever mean "the first one", so this walk
        // does not look: getEffectEQBand / getEffectDelayTap /
        // getEffectModuleSection take the missing index, and a generic caller
        // gets an honest miss instead of a silent write to instance 1. (The
        // reverb twin does descend into its EQ bands and always answers band 1;
        // that is a known wart, not the model to copy.)
        if (isInstancedEffectModuleType (child.getType()))
            continue;

        if (child.hasProperty (paramId))
            return child.getProperty (paramId);
    }
    return {};
}

void WFSValueTreeState::setEffectParameter (int channelIndex, const juce::Identifier& paramId, const juce::var& value)
{
    auto effect = getEffectState (channelIndex);
    if (! effect.isValid())
        return;

    for (int i = 0; i < effect.getNumChildren(); ++i)
    {
        auto child = effect.getChild (i);

        if (isInstancedEffectModuleType (child.getType()))
            continue;   // see getEffectParameter

        if (child.hasProperty (paramId))
        {
            writeProperty (child, paramId, value, getActiveUndoManager());

            // Placing a return by hand ends automatic layout, exactly as it does
            // for inputs and reverbs - but through the effects-only latch. The
            // return offsets count: that is where the effect is perceived.
            if (paramId == effectPositionX     || paramId == effectPositionY     || paramId == effectPositionZ
             || paramId == effectReturnOffsetX || paramId == effectReturnOffsetY || paramId == effectReturnOffsetZ)
                markEffectPositionsUserOwned();

            return;
        }
    }
}

juce::ValueTree WFSValueTreeState::getEffectChannelSection (int channelIndex)
{
    return getEffectState (channelIndex).getChildWithName (Channel);
}

juce::ValueTree WFSValueTreeState::getEffectPositionSection (int channelIndex)
{
    return getEffectState (channelIndex).getChildWithName (Position);
}

juce::ValueTree WFSValueTreeState::getEffectFeedSection (int channelIndex)
{
    return getEffectState (channelIndex).getChildWithName (Feed);
}

juce::ValueTree WFSValueTreeState::getEffectReturnSection (int channelIndex)
{
    // ReverbReturn is the C++ name; the XML tag is plain "Return", which is the
    // tag an effect return uses too (see the identifier header).
    return getEffectState (channelIndex).getChildWithName (ReverbReturn);
}

juce::ValueTree WFSValueTreeState::getEffectAutoMotionSection (int channelIndex)
{
    return getEffectState (channelIndex).getChildWithName (AutomOtion);
}

juce::ValueTree WFSValueTreeState::getEffectLFOSection (int channelIndex)
{
    return getEffectState (channelIndex).getChildWithName (LFO);
}

juce::ValueTree WFSValueTreeState::getEffectChainSection (int channelIndex)
{
    return getEffectState (channelIndex).getChildWithName (Chain);
}

juce::ValueTree WFSValueTreeState::getEffectSendsSection (int channelIndex)
{
    return getEffectState (channelIndex).getChildWithName (Sends);
}

//==============================================================================
// Link groups - propagation between the members of one group
//
// MODELLED ON THE OUTPUT ARRAY, NOT ON CLUSTERS (R5-6). The plan originally
// said to clone ClusterParamEdit, and clusters have membership with NO
// per-member mode - cloning that template would have inherited exactly the gap
// effectLinkMode closes. The shape here is setOutputParameterWithArrayPropagation:
// membership plus a mode on every member, and the RECEIVER's mode is consulted
// as well as the origin's, which is what lets a channel be detached from the
// detached channel's own side.
//
// Writes that do NOT come through these methods never propagate: OSC, MCP,
// snapshot recall and file loads all call the plain setters, exactly as the
// input and output families behave.
//==============================================================================

bool WFSValueTreeState::isEffectLinkExcluded (const juce::Identifier& paramId)
{
    // What a link group must never share. Position, the return offset and the
    // name are the channel's identity in the show; the send rows are its
    // routing; AutomOtion is a movement authored per channel.
    //
    // MUTES ARE HERE, NOT IN THE ABSOLUTE-ONLY SET (R5-1). The plan had them
    // propagating, which made two linked channels share one mute state so
    // neither could be silenced alone - the opposite of the requirement. A
    // group mute is an ACTION instead (setEffectGroupMute), which writes every
    // member once and leaves each independently editable afterwards.
    // effectSolo was already excluded, and solo independent while mute was
    // shared was never coherent.
    static const std::set<juce::Identifier> excluded = {
        effectName,
        effectPositionX, effectPositionY, effectPositionZ, effectCoordinateMode,
        effectReturnOffsetX, effectReturnOffsetY, effectReturnOffsetZ,
        effectLinkGroup, effectLinkMode,
        effectMute, effectMutes, effectMuteMacro, effectMuteReverbSends, effectSolo,
        effectSendLevels, effectSendOns, effectFxSendLevels, effectFxSendOns,
        effectSendLevel, effectSendOn, effectFxSendLevel, effectFxSendOn,
        effectOtomoX, effectOtomoY, effectOtomoZ, effectOtomoCoordinateMode,
        effectOtomoR, effectOtomoTheta, effectOtomoRsph, effectOtomoPhi,
        effectOtomoAbsoluteRelative, effectOtomoSpeedProfile, effectOtomoDuration,
        effectOtomoCurve, effectOtomoTrigger, effectOtomoThreshold, effectOtomoReset,
        effectOtomoPauseResume,
        effectLFOactive, effectLFOperiod, effectLFOphase,
        effectLFOshapeX, effectLFOshapeY, effectLFOshapeZ,
        effectLFOrateX, effectLFOrateY, effectLFOrateZ,
        effectLFOamplitudeX, effectLFOamplitudeY, effectLFOamplitudeZ,
        effectLFOphaseX, effectLFOphaseY, effectLFOphaseZ,
    };

    return excluded.count (paramId) != 0;
}

bool WFSValueTreeState::isEffectLinkAbsoluteOnly (const juce::Identifier& paramId)
{
    // Discrete values: copied outright in any mode, never delta'd. A toggle,
    // an enum or a validated set has no meaningful offset, and a delta would
    // invert already-matching members instead of sharing the state - the same
    // reasoning isBooleanOutputParameter records for the output family.
    //
    // THIS TABLE IS THE CSV'S "enum" COLUMN. It was generated from
    // Documentation/WFS-UI_effects.csv by taking every per-channel row with a
    // non-empty enum cell, minus the rows isEffectLinkExcluded already stops.
    // Add a row with an enum to that file and it belongs here; a startsWith
    // would be wrong for the usual reason (effectDist / effectDistance*).
    static const std::set<juce::Identifier> absoluteOnly = {
        effectMinimalLatency, effectFeedMiniLatency, effectAttenuationLaw,
        effectChainBypass,
        effectDistBypass, effectDistOversample,
        effectEQBypass, effectEQshape,
        effectDynBypass, effectDynDetector, effectDynAutoMakeup,
        effectDynCompOn, effectDynExpOn,
        effectModBypass, effectModMode, effectModVoices, effectModShape,
        effectModThroughZero,
        effectPhaserBypass, effectPhaserStages, effectPhaserShape,
        effectTremBypass,
        effectReverbBypass, effectReverbModel, effectReverbType,
        effectReverbERProfile, effectReverbShimmerPitch,
        effectDelayBypass, effectDelayTaps, effectDelayTapMode, effectDelayPattern,
        effectDelayFeedbackTap,
        effectCrushBypass, effectCrushFilter,

        // Not an enum, and absolute for a stronger reason: the chain order is
        // a permutation of eleven tokens. "Half a reorder" is not a value.
        effectChainOrder,
    };

    return absoluteOnly.count (paramId) != 0;
}

int WFSValueTreeState::getEffectLinkGroup (int channelIndex)
{
    auto channel = getEffectChannelSection (channelIndex);
    return channel.isValid()
         ? juce::jlimit (effectLinkGroupMin, effectLinkGroupMax,
                         WFSVar::toInt (channel.getProperty (effectLinkGroup), effectLinkGroupDefault))
         : 0;
}

int WFSValueTreeState::getEffectLinkMode (int channelIndex)
{
    auto channel = getEffectChannelSection (channelIndex);
    return channel.isValid()
         ? juce::jlimit (effectLinkModeMin, effectLinkModeMax,
                         WFSVar::toInt (channel.getProperty (effectLinkMode), effectLinkModeDefault))
         : 0;
}

void WFSValueTreeState::applyEffectLinkPropagation (int channelIndex,
                                                    const juce::Identifier& paramId,
                                                    const juce::var& newValue,
                                                    const juce::var& oldValue,
                                                    const std::function<juce::ValueTree (int)>& sectionFor)
{
    // The source channel has already been written by the caller - this walks
    // the other members only.
    if (isEffectLinkExcluded (paramId))
        return;

    const int group = getEffectLinkGroup (channelIndex);
    if (group == 0)                     // unlinked
        return;

    const int originMode = getEffectLinkMode (channelIndex);
    if (originMode == 0)                // this channel is detached
        return;

    const bool absoluteOnly = isEffectLinkAbsoluteOnly (paramId);
    const auto bounds = WFSNetwork::getBounds (paramId);

    const float delta = absoluteOnly ? 0.0f
                                     : static_cast<float> (static_cast<double> (newValue))
                                     - static_cast<float> (static_cast<double> (oldValue));

    const int numEffects = getNumEffectChannels();

    for (int member = 0; member < numEffects; ++member)
    {
        if (member == channelIndex)
            continue;

        if (getEffectLinkGroup (member) != group)
            continue;

        // THE RECEIVER'S MODE, not only the origin's (R5-6). This is what makes
        // "disengage temporarily" work from the detached channel's side rather
        // than requiring the operator to remember which channel they edit from.
        const int memberMode = getEffectLinkMode (member);
        if (memberMode == 0)
            continue;

        auto section = sectionFor (member);
        if (! section.isValid() || ! section.hasProperty (paramId))
            continue;

        if (absoluteOnly || (originMode == 1 && memberMode == 1))
        {
            writeProperty (section, paramId, newValue, getActiveUndoManager());
            continue;
        }

        // Relative: either side asking for it makes the member keep its offset.
        float memberNew = static_cast<float> (static_cast<double> (section.getProperty (paramId))) + delta;

        if (bounds.has_value())
        {
            memberNew = juce::jlimit (static_cast<float> (bounds->min),
                                      static_cast<float> (bounds->max), memberNew);

            if (bounds->isInt)
            {
                writeProperty (section, paramId, static_cast<int> (std::round (memberNew)),
                               getActiveUndoManager());
                continue;
            }
        }

        writeProperty (section, paramId, memberNew, getActiveUndoManager());
    }
}

juce::ValueTree WFSValueTreeState::findEffectSectionCarrying (int channelIndex,
                                                              const juce::Identifier& paramId)
{
    // The same walk setEffectParameter does, and skipping the instanced module
    // types for the same reason: a first-hit search would answer instance 1 and
    // report success. Anything on FxEq1/2 or FxDyn1/2 must come through
    // setEffectModuleParameterWithLinkPropagation instead.
    auto effect = getEffectState (channelIndex);
    if (! effect.isValid())
        return {};

    for (int i = 0; i < effect.getNumChildren(); ++i)
    {
        auto child = effect.getChild (i);

        if (isInstancedEffectModuleType (child.getType()))
            continue;

        if (child.hasProperty (paramId))
            return child;
    }

    return {};
}

void WFSValueTreeState::setEffectParameterWithLinkPropagation (int channelIndex,
                                                               const juce::Identifier& paramId,
                                                               const juce::var& value,
                                                               bool propagateToGroup)
{
    if (! propagateToGroup || isEffectLinkExcluded (paramId))
    {
        setEffectParameter (channelIndex, paramId, value);
        return;
    }

    const auto oldValue = getEffectParameter (channelIndex, paramId);
    setEffectParameter (channelIndex, paramId, value);   // the source, with its ownership latch

    applyEffectLinkPropagation (channelIndex, paramId, value, oldValue,
                                [this, &paramId] (int member)
                                {
                                    return findEffectSectionCarrying (member, paramId);
                                });
}

void WFSValueTreeState::setEffectModuleParameterWithLinkPropagation (int channelIndex,
                                                                     const juce::Identifier& moduleType,
                                                                     const juce::Identifier& paramId,
                                                                     const juce::var& value,
                                                                     bool propagateToGroup)
{
    auto section = getEffectModuleSection (channelIndex, moduleType);
    if (! section.isValid())
        return;

    const auto oldValue = section.getProperty (paramId);
    writeProperty (section, paramId, value, getActiveUndoManager());

    if (! propagateToGroup)
        return;

    applyEffectLinkPropagation (channelIndex, paramId, value, oldValue,
                                [this, &moduleType] (int member)
                                {
                                    return getEffectModuleSection (member, moduleType);
                                });
}

void WFSValueTreeState::setEffectEQBandParameterWithLinkPropagation (int channelIndex,
                                                                     int eqInstance,
                                                                     int bandIndex,
                                                                     const juce::Identifier& paramId,
                                                                     const juce::var& value,
                                                                     bool propagateToGroup)
{
    auto band = getEffectEQBand (channelIndex, eqInstance, bandIndex);
    if (! band.isValid())
        return;

    const auto oldValue = band.getProperty (paramId);
    writeProperty (band, paramId, value, getActiveUndoManager());

    if (! propagateToGroup)
        return;

    // The SAME band of the SAME instance on every member: a link group shares a
    // chain, so band 3 of EQ 2 answers to band 3 of EQ 2.
    applyEffectLinkPropagation (channelIndex, paramId, value, oldValue,
                                [this, eqInstance, bandIndex] (int member)
                                {
                                    return getEffectEQBand (member, eqInstance, bandIndex);
                                });
}

void WFSValueTreeState::setEffectDelayTapParameterWithLinkPropagation (int channelIndex,
                                                                       int tapIndex,
                                                                       const juce::Identifier& paramId,
                                                                       const juce::var& value,
                                                                       bool propagateToGroup)
{
    auto tap = getEffectDelayTap (channelIndex, tapIndex);
    if (! tap.isValid())
        return;

    const auto oldValue = tap.getProperty (paramId);
    writeProperty (tap, paramId, value, getActiveUndoManager());

    if (! propagateToGroup)
        return;

    applyEffectLinkPropagation (channelIndex, paramId, value, oldValue,
                                [this, tapIndex] (int member)
                                {
                                    return getEffectDelayTap (member, tapIndex);
                                });
}

void WFSValueTreeState::setEffectGroupMute (int group, bool muted)
{
    // AN ACTION, NOT A COUPLING (R5-2). One gesture writes the mute of every
    // member; afterwards each member is still independently mutable, which is
    // the whole point - under propagation, unmuting one member would unmute
    // all of them. This is the shape inputMutes + inputMuteMacro already has:
    // independent per-channel state plus a macro that writes many at once.
    //
    // The per-output row is deliberately NOT touched. effectMutes says which
    // speakers carry this return, which is spatial routing the operator
    // authored; effectMute is the channel's own mute and the only thing a
    // group shortcut has any business writing.
    if (group <= 0)
        return;

    beginUndoTransaction (muted ? "Mute Effects Group " + juce::String (group)
                                : "Unmute Effects Group " + juce::String (group));

    const int numEffects = getNumEffectChannels();

    for (int member = 0; member < numEffects; ++member)
        if (getEffectLinkGroup (member) == group)
            setEffectParameter (member, effectMute, muted ? 1 : 0);
}

//==============================================================================
// The send matrix
//==============================================================================

juce::String WFSValueTreeState::normaliseSendLevelList (const juce::var& list, int width,
                                                        float minDb, float maxDb, float defaultDb)
{
    const juce::String padding (juce::jlimit (minDb, maxDb, defaultDb));

    juce::StringArray tokens;
    tokens.addTokens (list.toString(), ",", "");

    for (int i = 0; i < tokens.size(); ++i)
    {
        const juce::String token = tokens[i].trim();

        // KEPT VERBATIM when it is already a number in range. A normaliser that
        // re-spells every token rewrites the operator's whole row on any touch -
        // one formatter's "-6.5" into another's "-6.50000" - and makes a file
        // diff where no value moved. Only what is wrong is rewritten.
        if (isNumericToken (token))
        {
            const float value = token.getFloatValue();
            tokens.set (i, (value >= minDb && value <= maxDb)
                               ? token
                               : juce::String (juce::jlimit (minDb, maxDb, value)));
        }
        else
        {
            // Not a number at all. NOT clamped - there is nothing to clamp - so
            // it becomes the row default, which for a send level is unity into a
            // switch that starts off, i.e. silence.
            tokens.set (i, padding);
        }
    }

    if (width > 0)
    {
        while (tokens.size() < width)
            tokens.add (padding);
        tokens.removeRange (width, tokens.size() - width);
    }

    return tokens.joinIntoString (",");
}

juce::String WFSValueTreeState::normaliseSendSwitchList (const juce::var& list, int width)
{
    // A switch row IS shaped like a mute row - fixed tokens, 0 or 1, the same
    // "any non-zero number is on" rule for files that wrote "1.0" - so it is
    // built the same way. It is NOT routed through normaliseMuteList: that
    // function fits a row to the live OUTPUT count, which is the one thing a
    // send row must never do (a column here is an input number or an effect
    // index), and the day someone changes its resize rule for the mute grid,
    // this row must not follow.
    juce::StringArray tokens;
    tokens.addTokens (list.toString(), ",", "");

    for (int i = 0; i < tokens.size(); ++i)
        tokens.set (i, tokens[i].trim().getIntValue() != 0 ? "1" : "0");

    if (width > 0)
    {
        while (tokens.size() < width)
            tokens.add ("0");
        tokens.removeRange (width, tokens.size() - width);
    }

    return tokens.joinIntoString (",");
}

juce::String WFSValueTreeState::canonicalEffectSendRow (const juce::Identifier& rowId,
                                                        const juce::var& list,
                                                        int selfEffectIndex)
{
    using namespace WFSParameterDefaults;

    // THE FOUR ROWS, AND THE SHAPES THEY DO NOT SHARE. This is the only place
    // that maps a row name onto a width, a value kind and a diagonal rule; every
    // writer asks here, so a row cannot end up shaped by which door the write
    // came in by.
    //
    // The widths are maxInputChannels and maxEffectChannels rather than the live
    // counts, and that is the point: an input column is a PERMANENT NUMBER,
    // which survives deletions, can leave gaps, and can be anything up to the
    // maximum however few channels are live today. Fitting one of these rows to
    // a live count would silently drop the columns of channels that still exist.
    if (rowId == effectSendLevels)
        return normaliseSendLevelList (list, maxInputChannels,
                                       effectSendLevelMin, effectSendLevelMax,
                                       effectSendLevelDefault);

    if (rowId == effectSendOns)
        return normaliseSendSwitchList (list, maxInputChannels);

    // The fx rows carry the DIAGONAL. Forced here, so every writer inherits it:
    // effect n may not feed itself, and a self-send is a feedback loop around a
    // delay line rather than a routing choice anyone asked for. The level cell
    // goes back to the default beside the switch, because a dB sitting in a cell
    // that can never sound is a number no reader may trust - and after a channel
    // removal shifts the columns, a stale level is exactly what would land
    // there.
    auto withDiagonalOff = [selfEffectIndex] (juce::String row, const juce::String& offToken)
    {
        if (selfEffectIndex < 0 || selfEffectIndex >= maxEffectChannels)
            return row;   // a detached node under construction knows no index

        juce::StringArray tokens;
        tokens.addTokens (row, ",", "");
        if (selfEffectIndex < tokens.size())
            tokens.set (selfEffectIndex, offToken);
        return tokens.joinIntoString (",");
    };

    if (rowId == effectFxSendLevels)
        return withDiagonalOff (normaliseSendLevelList (list, maxEffectChannels,
                                                        effectFxSendLevelMin, effectFxSendLevelMax,
                                                        effectFxSendLevelDefault),
                                juce::String (effectFxSendLevelDefault));

    if (rowId == effectFxSendOns)
        return withDiagonalOff (normaliseSendSwitchList (list, maxEffectChannels), "0");

    jassertfalse;   // not a send row - the caller has the wrong identifier
    return list.toString();
}

int WFSValueTreeState::denseEffectIndexOfNode (const juce::ValueTree& node) const
{
    // Up to the channel, then the same count-by-type walk getEffectState does.
    // Not `id - 1`: the id is bookkeeping a merged file can contradict, and this
    // has to name the channel every other accessor calls the nth one.
    auto effect = node;
    while (effect.isValid() && ! effect.hasType (Effect))
        effect = effect.getParent();

    if (! effect.isValid())
        return -1;

    auto effects = getEffectsState();
    int nth = 0;
    for (int i = 0; i < effects.getNumChildren(); ++i)
    {
        auto child = effects.getChild (i);
        if (! child.hasType (Effect))
            continue;
        if (child == effect)
            return nth;
        ++nth;
    }
    return -1;
}

juce::String WFSValueTreeState::readEffectSendCell (int channelIndex, const juce::Identifier& rowId,
                                                    int column) const
{
    if (column < 0)
        return {};

    auto sends = const_cast<WFSValueTreeState*> (this)->getEffectSendsSection (channelIndex);
    if (! sends.isValid() || ! sends.hasProperty (rowId))
        return {};

    // Canonicalised before it is read, so a hand-edited short row still answers
    // for every column it is supposed to have instead of returning nothing for
    // the tail.
    juce::StringArray tokens;
    tokens.addTokens (canonicalEffectSendRow (rowId, sends.getProperty (rowId),
                                              denseEffectIndexOfNode (sends)),
                      ",", "");
    return column < tokens.size() ? tokens[column] : juce::String();
}

bool WFSValueTreeState::writeEffectSendCell (int channelIndex, const juce::Identifier& rowId,
                                             int column, const juce::String& token)
{
    if (column < 0)
        return false;

    auto sends = getEffectSendsSection (channelIndex);
    if (! sends.isValid() || ! sends.hasProperty (rowId))
        return false;

    const int selfIndex = denseEffectIndexOfNode (sends);

    juce::StringArray tokens;
    tokens.addTokens (canonicalEffectSendRow (rowId, sends.getProperty (rowId), selfIndex), ",", "");
    if (column >= tokens.size())
        return false;

    tokens.set (column, token);

    // READ-MODIFY-WRITE OF THE WHOLE ROW through the family setter, the shape
    // setInputOutputMute has: one undo entry, one listener notification carrying
    // the whole row, and one pass through the write interceptor - which
    // canonicalises it again, so what lands is the same whether the value
    // arrived here or as a row string from OSC.
    setEffectParameter (channelIndex, rowId, tokens.joinIntoString (","));
    return true;
}

float WFSValueTreeState::getEffectSendLevelFromInput (int channelIndex, int inputPermanentNumber) const
{
    const auto cell = readEffectSendCell (channelIndex, effectSendLevels, inputPermanentNumber - 1);
    return cell.isEmpty() ? effectSendLevelDefault : cell.getFloatValue();
}

bool WFSValueTreeState::setEffectSendLevelFromInput (int channelIndex, int inputPermanentNumber,
                                                     float levelDb)
{
    const float clamped = juce::jlimit (effectSendLevelMin, effectSendLevelMax, levelDb);
    return writeEffectSendCell (channelIndex, effectSendLevels, inputPermanentNumber - 1,
                                juce::String (clamped));
}

bool WFSValueTreeState::getEffectSendOnFromInput (int channelIndex, int inputPermanentNumber) const
{
    return readEffectSendCell (channelIndex, effectSendOns, inputPermanentNumber - 1).getIntValue() != 0;
}

bool WFSValueTreeState::setEffectSendOnFromInput (int channelIndex, int inputPermanentNumber, bool on)
{
    return writeEffectSendCell (channelIndex, effectSendOns, inputPermanentNumber - 1,
                                on ? "1" : "0");
}

float WFSValueTreeState::getEffectFxSendLevelFromEffect (int channelIndex, int sourceEffectIndex) const
{
    const auto cell = readEffectSendCell (channelIndex, effectFxSendLevels, sourceEffectIndex);
    return cell.isEmpty() ? effectFxSendLevelDefault : cell.getFloatValue();
}

bool WFSValueTreeState::setEffectFxSendLevelFromEffect (int channelIndex, int sourceEffectIndex,
                                                        float levelDb)
{
    // REFUSED AT THE DOOR as well as forced in canonicalEffectSendRow. The
    // interceptor would put this cell back whatever happened here, but a setter
    // that reports success for a write it knows cannot take is the shape of bug
    // this family has already been bitten by twice.
    if (channelIndex == sourceEffectIndex)
        return false;

    const float clamped = juce::jlimit (effectFxSendLevelMin, effectFxSendLevelMax, levelDb);
    return writeEffectSendCell (channelIndex, effectFxSendLevels, sourceEffectIndex,
                                juce::String (clamped));
}

bool WFSValueTreeState::getEffectFxSendOnFromEffect (int channelIndex, int sourceEffectIndex) const
{
    return readEffectSendCell (channelIndex, effectFxSendOns, sourceEffectIndex).getIntValue() != 0;
}

bool WFSValueTreeState::setEffectFxSendOnFromEffect (int channelIndex, int sourceEffectIndex, bool on)
{
    if (channelIndex == sourceEffectIndex)
        return false;   // see setEffectFxSendLevelFromEffect

    return writeEffectSendCell (channelIndex, effectFxSendOns, sourceEffectIndex, on ? "1" : "0");
}

void WFSValueTreeState::readEffectSendRows (int channelIndex,
                                            std::array<float, maxInputChannels>& inLevelsDb,
                                            std::array<uint8_t, maxInputChannels>& inOns,
                                            std::array<float, maxEffectChannels>& fxLevelsDb,
                                            std::array<uint8_t, maxEffectChannels>& fxOns) const
{
    inLevelsDb.fill (effectSendLevelDefault);
    inOns.fill (0);
    fxLevelsDb.fill (effectFxSendLevelDefault);
    fxOns.fill (0);

    auto sends = const_cast<WFSValueTreeState*> (this)->getEffectSendsSection (channelIndex);
    if (! sends.isValid())
        return;

    // Canonicalised on the way out, as readEffectSendCell does, so a short or
    // hand-edited row answers for every column it is supposed to have. The self
    // index is the channel's own dense index: this section came from it, so
    // the fx diagonal is forced off before anyone downstream can read it.
    auto unpackLevels = [&] (const juce::Identifier& rowId, float* dest, int width)
    {
        if (! sends.hasProperty (rowId))
            return;

        juce::StringArray tokens;
        tokens.addTokens (canonicalEffectSendRow (rowId, sends.getProperty (rowId), channelIndex), ",", "");
        for (int i = 0; i < width && i < tokens.size(); ++i)
            dest[i] = tokens[i].getFloatValue();
    };

    auto unpackSwitches = [&] (const juce::Identifier& rowId, uint8_t* dest, int width)
    {
        if (! sends.hasProperty (rowId))
            return;

        juce::StringArray tokens;
        tokens.addTokens (canonicalEffectSendRow (rowId, sends.getProperty (rowId), channelIndex), ",", "");
        for (int i = 0; i < width && i < tokens.size(); ++i)
            dest[i] = tokens[i].getIntValue() != 0 ? 1 : 0;
    };

    unpackLevels   (effectSendLevels,   inLevelsDb.data(), maxInputChannels);
    unpackSwitches (effectSendOns,      inOns.data(),      maxInputChannels);
    unpackLevels   (effectFxSendLevels, fxLevelsDb.data(), maxEffectChannels);
    unpackSwitches (effectFxSendOns,    fxOns.data(),      maxEffectChannels);
}

void WFSValueTreeState::zeroEffectSendColumnsForInput (int inputPermanentNumber)
{
    const int column = inputPermanentNumber - 1;
    if (column < 0 || column >= maxInputChannels)
        return;

    const juce::String levelDefault (effectSendLevelDefault);
    const int total = getNumEffectChannels();

    for (int ch = 0; ch < total; ++ch)
    {
        auto sends = getEffectSendsSection (ch);
        if (! sends.isValid())
            continue;

        // Raw setProperty with no UndoManager, like remapClusterInputOrders and
        // the number compaction beside it: a channel delete is a structural edit
        // this family does not make undoable, and the callers clear the
        // histories on the way out.
        //
        // The two INPUT-keyed rows only, and -1 for the self index: a column
        // here is an input number, so this row has no diagonal to force and no
        // channel of its own to resolve.
        for (auto rowId : { effectSendLevels, effectSendOns })
        {
            if (! sends.hasProperty (rowId))
                continue;

            juce::StringArray tokens;
            tokens.addTokens (canonicalEffectSendRow (rowId, sends.getProperty (rowId), -1), ",", "");
            if (column < tokens.size())
                tokens.set (column, rowId == effectSendLevels ? levelDefault : juce::String ("0"));

            sends.setProperty (rowId, tokens.joinIntoString (","), nullptr);
        }
    }
}

void WFSValueTreeState::remapEffectSendColumnsByInputNumber (const std::map<int, int>& oldToNewNumbers)
{
    // Worth testing before anything else: both callers run on every structural
    // edit, whether or not a number actually moved.
    bool anyMoved = false;
    for (const auto& pair : oldToNewNumbers)
        anyMoved = anyMoved || (pair.first != pair.second);
    if (! anyMoved)
        return;

    const juce::String levelDefault (effectSendLevelDefault);
    const int total = getNumEffectChannels();

    for (int ch = 0; ch < total; ++ch)
    {
        auto sends = getEffectSendsSection (ch);
        if (! sends.isValid())
            continue;

        // Input-keyed rows, so -1 for the self index: no diagonal here.
        for (auto rowId : { effectSendLevels, effectSendOns })
        {
            if (! sends.hasProperty (rowId))
                continue;

            juce::StringArray before;
            before.addTokens (canonicalEffectSendRow (rowId, sends.getProperty (rowId), -1), ",", "");
            juce::StringArray after = before;

            const juce::String idle = (rowId == effectSendLevels) ? levelDefault : juce::String ("0");

            // ONE PASS, every read from `before`. Applied incrementally it would
            // read a column it had already overwritten, and a SWAP - which is
            // what a relabel produces and a dense compaction never does - would
            // put both channels' sends on one column and lose the other.
            for (const auto& pair : oldToNewNumbers)
            {
                const int from = pair.first - 1, to = pair.second - 1;
                if (from < 0 || to < 0 || from >= before.size() || to >= after.size())
                    continue;
                after.set (to, before[from]);
            }

            // A column whose owner moved AWAY and that nothing moved INTO is now
            // nobody's, so it goes back to idle. Columns the map never mentions
            // are left exactly as they are: this knows which channels moved and
            // nothing at all about a column no live channel owns today - a
            // number retired by a delete, or one a snapshot restored ahead of
            // the channel it belongs to. Clearing those would be inferring what
            // may be destroyed from what cannot be seen.
            for (const auto& pair : oldToNewNumbers)
            {
                const int from = pair.first - 1;
                if (from < 0 || from >= after.size())
                    continue;

                const bool somethingMovedIn =
                    std::any_of (oldToNewNumbers.begin(), oldToNewNumbers.end(),
                                 [&pair] (const std::pair<const int, int>& other)
                                 { return other.second == pair.first; });
                if (! somethingMovedIn)
                    after.set (from, idle);
            }

            sends.setProperty (rowId, after.joinIntoString (","), nullptr);
        }
    }
}

void WFSValueTreeState::dropEffectFxSendColumn (int removedEffectIndex)
{
    if (removedEffectIndex < 0 || removedEffectIndex >= maxEffectChannels)
        return;

    const juce::String levelDefault (effectFxSendLevelDefault);
    const int total = getNumEffectChannels();

    for (int ch = 0; ch < total; ++ch)
    {
        auto sends = getEffectSendsSection (ch);
        if (! sends.isValid())
            continue;

        for (auto rowId : { effectFxSendLevels, effectFxSendOns })
        {
            if (! sends.hasProperty (rowId))
                continue;

            const juce::String idle = (rowId == effectFxSendLevels) ? levelDefault : juce::String ("0");

            // Canonicalised with NO self index: the diagonal is forced after the
            // shift, at the channel's new index, never before it at the old one.
            juce::StringArray tokens;
            tokens.addTokens (canonicalEffectSendRow (rowId, sends.getProperty (rowId), -1), ",", "");
            if (removedEffectIndex < tokens.size())
            {
                tokens.remove (removedEffectIndex);
                tokens.add (idle);         // the width is fixed; the tail refills with idle
            }

            // ch IS the survivor's new dense index, which is what the diagonal
            // has to be forced at: everything above the hole has just moved down
            // one, this channel among them.
            sends.setProperty (rowId,
                               canonicalEffectSendRow (rowId, tokens.joinIntoString (","), ch),
                               nullptr);
        }
    }
}

juce::ValueTree WFSValueTreeState::getEffectModuleSection (int channelIndex, int slotIndex)
{
    return getEffectModuleSection (channelIndex, getEffectModuleType (slotIndex));
}

juce::ValueTree WFSValueTreeState::getEffectModuleSection (int channelIndex, const juce::Identifier& moduleType)
{
    if (moduleType.isNull())
        return {};
    return getEffectState (channelIndex).getChildWithName (moduleType);
}

juce::ValueTree WFSValueTreeState::getEffectEQSection (int channelIndex, int eqInstance)
{
    if (eqInstance < 0 || eqInstance >= numEffectEqInstances)
        return {};
    return getEffectModuleSection (channelIndex, eqInstance == 0 ? FxEq1 : FxEq2);
}

juce::ValueTree WFSValueTreeState::getEffectEQBand (int channelIndex, int eqInstance, int bandIndex)
{
    // By TYPE - see nthChildOfType. getEffectState counts <Effect> children by
    // type for one reason: the accessor and the count must agree about which
    // channel is the nth one whatever the file turned out to hold. The same file
    // that can leave an unknown node in <Effects> can leave one in <FxEq1>, and
    // this is that hazard one level further down the very same load path.
    return nthChildOfType (getEffectEQSection (channelIndex, eqInstance), Band, bandIndex);
}

juce::ValueTree WFSValueTreeState::getEffectDynSection (int channelIndex, int dynInstance)
{
    if (dynInstance < 0 || dynInstance >= numEffectDynInstances)
        return {};
    return getEffectModuleSection (channelIndex, dynInstance == 0 ? FxDyn1 : FxDyn2);
}

juce::ValueTree WFSValueTreeState::getEffectDelayTap (int channelIndex, int tapIndex)
{
    // By TYPE - see nthChildOfType, and getEffectEQBand just above for why the
    // fixed eight-tap child list is not a licence to index straight into it.
    return nthChildOfType (getEffectModuleSection (channelIndex, FxDelay), Tap, tapIndex);
}

//==============================================================================
// Cluster Access
//==============================================================================

juce::var WFSValueTreeState::getClusterParameter (int clusterIndex, const juce::Identifier& paramId) const
{
    auto cluster = const_cast<WFSValueTreeState*>(this)->getClusterState (clusterIndex);
    if (cluster.isValid() && cluster.hasProperty (paramId))
        return cluster.getProperty (paramId);
    return {};
}

void WFSValueTreeState::setClusterParameter (int clusterIndex, const juce::Identifier& paramId, const juce::var& value)
{
    auto cluster = getClusterState (clusterIndex);
    if (cluster.isValid())
        writeProperty (cluster, paramId, value, getActiveUndoManager());
}

//==============================================================================
// Binaural Enable/Solo Access
//==============================================================================

bool WFSValueTreeState::getBinauralEnabled() const
{
    JUCE_ASSERT_MESSAGE_THREAD  // ValueTree reads are message-thread only; RT threads use BinauralCalculationEngine's RtParams snapshot
    auto binaural = getBinauralState();
    if (binaural.isValid())
        return (bool) binaural.getProperty (binauralEnabled, binauralEnabledDefault);
    return binauralEnabledDefault;
}

void WFSValueTreeState::setBinauralEnabled (bool isEnabled)
{
    auto binaural = getBinauralState();
    if (binaural.isValid())
        binaural.setProperty (binauralEnabled, isEnabled, getActiveUndoManager());
}

int WFSValueTreeState::getBinauralSoloMode() const
{
    JUCE_ASSERT_MESSAGE_THREAD  // see getBinauralEnabled()
    auto binaural = getBinauralState();
    if (binaural.isValid())
        return (int) binaural.getProperty (binauralSoloMode, binauralSoloModeDefault);
    return binauralSoloModeDefault;
}

void WFSValueTreeState::setBinauralSoloMode (int mode)
{
    auto binaural = getBinauralState();
    if (binaural.isValid())
        binaural.setProperty (binauralSoloMode, mode, getActiveUndoManager());
}

// Solo lives ON the channel (Channel.inputSolo) so it travels with the node:
// the legacy Binaural.inputSoloStates csv was positional and silently followed
// the wrong channels across insertions/reorders/deletions.

bool WFSValueTreeState::isInputSoloed (int inputIndex) const
{
    JUCE_ASSERT_MESSAGE_THREAD  // ValueTree read — message thread only
    auto input = const_cast<WFSValueTreeState*> (this)->getInputState (inputIndex);
    auto channel = input.getChildWithName (Channel);
    return channel.isValid() && static_cast<int> (channel.getProperty (inputSolo, 0)) != 0;
}

void WFSValueTreeState::setInputSoloed (int inputIndex, bool soloed)
{
    auto input = getInputState (inputIndex);
    auto channel = input.getChildWithName (Channel);
    if (! channel.isValid())
        return;

    // In Single mode, clear all other solos first
    if (soloed && getBinauralSoloMode() == 0)
        clearAllSoloStates();

    channel.setProperty (inputSolo, soloed ? 1 : 0, getActiveUndoManager());
}

void WFSValueTreeState::clearAllSoloStates()
{
    auto inputs = getInputsState();
    for (int i = 0; i < inputs.getNumChildren(); ++i)
    {
        auto channel = inputs.getChild (i).getChildWithName (Channel);
        if (channel.isValid()
            && static_cast<int> (channel.getProperty (inputSolo, 0)) != 0)
            channel.setProperty (inputSolo, 0, getActiveUndoManager());
    }
}

int WFSValueTreeState::getNumSoloedInputs() const
{
    JUCE_ASSERT_MESSAGE_THREAD  // ValueTree read — message thread only
    int soloCount = 0;
    for (int i = 0; i < getNumInputChannels(); ++i)
        if (isInputSoloed (i))
            ++soloCount;
    return soloCount;
}

int WFSValueTreeState::getBinauralOutputChannel() const
{
    JUCE_ASSERT_MESSAGE_THREAD  // audio callback uses BinauralCalculationEngine::getBinauralOutputChannel() (atomic) instead
    auto binaural = getBinauralState();
    if (binaural.isValid())
        return (int) binaural.getProperty (binauralOutputChannel, binauralOutputChannelDefault);
    return binauralOutputChannelDefault;
}

void WFSValueTreeState::setBinauralOutputChannel (int channel)
{
    auto binaural = getBinauralState();
    if (binaural.isValid())
        binaural.setProperty (binauralOutputChannel, channel, getActiveUndoManager());
}

//==============================================================================
// Network Target Access
//==============================================================================

int WFSValueTreeState::getNumNetworkTargets() const
{
    auto network = const_cast<WFSValueTreeState*>(this)->getNetworkState();
    return network.getNumChildren();
}

void WFSValueTreeState::addNetworkTarget()
{
    auto network = getNetworkState();
    if (network.getNumChildren() < maxNetworkTargets)
    {
        auto target = createDefaultNetworkTarget (network.getNumChildren());
        network.appendChild (target, getActiveUndoManager());
    }
}

void WFSValueTreeState::removeNetworkTarget (int targetIndex)
{
    auto network = getNetworkState();
    if (targetIndex >= 0 && targetIndex < network.getNumChildren())
        network.removeChild (targetIndex, getActiveUndoManager());
}

juce::ValueTree WFSValueTreeState::getNetworkTargetState (int targetIndex)
{
    auto network = getNetworkState();
    if (targetIndex >= 0 && targetIndex < network.getNumChildren())
        return network.getChild (targetIndex);
    return {};
}

//==============================================================================
// Channel Management
//==============================================================================

int WFSValueTreeState::getNumInputChannels() const
{
    return const_cast<WFSValueTreeState*>(this)->getInputsState().getNumChildren();
}

int WFSValueTreeState::getNumStereoInputChannels() const
{
    // Derived from the per-channel type — the legacy IO property is read only
    // by migration (and removed from the tree afterwards).
    auto inputs = getInputsState();
    int stereo = 0;
    for (int i = 0; i < inputs.getNumChildren(); ++i)
        if (isInputChannelStereo (i))
            ++stereo;
    return stereo;
}

int WFSValueTreeState::getInputChannelNumber (int slot) const
{
    auto inputs = getInputsState();
    if (slot < 0 || slot >= inputs.getNumChildren())
        return 0;
    return static_cast<int> (inputs.getChild (slot).getProperty (id, 0));
}

int WFSValueTreeState::getSlotForChannelNumber (int number) const
{
    auto inputs = getInputsState();
    const int n = inputs.getNumChildren();
    if (number <= 0 || n == 0)
        return -1;

    // Fast path: dense, un-reordered list — slot is number - 1.
    if (number <= n
        && static_cast<int> (inputs.getChild (number - 1).getProperty (id, 0)) == number)
        return number - 1;

    // Tree order is the user's DISPLAY order (drag-to-reorder moves nodes),
    // so numbers carry no ordering guarantee — linear scan (n <= 64; runs on
    // the OSC/tracking ingress path, still trivially cheap).
    for (int i = 0; i < n; ++i)
        if (static_cast<int> (inputs.getChild (i).getProperty (id, 0)) == number)
            return i;
    return -1;
}

bool WFSValueTreeState::isInputChannelStereo (int slot) const
{
    auto inputs = getInputsState();
    if (slot < 0 || slot >= inputs.getNumChildren())
        return false;
    return inputs.getChild (slot).getProperty (inputChannelType).toString()
             == inputChannelTypeStereo;
}

int WFSValueTreeState::getHighestChannelNumber() const
{
    // Tree order is display order (drag-to-reorder), so scan for the max.
    auto inputs = getInputsState();
    int highest = 0;
    for (int i = 0; i < inputs.getNumChildren(); ++i)
        highest = juce::jmax (highest,
                              static_cast<int> (inputs.getChild (i).getProperty (id, 0)));
    return highest;
}

int WFSValueTreeState::getNextChannelNumber() const
{
    return getHighestChannelNumber() + 1;
}

void WFSValueTreeState::stampChannelTypesFromLegacySplit (juce::UndoManager* um, int stereoCountOverride)
{
    // Migration-only: stamps the pre-rework tail split ("the LAST N channels
    // are stereo") onto the per-channel property. The caller reads N from the
    // legacy IO property — getNumStereoInputChannels() is derived from the
    // types and cannot be used here.
    jassert (stereoCountOverride >= 0);

    auto inputs = getInputsState();
    const int total  = inputs.getNumChildren();
    const int stereo = juce::jmin (juce::jmax (0, stereoCountOverride), total);

    for (int i = 0; i < total; ++i)
    {
        auto input = inputs.getChild (i);
        const juce::String wanted = (i >= total - stereo) ? inputChannelTypeStereo
                                                          : inputChannelTypeMono;
        if (input.getProperty (inputChannelType).toString() != wanted)
            input.setProperty (inputChannelType, wanted, um);
    }
}

void WFSValueTreeState::migrateInputChannelModel()
{
    auto inputs = getInputsState();
    if (! inputs.isValid())
        return;

    const int total = inputs.getNumChildren();
    if (total == 0)
        return;

    // 1. Repair ids: every channel must carry a unique positive number.
    //    Well-formed files are dense 1..N in order; a hand-edited file with
    //    missing or duplicate ids gets one dense renumber here — the last
    //    renumbering that can ever happen to it.
    {
        juce::SortedSet<int> seen;
        bool idsValid = true;
        for (int i = 0; i < total && idsValid; ++i)
        {
            const int number = static_cast<int> (inputs.getChild (i).getProperty (id, 0));
            if (number <= 0 || seen.contains (number))
                idsValid = false;
            else
                seen.add (number);
        }
        // NO SEND REMAP HERE, and not by omission. The four <Sends> rows are
        // keyed by input NUMBER, so this renumber leaves a file's rows pointing
        // at the numbers it had before - but the map that would fix them cannot
        // be built: the repair fires exactly when ids are DUPLICATED or MISSING,
        // which is when "the channel that was number 3" names two channels or
        // none. Effects also load after inputs (loadCompleteConfig orders
        // system, network, inputs, outputs, reverbs, effects), so the rows are
        // not in the tree yet at this point. Reachable only with an
        // already-corrupt inputs.xml whose numbers are being rewritten out from
        // under every other reference to them as well.
        if (! idsValid)
            for (int i = 0; i < total; ++i)
                inputs.getChild (i).setProperty (id, i + 1, nullptr);
    }

    // (Tree order is preserved as-is: it is the user's saved display order —
    // drag-to-reorder moves nodes, numbers stay put.)

    // 2. Stamp types for files that predate the per-channel property — but
    //    only when the WHOLE list lacks it: a partially-typed list is
    //    post-rework data whose missing entries default to mono.
    bool anyTyped = false;
    for (int i = 0; i < total && ! anyTyped; ++i)
        anyTyped = inputs.getChild (i).hasProperty (inputChannelType);

    auto io = getIOState();
    if (! anyTyped)
    {
        const int legacyStereo = io.isValid()
            ? juce::jlimit (0, WFSParameterDefaults::maxStereoChannels,
                            (int) io.getProperty (WFSParameterIDs::stereoInputChannels, 0))
            : 0;
        stampChannelTypesFromLegacySplit (nullptr, legacyStereo);
    }
    else
    {
        for (int i = 0; i < total; ++i)
        {
            auto input = inputs.getChild (i);
            if (! input.hasProperty (inputChannelType))
                input.setProperty (inputChannelType, inputChannelTypeMono, nullptr);
        }
    }

    // 3. The legacy tail-split property is consumed; drop it so nothing can
    //    resurrect the count-based semantics from a stale file value.
    if (io.isValid() && io.hasProperty (WFSParameterIDs::stereoInputChannels))
        io.removeProperty (WFSParameterIDs::stereoInputChannels, nullptr);

    // 4. Solo: the legacy Binaural.inputSoloStates csv is positional (index =
    //    slot); convert to the per-channel Channel.inputSolo property so the
    //    state travels with the node, then drop the csv.
    auto binaural = getBinauralState();
    if (binaural.isValid() && binaural.hasProperty (WFSParameterIDs::inputSoloStates))
    {
        juce::StringArray states;
        states.addTokens (binaural.getProperty (WFSParameterIDs::inputSoloStates, "").toString(), ",", "");
        for (int i = 0; i < juce::jmin (states.size(), total); ++i)
        {
            auto channel = inputs.getChild (i).getChildWithName (Channel);
            if (channel.isValid() && states[i] == "1")
                channel.setProperty (inputSolo, 1, nullptr);
        }
        binaural.removeProperty (WFSParameterIDs::inputSoloStates, nullptr);
    }
}

void WFSValueTreeState::compactChannelNumbersToDisplayOrder()
{
    auto inputs = getInputsState();
    if (! inputs.isValid())
        return;

    // Ascending, id written BEFORE the tracking id: mid-walk the list can
    // briefly hold the same number twice (slot s takes s + 1 while the node that
    // still owns s + 1 has not been visited yet). Harmless —
    // getSlotForChannelNumber's fast path resolves a NEW number to the node just
    // renumbered, so the synchronous valueTreePropertyChanged fired by the
    // tracking-id write below still reports the right slot.
    const int total = inputs.getNumChildren();

    // THE PERMUTATION, WHOLE, BEFORE THE WALK. Anything keyed by the permanent
    // number has to move with it, and the send rows are - but mid-walk this list
    // is not a valid state to remap against: the comment above records that it
    // can briefly hold the same number twice. Remapped one write at a time, the
    // second channel to take a number would read a column the first had already
    // overwritten and the two channels' sends would collapse onto one. So the
    // map is taken here, applied once below, and read only from the row as it
    // was before any of it moved.
    // Identity entries are included deliberately - remapEffectSendColumnsByInputNumber
    // reads the map as the whole permutation, and a column is only cleared when
    // its number is a source and nothing's destination. A list already holding a
    // number twice (which migrateInputChannelModel repairs on load, and nothing
    // this application writes produces) would collapse to one entry here and one
    // of the two channels would lose its column - the numbers are being rewritten
    // out from under it in that case anyway.
    std::map<int, int> oldToNew;
    for (int slot = 0; slot < total; ++slot)
    {
        const int oldNumber = getInputChannelNumber (slot);
        if (oldNumber > 0)
            oldToNew[oldNumber] = slot + 1;
    }

    for (int slot = 0; slot < total; ++slot)
    {
        if (getInputChannelNumber (slot) != slot + 1)
            setInputChannelNumberAtSlot (slot, slot + 1);
    }

    remapEffectSendColumnsByInputNumber (oldToNew);
}

void WFSValueTreeState::setInputChannelNumberAtSlot (int slot, int newNumber)
{
    auto input = getInputsState().getChild (slot);
    if (! input.isValid())
        return;
    const int oldNumber = getInputChannelNumber (slot);
    if (oldNumber == newNumber)
        return;

    // Direct setProperty throughout, never setParameter/setInputParam: a
    // renumber is bookkeeping, and those wrappers carry undo entries,
    // dirty-tracking and ownership latches that must not fire for it.
    input.setProperty (id, newNumber, nullptr);

    // The tracking id, which createDefaultInputChannel stamps from the
    // number, follows it only while it still matches — a tracker mapping the
    // user has pointed elsewhere stays put. (The name is NOT the number's to
    // move: resequenceDefaultInputNames owns it.)
    auto position = input.getChildWithName (Position);
    if (position.isValid()
        && static_cast<int> (position.getProperty (inputTrackingID, 0)) == oldNumber)
        position.setProperty (inputTrackingID, newNumber, nullptr);
}

namespace
{
    // 1-based ordinal a default input name of the given shape carries ("Mono 3"
    // against "Mono" -> 3), or 0 when the name is not that shape. "Mono 0" and
    // "Mono 007" read as 0 and 7: the ordinal is what the name says, and only a
    // positive one is a name the app could have stamped.
    int defaultInputNameOrdinal (const juce::String& candidate, juce::StringRef word)
    {
        const int space = candidate.lastIndexOfChar (' ');
        if (space <= 0 || candidate.substring (0, space) != word)
            return 0;

        const juce::String tail = candidate.substring (space + 1);
        if (tail.isEmpty() || ! tail.containsOnly ("0123456789"))
            return 0;
        return tail.getIntValue();
    }
}

void WFSValueTreeState::resequenceDefaultInputNames()
{
    auto inputs = getInputsState();
    if (! inputs.isValid())
        return;

    // Any n, not the one this channel happens to carry now: by the time this
    // runs the ordinals have already shifted under the names, so testing
    // against the single name a channel *would* have had would freeze every
    // default the reorder displaced.
    auto isDefaultName = [] (const juce::String& candidate) -> bool
    {
        return defaultInputNameOrdinal (candidate, "Input") > 0
            || defaultInputNameOrdinal (candidate, "Mono") > 0
            || defaultInputNameOrdinal (candidate, "Stereo") > 0;
    };

    int monoCount = 0, stereoCount = 0;
    const int total = inputs.getNumChildren();
    for (int slot = 0; slot < total; ++slot)
    {
        const bool stereo = isInputChannelStereo (slot);
        const int ordinal = stereo ? ++stereoCount : ++monoCount;

        auto channel = inputs.getChild (slot).getChildWithName (Channel);
        if (! channel.isValid())
            continue;

        const juce::String current = channel.getProperty (inputName).toString();
        if (! isDefaultName (current))
            continue;

        // Direct setProperty, never setInputParam: the app renaming its own
        // defaults must not push an undo entry, mark the project dirty or trip
        // an ownership latch.
        const juce::String renamed = getDefaultInputNameForType (stereo, ordinal);
        if (current != renamed)
            channel.setProperty (inputName, renamed, nullptr);
    }
}

void WFSValueTreeState::setInputChannelCounts (int numMono, int numStereo)
{
    // Two-count entry point (System Config fields, load shim). Under the
    // stable-number model it is a thin loop over the structural ops: additions
    // APPEND after the last channel — numbers never shift and mono and stereo
    // channels may interleave — and reductions remove the LAST channel of that
    // type in DISPLAY order. Not "the highest-numbered": the lambda walks slots
    // descending, and on a latched list that has been dragged those are
    // different channels. The bottom of the Arrange list is what the operator
    // can see, so it is the rule; predictInputChannelReduction mirrors it and
    // the dialog shows exactly that. Not undoable (the ops clear the undo
    // history; a half-undone tree/patch pair would silently desync).
    clampInputChannelCounts (numMono, numStereo);

    auto removeLastOfTypeInDisplayOrder = [this] (bool stereo) -> bool
    {
        auto inputs = getInputsState();
        for (int i = inputs.getNumChildren(); --i >= 0;)
            if (isInputChannelStereo (i) == stereo)
                return removeInputChannel (getInputChannelNumber (i)).wasOk();
        return false;
    };

    // Reductions first so the 64-live budget is free before additions.
    while (getNumStereoInputChannels() > numStereo)
        if (! removeLastOfTypeInDisplayOrder (true)) break;
    while (getNumInputChannels() - getNumStereoInputChannels() > numMono)
        if (! removeLastOfTypeInDisplayOrder (false)) break;
    while (getNumStereoInputChannels() < numStereo)
        if (! addInputChannel (true).wasOk()) break;
    while (getNumInputChannels() - getNumStereoInputChannels() < numMono)
        if (! addInputChannel (false).wasOk()) break;
}

int WFSValueTreeState::getLowestFreeChannelNumber() const
{
    for (int n = 1; n <= WFSParameterDefaults::maxInputChannels; ++n)
        if (getSlotForChannelNumber (n) < 0)
            return n;
    return 0;
}

juce::Result WFSValueTreeState::addInputChannel (bool stereo, int explicitNumber)
{
    auto inputs = getInputsState();
    const int total = inputs.getNumChildren();

    if (total >= WFSParameterDefaults::maxInputChannels)
        return juce::Result::fail ("input list is full ("
                                   + juce::String (WFSParameterDefaults::maxInputChannels)
                                   + " live channels)");
    if (stereo && getNumStereoInputChannels() >= WFSParameterDefaults::maxStereoChannels)
        return juce::Result::fail ("stereo budget reached ("
                                   + juce::String (WFSParameterDefaults::maxStereoChannels)
                                   + " stereo channels)");

    int number = explicitNumber;
    if (number <= 0)
    {
        // Append-only: the next number is highest + 1. Once 64 has been used,
        // the caller must explicitly pick a free (retired) number — the UI
        // confirms with the user first, because snapshots/cues addressed to
        // that number will affect the new channel. (An unlatched session never
        // reaches exhaustion with gaps: the tail renumber keeps the list dense.)
        number = getNextChannelNumber();
        if (number > WFSParameterDefaults::maxInputChannels)
            return juce::Result::fail ("channel number space exhausted; reuse a free number (lowest free: "
                                       + juce::String (getLowestFreeChannelNumber()) + ")");
    }
    else
    {
        if (number > WFSParameterDefaults::maxInputChannels)
            return juce::Result::fail ("channel number above "
                                       + juce::String (WFSParameterDefaults::maxInputChannels));
        if (getSlotForChannelNumber (number) >= 0)
            return juce::Result::fail ("channel number " + juce::String (number) + " is already live");
    }

    // New channels always land at the END of the display order (append-only);
    // the user drags them into place afterwards. This holds for gap-reuse
    // creation too — the recycled NUMBER does not dictate a position.
    // (Unlatched, the tail renumber then gives it the number of that display
    // position.)
    const int slot = total;

    auto node = createDefaultInputChannel (slot, total + 1, number);
    node.setProperty (inputChannelType,
                      stereo ? juce::String (inputChannelTypeStereo)
                             : juce::String (inputChannelTypeMono), nullptr);

    // The default name can only be built once the real type is known:
    // createDefaultInputChannel stamps mono. The ordinal is the highest one any
    // live name of that shape already claims, never the count of channels of
    // that type: a latched session never resequences, so after a delete — or
    // after a setInputChannelType flip, which renames nothing — count + 1 would
    // hand out a name a live channel still carries, permanently. On a dense list
    // of untouched defaults the two are the same value. Shape, not type, drives
    // the scan: a flipped channel keeps the name of the type it was born as.
    const juce::StringRef word = stereo ? "Stereo" : "Mono";
    int ordinal = stereo ? getNumStereoInputChannels()
                         : total - getNumStereoInputChannels();
    for (int i = 0; i < total; ++i)
    {
        auto channel = inputs.getChild (i).getChildWithName (Channel);
        if (channel.isValid())
            ordinal = juce::jmax (ordinal,
                                  defaultInputNameOrdinal (channel.getProperty (inputName).toString(),
                                                           word));
    }

    auto newChannel = node.getChildWithName (Channel);
    if (newChannel.isValid())
        newChannel.setProperty (inputName,
                                getDefaultInputNameForType (stereo, ordinal + 1), nullptr);

    inputs.addChild (node, slot, nullptr);

    insertInputPatchRow (slot, stereo);

    auto io = getIOState();
    if (io.isValid())
        io.setProperty (inputChannels, inputs.getNumChildren(), nullptr);
    inputs.setProperty (count, inputs.getNumChildren(), nullptr);

    if (! arePositionsUserOwned())
        redistributeAllInputPositions();

    // Fresh session: nothing outside the app can reference these numbers yet,
    // so the appended channel must read as its display position, not as
    // highest + 1 past an earlier gap. The patch has to read the same way — a
    // gapless diagonal in DISPLAY order, which the diagonal-continue row
    // inserted above cannot give: it appends past the globally highest patched
    // column, i.e. in creation order.
    //
    // The ORDER of the two calls does not matter; do not "fix" it later. The
    // patch re-flow is slot-keyed and reads only tree SHAPE
    // (getNumInputChannels, isInputChannelStereo), writing only patchData/rows;
    // the number compaction writes only `id` and `inputTrackingID` on <Input>
    // nodes and moves no child. They commute.
    if (! areChannelNumbersUserOwned())
    {
        compactChannelNumbersToDisplayOrder();
        compactInputPatchToDisplayOrder();
    }

    // Structural edits are not undoable: the channel node and its patch row
    // must live and die together, and ValueTree undo cannot span the flat
    // patchData string edit safely.
    clearAllUndoHistories();
    return juce::Result::ok();
}

juce::Result WFSValueTreeState::removeInputChannel (int channelNumber)
{
    const int slot = getSlotForChannelNumber (channelNumber);
    if (slot < 0)
        return juce::Result::fail ("channel " + juce::String (channelNumber) + " is not live");

    auto inputs = getInputsState();
    if (inputs.getNumChildren() <= 1)
        return juce::Result::fail ("at least one input channel is required");

    inputs.removeChild (slot, nullptr);
    removeInputPatchRow (slot);

    // Slot-keyed side state: drop the deleted slot, shift the ones above
    remapClusterInputOrders ([slot] (int s)
    {
        return s == slot ? -1 : (s > slot ? s - 1 : s);
    });

    // NUMBER-keyed side state, and it has to happen HERE - after the channel is
    // gone, before the compaction below. This number has just been retired and
    // the gap it leaves can be handed back out by addInputChannel later, so a
    // column left holding the dead channel's sends is a routing an operator
    // never made, arriving on a channel they have just created. Zeroing it first
    // also means the compaction's permutation shifts a row with nothing dead
    // left in it; zeroing afterwards would clear whichever channel had moved
    // into that number.
    zeroEffectSendColumnsForInput (channelNumber);

    auto io = getIOState();
    if (io.isValid())
        io.setProperty (inputChannels, inputs.getNumChildren(), nullptr);
    inputs.setProperty (count, inputs.getNumChildren(), nullptr);

    if (! arePositionsUserOwned())
        redistributeAllInputPositions();

    // Fresh session: close the gap the removal just opened, so the list keeps
    // reading 1..N. The columns the deleted row held are handed back out by the
    // re-flow, so the diagonal closes up instead of leaving a hole no later
    // channel can ever reach.
    if (! areChannelNumbersUserOwned())
    {
        compactChannelNumbersToDisplayOrder();
        compactInputPatchToDisplayOrder();
    }

    clearAllUndoHistories();
    return juce::Result::ok();
}

juce::Result WFSValueTreeState::setInputChannelType (int channelNumber, bool stereo)
{
    const int slot = getSlotForChannelNumber (channelNumber);
    if (slot < 0)
        return juce::Result::fail ("channel " + juce::String (channelNumber) + " is not live");
    if (isInputChannelStereo (slot) == stereo)
        return juce::Result::ok();
    if (stereo && getNumStereoInputChannels() >= WFSParameterDefaults::maxStereoChannels)
        return juce::Result::fail ("stereo budget reached ("
                                   + juce::String (WFSParameterDefaults::maxStereoChannels)
                                   + " stereo channels)");

    // Latched, the patch row keeps its columns here and the caller's
    // reconfiguration pass (sanitizeMonoPatchRows / autoPatchStereoRightColumns)
    // drops the R column on stereo→mono and auto-assigns a free R on
    // mono→stereo. Unlatched, the re-flow below replaces both.
    getInputsState().getChild (slot).setProperty (
        inputChannelType,
        stereo ? juce::String (inputChannelTypeStereo)
               : juce::String (inputChannelTypeMono), nullptr);

    // PATCH only, deliberately NOT the numbers — the asymmetry with the other
    // three structural ops is the point, not an oversight. A type flip moves no
    // channel's display position, so compactChannelNumbersToDisplayOrder() here
    // would be a provable no-op. The patch is NOT invariant under it: the row's
    // capacity changes by one column, which shifts every column after it.
    // Today's substitute is the caller's autoPatchStereoRightColumns, a
    // heuristic that refuses when leftCol + 1 is already claimed — and under
    // strict packing the next column is ALWAYS claimed by the following row, so
    // mono→stereo would essentially never get its R. The "already this type"
    // early return above sits before this, so a no-op flip does not re-flow.
    if (! areChannelNumbersUserOwned())
        compactInputPatchToDisplayOrder();

    clearAllUndoHistories();
    return juce::Result::ok();
}

juce::ValueTree WFSValueTreeState::buildInputChannelInventory() const
{
    // Display order, because that is the half of the model `inputChannels`
    // cannot express: the sum says 22, it does not say that slots 0 and 21 are
    // the stereo pairs. Patch rows are positional, so losing this ordering
    // silently re-patches the show.
    juce::ValueTree inventory (InputChannelList);

    const int total = getNumInputChannels();
    for (int slot = 0; slot < total; ++slot)
    {
        const int number = getInputChannelNumber (slot);
        if (number <= 0)
            continue;   // migrateInputChannelModel repairs these; never write one out

        juce::ValueTree ch (Ch);
        ch.setProperty (chNumber, number, nullptr);
        ch.setProperty (chType, isInputChannelStereo (slot)
                                    ? juce::String (inputChannelTypeStereo)
                                    : juce::String (inputChannelTypeMono), nullptr);
        inventory.appendChild (ch, nullptr);
    }

    return inventory;
}

void WFSValueTreeState::applyInputChannelInventory (const juce::ValueTree& inventory)
{
    if (! inventory.isValid())
        return;

    // Parse first, act second: a malformed entry must not leave the list
    // half-reconciled. First occurrence of a number wins — a file listing one
    // twice is corrupt, and picking one reading beats creating a duplicate.
    struct Entry { int number; bool stereo; };
    std::vector<Entry> wanted;
    for (int i = 0; i < inventory.getNumChildren(); ++i)
    {
        auto ch = inventory.getChild (i);
        if (! ch.hasType (Ch))
            continue;

        const int number = static_cast<int> (ch.getProperty (chNumber, 0));
        if (number <= 0 || number > WFSParameterDefaults::maxInputChannels)
            continue;

        const bool duplicate = std::any_of (wanted.begin(), wanted.end(),
                                            [number] (const Entry& e) { return e.number == number; });
        if (duplicate)
        {
            juce::Logger::writeToLog ("Channel inventory lists channel " + juce::String (number)
                                      + " twice; keeping the first");
            continue;
        }

        wanted.push_back ({ number,
                            ch.getProperty (chType).toString() == inputChannelTypeStereo });
    }

    if (wanted.empty())
        return;   // nothing usable — the caller's sum fallback is the better answer

    auto isWanted = [&wanted] (int number)
    {
        return std::any_of (wanted.begin(), wanted.end(),
                            [number] (const Entry& e) { return e.number == number; });
    };

    // Extras first, so both the 64-channel and the 8-stereo budgets are free
    // before anything is created — the same ordering setInputChannelCounts uses.
    // removeInputChannel refuses to empty the list, so a wholesale replacement
    // needs a second pass after the additions have raised the count; that is
    // what the `again` sweep below is for, not a retry loop.
    auto removeExtras = [this, &isWanted]
    {
        auto inputs = getInputsState();
        for (int slot = inputs.getNumChildren(); --slot >= 0;)
        {
            const int number = getInputChannelNumber (slot);
            if (number > 0 && ! isWanted (number))
                removeInputChannel (number);   // may legitimately refuse on the last channel
        }
    };
    removeExtras();

    // Create what the file lists and the tree lacks, with the recorded type, so
    // no channel is ever born mono and flipped afterwards (a flip would have to
    // find a free patch column, which strict packing rarely leaves).
    for (const auto& e : wanted)
    {
        if (getSlotForChannelNumber (e.number) >= 0)
            continue;

        const auto r = addInputChannel (e.stereo, e.number);
        if (r.failed())
            juce::Logger::writeToLog ("Channel inventory: could not create channel "
                                      + juce::String (e.number) + " - " + r.getErrorMessage());
    }

    removeExtras();   // the channel the "at least one" floor protected, if any

    // Type corrections for channels that already existed. Stereo->mono first:
    // it frees stereo budget that a mono->stereo correction in the same pass
    // may need, and the reverse order would fail on a full-budget swap.
    for (int pass = 0; pass < 2; ++pass)
    {
        const bool toStereo = (pass == 1);
        for (const auto& e : wanted)
        {
            if (e.stereo != toStereo)
                continue;

            const int slot = getSlotForChannelNumber (e.number);
            if (slot < 0 || isInputChannelStereo (slot) == e.stereo)
                continue;

            const auto r = setInputChannelType (e.number, e.stereo);
            if (r.failed())
                juce::Logger::writeToLog ("Channel inventory: could not set channel "
                                          + juce::String (e.number) + " to "
                                          + (e.stereo ? "stereo" : "mono")
                                          + " - " + r.getErrorMessage());
        }
    }

    // Display order last. Moved directly rather than through moveInputChannel:
    // that one drags the patch row with it, which is right for a user drag but
    // wrong here — the rows are about to be overwritten wholesale by the file's
    // own patchData, and moving them first would shuffle rows that are already
    // in the file's order. Resolve the source slot against the live tree on
    // every iteration, since earlier moves shift the ones after them.
    auto inputs = getInputsState();
    int target = 0;
    for (const auto& e : wanted)
    {
        const int from = getSlotForChannelNumber (e.number);
        if (from < 0)
            continue;   // creation failed above; already logged

        if (from != target)
            inputs.moveChild (from, target, nullptr);
        ++target;
    }

    clearAllUndoHistories();
}

void WFSValueTreeState::remapClusterInputOrders (const std::function<int (int)>& oldToNew)
{
    // clusterInputOrder is a csv of 0-based SLOT indices in memory; structural
    // edits (delete/reorder) shift slots, so every cluster's order must be
    // remapped in the same operation or the ordering silently migrates to the
    // wrong channels.
    //
    // The shape — split, map each token, drop on negative, rejoin, write only if
    // changed — is also exactly what the FILE boundary needs, so the two
    // converters below reuse it with slot<->number lambdas. Hence the parameter
    // is named for the transform, not for slots: it is not always slot->slot.
    auto clusters = getClustersState();
    for (int c = 0; c < clusters.getNumChildren(); ++c)
    {
        auto cluster = clusters.getChild (c);
        const juce::String order = cluster.getProperty (clusterInputOrder, "").toString();
        if (order.isEmpty())
            continue;

        juce::StringArray tokens;
        tokens.addTokens (order, ",", "");
        juce::StringArray remapped;
        for (const auto& tok : tokens)
        {
            const int newSlot = oldToNew (tok.trim().getIntValue());
            if (newSlot >= 0)
                remapped.add (juce::String (newSlot));
        }

        const juce::String newOrder = remapped.joinIntoString (",");
        if (newOrder != order)
            cluster.setProperty (clusterInputOrder, newOrder, nullptr);
    }
}

void WFSValueTreeState::convertClusterOrdersSlotsToNumbers()
{
    // Save direction. NOTE the guard: getInputChannelNumber returns 0 — not -1 —
    // for an out-of-range slot, and remapClusterInputOrders keeps any token >= 0,
    // so returning it raw would write a bogus "0" into the file. 0 is never a
    // valid channel number, and it is exactly the token a reader might mistake
    // for a legacy slot, so it must never be emitted.
    remapClusterInputOrders ([this] (int slot)
    {
        const int number = getInputChannelNumber (slot);
        return number > 0 ? number : -1;
    });
}

void WFSValueTreeState::convertClusterOrdersNumbersToSlots()
{
    // Load direction. getSlotForChannelNumber returns -1 for a number with no
    // live channel, which remapClusterInputOrders drops — the same
    // drop-what-no-longer-exists semantics deserializeExtendedScope uses for
    // snapshot scope entries.
    remapClusterInputOrders ([this] (int number)
    {
        return getSlotForChannelNumber (number);
    });
}

void WFSValueTreeState::moveInputChannelNodeAndRow (int fromSlot, int toSlot)
{
    auto inputs = getInputsState();
    inputs.moveChild (fromSlot, toSlot, nullptr);
    moveInputPatchRow (fromSlot, toSlot);

    // Slot-keyed side state follows the move
    const int from = fromSlot, to = toSlot;
    remapClusterInputOrders ([from, to] (int s)
    {
        if (s == from) return to;
        if (from < to) return (s > from && s <= to) ? s - 1 : s;
        return (s >= to && s < from) ? s + 1 : s;
    });
}

juce::Result WFSValueTreeState::moveInputChannel (int channelNumber, int targetSlot)
{
    auto inputs = getInputsState();
    const int fromSlot = getSlotForChannelNumber (channelNumber);
    if (fromSlot < 0)
        return juce::Result::fail ("channel " + juce::String (channelNumber) + " is not live");

    targetSlot = juce::jlimit (0, inputs.getNumChildren() - 1, targetSlot);
    if (targetSlot == fromSlot)
        return juce::Result::ok();

    // Drag-to-reorder: the channel node and its patch row move TOGETHER, so
    // the patch matrix, map, picker and engine slots all follow the new
    // display order while the permanent number (and every external
    // reference) stays put once the session is latched; before that the
    // renumber below makes the numbers follow the display order.
    moveInputChannelNodeAndRow (fromSlot, targetSlot);

    // Fresh session: the dragged channel takes the number of its new display
    // position — the whole point of the unlatched regime. moveInputPatchRow
    // above carries the columns WITH the row, which is the latched behaviour and
    // is exactly what decouples the patch from display order; unlatched, the
    // re-flow overwrites it.
    if (! areChannelNumbersUserOwned())
    {
        compactChannelNumbersToDisplayOrder();
        compactInputPatchToDisplayOrder();
    }

    clearAllUndoHistories();
    return juce::Result::ok();
}

void WFSValueTreeState::clampInputChannelCounts (int& numMono, int& numStereo)
{
    numStereo = juce::jlimit (0, WFSParameterDefaults::maxStereoChannels, numStereo);
    numMono   = juce::jlimit (1, WFSParameterDefaults::maxInputChannels - numStereo, numMono);
}

std::vector<InputChannelRef> WFSValueTreeState::predictInputChannelReduction (int numMono, int numStereo) const
{
    clampInputChannelCounts (numMono, numStereo);

    const int total = getNumInputChannels();
    int stereoLeft  = getNumStereoInputChannels();
    int monoLeft    = total - stereoLeft;

    // Same walk, same order, as setInputChannelCounts: stereo victims first,
    // each type from the bottom of the display order up. Everything about a
    // victim is captured HERE, before any removal, because an unlatched session
    // renumbers the survivors after each one.
    std::vector<InputChannelRef> victims;
    auto capture = [this] (int slot)
    {
        InputChannelRef ref;
        ref.slot     = slot;
        ref.number   = getInputChannelNumber (slot);
        ref.stereo   = isInputChannelStereo (slot);
        ref.hwInputs = getInputPatchHardwareInputs (slot);
        auto channel = getInputsState().getChild (slot).getChildWithName (Channel);
        if (channel.isValid())
            ref.name = channel.getProperty (inputName).toString();
        return ref;
    };

    for (int slot = total; --slot >= 0 && stereoLeft > numStereo;)
        if (isInputChannelStereo (slot)) { victims.push_back (capture (slot)); --stereoLeft; }
    for (int slot = total; --slot >= 0 && monoLeft > numMono;)
        if (! isInputChannelStereo (slot)) { victims.push_back (capture (slot)); --monoLeft; }

    return victims;
}

std::vector<int> WFSValueTreeState::getInputPatchHardwareInputs (int slot) const
{
    std::vector<int> out;
    // getAudioPatchState has no const overload; this only reads. Same precedent
    // as WFSFileManager's const_cast around getInputState.
    auto patch = const_cast<WFSValueTreeState*> (this)->getAudioPatchState().getChildWithName (InputPatch);
    if (! patch.isValid() || slot < 0)
        return out;

    juce::StringArray rowStrings = juce::StringArray::fromTokens (patch.getProperty (patchData).toString(), ";", "");
    if (slot >= rowStrings.size())
        return out;

    juce::StringArray colStrings = juce::StringArray::fromTokens (rowStrings[slot], ",", "");
    for (int c = 0; c < colStrings.size(); ++c)
        if (colStrings[c].getIntValue() == 1)
            out.push_back (c + 1);   // 1-based, as the operator sees it
    return out;
}

InputChannelIdentity WFSValueTreeState::getInputChannelIdentity() const
{
    InputChannelIdentity live;
    auto inputs = getInputsState();
    const int total = inputs.getNumChildren();
    for (int slot = 0; slot < total; ++slot)
    {
        InputChannelRef ref;
        ref.slot     = slot;
        ref.number   = getInputChannelNumber (slot);
        ref.stereo   = isInputChannelStereo (slot);
        ref.hwInputs = getInputPatchHardwareInputs (slot);
        auto channel = inputs.getChild (slot).getChildWithName (Channel);
        if (channel.isValid())
            ref.name = channel.getProperty (inputName).toString();
        live.slots.push_back (std::move (ref));
    }
    if (! live.slots.empty())
    {
        live.source     = InputChannelIdentity::Source::liveTree;
        live.typesKnown = true;
        live.orderKnown = true;
        live.hwKnown    = true;
    }
    return live;
}

juce::Result WFSValueTreeState::assignInputChannelNumbersBySlot (const std::vector<int>& numbersBySlot,
                                                                 const juce::String& reason)
{
    const int total = getNumInputChannels();
    if ((int) numbersBySlot.size() != total)
        return juce::Result::fail ("relabel: " + juce::String ((int) numbersBySlot.size())
                                   + " numbers for " + juce::String (total) + " channels");

    // Everything validated before the first write, so a bad list changes nothing.
    std::vector<int> seen;
    for (int n : numbersBySlot)
    {
        if (n <= 0 || n > WFSParameterDefaults::maxInputChannels)
            return juce::Result::fail ("relabel: channel number " + juce::String (n) + " is out of range");
        if (std::find (seen.begin(), seen.end(), n) != seen.end())
            return juce::Result::fail ("relabel: channel number " + juce::String (n) + " appears twice");
        seen.push_back (n);
    }

    // The whole permutation first, for the reason compactChannelNumbersToDisplayOrder
    // spells out: this walk also writes slot by slot, so mid-walk the list holds
    // duplicates, and an incremental remap of anything keyed by the number would
    // read a column it had already written. This path makes that worse than the
    // compaction does - a relabel may SWAP two numbers, which a dense compaction
    // never produces, and a swap is exactly the case an incremental remap loses
    // a whole channel's sends to.
    std::map<int, int> oldToNew;
    for (int slot = 0; slot < total; ++slot)
    {
        const int oldNumber = getInputChannelNumber (slot);
        if (oldNumber > 0)
            oldToNew[oldNumber] = numbersBySlot[(size_t) slot];
    }

    juce::StringArray changes;
    for (int slot = 0; slot < total; ++slot)
    {
        const int oldNumber = getInputChannelNumber (slot);
        const int newNumber = numbersBySlot[(size_t) slot];
        if (oldNumber != newNumber)
        {
            changes.add ("#" + juce::String (oldNumber) + "->#" + juce::String (newNumber));
            setInputChannelNumberAtSlot (slot, newNumber);
        }
    }

    // The relabel exists so that snapshots, cues and OSC written against the
    // file's numbers still reach the right channel afterwards. A send row keyed
    // by number is one of those references.
    remapEffectSendColumnsByInputNumber (oldToNew);

    // The numbers are now an external contract, whatever they were before.
    markChannelNumbersUserOwned (reason);
    clearAllUndoHistories();

    WFSLogger::getInstance().logInfo ("Channel numbers relabelled (" + reason + "): "
                                      + (changes.isEmpty() ? juce::String ("no change")
                                                           : changes.joinIntoString (", ")));
    return juce::Result::ok();
}

juce::Result WFSValueTreeState::reorderInputChannelsToNumbers (const std::vector<int>& numbersInDisplayOrder,
                                                               const juce::String& reason)
{
    // FIRST, before any move: unlatched, moveInputChannel's tail recompacts the
    // numbers to display order, and the targets would drift under the loop.
    markChannelNumbersUserOwned (reason);

    int target = 0;
    for (int number : numbersInDisplayOrder)
    {
        const int from = getSlotForChannelNumber (number);
        if (from < 0)
            continue;
        if (from != target)
        {
            const auto r = moveInputChannel (number, target);
            if (r.failed())
                return r;
        }
        ++target;
    }

    WFSLogger::getInstance().logInfo ("Channel order rearranged (" + reason + ")");
    return juce::Result::ok();
}

void WFSValueTreeState::moveInputPatchRow (int fromSlot, int toSlot)
{
    auto patch = getAudioPatchState().getChildWithName (InputPatch);
    if (! patch.isValid())
        return;

    juce::StringArray rowsArr = juce::StringArray::fromTokens (
        patch.getProperty (patchData).toString(), ";", "");
    if (fromSlot < 0 || fromSlot >= rowsArr.size()
        || toSlot < 0 || toSlot >= rowsArr.size())
        return;

    rowsArr.move (fromSlot, toSlot);   // same semantics as ValueTree::moveChild
    patch.setProperty (patchData, rowsArr.joinIntoString (";"), nullptr);
}

void WFSValueTreeState::insertInputPatchRow (int slot, bool stereo)
{
    auto patch = getAudioPatchState().getChildWithName (InputPatch);
    if (! patch.isValid())
        return;

    juce::StringArray rowsArr = juce::StringArray::fromTokens (
        patch.getProperty (patchData).toString(), ";", "");

    // Diagonal-continue: the new row takes the next hardware column(s) past
    // everything already patched (two consecutive for stereo, lower = L).
    int cursor = 0;
    for (int r = 0; r < rowsArr.size(); ++r)
    {
        juce::StringArray rowCols = juce::StringArray::fromTokens (rowsArr[r], ",", "");
        for (int c = rowCols.size(); --c >= 0;)
        {
            if (rowCols[c].getIntValue() == 1)
            {
                cursor = juce::jmax (cursor, c + 1);
                break;
            }
        }
    }

    const int capacity = stereo ? 2 : 1;
    const int lastCol  = juce::jmin (maxHardwarePatchChannels - 1, cursor + capacity - 1);
    const int hwCols   = static_cast<int> (patch.getProperty (cols, 64));
    const int rowLen   = juce::jmax (hwCols, lastCol + 1);

    juce::StringArray rowCols;
    for (int c = 0; c < rowLen; ++c)
        rowCols.add (c >= cursor && c <= lastCol ? "1" : "0");

    rowsArr.insert (slot, rowCols.joinIntoString (","));

    patch.setProperty (patchData, rowsArr.joinIntoString (";"), nullptr);
    patch.setProperty (rows, rowsArr.size(), nullptr);
    recomputePatchCols();
}

void WFSValueTreeState::removeInputPatchRow (int slot)
{
    auto patch = getAudioPatchState().getChildWithName (InputPatch);
    if (! patch.isValid())
        return;

    juce::StringArray rowsArr = juce::StringArray::fromTokens (
        patch.getProperty (patchData).toString(), ";", "");
    if (slot < 0 || slot >= rowsArr.size())
        return;

    rowsArr.remove (slot);
    patch.setProperty (patchData, rowsArr.joinIntoString (";"), nullptr);
    patch.setProperty (rows, rowsArr.size(), nullptr);
    recomputePatchCols();
}

void WFSValueTreeState::normalizeInputPatchRows()
{
    // Config load rewrites patchData wholesale, so the stored rows may not
    // match the channel list: truncate extras, append diagonal-continue rows
    // (capacity from the channel's type) for the missing tail. Idempotent.
    auto patch = getAudioPatchState().getChildWithName (InputPatch);
    if (! patch.isValid())
        return;

    const int total = getNumInputChannels();
    juce::StringArray rowsArr = juce::StringArray::fromTokens (
        patch.getProperty (patchData).toString(), ";", "");

    if (rowsArr.size() > total)
    {
        rowsArr.removeRange (total, rowsArr.size() - total);
        patch.setProperty (patchData, rowsArr.joinIntoString (";"), nullptr);
        patch.setProperty (rows, rowsArr.size(), nullptr);
        recomputePatchCols();
    }

    while (rowsArr.size() < total)
    {
        insertInputPatchRow (rowsArr.size(), isInputChannelStereo (rowsArr.size()));
        rowsArr = juce::StringArray::fromTokens (
            patch.getProperty (patchData).toString(), ";", "");
    }
}

void WFSValueTreeState::compactInputPatchToDisplayOrder()
{
    auto patch = getAudioPatchState().getChildWithName (InputPatch);
    if (! patch.isValid())
        return;

    const int total = getNumInputChannels();
    if (total <= 0)
        return;

    // Rebuilt from the channel list, discarding the stored rows wholesale. That
    // is safe on a call-graph property, not on anything in this code: while the
    // numbers are unlatched, no operator click and no wire message has ever
    // reached patchData. The only interactive writer is
    // PatchMatrixComponent::savePatchesToValueTree, reachable only through
    // MainComponent::openAudioInterfaceWindow(), which calls
    // markChannelNumbersUserOwned() BEFORE it constructs the window; every load
    // path latches on success; MCP lists patchData under ignored_parameters and
    // OSC has no patch address. WARNING: anything future that lets a patch be
    // authored must latch first, or this will eat it.
    //
    // Rebuilding is also what makes the re-flow self-repairing: it drops a stale
    // row count and the ragged row lengths insertInputPatchRow leaves behind (it
    // sizes only the row it inserts), and it makes the result a pure function of
    // the channel list — hence idempotent by construction.
    //
    // Strict packing: consecutive columns, no gaps, and NO alignment to the
    // interface's odd/even input pairs, so N mono + M stereo always fit in
    // N + 2M hardware inputs. A stereo pair may therefore legitimately start on
    // hardware input 11.
    const int demand = total + getNumStereoInputChannels();
    const int hwCols = static_cast<int> (patch.getProperty (cols, 64));
    const int rowLen = juce::jmax (hwCols, juce::jmin (maxHardwarePatchChannels, demand));

    juce::StringArray rowsArr;
    int cursor = 0;
    for (int slot = 0; slot < total; ++slot)
    {
        const int capacity = isInputChannelStereo (slot) ? 2 : 1;
        const int first    = cursor;

        // The clamp mirrors insertInputPatchRow's literally. At the exact
        // boundary a stereo row would come out with a single column — 64 live
        // channels with at most 8 stereo demand 72 of 512 columns, so that is
        // structurally unreachable. It is a guard, not a policy: nobody should
        // read a rule out of it and "unify" the two functions on its strength.
        const int last = juce::jmin (maxHardwarePatchChannels - 1, cursor + capacity - 1);
        cursor += capacity;

        juce::StringArray rowCols;
        for (int c = 0; c < rowLen; ++c)
            rowCols.add (c >= first && c <= last ? "1" : "0");

        rowsArr.add (rowCols.joinIntoString (","));
    }

    // Direct setProperty throughout, never setParameter: re-flowing the app's
    // own default patch must not push an undo entry, mark the project dirty or
    // trip an ownership latch. Same rule compactChannelNumbersToDisplayOrder and
    // resequenceDefaultInputNames follow.
    patch.setProperty (patchData, rowsArr.joinIntoString (";"), nullptr);
    patch.setProperty (rows, rowsArr.size(), nullptr);
    recomputePatchCols();
}

int WFSValueTreeState::getNumOutputChannels() const
{
    return const_cast<WFSValueTreeState*>(this)->getOutputsState().getNumChildren();
}

int WFSValueTreeState::getNumReverbChannels() const
{
    // Count actual `Reverb`-typed children only; the Reverbs subtree also
    // hosts global siblings (ReverbAlgorithm, ReverbPreComp, ReverbPostEQ,
    // ReverbPostExp) that must NOT count toward channel total. Reading
    // the reverbChannels property directly used to do this job, but the
    // property could drift from the actual children when a writer
    // bypassed setNumReverbChannels — which then made
    // session_get_channel_full(reverb, ...) refuse valid IDs because the
    // child lookup ran out of Reverb-typed siblings before the ID range.
    auto reverbs = const_cast<WFSValueTreeState*>(this)->getReverbsState();
    int reverbCount = 0;
    for (int i = 0; i < reverbs.getNumChildren(); ++i)
        if (reverbs.getChild (i).hasType (Reverb))
            ++reverbCount;
    return reverbCount;
}

int WFSValueTreeState::getNumEffectChannels() const
{
    // Counted, never read off `count`: the property is bookkeeping and a writer
    // that bypasses setNumEffectChannels makes it lie, which is exactly the
    // drift getNumReverbChannels documents. The type test is NOT a guard on the
    // container's invariant, whatever this comment used to say: nothing this
    // application writes puts a non-<Effect> node in <Effects>, but a merged FILE
    // can, and getEffectState resolves the nth channel by this same walk so the
    // two cannot disagree about which channel that is.
    auto effects = getEffectsState();
    int effectCount = 0;
    for (int i = 0; i < effects.getNumChildren(); ++i)
        if (effects.getChild (i).hasType (Effect))
            ++effectCount;
    return effectCount;
}

void WFSValueTreeState::setNumInputChannels (int numChannels)
{
    // Blunt count entry point (config-load sync, OSC/MCP inputChannels
    // writes). Under the stable-number model: growth APPENDS default mono
    // channels after the last channel (numbers never shift), reduction
    // removes the HIGHEST-NUMBERED channels — nothing in the middle ever
    // moves. Patch rows are mirrored by the ops. Not undoable (the ops clear
    // the undo history).
    numChannels = juce::jlimit (1, maxInputChannels, numChannels);
    auto inputs = getInputsState();

    while (inputs.getNumChildren() > numChannels)
        if (! removeInputChannel (getHighestChannelNumber()).wasOk()) break;
    while (inputs.getNumChildren() < numChannels)
        if (! addInputChannel (false).wasOk()) break;

    // Ensure GradientMaps and Sampler sections exist for all inputs (migration for old configs)
    for (int i = 0; i < inputs.getNumChildren(); ++i)
    {
        ensureInputGradientMapsSection (i);
        ensureInputSamplerSection (i);
    }

    // Keep the count properties honest even when no structural change was
    // needed (the ops already stamp them on every add/remove). Direct writes,
    // NOT via setParameter — its routing would recurse into us.
    auto io = getIOState();
    if (io.isValid())
        io.setProperty (inputChannels, inputs.getNumChildren(), nullptr);
    inputs.setProperty (count, inputs.getNumChildren(), nullptr);
}

void WFSValueTreeState::setNumOutputChannels (int numChannels, int previousCount)
{
    numChannels = juce::jlimit (1, maxOutputChannels, numChannels);
    auto outputs = getOutputsState();
    int currentCount = outputs.getNumChildren();
    if (previousCount < 0)
        previousCount = currentCount;

    beginUndoTransaction ("Set Output Channel Count");

    if (numChannels > currentCount)
    {
        // Add new channels
        for (int i = currentCount; i < numChannels; ++i)
            outputs.appendChild (createDefaultOutputChannel (i), getActiveUndoManager());
    }
    else if (numChannels < currentCount)
    {
        // Remove excess channels
        while (outputs.getNumChildren() > numChannels)
            outputs.removeChild (outputs.getNumChildren() - 1, getActiveUndoManager());
    }

    // Update the count property directly (NOT via setParameter) so the
    // setParameter -> setNumOutputChannels routing in setParameter doesn't
    // recurse into us.
    {
        auto io = getIOState();
        if (io.isValid())
            io.setProperty (outputChannels, numChannels, getActiveUndoManager());
    }
    outputs.setProperty (count, numChannels, getActiveUndoManager());

    // Update input mute arrays. The old mute grid wrote every one of its
    // buttons, hidden ones included: 128 entries, 64 before the output cap was
    // raised. On a smaller rig, the entries of such a list past the outputs
    // that existed were never a choice anyone made, so outputs this resize adds
    // start unmuted. Any other length is kept whole: a list longer than the
    // live count can carry real mutes, brought back by a standalone input
    // reload or a snapshot recall before the count caught up. (A real 64- or
    // 128-entry list reloaded that way onto fewer outputs is the one case this
    // cannot tell apart; its added outputs start unmuted too, and it is the one
    // case in which a row is still CUT BACK to the live count.)
    auto inputs = getInputsState();
    for (int i = 0; i < inputs.getNumChildren(); ++i)
    {
        auto mutesTree = getInputMutesSection (i);
        if (! mutesTree.isValid())
            continue;

        const auto list = mutesTree.getProperty (inputMutes);
        juce::StringArray tokens;
        tokens.addTokens (list.toString(), ",", "");
        const bool oldGridList = tokens.size() > previousCount
                                 && (tokens.size() == maxOutputChannels || tokens.size() == 64);

        // NEVER NARROWER THAN WHAT IS STORED, outside that legacy case. Fitting
        // the row to the live count in BOTH directions means a rig that shrinks
        // deletes the mutes of the outputs it dropped and pads "0" back in their
        // place when the rig returns - an interface that disappears and comes
        // back, a System Config edit undone - with no error and nothing in the
        // log. The columns past the live count are not stale: they are the
        // operator's mutes for outputs that are not there today.
        mutesTree.setProperty (inputMutes,
                               normaliseMuteList (list,
                                                  oldGridList ? numChannels
                                                              : perOutputRowWidth (list, numChannels),
                                                  oldGridList ? previousCount : std::numeric_limits<int>::max()),
                               getActiveUndoManager());
    }

    // THE OTHER TWO FAMILIES THAT CARRY THE SAME ROW, and until now neither was
    // refitted here. A reverb's row kept whatever width it was created at, and
    // self-healed only because ReverbTab rewrites it whole whenever the operator
    // opens that tab - with a hard-coded fallback of 16, which is what hid the
    // gap for so long. An effect's row would have inherited exactly that. Both
    // are per-OUTPUT rows, so the live output count is their width, which is the
    // one thing that makes them unlike the four <Sends> rows: those are keyed by
    // input number and by effect index, neither of which has anything to do with
    // how many outputs the rig has, so nothing here touches them.
    //
    // No legacy keepTokens window, deliberately. That exists for inputMutes
    // because an old grid wrote 64 or 128 entries whatever the rig was; nothing
    // has ever written a reverb or effect row at any width but the live count.
    //
    // AND THE FIT ONLY EVER GROWS, which is the half a width-shaped reading of
    // this misses. PADDING is what these two rows were missing. TRIMMING is what
    // would make adding them here destructive: reverbMutes has shipped for years
    // and a temporary drop to fewer outputs has always been survivable precisely
    // because nothing refitted it. normaliseMuteList pads AND cuts, so the width
    // it is handed has to be the one that cannot lose a column.
    auto refitPerOutputRow = [this, numChannels] (juce::ValueTree returnSection,
                                                  const juce::Identifier& rowId)
    {
        if (! returnSection.isValid() || ! returnSection.hasProperty (rowId))
            return;   // never INVENT a row on a node that has none

        const auto stored = returnSection.getProperty (rowId);
        returnSection.setProperty (rowId,
                                   normaliseMuteList (stored, perOutputRowWidth (stored, numChannels)),
                                   getActiveUndoManager());
    };

    for (int i = 0; i < getNumReverbChannels(); ++i)
        refitPerOutputRow (getReverbReturnSection (i), reverbMutes);

    for (int i = 0; i < getNumEffectChannels(); ++i)
        refitPerOutputRow (getEffectReturnSection (i), effectMutes);
}

void WFSValueTreeState::setNumReverbChannels (int numChannels)
{
    numChannels = juce::jlimit (0, maxReverbChannels, numChannels);
    auto reverbs = getReverbsState();

    // Create Reverbs section if it doesn't exist
    if (!reverbs.isValid())
    {
        createReverbsSection();
        reverbs = getReverbsState();
    }

    // Count only Reverb channel children (not ReverbAlgorithm or other global sections)
    int currentCount = 0;
    for (int i = 0; i < reverbs.getNumChildren(); ++i)
        if (reverbs.getChild (i).hasType (Reverb))
            ++currentCount;
    const int originalCount = currentCount;   // the removal loop below mutates currentCount

    beginUndoTransaction ("Set Reverb Channel Count");

    if (numChannels > currentCount)
    {
        // Add new channels
        for (int i = currentCount; i < numChannels; ++i)
            reverbs.appendChild (createDefaultReverbChannel (i, numChannels), getActiveUndoManager());
    }
    else if (numChannels < currentCount)
    {
        // Remove excess Reverb channels (not global sections like ReverbAlgorithm)
        for (int i = reverbs.getNumChildren() - 1; i >= 0 && currentCount > numChannels; --i)
        {
            if (reverbs.getChild (i).hasType (Reverb))
            {
                reverbs.removeChild (i, getActiveUndoManager());
                --currentCount;
            }
        }
    }

    // Ensure all existing reverb channels have EQ sections (handles old configs without EQ)
    for (int i = 0; i < numChannels; ++i)
        ensureReverbEQSection (i);

    // Ensure global algorithm section exists (handles old configs)
    ensureReverbAlgorithmSection();

    // Ensure global pre-compressor section exists (handles old configs)
    ensureReverbPreCompSection();

    // Ensure global post-processing EQ section exists (handles old configs)
    ensureReverbPostEQSection();

    // Ensure global post-expander section exists (handles old configs)
    ensureReverbPostExpSection();

    // Update the count property directly (NOT via setParameter) so the
    // setParameter -> setNumReverbChannels routing in setParameter doesn't
    // recurse into us.
    {
        auto io = getIOState();
        if (io.isValid())
            io.setProperty (reverbChannels, numChannels, getActiveUndoManager());
    }
    reverbs.setProperty (count, numChannels, getActiveUndoManager());

    // The arc layout depends on the total node count, so channels created under
    // an earlier count sit on the wrong arc — re-lay the whole set. Gated on
    // ownership: once the user has positioned things, the new nodes land on the
    // default arc and everything else stays put (same rule as the input grid).
    if (originalCount != numChannels && ! arePositionsUserOwned())
        redistributeAllReverbPositions();
}

void WFSValueTreeState::setNumEffectChannels (int numChannels)
{
    // Floor of ZERO, like reverbs and unlike inputs/outputs: a show that uses no
    // effects keeps an empty container, and that is the default.
    numChannels = juce::jlimit (0, maxEffectChannels, numChannels);

    auto effects = getEffectsState();
    if (! effects.isValid())
    {
        createEffectsSection();
        effects = getEffectsState();
        if (! effects.isValid())
            return;
    }

    int currentCount = getNumEffectChannels();
    const int originalCount = currentCount;

    // Structural edits are not undoable (see addEffectChannel): every write here
    // passes nullptr, and the histories are cleared at the end if anything moved.
    if (numChannels > currentCount)
    {
        // The TARGET count is passed so each new channel is laid on the ring the
        // finished set will use, not on the ring that existed while it was born.
        for (int i = currentCount; i < numChannels; ++i)
            effects.appendChild (createDefaultEffectChannel (i, numChannels), nullptr);
    }
    else if (numChannels < currentCount)
    {
        for (int i = effects.getNumChildren() - 1; i >= 0 && currentCount > numChannels; --i)
        {
            if (effects.getChild (i).hasType (Effect))
            {
                effects.removeChild (i, nullptr);
                --currentCount;
            }
        }

        // The fx send columns of the channels that just went. They are ABOVE
        // every survivor, so nothing shifts and each drop simply refills the
        // tail with idle - but leaving them would mean a channel re-created at
        // that index inherits the sends of the one that used to be there, which
        // is the same defect the input delete closes on its own key. Same call
        // as removeEffectChannel uses, so there is one rule and not two.
        for (int dead = originalCount - 1; dead >= numChannels; --dead)
            dropEffectFxSendColumn (dead);
    }

    // Written DIRECTLY, never through setParameter: setParameter routes
    // effectChannels straight back into this function.
    if (auto io = getIOState(); io.isValid())
        io.setProperty (effectChannels, numChannels, nullptr);
    effects.setProperty (count, numChannels, nullptr);

    if (originalCount != numChannels)
    {
        // The ring depends on the total, so channels created under an earlier
        // count sit on the wrong one - re-lay the whole set. Gated on the
        // EFFECTS latch: the shared positionsUserOwned flag is already true in
        // any session where the operator has touched the map, and using it here
        // would stack every effect channel ever created on the origin.
        if (! areEffectPositionsUserOwned())
            redistributeAllEffectPositions();

        // Guarded on a real move, and deliberately HERE rather than nowhere.
        // That this is the setter a remote surface reaches (setParameter routes
        // effectChannels straight into it) argues FOR the clear, not against it:
        // a write of inputChannels over that same route already empties the
        // stack today, through addInputChannel / removeInputChannel. The two
        // count setters that clear nothing - setNumOutputChannels,
        // setNumReverbChannels - make their structural writes undoable instead;
        // this family's pass nullptr, so the only alternative to clearing is an
        // undo stack whose entries replay onto renumbered nodes.
        clearAllUndoHistories();
    }
}

juce::Result WFSValueTreeState::addEffectChannel()
{
    auto effects = getEffectsState();
    if (! effects.isValid())
    {
        createEffectsSection();
        effects = getEffectsState();
        if (! effects.isValid())
            return juce::Result::fail ("effects section is missing");
    }

    const int total = getNumEffectChannels();
    if (total >= maxEffectChannels)
        return juce::Result::fail ("effect list is full ("
                                   + juce::String (maxEffectChannels) + " channels)");

    effects.appendChild (createDefaultEffectChannel (total, total + 1), nullptr);

    if (auto io = getIOState(); io.isValid())
        io.setProperty (effectChannels, total + 1, nullptr);
    effects.setProperty (count, total + 1, nullptr);

    if (! areEffectPositionsUserOwned())
        redistributeAllEffectPositions();

    // Structural edits are not undoable, the rule every family here follows:
    // a channel subtree that Ctrl+Z can half-restore is worse than no undo.
    clearAllUndoHistories();
    return juce::Result::ok();
}

juce::Result WFSValueTreeState::removeEffectChannel (int channelIndex)
{
    auto effects = getEffectsState();
    const int total = getNumEffectChannels();
    if (channelIndex < 0 || channelIndex >= total)
        return juce::Result::fail ("effect channel " + juce::String (channelIndex + 1)
                                   + " is not live");

    // By TYPE, like getEffectState and getNumEffectChannels: channelIndex is the
    // nth <Effect>, which is only the nth child while nothing unrecognised sits
    // in the list - and a merged file can put something there.
    auto victim = getEffectState (channelIndex);
    if (! victim.hasType (Effect))
        return juce::Result::fail ("effect channel " + juce::String (channelIndex + 1)
                                   + " is not an effect node");

    effects.removeChild (effects.indexOf (victim), nullptr);

    // Effect ids are DENSE, so the hole closes: everything above moves down one.
    // No number is retired and none is ever reused, because an effect return is
    // addressed by its place in the list rather than by a permanent number.
    // Only the <Effect> children are numbered - stamping an id onto a foreign
    // sibling would invent a channel that is not one, and the merge matches
    // children by type AND id, so the invention would stick to the file.
    int renumbered = 0;
    for (int i = 0; i < effects.getNumChildren(); ++i)
        if (auto child = effects.getChild (i); child.hasType (Effect))
            child.setProperty (id, ++renumbered, nullptr);

    const int remaining = renumbered;   // by type, never getNumChildren()
    if (auto io = getIOState(); io.isValid())
        io.setProperty (effectChannels, remaining, nullptr);
    effects.setProperty (count, remaining, nullptr);

    // THE FX SEND COLUMNS FOLLOW THE IDS. They are keyed by dense index, so the
    // hole that just closed in the channel list has to close in every survivor's
    // row too: leave the columns where they are and every send above the deleted
    // channel silently re-points one channel down. The input-keyed rows are NOT
    // touched here - an input's permanent number means nothing to an effect
    // delete - which is the whole reason the two conventions are named apart.
    // AFTER the renumber loop: the call forces each survivor's diagonal at its
    // NEW index, and the new index is what that loop has just established.
    dropEffectFxSendColumn (channelIndex);

    if (! areEffectPositionsUserOwned())
        redistributeAllEffectPositions();

    clearAllUndoHistories();
    return juce::Result::ok();
}

bool WFSValueTreeState::areEffectPositionsUserOwned() const
{
    auto effects = getEffectsState();
    return effects.isValid()
        && (bool) effects.getProperty (effectPositionsUserOwned, false);
}

void WFSValueTreeState::markEffectPositionsUserOwned()
{
    auto effects = getEffectsState();
    if (effects.isValid() && ! (bool) effects.getProperty (effectPositionsUserOwned, false))
        effects.setProperty (effectPositionsUserOwned, true, nullptr);   // no undo: see header
}

void WFSValueTreeState::redistributeAllEffectPositions()
{
    const int n = getNumEffectChannels();
    if (n == 0)
        return;

    const auto nodes = layoutEffectNodes (n);

    for (int i = 0; i < n && i < (int) nodes.size(); ++i)
    {
        const auto& node = nodes[(size_t) i];

        // Raw setProperty deliberately: a re-layout is the app placing the
        // returns, not the operator, so it must not trip the ownership latch
        // that setEffectParameter carries. Same reason
        // redistributeAllReverbPositions bypasses setReverbParameter.
        if (auto pos = getEffectPositionSection (i); pos.isValid())
        {
            pos.setProperty (effectPositionX, node.x, nullptr);
            pos.setProperty (effectPositionY, node.y, nullptr);
            pos.setProperty (effectPositionZ, node.z, nullptr);
        }

        // The bearing is part of the placement: a node moved without it keeps
        // facing wherever it used to, and the angular attenuation can then mute
        // the feed outright.
        if (auto feed = getEffectFeedSection (i); feed.isValid())
            feed.setProperty (effectOrientation, node.orientationDeg, nullptr);
    }
}

std::vector<ReverbNodePlacement::Node> WFSValueTreeState::layoutEffectNodes (int totalCount)
{
    const auto stage = getStageForPlacement();
    auto nodes = ReverbNodePlacement::layout (stage, juce::jmax (1, totalCount));

    // Positions are origin-relative and the origin is not the stage centre; the
    // helper centres its arc on (-originW, -originD), so the push outwards has
    // to use the same centre or it would drag the ring off the stage instead of
    // widening it.
    const float centreX = -stage.originW;
    const float centreY = -stage.originD;

    for (auto& node : nodes)
    {
        node.x = centreX + (node.x - centreX) * kEffectRingExpansion;
        node.y = centreY + (node.y - centreY) * kEffectRingExpansion;

        // Recomputed after the push, not carried over: the bearing means "faces
        // away from the world origin", and moving the node changes it.
        node.orientationDeg = ReverbNodePlacement::orientationAwayFromOrigin (node.x, node.y);
    }

    return nodes;
}

namespace
{
    int computeHighestPatchedHardwareChannel (const juce::String& patchDataStr)
    {
        int highest = -1;
        auto rowTokens = juce::StringArray::fromTokens (patchDataStr, ";", "");
        for (auto& rowStr : rowTokens)
        {
            auto colTokens = juce::StringArray::fromTokens (rowStr, ",", "");
            for (int c = 0; c < colTokens.size(); ++c)
                if (colTokens[c].getIntValue() == 1 && c > highest)
                    highest = c;
        }
        return highest;
    }

    void applyColsPolicy (juce::ValueTree& patchTree, int deviceChannels)
    {
        if (! patchTree.isValid())
            return;

        constexpr int minCols = 64;
        constexpr int maxCols = WFSValueTreeState::maxHardwarePatchChannels;

        auto patchDataStr = patchTree.getProperty (WFSParameterIDs::patchData).toString();
        int highestPatched = computeHighestPatchedHardwareChannel (patchDataStr);

        int target = juce::jmax (minCols, deviceChannels, highestPatched + 1);
        int newCols = juce::jlimit (minCols, maxCols, target);

        int currentCols = patchTree.getProperty (WFSParameterIDs::cols, minCols);
        if (newCols != currentCols)
            patchTree.setProperty (WFSParameterIDs::cols, newCols, nullptr);
    }
}

void WFSValueTreeState::updateHardwareChannelCount (int hwInputs, int hwOutputs)
{
    auto audioPatch = state.getChildWithName (AudioPatch);
    if (! audioPatch.isValid())
        return;

    auto inputPatchTree = audioPatch.getChildWithName (InputPatch);
    if (inputPatchTree.isValid())
    {
        inputPatchTree.setProperty (WFSParameterIDs::activeHardwareInputs, hwInputs, nullptr);
        applyColsPolicy (inputPatchTree, hwInputs);
    }

    auto outputPatchTree = audioPatch.getChildWithName (OutputPatch);
    if (outputPatchTree.isValid())
    {
        outputPatchTree.setProperty (WFSParameterIDs::activeHardwareOutputs, hwOutputs, nullptr);
        applyColsPolicy (outputPatchTree, hwOutputs);
    }
}

void WFSValueTreeState::recomputePatchCols()
{
    auto audioPatch = state.getChildWithName (AudioPatch);
    if (! audioPatch.isValid())
        return;

    auto inputPatchTree = audioPatch.getChildWithName (InputPatch);
    if (inputPatchTree.isValid())
    {
        int stored = inputPatchTree.getProperty (WFSParameterIDs::activeHardwareInputs, 0);
        applyColsPolicy (inputPatchTree, stored);
    }

    auto outputPatchTree = audioPatch.getChildWithName (OutputPatch);
    if (outputPatchTree.isValid())
    {
        int stored = outputPatchTree.getProperty (WFSParameterIDs::activeHardwareOutputs, 0);
        applyColsPolicy (outputPatchTree, stored);
    }
}

//==============================================================================
// Undo / Redo + Listener Management — moved to TreeParameterStore.
// (Per-domain UndoManager array, MCP-origin undo suppression, parameter
// listener registry and ValueTree listener add/remove all live in the core;
// the UndoDomain-typed wrappers are inline in the header.)
//==============================================================================

//==============================================================================
// State Management
//==============================================================================

void WFSValueTreeState::resetToDefaults()
{
    state.removeListener (this);
    initializeDefaultState();
    state.addListener (this);
    clearAllUndoHistories();
}

void WFSValueTreeState::resetInputToDefaults (int channelIndex)
{
    auto input = getInputState (channelIndex);
    if (input.isValid())
    {
        beginUndoTransaction ("Reset Input " + juce::String (channelIndex + 1));
        auto newInput = createDefaultInputChannel (channelIndex);
        input.copyPropertiesAndChildrenFrom (newInput, getActiveUndoManager());
    }
}

void WFSValueTreeState::resetOutputToDefaults (int channelIndex)
{
    auto output = getOutputState (channelIndex);
    if (output.isValid())
    {
        beginUndoTransaction ("Reset Output " + juce::String (channelIndex + 1));
        auto newOutput = createDefaultOutputChannel (channelIndex);
        output.copyPropertiesAndChildrenFrom (newOutput, getActiveUndoManager());
    }
}

void WFSValueTreeState::resetReverbToDefaults (int channelIndex)
{
    auto reverb = getReverbState (channelIndex);
    if (reverb.isValid())
    {
        beginUndoTransaction ("Reset Reverb " + juce::String (channelIndex + 1));
        auto newReverb = createDefaultReverbChannel (channelIndex, getNumReverbChannels());
        reverb.copyPropertiesAndChildrenFrom (newReverb, getActiveUndoManager());
    }
}

void WFSValueTreeState::redistributeAllInputPositions()
{
    auto inputs = getInputsState();
    int numInputs = inputs.getNumChildren();
    if (numInputs == 0) return;

    auto stageTree = getStageState();
    float sw = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (stageWidth))  : stageWidthDefault;
    float sd = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (stageDepth))  : stageDepthDefault;
    float sh = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (stageHeight)) : stageHeightDefault;
    float ow = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (originWidth)) : originWidthDefault;
    float od = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (originDepth)) : originDepthDefault;
    float oh = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (originHeight)) : originHeightDefault;

    beginUndoTransaction ("Redistribute Input Positions");

    for (int i = 0; i < numInputs; ++i)
    {
        auto input = inputs.getChild (i);
        auto pos = input.getChildWithName (Position);
        if (! pos.isValid()) continue;

        float x, y, z;
        getDefaultInputPosition (i, numInputs, sw, sd, sh, ow, od, oh, x, y, z);
        pos.setProperty (inputPositionX, x, getActiveUndoManager());
        pos.setProperty (inputPositionY, y, getActiveUndoManager());
        pos.setProperty (inputPositionZ, z, getActiveUndoManager());
    }
}

void WFSValueTreeState::scaleAllInputPositions (float oldW, float oldD, float oldH,
                                                 float oldOW, float oldOD, float oldOH)
{
    auto inputs = getInputsState();
    int numInputs = inputs.getNumChildren();
    if (numInputs == 0) return;

    auto stageTree = getStageState();
    float newW  = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (stageWidth))  : stageWidthDefault;
    float newD  = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (stageDepth))  : stageDepthDefault;
    float newH  = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (stageHeight)) : stageHeightDefault;
    float newOW = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (originWidth)) : originWidthDefault;
    float newOD = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (originDepth)) : originDepthDefault;
    float newOH = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (originHeight)) : originHeightDefault;

    // Old and new stage bounds (origin-relative)
    float oldMinX = -oldW / 2.0f - oldOW;
    float oldMinY = -oldD / 2.0f - oldOD;
    float oldMinZ = -oldOH;
    float newMinX = -newW / 2.0f - newOW;
    float newMinY = -newD / 2.0f - newOD;
    float newMinZ = -newOH;

    beginUndoTransaction ("Scale Input Positions");

    for (int i = 0; i < numInputs; ++i)
    {
        auto input = inputs.getChild (i);
        auto pos = input.getChildWithName (Position);
        if (! pos.isValid()) continue;

        float x = static_cast<float> (pos.getProperty (inputPositionX));
        float y = static_cast<float> (pos.getProperty (inputPositionY));
        float z = static_cast<float> (pos.getProperty (inputPositionZ));

        // Map from old bounds to normalized [0..1], then to new bounds
        if (oldW > 0.0f)
            x = newMinX + ((x - oldMinX) / oldW) * newW;
        if (oldD > 0.0f)
            y = newMinY + ((y - oldMinY) / oldD) * newD;
        if (oldH > 0.0f)
            z = newMinZ + ((z - oldMinZ) / oldH) * newH;

        pos.setProperty (inputPositionX, x, getActiveUndoManager());
        pos.setProperty (inputPositionY, y, getActiveUndoManager());
        pos.setProperty (inputPositionZ, z, getActiveUndoManager());
    }
}

void WFSValueTreeState::fitAllInputPositionsToStage()
{
    auto inputs = getInputsState();
    int numInputs = inputs.getNumChildren();
    if (numInputs == 0) return;

    auto stageTree = getStageState();
    float sw = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (stageWidth))  : stageWidthDefault;
    float sd = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (stageDepth))  : stageDepthDefault;
    float sh = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (stageHeight)) : stageHeightDefault;
    float ow = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (originWidth)) : originWidthDefault;
    float od = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (originDepth)) : originDepthDefault;
    float oh = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (originHeight)) : originHeightDefault;

    float minX = -sw / 2.0f - ow;
    float maxX =  sw / 2.0f - ow;
    float minY = -sd / 2.0f - od;
    float maxY =  sd / 2.0f - od;
    float minZ = -oh;
    float maxZ =  sh - oh;

    beginUndoTransaction ("Fit Input Positions to Stage");

    for (int i = 0; i < numInputs; ++i)
    {
        auto input = inputs.getChild (i);
        auto pos = input.getChildWithName (Position);
        if (! pos.isValid()) continue;

        float x = static_cast<float> (pos.getProperty (inputPositionX));
        float y = static_cast<float> (pos.getProperty (inputPositionY));
        float z = static_cast<float> (pos.getProperty (inputPositionZ));

        float cx = juce::jlimit (minX, maxX, x);
        float cy = juce::jlimit (minY, maxY, y);
        float cz = juce::jlimit (minZ, maxZ, z);

        // Only write if changed (avoid unnecessary undo entries)
        if (cx != x) pos.setProperty (inputPositionX, cx, getActiveUndoManager());
        if (cy != y) pos.setProperty (inputPositionY, cy, getActiveUndoManager());
        if (cz != z) pos.setProperty (inputPositionZ, cz, getActiveUndoManager());
    }
}

void WFSValueTreeState::shiftAllInputPositions (float dx, float dy, float dz)
{
    auto inputs = getInputsState();
    int n = inputs.getNumChildren();
    if (n == 0) return;

    beginUndoTransaction ("Shift Input Positions");
    for (int i = 0; i < n; ++i)
    {
        auto pos = inputs.getChild (i).getChildWithName (Position);
        if (! pos.isValid()) continue;
        pos.setProperty (inputPositionX, static_cast<float> (pos.getProperty (inputPositionX)) + dx, getActiveUndoManager());
        pos.setProperty (inputPositionY, static_cast<float> (pos.getProperty (inputPositionY)) + dy, getActiveUndoManager());
        pos.setProperty (inputPositionZ, static_cast<float> (pos.getProperty (inputPositionZ)) + dz, getActiveUndoManager());
    }
}

void WFSValueTreeState::shiftAllOutputPositions (float dx, float dy, float dz)
{
    auto outputs = getOutputsState();
    int n = outputs.getNumChildren();
    if (n == 0) return;

    beginUndoTransaction ("Shift Output Positions");
    for (int i = 0; i < n; ++i)
    {
        auto pos = outputs.getChild (i).getChildWithName (Position);
        if (! pos.isValid()) continue;
        pos.setProperty (outputPositionX, static_cast<float> (pos.getProperty (outputPositionX)) + dx, getActiveUndoManager());
        pos.setProperty (outputPositionY, static_cast<float> (pos.getProperty (outputPositionY)) + dy, getActiveUndoManager());
        pos.setProperty (outputPositionZ, static_cast<float> (pos.getProperty (outputPositionZ)) + dz, getActiveUndoManager());
    }
}

void WFSValueTreeState::shiftAllReverbPositions (float dx, float dy, float dz)
{
    auto reverbs = getReverbsState();
    int n = reverbs.getNumChildren();
    if (n == 0) return;

    beginUndoTransaction ("Shift Reverb Positions");
    for (int i = 0; i < n; ++i)
    {
        auto child = reverbs.getChild (i);
        if (child.getType() != Reverb) continue;  // Skip ReverbAlgorithm
        auto pos = child.getChildWithName (Position);
        if (! pos.isValid()) continue;
        pos.setProperty (reverbPositionX, static_cast<float> (pos.getProperty (reverbPositionX)) + dx, getActiveUndoManager());
        pos.setProperty (reverbPositionY, static_cast<float> (pos.getProperty (reverbPositionY)) + dy, getActiveUndoManager());
        pos.setProperty (reverbPositionZ, static_cast<float> (pos.getProperty (reverbPositionZ)) + dz, getActiveUndoManager());
    }
}

void WFSValueTreeState::replaceState (const juce::ValueTree& newState)
{
    if (validateState (newState))
    {
        state.copyPropertiesAndChildrenFrom (newState, nullptr);
        migrateADMOSCSection();
        ensureInputAdmMappingProperty();
        // Stable-number model migration MUST precede ensureCompleteSchema: the
        // schema template stamps inputChannelType=mono, which would otherwise
        // preempt the legacy tail-split stamp for pre-rework files.
        migrateInputChannelModel();
        // Strays that older files carry in <IO> because nothing stamped them
        // anywhere. Must precede ensureCompleteSchema, or the backfill stamps a
        // default in the proper section and the real value stays orphaned.
        migrateStrayConfigProperties();
        stripObsoleteReverbProperties();
        // Same job for the effects family, and on the same side of the back-fill:
        // evict what the schema no longer declares, THEN stamp what it is missing.
        stripObsoleteEffectProperties();
        // Back-fill anything the loaded state omitted (incomplete / scope-filtered
        // files) so no parameter is left absent on this wholesale-replace path.
        ensureCompleteSchema();
        // A wholesale replace IS a project load: the numbers in the file are
        // already in use (cues, snapshots, plug-in automation, external
        // controllers), and a file written before this latch existed carries no
        // property at all. Both must land on the permanent-number regime — this
        // is the whole backward-compatibility story, and it must run after
        // ensureCompleteSchema so the IO node exists to hold the flag.
        markChannelNumbersUserOwned ("project load (state replace)");

        // A wholesale replace brings <Clusters> and <Inputs> in together, so a
        // slot-keyed clusterInputOrder is internally consistent and needs nothing
        // — which is why exportCompleteConfig, writing the live tree verbatim,
        // stays slot-keyed and carries no marker. Honour the marker anyway if one
        // is present, so a number-keyed file loaded through this path is not
        // silently read as slots.
        if (auto clusters = getClustersState();
            clusters.isValid() && clusters.getProperty (inputOrderKey).toString() == inputOrderKeyNumber)
        {
            convertClusterOrdersNumbersToSlots();
            clusters.removeProperty (inputOrderKey, nullptr);   // file artifact, not runtime state
        }

        // Array mutes are session state beside the tree: a load starts with
        // every array audible rather than inheriting the previous show's mutes.
        arrayMutes.clearAll();

        clearAllUndoHistories();
    }
}

namespace
{
    // Recursively add to `target` any property or child present in `tmpl` but
    // missing from `target`. Never overwrites an existing value and never removes
    // anything.
    //
    // Child matching mirrors spatcore::control::state::mergeTreeRecursive, and
    // must keep mirroring it: this function drifted once already and the two
    // rules below are the corrections that landed there and not here.
    //
    // An id'd child is matched on type AND id together. Matching on id alone and
    // then rejecting a type mismatch is not the same thing: getChildWithProperty
    // returns the FIRST child of any type carrying that id, so as soon as two
    // sibling node types share an id namespace, the wrong one is found, the
    // template child is declared missing, and a duplicate is appended - on every
    // load, for ever.
    //
    // CORRECTION, from a later audit: the commit that wrote this said no node
    // type in today's schema shares an id with a sibling of another type, and
    // that is FALSE. <ADMOSC> holds four <ADMCartMapping id="0".."3"> and four
    // <ADMPolarMapping id="0".."3"> as siblings (createADMOSCSection), and
    // <Sampler> holds <SamplerCell id> beside <SamplerSet id>. Both are in the
    // Config template this function walks. So the bug was LIVE, not latent:
    // every pass over an <ADMOSC> appended four duplicate polar mappings,
    // because the polar template child kept finding the cart mapping of the
    // same id. Do not restore the old matcher, and do not assume sibling types
    // have disjoint id namespaces - two pairs already do not.
    //
    // An id-less child is matched on type AND ordinal position among its
    // id-less same-type siblings. Matching on type alone returns the first such
    // child for every template child, so a run of repeated id-less siblings all
    // collapses into the first slot.
    void backfillFromTemplate (juce::ValueTree& target, const juce::ValueTree& tmpl,
                               juce::UndoManager* um)
    {
        for (int i = 0; i < tmpl.getNumProperties(); ++i)
        {
            const auto propName = tmpl.getPropertyName (i);
            if (! target.hasProperty (propName))
                target.setProperty (propName, tmpl.getProperty (propName), um);
        }

        for (int i = 0; i < tmpl.getNumChildren(); ++i)
        {
            const auto tmplChild = tmpl.getChild (i);
            juce::ValueTree match;

            if (tmplChild.hasProperty (id))
            {
                const auto tmplId = tmplChild.getProperty (id);

                for (int c = 0; c < target.getNumChildren(); ++c)
                {
                    auto candidate = target.getChild (c);

                    if (candidate.getType() == tmplChild.getType()
                        && candidate.getProperty (id) == tmplId)
                    {
                        match = candidate;
                        break;
                    }
                }
            }
            else
            {
                // Which id-less sibling of this type is this, counting from the
                // start of the template?
                int wanted = 0;
                for (int j = 0; j < i; ++j)
                {
                    const auto prev = tmpl.getChild (j);
                    if (prev.getType() == tmplChild.getType() && ! prev.hasProperty (id))
                        ++wanted;
                }

                int seen = 0;
                for (int c = 0; c < target.getNumChildren(); ++c)
                {
                    auto candidate = target.getChild (c);

                    if (candidate.getType() == tmplChild.getType() && ! candidate.hasProperty (id))
                    {
                        if (seen == wanted)
                        {
                            match = candidate;
                            break;
                        }

                        ++seen;
                    }
                }
            }

            if (match.isValid())
                backfillFromTemplate (match, tmplChild, um);
            else
                target.appendChild (tmplChild.createCopy(), um);
        }
    }
}

void WFSValueTreeState::ensureCompleteSchema()
{
    juce::UndoManager* um = nullptr;  // schema back-fill is not an undoable user edit

    // --- Config (singleton subsections) ---
    auto config = state.getChildWithName (Config);
    if (config.isValid())
    {
        juce::ValueTree defaultConfig (Config);
        createShowSection (defaultConfig);
        createIOSection (defaultConfig);
        createStageSection (defaultConfig);
        createMasterSection (defaultConfig);
        createNetworkSection (defaultConfig);
        createADMOSCSection (defaultConfig);
        createTrackingSection (defaultConfig);
        createClustersSection (defaultConfig);
        createBinauralSection (defaultConfig);
        createUISection (defaultConfig);
        createEffectsGlobalSection (defaultConfig);
        backfillFromTemplate (config, defaultConfig, um);
    }

    // --- Inputs (per channel; Config is back-filled first so IO counts exist) ---
    auto inputs = state.getChildWithName (Inputs);
    if (inputs.isValid())
    {
        for (int i = 0; i < inputs.getNumChildren(); ++i)
        {
            auto child = inputs.getChild (i);
            if (! child.hasType (Input))
                continue;
            auto tmpl = createDefaultInputChannel (i);
            backfillFromTemplate (child, tmpl, um);
        }
    }

    // --- Outputs (per channel) ---
    auto outputs = state.getChildWithName (Outputs);
    if (outputs.isValid())
    {
        for (int i = 0; i < outputs.getNumChildren(); ++i)
        {
            auto child = outputs.getChild (i);
            if (! child.hasType (Output))
                continue;
            auto tmpl = createDefaultOutputChannel (i);
            backfillFromTemplate (child, tmpl, um);
        }
    }

    // --- Reverbs (per channel + global sibling sections) ---
    auto reverbs = state.getChildWithName (Reverbs);
    if (reverbs.isValid())
    {
        int reverbCount = 0;
        for (int i = 0; i < reverbs.getNumChildren(); ++i)
            if (reverbs.getChild (i).hasType (Reverb))
                ++reverbCount;

        int revIdx = 0;
        for (int i = 0; i < reverbs.getNumChildren(); ++i)
        {
            auto child = reverbs.getChild (i);
            if (! child.hasType (Reverb))
                continue;
            auto tmpl = createDefaultReverbChannel (revIdx++, reverbCount);
            backfillFromTemplate (child, tmpl, um);
        }

        // Global sibling sections (ReverbAlgorithm / PreComp / PostEQ / PostExp):
        // match the template's own type so we never have to name them here.
        auto backfillGlobal = [um, &reverbs] (juce::ValueTree tmpl)
        {
            auto existing = reverbs.getChildWithName (tmpl.getType());
            if (existing.isValid())
                backfillFromTemplate (existing, tmpl, um);
            else
                reverbs.appendChild (tmpl.createCopy(), um);
        };
        backfillGlobal (createReverbAlgorithmSection());
        backfillGlobal (createReverbPreCompSection());
        backfillGlobal (createReverbPostEQSection());
        backfillGlobal (createReverbPostExpSection());
    }

    // --- Effects (per channel) ---
    // Unlike the reverb branch above, this one CREATES the container when it is
    // absent. It has to: validateState requires only WFSProcessor/Config/Inputs/
    // Outputs, so a project written before this family existed replaces the
    // state successfully with no <Effects> node at all, and a branch guarded on
    // isValid() would leave every effects accessor returning nothing for the
    // life of the session. (The reverb branch has the same latent gap.)
    auto effects = state.getChildWithName (Effects);
    if (! effects.isValid())
        createEffectsSection();
    else
        backfillEffectChannelsFromTemplate();
}

void WFSValueTreeState::backfillEffectChannelsFromTemplate()
{
    juce::UndoManager* um = nullptr;  // schema back-fill is not an undoable user edit

    auto effects = getEffectsState();
    if (! effects.isValid())
        return;

    // Count first, so each per-channel template is built for the count the
    // finished set has and the ring the backfill would stamp matches the one
    // the channels are already on.
    const int effectCount = getNumEffectChannels();

    juce::StringArray repairedRows;

    int fxIdx = 0;
    for (int i = 0; i < effects.getNumChildren(); ++i)
    {
        auto child = effects.getChild (i);
        if (! child.hasType (Effect))
            continue;

        const int denseIndex = fxIdx++;
        auto tmpl = createDefaultEffectChannel (denseIndex, effectCount);
        backfillFromTemplate (child, tmpl, um);

        // THE FOUR SEND ROWS, CANONICALISED ON THE WAY IN. Every accessor
        // canonicalises what it READS, so the app itself was already safe from a
        // row a file carries in the wrong shape - but only the app. The stored
        // text is what the next save writes back, so a self-feed hand-edited into
        // effects.xml stayed in that operator's file indefinitely: a unity-gain
        // loop around a delay line that no load and no save was going to take
        // out. A row this build wrote is already canonical, so this is a no-op on
        // every file the app itself produced.
        //
        // denseIndex, never the file's id: the diagonal belongs at the position
        // every other effect accessor calls this channel by, and a merged file
        // can contradict its own ids.
        auto sends = child.getChildWithName (Sends);
        if (sends.isValid())
        {
            for (auto rowId : { effectSendLevels, effectSendOns, effectFxSendLevels, effectFxSendOns })
            {
                if (! sends.hasProperty (rowId))
                    continue;   // an absent row is the backfill's business, not this one's

                const juce::String stored = sends.getProperty (rowId).toString();
                const juce::String canonical = canonicalEffectSendRow (rowId, stored, denseIndex);
                if (canonical != stored)
                {
                    sends.setProperty (rowId, canonical, um);
                    repairedRows.addIfNotAlreadyThere (rowId.toString());
                }
            }
        }
    }

    // SAY SO, for the reason the eviction hook says so: this rewrites an
    // operator's routing with no undo entry, and a repair nobody can see is a
    // repair nobody can check. By ROW rather than by channel - a file wrong in
    // one row is usually wrong in it on every channel, and 32 identical lines
    // would bury the one thing a reader needs.
    if (! repairedRows.isEmpty())
        WFSLogger::getInstance().logWarning (
            "Effects sends: repaired " + juce::String (repairedRows.size())
            + " row(s) a file carried out of canonical form - " + repairedRows.joinIntoString (", "));

    // Container properties, if the file predates either of them. `count` is
    // bookkeeping (getNumEffectChannels counts children), and the ownership
    // latch defaults to "not owned" so an older file still gets its returns
    // laid out on a count change.
    if (! effects.hasProperty (count))
        effects.setProperty (count, effectCount, um);
    if (! effects.hasProperty (effectPositionsUserOwned))
        effects.setProperty (effectPositionsUserOwned, false, um);
}

void WFSValueTreeState::migrateADMOSCSection()
{
    auto config = state.getChildWithName (Config);
    if (!config.isValid()) return;

    auto admosc = config.getChildWithName (ADMOSC);
    if (!admosc.isValid())
    {
        // No ADMOSC section at all — create fresh
        createADMOSCSection (config);
        return;
    }

    // Detect old-style flat ADMOSC (has admOscOffsetX property)
    if (!admosc.hasProperty (admOscOffsetX)) return;  // Already new format

    // Read old values
    float oldOffsetX = static_cast<float> (admosc.getProperty (admOscOffsetX, 0.0f));
    float oldOffsetY = static_cast<float> (admosc.getProperty (juce::Identifier ("admOscOffsetY"), 0.0f));
    float oldOffsetZ = static_cast<float> (admosc.getProperty (juce::Identifier ("admOscOffsetZ"), 0.0f));
    float oldScaleX  = static_cast<float> (admosc.getProperty (admOscScaleX, 1.0f));
    float oldScaleY  = static_cast<float> (admosc.getProperty (juce::Identifier ("admOscScaleY"), 1.0f));
    float oldScaleZ  = static_cast<float> (admosc.getProperty (juce::Identifier ("admOscScaleZ"), 1.0f));
    int oldFlipX     = static_cast<int>   (admosc.getProperty (admOscFlipX, 0));
    int oldFlipY     = static_cast<int>   (admosc.getProperty (juce::Identifier ("admOscFlipY"), 0));
    int oldFlipZ     = static_cast<int>   (admosc.getProperty (juce::Identifier ("admOscFlipZ"), 0));

    // Remove old ADMOSC node
    config.removeChild (admosc, nullptr);

    // Create new structure
    createADMOSCSection (config);

    // Apply old values to Cart mapping 0
    auto newAdmosc = config.getChildWithName (ADMOSC);
    auto cartMapping0 = newAdmosc.getChildWithName (ADMCartMapping);
    if (!cartMapping0.isValid()) return;

    float oldOffsets[3] = { oldOffsetX, oldOffsetY, oldOffsetZ };
    float oldScales[3]  = { oldScaleX,  oldScaleY,  oldScaleZ };
    int   oldFlips[3]   = { oldFlipX,   oldFlipY,   oldFlipZ };

    for (int a = 0; a < 3; ++a)
    {
        auto axis = cartMapping0.getChild (a);
        if (!axis.isValid()) continue;

        axis.setProperty (admCartCenterOffset,  oldOffsets[a], nullptr);
        axis.setProperty (admCartSignFlip,      oldFlips[a], nullptr);
        // Convert scale to half-widths: old mapping was offset + v * scale
        // New piecewise: at breakpoint 0.5, inner = scale*0.5, outer = scale*0.5
        float hw = std::abs (oldScales[a]) * 0.5f;
        if (hw < admCartWidthMin) hw = admCartWidthDefault;
        axis.setProperty (admCartPosInnerWidth, hw, nullptr);
        axis.setProperty (admCartPosOuterWidth, hw, nullptr);
        axis.setProperty (admCartNegInnerWidth, hw, nullptr);
        axis.setProperty (admCartNegOuterWidth, hw, nullptr);
    }
}

void WFSValueTreeState::ensureInputAdmMappingProperty()
{
    auto inputs = state.getChildWithName (Inputs);
    if (!inputs.isValid()) return;

    for (int i = 0; i < inputs.getNumChildren(); ++i)
    {
        auto input = inputs.getChild (i);
        auto position = input.getChildWithName (Position);
        if (position.isValid() && !position.hasProperty (inputAdmMapping))
            position.setProperty (inputAdmMapping, inputAdmMappingDefault, nullptr);
    }
}

bool WFSValueTreeState::validateState (const juce::ValueTree& stateToValidate) const
{
    // Check root type
    if (stateToValidate.getType() != WFSProcessor)
        return false;

    // Check for required sections
    if (!stateToValidate.getChildWithName (Config).isValid())
        return false;
    if (!stateToValidate.getChildWithName (Inputs).isValid())
        return false;
    if (!stateToValidate.getChildWithName (Outputs).isValid())
        return false;

    return true;
}

void WFSValueTreeState::copyStateFrom (const WFSValueTreeState& other)
{
    replaceState (other.state);
}

//==============================================================================
// TreeParameterStore seams (change-notification dispatch hooks)
//==============================================================================
// The ValueTree::Listener plumbing lives in TreeParameterStore. Its
// valueTreePropertyChanged calls resolveChannelIndex, then the POST-WRITE
// HOOK (handlePostWrite — the WFS semantic invariants below), then
// notifyParameterListeners — the same order as the pre-split monolith.

int WFSValueTreeState::resolveChannelIndex (const juce::ValueTree& changedNode) const
{
    // Determine channel index if this is an input/output/reverb parameter.
    // The notified index is the SLOT (dense child index): for inputs the id
    // is the permanent channel number and the list may have gaps, so it must
    // go through the number->slot lookup — id - 1 would point at the wrong
    // channel. Outputs/reverbs/effects stay dense (id == index + 1).
    //
    // The walk is exactly two levels, which is what an <Effect> needs: a flat
    // section is the parent, and a <Band> or <Tap> under a module node makes
    // the module the parent and the <Effect> the grandparent. Anything deeper
    // would notify with -1.
    auto slotOf = [this] (const juce::ValueTree& node) -> int
    {
        if (node.getType() == Input)
            return getSlotForChannelNumber (static_cast<int> (node.getProperty (id)));
        return static_cast<int> (node.getProperty (id)) - 1;
    };

    int channelIndex = -1;
    auto parent = changedNode.getParent();

    if (parent.isValid())
    {
        if (parent.getType() == Input || parent.getType() == Output || parent.getType() == Reverb
            || parent.getType() == Effect)
            channelIndex = slotOf (parent);
        else if (parent.getParent().isValid() &&
                 (parent.getParent().getType() == Input || parent.getParent().getType() == Output ||
                  parent.getParent().getType() == Reverb || parent.getParent().getType() == Effect))
            channelIndex = slotOf (parent.getParent());
    }

    return channelIndex;
}

void WFSValueTreeState::handlePostWrite (juce::ValueTree& changedNode, const juce::Identifier& property,
                                         const juce::var& value, int channelIndex)
{
    // Enforce tracking constraint: only one tracked input per cluster
    // This catches changes from OSC, file loading, and any other source
    if (property == inputTrackingActive && channelIndex >= 0)
    {
        enforceClusterTrackingConstraint (channelIndex);
    }
    else if (property == inputCluster && channelIndex >= 0)
    {
        // When cluster assignment changes, also check constraint
        enforceClusterTrackingConstraint (channelIndex);

        // And enforce the Shared-Position invariant on the new cluster so
        // a freshly assigned input snaps to the cluster's shared position.
        int newCluster = static_cast<int> (value);
        if (newCluster >= 1)
            enforceSharedClusterInvariant (newCluster);
    }
    else if (property == clusterReferenceMode &&
             changedNode.getType() == Cluster)
    {
        // Reference mode flipped (from OSC, MCP, file load, etc.). If the
        // cluster just entered Shared Position, snap all its members to the
        // first-ordered member's position. Idempotent for non-shared modes.
        int clusterIdx = static_cast<int> (changedNode.getProperty (id));
        if (clusterIdx >= 1 && static_cast<int> (value) == 2)
            enforceSharedClusterInvariant (clusterIdx);
    }
}

//==============================================================================
// Initialization
//==============================================================================

void WFSValueTreeState::initializeDefaultState()
{
    state = juce::ValueTree (WFSProcessor);
    state.setProperty (version, "1.0", nullptr);

    createConfigSection();
    createInputsSection();
    createOutputsSection();
    createReverbsSection();
    createEffectsSection();
    createAudioPatchSection();
}

void WFSValueTreeState::createConfigSection()
{
    juce::ValueTree config (Config);

    createShowSection (config);
    createIOSection (config);
    createStageSection (config);
    createMasterSection (config);
    createNetworkSection (config);
    createADMOSCSection (config);
    createTrackingSection (config);
    createClustersSection (config);
    createBinauralSection (config);
    createUISection (config);
    createEffectsGlobalSection (config);

    state.appendChild (config, nullptr);
}

void WFSValueTreeState::createShowSection (juce::ValueTree& config)
{
    juce::ValueTree show (Show);
    show.setProperty (showName, showNameDefault, nullptr);
    show.setProperty (showLocation, showLocationDefault, nullptr);
    show.setProperty (autoPreselectDirty, false, nullptr);
    config.appendChild (show, nullptr);
}

void WFSValueTreeState::createIOSection (juce::ValueTree& config)
{
    juce::ValueTree io (IO);
    io.setProperty (inputChannels, inputChannelsDefault, nullptr);
    io.setProperty (outputChannels, outputChannelsDefault, nullptr);
    io.setProperty (reverbChannels, reverbChannelsDefault, nullptr);
    // Zero by default, so a show that uses no effects gains one attribute and
    // nothing else. It lives HERE rather than on <Effects> for the same reason
    // the other three counts do: applyConfigSection reads the channel inventory
    // off <IO> before any per-family file is touched.
    io.setProperty (effectChannels, effectChannelsDefault, nullptr);
    io.setProperty (algorithmDSP, algorithmDSPDefault, nullptr);
    io.setProperty (runDSP, runDSPDefault, nullptr);
    config.appendChild (io, nullptr);
}

void WFSValueTreeState::createStageSection (juce::ValueTree& config)
{
    juce::ValueTree stage (Stage);
    stage.setProperty (stageShape, stageShapeDefault, nullptr);
    stage.setProperty (stageWidth, stageWidthDefault, nullptr);
    stage.setProperty (stageDepth, stageDepthDefault, nullptr);
    stage.setProperty (stageHeight, stageHeightDefault, nullptr);
    stage.setProperty (stageDiameter, stageDiameterDefault, nullptr);
    stage.setProperty (domeElevation, domeElevationDefault, nullptr);
    stage.setProperty (originWidth, originWidthDefault, nullptr);
    stage.setProperty (originDepth, originDepthDefault, nullptr);
    stage.setProperty (originHeight, originHeightDefault, nullptr);
    stage.setProperty (speedOfSound, speedOfSoundDefault, nullptr);
    stage.setProperty (temperature, temperatureDefault, nullptr);
    config.appendChild (stage, nullptr);
}

void WFSValueTreeState::createMasterSection (juce::ValueTree& config)
{
    juce::ValueTree master (Master);
    master.setProperty (masterLevel, masterLevelDefault, nullptr);
    master.setProperty (systemLatency, systemLatencyDefault, nullptr);
    master.setProperty (haasEffect, haasEffectDefault, nullptr);
    master.setProperty (reverbsMapVisible, 1, nullptr);  // Default: visible
    master.setProperty (effectsMapVisible, effectsMapVisibleDefault, nullptr);  // Default: visible
    config.appendChild (master, nullptr);
}

void WFSValueTreeState::createEffectsGlobalSection (juce::ValueTree& config)
{
    juce::ValueTree effectsGlobal (EffectsGlobal);
    effectsGlobal.setProperty (effectsGlobalLinkNames, effectsGlobalLinkNamesDefault, nullptr);
    effectsGlobal.setProperty (effectsGlobalLinkMode, effectsGlobalLinkModeDefault, nullptr);
    effectsGlobal.setProperty (effectsGlobalFxFeedGeometric, effectsGlobalFxFeedGeometricDefault, nullptr);
    effectsGlobal.setProperty (effectsGlobalWorkerThreads, effectsGlobalWorkerThreadsDefault, nullptr);
    effectsGlobal.setProperty (effectsGlobalReturnCushion, effectsGlobalReturnCushionDefault, nullptr);
    effectsGlobal.setProperty (effectsGlobalLoopGuard, effectsGlobalLoopGuardDefault, nullptr);
    effectsGlobal.setProperty (effectsGlobalLoopGuardCeiling, effectsGlobalLoopGuardCeilingDefault, nullptr);
    effectsGlobal.setProperty (effectsGlobalMaxDelaySeconds, effectsGlobalMaxDelaySecondsDefault, nullptr);
    effectsGlobal.setProperty (effectsGlobalFeedGpuDevice, effectsGlobalFeedGpuDeviceDefault, nullptr);
    config.appendChild (effectsGlobal, nullptr);
}

void WFSValueTreeState::migrateStrayConfigProperties()
{
    // A handful of config parameters were never stamped by any createXSection.
    // They came into being the first time the GUI wrote one, and
    // WfsParameters::setConfigParamBySection put them wherever its prefix tests
    // led - which for a lowercase name like "trackingMqttHost" is none of them,
    // so they landed in <IO>, the fallback. They worked, because getConfigParam
    // searches every section and takes the first hit.
    //
    // They are stamped in their proper sections now, which means a file written
    // before this change would carry the real value in <IO> AND a fresh default
    // in the right section. Both lookups take the first hit in the same order, so
    // nothing would break - but the duplicate is the kind of thing that bites
    // later, when someone edits the one nobody reads. Move the value across and
    // drop the stray.
    //
    // Runs BEFORE ensureCompleteSchema, so the value is already in place when the
    // backfill looks, and no default is stamped over it.
    auto config = getConfigState();
    if (! config.isValid())
        return;

    auto io = config.getChildWithName (IO);
    if (! io.isValid())
        return;

    struct Stray { juce::Identifier property; juce::Identifier section; };
    static const Stray strays[] = {
        { networkOscQueryEnabled, Network },  { networkOscQueryPort,  Network },
        { networkOscSourceFilter, Network },  { trackingOscPath,      Tracking },
        { trackingPsnInterface,   Tracking }, { trackingMqttHost,     Tracking },
        { trackingMqttTopic,      Tracking }, { trackingMqttTagIds,   Tracking },
        { trackingMqttJsonX,      Tracking }, { trackingMqttJsonY,    Tracking },
        { trackingMqttJsonZ,      Tracking }, { trackingMqttJsonQ,    Tracking },
        { samplerControllerMode,  UI },
    };

    for (const auto& stray : strays)
    {
        if (! io.hasProperty (stray.property))
            continue;

        auto target = config.getChildWithName (stray.section);
        if (! target.isValid())
            continue;   // section missing: leave the value where it is rather than lose it

        target.setProperty (stray.property, io.getProperty (stray.property), nullptr);
        io.removeProperty (stray.property, nullptr);
    }

    // colorScheme is the same story with a rename on top: the GUI wrote it under
    // the unmapped name "ColorScheme", so older files carry that spelling on <IO>
    // while the MCP surface has always advertised "colorScheme". Carry the value
    // across to the real identifier so an operator's chosen theme survives.
    static const juce::Identifier legacyColorScheme ("ColorScheme");
    if (io.hasProperty (legacyColorScheme))
    {
        auto ui = config.getChildWithName (UI);
        if (ui.isValid())
            ui.setProperty (colorScheme, io.getProperty (legacyColorScheme), nullptr);
        io.removeProperty (legacyColorScheme, nullptr);
    }
}

void WFSValueTreeState::stripObsoleteReverbProperties()
{
    // reverbLSenable (Live Source attenuation on reverb feeds) was removed on
    // 2026-08-28 without ever having been wired: the Tamer computes gains per
    // (input, speaker) and the feed matrix never read them. Files written
    // before that carry the attribute on every <Feed>; mergeTreeRecursive
    // never removes a property, so without this it would ride along in the
    // live tree and be re-saved forever. Same shape as legacyColorScheme
    // above: a local identifier, because the real one no longer exists.
    static const juce::Identifier legacyReverbLSenable ("reverbLSenable");

    const int n = getNumReverbChannels();
    for (int i = 0; i < n; ++i)
    {
        auto feed = getReverbFeedSection (i);
        if (feed.isValid() && feed.hasProperty (legacyReverbLSenable))
            feed.removeProperty (legacyReverbLSenable, nullptr);
    }
}

void WFSValueTreeState::stripObsoleteEffectProperties()
{
    // The reverb twin above names one retired identifier by hand. This one names
    // none, on purpose: every property anywhere under an <Effect> is stamped by
    // exactly one builder under createDefaultEffectChannel, so a freshly built
    // channel IS the list of what the schema still declares. Diffing against it
    // evicts a retired attribute the moment its setProperty line is deleted,
    // with no second table to keep in step - and the reverb hook's history (a
    // name removed in 125e00b, re-saved in every show until someone noticed) is
    // what that table costs when it is forgotten.
    //
    // Structure, not just the top level: <Band> and <Tap> carry ids, the eleven
    // module nodes carry none but each has its own type, and the walk matches on
    // exactly that - the same rule backfillFromTemplate uses in the opposite
    // direction.
    const int n = getNumEffectChannels();
    if (n == 0)
        return;

    // One template for the whole set. createDefaultEffectChannel lays the
    // channel out on the ring for (index, totalCount), but only VALUES depend on
    // that; the property NAMES - all this walk reads - do not, so the shape of
    // channel 0 answers for every channel.
    const auto tmplChannel = createDefaultEffectChannel (0, juce::jmax (1, n));

    // ONCE DECLARED-BUT-UNSTAMPED, NOW STAMPED - and the exemption that stood in
    // for that is deleted, which is this design working rather than a
    // regression. A template diff cannot tell PENDING from RETIRED: both are
    // absent from a freshly built channel. While <Sends> was built EMPTY, the
    // four packed rows (effectSendLevels / effectSendOns / effectFxSendLevels /
    // effectFxSendOns) were declared in WFSParameterIDs, written at runtime, and
    // therefore indistinguishable from retired names by the only evidence this
    // hook has - so they were named here by hand and skipped, or the first
    // runtime write would have been saved correctly, restored faithfully by the
    // merge, and deleted right here with no error and no undo entry.
    //
    // createEffectSendsSection stamps all four now, at their fixed widths, so
    // the template carries them like every other property and the list is dead
    // weight. Its own comment said an entry must be REMOVED the day its property
    // stops being declared-but-unstamped; this is that day. What goes with it:
    // the second warning that named an exempt row found outside <Sends>, because
    // there is no longer anything exempt to find. A send row name on a <Chain> or
    // a <Band> is now what it always looked like - a property the template does
    // not have on that node - and is evicted like any other ghost.
    //
    // The rule the exemption bought stays, and is now the general one: nothing
    // may stamp a property onto an <Effect> subtree that createDefaultEffectChannel
    // does not also stamp. A runtime-only flag parked there is evicted on the
    // next load and belongs outside the persisted subtree.

    // What was actually dropped, for the log. Collected rather than logged in
    // place: a name retired from one builder is evicted from the same node of
    // all 32 channels, and thirty-two identical warnings would bury the one
    // thing a reader needs, which is WHICH names went.
    juce::StringArray evicted;

    // Depth-first, template-driven. A node the template does not have at all is
    // left alone rather than deleted: removing a whole subtree is a different
    // and much more destructive decision than dropping a retired attribute, and
    // nothing has ever needed it. Note the consequence, which is wider than the
    // node itself - the walk does not DESCEND into an unmatched node either, so
    // every property under it is out of this hook's reach. Retiring a whole
    // module type (dropping <FxTrem> from the chain, say) therefore needs a
    // deliberate decision here, exactly as the reverb hook's hand-written list
    // does at property granularity.
    std::function<void (juce::ValueTree&, const juce::ValueTree&)> evict =
        [&evict, &evicted] (juce::ValueTree& target, const juce::ValueTree& tmpl)
    {
        for (int i = target.getNumProperties(); --i >= 0;)
        {
            const auto propName = target.getPropertyName (i);

            if (! tmpl.hasProperty (propName))
            {
                evicted.addIfNotAlreadyThere (propName.toString());
                target.removeProperty (propName, nullptr);   // schema eviction is not an undoable user edit
            }
        }

        for (int c = 0; c < target.getNumChildren(); ++c)
        {
            auto child = target.getChild (c);
            juce::ValueTree match;

            if (child.hasProperty (id))
            {
                // Type AND id together, never id alone: two sibling types
                // sharing an id namespace would otherwise cross-match and strip
                // each other's properties wholesale.
                for (int t = 0; t < tmpl.getNumChildren(); ++t)
                {
                    auto candidate = tmpl.getChild (t);
                    if (candidate.getType() == child.getType()
                        && candidate.getProperty (id) == child.getProperty (id))
                    {
                        match = candidate;
                        break;
                    }
                }
            }
            else
            {
                match = tmpl.getChildWithName (child.getType());
                if (match.isValid() && match.hasProperty (id))
                    match = juce::ValueTree();   // an id-less node never answers for an id'd template child
            }

            if (match.isValid())
                evict (child, match);
        }
    };

    auto effects = getEffectsState();
    for (int i = 0; i < effects.getNumChildren(); ++i)
    {
        auto child = effects.getChild (i);
        if (child.hasType (Effect))
            evict (child, tmplChannel);
    }

    // SAY SO. Eviction passes nullptr for the UndoManager by design - a schema
    // change is not a user edit and Ctrl+Z must not resurrect a retired name -
    // so a wrong eviction cannot be undone and, until this line, could not even
    // be noticed: the <Sends> hole would have destroyed an operator's entire
    // send routing on load with nothing in the log to say it had happened. A
    // retired name appears here on EVERY load of a file that still carries it and
    // stops the first time that file is saved back, which is the honest shape of
    // it: the hook strips the live tree, not the file. Anything else appearing
    // here is a bug in this hook.
    if (! evicted.isEmpty())
        WFSLogger::getInstance().logWarning (
            "Effects schema: dropped " + juce::String (evicted.size())
            + " attribute(s) no longer declared by the effect channel template - "
            + evicted.joinIntoString (", "));
}

void WFSValueTreeState::createNetworkSection (juce::ValueTree& config)
{
    juce::ValueTree network (Network);
    network.setProperty (networkInterface, "", nullptr);
    network.setProperty (networkCurrentIP, networkCurrentIPDefault, nullptr);
    network.setProperty (networkRxUDPport, networkRxUDPportDefault, nullptr);
    network.setProperty (networkRxTCPport, networkRxTCPportDefault, nullptr);
    network.setProperty (findDevicePassword, findDevicePasswordDefault, nullptr);
    // Stamped so they exist from the start. Until now these came into being only
    // when the GUI first wrote one, which left every generic reader and writer -
    // MCP among them - addressing a property that was not there yet.
    network.setProperty (networkOscQueryEnabled, 0,    nullptr);
    network.setProperty (networkOscQueryPort,    5005, nullptr);
    network.setProperty (networkOscSourceFilter, 0,    nullptr);
    config.appendChild (network, nullptr);
}

void WFSValueTreeState::createADMOSCSection (juce::ValueTree& config)
{
    juce::ValueTree admosc (ADMOSC);

    // Create 4 Cartesian mappings
    for (int m = 0; m < admCartMappingCount; ++m)
    {
        juce::ValueTree mapping (ADMCartMapping);
        mapping.setProperty (id, m, nullptr);

        for (int a = 0; a < 3; ++a)
        {
            juce::ValueTree axis (ADMCartAxis);
            axis.setProperty (admCartAxisId,        a, nullptr);
            axis.setProperty (admCartAxisSwap,      a, nullptr);  // identity: X→X, Y→Y, Z→Z

            // ADM-OSC y=forward vs WFS y=upstage are opposite conventions,
            // so the Y axis needs a sign flip by default to get audience-side
            // sources to land on the audience side of the stage.
            const int signFlip = (a == 1) ? 1 : admCartSignFlipDefault;
            axis.setProperty (admCartSignFlip,      signFlip, nullptr);
            axis.setProperty (admCartCenterOffset,  admCartCenterOffsetDefault, nullptr);
            axis.setProperty (admCartBreakpoint,    admCartBreakpointDefault, nullptr);
            axis.setProperty (admCartPosInnerWidth, admCartWidthDefault, nullptr);
            axis.setProperty (admCartPosOuterWidth, admCartWidthDefault, nullptr);
            axis.setProperty (admCartNegInnerWidth, admCartWidthDefault, nullptr);
            axis.setProperty (admCartNegOuterWidth, admCartWidthDefault, nullptr);
            mapping.appendChild (axis, nullptr);
        }
        admosc.appendChild (mapping, nullptr);
    }

    // Create 4 Polar mappings
    for (int m = 0; m < admPolarMappingCount; ++m)
    {
        juce::ValueTree mapping (ADMPolarMapping);
        mapping.setProperty (id,                    m, nullptr);
        mapping.setProperty (admPolarAzimuthOffset,  admPolarAzimuthOffsetDefault, nullptr);
        mapping.setProperty (admPolarAzimuthFlip,    admPolarAzimuthFlipDefault, nullptr);
        mapping.setProperty (admPolarElevationFlip,  admPolarElevationFlipDefault, nullptr);
        mapping.setProperty (admPolarDistMin,        admPolarDistMinDefault, nullptr);
        mapping.setProperty (admPolarDistMax,        admPolarDistMaxDefault, nullptr);
        admosc.appendChild (mapping, nullptr);
    }

    config.appendChild (admosc, nullptr);
}

void WFSValueTreeState::createTrackingSection (juce::ValueTree& config)
{
    juce::ValueTree tracking (Tracking);
    tracking.setProperty (trackingEnabled, trackingEnabledDefault, nullptr);
    tracking.setProperty (trackingProtocol, trackingProtocolDefault, nullptr);
    tracking.setProperty (trackingPort, trackingPortDefault, nullptr);
    tracking.setProperty (trackingOffsetX, trackingOffsetDefault, nullptr);
    tracking.setProperty (trackingOffsetY, trackingOffsetDefault, nullptr);
    tracking.setProperty (trackingOffsetZ, trackingOffsetDefault, nullptr);
    tracking.setProperty (trackingScaleX, trackingScaleDefault, nullptr);
    tracking.setProperty (trackingScaleY, trackingScaleDefault, nullptr);
    tracking.setProperty (trackingScaleZ, trackingScaleDefault, nullptr);
    tracking.setProperty (trackingFlipX, trackingFlipDefault, nullptr);
    tracking.setProperty (trackingFlipY, trackingFlipDefault, nullptr);
    tracking.setProperty (trackingFlipZ, trackingFlipDefault, nullptr);
    // Same story as the OSCQuery trio above. Defaults match what the GUI falls
    // back to when the property is absent, so stamping them changes nothing an
    // operator can see.
    tracking.setProperty (trackingOscPath,      "/wfs/tracking <ID> <x> <y> <z>", nullptr);
    tracking.setProperty (trackingPsnInterface, "",                               nullptr);
    tracking.setProperty (trackingMqttHost,     "192.168.1.1",                    nullptr);
    tracking.setProperty (trackingMqttTopic,    "dwm/node/+/uplink/location",     nullptr);
    tracking.setProperty (trackingMqttTagIds,   "",                               nullptr);
    tracking.setProperty (trackingMqttJsonX,    "x",                              nullptr);
    tracking.setProperty (trackingMqttJsonY,    "y",                              nullptr);
    tracking.setProperty (trackingMqttJsonZ,    "z",                              nullptr);
    tracking.setProperty (trackingMqttJsonQ,    "quality",                        nullptr);
    config.appendChild (tracking, nullptr);
}

void WFSValueTreeState::createClustersSection (juce::ValueTree& config)
{
    juce::ValueTree clusters (Clusters);
    clusters.setProperty (count, maxClusters, nullptr);

    // Create 10 cluster entries
    for (int i = 0; i < maxClusters; ++i)
    {
        juce::ValueTree cluster (Cluster);
        cluster.setProperty (id, i + 1, nullptr);
        cluster.setProperty (clusterReferenceMode, clusterReferenceModeDefault, nullptr);
        cluster.setProperty (clusterInputOrder, clusterInputOrderDefault, nullptr);
        cluster.setProperty (clusterInputsVisible, clusterInputsVisibleDefault, nullptr);

        // Cluster LFO section
        juce::ValueTree lfo (ClusterLFO);
        lfo.setProperty (clusterLFOactive,         clusterLFOactiveDefault,          nullptr);
        lfo.setProperty (clusterLFOperiod,         clusterLFOperiodDefault,          nullptr);
        lfo.setProperty (clusterLFOphase,          clusterLFOphaseDefault,           nullptr);
        lfo.setProperty (clusterLFOshapeX,         clusterLFOshapeDefault,           nullptr);
        lfo.setProperty (clusterLFOshapeY,         clusterLFOshapeDefault,           nullptr);
        lfo.setProperty (clusterLFOshapeZ,         clusterLFOshapeDefault,           nullptr);
        lfo.setProperty (clusterLFOshapeRot,       clusterLFOshapeDefault,           nullptr);
        lfo.setProperty (clusterLFOshapeScale,     clusterLFOshapeDefault,           nullptr);
        lfo.setProperty (clusterLFOrateX,          clusterLFOrateDefault,            nullptr);
        lfo.setProperty (clusterLFOrateY,          clusterLFOrateDefault,            nullptr);
        lfo.setProperty (clusterLFOrateZ,          clusterLFOrateDefault,            nullptr);
        lfo.setProperty (clusterLFOrateRot,        clusterLFOrateDefault,            nullptr);
        lfo.setProperty (clusterLFOrateScale,      clusterLFOrateDefault,            nullptr);
        lfo.setProperty (clusterLFOamplitudeX,     clusterLFOamplitudeXYZDefault,    nullptr);
        lfo.setProperty (clusterLFOamplitudeY,     clusterLFOamplitudeXYZDefault,    nullptr);
        lfo.setProperty (clusterLFOamplitudeZ,     clusterLFOamplitudeXYZDefault,    nullptr);
        lfo.setProperty (clusterLFOamplitudeRot,   clusterLFOamplitudeRotDefault,    nullptr);
        lfo.setProperty (clusterLFOamplitudeScale, clusterLFOamplitudeScaleDefault,  nullptr);
        lfo.setProperty (clusterLFOphaseX,         clusterLFOphaseDefault,           nullptr);
        lfo.setProperty (clusterLFOphaseY,         clusterLFOphaseDefault,           nullptr);
        lfo.setProperty (clusterLFOphaseZ,         clusterLFOphaseDefault,           nullptr);
        lfo.setProperty (clusterLFOphaseRot,       clusterLFOphaseDefault,           nullptr);
        lfo.setProperty (clusterLFOphaseScale,     clusterLFOphaseDefault,           nullptr);
        cluster.appendChild (lfo, nullptr);

        clusters.appendChild (cluster, nullptr);
    }

    config.appendChild (clusters, nullptr);
}

void WFSValueTreeState::createBinauralSection (juce::ValueTree& config)
{
    juce::ValueTree binaural (Binaural);
    binaural.setProperty (binauralEnabled, binauralEnabledDefault, nullptr);
    binaural.setProperty (binauralSoloMode, binauralSoloModeDefault, nullptr);
    binaural.setProperty (binauralOutputChannel, binauralOutputChannelDefault, nullptr);
    binaural.setProperty (binauralListenerDistance, binauralListenerDistanceDefault, nullptr);
    binaural.setProperty (binauralListenerAngle, binauralListenerAngleDefault, nullptr);
    binaural.setProperty (binauralAttenuation, binauralAttenuationDefault, nullptr);
    binaural.setProperty (binauralDelay, binauralDelayDefault, nullptr);
    // (solo is per-channel: Channel.inputSolo — no csv here anymore)
    config.appendChild (binaural, nullptr);
}

void WFSValueTreeState::createUISection (juce::ValueTree& config)
{
    juce::ValueTree ui (WFSParameterIDs::UI);
    ui.setProperty (streamDeckEnabled, streamDeckEnabledDefault, nullptr);
    ui.setProperty (samplerEnabled, samplerEnabledDefault, nullptr);
    ui.setProperty (samplerBlockSerial, "", nullptr);
    ui.setProperty (lightpadPad0Split, lightpadSplitDefault, nullptr);
    ui.setProperty (lightpadPad1Split, lightpadSplitDefault, nullptr);
    ui.setProperty (lightpadPad2Split, lightpadSplitDefault, nullptr);
    ui.setProperty (lightpadPad0DeviceId, "", nullptr);
    ui.setProperty (lightpadPad1DeviceId, "", nullptr);
    ui.setProperty (lightpadPad2DeviceId, "", nullptr);
    ui.setProperty (lightpadSensitivity, lightpadSensitivityDefault, nullptr);
    ui.setProperty (samplerControllerMode, 0, nullptr);
    ui.setProperty (colorScheme, 0, nullptr);
    config.appendChild (ui, nullptr);
}

void WFSValueTreeState::createInputsSection()
{
    juce::ValueTree inputs (Inputs);
    inputs.setProperty (count, inputChannelsDefault, nullptr);

    for (int i = 0; i < inputChannelsDefault; ++i)
        inputs.appendChild (createDefaultInputChannel (i), nullptr);

    state.appendChild (inputs, nullptr);
}

void WFSValueTreeState::createOutputsSection()
{
    juce::ValueTree outputs (Outputs);
    outputs.setProperty (count, outputChannelsDefault, nullptr);

    for (int i = 0; i < outputChannelsDefault; ++i)
        outputs.appendChild (createDefaultOutputChannel (i), nullptr);

    state.appendChild (outputs, nullptr);
}

void WFSValueTreeState::createReverbsSection()
{
    juce::ValueTree reverbs (Reverbs);
    reverbs.setProperty (count, reverbChannelsDefault, nullptr);

    // Create reverb channels based on default count (typically 0)
    for (int i = 0; i < reverbChannelsDefault; ++i)
        reverbs.appendChild (createDefaultReverbChannel (i, reverbChannelsDefault), nullptr);

    // Create global algorithm section
    reverbs.appendChild (createReverbAlgorithmSection(), nullptr);

    // Create global pre-compressor section
    reverbs.appendChild (createReverbPreCompSection(), nullptr);

    // Create global post-processing EQ section
    reverbs.appendChild (createReverbPostEQSection(), nullptr);

    // Create global post-expander section
    reverbs.appendChild (createReverbPostExpSection(), nullptr);

    state.appendChild (reverbs, nullptr);
}

void WFSValueTreeState::createEffectsSection()
{
    juce::ValueTree effects (Effects);
    effects.setProperty (count, effectChannelsDefault, nullptr);

    // The family's own ownership latch, stamped at creation so it is never
    // absent and never has to be inferred. See the header for why it must not
    // be the shared <Stage> flag.
    effects.setProperty (effectPositionsUserOwned, false, nullptr);

    // Zero by default: <Effects count="0"/> and nothing else. Unlike
    // createReverbsSection there are NO global siblings to append - the effects
    // globals live in Config/EffectsGlobal, so nothing this application writes
    // ever puts a non-<Effect> node in here. The accessors still count by type
    // rather than trusting that, because a merged file is not this application:
    // see getEffectState.
    for (int i = 0; i < effectChannelsDefault; ++i)
        effects.appendChild (createDefaultEffectChannel (i, effectChannelsDefault), nullptr);

    state.appendChild (effects, nullptr);
}

void WFSValueTreeState::createAudioPatchSection()
{
    juce::ValueTree audioPatch (AudioPatch);
    audioPatch.setProperty (driverMode, driverModeDefault, nullptr);
    audioPatch.setProperty (audioInterface, audioInterfaceDefault, nullptr);

    // Create input patch matrix (diagonal by default)
    juce::ValueTree inputPatchTree (InputPatch);
    inputPatchTree.setProperty (rows, inputChannelsDefault, nullptr);
    inputPatchTree.setProperty (cols, maxInputChannels, nullptr);

    juce::StringArray inputPatchData;
    for (int r = 0; r < inputChannelsDefault; ++r)
    {
        juce::StringArray row;
        for (int c = 0; c < maxInputChannels; ++c)
            row.add (r == c ? "1" : "0");
        inputPatchData.add (row.joinIntoString (","));
    }
    inputPatchTree.setProperty (patchData, inputPatchData.joinIntoString (";"), nullptr);
    audioPatch.appendChild (inputPatchTree, nullptr);

    // Create output patch matrix (diagonal by default)
    juce::ValueTree outputPatchTree (OutputPatch);
    outputPatchTree.setProperty (rows, outputChannelsDefault, nullptr);
    outputPatchTree.setProperty (cols, maxOutputChannels, nullptr);

    juce::StringArray outputPatchData;
    for (int r = 0; r < outputChannelsDefault; ++r)
    {
        juce::StringArray row;
        for (int c = 0; c < maxOutputChannels; ++c)
            row.add (r == c ? "1" : "0");
        outputPatchData.add (row.joinIntoString (","));
    }
    outputPatchTree.setProperty (patchData, outputPatchData.joinIntoString (";"), nullptr);
    audioPatch.appendChild (outputPatchTree, nullptr);

    state.appendChild (audioPatch, nullptr);
}

juce::ValueTree WFSValueTreeState::createDefaultInputChannel (int index, int totalInputsIn, int channelNumber)
{
    // The caller must pass the TARGET count when it is growing the channel
    // list. setNumInputChannels creates the new channels first and only writes
    // the new count afterwards, so reading it from the tree here yielded the
    // OLD count: growing 8 -> 64 gave every new channel numRows = 1, and
    // getDefaultInputPosition's fracY = (row+1)/(numRows+1) then ran past 1 —
    // index 63 landed at fracY = 4, i.e. four stage-depths off the front edge.
    int totalInputs = totalInputsIn;
    if (totalInputs <= 0)
    {
        totalInputs = inputChannelsDefault;
        auto io = getIOState();
        if (io.isValid())
            totalInputs = static_cast<int> (io.getProperty (inputChannels));
    }
    totalInputs = juce::jmax (1, totalInputs, index + 1);

    // The permanent channel number defaults to index + 1 (dense creation);
    // addInputChannel passes it explicitly, since with gaps in the list the
    // number and the slot no longer coincide. Id and tracking id follow the
    // NUMBER; the position default follows the SLOT (grid layout).
    const int number = channelNumber > 0 ? channelNumber : index + 1;

    juce::ValueTree input (Input);
    input.setProperty (id, number, nullptr);
    input.setProperty (inputChannelType, inputChannelTypeMono, nullptr);

    // Born mono, so the number doubles as the mono ordinal — exact for the dense
    // all-mono list initializeDefaultState builds. A caller creating anything
    // else (addInputChannel) overwrites the type and the name together.
    input.appendChild (createInputChannelSection (false, number), nullptr);
    input.appendChild (createInputPositionSection (index, totalInputs), nullptr);
    input.appendChild (createInputAttenuationSection(), nullptr);
    input.appendChild (createInputDirectivitySection(), nullptr);
    input.appendChild (createInputLiveSourceSection(), nullptr);
    input.appendChild (createInputHackousticsSection(), nullptr);
    input.appendChild (createInputLFOSection(), nullptr);
    input.appendChild (createInputAutoMotionSection(), nullptr);
    input.appendChild (createInputMutesSection (getNumOutputChannels()), nullptr);
    input.appendChild (createInputGradientMapsSection(), nullptr);
    input.appendChild (createInputSamplerSection(), nullptr);

    // Tracking id follows the permanent number, not the slot (the position
    // section builder only knows the slot).
    auto position = input.getChildWithName (Position);
    if (position.isValid())
        position.setProperty (inputTrackingID, number, nullptr);

    return input;
}

juce::ValueTree WFSValueTreeState::createInputChannelSection (bool stereo, int ordinal)
{
    juce::ValueTree channel (Channel);
    channel.setProperty (inputName, getDefaultInputNameForType (stereo, ordinal), nullptr);
    channel.setProperty (inputColour, inputColourDefault, nullptr);   // Default: -1 = auto
    channel.setProperty (inputSolo, 0, nullptr);
    channel.setProperty (inputStereoWidth, inputStereoWidthDefault, nullptr);
    channel.setProperty (inputStereoAxisOffset, inputStereoAxisOffsetDefault, nullptr);
    channel.setProperty (inputStereoAxisLock, inputStereoAxisLockDefault, nullptr);
    channel.setProperty (inputAttenuation, inputAttenuationDefault, nullptr);
    channel.setProperty (inputDelayLatency, inputDelayLatencyDefault, nullptr);
    channel.setProperty (inputMinimalLatency, inputMinimalLatencyDefault, nullptr);
    channel.setProperty (inputMapLocked, 0, nullptr);    // Default: unlocked
    channel.setProperty (inputMapVisible, 1, nullptr);   // Default: visible
    channel.setProperty (inputHiddenByCluster, 0, nullptr);  // Default: not hidden by a cluster toggle
    channel.setProperty (inputSamplerActive, inputSamplerActiveDefault, nullptr);
    channel.setProperty (samplerMidiZoneQuadrant, samplerMidiZoneQuadrantDefault, nullptr);
    channel.setProperty (lightpadZoneId, lightpadZoneIdDefault, nullptr);
    return channel;
}

juce::ValueTree WFSValueTreeState::createInputPositionSection (int index, int totalInputs)
{
    juce::ValueTree position (Position);

    // Calculate default position
    float x, y, z;
    auto stageTree = getStageState();
    float sw = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (stageWidth)) : stageWidthDefault;
    float sd = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (stageDepth)) : stageDepthDefault;
    float sh = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (stageHeight)) : stageHeightDefault;
    float ow = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (originWidth)) : originWidthDefault;
    float od = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (originDepth)) : originDepthDefault;
    float oh = stageTree.isValid() ? static_cast<float> (stageTree.getProperty (originHeight)) : originHeightDefault;

    getDefaultInputPosition (index, totalInputs, sw, sd, sh, ow, od, oh, x, y, z);

    position.setProperty (inputPositionX, x, nullptr);
    position.setProperty (inputPositionY, y, nullptr);
    position.setProperty (inputPositionZ, z, nullptr);
    position.setProperty (inputOffsetX, inputOffsetDefault, nullptr);
    position.setProperty (inputOffsetY, inputOffsetDefault, nullptr);
    position.setProperty (inputOffsetZ, inputOffsetDefault, nullptr);
    position.setProperty (inputConstraintX, inputConstraintDefault, nullptr);
    position.setProperty (inputConstraintY, inputConstraintDefault, nullptr);
    position.setProperty (inputConstraintZ, inputConstraintDefault, nullptr);
    position.setProperty (inputConstraintDistance, inputConstraintDistanceDefault, nullptr);
    position.setProperty (inputConstraintDistanceMin, inputConstraintDistanceMinDefault, nullptr);
    position.setProperty (inputConstraintDistanceMax, inputConstraintDistanceMaxDefault, nullptr);
    position.setProperty (inputFlipX, inputFlipDefault, nullptr);
    position.setProperty (inputFlipY, inputFlipDefault, nullptr);
    position.setProperty (inputFlipZ, inputFlipDefault, nullptr);
    position.setProperty (inputCluster, inputClusterDefault, nullptr);
    position.setProperty (inputTrackingActive, inputTrackingActiveDefault, nullptr);
    position.setProperty (inputTrackingID, index + 1, nullptr);  // Default to channel index
    position.setProperty (inputTrackingSmooth, inputTrackingSmoothDefault, nullptr);
    position.setProperty (inputMaxSpeedActive, inputMaxSpeedActiveDefault, nullptr);
    position.setProperty (inputMaxSpeed, inputMaxSpeedDefault, nullptr);
    position.setProperty (inputPathModeActive, inputPathModeActiveDefault, nullptr);
    position.setProperty (inputHeightFactor, inputHeightFactorDefault, nullptr);
    position.setProperty (inputCoordinateMode, inputCoordinateModeDefault, nullptr);
    position.setProperty (inputAdmMapping, inputAdmMappingDefault, nullptr);
    position.setProperty (inputJitter, inputJitterDefault, nullptr);

    return position;
}

juce::ValueTree WFSValueTreeState::createInputAttenuationSection()
{
    juce::ValueTree attenuation (Attenuation);
    // Note: inputAttenuation itself lives in the Channel section (that is where the
    // GUI/OSC/snapshot system and the calc engine read+write it). The Attenuation
    // section only holds the distance law, ratio and common-attenuation parameters.
    attenuation.setProperty (inputAttenuationLaw, inputAttenuationLawDefault, nullptr);
    attenuation.setProperty (inputDistanceAttenuation, inputDistanceAttenuationDefault, nullptr);
    attenuation.setProperty (inputDistanceRatio, inputDistanceRatioDefault, nullptr);
    attenuation.setProperty (inputCommonAtten, inputCommonAttenDefault, nullptr);
    return attenuation;
}

juce::ValueTree WFSValueTreeState::createInputDirectivitySection()
{
    juce::ValueTree directivity (Directivity);
    directivity.setProperty (inputDirectivity, inputDirectivityDefault, nullptr);
    directivity.setProperty (inputRotation, inputRotationDefault, nullptr);
    directivity.setProperty (inputTilt, inputTiltDefault, nullptr);
    directivity.setProperty (inputHFshelf, inputHFshelfDefault, nullptr);
    return directivity;
}

juce::ValueTree WFSValueTreeState::createInputLiveSourceSection()
{
    juce::ValueTree liveSource (LiveSourceTamer);
    liveSource.setProperty (inputLSactive, inputLSactiveDefault, nullptr);
    liveSource.setProperty (inputLSradius, inputLSradiusDefault, nullptr);
    liveSource.setProperty (inputLSshape, inputLSshapeDefault, nullptr);
    liveSource.setProperty (inputLSattenuation, inputLSattenuationDefault, nullptr);
    liveSource.setProperty (inputLSpeakEnable, inputLSpeakEnableDefault, nullptr);
    liveSource.setProperty (inputLSpeakThreshold, inputLSpeakThresholdDefault, nullptr);
    liveSource.setProperty (inputLSpeakRatio, inputLSpeakRatioDefault, nullptr);
    liveSource.setProperty (inputLSslowEnable, inputLSslowEnableDefault, nullptr);
    liveSource.setProperty (inputLSslowThreshold, inputLSslowThresholdDefault, nullptr);
    liveSource.setProperty (inputLSslowRatio, inputLSslowRatioDefault, nullptr);
    return liveSource;
}

juce::ValueTree WFSValueTreeState::createInputHackousticsSection()
{
    juce::ValueTree hackoustics (Hackoustics);
    hackoustics.setProperty (inputFRactive, inputFRactiveDefault, nullptr);
    hackoustics.setProperty (inputFRattenuation, inputFRattenuationDefault, nullptr);
    hackoustics.setProperty (inputFRlowCutActive, inputFRlowCutActiveDefault, nullptr);
    hackoustics.setProperty (inputFRlowCutFreq, inputFRlowCutFreqDefault, nullptr);
    hackoustics.setProperty (inputFRhighShelfActive, inputFRhighShelfActiveDefault, nullptr);
    hackoustics.setProperty (inputFRhighShelfFreq, inputFRhighShelfFreqDefault, nullptr);
    hackoustics.setProperty (inputFRhighShelfGain, inputFRhighShelfGainDefault, nullptr);
    hackoustics.setProperty (inputFRhighShelfSlope, inputFRhighShelfSlopeDefault, nullptr);
    hackoustics.setProperty (inputFRdiffusion, inputFRdiffusionDefault, nullptr);
    return hackoustics;
}

juce::ValueTree WFSValueTreeState::createInputLFOSection()
{
    juce::ValueTree lfo (LFO);
    lfo.setProperty (inputLFOactive, inputLFOactiveDefault, nullptr);
    lfo.setProperty (inputLFOperiod, inputLFOperiodDefault, nullptr);
    lfo.setProperty (inputLFOphase, inputLFOphaseDefault, nullptr);
    lfo.setProperty (inputLFOshapeX, inputLFOshapeDefault, nullptr);
    lfo.setProperty (inputLFOshapeY, inputLFOshapeDefault, nullptr);
    lfo.setProperty (inputLFOshapeZ, inputLFOshapeDefault, nullptr);
    lfo.setProperty (inputLFOrateX, inputLFOrateDefault, nullptr);
    lfo.setProperty (inputLFOrateY, inputLFOrateDefault, nullptr);
    lfo.setProperty (inputLFOrateZ, inputLFOrateDefault, nullptr);
    lfo.setProperty (inputLFOamplitudeX, inputLFOamplitudeDefault, nullptr);
    lfo.setProperty (inputLFOamplitudeY, inputLFOamplitudeDefault, nullptr);
    lfo.setProperty (inputLFOamplitudeZ, inputLFOamplitudeDefault, nullptr);
    lfo.setProperty (inputLFOphaseX, inputLFOphaseDefault, nullptr);
    lfo.setProperty (inputLFOphaseY, inputLFOphaseDefault, nullptr);
    lfo.setProperty (inputLFOphaseZ, inputLFOphaseDefault, nullptr);
    lfo.setProperty (inputLFOgyrophone, inputLFOgyrophoneDefault, nullptr);
    return lfo;
}

juce::ValueTree WFSValueTreeState::createInputAutoMotionSection()
{
    juce::ValueTree automOtion (AutomOtion);
    automOtion.setProperty (inputOtomoX, inputOtomoDefault, nullptr);
    automOtion.setProperty (inputOtomoY, inputOtomoDefault, nullptr);
    automOtion.setProperty (inputOtomoZ, inputOtomoDefault, nullptr);
    automOtion.setProperty (inputOtomoAbsoluteRelative, inputOtomoAbsoluteRelativeDefault, nullptr);
    automOtion.setProperty (inputOtomoStayReturn, inputOtomoStayReturnDefault, nullptr);
    automOtion.setProperty (inputOtomoSpeedProfile, inputOtomoSpeedProfileDefault, nullptr);
    automOtion.setProperty (inputOtomoDuration, inputOtomoDurationDefault, nullptr);
    automOtion.setProperty (inputOtomoCurve, inputOtomoCurveDefault, nullptr);
    automOtion.setProperty (inputOtomoTrigger, inputOtomoTriggerDefault, nullptr);
    automOtion.setProperty (inputOtomoThreshold, inputOtomoThresholdDefault, nullptr);
    automOtion.setProperty (inputOtomoReset, inputOtomoResetDefault, nullptr);
    automOtion.setProperty (inputOtomoPauseResume, inputOtomoPauseResumeDefault, nullptr);

    // Polar coordinate parameters
    automOtion.setProperty (inputOtomoCoordinateMode, inputOtomoCoordinateModeDefault, nullptr);
    automOtion.setProperty (inputOtomoR, inputOtomoRDefault, nullptr);
    automOtion.setProperty (inputOtomoTheta, inputOtomoThetaDefault, nullptr);
    automOtion.setProperty (inputOtomoRsph, inputOtomoRsphDefault, nullptr);
    automOtion.setProperty (inputOtomoPhi, inputOtomoPhiDefault, nullptr);

    return automOtion;
}

juce::ValueTree WFSValueTreeState::createInputMutesSection (int numOutputs)
{
    juce::ValueTree mutes (Mutes);

    // Create comma-separated string of zeros
    juce::StringArray muteArray;
    for (int i = 0; i < numOutputs; ++i)
        muteArray.add ("0");
    mutes.setProperty (inputMutes, muteArray.joinIntoString (","), nullptr);

    // Sidelines (auto-mute at stage edges)
    mutes.setProperty (inputSidelinesActive, inputSidelinesActiveDefault, nullptr);
    mutes.setProperty (inputSidelinesFringe, inputSidelinesFringeDefault, nullptr);

    // Mute reverb sends (default 0 = sends active)
    mutes.setProperty (inputMuteReverbSends, 0, nullptr);

    // Array attenuation (per-array level control, 0 dB default)
    mutes.setProperty (inputArrayAtten1, inputArrayAttenDefault, nullptr);
    mutes.setProperty (inputArrayAtten2, inputArrayAttenDefault, nullptr);
    mutes.setProperty (inputArrayAtten3, inputArrayAttenDefault, nullptr);
    mutes.setProperty (inputArrayAtten4, inputArrayAttenDefault, nullptr);
    mutes.setProperty (inputArrayAtten5, inputArrayAttenDefault, nullptr);
    mutes.setProperty (inputArrayAtten6, inputArrayAttenDefault, nullptr);
    mutes.setProperty (inputArrayAtten7, inputArrayAttenDefault, nullptr);
    mutes.setProperty (inputArrayAtten8, inputArrayAttenDefault, nullptr);
    mutes.setProperty (inputArrayAtten9, inputArrayAttenDefault, nullptr);
    mutes.setProperty (inputArrayAtten10, inputArrayAttenDefault, nullptr);

    return mutes;
}

juce::ValueTree WFSValueTreeState::createInputGradientMapsSection()
{
    using namespace WFSParameterIDs;
    using namespace WFSParameterDefaults;

    juce::ValueTree gm (GradientMaps);

    // Create 3 layers with default parameter assignments
    const int defaultParams[3] = { 0, 1, 2 };  // Attenuation, Height, HF Shelf

    for (int i = 0; i < maxGradientLayers; ++i)
    {
        juce::ValueTree layer (GradientLayer);
        layer.setProperty (WFSParameterIDs::id, i, nullptr);
        layer.setProperty (gmLayerEnabled,  gmLayerEnabledDefault, nullptr);
        layer.setProperty (gmLayerParam,    defaultParams[i], nullptr);
        layer.setProperty (gmLayerWhite,    gmLayerWhiteDefault, nullptr);
        layer.setProperty (gmLayerBlack,    gmLayerBlackDefault, nullptr);
        layer.setProperty (gmLayerCurve,    gmLayerCurveDefault, nullptr);
        layer.setProperty (gmLayerVisible,  gmLayerVisibleDefault, nullptr);
        gm.appendChild (layer, nullptr);
    }

    return gm;
}

juce::ValueTree WFSValueTreeState::getADMCartMapping (int mappingIndex)
{
    return findChildByIntProperty (getADMOSCState(), ADMCartMapping, id, mappingIndex);
}

/** NOTE: admPolarDistBreakpoint/Inner/Outer/Center are deliberately NOT stamped by
    createADMOSCSection, unlike the stray config properties fixed elsewhere in this
    series. ADMOSCMapping::loadPolarConfig uses the ABSENCE of admPolarDistInner to
    decide whether to migrate a legacy admPolarDistMin/Max pair; stamping defaults
    would send every pre-existing file down the new-style branch with default
    widths and quietly discard its real polar mapping. A write through this
    accessor creates the property, which is exactly the intended transition. */
juce::ValueTree WFSValueTreeState::getADMPolarMapping (int mappingIndex)
{
    return findChildByIntProperty (getADMOSCState(), ADMPolarMapping, id, mappingIndex);
}

juce::ValueTree WFSValueTreeState::getADMCartAxis (int mappingIndex, int axisIndex)
{
    auto mapping = getADMCartMapping (mappingIndex);
    if (! mapping.isValid())
        return {};
    return findChildByIntProperty (mapping, ADMCartAxis, admCartAxisId, axisIndex);
}

juce::ValueTree WFSValueTreeState::createInputSamplerSection()
{
    using namespace WFSParameterIDs;
    using namespace WFSParameterDefaults;

    juce::ValueTree sampler (Sampler);

    // Pre-create 36 cells (6x6 grid) with empty defaults
    for (int i = 0; i < samplerGridCells; ++i)
    {
        juce::ValueTree cell (SamplerCell);
        cell.setProperty (WFSParameterIDs::id, i, nullptr);
        cell.setProperty (samplerCellName, "", nullptr);
        cell.setProperty (samplerCellFile, "", nullptr);
        cell.setProperty (samplerCellInTime, samplerCellInTimeDefault, nullptr);
        cell.setProperty (samplerCellOutTime, samplerCellOutTimeDefault, nullptr);
        cell.setProperty (samplerCellOffsetX, samplerCellOffsetDefault, nullptr);
        cell.setProperty (samplerCellOffsetY, samplerCellOffsetDefault, nullptr);
        cell.setProperty (samplerCellOffsetZ, samplerCellOffsetDefault, nullptr);
        cell.setProperty (samplerCellAttenuation, samplerCellAttenuationDefault, nullptr);
        sampler.appendChild (cell, nullptr);
    }

    // No sets by default — user creates them dynamically
    sampler.setProperty (inputSamplerActiveSet, inputSamplerActiveSetDefault, nullptr);

    return sampler;
}

juce::ValueTree WFSValueTreeState::createDefaultOutputChannel (int index)
{
    juce::ValueTree output (Output);
    output.setProperty (id, index + 1, nullptr);

    output.appendChild (createOutputChannelSection (index), nullptr);
    output.appendChild (createOutputPositionSection(), nullptr);
    output.appendChild (createOutputOptionsSection(), nullptr);
    output.appendChild (createOutputEQSection(), nullptr);

    return output;
}

juce::ValueTree WFSValueTreeState::createOutputChannelSection (int index)
{
    juce::ValueTree channel (Channel);
    channel.setProperty (outputName, getDefaultOutputName (index), nullptr);
    channel.setProperty (outputArray, outputArrayDefault, nullptr);
    channel.setProperty (outputApplyToArray, outputApplyToArrayDefault, nullptr);
    channel.setProperty (outputAttenuation, outputAttenuationDefault, nullptr);
    channel.setProperty (outputDelayLatency, outputDelayLatencyDefault, nullptr);
    channel.setProperty (outputMapVisible, 1, nullptr);       // Default: visible
    channel.setProperty (outputArrayMapVisible, 1, nullptr);  // Default: visible
    return channel;
}

juce::ValueTree WFSValueTreeState::createOutputPositionSection()
{
    juce::ValueTree position (Position);
    position.setProperty (outputPositionX, outputPositionDefault, nullptr);
    position.setProperty (outputPositionY, outputPositionDefault, nullptr);
    position.setProperty (outputPositionZ, outputPositionDefault, nullptr);
    position.setProperty (outputOrientation, outputOrientationDefault, nullptr);
    position.setProperty (outputAngleOn, outputAngleOnDefault, nullptr);
    position.setProperty (outputAngleOff, outputAngleOffDefault, nullptr);
    position.setProperty (outputPitch, outputPitchDefault, nullptr);
    position.setProperty (outputHFdamping, outputHFdampingDefault, nullptr);
    position.setProperty (outputCoordinateMode, outputCoordinateModeDefault, nullptr);
    return position;
}

juce::ValueTree WFSValueTreeState::createOutputOptionsSection()
{
    juce::ValueTree options (Options);
    options.setProperty (outputMiniLatencyEnable, outputMiniLatencyEnableDefault, nullptr);
    options.setProperty (outputLSattenEnable, outputLSattenEnableDefault, nullptr);
    options.setProperty (outputFRenable, outputFRenableDefault, nullptr);
    options.setProperty (outputDistanceAttenPercent, outputDistanceAttenPercentDefault, nullptr);
    options.setProperty (outputHparallax, outputHparallaxDefault, nullptr);
    options.setProperty (outputVparallax, outputVparallaxDefault, nullptr);
    return options;
}

juce::ValueTree WFSValueTreeState::createOutputEQSection()
{
    juce::ValueTree eq (EQ);
    eq.setProperty (outputEQenabled, outputEQenabledDefault, nullptr);

    for (int i = 0; i < numEQBands; ++i)
    {
        juce::ValueTree band (Band);
        band.setProperty (id, i + 1, nullptr);
        band.setProperty (eqShape, eqBandShapes[i], nullptr);
        band.setProperty (eqFrequency, eqBandFrequencies[i], nullptr);
        band.setProperty (eqGain, eqGainDefault, nullptr);
        band.setProperty (eqQ, eqQDefault, nullptr);
        band.setProperty (eqSlope, eqSlopeDefault, nullptr);
        eq.appendChild (band, nullptr);
    }

    return eq;
}

juce::ValueTree WFSValueTreeState::createDefaultReverbChannel (int index, int totalCount)
{
    juce::ValueTree reverb (Reverb);
    reverb.setProperty (id, index + 1, nullptr);

    reverb.appendChild (createReverbChannelSection (index), nullptr);
    reverb.appendChild (createReverbPositionSection (index, totalCount), nullptr);
    reverb.appendChild (createReverbFeedSection (
                            getDefaultReverbNode (index, totalCount).orientationDeg), nullptr);
    reverb.appendChild (createReverbEQSection(), nullptr);
    reverb.appendChild (createReverbReturnSection (getNumOutputChannels()), nullptr);

    return reverb;
}

bool WFSValueTreeState::arePositionsUserOwned()
{
    auto stageTree = getStageState();
    return stageTree.isValid()
        && (bool) stageTree.getProperty (positionsUserOwned, false);
}

void WFSValueTreeState::markPositionsUserOwned()
{
    auto stageTree = getStageState();
    if (stageTree.isValid() && ! (bool) stageTree.getProperty (positionsUserOwned, false))
        stageTree.setProperty (positionsUserOwned, true, nullptr);   // no undo: see header
}

bool WFSValueTreeState::areChannelNumbersUserOwned()
{
    // An invalid IO tree reads as OWNED — the opposite fallback to
    // arePositionsUserOwned. Being "unowned" here licenses a rewrite of every
    // channel id, so malformed or half-built state must land on the permanent
    // regime; a wrong "fresh" verdict would renumber a real show.
    auto io = getIOState();
    return (! io.isValid())
        || (bool) io.getProperty (channelNumbersUserOwned, false);
}

void WFSValueTreeState::markChannelNumbersUserOwned (const juce::String& reason)
{
    auto io = getIOState();
    if (io.isValid() && ! (bool) io.getProperty (channelNumbersUserOwned, false))
    {
        io.setProperty (channelNumbersUserOwned, true, nullptr);   // no undo: see header
        WFSLogger::getInstance().logInfo ("Channel numbers latched: " + reason
                                          + " (structural edits keep permanent numbers from here on)");
    }
}

void WFSValueTreeState::redistributeAllReverbPositions()
{
    const int n = getNumReverbChannels();
    if (n <= 0)
        return;

    const auto nodes = ReverbNodePlacement::layout (getStageForPlacement(), n);

    beginUndoTransaction ("Redistribute Reverb Positions");

    // Writes go through setProperty directly, like redistributeAllInputPositions:
    // the engine re-laying its own nodes must not trip the user-ownership latch
    // that setReverbParameter carries.
    for (int i = 0; i < n; ++i)
    {
        auto pos = getReverbPositionSection (i);
        if (! pos.isValid()) continue;
        const auto& node = nodes[(size_t) i];
        pos.setProperty (reverbPositionX, node.x, getActiveUndoManager());
        pos.setProperty (reverbPositionY, node.y, getActiveUndoManager());
        pos.setProperty (reverbPositionZ, node.z, getActiveUndoManager());

        // The feed direction is part of the placement, not a separate setting:
        // a node moved without its orientation keeps facing wherever it used to
        // be, and the angular attenuation can then mute the feed outright. Same
        // rule the Map tab follows when the user drags a node.
        auto feed = getReverbFeedSection (i);
        if (feed.isValid())
            feed.setProperty (reverbOrientation, node.orientationDeg, getActiveUndoManager());
    }
}

ReverbNodePlacement::Stage WFSValueTreeState::getStageForPlacement()
{
    ReverbNodePlacement::Stage s;
    auto stageTree = getStageState();
    if (! stageTree.isValid())
        return s;   // helper falls back to a nominal extent

    s.shape    = static_cast<int>   (stageTree.getProperty (stageShape,    0));
    s.width    = static_cast<float> (stageTree.getProperty (stageWidth,    stageWidthDefault));
    s.depth    = static_cast<float> (stageTree.getProperty (stageDepth,    stageDepthDefault));
    s.height   = static_cast<float> (stageTree.getProperty (stageHeight,   stageHeightDefault));
    s.diameter = static_cast<float> (stageTree.getProperty (stageDiameter, 0.0f));
    s.originW  = static_cast<float> (stageTree.getProperty (originWidth,  originWidthDefault));
    s.originD  = static_cast<float> (stageTree.getProperty (originDepth,  originDepthDefault));
    return s;
}

ReverbNodePlacement::Node WFSValueTreeState::getDefaultReverbNode (int index, int totalCount)
{
    // Recomputes the whole layout per channel rather than threading it through
    // the callers: this is a setup path and the node count is <= 32.
    const auto nodes = ReverbNodePlacement::layout (getStageForPlacement(),
                                                    juce::jmax (1, totalCount));
    return nodes[(size_t) juce::jlimit (0, (int) nodes.size() - 1, index)];
}

juce::ValueTree WFSValueTreeState::createReverbChannelSection (int index)
{
    juce::ValueTree channel (Channel);
    channel.setProperty (reverbName, getDefaultReverbName (index), nullptr);
    channel.setProperty (reverbAttenuation, reverbAttenuationDefault, nullptr);
    channel.setProperty (reverbDelayLatency, reverbDelayLatencyDefault, nullptr);
    return channel;
}

juce::ValueTree WFSValueTreeState::createReverbPositionSection (int index, int totalCount)
{
    juce::ValueTree position (Position);

    // Default layout: a semi-ellipse (box) or ring (cylinder/dome) at 1.5x the
    // stage, 2 m high, jittered to break symmetry and de-crowded in Z. The old
    // default put every node on a straight line along X at 1 m spacing with
    // Z = 0 — collinear, on the floor, and mirror-symmetrical, which also gave
    // the SDN inter-node delays of a few samples. See ReverbNodePlacement.h.
    //
    const auto n = getDefaultReverbNode (index, totalCount);

    position.setProperty (reverbPositionX, n.x, nullptr);
    position.setProperty (reverbPositionY, n.y, nullptr);
    position.setProperty (reverbPositionZ, n.z, nullptr);
    position.setProperty (reverbReturnOffsetX, reverbReturnOffsetDefault, nullptr);
    position.setProperty (reverbReturnOffsetY, reverbReturnOffsetDefault, nullptr);
    position.setProperty (reverbReturnOffsetZ, reverbReturnOffsetDefault, nullptr);
    position.setProperty (reverbCoordinateMode, reverbCoordinateModeDefault, nullptr);
    return position;
}

juce::ValueTree WFSValueTreeState::createReverbFeedSection (int orientationDeg)
{
    juce::ValueTree feed (Feed);

    // Not reverbOrientationDefault: a fresh session laid its nodes on an arc
    // around the stage, and leaving every one of them facing 0 deg pointed the
    // upstage half of the arc away from the stage. The default layout supplies
    // the matching bearing so the nodes face outwards from the origin and show
    // their green side to the stage, exactly as they do when placed by hand.
    feed.setProperty (reverbOrientation, orientationDeg, nullptr);
    feed.setProperty (reverbAngleOn, reverbAngleOnDefault, nullptr);
    feed.setProperty (reverbAngleOff, reverbAngleOffDefault, nullptr);
    feed.setProperty (reverbPitch, reverbPitchDefault, nullptr);
    feed.setProperty (reverbHFdamping, reverbHFdampingDefault, nullptr);
    feed.setProperty (reverbMiniLatencyEnable, reverbMiniLatencyEnableDefault, nullptr);
    feed.setProperty (reverbDistanceAttenEnable, reverbDistanceAttenEnableDefault, nullptr);
    return feed;
}

juce::ValueTree WFSValueTreeState::createReverbEQSection()
{
    juce::ValueTree eq (EQ);
    eq.setProperty (reverbPreEQenable, reverbPreEQenableDefault, nullptr);

    for (int i = 0; i < numReverbPreEQBands; ++i)
    {
        juce::ValueTree band (Band);
        band.setProperty (id, i + 1, nullptr);
        band.setProperty (reverbPreEQshape, reverbPreEQBandShapes[i], nullptr);
        band.setProperty (reverbPreEQfreq, reverbPreEQBandFrequencies[i], nullptr);
        band.setProperty (reverbPreEQgain, reverbPreEQgainDefault, nullptr);
        band.setProperty (reverbPreEQq, reverbPreEQqDefault, nullptr);
        band.setProperty (reverbPreEQslope, reverbPreEQslopeDefault, nullptr);
        eq.appendChild (band, nullptr);
    }

    return eq;
}

juce::ValueTree WFSValueTreeState::createReverbReturnSection (int numOutputs)
{
    juce::ValueTree returnSection (ReverbReturn);
    returnSection.setProperty (reverbDistanceAttenuation, reverbDistanceAttenuationDefault, nullptr);
    returnSection.setProperty (reverbCommonAtten, reverbCommonAttenDefault, nullptr);

    // Create comma-separated string of zeros for mutes
    juce::StringArray muteArray;
    int outputCount = numOutputs > 0 ? numOutputs : outputChannelsDefault;
    for (int i = 0; i < outputCount; ++i)
        muteArray.add ("0");
    returnSection.setProperty (reverbMutes, muteArray.joinIntoString (","), nullptr);

    returnSection.setProperty (reverbMuteMacro, reverbMuteMacroDefault, nullptr);
    return returnSection;
}

juce::ValueTree WFSValueTreeState::createReverbAlgorithmSection()
{
    juce::ValueTree algo (ReverbAlgorithm);
    algo.setProperty (reverbAlgoType,        reverbAlgoTypeDefault, nullptr);
    algo.setProperty (reverbRT60,            reverbRT60Default, nullptr);
    algo.setProperty (reverbRT60LowMult,     reverbRT60LowMultDefault, nullptr);
    algo.setProperty (reverbRT60HighMult,    reverbRT60HighMultDefault, nullptr);
    algo.setProperty (reverbCrossoverLow,    reverbCrossoverLowDefault, nullptr);
    algo.setProperty (reverbCrossoverHigh,   reverbCrossoverHighDefault, nullptr);
    algo.setProperty (reverbDiffusion,       reverbDiffusionDefault, nullptr);
    algo.setProperty (reverbSDNscale,        reverbSDNscaleDefault, nullptr);
    algo.setProperty (reverbFDNsize,         reverbFDNsizeDefault, nullptr);
    algo.setProperty (reverbIRfile,          "", nullptr);
    algo.setProperty (reverbIRtrim,          reverbIRtrimDefault, nullptr);
    algo.setProperty (reverbIRlength,        reverbIRlengthDefault, nullptr);
    algo.setProperty (reverbPerNodeIR,       reverbPerNodeIRDefault, nullptr);
    algo.setProperty (reverbIRGpuDevice,     reverbIRGpuDeviceDefault, nullptr);
    algo.setProperty (reverbFDNGpuDevice,    reverbFDNGpuDeviceDefault, nullptr);
    algo.setProperty (reverbSDNGpuDevice,    reverbSDNGpuDeviceDefault, nullptr);
    algo.setProperty (reverbWetLevel,        reverbWetLevelDefault, nullptr);
    return algo;
}

juce::ValueTree WFSValueTreeState::createReverbPostEQSection()
{
    juce::ValueTree postEQ (ReverbPostEQ);
    postEQ.setProperty (reverbPostEQenable, reverbPostEQenableDefault, nullptr);

    for (int i = 0; i < numReverbPostEQBands; ++i)
    {
        juce::ValueTree band (PostEQBand);
        band.setProperty (id, i + 1, nullptr);
        band.setProperty (reverbPostEQshape, reverbPostEQBandShapes[i], nullptr);
        band.setProperty (reverbPostEQfreq, reverbPostEQBandFrequencies[i], nullptr);
        band.setProperty (reverbPostEQgain, reverbPostEQgainDefault, nullptr);
        band.setProperty (reverbPostEQq, reverbPostEQqDefault, nullptr);
        band.setProperty (reverbPostEQslope, reverbPostEQslopeDefault, nullptr);
        postEQ.appendChild (band, nullptr);
    }

    return postEQ;
}

juce::ValueTree WFSValueTreeState::createReverbPreCompSection()
{
    using namespace WFSParameterIDs;
    using namespace WFSParameterDefaults;
    juce::ValueTree preComp (ReverbPreComp);
    preComp.setProperty (reverbPreCompBypass,    reverbPreCompBypassDefault, nullptr);
    preComp.setProperty (reverbPreCompThreshold, reverbPreCompThresholdDefault, nullptr);
    preComp.setProperty (reverbPreCompRatio,     reverbPreCompRatioDefault, nullptr);
    preComp.setProperty (reverbPreCompAttack,    reverbPreCompAttackDefault, nullptr);
    preComp.setProperty (reverbPreCompRelease,   reverbPreCompReleaseDefault, nullptr);
    return preComp;
}

juce::ValueTree WFSValueTreeState::createReverbPostExpSection()
{
    using namespace WFSParameterIDs;
    using namespace WFSParameterDefaults;
    juce::ValueTree postExp (ReverbPostExp);
    postExp.setProperty (reverbPostExpBypass,    reverbPostExpBypassDefault, nullptr);
    postExp.setProperty (reverbPostExpThreshold, reverbPostExpThresholdDefault, nullptr);
    postExp.setProperty (reverbPostExpRatio,     reverbPostExpRatioDefault, nullptr);
    postExp.setProperty (reverbPostExpAttack,    reverbPostExpAttackDefault, nullptr);
    postExp.setProperty (reverbPostExpRelease,   reverbPostExpReleaseDefault, nullptr);
    return postExp;
}

juce::ValueTree WFSValueTreeState::createDefaultEffectChannel (int index, int totalCount)
{
    juce::ValueTree effect (Effect);
    effect.setProperty (id, index + 1, nullptr);

    // ONE layout call for the whole channel. The position and the feed bearing
    // are two halves of one placement - a node that keeps its old bearing after
    // a move can have its feed muted outright by the angular attenuation - and
    // recomputing the ring inside each builder is what makes the reverb twin
    // quadratic (it lays the whole set out twice per channel).
    const auto nodes = layoutEffectNodes (juce::jmax (1, totalCount));
    const auto node  = nodes[(size_t) juce::jlimit (0, (int) nodes.size() - 1, index)];

    effect.appendChild (createEffectChannelSection (index), nullptr);
    effect.appendChild (createEffectPositionSection (node), nullptr);
    effect.appendChild (createEffectFeedSection (node.orientationDeg), nullptr);
    effect.appendChild (createEffectReturnSection (getNumOutputChannels()), nullptr);
    effect.appendChild (createEffectAutoMotionSection(), nullptr);
    effect.appendChild (createEffectLFOSection(), nullptr);
    effect.appendChild (createEffectChainSection(), nullptr);

    // The eleven chain slots, in the declared order, so the child list reads
    // like spatcore::effects::kSlots. Each is its own id-less node type: two
    // <FxEQ id="1"/"2"> siblings would put two node types in one id namespace,
    // repeat every EQ property name on a sibling, and make every first-hit
    // search answer instance 1 while reporting success.
    effect.appendChild (createEffectDistSection(), nullptr);
    effect.appendChild (createEffectEQSection (FxEq1), nullptr);
    effect.appendChild (createEffectEQSection (FxEq2), nullptr);
    effect.appendChild (createEffectDynSection (FxDyn1), nullptr);
    effect.appendChild (createEffectDynSection (FxDyn2), nullptr);
    effect.appendChild (createEffectModSection(), nullptr);
    effect.appendChild (createEffectPhaserSection(), nullptr);
    effect.appendChild (createEffectTremSection(), nullptr);
    effect.appendChild (createEffectReverbSection(), nullptr);
    effect.appendChild (createEffectDelaySection(), nullptr);
    effect.appendChild (createEffectCrushSection(), nullptr);

    effect.appendChild (createEffectSendsSection (index), nullptr);

    return effect;
}

int WFSValueTreeState::getDefaultEffectLinkMode() const
{
    // Reads the global, falls back to the constant on a half-built tree. The
    // fallback matters: createEffectChannelSection runs from the schema
    // template as well as from addEffectChannel, and the template is built
    // before <Config><EffectsGlobal> exists on some paths.
    auto globals = getEffectsGlobalSection();
    if (! globals.isValid() || ! globals.hasProperty (effectsGlobalLinkMode))
        return effectLinkModeDefault;

    return juce::jlimit (effectLinkModeMin, effectLinkModeMax,
                         WFSVar::toInt (globals.getProperty (effectsGlobalLinkMode), effectLinkModeDefault));
}

const juce::Identifier& WFSValueTreeState::getEffectArrayAttenId (int arrayIndex)
{
    // Index 0..9 -> effectArrayAtten1..10. A table, never a name built from a
    // string: effectArrayAtten1 is a strict prefix of effectArrayAtten10, and
    // an Identifier assembled at runtime would also allocate on every call.
    static const juce::Identifier* const ids[10] = {
        &effectArrayAtten1, &effectArrayAtten2, &effectArrayAtten3, &effectArrayAtten4,
        &effectArrayAtten5, &effectArrayAtten6, &effectArrayAtten7, &effectArrayAtten8,
        &effectArrayAtten9, &effectArrayAtten10
    };

    return *ids[juce::jlimit (0, 9, arrayIndex)];
}

bool WFSValueTreeState::isEffectArrayAttenId (const juce::Identifier& paramId)
{
    for (int a = 0; a < 10; ++a)
        if (paramId == getEffectArrayAttenId (a))
            return true;

    return false;
}

juce::ValueTree WFSValueTreeState::createEffectChannelSection (int index)
{
    juce::ValueTree channel (Channel);
    channel.setProperty (effectName, getDefaultEffectName (index), nullptr);
    channel.setProperty (effectAttenuation, effectAttenuationDefault, nullptr);
    channel.setProperty (effectDelayLatency, effectDelayLatencyDefault, nullptr);
    channel.setProperty (effectMinimalLatency, effectMinimalLatencyDefault, nullptr);
    channel.setProperty (effectLinkGroup, effectLinkGroupDefault, nullptr);

    // The link MODE is per channel (R5-5), stamped from the global rather than
    // read from it. The global is nothing more than "what a new channel gets":
    // with only a global, detaching one channel from its group detached every
    // group at once, which is the one thing an output array can already do and
    // effects could not. A stamped copy is also what lets a Stream Deck or an
    // OSC client detach a single channel - a per-channel mode is an ordinary
    // parameter, a global is not.
    channel.setProperty (effectLinkMode, getDefaultEffectLinkMode(), nullptr);

    channel.setProperty (effectMute, effectMuteDefault, nullptr);
    channel.setProperty (effectSolo, effectSoloDefault, nullptr);
    return channel;
}

juce::ValueTree WFSValueTreeState::createEffectPositionSection (const ReverbNodePlacement::Node& node)
{
    juce::ValueTree position (Position);

    // The placement, not effectPositionDefault. A channel born at the origin is
    // the failure mode the family's own ownership latch exists to prevent: it
    // stacks every effect return on one spot, where the feed geometry and the
    // inter-node spacing both stop meaning anything.
    position.setProperty (effectPositionX, node.x, nullptr);
    position.setProperty (effectPositionY, node.y, nullptr);
    position.setProperty (effectPositionZ, node.z, nullptr);
    position.setProperty (effectCoordinateMode, effectCoordinateModeDefault, nullptr);
    position.setProperty (effectReturnOffsetX, effectReturnOffsetDefault, nullptr);
    position.setProperty (effectReturnOffsetY, effectReturnOffsetDefault, nullptr);
    position.setProperty (effectReturnOffsetZ, effectReturnOffsetDefault, nullptr);
    return position;
}

juce::ValueTree WFSValueTreeState::createEffectFeedSection (int orientationDeg)
{
    juce::ValueTree feed (Feed);

    // The bearing comes from the placement for the same reason the reverb feed
    // takes one as an argument: leaving every node facing 0 deg points the
    // upstage half of the ring away from the stage.
    feed.setProperty (effectOrientation, orientationDeg, nullptr);
    feed.setProperty (effectAngleOn, effectAngleOnDefault, nullptr);
    feed.setProperty (effectAngleOff, effectAngleOffDefault, nullptr);
    feed.setProperty (effectPitch, effectPitchDefault, nullptr);
    feed.setProperty (effectHFdamping, effectHFdampingDefault, nullptr);
    feed.setProperty (effectFeedMiniLatency, effectFeedMiniLatencyDefault, nullptr);
    feed.setProperty (effectDistanceAttenPercent, effectDistanceAttenPercentDefault, nullptr);
    return feed;
}

juce::ValueTree WFSValueTreeState::createEffectReturnSection (int numOutputs)
{
    // ReverbReturn is the C++ name of the identifier; its XML tag is "Return".
    juce::ValueTree returnSection (ReverbReturn);
    returnSection.setProperty (effectAttenuationLaw, effectAttenuationLawDefault, nullptr);
    returnSection.setProperty (effectDistanceAttenuation, effectDistanceAttenuationDefault, nullptr);
    returnSection.setProperty (effectDistanceRatio, effectDistanceRatioDefault, nullptr);
    returnSection.setProperty (effectCommonAtten, effectCommonAttenDefault, nullptr);
    returnSection.setProperty (effectHFshelf, effectHFshelfDefault, nullptr);

    // One token per output, all unmuted. A packed CSV row like inputMutes and
    // reverbMutes, and guarded like them now: a bare number written over it
    // leaves the row alone, and setNumOutputChannels refits its width when the
    // rig changes. Per-OUTPUT, which is what separates it from the four <Sends>
    // rows next door - those are keyed by input number and by effect index and
    // never follow the output count.
    juce::StringArray muteArray;
    const int outputCount = numOutputs > 0 ? numOutputs : outputChannelsDefault;
    for (int i = 0; i < outputCount; ++i)
        muteArray.add ("0");
    returnSection.setProperty (effectMutes, muteArray.joinIntoString (","), nullptr);

    returnSection.setProperty (effectMuteMacro, effectMuteMacroDefault, nullptr);
    returnSection.setProperty (effectMuteReverbSends, effectMuteReverbSendsDefault, nullptr);

    // Level 3's trim (R5-4). Ten properties, one per output array, applied to
    // the return rows by the calculation engine against outputArrayAssignments.
    // They complete the third matrix level: it had a per-output MUTE and no
    // level at all, while levels 1 and 2 each carry an on/off row AND a level
    // row. Deliberately not a free per-output matrix - a return is a render
    // source, so its per-output gains are solved from geometry, and an
    // arbitrary per-output level would overwrite the spatialisation.
    for (int a = 0; a < 10; ++a)
        returnSection.setProperty (getEffectArrayAttenId (a), effectArrayAttenDefault, nullptr);

    return returnSection;
}

juce::ValueTree WFSValueTreeState::createEffectAutoMotionSection()
{
    juce::ValueTree otomo (AutomOtion);
    otomo.setProperty (effectOtomoX, effectOtomoDefault, nullptr);
    otomo.setProperty (effectOtomoY, effectOtomoDefault, nullptr);
    otomo.setProperty (effectOtomoZ, effectOtomoDefault, nullptr);
    otomo.setProperty (effectOtomoAbsoluteRelative, effectOtomoAbsoluteRelativeDefault, nullptr);
    otomo.setProperty (effectOtomoSpeedProfile, effectOtomoSpeedProfileDefault, nullptr);
    otomo.setProperty (effectOtomoDuration, effectOtomoDurationDefault, nullptr);
    otomo.setProperty (effectOtomoCurve, effectOtomoCurveDefault, nullptr);
    otomo.setProperty (effectOtomoTrigger, effectOtomoTriggerDefault, nullptr);
    otomo.setProperty (effectOtomoThreshold, effectOtomoThresholdDefault, nullptr);
    otomo.setProperty (effectOtomoReset, effectOtomoResetDefault, nullptr);
    otomo.setProperty (effectOtomoPauseResume, effectOtomoPauseResumeDefault, nullptr);
    otomo.setProperty (effectOtomoCoordinateMode, effectOtomoCoordinateModeDefault, nullptr);
    otomo.setProperty (effectOtomoR, effectOtomoRDefault, nullptr);
    otomo.setProperty (effectOtomoTheta, effectOtomoThetaDefault, nullptr);
    otomo.setProperty (effectOtomoRsph, effectOtomoRsphDefault, nullptr);
    otomo.setProperty (effectOtomoPhi, effectOtomoPhiDefault, nullptr);
    return otomo;
}

juce::ValueTree WFSValueTreeState::createEffectLFOSection()
{
    // The input LFO section minus gyrophone. An older effects.xml without the
    // node gains it through backfillEffectChannelsFromTemplate, which walks
    // this template child by child.
    juce::ValueTree lfo (LFO);
    lfo.setProperty (effectLFOactive, effectLFOactiveDefault, nullptr);
    lfo.setProperty (effectLFOperiod, effectLFOperiodDefault, nullptr);
    lfo.setProperty (effectLFOphase, effectLFOphaseDefault, nullptr);
    lfo.setProperty (effectLFOshapeX, effectLFOshapeDefault, nullptr);
    lfo.setProperty (effectLFOshapeY, effectLFOshapeDefault, nullptr);
    lfo.setProperty (effectLFOshapeZ, effectLFOshapeDefault, nullptr);
    lfo.setProperty (effectLFOrateX, effectLFOrateDefault, nullptr);
    lfo.setProperty (effectLFOrateY, effectLFOrateDefault, nullptr);
    lfo.setProperty (effectLFOrateZ, effectLFOrateDefault, nullptr);
    lfo.setProperty (effectLFOamplitudeX, effectLFOamplitudeDefault, nullptr);
    lfo.setProperty (effectLFOamplitudeY, effectLFOamplitudeDefault, nullptr);
    lfo.setProperty (effectLFOamplitudeZ, effectLFOamplitudeDefault, nullptr);
    lfo.setProperty (effectLFOphaseX, effectLFOphaseDefault, nullptr);
    lfo.setProperty (effectLFOphaseY, effectLFOphaseDefault, nullptr);
    lfo.setProperty (effectLFOphaseZ, effectLFOphaseDefault, nullptr);
    return lfo;
}

juce::ValueTree WFSValueTreeState::createEffectChainSection()
{
    juce::ValueTree chain (Chain);

    // A permutation of the eleven slot tokens, in their declared order. Stored
    // as a plain string in this commit; validating it against
    // spatcore::effects::parseChainOrder is a decision for the commit that
    // teaches the write interceptor its first string rule.
    chain.setProperty (effectChainOrder, effectChainOrderDefault, nullptr);
    chain.setProperty (effectChainBypass, effectChainBypassDefault, nullptr);
    return chain;
}

juce::ValueTree WFSValueTreeState::createEffectSendsSection (int channelIndex)
{
    juce::ValueTree sends (Sends);

    // ALL FOUR ROWS, AT THEIR FIXED WIDTHS. The node used to be built empty,
    // and the comment here recorded what a row wanted before anything could
    // stamp one: a width, a keying convention, cell accessors, an interceptor
    // clause each and column maintenance on input delete and renumber. That list
    // is the changelog of this commit - all of it exists now, so the rows do.
    //
    // Stamping them is what retires the eviction exemption too. A property the
    // schema DECLARES but no builder stamps is indistinguishable from a retired
    // one by the only evidence stripObsoleteEffectProperties has (absence from a
    // freshly built channel), which is why those four names had to be exempted
    // by hand while this returned a bare node. The template carries them now, so
    // the hand-maintained list is gone and a send row name found anywhere else is
    // a genuine ghost again.
    //
    // The defaults are silence, in two halves: every level is unity (0 dB) and
    // every switch is off, so a channel is born routed to nothing and one switch
    // is all it takes to hear a source at the level the grid already shows.
    for (auto rowId : { effectSendLevels, effectSendOns, effectFxSendLevels, effectFxSendOns })
        sends.setProperty (rowId, canonicalEffectSendRow (rowId, {}, channelIndex), nullptr);

    return sends;
}

juce::ValueTree WFSValueTreeState::createEffectDistSection()
{
    juce::ValueTree dist (FxDist);
    dist.setProperty (effectDistBypass, effectDistBypassDefault, nullptr);
    dist.setProperty (effectDistDrive, effectDistDriveDefault, nullptr);
    dist.setProperty (effectDistShape, effectDistShapeDefault, nullptr);
    dist.setProperty (effectDistBias, effectDistBiasDefault, nullptr);
    dist.setProperty (effectDistPreLoShelfFreq, effectDistPreLoShelfFreqDefault, nullptr);
    dist.setProperty (effectDistPreLoShelfGain, effectDistPreLoShelfGainDefault, nullptr);
    dist.setProperty (effectDistPreHiShelfFreq, effectDistPreHiShelfFreqDefault, nullptr);
    dist.setProperty (effectDistPreHiShelfGain, effectDistPreHiShelfGainDefault, nullptr);
    dist.setProperty (effectDistPostLoShelfFreq, effectDistPostLoShelfFreqDefault, nullptr);
    dist.setProperty (effectDistPostLoShelfGain, effectDistPostLoShelfGainDefault, nullptr);
    dist.setProperty (effectDistPostHiShelfFreq, effectDistPostHiShelfFreqDefault, nullptr);
    dist.setProperty (effectDistPostHiShelfGain, effectDistPostHiShelfGainDefault, nullptr);
    dist.setProperty (effectDistOutput, effectDistOutputDefault, nullptr);
    dist.setProperty (effectDistMix, effectDistMixDefault, nullptr);
    dist.setProperty (effectDistOversample, effectDistOversampleDefault, nullptr);
    return dist;
}

juce::ValueTree WFSValueTreeState::createEffectEQSection (const juce::Identifier& nodeType)
{
    // One builder, two node types. FxEq1 and FxEq2 are identical apart from
    // their type - the type IS the instance - so the defaults are stamped once
    // and the caller says which slot is being built.
    juce::ValueTree eq (nodeType);
    eq.setProperty (effectEQBypass, effectEQBypassDefault, nullptr);

    for (int i = 0; i < numEffectEQBands; ++i)
    {
        juce::ValueTree band (Band);
        band.setProperty (id, i + 1, nullptr);
        band.setProperty (effectEQshape, effectEQBandShapes[i], nullptr);
        band.setProperty (effectEQfreq, effectEQBandFrequencies[i], nullptr);
        band.setProperty (effectEQgain, effectEQgainDefault, nullptr);
        band.setProperty (effectEQq, effectEQqDefault, nullptr);
        band.setProperty (effectEQslope, effectEQslopeDefault, nullptr);
        eq.appendChild (band, nullptr);
    }

    return eq;
}

juce::ValueTree WFSValueTreeState::createEffectDynSection (const juce::Identifier& nodeType)
{
    juce::ValueTree dyn (nodeType);   // FxDyn1 or FxDyn2 - see createEffectEQSection
    dyn.setProperty (effectDynBypass, effectDynBypassDefault, nullptr);
    dyn.setProperty (effectDynDetector, effectDynDetectorDefault, nullptr);
    dyn.setProperty (effectDynLookahead, effectDynLookaheadDefault, nullptr);
    dyn.setProperty (effectDynMakeup, effectDynMakeupDefault, nullptr);
    dyn.setProperty (effectDynAutoMakeup, effectDynAutoMakeupDefault, nullptr);

    dyn.setProperty (effectDynCompOn, effectDynCompOnDefault, nullptr);
    dyn.setProperty (effectDynCompThreshold, effectDynCompThresholdDefault, nullptr);
    dyn.setProperty (effectDynCompRatio, effectDynCompRatioDefault, nullptr);
    dyn.setProperty (effectDynCompKnee, effectDynCompKneeDefault, nullptr);
    dyn.setProperty (effectDynCompAttack, effectDynCompAttackDefault, nullptr);
    dyn.setProperty (effectDynCompRelease, effectDynCompReleaseDefault, nullptr);
    dyn.setProperty (effectDynCompDetectorDelay, effectDynCompDetectorDelayDefault, nullptr);
    dyn.setProperty (effectDynCompScLoCut, effectDynCompScLoCutDefault, nullptr);
    dyn.setProperty (effectDynCompScHiCut, effectDynCompScHiCutDefault, nullptr);

    dyn.setProperty (effectDynExpOn, effectDynExpOnDefault, nullptr);
    dyn.setProperty (effectDynExpThreshold, effectDynExpThresholdDefault, nullptr);
    dyn.setProperty (effectDynExpRatio, effectDynExpRatioDefault, nullptr);
    dyn.setProperty (effectDynExpAttack, effectDynExpAttackDefault, nullptr);
    dyn.setProperty (effectDynExpRelease, effectDynExpReleaseDefault, nullptr);
    dyn.setProperty (effectDynExpRange, effectDynExpRangeDefault, nullptr);
    dyn.setProperty (effectDynExpHold, effectDynExpHoldDefault, nullptr);
    dyn.setProperty (effectDynExpScLoCut, effectDynExpScLoCutDefault, nullptr);
    dyn.setProperty (effectDynExpScHiCut, effectDynExpScHiCutDefault, nullptr);
    return dyn;
}

juce::ValueTree WFSValueTreeState::createEffectModSection()
{
    juce::ValueTree mod (FxMod);
    mod.setProperty (effectModBypass, effectModBypassDefault, nullptr);
    mod.setProperty (effectModMode, effectModModeDefault, nullptr);
    mod.setProperty (effectModRate, effectModRateDefault, nullptr);
    mod.setProperty (effectModDepth, effectModDepthDefault, nullptr);
    mod.setProperty (effectModDelay, effectModDelayDefault, nullptr);
    mod.setProperty (effectModFeedback, effectModFeedbackDefault, nullptr);
    mod.setProperty (effectModVoices, effectModVoicesDefault, nullptr);
    mod.setProperty (effectModShape, effectModShapeDefault, nullptr);
    mod.setProperty (effectModPhase, effectModPhaseDefault, nullptr);
    mod.setProperty (effectModLoCut, effectModLoCutDefault, nullptr);
    mod.setProperty (effectModThroughZero, effectModThroughZeroDefault, nullptr);
    mod.setProperty (effectModMix, effectModMixDefault, nullptr);
    return mod;
}

juce::ValueTree WFSValueTreeState::createEffectPhaserSection()
{
    juce::ValueTree phaser (FxPhaser);
    phaser.setProperty (effectPhaserBypass, effectPhaserBypassDefault, nullptr);
    phaser.setProperty (effectPhaserStages, effectPhaserStagesDefault, nullptr);
    phaser.setProperty (effectPhaserCentre, effectPhaserCentreDefault, nullptr);
    phaser.setProperty (effectPhaserSpread, effectPhaserSpreadDefault, nullptr);
    phaser.setProperty (effectPhaserRate, effectPhaserRateDefault, nullptr);
    phaser.setProperty (effectPhaserDepth, effectPhaserDepthDefault, nullptr);
    phaser.setProperty (effectPhaserShape, effectPhaserShapeDefault, nullptr);
    phaser.setProperty (effectPhaserFeedback, effectPhaserFeedbackDefault, nullptr);
    phaser.setProperty (effectPhaserMix, effectPhaserMixDefault, nullptr);
    return phaser;
}

juce::ValueTree WFSValueTreeState::createEffectTremSection()
{
    juce::ValueTree trem (FxTrem);
    trem.setProperty (effectTremBypass, effectTremBypassDefault, nullptr);
    trem.setProperty (effectTremRate, effectTremRateDefault, nullptr);
    trem.setProperty (effectTremDepth, effectTremDepthDefault, nullptr);
    trem.setProperty (effectTremShape, effectTremShapeDefault, nullptr);
    trem.setProperty (effectTremMix, effectTremMixDefault, nullptr);
    return trem;
}

juce::ValueTree WFSValueTreeState::createEffectReverbSection()
{
    // The per-chain reverb MODULE. Unrelated to the <Reverbs> family, and its
    // properties are named effectReverb* precisely so the two never collide.
    juce::ValueTree reverb (FxReverb);
    reverb.setProperty (effectReverbBypass, effectReverbBypassDefault, nullptr);
    reverb.setProperty (effectReverbModel, effectReverbModelDefault, nullptr);
    reverb.setProperty (effectReverbType, effectReverbTypeDefault, nullptr);
    reverb.setProperty (effectReverbPredelay, effectReverbPredelayDefault, nullptr);
    reverb.setProperty (effectReverbRT60, effectReverbRT60Default, nullptr);
    reverb.setProperty (effectReverbRT60LowMult, effectReverbRT60LowMultDefault, nullptr);
    reverb.setProperty (effectReverbRT60HighMult, effectReverbRT60HighMultDefault, nullptr);
    reverb.setProperty (effectReverbCrossoverLow, effectReverbCrossoverLowDefault, nullptr);
    reverb.setProperty (effectReverbCrossoverHigh, effectReverbCrossoverHighDefault, nullptr);
    reverb.setProperty (effectReverbDiffusion, effectReverbDiffusionDefault, nullptr);
    reverb.setProperty (effectReverbSize, effectReverbSizeDefault, nullptr);
    reverb.setProperty (effectReverbTone, effectReverbToneDefault, nullptr);
    reverb.setProperty (effectReverbMix, effectReverbMixDefault, nullptr);
    reverb.setProperty (effectReverbERProfile, effectReverbERProfileDefault, nullptr);
    reverb.setProperty (effectReverbERLevel, effectReverbERLevelDefault, nullptr);
    reverb.setProperty (effectReverbModRate, effectReverbModRateDefault, nullptr);
    reverb.setProperty (effectReverbModDepth, effectReverbModDepthDefault, nullptr);
    reverb.setProperty (effectReverbShimmerPitch, effectReverbShimmerPitchDefault, nullptr);
    reverb.setProperty (effectReverbShimmerAmount, effectReverbShimmerAmountDefault, nullptr);
    return reverb;
}

juce::ValueTree WFSValueTreeState::createEffectDelaySection()
{
    juce::ValueTree delay (FxDelay);
    delay.setProperty (effectDelayBypass, effectDelayBypassDefault, nullptr);
    delay.setProperty (effectDelayTime, effectDelayTimeDefault, nullptr);
    delay.setProperty (effectDelayTaps, effectDelayTapsDefault, nullptr);
    delay.setProperty (effectDelayTapMode, effectDelayTapModeDefault, nullptr);
    delay.setProperty (effectDelayPattern, effectDelayPatternDefault, nullptr);
    delay.setProperty (effectDelayFeedback, effectDelayFeedbackDefault, nullptr);
    delay.setProperty (effectDelayFeedbackTap, effectDelayFeedbackTapDefault, nullptr);
    delay.setProperty (effectDelayInLoCut, effectDelayInLoCutDefault, nullptr);
    delay.setProperty (effectDelayFbLoShelfFreq, effectDelayFbLoShelfFreqDefault, nullptr);
    delay.setProperty (effectDelayFbLoShelfGain, effectDelayFbLoShelfGainDefault, nullptr);
    delay.setProperty (effectDelayFbHiShelfFreq, effectDelayFbHiShelfFreqDefault, nullptr);
    delay.setProperty (effectDelayFbHiShelfGain, effectDelayFbHiShelfGainDefault, nullptr);
    delay.setProperty (effectDelayModRate, effectDelayModRateDefault, nullptr);
    delay.setProperty (effectDelayModDepth, effectDelayModDepthDefault, nullptr);
    delay.setProperty (effectDelayDiffusion, effectDelayDiffusionDefault, nullptr);
    delay.setProperty (effectDelayGlide, effectDelayGlideDefault, nullptr);
    delay.setProperty (effectDelayMix, effectDelayMixDefault, nullptr);

    // All eight taps always exist; effectDelayTaps says how many are live.
    // A fixed child count is what lets the schema backfill match them by id.
    // It does NOT make getEffectDelayTap a straight positional index: a merged
    // file can leave an unknown node in here, and indexing would then hand the
    // caller the wrong tap - see nthChildOfType.
    for (int i = 0; i < numEffectDelayTaps; ++i)
    {
        juce::ValueTree tap (Tap);
        tap.setProperty (id, i + 1, nullptr);
        tap.setProperty (effectDelayTapTime, effectDelayTapTimes[i], nullptr);
        tap.setProperty (effectDelayTapLevel, effectDelayTapLevels[i], nullptr);
        delay.appendChild (tap, nullptr);
    }

    return delay;
}

juce::ValueTree WFSValueTreeState::createEffectCrushSection()
{
    juce::ValueTree crush (FxCrush);
    crush.setProperty (effectCrushBypass, effectCrushBypassDefault, nullptr);
    crush.setProperty (effectCrushBits, effectCrushBitsDefault, nullptr);
    crush.setProperty (effectCrushRate, effectCrushRateDefault, nullptr);
    crush.setProperty (effectCrushFilter, effectCrushFilterDefault, nullptr);
    crush.setProperty (effectCrushDither, effectCrushDitherDefault, nullptr);
    crush.setProperty (effectCrushMix, effectCrushMixDefault, nullptr);
    return crush;
}

juce::ValueTree WFSValueTreeState::createDefaultNetworkTarget (int index)
{
    juce::ValueTree target (NetworkTarget);
    target.setProperty (id, index + 1, nullptr);
    target.setProperty (networkTSname, networkTSnameDefault + " " + juce::String (index + 1), nullptr);
    target.setProperty (networkTSdataMode, networkTSdataModeDefault, nullptr);
    target.setProperty (networkTSip, networkTSipDefault, nullptr);
    target.setProperty (networkTSport, networkTSportDefault + index, nullptr);
    target.setProperty (networkTSrxEnable, networkTSrxEnableDefault, nullptr);
    target.setProperty (networkTStxEnable, networkTStxEnableDefault, nullptr);
    target.setProperty (networkTSProtocol, networkTSProtocolDefault, nullptr);
    return target;
}

//==============================================================================
// Helper Methods
//==============================================================================

juce::ValueTree WFSValueTreeState::getTreeForParameter (const juce::Identifier& paramId, int channelIndex) const
{
    auto scope = getParameterScope (paramId);
    auto& mutableState = const_cast<juce::ValueTree&> (state);

    switch (scope)
    {
        case ParameterScope::Config:
        {
            // Check each config subsection
            auto config = mutableState.getChildWithName (Config);
            if (!config.isValid())
                return {};

            // Show section
            auto show = config.getChildWithName (Show);
            if (show.hasProperty (paramId))
                return show;

            // IO section
            auto io = config.getChildWithName (IO);
            if (io.hasProperty (paramId))
                return io;

            // Stage section
            auto stage = config.getChildWithName (Stage);
            if (stage.hasProperty (paramId))
                return stage;

            // Master section
            auto master = config.getChildWithName (Master);
            if (master.hasProperty (paramId))
                return master;

            // Network section
            auto network = config.getChildWithName (Network);
            if (network.hasProperty (paramId))
                return network;

            // ADM-OSC section
            auto admosc = config.getChildWithName (ADMOSC);
            if (admosc.hasProperty (paramId))
                return admosc;

            // Tracking section
            auto tracking = config.getChildWithName (Tracking);
            if (tracking.hasProperty (paramId))
                return tracking;

            // Binaural section. Its seven parameters have their own typed setters,
            // which the GUI uses; every generic caller — MCP, and config-scope OSC
            // ingress — landed here and fell off the end of this list, so those
            // writes were dropped in silence.
            auto binaural = config.getChildWithName (Binaural);
            if (binaural.hasProperty (paramId))
                return binaural;

            // UI section (Stream Deck / Sampler / Lightpad toggles).
            auto ui = config.getChildWithName (UI);
            if (ui.hasProperty (paramId))
                return ui;

            // EffectsGlobal. getParameterScope sends every effectsGlobal* name
            // here by name, ahead of the per-channel "effect" prefix test - but
            // the routing is only half the journey: a scope with no node to land
            // on falls off the end of this list and the write is dropped in
            // silence (TreeParameterStore::setParameter is `if (tree.isValid())`
            // with no else and a void return). That was the state of things
            // until <Config><EffectsGlobal> existed.
            auto effectsGlobal = config.getChildWithName (EffectsGlobal);
            if (effectsGlobal.hasProperty (paramId))
                return effectsGlobal;

            return {};
        }

        case ParameterScope::Cluster:
        {
            if (channelIndex < 0)
                return {};

            // These accessors are non-const and may materialise a missing node
            // (old files). Same const_cast idiom this function already uses for
            // the tree itself, a few lines up.
            auto* self = const_cast<WFSValueTreeState*> (this);

            // Preset names are the ONE property unique to the preset list. Every
            // other clusterLFO* name appears on both a live cluster's <ClusterLFO>
            // and on each stored preset, so order matters here: resolve the live
            // cluster first, or a period write would land in a preset slot.
            if (paramId == clusterLFOPresetName)
            {
                auto presets = self->getClusterLFOPresetsSection();
                if (! presets.isValid() || channelIndex >= presets.getNumChildren())
                    return {};
                return presets.getChild (channelIndex);
            }

            // getClusterState and getClusterLFOSection take a ONE-based cluster
            // index (they subtract 1 internally), while channelIndex arrives
            // zero-based — resolveChannelSlot turns cluster_id into displayId - 1.
            // Convert here rather than at either end, so both conventions stay
            // true where they are documented.
            const int oneBasedCluster = channelIndex + 1;

            auto cluster = self->getClusterState (oneBasedCluster);
            if (cluster.isValid() && cluster.hasProperty (paramId))
                return cluster;

            auto lfo = self->getClusterLFOSection (oneBasedCluster);
            if (lfo.isValid() && lfo.hasProperty (paramId))
                return lfo;

            return {};
        }

        case ParameterScope::Input:
        {
            if (channelIndex < 0)
                return {};

            auto inputs = mutableState.getChildWithName (Inputs);
            if (!inputs.isValid() || channelIndex >= inputs.getNumChildren())
                return {};

            auto input = inputs.getChild (channelIndex);

            // Search subsections
            for (int i = 0; i < input.getNumChildren(); ++i)
            {
                auto child = input.getChild (i);
                if (child.hasProperty (paramId))
                    return child;
            }

            // A property younger than the file this channel came from: the section
            // it belongs in, so the write creates it (setInputParameter does the same)
            if (const auto sectionId = sectionForMissingInputProperty (paramId); sectionId.isValid())
                return input.getChildWithName (sectionId);

            return {};
        }

        case ParameterScope::Output:
        {
            if (channelIndex < 0)
                return {};

            auto outputs = mutableState.getChildWithName (Outputs);
            if (!outputs.isValid() || channelIndex >= outputs.getNumChildren())
                return {};

            auto output = outputs.getChild (channelIndex);

            // Search subsections
            for (int i = 0; i < output.getNumChildren(); ++i)
            {
                auto child = output.getChild (i);
                if (child.hasProperty (paramId))
                    return child;

                // Check EQ bands
                if (child.getType() == EQ)
                {
                    for (int j = 0; j < child.getNumChildren(); ++j)
                    {
                        auto band = child.getChild (j);
                        if (band.hasProperty (paramId))
                            return band;
                    }
                }
            }
            return {};
        }

        case ParameterScope::Reverb:
        {
            auto reverbs = mutableState.getChildWithName (Reverbs);
            if (! reverbs.isValid())
                return {};

            // Algo-level (global) subtrees first. ReverbAlgorithm /
            // ReverbPreComp / ReverbPostExp / ReverbPostEQ are siblings
            // of per-channel Reverb children inside Reverbs and carry
            // unique properties — a hasProperty hit here is unambiguous
            // regardless of the channelIndex argument. PostEQ band props
            // (reverbPostEQshape/freq/gain/q/slope) live on PostEQBand
            // children and are routed through the EQ-band path in the
            // MCP dispatcher; they need a band index this signature
            // doesn't carry.
            auto algo = reverbs.getChildWithName (ReverbAlgorithm);
            if (algo.hasProperty (paramId))
                return algo;
            auto preComp = reverbs.getChildWithName (ReverbPreComp);
            if (preComp.hasProperty (paramId))
                return preComp;
            auto postExp = reverbs.getChildWithName (ReverbPostExp);
            if (postExp.hasProperty (paramId))
                return postExp;
            auto postEQ = reverbs.getChildWithName (ReverbPostEQ);
            if (postEQ.hasProperty (paramId))
                return postEQ;

            // Per-channel: walk Reverb-typed children only, skipping
            // algo subtree siblings, and pick the nth one (zero-based).
            // Reverbs.getChild(channelIndex) is unsafe here because the
            // algo subtrees share the same parent and may be ordered
            // before or after the per-channel children depending on
            // when they were appended.
            if (channelIndex < 0)
                return {};
            int nth = 0;
            for (int i = 0; i < reverbs.getNumChildren(); ++i)
            {
                auto child = reverbs.getChild (i);
                if (child.getType() != Reverb)
                    continue;
                if (nth == channelIndex)
                {
                    for (int j = 0; j < child.getNumChildren(); ++j)
                    {
                        auto sub = child.getChild (j);
                        if (sub.hasProperty (paramId))
                            return sub;
                        if (sub.getType() == EQ)
                        {
                            for (int k = 0; k < sub.getNumChildren(); ++k)
                            {
                                auto band = sub.getChild (k);
                                if (band.hasProperty (paramId))
                                    return band;
                            }
                        }
                    }
                    return {};
                }
                ++nth;
            }
            return {};
        }

        case ParameterScope::Effect:
        {
            if (channelIndex < 0)
                return {};

            auto effects = mutableState.getChildWithName (Effects);
            if (! effects.isValid())
                return {};

            // BY TYPE, exactly as the Reverb case above walks its nth, and for
            // the reason getEffectState records: this must resolve the same
            // channel getNumEffectChannels counted, whatever the container turns
            // out to hold. There is still no hasProperty pre-pass over global
            // siblings - the effects globals are Config properties, routed there
            // by name in getParameterScope - but "no globals" was never a
            // promise about what a MERGED FILE can leave in the child list, and
            // this is the resolution path every OSC, MCP and GUI write takes. It
            // indexed positionally, so a foreign child ahead of a channel made
            // canWriteParameter answer FALSE for a channel the count promised,
            // and every remote write to it was refused for the life of the show.
            int nth = 0;
            juce::ValueTree effect;
            for (int i = 0; i < effects.getNumChildren(); ++i)
            {
                auto candidate = effects.getChild (i);
                if (! candidate.hasType (Effect))
                    continue;
                if (nth == channelIndex)
                {
                    effect = candidate;
                    break;
                }
                ++nth;
            }

            if (! effect.isValid())
                return {};

            for (int i = 0; i < effect.getNumChildren(); ++i)
            {
                auto sub = effect.getChild (i);

                // Skip the doubled module types and never descend into <Band> or
                // <Tap>: those property names exist on more than one node, so a
                // hit here could only mean "the first one". Returning an invalid
                // tree makes canWriteParameter answer FALSE, which is what lets
                // a remote surface report an error instead of being handed a
                // success for a write that landed on instance 1. The sends CELL
                // pseudo-identifiers miss here for the same reason - no node
                // carries them, deliberately.
                if (isInstancedEffectModuleType (sub.getType()))
                    continue;

                if (sub.hasProperty (paramId))
                    return sub;
            }
            return {};
        }

        case ParameterScope::AudioPatch:
        {
            auto audioPatch = mutableState.getChildWithName (AudioPatch);
            if (audioPatch.hasProperty (paramId))
                return audioPatch;
            return {};
        }

        default:
            return {};
    }
}

void WFSValueTreeState::enforceClusterTrackingConstraint (int changedInputIndex)
{
    // Get tracking state for the changed input
    auto changedInput = getInputState (changedInputIndex);
    if (!changedInput.isValid())
        return;

    auto posSection = changedInput.getChildWithName (Position);
    if (!posSection.isValid())
        return;

    int clusterIdx = static_cast<int> (posSection.getProperty (inputCluster));
    bool trackingActive = static_cast<int> (posSection.getProperty (inputTrackingActive)) != 0;

    // Only check if this input is in a cluster (not "Single" which is 0)
    // and has tracking enabled
    if (clusterIdx < 1 || !trackingActive)
        return;

    // Check global tracking state - constraints only matter when global tracking is active
    auto trackingSection = getTrackingState();
    bool globalEnabled = trackingSection.isValid() &&
                         static_cast<int> (trackingSection.getProperty (trackingEnabled)) != 0;
    int protocol = trackingSection.isValid() ?
                   static_cast<int> (trackingSection.getProperty (trackingProtocol)) : 0;

    if (!globalEnabled || protocol == 0)
        return;  // Global tracking not active, constraint doesn't apply

    // Find all other inputs in the same cluster with tracking enabled
    auto inputs = getInputsState();
    int numInputs = inputs.getNumChildren();

    for (int i = 0; i < numInputs; ++i)
    {
        if (i == changedInputIndex)
            continue;  // Skip the changed input

        auto input = inputs.getChild (i);
        auto pos = input.getChildWithName (Position);
        if (!pos.isValid())
            continue;

        int otherCluster = static_cast<int> (pos.getProperty (inputCluster));
        bool otherTracking = static_cast<int> (pos.getProperty (inputTrackingActive)) != 0;

        if (otherCluster == clusterIdx && otherTracking)
        {
            // Found another input in same cluster with tracking enabled
            // Disable tracking on the OTHER input (keep the one that was just changed)
            pos.setProperty (inputTrackingActive, 0, nullptr);
        }
    }
}

// Find the first-ordered member of a cluster, honouring clusterInputOrder
// when present, otherwise falling back to the lowest-index member. Returns
// -1 if the cluster has no members. Used by the Shared-Position invariant.
static int findFirstOrderedClusterMember (juce::ValueTree inputs,
                                          const juce::Identifier& inputClusterId,
                                          const juce::Identifier& positionId,
                                          juce::String inputOrder,
                                          int clusterIndex)
{
    const int numInputs = inputs.getNumChildren();

    auto isMember = [&] (int idx) -> bool
    {
        if (idx < 0 || idx >= numInputs) return false;
        auto input = inputs.getChild (idx);
        auto pos = input.getChildWithName (positionId);
        if (! pos.isValid()) return false;
        return static_cast<int> (pos.getProperty (inputClusterId)) == clusterIndex;
    };

    if (inputOrder.isNotEmpty())
    {
        juce::StringArray tokens;
        tokens.addTokens (inputOrder, ",", "");
        for (const auto& tok : tokens)
        {
            int candidate = tok.trim().getIntValue();
            if (isMember (candidate))
                return candidate;
        }
    }

    for (int i = 0; i < numInputs; ++i)
        if (isMember (i))
            return i;

    return -1;
}

void WFSValueTreeState::propagateSharedClusterPosition (int sourceInputIndex)
{
    auto sourceInput = getInputState (sourceInputIndex);
    if (! sourceInput.isValid())
        return;

    auto sourcePos = sourceInput.getChildWithName (Position);
    if (! sourcePos.isValid())
        return;

    const int clusterIdx = static_cast<int> (sourcePos.getProperty (inputCluster));
    if (clusterIdx < 1)
        return;

    const int mode = static_cast<int> (getClusterParameter (clusterIdx, clusterReferenceMode));
    if (mode != 2)
        return;

    const float x = static_cast<float> (sourcePos.getProperty (inputPositionX));
    const float y = static_cast<float> (sourcePos.getProperty (inputPositionY));
    const float z = static_cast<float> (sourcePos.getProperty (inputPositionZ));

    auto inputs = getInputsState();
    const int numInputs = inputs.getNumChildren();

    for (int i = 0; i < numInputs; ++i)
    {
        if (i == sourceInputIndex) continue;

        auto input = inputs.getChild (i);
        auto pos = input.getChildWithName (Position);
        if (! pos.isValid()) continue;

        if (static_cast<int> (pos.getProperty (inputCluster)) != clusterIdx)
            continue;

        // Skip the write when it's already correct — avoids reentry loops if
        // any listener calls back into propagation.
        if (static_cast<float> (pos.getProperty (inputPositionX)) != x)
            pos.setProperty (inputPositionX, x, nullptr);
        if (static_cast<float> (pos.getProperty (inputPositionY)) != y)
            pos.setProperty (inputPositionY, y, nullptr);
        if (static_cast<float> (pos.getProperty (inputPositionZ)) != z)
            pos.setProperty (inputPositionZ, z, nullptr);
    }
}

void WFSValueTreeState::enforceSharedClusterInvariant (int clusterIndex)
{
    if (clusterIndex < 1)
        return;

    const int mode = static_cast<int> (getClusterParameter (clusterIndex, clusterReferenceMode));
    if (mode != 2)
        return;

    auto inputs = getInputsState();
    const juce::String order = getClusterParameter (clusterIndex, clusterInputOrder).toString();
    const int refIdx = findFirstOrderedClusterMember (inputs, inputCluster, Position, order, clusterIndex);
    if (refIdx < 0)
        return;

    auto refInput = inputs.getChild (refIdx);
    auto refPos = refInput.getChildWithName (Position);
    if (! refPos.isValid())
        return;

    const float x = static_cast<float> (refPos.getProperty (inputPositionX));
    const float y = static_cast<float> (refPos.getProperty (inputPositionY));
    const float z = static_cast<float> (refPos.getProperty (inputPositionZ));

    const int numInputs = inputs.getNumChildren();
    for (int i = 0; i < numInputs; ++i)
    {
        if (i == refIdx) continue;

        auto input = inputs.getChild (i);
        auto pos = input.getChildWithName (Position);
        if (! pos.isValid()) continue;

        if (static_cast<int> (pos.getProperty (inputCluster)) != clusterIndex)
            continue;

        if (static_cast<float> (pos.getProperty (inputPositionX)) != x)
            pos.setProperty (inputPositionX, x, nullptr);
        if (static_cast<float> (pos.getProperty (inputPositionY)) != y)
            pos.setProperty (inputPositionY, y, nullptr);
        if (static_cast<float> (pos.getProperty (inputPositionZ)) != z)
            pos.setProperty (inputPositionZ, z, nullptr);
    }
}

void WFSValueTreeState::enforceAllSharedClusterInvariants()
{
    for (int c = 1; c <= 10; ++c)
        enforceSharedClusterInvariant (c);
}

WFSValueTreeState::ParameterScope WFSValueTreeState::getParameterScope (const juce::Identifier& paramId) const
{
    // Check for config-level parameters that might have misleading prefixes
    // inputChannels, outputChannels, reverbChannels are stored in Config/IO,
    // not in their respective channel sections
    if (paramId == inputChannels || paramId == outputChannels || paramId == reverbChannels
        || paramId == effectChannels)
        return ParameterScope::Config;

    // reverbsMapVisible is a Master-section display toggle, not a per-reverb
    // parameter. It only LOOKS like one: the prefix test below would send it to
    // the Reverb branch, which searches <Reverb> channel nodes and never finds
    // it, so the write vanished. Named here for the same reason the channel
    // counts are — the prefix lies about where the property lives.
    if (paramId == reverbsMapVisible || paramId == effectsMapVisible)
        return ParameterScope::Config;

    // Check if it's an input parameter
    juce::String paramName = paramId.toString();
    if (paramName.startsWith ("input"))
        return ParameterScope::Input;

    // Sampler parameters hang off an <Input>, but are named for the feature
    // rather than the scope, so they never matched the "input" prefix and
    // defaulted to Config — where nothing could find them. The ones on <Channel>
    // (a direct child) resolve from here; cell and set properties live on
    // grandchildren and still need the sub-index path.
    if (paramName.startsWith ("sampler"))
        return ParameterScope::Input;

    // Cluster parameters carry their index as cluster_id, which arrives as the
    // channelIndex argument — the plumbing was always there, the scope was not.
    if (paramName.startsWith ("cluster"))
        return ParameterScope::Cluster;

    // Check if it's a reverb parameter
    if (paramName.startsWith ("reverb"))
        return ParameterScope::Reverb;

    // The effects globals live in <Config><EffectsGlobal>, not on a channel, and
    // every one of them starts with "effect" - so this test MUST come before the
    // per-channel prefix branch below. Without it the prefix sends them to the
    // Effect branch, which searches <Effect> channel nodes, finds nothing, and
    // the write is dropped with no error at all (TreeParameterStore::setParameter
    // is `if (tree.isValid()) write(...)` with no else and a void return). That
    // is the exact incident the reverbsMapVisible exception above records.
    // effectChannels and effectsMapVisible are named individually further up for
    // the same reason.
    if (paramName.startsWith ("effectsGlobal"))
        return ParameterScope::Config;

    // Check if it's a per-channel effect parameter
    if (paramName.startsWith ("effect"))
        return ParameterScope::Effect;

    // Check if it's an output parameter
    if (paramName.startsWith ("output") || paramName.startsWith ("eq"))
        return ParameterScope::Output;

    // Check if it's an audio patch parameter
    if (paramId == driverMode || paramId == audioInterface ||
        paramId == inputMatrixMode || paramId == outputMatrixMode ||
        paramId == testTone || paramId == sineFrequency || paramId == testToneLevel ||
        paramId == patchData)
        return ParameterScope::AudioPatch;

    // Default to config
    return ParameterScope::Config;
}
