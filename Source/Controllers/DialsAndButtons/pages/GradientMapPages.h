#pragma once

/**
 * GradientMapPages — Stream Deck+ page definitions for the Gradient Map subtab.
 *
 * Top row: Attenuation | Height | HF Shelf | Shape (section selectors)
 * Sections 0-2: Layer controls (enable, visibility, white, black, curve)
 * Section 3: Shape controls (select, edit points, type cycle, fill cycle, fill/blur dials)
 */

#include "../StreamDeckPage.h"
#include "../../../GradientMap/GradientMapEditor.h"
#include "../../../Localization/LocalizationManager.h"

namespace GradientMapPages
{

//==============================================================================
// Callbacks struct
//==============================================================================

struct GradientMapCallbacks
{
    std::function<GradientMapEditor*()> getEditor;
};

//==============================================================================
// Layer section helper (sections 0-2)
//==============================================================================

static inline void buildLayerSection (StreamDeckSection& sec, int layerIdx,
                                       const GradientMapCallbacks& cb)
{
    static const juce::String layerNames[] = {
        LOC ("inputs.gradientMap.layers.attenuation"),
        LOC ("inputs.gradientMap.layers.height"),
        LOC ("inputs.gradientMap.layers.hfShelf")
    };
    static const juce::Colour layerColours[] = {
        juce::Colour (0xFF4A90D9),   // Blue
        juce::Colour (0xFF26A69A),   // Teal
        juce::Colour (0xFF9B6FC3)    // Purple
    };

    sec.sectionName   = layerNames[layerIdx];
    sec.sectionColour = layerColours[layerIdx];

    //------------------------------------------------------------------
    // Button 0: Enable / Disable Layer
    //------------------------------------------------------------------
    {
        auto& btn = sec.buttons[0];
        btn.label = LOC ("inputs.gradientMap.buttons.enable");
        btn.colour = juce::Colour (0xFF3A3A3A);
        btn.activeColour = layerColours[layerIdx];
        btn.type = ButtonBinding::Toggle;
        btn.requestsPageRebuild = true;

        btn.getState = [cb, layerIdx]() -> bool
        {
            auto* ed = cb.getEditor ? cb.getEditor() : nullptr;
            if (ed == nullptr) return false;
            if (layerIdx == ed->getActiveLayerIndex())
                return ed->getCurrentLayerData().enabled;
            auto tree = ed->getGradientMapsTree().getChild (layerIdx);
            return tree.isValid() && static_cast<int> (tree.getProperty (WFSParameterIDs::gmLayerEnabled, 0)) != 0;
        };

        btn.onPress = [cb, layerIdx]()
        {
            auto* ed = cb.getEditor ? cb.getEditor() : nullptr;
            if (ed == nullptr) return;
            bool current = false;
            if (layerIdx == ed->getActiveLayerIndex())
                current = ed->getCurrentLayerData().enabled;
            else
            {
                auto tree = ed->getGradientMapsTree().getChild (layerIdx);
                current = tree.isValid() && static_cast<int> (tree.getProperty (WFSParameterIDs::gmLayerEnabled, 0)) != 0;
            }
            juce::MessageManager::callAsync ([ed, layerIdx, current]() {
                ed->setLayerEnabled (layerIdx, ! current);
            });
        };
    }

    //------------------------------------------------------------------
    // Button 1: View / Hide Layer
    //------------------------------------------------------------------
    {
        auto& btn = sec.buttons[1];
        btn.label = LOC ("inputs.gradientMap.labels.visible");
        btn.colour = juce::Colour (0xFF3A3A3A);
        btn.activeColour = layerColours[layerIdx];
        btn.type = ButtonBinding::Toggle;

        btn.getState = [cb, layerIdx]() -> bool
        {
            auto* ed = cb.getEditor ? cb.getEditor() : nullptr;
            if (ed == nullptr) return false;
            if (layerIdx == ed->getActiveLayerIndex())
                return ed->getCurrentLayerData().visible;
            auto tree = ed->getGradientMapsTree().getChild (layerIdx);
            return tree.isValid() && static_cast<int> (tree.getProperty (WFSParameterIDs::gmLayerVisible, 1)) != 0;
        };

        btn.onPress = [cb, layerIdx]()
        {
            auto* ed = cb.getEditor ? cb.getEditor() : nullptr;
            if (ed == nullptr) return;
            bool current = false;
            if (layerIdx == ed->getActiveLayerIndex())
                current = ed->getCurrentLayerData().visible;
            else
            {
                auto tree = ed->getGradientMapsTree().getChild (layerIdx);
                current = ! tree.isValid() || static_cast<int> (tree.getProperty (WFSParameterIDs::gmLayerVisible, 1)) != 0;
            }
            juce::MessageManager::callAsync ([ed, layerIdx, current]() {
                ed->setLayerVisible (layerIdx, ! current);
            });
        };
    }

    //------------------------------------------------------------------
    // Dial 0: White
    //------------------------------------------------------------------
    {
        auto bounds = GradientMapEditor::getLayerValueBounds (layerIdx);
        auto& d = sec.dials[0];
        d.paramName = LOC ("inputs.gradientMap.labels.white");
        d.paramUnit = (layerIdx == 1) ? "m" : "dB";
        d.minValue = bounds.min;
        d.maxValue = bounds.max;
        d.step = (layerIdx == 1) ? 0.5f : 1.0f;
        d.fineStep = (layerIdx == 1) ? 0.1f : 0.1f;
        d.decimalPlaces = 2;
        d.barColour = layerColours[layerIdx];

        d.getValue = [cb, layerIdx]() -> float
        {
            auto* ed = cb.getEditor ? cb.getEditor() : nullptr;
            if (ed == nullptr || ed->getActiveLayerIndex() != layerIdx) return 0.0f;
            return ed->getCurrentLayerData().whiteValue;
        };

        d.setValue = [cb, layerIdx] (float v)
        {
            auto* ed = cb.getEditor ? cb.getEditor() : nullptr;
            if (ed == nullptr) return;
            if (ed->getActiveLayerIndex() != layerIdx)
                ed->switchToLayer (layerIdx);
            juce::MessageManager::callAsync ([ed, v]() { ed->setLayerWhite (v); });
        };
    }

    //------------------------------------------------------------------
    // Dial 1: Black
    //------------------------------------------------------------------
    {
        auto bounds = GradientMapEditor::getLayerValueBounds (layerIdx);
        auto& d = sec.dials[1];
        d.paramName = LOC ("inputs.gradientMap.labels.black");
        d.paramUnit = (layerIdx == 1) ? "m" : "dB";
        d.minValue = bounds.min;
        d.maxValue = bounds.max;
        d.step = (layerIdx == 1) ? 0.5f : 1.0f;
        d.fineStep = (layerIdx == 1) ? 0.1f : 0.1f;
        d.decimalPlaces = 2;
        d.barColour = layerColours[layerIdx];

        d.getValue = [cb, layerIdx]() -> float
        {
            auto* ed = cb.getEditor ? cb.getEditor() : nullptr;
            if (ed == nullptr || ed->getActiveLayerIndex() != layerIdx) return 0.0f;
            return ed->getCurrentLayerData().blackValue;
        };

        d.setValue = [cb, layerIdx] (float v)
        {
            auto* ed = cb.getEditor ? cb.getEditor() : nullptr;
            if (ed == nullptr) return;
            if (ed->getActiveLayerIndex() != layerIdx)
                ed->switchToLayer (layerIdx);
            juce::MessageManager::callAsync ([ed, v]() { ed->setLayerBlack (v); });
        };
    }

    //------------------------------------------------------------------
    // Dial 2: Curve
    //------------------------------------------------------------------
    {
        auto& d = sec.dials[2];
        d.paramName = LOC ("inputs.gradientMap.labels.curve");
        d.minValue = -1.0f;
        d.maxValue = 1.0f;
        d.step = 0.05f;
        d.fineStep = 0.01f;
        d.decimalPlaces = 2;
        d.barColour = layerColours[layerIdx];

        d.getValue = [cb, layerIdx]() -> float
        {
            auto* ed = cb.getEditor ? cb.getEditor() : nullptr;
            if (ed == nullptr || ed->getActiveLayerIndex() != layerIdx) return 0.0f;
            return ed->getCurrentLayerData().curve;
        };

        d.setValue = [cb, layerIdx] (float v)
        {
            auto* ed = cb.getEditor ? cb.getEditor() : nullptr;
            if (ed == nullptr) return;
            if (ed->getActiveLayerIndex() != layerIdx)
                ed->switchToLayer (layerIdx);
            juce::MessageManager::callAsync ([ed, v]() { ed->setLayerCurve (v); });
        };
    }
}

//==============================================================================
// Shape section (section 3)
//==============================================================================

static inline void buildShapeSection (StreamDeckSection& sec,
                                       const GradientMapCallbacks& cb)
{
    sec.sectionName   = LOC ("inputs.gradientMap.labels.shape");
    sec.sectionColour = juce::Colour (0xFFD4A843);  // Gold

    const auto grey = juce::Colour (0xFF3A3A3A);

    //------------------------------------------------------------------
    // Button 0: Cycle Selection (next shape)
    //------------------------------------------------------------------
    {
        auto& btn = sec.buttons[0];
        btn.label = LOC ("inputs.gradientMap.tools.select");
        btn.colour = grey;
        btn.activeColour = juce::Colour (0xFFD4A843);
        btn.type = ButtonBinding::Toggle;
        btn.requestsPageRebuild = true;

        btn.getState = [cb]() -> bool
        {
            auto* ed = cb.getEditor ? cb.getEditor() : nullptr;
            return ed && ed->getCurrentTool() == GradientMapEditor::Tool::Select;
        };

        btn.onPress = [cb]()
        {
            auto* ed = cb.getEditor ? cb.getEditor() : nullptr;
            if (ed)
            {
                if (ed->getCurrentTool() == GradientMapEditor::Tool::Select)
                    juce::MessageManager::callAsync ([ed]() { ed->cycleShapeSelection(); });
                else
                    juce::MessageManager::callAsync ([ed]() { ed->setCurrentTool (GradientMapEditor::Tool::Select); });
            }
        };

        btn.getDynamicLabel = [cb]() -> juce::String
        {
            auto* ed = cb.getEditor ? cb.getEditor() : nullptr;
            if (ed == nullptr) return LOC ("inputs.gradientMap.tools.select");
            auto& sel = ed->getSelectedShapeIndices();
            if (sel.empty())
                return LOC ("inputs.gradientMap.tools.select");
            return LOC ("inputs.gradientMap.tools.select") + "\n"
                   + juce::String (sel[0] + 1) + "/" + juce::String (static_cast<int> (ed->getCurrentLayerData().shapes.size()));
        };
    }

    //------------------------------------------------------------------
    // Button 1: Toggle Edit Points
    //------------------------------------------------------------------
    {
        auto& btn = sec.buttons[1];
        btn.label = LOC ("inputs.gradientMap.tools.editPoints");
        btn.colour = grey;
        btn.activeColour = juce::Colour (0xFFD4A843);
        btn.type = ButtonBinding::Toggle;

        btn.getState = [cb]() -> bool
        {
            auto* ed = cb.getEditor ? cb.getEditor() : nullptr;
            return ed && ed->isEditVerticesMode();
        };

        btn.onPress = [cb]()
        {
            auto* ed = cb.getEditor ? cb.getEditor() : nullptr;
            if (ed)
            {
                bool newState = ! ed->isEditVerticesMode();
                juce::MessageManager::callAsync ([ed, newState]() { ed->setEditVerticesMode (newState); });
            }
        };
    }

    //------------------------------------------------------------------
    // Dials: depend on selected shape's fill type
    //------------------------------------------------------------------
    auto* ed = cb.getEditor ? cb.getEditor() : nullptr;
    const auto* shape = ed ? ed->getSelectedShape() : nullptr;
    auto fillType = shape ? shape->fillType : GradientMap::FillType::Uniform;
    bool hasGradient = (fillType != GradientMap::FillType::Uniform);

    if (! hasGradient)
    {
        // Dial 0: Fill value (uniform)
        auto& d = sec.dials[0];
        d.paramName = LOC ("inputs.gradientMap.labels.fill");
        d.minValue = 0.0f;
        d.maxValue = 1.0f;
        d.step = 0.02f;
        d.fineStep = 0.005f;
        d.decimalPlaces = 2;
        d.barColour = juce::Colour (0xFFD4A843);

        d.getValue = [cb]() -> float
        {
            auto* e = cb.getEditor ? cb.getEditor() : nullptr;
            const auto* s = e ? e->getSelectedShape() : nullptr;
            return s ? s->fillValue : 0.0f;
        };

        d.setValue = [cb] (float v)
        {
            auto* e = cb.getEditor ? cb.getEditor() : nullptr;
            if (e) juce::MessageManager::callAsync ([e, v]() { e->setShapeFillValue (v); });
        };
    }
    else
    {
        // Dial 1: Gradient Start
        {
            auto& d = sec.dials[1];
            d.paramName = LOC ("inputs.gradientMap.labels.start");
            d.minValue = 0.0f;
            d.maxValue = 1.0f;
            d.step = 0.02f;
            d.fineStep = 0.005f;
            d.decimalPlaces = 2;
            d.barColour = juce::Colour (0xFFD4A843);

            d.getValue = [cb]() -> float
            {
                auto* e = cb.getEditor ? cb.getEditor() : nullptr;
                const auto* s = e ? e->getSelectedShape() : nullptr;
                if (s == nullptr) return 0.0f;
                if (s->fillType == GradientMap::FillType::LinearGradient) return s->linearGradient.value1;
                if (s->fillType == GradientMap::FillType::RadialGradient) return s->radialGradient.centerValue;
                return 0.0f;
            };

            d.setValue = [cb] (float v)
            {
                auto* e = cb.getEditor ? cb.getEditor() : nullptr;
                if (e) juce::MessageManager::callAsync ([e, v]() { e->setGradientStart (v); });
            };
        }

        // Dial 2: Gradient End
        {
            auto& d = sec.dials[2];
            d.paramName = LOC ("inputs.gradientMap.labels.end");
            d.minValue = 0.0f;
            d.maxValue = 1.0f;
            d.step = 0.02f;
            d.fineStep = 0.005f;
            d.decimalPlaces = 2;
            d.barColour = juce::Colour (0xFFD4A843);

            d.getValue = [cb]() -> float
            {
                auto* e = cb.getEditor ? cb.getEditor() : nullptr;
                const auto* s = e ? e->getSelectedShape() : nullptr;
                if (s == nullptr) return 0.0f;
                if (s->fillType == GradientMap::FillType::LinearGradient) return s->linearGradient.value2;
                if (s->fillType == GradientMap::FillType::RadialGradient) return s->radialGradient.edgeValue;
                return 0.0f;
            };

            d.setValue = [cb] (float v)
            {
                auto* e = cb.getEditor ? cb.getEditor() : nullptr;
                if (e) juce::MessageManager::callAsync ([e, v]() { e->setGradientEnd (v); });
            };
        }
    }

    // Dial 3: Blur (always present)
    {
        auto& d = sec.dials[3];
        d.paramName = LOC ("inputs.gradientMap.labels.blur");
        d.paramUnit = "m";
        d.minValue = 0.0f;
        d.maxValue = 2.0f;
        d.step = 0.05f;
        d.fineStep = 0.01f;
        d.decimalPlaces = 2;
        d.barColour = juce::Colour (0xFFD4A843);

        d.getValue = [cb]() -> float
        {
            auto* e = cb.getEditor ? cb.getEditor() : nullptr;
            const auto* s = e ? e->getSelectedShape() : nullptr;
            return s ? s->blur : 0.0f;
        };

        d.setValue = [cb] (float v)
        {
            auto* e = cb.getEditor ? cb.getEditor() : nullptr;
            if (e) juce::MessageManager::callAsync ([e, v]() { e->setShapeBlur (v); });
        };
    }
}

//==============================================================================
// Page creation
//==============================================================================

inline StreamDeckPage createGradientMapPage (const GradientMapCallbacks& cb)
{
    StreamDeckPage page ("Gradient Map");

    // Build 3 layer sections + 1 shape section
    for (int i = 0; i < 3; ++i)
        buildLayerSection (page.sections[i], i, cb);

    buildShapeSection (page.sections[3], cb);
    page.numSections = 4;

    // Set active section based on editor's active layer
    auto* ed = cb.getEditor ? cb.getEditor() : nullptr;
    if (ed)
        page.activeSectionIndex = ed->getActiveLayerIndex();

    // Bidirectional: when StreamDeck section changes, switch editor layer
    page.onSectionChanged = [cb] (int sectionIndex)
    {
        if (sectionIndex >= 0 && sectionIndex < 3)
        {
            auto* editor = cb.getEditor ? cb.getEditor() : nullptr;
            if (editor && editor->getActiveLayerIndex() != sectionIndex)
                juce::MessageManager::callAsync ([editor, sectionIndex]() { editor->switchToLayer (sectionIndex); });
        }
    };

    return page;
}

} // namespace GradientMapPages
