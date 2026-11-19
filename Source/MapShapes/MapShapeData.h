#pragma once

#include <JuceHeader.h>
#include <optional>
#include <vector>
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/VarCoercion.h"
#include "../GradientMap/GradientMapData.h"

/**
 * Map Shapes - outline-only visual aids drawn on the Map tab.
 *
 * Stored under <Config><MapShapes> (system.xml), shown on the desktop Map and
 * mirrored read-only to the tablet map (/remote/map/*).
 *
 * Geometry deliberately follows GradientMap::Shape: the centre (posX/posY) is
 * in origin-relative stage metres, rotation in degrees, scaleX/scaleY are
 * half-extents, and polygon/polyline vertices are shape-local before scale and
 * rotation. Both editors use the same stage frame, so a shape copied between
 * the Map and a gradient map lands exactly where it was.
 *
 * Unlike gradient shapes these are never filled, and a Polyline stays open.
 */
namespace MapDrawing
{

enum class Type { Rectangle = 0, Ellipse = 1, Polygon = 2, Polyline = 3 };

constexpr int   maxShapes          = 256;
constexpr int   maxVertices        = 256;
constexpr int   freehandPointCap   = 128;
constexpr int   ellipseSegments    = 48;
constexpr int   maxLabelLength     = 64;
constexpr float minLineWidth       = 0.5f;
constexpr float maxLineWidth       = 10.0f;
constexpr float defaultLineWidth   = 2.0f;
constexpr float minHalfExtent      = 0.01f;
constexpr float maxCoordinate      = 10000.0f;

/** Dash pattern shared with the tablet: dash max(4, 3w), gap max(3, 2w). */
inline float dashLength (float lineWidth) { return juce::jmax (4.0f, 3.0f * lineWidth); }
inline float gapLength  (float lineWidth) { return juce::jmax (3.0f, 2.0f * lineWidth); }

/** The style the panel applies to new shapes (and to the selection). */
struct Style
{
    juce::Colour colour { 0xFFFFFFFF };
    float lineWidth = defaultLineWidth;
    bool dashed = false;
};

//==============================================================================
// Small geometry helpers
//==============================================================================

inline float distanceToSegment (juce::Point<float> p, juce::Point<float> a, juce::Point<float> b)
{
    const auto ab = b - a;
    const float len2 = ab.x * ab.x + ab.y * ab.y;
    if (len2 <= 1.0e-12f)
        return p.getDistanceFrom (a);
    const float t = juce::jlimit (0.0f, 1.0f, ((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / len2);
    return p.getDistanceFrom (a + ab * t);
}

/** Distance from p to a polyline (closed adds the last→first segment). */
inline float distanceToPolyline (juce::Point<float> p, const std::vector<juce::Point<float>>& pts, bool closed)
{
    if (pts.empty())
        return std::numeric_limits<float>::max();
    if (pts.size() == 1)
        return p.getDistanceFrom (pts[0]);

    float best = std::numeric_limits<float>::max();
    for (size_t i = 0; i + 1 < pts.size(); ++i)
        best = juce::jmin (best, distanceToSegment (p, pts[i], pts[i + 1]));
    if (closed && pts.size() > 2)
        best = juce::jmin (best, distanceToSegment (p, pts.back(), pts.front()));
    return best;
}

/** Ramer-Douglas-Peucker simplification. Keeps both end points. */
inline std::vector<juce::Point<float>> simplifyRdp (const std::vector<juce::Point<float>>& pts, float epsilon)
{
    if (pts.size() < 3)
        return pts;

    std::vector<bool> keep (pts.size(), false);
    keep.front() = keep.back() = true;

    std::vector<std::pair<size_t, size_t>> stack { { 0, pts.size() - 1 } };
    while (! stack.empty())
    {
        auto [first, last] = stack.back();
        stack.pop_back();

        float maxDist = 0.0f;
        size_t index = first;
        for (size_t i = first + 1; i < last; ++i)
        {
            const float d = distanceToSegment (pts[i], pts[first], pts[last]);
            if (d > maxDist) { maxDist = d; index = i; }
        }

        if (maxDist > epsilon && index != first)
        {
            keep[index] = true;
            stack.push_back ({ first, index });
            stack.push_back ({ index, last });
        }
    }

    std::vector<juce::Point<float>> out;
    for (size_t i = 0; i < pts.size(); ++i)
        if (keep[i])
            out.push_back (pts[i]);
    return out;
}

/** RDP with epsilon doubled until the result fits within cap points. */
inline std::vector<juce::Point<float>> simplifyToCap (const std::vector<juce::Point<float>>& pts,
                                                      float epsilon, int cap = freehandPointCap)
{
    auto out = simplifyRdp (pts, epsilon);
    float eps = juce::jmax (epsilon, 1.0e-6f);
    for (int guard = 0; static_cast<int> (out.size()) > cap && guard < 64; ++guard)
    {
        eps *= 2.0f;
        out = simplifyRdp (pts, eps);
    }
    return out;
}

//==============================================================================
// Shape
//==============================================================================

struct Shape
{
    Type type = Type::Rectangle;
    float posX = 0.0f, posY = 0.0f;
    float rotation = 0.0f;
    float scaleX = 1.0f, scaleY = 1.0f;
    std::vector<juce::Point<float>> vertices;   // Polygon / Polyline only

    juce::Colour colour { 0xFFFFFFFF };
    float lineWidth = defaultLineWidth;
    bool dashed = false;
    juce::String label;

    bool isClosed() const       { return type != Type::Polyline; }
    bool usesVertices() const   { return type == Type::Polygon || type == Type::Polyline; }
    int  minVertexCount() const { return type == Type::Polyline ? 2 : 3; }

    Style getStyle() const { return { colour, lineWidth, dashed }; }
    void applyStyle (const Style& s) { colour = s.colour; lineWidth = s.lineWidth; dashed = s.dashed; }

    juce::AffineTransform getTransform() const
    {
        return juce::AffineTransform::scale (scaleX, scaleY)
                   .rotated (juce::degreesToRadians (rotation))
                   .translated (posX, posY);
    }

    juce::Point<float> localToStage (juce::Point<float> local) const { return local.transformedBy (getTransform()); }
    juce::Point<float> stageToLocal (juce::Point<float> stage) const { return stage.transformedBy (getTransform().inverted()); }

    /** Outline in stage coordinates (never filled; a Polyline is not closed). */
    juce::Path getPath() const
    {
        juce::Path path;
        const auto pts = getOutlinePoints();
        if (pts.size() < 2)
            return path;

        path.startNewSubPath (pts[0]);
        for (size_t i = 1; i < pts.size(); ++i)
            path.lineTo (pts[i]);
        if (isClosed())
            path.closeSubPath();
        return path;
    }

    /** The outline flattened to stage-coordinate points. For closed shapes the
        first point is NOT repeated at the end. Rectangles give 4 points,
        ellipses `segments` points. This is what the tablet receives. */
    std::vector<juce::Point<float>> getOutlinePoints (int segments = ellipseSegments) const
    {
        std::vector<juce::Point<float>> local;
        switch (type)
        {
            case Type::Rectangle:
                local = { { -1.0f, -1.0f }, { 1.0f, -1.0f }, { 1.0f, 1.0f }, { -1.0f, 1.0f } };
                break;
            case Type::Ellipse:
                local.reserve (static_cast<size_t> (segments));
                for (int i = 0; i < segments; ++i)
                {
                    const float a = juce::MathConstants<float>::twoPi * static_cast<float> (i) / static_cast<float> (segments);
                    local.push_back ({ std::cos (a), std::sin (a) });
                }
                break;
            case Type::Polygon:
            case Type::Polyline:
                local = vertices;
                break;
        }

        const auto t = getTransform();
        for (auto& p : local)
            p = p.transformedBy (t);
        return local;
    }

    juce::Rectangle<float> getStageBounds() const
    {
        const auto pts = getOutlinePoints();
        if (pts.empty())
            return { posX, posY, 0.0f, 0.0f };
        float minX = pts[0].x, maxX = pts[0].x, minY = pts[0].y, maxY = pts[0].y;
        for (auto& p : pts)
        {
            minX = juce::jmin (minX, p.x); maxX = juce::jmax (maxX, p.x);
            minY = juce::jmin (minY, p.y); maxY = juce::jmax (maxY, p.y);
        }
        return { minX, minY, maxX - minX, maxY - minY };
    }

    /** Where the label sits: top-centre of the outline's stage bounding box
        (stage +Y is up, so "top" is maxY). Sent explicitly to the tablet. */
    juce::Point<float> getLabelAnchor() const
    {
        const auto b = getStageBounds();
        return { b.getCentreX(), b.getBottom() };   // Rectangle<float> bottom = y + h = maxY
    }

    /** Vertex shapes: make the centroid the shape's position (keeps every
        vertex where it is on stage). Called after vertex edits. */
    void recentre()
    {
        if (! usesVertices() || vertices.empty())
            return;
        juce::Point<float> c;
        for (auto& v : vertices) c += v;
        c /= static_cast<float> (vertices.size());

        const auto newPos = localToStage (c);
        for (auto& v : vertices) v -= c;
        posX = newPos.x;
        posY = newPos.y;
    }

    /** Builds a vertex shape from stage points (scale 1, no rotation). */
    static Shape fromStagePoints (Type t, const std::vector<juce::Point<float>>& stagePts, const Style& style)
    {
        Shape s;
        s.type = t;
        s.applyStyle (style);
        juce::Point<float> c;
        for (auto& p : stagePts) c += p;
        if (! stagePts.empty())
            c /= static_cast<float> (stagePts.size());
        s.posX = c.x;
        s.posY = c.y;
        for (auto& p : stagePts)
            s.vertices.push_back (p - c);
        return s;
    }

    bool isValid() const
    {
        auto finite = [] (float v) { return std::isfinite (v) && std::abs (v) <= maxCoordinate; };
        if (! finite (posX) || ! finite (posY) || ! std::isfinite (rotation)
            || ! finite (scaleX) || ! finite (scaleY))
            return false;
        if (std::abs (scaleX) < 1.0e-6f || std::abs (scaleY) < 1.0e-6f)
            return false;
        if (usesVertices())
        {
            if (static_cast<int> (vertices.size()) < minVertexCount()
                || static_cast<int> (vertices.size()) > maxVertices)
                return false;
            for (auto& v : vertices)
                if (! finite (v.x) || ! finite (v.y))
                    return false;
        }
        return true;
    }

    //==========================================================================
    // Serialisation
    //==========================================================================

    juce::ValueTree toValueTree() const
    {
        using namespace WFSParameterIDs;
        juce::ValueTree tree (MapShape);
        tree.setProperty (msType,      static_cast<int> (type), nullptr);
        tree.setProperty (msPosX,      posX, nullptr);
        tree.setProperty (msPosY,      posY, nullptr);
        tree.setProperty (msRotation,  rotation, nullptr);
        tree.setProperty (msScaleX,    scaleX, nullptr);
        tree.setProperty (msScaleY,    scaleY, nullptr);
        if (usesVertices())
            tree.setProperty (msVertices, verticesToString (vertices), nullptr);
        tree.setProperty (msColour,    colour.toString(), nullptr);
        tree.setProperty (msLineWidth, lineWidth, nullptr);
        tree.setProperty (msDashed,    dashed ? 1 : 0, nullptr);
        if (label.isNotEmpty())
            tree.setProperty (msLabel, label, nullptr);
        return tree;
    }

    /** Tolerant read (properties are strings after a load). Out-of-range style
        values are clamped; a shape whose geometry is unusable is dropped. */
    static std::optional<Shape> fromValueTree (const juce::ValueTree& tree)
    {
        using namespace WFSParameterIDs;
        if (! tree.hasType (MapShape))
            return std::nullopt;

        Shape s;
        const int t = WFSVar::toInt (tree.getProperty (msType), 0);
        if (t < 0 || t > 3)
            return std::nullopt;
        s.type      = static_cast<Type> (t);
        s.posX      = WFSVar::toFloat (tree.getProperty (msPosX), 0.0f);
        s.posY      = WFSVar::toFloat (tree.getProperty (msPosY), 0.0f);
        s.rotation  = WFSVar::toFloat (tree.getProperty (msRotation), 0.0f);
        s.scaleX    = WFSVar::toFloat (tree.getProperty (msScaleX), 1.0f);
        s.scaleY    = WFSVar::toFloat (tree.getProperty (msScaleY), 1.0f);
        if (s.usesVertices())
            s.vertices = verticesFromString (tree.getProperty (msVertices).toString());

        const auto colourText = tree.getProperty (msColour).toString().trim();
        if (colourText.isNotEmpty())
            s.colour = juce::Colour::fromString (colourText);
        s.lineWidth = juce::jlimit (minLineWidth, maxLineWidth,
                                    WFSVar::toFloat (tree.getProperty (msLineWidth), defaultLineWidth));
        if (! std::isfinite (s.lineWidth))
            s.lineWidth = defaultLineWidth;
        s.dashed    = WFSVar::toBool (tree.getProperty (msDashed), false);
        s.label     = tree.getProperty (msLabel).toString().substring (0, maxLabelLength);

        if (! s.isValid())
            return std::nullopt;
        return s;
    }

    static juce::String verticesToString (const std::vector<juce::Point<float>>& verts)
    {
        juce::String out;
        for (size_t i = 0; i < verts.size(); ++i)
        {
            if (i > 0) out += ";";
            out += juce::String (verts[i].x, 4) + "," + juce::String (verts[i].y, 4);
        }
        return out;
    }

    static std::vector<juce::Point<float>> verticesFromString (const juce::String& text)
    {
        std::vector<juce::Point<float>> out;
        for (auto& pair : juce::StringArray::fromTokens (text, ";", ""))
        {
            auto coords = juce::StringArray::fromTokens (pair, ",", "");
            if (coords.size() >= 2)
                out.push_back ({ coords[0].getFloatValue(), coords[1].getFloatValue() });
        }
        return out;
    }
};

//==============================================================================
// List <-> ValueTree
//==============================================================================

/** Reads <MapShapes>: valid shapes in child order, at most maxShapes. */
inline std::vector<Shape> readShapes (const juce::ValueTree& mapShapesTree)
{
    std::vector<Shape> out;
    for (int i = 0; i < mapShapesTree.getNumChildren() && static_cast<int> (out.size()) < maxShapes; ++i)
        if (auto s = Shape::fromValueTree (mapShapesTree.getChild (i)))
            out.push_back (*s);
    return out;
}

/** Replace every child of a live <MapShapes> node with sanitised copies of
    `source`'s children (round-tripped through Shape, capped). An invalid
    source empties the list. */
inline void replaceShapes (juce::ValueTree liveMapShapes, const juce::ValueTree& source, juce::UndoManager* um)
{
    if (! liveMapShapes.isValid())
        return;
    liveMapShapes.removeAllChildren (um);
    if (! source.isValid())
        return;
    for (auto& s : readShapes (source))
        liveMapShapes.appendChild (s.toValueTree(), um);
}

//==============================================================================
// Conversions to / from gradient-map shapes (always in place)
//==============================================================================

/** Map → gradient. Outline style is dropped; the result is a uniform white
    fill. An open polyline is closed by a straight segment (it becomes a
    Polygon). A 2-point line encloses nothing, so it has no gradient form. */
inline std::optional<GradientMap::Shape> toGradientShape (const Shape& s)
{
    GradientMap::Shape g;
    g.posX = s.posX;
    g.posY = s.posY;
    g.rotation = s.rotation;
    g.scaleX = s.scaleX;
    g.scaleY = s.scaleY;
    g.fillType = GradientMap::FillType::Uniform;
    g.fillValue = 1.0f;
    g.blur = 0.0f;
    g.enabled = true;
    g.locked = false;
    g.name = s.label;

    switch (s.type)
    {
        case Type::Rectangle: g.type = GradientMap::ShapeType::Rectangle; break;
        case Type::Ellipse:   g.type = GradientMap::ShapeType::Ellipse;   break;
        case Type::Polygon:
        case Type::Polyline:
            if (s.vertices.size() < 3)
                return std::nullopt;
            g.type = GradientMap::ShapeType::Polygon;
            g.vertices = s.vertices;
            break;
    }
    return g;
}

/** Gradient → map. Fill and blur are dropped; the outline takes `style`. */
inline Shape fromGradientShape (const GradientMap::Shape& g, const Style& style)
{
    Shape s;
    s.posX = g.posX;
    s.posY = g.posY;
    s.rotation = g.rotation;
    s.scaleX = g.scaleX;
    s.scaleY = g.scaleY;
    s.applyStyle (style);
    s.label = g.name.substring (0, maxLabelLength);

    switch (g.type)
    {
        case GradientMap::ShapeType::Rectangle:
            // GradientMap::Shape::getPath draws a Rectangle as the unit box
            // whatever its vertex list holds ("Edit Points" retypes it to
            // Polygon before editing vertices), so the box is what it is.
            s.type = Type::Rectangle;
            break;
        case GradientMap::ShapeType::Ellipse:
            s.type = Type::Ellipse;
            break;
        case GradientMap::ShapeType::Polygon:
            s.type = Type::Polygon;
            s.vertices = g.vertices;
            break;
    }
    return s;
}

} // namespace MapDrawing
