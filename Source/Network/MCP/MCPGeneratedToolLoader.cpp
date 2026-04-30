#include "MCPGeneratedToolLoader.h"
#include "MCPToolRegistry.h"
#include "MCPChangeRecords.h"
#include "MCPLogger.h"
#include "../../Parameters/WFSValueTreeState.h"

namespace WFSNetwork::Tools::Generated
{

namespace
{
    /** Per-tool data the generic dispatch handler needs to do its job.
        Captured by value into the handler lambda for each registered tool.
        Range fields mirror the schema's `minimum`/`maximum`; populated for
        numeric-typed value args, used to fail-closed on out-of-range writes
        (matches OSC ingress behaviour in OSCParameterBounds). */
    struct ToolBinding
    {
        juce::String name;              // "input.position.set_x"
        juce::String internalVariable;  // canonical ValueTree property name (case-corrected)
        juce::String csvSection;        // "Position" — used in affected_groups + operatorDescription
        juce::StringArray enumValues;   // empty if not an enum tool
        juce::String channelArgName;    // input_id / output_id / reverb_id / cluster_id, or empty for global
        juce::String valueArgName { "value" };  // self-documenting renames: "name" / "mode" / "shape" / "protocol"
        bool isEqBand = false;
        bool hasRange = false;
        double minValue = 0.0;
        double maxValue = 0.0;
        bool isIntegerType = false;     // schema declared type: "integer" → write int, not double
        bool isNumericType = false;     // schema declared type: "number" / "integer"
    };

    /** Extension of ToolBinding for nudge variants. The range fields on
        ToolBinding feed the clamp; this subtype exists for naming clarity
        and future divergence (e.g. nudges may grow per-press defaults). */
    struct NudgeBinding : ToolBinding {};

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

        // Resolve channel index (1-based MCP arg → 0-based ValueTree index)
        int channelIndex = -1;
        int displayId = 0;  // 1-based for descriptions and affected_groups
        if (binding.channelArgName.isNotEmpty())
        {
            if (! argsObj->hasProperty (binding.channelArgName))
                return ToolResult::error ("invalid_args",
                                          "Missing required arg: " + binding.channelArgName);
            displayId = static_cast<int> (argsObj->getProperty (binding.channelArgName));
            channelIndex = displayId - 1;
            if (channelIndex < 0)
                return ToolResult::error ("invalid_args",
                                          binding.channelArgName + " out of range: " + juce::String (displayId));
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
        result->setProperty ("value", value);
        return ToolResult::ok (juce::var (result.release()));
    }

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

    /** Nudge dispatcher: read-modify-write with clamping. */
    ToolResult dispatchGenericNudge (WFSValueTreeState& state,
                                     const NudgeBinding& binding,
                                     const juce::var& args,
                                     ChangeRecord* record)
    {
        if (! args.isObject())
            return ToolResult::error ("invalid_args", "Arguments must be a JSON object");

        auto* argsObj = asObject (args);

        // Resolve channel index
        int channelIndex = -1;
        int displayId = 0;
        if (binding.channelArgName.isNotEmpty())
        {
            if (! argsObj->hasProperty (binding.channelArgName))
                return ToolResult::error ("invalid_args",
                                          "Missing required arg: " + binding.channelArgName);
            displayId = static_cast<int> (argsObj->getProperty (binding.channelArgName));
            channelIndex = displayId - 1;
            if (channelIndex < 0)
                return ToolResult::error ("invalid_args",
                                          binding.channelArgName + " out of range: " + juce::String (displayId));
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
                              MCPLogger& mcpLogger)
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
        d.handler = [&state, binding] (const juce::var& args, ChangeRecord* record) -> ToolResult
        {
            return dispatchGenericSet (state, binding, args, record);
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
