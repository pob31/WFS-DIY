#pragma once

#include <JuceHeader.h>
#include "../MCPToolRegistry.h"
#include "../MCPParameterRegistry.h"
#include "../../../Parameters/WFSValueTreeState.h"
#include "../../../Parameters/WFSParameterIDs.h"

namespace WFSNetwork::Tools::GetParameter
{

inline constexpr int kMaxBatchSize = 100;

namespace detail
{
    enum class EqFamily { None, Output, ReverbPre, ReverbPost };

    inline EqFamily classifyEq (const juce::String& variable)
    {
        if (variable.startsWith ("reverbPostEQ")) return EqFamily::ReverbPost;
        if (variable.startsWith ("reverbPreEQ"))  return EqFamily::ReverbPre;
        if (variable.startsWith ("eq"))           return EqFamily::Output;
        return EqFamily::None;
    }

    /** Resolution result for one read. */
    struct Resolved
    {
        juce::String requestedVariable;
        juce::String variable;          // canonical
        int channelIndex = -1;
        int bandIndex    = -1;
        int displayId    = 0;
        EqFamily eqFamily = EqFamily::None;
    };

    /** Returns either a populated Resolved on success, or a non-empty
        ToolResult on failure (which the caller surfaces as-is for single
        reads or aggregates into the per-entry errors list for batch). */
    struct ResolveOutcome
    {
        bool ok = false;
        Resolved value;
        juce::String errorCode;
        juce::String errorMessage;
        juce::Array<juce::var> didYouMean;
    };

    inline ResolveOutcome resolveEntry (const juce::DynamicObject& entry)
    {
        ResolveOutcome out;
        out.value.requestedVariable = entry.getProperty ("variable").toString();
        if (out.value.requestedVariable.isEmpty())
        {
            out.errorCode    = "invalid_args";
            out.errorMessage = "missing 'variable'";
            return out;
        }

        const auto& reg = MCPParameterRegistry::getInstance();
        if (reg.size() > 0 && ! reg.isKnown (out.value.requestedVariable))
        {
            out.errorCode    = "unknown_parameter";
            out.errorMessage = "unknown parameter '" + out.value.requestedVariable + "'";
            for (const auto& s : reg.suggestSimilar (out.value.requestedVariable))
                out.didYouMean.add (s);
            return out;
        }
        out.value.variable = reg.canonicalize (out.value.requestedVariable);

        if (entry.hasProperty ("channel_id"))
        {
            out.value.displayId = static_cast<int> (entry.getProperty ("channel_id"));
            out.value.channelIndex = out.value.displayId - 1;
            if (out.value.channelIndex < 0)
            {
                out.errorCode    = "invalid_args";
                out.errorMessage = "channel_id out of range: "
                                   + juce::String (out.value.displayId);
                return out;
            }
        }

        out.value.eqFamily = classifyEq (out.value.variable);

        if (entry.hasProperty ("band"))
        {
            out.value.bandIndex = static_cast<int> (entry.getProperty ("band")) - 1;
            if (out.value.bandIndex < 0)
            {
                out.errorCode    = "invalid_args";
                out.errorMessage = "band out of range";
                return out;
            }
        }

        out.ok = true;
        return out;
    }

    /** Read the current value matching the resolved coordinates. */
    inline juce::var readValue (WFSValueTreeState& state, const Resolved& r)
    {
        const juce::Identifier paramId (r.variable);

        if (r.eqFamily == EqFamily::Output && r.bandIndex >= 0 && r.channelIndex >= 0)
        {
            auto bandTree = state.getOutputEQBand (r.channelIndex, r.bandIndex);
            return bandTree.isValid() ? bandTree.getProperty (paramId) : juce::var();
        }
        if (r.eqFamily == EqFamily::ReverbPre && r.bandIndex >= 0 && r.channelIndex >= 0)
        {
            auto bandTree = state.getReverbEQBand (r.channelIndex, r.bandIndex);
            return bandTree.isValid() ? bandTree.getProperty (paramId) : juce::var();
        }
        if (r.eqFamily == EqFamily::ReverbPost && r.bandIndex >= 0)
        {
            auto bandTree = state.getReverbPostEQBand (r.bandIndex);
            return bandTree.isValid() ? bandTree.getProperty (paramId) : juce::var();
        }
        return state.getParameter (paramId, r.channelIndex);
    }

    inline juce::var entryToVar (const Resolved& r, const juce::var& value)
    {
        auto obj = std::make_unique<juce::DynamicObject>();
        obj->setProperty ("variable", r.variable);
        if (r.variable != r.requestedVariable)
            obj->setProperty ("synonym_of", r.requestedVariable);
        if (r.channelIndex >= 0)
            obj->setProperty ("channel_id", r.displayId);
        if (r.bandIndex >= 0)
            obj->setProperty ("band", r.bandIndex + 1);
        obj->setProperty ("value", value);
        return juce::var (obj.release());
    }
} // namespace detail

//==============================================================================
// wfs_get_parameter — single read
//==============================================================================

inline juce::var singleSchema()
{
    auto variable = std::make_unique<juce::DynamicObject>();
    variable->setProperty ("type", "string");
    variable->setProperty ("description",
        "Canonical parameter identifier (case-sensitive). Synonyms like "
        "stageOriginX/Y/Z resolve to the canonical name automatically.");

    auto channelId = std::make_unique<juce::DynamicObject>();
    channelId->setProperty ("type", "integer");
    channelId->setProperty ("minimum", 1);
    channelId->setProperty ("description",
        "Channel number (1-based) for per-channel parameters. Omit for globals.");

    auto band = std::make_unique<juce::DynamicObject>();
    band->setProperty ("type", "integer");
    band->setProperty ("minimum", 1);
    band->setProperty ("maximum", 6);
    band->setProperty ("description",
        "EQ band number (1-6) for output-EQ parameters. Omit for non-EQ.");

    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("variable",   juce::var (variable.release()));
    props->setProperty ("channel_id", juce::var (channelId.release()));
    props->setProperty ("band",       juce::var (band.release()));

    auto required = juce::Array<juce::var>();
    required.add ("variable");

    auto schema = std::make_unique<juce::DynamicObject>();
    schema->setProperty ("type", "object");
    schema->setProperty ("properties", juce::var (props.release()));
    schema->setProperty ("required", juce::var (required));
    schema->setProperty ("additionalProperties", false);
    return juce::var (schema.release());
}

inline ToolResult getOne (WFSValueTreeState& state, const juce::var& args)
{
    if (! args.isObject())
        return ToolResult::error ("invalid_args", "Arguments must be a JSON object");

    auto outcome = detail::resolveEntry (*args.getDynamicObject());
    if (! outcome.ok)
    {
        juce::String message = outcome.errorMessage;
        if (! outcome.didYouMean.isEmpty())
        {
            juce::StringArray suggestions;
            for (const auto& s : outcome.didYouMean) suggestions.add (s.toString());
            message += ". Did you mean: " + suggestions.joinIntoString (", ")
                     + "? Use mcp_describe_parameters to browse the registry.";
        }
        return ToolResult::error (outcome.errorCode, message);
    }

    const auto value = detail::readValue (state, outcome.value);
    return ToolResult::ok (detail::entryToVar (outcome.value, value));
}

inline ToolDescriptor describeSingle (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "wfs_get_parameter";
    d.description = "Read the current value of one parameter. Mirrors "
                    "wfs_set_parameter's argument shape: {variable, "
                    "channel_id?, band?}. Whitelist + synonym + did-you-mean "
                    "behave the same as the writer. Returns {variable, "
                    "channel_id?, band?, value, synonym_of?}. Use "
                    "wfs_get_parameters for batch reads, "
                    "session_get_channel_full for everything on one channel, "
                    "or session_get_state_delta to track operator/OSC writes "
                    "between turns. Tier 1, no confirmation.";
    d.inputSchema   = singleSchema();
    d.modifiesState = false;
    d.tier        = 1;
    d.handler = [&state] (const juce::var& args, ChangeRecord*) -> ToolResult
    {
        return getOne (state, args);
    };
    return d;
}

//==============================================================================
// wfs_get_parameters — batch read
//==============================================================================

inline juce::var batchSchema()
{
    auto entryVariable = std::make_unique<juce::DynamicObject>();
    entryVariable->setProperty ("type", "string");
    entryVariable->setProperty ("description",
        "Canonical parameter identifier (case-sensitive). Synonyms resolve.");

    auto entryChannelId = std::make_unique<juce::DynamicObject>();
    entryChannelId->setProperty ("type", "integer");
    entryChannelId->setProperty ("minimum", 1);
    entryChannelId->setProperty ("description",
        "Channel number (1-based) for per-channel parameters. Omit for globals.");

    auto entryBand = std::make_unique<juce::DynamicObject>();
    entryBand->setProperty ("type", "integer");
    entryBand->setProperty ("minimum", 1);
    entryBand->setProperty ("maximum", 6);
    entryBand->setProperty ("description",
        "EQ band number (1-6) for output-EQ parameters. Omit for non-EQ.");

    auto entryProps = std::make_unique<juce::DynamicObject>();
    entryProps->setProperty ("variable",   juce::var (entryVariable.release()));
    entryProps->setProperty ("channel_id", juce::var (entryChannelId.release()));
    entryProps->setProperty ("band",       juce::var (entryBand.release()));

    auto entryRequired = juce::Array<juce::var>();
    entryRequired.add ("variable");

    auto entrySchema = std::make_unique<juce::DynamicObject>();
    entrySchema->setProperty ("type", "object");
    entrySchema->setProperty ("properties", juce::var (entryProps.release()));
    entrySchema->setProperty ("required",   juce::var (entryRequired));
    entrySchema->setProperty ("additionalProperties", false);

    auto reads = std::make_unique<juce::DynamicObject>();
    reads->setProperty ("type", "array");
    reads->setProperty ("minItems", 1);
    reads->setProperty ("maxItems", kMaxBatchSize);
    reads->setProperty ("items", juce::var (entrySchema.release()));
    reads->setProperty ("description",
        "List of reads. Each entry has the same shape as wfs_get_parameter. "
        "Up to " + juce::String (kMaxBatchSize) + " entries per call. "
        "Per-entry failures land in `errors[]` keyed by index; valid entries "
        "still return their values in `results[]`.");

    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("reads", juce::var (reads.release()));

    auto required = juce::Array<juce::var>();
    required.add ("reads");

    auto schema = std::make_unique<juce::DynamicObject>();
    schema->setProperty ("type", "object");
    schema->setProperty ("properties", juce::var (props.release()));
    schema->setProperty ("required",   juce::var (required));
    schema->setProperty ("additionalProperties", false);
    return juce::var (schema.release());
}

inline ToolResult getBatch (WFSValueTreeState& state, const juce::var& args)
{
    if (! args.isObject())
        return ToolResult::error ("invalid_args", "Arguments must be a JSON object");
    auto* obj = args.getDynamicObject();
    const auto readsVar = obj->getProperty ("reads");
    if (! readsVar.isArray())
        return ToolResult::error ("invalid_args", "Missing required arg: reads (array)");
    const auto* readsArr = readsVar.getArray();
    if (readsArr->isEmpty())
        return ToolResult::error ("invalid_args", "reads array is empty");
    if (readsArr->size() > kMaxBatchSize)
        return ToolResult::error ("invalid_args",
            "Batch size " + juce::String (readsArr->size())
            + " exceeds limit " + juce::String (kMaxBatchSize));

    juce::Array<juce::var> results;
    juce::Array<juce::var> errors;

    for (int i = 0; i < readsArr->size(); ++i)
    {
        const auto& entryVar = (*readsArr)[i];
        if (! entryVar.isObject())
        {
            auto err = std::make_unique<juce::DynamicObject>();
            err->setProperty ("index",   i);
            err->setProperty ("code",    "invalid_args");
            err->setProperty ("message", "entry is not a JSON object");
            errors.add (juce::var (err.release()));
            continue;
        }
        auto outcome = detail::resolveEntry (*entryVar.getDynamicObject());
        if (! outcome.ok)
        {
            auto err = std::make_unique<juce::DynamicObject>();
            err->setProperty ("index",   i);
            err->setProperty ("code",    outcome.errorCode);
            err->setProperty ("message", outcome.errorMessage);
            if (! outcome.didYouMean.isEmpty())
                err->setProperty ("did_you_mean", juce::var (outcome.didYouMean));
            errors.add (juce::var (err.release()));
            continue;
        }
        const auto value = detail::readValue (state, outcome.value);
        results.add (detail::entryToVar (outcome.value, value));
    }

    auto root = std::make_unique<juce::DynamicObject>();
    root->setProperty ("count",   results.size());
    root->setProperty ("results", juce::var (results));
    root->setProperty ("errors",  juce::var (errors));
    return ToolResult::ok (juce::var (root.release()));
}

inline ToolDescriptor describeBatch (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "wfs_get_parameters";
    d.description = "Read up to " + juce::String (kMaxBatchSize) + " parameters "
                    "in one call. Mirrors wfs_set_parameter_batch's entry shape "
                    "(reads of {variable, channel_id?, band?}). Per-entry "
                    "failures (unknown name, bad channel) surface in `errors[]` "
                    "with their index; valid entries still return values in "
                    "`results[]`. Tier 1, no confirmation.";
    d.inputSchema   = batchSchema();
    d.modifiesState = false;
    d.tier        = 1;
    d.handler = [&state] (const juce::var& args, ChangeRecord*) -> ToolResult
    {
        return getBatch (state, args);
    };
    return d;
}

} // namespace WFSNetwork::Tools::GetParameter
