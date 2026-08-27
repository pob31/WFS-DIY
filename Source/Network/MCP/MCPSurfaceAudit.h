#pragma once

#include <JuceHeader.h>
#include "../../Parameters/WFSValueTreeState.h"
#include "../../Parameters/WFSParameterIDs.h"

namespace WFSNetwork::SurfaceAudit
{

/** Does the app's advertised MCP tool surface describe writes it can perform?

    The generated tool manifest, the ValueTree schema and the resolver in
    WFSValueTreeState are maintained independently, so a parameter can be
    advertised long after — or long before — anything can store it. The write
    seam gives no help: TreeParameterStore::setParameter is
    `if (tree.isValid()) write(...)` with no else and a void return, so a
    parameter that resolves to nothing is dropped in silence. That is how
    roughly a quarter of the surface came to be inert without anyone noticing.

    This is the check that would have caught it. Two callers share it so they
    cannot drift apart: the startup auditor (always on, reports to the Network
    Log) and the WFS_TEST_MCP_SURFACE self-test (on demand, reports PASS/FAIL). */
struct Result
{
    int checked = 0;                  ///< entries actually judged
    int skippedSubTree = 0;           ///< resolve their own node; see below
    int skippedTemplate = 0;          ///< numeric-suffix families ("{array}")
    int skippedOverridden = 0;        ///< a hand-written tool of the same name wins
    int skippedNoChannel = 0;         ///< session has no channel of that kind
    juce::StringArray skippedKinds;   ///< which kinds those were
    juce::StringArray deadVariables;  ///< distinct variables that cannot be written
    juce::StringArray deadDetails;    ///< "tool 'x' advertises 'y'" lines

    int deadCount() const { return deadDetails.size(); }
};

/** Names supplied by a hand-written tool registered AFTER the generated set,
    which silently overwrites by name. Asking whether setParameter could carry
    the write is then asking about code that never runs. */
inline bool isHandWrittenOverride (const juce::String& toolName)
{
    return toolName == "system_i_o_set_input_channels"
        || toolName == "system_i_o_set_stereo_input_channels";
}

/** Parameters stored below the level one channel index can address — sampler
    cells and sets, gradient layers and shapes, ADM mappings and axes, network
    targets, EQ bands. The dispatcher resolves the node itself and writes it
    directly, so setParameter is not on that path either. Detected from the
    schema, plus the three gradient aliases which carry their index in the NAME
    and so have no argument to spot. */
inline bool isSubTreeRouted (const juce::DynamicObject* props, const juce::String& variable)
{
    if (variable == "gmLayer0Enabled" || variable == "gmLayer1Enabled"
        || variable == "gmLayer2Enabled")
        return true;

    if (props == nullptr)
        return false;

    for (const char* sub : { "band", "layer", "shape", "cell_id",
                             "set_id", "mapping", "axis", "target_id" })
        if (props->hasProperty (sub))
            return true;

    return false;
}

/** Walk a parsed generated_tools.json against a live state and report what it
    advertises but cannot write. `manifest` is the parsed root object. */
inline Result run (const juce::var& manifest, WFSValueTreeState& state)
{
    Result r;
    auto* root = manifest.getDynamicObject();
    if (root == nullptr)
        return r;

    // A per-channel parameter is only judged when a channel of that kind
    // exists, or the answer is about the index rather than the parameter: a
    // default session has no reverb channels, and asking about reverb slot 0
    // would indict every reverb parameter in the app.
    const int firstInputNumber = state.getNumInputChannels() > 0
                                   ? state.getInputChannelNumber (0) : 0;

    auto walk = [&] (const juce::var& arr, const juce::String& label)
    {
        if (! arr.isArray())
            return;

        for (const auto& entry : *arr.getArray())
        {
            auto* obj = entry.getDynamicObject();
            if (obj == nullptr)
                continue;

            const auto variable = obj->getProperty ("internal_variable").toString();
            if (variable.isEmpty())
            {
                if (obj->getProperty ("internal_variable_template").toString().isNotEmpty())
                    ++r.skippedTemplate;
                continue;
            }

            const auto toolName = obj->getProperty ("name").toString();
            if (isHandWrittenOverride (toolName))
            {
                ++r.skippedOverridden;
                continue;
            }

            juce::String channelArg;
            juce::DynamicObject* props = nullptr;
            if (auto* params = obj->getProperty ("parameters").getDynamicObject())
                if ((props = params->getProperty ("properties").getDynamicObject()) != nullptr)
                    for (const char* candidate : { "input_id", "output_id",
                                                   "reverb_id", "cluster_id" })
                        if (props->hasProperty (candidate))
                            { channelArg = candidate; break; }

            if (isSubTreeRouted (props, variable))
            {
                ++r.skippedSubTree;
                continue;
            }

            const int liveOfKind = channelArg.isEmpty()      ? 1
                                 : channelArg == "input_id"  ? state.getNumInputChannels()
                                 : channelArg == "output_id" ? state.getNumOutputChannels()
                                 : channelArg == "reverb_id" ? state.getNumReverbChannels()
                                                             : WFSParameterDefaults::maxClusters;
            if (liveOfKind <= 0)
            {
                ++r.skippedNoChannel;
                r.skippedKinds.addIfNotAlreadyThere (channelArg);
                continue;
            }

            const int channelIndex = channelArg.isEmpty() ? -1
                                   : channelArg == "input_id"
                                        ? state.getSlotForChannelNumber (firstInputNumber)
                                        : 0;

            ++r.checked;
            if (! state.canWriteParameter (juce::Identifier (variable), channelIndex))
            {
                r.deadVariables.addIfNotAlreadyThere (variable);
                r.deadDetails.add ("[" + label + "] '" + toolName
                                   + "' advertises '" + variable + "' but no write can land");
            }
        }
    };

    walk (root->getProperty ("tools"),       "set");
    walk (root->getProperty ("nudge_tools"), "nudge");
    return r;
}

/** One-line summary, shared so both callers phrase it identically. */
inline juce::String summarise (const Result& r)
{
    juce::String s;
    s << r.checked << " registrations checked, " << r.deadCount() << " unwritable ("
      << r.deadVariables.size() << " distinct parameters); skipped: "
      << r.skippedSubTree << " sub-tree routed, "
      << r.skippedTemplate << " templated, "
      << r.skippedOverridden << " hand-written override, "
      << r.skippedNoChannel << " no live channel";
    if (! r.skippedKinds.isEmpty())
        s << " (" << r.skippedKinds.joinIntoString (", ") << ")";
    return s;
}

} // namespace WFSNetwork::SurfaceAudit
