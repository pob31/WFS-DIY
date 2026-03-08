#pragma once

#include <JuceHeader.h>
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/WFSParameterDefaults.h"

/**
 * Gradient Map Data Structures
 *
 * Runtime data model for gradient map layers and shapes.
 * Provides serialization to/from ValueTree for persistence.
 */
namespace GradientMap
{

//==============================================================================
// Enums
//==============================================================================

enum class ShapeType { Rectangle = 0, Ellipse = 1, Polygon = 2 };
enum class FillType  { Uniform = 0, LinearGradient = 1, RadialGradient = 2 };
enum class TargetParam { Attenuation = 0, Height = 1, HFShelf = 2 };

//==============================================================================
// Gradient Fill Parameters
//==============================================================================

struct LinearGradientParams
{
    float x1 = -1.0f, y1 = 0.0f, value1 = 1.0f;   // Start point (shape-local coords) + greyscale
    float x2 = 1.0f,  y2 = 0.0f, value2 = 0.0f;    // End point + greyscale
};

struct RadialGradientParams
{
    float cx = 0.0f, cy = 0.0f;       // Center (shape-local coords)
    float centerValue = 1.0f;          // Greyscale at center
    float radius = 1.0f;               // Radius in shape-local coords
    float edgeValue = 0.0f;            // Greyscale at edge
};

//==============================================================================
// Shape
//==============================================================================

struct Shape
{
    ShapeType type = ShapeType::Rectangle;
    float posX = 0.0f, posY = 0.0f;           // Center in stage coords (meters)
    float rotation = 0.0f;                      // Degrees
    float scaleX = 1.0f, scaleY = 1.0f;        // Half-extents in meters

    // Polygon vertices (in shape-local coords, before rotation/scale)
    std::vector<juce::Point<float>> vertices;

    // Fill
    FillType fillType = FillType::Uniform;
    float fillValue = 1.0f;                     // Uniform fill greyscale [0-1]
    LinearGradientParams linearGradient;
    RadialGradientParams radialGradient;

    // Edge
    float blur = 0.0f;                          // Edge blur in meters
    bool locked = false;
    bool enabled = true;
    int order = 0;                              // Z-order (painter's algorithm)

    //==========================================================================
    // Geometry
    //==========================================================================

    /** Build a JUCE Path for this shape in stage coordinates */
    juce::Path getPath() const
    {
        juce::Path path;

        switch (type)
        {
            case ShapeType::Rectangle:
                path.addRectangle (-1.0f, -1.0f, 2.0f, 2.0f);
                break;

            case ShapeType::Ellipse:
                path.addEllipse (-1.0f, -1.0f, 2.0f, 2.0f);
                break;

            case ShapeType::Polygon:
                if (vertices.size() >= 3)
                {
                    path.startNewSubPath (vertices[0]);
                    for (size_t i = 1; i < vertices.size(); ++i)
                        path.lineTo (vertices[i]);
                    path.closeSubPath();
                }
                break;
        }

        // Apply scale, rotation, translation (in that order)
        auto transform = juce::AffineTransform::scale (scaleX, scaleY)
                            .rotated (juce::degreesToRadians (rotation))
                            .translated (posX, posY);
        path.applyTransform (transform);

        return path;
    }

    //==========================================================================
    // Serialization
    //==========================================================================

    /** Serialize to ValueTree */
    juce::ValueTree toValueTree() const
    {
        using namespace WFSParameterIDs;

        juce::ValueTree tree (GradientShape);
        tree.setProperty (gmShapeType,      static_cast<int> (type), nullptr);
        tree.setProperty (gmShapePosX,      posX, nullptr);
        tree.setProperty (gmShapePosY,      posY, nullptr);
        tree.setProperty (gmShapeRotation,  rotation, nullptr);
        tree.setProperty (gmShapeScaleX,    scaleX, nullptr);
        tree.setProperty (gmShapeScaleY,    scaleY, nullptr);
        tree.setProperty (gmShapeFillType,  static_cast<int> (fillType), nullptr);
        tree.setProperty (gmShapeFillValue, fillValue, nullptr);
        tree.setProperty (gmShapeBlur,      blur, nullptr);
        tree.setProperty (gmShapeLocked,    locked ? 1 : 0, nullptr);
        tree.setProperty (gmShapeEnabled,   enabled ? 1 : 0, nullptr);
        tree.setProperty (gmShapeOrder,     order, nullptr);

        // Serialize vertices for polygons
        if (type == ShapeType::Polygon && ! vertices.empty())
        {
            juce::String vertStr;
            for (size_t i = 0; i < vertices.size(); ++i)
            {
                if (i > 0) vertStr += ";";
                vertStr += juce::String (vertices[i].x, 4) + "," + juce::String (vertices[i].y, 4);
            }
            tree.setProperty (gmShapeVertices, vertStr, nullptr);
        }

        // Serialize gradient params as string
        if (fillType == FillType::LinearGradient)
        {
            auto& lg = linearGradient;
            juce::String params = juce::String (lg.x1, 4) + "," + juce::String (lg.y1, 4) + ","
                                + juce::String (lg.value1, 4) + ","
                                + juce::String (lg.x2, 4) + "," + juce::String (lg.y2, 4) + ","
                                + juce::String (lg.value2, 4);
            tree.setProperty (gmShapeFillParams, params, nullptr);
        }
        else if (fillType == FillType::RadialGradient)
        {
            auto& rg = radialGradient;
            juce::String params = juce::String (rg.cx, 4) + "," + juce::String (rg.cy, 4) + ","
                                + juce::String (rg.centerValue, 4) + ","
                                + juce::String (rg.radius, 4) + ","
                                + juce::String (rg.edgeValue, 4);
            tree.setProperty (gmShapeFillParams, params, nullptr);
        }

        return tree;
    }

    /** Deserialize from ValueTree */
    static Shape fromValueTree (const juce::ValueTree& tree)
    {
        using namespace WFSParameterIDs;
        using namespace WFSParameterDefaults;

        Shape s;
        s.type      = static_cast<ShapeType> (static_cast<int> (tree.getProperty (gmShapeType, gmShapeTypeDefault)));
        s.posX      = tree.getProperty (gmShapePosX, gmShapePosDefault);
        s.posY      = tree.getProperty (gmShapePosY, gmShapePosDefault);
        s.rotation  = tree.getProperty (gmShapeRotation, gmShapeRotationDefault);
        s.scaleX    = tree.getProperty (gmShapeScaleX, gmShapeScaleXDefault);
        s.scaleY    = tree.getProperty (gmShapeScaleY, gmShapeScaleYDefault);
        s.fillType  = static_cast<FillType> (static_cast<int> (tree.getProperty (gmShapeFillType, gmShapeFillTypeDefault)));
        s.fillValue = tree.getProperty (gmShapeFillValue, gmShapeFillValueDefault);
        s.blur      = tree.getProperty (gmShapeBlur, gmShapeBlurDefault);
        s.locked    = static_cast<int> (tree.getProperty (gmShapeLocked, gmShapeLockedDefault)) != 0;
        s.enabled   = static_cast<int> (tree.getProperty (gmShapeEnabled, gmShapeEnabledDefault)) != 0;
        s.order     = tree.getProperty (gmShapeOrder, gmShapeOrderDefault);

        // Parse polygon vertices
        if (s.type == ShapeType::Polygon)
        {
            juce::String vertStr = tree.getProperty (gmShapeVertices, "").toString();
            if (vertStr.isNotEmpty())
            {
                auto pairs = juce::StringArray::fromTokens (vertStr, ";", "");
                for (auto& pair : pairs)
                {
                    auto coords = juce::StringArray::fromTokens (pair, ",", "");
                    if (coords.size() >= 2)
                        s.vertices.push_back ({ coords[0].getFloatValue(), coords[1].getFloatValue() });
                }
            }
        }

        // Parse gradient params
        juce::String fillParams = tree.getProperty (gmShapeFillParams, "").toString();
        if (fillParams.isNotEmpty())
        {
            auto parts = juce::StringArray::fromTokens (fillParams, ",", "");

            if (s.fillType == FillType::LinearGradient && parts.size() >= 6)
            {
                s.linearGradient.x1     = parts[0].getFloatValue();
                s.linearGradient.y1     = parts[1].getFloatValue();
                s.linearGradient.value1 = parts[2].getFloatValue();
                s.linearGradient.x2     = parts[3].getFloatValue();
                s.linearGradient.y2     = parts[4].getFloatValue();
                s.linearGradient.value2 = parts[5].getFloatValue();
            }
            else if (s.fillType == FillType::RadialGradient && parts.size() >= 5)
            {
                s.radialGradient.cx          = parts[0].getFloatValue();
                s.radialGradient.cy          = parts[1].getFloatValue();
                s.radialGradient.centerValue = parts[2].getFloatValue();
                s.radialGradient.radius      = parts[3].getFloatValue();
                s.radialGradient.edgeValue   = parts[4].getFloatValue();
            }
        }

        return s;
    }
};

//==============================================================================
// Layer
//==============================================================================

struct Layer
{
    bool enabled = false;
    TargetParam param = TargetParam::Attenuation;
    float whiteValue = 0.0f;       // Parameter value mapped to white
    float blackValue = 0.0f;       // Parameter value mapped to black
    float curve = 0.0f;            // -1 to +1 midpoint shift
    bool visible = true;           // Visibility in editor

    std::vector<Shape> shapes;

    /** Serialize to ValueTree (including shapes as children) */
    juce::ValueTree toValueTree (int layerIndex) const
    {
        using namespace WFSParameterIDs;

        juce::ValueTree tree (GradientLayer);
        tree.setProperty (id,              layerIndex, nullptr);
        tree.setProperty (gmLayerEnabled,  enabled ? 1 : 0, nullptr);
        tree.setProperty (gmLayerParam,    static_cast<int> (param), nullptr);
        tree.setProperty (gmLayerWhite,    whiteValue, nullptr);
        tree.setProperty (gmLayerBlack,    blackValue, nullptr);
        tree.setProperty (gmLayerCurve,    curve, nullptr);
        tree.setProperty (gmLayerVisible,  visible ? 1 : 0, nullptr);

        for (auto& shape : shapes)
            tree.appendChild (shape.toValueTree(), nullptr);

        return tree;
    }

    /** Deserialize from ValueTree (including shapes) */
    static Layer fromValueTree (const juce::ValueTree& tree)
    {
        using namespace WFSParameterIDs;
        using namespace WFSParameterDefaults;

        Layer l;
        l.enabled    = static_cast<int> (tree.getProperty (gmLayerEnabled, gmLayerEnabledDefault)) != 0;
        l.param      = static_cast<TargetParam> (static_cast<int> (tree.getProperty (gmLayerParam, gmLayerParamDefault)));
        l.whiteValue = tree.getProperty (gmLayerWhite, gmLayerWhiteDefault);
        l.blackValue = tree.getProperty (gmLayerBlack, gmLayerBlackDefault);
        l.curve      = tree.getProperty (gmLayerCurve, gmLayerCurveDefault);
        l.visible    = static_cast<int> (tree.getProperty (gmLayerVisible, gmLayerVisibleDefault)) != 0;

        // Load shapes
        for (int i = 0; i < tree.getNumChildren(); ++i)
        {
            auto child = tree.getChild (i);
            if (child.hasType (GradientShape))
                l.shapes.push_back (Shape::fromValueTree (child));
        }

        // Sort shapes by order for consistent rendering
        std::sort (l.shapes.begin(), l.shapes.end(),
                   [] (const Shape& a, const Shape& b) { return a.order < b.order; });

        return l;
    }
};

//==============================================================================
// Input Gradient Map (all 3 layers for one input)
//==============================================================================

struct InputGradientMap
{
    std::array<Layer, 3> layers;

    /** Load from a GradientMaps ValueTree node */
    static InputGradientMap fromValueTree (const juce::ValueTree& gmTree)
    {
        InputGradientMap map;

        for (int i = 0; i < gmTree.getNumChildren() && i < 3; ++i)
        {
            auto layerTree = gmTree.getChild (i);
            if (layerTree.hasType (WFSParameterIDs::GradientLayer))
                map.layers[static_cast<size_t> (i)] = Layer::fromValueTree (layerTree);
        }

        return map;
    }

    /** Save to a GradientMaps ValueTree node (replaces children) */
    juce::ValueTree toValueTree() const
    {
        juce::ValueTree gmTree (WFSParameterIDs::GradientMaps);

        for (int i = 0; i < 3; ++i)
            gmTree.appendChild (layers[static_cast<size_t> (i)].toValueTree (i), nullptr);

        return gmTree;
    }

    /** Check if any layer is enabled */
    bool hasAnyEnabledLayer() const
    {
        for (auto& layer : layers)
            if (layer.enabled)
                return true;
        return false;
    }
};

} // namespace GradientMap
