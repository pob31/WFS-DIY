#pragma once

#include <JuceHeader.h>
#include "GradientMapData.h"

/**
 * Gradient Map Evaluator
 *
 * Rasterizes vector shapes to greyscale bitmaps for O(1) position lookup.
 * Each layer is cached as a juce::Image. Re-rasterization only occurs when
 * shapes are edited, not per-frame.
 *
 * Runtime lookup: stage (x,y) → bitmap pixel → greyscale → curve → parameter offset
 */
class GradientMapEvaluator
{
public:
    //==========================================================================
    // Offset Results
    //==========================================================================

    struct Offsets
    {
        float attenuationDb = 0.0f;
        float heightMeters  = 0.0f;
        float hfShelfDb     = 0.0f;
    };

    //==========================================================================
    // Construction
    //==========================================================================

    GradientMapEvaluator() = default;

    //==========================================================================
    // Stage Bounds (must be set before rasterization)
    //==========================================================================

    void setStageBounds (float minX, float maxX, float minY, float maxY)
    {
        stageMinX = minX;
        stageMaxX = maxX;
        stageMinY = minY;
        stageMaxY = maxY;
    }

    void setPixelsPerMeter (float ppm)
    {
        pixelsPerMeter = juce::jmax (1.0f, ppm);
    }

    //==========================================================================
    // Rasterization (called on shape edit, NOT per-frame)
    //==========================================================================

    /** Rasterize a single layer's shapes to its cached bitmap */
    void rasterizeLayer (int layerIndex, const GradientMap::Layer& layer)
    {
        if (layerIndex < 0 || layerIndex >= 3)
            return;

        auto& cache = layerCaches[static_cast<size_t> (layerIndex)];
        cache.enabled    = layer.enabled;
        cache.param      = layer.param;
        cache.whiteValue = layer.whiteValue;
        cache.blackValue = layer.blackValue;
        cache.curve      = layer.curve;

        if (! layer.enabled || layer.shapes.empty())
        {
            cache.bitmap = juce::Image();
            return;
        }

        // Calculate bitmap dimensions
        float stageW = stageMaxX - stageMinX;
        float stageH = stageMaxY - stageMinY;

        if (stageW <= 0.0f || stageH <= 0.0f)
        {
            cache.bitmap = juce::Image();
            return;
        }

        int bmpW = juce::jmax (1, juce::roundToInt (stageW * pixelsPerMeter));
        int bmpH = juce::jmax (1, juce::roundToInt (stageH * pixelsPerMeter));

        // Cap bitmap size to prevent excessive memory usage
        constexpr int maxBitmapDim = 2048;
        bmpW = juce::jmin (bmpW, maxBitmapDim);
        bmpH = juce::jmin (bmpH, maxBitmapDim);

        // Create bitmap (ARGB for alpha compositing, clear to transparent black)
        cache.bitmap = juce::Image (juce::Image::ARGB, bmpW, bmpH, true);
        juce::Graphics g (cache.bitmap);

        // Transform from stage coords to bitmap coords:
        // bitmap_x = (stage_x - stageMinX) / stageW * bmpW
        // bitmap_y = (stageMaxY - stage_y) / stageH * bmpH  (Y inverted)
        float scaleXFactor = static_cast<float> (bmpW) / stageW;
        float scaleYFactor = static_cast<float> (bmpH) / stageH;

        auto stageToBitmapTransform = juce::AffineTransform::translation (-stageMinX, -stageMaxY)
                                        .scaled (scaleXFactor, -scaleYFactor);

        // Paint shapes in order (painter's algorithm)
        for (const auto& shape : layer.shapes)
        {
            if (! shape.enabled)
                continue;

            renderShapeToBitmap (g, shape, stageToBitmapTransform, bmpW, bmpH);
        }
    }

    /** Rasterize all 3 layers from an InputGradientMap */
    void rasterizeAll (const GradientMap::InputGradientMap& map)
    {
        for (int i = 0; i < 3; ++i)
            rasterizeLayer (i, map.layers[static_cast<size_t> (i)]);
    }

    //==========================================================================
    // Runtime Evaluation — O(1) bitmap lookup
    //==========================================================================

    /** Evaluate all enabled layers at stage position (x,y).
        Returns combined offsets for attenuation, height, and HF shelf. */
    Offsets evaluate (float x, float y) const
    {
        Offsets result;

        for (const auto& cache : layerCaches)
        {
            if (! cache.enabled || ! cache.bitmap.isValid())
                continue;

            float greyValue = sampleBitmap (cache, x, y);

            // Apply curve: value = pow(value, 2^(-curve))
            // curve=-1 → exp=2 (compresses toward black)
            // curve=0  → exp=1 (linear)
            // curve=+1 → exp=0.5 (compresses toward white)
            if (cache.curve != 0.0f && greyValue > 0.0f && greyValue < 1.0f)
            {
                float exponent = std::pow (2.0f, -cache.curve);
                greyValue = std::pow (greyValue, exponent);
            }

            // Map grey [0,1] → parameter offset [blackValue, whiteValue]
            float offset = cache.blackValue + greyValue * (cache.whiteValue - cache.blackValue);

            switch (cache.param)
            {
                case GradientMap::TargetParam::Attenuation:
                    result.attenuationDb += offset;
                    break;
                case GradientMap::TargetParam::Height:
                    result.heightMeters += offset;
                    break;
                case GradientMap::TargetParam::HFShelf:
                    result.hfShelfDb += offset;
                    break;
            }
        }

        return result;
    }

    /** Check if any layer has a valid bitmap */
    bool hasAnyActiveBitmap() const
    {
        for (const auto& cache : layerCaches)
            if (cache.enabled && cache.bitmap.isValid())
                return true;
        return false;
    }

    /** Get the rasterized bitmap for a layer (for debug/preview overlay) */
    const juce::Image& getLayerBitmap (int layerIndex) const
    {
        static const juce::Image emptyImage;
        if (layerIndex < 0 || layerIndex >= 3)
            return emptyImage;
        return layerCaches[static_cast<size_t> (layerIndex)].bitmap;
    }

private:
    //==========================================================================
    // Layer Cache
    //==========================================================================

    struct LayerCache
    {
        juce::Image bitmap;
        GradientMap::TargetParam param = GradientMap::TargetParam::Attenuation;
        float whiteValue = 0.0f;
        float blackValue = 0.0f;
        float curve = 0.0f;
        bool enabled = false;
    };

    std::array<LayerCache, 3> layerCaches;

    float stageMinX = -10.0f, stageMaxX = 10.0f;
    float stageMinY = -10.0f, stageMaxY = 0.0f;
    float pixelsPerMeter = WFSParameterDefaults::gmBitmapPixelsPerMeter;

    //==========================================================================
    // Bitmap Sampling
    //==========================================================================

    /** Sample a layer bitmap at stage coordinates. Returns 0-1 greyscale. */
    float sampleBitmap (const LayerCache& cache, float stageX, float stageY) const
    {
        float stageW = stageMaxX - stageMinX;
        float stageH = stageMaxY - stageMinY;

        if (stageW <= 0.0f || stageH <= 0.0f)
            return 0.0f;

        // Stage to normalised [0,1]
        float normX = (stageX - stageMinX) / stageW;
        float normY = (stageMaxY - stageY) / stageH;  // Y inverted

        // To pixel coords
        int px = juce::roundToInt (normX * static_cast<float> (cache.bitmap.getWidth() - 1));
        int py = juce::roundToInt (normY * static_cast<float> (cache.bitmap.getHeight() - 1));

        // Bounds check — outside bitmap = 0 (black / no effect)
        if (px < 0 || px >= cache.bitmap.getWidth() || py < 0 || py >= cache.bitmap.getHeight())
            return 0.0f;

        // Read pixel and convert to greyscale [0-1]
        // We use the alpha channel as the "painted" indicator and brightness as the value.
        // For shapes painted with greyscale colours on a transparent background:
        auto pixel = cache.bitmap.getPixelAt (px, py);
        float alpha = pixel.getFloatAlpha();

        if (alpha < 0.001f)
            return 0.0f;  // Transparent = no shape here = return 0 (black default)

        // Return brightness weighted by alpha for proper compositing
        return pixel.getBrightness() * alpha;
    }

    //==========================================================================
    // Shape Rendering to Bitmap
    //==========================================================================

    /** Render a single shape to the bitmap graphics context */
    void renderShapeToBitmap (juce::Graphics& g, const GradientMap::Shape& shape,
                              const juce::AffineTransform& stageToBitmap,
                              int bmpW, int bmpH) const
    {
        // Get the shape path in stage coords
        juce::Path path = shape.getPath();

        // Transform to bitmap coords
        path.applyTransform (stageToBitmap);

        // Handle edge blur
        if (shape.blur > 0.0f)
        {
            renderShapeWithBlur (g, shape, path, stageToBitmap, bmpW, bmpH);
            return;
        }

        // Apply fill
        applyFill (g, shape, path, stageToBitmap);
        g.fillPath (path);
    }

    /** Render a shape with edge blur by rasterizing to a temp image and applying gaussian blur */
    void renderShapeWithBlur (juce::Graphics& g, const GradientMap::Shape& shape,
                              const juce::Path& bitmapPath,
                              const juce::AffineTransform& stageToBitmap,
                              int bmpW, int bmpH) const
    {
        // Create temp image for this shape
        juce::Image tempImage (juce::Image::ARGB, bmpW, bmpH, true);
        {
            juce::Graphics tempG (tempImage);
            applyFill (tempG, shape, bitmapPath, stageToBitmap);
            tempG.fillPath (bitmapPath);
        }

        // Apply gaussian blur
        float blurPixels = shape.blur * pixelsPerMeter;
        int kernelSize = juce::jmax (3, juce::roundToInt (blurPixels * 2.0f) | 1);  // Odd number
        kernelSize = juce::jmin (kernelSize, 63);  // Cap kernel size

        juce::ImageConvolutionKernel kernel (kernelSize);
        kernel.createGaussianBlur (blurPixels);
        kernel.applyToImage (tempImage, tempImage, tempImage.getBounds());

        // Composite onto main bitmap
        g.drawImageAt (tempImage, 0, 0);
    }

    /** Set the appropriate fill colour/gradient on a Graphics context for a shape */
    void applyFill (juce::Graphics& g, const GradientMap::Shape& shape,
                    const juce::Path& /*bitmapPath*/,
                    const juce::AffineTransform& stageToBitmap) const
    {
        switch (shape.fillType)
        {
            case GradientMap::FillType::Uniform:
            {
                g.setColour (juce::Colour::greyLevel (shape.fillValue));
                break;
            }

            case GradientMap::FillType::LinearGradient:
            {
                auto& lg = shape.linearGradient;

                // Gradient control points are in shape-local coords → transform to stage → bitmap
                auto shapeToStage = juce::AffineTransform::scale (shape.scaleX, shape.scaleY)
                                       .rotated (juce::degreesToRadians (shape.rotation))
                                       .translated (shape.posX, shape.posY);
                auto shapeTobitmap = shapeToStage.followedBy (stageToBitmap);

                auto p1 = juce::Point<float> (lg.x1, lg.y1).transformedBy (shapeTobitmap);
                auto p2 = juce::Point<float> (lg.x2, lg.y2).transformedBy (shapeTobitmap);

                juce::ColourGradient gradient (juce::Colour::greyLevel (lg.value1), p1.x, p1.y,
                                               juce::Colour::greyLevel (lg.value2), p2.x, p2.y,
                                               false);
                g.setGradientFill (gradient);
                break;
            }

            case GradientMap::FillType::RadialGradient:
            {
                auto& rg = shape.radialGradient;

                // Transform center to bitmap coords
                auto shapeToStage = juce::AffineTransform::scale (shape.scaleX, shape.scaleY)
                                       .rotated (juce::degreesToRadians (shape.rotation))
                                       .translated (shape.posX, shape.posY);
                auto shapeToBitmap = shapeToStage.followedBy (stageToBitmap);

                auto center = juce::Point<float> (rg.cx, rg.cy).transformedBy (shapeToBitmap);

                // Transform radius: use the average scale factor
                auto edgePoint = juce::Point<float> (rg.cx + rg.radius, rg.cy).transformedBy (shapeToBitmap);
                float radiusPx = center.getDistanceFrom (edgePoint);

                juce::ColourGradient gradient (juce::Colour::greyLevel (rg.centerValue), center.x, center.y,
                                               juce::Colour::greyLevel (rg.edgeValue),
                                               center.x + radiusPx, center.y,
                                               true);
                g.setGradientFill (gradient);
                break;
            }
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GradientMapEvaluator)
};
