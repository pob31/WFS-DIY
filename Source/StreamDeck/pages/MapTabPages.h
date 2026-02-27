#pragma once

/**
 * MapTabPages — Stream Deck+ page definitions for the Map tab.
 *
 * Creates a context-sensitive StreamDeckPage for the Map tab (tab index 6).
 *
 * Three modes based on current map selection:
 *   1. No selection — ComboBox dials to browse/select inputs and clusters.
 *   2. Input selected — Position/Offset X/Y/Z + Orientation dials.
 *   3. Cluster selected — Ref position X/Y + relative Scale/Rotation dials.
 *
 * Top row: navigation buttons to Outputs, Reverb, Inputs.
 *          Button 0 overridden with "Input N" or "Cluster N" when selected.
 * Bottom row: Show Levels toggle, Position/Offset toggle, Fit Stage, Fit All Inputs.
 * When dragging via touch, all dials are suppressed.
 */

#include "../StreamDeckPage.h"
#include "../../Parameters/WFSValueTreeState.h"
#include "../../Parameters/WFSParameterIDs.h"
#include "../../Parameters/WFSParameterDefaults.h"
#include "../../Parameters/WFSConstraints.h"
#include "../../Helpers/CoordinateConverter.h"
#include "../../Localization/LocalizationManager.h"

namespace MapTabPages
{

static constexpr int MAP_MAIN_TAB_INDEX = 6;

//==============================================================================
// Callbacks struct — actions that must go through the GUI
//==============================================================================

struct MapCallbacks
{
    std::function<void()>                    toggleLevelOverlay;
    std::function<void()>                    fitStageToScreen;
    std::function<void()>                    fitAllInputsToScreen;
    std::function<void (int)>                selectInput;         // 0-based index
    std::function<void (int)>                selectCluster;       // 1-based (1-10)
    std::function<void (int, float, float)>  moveClusterRef;      // (cluster, x, y)
    std::function<void (int, float)>         scaleCluster;        // (cluster, scaleFactor)
    std::function<void (int, float)>         rotateCluster;       // (cluster, angleDeg)
    std::function<void()>                    repaintMap;          // trigger map redraw after param change
    std::function<void()>                    deselectAll;         // deselect input/cluster on map
    std::function<float()>                   getViewCenterX;      // pan/zoom accessors
    std::function<float()>                   getViewCenterY;
    std::function<void (float)>              setViewCenterX;
    std::function<void (float)>              setViewCenterY;
    std::function<float()>                   getViewScale;
    std::function<void (float)>              setViewScale;
};

//==============================================================================
// State queries struct — read-only state from the GUI
//==============================================================================

struct MapStateQueries
{
    std::function<int()>                          getSelectedInput;        // -1 or 0-based
    std::function<int()>                          getSelectedCluster;      // -1 or 1-10
    std::function<bool()>                         isDragging;
    std::function<int()>                          getNumInputs;
    std::function<bool()>                         getLevelOverlayEnabled;
    std::function<juce::Point<float> (int)>       getClusterRefPosition;   // cluster num → pos
};

//==============================================================================
// Page factory
//==============================================================================

inline StreamDeckPage createMapPage (WFSValueTreeState& state,
                                      const MapCallbacks& callbacks,
                                      const MapStateQueries& queries,
                                      std::shared_ptr<bool> posOffsetMode)
{
    using namespace WFSParameterIDs;
    using namespace WFSParameterDefaults;

    StreamDeckPage page ("Map");

    const auto grey = juce::Colour (0xFF3A3A3A);

    // Query current state
    const int selInput   = queries.getSelectedInput   ? queries.getSelectedInput()   : -1;
    const int selCluster = queries.getSelectedCluster ? queries.getSelectedCluster() : -1;
    const bool dragging  = queries.isDragging         ? queries.isDragging()          : false;
    const int numInputs  = queries.getNumInputs       ? queries.getNumInputs()        : 0;

    //======================================================================
    // Top row: navigation buttons
    //======================================================================

    // Button 0: → Outputs (tab 2) — always present
    page.topRowNavigateToTab[0]     = 2;
    page.topRowOverrideLabel[0]     = LOC ("tabs.outputs");
    page.topRowOverrideColour[0]    = juce::Colour (0xFF4A90D9);

    // Button 1: → Reverb (tab 3)
    page.topRowNavigateToTab[1]     = 3;
    page.topRowOverrideLabel[1]     = LOC ("tabs.reverb");
    page.topRowOverrideColour[1]    = juce::Colour (0xFF9B6FC3);

    // Button 2: → Inputs (tab 4) — show selected channel if any
    page.topRowNavigateToTab[2]     = 4;
    if (selInput >= 0)
    {
        page.topRowOverrideLabel[2]    = LOC ("tabs.inputs") + "\n(Ch " + juce::String (selInput + 1) + ")";
        page.topRowNavigateToItem[2]   = selInput;
    }
    else
        page.topRowOverrideLabel[2] = LOC ("tabs.inputs");
    page.topRowOverrideColour[2]    = juce::Colour (0xFF26A69A);

    // Button 3: Deselect when input/cluster selected, otherwise informational
    if (selInput >= 0 || selCluster > 0)
    {
        auto& btn = page.topRowButtons[3];
        if (selInput >= 0)
            btn.label = LOC ("streamDeck.map.labels.inputN").replace ("%d", juce::String (selInput + 1)) + "\n" + LOC ("streamDeck.map.labels.deselect");
        else
            btn.label = LOC ("streamDeck.map.labels.clusterN").replace ("%d", juce::String (selCluster)) + "\n" + LOC ("streamDeck.map.labels.deselect");
        btn.colour = juce::Colour (0xFF666666);
        btn.type   = ButtonBinding::Action;
        btn.onPress = [callbacks]() { if (callbacks.deselectAll) callbacks.deselectAll(); };
    }

    //======================================================================
    // Single section: Map controls
    //======================================================================
    {
        auto& sec = page.sections[0];
        sec.sectionName   = LOC ("tabs.map");
        sec.sectionColour = juce::Colour (0xFF7B68EE);

        //------------------------------------------------------------------
        // Button 0: Show Levels toggle (dynamic label)
        //------------------------------------------------------------------
        {
            auto& btn = sec.buttons[0];
            btn.colour       = grey;
            btn.activeColour = juce::Colour (0xFF2ECC71);
            btn.type         = ButtonBinding::Toggle;

            btn.getState = [queries]()
            {
                return queries.getLevelOverlayEnabled ? queries.getLevelOverlayEnabled() : false;
            };

            btn.getDynamicLabel = [queries]()
            {
                bool isOn = queries.getLevelOverlayEnabled ? queries.getLevelOverlayEnabled() : false;
                return isOn ? LOC ("streamDeck.map.buttons.showLevelsOn")
                            : LOC ("streamDeck.map.buttons.showLevelsOff");
            };

            btn.onPress = [callbacks]()
            {
                if (callbacks.toggleLevelOverlay)
                    callbacks.toggleLevelOverlay();
            };
        }

        //------------------------------------------------------------------
        // Button 1: Position / Offset toggle (dynamic label)
        //------------------------------------------------------------------
        {
            auto& btn = sec.buttons[1];
            btn.colour       = grey;
            btn.activeColour = juce::Colour (0xFF4A90D9);
            btn.type         = ButtonBinding::Toggle;
            btn.requestsPageRebuild = true;

            btn.getState = [posOffsetMode]()
            {
                return posOffsetMode ? *posOffsetMode : false;
            };

            btn.getDynamicLabel = [posOffsetMode]()
            {
                bool isOffset = posOffsetMode ? *posOffsetMode : false;
                return isOffset ? LOC ("streamDeck.map.buttons.offsetMode")
                                : LOC ("streamDeck.map.buttons.positionMode");
            };

            btn.onPress = [posOffsetMode]()
            {
                if (posOffsetMode)
                    *posOffsetMode = ! *posOffsetMode;
            };
        }

        //------------------------------------------------------------------
        // Button 2: Fit All Inputs to Screen
        //------------------------------------------------------------------
        {
            auto& btn = sec.buttons[2];
            btn.label  = LOC ("streamDeck.map.buttons.fitAllInputs");
            btn.colour = grey;
            btn.type   = ButtonBinding::Action;

            btn.onPress = [callbacks]()
            {
                if (callbacks.fitAllInputsToScreen)
                    callbacks.fitAllInputsToScreen();
            };
        }

        //------------------------------------------------------------------
        // Button 3: Fit Stage to Screen
        //------------------------------------------------------------------
        {
            auto& btn = sec.buttons[3];
            btn.label  = LOC ("streamDeck.map.buttons.fitStage");
            btn.colour = grey;
            btn.type   = ButtonBinding::Action;

            btn.onPress = [callbacks]()
            {
                if (callbacks.fitStageToScreen)
                    callbacks.fitStageToScreen();
            };
        }

        //==================================================================
        // Dials — mode-dependent
        //==================================================================

        if (dragging)
        {
            // Dragging mode: all dials unbound (empty LCD zones)
        }
        else if (selInput >= 0)
        {
            //--------------------------------------------------------------
            // Mode 2: Input selected — Position/Offset + Orientation
            //--------------------------------------------------------------
            const int ch = selInput;
            const bool offsetMode = posOffsetMode ? *posOffsetMode : false;

            auto bounds = WFSConstraints::getStageBounds (state);

            if (offsetMode)
            {
                // Offset dials — adapt to input's coordinate mode
                int coordModeVal = static_cast<int> (state.getInputParameter (ch, inputCoordinateMode));
                auto coordMode = static_cast<WFSCoordinates::Mode> (coordModeVal);

                float curPosX = static_cast<float> (state.getInputParameter (ch, inputPositionX));
                float curPosY = static_cast<float> (state.getInputParameter (ch, inputPositionY));
                float curPosZ = static_cast<float> (state.getInputParameter (ch, inputPositionZ));

                juce::String labels[3], unitsArr[3];
                float mins[3], maxs[3], steps[3], fines[3];
                int decs[3];

                switch (coordMode)
                {
                    case WFSCoordinates::Mode::Cylindrical:
                        labels[0]   = LOC ("streamDeck.map.dials.radius");
                        labels[1]   = LOC ("streamDeck.map.dials.azimuth");
                        labels[2]   = LOC ("streamDeck.map.dials.height");
                        unitsArr[0] = LOC ("units.meters");
                        unitsArr[1] = LOC ("units.degrees");
                        unitsArr[2] = LOC ("units.meters");
                        mins[0]     = -50.0f;        maxs[0] = 50.0f;
                        mins[1]     = -180.0f;       maxs[1] = 180.0f;
                        mins[2]     = bounds.minZ - curPosZ;  maxs[2] = bounds.maxZ - curPosZ;
                        steps[0]    = 0.1f;  fines[0] = 0.01f;  decs[0] = 2;
                        steps[1]    = 5.0f;  fines[1] = 1.0f;   decs[1] = 0;
                        steps[2]    = 0.1f;  fines[2] = 0.01f;  decs[2] = 2;
                        break;

                    case WFSCoordinates::Mode::Spherical:
                        labels[0]   = LOC ("streamDeck.map.dials.radius");
                        labels[1]   = LOC ("streamDeck.map.dials.azimuth");
                        labels[2]   = LOC ("streamDeck.map.dials.elevation");
                        unitsArr[0] = LOC ("units.meters");
                        unitsArr[1] = LOC ("units.degrees");
                        unitsArr[2] = LOC ("units.degrees");
                        mins[0]     = -50.0f;    maxs[0] = 50.0f;
                        mins[1]     = -180.0f;   maxs[1] = 180.0f;
                        mins[2]     = -90.0f;    maxs[2] = 90.0f;
                        steps[0]    = 0.1f;  fines[0] = 0.01f;  decs[0] = 2;
                        steps[1]    = 5.0f;  fines[1] = 1.0f;   decs[1] = 0;
                        steps[2]    = 5.0f;  fines[2] = 1.0f;   decs[2] = 0;
                        break;

                    default: // Cartesian
                        labels[0]   = LOC ("streamDeck.map.dials.offsetX");
                        labels[1]   = LOC ("streamDeck.map.dials.offsetY");
                        labels[2]   = LOC ("streamDeck.map.dials.offsetZ");
                        unitsArr[0] = LOC ("units.meters");
                        unitsArr[1] = LOC ("units.meters");
                        unitsArr[2] = LOC ("units.meters");
                        mins[0]     = bounds.minX - curPosX;  maxs[0] = bounds.maxX - curPosX;
                        mins[1]     = bounds.minY - curPosY;  maxs[1] = bounds.maxY - curPosY;
                        mins[2]     = bounds.minZ - curPosZ;  maxs[2] = bounds.maxZ - curPosZ;
                        steps[0]    = 0.1f;  fines[0] = 0.01f;  decs[0] = 2;
                        steps[1]    = 0.1f;  fines[1] = 0.01f;  decs[1] = 2;
                        steps[2]    = 0.1f;  fines[2] = 0.01f;  decs[2] = 2;
                        break;
                }

                for (int di = 0; di < 3; ++di)
                {
                    auto& d = sec.dials[di];
                    d.paramName     = labels[di];
                    d.paramUnit     = unitsArr[di];
                    d.minValue      = mins[di];
                    d.maxValue      = maxs[di];
                    d.step          = steps[di];
                    d.fineStep      = fines[di];
                    d.decimalPlaces = decs[di];
                    d.type          = DialBinding::Float;

                    d.getValue = [&state, ch, coordMode, di]()
                    {
                        float ox = static_cast<float> (state.getInputParameter (ch, inputOffsetX));
                        float oy = static_cast<float> (state.getInputParameter (ch, inputOffsetY));
                        float oz = static_cast<float> (state.getInputParameter (ch, inputOffsetZ));
                        float v1, v2, v3;
                        WFSCoordinates::cartesianToDisplay (coordMode, ox, oy, oz, v1, v2, v3);
                        return (di == 0) ? v1 : (di == 1) ? v2 : v3;
                    };

                    d.setValue = [&state, ch, coordMode, di, callbacks] (float v)
                    {
                        float ox = static_cast<float> (state.getInputParameter (ch, inputOffsetX));
                        float oy = static_cast<float> (state.getInputParameter (ch, inputOffsetY));
                        float oz = static_cast<float> (state.getInputParameter (ch, inputOffsetZ));
                        float v1, v2, v3;
                        WFSCoordinates::cartesianToDisplay (coordMode, ox, oy, oz, v1, v2, v3);

                        if (di == 0) v1 = v;
                        else if (di == 1) v2 = v;
                        else v3 = v;

                        auto cart = WFSCoordinates::displayToCartesian (coordMode, v1, v2, v3);
                        WFSConstraints::constrainOffset (state, ch, cart.x, cart.y, cart.z);
                        state.setInputParameter (ch, inputOffsetX, cart.x);
                        state.setInputParameter (ch, inputOffsetY, cart.y);
                        state.setInputParameter (ch, inputOffsetZ, cart.z);
                        if (callbacks.repaintMap) callbacks.repaintMap();
                    };
                }
            }
            else
            {
                // Position dials — adapt to input's coordinate mode
                int coordModeVal = static_cast<int> (state.getInputParameter (ch, inputCoordinateMode));
                auto coordMode = static_cast<WFSCoordinates::Mode> (coordModeVal);

                // Labels/units/ranges per mode
                juce::String labels[3], units[3];
                float mins[3], maxs[3], steps[3], fines[3];
                int decimals[3];

                switch (coordMode)
                {
                    case WFSCoordinates::Mode::Cylindrical:
                        labels[0]   = LOC ("streamDeck.map.dials.radius");
                        labels[1]   = LOC ("streamDeck.map.dials.azimuth");
                        labels[2]   = LOC ("streamDeck.map.dials.height");
                        units[0]    = LOC ("units.meters");
                        units[1]    = LOC ("units.degrees");
                        units[2]    = LOC ("units.meters");
                        mins[0]     = 0.0f;         maxs[0] = 50.0f;
                        mins[1]     = -180.0f;       maxs[1] = 180.0f;
                        mins[2]     = bounds.minZ;   maxs[2] = bounds.maxZ;
                        steps[0]    = 0.1f;  fines[0] = 0.01f;  decimals[0] = 2;
                        steps[1]    = 5.0f;  fines[1] = 1.0f;   decimals[1] = 0;
                        steps[2]    = 0.1f;  fines[2] = 0.01f;  decimals[2] = 2;
                        break;

                    case WFSCoordinates::Mode::Spherical:
                        labels[0]   = LOC ("streamDeck.map.dials.radius");
                        labels[1]   = LOC ("streamDeck.map.dials.azimuth");
                        labels[2]   = LOC ("streamDeck.map.dials.elevation");
                        units[0]    = LOC ("units.meters");
                        units[1]    = LOC ("units.degrees");
                        units[2]    = LOC ("units.degrees");
                        mins[0]     = 0.0f;     maxs[0] = 50.0f;
                        mins[1]     = -180.0f;   maxs[1] = 180.0f;
                        mins[2]     = -90.0f;    maxs[2] = 90.0f;
                        steps[0]    = 0.1f;  fines[0] = 0.01f;  decimals[0] = 2;
                        steps[1]    = 5.0f;  fines[1] = 1.0f;   decimals[1] = 0;
                        steps[2]    = 5.0f;  fines[2] = 1.0f;   decimals[2] = 0;
                        break;

                    default: // Cartesian
                        labels[0]   = LOC ("streamDeck.map.dials.positionX");
                        labels[1]   = LOC ("streamDeck.map.dials.positionY");
                        labels[2]   = LOC ("streamDeck.map.dials.positionZ");
                        units[0]    = LOC ("units.meters");
                        units[1]    = LOC ("units.meters");
                        units[2]    = LOC ("units.meters");
                        mins[0]     = bounds.minX;   maxs[0] = bounds.maxX;
                        mins[1]     = bounds.minY;   maxs[1] = bounds.maxY;
                        mins[2]     = bounds.minZ;   maxs[2] = bounds.maxZ;
                        steps[0]    = 0.1f;  fines[0] = 0.01f;  decimals[0] = 2;
                        steps[1]    = 0.1f;  fines[1] = 0.01f;  decimals[1] = 2;
                        steps[2]    = 0.1f;  fines[2] = 0.01f;  decimals[2] = 2;
                        break;
                }

                for (int di = 0; di < 3; ++di)
                {
                    auto& d = sec.dials[di];
                    d.paramName     = labels[di];
                    d.paramUnit     = units[di];
                    d.minValue      = mins[di];
                    d.maxValue      = maxs[di];
                    d.step          = steps[di];
                    d.fineStep      = fines[di];
                    d.decimalPlaces = decimals[di];
                    d.type          = DialBinding::Float;

                    d.getValue = [&state, ch, coordMode, di]()
                    {
                        float x = static_cast<float> (state.getInputParameter (ch, inputPositionX));
                        float y = static_cast<float> (state.getInputParameter (ch, inputPositionY));
                        float z = static_cast<float> (state.getInputParameter (ch, inputPositionZ));
                        float v1, v2, v3;
                        WFSCoordinates::cartesianToDisplay (coordMode, x, y, z, v1, v2, v3);
                        return (di == 0) ? v1 : (di == 1) ? v2 : v3;
                    };

                    d.setValue = [&state, ch, coordMode, di, callbacks] (float v)
                    {
                        float x = static_cast<float> (state.getInputParameter (ch, inputPositionX));
                        float y = static_cast<float> (state.getInputParameter (ch, inputPositionY));
                        float z = static_cast<float> (state.getInputParameter (ch, inputPositionZ));
                        float v1, v2, v3;
                        WFSCoordinates::cartesianToDisplay (coordMode, x, y, z, v1, v2, v3);

                        if (di == 0) v1 = v;
                        else if (di == 1) v2 = v;
                        else v3 = v;

                        auto cart = WFSCoordinates::displayToCartesian (coordMode, v1, v2, v3);
                        WFSConstraints::constrainPosition (state, ch, cart.x, cart.y, cart.z);
                        state.setInputParameter (ch, inputPositionX, cart.x);
                        state.setInputParameter (ch, inputPositionY, cart.y);
                        state.setInputParameter (ch, inputPositionZ, cart.z);
                        if (callbacks.repaintMap) callbacks.repaintMap();
                    };
                }
            }

            // Dial 3: Orientation (always active in input-selected mode)
            {
                auto& d = sec.dials[3];
                d.paramName        = LOC ("streamDeck.map.dials.orientation");
                d.paramUnit        = LOC ("units.degrees");
                d.minValue         = static_cast<float> (inputRotationMin);
                d.maxValue         = static_cast<float> (inputRotationMax);
                d.step             = 5.0f;
                d.fineStep         = 1.0f;
                d.decimalPlaces    = 0;
                d.type             = DialBinding::Int;
                d.invertDirection  = true;

                d.getValue = [&state, ch]()
                {
                    return static_cast<float> (static_cast<int> (state.getInputParameter (ch, inputRotation)));
                };
                d.setValue = [&state, ch, callbacks] (float v)
                {
                    state.setInputParameter (ch, inputRotation, juce::roundToInt (v));
                    if (callbacks.repaintMap) callbacks.repaintMap();
                };
            }
        }
        else if (selCluster > 0)
        {
            //--------------------------------------------------------------
            // Mode 3: Cluster selected — Ref position + Scale/Rotation
            //--------------------------------------------------------------
            const int cluster = selCluster;
            auto clusterBounds = WFSConstraints::getStageBounds (state);

            // Dial 0: Cluster Ref X
            {
                auto& d = sec.dials[0];
                d.paramName     = LOC ("streamDeck.map.dials.clusterRefX");
                d.paramUnit     = LOC ("units.meters");
                d.minValue      = clusterBounds.minX;
                d.maxValue      = clusterBounds.maxX;
                d.step          = 0.1f;
                d.fineStep      = 0.01f;
                d.decimalPlaces = 2;
                d.type          = DialBinding::Float;

                d.getValue = [queries, cluster]()
                {
                    return queries.getClusterRefPosition
                         ? queries.getClusterRefPosition (cluster).x
                         : 0.0f;
                };
                d.setValue = [callbacks, queries, cluster] (float x)
                {
                    float currentY = queries.getClusterRefPosition
                                   ? queries.getClusterRefPosition (cluster).y
                                   : 0.0f;
                    if (callbacks.moveClusterRef)
                        callbacks.moveClusterRef (cluster, x, currentY);
                };
            }

            // Dial 1: Cluster Ref Y
            {
                auto& d = sec.dials[1];
                d.paramName     = LOC ("streamDeck.map.dials.clusterRefY");
                d.paramUnit     = LOC ("units.meters");
                d.minValue      = clusterBounds.minY;
                d.maxValue      = clusterBounds.maxY;
                d.step          = 0.1f;
                d.fineStep      = 0.01f;
                d.decimalPlaces = 2;
                d.type          = DialBinding::Float;

                d.getValue = [queries, cluster]()
                {
                    return queries.getClusterRefPosition
                         ? queries.getClusterRefPosition (cluster).y
                         : 0.0f;
                };
                d.setValue = [callbacks, queries, cluster] (float y)
                {
                    float currentX = queries.getClusterRefPosition
                                   ? queries.getClusterRefPosition (cluster).x
                                   : 0.0f;
                    if (callbacks.moveClusterRef)
                        callbacks.moveClusterRef (cluster, currentX, y);
                };
            }

            // Dial 2: Cluster Scale (relative — getValue always returns 1.0)
            {
                auto& d = sec.dials[2];
                d.paramName     = LOC ("streamDeck.map.dials.clusterScale");
                d.paramUnit     = juce::CharPointer_UTF8 ("\xc3\x97");  // "×"
                d.minValue      = 0.5f;
                d.maxValue      = 2.0f;
                d.step          = 0.05f;
                d.fineStep      = 0.01f;
                d.decimalPlaces = 2;
                d.type          = DialBinding::Float;

                d.getValue = []() { return 1.0f; };
                d.setValue = [callbacks, cluster] (float v)
                {
                    if (callbacks.scaleCluster)
                        callbacks.scaleCluster (cluster, v);
                };
            }

            // Dial 3: Cluster Rotation (relative — getValue always returns 0)
            {
                auto& d = sec.dials[3];
                d.paramName     = LOC ("streamDeck.map.dials.clusterRotation");
                d.paramUnit     = LOC ("units.degrees");
                d.minValue      = -180.0f;
                d.maxValue      = 180.0f;
                d.step          = 5.0f;
                d.fineStep      = 1.0f;
                d.decimalPlaces = 0;
                d.type          = DialBinding::Float;

                d.getValue = []() { return 0.0f; };
                d.setValue = [callbacks, cluster] (float v)
                {
                    if (callbacks.rotateCluster)
                        callbacks.rotateCluster (cluster, v);
                };
            }
        }
        else
        {
            //--------------------------------------------------------------
            // Mode 1: No selection — Input/Cluster selectors
            //--------------------------------------------------------------

            // Dial 0: Input selector (ComboBox)
            {
                auto& d = sec.dials[0];
                d.paramName = LOC ("streamDeck.map.dials.selectInput");
                d.type      = DialBinding::ComboBox;

                for (int i = 0; i < numInputs; ++i)
                {
                    float px = static_cast<float> (state.getInputParameter (i, inputPositionX));
                    float py = static_cast<float> (state.getInputParameter (i, inputPositionY));
                    d.comboOptions.add ("Input " + juce::String (i + 1)
                                        + " (" + juce::String (px, 1)
                                        + ", " + juce::String (py, 1) + ")");
                }

                d.getValue = []() { return 0.0f; };
                d.setValue = [callbacks] (float v)
                {
                    int idx = juce::roundToInt (v);
                    if (callbacks.selectInput)
                        callbacks.selectInput (idx);
                };
            }

            // Dial 1: Cluster selector (ComboBox)
            {
                auto& d = sec.dials[1];
                d.paramName = LOC ("streamDeck.map.dials.selectCluster");
                d.type      = DialBinding::ComboBox;

                for (int c = 1; c <= 10; ++c)
                    d.comboOptions.add ("Cluster " + juce::String (c));

                d.getValue = []() { return 0.0f; };
                d.setValue = [callbacks] (float v)
                {
                    int clusterNum = juce::roundToInt (v) + 1;  // combo index 0 → cluster 1
                    if (callbacks.selectCluster)
                        callbacks.selectCluster (clusterNum);
                };
            }

            // Dial 2: Pan X (normal) / Pan Y (press+turn altBinding)
            {
                auto& d = sec.dials[2];
                d.paramName     = LOC ("streamDeck.map.dials.panX");
                d.paramUnit     = LOC ("units.meters");
                d.minValue      = -100.0f;
                d.maxValue      = 100.0f;
                d.step          = 0.5f;
                d.fineStep      = 0.0f;   // no fine — press+turn switches to Pan Y
                d.invertDirection = true;  // clockwise = pan right (decrease center X)
                d.decimalPlaces = 1;
                d.type          = DialBinding::Float;

                d.getValue = [callbacks]() { return callbacks.getViewCenterX ? callbacks.getViewCenterX() : 0.0f; };
                d.setValue = [callbacks] (float v) { if (callbacks.setViewCenterX) callbacks.setViewCenterX (v); };

                // Alt binding (press+turn): Pan Y
                auto alt = std::make_unique<DialBinding>();
                alt->paramName     = LOC ("streamDeck.map.dials.panY");
                alt->paramUnit     = LOC ("units.meters");
                alt->minValue      = -100.0f;
                alt->maxValue      = 100.0f;
                alt->step          = 0.5f;
                alt->fineStep      = 0.0f;   // already in press+turn mode
                alt->decimalPlaces = 1;
                alt->type          = DialBinding::Float;

                alt->getValue = [callbacks]() { return callbacks.getViewCenterY ? callbacks.getViewCenterY() : 0.0f; };
                alt->setValue = [callbacks] (float v) { if (callbacks.setViewCenterY) callbacks.setViewCenterY (v); };

                d.altBinding = std::move (alt);
            }

            // Dial 3: Zoom
            {
                auto& d = sec.dials[3];
                d.paramName     = LOC ("streamDeck.map.dials.zoom");
                d.paramUnit     = {};
                d.minValue      = 5.0f;
                d.maxValue      = 500.0f;
                d.step          = 10.0f;
                d.fineStep      = 2.0f;
                d.decimalPlaces = 0;
                d.type          = DialBinding::Float;

                d.getValue = [callbacks]() { return callbacks.getViewScale ? callbacks.getViewScale() : 30.0f; };
                d.setValue = [callbacks] (float v) { if (callbacks.setViewScale) callbacks.setViewScale (v); };
            }
        }
    }

    page.numSections = 1;
    page.activeSectionIndex = 0;

    return page;
}

//==============================================================================
// Factory
//==============================================================================

inline StreamDeckPage createPage (int subTabIndex,
                                   WFSValueTreeState& state,
                                   const MapCallbacks& callbacks,
                                   const MapStateQueries& queries,
                                   std::shared_ptr<bool> posOffsetMode)
{
    (void) subTabIndex;
    return createMapPage (state, callbacks, queries, posOffsetMode);
}

} // namespace MapTabPages
