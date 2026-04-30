#pragma once

#include <JuceHeader.h>
#include "../MCPToolRegistry.h"
#include "../MCPChangeRecords.h"
#include "../MCPParameterRegistry.h"
#include "../../OSCParameterBounds.h"
#include "../../../Parameters/WFSValueTreeState.h"
#include "../../../Parameters/WFSParameterIDs.h"

namespace WFSNetwork::Tools::SetParameterBatch
{

/** Hard cap on writes per call. The 5 s tool-execution timeout in
    MCPDispatcher comfortably handles 100 ValueTree writes; larger
    batches should be split client-side both to stay within that budget
    and to keep the resulting undo entry from being unwieldy. */
inline constexpr int kMaxBatchSize = 100;

inline juce::var buildSchema()
{
    // Per-entry shape mirrors wfs_set_parameter's required fields.
    auto entryVariable = std::make_unique<juce::DynamicObject>();
    entryVariable->setProperty ("type", "string");
    entryVariable->setProperty ("description",
        "Canonical parameter identifier (case-sensitive). Synonyms like "
        "stageOriginX/Y/Z resolve to the canonical name automatically.");

    auto entryValue = std::make_unique<juce::DynamicObject>();
    entryValue->setProperty ("description",
        "Value to write. Type matches the underlying parameter.");

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
    entryProps->setProperty ("value",      juce::var (entryValue.release()));
    entryProps->setProperty ("channel_id", juce::var (entryChannelId.release()));
    entryProps->setProperty ("band",       juce::var (entryBand.release()));

    auto entryRequired = juce::Array<juce::var>();
    entryRequired.add ("variable");
    entryRequired.add ("value");

    auto entrySchema = std::make_unique<juce::DynamicObject>();
    entrySchema->setProperty ("type", "object");
    entrySchema->setProperty ("properties", juce::var (entryProps.release()));
    entrySchema->setProperty ("required",   juce::var (entryRequired));
    entrySchema->setProperty ("additionalProperties", false);

    auto writes = std::make_unique<juce::DynamicObject>();
    writes->setProperty ("type", "array");
    writes->setProperty ("minItems", 1);
    writes->setProperty ("maxItems", kMaxBatchSize);
    writes->setProperty ("items", juce::var (entrySchema.release()));
    writes->setProperty ("description",
        "List of writes to apply atomically. Each entry has the same shape "
        "as wfs_set_parameter (variable + value, plus channel_id for "
        "per-channel params and band for output-EQ). Up to "
        + juce::String (kMaxBatchSize) + " entries per call.");

    auto confirm = std::make_unique<juce::DynamicObject>();
    confirm->setProperty ("type", "string");
    confirm->setProperty ("description",
        "Confirmation token. Tier-2 handshake covers the whole batch - "
        "the operator's tier-2 auto-confirm window (Network tab) skips "
        "this when active.");

    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("writes",  juce::var (writes.release()));
    props->setProperty ("confirm", juce::var (confirm.release()));

    auto required = juce::Array<juce::var>();
    required.add ("writes");

    auto schema = std::make_unique<juce::DynamicObject>();
    schema->setProperty ("type", "object");
    schema->setProperty ("properties", juce::var (props.release()));
    schema->setProperty ("required",   juce::var (required));
    schema->setProperty ("additionalProperties", false);
    return juce::var (schema.release());
}

namespace detail
{
    enum class EqFamily { None, Output, ReverbPre, ReverbPost };

    inline EqFamily classifyEq (const juce::String& variable)
    {
        if (variable.startsWith ("reverbPostEQ")) return EqFamily::ReverbPost;
        if (variable.startsWith ("reverbPreEQ"))  return EqFamily::ReverbPre;
        if (variable.startsWith ("eq"))           return EqFamily::Output;  // eqShape, eqFrequency, ...
        return EqFamily::None;
    }

    /** Per-entry resolution: everything pre-validated, ready to apply. */
    struct ResolvedWrite
    {
        int sourceIndex = -1;            // index in caller's writes array
        juce::String requestedVariable;  // raw name from input (may be a synonym)
        juce::String variable;           // canonical name
        juce::var    value;              // type-coerced
        int channelIndex = -1;           // -1 for global; 0-based otherwise
        int bandIndex    = -1;           // -1 for non-EQ
        int displayId    = 0;            // 1-based for affectedGroups
        juce::String groupName;          // CSV section, used in affectedGroups
        EqFamily eqFamily = EqFamily::None;
        int tier = 1;
    };

    /** Build the dynamic-object error envelope for a per-entry failure.
        Mirrors the JSON-RPC tool-error shape but lives inside the OK
        ToolResult so the AI can see the entire batch's validation
        outcome at once. */
    inline ToolResult fail (const juce::String& code, int failureIndex,
                              const juce::String& message,
                              juce::Array<juce::var> didYouMean = {})
    {
        auto err = std::make_unique<juce::DynamicObject>();
        err->setProperty ("failure_index", failureIndex);
        if (! didYouMean.isEmpty())
            err->setProperty ("did_you_mean", juce::var (didYouMean));
        return ToolResult::error (code,
            "Batch rejected at index " + juce::String (failureIndex)
            + ": " + message
            + (didYouMean.isEmpty() ? juce::String()
                                     : " (suggestions in details)"));
    }

    /** Read the current value for the (variable, channel, band) tuple.
        Used to capture the per-write `before` state before any write
        lands. Mirrors SetParameterTool's read path. */
    inline juce::var readCurrent (WFSValueTreeState& state, const ResolvedWrite& w)
    {
        if (w.eqFamily == EqFamily::Output && w.bandIndex >= 0 && w.channelIndex >= 0)
        {
            auto bandTree = state.getOutputEQBand (w.channelIndex, w.bandIndex);
            return bandTree.isValid() ? bandTree.getProperty (juce::Identifier (w.variable))
                                       : juce::var();
        }
        if (w.eqFamily == EqFamily::ReverbPre && w.bandIndex >= 0 && w.channelIndex >= 0)
        {
            auto bandTree = state.getReverbEQBand (w.channelIndex, w.bandIndex);
            return bandTree.isValid() ? bandTree.getProperty (juce::Identifier (w.variable))
                                       : juce::var();
        }
        if (w.eqFamily == EqFamily::ReverbPost && w.bandIndex >= 0)
        {
            auto bandTree = state.getReverbPostEQBand (w.bandIndex);
            return bandTree.isValid() ? bandTree.getProperty (juce::Identifier (w.variable))
                                       : juce::var();
        }
        return state.getParameter (juce::Identifier (w.variable), w.channelIndex);
    }

    /** Apply one resolved write. The pre-validation pass already proved
        the variable is known and the value in range; this just routes
        to the correct setter. */
    inline void applyOne (WFSValueTreeState& state, const ResolvedWrite& w)
    {
        const juce::Identifier paramId (w.variable);
        if (w.eqFamily == EqFamily::Output && w.bandIndex >= 0 && w.channelIndex >= 0)
        {
            state.setOutputEQBandParameterWithArrayPropagation (w.channelIndex, w.bandIndex, paramId, w.value);
            return;
        }
        if (w.eqFamily == EqFamily::ReverbPre && w.bandIndex >= 0 && w.channelIndex >= 0)
        {
            auto bandTree = state.getReverbEQBand (w.channelIndex, w.bandIndex);
            if (bandTree.isValid())
                bandTree.setProperty (paramId, w.value, state.getActiveUndoManager());
            return;
        }
        if (w.eqFamily == EqFamily::ReverbPost && w.bandIndex >= 0)
        {
            auto bandTree = state.getReverbPostEQBand (w.bandIndex);
            if (bandTree.isValid())
                bandTree.setProperty (paramId, w.value, state.getActiveUndoManager());
            return;
        }
        state.setParameter (paramId, w.value, w.channelIndex);
    }
} // namespace detail

inline ToolResult batch (WFSValueTreeState& state, const juce::var& args, ChangeRecord* record)
{
    using namespace detail;

    if (! args.isObject())
        return ToolResult::error ("invalid_args", "Arguments must be a JSON object");
    auto* obj = args.getDynamicObject();

    const auto writesVar = obj->getProperty ("writes");
    if (! writesVar.isArray())
        return ToolResult::error ("invalid_args", "Missing required arg: writes (array)");
    const auto* writesArr = writesVar.getArray();
    if (writesArr->isEmpty())
        return ToolResult::error ("invalid_args", "writes array is empty");
    if (writesArr->size() > kMaxBatchSize)
        return ToolResult::error ("invalid_args",
            "Batch size " + juce::String (writesArr->size())
            + " exceeds limit " + juce::String (kMaxBatchSize)
            + "; split into smaller batches.");

    auto& reg = MCPParameterRegistry::getInstance();

    // ---- PASS 1: resolve + validate every entry, fail fast on first error.
    std::vector<ResolvedWrite> resolved;
    resolved.reserve (writesArr->size());

    juce::Array<int> tier3Offenders;
    juce::StringArray tier3Variables;

    for (int i = 0; i < writesArr->size(); ++i)
    {
        const auto& entryVar = (*writesArr)[i];
        if (! entryVar.isObject())
            return fail ("invalid_args", i, "entry is not a JSON object");
        auto* entryObj = entryVar.getDynamicObject();

        ResolvedWrite w;
        w.sourceIndex = i;
        w.requestedVariable = entryObj->getProperty ("variable").toString();
        if (w.requestedVariable.isEmpty())
            return fail ("invalid_args", i, "missing 'variable'");
        if (! entryObj->hasProperty ("value"))
            return fail ("invalid_args", i, "missing 'value'");
        w.value = entryObj->getProperty ("value");

        // Whitelist + did-you-mean.
        if (reg.size() > 0 && ! reg.isKnown (w.requestedVariable))
        {
            juce::Array<juce::var> suggestions;
            for (const auto& s : reg.suggestSimilar (w.requestedVariable))
                suggestions.add (s);
            return fail ("unknown_parameter", i,
                "unknown parameter '" + w.requestedVariable + "'",
                std::move (suggestions));
        }
        w.variable = reg.canonicalize (w.requestedVariable);

        // Channel + band coordinates.
        if (entryObj->hasProperty ("channel_id"))
        {
            w.displayId = static_cast<int> (entryObj->getProperty ("channel_id"));
            w.channelIndex = w.displayId - 1;
            if (w.channelIndex < 0)
                return fail ("invalid_args", i,
                    "channel_id out of range: " + juce::String (w.displayId));
        }
        w.eqFamily = classifyEq (w.variable);
        if (entryObj->hasProperty ("band"))
        {
            w.bandIndex = static_cast<int> (entryObj->getProperty ("band")) - 1;
            if (w.bandIndex < 0)
                return fail ("invalid_args", i, "band out of range");
        }

        // Enum string -> int coercion (mirrors SetParameterTool). Run
        // BEFORE the numeric coercion so labels like "Dome" don't get
        // rejected as "not numeric".
        if (w.value.isString())
        {
            if (auto resolved = reg.resolveEnumLabel (w.variable, w.value.toString()))
                w.value = juce::var (*resolved);
        }

        // String -> number coercion when the param has numeric bounds.
        const juce::Identifier paramId (w.variable);
        const auto bounds = WFSNetwork::getBounds (paramId);
        if (bounds.has_value() && w.value.isString())
        {
            const auto s = w.value.toString().trim();
            if (s.isEmpty() || ! s.containsOnly ("0123456789.+-eE"))
                return fail ("invalid_args", i,
                    "value not numeric for " + w.variable + ": " + s.quoted());
            w.value = juce::var (s.getDoubleValue());
        }

        // Range gate.
        if (w.value.isDouble() || w.value.isInt() || w.value.isInt64())
        {
            const double d = static_cast<double> (w.value);
            if (! WFSNetwork::isInRange (paramId, d))
                return fail ("out_of_range", i,
                    WFSNetwork::formatOutOfRangeReason (paramId, d));

            // Type-coerce to match declared int-vs-float so an MCP-sent
            // int doesn't land in a float slot (mirrors wfs_set_parameter).
            if (bounds.has_value())
            {
                if (bounds->isInt) w.value = juce::var (juce::roundToInt (d));
                else               w.value = juce::var (d);
            }
        }

        // Tier lookup. Tier-3 sub-writes are disallowed in batches —
        // they each carry their own destructive risk profile that
        // doesn't generalise across a multi-write commit.
        if (auto* rec = reg.findByVariable (w.variable))
        {
            w.tier = rec->tier;
            w.groupName = rec->csvSection;  // matches the auto-gen path
        }
        if (w.tier == 3)
        {
            tier3Offenders.add (i);
            tier3Variables.add (w.variable);
        }

        resolved.push_back (std::move (w));
    }

    if (! tier3Offenders.isEmpty())
    {
        auto details = std::make_unique<juce::DynamicObject>();
        juce::Array<juce::var> indicesArr;
        for (int idx : tier3Offenders) indicesArr.add (idx);
        details->setProperty ("indices", juce::var (indicesArr));
        juce::Array<juce::var> namesArr;
        for (const auto& v : tier3Variables) namesArr.add (v);
        details->setProperty ("variables", juce::var (namesArr));
        return ToolResult::error ("tier_3_in_batch",
            "Batch contains tier-3 (destructive) writes that must be issued "
            "individually so each gets its own confirmation: "
            + tier3Variables.joinIntoString (", "));
    }

    // ---- PASS 2: capture before-values (one read per entry) before any write.
    std::vector<juce::var> beforeValues;
    beforeValues.reserve (resolved.size());
    for (const auto& w : resolved)
        beforeValues.push_back (readCurrent (state, w));

    // ---- PASS 3: apply all writes.
    for (const auto& w : resolved)
        applyOne (state, w);

    // ---- PASS 4: capture after-values (one read per entry).
    std::vector<juce::var> afterValues;
    afterValues.reserve (resolved.size());
    for (const auto& w : resolved)
        afterValues.push_back (readCurrent (state, w));

    // ---- Build the change record envelope.
    if (record != nullptr)
    {
        // SubWrites: per-entry payload for the undo engine.
        record->subWrites.reserve (resolved.size());

        // Synthesized flat before/after: last-write-per-paramId wins. The
        // staleness check + cross-actor notification path key off paramId
        // alone, so this matches existing semantics.
        auto flatBefore = std::make_unique<juce::DynamicObject>();
        auto flatAfter  = std::make_unique<juce::DynamicObject>();

        // Deduped collections for affectedGroups + affectedParameters.
        juce::StringArray seenParams;
        std::vector<std::pair<int, juce::String>> seenGroups;

        // Per-section summary for the operator description.
        std::map<juce::String, int> sectionCounts;

        for (size_t i = 0; i < resolved.size(); ++i)
        {
            const auto& w = resolved[i];
            const juce::Identifier paramId (w.variable);

            ChangeSubWrite sw;
            sw.channelIndex = w.channelIndex;
            sw.bandIndex    = w.bandIndex;

            auto subBefore = std::make_unique<juce::DynamicObject>();
            subBefore->setProperty (paramId, beforeValues[i]);
            sw.beforeState = juce::var (subBefore.release());

            auto subAfter = std::make_unique<juce::DynamicObject>();
            subAfter->setProperty (paramId, afterValues[i]);
            sw.afterState = juce::var (subAfter.release());

            record->subWrites.push_back (std::move (sw));

            // Synthesized flat states: last value per paramId wins
            // (mirrors lastWriterByParamId semantics).
            flatBefore->setProperty (paramId, beforeValues[i]);
            flatAfter ->setProperty (paramId, afterValues[i]);

            seenParams.addIfNotAlreadyThere (w.variable);

            // Use the entry's group name; fall back to "batch" if the
            // registry didn't carry one for this variable.
            const auto group = w.groupName.isNotEmpty() ? w.groupName : juce::String ("batch");
            std::pair<int, juce::String> key { w.displayId, group };
            if (std::find (seenGroups.begin(), seenGroups.end(), key) == seenGroups.end())
                seenGroups.push_back (key);

            sectionCounts[group]++;
        }

        record->beforeState = juce::var (flatBefore.release());
        record->afterState  = juce::var (flatAfter.release());
        record->affectedParameters = std::move (seenParams);
        for (const auto& g : seenGroups)
            record->affectedGroups.push_back ({ g.first, g.second });

        // Compose a one-line description: "Batch: 36 writes (12 Position,
        // 12 Orientation, 12 Pitch)".
        juce::String desc = "Batch: " + juce::String ((int) resolved.size()) + " writes";
        if (! sectionCounts.empty())
        {
            juce::StringArray parts;
            for (const auto& [section, count] : sectionCounts)
                parts.add (juce::String (count) + " " + section);
            desc += " (" + parts.joinIntoString (", ") + ")";
        }
        record->operatorDescription = desc;
    }

    // ---- Return result envelope.
    juce::Array<juce::var> applied;
    for (size_t i = 0; i < resolved.size(); ++i)
    {
        const auto& w = resolved[i];
        auto entry = std::make_unique<juce::DynamicObject>();
        entry->setProperty ("variable", w.variable);
        if (w.requestedVariable != w.variable)
            entry->setProperty ("synonym_of", w.requestedVariable);
        if (w.channelIndex >= 0)
            entry->setProperty ("channel_id", w.displayId);
        if (w.bandIndex >= 0)
            entry->setProperty ("band", w.bandIndex + 1);
        entry->setProperty ("before", beforeValues[i]);
        entry->setProperty ("after",  afterValues[i]);
        applied.add (juce::var (entry.release()));
    }

    auto root = std::make_unique<juce::DynamicObject>();
    root->setProperty ("count", static_cast<int> (resolved.size()));
    root->setProperty ("applied", juce::var (applied));
    if (record != nullptr)
        root->setProperty ("operator_description", record->operatorDescription);
    return ToolResult::ok (juce::var (root.release()));
}

inline ToolDescriptor describe (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "wfs_set_parameter_batch";
    d.description = "Apply up to " + juce::String (kMaxBatchSize) + " parameter "
                    "writes atomically and record them as a single undoable "
                    "entry. Pre-validates everything (whitelist + did-you-mean, "
                    "range, type coercion, tier) before touching state - any "
                    "failure rejects the entire batch with no writes applied. "
                    "Tier-3 (destructive) variables are not allowed in a batch; "
                    "issue those individually. Each entry has the same shape as "
                    "wfs_set_parameter ({variable, value, channel_id?, band?}). "
                    "Use this for setting many channels or many parameters in "
                    "one operator action - e.g. positioning a 12-speaker array.";
    d.inputSchema   = buildSchema();
    d.modifiesState = true;
    d.tier        = 2;
    d.handler = [&state] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        return batch (state, args, record);
    };
    return d;
}

} // namespace WFSNetwork::Tools::SetParameterBatch
