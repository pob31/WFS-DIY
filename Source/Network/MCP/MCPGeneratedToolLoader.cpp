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
        bool isEqBand = false;
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

        // Resolve and coerce value
        if (! argsObj->hasProperty ("value"))
            return ToolResult::error ("invalid_args", "Missing required arg: value");
        const juce::var value = coerceValue (argsObj->getProperty ("value"), binding);

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

                const auto valueProp = propsObj->getProperty ("value");
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

    for (const auto& toolVar : *toolsArr.getArray())
    {
        auto* toolObj = asObject (toolVar);
        if (toolObj == nullptr) { stats.skipped++; continue; }

        ToolBinding binding;
        juce::String skipReason;
        if (! buildBinding (*toolObj, knownProps, binding, skipReason))
        {
            stats.skipped++;
            // Log only at DBG level to avoid flooding the Network Log on every startup
            // — these are CSV-vs-WFSParameterIDs drift cases that need a generator-side fix.
            DBG ("MCPGeneratedToolLoader: skipping tool '"
                 << toolObj->getProperty ("name").toString() << "': " << skipReason);
            continue;
        }

        ToolDescriptor d;
        d.name           = binding.name;
        d.description    = toolObj->getProperty ("description").toString();
        d.inputSchema    = toolObj->getProperty ("parameters");
        d.modifiesState  = true;
        d.handler = [&state, binding] (const juce::var& args, ChangeRecord* record) -> ToolResult
        {
            return dispatchGenericSet (state, binding, args, record);
        };

        registry.registerTool (std::move (d));
        stats.toolsLoaded++;
    }

    mcpLogger.logInfo ("Loaded " + juce::String (stats.toolsLoaded)
                       + " generated MCP tools (" + juce::String (stats.skipped)
                       + " skipped — see DBG output for variable-name mismatches)");

    return stats;
}

} // namespace WFSNetwork::Tools::Generated
