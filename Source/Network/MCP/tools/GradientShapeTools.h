#pragma once

#include <JuceHeader.h>
#include "../MCPCompat.h"
#include "../../../Parameters/WFSValueTreeState.h"
#include "../../../Parameters/WFSParameterIDs.h"
#include "../../../Parameters/VarCoercion.h"
#include "../../../GradientMap/GradientMapData.h"
#include "InputTools.h"
#include "ToolArgs.h"

/*
    Gradient-map shapes over MCP: list, add, delete, switch on/off, bring to
    front or send to back. The generated input_gradient_map_set_gm_shape_* tools
    edit an existing shape; a layer starts with none, and those could only be
    drawn in the editor.

    A shape is addressed as the generated tools address it: `shape`, 0-based,
    its position among the layer's GradientShape children. Paint order is the
    separate gmShapeOrder (later = on top), which is what front / back change; a
    new shape is appended and goes on top. Every change rebuilds the input's
    rasterised map (the gradientMapChanged hook), as the editor does.
*/
namespace WFSNetwork::Tools::GradientShapes
{

namespace detail
{
    struct Target { int inputId = 0; int slot = -1; int layer = 0; juce::ValueTree layerTree; };

    inline std::optional<Target> resolve (WFSValueTreeState& state, const juce::DynamicObject& obj,
                                          juce::String& code, juce::String& message)
    {
        for (const char* arg : { "input_id", "layer" })
            if (Args::missing (obj, arg))
            {
                code = "invalid_args";
                message = juce::String ("Missing required arg: ") + arg;
                return std::nullopt;
            }
        Target t;
        const auto id = Args::wholeNumber (obj.getProperty ("input_id"), "input_id", 1,
                                           WFSParameterDefaults::maxInputChannels, code, message);
        if (! id.has_value()) return std::nullopt;
        const auto layer = Args::wholeNumber (obj.getProperty ("layer"), "layer", 0, 2, code, message);
        if (! layer.has_value()) return std::nullopt;
        t.inputId = *id;
        t.layer = *layer;
        t.slot = Input::detail::resolveChannelIndex (state, t.inputId);
        if (t.slot < 0)
        {
            code = "invalid_args";
            message = "input_id is not a live input channel: " + juce::String (t.inputId);
            return std::nullopt;
        }
        t.layerTree = state.getInputGradientLayer (t.slot, t.layer);
        if (! t.layerTree.isValid())
        {
            code = "invalid_args";
            message = "Input " + juce::String (t.inputId) + " has no gradient-map layer " + juce::String (t.layer);
            return std::nullopt;
        }
        return t;
    }

    inline std::vector<juce::ValueTree> shapes (const juce::ValueTree& layer)
    {
        std::vector<juce::ValueTree> out;
        for (int i = 0; i < layer.getNumChildren(); ++i)
            if (layer.getChild (i).hasType (WFSParameterIDs::GradientShape))
                out.push_back (layer.getChild (i));
        return out;
    }

    inline std::optional<juce::ValueTree> shapeAt (const Target& t, const juce::DynamicObject& obj,
                                                   int& index, juce::String& code, juce::String& message)
    {
        const auto list = shapes (t.layerTree);
        if (list.empty())
        {
            code = "empty";
            message = "That layer has no shapes";
            return std::nullopt;
        }
        const auto i = Args::wholeNumber (obj.getProperty ("shape"), "shape", 0,
                                          static_cast<int> (list.size()) - 1, code, message);
        if (! i.has_value()) return std::nullopt;
        index = *i;
        return list[static_cast<size_t> (index)];
    }

    inline void notify (const std::function<void (int)>* changed, int slot)
    {
        if (changed != nullptr && *changed)
            (*changed) (slot);
    }

    inline void file (ChangeRecord* record, const Target& t, const juce::String& what)
    {
        if (record == nullptr)
            return;
        record->operatorDescription = what + " on input " + juce::String (t.inputId) + " layer "
                                    + juce::String (t.layer) + " (not undoable)";
        record->affectedParameters.add (WFSParameterIDs::GradientShape.toString());
        record->affectedGroups.push_back ({ t.inputId, juce::String ("Gradient Map") });
        record->undoable = false;
    }

    inline juce::var describeShape (const juce::ValueTree& tree, int index)
    {
        const auto s = GradientMap::Shape::fromValueTree (tree);
        static const char* types[] = { "rectangle", "ellipse", "polygon" };
        static const char* fills[] = { "uniform", "linear_gradient", "radial_gradient" };
        auto obj = std::make_unique<juce::DynamicObject>();
        obj->setProperty ("shape", index);
        obj->setProperty ("type", types[juce::jlimit (0, 2, static_cast<int> (s.type))]);
        obj->setProperty ("x", s.posX);
        obj->setProperty ("y", s.posY);
        obj->setProperty ("rotation", s.rotation);
        obj->setProperty ("half_width", s.scaleX);
        obj->setProperty ("half_height", s.scaleY);
        obj->setProperty ("fill", fills[juce::jlimit (0, 2, static_cast<int> (s.fillType))]);
        obj->setProperty ("fill_value", s.fillValue);
        obj->setProperty ("blur", s.blur);
        obj->setProperty ("enabled", s.enabled);
        obj->setProperty ("order", s.order);
        if (s.name.isNotEmpty())
            obj->setProperty ("name", s.name);
        return juce::var (obj.release());
    }

    inline juce::var baseProps (juce::DynamicObject& props)
    {
        props.setProperty ("input_id", Args::integerArg ("Permanent input number.", 1, WFSParameterDefaults::maxInputChannels));
        props.setProperty ("layer", Args::integerArg ("Gradient-map layer: 0 attenuation, 1 height, 2 HF.", 0, 2));
        return {};
    }
}

inline ToolDescriptor describeList (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "input_gradient_shapes_list";
    d.description = "List the shapes of one input's gradient-map layer: each with its 0-based `shape` index "
                    "(what the input_gradient_map_set_gm_shape_* tools take), type, centre, rotation, "
                    "half-extents (metres), fill, blur, enabled and paint order (higher draws on top). "
                    "Read-only.";
    auto props = std::make_unique<juce::DynamicObject>();
    detail::baseProps (*props);
    d.inputSchema   = Args::schemaOf (std::move (props), { "input_id", "layer" });
    d.modifiesState = false;
    d.tier          = 1;
    d.handler = [&state] (const juce::var& args, ChangeRecord*) -> ToolResult
    {
        auto* obj = args.getDynamicObject();
        juce::String code, message;
        if (obj == nullptr) return ToolResult::error ("invalid_args", "Arguments must be a JSON object");
        const auto t = detail::resolve (state, *obj, code, message);
        if (! t.has_value()) return ToolResult::error (code, message);
        juce::Array<juce::var> out;
        const auto list = detail::shapes (t->layerTree);
        for (size_t i = 0; i < list.size(); ++i)
            out.add (detail::describeShape (list[i], static_cast<int> (i)));
        auto result = std::make_unique<juce::DynamicObject>();
        result->setProperty ("input_id", t->inputId);
        result->setProperty ("layer", t->layer);
        result->setProperty ("shapes", juce::var (out));
        return ToolResult::ok (juce::var (result.release()));
    };
    return d;
}

inline ToolDescriptor describeAdd (WFSValueTreeState& state, const std::function<void (int)>* changed)
{
    ToolDescriptor d;
    d.name        = "input_gradient_shape_add";
    d.description = "Draw a shape on one input's gradient-map layer, filled with a uniform grey (fill_value "
                    "0 black .. 1 white; the layer maps white and black to its values). Rectangle or ellipse: "
                    "centre x, y and full width, height in stage metres, optional rotation. Polygon: points "
                    "[[x, y], ...] in stage metres (3 or more). Optional blur (metres) and name. It goes on "
                    "top and returns its `shape` index; gradients and fine edits use the "
                    "input_gradient_map_set_gm_shape_* tools. The layer must be enabled to act. Not undoable: "
                    "delete it instead.";
    auto props = std::make_unique<juce::DynamicObject>();
    detail::baseProps (*props);
    props->setProperty ("type", Args::enumArg ("Shape type.", { "rectangle", "ellipse", "polygon" }));
    props->setProperty ("x", Args::numberArg ("Centre x, metres.", -50.0, 50.0));
    props->setProperty ("y", Args::numberArg ("Centre y, metres.", -50.0, 50.0));
    props->setProperty ("width", Args::numberArg ("Full width, metres.", 0.02, 100.0));
    props->setProperty ("height", Args::numberArg ("Full height, metres.", 0.02, 100.0));
    props->setProperty ("rotation", Args::numberArg ("Degrees, counter-clockwise. Default 0."));
    auto points = std::make_unique<juce::DynamicObject>();
    points->setProperty ("type", "array");
    points->setProperty ("description", "Polygon vertices [[x, y], ...] in stage metres.");
    props->setProperty ("points", juce::var (points.release()));
    props->setProperty ("fill_value", Args::numberArg ("Grey level 0 (black) .. 1 (white). Default 1.", 0.0, 1.0));
    props->setProperty ("blur", Args::numberArg ("Edge blur, metres. Default 0.", 0.0, 5.0));
    props->setProperty ("name", Args::stringArg ("Optional name."));
    d.inputSchema   = Args::schemaOf (std::move (props), { "input_id", "layer", "type" });
    d.modifiesState = true;
    d.tier          = 1;
    d.handler = [&state, changed] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        auto* obj = args.getDynamicObject();
        juce::String code, message;
        if (obj == nullptr || Args::missing (*obj, "type"))
            return ToolResult::error ("invalid_args", "Missing required arg: type");
        const auto t = detail::resolve (state, *obj, code, message);
        if (! t.has_value()) return ToolResult::error (code, message);

        GradientMap::Shape s;
        const auto type = obj->getProperty ("type").toString().toLowerCase();
        auto num = [&] (const char* name, float fallback, double lo, double hi) -> std::optional<float>
        {
            if (Args::missing (*obj, name)) return fallback;
            const auto v = Args::number (obj->getProperty (name), name, code, message);
            if (! v.has_value()) return std::nullopt;
            if (*v < lo || *v > hi)
            {
                code = "out_of_range";
                message = juce::String (name) + " " + juce::String (*v) + " not in [" + juce::String (lo) + ", " + juce::String (hi) + "]";
                return std::nullopt;
            }
            return static_cast<float> (*v);
        };

        if (type == "polygon")
        {
            const auto* arr = obj->getProperty ("points").getArray();
            if (arr == nullptr || arr->size() < 3)
                return ToolResult::error ("invalid_args", "A polygon needs points: at least 3 [x, y] pairs");
            std::vector<juce::Point<float>> pts;
            for (const auto& p : *arr)
            {
                const auto* pair = p.getArray();
                if (pair == nullptr || pair->size() != 2)
                    return ToolResult::error ("invalid_args", "Each point must be [x, y]");
                pts.push_back ({ static_cast<float> (static_cast<double> (pair->getReference (0))),
                                 static_cast<float> (static_cast<double> (pair->getReference (1))) });
            }
            juce::Point<float> c;
            for (auto& p : pts) c += p;
            c /= static_cast<float> (pts.size());
            s.type = GradientMap::ShapeType::Polygon;
            s.posX = c.x;
            s.posY = c.y;
            for (auto& p : pts) s.vertices.push_back (p - c);
        }
        else if (type == "rectangle" || type == "ellipse")
        {
            for (const char* name : { "x", "y", "width", "height" })
                if (Args::missing (*obj, name))
                    return ToolResult::error ("invalid_args", juce::String ("Missing required arg for a ") + type + ": " + name);
            const auto x = num ("x", 0, -50, 50), y = num ("y", 0, -50, 50);
            const auto w = num ("width", 2, 0.02, 100), h = num ("height", 2, 0.02, 100);
            if (! x || ! y || ! w || ! h) return ToolResult::error (code, message);
            s.type = type == "rectangle" ? GradientMap::ShapeType::Rectangle : GradientMap::ShapeType::Ellipse;
            s.posX = *x; s.posY = *y; s.scaleX = *w * 0.5f; s.scaleY = *h * 0.5f;
        }
        else
            return ToolResult::error ("invalid_args", "type must be rectangle, ellipse or polygon");

        const auto rot = num ("rotation", 0, -360, 360), fill = num ("fill_value", 1, 0, 1), blur = num ("blur", 0, 0, 5);
        if (! rot || ! fill || ! blur) return ToolResult::error (code, message);
        s.rotation  = *rot;
        s.fillType  = GradientMap::FillType::Uniform;
        s.fillValue = *fill;
        s.blur      = *blur;
        s.name      = obj->getProperty ("name").toString();

        int top = -1;
        const auto list = detail::shapes (t->layerTree);
        for (const auto& existing : list)
            top = juce::jmax (top, WFSVar::toInt (existing.getProperty (WFSParameterIDs::gmShapeOrder)));
        s.order = top + 1;

        juce::ValueTree layerTree (t->layerTree);   // a handle on the same node
        layerTree.appendChild (s.toValueTree(), nullptr);
        detail::notify (changed, t->slot);
        detail::file (record, *t, "Drew a gradient shape");
        return ToolResult::ok (detail::describeShape (detail::shapes (t->layerTree).back(),
                                                      static_cast<int> (list.size())));
    };
    return d;
}

inline ToolDescriptor describeEdit (WFSValueTreeState& state, const std::function<void (int)>* changed)
{
    ToolDescriptor d;
    d.name        = "input_gradient_shape_arrange";
    d.description = "Delete, switch on/off, bring to front or send to back one gradient-map shape (by its "
                    "`shape` index from input_gradient_shapes_list). action: delete | enable | disable | "
                    "front | back. A disabled shape stays in the layer but no longer shapes the map. After a "
                    "delete, later shapes move down one index. Not undoable.";
    auto props = std::make_unique<juce::DynamicObject>();
    detail::baseProps (*props);
    props->setProperty ("shape", Args::integerArg ("Shape index (0-based).", 0));
    props->setProperty ("action", Args::enumArg ("What to do.", { "delete", "enable", "disable", "front", "back" }));
    d.inputSchema   = Args::schemaOf (std::move (props), { "input_id", "layer", "shape", "action" });
    d.modifiesState = true;
    d.tier          = 1;
    d.handler = [&state, changed] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        auto* obj = args.getDynamicObject();
        juce::String code, message;
        if (obj == nullptr || Args::missing (*obj, "shape") || Args::missing (*obj, "action"))
            return ToolResult::error ("invalid_args", "Missing required arg: shape and action");
        const auto t = detail::resolve (state, *obj, code, message);
        if (! t.has_value()) return ToolResult::error (code, message);
        int index = 0;
        auto shape = detail::shapeAt (*t, *obj, index, code, message);
        if (! shape.has_value()) return ToolResult::error (code, message);

        const auto action = obj->getProperty ("action").toString().toLowerCase();
        const auto list = detail::shapes (t->layerTree);
        if (action == "delete")
            juce::ValueTree (t->layerTree).removeChild (*shape, nullptr);
        else if (action == "enable" || action == "disable")
            shape->setProperty (WFSParameterIDs::gmShapeEnabled, action == "enable" ? 1 : 0, nullptr);
        else if (action == "front" || action == "back")
        {
            int lo = 0, hi = 0;
            for (const auto& s : list)
            {
                const int o = WFSVar::toInt (s.getProperty (WFSParameterIDs::gmShapeOrder));
                lo = juce::jmin (lo, o);
                hi = juce::jmax (hi, o);
            }
            shape->setProperty (WFSParameterIDs::gmShapeOrder, action == "front" ? hi + 1 : lo - 1, nullptr);
        }
        else
            return ToolResult::error ("invalid_args", "action must be delete, enable, disable, front or back");

        detail::notify (changed, t->slot);
        detail::file (record, *t, "Gradient shape " + juce::String (index) + ": " + action);
        auto result = std::make_unique<juce::DynamicObject>();
        result->setProperty ("input_id", t->inputId);
        result->setProperty ("layer", t->layer);
        result->setProperty ("shape", index);
        result->setProperty ("action", action);
        result->setProperty ("shapes_in_layer", static_cast<int> (detail::shapes (t->layerTree).size()));
        return ToolResult::ok (juce::var (result.release()));
    };
    return d;
}

} // namespace WFSNetwork::Tools::GradientShapes
