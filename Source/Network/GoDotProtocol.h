#pragma once

#include <JuceHeader.h>
#include "OSCParameterBounds.h"
#include "OSCMessageRouter.h"

#include <map>
#include <utility>
#include <vector>

/**
 * GoDotProtocol
 *
 * What WFS-DIY says to Go.dot, and how it spells it. The contract is Go.dot's
 * docs/godot-authoring-protocol-0.1.md (in the go.dot repository, sibling to
 * this one); this header is its WFS-DIY half, kept in one place so that the
 * OSCQuery tree WFS-DIY serves and the cues WFS-DIY writes into Go.dot can
 * never disagree about an address or a type.
 *
 * Why that matters: Go.dot fetches WFS-DIY's OSCQuery description when WFS-DIY
 * declares itself, and from then on checks every cue aimed at /wfs against it.
 * A cue that names an address the tree does not have, or sends a string where
 * the tree says float, fails at GO. So the tree and the builder both ask this
 * header: one address per parameter (the OSCQuery short form,
 * /wfs/input/<n>/<name>, /wfs/effect/<ID>/<name>), one type per parameter.
 */
namespace WFSNetwork::GoDot
{
    //==========================================================================
    // Addresses
    //==========================================================================

    /** The root WFS-DIY declares in Go.dot. */
    inline constexpr const char* prefix = "/wfs";

    inline constexpr const char* declareAddress   = "/godot/cmd/mount/declare";
    inline constexpr const char* captureAddress   = "/godot/cmd/cue/capture";
    inline constexpr const char* declaredAddress  = "/godot/declared";
    inline constexpr const char* describedAddress = "/godot/described";
    inline constexpr const char* capturedAddress  = "/godot/captured";

    /** Go.dot's answers all live under /godot/; nothing else WFS-DIY receives does. */
    inline bool isAnswer (const juce::String& address)
    {
        return address == declaredAddress || address == describedAddress || address == capturedAddress;
    }

    /** A datagram never grows past this: the family's ceiling (WFS-DIY's own
        bundles, Go.dot's MountSender::bundleBytes), under an Ethernet frame
        with room for the headers, so it is never fragmented on the way. */
    inline constexpr int datagramCeiling = 1200;

    /** How long a capture waits for each answer it expects. */
    inline constexpr int answerTimeoutMs = 2000;

    //==========================================================================
    // Types
    //==========================================================================

    /** The OSC type of a parameter's value, decided by the PARAMETER and never
        by the var holding it: "i" or "f" where the bounds table knows it,
        "s" for everything else (names, mute lists, a parameter the table does
        not cover yet). WFS-DIY's router reads a numeric string as a number,
        so "s" is always safe to send to it. */
    inline juce::String valueTypeTag (const juce::Identifier& paramId)
    {
        if (const auto bounds = getBounds (paramId))
            return bounds->isInt ? "i" : "f";
        return "s";
    }

    /** The type tags of /wfs/effect/<ID>/<name>, after the channel the address
        spends: the sub-indices the shape owns (OSCMessageRouter::getEffectParamKind)
        and then the value. Empty for a shape that is not a per-channel parameter. */
    inline juce::String effectTypeTags (const juce::Identifier& paramId)
    {
        using Kind = OSCMessageRouter::ParsedEffectMessage::Kind;

        switch (OSCMessageRouter::getEffectParamKind (paramId))
        {
            case Kind::Scalar:    return valueTypeTag (paramId);
            case Kind::Instanced: return "i" + valueTypeTag (paramId);
            case Kind::Band:      return "ii" + valueTypeTag (paramId);
            case Kind::Tap:       return "i" + valueTypeTag (paramId);
            case Kind::InputCell: return "i" + valueTypeTag (paramId);
            case Kind::FxCell:    return "i" + valueTypeTag (paramId);
            case Kind::Row:       return "s";
            case Kind::Unknown:
            case Kind::Global:
            case Kind::Verb:
            default:              return {};
        }
    }

    //==========================================================================
    // Names
    //==========================================================================

    /** The OSCQuery name of an input parameter: the key the router reads after
        /wfs/input/<n>/. Empty when it has none. */
    inline juce::String inputNameOf (const juce::Identifier& paramId)
    {
        static const std::map<juce::Identifier, juce::String> reverse = []
        {
            std::map<juce::Identifier, juce::String> r;
            for (const auto& [name, id] : OSCMessageRouter::getInputAddressMap())
                r.emplace (id, name);
            return r;
        }();

        const auto it = reverse.find (paramId);
        return it != reverse.end() ? it->second : juce::String();
    }

    /** The OSCQuery name of an effect parameter, after /wfs/effect/<ID>/. */
    inline juce::String effectNameOf (const juce::Identifier& paramId)
    {
        static const std::map<juce::Identifier, juce::String> reverse = []
        {
            std::map<juce::Identifier, juce::String> r;
            for (const auto& [name, id] : OSCMessageRouter::getEffectAddressMap())
                r.emplace (id, name);
            return r;
        }();

        const auto it = reverse.find (paramId);
        return it != reverse.end() ? it->second : juce::String();
    }

    //==========================================================================
    // Atoms: how an OSC cue's value row spells its arguments
    //==========================================================================

    /** A string atom: always quoted, with \\ \" and \n escaped. */
    inline juce::String quoted (const juce::String& text)
    {
        return "s:\"" + text.replace ("\\", "\\\\").replace ("\"", "\\\"").replace ("\n", "\\n") + "\"";
    }

    inline juce::String intAtom (int value) { return "i:" + juce::String (value); }

    inline juce::String floatAtom (double value) { return "f:" + juce::String (value, 6); }

    /** One argument, as the tag asks: a number is read from the text the
        snapshot file holds (ValueTree::fromXml types every property as a
        string), text goes quoted. */
    inline juce::String atom (juce::juce_wchar tag, const juce::var& value)
    {
        const auto text = value.toString().trim();

        if (tag == 'i')
            return intAtom (juce::roundToInt (text.getDoubleValue()));
        if (tag == 'f')
            return floatAtom (text.getDoubleValue());
        return quoted (value.toString());
    }

    /** True when the text reads as a number: a value that does not cannot go
        to an "i" or "f" node. */
    inline bool isNumber (const juce::String& text)
    {
        const auto t = text.trim();
        return t.isNotEmpty() && t.containsOnly ("0123456789.+-eE");
    }

    //==========================================================================
    // Identifiers
    //==========================================================================

    /** Eight characters of Crockford base32, Go.dot's identifier alphabet (no
        I, L, O or U). WFS-DIY draws its own so it can keep it beside the
        snapshot and send it again: the second export updates the same cue. */
    inline juce::String newCueId()
    {
        static const char* alphabet = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";
        auto& random = juce::Random::getSystemRandom();

        juce::String id;
        for (int i = 0; i < 8; ++i)
            id << juce::String::charToString (static_cast<juce::juce_wchar> (alphabet[random.nextInt (32)]));
        return id;
    }

    inline bool isCueId (const juce::String& text)
    {
        return text.length() == 8 && text.containsOnly ("0123456789ABCDEFGHJKMNPQRSTVWXYZ");
    }

    //==========================================================================
    // Messages
    //==========================================================================

    /** One cue to write: its own message first, then the rest, in order. */
    struct Cue
    {
        juce::String id;
        juce::String name;
        juce::String number;
        juce::String notes;
        std::vector<std::pair<juce::String, juce::String>> messages;   // address, atoms
    };

    /** /godot/cmd/mount/declare "/wfs" <port> <queryPort> "WFS-DIY" */
    inline juce::OSCMessage declareMessage (int receivePort, int queryPort)
    {
        juce::OSCMessage message (declareAddress);
        message.addString (prefix);
        message.addInt32 (receivePort);
        message.addInt32 (queryPort);
        message.addString ("WFS-DIY");
        return message;
    }

    /** The bytes an OSC string takes on the wire: its UTF-8, a terminator, padded to four. */
    inline int oscStringBytes (const juce::String& text)
    {
        return (static_cast<int> (text.getNumBytesAsUTF8()) + 4) & ~3;
    }

    /** The datagrams of one capture: the head and as many pairs as fit under
        the ceiling, then `more` chunks on the same identifier for the rest.
        A pair too long to fit even alone still goes, alone, in its own chunk. */
    inline std::vector<juce::OSCMessage> captureMessages (const juce::String& where,
                                                          const juce::String& target,
                                                          const Cue& cue)
    {
        std::vector<juce::OSCMessage> chunks;

        auto startChunk = [&] (bool first)
        {
            juce::OSCMessage message (captureAddress);
            message.addString (first ? where : juce::String ("more"));
            message.addString (first ? target : cue.id);
            message.addString (cue.id);
            message.addString (first ? cue.name : juce::String());
            message.addString (first ? cue.number : juce::String());
            message.addString (first ? cue.notes : juce::String());
            message.addString ({});    // messageIds: Go.dot fills it in its log
            return message;
        };

        auto bytesOf = [] (const juce::OSCMessage& message)
        {
            int bytes = oscStringBytes (message.getAddressPattern().toString())
                      + oscStringBytes ("," + juce::String::repeatedString ("s", message.size()));
            for (const auto& argument : message)
                bytes += oscStringBytes (argument.getString());
            return bytes;
        };

        auto current = startChunk (true);
        int pairsInChunk = 0;

        for (const auto& [address, atoms] : cue.messages)
        {
            const int pairBytes = oscStringBytes (address) + oscStringBytes (atoms) + 8;   // two more type tags at most

            if (pairsInChunk > 0 && bytesOf (current) + pairBytes > datagramCeiling)
            {
                chunks.push_back (std::move (current));
                current = startChunk (false);
                pairsInChunk = 0;
            }

            current.addString (address);
            current.addString (atoms);
            ++pairsInChunk;
        }

        chunks.push_back (std::move (current));
        return chunks;
    }

    //==========================================================================
    // Answers
    //==========================================================================

    /** /godot/captured <id> <outcome> <number> <name>, /godot/declared <id> <outcome>,
        /godot/described <id> <nodeCount> <problem>, read leniently. */
    struct Answer
    {
        juce::String address;
        juce::String id;
        juce::String outcome;
        juce::String number;
        juce::String name;
        int nodeCount = 0;
        juce::String problem;

        bool isCaptured()  const { return address == capturedAddress; }
        bool isDeclared()  const { return address == declaredAddress; }
        bool isDescribed() const { return address == describedAddress; }

        /** Created, updated or appended: anything else is a refusal's reason. */
        bool landed() const
        {
            return outcome == "created" || outcome == "updated" || outcome == "appended";
        }
    };

    inline Answer readAnswer (const juce::OSCMessage& message)
    {
        auto text = [&message] (int i)
        {
            return i < message.size() && message[i].isString() ? message[i].getString() : juce::String();
        };

        Answer answer;
        answer.address = message.getAddressPattern().toString();
        answer.id = text (0);

        if (answer.isDescribed())
        {
            answer.nodeCount = message.size() > 1 && message[1].isInt32() ? message[1].getInt32() : 0;
            answer.problem = text (2);
            answer.outcome = answer.problem.isEmpty() ? "described" : answer.problem;
            return answer;
        }

        answer.outcome = text (1);
        answer.number = text (2);
        answer.name = text (3);
        return answer;
    }
}
