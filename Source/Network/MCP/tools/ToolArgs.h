#pragma once

#include <JuceHeader.h>
#include <cmath>
#include <optional>
#include <vector>

/*
    Argument helpers shared by the hand-written MCP tools.

    The dispatcher enforces neither `required` nor `additionalProperties`: a
    missing argument casts to 0 / false / "" and the write goes through with the
    wrong meaning. Every hand-written handler therefore checks its own arguments,
    and these are the checks, in one place, so the tools agree on what a client
    may send (loose forms such as "3" or 3.0 for an integer are accepted, as the
    generated tools accept them).
*/
namespace WFSNetwork::Tools::Args
{

inline juce::var integerArg (const juce::String& description, int minimum)
{
    auto v = std::make_unique<juce::DynamicObject>();
    v->setProperty ("type", "integer");
    v->setProperty ("minimum", minimum);
    v->setProperty ("description", description);
    return juce::var (v.release());
}

inline juce::var integerArg (const juce::String& description, int minimum, int maximum)
{
    auto v = std::make_unique<juce::DynamicObject>();
    v->setProperty ("type", "integer");
    v->setProperty ("minimum", minimum);
    v->setProperty ("maximum", maximum);
    v->setProperty ("description", description);
    return juce::var (v.release());
}

inline juce::var numberArg (const juce::String& description)
{
    auto v = std::make_unique<juce::DynamicObject>();
    v->setProperty ("type", "number");
    v->setProperty ("description", description);
    return juce::var (v.release());
}

inline juce::var numberArg (const juce::String& description, double minimum, double maximum)
{
    auto v = std::make_unique<juce::DynamicObject>();
    v->setProperty ("type", "number");
    v->setProperty ("minimum", minimum);
    v->setProperty ("maximum", maximum);
    v->setProperty ("description", description);
    return juce::var (v.release());
}

inline juce::var stringArg (const juce::String& description)
{
    auto v = std::make_unique<juce::DynamicObject>();
    v->setProperty ("type", "string");
    v->setProperty ("description", description);
    return juce::var (v.release());
}

inline juce::var enumArg (const juce::String& description, const juce::StringArray& values)
{
    juce::Array<juce::var> vals;
    for (const auto& s : values)
        vals.add (s);
    auto v = std::make_unique<juce::DynamicObject>();
    v->setProperty ("type", "string");
    v->setProperty ("enum", juce::var (vals));
    v->setProperty ("description", description);
    return juce::var (v.release());
}

inline juce::var booleanArg (const juce::String& description)
{
    auto v = std::make_unique<juce::DynamicObject>();
    v->setProperty ("type", "boolean");
    v->setProperty ("description", description);
    return juce::var (v.release());
}

inline juce::var integerArrayArg (const juce::String& description, int minimum, int maximum)
{
    auto items = std::make_unique<juce::DynamicObject>();
    items->setProperty ("type", "integer");
    items->setProperty ("minimum", minimum);
    items->setProperty ("maximum", maximum);

    auto v = std::make_unique<juce::DynamicObject>();
    v->setProperty ("type", "array");
    v->setProperty ("items", juce::var (items.release()));
    v->setProperty ("description", description);
    return juce::var (v.release());
}

/** The optional confirm token a tier-2/3 tool must declare, because its schema
    says additionalProperties:false and the second call carries it. */
inline juce::var confirmArg()
{
    return stringArg ("Confirmation token from the previous call's tier_enforcement "
                      "envelope. Re-call with confirm set to this token within 30s.");
}

inline juce::var schemaOf (std::unique_ptr<juce::DynamicObject> props,
                           std::initializer_list<const char*> required)
{
    juce::Array<juce::var> req;
    for (auto* r : required)
        req.add (r);

    auto schema = std::make_unique<juce::DynamicObject>();
    schema->setProperty ("type", "object");
    schema->setProperty ("properties", juce::var (props.release()));
    if (! req.isEmpty())
        schema->setProperty ("required", juce::var (req));
    schema->setProperty ("additionalProperties", false);
    return juce::var (schema.release());
}

inline bool isNumber (const juce::var& v) { return v.isInt() || v.isInt64() || v.isDouble(); }

inline bool missing (const juce::DynamicObject& obj, const char* name)
{
    return ! obj.hasProperty (name) || obj.getProperty (name).isVoid();
}

/** A finite number, from a JSON number or a numeric string, or an explanation. */
inline std::optional<double> number (const juce::var& raw, const char* name,
                                     juce::String& errorCode, juce::String& errorMessage)
{
    const auto text = raw.toString().trim();
    double d = 0.0;
    if (isNumber (raw))
        d = static_cast<double> (raw);
    else if (raw.isString() && text.isNotEmpty() && text.containsOnly ("0123456789.+-eE"))
        d = text.getDoubleValue();
    else
    {
        errorCode = "invalid_args";
        errorMessage = juce::String (name) + " must be a number, got " + text.quoted();
        return std::nullopt;
    }
    if (! std::isfinite (d))
    {
        errorCode = "invalid_args";
        errorMessage = juce::String (name) + " must be a finite number";
        return std::nullopt;
    }
    return d;
}

/** A whole number within [minimum, maximum], or an explanation. */
inline std::optional<int> wholeNumber (const juce::var& raw, const char* name, int minimum, int maximum,
                                       juce::String& errorCode, juce::String& errorMessage)
{
    const auto text = raw.toString().trim();
    double d = 0.0;
    if (isNumber (raw))
        d = static_cast<double> (raw);
    else if (raw.isString() && text.isNotEmpty() && text.containsOnly ("0123456789.+-eE"))
        d = text.getDoubleValue();
    else
    {
        errorCode = "invalid_args";
        errorMessage = juce::String (name) + " must be an integer, got " + text.quoted();
        return std::nullopt;
    }

    if (! std::isfinite (d) || std::floor (d) != d)
    {
        errorCode = "invalid_args";
        errorMessage = juce::String (name) + " must be an integer, got " + text.quoted();
        return std::nullopt;
    }
    if (d < minimum || d > maximum)
    {
        errorCode = "out_of_range";
        errorMessage = juce::String (name) + " " + text + " not in [" + juce::String (minimum)
                     + ", " + juce::String (maximum) + "]";
        return std::nullopt;
    }
    return static_cast<int> (d);
}

/** A boolean from true/false or a number (0 = false). */
inline std::optional<bool> boolean (const juce::var& raw, const char* name,
                                    juce::String& errorCode, juce::String& errorMessage)
{
    if (raw.isBool() || isNumber (raw))
        return static_cast<bool> (raw);
    const auto text = raw.toString().trim().toLowerCase();
    if (text == "true" || text == "on")   return true;
    if (text == "false" || text == "off") return false;
    errorCode = "invalid_args";
    errorMessage = juce::String (name) + " must be true or false";
    return std::nullopt;
}

/** An array of whole numbers within [minimum, maximum], or an explanation. */
inline std::optional<std::vector<int>> wholeNumberArray (const juce::var& raw, const char* name,
                                                         int minimum, int maximum,
                                                         juce::String& errorCode, juce::String& errorMessage)
{
    const auto* arr = raw.getArray();
    if (arr == nullptr)
    {
        errorCode = "invalid_args";
        errorMessage = juce::String (name) + " must be an array of integers";
        return std::nullopt;
    }
    std::vector<int> out;
    for (int i = 0; i < arr->size(); ++i)
    {
        const auto label = juce::String (name) + "[" + juce::String (i) + "]";
        auto v = wholeNumber (arr->getReference (i), label.toRawUTF8(), minimum, maximum, errorCode, errorMessage);
        if (! v.has_value())
            return std::nullopt;
        out.push_back (*v);
    }
    return out;
}

} // namespace WFSNetwork::Tools::Args
