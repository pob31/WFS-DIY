#pragma once

#include <JuceHeader.h>
#include "OSCProtocolTypes.h"
#include "OSCMessageBuilder.h"
#include "OSCMessageRouter.h"
#include "../Parameters/EffectsSnapshotScope.h"
#include "../gui/effects/EffectsModuleDescriptors.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/WFSFileManager.h"
#include "../Localization/LocalizationManager.h"

namespace WFSNetwork
{

/**
 * Structured output from QLabCueBuilder.
 * Group and network cue messages are separated so that sendToQLab()
 * can query QLab for unique IDs between steps and construct move commands.
 */
struct QLabCueSequence
{
    std::vector<juce::OSCMessage> groupMessages;  // /new group, name, mode, number

    struct NetworkCue
    {
        std::vector<juce::OSCMessage> messages;   // /new network, number, patch, customString
        int movePosition = 0;                      // 1-based position in group
    };

    std::vector<NetworkCue> networkCues;
};

/**
 * QLabCueBuilder
 *
 * Builds a sequence of OSC messages that, when sent to QLab,
 * create a Group cue containing Network cues for each in-scope
 * parameter/channel from a WFS snapshot. Each Network cue sends
 * an OSC message back to the WFS processor to recall the parameter value.
 */
class QLabCueBuilder
{
public:
    /**
     * Build the structured cue sequence for a snapshot export to QLab.
     *
     * @param snapshotName     Display name for the QLab group cue
     * @param snapshotData     The <Inputs> ValueTree from the snapshot
     * @param scope            Extended scope filtering
     * @param numChannels      Total number of input channels
     * @param qlabPatchNumber  QLab network patch to assign to created cues
     * @param numberToSlot     Resolves a permanent input channel number to the slot it
     *                         currently occupies, negative when no live channel carries
     *                         it (WFSValueTreeState::getSlotForChannelNumber)
     * @param numOutputs       Live output count: each input's mute list is fitted to it,
     *                         so the cue carries exactly one entry per output (0 = as stored)
     * @param effectsData      The <Effects> ValueTree from the snapshot (plan revision 8:
     *                         one snapshot carries both families); invalid = none
     * @param numEffects       Live effect count: an <Effect> beyond it is a ghost and gets
     *                         no cue, exactly as recall skips it
     * @return QLabCueSequence with group messages and per-cue messages
     */
    static QLabCueSequence buildSnapshotCues (
        const juce::String& snapshotName,
        const juce::ValueTree& snapshotData,
        const WFSFileManager::ExtendedSnapshotScope& scope,
        int numChannels,
        int qlabPatchNumber,
        const std::function<int (int)>& numberToSlot = {},
        int numOutputs = 0,
        const juce::ValueTree& effectsData = {},
        int numEffects = 0)
    {
        QLabCueSequence sequence;

        // 1. Create group cue
        sequence.groupMessages.push_back (juce::OSCMessage ("/new", juce::String ("group")));

        // 2. Name the group
        sequence.groupMessages.push_back (juce::OSCMessage ("/cue/selected/name",
            LOC("snapshot.qlabGroupName").replace ("{name}", snapshotName)));

        // 3. Set playlist mode (mode 6)
        sequence.groupMessages.push_back (juce::OSCMessage ("/cue/selected/mode", (int) 6));

        // 4. For each input channel in scope, create network cues
        int cueCounter = 0;

        for (int i = 0; i < snapshotData.getNumChildren(); ++i)
        {
            auto inputData = snapshotData.getChild (i);

            // channelId is the permanent number: it stays the OSC address of the cue.
            // The scope mask is keyed by slot, so it needs the separate lookup below.
            int channelId = static_cast<int> (inputData.getProperty (WFSParameterIDs::id, 0));
            const int channelIndex = resolveSlot (channelId, numberToSlot, numChannels);

            if (channelIndex < 0)
                continue;

            appendChannelCues (sequence.networkCues, inputData, channelIndex, channelId,
                               scope, qlabPatchNumber, numOutputs, cueCounter);
        }

        // 5. The effects half, after the inputs, in the same group.
        for (const auto& effectCue : collectEffectCues (effectsData, scope.effects, numEffects, numOutputs))
        {
            QLabCueSequence::NetworkCue cue;
            cue.movePosition = ++cueCounter;  // 1-based
            cue.messages.push_back (juce::OSCMessage ("/new", juce::String ("network")));
            cue.messages.push_back (juce::OSCMessage ("/cue/selected/patch", qlabPatchNumber));
            cue.messages.push_back (juce::OSCMessage ("/cue/selected/customString", effectCue.customString));
            cue.messages.push_back (juce::OSCMessage ("/cue/selected/name", effectCue.name));
            sequence.networkCues.push_back (std::move (cue));
        }

        return sequence;
    }

    /** Get the count of network cues that would be created (for progress display).
        Takes the same number-to-slot resolver as buildSnapshotCues: the two must
        select the same channels or the progress total disagrees with the cues sent. */
    static int countCues (
        const juce::ValueTree& snapshotData,
        const WFSFileManager::ExtendedSnapshotScope& scope,
        int numChannels,
        const std::function<int (int)>& numberToSlot = {},
        const juce::ValueTree& effectsData = {},
        int numEffects = 0,
        int numOutputs = 0)
    {
        int count = 0;
        const auto& inputMappings = OSCMessageBuilder::getInputMappings();

        for (int i = 0; i < snapshotData.getNumChildren(); ++i)
        {
            auto inputData = snapshotData.getChild (i);

            // The id property is the permanent number; the scope mask below is
            // keyed by slot.
            const int channelIndex = resolveSlot (
                static_cast<int> (inputData.getProperty (WFSParameterIDs::id, 0)),
                numberToSlot, numChannels);

            if (channelIndex < 0)
                continue;

            for (int s = 0; s < inputData.getNumChildren(); ++s)
            {
                auto section = inputData.getChild (s);
                for (int p = 0; p < section.getNumProperties(); ++p)
                {
                    auto paramId = section.getPropertyName (p);
                    if (paramId == WFSParameterIDs::inputName)
                        continue;

                    if (inputMappings.find (paramId) != inputMappings.end()
                        && scope.isParameterIncluded (paramId, channelIndex))
                    {
                        ++count;
                    }
                }
            }
        }
        return count + static_cast<int> (collectEffectCues (effectsData, scope.effects, numEffects, numOutputs).size());
    }

    /** One effect cue: what QLab sends, and what the cue is called. */
    struct EffectCue
    {
        juce::String customString;
        juce::String name;
    };

    /** The per-parameter cues of a snapshot's effects half (plan revision 8).

        Each value goes out in the shape the /wfs/effect/ parser expects for it -
        OSCMessageRouter::getEffectParamKind is the one table:
          Scalar     /wfs/effect/<name> <ID> <value>
          Instanced  /wfs/effect/<name> <ID> <instance 1|2> <value>        (FxEq1/2, FxDyn1/2)
          Band       /wfs/effect/<name> <ID> <instance> <band 1..6> <value>
          Tap        /wfs/effect/<name> <ID> <tap 1..8> <value>
          Row        /wfs/effect/<name> <ID> "<whole row>"                  (quoted: one string)
        The effects grid decides what is exported, per item and per channel,
        through the same EffectsSnapshotScope::itemIdFor the store and the recall
        use; effectName is never exported (as inputName is not), and a ghost
        <Effect> beyond the live count gets no cue. `oneTokenRowsSkipped` counts
        the per-output mute rows of a ONE-output rig: such a row is a lone
        number, which the receiver refuses as a row, and effects have no
        per-output mute form. */
    static std::vector<EffectCue> collectEffectCues (const juce::ValueTree& effectsData,
                                                     const WFSFileManager::ScopeMatrix& grid,
                                                     int numEffects, int numOutputs,
                                                     int* oneTokenRowsSkipped = nullptr)
    {
        std::vector<EffectCue> cues;

        for (int e = 0; e < effectsData.getNumChildren(); ++e)
        {
            const auto entry = effectsData.getChild (e);
            if (! entry.hasType (WFSParameterIDs::Effect))
                continue;

            const int effectId = entry.getProperty (WFSParameterIDs::id).toString().getIntValue();
            if (effectId < 1 || effectId > numEffects)
                continue;

            for (int c = 0; c < entry.getNumChildren(); ++c)
            {
                const auto node = entry.getChild (c);
                const auto nodeType = node.getType();
                const int instance = effectInstanceOf (nodeType);

                appendEffectNodeCues (cues, node, nodeType, instance, 0, 0, effectId, grid, numOutputs, oneTokenRowsSkipped);

                for (int k = 0; k < node.getNumChildren(); ++k)
                {
                    const auto child = node.getChild (k);
                    const int index = child.getProperty (WFSParameterIDs::id).toString().getIntValue();

                    if (child.hasType (WFSParameterIDs::Band))
                        appendEffectNodeCues (cues, child, nodeType, instance, index, 0, effectId, grid, numOutputs, oneTokenRowsSkipped);
                    else if (child.hasType (WFSParameterIDs::Tap))
                        appendEffectNodeCues (cues, child, nodeType, instance, 0, index, effectId, grid, numOutputs, oneTokenRowsSkipped);
                }
            }
        }

        return cues;
    }

    /**
     * Build a QLab group containing a memo and two network cues
     * for loading and updating a snapshot.
     *
     * Structure:
     *   Group "Snapshot "<name>"" (mode 2 = start first and go to next)
     *     ├─ Memo: instructions
     *     ├─ Network: /wfs/input/snapshot/load <name>  ("Reload "<name>"")
     *     └─ Network: /wfs/input/snapshot/store <name> ("Update "<name>"")
     *
     * @param snapshotName     The snapshot name
     * @param qlabPatchNumber  QLab network patch to assign to network cues
     * @return QLabCueSequence with group + 3 child cues
     */
    static QLabCueSequence buildSnapshotLoadCue (
        const juce::String& snapshotName,
        int qlabPatchNumber)
    {
        QLabCueSequence sequence;

        // 1. Group cue (mode 2 = start first and go to next cue)
        sequence.groupMessages.push_back (juce::OSCMessage ("/new", juce::String ("group")));
        sequence.groupMessages.push_back (juce::OSCMessage ("/cue/selected/name",
            LOC("snapshot.qlabGroupName").replace ("{name}", snapshotName)));
        sequence.groupMessages.push_back (juce::OSCMessage ("/cue/selected/mode", (int) 2));

        // 2. Memo cue (child)
        {
            QLabCueSequence::NetworkCue memo;
            memo.movePosition = 1;
            memo.messages.push_back (juce::OSCMessage ("/new", juce::String ("memo")));
            memo.messages.push_back (juce::OSCMessage ("/cue/selected/name",
                LOC("snapshot.qlabMemoText")));
            memo.messages.push_back (juce::OSCMessage ("/cue/selected/notes",
                LOC("snapshot.qlabMemoText")));
            sequence.networkCues.push_back (std::move (memo));
        }

        // 3. Reload network cue (child)
        {
            QLabCueSequence::NetworkCue reload;
            reload.movePosition = 2;
            reload.messages.push_back (juce::OSCMessage ("/new", juce::String ("network")));
            reload.messages.push_back (juce::OSCMessage ("/cue/selected/patch", qlabPatchNumber));
            // Quote the name: QLab tokenises unquoted custom-message arguments
            // at spaces, which would split "snapshot 34" into two OSC args.
            reload.messages.push_back (juce::OSCMessage ("/cue/selected/customString",
                juce::String ("/wfs/input/snapshot/load \"" + snapshotName + "\"")));
            reload.messages.push_back (juce::OSCMessage ("/cue/selected/name",
                LOC("snapshot.qlabReloadName").replace ("{name}", snapshotName)));
            sequence.networkCues.push_back (std::move (reload));
        }

        // 4. Update network cue (child)
        {
            QLabCueSequence::NetworkCue update;
            update.movePosition = 3;
            update.messages.push_back (juce::OSCMessage ("/new", juce::String ("network")));
            update.messages.push_back (juce::OSCMessage ("/cue/selected/patch", qlabPatchNumber));
            update.messages.push_back (juce::OSCMessage ("/cue/selected/customString",
                juce::String ("/wfs/input/snapshot/store \"" + snapshotName + "\"")));
            update.messages.push_back (juce::OSCMessage ("/cue/selected/name",
                LOC("snapshot.qlabUpdateName").replace ("{name}", snapshotName)));
            sequence.networkCues.push_back (std::move (update));
        }

        return sequence;
    }

    /**
     * Build a single QLab network cue to select a sampler set on a channel.
     *
     * @param channelId        1-based input channel number
     * @param setNumber        1-based set number
     * @param setName          Display name of the set
     * @param qlabPatchNumber  QLab network patch to assign
     * @return QLabCueSequence with a single network cue
     */
    static QLabCueSequence buildSamplerSetCue (
        int channelId,
        int setNumber,
        const juce::String& setName,
        int qlabPatchNumber)
    {
        QLabCueSequence sequence;

        QLabCueSequence::NetworkCue cue;
        cue.movePosition = 0;  // No group — standalone cue
        cue.messages.push_back (juce::OSCMessage ("/new", juce::String ("network")));
        cue.messages.push_back (juce::OSCMessage ("/cue/selected/patch", qlabPatchNumber));
        cue.messages.push_back (juce::OSCMessage ("/cue/selected/customString",
            juce::String ("/wfs/input/samplerSet " + juce::String (channelId) + " " + juce::String (setNumber))));
        cue.messages.push_back (juce::OSCMessage ("/cue/selected/name",
            LOC ("sampler.qlabSetCueName")
                .replace ("{channel}", juce::String (channelId))
                .replace ("{name}", setName)));
        sequence.networkCues.push_back (std::move (cue));

        return sequence;
    }

    /**
     * Build a single QLab network cue to recall a cluster LFO preset.
     */
    static QLabCueSequence buildClusterLFOPresetCue (
        int clusterId,
        int presetNumber,
        const juce::String& presetName,
        int qlabPatchNumber)
    {
        QLabCueSequence sequence;

        QLabCueSequence::NetworkCue cue;
        cue.movePosition = 0;
        cue.messages.push_back (juce::OSCMessage ("/new", juce::String ("network")));
        cue.messages.push_back (juce::OSCMessage ("/cue/selected/patch", qlabPatchNumber));
        cue.messages.push_back (juce::OSCMessage ("/cue/selected/customString",
            juce::String ("/wfs/cluster/lfoPresetRecall " + juce::String (clusterId) + " " + juce::String (presetNumber))));
        cue.messages.push_back (juce::OSCMessage ("/cue/selected/name",
            LOC ("clusters.qlabPresetCueName")
                .replace ("{cluster}", juce::String (clusterId))
                .replace ("{name}", presetName)));
        sequence.networkCues.push_back (std::move (cue));

        return sequence;
    }

private:
    /** Snapshot entries are keyed by the permanent channel NUMBER, while the scope
        mask is built over live SLOTS. Numbers may have gaps and stop following slot
        order once channels are reordered or deleted, so only the live state can pair
        the two. Returns a negative value for a number no live channel carries — the
        ghost entry of a deleted channel, which recall skips and which gets no cue.
        Without a resolver the caller has no live state to consult and all that is
        left is the dense assumption; it cannot survive a reorder. */
    static int resolveSlot (int channelNumber,
                            const std::function<int (int)>& numberToSlot,
                            int numChannels)
    {
        if (numberToSlot != nullptr)
            return numberToSlot (channelNumber);

        const int denseSlot = channelNumber - 1;
        return denseSlot < numChannels ? denseSlot : -1;
    }

    /** 1 / 2 for the doubled module nodes, 0 for every other node. */
    static int effectInstanceOf (const juce::Identifier& nodeType)
    {
        using namespace WFSParameterIDs;
        if (nodeType == FxEq1 || nodeType == FxDyn1) return 1;
        if (nodeType == FxEq2 || nodeType == FxDyn2) return 2;
        return 0;
    }

    /** The cues of one node of an <Effect> entry (a flat node, a module, or one
        of a module's <Band> / <Tap> children, `band` / `tap` 1-based). */
    static void appendEffectNodeCues (std::vector<EffectCue>& cues, const juce::ValueTree& node,
                                      const juce::Identifier& nodeType, int instance, int band, int tap,
                                      int effectId, const WFSFileManager::ScopeMatrix& grid,
                                      int numOutputs, int* oneTokenRowsSkipped)
    {
        using Kind = OSCMessageRouter::ParsedEffectMessage::Kind;
        const auto& paths = OSCMessageBuilder::getEffectMappings();

        for (int p = 0; p < node.getNumProperties(); ++p)
        {
            const auto paramId = node.getPropertyName (p);
            if (paramId == WFSParameterIDs::id || paramId == WFSParameterIDs::effectName)
                continue;

            const auto itemId = EffectsSnapshotScope::itemIdFor (nodeType, paramId);
            if (itemId.isEmpty() || ! grid.isIncluded (itemId, effectId - 1))
                continue;

            const auto path = paths.find (paramId);
            if (path == paths.end())
                continue;

            const auto value = node.getProperty (paramId);
            juce::String args (effectId);

            switch (OSCMessageRouter::getEffectParamKind (paramId))
            {
                case Kind::Scalar:
                    break;

                case Kind::Instanced:
                    if (instance < 1) continue;
                    args << " " << instance;
                    break;

                case Kind::Band:
                    if (instance < 1 || band < 1) continue;
                    args << " " << instance << " " << band;
                    break;

                case Kind::Tap:
                    if (tap < 1) continue;
                    args << " " << tap;
                    break;

                case Kind::Row:
                {
                    const auto text = paramId == WFSParameterIDs::effectMutes
                                          ? WFSValueTreeState::normaliseMuteList (value, numOutputs)
                                          : value.toString().trim();

                    // A row is ONE string. A lone number is not one: the receiver
                    // refuses it rather than wiping the row to one column.
                    if (text.isEmpty() || text.containsOnly ("0123456789.+-eE"))
                    {
                        if (oneTokenRowsSkipped != nullptr)
                            ++*oneTokenRowsSkipped;
                        continue;
                    }

                    cues.push_back ({ path->second.oscPath + " " + args + " " + text.quoted(),
                                      effectCueName (paramId, nodeType, band, tap, effectId, text, true) });
                    continue;
                }

                default:
                    continue;
            }

            cues.push_back ({ path->second.oscPath + " " + args + " " + formatEffectValue (value),
                              effectCueName (paramId, nodeType, band, tap, effectId, value, false) });
        }
    }

    /** A value as QLab should send it: a float keeps a decimal point, an int has
        none, text goes quoted (the rule formatCustomString applies to inputs). */
    static juce::String formatEffectValue (const juce::var& value)
    {
        const auto text = value.toString().trim();
        if (text.isNotEmpty() && ! text.containsOnly ("0123456789.+-eE"))
            return text.quoted();
        if (text.containsChar ('.'))
            return juce::String (text.getDoubleValue(), 6);
        return juce::String (text.getIntValue());
    }

    /** "Effect 2 EQ 2 Band 3 EQ Gain 5.5 dB", "Effect 1 Attenuation -6.0 dB",
        "Effect 1 Mutes: 2, 4". Module parameters take their label and unit from
        the generated descriptors (the CSV), prefixed with the module's name;
        the flat ones from effectParamDisplayMap. */
    static juce::String effectCueName (const juce::Identifier& paramId, const juce::Identifier& nodeType,
                                       int band, int tap, int effectId, const juce::var& value, bool isRow)
    {
        juce::String what, unit;

        if (const auto* desc = effectModuleDescriptor (paramId))
        {
            what = effectModuleName (nodeType);
            if (band > 0) what << " Band " << band;
            if (tap > 0)  what << " Tap " << tap;
            what << " " << LOC (juce::String ("effects.labels.") + desc->key).trimCharactersAtEnd (": ");
            unit = desc->unit;
        }
        else
        {
            const auto& flat = effectParamDisplayMap();
            const auto it = flat.find (paramId);
            what = it != flat.end() ? juce::String (it->second.first)
                                    : paramId.toString().fromFirstOccurrenceOf ("effect", false, false);
            unit = it != flat.end() ? juce::String (it->second.second) : juce::String();
        }

        const juce::String prefix = "Effect " + juce::String (effectId) + " ";

        if (paramId == WFSParameterIDs::effectMutes)
        {
            juce::StringArray tokens, runs;
            tokens.addTokens (value.toString(), ",", "");
            for (int i = 0; i < tokens.size(); ++i)
            {
                if (tokens[i].trim() != "1")
                    continue;
                int last = i;
                while (last + 1 < tokens.size() && tokens[last + 1].trim() == "1")
                    ++last;
                runs.add (last == i ? juce::String (i + 1) : juce::String (i + 1) + "-" + juce::String (last + 1));
                i = last;
            }
            return prefix + "Mutes: " + (runs.isEmpty() ? juce::String ("none") : runs.joinIntoString (", "));
        }

        if (isRow)
            return prefix + what + (paramId == WFSParameterIDs::effectChainOrder ? " " + value.toString() : juce::String());

        const auto text = value.toString().trim();
        juce::String shown = text.containsChar ('.') ? juce::String (text.getDoubleValue(), 1)
                                                     : juce::String (text.getIntValue());
        if (unit.isNotEmpty())
            shown << " " << unit;
        return prefix + what + " " + shown;
    }

    /** The generated descriptor of a module parameter, or null for a flat one. */
    static const EffectsUi::ControlDesc* effectModuleDescriptor (const juce::Identifier& paramId)
    {
        static const std::map<juce::Identifier, const EffectsUi::ControlDesc*> table = []
        {
            std::map<juce::Identifier, const EffectsUi::ControlDesc*> t;
            for (int slot = 0; slot < 11; ++slot)
            {
                const auto controls = EffectsUi::controlsForSlot (slot);
                for (int i = 0; i < controls.count; ++i)
                    t[controls.controls[i].id] = &controls.controls[i];
            }
            for (const auto* d : { &EffectsUi::descEQshape(), &EffectsUi::descEQfreq(), &EffectsUi::descEQgain(),
                                   &EffectsUi::descEQq(), &EffectsUi::descEQslope(),
                                   &EffectsUi::descDelayTapTime(), &EffectsUi::descDelayTapLevel() })
                t[d->id] = d;
            return t;
        }();

        const auto it = table.find (paramId);
        return it != table.end() ? it->second : nullptr;
    }

    /** "EQ 2", "Dynamics 1", "Multitap Delay" - the Chain tab's own names. */
    static juce::String effectModuleName (const juce::Identifier& nodeType)
    {
        using namespace WFSParameterIDs;
        const char* token = nodeType == FxDist ? "dist" : nodeType == FxEq1 ? "eq1" : nodeType == FxEq2 ? "eq2"
                          : nodeType == FxDyn1 ? "dyn1" : nodeType == FxDyn2 ? "dyn2" : nodeType == FxMod ? "mod"
                          : nodeType == FxPhaser ? "phaser" : nodeType == FxTrem ? "trem"
                          : nodeType == FxReverb ? "reverb" : nodeType == FxDelay ? "delay"
                          : nodeType == FxCrush ? "crush" : nullptr;
        return token != nullptr ? LOC (juce::String ("effects.modules.") + token) : nodeType.toString();
    }

    /** Names and units of the flat effect parameters, for cue names. */
    static const std::map<juce::Identifier, std::pair<const char*, const char*>>& effectParamDisplayMap()
    {
        using namespace WFSParameterIDs;
        static const std::map<juce::Identifier, std::pair<const char*, const char*>> map = {
            // Channel
            { effectAttenuation,           { "Attenuation",             "dB"   } },
            { effectDelayLatency,          { "Delay",                   "ms"   } },
            { effectMinimalLatency,        { "Min Latency",             ""     } },
            { effectLinkGroup,             { "Link Group",              ""     } },
            { effectLinkMode,              { "Link Mode",               ""     } },
            { effectMute,                  { "Mute",                    ""     } },
            // Position
            { effectPositionX,             { "Position X",              "m"    } },
            { effectPositionY,             { "Position Y",              "m"    } },
            { effectPositionZ,             { "Position Z",              "m"    } },
            { effectCoordinateMode,        { "Coordinate Mode",         ""     } },
            { effectReturnOffsetX,         { "Return Offset X",         "m"    } },
            { effectReturnOffsetY,         { "Return Offset Y",         "m"    } },
            { effectReturnOffsetZ,         { "Return Offset Z",         "m"    } },
            // Feed
            { effectOrientation,           { "Orientation",             "deg"  } },
            { effectAngleOn,               { "Angle On",                "deg"  } },
            { effectAngleOff,              { "Angle Off",               "deg"  } },
            { effectPitch,                 { "Pitch",                   "deg"  } },
            { effectHFdamping,             { "HF Damping",              "dB/m" } },
            { effectFeedMiniLatency,       { "Feed Min Latency",        ""     } },
            { effectDistanceAttenPercent,  { "Distance Atten",          "%"    } },
            // Return
            { effectAttenuationLaw,        { "Attenuation Law",         ""     } },
            { effectDistanceAttenuation,   { "Distance Atten",          "dB/m" } },
            { effectDistanceRatio,         { "Distance Ratio",          ""     } },
            { effectCommonAtten,           { "Common Atten",            "%"    } },
            { effectHFshelf,               { "HF Shelf",                "dB"   } },
            { effectMuteMacro,             { "Mute Macro",              ""     } },
            { effectMuteReverbSends,       { "Mute Reverb Sends",       ""     } },
            { effectArrayAtten1,           { "Array 1 Atten",           "dB"   } },
            { effectArrayAtten2,           { "Array 2 Atten",           "dB"   } },
            { effectArrayAtten3,           { "Array 3 Atten",           "dB"   } },
            { effectArrayAtten4,           { "Array 4 Atten",           "dB"   } },
            { effectArrayAtten5,           { "Array 5 Atten",           "dB"   } },
            { effectArrayAtten6,           { "Array 6 Atten",           "dB"   } },
            { effectArrayAtten7,           { "Array 7 Atten",           "dB"   } },
            { effectArrayAtten8,           { "Array 8 Atten",           "dB"   } },
            { effectArrayAtten9,           { "Array 9 Atten",           "dB"   } },
            { effectArrayAtten10,          { "Array 10 Atten",          "dB"   } },
            // AutomOtion
            { effectOtomoX,                { "AutomOtion X",            "m"    } },
            { effectOtomoY,                { "AutomOtion Y",            "m"    } },
            { effectOtomoZ,                { "AutomOtion Z",            "m"    } },
            { effectOtomoAbsoluteRelative, { "AutomOtion Abs/Rel",      ""     } },
            { effectOtomoCoordinateMode,   { "AutomOtion Coord Mode",   ""     } },
            { effectOtomoR,                { "AutomOtion R",            "m"    } },
            { effectOtomoTheta,            { "AutomOtion Theta",        "deg"  } },
            { effectOtomoRsph,             { "AutomOtion R (sph)",      "m"    } },
            { effectOtomoPhi,              { "AutomOtion Phi",          "deg"  } },
            { effectOtomoSpeedProfile,     { "AutomOtion Speed",        "%"    } },
            { effectOtomoDuration,         { "AutomOtion Duration",     "s"    } },
            { effectOtomoCurve,            { "AutomOtion Curve",        ""     } },
            { effectOtomoTrigger,          { "AutomOtion Trigger",      ""     } },
            { effectOtomoThreshold,        { "AutomOtion Threshold",    "dB"   } },
            { effectOtomoReset,            { "AutomOtion Reset",        "dB"   } },
            // LFO
            { effectLFOactive,             { "LFO Active",              ""     } },
            { effectLFOperiod,             { "LFO Period",              "s"    } },
            { effectLFOphase,              { "LFO Phase",               "deg"  } },
            { effectLFOshapeX,             { "LFO Shape X",             ""     } },
            { effectLFOshapeY,             { "LFO Shape Y",             ""     } },
            { effectLFOshapeZ,             { "LFO Shape Z",             ""     } },
            { effectLFOrateX,              { "LFO Rate X",              ""     } },
            { effectLFOrateY,              { "LFO Rate Y",              ""     } },
            { effectLFOrateZ,              { "LFO Rate Z",              ""     } },
            { effectLFOamplitudeX,         { "LFO Amplitude X",         "m"    } },
            { effectLFOamplitudeY,         { "LFO Amplitude Y",         "m"    } },
            { effectLFOamplitudeZ,         { "LFO Amplitude Z",         "m"    } },
            { effectLFOphaseX,             { "LFO Phase X",             "deg"  } },
            { effectLFOphaseY,             { "LFO Phase Y",             "deg"  } },
            { effectLFOphaseZ,             { "LFO Phase Z",             "deg"  } },
            // Chain and sends (rows: named, not valued)
            { effectChainOrder,            { "Chain Order",             ""     } },
            { effectChainBypass,           { "Chain Bypass",            ""     } },
            { effectSendLevels,            { "Send Levels from Inputs", ""     } },
            { effectSendOns,               { "Sends on from Inputs",    ""     } },
            { effectFxSendLevels,          { "Send Levels from Effects",""     } },
            { effectFxSendOns,             { "Sends on from Effects",   ""     } },
        };
        return map;
    }

    /** Append QLab network cue entries for all in-scope parameters of one channel */
    static void appendChannelCues (
        std::vector<QLabCueSequence::NetworkCue>& networkCues,
        const juce::ValueTree& inputData,
        int channelIndex,
        int channelId,
        const WFSFileManager::ExtendedSnapshotScope& scope,
        int qlabPatchNumber,
        int numOutputs,
        int& cueCounter)
    {
        const auto& inputMappings = OSCMessageBuilder::getInputMappings();

        for (int s = 0; s < inputData.getNumChildren(); ++s)
        {
            auto section = inputData.getChild (s);

            for (int p = 0; p < section.getNumProperties(); ++p)
            {
                auto paramId = section.getPropertyName (p);

                if (paramId == WFSParameterIDs::inputName)
                    continue;

                auto it = inputMappings.find (paramId);
                if (it == inputMappings.end())
                    continue;

                if (!scope.isParameterIncluded (paramId, channelIndex))
                    continue;

                auto oscPath = it->second.oscPath;
                auto value = section.getProperty (paramId);
                ++cueCounter;

                // Stored zero-based; /wfs/input/samplerSet counts sets from 1.
                if (paramId == WFSParameterIDs::inputSamplerActiveSet)
                    value = juce::var (value.toString().getIntValue() + 1);

                const bool isMuteList = (paramId == WFSParameterIDs::inputMutes);
                if (isMuteList)
                    value = juce::var (WFSValueTreeState::normaliseMuteList (value, numOutputs));

                QLabCueSequence::NetworkCue cue;
                cue.movePosition = cueCounter;  // 1-based

                // a. Create network cue
                cue.messages.push_back (juce::OSCMessage ("/new", juce::String ("network")));

                // b. Set QLab network patch
                cue.messages.push_back (juce::OSCMessage ("/cue/selected/patch",
                    qlabPatchNumber));

                // d. Set customString (the OSC message QLab will send).
                // A one-output list is a lone "0" or "1", which the receiver
                // refuses as the bare number that used to wipe lists, so it
                // goes out in the one-output form instead.
                const bool singleMute = isMuteList && value.toString().isNotEmpty()
                                        && ! value.toString().containsChar (',');
                cue.messages.push_back (juce::OSCMessage ("/cue/selected/customString",
                    singleMute ? oscPath + " " + juce::String (channelId) + " 1 " + value.toString()
                               : formatCustomString (oscPath, channelId, value)));

                // e. Set descriptive cue name
                cue.messages.push_back (juce::OSCMessage ("/cue/selected/name",
                    isMuteList ? formatMuteCueName (channelId, value.toString())
                               : formatCueName (paramId, channelId, value)));

                networkCues.push_back (std::move (cue));
            }
        }
    }

    /**
     * Format a parameter value as a QLab customString.
     * Format: "{oscPath} {channelId} {value}"
     * Float values always include a decimal point (so QLab sends float32).
     * Integer values are written without decimal (so QLab sends int32).
     */
    static juce::String formatCustomString (
        const juce::String& oscPath,
        int channelId,
        const juce::var& value)
    {
        juce::String result = oscPath + " " + juce::String (channelId) + " ";
        juce::String strVal = value.toString();

        // Text that is not a number (the per-output mute list "0,1,0,...") goes
        // out whole and quoted: QLab splits unquoted arguments at spaces, and
        // reading it as a number kept only its first entry.
        const auto trimmed = strVal.trim();
        if (trimmed.isNotEmpty() && ! trimmed.containsOnly ("0123456789.+-eE"))
            return result + trimmed.quoted();

        // Values loaded from XML are always string vars — parse as number by content
        if (strVal.containsChar ('.'))
            result += juce::String (strVal.getDoubleValue(), 6);
        else
            result += juce::String (strVal.getIntValue());

        return result;
    }

    /** "Input 3 Mutes: 2, 4, 9-12", or "Input 3 Mutes: none", from a
        normalised mute list (one "0"/"1" per output). */
    static juce::String formatMuteCueName (int channelId, const juce::String& muteList)
    {
        juce::StringArray tokens;
        tokens.addTokens (muteList, ",", "");

        juce::StringArray runs;
        for (int i = 0; i < tokens.size(); ++i)
        {
            if (tokens[i] != "1")
                continue;

            int last = i;
            while (last + 1 < tokens.size() && tokens[last + 1] == "1")
                ++last;

            runs.add (last == i ? juce::String (i + 1)
                                : juce::String (i + 1) + "-" + juce::String (last + 1));
            i = last;
        }

        return "Input " + juce::String (channelId) + " Mutes: "
               + (runs.isEmpty() ? juce::String ("none") : runs.joinIntoString (", "));
    }

    //==========================================================================
    // Parameter display metadata for QLab cue naming
    //==========================================================================

    struct ParamDisplayInfo
    {
        juce::String displayName;
        juce::String unit;
        bool isCompressionRatio;
    };

    static const std::map<juce::Identifier, ParamDisplayInfo>& getParamDisplayMap()
    {
        static const std::map<juce::Identifier, ParamDisplayInfo> map = {
            // Channel
            { WFSParameterIDs::inputAttenuation,          { "Attenuation",          "dB",  false } },
            { WFSParameterIDs::inputDelayLatency,         { "Delay",                "ms",  false } },
            { WFSParameterIDs::inputMinimalLatency,       { "Min Latency",          "",    false } },
            { WFSParameterIDs::inputStereoWidth,          { "Stereo Width",         "m",   false } },
            { WFSParameterIDs::inputStereoAxisOffset,     { "Stereo Axis",          "deg", false } },
            { WFSParameterIDs::inputStereoAxisLock,       { "Stereo Axis Lock",     "",    false } },

            // Position
            { WFSParameterIDs::inputPositionX,            { "Position X",           "m",   false } },
            { WFSParameterIDs::inputPositionY,            { "Position Y",           "m",   false } },
            { WFSParameterIDs::inputPositionZ,            { "Position Z",           "m",   false } },
            { WFSParameterIDs::inputOffsetX,              { "Offset X",             "m",   false } },
            { WFSParameterIDs::inputOffsetY,              { "Offset Y",             "m",   false } },
            { WFSParameterIDs::inputOffsetZ,              { "Offset Z",             "m",   false } },
            { WFSParameterIDs::inputConstraintX,          { "Constraint X",         "",    false } },
            { WFSParameterIDs::inputConstraintY,          { "Constraint Y",         "",    false } },
            { WFSParameterIDs::inputConstraintZ,          { "Constraint Z",         "",    false } },
            { WFSParameterIDs::inputConstraintDistance,    { "Constraint Distance",  "",    false } },
            { WFSParameterIDs::inputConstraintDistanceMin, { "Constraint Dist Min",  "m",  false } },
            { WFSParameterIDs::inputConstraintDistanceMax, { "Constraint Dist Max",  "m",  false } },
            { WFSParameterIDs::inputFlipX,                { "Flip X",               "",    false } },
            { WFSParameterIDs::inputFlipY,                { "Flip Y",               "",    false } },
            { WFSParameterIDs::inputFlipZ,                { "Flip Z",               "",    false } },
            { WFSParameterIDs::inputCluster,              { "Cluster",              "",    false } },
            { WFSParameterIDs::inputTrackingActive,       { "Tracking Active",      "",    false } },
            { WFSParameterIDs::inputTrackingID,           { "Tracking ID",          "",    false } },
            { WFSParameterIDs::inputTrackingSmooth,       { "Tracking Smooth",      "",    false } },
            { WFSParameterIDs::inputMaxSpeedActive,       { "Max Speed Active",     "",    false } },
            { WFSParameterIDs::inputMaxSpeed,             { "Max Speed",            "m/s", false } },
            { WFSParameterIDs::inputPathModeActive,       { "Path Mode",            "",    false } },
            { WFSParameterIDs::inputHeightFactor,         { "Height Factor",        "",    false } },
            { WFSParameterIDs::inputCoordinateMode,       { "Coordinate Mode",      "",    false } },

            // Attenuation
            { WFSParameterIDs::inputAttenuationLaw,       { "Attenuation Law",      "",    false } },
            { WFSParameterIDs::inputDistanceAttenuation,  { "Distance Atten",       "dB",  false } },
            { WFSParameterIDs::inputDistanceRatio,        { "Distance Ratio",       "",    false } },
            { WFSParameterIDs::inputCommonAtten,          { "Common Atten",         "dB",  false } },

            // Directivity
            { WFSParameterIDs::inputDirectivity,          { "Directivity",          "",    false } },
            { WFSParameterIDs::inputRotation,             { "Rotation",             "deg", false } },
            { WFSParameterIDs::inputTilt,                 { "Tilt",                 "deg", false } },
            { WFSParameterIDs::inputHFshelf,              { "HF Shelf",             "dB",  false } },

            // Live Source Tamer
            { WFSParameterIDs::inputLSactive,             { "LS Active",            "",    false } },
            { WFSParameterIDs::inputLSradius,             { "LS Radius",            "m",   false } },
            { WFSParameterIDs::inputLSshape,              { "LS Shape",             "",    false } },
            { WFSParameterIDs::inputLSattenuation,        { "LS Attenuation",       "dB",  false } },
            { WFSParameterIDs::inputLSpeakThreshold,      { "LS Peak Threshold",    "dB",  false } },
            { WFSParameterIDs::inputLSpeakRatio,          { "LS Peak Ratio",        "",    true  } },
            { WFSParameterIDs::inputLSslowThreshold,      { "LS Slow Threshold",    "dB",  false } },
            { WFSParameterIDs::inputLSslowRatio,          { "LS Slow Ratio",        "",    true  } },

            // Hackoustics (Floor Reflections)
            { WFSParameterIDs::inputFRactive,             { "FR Active",            "",    false } },
            { WFSParameterIDs::inputFRattenuation,        { "FR Attenuation",       "dB",  false } },
            { WFSParameterIDs::inputFRlowCutActive,       { "FR Low Cut Active",    "",    false } },
            { WFSParameterIDs::inputFRlowCutFreq,         { "FR Low Cut Freq",      "Hz",  false } },
            { WFSParameterIDs::inputFRhighShelfActive,    { "FR High Shelf Active", "",    false } },
            { WFSParameterIDs::inputFRhighShelfFreq,      { "FR High Shelf Freq",   "Hz",  false } },
            { WFSParameterIDs::inputFRhighShelfGain,      { "FR High Shelf Gain",   "dB",  false } },
            { WFSParameterIDs::inputFRhighShelfSlope,     { "FR High Shelf Slope",  "",    false } },
            { WFSParameterIDs::inputFRdiffusion,          { "FR Diffusion",         "",    false } },

            // Jitter
            { WFSParameterIDs::inputJitter,               { "Jitter",               "",    false } },

            // LFO
            { WFSParameterIDs::inputLFOactive,            { "LFO Active",           "",    false } },
            { WFSParameterIDs::inputLFOperiod,            { "LFO Period",           "s",   false } },
            { WFSParameterIDs::inputLFOphase,             { "LFO Phase",            "deg", false } },
            { WFSParameterIDs::inputLFOshapeX,            { "LFO Shape X",          "",    false } },
            { WFSParameterIDs::inputLFOshapeY,            { "LFO Shape Y",          "",    false } },
            { WFSParameterIDs::inputLFOshapeZ,            { "LFO Shape Z",          "",    false } },
            { WFSParameterIDs::inputLFOrateX,             { "LFO Rate X",           "Hz",  false } },
            { WFSParameterIDs::inputLFOrateY,             { "LFO Rate Y",           "Hz",  false } },
            { WFSParameterIDs::inputLFOrateZ,             { "LFO Rate Z",           "Hz",  false } },
            { WFSParameterIDs::inputLFOamplitudeX,        { "LFO Amplitude X",      "m",   false } },
            { WFSParameterIDs::inputLFOamplitudeY,        { "LFO Amplitude Y",      "m",   false } },
            { WFSParameterIDs::inputLFOamplitudeZ,        { "LFO Amplitude Z",      "m",   false } },
            { WFSParameterIDs::inputLFOphaseX,            { "LFO Phase X",          "deg", false } },
            { WFSParameterIDs::inputLFOphaseY,            { "LFO Phase Y",          "deg", false } },
            { WFSParameterIDs::inputLFOphaseZ,            { "LFO Phase Z",          "deg", false } },
            { WFSParameterIDs::inputLFOgyrophone,         { "LFO Gyrophone",        "",    false } },

            // AutomOtion
            { WFSParameterIDs::inputOtomoX,               { "AutomOtion X",         "m",   false } },
            { WFSParameterIDs::inputOtomoY,               { "AutomOtion Y",         "m",   false } },
            { WFSParameterIDs::inputOtomoZ,               { "AutomOtion Z",         "m",   false } },
            { WFSParameterIDs::inputOtomoAbsoluteRelative, { "AutomOtion Abs/Rel",  "",    false } },
            { WFSParameterIDs::inputOtomoStayReturn,      { "AutomOtion Stay/Return", "",  false } },
            { WFSParameterIDs::inputOtomoSpeedProfile,    { "AutomOtion Speed",     "",    false } },
            { WFSParameterIDs::inputOtomoDuration,        { "AutomOtion Duration",  "s",   false } },
            { WFSParameterIDs::inputOtomoCurve,           { "AutomOtion Curve",     "",    false } },
            { WFSParameterIDs::inputOtomoTrigger,         { "AutomOtion Trigger",   "",    false } },
            { WFSParameterIDs::inputOtomoThreshold,       { "AutomOtion Threshold", "dB",  false } },
            { WFSParameterIDs::inputOtomoReset,           { "AutomOtion Reset",     "",    false } },
            { WFSParameterIDs::inputOtomoPauseResume,     { "AutomOtion Pause",     "",    false } },

            // Mutes
            { WFSParameterIDs::inputMutes,                { "Mutes",                "",    false } },
            { WFSParameterIDs::inputMuteMacro,            { "Mute Macro",           "",    false } },

            // Sidelines
            { WFSParameterIDs::inputSidelinesActive,      { "Sidelines Active",     "",    false } },
            { WFSParameterIDs::inputSidelinesFringe,      { "Sidelines Fringe",     "m",   false } },

            // Reverb Sends

            // Array Attenuation
            { WFSParameterIDs::inputArrayAtten1,          { "Array 1 Atten",        "dB",  false } },
            { WFSParameterIDs::inputArrayAtten2,          { "Array 2 Atten",        "dB",  false } },
            { WFSParameterIDs::inputArrayAtten3,          { "Array 3 Atten",        "dB",  false } },
            { WFSParameterIDs::inputArrayAtten4,          { "Array 4 Atten",        "dB",  false } },
            { WFSParameterIDs::inputArrayAtten5,          { "Array 5 Atten",        "dB",  false } },
            { WFSParameterIDs::inputArrayAtten6,          { "Array 6 Atten",        "dB",  false } },
            { WFSParameterIDs::inputArrayAtten7,          { "Array 7 Atten",        "dB",  false } },
            { WFSParameterIDs::inputArrayAtten8,          { "Array 8 Atten",        "dB",  false } },
            { WFSParameterIDs::inputArrayAtten9,          { "Array 9 Atten",        "dB",  false } },
            { WFSParameterIDs::inputArrayAtten10,         { "Array 10 Atten",       "dB",  false } },
        };

        return map;
    }

    /**
     * Format a descriptive QLab cue name.
     * Format: "Input <channelId> <paramName> <value><unit>"
     * Compression ratios are formatted as "1:<value>".
     */
    static juce::String formatCueName (
        const juce::Identifier& paramId,
        int channelId,
        const juce::var& value)
    {
        // Look up display info
        const auto& displayMap = getParamDisplayMap();
        auto it = displayMap.find (paramId);

        juce::String displayName;
        juce::String unit;
        bool isRatio = false;

        if (it != displayMap.end())
        {
            displayName = it->second.displayName;
            unit = it->second.unit;
            isRatio = it->second.isCompressionRatio;
        }
        else
        {
            // Derive name from parameter ID: strip "input" prefix, insert spaces before uppercase
            displayName = paramId.toString();
            if (displayName.startsWith ("input"))
                displayName = displayName.substring (5);

            // Insert spaces before uppercase letters
            juce::String spaced;
            for (int i = 0; i < displayName.length(); ++i)
            {
                auto ch = displayName[i];
                if (i > 0 && juce::CharacterFunctions::isUpperCase (ch))
                    spaced += ' ';
                spaced += ch;
            }
            displayName = spaced;
        }

        // Format value
        juce::String strVal = value.toString();
        juce::String formattedValue;

        if (isRatio)
        {
            formattedValue = "1:" + juce::String (strVal.getDoubleValue(), 1);
        }
        else if (strVal.containsChar ('.'))
        {
            formattedValue = juce::String (strVal.getDoubleValue(), 1);
            if (unit.isNotEmpty())
                formattedValue += " " + unit;
        }
        else
        {
            formattedValue = juce::String (strVal.getIntValue());
            if (unit.isNotEmpty())
                formattedValue += " " + unit;
        }

        return "Input " + juce::String (channelId) + " " + displayName + " " + formattedValue;
    }
};

} // namespace WFSNetwork
