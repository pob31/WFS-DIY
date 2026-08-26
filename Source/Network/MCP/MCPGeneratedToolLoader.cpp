#include "MCPGeneratedToolLoader.h"
#include "MCPGenericDispatch.h"
#include "MCPCompat.h"
#include "MCPLogger.h"
#include "../../Parameters/WFSValueTreeState.h"
#include "../../Parameters/WFSParameterIDs.h"

namespace WFSNetwork::Tools::Generated
{

// ToolBinding / NudgeBinding and the two dispatch entry points now live in
// namespace Detail (declared in MCPGenericDispatch.h) so hand-written tools
// can share them. Everything below in the anonymous namespace remains
// private to the loader.
using Detail::ToolBinding;
using Detail::NudgeBinding;
using Detail::coerceValue;
using Detail::channelArgToScopeLabel;
using Detail::dispatchGenericSet;
using Detail::dispatchGenericNudge;

namespace
{
    /** Catalog policy: which generated tools appear in tools/list.

        There are ~393 of these, and listing them all made tools/list about
        240KB — roughly 60-70k tokens loaded into a model's context before
        it had been asked anything. Tier-1 and tier-2 entries are therefore
        hidden: `wfs_set_parameter` (and its batch form) covers the same
        parameters, `wfs_nudge_parameter` covers relative moves, and every
        hidden tool remains callable by name for a client that knows it —
        `mcp_describe_parameters` reports the owning `tool_name` for each
        parameter.

        Tier-3 is the deliberate exception. `wfs_set_parameter` refuses
        tier-3 parameters outright, so hiding those eight tools would leave
        the most destructive operations reachable only by guessing an
        unlisted name. They cost ~5KB and stay visible, sorted first, which
        is exactly where an operator-gated action belongs.

        To restore the old full catalog, return true unconditionally — the
        hidden tools are otherwise registered identically. */
    bool shouldListGeneratedTool (int tier) noexcept
    {
        return tier >= 3;
    }

    /** Build a lowercase-keyed lookup of every property name on the live
        ValueTree at startup. Some CSVs declare variables with different
        casing than `WFSParameterIDs.h` (e.g. `StageWidth` vs `stageWidth`);
        this lets the loader resolve to the canonical form before
        constructing `juce::Identifier` (which is case-sensitive). */
    void collectPropertyNamesRecursive (const juce::ValueTree& tree,
                                        std::map<juce::String, juce::String>& out)
    {
        for (int i = 0; i < tree.getNumProperties(); ++i)
        {
            auto canonical = tree.getPropertyName (i).toString();
            out.emplace (canonical.toLowerCase(), canonical);
        }
        for (int i = 0; i < tree.getNumChildren(); ++i)
            collectPropertyNamesRecursive (tree.getChild (i), out);
    }

    std::map<juce::String, juce::String> buildKnownPropertyMap (const WFSValueTreeState& state)
    {
        std::map<juce::String, juce::String> m;
        collectPropertyNamesRecursive (state.getState(), m);
        return m;
    }

    juce::DynamicObject* asObject (const juce::var& v)
    {
        return v.isObject() ? v.getDynamicObject() : nullptr;
    }

    /** Detect which channel-id arg this tool expects (if any). Per-channel
        tools list exactly one of {input_id, output_id, reverb_id, cluster_id}
        in their parameters; global tools list none. */
    juce::String detectChannelArg (const juce::var& parametersSchema)
    {
        auto* obj = asObject (parametersSchema);
        if (obj == nullptr) return {};

        auto props = obj->getProperty ("properties");
        auto* propsObj = asObject (props);
        if (propsObj == nullptr) return {};

        static const juce::StringArray candidates { "input_id", "output_id", "reverb_id", "cluster_id" };
        for (const auto& c : candidates)
            if (propsObj->hasProperty (c))
                return c;
        return {};
    }

    /** Resolve a per-channel tool's 1-based id arg to a 0-based slot.

        input_id is a permanent channel number, so the list can carry gaps after
        deletions and it has to go through the state lookup rather than id - 1.
        Naming a channel by number also makes that number an external reference
        an MCP client will quote back later, so the numbering is frozen first —
        without that, a drag-reorder in the GUI between two tool calls would
        silently retarget the cached id. output_id / reverb_id / cluster_id are
        dense slot positions rather than permanent numbers, and must leave input
        numbering alone. */
    int resolveChannelSlot (WFSValueTreeState& state,
                            const juce::String& channelArgName,
                            int displayId)
    {
        if (channelArgName != "input_id")
            return displayId - 1;

        state.markChannelNumbersUserOwned ("MCP tool addressing an input by number");
        return state.getSlotForChannelNumber (displayId);
    }
}  // anonymous namespace

// ---------------------------------------------------------------------------
// Shared dispatch — declared in MCPGenericDispatch.h. Kept in this file so the
// generated-tool path and the hand-written generic tools run identical logic.
// ---------------------------------------------------------------------------
namespace Detail
{
    /** Convert "<scope>_id" → "<scope>" for human-readable descriptions. */
    juce::String channelArgToScopeLabel (const juce::String& argName)
    {
        if (argName == "input_id")   return "input";
        if (argName == "output_id")  return "output";
        if (argName == "reverb_id")  return "reverb";
        if (argName == "cluster_id") return "cluster";
        return argName;
    }

    /** If the tool's value schema declares an `enum` of strings, map the
        incoming arg from string to its 0-based index in the enum array.
        First try an exact match; if that fails, retry with whitespace
        stripped from both sides — the generator emits "Cluster1" but
        humans / AIs commonly send "Cluster 1" matching the CSV display
        form. On unknown enum values pass through unchanged so the
        validation in dispatchGenericSet can produce a clear error. */
    juce::var coerceValue (const juce::var& incoming, const ToolBinding& binding)
    {
        if (binding.enumValues.size() > 0 && incoming.isString())
        {
            const auto s = incoming.toString();
            for (int i = 0; i < binding.enumValues.size(); ++i)
                if (binding.enumValues[i] == s)
                    return juce::var (i);

            const auto sNoSpace = s.removeCharacters (" \t");
            for (int i = 0; i < binding.enumValues.size(); ++i)
                if (binding.enumValues[i].removeCharacters (" \t") == sNoSpace)
                    return juce::var (i);
        }
        return incoming;
    }

    /** The shared handler logic for every generated setter tool. */
    ToolResult dispatchGenericSet (WFSValueTreeState& state,
                                   const ToolBinding& binding,
                                   const juce::var& args,
                                   ChangeRecord* record)
    {
        if (! args.isObject())
            return ToolResult::error ("invalid_args", "Arguments must be a JSON object");

        auto* argsObj = asObject (args);

        // Resolve channel index (1-based MCP arg → 0-based ValueTree index).
        // input_id is a permanent channel number — the input list may have
        // gaps after deletions, so it resolves through the state lookup.
        int channelIndex = -1;
        int displayId = 0;  // 1-based for descriptions and affected_groups
        if (binding.channelArgName.isNotEmpty())
        {
            if (! argsObj->hasProperty (binding.channelArgName))
                return ToolResult::error ("invalid_args",
                                          "Missing required arg: " + binding.channelArgName);
            displayId = static_cast<int> (argsObj->getProperty (binding.channelArgName));
            channelIndex = resolveChannelSlot (state, binding.channelArgName, displayId);
            if (channelIndex < 0)
                return ToolResult::error ("invalid_args",
                                          binding.channelArgName + " not a live channel: " + juce::String (displayId));
        }

        // Resolve EQ band sub-index (1-based → 0-based)
        int bandIndex = -1;
        if (binding.isEqBand)
        {
            if (! argsObj->hasProperty ("band"))
                return ToolResult::error ("invalid_args", "Missing required arg: band");
            bandIndex = static_cast<int> (argsObj->getProperty ("band")) - 1;
            if (bandIndex < 0)
                return ToolResult::error ("invalid_args", "band out of range");
        }

        // Resolve and coerce value. The arg name is "value" for most tools
        // but renamed for self-documenting cases ("name" for renames, "mode"
        // for coordinate-mode dropdowns, etc.) — driven by `value_arg_name`
        // in generated_tools.json.
        if (! argsObj->hasProperty (binding.valueArgName))
            return ToolResult::error ("invalid_args",
                                      "Missing required arg: " + binding.valueArgName);
        juce::var value = coerceValue (argsObj->getProperty (binding.valueArgName), binding);

        // Enum validation. After coerceValue (above) has tried both
        // exact and whitespace-tolerant string-to-index lookups, a
        // remaining string can only be valid if it's a numeric literal
        // (loose-typed harnesses sometimes wrap ints in strings). A
        // remaining double must be a whole number. Anything else —
        // "2.7" sent for a cluster selector, fractional doubles, or
        // out-of-range indices — is rejected so the bad value never
        // lands in the ValueTree slot.
        if (! binding.enumValues.isEmpty())
        {
            auto rejectValue = [&] (const juce::String& shown)
            {
                return ToolResult::error ("invalid_enum_value",
                                          "value " + shown.quoted()
                                          + " is not a valid " + binding.internalVariable
                                          + " value (expected an integer index 0.."
                                          + juce::String (binding.enumValues.size() - 1)
                                          + " or one of: " + binding.enumValues.joinIntoString (", ") + ")");
            };

            if (value.isString())
            {
                const auto s = value.toString().trim();
                if (s.isEmpty() || ! s.containsOnly ("0123456789.+-eE"))
                    return rejectValue (s);
                const double d = s.getDoubleValue();
                if (d != std::floor (d))
                    return rejectValue (s);
                value = juce::var (static_cast<int> (d));
            }
            else if (value.isDouble())
            {
                const double d = static_cast<double> (value);
                if (d != std::floor (d))
                    return rejectValue (juce::String (d));
                value = juce::var (static_cast<int> (d));
            }

            if (! (value.isInt() || value.isInt64()))
                return rejectValue (value.toString());
            const int idx = static_cast<int> (value);
            if (idx < 0 || idx >= binding.enumValues.size())
                return rejectValue (juce::String (idx));
        }

        // Range gate: the schema declares `minimum`/`maximum` for numeric
        // value args; fail closed on out-of-range so a confirmed AI write
        // can't push outputAttenuation = +1e9 etc. Mirrors the OSC ingress
        // policy in OSCParameterBounds (rejected, not silently clamped).
        // Skipped for enum tools (the string→index path already validated
        // membership in coerceValue).
        // Loose-typed clients (e.g. an MCP harness emitting JSON numbers
        // as strings) get coerced to a numeric var here; the coerced
        // numeric is then both range-checked AND written to the
        // ValueTree, preventing string-in-float-slot corruption.
        if (binding.hasRange && binding.enumValues.isEmpty())
        {
            if (value.isString())
            {
                const auto s = value.toString().trim();
                // Reject empty / non-numeric strings up front
                if (s.isEmpty() || ! s.containsOnly ("0123456789.+-eE"))
                    return ToolResult::error ("invalid_args",
                                              "value not numeric for " + binding.internalVariable
                                              + ": " + s.quoted());
                value = juce::var (s.getDoubleValue());
            }
            if (value.isDouble() || value.isInt() || value.isInt64())
            {
                const double d = static_cast<double> (value);
                if (d < binding.minValue || d > binding.maxValue)
                    return ToolResult::error ("out_of_range",
                                              "value " + juce::String (d, 6)
                                              + " not in [" + juce::String (binding.minValue, 6)
                                              + ", " + juce::String (binding.maxValue, 6) + "] for "
                                              + binding.internalVariable);
            }
        }

        // Type-coerce the var to match the parameter's declared schema type
        // before writing. Stops int-typed vars landing in float ValueTree
        // slots (and vice-versa) when an MCP client sends a numeric of the
        // wrong-but-equivalent type. Skipped for enums (already integer-
        // mapped above) and string-typed params.
        if (binding.isNumericType && binding.enumValues.isEmpty()
            && (value.isDouble() || value.isInt() || value.isInt64()))
        {
            if (binding.isIntegerType)
                value = juce::var (juce::roundToInt (static_cast<double> (value)));
            else
                value = juce::var (static_cast<double> (value));
        }

        const juce::Identifier paramId (binding.internalVariable);

        // Resolve the EQ-band ValueTree for the three EQ families. Output
        // EQ uses array-propagation semantics; reverb pre-EQ is per-
        // channel (simple band lookup); reverb post-EQ is global (no
        // channel index). The dispatcher detects these from the param-
        // name prefix because the schema's `band` arg alone doesn't tell
        // us which family.
        enum class EqFamily { Output, ReverbPre, ReverbPost };
        auto eqFamily = [&]() -> EqFamily
        {
            if (binding.internalVariable.startsWith ("reverbPostEQ")) return EqFamily::ReverbPost;
            if (binding.internalVariable.startsWith ("reverbPreEQ"))  return EqFamily::ReverbPre;
            return EqFamily::Output;
        }();

        auto getBandTree = [&]() -> juce::ValueTree
        {
            switch (eqFamily)
            {
                case EqFamily::ReverbPost: return state.getReverbPostEQBand (bandIndex);
                case EqFamily::ReverbPre:  return state.getReverbEQBand (channelIndex, bandIndex);
                case EqFamily::Output:
                default:                   return state.getOutputEQBand (channelIndex, bandIndex);
            }
        };

        // Refuse before writing, rather than reporting a success we did not earn.
        // The generic path bottoms out in TreeParameterStore::setParameter, which is
        // `if (tree.isValid()) write(...)` with no else and a void return — so a
        // parameter this state cannot resolve is dropped in total silence. Every
        // such tool used to answer ok with the REQUESTED value echoed back, which is
        // why whole families of them were dead for a long time without anyone
        // noticing. EQ bands write their subtree directly and are checked by
        // getBandTree() below, so they are exempt.
        if (! binding.isEqBand && ! state.canWriteParameter (paramId, channelIndex))
        {
            return ToolResult::error ("unwritable_parameter",
                                      "Parameter '" + binding.internalVariable
                                        + "' is advertised but cannot be written: the app "
                                          "has no resolvable home for it. This is an app "
                                          "defect, not a bad argument - please report it.");
        }

        // Capture before-state
        juce::var beforeValue;
        if (binding.isEqBand)
        {
            auto band = getBandTree();
            if (band.isValid())
                beforeValue = band.getProperty (paramId);
        }
        else
        {
            beforeValue = state.getParameter (paramId, channelIndex);
        }

        // Write
        if (binding.isEqBand)
        {
            if (eqFamily == EqFamily::Output)
            {
                state.setOutputEQBandParameterWithArrayPropagation (channelIndex, bandIndex, paramId, value);
            }
            else
            {
                auto band = getBandTree();
                if (band.isValid())
                    band.setProperty (paramId, value, state.getActiveUndoManager());
            }
        }
        else
        {
            state.setParameter (paramId, value, channelIndex);
        }

        // Capture after-state
        juce::var afterValue;
        if (binding.isEqBand)
        {
            auto band = getBandTree();
            if (band.isValid())
                afterValue = band.getProperty (paramId);
        }
        else
        {
            afterValue = state.getParameter (paramId, channelIndex);
        }

        if (record != nullptr)
        {
            record->affectedParameters.add (binding.internalVariable);
            record->affectedGroups.push_back ({ displayId, binding.csvSection });

            auto before = std::make_unique<juce::DynamicObject>();
            before->setProperty (paramId, beforeValue);
            record->beforeState = juce::var (before.release());

            auto after = std::make_unique<juce::DynamicObject>();
            after->setProperty (paramId, afterValue);
            record->afterState = juce::var (after.release());

            juce::String desc = "Set " + binding.internalVariable;
            if (binding.channelArgName.isNotEmpty())
            {
                desc += " for " + channelArgToScopeLabel (binding.channelArgName) + " "
                       + juce::String (displayId);
                if (binding.isEqBand)
                    desc += " band " + juce::String (bandIndex + 1);
            }
            desc += " to " + value.toString();
            record->operatorDescription = desc;
        }

        auto result = std::make_unique<juce::DynamicObject>();
        result->setProperty ("variable", binding.internalVariable);
        if (binding.channelArgName.isNotEmpty())
            result->setProperty ("channel_id", displayId);
        if (binding.isEqBand)
            result->setProperty ("band", bandIndex + 1);
        // The value that LANDED, not the one that was asked for. They differ whenever
        // the state clamps or re-routes, and reporting the request hid exactly that.
        // `requested` stays alongside so a clamp is visible rather than merely implied.
        result->setProperty ("value", afterValue);
        if (afterValue != value)
            result->setProperty ("requested", value);
        result->setProperty ("before", beforeValue);
        return ToolResult::ok (juce::var (result.release()));
    }
}  // namespace Detail

namespace
{
    /** Read min/max from a tool entry's `parameters.properties.value` schema.
        Returns false if the schema doesn't declare a numeric range (e.g.
        string-typed values or enums). */
    bool extractValueRange (const juce::var& parametersSchema, double& outMin, double& outMax)
    {
        auto* paramsObj = asObject (parametersSchema);
        if (paramsObj == nullptr) return false;
        auto props = paramsObj->getProperty ("properties");
        auto* propsObj = asObject (props);
        if (propsObj == nullptr) return false;
        auto valueProp = propsObj->getProperty ("value");
        auto* valueObj = asObject (valueProp);
        if (valueObj == nullptr) return false;

        auto minVar = valueObj->getProperty ("minimum");
        auto maxVar = valueObj->getProperty ("maximum");
        if (! minVar.isDouble() && ! minVar.isInt()) return false;
        if (! maxVar.isDouble() && ! maxVar.isInt()) return false;

        outMin = static_cast<double> (minVar);
        outMax = static_cast<double> (maxVar);
        return true;
    }

}  // anonymous namespace

namespace Detail
{
    /** Nudge dispatcher: read-modify-write with clamping. */
    ToolResult dispatchGenericNudge (WFSValueTreeState& state,
                                     const NudgeBinding& binding,
                                     const juce::var& args,
                                     ChangeRecord* record)
    {
        if (! args.isObject())
            return ToolResult::error ("invalid_args", "Arguments must be a JSON object");

        auto* argsObj = asObject (args);

        // Resolve channel index (input_id is a permanent channel number —
        // the input list may have gaps after deletions)
        int channelIndex = -1;
        int displayId = 0;
        if (binding.channelArgName.isNotEmpty())
        {
            if (! argsObj->hasProperty (binding.channelArgName))
                return ToolResult::error ("invalid_args",
                                          "Missing required arg: " + binding.channelArgName);
            displayId = static_cast<int> (argsObj->getProperty (binding.channelArgName));
            channelIndex = resolveChannelSlot (state, binding.channelArgName, displayId);
            if (channelIndex < 0)
                return ToolResult::error ("invalid_args",
                                          binding.channelArgName + " not a live channel: " + juce::String (displayId));
        }

        // Resolve direction
        if (! argsObj->hasProperty ("direction"))
            return ToolResult::error ("invalid_args", "Missing required arg: direction");
        const auto direction = argsObj->getProperty ("direction").toString();
        if (direction != "inc" && direction != "dec")
            return ToolResult::error ("invalid_args", "direction must be 'inc' or 'dec'");

        // Resolve amount (default 1.0)
        const double amount = argsObj->hasProperty ("amount")
                                ? static_cast<double> (argsObj->getProperty ("amount"))
                                : 1.0;
        const double signedDelta = (direction == "dec") ? -amount : amount;

        const juce::Identifier paramId (binding.internalVariable);

        // Same refusal as the setter path: a nudge on an unresolvable parameter
        // reads void as 0, adds the delta and writes nowhere, so the reported
        // before/after are both null and the caller has no idea why.
        if (! state.canWriteParameter (paramId, channelIndex))
        {
            return ToolResult::error ("unwritable_parameter",
                                      "Parameter '" + binding.internalVariable
                                        + "' is advertised but cannot be written: the app "
                                          "has no resolvable home for it. This is an app "
                                          "defect, not a bad argument - please report it.");
        }

        // Read current — coerce to double for arithmetic.
        const auto beforeVar = state.getParameter (paramId, channelIndex);
        const double beforeValue = static_cast<double> (beforeVar);

        // Apply delta, clamp if range known.
        double newValue = beforeValue + signedDelta;
        if (binding.hasRange)
            newValue = juce::jlimit (binding.minValue, binding.maxValue, newValue);

        // Preserve integer type if the original was int (avoid 0.0 → 0 widening surprises).
        juce::var writeValue;
        if (beforeVar.isInt())
            writeValue = juce::var (juce::roundToInt (newValue));
        else
            writeValue = juce::var (newValue);

        state.setParameter (paramId, writeValue, channelIndex);
        const auto afterVar = state.getParameter (paramId, channelIndex);

        if (record != nullptr)
        {
            record->affectedParameters.add (binding.internalVariable);
            record->affectedGroups.push_back ({ displayId, binding.csvSection });

            auto before = std::make_unique<juce::DynamicObject>();
            before->setProperty (paramId, beforeVar);
            record->beforeState = juce::var (before.release());

            auto after = std::make_unique<juce::DynamicObject>();
            after->setProperty (paramId, afterVar);
            record->afterState = juce::var (after.release());

            juce::String desc = "Nudged " + binding.internalVariable;
            if (binding.channelArgName.isNotEmpty())
                desc += " for " + channelArgToScopeLabel (binding.channelArgName) + " " + juce::String (displayId);
            const juce::String signedAmount = (signedDelta >= 0 ? "+" : "") + juce::String (signedDelta, 3);
            desc += " by " + signedAmount + " (now " + afterVar.toString() + ")";
            record->operatorDescription = desc;
        }

        auto result = std::make_unique<juce::DynamicObject>();
        result->setProperty ("variable", binding.internalVariable);
        if (binding.channelArgName.isNotEmpty())
            result->setProperty ("channel_id", displayId);
        result->setProperty ("direction", direction);
        result->setProperty ("amount", amount);
        result->setProperty ("before", beforeVar);
        result->setProperty ("after", afterVar);
        return ToolResult::ok (juce::var (result.release()));
    }
}  // namespace Detail

namespace
{
    /** Build the binding metadata for one tool entry. Returns false only on
        truly-malformed entries (missing name or internal_variable). When a
        case-only mismatch with the live ValueTree is detected, the canonical
        casing from the tree is used; otherwise the raw `internal_variable`
        from the JSON is preserved so per-channel tools that haven't yet
        materialized a tree property at startup still register. Truly-bogus
        variable names will surface at write time as no-op writes, which is
        no worse than the AI sending an unknown tool name. */
    bool buildBinding (const juce::DynamicObject& toolObj,
                       const std::map<juce::String, juce::String>& knownProps,
                       ToolBinding& outBinding,
                       juce::String& outSkipReason)
    {
        outBinding.name             = toolObj.getProperty ("name").toString();
        const auto rawVariable      = toolObj.getProperty ("internal_variable").toString();
        outBinding.csvSection       = toolObj.getProperty ("csv_section").toString();
        const auto valueArgName     = toolObj.getProperty ("value_arg_name").toString();
        if (valueArgName.isNotEmpty())
            outBinding.valueArgName = valueArgName;

        if (outBinding.name.isEmpty() || rawVariable.isEmpty())
        {
            outSkipReason = "missing name or internal_variable";
            return false;
        }

        // Case-correct against the live ValueTree's known property names.
        // Defends against CSV-vs-WFSParameterIDs.h drift (e.g. CSV has
        // `StageWidth` from spreadsheet auto-capitalization but the C++
        // identifier is `stageWidth`). When there's no live-tree match,
        // fall through with the raw value — many params only get a property
        // when their section is materialized, so absence here doesn't mean
        // the variable is bogus.
        auto it = knownProps.find (rawVariable.toLowerCase());
        outBinding.internalVariable = (it != knownProps.end()) ? it->second : rawVariable;

        const auto parameters = toolObj.getProperty ("parameters");
        outBinding.channelArgName = detectChannelArg (parameters);

        if (auto* paramsObj = asObject (parameters))
        {
            const auto props = paramsObj->getProperty ("properties");
            if (auto* propsObj = asObject (props))
            {
                outBinding.isEqBand = propsObj->hasProperty ("band");

                // Look up the enum on the actual value-arg (which may have
                // been renamed to "name"/"mode"/"shape"/"protocol").
                const auto valueProp = propsObj->getProperty (outBinding.valueArgName);
                if (auto* valuePropObj = asObject (valueProp))
                {
                    const auto enumVar = valuePropObj->getProperty ("enum");
                    if (enumVar.isArray())
                        for (const auto& e : *enumVar.getArray())
                            outBinding.enumValues.add (e.toString());

                    // Pick up the JSON-Schema type so dispatchGenericSet can
                    // write int vs double correctly. Without this, the AI
                    // sending an int for a float param stores int-typed in
                    // the float slot (the same class of corruption that the
                    // string-coercion fix already handles).
                    const auto typeStr = valuePropObj->getProperty ("type").toString();
                    if (typeStr == "integer")
                    {
                        outBinding.isIntegerType = true;
                        outBinding.isNumericType = true;
                    }
                    else if (typeStr == "number")
                    {
                        outBinding.isNumericType = true;
                    }
                }
            }
        }

        return true;
    }
} // anonymous namespace

LoadStats loadGeneratedTools (MCPToolRegistry& registry,
                              WFSValueTreeState& state,
                              const juce::File& jsonPath,
                              MCPLogger& mcpLogger,
                              const std::function<void()>* onTopologyChanged)
{
    LoadStats stats;

    if (! jsonPath.existsAsFile())
    {
        stats.errorMessage = "generated_tools.json not found at " + jsonPath.getFullPathName();
        mcpLogger.logError (stats.errorMessage);
        return stats;
    }

    const juce::var rootVar = juce::JSON::parse (jsonPath);
    auto* rootObj = asObject (rootVar);
    if (rootObj == nullptr)
    {
        stats.errorMessage = "generated_tools.json: failed to parse as JSON object";
        mcpLogger.logError (stats.errorMessage);
        return stats;
    }

    const auto toolsArr = rootObj->getProperty ("tools");
    if (! toolsArr.isArray())
    {
        stats.errorMessage = "generated_tools.json: 'tools' is not an array";
        mcpLogger.logError (stats.errorMessage);
        return stats;
    }

    const auto knownProps = buildKnownPropertyMap (state);

    // Side map populated during the tools[] pass and consumed by the
    // nudge_tools[] pass for clamp-range lookup. Keyed by canonical
    // internal_variable so case-corrected nudges still find their range.
    std::map<juce::String, std::pair<double, double>> rangeByVariable;

    for (const auto& toolVar : *toolsArr.getArray())
    {
        auto* toolObj = asObject (toolVar);
        if (toolObj == nullptr) { stats.skipped++; continue; }

        ToolBinding binding;
        juce::String skipReason;
        if (! buildBinding (*toolObj, knownProps, binding, skipReason))
        {
            stats.skipped++;
            DBG ("MCPGeneratedToolLoader: skipping tool '"
                 << toolObj->getProperty ("name").toString() << "': " << skipReason);
            continue;
        }

        // Stash the value range (if any) so the matching nudge variant can
        // clamp during read-modify-write, and also bind it to this set
        // tool so dispatchGenericSet can fail-closed on out-of-range.
        const auto parameters = toolObj->getProperty ("parameters");
        double rmin = 0.0, rmax = 0.0;
        if (extractValueRange (parameters, rmin, rmax))
        {
            rangeByVariable[binding.internalVariable] = { rmin, rmax };
            binding.hasRange = true;
            binding.minValue = rmin;
            binding.maxValue = rmax;
        }

        ToolDescriptor d;
        d.name           = binding.name;
        d.description    = toolObj->getProperty ("description").toString();
        d.inputSchema    = parameters;
        d.modifiesState  = true;
        // Phase 6: read tier from generated_tools.json (1 default).
        if (toolObj->hasProperty ("tier"))
            d.tier = juce::jlimit (1, 3, static_cast<int> (toolObj->getProperty ("tier")));
        d.listable = shouldListGeneratedTool (d.tier);
        // A channel-count write restructures the session; everything else is a
        // value change the engine already follows. Deciding here, from the bound
        // variable, keeps the knowledge in one place rather than in a name test
        // inside the dispatcher.
        const bool isChannelCount =
               binding.internalVariable == WFSParameterIDs::inputChannels.toString()
            || binding.internalVariable == WFSParameterIDs::outputChannels.toString()
            || binding.internalVariable == WFSParameterIDs::reverbChannels.toString();

        d.handler = [&state, binding, isChannelCount, onTopologyChanged]
                    (const juce::var& args, ChangeRecord* record) -> ToolResult
        {
            // Channel structure is stopped-only. The GUI enforces that by greying
            // its I/O controls; nothing enforced it here, so a remote caller could
            // resize the routing matrices under a live audio callback.
            if (isChannelCount && state.isProcessingEnabled())
                return ToolResult::error ("engine_running",
                                          "Cannot change the channel count while the audio "
                                          "engine is running. Channel structure is "
                                          "stopped-only: stop processing first (System "
                                          "Config > Run DSP), then retry.");

            auto result = dispatchGenericSet (state, binding, args, record);
            if (result.success && isChannelCount
                && onTopologyChanged != nullptr && *onTopologyChanged)
                (*onTopologyChanged)();
            return result;
        };

        registry.registerTool (std::move (d));
        stats.toolsLoaded++;
    }

    // Phase 2 Block 2 — nudge variants. Same binding builder, plus the
    // {min, max} side-map populated above for clamping the read-modify-write.
    const auto nudgeArr = rootObj->getProperty ("nudge_tools");
    if (nudgeArr.isArray())
    {
        for (const auto& toolVar : *nudgeArr.getArray())
        {
            auto* toolObj = asObject (toolVar);
            if (toolObj == nullptr) { stats.skipped++; continue; }

            ToolBinding base;
            juce::String skipReason;
            if (! buildBinding (*toolObj, knownProps, base, skipReason))
            {
                stats.skipped++;
                DBG ("MCPGeneratedToolLoader: skipping nudge '"
                     << toolObj->getProperty ("name").toString() << "': " << skipReason);
                continue;
            }

            NudgeBinding binding;
            static_cast<ToolBinding&> (binding) = base;

            auto rangeIt = rangeByVariable.find (binding.internalVariable);
            if (rangeIt != rangeByVariable.end())
            {
                binding.hasRange = true;
                binding.minValue = rangeIt->second.first;
                binding.maxValue = rangeIt->second.second;
            }
            // No-range nudges still register; clamp is just skipped. Most
            // bool/enum nudges fall in this bucket and rarely make sense
            // anyway, but the AI's call should fail closed at write time
            // rather than at registration.

            ToolDescriptor d;
            d.name           = binding.name;
            d.description    = toolObj->getProperty ("description").toString();
            d.inputSchema    = toolObj->getProperty ("parameters");
            d.modifiesState  = true;
            if (toolObj->hasProperty ("tier"))
                d.tier = juce::jlimit (1, 3, static_cast<int> (toolObj->getProperty ("tier")));
            d.listable = shouldListGeneratedTool (d.tier);
            d.handler = [&state, binding] (const juce::var& args, ChangeRecord* record) -> ToolResult
            {
                return dispatchGenericNudge (state, binding, args, record);
            };

            registry.registerTool (std::move (d));
            stats.nudgeToolsLoaded++;
        }
    }

    mcpLogger.logInfo ("Loaded " + juce::String (stats.toolsLoaded)
                       + " generated tools + " + juce::String (stats.nudgeToolsLoaded)
                       + " nudge variants (" + juce::String (stats.skipped)
                       + " skipped - see DBG output for variable-name mismatches)");

    return stats;
}

} // namespace WFSNetwork::Tools::Generated
