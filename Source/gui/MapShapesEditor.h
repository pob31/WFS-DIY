#pragma once

#include <JuceHeader.h>
#include <algorithm>
#include <functional>
#include <optional>
#include <vector>
#include "../MapShapes/MapShapeData.h"
#include "../MapShapes/ShapeClipboard.h"
#include "../Parameters/WFSValueTreeState.h"
#include "../Localization/LocalizationManager.h"
#include "ColorScheme.h"
#include "WfsLookAndFeel.h"

/**
 * The Map tab's shape layer: draws the outline-only visual aids stored under
 * <Config><MapShapes> on every paint, and - while the Map is in shape-edit
 * mode - turns pointer and key events into shape edits.
 *
 * A controller, not a Component: MapTab owns the pixels and the events and
 * forwards them here, so the shapes share the Map's view transform exactly.
 *
 * Edits run on an in-memory copy for the length of a gesture and are written
 * to the tree once, on release, as one transaction in the MAP undo domain
 * (by the Map's own UndoManager, not the active one: the Map can be detached
 * beside any tab). The tree is the truth; the cached list is re-read whenever
 * MapTab reports a change under <MapShapes>.
 */
class MapShapesEditor
{
public:
    using Shape = MapDrawing::Shape;
    using Type  = MapDrawing::Type;
    using Style = MapDrawing::Style;

    enum class Tool { Select, EditPoints, Line, Polygon, Rectangle, Ellipse, Freehand };

    //==========================================================================
    // Wiring (set by MapTab)
    //==========================================================================

    std::function<juce::Point<float> (juce::Point<float>)> toScreen;   // stage → component
    std::function<juce::Point<float> (juce::Point<float>)> toStage;    // component → stage
    std::function<float()> getViewScale;                                // px per metre
    std::function<void()> requestRepaint;
    std::function<void()> onSelectionChanged;                           // panel refresh
    std::function<void (const juce::String&)> showMessage;              // status bar

    MapShapesEditor (WFSValueTreeState& state, std::function<juce::ValueTree()> configProvider)
        : vts (state), getConfig (std::move (configProvider))
    {
    }

    //==========================================================================
    // Model
    //==========================================================================

    /** MapTab calls this for any change under <MapShapes>. Our own commits
        are ignored (the cache already holds what was written). A change from
        elsewhere (undo, a load, ...) mid-gesture cancels the gesture. */
    void markModelDirty()
    {
        if (isWriting)
            return;
        if (gesture != Gesture::None)
            cancelGesture (false);
        modelDirty = true;
        if (requestRepaint) requestRepaint();
    }

    static bool isShapeTree (const juce::ValueTree& t)
    {
        return t.hasType (WFSParameterIDs::MapShapes) || t.hasType (WFSParameterIDs::MapShape);
    }

    const std::vector<Shape>& getShapes()
    {
        ensureModel();
        return shapes;
    }

    //==========================================================================
    // Mode / tool
    //==========================================================================

    void setActive (bool shouldBeActive)
    {
        if (active == shouldBeActive)
            return;
        if (! shouldBeActive)
        {
            finishBuilding (true);
            cancelGesture (true);
            clearSelection();
        }
        active = shouldBeActive;
        hoverScreen.reset();
        if (requestRepaint) requestRepaint();
    }

    bool isActive() const { return active; }

    void setTool (Tool t)
    {
        if (tool == t)
            return;
        finishBuilding (true);
        cancelGesture (true);
        tool = t;
        if (requestRepaint) requestRepaint();
        notifySelection();
    }

    Tool getTool() const { return tool; }

    //==========================================================================
    // Style
    //==========================================================================

    bool hasSelection() const { return ! selected.empty(); }

    /** The style the panel shows: the first selected shape's, else the style
        for new shapes. */
    Style getDisplayedStyle()
    {
        ensureModel();
        if (! selected.empty() && isIndexValid (selected.front()))
            return shapes[(size_t) selected.front()].getStyle();
        return currentStyle;
    }

    juce::String getDisplayedLabel()
    {
        ensureModel();
        if (! selected.empty() && isIndexValid (selected.front()))
            return shapes[(size_t) selected.front()].label;
        return {};
    }

    /** `newTransaction` false lets a colour-wheel drag collapse into one undo
        step (beginStyleGesture opened it). */
    void setColour (juce::Colour c, bool newTransaction = true)
    {
        currentStyle.colour = c;
        applyToSelection ([c] (Shape& s) { s.colour = c; }, "Map Shape: Colour", newTransaction);
    }

    void beginStyleGesture()
    {
        if (auto* um = undoManager())
            um->beginNewTransaction ("Map Shape: Colour");
    }

    void setLineWidth (float w)
    {
        w = juce::jlimit (MapDrawing::minLineWidth, MapDrawing::maxLineWidth, w);
        currentStyle.lineWidth = w;
        applyToSelection ([w] (Shape& s) { s.lineWidth = w; }, "Map Shape: Line Width", true);
    }

    void setDashed (bool d)
    {
        currentStyle.dashed = d;
        applyToSelection ([d] (Shape& s) { s.dashed = d; }, "Map Shape: Dashed", true);
    }

    void setLabel (const juce::String& text)
    {
        const auto t = text.substring (0, MapDrawing::maxLabelLength);
        applyToSelection ([t] (Shape& s) { s.label = t; }, "Map Shape: Label", true);
    }

    //==========================================================================
    // Actions
    //==========================================================================

    void copySelection()
    {
        ensureModel();
        std::vector<Shape> out;
        for (int i : selected)
            if (isIndexValid (i))
                out.push_back (shapes[(size_t) i]);
        if (out.empty())
            return;
        MapDrawing::ShapeClipboard::get().setMapShapes (std::move (out));
        message (LOC ("map.shapes.messages.copied").replace ("{count}", juce::String ((int) selected.size())));
    }

    bool canPaste() const { return ! MapDrawing::ShapeClipboard::get().isEmpty(); }

    void paste()
    {
        using Kind = MapDrawing::ShapeClipboard::Kind;
        auto& clip = MapDrawing::ShapeClipboard::get();
        if (clip.isEmpty())
            return;

        // Map → Map: offset so the copy is visible. Gradient → Map: in place.
        const float offset = clip.getKind() == Kind::MapShapes ? 0.5f : 0.0f;
        auto incoming = clip.asMapShapes (currentStyle);
        for (auto& s : incoming)
        {
            s.posX += offset;
            s.posY += offset;
        }
        addShapes (std::move (incoming), "Map Shape: Paste");
    }

    void deleteSelection()
    {
        ensureModel();
        if (selected.empty())
            return;

        auto tree = getShapesTree (true);
        if (! tree.isValid())
            return;

        auto* um = undoManager();
        if (um != nullptr) um->beginNewTransaction ("Map Shape: Delete");

        auto children = selectedChildIndices();
        std::sort (children.rbegin(), children.rend());
        {
            const juce::ScopedValueSetter<bool> writing (isWriting, true);
            for (int c : children)
                tree.removeChild (c, um);
        }
        selected.clear();
        reloadModel();
        notifySelection();
    }

    void selectAll()
    {
        ensureModel();
        selected.clear();
        for (int i = 0; i < (int) shapes.size(); ++i)
            selected.push_back (i);
        notifySelection();
    }

    bool undo()
    {
        auto* um = undoManager();
        if (um == nullptr || ! um->canUndo())
            return false;
        cancelGesture (true);
        um->undo();
        return true;
    }

    bool redo()
    {
        auto* um = undoManager();
        if (um == nullptr || ! um->canRedo())
            return false;
        cancelGesture (true);
        um->redo();
        return true;
    }

    //==========================================================================
    // Painting
    //==========================================================================

    /** Every shape, in list order. Always drawn, in and out of edit mode. */
    void paintShapes (juce::Graphics& g)
    {
        ensureModel();
        for (auto& s : shapes)
            paintShape (g, s);
    }

    /** Selection, handles and in-progress drawing. Edit mode only. */
    void paintEditOverlay (juce::Graphics& g)
    {
        if (! active)
            return;
        ensureModel();

        const auto accent = ColorScheme::get().accentBlue;

        // Selection outlines
        for (int i : selected)
        {
            if (! isIndexValid (i)) continue;
            auto path = screenPath (shapes[(size_t) i]);
            juce::Path dashed;
            const float dash[] = { 4.0f, 3.0f };
            juce::PathStrokeType (1.0f).createDashedStroke (dashed, path, dash, 2);
            g.setColour (accent.withAlpha (0.9f));
            g.fillPath (dashed);
        }

        if (selected.size() == 1 && isIndexValid (selected.front()))
        {
            const auto& s = shapes[(size_t) selected.front()];
            if (tool == Tool::Select)
            {
                juce::Point<float> corners[4];
                getCornerScreen (s, corners);
                juce::Path box;
                box.startNewSubPath (corners[0]);
                for (int k = 1; k < 4; ++k) box.lineTo (corners[k]);
                box.closeSubPath();
                g.setColour (accent.withAlpha (0.35f));
                g.strokePath (box, juce::PathStrokeType (1.0f));

                const float hs = handleDrawSize();
                g.setColour (accent);
                for (auto& c : corners)
                    g.fillRect (c.x - hs * 0.5f, c.y - hs * 0.5f, hs, hs);

                juce::Point<float> rotPos, topPos;
                if (getRotationHandle (s, rotPos, topPos))
                {
                    g.drawLine ({ topPos, rotPos }, 1.0f);
                    g.drawEllipse (rotPos.x - hs * 0.7f, rotPos.y - hs * 0.7f, hs * 1.4f, hs * 1.4f, 1.5f);
                }
            }
            else if (tool == Tool::EditPoints && (s.usesVertices() || s.type == Type::Rectangle))
            {
                for (auto& p : vertexScreenPoints (s))
                {
                    const float r = handleDrawSize() * 0.6f;
                    g.setColour (accent);
                    g.fillEllipse (p.x - r, p.y - r, r * 2.0f, r * 2.0f);
                    g.setColour (juce::Colours::white);
                    g.drawEllipse (p.x - r, p.y - r, r * 2.0f, r * 2.0f, 1.0f);
                }
            }
        }

        // Line / polygon under construction
        if (! building.empty())
        {
            juce::Path p;
            p.startNewSubPath (toScreen (building.front()));
            for (size_t k = 1; k < building.size(); ++k)
                p.lineTo (toScreen (building[k]));
            if (hoverScreen.has_value())
                p.lineTo (*hoverScreen);
            strokeStyled (g, p, currentStyle);

            g.setColour (accent);
            for (size_t k = 0; k < building.size(); ++k)
            {
                auto sp = toScreen (building[k]);
                const float r = (k == 0 && tool == Tool::Polygon && building.size() >= 3) ? 6.0f : 3.5f;
                g.drawEllipse (sp.x - r, sp.y - r, r * 2.0f, r * 2.0f, 1.5f);
            }
        }

        // Rectangle / ellipse being dragged out
        if (gesture == Gesture::CreateBox)
        {
            auto r = createBoxScreenRect();
            juce::Path p;
            if (tool == Tool::Ellipse) p.addEllipse (r);
            else                       p.addRectangle (r);
            strokeStyled (g, p, currentStyle);
        }

        // Freehand trail
        if (gesture == Gesture::Freehand && freehandScreen.size() >= 2)
        {
            juce::Path p;
            p.startNewSubPath (freehandScreen.front());
            for (size_t k = 1; k < freehandScreen.size(); ++k)
                p.lineTo (freehandScreen[k]);
            strokeStyled (g, p, currentStyle);
        }

        // Rubber band
        if (gesture == Gesture::Rubber)
        {
            auto r = juce::Rectangle<float> (gestureStartScreen, currentScreen);
            g.setColour (accent.withAlpha (0.12f));
            g.fillRect (r);
            g.setColour (accent.withAlpha (0.7f));
            g.drawRect (r, 1.0f);
        }
    }

    //==========================================================================
    // Pointer events (MapTab forwards these only in edit mode)
    //==========================================================================

    bool hasPointer() const            { return ownerSource >= 0; }
    bool ownsPointer (int source) const { return ownerSource >= 0 && ownerSource == source; }

    /** A second finger landed: give the first one back to the view gesture.
        Undoes whatever the press had done (restores the shapes, drops the
        vertex the press added). Returns the finger and where it is now. */
    bool yieldPointer (int& sourceOut, juce::Point<float>& posOut)
    {
        if (ownerSource < 0)
            return false;
        sourceOut = ownerSource;
        posOut = currentScreen;
        if (pressAddedVertex && ! building.empty())
            building.pop_back();
        pressAddedVertex = false;
        cancelGesture (true);
        ownerSource = -1;
        if (requestRepaint) requestRepaint();
        return true;
    }

    void pointerDown (int source, juce::Point<float> pos, juce::ModifierKeys mods, bool isTouch)
    {
        ensureModel();
        ownerSource = source;
        touchPointer = isTouch;
        gestureStartScreen = currentScreen = pos;
        gestureStartStage = toStage (pos);
        travelled = false;
        pressAddedVertex = false;
        if (isTouch)
            hoverScreen.reset();

        switch (tool)
        {
            case Tool::Select:      selectDown (pos, mods); break;
            case Tool::EditPoints:  editPointsDown (pos, mods); break;
            case Tool::Line:
            case Tool::Polygon:     polyDown (pos); break;
            case Tool::Rectangle:
            case Tool::Ellipse:     gesture = Gesture::CreateBox; break;
            case Tool::Freehand:
                gesture = Gesture::Freehand;
                freehandScreen = { pos };
                break;
        }
        if (requestRepaint) requestRepaint();
    }

    void pointerDrag (int source, juce::Point<float> pos, juce::ModifierKeys mods)
    {
        if (! ownsPointer (source))
            return;
        currentScreen = pos;
        if (pos.getDistanceFrom (gestureStartScreen) > (touchPointer ? 8.0f : 3.0f))
            travelled = true;

        const auto stagePos = toStage (pos);
        switch (gesture)
        {
            case Gesture::Move:
            {
                if (! travelled) break;
                const auto d = stagePos - gestureStartStage;
                for (size_t k = 0; k < gestureIndices.size(); ++k)
                {
                    auto& s = shapes[(size_t) gestureIndices[k]];
                    s.posX = gestureStartShapes[k].posX + d.x;
                    s.posY = gestureStartShapes[k].posY + d.y;
                }
                break;
            }
            case Gesture::Scale:   applyScale (stagePos, mods.isShiftDown()); break;
            case Gesture::Rotate:  applyRotate (stagePos, mods.isShiftDown()); break;
            case Gesture::Vertex:
            {
                if (! travelled) break;
                auto& s = shapes[(size_t) gestureIndices.front()];
                if (vertexIndex >= 0 && vertexIndex < (int) s.vertices.size())
                    s.vertices[(size_t) vertexIndex] = s.stageToLocal (stagePos);
                break;
            }
            case Gesture::Freehand:
                if (freehandScreen.empty() || pos.getDistanceFrom (freehandScreen.back()) >= 2.0f)
                    freehandScreen.push_back (pos);
                break;
            case Gesture::CreateBox:
            case Gesture::Rubber:
            case Gesture::None:
                break;
        }
        if (requestRepaint) requestRepaint();
    }

    void pointerUp (int source, juce::Point<float> pos, juce::ModifierKeys mods)
    {
        if (! ownsPointer (source))
            return;
        currentScreen = pos;
        ownerSource = -1;

        switch (gesture)
        {
            case Gesture::Move:
                if (travelled) commitModified (gestureIndices, "Map Shape: Move");
                break;
            case Gesture::Scale:
                if (travelled) commitModified (gestureIndices, "Map Shape: Resize");
                break;
            case Gesture::Rotate:
                if (travelled) commitModified (gestureIndices, "Map Shape: Rotate");
                break;
            case Gesture::Vertex:
                if (travelled)
                {
                    shapes[(size_t) gestureIndices.front()].recentre();
                    commitModified (gestureIndices, "Map Shape: Edit Point");
                }
                else if (convertedRectangle)
                {
                    // A press on a rectangle corner that did not move: no edit.
                    shapes[(size_t) gestureIndices.front()] = gestureStartShapes.front();
                }
                break;
            case Gesture::Rubber:
                finishRubber (mods.isShiftDown());
                break;
            case Gesture::CreateBox:
                finishCreateBox (mods.isShiftDown());
                break;
            case Gesture::Freehand:
                finishFreehand();
                break;
            case Gesture::None:
                break;
        }

        gesture = Gesture::None;
        gestureIndices.clear();
        gestureStartShapes.clear();
        convertedRectangle = false;
        pressAddedVertex = false;
        if (requestRepaint) requestRepaint();
    }

    void pointerMove (juce::Point<float> pos)
    {
        if (building.empty())
        {
            hoverScreen.reset();
            return;
        }
        hoverScreen = pos;
        if (requestRepaint) requestRepaint();
    }

    void pointerExit()
    {
        if (hoverScreen.has_value())
        {
            hoverScreen.reset();
            if (requestRepaint) requestRepaint();
        }
    }

    void doubleClick (juce::Point<float> pos)
    {
        ensureModel();

        if (tool == Tool::Line || tool == Tool::Polygon)
        {
            // The double-click's second press added a vertex on top of the
            // first one: drop it before finishing.
            if (building.size() >= 2
                && toScreen (building.back()).getDistanceFrom (toScreen (building[building.size() - 2])) < 6.0f)
                building.pop_back();
            finishBuilding (true);
            return;
        }

        if (tool == Tool::EditPoints && selected.size() == 1 && isIndexValid (selected.front()))
        {
            const int idx = selected.front();
            Shape s = shapes[(size_t) idx];
            if (! s.usesVertices() && s.type != Type::Rectangle)
                return;
            if (s.type == Type::Rectangle)
                convertRectangleToPolygon (s);

            const auto pts = vertexScreenPoints (s);
            const float r = handleHitRadius();

            for (int k = 0; k < (int) pts.size(); ++k)
            {
                if (pts[(size_t) k].getDistanceFrom (pos) < r)
                {
                    if ((int) s.vertices.size() > s.minVertexCount())
                    {
                        s.vertices.erase (s.vertices.begin() + k);
                        s.recentre();
                        shapes[(size_t) idx] = s;
                        commitModified ({ idx }, "Map Shape: Delete Point");
                    }
                    return;
                }
            }

            const int n = (int) pts.size();
            const int segCount = s.isClosed() ? n : n - 1;
            for (int k = 0; k < segCount; ++k)
            {
                const auto a = pts[(size_t) k];
                const auto b = pts[(size_t) ((k + 1) % n)];
                if (MapDrawing::distanceToSegment (pos, a, b) < r
                    && (int) s.vertices.size() < MapDrawing::maxVertices)
                {
                    s.vertices.insert (s.vertices.begin() + k + 1, s.stageToLocal (toStage (pos)));
                    s.recentre();
                    shapes[(size_t) idx] = s;
                    commitModified ({ idx }, "Map Shape: Add Point");
                    return;
                }
            }
        }
    }

    bool keyPressed (const juce::KeyPress& key)
    {
        if (! active)
            return false;

        const auto cmd = juce::ModifierKeys::commandModifier;
        const int kc = key.getKeyCode();

        if (kc == juce::KeyPress::escapeKey)
        {
            if (! building.empty())                { building.clear(); hoverScreen.reset(); }
            else if (gesture != Gesture::None)     { cancelGesture (true); ownerSource = -1; }
            else if (! selected.empty())           clearSelection();
            else                                   return false;
            if (requestRepaint) requestRepaint();
            return true;
        }
        if (kc == juce::KeyPress::returnKey && ! building.empty())
        {
            finishBuilding (true);
            return true;
        }
        if (kc == juce::KeyPress::backspaceKey && ! building.empty())
        {
            building.pop_back();
            if (requestRepaint) requestRepaint();
            return true;
        }
        if ((kc == juce::KeyPress::deleteKey || kc == juce::KeyPress::backspaceKey) && ! selected.empty())
        {
            deleteSelection();
            return true;
        }
        if (key == juce::KeyPress ('c', cmd, 0)) { copySelection(); return true; }
        if (key == juce::KeyPress ('v', cmd, 0)) { paste(); return true; }
        if (key == juce::KeyPress ('a', cmd, 0)) { selectAll(); return true; }
        if (key == juce::KeyPress ('z', cmd, 0)) { undo(); return true; }
        if (key == juce::KeyPress ('z', cmd | juce::ModifierKeys::shiftModifier, 0)
            || key == juce::KeyPress ('y', cmd, 0))
        {
            redo();
            return true;
        }

        if (! selected.empty() && gesture == Gesture::None)
        {
            const float step = key.getModifiers().isShiftDown() ? 0.01f : 0.1f;
            float dx = 0.0f, dy = 0.0f;
            if      (kc == juce::KeyPress::leftKey)  dx = -step;
            else if (kc == juce::KeyPress::rightKey) dx =  step;
            else if (kc == juce::KeyPress::upKey)    dy =  step;
            else if (kc == juce::KeyPress::downKey)  dy = -step;

            if (dx != 0.0f || dy != 0.0f)
            {
                ensureModel();
                for (int i : selected)
                    if (isIndexValid (i))
                    {
                        shapes[(size_t) i].posX += dx;
                        shapes[(size_t) i].posY += dy;
                    }
                commitModified (selected, "Map Shape: Nudge");
                return true;
            }
        }
        return false;
    }

private:
    enum class Gesture { None, Move, Scale, Rotate, Vertex, Rubber, CreateBox, Freehand };

    WFSValueTreeState& vts;
    std::function<juce::ValueTree()> getConfig;

    std::vector<Shape> shapes;          // cached model (+ live edits during a gesture)
    std::vector<int> childIndex;        // shapes[i] ↔ <MapShapes> child index
    bool modelDirty = true;
    bool isWriting = false;

    bool active = false;
    Tool tool = Tool::Select;
    Style currentStyle { juce::Colour (0xFFB0B0B0), MapDrawing::defaultLineWidth, false };
    std::vector<int> selected;

    // Gesture state
    Gesture gesture = Gesture::None;
    int ownerSource = -1;
    bool touchPointer = false;
    bool travelled = false;
    juce::Point<float> gestureStartScreen, currentScreen, gestureStartStage;
    std::vector<int> gestureIndices;
    std::vector<Shape> gestureStartShapes;
    int scaleCorner = -1;
    juce::Point<float> scaleAnchor, scaleCornerStart;   // pre-rotation metres, relative to pos
    float rotateStartAngle = 0.0f;
    int vertexIndex = -1;
    bool convertedRectangle = false;

    // Drawing state
    std::vector<juce::Point<float>> building;           // stage points (Line / Polygon)
    bool pressAddedVertex = false;
    std::optional<juce::Point<float>> hoverScreen;
    std::vector<juce::Point<float>> freehandScreen;

    //==========================================================================
    // Model plumbing
    //==========================================================================

    juce::UndoManager* undoManager() { return vts.getUndoManagerForDomain (UndoDomain::Map); }

    juce::ValueTree getShapesTree (bool create)
    {
        auto config = getConfig ? getConfig() : juce::ValueTree();
        if (! config.isValid())
            return {};
        auto tree = config.getChildWithName (WFSParameterIDs::MapShapes);
        if (! tree.isValid() && create)
        {
            tree = juce::ValueTree (WFSParameterIDs::MapShapes);
            const juce::ScopedValueSetter<bool> writing (isWriting, true);
            config.appendChild (tree, nullptr);
        }
        return tree;
    }

    void ensureModel()
    {
        if (modelDirty)
            reloadModel();
    }

    void reloadModel()
    {
        modelDirty = false;
        const auto previousCount = shapes.size();
        shapes.clear();
        childIndex.clear();

        auto tree = getShapesTree (false);
        for (int c = 0; tree.isValid() && c < tree.getNumChildren(); ++c)
        {
            if (auto s = Shape::fromValueTree (tree.getChild (c)))
            {
                shapes.push_back (*s);
                childIndex.push_back (c);
            }
        }

        // A list that changed size under us (undo of an add/delete, a load)
        // invalidates index-based selection.
        if (shapes.size() != previousCount)
            selected.clear();
        selected.erase (std::remove_if (selected.begin(), selected.end(),
                                        [this] (int i) { return ! isIndexValid (i); }),
                        selected.end());
        if (building.empty())
            hoverScreen.reset();
        notifySelection();
    }

    bool isIndexValid (int i) const { return i >= 0 && i < (int) shapes.size(); }

    std::vector<int> selectedChildIndices() const
    {
        std::vector<int> out;
        for (int i : selected)
            if (i >= 0 && i < (int) childIndex.size())
                out.push_back (childIndex[(size_t) i]);
        return out;
    }

    void commitModified (const std::vector<int>& indices, const juce::String& name)
    {
        auto tree = getShapesTree (false);
        if (! tree.isValid())
            return;
        auto* um = undoManager();
        if (um != nullptr) um->beginNewTransaction (name);
        writeShapes (tree, indices, um);
        notifySelection();
    }

    void writeShapes (juce::ValueTree& tree, const std::vector<int>& indices, juce::UndoManager* um)
    {
        const juce::ScopedValueSetter<bool> writing (isWriting, true);
        for (int i : indices)
        {
            if (! isIndexValid (i) || i >= (int) childIndex.size())
                continue;
            auto child = tree.getChild (childIndex[(size_t) i]);
            if (child.isValid())
                child.copyPropertiesFrom (shapes[(size_t) i].toValueTree(), um);
        }
    }

    void addShapes (std::vector<Shape> incoming, const juce::String& name)
    {
        ensureModel();
        incoming.erase (std::remove_if (incoming.begin(), incoming.end(),
                                        [] (const Shape& s) { return ! s.isValid(); }),
                        incoming.end());
        if (incoming.empty())
            return;

        const int room = MapDrawing::maxShapes - (int) shapes.size();
        if (room <= 0)
        {
            message (LOC ("map.shapes.messages.limit").replace ("{count}", juce::String (MapDrawing::maxShapes)));
            return;
        }
        if ((int) incoming.size() > room)
            incoming.resize ((size_t) room);

        auto tree = getShapesTree (true);
        if (! tree.isValid())
            return;

        auto* um = undoManager();
        if (um != nullptr) um->beginNewTransaction (name);
        {
            const juce::ScopedValueSetter<bool> writing (isWriting, true);
            for (auto& s : incoming)
                tree.appendChild (s.toValueTree(), um);
        }

        const int firstNew = (int) shapes.size();
        reloadModel();
        selected.clear();
        for (int i = firstNew; i < (int) shapes.size(); ++i)
            selected.push_back (i);
        notifySelection();
    }

    void applyToSelection (const std::function<void (Shape&)>& fn, const juce::String& name, bool newTransaction)
    {
        ensureModel();
        if (selected.empty())
        {
            notifySelection();
            return;
        }
        auto tree = getShapesTree (false);
        if (! tree.isValid())
            return;
        for (int i : selected)
            if (isIndexValid (i))
                fn (shapes[(size_t) i]);

        auto* um = undoManager();
        if (um != nullptr && newTransaction) um->beginNewTransaction (name);
        writeShapes (tree, selected, um);
        if (requestRepaint) requestRepaint();
    }

    void clearSelection()
    {
        if (selected.empty())
            return;
        selected.clear();
        notifySelection();
        if (requestRepaint) requestRepaint();
    }

    void notifySelection()
    {
        if (onSelectionChanged) onSelectionChanged();
    }

    void message (const juce::String& text)
    {
        if (showMessage) showMessage (text);
    }

    //==========================================================================
    // Geometry in screen space
    //==========================================================================

    float handleDrawSize() const  { return juce::jmax (7.0f, 8.0f * WfsLookAndFeel::uiScale); }
    float handleHitRadius() const { return touchPointer ? 18.0f : 9.0f; }

    std::vector<juce::Point<float>> screenOutline (const Shape& s) const
    {
        auto pts = s.getOutlinePoints();
        for (auto& p : pts) p = toScreen (p);
        return pts;
    }

    juce::Path screenPath (const Shape& s) const
    {
        juce::Path p;
        auto pts = screenOutline (s);
        if (pts.size() < 2)
            return p;
        p.startNewSubPath (pts[0]);
        for (size_t k = 1; k < pts.size(); ++k)
            p.lineTo (pts[k]);
        if (s.isClosed())
            p.closeSubPath();
        return p;
    }

    static void strokeStyled (juce::Graphics& g, const juce::Path& path, const Style& style)
    {
        g.setColour (style.colour);
        juce::PathStrokeType stroke (style.lineWidth, juce::PathStrokeType::mitered, juce::PathStrokeType::butt);
        if (style.dashed)
        {
            const float dash[] = { MapDrawing::dashLength (style.lineWidth), MapDrawing::gapLength (style.lineWidth) };
            juce::Path dashed;
            stroke.createDashedStroke (dashed, path, dash, 2);
            g.fillPath (dashed);
        }
        else
        {
            g.strokePath (path, stroke);
        }
    }

    void paintShape (juce::Graphics& g, const Shape& s) const
    {
        strokeStyled (g, screenPath (s), s.getStyle());

        if (s.label.isNotEmpty())
        {
            const float us = WfsLookAndFeel::uiScale;
            const auto anchor = toScreen (s.getLabelAnchor());
            const float h = juce::jmax (10.0f, 12.0f * us);
            juce::Font font (juce::FontOptions().withHeight (h));
            g.setFont (font);
            g.setColour (s.colour);
            const float w = juce::GlyphArrangement::getStringWidth (font, s.label) + 4.0f;
            g.drawText (s.label, juce::Rectangle<float> (anchor.x - w * 0.5f, anchor.y - 4.0f - h, w, h),
                        juce::Justification::centredBottom, false);
        }
    }

    int hitTestStroke (juce::Point<float> pos) const
    {
        for (int i = (int) shapes.size(); --i >= 0;)
        {
            const auto& s = shapes[(size_t) i];
            const float tol = juce::jmax (touchPointer ? 14.0f : 6.0f, s.lineWidth * 0.5f + 4.0f);
            if (MapDrawing::distanceToPolyline (pos, screenOutline (s), s.isClosed()) <= tol)
                return i;
        }
        return -1;
    }

    /** Local box: unit square for rectangles/ellipses, vertex bounds otherwise. */
    static juce::Rectangle<float> localBox (const Shape& s)
    {
        if (! s.usesVertices() || s.vertices.empty())
            return { -1.0f, -1.0f, 2.0f, 2.0f };
        float minX = s.vertices[0].x, maxX = minX, minY = s.vertices[0].y, maxY = minY;
        for (auto& v : s.vertices)
        {
            minX = juce::jmin (minX, v.x); maxX = juce::jmax (maxX, v.x);
            minY = juce::jmin (minY, v.y); maxY = juce::jmax (maxY, v.y);
        }
        return { minX, minY, maxX - minX, maxY - minY };
    }

    static juce::Point<float> localCorner (const juce::Rectangle<float>& b, int k)
    {
        switch (k)
        {
            case 0:  return { b.getX(),     b.getY() };
            case 1:  return { b.getRight(), b.getY() };
            case 2:  return { b.getRight(), b.getBottom() };
            default: return { b.getX(),     b.getBottom() };
        }
    }

    void getCornerScreen (const Shape& s, juce::Point<float> out[4]) const
    {
        const auto b = localBox (s);
        for (int k = 0; k < 4; ++k)
            out[k] = toScreen (s.localToStage (localCorner (b, k)));
    }

    /** Rotation handle: beyond the middle of the box's top edge (local maxY,
        which is stage-up for an unrotated shape). */
    bool getRotationHandle (const Shape& s, juce::Point<float>& handle, juce::Point<float>& top) const
    {
        const auto b = localBox (s);
        top = toScreen (s.localToStage ({ b.getCentreX(), b.getBottom() }));
        const auto centre = toScreen (s.localToStage (b.getCentre()));
        auto dir = top - centre;
        float len = dir.getDistanceFromOrigin();
        if (len < 0.5f)
        {
            dir = { 0.0f, -1.0f };   // degenerate box (a horizontal line): straight up
            len = 1.0f;
        }
        handle = top + dir * (24.0f / len);
        return true;
    }

    std::vector<juce::Point<float>> vertexScreenPoints (const Shape& s) const
    {
        std::vector<juce::Point<float>> out;
        if (s.type == Type::Rectangle)
        {
            for (int k = 0; k < 4; ++k)
                out.push_back (toScreen (s.localToStage (localCorner ({ -1.0f, -1.0f, 2.0f, 2.0f }, k))));
            return out;
        }
        for (auto& v : s.vertices)
            out.push_back (toScreen (s.localToStage (v)));
        return out;
    }

    static void convertRectangleToPolygon (Shape& s)
    {
        s.type = Type::Polygon;
        s.vertices = { { -1.0f, -1.0f }, { 1.0f, -1.0f }, { 1.0f, 1.0f }, { -1.0f, 1.0f } };
    }

    juce::Rectangle<float> createBoxScreenRect() const
    {
        return juce::Rectangle<float> (gestureStartScreen, currentScreen);
    }

    //==========================================================================
    // Tool handlers
    //==========================================================================

    void startShapeGesture (Gesture g, const std::vector<int>& indices)
    {
        gesture = g;
        gestureIndices = indices;
        gestureStartShapes.clear();
        for (int i : indices)
            gestureStartShapes.push_back (shapes[(size_t) i]);
    }

    void selectDown (juce::Point<float> pos, juce::ModifierKeys mods)
    {
        // 1) Handles of a single selection
        if (selected.size() == 1 && isIndexValid (selected.front()))
        {
            const int idx = selected.front();
            const auto& s = shapes[(size_t) idx];
            const float r = handleHitRadius();

            juce::Point<float> rotPos, topPos;
            if (getRotationHandle (s, rotPos, topPos) && rotPos.getDistanceFrom (pos) < r)
            {
                startShapeGesture (Gesture::Rotate, { idx });
                const auto d = gestureStartStage - juce::Point<float> (s.posX, s.posY);
                rotateStartAngle = std::atan2 (d.y, d.x);
                return;
            }

            juce::Point<float> corners[4];
            getCornerScreen (s, corners);
            for (int k = 0; k < 4; ++k)
            {
                if (corners[k].getDistanceFrom (pos) < r)
                {
                    startShapeGesture (Gesture::Scale, { idx });
                    scaleCorner = k;
                    const auto b = localBox (s);
                    auto toFrame = [&s] (juce::Point<float> local) { return juce::Point<float> (local.x * s.scaleX, local.y * s.scaleY); };
                    scaleCornerStart = toFrame (localCorner (b, k));
                    scaleAnchor = toFrame (localCorner (b, (k + 2) % 4));
                    return;
                }
            }
        }

        // 2) A shape's outline
        const int hit = hitTestStroke (pos);
        if (hit >= 0)
        {
            const bool isSelected = std::find (selected.begin(), selected.end(), hit) != selected.end();
            if (mods.isShiftDown())
            {
                if (isSelected) selected.erase (std::remove (selected.begin(), selected.end(), hit), selected.end());
                else            selected.push_back (hit);
                notifySelection();
                return;
            }
            if (! isSelected)
            {
                selected = { hit };
                notifySelection();
            }
            startShapeGesture (Gesture::Move, selected);
            return;
        }

        // 3) Inside a selected shape's box: move the selection
        for (int i : selected)
        {
            if (! isIndexValid (i)) continue;
            juce::Point<float> c[4];
            getCornerScreen (shapes[(size_t) i], c);
            juce::Path box;
            box.startNewSubPath (c[0]);
            for (int k = 1; k < 4; ++k) box.lineTo (c[k]);
            box.closeSubPath();
            if (box.contains (pos))
            {
                startShapeGesture (Gesture::Move, selected);
                return;
            }
        }

        // 4) Empty space: rubber band
        if (! mods.isShiftDown())
            clearSelection();
        gesture = Gesture::Rubber;
    }

    void editPointsDown (juce::Point<float> pos, juce::ModifierKeys mods)
    {
        if (selected.size() == 1 && isIndexValid (selected.front()))
        {
            const int idx = selected.front();
            auto& s = shapes[(size_t) idx];
            if (s.usesVertices() || s.type == Type::Rectangle)
            {
                const auto pts = vertexScreenPoints (s);
                for (int k = 0; k < (int) pts.size(); ++k)
                {
                    if (pts[(size_t) k].getDistanceFrom (pos) < handleHitRadius())
                    {
                        startShapeGesture (Gesture::Vertex, { idx });
                        convertedRectangle = (s.type == Type::Rectangle);
                        if (convertedRectangle)
                            convertRectangleToPolygon (s);
                        vertexIndex = k;
                        return;
                    }
                }
            }
        }

        // Elsewhere: pick the shape under the pointer (or move a selection).
        const int hit = hitTestStroke (pos);
        if (hit >= 0)
        {
            if (std::find (selected.begin(), selected.end(), hit) == selected.end() || mods.isShiftDown())
            {
                selected = { hit };
                notifySelection();
                if (! shapes[(size_t) hit].usesVertices() && shapes[(size_t) hit].type != Type::Rectangle)
                    message (LOC ("map.shapes.messages.noPoints"));
            }
            startShapeGesture (Gesture::Move, selected);
            return;
        }
        clearSelection();
    }

    void polyDown (juce::Point<float> pos)
    {
        const auto stagePos = toStage (pos);

        // Polygon: a press on the first vertex closes it
        if (tool == Tool::Polygon && building.size() >= 3
            && toScreen (building.front()).getDistanceFrom (pos) < (touchPointer ? 18.0f : 10.0f))
        {
            finishBuilding (true);
            return;
        }

        if ((int) building.size() >= MapDrawing::maxVertices)
            return;
        building.push_back (stagePos);
        pressAddedVertex = true;
    }

    void finishBuilding (bool commit)
    {
        if (building.empty())
            return;
        auto pts = std::move (building);
        building.clear();
        hoverScreen.reset();

        const Type t = (tool == Tool::Polygon) ? Type::Polygon : Type::Polyline;
        const int minCount = (t == Type::Polygon) ? 3 : 2;
        if (! commit || (int) pts.size() < minCount)
        {
            if (requestRepaint) requestRepaint();
            return;
        }
        addShapes ({ Shape::fromStagePoints (t, pts, currentStyle) },
                   t == Type::Polygon ? "Map Shape: Draw Polygon" : "Map Shape: Draw Line");
    }

    void finishCreateBox (bool keepSquare)
    {
        auto a = gestureStartStage;
        auto b = toStage (currentScreen);
        if (keepSquare)
        {
            const float side = juce::jmax (std::abs (b.x - a.x), std::abs (b.y - a.y));
            b = { a.x + (b.x >= a.x ? side : -side), a.y + (b.y >= a.y ? side : -side) };
        }
        if (! travelled)
            return;

        Shape s;
        s.type = (tool == Tool::Ellipse) ? Type::Ellipse : Type::Rectangle;
        s.applyStyle (currentStyle);
        s.posX = (a.x + b.x) * 0.5f;
        s.posY = (a.y + b.y) * 0.5f;
        s.scaleX = std::abs (b.x - a.x) * 0.5f;
        s.scaleY = std::abs (b.y - a.y) * 0.5f;
        if (s.scaleX < MapDrawing::minHalfExtent || s.scaleY < MapDrawing::minHalfExtent)
            return;
        addShapes ({ s }, s.type == Type::Ellipse ? "Map Shape: Draw Ellipse" : "Map Shape: Draw Rectangle");
    }

    void finishFreehand()
    {
        auto screenPts = std::move (freehandScreen);
        freehandScreen.clear();
        if (screenPts.size() < 2 || ! travelled)
            return;

        const bool closes = screenPts.size() >= 4
                            && screenPts.front().getDistanceFrom (screenPts.back()) < 12.0f;

        std::vector<juce::Point<float>> stagePts;
        for (auto& p : screenPts)
            stagePts.push_back (toStage (p));
        const float scale = getViewScale ? juce::jmax (0.001f, getViewScale()) : 30.0f;
        stagePts = MapDrawing::simplifyToCap (stagePts, 1.5f / scale, MapDrawing::freehandPointCap);

        if (closes && stagePts.size() >= 4)
        {
            stagePts.pop_back();   // the end that met the start
            addShapes ({ Shape::fromStagePoints (Type::Polygon, stagePts, currentStyle) }, "Map Shape: Draw Freehand");
        }
        else if (stagePts.size() >= 2)
        {
            addShapes ({ Shape::fromStagePoints (Type::Polyline, stagePts, currentStyle) }, "Map Shape: Draw Freehand");
        }
    }

    void finishRubber (bool additive)
    {
        const auto r = juce::Rectangle<float> (gestureStartScreen, currentScreen);
        if (! travelled)
            return;
        if (! additive)
            selected.clear();
        for (int i = 0; i < (int) shapes.size(); ++i)
        {
            if (std::find (selected.begin(), selected.end(), i) != selected.end())
                continue;
            const auto pts = screenOutline (shapes[(size_t) i]);
            const bool inside = std::any_of (pts.begin(), pts.end(), [&r] (juce::Point<float> p) { return r.contains (p); });
            if (inside)
                selected.push_back (i);
        }
        notifySelection();
    }

    void applyScale (juce::Point<float> stagePos, bool uniform)
    {
        if (gestureIndices.empty() || gestureStartShapes.empty())
            return;
        const auto& start = gestureStartShapes.front();
        auto& s = shapes[(size_t) gestureIndices.front()];

        // Pointer in the shape's frame: rotation removed, metres, relative to pos.
        const float rad = juce::degreesToRadians (start.rotation);
        const auto d = stagePos - juce::Point<float> (start.posX, start.posY);
        const juce::Point<float> m (d.x * std::cos (-rad) - d.y * std::sin (-rad),
                                    d.x * std::sin (-rad) + d.y * std::cos (-rad));

        auto factor = [] (float now, float anchor, float was)
        {
            const float span = was - anchor;
            if (std::abs (span) < 1.0e-5f)
                return 1.0f;
            return juce::jmax (0.01f, (now - anchor) / span);
        };
        float fx = factor (m.x, scaleAnchor.x, scaleCornerStart.x);
        float fy = factor (m.y, scaleAnchor.y, scaleCornerStart.y);

        // A degenerate axis (a horizontal or vertical line) cannot scale.
        const bool flatX = std::abs (scaleCornerStart.x - scaleAnchor.x) < 1.0e-5f;
        const bool flatY = std::abs (scaleCornerStart.y - scaleAnchor.y) < 1.0e-5f;
        if (uniform && ! flatX && ! flatY)
            fx = fy = juce::jmax (fx, fy);
        else if (uniform && flatX) fx = fy;
        else if (uniform && flatY) fy = fx;

        s = start;
        if (s.usesVertices())
        {
            for (auto& v : s.vertices)
            {
                juce::Point<float> p (v.x * start.scaleX, v.y * start.scaleY);
                p = { scaleAnchor.x + (p.x - scaleAnchor.x) * fx, scaleAnchor.y + (p.y - scaleAnchor.y) * fy };
                v = { p.x / start.scaleX, p.y / start.scaleY };
            }
            s.recentre();
        }
        else
        {
            s.scaleX = juce::jmax (MapDrawing::minHalfExtent, start.scaleX * fx);
            s.scaleY = juce::jmax (MapDrawing::minHalfExtent, start.scaleY * fy);
            // The box centre (frame origin) moves so the anchor corner stays put.
            const juce::Point<float> c (scaleAnchor.x * (1.0f - fx), scaleAnchor.y * (1.0f - fy));
            s.posX = start.posX + c.x * std::cos (rad) - c.y * std::sin (rad);
            s.posY = start.posY + c.x * std::sin (rad) + c.y * std::cos (rad);
        }
    }

    void applyRotate (juce::Point<float> stagePos, bool snap)
    {
        if (gestureIndices.empty() || gestureStartShapes.empty())
            return;
        const auto& start = gestureStartShapes.front();
        auto& s = shapes[(size_t) gestureIndices.front()];
        const auto d = stagePos - juce::Point<float> (start.posX, start.posY);
        if (d.getDistanceFromOrigin() < 1.0e-4f)
            return;
        float rot = start.rotation + juce::radiansToDegrees (std::atan2 (d.y, d.x) - rotateStartAngle);
        if (snap)
            rot = std::round (rot / 15.0f) * 15.0f;
        while (rot > 180.0f)   rot -= 360.0f;
        while (rot <= -180.0f) rot += 360.0f;
        s.rotation = rot;
    }

    void cancelGesture (bool restore)
    {
        if (restore)
            for (size_t k = 0; k < gestureIndices.size() && k < gestureStartShapes.size(); ++k)
                if (isIndexValid (gestureIndices[k]))
                    shapes[(size_t) gestureIndices[k]] = gestureStartShapes[k];
        gesture = Gesture::None;
        gestureIndices.clear();
        gestureStartShapes.clear();
        freehandScreen.clear();
        convertedRectangle = false;
    }

    JUCE_DECLARE_NON_COPYABLE (MapShapesEditor)
};
