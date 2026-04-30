#include "MCPParameterRegistry.h"
#include "MCPLogger.h"

#include <algorithm>
#include <vector>

namespace WFSNetwork
{

namespace
{
    juce::DynamicObject* asObject (const juce::var& v)
    {
        return v.isObject() ? v.getDynamicObject() : nullptr;
    }

    /** Pull `properties` map off a `parameters` JSON-Schema object. */
    juce::DynamicObject* getProperties (const juce::var& parameters)
    {
        auto* obj = asObject (parameters);
        if (obj == nullptr)
            return nullptr;
        return asObject (obj->getProperty ("properties"));
    }

    /** Detect which channel-id arg this tool expects (mirrors the loader's
        helper but kept private here to avoid coupling). */
    juce::String detectChannelArg (juce::DynamicObject* propsObj)
    {
        if (propsObj == nullptr) return {};
        static const juce::StringArray candidates {
            "input_id", "output_id", "reverb_id", "cluster_id"
        };
        for (const auto& c : candidates)
            if (propsObj->hasProperty (c))
                return c;
        return {};
    }

    /** Map channel-id arg name + EQ-band presence to the registry's `scope`
        string. Globals → "global". */
    juce::String deriveScope (juce::DynamicObject* propsObj)
    {
        if (propsObj == nullptr) return "global";
        const bool hasBand = propsObj->hasProperty ("band");
        const auto chanArg = detectChannelArg (propsObj);
        if (hasBand) return "eq_band";
        if (chanArg == "input_id")   return "input";
        if (chanArg == "output_id")  return "output";
        if (chanArg == "reverb_id")  return "reverb";
        if (chanArg == "cluster_id") return "cluster";
        return "global";
    }

    /** Pull the value-arg sub-schema. Generated tools rename the value arg
        to "name"/"mode"/"shape"/"protocol" via `value_arg_name`; default
        is "value". */
    juce::DynamicObject* getValueArg (juce::DynamicObject* propsObj,
                                       const juce::String& valueArgName)
    {
        if (propsObj == nullptr) return nullptr;
        return asObject (propsObj->getProperty (valueArgName));
    }

    std::optional<double> readNumeric (const juce::var& v)
    {
        if (v.isDouble() || v.isInt() || v.isInt64())
            return static_cast<double> (v);
        return std::nullopt;
    }

    /** Best-effort unit extraction from the description's tail. Generator
        emits "Label unit." as the description tail (e.g. "LFO Period s.").
        Recovering the unit cleanly would require a separate manifest field;
        Phase C (CSV column enrichment) will add one. For now we leave the
        unit empty so we never lie about it. */
    juce::String extractUnit (const juce::String& /*description*/)
    {
        return {};
    }

    /** Build a registry record from one tool entry. Returns false if the
        entry is malformed (missing internal_variable / name). */
    bool buildRecord (juce::DynamicObject& toolObj,
                       ParameterRegistryRecord& out)
    {
        out.toolName = toolObj.getProperty ("name").toString();
        out.variable = toolObj.getProperty ("internal_variable").toString();

        // Tools that operate on numeric-suffix families carry
        // `internal_variable_template` instead of `internal_variable`.
        // Treat the template literally as the registry name — the AI can
        // discover the family pattern via `internal_osc_path_template`
        // visible in the description / osc_path field.
        if (out.variable.isEmpty())
            out.variable = toolObj.getProperty ("internal_variable_template").toString();

        if (out.variable.isEmpty() || out.toolName.isEmpty())
            return false;

        out.description = toolObj.getProperty ("description").toString();
        out.tier        = juce::jlimit (1, 3, static_cast<int> (toolObj.getProperty ("tier")));
        out.groupKey    = toolObj.getProperty ("group_key").toString();
        out.csvSection  = toolObj.getProperty ("csv_section").toString();

        const auto explicitPath = toolObj.getProperty ("internal_osc_path").toString();
        out.oscPath = explicitPath.isNotEmpty()
                        ? explicitPath
                        : toolObj.getProperty ("internal_osc_path_template").toString();

        const auto parameters = toolObj.getProperty ("parameters");
        auto* propsObj = getProperties (parameters);
        out.scope = deriveScope (propsObj);

        const auto valueArgName = toolObj.hasProperty ("value_arg_name")
                                    ? toolObj.getProperty ("value_arg_name").toString()
                                    : juce::String ("value");
        if (auto* valArg = getValueArg (propsObj, valueArgName))
        {
            out.type = valArg->getProperty ("type").toString();
            out.minValue = readNumeric (valArg->getProperty ("minimum"));
            out.maxValue = readNumeric (valArg->getProperty ("maximum"));

            const auto enumVar = valArg->getProperty ("enum");
            if (enumVar.isArray())
                for (const auto& e : *enumVar.getArray())
                    out.enumValues.add (e.toString());

            if (valArg->hasProperty ("default"))
                out.defaultValue = valArg->getProperty ("default");
        }

        out.unit = extractUnit (out.description);
        return true;
    }

    /** Iterative Levenshtein with a single rolling row. O(|a| · |b|) time,
        O(min(|a|,|b|)) space. Used only on demand for did-you-mean — the
        registry has ~330 entries, so a full scan is cheap. */
    int levenshtein (const juce::String& a, const juce::String& b)
    {
        const auto au = a.toStdString();
        const auto bu = b.toStdString();
        const int n = static_cast<int> (au.size());
        const int m = static_cast<int> (bu.size());
        if (n == 0) return m;
        if (m == 0) return n;

        std::vector<int> prev (m + 1), curr (m + 1);
        for (int j = 0; j <= m; ++j) prev[j] = j;

        for (int i = 1; i <= n; ++i)
        {
            curr[0] = i;
            for (int j = 1; j <= m; ++j)
            {
                const int cost = (au[(size_t) (i - 1)] == bu[(size_t) (j - 1)]) ? 0 : 1;
                curr[j] = std::min ({ prev[j] + 1,
                                       curr[j - 1] + 1,
                                       prev[j - 1] + cost });
            }
            std::swap (prev, curr);
        }
        return prev[m];
    }
} // namespace

MCPParameterRegistry& MCPParameterRegistry::getInstance()
{
    static MCPParameterRegistry instance;
    return instance;
}

void MCPParameterRegistry::loadFromManifest (const juce::File& jsonPath,
                                              MCPLogger& mcpLogger)
{
    clear();

    if (! jsonPath.existsAsFile())
    {
        mcpLogger.logError ("MCPParameterRegistry: generated_tools.json not found at "
                            + jsonPath.getFullPathName());
        return;
    }

    const juce::var rootVar = juce::JSON::parse (jsonPath);
    auto* rootObj = asObject (rootVar);
    if (rootObj == nullptr)
    {
        mcpLogger.logError ("MCPParameterRegistry: failed to parse "
                            + jsonPath.getFullPathName());
        return;
    }

    const auto walkArray = [this, &mcpLogger] (const juce::var& arr,
                                                const juce::String& kind)
    {
        if (! arr.isArray())
            return;
        for (const auto& toolVar : *arr.getArray())
        {
            auto* toolObj = asObject (toolVar);
            if (toolObj == nullptr) continue;

            ParameterRegistryRecord rec;
            if (! buildRecord (*toolObj, rec))
                continue;

            // Dedup by variable: each canonical param shows up once even
            // when both a setter and a nudge-variant tool reference it.
            // The setter wins (loader visits `tools` first), so subsequent
            // nudge entries are ignored here.
            if (knownVariables.insert (rec.variable).second)
                records.push_back (std::move (rec));

            (void) kind; // reserved for future per-array logging
        }
    };

    walkArray (rootObj->getProperty ("tools"), "tools");
    walkArray (rootObj->getProperty ("nudge_tools"), "nudge_tools");

    installSynonyms();

    mcpLogger.logInfo ("MCPParameterRegistry: loaded " + juce::String (records.size())
                       + " unique parameters from "
                       + jsonPath.getFileName()
                       + " (" + juce::String ((int) synonymToCanonical.size())
                       + " synonyms)");
}

void MCPParameterRegistry::clear()
{
    records.clear();
    knownVariables.clear();
    synonymToCanonical.clear();
}

void MCPParameterRegistry::installSynonyms()
{
    // Stage origin aliases. The user-feedback session burned multiple
    // round-trips guessing `stageOriginX/Y/Z`; the underlying writable
    // params are originWidth/Depth/Height (legacy names from the time
    // the stage-shape model only had Box). Surface both.
    struct Alias { const char* synonym; const char* canonical; };
    static const Alias aliases[] = {
        { "stageOriginX", "originWidth"  },
        { "stageOriginY", "originDepth"  },
        { "stageOriginZ", "originHeight" },
    };

    for (const auto& a : aliases)
    {
        const juce::String synonym  = a.synonym;
        const juce::String canonical = a.canonical;

        // Only register the alias if the canonical exists in the manifest;
        // otherwise the synonym would become a dangling whitelist entry.
        if (knownVariables.find (canonical) == knownVariables.end())
            continue;

        synonymToCanonical[synonym] = canonical;
        knownVariables.insert (synonym);

        for (auto& r : records)
        {
            if (r.variable == canonical)
            {
                r.synonyms.add (synonym);
                break;
            }
        }
    }
}

bool MCPParameterRegistry::isKnown (const juce::String& variable) const noexcept
{
    return knownVariables.find (variable) != knownVariables.end();
}

juce::String MCPParameterRegistry::canonicalize (const juce::String& variable) const
{
    auto it = synonymToCanonical.find (variable);
    return (it != synonymToCanonical.end()) ? it->second : variable;
}

juce::StringArray MCPParameterRegistry::suggestSimilar (const juce::String& variable,
                                                         int maxResults,
                                                         int maxDistance) const
{
    struct Scored { juce::String name; int distance; };
    std::vector<Scored> scored;
    scored.reserve (records.size());
    for (const auto& r : records)
    {
        const int d = levenshtein (variable, r.variable);
        if (d <= maxDistance)
            scored.push_back ({ r.variable, d });
    }
    std::sort (scored.begin(), scored.end(),
               [] (const Scored& a, const Scored& b)
               {
                   if (a.distance != b.distance) return a.distance < b.distance;
                   return a.name.compare (b.name) < 0;
               });

    juce::StringArray out;
    for (int i = 0; i < (int) scored.size() && i < maxResults; ++i)
        out.add (scored[(size_t) i].name);
    return out;
}

std::vector<ParameterRegistryRecord>
MCPParameterRegistry::filter (const juce::String& prefix,
                               const juce::String& scope,
                               const juce::String& groupKey) const
{
    std::vector<ParameterRegistryRecord> out;
    out.reserve (records.size());
    for (const auto& r : records)
    {
        if (prefix.isNotEmpty()   && ! r.variable.startsWith (prefix)) continue;
        if (scope.isNotEmpty()    && r.scope    != scope)               continue;
        if (groupKey.isNotEmpty() && r.groupKey != groupKey)            continue;
        out.push_back (r);
    }
    return out;
}

int MCPParameterRegistry::size() const noexcept
{
    return static_cast<int> (records.size());
}

const ParameterRegistryRecord*
MCPParameterRegistry::findByVariable (const juce::String& canonicalVariable) const
{
    for (const auto& r : records)
        if (r.variable == canonicalVariable)
            return &r;
    return nullptr;
}

} // namespace WFSNetwork
