#pragma once

/*
    MCPToolTemplate — one generated tool standing for a numbered family of
    parameters.

    The codegen collapses a numeric-suffix family such as inputArrayAtten1..10
    into ONE tool whose extra argument picks the member ("array", 1-10), and
    for that tool writes `internal_variable_template` ("inputArrayAtten{array}")
    and `internal_osc_path_template` ("/wfs/input/arrayAtten{array}") instead of
    `internal_variable` / `internal_osc_path`. The placeholder names the
    argument that fills it. Only the family's first row survives the collapse,
    so the tool's prose still describes that member ("... for Array 1.").

    The loader, the parameter registry and both surface auditors read the
    template through here, so they agree on what it means: the loader writes
    the member a call names, and the others see the family as its members.
*/

#include <juce_core/juce_core.h>
#include <cmath>
#include <optional>

namespace WFSNetwork::MCPToolTemplate
{

struct Spec
{
    juce::String variableTemplate;   // "inputArrayAtten{array}"
    juce::String oscPathTemplate;    // "/wfs/input/arrayAtten{array}", may be empty
    juce::String argName;            // "array": the placeholder and the argument that fills it
    int minIndex = 1;                // the argument's declared range
    int maxIndex = 0;

    juce::String placeholder() const               { return "{" + argName + "}"; }
    juce::String variableFor (int index) const     { return variableTemplate.replace (placeholder(), juce::String (index)); }
    juce::String oscPathFor (int index) const      { return oscPathTemplate.replace (placeholder(), juce::String (index)); }
    int memberCount() const noexcept               { return maxIndex - minIndex + 1; }
};

/** The template of one generated_tools.json entry, or nullopt when it has none.
    Also nullopt when the schema does not declare the placeholder as a bounded
    integer argument: with no range there is no member list to expand, and a
    name with the braces still in it is never writable. */
inline std::optional<Spec> read (const juce::DynamicObject& toolObj)
{
    Spec spec;
    spec.variableTemplate = toolObj.getProperty ("internal_variable_template").toString();
    if (spec.variableTemplate.isEmpty())
        return std::nullopt;
    spec.oscPathTemplate = toolObj.getProperty ("internal_osc_path_template").toString();

    const int open  = spec.variableTemplate.indexOfChar ('{');
    const int close = open < 0 ? -1 : spec.variableTemplate.indexOfChar (open + 1, '}');
    if (open < 0 || close <= open + 1)
        return std::nullopt;
    spec.argName = spec.variableTemplate.substring (open + 1, close);

    const auto* params = toolObj.getProperty ("parameters").getDynamicObject();
    const auto* props  = params != nullptr ? params->getProperty ("properties").getDynamicObject() : nullptr;
    const auto* arg    = props  != nullptr ? props->getProperty (spec.argName).getDynamicObject() : nullptr;
    if (arg == nullptr)
        return std::nullopt;

    const auto minVar = arg->getProperty ("minimum");
    const auto maxVar = arg->getProperty ("maximum");
    auto isNumber = [] (const juce::var& v) { return v.isInt() || v.isInt64() || v.isDouble(); };
    if (! isNumber (minVar) || ! isNumber (maxVar))
        return std::nullopt;

    spec.minIndex = static_cast<int> (minVar);
    spec.maxIndex = static_cast<int> (maxVar);
    if (spec.maxIndex < spec.minIndex || spec.memberCount() > 1000)
        return std::nullopt;

    return spec;
}

/** The member one call names: its template argument, which must be a whole
    number within the declared range. A loose-typed client's "3" or 3.0 is
    accepted, as the generated setters accept them for values. On failure
    `errorCode` / `errorMessage` say why, ready for ToolResult::error. */
inline std::optional<int> readIndex (const Spec& spec, const juce::var& args,
                                     juce::String& errorCode, juce::String& errorMessage)
{
    const auto* argsObj = args.getDynamicObject();
    if (argsObj == nullptr || ! argsObj->hasProperty (spec.argName))
    {
        errorCode = "invalid_args";
        errorMessage = "Missing required arg: " + spec.argName;
        return std::nullopt;
    }

    const auto raw = argsObj->getProperty (spec.argName);
    const auto text = raw.toString().trim();
    double d = 0.0;
    bool numeric = true;
    if (raw.isInt() || raw.isInt64() || raw.isDouble())
        d = static_cast<double> (raw);
    else if (raw.isString() && text.isNotEmpty() && text.containsOnly ("0123456789.+-eE"))
        d = text.getDoubleValue();
    else
        numeric = false;

    if (! numeric || ! std::isfinite (d) || std::floor (d) != d)
    {
        errorCode = "invalid_args";
        errorMessage = spec.argName + " must be an integer from " + juce::String (spec.minIndex)
                     + " to " + juce::String (spec.maxIndex) + ", got " + text.quoted();
        return std::nullopt;
    }

    if (d < spec.minIndex || d > spec.maxIndex)
    {
        errorCode = "out_of_range";
        errorMessage = spec.argName + " " + text + " not in ["
                     + juce::String (spec.minIndex) + ", " + juce::String (spec.maxIndex) + "]";
        return std::nullopt;
    }
    return static_cast<int> (d);
}

/** The family's prose for one member. The surviving text names the first
    member: every whole-number occurrence of `firstIndex` becomes `replacement`
    ("for Array 1." -> "for Array 3."), which is what that member's own CSV row
    says. Digits inside a longer number or a word are left alone. */
inline juce::String describeMember (const juce::String& text, int firstIndex, const juce::String& replacement)
{
    const juce::String token (firstIndex);
    juce::String out;
    int pos = 0;
    for (;;)
    {
        const int found = text.indexOf (pos, token);
        if (found < 0)
            break;
        const int end = found + token.length();
        const bool startsWord = found == 0 || ! juce::CharacterFunctions::isLetterOrDigit (text[found - 1]);
        const bool endsWord   = end >= text.length() || ! juce::CharacterFunctions::isLetterOrDigit (text[end]);
        out << text.substring (pos, found) << (startsWord && endsWord ? replacement : token);
        pos = end;
    }
    out << text.substring (pos);
    return out;
}

} // namespace WFSNetwork::MCPToolTemplate
