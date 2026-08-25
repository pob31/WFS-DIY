#pragma once

#include <JuceHeader.h>
#include <map>
#include <optional>
#include <vector>

#include "WFSParameterIDs.h"
#include "WFSParameterDefaults.h"

/**
    Input channel IDENTITY, as a value that can be read from a file without
    applying it and compared against the live session.

    Three keyings cross the file boundary and disagree independently:
      - channel parameter data, snapshot entries and snapshot scope go by
        permanent NUMBER (mergeTreeRecursive matches <Input> by id);
      - patchData rows go by SLOT (row = position);
      - and until this existed, nothing carried the channel <-> hardware-input
        relation at all.

    So when a session's (position <-> number <-> type) relation differs from a
    file's, parameter data lands wrong BY NUMBER while the patch lands wrong BY
    POSITION - in opposite directions at once. A user who hand-rebuilt an
    arrangement to match a saved config got mono channels carrying stereo
    parameter sets, and could never have done otherwise: dragging changes
    position, the merge reads number. This is the comparison that says so before
    the load, instead of after.

    Everything here is pure: no GUI, no latch, no writes.
*/
struct InputChannelRef
{
    int slot = -1;
    int number = 0;
    juce::String name;          // empty when the source has none (system.xml)
    bool stereo = false;
    std::vector<int> hwInputs;  // 1-based hardware inputs the patch row holds; 0..2 entries

    bool isFullyPatched() const { return (int) hwInputs.size() == (stereo ? 2 : 1); }
};

struct InputChannelIdentity
{
    enum class Source { none, inventory, inputNodes, snapshot, liveTree };

    Source source = Source::none;
    std::vector<InputChannelRef> slots;   // DISPLAY order
    bool typesKnown = false;              // snapshots carry no type
    bool orderKnown = false;              // snapshots carry no display order
    bool hwKnown    = false;              // the source carried hardware fingerprints
    int  legacyCount = 0;                 // <IO inputChannels> sum, only when source == none

    bool hasIdentity() const { return source != Source::none && ! slots.empty(); }

    /** <InputChannelList><Ch n= type=/>... — mirrors applyInputChannelInventory's
        parse exactly: n outside 1..maxInputChannels is skipped, first occurrence
        of a duplicate wins. */
    static InputChannelIdentity fromInventory (const juce::ValueTree& inputChannelList);

    /** <Inputs><Input id= inputChannelType= hwInputs=>... — mirrors
        migrateInputChannelModel exactly: any missing or duplicated id means the
        load will renumber the whole list dense 1..N; if NO node carries a type,
        the last `legacyStereoTail` are stereo; a partially typed list defaults
        the untyped to mono. */
    static InputChannelIdentity fromInputNodes (const juce::ValueTree& inputs, int legacyStereoTail);

    /** A snapshot's <Inputs><Input id= hwInputs=>: numbers and fingerprints only. */
    static InputChannelIdentity fromSnapshot (const juce::ValueTree& inputs);
};

struct InputChannelIdentityDiff
{
    /** Priority order: a file with no identity, then identical, then the same
        (number, type) multiset in a different order, then the same TYPES position
        by position with different numbers - the hand-rebuilt arrangement - then
        anything else. */
    enum class Relation { identical, orderOnly, positionalTypesMatch, conflicting, fileHasNoIdentity };

    struct Retyped       { InputChannelRef live; bool fileStereo = false; };
    struct PatchMismatch { InputChannelRef live; std::vector<int> fileHwInputs; };

    Relation relation = Relation::identical;

    std::vector<Retyped>         retyped;        // same number, different type
    std::vector<InputChannelRef> removed;        // live numbers the file lacks
    std::vector<InputChannelRef> added;          // file numbers the session lacks
    std::vector<PatchMismatch>   patchDiffers;   // same number, different hardware inputs

    std::vector<int> fileNumbersBySlot;          // relabel by position (positionalTypesMatch)
    std::vector<int> fileNumbersInOrder;         // reorder to the file's order (orderOnly)

    /** Live slot -> file number, derived by matching hardware fingerprints.
        Present ONLY when it is a clean one-to-one match over every live channel
        with agreeing types and no empty or half fingerprints, and when it would
        actually change something. It assumes nobody re-cabled between save and
        load, which only the operator knows - it is a suggestion to confirm, never
        something to apply on its own. */
    std::optional<std::vector<int>> hardwareRelabel;

    int fileLegacyCount = 0;                     // when fileHasNoIdentity

    bool anyDifference() const
    {
        return relation != Relation::identical || ! patchDiffers.empty();
    }
};

InputChannelIdentityDiff compareInputChannelIdentity (const InputChannelIdentity& live,
                                                       const InputChannelIdentity& file);

//==============================================================================
// Implementation
//==============================================================================

namespace InputChannelIdentityDetail
{
    inline std::vector<int> parseHwInputs (const juce::var& v)
    {
        std::vector<int> out;
        if (v.isVoid())
            return out;
        juce::StringArray tokens;
        tokens.addTokens (v.toString(), ",", "");
        for (const auto& t : tokens)
        {
            const int hw = t.trim().getIntValue();
            if (hw > 0)
                out.push_back (hw);
        }
        return out;
    }

    inline juce::String hwInputsToString (const std::vector<int>& hw)
    {
        juce::StringArray s;
        for (int h : hw)
            s.add (juce::String (h));
        return s.joinIntoString (",");
    }
}

inline InputChannelIdentity InputChannelIdentity::fromInventory (const juce::ValueTree& list)
{
    using namespace WFSParameterIDs;
    InputChannelIdentity out;
    if (! list.isValid() || ! list.hasType (InputChannelList))
        return out;

    std::vector<int> seen;
    for (int i = 0; i < list.getNumChildren(); ++i)
    {
        auto ch = list.getChild (i);
        if (! ch.hasType (Ch))
            continue;
        const int number = static_cast<int> (ch.getProperty (chNumber, 0));
        if (number <= 0 || number > WFSParameterDefaults::maxInputChannels)
            continue;
        if (std::find (seen.begin(), seen.end(), number) != seen.end())
            continue;
        seen.push_back (number);

        InputChannelRef ref;
        ref.slot   = (int) out.slots.size();
        ref.number = number;
        ref.stereo = ch.getProperty (chType).toString() == WFSParameterDefaults::inputChannelTypeStereo;
        out.slots.push_back (std::move (ref));
    }

    if (! out.slots.empty())
    {
        out.source     = Source::inventory;
        out.typesKnown = true;
        out.orderKnown = true;
    }
    return out;
}

inline InputChannelIdentity InputChannelIdentity::fromInputNodes (const juce::ValueTree& inputs, int legacyStereoTail)
{
    using namespace WFSParameterIDs;
    InputChannelIdentity out;
    if (! inputs.isValid())
        return out;

    std::vector<juce::ValueTree> nodes;
    for (int i = 0; i < inputs.getNumChildren(); ++i)
        if (inputs.getChild (i).hasType (Input))
            nodes.push_back (inputs.getChild (i));
    if (nodes.empty())
        return out;

    // Ids: the load repairs any missing/duplicate id with ONE dense renumber,
    // so that is what this must report - not the ids as written.
    bool idsSound = true;
    std::vector<int> seen;
    for (auto& n : nodes)
    {
        const int number = static_cast<int> (n.getProperty (id, 0));
        if (number <= 0 || number > WFSParameterDefaults::maxInputChannels
            || std::find (seen.begin(), seen.end(), number) != seen.end())
        {
            idsSound = false;
            break;
        }
        seen.push_back (number);
    }

    bool anyTyped = false;
    for (auto& n : nodes)
        if (n.hasProperty (inputChannelType)) { anyTyped = true; break; }

    const int total = (int) nodes.size();
    for (int i = 0; i < total; ++i)
    {
        auto& n = nodes[(size_t) i];
        InputChannelRef ref;
        ref.slot   = i;
        ref.number = idsSound ? static_cast<int> (n.getProperty (id, 0)) : i + 1;
        ref.stereo = anyTyped
                         ? n.getProperty (inputChannelType).toString() == WFSParameterDefaults::inputChannelTypeStereo
                         : (i >= total - legacyStereoTail);
        auto channel = n.getChildWithName (Channel);
        if (channel.isValid())
            ref.name = channel.getProperty (inputName).toString();
        if (n.hasProperty (hwInputs))
        {
            ref.hwInputs = InputChannelIdentityDetail::parseHwInputs (n.getProperty (hwInputs));
            out.hwKnown = true;
        }
        out.slots.push_back (std::move (ref));
    }

    out.source     = Source::inputNodes;
    out.typesKnown = true;
    out.orderKnown = true;
    return out;
}

inline InputChannelIdentity InputChannelIdentity::fromSnapshot (const juce::ValueTree& inputs)
{
    using namespace WFSParameterIDs;
    InputChannelIdentity out;
    if (! inputs.isValid())
        return out;

    for (int i = 0; i < inputs.getNumChildren(); ++i)
    {
        auto n = inputs.getChild (i);
        if (! n.hasType (Input))
            continue;
        const int number = static_cast<int> (n.getProperty (id, 0));
        if (number <= 0)
            continue;

        InputChannelRef ref;
        ref.slot   = (int) out.slots.size();
        ref.number = number;
        auto channel = n.getChildWithName (Channel);
        if (channel.isValid())
            ref.name = channel.getProperty (inputName).toString();
        if (n.hasProperty (hwInputs))
        {
            ref.hwInputs = InputChannelIdentityDetail::parseHwInputs (n.getProperty (hwInputs));
            out.hwKnown = true;
        }
        out.slots.push_back (std::move (ref));
    }

    if (! out.slots.empty())
        out.source = Source::snapshot;   // types and order deliberately unknown
    return out;
}

inline InputChannelIdentityDiff compareInputChannelIdentity (const InputChannelIdentity& live,
                                                              const InputChannelIdentity& file)
{
    InputChannelIdentityDiff diff;

    if (! file.hasIdentity())
    {
        diff.relation        = InputChannelIdentityDiff::Relation::fileHasNoIdentity;
        diff.fileLegacyCount = file.legacyCount;
        return diff;
    }

    const bool compareTypes = live.typesKnown && file.typesKnown;
    const bool compareHw    = live.hwKnown    && file.hwKnown;

    std::map<int, const InputChannelRef*> liveByNumber, fileByNumber;
    for (const auto& r : live.slots) liveByNumber[r.number] = &r;
    for (const auto& r : file.slots) fileByNumber[r.number] = &r;

    for (const auto& r : live.slots)
    {
        auto it = fileByNumber.find (r.number);
        if (it == fileByNumber.end())
        {
            diff.removed.push_back (r);
            continue;
        }
        if (compareTypes && it->second->stereo != r.stereo)
            diff.retyped.push_back ({ r, it->second->stereo });
        if (compareHw && it->second->hwInputs != r.hwInputs)
            diff.patchDiffers.push_back ({ r, it->second->hwInputs });
    }
    for (const auto& r : file.slots)
        if (liveByNumber.find (r.number) == liveByNumber.end())
            diff.added.push_back (r);

    // Relation. A snapshot carries no order, so for it "same numbers" is
    // identical whatever position they sit at.
    const bool sameLength = live.slots.size() == file.slots.size();
    bool identical = sameLength && diff.removed.empty() && diff.added.empty() && diff.retyped.empty();
    if (identical && file.orderKnown)
        for (size_t i = 0; i < live.slots.size(); ++i)
            if (live.slots[i].number != file.slots[i].number) { identical = false; break; }

    if (identical)
        diff.relation = InputChannelIdentityDiff::Relation::identical;
    else if (sameLength && diff.removed.empty() && diff.added.empty() && diff.retyped.empty())
        diff.relation = InputChannelIdentityDiff::Relation::orderOnly;
    else
    {
        bool positional = sameLength && compareTypes;
        if (positional)
            for (size_t i = 0; i < live.slots.size(); ++i)
                if (live.slots[i].stereo != file.slots[i].stereo) { positional = false; break; }
        diff.relation = positional ? InputChannelIdentityDiff::Relation::positionalTypesMatch
                                   : InputChannelIdentityDiff::Relation::conflicting;
    }

    for (const auto& r : file.slots)
    {
        diff.fileNumbersInOrder.push_back (r.number);
        if (sameLength)
            diff.fileNumbersBySlot.push_back (r.number);
    }

    // Hardware-derived relabel: every live slot's fingerprint must be full and
    // must match exactly one file channel, the match must be a bijection, and
    // types must agree where both are known. Anything less and the suggestion
    // is withheld rather than guessed.
    if (compareHw && diff.anyDifference())
    {
        std::vector<int> proposal;
        std::vector<int> used;
        bool clean = true;
        for (const auto& r : live.slots)
        {
            if (! r.isFullyPatched()) { clean = false; break; }
            const InputChannelRef* match = nullptr;
            for (const auto& f : file.slots)
            {
                if (f.hwInputs != r.hwInputs)
                    continue;
                if (match != nullptr) { match = nullptr; clean = false; break; }   // ambiguous
                match = &f;
            }
            if (! clean || match == nullptr) { clean = false; break; }
            if (compareTypes && match->stereo != r.stereo) { clean = false; break; }
            if (std::find (used.begin(), used.end(), match->number) != used.end()) { clean = false; break; }
            used.push_back (match->number);
            proposal.push_back (match->number);
        }
        if (clean && proposal.size() == live.slots.size())
        {
            bool changesSomething = false;
            for (size_t i = 0; i < live.slots.size(); ++i)
                if (live.slots[i].number != proposal[i]) { changesSomething = true; break; }
            if (changesSomething)
                diff.hardwareRelabel = std::move (proposal);
        }
    }

    return diff;
}
