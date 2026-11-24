#pragma once

#include <JuceHeader.h>
#include <array>
#include "../MCPCompat.h"
#include "../../../Parameters/WFSValueTreeState.h"
#include "../../../Parameters/WFSParameterIDs.h"
#include "../../../MapShapes/MapShapeData.h"
#include "ToolArgs.h"

/*
    The Map tab's drawn shapes over MCP: outline-only visual aids (stage edges,
    set pieces, audience areas, notes) stored under <Config><MapShapes>.

    ADDRESSING. A shape has no id: it is its position among the <MapShapes>
    children, which is also the paint order. The tools number them from 1 in
    child order, the same order map_shapes_list reports; deleting a shape shifts
    the ones after it down by one.

    WRITES go through MapDrawing::Shape (validated with isValid(), serialised
    with toValueTree()), so the tree only ever holds what the Map tab editor
    itself would write. The editor and the tablet sync follow tree changes on
    their own (MapTab marks the editor dirty, OSCManager re-sends /remote/map).

    UNDO. By the operator's choice the writes take no UndoManager: they are
    filed in the MCP history but not undoable. An add or delete would leave the
    Map tab's own undo history (child-index based) pointing at the wrong shapes,
    so those two clear it; a property edit cannot misplace anything and keeps it.
*/
namespace WFSNetwork::Tools::MapShapes
{

namespace detail
{
    struct NamedColour { const char* name; juce::uint32 argb; };

    // The Map tab's swatches (MapShapeEditPanel::swatchColours), by name.
    inline const std::array<NamedColour, 8>& palette()
    {
        static const std::array<NamedColour, 8> p {{
            { "white",   0xFFFFFFFF },
            { "grey",    0xFFB0B0B0 },
            { "amber",   0xFFFFC107 },
            { "red",     0xFFFF5252 },
            { "green",   0xFF66BB6A },
            { "cyan",    0xFF26C6DA },
            { "blue",    0xFF448AFF },
            { "magenta", 0xFFE040FB },
        }};
        return p;
    }

    inline constexpr juce::uint32 defaultColour = 0xFFB0B0B0;   // grey, the editor's default

    inline std::optional<juce::Colour> parseColour (const juce::var& raw, juce::String& errorMessage)
    {
        auto text = raw.toString().trim().toLowerCase();
        if (text == "gray") text = "grey";
        for (const auto& c : palette())
            if (text == c.name)
                return juce::Colour (c.argb);

        if (text.startsWith ("#"))   text = text.substring (1);
        if (text.startsWith ("0x"))  text = text.substring (2);
        if ((text.length() == 6 || text.length() == 8) && text.containsOnly ("0123456789abcdef"))
            return juce::Colour::fromString (text.length() == 6 ? "ff" + text : text);

        juce::StringArray names;
        for (const auto& c : palette())
            names.add (c.name);
        errorMessage = "colour must be one of " + names.joinIntoString (", ")
                     + " or a hex value like #ff8800, got " + raw.toString().quoted();
        return std::nullopt;
    }

    inline juce::String colourText (juce::Colour c)
    {
        for (const auto& n : palette())
            if (c.getARGB() == n.argb)
                return n.name;
        const auto hex = c.toDisplayString (c.getAlpha() != 0xff);
        return "#" + hex.toLowerCase();
    }

    inline const char* typeName (MapDrawing::Type t)
    {
        switch (t)
        {
            case MapDrawing::Type::Rectangle: return "rectangle";
            case MapDrawing::Type::Ellipse:   return "ellipse";
            case MapDrawing::Type::Polygon:   return "polygon";
            case MapDrawing::Type::Polyline:  return "polyline";
        }
        return "rectangle";
    }

    inline std::optional<MapDrawing::Type> parseType (const juce::String& text)
    {
        const auto t = text.trim().toLowerCase();
        if (t == "rectangle" || t == "rect")  return MapDrawing::Type::Rectangle;
        if (t == "ellipse"   || t == "circle") return MapDrawing::Type::Ellipse;
        if (t == "polygon")                   return MapDrawing::Type::Polygon;
        if (t == "polyline"  || t == "line")  return MapDrawing::Type::Polyline;
        return std::nullopt;
    }

    /** Points as [[x,y], ...] or [{x,y}, ...], in stage metres. */
    inline std::optional<std::vector<juce::Point<float>>> parsePoints (const juce::var& raw, juce::String& errorMessage)
    {
        const auto* arr = raw.getArray();
        if (arr == nullptr)
        {
            errorMessage = "points must be an array of [x, y] pairs in metres";
            return std::nullopt;
        }
        if (arr->size() > MapDrawing::maxVertices)
        {
            errorMessage = "points has " + juce::String (arr->size()) + " entries; at most "
                         + juce::String (MapDrawing::maxVertices);
            return std::nullopt;
        }

        std::vector<juce::Point<float>> out;
        for (int i = 0; i < arr->size(); ++i)
        {
            const auto& p = arr->getReference (i);
            juce::var xv, yv;
            if (const auto* pair = p.getArray(); pair != nullptr && pair->size() == 2)
            {
                xv = pair->getReference (0);
                yv = pair->getReference (1);
            }
            else if (auto* o = p.getDynamicObject(); o != nullptr && o->hasProperty ("x") && o->hasProperty ("y"))
            {
                xv = o->getProperty ("x");
                yv = o->getProperty ("y");
            }
            else
            {
                errorMessage = "points[" + juce::String (i) + "] must be [x, y]";
                return std::nullopt;
            }

            juce::String code;
            const auto x = Args::number (xv, "x", code, errorMessage);
            const auto y = Args::number (yv, "y", code, errorMessage);
            if (! x.has_value() || ! y.has_value())
            {
                errorMessage = "points[" + juce::String (i) + "]: " + errorMessage;
                return std::nullopt;
            }
            if (std::abs (*x) > MapDrawing::maxCoordinate || std::abs (*y) > MapDrawing::maxCoordinate)
            {
                errorMessage = "points[" + juce::String (i) + "] is outside +/-"
                             + juce::String (MapDrawing::maxCoordinate, 0) + " m";
                return std::nullopt;
            }
            out.push_back ({ static_cast<float> (*x), static_cast<float> (*y) });
        }
        return out;
    }

    inline juce::ValueTree shapesTree (WFSValueTreeState& state)
    {
        return state.getConfigState().getOrCreateChildWithName (WFSParameterIDs::MapShapes, nullptr);
    }

    inline juce::var describeShape (const MapDrawing::Shape& s, int index)
    {
        auto obj = std::make_unique<juce::DynamicObject>();
        obj->setProperty ("index",    index);
        obj->setProperty ("type",     typeName (s.type));
        obj->setProperty ("x",        s.posX);
        obj->setProperty ("y",        s.posY);
        obj->setProperty ("rotation", s.rotation);
        if (s.usesVertices())
        {
            juce::Array<juce::var> pts;
            for (const auto& v : s.vertices)
            {
                const auto p = s.localToStage (v);
                pts.add (juce::var (juce::Array<juce::var> { juce::var (p.x), juce::var (p.y) }));
            }
            obj->setProperty ("points", juce::var (pts));
        }
        else
        {
            obj->setProperty ("width",  2.0f * std::abs (s.scaleX));
            obj->setProperty ("height", 2.0f * std::abs (s.scaleY));
        }
        obj->setProperty ("colour",     colourText (s.colour));
        obj->setProperty ("line_width", s.lineWidth);
        obj->setProperty ("dashed",     s.dashed);
        if (s.label.isNotEmpty())
            obj->setProperty ("label", s.label);

        const auto b = s.getStageBounds();
        auto bounds = std::make_unique<juce::DynamicObject>();
        bounds->setProperty ("min_x", b.getX());
        bounds->setProperty ("min_y", b.getY());
        bounds->setProperty ("max_x", b.getRight());
        bounds->setProperty ("max_y", b.getBottom());
        obj->setProperty ("bounds", juce::var (bounds.release()));
        return juce::var (obj.release());
    }

    inline void fileRecord (ChangeRecord* record, const juce::String& description)
    {
        if (record == nullptr)
            return;
        record->operatorDescription = description + " (map drawing - not undoable)";
        record->affectedParameters.add (WFSParameterIDs::MapShapes.toString());
        record->affectedGroups.push_back ({ 0, juce::String ("Map Shapes") });
        record->undoable = false;
    }

    inline void clearMapUndo (WFSValueTreeState& state)
    {
        if (auto* um = state.getUndoManagerForDomain (UndoDomain::Map))
            um->clearUndoHistory();
    }

    /** Applies the optional style and geometry arguments common to add and
        edit. Returns false with an explanation on a bad argument. */
    inline bool applyCommon (MapDrawing::Shape& s, const juce::DynamicObject& obj,
                             juce::String& code, juce::String& message)
    {
        if (! Args::missing (obj, "colour"))
        {
            auto c = parseColour (obj.getProperty ("colour"), message);
            if (! c.has_value()) { code = "invalid_args"; return false; }
            s.colour = *c;
        }
        if (! Args::missing (obj, "line_width"))
        {
            auto w = Args::number (obj.getProperty ("line_width"), "line_width", code, message);
            if (! w.has_value()) return false;
            if (*w < MapDrawing::minLineWidth || *w > MapDrawing::maxLineWidth)
            {
                code = "out_of_range";
                message = "line_width " + juce::String (*w) + " not in [" + juce::String (MapDrawing::minLineWidth)
                        + ", " + juce::String (MapDrawing::maxLineWidth) + "]";
                return false;
            }
            s.lineWidth = static_cast<float> (*w);
        }
        if (! Args::missing (obj, "dashed"))
        {
            auto d = Args::boolean (obj.getProperty ("dashed"), "dashed", code, message);
            if (! d.has_value()) return false;
            s.dashed = *d;
        }
        if (! Args::missing (obj, "label"))
        {
            const auto label = obj.getProperty ("label").toString();
            if (label.length() > MapDrawing::maxLabelLength)
            {
                code = "out_of_range";
                message = "label is longer than " + juce::String (MapDrawing::maxLabelLength) + " characters";
                return false;
            }
            s.label = label;
        }
        if (! Args::missing (obj, "rotation"))
        {
            auto r = Args::number (obj.getProperty ("rotation"), "rotation", code, message);
            if (! r.has_value()) return false;
            s.rotation = static_cast<float> (std::fmod (*r, 360.0));
        }

        const bool hasSize = ! Args::missing (obj, "width") || ! Args::missing (obj, "height");
        if (hasSize)
        {
            if (s.usesVertices())
            {
                code = "invalid_args";
                message = "width/height apply to rectangles and ellipses; give a polygon or polyline new points instead";
                return false;
            }
            for (const char* name : { "width", "height" })
            {
                if (Args::missing (obj, name))
                    continue;
                auto v = Args::number (obj.getProperty (name), name, code, message);
                if (! v.has_value()) return false;
                const double minimum = 2.0 * MapDrawing::minHalfExtent;
                if (*v < minimum || *v > 2.0 * MapDrawing::maxCoordinate)
                {
                    code = "out_of_range";
                    message = juce::String (name) + " " + juce::String (*v) + " m not in ["
                            + juce::String (minimum) + ", " + juce::String (2.0 * MapDrawing::maxCoordinate, 0) + "]";
                    return false;
                }
                (juce::String (name) == "width" ? s.scaleX : s.scaleY) = static_cast<float> (*v * 0.5);
            }
        }

        for (const char* name : { "x", "y" })
        {
            if (Args::missing (obj, name))
                continue;
            auto v = Args::number (obj.getProperty (name), name, code, message);
            if (! v.has_value()) return false;
            if (std::abs (*v) > MapDrawing::maxCoordinate)
            {
                code = "out_of_range";
                message = juce::String (name) + " is outside +/-" + juce::String (MapDrawing::maxCoordinate, 0) + " m";
                return false;
            }
            (juce::String (name) == "x" ? s.posX : s.posY) = static_cast<float> (*v);
        }
        return true;
    }

    /** 1-based index argument resolved to a child, with its parsed shape. */
    inline std::optional<std::pair<juce::ValueTree, MapDrawing::Shape>>
        resolveIndex (WFSValueTreeState& state, const juce::DynamicObject& obj, int& index,
                      juce::String& code, juce::String& message)
    {
        auto tree = shapesTree (state);
        const int count = tree.getNumChildren();
        if (count == 0)
        {
            code = "empty";
            message = "There are no shapes on the map";
            return std::nullopt;
        }
        const auto i = Args::wholeNumber (obj.getProperty ("index"), "index", 1, count, code, message);
        if (! i.has_value())
            return std::nullopt;
        index = *i;
        auto child = tree.getChild (index - 1);
        auto shape = MapDrawing::Shape::fromValueTree (child);
        if (! shape.has_value())
        {
            code = "invalid_args";
            message = "Shape " + juce::String (index) + " is unreadable (it is skipped on the map); delete it";
            return std::nullopt;
        }
        return std::make_pair (child, *shape);
    }

    inline void styleProps (juce::DynamicObject& props)
    {
        props.setProperty ("colour", Args::stringArg (
            "Outline colour: white, grey, amber, red, green, cyan, blue, magenta (the Map tab's "
            "swatches) or a hex value like #ff8800."));
        props.setProperty ("line_width", Args::numberArg (
            "Outline width in screen pixels (not metres), 0.5-10. Default 2.",
            MapDrawing::minLineWidth, MapDrawing::maxLineWidth));
        props.setProperty ("dashed", Args::booleanArg ("Dashed outline. Default false."));
        props.setProperty ("label", Args::stringArg (
            "Text shown at the top centre of the shape, at most 64 characters. Empty removes it."));
    }

    inline juce::var pointsArg()
    {
        auto pair = std::make_unique<juce::DynamicObject>();
        pair->setProperty ("type", "array");
        auto num = std::make_unique<juce::DynamicObject>();
        num->setProperty ("type", "number");
        pair->setProperty ("items", juce::var (num.release()));
        pair->setProperty ("minItems", 2);
        pair->setProperty ("maxItems", 2);

        auto v = std::make_unique<juce::DynamicObject>();
        v->setProperty ("type", "array");
        v->setProperty ("items", juce::var (pair.release()));
        v->setProperty ("maxItems", MapDrawing::maxVertices);
        v->setProperty ("description",
            "Vertices in stage metres as [[x, y], ...]: at least 3 for a polygon (closed), "
            "at least 2 for a polyline (open; 2 points draw a straight line).");
        return juce::var (v.release());
    }
}

//==============================================================================
// map_shapes_list
//==============================================================================

inline ToolResult list (WFSValueTreeState& state)
{
    auto tree = detail::shapesTree (state);
    juce::Array<juce::var> shapes;
    int unreadable = 0;
    for (int i = 0; i < tree.getNumChildren(); ++i)
    {
        if (auto s = MapDrawing::Shape::fromValueTree (tree.getChild (i)))
            shapes.add (detail::describeShape (*s, i + 1));
        else
            ++unreadable;
    }

    auto result = std::make_unique<juce::DynamicObject>();
    result->setProperty ("count",      tree.getNumChildren());
    result->setProperty ("max_shapes", MapDrawing::maxShapes);
    result->setProperty ("shapes",     juce::var (shapes));
    if (unreadable > 0)
        result->setProperty ("unreadable", unreadable);
    return ToolResult::ok (juce::var (result.release()));
}

inline ToolDescriptor describeList (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "map_shapes_list";
    d.description = "List the outline shapes drawn on the Map tab (visual aids such as stage edges, "
                    "set pieces or audience areas; they do not affect the sound). Each shape has a "
                    "1-based index (its place in the list, also its paint order: later shapes draw "
                    "on top), type, centre x/y and rotation, width/height (rectangle, ellipse) or "
                    "points (polygon, polyline), colour, line_width, dashed, label and its stage "
                    "bounds. Coordinates are stage metres from the origin, +y upstage. Read-only.";
    d.inputSchema   = Args::schemaOf (std::make_unique<juce::DynamicObject>(), {});
    d.modifiesState = false;
    d.tier          = 1;
    d.handler = [&state] (const juce::var&, ChangeRecord*) -> ToolResult { return list (state); };
    return d;
}

//==============================================================================
// map_shape_add
//==============================================================================

inline ToolResult add (WFSValueTreeState& state, const juce::var& args, ChangeRecord* record)
{
    auto* obj = args.getDynamicObject();
    if (obj == nullptr || Args::missing (*obj, "type"))
        return ToolResult::error ("invalid_args", "Missing required arg: type");

    const auto type = detail::parseType (obj->getProperty ("type").toString());
    if (! type.has_value())
        return ToolResult::error ("invalid_args", "type must be rectangle, ellipse, polygon or polyline");

    auto tree = detail::shapesTree (state);
    if (tree.getNumChildren() >= MapDrawing::maxShapes)
        return ToolResult::error ("at_capacity", "The map already holds the maximum of "
                                                 + juce::String (MapDrawing::maxShapes) + " shapes");

    juce::String code, message;
    MapDrawing::Shape s;
    const MapDrawing::Style style { juce::Colour (detail::defaultColour), MapDrawing::defaultLineWidth, false };

    if (*type == MapDrawing::Type::Polygon || *type == MapDrawing::Type::Polyline)
    {
        if (Args::missing (*obj, "points"))
            return ToolResult::error ("invalid_args", "A polygon or polyline needs points");
        for (const char* name : { "x", "y", "width", "height" })
            if (! Args::missing (*obj, name))
                return ToolResult::error ("invalid_args",
                    juce::String (name) + " does not apply to a polygon or polyline: its points place it");
        auto pts = detail::parsePoints (obj->getProperty ("points"), message);
        if (! pts.has_value())
            return ToolResult::error ("invalid_args", message);
        s = MapDrawing::Shape::fromStagePoints (*type, *pts, style);
        if (static_cast<int> (s.vertices.size()) < s.minVertexCount())
            return ToolResult::error ("invalid_args",
                juce::String (detail::typeName (*type)) + " needs at least "
                + juce::String (s.minVertexCount()) + " points");
    }
    else
    {
        if (! Args::missing (*obj, "points"))
            return ToolResult::error ("invalid_args", "points apply to polygons and polylines; a "
                                                      + juce::String (detail::typeName (*type))
                                                      + " takes x, y, width and height");
        for (const char* name : { "x", "y", "width", "height" })
            if (Args::missing (*obj, name))
                return ToolResult::error ("invalid_args", juce::String ("Missing required arg for a ")
                                                            + detail::typeName (*type) + ": " + name);
        s.type = *type;
        s.applyStyle (style);
    }

    if (! detail::applyCommon (s, *obj, code, message))
        return ToolResult::error (code, message);
    if (! s.isValid())
        return ToolResult::error ("invalid_args", "The shape's geometry is not usable (check sizes and coordinates)");

    tree.appendChild (s.toValueTree(), nullptr);
    detail::clearMapUndo (state);
    const int index = tree.getNumChildren();

    detail::fileRecord (record, "Drew a " + juce::String (detail::typeName (s.type)) + " on the map (shape "
                                  + juce::String (index) + (s.label.isNotEmpty() ? ", \"" + s.label + "\"" : juce::String()) + ")");
    return ToolResult::ok (detail::describeShape (s, index));
}

inline ToolDescriptor describeAdd (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "map_shape_add";
    d.description = "Draw an outline shape on the Map tab (a visual aid; it does not affect the sound), "
                    "shown on the desktop map and on connected tablets. Rectangle or ellipse: give the "
                    "centre x, y and the full width, height in metres, optionally rotation in degrees. "
                    "Polygon (closed) or polyline (open, 2 points = a line): give points in stage "
                    "metres. Coordinates are from the origin, +y upstage. Optional colour, line_width, "
                    "dashed, label. Returns the new shape with its index (it goes on top). Up to 256 "
                    "shapes. Clears the Map tab's undo history. Not undoable: delete it instead.";

    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("type", Args::enumArg ("Shape type.", { "rectangle", "ellipse", "polygon", "polyline" }));
    props->setProperty ("x", Args::numberArg ("Centre x in metres (rectangle, ellipse)."));
    props->setProperty ("y", Args::numberArg ("Centre y in metres (rectangle, ellipse)."));
    props->setProperty ("width",  Args::numberArg ("Full width in metres (rectangle, ellipse), before rotation."));
    props->setProperty ("height", Args::numberArg ("Full height in metres (rectangle, ellipse), before rotation."));
    props->setProperty ("rotation", Args::numberArg ("Rotation in degrees, counter-clockwise. Default 0."));
    props->setProperty ("points", detail::pointsArg());
    detail::styleProps (*props);
    d.inputSchema   = Args::schemaOf (std::move (props), { "type" });
    d.modifiesState = true;
    d.tier          = 1;   // a drawing aid, no effect on the sound
    d.handler = [&state] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        return add (state, args, record);
    };
    return d;
}

//==============================================================================
// map_shape_edit
//==============================================================================

inline ToolResult edit (WFSValueTreeState& state, const juce::var& args, ChangeRecord* record)
{
    auto* obj = args.getDynamicObject();
    if (obj == nullptr || Args::missing (*obj, "index"))
        return ToolResult::error ("invalid_args", "Missing required arg: index");

    juce::String code, message;
    int index = 0;
    auto resolved = detail::resolveIndex (state, *obj, index, code, message);
    if (! resolved.has_value())
        return ToolResult::error (code, message);
    auto [child, s] = *resolved;

    if (! Args::missing (*obj, "points"))
    {
        if (! s.usesVertices())
            return ToolResult::error ("invalid_args", "points apply to polygons and polylines; move or resize a "
                                                      + juce::String (detail::typeName (s.type))
                                                      + " with x, y, width and height");
        auto pts = detail::parsePoints (obj->getProperty ("points"), message);
        if (! pts.has_value())
            return ToolResult::error ("invalid_args", message);
        auto rebuilt = MapDrawing::Shape::fromStagePoints (s.type, *pts, s.getStyle());
        rebuilt.label = s.label;
        if (static_cast<int> (rebuilt.vertices.size()) < rebuilt.minVertexCount())
            return ToolResult::error ("invalid_args", juce::String (detail::typeName (s.type)) + " needs at least "
                                                      + juce::String (rebuilt.minVertexCount()) + " points");
        s = rebuilt;
    }

    // x/y move the centre; a vertex shape's points are relative to it, so they follow.
    if (! detail::applyCommon (s, *obj, code, message))
        return ToolResult::error (code, message);

    if (! s.isValid())
        return ToolResult::error ("invalid_args", "The edited shape's geometry is not usable (check sizes and coordinates)");

    child.copyPropertiesFrom (s.toValueTree(), nullptr);
    detail::fileRecord (record, "Edited map shape " + juce::String (index));
    return ToolResult::ok (detail::describeShape (s, index));
}

inline ToolDescriptor describeEdit (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "map_shape_edit";
    d.description = "Change one shape drawn on the Map tab, by its 1-based index from map_shapes_list. "
                    "Give only what changes: x, y (moves the centre; a polygon's points follow), "
                    "rotation, width/height (rectangle, ellipse), points (polygon, polyline: replaces "
                    "every vertex), colour, line_width, dashed, label (empty string removes it). The "
                    "type cannot change: delete and add instead. Returns the shape as it now is. "
                    "Not undoable: edit it back instead.";

    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("index", Args::integerArg ("Shape index from map_shapes_list (1-based).", 1, MapDrawing::maxShapes));
    props->setProperty ("x", Args::numberArg ("New centre x in metres."));
    props->setProperty ("y", Args::numberArg ("New centre y in metres."));
    props->setProperty ("width",  Args::numberArg ("New full width in metres (rectangle, ellipse)."));
    props->setProperty ("height", Args::numberArg ("New full height in metres (rectangle, ellipse)."));
    props->setProperty ("rotation", Args::numberArg ("New rotation in degrees, counter-clockwise."));
    props->setProperty ("points", detail::pointsArg());
    detail::styleProps (*props);
    d.inputSchema   = Args::schemaOf (std::move (props), { "index" });
    d.modifiesState = true;
    d.tier          = 1;
    d.handler = [&state] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        return edit (state, args, record);
    };
    return d;
}

//==============================================================================
// map_shape_delete / map_shapes_clear
//==============================================================================

inline ToolResult remove (WFSValueTreeState& state, const juce::var& args, ChangeRecord* record)
{
    auto* obj = args.getDynamicObject();
    if (obj == nullptr || Args::missing (*obj, "index"))
        return ToolResult::error ("invalid_args", "Missing required arg: index");

    auto tree = detail::shapesTree (state);
    if (tree.getNumChildren() == 0)
        return ToolResult::error ("empty", "There are no shapes on the map");

    juce::String code, message;
    const auto index = Args::wholeNumber (obj->getProperty ("index"), "index", 1, tree.getNumChildren(), code, message);
    if (! index.has_value())
        return ToolResult::error (code, message);

    const auto label = tree.getChild (*index - 1).getProperty (WFSParameterIDs::msLabel).toString();
    tree.removeChild (*index - 1, nullptr);
    detail::clearMapUndo (state);

    detail::fileRecord (record, "Deleted map shape " + juce::String (*index)
                                  + (label.isNotEmpty() ? " (\"" + label + "\")" : juce::String()));

    auto result = std::make_unique<juce::DynamicObject>();
    result->setProperty ("deleted_index", *index);
    result->setProperty ("count", tree.getNumChildren());
    return ToolResult::ok (juce::var (result.release()));
}

inline ToolDescriptor describeDelete (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "map_shape_delete";
    d.description = "Delete one shape drawn on the Map tab, by its 1-based index from map_shapes_list. "
                    "The shapes after it move down one index. Clears the Map tab's undo history. "
                    "Not undoable.";

    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("index", Args::integerArg ("Shape index from map_shapes_list (1-based).", 1, MapDrawing::maxShapes));
    d.inputSchema   = Args::schemaOf (std::move (props), { "index" });
    d.modifiesState = true;
    d.tier          = 1;
    d.handler = [&state] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        return remove (state, args, record);
    };
    return d;
}

inline ToolDescriptor describeClear (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "map_shapes_clear";
    d.description = juce::String (
        "Delete EVERY shape drawn on the Map tab. Clears the Map tab's undo history. Not undoable.")
        + juce::String (kTier2DescriptionSuffix);

    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("confirm", Args::confirmArg());
    d.inputSchema   = Args::schemaOf (std::move (props), {});
    d.modifiesState = true;
    d.tier          = 2;
    d.handler = [&state] (const juce::var&, ChangeRecord* record) -> ToolResult
    {
        auto tree = detail::shapesTree (state);
        const int count = tree.getNumChildren();
        tree.removeAllChildren (nullptr);
        detail::clearMapUndo (state);
        detail::fileRecord (record, "Deleted all " + juce::String (count) + " map shapes");

        auto result = std::make_unique<juce::DynamicObject>();
        result->setProperty ("deleted", count);
        return ToolResult::ok (juce::var (result.release()));
    };
    return d;
}

} // namespace WFSNetwork::Tools::MapShapes
