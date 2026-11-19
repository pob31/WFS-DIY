#pragma once

#include <JuceHeader.h>
#include <vector>
#include "MapShapeData.h"

/**
 * One in-memory clipboard shared by the Map's shape editor and the gradient
 * map editor, tagged with where its content came from.
 *
 * Paste rule:
 *  - same kind (Map → Map, gradient → gradient): offset +0.5 m, so the copy is
 *    visible next to the original (the gradient editor's long-standing rule);
 *  - across kinds (Map ↔ gradient): always IN PLACE - the two editors share
 *    the origin-relative stage frame, so the shape lands where it was drawn.
 *
 * Message thread only. Listeners are told synchronously so each editor can
 * refresh its Paste button.
 */
namespace MapDrawing
{

class ShapeClipboard
{
public:
    enum class Kind { None, GradientShapes, GradientLayer, MapShapes };

    struct Listener
    {
        virtual ~Listener() = default;
        virtual void shapeClipboardChanged() = 0;
    };

    static ShapeClipboard& get()
    {
        static ShapeClipboard instance;
        return instance;
    }

    Kind getKind() const { return kind; }
    bool isEmpty() const { return kind == Kind::None; }

    void setGradientShapes (std::vector<GradientMap::Shape> shapes)
    {
        clear();
        if (shapes.empty())
            return notify();
        gradientShapes = std::move (shapes);
        kind = Kind::GradientShapes;
        notify();
    }

    void setGradientLayer (const GradientMap::Layer& layer)
    {
        clear();
        gradientLayer = layer;
        kind = Kind::GradientLayer;
        notify();
    }

    void setMapShapes (std::vector<Shape> shapes)
    {
        clear();
        if (shapes.empty())
            return notify();
        mapShapes = std::move (shapes);
        kind = Kind::MapShapes;
        notify();
    }

    const std::vector<GradientMap::Shape>& getGradientShapes() const { return gradientShapes; }
    const GradientMap::Layer& getGradientLayer() const               { return gradientLayer; }
    const std::vector<Shape>& getMapShapes() const                   { return mapShapes; }

    /** Clipboard content as Map shapes. Gradient content (shapes, or a whole
        layer's shapes) is converted with `style`. */
    std::vector<Shape> asMapShapes (const Style& style) const
    {
        std::vector<Shape> out;
        switch (kind)
        {
            case Kind::MapShapes:
                out = mapShapes;
                break;
            case Kind::GradientShapes:
                for (auto& g : gradientShapes) out.push_back (fromGradientShape (g, style));
                break;
            case Kind::GradientLayer:
                for (auto& g : gradientLayer.shapes) out.push_back (fromGradientShape (g, style));
                break;
            case Kind::None:
                break;
        }
        return out;
    }

    /** Map shapes as gradient shapes; `skipped` counts 2-point lines, which
        enclose nothing and so have no gradient form. */
    std::vector<GradientMap::Shape> mapShapesAsGradient (int& skipped) const
    {
        skipped = 0;
        std::vector<GradientMap::Shape> out;
        for (auto& s : mapShapes)
        {
            if (auto g = toGradientShape (s))
                out.push_back (*g);
            else
                ++skipped;
        }
        return out;
    }

    void addListener (Listener* l)    { if (l != nullptr && std::find (listeners.begin(), listeners.end(), l) == listeners.end()) listeners.push_back (l); }
    void removeListener (Listener* l) { listeners.erase (std::remove (listeners.begin(), listeners.end(), l), listeners.end()); }

private:
    ShapeClipboard() = default;

    void clear()
    {
        kind = Kind::None;
        gradientShapes.clear();
        gradientLayer = {};
        mapShapes.clear();
    }

    void notify()
    {
        auto copy = listeners;   // a listener may remove itself
        for (auto* l : copy)
            if (std::find (listeners.begin(), listeners.end(), l) != listeners.end())
                l->shapeClipboardChanged();
    }

    Kind kind = Kind::None;
    std::vector<GradientMap::Shape> gradientShapes;
    GradientMap::Layer gradientLayer;
    std::vector<Shape> mapShapes;
    std::vector<Listener*> listeners;
};

} // namespace MapDrawing
