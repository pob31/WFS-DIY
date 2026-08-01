#pragma once

#include <JuceHeader.h>
#include <map>
#include "../MCPCompat.h"
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

    auto domain = std::make_unique<juce::DynamicObject>();
    domain->setProperty ("type", "string");
    juce::Array<juce::var> domainEnum;
    domainEnum.add ("wfs_synthesis");
    domainEnum.add ("reverb");
    domainEnum.add ("binaural");
    domainEnum.add ("adm_osc");
    domainEnum.add ("floor_reflections");
    domainEnum.add ("live_source");
    domainEnum.add ("tracking");
    domainEnum.add ("routing");
    domainEnum.add ("network");
    domainEnum.add ("visualisation_only");
    domainEnum.add ("metadata");
    domain->setProperty ("enum", domainEnum);
    domain->setProperty ("description",
        "Optional domain tag filter. Use this to narrow the registry to "
        "params that are meaningful for a specific intent - e.g. domain="
        "\"wfs_synthesis\" returns only params that change what the WFS "
        "speakers emit; domain=\"visualisation_only\" returns map / lock "
        "toggles that don't affect audio.");

    auto mode = std::make_unique<juce::DynamicObject>();
    mode->setProperty ("type", "string");
    juce::Array<juce::var> modeEnum;
    modeEnum.add ("summary");
    modeEnum.add ("full");
    mode->setProperty ("enum", modeEnum);
    mode->setProperty ("default", "summary");
    mode->setProperty ("description",
        "\"summary\" (default) returns the fields needed to plan a write: "
        "variable, type, scope, tier, group_key, unit, min/max and enum. "
        "\"full\" adds description, osc_path, synonyms, domains, default and "
        "csv_section - several times larger, so ask for it only when you "
        "actually need the prose.");

    auto limit = std::make_unique<juce::DynamicObject>();
    limit->setProperty ("type", "integer");
    limit->setProperty ("minimum", 1);
    limit->setProperty ("maximum", 200);
    limit->setProperty ("default", 50);
    limit->setProperty ("description",
        "Maximum records to return (default 50). The response sets "
        "truncated=true when matches were dropped - narrow the filter "
        "rather than raising this.");

    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("prefix",    juce::var (prefix.release()));
    props->setProperty ("scope",     juce::var (scope.release()));
    props->setProperty ("group_key", juce::var (groupKey.release()));
    props->setProperty ("domain",    juce::var (domain.release()));
    props->setProperty ("mode",      juce::var (mode.release()));
    props->setProperty ("limit",     juce::var (limit.release()));

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
    if (r.enumIntValues.size() > 0)
    {
        // Parallel to `enum` — enum_int_values[i] is the integer the writer
        // stores for enum[i]. Both the auto-gen tool and wfs_set_parameter
        // accept either the label string OR the integer; this field tells
        // the AI exactly which integer each label maps to.
        juce::Array<juce::var> intArr;
        for (int i = 0; i < r.enumIntValues.size(); ++i)
            intArr.add (r.enumIntValues[i]);
        obj->setProperty ("enum_int_values", intArr);
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
    if (r.domains.size() > 0)
    {
        juce::Array<juce::var> dom;
        for (const auto& d : r.domains)
            dom.add (d);
        obj->setProperty ("domains", juce::var (dom));
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

/** Trimmed record: everything needed to decide on and validate a write,
    without the prose. This is where most of the token saving comes from —
    `description` alone averages ~150 characters per record. */
inline juce::var recordToSummaryVar (const ParameterRegistryRecord& r)
{
    auto obj = std::make_unique<juce::DynamicObject>();
    obj->setProperty ("variable",  r.variable);
    obj->setProperty ("type",      r.type);
    obj->setProperty ("scope",     r.scope);
    obj->setProperty ("tier",      r.tier);
    obj->setProperty ("group_key", r.groupKey);
    if (r.unit.isNotEmpty())
        obj->setProperty ("unit", r.unit);
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
    return juce::var (obj.release());
}

/** Overview returned when the caller supplied no filters at all.

    An unfiltered dump of every record ran to roughly 150KB, which is a
    poor answer to "what can I change?". Returning the group catalog
    instead gives the model a map it can navigate in one more call. */
inline ToolResult describeGroups (const MCPParameterRegistry& reg)
{
    struct GroupSummary
    {
        juce::String scope;
        int count = 0;
        juce::StringArray examples;  // first few variable names
    };

    std::map<juce::String, GroupSummary> groups;
    for (const auto& r : reg.filter ({}, {}, {}, {}))
    {
        const auto key = r.groupKey.isNotEmpty() ? r.groupKey : juce::String ("ungrouped");
        auto& g = groups[key];
        if (g.count == 0)
            g.scope = r.scope;
        ++g.count;
        if (g.examples.size() < 3)
            g.examples.add (r.variable);
    }

    juce::Array<juce::var> arr;
    for (const auto& [key, g] : groups)
    {
        auto obj = std::make_unique<juce::DynamicObject>();
        obj->setProperty ("group_key", key);
        obj->setProperty ("scope", g.scope);
        obj->setProperty ("param_count", g.count);

        juce::Array<juce::var> examples;
        for (const auto& e : g.examples)
            examples.add (e);
        obj->setProperty ("example_variables", juce::var (examples));
        arr.add (juce::var (obj.release()));
    }

    auto root = std::make_unique<juce::DynamicObject>();
    root->setProperty ("view", "groups");
    root->setProperty ("group_count", static_cast<int> (arr.size()));
    root->setProperty ("total_parameters", reg.size());
    root->setProperty ("groups", juce::var (arr));
    root->setProperty ("hint",
        "Re-call with group_key, scope, prefix or domain to list parameters. "
        "Add mode=\"full\" for descriptions and OSC paths.");
    return ToolResult::ok (juce::var (root.release()));
}

inline ToolResult describe (const juce::var& args)
{
    juce::String prefix, scope, groupKey, domain, mode;
    int limit = 50;
    if (auto* obj = args.getDynamicObject())
    {
        prefix   = obj->getProperty ("prefix").toString().trim();
        scope    = obj->getProperty ("scope").toString().trim();
        groupKey = obj->getProperty ("group_key").toString().trim();
        domain   = obj->getProperty ("domain").toString().trim();
        mode     = obj->getProperty ("mode").toString().trim();
        if (obj->hasProperty ("limit"))
            limit = juce::jlimit (1, 200, static_cast<int> (obj->getProperty ("limit")));
    }

    const auto& reg = MCPParameterRegistry::getInstance();

    // No filters at all → hand back the group map rather than everything.
    if (prefix.isEmpty() && scope.isEmpty() && groupKey.isEmpty() && domain.isEmpty())
        return describeGroups (reg);

    const auto matches = reg.filter (prefix, scope, groupKey, domain);
    const bool fullMode = (mode == "full");

    juce::Array<juce::var> arr;
    for (const auto& r : matches)
    {
        if (arr.size() >= limit)
            break;
        arr.add (fullMode ? recordToVar (r) : recordToSummaryVar (r));
    }

    auto root = std::make_unique<juce::DynamicObject>();
    root->setProperty ("count", static_cast<int> (arr.size()));
    root->setProperty ("matched", static_cast<int> (matches.size()));
    root->setProperty ("total", reg.size());
    root->setProperty ("mode", fullMode ? "full" : "summary");
    if (static_cast<int> (matches.size()) > arr.size())
    {
        root->setProperty ("truncated", true);
        root->setProperty ("hint", "Narrow the filter to see the rest, or raise limit (max 200).");
    }
    root->setProperty ("parameters", juce::var (arr));
    return ToolResult::ok (juce::var (root.release()));
}

inline ToolDescriptor describeTool()
{
    ToolDescriptor d;
    d.name        = "mcp_describe_parameters";
    d.description = "Read-only registry of every writable parameter - the "
                    "source of truth for parameter names. Call it with NO "
                    "arguments first: that returns a compact map of parameter "
                    "groups. Then re-call with `prefix`, `scope`, `group_key` "
                    "or `domain` to list the parameters in one area. Records "
                    "carry the canonical case-sensitive `variable` name (what "
                    "wfs_set_parameter expects), type, scope, tier, unit and "
                    "min/max or enum values; mode=\"full\" adds description, "
                    "OSC path, synonyms, domain tags and the `tool_name` of "
                    "the dedicated (unlisted) tool that writes it. Results are "
                    "capped by `limit` (default 50). For current values use "
                    "wfs_get_parameter / wfs_get_parameters; for one channel's "
                    "entire state call session_get_channel_full.";
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
