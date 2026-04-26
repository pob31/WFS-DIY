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
        Captured by value into the handler lambda for each registered tool. */
    struct ToolBinding
    {
        juce::String name;              // "input.position.set_x"
        juce::String internalVariable;  // canonical ValueTree property name (case-corrected)
        juce::String csvSection;        // "Position" — used in affected_groups + operatorDescription
        juce::StringArray enumValues;   // empty if not an enum tool
        juce::String channelArgName;    // input_id / output_id / reverb_id / cluster_id, or empty for global
        juce::String valueArgName { "value" };  // self-documenting renames: "name" / "mode" / "shape" / "protocol"
        bool isEqBand = false;
    };

    /** Extension of ToolBinding for nudge variants. Carries the min/max
        from the corresponding non-nudge tool so we can clamp the
        read-modify-write step at the server boundary. */
    struct NudgeBinding : ToolBinding
    {
        bool hasRange = false;
        double minValue = 0.0;
        double maxValue = 0.0;
    };

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
        Otherwise (or on unknown enum values) pass through as-is so that
        downstream JUCE coercion still has a chance. */
    juce::var coerceValue (const juce::var& incoming, const ToolBinding& binding)
    {
        if (binding.enumValues.size() > 0 && incoming.isString())
        {
            const auto s = incoming.toString();
            for (int i = 0; i < binding.enumValues.size(); ++i)
                if (binding.enumValues[i] == s)
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
        const juce::var value = coerceValue (argsObj->getProperty (binding.valueArgName), binding);

        const juce::Identifier paramId (binding.internalVariable);

        // Capture before-state
        juce::var beforeValue;
        if (binding.isEqBand)
        {
            auto band = state.getOutputEQBand (channelIndex, bandIndex);
            if (band.isValid())
                beforeValue = band.getProperty (paramId);
        }
        else
        {
            beforeValue = state.getParameter (paramId, channelIndex);
        }

        // Write
        if (binding.isEqBand)
            state.setOutputEQBandParameterWithArrayPropagation (channelIndex, bandIndex, paramId, value);
        else
            state.setParameter (paramId, value, channelIndex);

        // Capture after-state
        juce::var afterValue;
        if (binding.isEqBand)
        {
            auto band = state.getOutputEQBand (channelIndex, bandIndex);
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
        // clamp during read-modify-write.
        const auto parameters = toolObj->getProperty ("parameters");
        double rmin = 0.0, rmax = 0.0;
        if (extractValueRange (parameters, rmin, rmax))
            rangeByVariable[binding.internalVariable] = { rmin, rmax };

        ToolDescriptor d;
        d.name           = binding.name;
        d.description    = toolObj->getProperty ("description").toString();
        d.inputSchema    = parameters;
        d.modifiesState  = true;
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
                       + " skipped — see DBG output for variable-name mismatches)");

    return stats;
}

} // namespace WFSNetwork::Tools::Generated
