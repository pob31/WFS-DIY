#pragma once

#include <JuceHeader.h>
#include "GoDotProtocol.h"
#include "QLabCueBuilder.h"

namespace WFSNetwork
{

/**
 * GoDotCueBuilder
 *
 * The four exports WFS-DIY writes to QLab, written as Go.dot cues instead
 * (Go.dot's docs/godot-authoring-protocol-0.1.md §7.1). A Go.dot OSC cue holds
 * many messages, sent in one tick, so where QLab gets a playlist group of one
 * network cue per parameter, Go.dot gets ONE cue: its first message is the cue's
 * own, the rest follow in order.
 *
 * Every address is the OSCQuery short form the tree publishes, and every value
 * is typed as GoDot::valueTypeTag says, because Go.dot checks the cue against
 * the description it fetched from that tree.
 *
 * The parameter walks are QLabCueBuilder's own (forEachInScopeInputParameter,
 * forEachEffectNode), so the two exports select exactly the same parameters.
 */
class GoDotCueBuilder
{
public:
    /** What a snapshot export left out, so the caller can say so. */
    struct Skipped
    {
        int oneOutputMuteRows = 0;   // a mute row of a one-output rig is a lone number, which the receiver refuses as a row
        int notNumbers = 0;          // text stored where the parameter is a number
        int unnamed = 0;             // a parameter with no OSCQuery name
    };

    /** The snapshot's in-scope parameters as one cue: the inputs, then the
        effects. Takes what QLabCueBuilder::buildSnapshotCues takes. */
    static GoDot::Cue buildSnapshotCue (const juce::String& snapshotName,
                                        const juce::ValueTree& snapshotData,
                                        const WFSFileManager::ExtendedSnapshotScope& scope,
                                        int numChannels,
                                        const std::function<int (int)>& numberToSlot,
                                        int numOutputs,
                                        const juce::ValueTree& effectsData,
                                        int numEffects,
                                        Skipped* skipped = nullptr)
    {
        GoDot::Cue cue;
        cue.name = LOC ("snapshot.qlabGroupName").replace ("{name}", snapshotName);

        Skipped local;
        auto& gaps = skipped != nullptr ? *skipped : local;

        for (int i = 0; i < snapshotData.getNumChildren(); ++i)
        {
            const auto inputData = snapshotData.getChild (i);
            const int channelId = static_cast<int> (inputData.getProperty (WFSParameterIDs::id, 0));
            const int channelIndex = QLabCueBuilder::slotOf (channelId, numberToSlot, numChannels);

            if (channelIndex < 0)
                continue;

            QLabCueBuilder::forEachInScopeInputParameter (inputData, channelIndex, scope, numOutputs,
                [&] (const juce::Identifier& paramId, const juce::String&, const juce::var& value, bool isMuteList)
                {
                    const auto name = GoDot::inputNameOf (paramId);
                    if (name.isEmpty())
                    {
                        ++gaps.unnamed;
                        return;
                    }

                    // The mute node is a string: always the whole row. A one-output
                    // row is a lone "0" or "1", which the receiver refuses as a row.
                    if (isMuteList && ! value.toString().containsChar (','))
                    {
                        ++gaps.oneOutputMuteRows;
                        return;
                    }

                    const auto tag = GoDot::valueTypeTag (paramId);
                    if (tag != "s" && ! GoDot::isNumber (value.toString()))
                    {
                        ++gaps.notNumbers;
                        return;
                    }

                    cue.messages.emplace_back ("/wfs/input/" + juce::String (channelId) + "/" + name,
                                               GoDot::atom (tag[0], value));
                });
        }

        appendEffectMessages (cue, effectsData, scope.effects, numEffects, numOutputs, gaps);
        return cue;
    }

    /** The cue that loads a snapshot from its file: /wfs/input/snapshot/load "<name>". */
    static GoDot::Cue buildSnapshotLoadCue (const juce::String& snapshotName)
    {
        GoDot::Cue cue;
        cue.name = LOC ("snapshot.qlabReloadName").replace ("{name}", snapshotName);
        cue.messages.emplace_back ("/wfs/input/snapshot/load", GoDot::quoted (snapshotName));
        return cue;
    }

    /** /wfs/input/<n>/samplerSet <set, 1-based>. */
    static GoDot::Cue buildSamplerSetCue (int channelId, int setNumber, const juce::String& setName)
    {
        GoDot::Cue cue;
        cue.name = LOC ("sampler.qlabSetCueName")
                       .replace ("{channel}", juce::String (channelId))
                       .replace ("{name}", setName);
        cue.messages.emplace_back ("/wfs/input/" + juce::String (channelId) + "/samplerSet",
                                   GoDot::intAtom (setNumber));
        return cue;
    }

    /** /wfs/cluster/<n>/lfoPresetRecall <preset, 1-based>. */
    static GoDot::Cue buildClusterLFOPresetCue (int clusterId, int presetNumber, const juce::String& presetName)
    {
        GoDot::Cue cue;
        cue.name = LOC ("clusters.qlabPresetCueName")
                       .replace ("{cluster}", juce::String (clusterId))
                       .replace ("{name}", presetName);
        cue.messages.emplace_back ("/wfs/cluster/" + juce::String (clusterId) + "/lfoPresetRecall",
                                   GoDot::intAtom (presetNumber));
        return cue;
    }

private:
    /** The effects half, in the shapes OSCMessageRouter::getEffectParamKind reads
        after /wfs/effect/<ID>/<name>: the sub-indices, then the value. The same
        parameters QLabCueBuilder::collectEffectCues exports. */
    static void appendEffectMessages (GoDot::Cue& cue, const juce::ValueTree& effectsData,
                                      const WFSFileManager::ScopeMatrix& grid,
                                      int numEffects, int numOutputs, Skipped& gaps)
    {
        using Kind = OSCMessageRouter::ParsedEffectMessage::Kind;
        const auto& paths = OSCMessageBuilder::getEffectMappings();

        QLabCueBuilder::forEachEffectNode (effectsData, numEffects,
            [&] (const juce::ValueTree& node, const juce::Identifier& nodeType,
                 int instance, int band, int tap, int effectId)
            {
                for (int p = 0; p < node.getNumProperties(); ++p)
                {
                    const auto paramId = node.getPropertyName (p);
                    if (paramId == WFSParameterIDs::id || paramId == WFSParameterIDs::effectName)
                        continue;

                    const auto itemId = EffectsSnapshotScope::itemIdFor (nodeType, paramId);
                    if (itemId.isEmpty() || ! grid.isIncluded (itemId, effectId - 1))
                        continue;

                    if (paths.find (paramId) == paths.end())
                        continue;

                    const auto name = GoDot::effectNameOf (paramId);
                    if (name.isEmpty())
                    {
                        ++gaps.unnamed;
                        continue;
                    }

                    const auto address = "/wfs/effect/" + juce::String (effectId) + "/" + name;
                    const auto value = node.getProperty (paramId);
                    const auto tag = GoDot::valueTypeTag (paramId);
                    juce::String atoms;

                    switch (OSCMessageRouter::getEffectParamKind (paramId))
                    {
                        case Kind::Scalar:
                            break;

                        case Kind::Instanced:
                            if (instance < 1) continue;
                            atoms << GoDot::intAtom (instance) << " ";
                            break;

                        case Kind::Band:
                            if (instance < 1 || band < 1) continue;
                            atoms << GoDot::intAtom (instance) << " " << GoDot::intAtom (band) << " ";
                            break;

                        case Kind::Tap:
                            if (tap < 1) continue;
                            atoms << GoDot::intAtom (tap) << " ";
                            break;

                        case Kind::Row:
                        {
                            const auto text = paramId == WFSParameterIDs::effectMutes
                                                  ? WFSValueTreeState::normaliseMuteList (value, numOutputs)
                                                  : value.toString().trim();

                            // A row is ONE string; a lone number is not one, and the
                            // receiver refuses it rather than wipe the row.
                            if (text.isEmpty() || GoDot::isNumber (text))
                            {
                                ++gaps.oneOutputMuteRows;
                                continue;
                            }

                            cue.messages.emplace_back (address, GoDot::quoted (text));
                            continue;
                        }

                        case Kind::Unknown:
                        case Kind::InputCell:
                        case Kind::FxCell:
                        case Kind::Global:
                        case Kind::Verb:
                        default:
                            continue;
                    }

                    if (tag != "s" && ! GoDot::isNumber (value.toString()))
                    {
                        ++gaps.notNumbers;
                        continue;
                    }

                    atoms << GoDot::atom (tag[0], value);
                    cue.messages.emplace_back (address, atoms);
                }
            });
    }
};

} // namespace WFSNetwork
