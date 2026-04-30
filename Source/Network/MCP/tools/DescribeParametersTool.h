#pragma once

#include <JuceHeader.h>
#include "../MCPToolRegistry.h"
#include "../MCPParameterRegistry.h"

namespace WFSNetwork::Tools::DescribeParameters
{

inline juce::var buildSchema()
{
    auto prefix = std::make_unique<juce::DynamicObject>();
    prefix->setProperty ("type", "string");
    prefix->setProperty ("description",
        "Optional case-sensitive prefix on the canonical parameter name "
        "(e.g. \"origin\" returns originWidth/originDepth/originHeight). "
        "Omit for the full registry dump.");

    auto scope = std::make_unique<juce::DynamicObject>();
    scope->setProperty ("type", "string");
    juce::Array<juce::var> scopeEnum;
    scopeEnum.add ("global");
    scopeEnum.add ("input");
    scopeEnum.add ("output");
    scopeEnum.add ("reverb");
    scopeEnum.add ("cluster");
    scopeEnum.add ("eq_band");
    scope->setProperty ("enum", scopeEnum);
    scope->setProperty ("description",
        "Optional scope filter. \"global\" = stage / master / network / "
        "binaural / config; the rest are per-channel.");

    auto groupKey = std::make_unique<juce::DynamicObject>();
    groupKey->setProperty ("type", "string");
    groupKey->setProperty ("description",
        "Optional group_key filter (e.g. \"input_position\"). Group keys "
        "match the auto-generated tool family that writes the parameter.");

    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("prefix",    juce::var (prefix.release()));
    props->setProperty ("scope",     juce::var (scope.release()));
    props->setProperty ("group_key", juce::var (groupKey.release()));

    auto schema = std::make_unique<juce::DynamicObject>();
    schema->setProperty ("type", "object");
    schema->setProperty ("properties", juce::var (props.release()));
    schema->setProperty ("additionalProperties", false);
    return juce::var (schema.release());
}

inline juce::var recordToVar (const ParameterRegistryRecord& r)
{
    auto obj = std::make_unique<juce::DynamicObject>();
    obj->setProperty ("variable",    r.variable);
    obj->setProperty ("tool_name",   r.toolName);
    obj->setProperty ("scope",       r.scope);
    obj->setProperty ("type",        r.type);
    if (r.minValue.has_value())
        obj->setProperty ("min", *r.minValue);
    if (r.maxValue.has_value())
        obj->setProperty ("max", *r.maxValue);
    if (r.enumValues.size() > 0)
    {
        juce::Array<juce::var> enumArr;
        for (const auto& e : r.enumValues)
            enumArr.add (e);
        obj->setProperty ("enum", enumArr);
    }
    if (! r.defaultValue.isVoid())
        obj->setProperty ("default", r.defaultValue);
    if (r.synonyms.size() > 0)
    {
        juce::Array<juce::var> syn;
        for (const auto& s : r.synonyms)
            syn.add (s);
        obj->setProperty ("synonyms", juce::var (syn));
    }
    obj->setProperty ("description", r.description);
    if (r.unit.isNotEmpty())
        obj->setProperty ("unit", r.unit);
    if (r.oscPath.isNotEmpty())
        obj->setProperty ("osc_path", r.oscPath);
    obj->setProperty ("tier",        r.tier);
    obj->setProperty ("group_key",   r.groupKey);
    obj->setProperty ("csv_section", r.csvSection);
    return juce::var (obj.release());
}

inline ToolResult describe (const juce::var& args)
{
    juce::String prefix, scope, groupKey;
    if (auto* obj = args.getDynamicObject())
    {
        prefix   = obj->getProperty ("prefix").toString().trim();
        scope    = obj->getProperty ("scope").toString().trim();
        groupKey = obj->getProperty ("group_key").toString().trim();
    }

    const auto& reg = MCPParameterRegistry::getInstance();
    const auto matches = reg.filter (prefix, scope, groupKey);

    juce::Array<juce::var> arr;
    for (const auto& r : matches)
        arr.add (recordToVar (r));

    auto root = std::make_unique<juce::DynamicObject>();
    root->setProperty ("count", static_cast<int> (arr.size()));
    root->setProperty ("total", reg.size());
    root->setProperty ("parameters", juce::var (arr));
    return ToolResult::ok (juce::var (root.release()));
}

inline ToolDescriptor describeTool()
{
    ToolDescriptor d;
    d.name        = "mcp_describe_parameters";
    d.description = "Read-only registry of every parameter the MCP surface "
                    "knows about. Each record carries the canonical "
                    "case-sensitive `variable` name (the one wfs_set_parameter "
                    "expects), the auto-generated `tool_name` that writes it, "
                    "scope (global/input/output/reverb/cluster/eq_band), JSON "
                    "type, min/max, enum values when applicable, default "
                    "value, OSC path, and tier. Filter with `prefix` (matches "
                    "variable start), `scope`, or `group_key`. Use this BEFORE "
                    "guessing parameter names — it's the source of truth for "
                    "the wfs_set_parameter escape hatch.";
    d.inputSchema   = buildSchema();
    d.modifiesState = false;
    d.tier        = 1;
    d.handler = [] (const juce::var& args, ChangeRecord*) -> ToolResult
    {
        return describe (args);
    };
    return d;
}

} // namespace WFSNetwork::Tools::DescribeParameters
