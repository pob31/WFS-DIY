#pragma once

#include <JuceHeader.h>
#include <map>
#include "../WfsParameters.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/WFSParameterDefaults.h"
#include "../Parameters/WFSConstraints.h"
#include "ColorUtilities.h"
#include "ColorScheme.h"
#include "WfsLookAndFeel.h"
#include "StatusBar.h"
#include "../Helpers/CoordinateConverter.h"
#include "../Localization/LocalizationManager.h"

/**
 * Map Tab Component
 * Interactive 2D spatial mapping and visualization for WFS sources, speakers, and clusters.
 */
class MapTab : public juce::Component,
               private juce::ValueTree::Listener,
               private juce::Timer
{
public:
    MapTab(WfsParameters& params)
        : parameters(params),
          inputsTree(params.getInputTree()),
          outputsTree(params.getOutputTree()),
          reverbsTree(params.getReverbTree()),
          configTree(params.getConfigTree())
    {
        // Don't capture keyboard focus - let MainComponent handle keyboard shortcuts
        setWantsKeyboardFocus(false);

        // Add ValueTree listeners
        inputsTree.addListener(this);
        outputsTree.addListener(this);
        reverbsTree.addListener(this);
        configTree.addListener(this);

        // Home button to reset view to stage
        addAndMakeVisible(homeButton);
        homeButton.setButtonText(LOC("map.buttons.fitStage"));
        homeButton.onClick = [this]() {
            resetView();
            repaint();
        };

        // Fit all inputs button
        addAndMakeVisible(fitInputsButton);
        fitInputsButton.setButtonText(LOC("map.buttons.fitInputs"));
        fitInputsButton.onClick = [this]() {
            fitAllInputsToScreen();
            repaint();
        };

        // Level overlay toggle button
        addAndMakeVisible(levelOverlayButton);
        levelOverlayButton.setButtonText(LOC("map.buttons.showLevels"));
        levelOverlayButton.onClick = [this]() {
            levelOverlayEnabled = !levelOverlayEnabled;
            levelOverlayButton.setButtonText(levelOverlayEnabled
                ? LOC("map.buttons.hideLevels")
                : LOC("map.buttons.showLevels"));
            if (onLevelOverlayChanged)
                onLevelOverlayChanged(levelOverlayEnabled);
            repaint();
        };

        // Initialize view to center on stage
        resetView();

        // Setup help text for status bar
        setupHelpText();
        setupMouseListeners();
    }

    ~MapTab() override
    {
        stopTimer();
        inputsTree.removeListener(this);
        outputsTree.removeListener(this);
        reverbsTree.removeListener(this);
        configTree.removeListener(this);
    }

    void setStatusBar(StatusBar* bar)
    {
        statusBar = bar;
    }

    /** Get the currently selected input on the map (-1 if none) */
    int getSelectedInput() const { return selectedInput; }

    /** Get the currently selected cluster/barycenter on the map (-1 if none, 1-10 for cluster) */
    int getSelectedBarycenter() const { return selectedBarycenter; }

    /** Check if the user is currently dragging an input or barycenter (via touch/mouse) */
    bool getIsDragging() const { return isDraggingInput || isDraggingBarycenter; }

    /** Check if level overlay is enabled */
    bool getLevelOverlayEnabled() const { return levelOverlayEnabled; }

    //==========================================================================
    // Stream Deck programmatic control
    //==========================================================================

    /** Programmatically select an input on the map (for Stream Deck). */
    void selectInputProgrammatically (int inputIndex)
    {
        selectedInput = inputIndex;
        selectedBarycenter = -1;
        isDraggingInput = false;
        isDraggingBarycenter = false;
        repaint();
        if (onMapSelectionChanged) onMapSelectionChanged();
    }

    /** Programmatically deselect everything on the map (for Stream Deck). */
    void deselectAllProgrammatically()
    {
        selectedInput = -1;
        selectedBarycenter = -1;
        isDraggingInput = false;
        isDraggingBarycenter = false;
        repaint();
        if (onMapSelectionChanged) onMapSelectionChanged();
    }

    /** Programmatically select a cluster/barycenter on the map (for Stream Deck). */
    void selectClusterProgrammatically (int clusterNum)
    {
        selectedBarycenter = clusterNum;  // 1-10
        selectedInput = -1;
        isDraggingInput = false;
        isDraggingBarycenter = false;
        repaint();
        if (onMapSelectionChanged) onMapSelectionChanged();
    }

    /** Programmatically toggle the level overlay (for Stream Deck). */
    void toggleLevelOverlay()
    {
        levelOverlayEnabled = ! levelOverlayEnabled;
        levelOverlayButton.setButtonText (levelOverlayEnabled
            ? LOC ("map.buttons.hideLevels")
            : LOC ("map.buttons.showLevels"));
        if (onLevelOverlayChanged)
            onLevelOverlayChanged (levelOverlayEnabled);
        repaint();
    }

    /** Move cluster reference point (tracking-aware). For Stream Deck dials. */
    void moveClusterRefFromStreamDeck (int clusterNum, float newX, float newY)
    {
        int refInput = getClusterReferenceInput (clusterNum);
        if (refInput >= 0 && isInputFullyTracked (refInput))
            moveClusterRelative (clusterNum, refInput, { newX, newY });
        else if (refInput >= 0)
            moveClusterWithReference (clusterNum, refInput, { newX, newY });
        else
        {
            // Barycenter mode: move all members by delta from current barycenter
            auto bary = getClusterBarycenter (clusterNum);
            float dx = newX - bary.x;
            float dy = newY - bary.y;
            int n = parameters.getNumInputChannels();
            for (int i = 0; i < n; ++i)
            {
                if (static_cast<int> (parameters.getInputParam (i, "inputCluster")) == clusterNum)
                {
                    float mx = static_cast<float> (parameters.getInputParam (i, "inputPositionX"));
                    float my = static_cast<float> (parameters.getInputParam (i, "inputPositionY"));
                    parameters.setInputParam (i, "inputPositionX", mx + dx);
                    parameters.setInputParam (i, "inputPositionY", my + dy);
                }
            }
        }
        repaint();
    }

    /** Apply relative scale to cluster (for Stream Deck dial). */
    void scaleClusterFromStreamDeck (int clusterNum, float scale)
    {
        applyClusterScale (clusterNum, scale, scale);
        repaint();
    }

    /** Apply relative rotation to cluster (for Stream Deck dial). */
    void rotateClusterFromStreamDeck (int clusterNum, float angleDeg)
    {
        applyClusterRotation (clusterNum, angleDeg);
        repaint();
    }

    /** Get the effective reference position for a cluster. */
    juce::Point<float> getClusterRefPosition (int clusterNum) const
    {
        int refInput = getClusterReferenceInput (clusterNum);
        if (refInput >= 0)
        {
            float x = static_cast<float> (parameters.getInputParam (refInput, "inputPositionX"));
            float y = static_cast<float> (parameters.getInputParam (refInput, "inputPositionY"));
            if (isInputFullyTracked (refInput))
            {
                x += static_cast<float> (parameters.getInputParam (refInput, "inputOffsetX"));
                y += static_cast<float> (parameters.getInputParam (refInput, "inputOffsetY"));
            }
            return { x, y };
        }
        return getClusterBarycenter (clusterNum);
    }

    /** Reset the map view to show the entire stage (for Stream Deck). */
    void requestResetView() { resetView(); repaint(); }

    /** Fit all inputs to the screen (for Stream Deck). */
    void requestFitAllInputsToScreen() { fitAllInputsToScreen(); repaint(); }

    /** Set callback for map selection changes (for Stream Deck rebuild). */
    void setMapSelectionChangedCallback (std::function<void()> callback)
    {
        onMapSelectionChanged = std::move (callback);
    }

    void mouseEnter(const juce::MouseEvent& event) override
    {
        if (statusBar == nullptr) return;

        juce::Component* component = event.eventComponent;
        while (component != nullptr)
        {
            if (helpTextMap.find(component) != helpTextMap.end())
            {
                statusBar->setHelpText(helpTextMap[component]);
                return;
            }
            component = component->getParentComponent();
        }
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        if (statusBar != nullptr)
            statusBar->clearText();
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(ColorScheme::get().background);

        drawGrid(g);
        drawStageBounds(g);
        drawOriginMarker(g);
        drawOutputs(g);
        drawReverbs(g);
        drawClusters(g);
        drawInputs(g);
        drawSecondaryTouchFeedback(g);
    }

    void resized() override
    {
        const float us = WfsLookAndFeel::uiScale;
        markerRadius = juce::jmax(8.0f, 14.0f * us);

        // Position Show Levels button in top-left corner
        const int btnH = juce::jmax(24, static_cast<int>(30.0f * us));
        const int margin = juce::jmax(6, static_cast<int>(10.0f * us));
        levelOverlayButton.setBounds(margin, margin, juce::jmax(90, static_cast<int>(130.0f * us)), btnH);

        // Position fit buttons in top-right corner
        const int fitW = juce::jmax(100, static_cast<int>(150.0f * us));
        const int homeW = juce::jmax(90, static_cast<int>(135.0f * us));
        const int fitBtnGap = juce::jmax(3, static_cast<int>(5.0f * us));
        homeButton.setBounds(getWidth() - margin - homeW, margin, homeW, btnH);
        fitInputsButton.setBounds(getWidth() - margin - homeW - fitBtnGap - fitW, margin, fitW, btnH);

        // Reset view offset to center when resized
        if (viewOffset.isOrigin())
            resetView();
    }

    //==========================================================================
    // Mouse Handling
    //==========================================================================

    void mouseDown(const juce::MouseEvent& e) override
    {
        int sourceIndex = e.source.getIndex();

        // Handle touch input
        if (e.source.isTouch())
        {
            // Clear mouse selection when using touch
            selectedInput = -1;
            if (onMapSelectionChanged) onMapSelectionChanged();

            TouchInfo touch;
            touch.startPos = e.position;
            touch.currentPos = e.position;

            // Check for input hit
            int hitInput = getInputAtPosition(e.position);
            if (hitInput >= 0)
            {
                touch.type = TouchInfo::Type::Input;
                touch.targetIndex = hitInput;

                // Get flip-applied position to match visual coordinate system
                auto rawPos = getInputPosition(hitInput);
                bool flipX = static_cast<int>(parameters.getInputParam(hitInput, "inputFlipX")) != 0;
                bool flipY = static_cast<int>(parameters.getInputParam(hitInput, "inputFlipY")) != 0;
                touch.startStagePos = { flipX ? -rawPos.x : rawPos.x,
                                        flipY ? -rawPos.y : rawPos.y };

                float offsetX = static_cast<float>(parameters.getInputParam(hitInput, "inputOffsetX"));
                float offsetY = static_cast<float>(parameters.getInputParam(hitInput, "inputOffsetY"));
                touch.startOffset = { offsetX, offsetY };
                activeTouches[sourceIndex] = touch;

                // Set up long-press state for navigation
                longPressState.active = true;
                longPressState.targetType = LongPressState::TargetType::Input;
                longPressState.targetIndex = hitInput;
                longPressState.startPos = e.position;
                longPressState.startTime = juce::Time::getCurrentTime();

                // Notify path mode waypoint recording
                if (onDragStartCallback)
                    onDragStartCallback(hitInput);

                repaint();
                return;
            }

            // Check for hidden cluster reference marker hit
            auto [hitCluster, hitRefInput] = getHiddenClusterRefAtPosition(e.position);
            if (hitCluster > 0)
            {
                touch.type = TouchInfo::Type::Input;
                touch.targetIndex = hitRefInput;

                auto rawPos = getInputPosition(hitRefInput);
                bool flipX = static_cast<int>(parameters.getInputParam(hitRefInput, "inputFlipX")) != 0;
                bool flipY = static_cast<int>(parameters.getInputParam(hitRefInput, "inputFlipY")) != 0;
                touch.startStagePos = { flipX ? -rawPos.x : rawPos.x,
                                        flipY ? -rawPos.y : rawPos.y };

                float offsetX = static_cast<float>(parameters.getInputParam(hitRefInput, "inputOffsetX"));
                float offsetY = static_cast<float>(parameters.getInputParam(hitRefInput, "inputOffsetY"));
                touch.startOffset = { offsetX, offsetY };
                activeTouches[sourceIndex] = touch;

                // Set up long-press state for navigation to cluster
                longPressState.active = true;
                longPressState.targetType = LongPressState::TargetType::Cluster;
                longPressState.targetIndex = hitCluster;
                longPressState.startPos = e.position;
                longPressState.startTime = juce::Time::getCurrentTime();

                if (onDragStartCallback)
                    onDragStartCallback(hitRefInput);

                repaint();
                return;
            }

            // Check for barycenter hit
            int hitBarycenter = getBarycenterAtPosition(e.position);
            if (hitBarycenter > 0)
            {
                touch.type = TouchInfo::Type::Barycenter;
                touch.targetIndex = hitBarycenter;
                activeTouches[sourceIndex] = touch;

                // Set up long-press state for navigation
                longPressState.active = true;
                longPressState.targetType = LongPressState::TargetType::Cluster;
                longPressState.targetIndex = hitBarycenter;
                longPressState.startPos = e.position;
                longPressState.startTime = juce::Time::getCurrentTime();

                repaint();
                return;
            }

            // Check for output marker hit (not draggable, but long-press navigates)
            int hitOutput = getOutputAtPosition(e.position);
            if (hitOutput >= 0)
            {
                longPressState.active = true;
                longPressState.targetType = LongPressState::TargetType::Output;
                longPressState.targetIndex = hitOutput;
                longPressState.startPos = e.position;
                longPressState.startTime = juce::Time::getCurrentTime();
                repaint();
                return;
            }

            // Check for reverb marker hit (not draggable, but long-press navigates)
            int hitReverb = getReverbAtPosition(e.position);
            if (hitReverb >= 0)
            {
                longPressState.active = true;
                longPressState.targetType = LongPressState::TargetType::Reverb;
                longPressState.targetIndex = hitReverb;
                longPressState.startPos = e.position;
                longPressState.startTime = juce::Time::getCurrentTime();
                repaint();
                return;
            }

            // Touch on empty area - clear long-press state
            longPressState.active = false;
            int itemDraggingCount = countItemDraggingTouches();

            if (itemDraggingCount == 0)
            {
                // No items being dragged - this is a view gesture
                touch.type = TouchInfo::Type::ViewGesture;
                activeTouches[sourceIndex] = touch;

                int viewTouches = countViewGestureTouches();
                if (viewTouches == 2 || viewTouches == 3)
                    initializeViewGesture();

                repaint();
            }
            else
            {
                // Items being dragged - this could be a secondary touch
                auto [closestTarget, isClusterTarget] = findClosestSecondaryTouchTarget(e.position);

                if (closestTarget >= 0)
                {
                    // Create secondary touch
                    touch.type = TouchInfo::Type::SecondaryTouch;
                    touch.targetIndex = closestTarget;
                    activeTouches[sourceIndex] = touch;

                    // Initialize secondary touch info
                    if (isClusterTarget)
                    {
                        int primarySource = findPrimaryTouchForTarget(closestTarget, true);
                        activeSecondaryTouches[sourceIndex] = initSecondaryTouchForBarycenter(
                            closestTarget, primarySource, e.position);
                    }
                    else
                    {
                        int primarySource = findPrimaryTouchForTarget(closestTarget, false);
                        activeSecondaryTouches[sourceIndex] = initSecondaryTouchForInput(
                            closestTarget, primarySource, e.position);
                    }

                    repaint();
                }
            }
            return;
        }

        // Mouse input (existing behavior)
        lastMousePos = e.position;
        dragStartPos = e.position;
        dragStartOffset = viewOffset;
        dragStartScale = viewScale;

        // Middle click resets view to fit stage
        if (e.mods.isMiddleButtonDown())
        {
            resetView();
            repaint();
            return;
        }

        // Right-click (with or without left): enter view gesture mode
        if (e.mods.isRightButtonDown())
        {
            isDraggingInput = false;
            isDraggingBarycenter = false;
            isInViewGesture = true;

            if (e.mods.isLeftButtonDown())
                gestureMode = GestureMode::Zoom;
            else
                gestureMode = GestureMode::Pan;

            startTimer(50);
            return;
        }

        // Left-click only: check for input/barycenter/output/reverb hit
        if (e.mods.isLeftButtonDown())
        {
            int hitInput = getInputAtPosition(e.position);
            if (hitInput >= 0)
            {
                selectedInput = hitInput;
                isDraggingInput = true;
                isDraggingBarycenter = false;
                selectedBarycenter = -1;
                isInViewGesture = false;

                // Begin undo transaction for this map drag gesture
                parameters.getValueTreeState().beginUndoTransaction ("Map Drag Input " + juce::String (hitInput + 1));

                // Get flip-applied position to match visual coordinate system
                // (getInputPosition returns raw stored position, but drag works in visual coords)
                auto rawPos = getInputPosition(hitInput);
                bool flipX = static_cast<int>(parameters.getInputParam(hitInput, "inputFlipX")) != 0;
                bool flipY = static_cast<int>(parameters.getInputParam(hitInput, "inputFlipY")) != 0;
                inputDragStartStagePos = { flipX ? -rawPos.x : rawPos.x,
                                           flipY ? -rawPos.y : rawPos.y };

                float offsetX = static_cast<float>(parameters.getInputParam(hitInput, "inputOffsetX"));
                float offsetY = static_cast<float>(parameters.getInputParam(hitInput, "inputOffsetY"));
                inputDragStartOffset = { offsetX, offsetY };

                // Set up long-press state for navigation
                longPressState.active = true;
                longPressState.targetType = LongPressState::TargetType::Input;
                longPressState.targetIndex = hitInput;
                longPressState.startPos = e.position;
                longPressState.startTime = juce::Time::getCurrentTime();

                // Notify path mode waypoint recording
                if (onDragStartCallback)
                    onDragStartCallback(hitInput);

                if (onMapSelectionChanged) onMapSelectionChanged();
                repaint();
                return;
            }

            // Check for hidden cluster reference marker hit
            auto [hitCluster, hitRefInput] = getHiddenClusterRefAtPosition(e.position);
            if (hitCluster > 0)
            {
                selectedInput = hitRefInput;
                isDraggingInput = true;
                isDraggingBarycenter = false;
                selectedBarycenter = -1;
                isInViewGesture = false;

                auto rawPos = getInputPosition(hitRefInput);
                bool flipX = static_cast<int>(parameters.getInputParam(hitRefInput, "inputFlipX")) != 0;
                bool flipY = static_cast<int>(parameters.getInputParam(hitRefInput, "inputFlipY")) != 0;
                inputDragStartStagePos = { flipX ? -rawPos.x : rawPos.x,
                                           flipY ? -rawPos.y : rawPos.y };

                float offsetX = static_cast<float>(parameters.getInputParam(hitRefInput, "inputOffsetX"));
                float offsetY = static_cast<float>(parameters.getInputParam(hitRefInput, "inputOffsetY"));
                inputDragStartOffset = { offsetX, offsetY };

                // Set up long-press state for navigation to cluster
                longPressState.active = true;
                longPressState.targetType = LongPressState::TargetType::Cluster;
                longPressState.targetIndex = hitCluster;
                longPressState.startPos = e.position;
                longPressState.startTime = juce::Time::getCurrentTime();

                if (onDragStartCallback)
                    onDragStartCallback(hitRefInput);

                if (onMapSelectionChanged) onMapSelectionChanged();
                repaint();
                return;
            }

            int hitBarycenter = getBarycenterAtPosition(e.position);
            if (hitBarycenter > 0)
            {
                selectedBarycenter = hitBarycenter;
                isDraggingBarycenter = true;
                isDraggingInput = false;
                selectedInput = -1;
                isInViewGesture = false;
                barycenterDragStartStagePos = getClusterBarycenter(hitBarycenter);

                // Begin undo transaction for cluster barycenter drag
                parameters.getValueTreeState().beginUndoTransaction ("Map Drag Cluster " + juce::String (hitBarycenter));

                // Set up long-press state for navigation
                longPressState.active = true;
                longPressState.targetType = LongPressState::TargetType::Cluster;
                longPressState.targetIndex = hitBarycenter;
                longPressState.startPos = e.position;
                longPressState.startTime = juce::Time::getCurrentTime();

                if (onMapSelectionChanged) onMapSelectionChanged();
                repaint();
                return;
            }

            // Check for output marker hit (not draggable, but long-press navigates)
            int hitOutput = getOutputAtPosition(e.position);
            if (hitOutput >= 0)
            {
                longPressState.active = true;
                longPressState.targetType = LongPressState::TargetType::Output;
                longPressState.targetIndex = hitOutput;
                longPressState.startPos = e.position;
                longPressState.startTime = juce::Time::getCurrentTime();
                repaint();
                return;
            }

            // Check for reverb marker hit (not draggable, but long-press navigates)
            int hitReverb = getReverbAtPosition(e.position);
            if (hitReverb >= 0)
            {
                longPressState.active = true;
                longPressState.targetType = LongPressState::TargetType::Reverb;
                longPressState.targetIndex = hitReverb;
                longPressState.startPos = e.position;
                longPressState.startTime = juce::Time::getCurrentTime();
                repaint();
                return;
            }

            // Left-click in empty area
            selectedInput = -1;
            isDraggingInput = false;
            selectedBarycenter = -1;
            isDraggingBarycenter = false;
            isInViewGesture = true;
            gestureMode = GestureMode::None;
            longPressState.active = false;
            startTimer(50);
            if (onMapSelectionChanged) onMapSelectionChanged();
            repaint();
        }
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        int sourceIndex = e.source.getIndex();

        // Handle touch input
        if (e.source.isTouch())
        {
            auto it = activeTouches.find(sourceIndex);
            if (it == activeTouches.end())
                return;

            TouchInfo& touch = it->second;
            touch.currentPos = e.position;

            switch (touch.type)
            {
                case TouchInfo::Type::Input:
                    applyTouchInputDrag(touch);
                    repaint();
                    break;

                case TouchInfo::Type::Barycenter:
                    applyTouchBarycenterDrag(touch);
                    repaint();
                    break;

                case TouchInfo::Type::ViewGesture:
                {
                    int viewTouches = countViewGestureTouches();
                    if (viewTouches >= 2)
                        applyMultitouchPanZoom();
                    break;
                }

                case TouchInfo::Type::SecondaryTouch:
                    applySecondaryTouch(sourceIndex, e.position);
                    repaint();
                    break;

                default:
                    break;
            }
            return;
        }

        // Mouse input (existing behavior)
        lastMousePos = e.position;

        // If in view gesture mode (pan/zoom), apply gesture directly for smooth motion
        if (isInViewGesture)
        {
            if (gestureMode == GestureMode::Zoom)
            {
                auto delta = e.position - dragStartPos;
                float zoomFactor = 1.0f - delta.y * 0.005f;
                float newScale = dragStartScale * zoomFactor;
                viewScale = juce::jlimit(5.0f, 500.0f, newScale);
                repaint();
            }
            else if (gestureMode == GestureMode::Pan)
            {
                auto delta = e.position - dragStartPos;
                viewOffset = dragStartOffset + delta;
                repaint();
            }
            return;
        }

        // Handle input dragging
        if (isDraggingInput && selectedInput >= 0)
        {
            auto stagePos = screenToStage(e.position);

            // Apply position constraints (shared utility reads constraint flags from parameters)
            {
                float z = static_cast<float>(parameters.getInputParam(selectedInput, "inputPositionZ"));
                WFSConstraints::constrainPosition (parameters.getValueTreeState(), selectedInput,
                                                    stagePos.x, stagePos.y, z);
                // Note: Z may be modified by spherical distance constraint but not saved here (map is 2D)
            }

            // Determine drag behavior based on input state
            bool isTracked = isInputFullyTracked(selectedInput);
            int cluster = static_cast<int>(parameters.getInputParam(selectedInput, "inputCluster"));
            bool isReference = (cluster > 0) && (getClusterReferenceInput(cluster) == selectedInput);

            if (isTracked)
            {
                // Tracked input: update offset incrementally (delta from drag start)
                float deltaX = stagePos.x - inputDragStartStagePos.x;
                float deltaY = stagePos.y - inputDragStartStagePos.y;

                // Get current offset BEFORE updating (for cluster member delta calculation)
                float oldOffsetX = static_cast<float>(parameters.getInputParam(selectedInput, "inputOffsetX"));
                float oldOffsetY = static_cast<float>(parameters.getInputParam(selectedInput, "inputOffsetY"));

                // New offset = initial offset + drag delta
                float newOffsetX = inputDragStartOffset.x + deltaX;
                float newOffsetY = inputDragStartOffset.y + deltaY;

                parameters.setInputParam(selectedInput, "inputOffsetX", newOffsetX);
                parameters.setInputParam(selectedInput, "inputOffsetY", newOffsetY);

                // If tracked AND reference, move cluster members by the same frame delta
                if (isReference && cluster > 0)
                {
                    // Instantaneous delta = what the offset changed by this frame
                    float instantDeltaX = newOffsetX - oldOffsetX;
                    float instantDeltaY = newOffsetY - oldOffsetY;

                    // Move all other cluster members' positions by the same instantaneous delta
                    int numInputs = parameters.getNumInputChannels();
                    for (int i = 0; i < numInputs; ++i)
                    {
                        if (i == selectedInput) continue;

                        int inputCluster = static_cast<int>(parameters.getInputParam(i, "inputCluster"));
                        if (inputCluster == cluster)
                        {
                            float memberX = static_cast<float>(parameters.getInputParam(i, "inputPositionX"));
                            float memberY = static_cast<float>(parameters.getInputParam(i, "inputPositionY"));
                            parameters.setInputParam(i, "inputPositionX", memberX + instantDeltaX);
                            parameters.setInputParam(i, "inputPositionY", memberY + instantDeltaY);
                        }
                    }
                }
            }
            else if (isReference && cluster > 0)
            {
                // Reference input (not tracked): move entire cluster maintaining geometry
                moveClusterWithReference(cluster, selectedInput, stagePos);
            }
            else
            {
                // Normal input: just update position (no flip - marker shows at drag location)
                parameters.setInputParam(selectedInput, "inputPositionX", stagePos.x);
                parameters.setInputParam(selectedInput, "inputPositionY", stagePos.y);

                // Capture waypoint for path mode
                if (waypointCaptureCallback)
                {
                    float storedZ = static_cast<float>(parameters.getInputParam(selectedInput, "inputPositionZ"));
                    waypointCaptureCallback(selectedInput, stagePos.x, stagePos.y, storedZ);
                }
            }

            repaint();
        }
        else if (isDraggingBarycenter && selectedBarycenter > 0)
        {
            // Barycenter dragging: move all cluster members by the delta
            auto newStagePos = screenToStage(e.position);
            auto currentBarycenter = getClusterBarycenter(selectedBarycenter);

            float deltaX = newStagePos.x - currentBarycenter.x;
            float deltaY = newStagePos.y - currentBarycenter.y;

            // Move all cluster members
            int numInputs = parameters.getNumInputChannels();
            for (int i = 0; i < numInputs; ++i)
            {
                int inputCluster = static_cast<int>(parameters.getInputParam(i, "inputCluster"));
                if (inputCluster == selectedBarycenter)
                {
                    float memberX = static_cast<float>(parameters.getInputParam(i, "inputPositionX"));
                    float memberY = static_cast<float>(parameters.getInputParam(i, "inputPositionY"));
                    parameters.setInputParam(i, "inputPositionX", memberX + deltaX);
                    parameters.setInputParam(i, "inputPositionY", memberY + deltaY);
                }
            }

            repaint();
        }
        // Note: Pan/zoom handled by timer in timerCallback()
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        int sourceIndex = e.source.getIndex();

        // Handle touch input
        if (e.source.isTouch())
        {
            // Check for long-press gesture (navigation)
            // Note: Short double-tap (clear offsets) is handled immediately in mouseDoubleClick
            if (longPressState.active)
            {
                auto holdDuration = juce::Time::getCurrentTime() - longPressState.startTime;
                float movement = e.position.getDistanceFrom(longPressState.startPos);

                // Long hold (700-1200ms) with minimal movement (< 5px): navigate to tab
                if (holdDuration.inMilliseconds() >= 700 && holdDuration.inMilliseconds() <= 1200 && movement < 5.0f)
                {
                    if (navigateToItemCallback)
                    {
                        int tabType = -1;
                        switch (longPressState.targetType)
                        {
                            case LongPressState::TargetType::Input:   tabType = 0; break;
                            case LongPressState::TargetType::Cluster: tabType = 1; break;
                            case LongPressState::TargetType::Output:  tabType = 2; break;
                            case LongPressState::TargetType::Reverb:  tabType = 3; break;
                            default: break;
                        }
                        if (tabType >= 0)
                            navigateToItemCallback(tabType, longPressState.targetIndex);
                    }
                }

                longPressState.active = false;
            }

            auto it = activeTouches.find(sourceIndex);
            if (it != activeTouches.end())
            {
                bool wasViewGesture = (it->second.type == TouchInfo::Type::ViewGesture);
                bool wasSecondaryTouch = (it->second.type == TouchInfo::Type::SecondaryTouch);
                bool wasInputDrag = (it->second.type == TouchInfo::Type::Input);
                int draggedInputIndex = it->second.targetIndex;

                // Notify path mode that drag ended (before erasing touch)
                if (wasInputDrag && draggedInputIndex >= 0 && onDragEndCallback)
                    onDragEndCallback(draggedInputIndex);

                activeTouches.erase(it);

                // Clean up secondary touch data if this was a secondary touch
                if (wasSecondaryTouch)
                    activeSecondaryTouches.erase(sourceIndex);

                // If a view gesture touch was released, re-initialize gesture
                // (in case we went from 3 fingers to 2, need to restart pan)
                if (wasViewGesture && countViewGestureTouches() >= 2)
                    initializeViewGesture();
            }
            repaint();
            return;
        }

        // Mouse input (existing behavior)
        // Notify path mode that drag ended (before clearing state)
        if (isDraggingInput && selectedInput >= 0 && onDragEndCallback)
            onDragEndCallback(selectedInput);

        isDraggingInput = false;
        isDraggingBarycenter = false;
        isInViewGesture = false;
        gestureMode = GestureMode::None;
        stopTimer();

        // Notify Stream Deck that drag ended — page can now show dials
        if (onMapSelectionChanged)
            onMapSelectionChanged();

        // Check for long-press navigation gesture
        if (longPressState.active)
        {
            auto holdDuration = juce::Time::getCurrentTime() - longPressState.startTime;
            float movement = e.position.getDistanceFrom(longPressState.startPos);

            // Long hold (700-1200ms) with minimal movement (< 5px): navigate to tab
            if (holdDuration.inMilliseconds() >= 700 && holdDuration.inMilliseconds() <= 1200 && movement < 5.0f)
            {
                if (navigateToItemCallback)
                {
                    int tabType = -1;
                    switch (longPressState.targetType)
                    {
                        case LongPressState::TargetType::Input:   tabType = 0; break;
                        case LongPressState::TargetType::Cluster: tabType = 1; break;
                        case LongPressState::TargetType::Output:  tabType = 2; break;
                        case LongPressState::TargetType::Reverb:  tabType = 3; break;
                        default: break;
                    }
                    if (tabType >= 0)
                        navigateToItemCallback(tabType, longPressState.targetIndex);
                }
            }

            longPressState.active = false;
        }
    }

    void mouseDoubleClick(const juce::MouseEvent& e) override
    {
        // Double-tap/click on input marker: clear position offsets
        int hitInput = getInputAtPosition(e.position);
        if (hitInput >= 0)
        {
            parameters.setInputParam(hitInput, "inputOffsetX", 0.0f);
            parameters.setInputParam(hitInput, "inputOffsetY", 0.0f);
            parameters.setInputParam(hitInput, "inputOffsetZ", 0.0f);
            repaint();
            return;
        }

        // Double-tap/click on cluster barycenter: clear offsets for all cluster members
        int hitBarycenter = getBarycenterAtPosition(e.position);
        if (hitBarycenter > 0)
        {
            int numInputs = parameters.getNumInputChannels();
            for (int i = 0; i < numInputs; ++i)
            {
                int inputCluster = static_cast<int>(parameters.getInputParam(i, "inputCluster"));
                if (inputCluster == hitBarycenter)
                {
                    parameters.setInputParam(i, "inputOffsetX", 0.0f);
                    parameters.setInputParam(i, "inputOffsetY", 0.0f);
                    parameters.setInputParam(i, "inputOffsetZ", 0.0f);
                }
            }
            repaint();
        }
    }

    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override
    {
        // Zoom centered on mouse position
        auto mouseStagePos = screenToStage(e.position);

        float zoomFactor = 1.0f + wheel.deltaY * 0.1f;
        float newScale = viewScale * zoomFactor;
        viewScale = juce::jlimit(5.0f, 500.0f, newScale);

        // Adjust offset to keep mouse position stable
        auto newScreenPos = stageToScreen(mouseStagePos);
        viewOffset += e.position - newScreenPos;

        repaint();
    }

    //==========================================================================
    // Timer Callback for Polling Button State
    //==========================================================================

    void timerCallback() override
    {
        // Poll current mouse button state
        bool leftDown = juce::ModifierKeys::currentModifiers.isLeftButtonDown();
        bool rightDown = juce::ModifierKeys::currentModifiers.isRightButtonDown();

        // Determine target gesture mode based on button state
        GestureMode targetMode = GestureMode::None;
        if (leftDown && rightDown)
            targetMode = GestureMode::Zoom;
        else if (rightDown)
            targetMode = GestureMode::Pan;
        else if (leftDown)
            targetMode = GestureMode::None;  // Left only in view gesture = do nothing (wait for right)

        // Mode transition
        if (targetMode != gestureMode)
        {
            // Reset reference point when entering a new mode
            if (targetMode == GestureMode::Zoom)
            {
                dragStartPos = lastMousePos;
                dragStartScale = viewScale;
            }
            else if (targetMode == GestureMode::Pan)
            {
                dragStartPos = lastMousePos;
                dragStartOffset = viewOffset;
            }
            gestureMode = targetMode;
        }

        // Apply gesture
        if (gestureMode == GestureMode::Zoom)
        {
            auto delta = lastMousePos - dragStartPos;
            float zoomFactor = 1.0f - delta.y * 0.005f;
            float newScale = dragStartScale * zoomFactor;
            viewScale = juce::jlimit(5.0f, 500.0f, newScale);
            repaint();
        }
        else if (gestureMode == GestureMode::Pan)
        {
            auto delta = lastMousePos - dragStartPos;
            viewOffset = dragStartOffset + delta;
            repaint();
        }

        // Stop timer if no buttons are held
        if (!leftDown && !rightDown)
        {
            isInViewGesture = false;
            gestureMode = GestureMode::None;
            stopTimer();
        }
    }

    //==========================================================================
    // Multitouch Types and State (must be declared before helper functions)
    //==========================================================================

    // Per-touch tracking info
    struct TouchInfo
    {
        enum class Type { None, Input, Barycenter, ViewGesture, SecondaryTouch };
        Type type = Type::None;
        int targetIndex = -1;  // Input index or barycenter cluster number
        juce::Point<float> startPos;  // Screen position at touch start
        juce::Point<float> currentPos;  // Current screen position
        juce::Point<float> startStagePos;  // Stage position at touch start (for inputs)
        juce::Point<float> startOffset;  // Initial offset (for tracked inputs)
    };

    // Secondary touch tracking (for two-finger gestures on inputs/clusters)
    struct SecondaryTouchInfo
    {
        enum class TargetType { None, InputZ, InputRotation, ClusterScaleRotation };
        TargetType targetType = TargetType::None;
        int targetIndex = -1;           // Input index or cluster number
        int primaryTouchSourceIndex = -1;  // Source index of the primary touch this is associated with
        juce::Point<float> initialMarkerScreenPos;  // Marker position when touch started
        juce::Point<float> initialTouchScreenPos;   // Touch position at start
        float initialDistance = 0.0f;    // Initial distance from marker to touch
        float initialAngle = 0.0f;       // Initial angle from marker to touch
        float startZ = 0.0f;             // Initial Z value (position or offset)
        float startRotation = 0.0f;      // Initial rotation value (inputRotation or cluster rotation)
        float startScaleX = 1.0f;        // For cluster scale (not used for inputs)
        float startScaleY = 1.0f;        // For cluster scale (not used for inputs)
    };

    //==========================================================================
    // Multitouch Helpers
    //==========================================================================

    // Count touches that are dragging items (inputs or barycenters)
    int countItemDraggingTouches() const
    {
        int count = 0;
        for (const auto& [idx, touch] : activeTouches)
        {
            if (touch.type == TouchInfo::Type::Input ||
                touch.type == TouchInfo::Type::Barycenter)
                ++count;
        }
        return count;
    }

    // Check if a specific input is being touch-dragged
    bool isInputBeingTouchDragged(int inputIndex) const
    {
        for (const auto& [idx, touch] : activeTouches)
        {
            if (touch.type == TouchInfo::Type::Input && touch.targetIndex == inputIndex)
                return true;
        }
        return false;
    }

    // Count touches in view gesture mode
    int countViewGestureTouches() const
    {
        int count = 0;
        for (const auto& [idx, touch] : activeTouches)
        {
            if (touch.type == TouchInfo::Type::ViewGesture)
                ++count;
        }
        return count;
    }

    // Get center point of all view gesture touches
    juce::Point<float> getViewGestureTouchCenter() const
    {
        juce::Point<float> sum { 0.0f, 0.0f };
        int count = 0;
        for (const auto& [idx, touch] : activeTouches)
        {
            if (touch.type == TouchInfo::Type::ViewGesture)
            {
                sum += touch.currentPos;
                ++count;
            }
        }
        return count > 0 ? sum / static_cast<float>(count) : juce::Point<float>();
    }

    // Get average Y position of view gesture touches (for 3-finger zoom)
    float getViewGestureAverageY() const
    {
        float sum = 0.0f;
        int count = 0;
        for (const auto& [idx, touch] : activeTouches)
        {
            if (touch.type == TouchInfo::Type::ViewGesture)
            {
                sum += touch.currentPos.y;
                ++count;
            }
        }
        return count > 0 ? sum / static_cast<float>(count) : 0.0f;
    }

    // Get distance between view gesture touches (for pinch-to-zoom)
    float getViewGestureTouchSpan() const
    {
        std::vector<juce::Point<float>> positions;
        for (const auto& [idx, touch] : activeTouches)
        {
            if (touch.type == TouchInfo::Type::ViewGesture)
                positions.push_back(touch.currentPos);
        }
        if (positions.size() < 2)
            return 0.0f;
        return positions[0].getDistanceFrom(positions[1]);
    }

    // Initialize view gesture when touch count reaches 2 or more
    void initializeViewGesture()
    {
        viewGestureStartCenter = getViewGestureTouchCenter();
        viewGestureStartOffset = viewOffset;
        viewGestureStartScale = viewScale;
        viewGestureStartSpan = getViewGestureTouchSpan();
    }

    // Apply multitouch pan + zoom gesture (2+ fingers)
    void applyMultitouchPanZoom()
    {
        auto currentCenter = getViewGestureTouchCenter();
        float currentSpan = getViewGestureTouchSpan();

        // Apply zoom if span changed (pinch gesture)
        if (viewGestureStartSpan > 10.0f && currentSpan > 10.0f)
        {
            float zoomFactor = currentSpan / viewGestureStartSpan;
            float newScale = viewGestureStartScale * zoomFactor;
            viewScale = juce::jlimit(5.0f, 500.0f, newScale);
        }

        // Apply pan (center movement)
        auto delta = currentCenter - viewGestureStartCenter;
        viewOffset = viewGestureStartOffset + delta;

        repaint();
    }

    // Handle touch input dragging
    void applyTouchInputDrag(TouchInfo& touch)
    {
        int inputIdx = touch.targetIndex;
        auto stagePos = screenToStage(touch.currentPos);

        // Apply position constraints (shared utility reads constraint flags from parameters)
        {
            float z = static_cast<float>(parameters.getInputParam(inputIdx, "inputPositionZ"));
            WFSConstraints::constrainPosition (parameters.getValueTreeState(), inputIdx,
                                                stagePos.x, stagePos.y, z);
        }

        // Determine drag behavior
        bool isTracked = isInputFullyTracked(inputIdx);
        int cluster = static_cast<int>(parameters.getInputParam(inputIdx, "inputCluster"));
        bool isReference = (cluster > 0) && (getClusterReferenceInput(cluster) == inputIdx);

        if (isTracked)
        {
            // Tracked input: update offset incrementally
            float deltaX = stagePos.x - touch.startStagePos.x;
            float deltaY = stagePos.y - touch.startStagePos.y;

            float oldOffsetX = static_cast<float>(parameters.getInputParam(inputIdx, "inputOffsetX"));
            float oldOffsetY = static_cast<float>(parameters.getInputParam(inputIdx, "inputOffsetY"));

            float newOffsetX = touch.startOffset.x + deltaX;
            float newOffsetY = touch.startOffset.y + deltaY;

            parameters.setInputParam(inputIdx, "inputOffsetX", newOffsetX);
            parameters.setInputParam(inputIdx, "inputOffsetY", newOffsetY);

            if (isReference && cluster > 0)
            {
                float instantDeltaX = newOffsetX - oldOffsetX;
                float instantDeltaY = newOffsetY - oldOffsetY;

                int numInputs = parameters.getNumInputChannels();
                for (int i = 0; i < numInputs; ++i)
                {
                    if (i == inputIdx) continue;
                    int inputCluster = static_cast<int>(parameters.getInputParam(i, "inputCluster"));
                    if (inputCluster == cluster)
                    {
                        float memberX = static_cast<float>(parameters.getInputParam(i, "inputPositionX"));
                        float memberY = static_cast<float>(parameters.getInputParam(i, "inputPositionY"));
                        parameters.setInputParam(i, "inputPositionX", memberX + instantDeltaX);
                        parameters.setInputParam(i, "inputPositionY", memberY + instantDeltaY);
                    }
                }
            }
        }
        else if (isReference && cluster > 0)
        {
            moveClusterWithReference(cluster, inputIdx, stagePos);
        }
        else
        {
            // Note: Touch doesn't apply flip - it stores stagePos directly
            // This is different from mouse input which applies flip
            parameters.setInputParam(inputIdx, "inputPositionX", stagePos.x);
            parameters.setInputParam(inputIdx, "inputPositionY", stagePos.y);

            // Capture waypoint for path mode (read back stored values for consistency)
            if (waypointCaptureCallback)
            {
                float storedZ = static_cast<float>(parameters.getInputParam(inputIdx, "inputPositionZ"));
                waypointCaptureCallback(inputIdx, stagePos.x, stagePos.y, storedZ);
            }
        }
    }

    // Handle touch barycenter dragging
    void applyTouchBarycenterDrag(TouchInfo& touch)
    {
        int clusterNum = touch.targetIndex;
        auto newStagePos = screenToStage(touch.currentPos);
        auto currentBarycenter = getClusterBarycenter(clusterNum);

        float deltaX = newStagePos.x - currentBarycenter.x;
        float deltaY = newStagePos.y - currentBarycenter.y;

        int numInputs = parameters.getNumInputChannels();
        for (int i = 0; i < numInputs; ++i)
        {
            int inputCluster = static_cast<int>(parameters.getInputParam(i, "inputCluster"));
            if (inputCluster == clusterNum)
            {
                float memberX = static_cast<float>(parameters.getInputParam(i, "inputPositionX"));
                float memberY = static_cast<float>(parameters.getInputParam(i, "inputPositionY"));
                parameters.setInputParam(i, "inputPositionX", memberX + deltaX);
                parameters.setInputParam(i, "inputPositionY", memberY + deltaY);
            }
        }
    }

    //==========================================================================
    // Secondary Touch Helpers
    //==========================================================================

    // Check if an input is already engaged in a secondary touch
    bool isInputEngagedInSecondaryTouch(int inputIndex) const
    {
        for (const auto& [idx, secTouch] : activeSecondaryTouches)
        {
            if (secTouch.targetType != SecondaryTouchInfo::TargetType::ClusterScaleRotation &&
                secTouch.targetIndex == inputIndex)
                return true;
        }
        return false;
    }

    // Check if a cluster is already engaged in a secondary touch
    bool isClusterEngagedInSecondaryTouch(int clusterNum) const
    {
        for (const auto& [idx, secTouch] : activeSecondaryTouches)
        {
            if (secTouch.targetType == SecondaryTouchInfo::TargetType::ClusterScaleRotation &&
                secTouch.targetIndex == clusterNum)
                return true;
        }
        return false;
    }

    // Find the closest eligible target for a secondary touch
    // Returns: pair<target index, is cluster (true) or input (false)>
    // Returns {-1, false} if no eligible target found
    std::pair<int, bool> findClosestSecondaryTouchTarget(juce::Point<float> screenPos) const
    {
        float minDistance = (std::numeric_limits<float>::max)();
        int closestTarget = -1;
        bool isCluster = false;

        // Check dragging inputs
        for (const auto& [sourceIdx, touch] : activeTouches)
        {
            if (touch.type == TouchInfo::Type::Input)
            {
                int inputIdx = touch.targetIndex;

                // Skip if already engaged in secondary touch
                if (isInputEngagedInSecondaryTouch(inputIdx))
                    continue;

                // Get current input screen position
                float posX = static_cast<float>(parameters.getInputParam(inputIdx, "inputPositionX"));
                float posY = static_cast<float>(parameters.getInputParam(inputIdx, "inputPositionY"));
                auto markerScreen = stageToScreen({ posX, posY });

                float dist = screenPos.getDistanceFrom(markerScreen);
                if (dist < minDistance)
                {
                    minDistance = dist;
                    closestTarget = inputIdx;
                    isCluster = false;
                }
            }
            else if (touch.type == TouchInfo::Type::Barycenter)
            {
                int clusterNum = touch.targetIndex;

                // Skip if already engaged in secondary touch
                if (isClusterEngagedInSecondaryTouch(clusterNum))
                    continue;

                // Get barycenter screen position
                auto baryStage = getClusterBarycenter(clusterNum);
                auto baryScreen = stageToScreen(baryStage);

                float dist = screenPos.getDistanceFrom(baryScreen);
                if (dist < minDistance)
                {
                    minDistance = dist;
                    closestTarget = clusterNum;
                    isCluster = true;
                }
            }
        }

        return { closestTarget, isCluster };
    }

    // Find the primary touch source index for a given input or barycenter
    int findPrimaryTouchForTarget(int targetIndex, bool isBarycenter) const
    {
        for (const auto& [sourceIdx, touch] : activeTouches)
        {
            if (isBarycenter && touch.type == TouchInfo::Type::Barycenter && touch.targetIndex == targetIndex)
                return sourceIdx;
            if (!isBarycenter && touch.type == TouchInfo::Type::Input && touch.targetIndex == targetIndex)
                return sourceIdx;
        }
        return -1;
    }

    // Initialize a secondary touch for an input
    SecondaryTouchInfo initSecondaryTouchForInput(int inputIdx, int primarySourceIdx, juce::Point<float> touchScreenPos)
    {
        SecondaryTouchInfo secTouch;
        secTouch.targetIndex = inputIdx;
        secTouch.primaryTouchSourceIndex = primarySourceIdx;
        secTouch.initialTouchScreenPos = touchScreenPos;

        // Get input position
        float posX = static_cast<float>(parameters.getInputParam(inputIdx, "inputPositionX"));
        float posY = static_cast<float>(parameters.getInputParam(inputIdx, "inputPositionY"));
        secTouch.initialMarkerScreenPos = stageToScreen({ posX, posY });

        // Calculate initial distance and angle
        auto delta = touchScreenPos - secTouch.initialMarkerScreenPos;
        secTouch.initialDistance = delta.getDistanceFromOrigin();
        secTouch.initialAngle = std::atan2(delta.y, delta.x);

        // Determine target type based on input state
        bool isTracked = isInputFullyTracked(inputIdx);
        int cluster = static_cast<int>(parameters.getInputParam(inputIdx, "inputCluster"));
        bool isReference = (cluster > 0) && (getClusterReferenceInput(cluster) == inputIdx);

        if (isTracked && cluster > 0)
        {
            // Tracked input in cluster - controls cluster scale/rotation
            secTouch.targetType = SecondaryTouchInfo::TargetType::ClusterScaleRotation;
            secTouch.targetIndex = cluster;  // Change to cluster number
            secTouch.startRotation = 0.0f;  // Rotation is applied incrementally
        }
        else if (isReference && cluster > 0)
        {
            // Reference input (not tracked) - controls cluster scale/rotation
            secTouch.targetType = SecondaryTouchInfo::TargetType::ClusterScaleRotation;
            secTouch.targetIndex = cluster;
            secTouch.startRotation = 0.0f;
        }
        else if (isTracked)
        {
            // Tracked input not in cluster - controls Offset Z and inputRotation
            secTouch.targetType = SecondaryTouchInfo::TargetType::InputZ;
            secTouch.startZ = static_cast<float>(parameters.getInputParam(inputIdx, "inputOffsetZ"));
            secTouch.startRotation = static_cast<float>(parameters.getInputParam(inputIdx, "inputRotation"));
        }
        else
        {
            // Normal input - controls Position Z and inputRotation
            secTouch.targetType = SecondaryTouchInfo::TargetType::InputRotation;
            secTouch.startZ = static_cast<float>(parameters.getInputParam(inputIdx, "inputPositionZ"));
            secTouch.startRotation = static_cast<float>(parameters.getInputParam(inputIdx, "inputRotation"));
        }

        return secTouch;
    }

    // Initialize a secondary touch for a barycenter (cluster)
    SecondaryTouchInfo initSecondaryTouchForBarycenter(int clusterNum, int primarySourceIdx, juce::Point<float> touchScreenPos)
    {
        SecondaryTouchInfo secTouch;
        secTouch.targetType = SecondaryTouchInfo::TargetType::ClusterScaleRotation;
        secTouch.targetIndex = clusterNum;
        secTouch.primaryTouchSourceIndex = primarySourceIdx;
        secTouch.initialTouchScreenPos = touchScreenPos;

        // Get barycenter position
        auto baryStage = getClusterBarycenter(clusterNum);
        secTouch.initialMarkerScreenPos = stageToScreen(baryStage);

        // Calculate initial distance and angle
        auto delta = touchScreenPos - secTouch.initialMarkerScreenPos;
        secTouch.initialDistance = delta.getDistanceFromOrigin();
        secTouch.initialAngle = std::atan2(delta.y, delta.x);
        secTouch.startRotation = 0.0f;  // Cluster rotation is applied incrementally

        return secTouch;
    }

    // Apply secondary touch effects (called during drag)
    void applySecondaryTouch(int sourceIndex, juce::Point<float> currentTouchPos)
    {
        auto it = activeSecondaryTouches.find(sourceIndex);
        if (it == activeSecondaryTouches.end())
            return;

        SecondaryTouchInfo& secTouch = it->second;

        // Get current marker position (may have moved since touch started)
        juce::Point<float> currentMarkerScreen;
        if (secTouch.targetType == SecondaryTouchInfo::TargetType::ClusterScaleRotation)
        {
            auto baryStage = getClusterBarycenter(secTouch.targetIndex);
            currentMarkerScreen = stageToScreen(baryStage);
        }
        else
        {
            float posX = static_cast<float>(parameters.getInputParam(secTouch.targetIndex, "inputPositionX"));
            float posY = static_cast<float>(parameters.getInputParam(secTouch.targetIndex, "inputPositionY"));
            currentMarkerScreen = stageToScreen({ posX, posY });
        }

        // Calculate current distance and angle
        auto delta = currentTouchPos - currentMarkerScreen;
        float currentDistance = delta.getDistanceFromOrigin();
        float currentAngle = std::atan2(delta.y, delta.x);

        // Calculate ratio and rotation delta
        float distanceRatio = (secTouch.initialDistance > 10.0f) ? currentDistance / secTouch.initialDistance : 1.0f;
        float angleDelta = currentAngle - secTouch.initialAngle;

        // Normalize angle delta to -PI to PI
        while (angleDelta > juce::MathConstants<float>::pi) angleDelta -= juce::MathConstants<float>::twoPi;
        while (angleDelta < -juce::MathConstants<float>::pi) angleDelta += juce::MathConstants<float>::twoPi;

        switch (secTouch.targetType)
        {
            case SecondaryTouchInfo::TargetType::InputZ:
            {
                // Tracked input: modify offset Z by ratio, modify inputRotation by angle
                int inputIdx = secTouch.targetIndex;

                // For Z: if starting value is near 0, use additive offset instead of ratio
                // Scale factor converts screen distance change to meters (approximate)
                float newOffsetZ;
                if (std::abs(secTouch.startZ) < 0.1f)
                {
                    // Additive mode: distance change in pixels / 50 = meters offset
                    float distanceChange = currentDistance - secTouch.initialDistance;
                    newOffsetZ = secTouch.startZ + distanceChange / 50.0f;
                }
                else
                {
                    newOffsetZ = secTouch.startZ * distanceRatio;
                }

                // Check Z constraint - if enabled, limit to stage height
                int constraintZ = static_cast<int>(parameters.getInputParam(inputIdx, "inputConstraintZ"));
                if (constraintZ != 0)
                {
                    // Constrained: limit offset Z so total Z stays within stage height
                    float posZ = static_cast<float>(parameters.getInputParam(inputIdx, "inputPositionZ"));
                    float stageH = getStageHeight();
                    // Total Z = posZ + offsetZ must be in [0, stageH]
                    float minOffset = -posZ;
                    float maxOffset = stageH - posZ;
                    newOffsetZ = juce::jlimit(minOffset, maxOffset, newOffsetZ);
                }
                else
                {
                    // Unconstrained: use wide range
                    newOffsetZ = juce::jlimit(-20.0f, 20.0f, newOffsetZ);
                }
                parameters.setInputParam(inputIdx, "inputOffsetZ", newOffsetZ);

                float angleDeg = -juce::radiansToDegrees(angleDelta);
                float newRotation = secTouch.startRotation + angleDeg;
                // Wrap to -179 to 180
                while (newRotation > 180.0f) newRotation -= 360.0f;
                while (newRotation < -179.0f) newRotation += 360.0f;
                parameters.setInputParam(inputIdx, "inputRotation", static_cast<int>(newRotation));
                break;
            }

            case SecondaryTouchInfo::TargetType::InputRotation:
            {
                // Normal input: modify position Z by ratio, modify inputRotation by angle
                int inputIdx = secTouch.targetIndex;

                // For Z: if starting value is near 0, use additive offset instead of ratio
                float newPosZ;
                if (std::abs(secTouch.startZ) < 0.1f)
                {
                    // Additive mode: distance change in pixels / 50 = meters offset
                    float distanceChange = currentDistance - secTouch.initialDistance;
                    newPosZ = secTouch.startZ + distanceChange / 50.0f;
                }
                else
                {
                    newPosZ = secTouch.startZ * distanceRatio;
                }

                // Check Z constraint - if enabled, limit to stage height
                int constraintZ = static_cast<int>(parameters.getInputParam(inputIdx, "inputConstraintZ"));
                if (constraintZ != 0)
                {
                    // Constrained: limit position Z to stage height
                    float stageH = getStageHeight();
                    newPosZ = juce::jlimit(0.0f, stageH, newPosZ);
                }
                else
                {
                    // Unconstrained: use wide range
                    newPosZ = juce::jlimit(-20.0f, 20.0f, newPosZ);
                }
                parameters.setInputParam(inputIdx, "inputPositionZ", newPosZ);

                float angleDeg = -juce::radiansToDegrees(angleDelta);
                float newRotation = secTouch.startRotation + angleDeg;
                while (newRotation > 180.0f) newRotation -= 360.0f;
                while (newRotation < -179.0f) newRotation += 360.0f;
                parameters.setInputParam(inputIdx, "inputRotation", static_cast<int>(newRotation));
                break;
            }

            case SecondaryTouchInfo::TargetType::ClusterScaleRotation:
            {
                // Cluster: apply scale and rotation
                int clusterNum = secTouch.targetIndex;

                // Apply incremental scale based on distance ratio change
                // Use XY plane (matching ClustersTab default behavior)
                float scaleFactorX = distanceRatio;
                float scaleFactorY = distanceRatio;
                applyClusterScale(clusterNum, scaleFactorX, scaleFactorY);

                // Apply incremental rotation (negated: screen Y is down, stage Y is up)
                float angleDeg = -juce::radiansToDegrees(angleDelta);
                applyClusterRotation(clusterNum, angleDeg);

                // Update start values for next incremental update
                secTouch.initialDistance = currentDistance;
                secTouch.initialAngle = currentAngle;
                break;
            }

            default:
                break;
        }
    }

    // Apply scale to cluster members around reference point (XY plane)
    void applyClusterScale(int clusterNum, float scaleX, float scaleY)
    {
        // Limit scale factor to reasonable range to prevent extreme values
        scaleX = juce::jlimit(0.1f, 10.0f, scaleX);
        scaleY = juce::jlimit(0.1f, 10.0f, scaleY);

        // Find reference point
        int refInput = getClusterReferenceInput(clusterNum);
        juce::Point<float> refPos;

        if (refInput >= 0)
        {
            refPos.x = static_cast<float>(parameters.getInputParam(refInput, "inputPositionX"));
            refPos.y = static_cast<float>(parameters.getInputParam(refInput, "inputPositionY"));
        }
        else
        {
            refPos = getClusterBarycenter(clusterNum);
        }

        // Scale all members relative to reference
        int numInputs = parameters.getNumInputChannels();
        for (int i = 0; i < numInputs; ++i)
        {
            int inputCluster = static_cast<int>(parameters.getInputParam(i, "inputCluster"));
            if (inputCluster != clusterNum)
                continue;
            if (i == refInput)
                continue;  // Don't scale the reference input

            float posX = static_cast<float>(parameters.getInputParam(i, "inputPositionX"));
            float posY = static_cast<float>(parameters.getInputParam(i, "inputPositionY"));

            float relX = posX - refPos.x;
            float relY = posY - refPos.y;

            float newPosX = refPos.x + relX * scaleX;
            float newPosY = refPos.y + relY * scaleY;

            // Clamp to stage bounds (shared utility)
            auto scaleBounds = WFSConstraints::getStageBounds (parameters.getValueTreeState());
            newPosX = juce::jlimit(scaleBounds.minX, scaleBounds.maxX, newPosX);
            newPosY = juce::jlimit(scaleBounds.minY, scaleBounds.maxY, newPosY);

            parameters.setInputParam(i, "inputPositionX", newPosX);
            parameters.setInputParam(i, "inputPositionY", newPosY);
        }
    }

    // Apply rotation to cluster members around reference point (XY plane)
    void applyClusterRotation(int clusterNum, float angleDeg)
    {
        if (std::abs(angleDeg) < 0.01f)
            return;

        // Find reference point
        int refInput = getClusterReferenceInput(clusterNum);
        juce::Point<float> refPos;

        if (refInput >= 0)
        {
            refPos.x = static_cast<float>(parameters.getInputParam(refInput, "inputPositionX"));
            refPos.y = static_cast<float>(parameters.getInputParam(refInput, "inputPositionY"));
        }
        else
        {
            refPos = getClusterBarycenter(clusterNum);
        }

        float angleRad = juce::degreesToRadians(angleDeg);
        float cosA = std::cos(angleRad);
        float sinA = std::sin(angleRad);

        // Rotate all members around reference
        int numInputs = parameters.getNumInputChannels();
        for (int i = 0; i < numInputs; ++i)
        {
            int inputCluster = static_cast<int>(parameters.getInputParam(i, "inputCluster"));
            if (inputCluster != clusterNum)
                continue;
            if (i == refInput)
                continue;  // Don't rotate the reference input

            float posX = static_cast<float>(parameters.getInputParam(i, "inputPositionX"));
            float posY = static_cast<float>(parameters.getInputParam(i, "inputPositionY"));

            float relX = posX - refPos.x;
            float relY = posY - refPos.y;

            // Rotate
            float newRelX = relX * cosA - relY * sinA;
            float newRelY = relX * sinA + relY * cosA;

            float newPosX = refPos.x + newRelX;
            float newPosY = refPos.y + newRelY;

            // Clamp to stage bounds (shared utility)
            auto rotBounds = WFSConstraints::getStageBounds (parameters.getValueTreeState());
            newPosX = juce::jlimit(rotBounds.minX, rotBounds.maxX, newPosX);
            newPosY = juce::jlimit(rotBounds.minY, rotBounds.maxY, newPosY);

            parameters.setInputParam(i, "inputPositionX", newPosX);
            parameters.setInputParam(i, "inputPositionY", newPosY);
        }
    }

    // Get current marker screen position for secondary touch visual feedback
    juce::Point<float> getSecondaryTouchMarkerScreenPos(const SecondaryTouchInfo& secTouch) const
    {
        if (secTouch.targetType == SecondaryTouchInfo::TargetType::ClusterScaleRotation)
        {
            auto baryStage = getClusterBarycenter(secTouch.targetIndex);
            return stageToScreen(baryStage);
        }
        else
        {
            float posX = static_cast<float>(parameters.getInputParam(secTouch.targetIndex, "inputPositionX"));
            float posY = static_cast<float>(parameters.getInputParam(secTouch.targetIndex, "inputPositionY"));
            return stageToScreen({ posX, posY });
        }
    }

    //==========================================================================
    // LFO Offset Callback
    //==========================================================================

    /** Set callback to get LFO offsets for input visualization.
        The callback takes inputIndex and returns x, y, z offset values.
        If not set, LFO offsets are treated as zero. */
    void setLFOOffsetCallback(std::function<void(int, float&, float&, float&)> callback)
    {
        lfoOffsetCallback = std::move(callback);
    }

    //==========================================================================
    // Speed-Limited Position Callback
    //==========================================================================

    /** Set callback to get speed-limited positions for input visualization.
        The callback takes inputIndex and returns x, y, z position values.
        If not set, raw ValueTree positions are used. */
    void setSpeedLimitedPositionCallback(std::function<void(int, float&, float&, float&)> callback)
    {
        speedLimitedPositionCallback = std::move(callback);
    }

    /** Set callback for navigating to an item in another tab via long-press gesture.
        Parameters: (tabType, index) where tabType is: 0=Input, 1=Cluster, 2=Output, 3=Reverb */
    void setNavigateToItemCallback(std::function<void(int, int)> callback)
    {
        navigateToItemCallback = std::move(callback);
    }

    /** Set callback for when a drag operation starts on an input (for path mode waypoint recording) */
    void setDragStartCallback(std::function<void(int)> callback)
    {
        onDragStartCallback = std::move(callback);
    }

    /** Set callback for when a drag operation ends on an input (for path mode waypoint recording) */
    void setDragEndCallback(std::function<void(int)> callback)
    {
        onDragEndCallback = std::move(callback);
    }

    /** Set callback for capturing waypoints during drag (for path mode).
        Parameters: (inputIndex, x, y, z) - flip-adjusted coordinates */
    void setWaypointCaptureCallback(std::function<void(int, float, float, float)> callback)
    {
        waypointCaptureCallback = std::move(callback);
    }

    //==========================================================================
    // Level Overlay Callbacks
    //==========================================================================

    /** Set callback that will be called when level overlay is toggled */
    void setLevelOverlayChangedCallback(std::function<void(bool)> callback)
    {
        onLevelOverlayChanged = std::move(callback);
    }

    /** Set callback to get input peak level in dB for visualization */
    void setInputLevelCallback(std::function<float(int)> callback)
    {
        getInputLevelDb = std::move(callback);
    }

    /** Set callback to get output peak level in dB for visualization */
    void setOutputLevelCallback(std::function<float(int)> callback)
    {
        getOutputLevelDb = std::move(callback);
    }

    //==========================================================================
    // View accessors for Stream Deck+ pan/zoom
    //==========================================================================

    float getViewCenterX() const { return -viewOffset.x / viewScale; }
    float getViewCenterY() const { return viewOffset.y / viewScale; }
    void setViewCenterX (float x) { viewOffset.x = -x * viewScale; repaint(); }
    void setViewCenterY (float y) { viewOffset.y = y * viewScale; repaint(); }
    float getViewScale() const { return viewScale; }
    void setViewScale (float s) { viewScale = juce::jlimit (5.0f, 500.0f, s); repaint(); }

private:
    WfsParameters& parameters;
    juce::ValueTree inputsTree;
    juce::ValueTree outputsTree;
    juce::ValueTree reverbsTree;
    juce::ValueTree configTree;

    // LFO offset callback for visualization
    std::function<void(int, float&, float&, float&)> lfoOffsetCallback;

    // Speed-limited position callback for visualization
    std::function<void(int, float&, float&, float&)> speedLimitedPositionCallback;

    // Navigation callback for long-press gesture
    // Parameters: (tabType, index) where tabType is: 0=Input, 1=Cluster, 2=Output, 3=Reverb
    std::function<void(int, int)> navigateToItemCallback;

    // Path mode waypoint capture callbacks
    std::function<void(int)> onDragStartCallback;
    std::function<void(int)> onDragEndCallback;
    std::function<void(int, float, float, float)> waypointCaptureCallback;

    // Long-press gesture tracking (for navigating to item's tab)
    struct LongPressState
    {
        bool active = false;
        enum class TargetType { None, Input, Cluster, Output, Reverb };
        TargetType targetType = TargetType::None;
        int targetIndex = -1;
        juce::Point<float> startPos;
        juce::Time startTime;
    };
    LongPressState longPressState;

    // View state
    float viewScale = 30.0f;  // pixels per meter
    juce::Point<float> viewOffset { 0.0f, 0.0f };

    // Interaction state
    int selectedInput = -1;
    bool isDraggingInput = false;
    juce::Point<float> dragStartPos;
    juce::Point<float> dragStartOffset;
    float dragStartScale = 30.0f;  // For left+right zoom gesture
    juce::Point<float> inputDragStartStagePos;
    juce::Point<float> inputDragStartOffset;  // Initial offset at drag start (for tracked inputs)

    // View gesture state (pan/zoom when clicking on empty space - mouse only)
    enum class GestureMode { None, Pan, Zoom };
    GestureMode gestureMode = GestureMode::None;
    bool isInViewGesture = false;  // True when we started a gesture on empty space
    juce::Point<float> lastMousePos;  // For timer-based updates

    //==========================================================================
    // Multitouch State
    //==========================================================================

    std::map<int, TouchInfo> activeTouches;  // Keyed by source index
    std::map<int, SecondaryTouchInfo> activeSecondaryTouches;  // Keyed by touch source index

    // Multitouch view gesture state
    juce::Point<float> viewGestureStartCenter;
    juce::Point<float> viewGestureStartOffset;
    float viewGestureStartScale = 30.0f;
    float viewGestureStartSpan = 0.0f;  // For pinch-to-zoom

    // Barycenter dragging state
    int selectedBarycenter = -1;  // Cluster number (1-10), -1 if none
    bool isDraggingBarycenter = false;
    juce::Point<float> barycenterDragStartStagePos;

    // UI Components
    juce::TextButton homeButton;
    juce::TextButton fitInputsButton;
    juce::TextButton levelOverlayButton;

    // Level overlay state
    bool levelOverlayEnabled = false;

    // Map selection change callback (for Stream Deck rebuild)
    std::function<void()> onMapSelectionChanged;

    // Level overlay callbacks
    std::function<void(bool)> onLevelOverlayChanged;
    std::function<float(int)> getInputLevelDb;   // Returns peak dB for input index
    std::function<float(int)> getOutputLevelDb;  // Returns peak dB for output index

    // Status bar and help text
    StatusBar* statusBar = nullptr;
    std::map<juce::Component*, juce::String> helpTextMap;

    void setupHelpText()
    {
        helpTextMap[&homeButton] = LOC("map.tooltips.fitStage");
        helpTextMap[&fitInputsButton] = LOC("map.tooltips.fitInputs");
        helpTextMap[&levelOverlayButton] = LOC("map.tooltips.levels");
    }

    void setupMouseListeners()
    {
        for (auto& pair : helpTextMap)
            pair.first->addMouseListener(this, false);
    }

    // Constants
    float markerRadius = 14.0f;
    static constexpr float innerRadiusRatio = 0.6f;

    /** Convert level in dB to color for metering visualization.
        Green for quiet, yellow for moderate, red for loud. */
    static juce::Colour levelToColor(float db)
    {
        if (db < -12.0f)
            return juce::Colours::green;
        else if (db < -6.0f)
            return juce::Colours::yellow;
        else
            return juce::Colours::red;
    }

    /** Normalize level from dB to 0-1 range for visualization.
        -60 dB maps to 0, 0 dB maps to 1. */
    static float normalizeLevelDb(float db)
    {
        return juce::jlimit(0.0f, 1.0f, (db + 60.0f) / 60.0f);
    }

    //==========================================================================
    // Input State Detection Helpers
    //==========================================================================

    // Check if tracking is fully active for an input (all 3 conditions must be true)
    bool isInputFullyTracked(int inputIdx) const
    {
        int globalTracking = static_cast<int>(parameters.getConfigParam("trackingEnabled"));
        int protocolEnabled = static_cast<int>(parameters.getConfigParam("trackingProtocol"));
        int localTracking = static_cast<int>(parameters.getInputParam(inputIdx, "inputTrackingActive"));
        return (globalTracking != 0) && (protocolEnabled != 0) && (localTracking != 0);
    }

    // Get cluster reference input index (-1 if barycenter mode)
    // Priority: tracked input > first input (mode 0) > barycenter (mode 1)
    int getClusterReferenceInput(int clusterNum) const
    {
        std::vector<int> clusterMembers;
        int numInputs = parameters.getNumInputChannels();

        // Collect cluster members and check for tracked input
        for (int i = 0; i < numInputs; ++i)
        {
            int inputCluster = static_cast<int>(parameters.getInputParam(i, "inputCluster"));
            if (inputCluster == clusterNum)
            {
                clusterMembers.push_back(i);
                if (isInputFullyTracked(i))
                    return i;  // Tracked input takes highest priority
            }
        }

        // Check reference mode from cluster parameters
        int refMode = static_cast<int>(parameters.getValueTreeState().getClusterParameter(
            clusterNum, WFSParameterIDs::clusterReferenceMode));

        if (refMode == 0 && !clusterMembers.empty())
            return clusterMembers[0];  // First Input mode

        return -1;  // Barycenter mode
    }

    //==========================================================================
    // Cluster Movement Helpers
    //==========================================================================

    // Move cluster members when tracked reference is moved
    // Reference input changes offset, other members change position
    void moveClusterRelative(int clusterNum, int referenceInput, juce::Point<float> newRefPos)
    {
        float oldPosX = static_cast<float>(parameters.getInputParam(referenceInput, "inputPositionX"));
        float oldPosY = static_cast<float>(parameters.getInputParam(referenceInput, "inputPositionY"));
        float oldOffsetX = static_cast<float>(parameters.getInputParam(referenceInput, "inputOffsetX"));
        float oldOffsetY = static_cast<float>(parameters.getInputParam(referenceInput, "inputOffsetY"));

        // Effective old position was pos + offset
        float effectiveOldX = oldPosX + oldOffsetX;
        float effectiveOldY = oldPosY + oldOffsetY;

        // Delta from effective old to new
        float deltaX = newRefPos.x - effectiveOldX;
        float deltaY = newRefPos.y - effectiveOldY;

        // Move all other cluster members by updating their positions
        int numInputs = parameters.getNumInputChannels();
        for (int i = 0; i < numInputs; ++i)
        {
            if (i == referenceInput) continue;

            int inputCluster = static_cast<int>(parameters.getInputParam(i, "inputCluster"));
            if (inputCluster == clusterNum)
            {
                float memberX = static_cast<float>(parameters.getInputParam(i, "inputPositionX"));
                float memberY = static_cast<float>(parameters.getInputParam(i, "inputPositionY"));
                parameters.setInputParam(i, "inputPositionX", memberX + deltaX);
                parameters.setInputParam(i, "inputPositionY", memberY + deltaY);
            }
        }
    }

    // Move entire cluster when non-tracked reference is dragged
    void moveClusterWithReference(int clusterNum, int referenceInput, juce::Point<float> newRefPos)
    {
        // Get old position (no flip - marker displays at raw position)
        float oldPosX = static_cast<float>(parameters.getInputParam(referenceInput, "inputPositionX"));
        float oldPosY = static_cast<float>(parameters.getInputParam(referenceInput, "inputPositionY"));

        // Calculate delta from raw positions (no flip transformation)
        float deltaX = newRefPos.x - oldPosX;
        float deltaY = newRefPos.y - oldPosY;

        // Move ALL cluster members including reference by the same delta
        int numInputs = parameters.getNumInputChannels();
        for (int i = 0; i < numInputs; ++i)
        {
            int inputCluster = static_cast<int>(parameters.getInputParam(i, "inputCluster"));
            if (inputCluster == clusterNum)
            {
                float memberX = static_cast<float>(parameters.getInputParam(i, "inputPositionX"));
                float memberY = static_cast<float>(parameters.getInputParam(i, "inputPositionY"));
                parameters.setInputParam(i, "inputPositionX", memberX + deltaX);
                parameters.setInputParam(i, "inputPositionY", memberY + deltaY);
            }
        }
    }

    //==========================================================================
    // Coordinate Conversion
    //==========================================================================

    juce::Point<float> stageToScreen(juce::Point<float> stagePos) const
    {
        // Convert origin-relative stage coordinates to screen coordinates
        // Stage Y increases upward, screen Y increases downward
        // stagePos.x = 0 means at the coordinate origin (screen center)
        float screenX = getWidth() / 2.0f + stagePos.x * viewScale + viewOffset.x;
        float screenY = getHeight() / 2.0f - stagePos.y * viewScale + viewOffset.y;

        return { screenX, screenY };
    }

    juce::Point<float> screenToStage(juce::Point<float> screenPos) const
    {
        // Convert screen coordinates to origin-relative stage coordinates
        // Screen center maps to stagePos = (0, 0) = coordinate origin
        float stageX = (screenPos.x - getWidth() / 2.0f - viewOffset.x) / viewScale;
        float stageY = (getHeight() / 2.0f + viewOffset.y - screenPos.y) / viewScale;

        return { stageX, stageY };
    }

    //==========================================================================
    // Stage Dimension Helpers
    //==========================================================================

    float getStageWidth() const
    {
        auto stageTree = configTree.getChildWithName(WFSParameterIDs::Stage);
        if (stageTree.isValid())
            return static_cast<float>(stageTree.getProperty(WFSParameterIDs::stageWidth,
                                      WFSParameterDefaults::stageWidthDefault));
        return WFSParameterDefaults::stageWidthDefault;
    }

    float getStageDepth() const
    {
        auto stageTree = configTree.getChildWithName(WFSParameterIDs::Stage);
        if (stageTree.isValid())
            return static_cast<float>(stageTree.getProperty(WFSParameterIDs::stageDepth,
                                      WFSParameterDefaults::stageDepthDefault));
        return WFSParameterDefaults::stageDepthDefault;
    }

    float getOriginWidth() const
    {
        auto stageTree = configTree.getChildWithName(WFSParameterIDs::Stage);
        if (stageTree.isValid())
            return static_cast<float>(stageTree.getProperty(WFSParameterIDs::originWidth,
                                      WFSParameterDefaults::originWidthDefault));
        return WFSParameterDefaults::originWidthDefault;
    }

    float getOriginDepth() const
    {
        auto stageTree = configTree.getChildWithName(WFSParameterIDs::Stage);
        if (stageTree.isValid())
            return static_cast<float>(stageTree.getProperty(WFSParameterIDs::originDepth,
                                      WFSParameterDefaults::originDepthDefault));
        return WFSParameterDefaults::originDepthDefault;
    }

    float getStageHeight() const
    {
        auto stageTree = configTree.getChildWithName(WFSParameterIDs::Stage);
        if (stageTree.isValid())
            return static_cast<float>(stageTree.getProperty(WFSParameterIDs::stageHeight,
                                      WFSParameterDefaults::stageHeightDefault));
        return WFSParameterDefaults::stageHeightDefault;
    }

    int getStageShape() const
    {
        auto stageTree = configTree.getChildWithName(WFSParameterIDs::Stage);
        if (stageTree.isValid())
            return static_cast<int>(stageTree.getProperty(WFSParameterIDs::stageShape,
                                    WFSParameterDefaults::stageShapeDefault));
        return WFSParameterDefaults::stageShapeDefault;
    }

    float getStageDiameter() const
    {
        auto stageTree = configTree.getChildWithName(WFSParameterIDs::Stage);
        if (stageTree.isValid())
            return static_cast<float>(stageTree.getProperty(WFSParameterIDs::stageDiameter,
                                      WFSParameterDefaults::stageDiameterDefault));
        return WFSParameterDefaults::stageDiameterDefault;
    }

    // Origin-relative stage bounds (center-referenced for X/Y, floor-referenced for Z)
    // For circular shapes (cylinder/dome), use diameter instead of width/depth
    float getStageMinX() const
    {
        float halfSize = (getStageShape() == 0) ? getStageWidth() / 2.0f : getStageDiameter() / 2.0f;
        return -halfSize - getOriginWidth();
    }
    float getStageMaxX() const
    {
        float halfSize = (getStageShape() == 0) ? getStageWidth() / 2.0f : getStageDiameter() / 2.0f;
        return halfSize - getOriginWidth();
    }
    float getStageMinY() const
    {
        float halfSize = (getStageShape() == 0) ? getStageDepth() / 2.0f : getStageDiameter() / 2.0f;
        return -halfSize - getOriginDepth();
    }
    float getStageMaxY() const
    {
        float halfSize = (getStageShape() == 0) ? getStageDepth() / 2.0f : getStageDiameter() / 2.0f;
        return halfSize - getOriginDepth();
    }
    float getStageMinZ() const { return 0.0f; }
    float getStageMaxZ() const { return getStageHeight(); }

    void resetView()
    {
        // Reset view to show entire stage centered in viewport
        int shape = getStageShape();
        float stageW = (shape == 0) ? getStageWidth() : getStageDiameter();
        float stageD = (shape == 0) ? getStageDepth() : getStageDiameter();
        float originW = getOriginWidth();
        float originD = getOriginDepth();

        // Calculate scale to fit stage in view with some padding
        float scaleW = (getWidth() > 0) ? (getWidth() * 0.8f) / stageW : 30.0f;
        float scaleD = (getHeight() > 0) ? (getHeight() * 0.8f) / stageD : 30.0f;
        viewScale = juce::jmin(scaleW, scaleD);
        viewScale = juce::jlimit(5.0f, 500.0f, viewScale);

        // Calculate offset to center the stage (not the origin) in the viewport
        // With center-referenced system, stage center is at (-originW, -originD) in origin-relative coords
        // stageToScreen: screenX = width/2 + stageX * scale + offsetX
        // To center the stage: offsetX = -stageCenterX * scale = originW * scale
        viewOffset.x = originW * viewScale;
        viewOffset.y = -originD * viewScale;  // Y is inverted in screen coords
    }

    void fitAllInputsToScreen()
    {
        int numInputs = parameters.getNumInputChannels();
        float minX = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::lowest();
        float minY = std::numeric_limits<float>::max();
        float maxY = std::numeric_limits<float>::lowest();
        int visibleCount = 0;

        // Find bounding box of all visible inputs
        for (int i = 0; i < numInputs; ++i)
        {
            auto visibleVar = parameters.getInputParam(i, "inputMapVisible");
            bool visible = visibleVar.isVoid() || static_cast<int>(visibleVar) != 0;
            if (!visible)
                continue;

            float posX = static_cast<float>(parameters.getInputParam(i, "inputPositionX"));
            float posY = static_cast<float>(parameters.getInputParam(i, "inputPositionY"));
            minX = juce::jmin(minX, posX);
            maxX = juce::jmax(maxX, posX);
            minY = juce::jmin(minY, posY);
            maxY = juce::jmax(maxY, posY);
            visibleCount++;
        }

        if (visibleCount == 0)
        {
            resetView();  // Fall back to stage view if no visible inputs
            return;
        }

        // Add padding (1 meter around edges)
        const float padding = 1.0f;
        minX -= padding;
        maxX += padding;
        minY -= padding;
        maxY += padding;

        float boundsW = maxX - minX;
        float boundsH = maxY - minY;

        // Ensure minimum size to avoid extreme zoom
        boundsW = juce::jmax(boundsW, 2.0f);
        boundsH = juce::jmax(boundsH, 2.0f);

        // Calculate scale to fit bounds with 80% of viewport
        float scaleW = (getWidth() > 0) ? (getWidth() * 0.8f) / boundsW : 30.0f;
        float scaleH = (getHeight() > 0) ? (getHeight() * 0.8f) / boundsH : 30.0f;
        viewScale = juce::jmin(scaleW, scaleH);
        viewScale = juce::jlimit(5.0f, 500.0f, viewScale);

        // Calculate offset to center the bounding box
        float centerX = (minX + maxX) / 2.0f;
        float centerY = (minY + maxY) / 2.0f;
        viewOffset.x = -centerX * viewScale;
        viewOffset.y = centerY * viewScale;  // Y inverted in screen coords
    }

    //==========================================================================
    // Color Helpers (matching Android WFS Control app)
    //==========================================================================
    // NOTE: Color functions moved to ColorUtilities.h for shared use across components.
    // Use WfsColorUtilities::getMarkerColor(), getInputColor(), or getArrayColor() instead.

    //==========================================================================
    // Drawing Methods
    //==========================================================================

    void drawGrid(juce::Graphics& g)
    {
        g.setColour(juce::Colours::darkgrey);

        // Calculate visible stage area
        auto topLeft = screenToStage({ 0.0f, 0.0f });
        auto bottomRight = screenToStage({ static_cast<float>(getWidth()),
                                           static_cast<float>(getHeight()) });

        // Draw vertical lines (every 1 meter)
        int startX = static_cast<int>(std::floor(juce::jmin(topLeft.x, bottomRight.x)));
        int endX = static_cast<int>(std::ceil(juce::jmax(topLeft.x, bottomRight.x)));

        for (int x = startX; x <= endX; ++x)
        {
            auto screenX = stageToScreen({ static_cast<float>(x), 0.0f }).x;
            g.drawVerticalLine(static_cast<int>(screenX), 0.0f, static_cast<float>(getHeight()));
        }

        // Draw horizontal lines (every 1 meter)
        int startY = static_cast<int>(std::floor(juce::jmin(topLeft.y, bottomRight.y)));
        int endY = static_cast<int>(std::ceil(juce::jmax(topLeft.y, bottomRight.y)));

        for (int y = startY; y <= endY; ++y)
        {
            auto screenY = stageToScreen({ 0.0f, static_cast<float>(y) }).y;
            g.drawHorizontalLine(static_cast<int>(screenY), 0.0f, static_cast<float>(getWidth()));
        }
    }

    void drawStageBounds(juce::Graphics& g)
    {
        g.setColour(ColorScheme::get().textPrimary);

        int shape = getStageShape();
        if (shape == 0)  // Box - draw rectangle
        {
            auto topLeft = stageToScreen({ getStageMinX(), getStageMaxY() });
            auto bottomRight = stageToScreen({ getStageMaxX(), getStageMinY() });

            juce::Rectangle<float> stageRect(topLeft.x, topLeft.y,
                                             bottomRight.x - topLeft.x,
                                             bottomRight.y - topLeft.y);
            g.drawRect(stageRect, 2.0f);
        }
        else  // Cylinder or Dome - draw circle
        {
            // Circle center is at stage center (in origin-relative coords: -originW, -originD)
            float originW = getOriginWidth();
            float originD = getOriginDepth();
            auto center = stageToScreen({ -originW, -originD });

            // Circle radius in screen pixels
            float diameter = getStageDiameter();
            float radiusPixels = (diameter / 2.0f) * viewScale;

            g.drawEllipse(center.x - radiusPixels, center.y - radiusPixels,
                          radiusPixels * 2.0f, radiusPixels * 2.0f, 2.0f);
        }
    }

    void drawOriginMarker(juce::Graphics& g)
    {
        // Draw origin marker at (0, 0) in origin-relative coordinates
        auto originScreen = stageToScreen({ 0.0f, 0.0f });

        // Draw crosshairs
        float crosshairLength = 20.0f;
        g.setColour(ColorScheme::get().textPrimary.withAlpha(0.7f));
        g.drawLine(originScreen.x - crosshairLength, originScreen.y,
                   originScreen.x + crosshairLength, originScreen.y, 2.0f);
        g.drawLine(originScreen.x, originScreen.y - crosshairLength,
                   originScreen.x, originScreen.y + crosshairLength, 2.0f);

        // Draw small circle at origin
        g.drawEllipse(originScreen.x - 5.0f, originScreen.y - 5.0f, 10.0f, 10.0f, 2.0f);
    }

    void drawOutputs(juce::Graphics& g)
    {
        int numOutputs = parameters.getNumOutputChannels();

        for (int i = 0; i < numOutputs; ++i)
        {
            // Check visibility - individual or array-based
            int array = static_cast<int>(parameters.getOutputParam(i, "outputArray"));
            bool visible = true;

            if (array == 0)
            {
                // Single speaker - check individual visibility
                // For backwards compatibility, treat unset (void) as visible
                auto val = parameters.getOutputParam(i, "outputMapVisible");
                visible = val.isVoid() || static_cast<int>(val) != 0;
            }
            else
            {
                // Part of array - check array visibility
                auto val = parameters.getOutputParam(i, "outputArrayMapVisible");
                visible = val.isVoid() || static_cast<int>(val) != 0;
            }

            if (!visible)
                continue;

            // Output positions are already origin-relative (like inputs)
            // stageToScreen expects origin-relative coordinates
            float posX = static_cast<float>(parameters.getOutputParam(i, "outputPositionX"));
            float posY = static_cast<float>(parameters.getOutputParam(i, "outputPositionY"));
            int orientation = static_cast<int>(parameters.getOutputParam(i, "outputOrientation"));

            auto screenPos = stageToScreen({ posX, posY });

            // Get membrane color - array color if part of array, light grey otherwise
            juce::Colour membraneColor = (array == 0) ? juce::Colours::lightgrey : WfsColorUtilities::getArrayColor(array);

            // Draw speaker keystone shape showing orientation
            // Orientation 0° = pointing toward audience (down on screen, +Y in screen coords)
            // Orientation 180° = pointing toward back of stage (up on screen, -Y in screen coords)
            // Note: keystone uses inverted angle compared to arrow for correct visual
            float angleRad = juce::degreesToRadians(static_cast<float>(orientation) - 90.0f);

            // Direction vector (where speaker points)
            float dirX = std::cos(angleRad);
            float dirY = std::sin(angleRad);
            // Perpendicular vector (left side when facing direction)
            float perpX = -dirY;
            float perpY = dirX;

            // Keystone dimensions - wide base at back, narrow tip at front (1.5x size)
            float height = 24.0f;        // Total height from back to front
            float backWidth = 21.0f;     // Wide end (back/base)
            float frontWidth = 11.0f;    // Narrow end (front/tip) - slightly larger

            // Calculate the 4 corners of the trapezoid
            // Front (narrow end) - in the direction the speaker points
            float frontCenterX = screenPos.x + dirX * height * 0.5f;
            float frontCenterY = screenPos.y + dirY * height * 0.5f;
            // Back (wide end) - opposite direction
            float backCenterX = screenPos.x - dirX * height * 0.5f;
            float backCenterY = screenPos.y - dirY * height * 0.5f;

            // Four corners
            float frontLeftX = frontCenterX + perpX * frontWidth * 0.5f;
            float frontLeftY = frontCenterY + perpY * frontWidth * 0.5f;
            float frontRightX = frontCenterX - perpX * frontWidth * 0.5f;
            float frontRightY = frontCenterY - perpY * frontWidth * 0.5f;
            float backLeftX = backCenterX + perpX * backWidth * 0.5f;
            float backLeftY = backCenterY + perpY * backWidth * 0.5f;
            float backRightX = backCenterX - perpX * backWidth * 0.5f;
            float backRightY = backCenterY - perpY * backWidth * 0.5f;

            // Draw keystone (trapezoid)
            juce::Path keystone;
            keystone.startNewSubPath(backLeftX, backLeftY);
            keystone.lineTo(frontLeftX, frontLeftY);
            keystone.lineTo(frontRightX, frontRightY);
            keystone.lineTo(backRightX, backRightY);
            keystone.closeSubPath();

            g.setColour(ColorScheme::get().background);  // Fill with background color
            g.fillPath(keystone);
            g.setColour(ColorScheme::get().textPrimary);
            g.strokePath(keystone, juce::PathStrokeType(1.5f));

            // Draw membrane triangle - base corners at trapezoid back corners, tip toward front
            float membraneHeight = height * 0.55f;  // How far the tip extends toward front
            float memTipX = backCenterX + dirX * membraneHeight;
            float memTipY = backCenterY + dirY * membraneHeight;

            juce::Path membrane;
            membrane.startNewSubPath(backLeftX, backLeftY);   // Use trapezoid's back left corner
            membrane.lineTo(memTipX, memTipY);
            membrane.lineTo(backRightX, backRightY);          // Use trapezoid's back right corner
            membrane.closeSubPath();

            g.setColour(membraneColor);
            g.fillPath(membrane);
            g.setColour(ColorScheme::get().textPrimary);
            g.strokePath(membrane, juce::PathStrokeType(1.0f));

            // Draw channel number at center of membrane triangle (centroid)
            float triangleCenterX = (backLeftX + backRightX + memTipX) / 3.0f;
            float triangleCenterY = (backLeftY + backRightY + memTipY) / 3.0f;
            const float us = WfsLookAndFeel::uiScale;
            g.setFont(juce::FontOptions().withHeight(juce::jmax(8.0f, 12.0f * us)).withStyle("Bold"));
            g.setColour(juce::Colours::black);
            const int tw = static_cast<int>(juce::jmax(14.0f, 20.0f * us));
            const int th = static_cast<int>(juce::jmax(8.0f, 12.0f * us));
            g.drawText(juce::String(i + 1),
                       static_cast<int>(triangleCenterX) - tw / 2, static_cast<int>(triangleCenterY) - th / 2,
                       tw, th, juce::Justification::centred);

            // Draw level overlay if enabled
            if (levelOverlayEnabled && getOutputLevelDb)
            {
                float levelDb = getOutputLevelDb(i);
                float normalized = normalizeLevelDb(levelDb);

                if (normalized > 0.01f)  // Only draw if there's signal
                {
                    juce::Colour levelColor = levelToColor(levelDb);
                    g.setColour(levelColor.withAlpha(0.5f * normalized));
                    g.fillPath(keystone);

                    // Draw level ring around the speaker
                    float ringRadius = height * 0.5f + 4.0f + normalized * 6.0f;
                    g.setColour(levelColor.withAlpha(0.4f * normalized));
                    g.drawEllipse(screenPos.x - ringRadius, screenPos.y - ringRadius,
                                  ringRadius * 2, ringRadius * 2, 2.0f);
                }
            }
        }
    }

    void drawReverbs(juce::Graphics& g)
    {
        // Check global reverb visibility toggle
        auto reverbsVisibleVar = parameters.getConfigParam("reverbsMapVisible");
        bool reverbsVisible = reverbsVisibleVar.isVoid() || static_cast<int>(reverbsVisibleVar) != 0;
        if (!reverbsVisible)
            return;

        int numReverbs = parameters.getNumReverbChannels();

        for (int i = 0; i < numReverbs; ++i)
        {
            // Reverb positions are already origin-relative (like inputs)
            // stageToScreen expects origin-relative coordinates
            float posX = static_cast<float>(parameters.getReverbParam(i, "reverbPositionX"));
            float posY = static_cast<float>(parameters.getReverbParam(i, "reverbPositionY"));

            auto screenPos = stageToScreen({ posX, posY });

            // Draw diamond shape for reverb
            juce::Path diamond;
            float size = 10.0f;
            diamond.startNewSubPath(screenPos.x, screenPos.y - size);
            diamond.lineTo(screenPos.x + size, screenPos.y);
            diamond.lineTo(screenPos.x, screenPos.y + size);
            diamond.lineTo(screenPos.x - size, screenPos.y);
            diamond.closeSubPath();

            g.setColour(juce::Colour(0xFF9C27B0).withAlpha(0.6f));  // Purple, semi-transparent
            g.fillPath(diamond);
            g.setColour(juce::Colour(0xFF9C27B0));
            g.strokePath(diamond, juce::PathStrokeType(1.5f));

            // Draw channel number
            {
                const float us = WfsLookAndFeel::uiScale;
                g.setFont(juce::jmax(7.0f, 9.0f * us));
                g.setColour(ColorScheme::get().textPrimary);
                const int tw = static_cast<int>(juce::jmax(14.0f, 20.0f * us));
                const int th = static_cast<int>(juce::jmax(7.0f, 10.0f * us));
                g.drawText(LOC("map.labels.reverbPrefix") + juce::String(i + 1),
                           static_cast<int>(screenPos.x) - tw / 2, static_cast<int>(screenPos.y) - th / 2,
                           tw, th, juce::Justification::centred);
            }
        }
    }

    void drawClusters(juce::Graphics& g)
    {
        int numInputs = parameters.getNumInputChannels();

        // For each cluster (1-10), draw lines from reference to members
        for (int cluster = 1; cluster <= 10; ++cluster)
        {
            std::vector<int> allClusterMembers;     // For calculations (ALL inputs)
            std::vector<int> visibleClusterMembers; // For drawing lines (VISIBLE only)

            // Collect ALL cluster members and track which are visible
            for (int i = 0; i < numInputs; ++i)
            {
                int inputCluster = static_cast<int>(parameters.getInputParam(i, "inputCluster"));
                if (inputCluster == cluster)
                {
                    allClusterMembers.push_back(i);

                    auto visibleVar = parameters.getInputParam(i, "inputMapVisible");
                    bool visible = visibleVar.isVoid() || static_cast<int>(visibleVar) != 0;
                    if (visible)
                        visibleClusterMembers.push_back(i);
                }
            }

            // Need at least 2 total members to be a cluster (regardless of visibility)
            if (allClusterMembers.size() < 2)
                continue;

            // Find reference point
            juce::Point<float> refPos;
            int refInput = getClusterReferenceInput(cluster);

            if (refInput >= 0)
            {
                // Reference is a specific input
                float posX = static_cast<float>(parameters.getInputParam(refInput, "inputPositionX"));
                float posY = static_cast<float>(parameters.getInputParam(refInput, "inputPositionY"));
                refPos = stageToScreen({ posX, posY });

                // Draw cluster marker if reference input is hidden
                auto visibleVar = parameters.getInputParam(refInput, "inputMapVisible");
                bool refVisible = visibleVar.isVoid() || static_cast<int>(visibleVar) != 0;
                if (!refVisible)
                {
                    float clusterMarkerRadius = 10.0f;
                    bool isSelected = (selectedInput == refInput && isDraggingInput);

                    // Fill with cluster color
                    g.setColour(WfsColorUtilities::getMarkerColor(cluster, true));
                    g.fillEllipse(refPos.x - clusterMarkerRadius, refPos.y - clusterMarkerRadius,
                                  clusterMarkerRadius * 2, clusterMarkerRadius * 2);

                    // Selection highlight when dragging
                    if (isSelected)
                    {
                        g.setColour(juce::Colours::yellow);
                        g.drawEllipse(refPos.x - clusterMarkerRadius - 2, refPos.y - clusterMarkerRadius - 2,
                                      (clusterMarkerRadius + 2) * 2, (clusterMarkerRadius + 2) * 2, 2.0f);
                    }

                    // Draw cluster number in black
                    {
                        const float us = WfsLookAndFeel::uiScale;
                        g.setColour(juce::Colours::black);
                        g.setFont(juce::FontOptions().withHeight(juce::jmax(8.0f, 11.0f * us)).withStyle("Bold"));
                        const int tw = static_cast<int>(juce::jmax(11.0f, 16.0f * us));
                        const int th = static_cast<int>(juce::jmax(8.0f, 11.0f * us));
                        g.drawText(juce::String(cluster),
                                   static_cast<int>(refPos.x) - tw / 2, static_cast<int>(refPos.y) - th / 2,
                                   tw, th, juce::Justification::centred);
                    }
                }
            }
            else
            {
                // Barycenter mode - calculate center of mass using ALL members
                float sumX = 0, sumY = 0;
                for (int idx : allClusterMembers)
                {
                    sumX += static_cast<float>(parameters.getInputParam(idx, "inputPositionX"));
                    sumY += static_cast<float>(parameters.getInputParam(idx, "inputPositionY"));
                }
                float n = static_cast<float>(allClusterMembers.size());
                refPos = stageToScreen({ sumX / n, sumY / n });

                // Draw draggable barycenter marker
                float barycenterRadius = 10.0f;
                bool isSelected = (selectedBarycenter == cluster);

                // Fill with cluster color
                g.setColour(WfsColorUtilities::getMarkerColor(cluster, true));
                g.fillEllipse(refPos.x - barycenterRadius, refPos.y - barycenterRadius,
                              barycenterRadius * 2, barycenterRadius * 2);

                // Selection highlight when dragging
                if (isSelected && isDraggingBarycenter)
                {
                    g.setColour(juce::Colours::yellow);
                    g.drawEllipse(refPos.x - barycenterRadius - 2, refPos.y - barycenterRadius - 2,
                                  (barycenterRadius + 2) * 2, (barycenterRadius + 2) * 2, 2.0f);
                }

                // Draw cluster number in black
                {
                    const float us = WfsLookAndFeel::uiScale;
                    g.setColour(juce::Colours::black);
                    g.setFont(juce::FontOptions().withHeight(juce::jmax(8.0f, 11.0f * us)).withStyle("Bold"));
                    const int tw = static_cast<int>(juce::jmax(11.0f, 16.0f * us));
                    const int th = static_cast<int>(juce::jmax(8.0f, 11.0f * us));
                    g.drawText(juce::String(cluster),
                               static_cast<int>(refPos.x) - tw / 2,
                               static_cast<int>(refPos.y) - th / 2,
                               tw, th, juce::Justification::centred);
                }
            }

            // Draw lines from reference to each VISIBLE member
            g.setColour(WfsColorUtilities::getMarkerColor(cluster, true).withAlpha(0.4f));

            for (int idx : visibleClusterMembers)
            {
                if (idx == refInput)
                    continue;  // Don't draw line to itself

                float posX = static_cast<float>(parameters.getInputParam(idx, "inputPositionX"));
                float posY = static_cast<float>(parameters.getInputParam(idx, "inputPositionY"));
                auto memberPos = stageToScreen({ posX, posY });

                g.drawLine(refPos.x, refPos.y, memberPos.x, memberPos.y, 1.5f);
            }
        }
    }

    void drawInputs(juce::Graphics& g)
    {
        int numInputs = parameters.getNumInputChannels();

        // Draw inputs in reverse order so lower indices are on top
        for (int i = numInputs - 1; i >= 0; --i)
        {
            // Check visibility
            auto visibleVar = parameters.getInputParam(i, "inputMapVisible");
            bool visible = visibleVar.isVoid() || static_cast<int>(visibleVar) != 0;
            if (!visible)
                continue;

            drawInputMarker(g, i, i == selectedInput);
        }
    }

    void drawInputMarker(juce::Graphics& g, int inputIndex, bool isSelected)
    {
        // Get TARGET position from ValueTree (what user is controlling/dragging)
        float targetX = static_cast<float>(parameters.getInputParam(inputIndex, "inputPositionX"));
        float targetY = static_cast<float>(parameters.getInputParam(inputIndex, "inputPositionY"));
        float targetZ = static_cast<float>(parameters.getInputParam(inputIndex, "inputPositionZ"));

        // Main marker shows RAW position (no flip) - this is the control position
        float posX = targetX;
        float posY = targetY;

        // Get flip settings (affects DSP position, not display)
        bool flipX = static_cast<int>(parameters.getInputParam(inputIndex, "inputFlipX")) != 0;
        bool flipY = static_cast<int>(parameters.getInputParam(inputIndex, "inputFlipY")) != 0;

        // Get speed-limited position for grey dot (actual DSP position)
        float speedLimitedX = targetX, speedLimitedY = targetY, speedLimitedZ = targetZ;
        if (speedLimitedPositionCallback)
            speedLimitedPositionCallback(inputIndex, speedLimitedX, speedLimitedY, speedLimitedZ);

        // Apply flip to speed-limited position for grey dot (DSP position)
        float actualPosX = flipX ? -speedLimitedX : speedLimitedX;
        float actualPosY = flipY ? -speedLimitedY : speedLimitedY;

        float offsetX = static_cast<float>(parameters.getInputParam(inputIndex, "inputOffsetX"));
        float offsetY = static_cast<float>(parameters.getInputParam(inputIndex, "inputOffsetY"));

        int lsActive = static_cast<int>(parameters.getInputParam(inputIndex, "inputLSactive"));
        float lsRadius = static_cast<float>(parameters.getInputParam(inputIndex, "inputLSradius"));

        auto screenPos = stageToScreen({ posX, posY });

        // Check if input is locked on map
        auto lockedVar = parameters.getInputParam(inputIndex, "inputMapLocked");
        bool isLocked = !lockedVar.isVoid() && static_cast<int>(lockedVar) != 0;
        bool isBeingMouseDragged = isSelected && isDraggingInput;
        bool isBeingTouchDragged = isInputBeingTouchDragged(inputIndex);
        bool isBeingDragged = isBeingMouseDragged || isBeingTouchDragged;

        // Draw LS radius if active - radial gradient following the shape profile
        if (lsActive != 0)
        {
            float radiusPixels = lsRadius * viewScale;
            int lsShape = static_cast<int>(parameters.getInputParam(inputIndex, "inputLSshape"));

            // Get input marker color for the gradient
            juce::Colour inputColor = WfsColorUtilities::getInputColor(inputIndex + 1);
            const float baseAlpha = 0.25f;

            // Create radial gradient with shape-dependent color stops
            // Shape: 0=linear, 1=log, 2=square, 3=sine
            juce::ColourGradient gradient(
                inputColor.withAlpha(baseAlpha),  // Center color
                screenPos.x, screenPos.y,          // Center point
                inputColor.withAlpha(0.0f),       // Edge color (transparent)
                screenPos.x + radiusPixels, screenPos.y,  // Edge point
                true);  // isRadial = true

            // Add intermediate color stops based on shape profile
            // t = normalized distance (0 = center, 1 = edge)
            // Shape: 0=linear, 1=log, 2=square d^2, 3=sine (matches Max patch gate order)
            auto calcAlpha = [baseAlpha, lsShape](float t) -> float {
                float attenuation;
                switch (lsShape)
                {
                    case 1:  // Log: 1 - log10(1 + 9*t)
                        attenuation = 1.0f - std::log10(1.0f + 9.0f * t);
                        break;
                    case 2:  // Square (d^2): 1 - t^2
                        attenuation = 1.0f - t * t;
                        break;
                    case 3:  // Sine: 0.5 + 0.5*cos(t*pi)
                        attenuation = 0.5f + 0.5f * std::cos(t * juce::MathConstants<float>::pi);
                        break;
                    default: // Linear (case 0): 1 - t
                        attenuation = 1.0f - t;
                        break;
                }
                return baseAlpha * attenuation;
            };

            // Add color stops at regular intervals to approximate the curve
            for (float t = 0.1f; t < 1.0f; t += 0.1f)
                gradient.addColour(t, inputColor.withAlpha(calcAlpha(t)));

            // Draw gradient-filled disc
            g.setGradientFill(gradient);
            g.fillEllipse(screenPos.x - radiusPixels, screenPos.y - radiusPixels,
                          radiusPixels * 2, radiusPixels * 2);

            // Draw outline circle at radius edge
            g.setColour(inputColor.withAlpha(0.4f));
            g.drawEllipse(screenPos.x - radiusPixels, screenPos.y - radiusPixels,
                          radiusPixels * 2, radiusPixels * 2, 1.0f);
        }

        // Get LFO offset if callback is available
        float lfoOffsetX = 0.0f, lfoOffsetY = 0.0f, lfoOffsetZ = 0.0f;
        if (lfoOffsetCallback)
            lfoOffsetCallback(inputIndex, lfoOffsetX, lfoOffsetY, lfoOffsetZ);

        // Calculate total offset (tracking offset + LFO offset)
        float totalOffsetX = offsetX + lfoOffsetX;
        float totalOffsetY = offsetY + lfoOffsetY;

        // Calculate grey dot position: speed-limited position + flip + offset + LFO
        // This shows the ACTUAL DSP position (where the sound is)
        float greyDotX = actualPosX + totalOffsetX;
        float greyDotY = actualPosY + totalOffsetY;

        // Check if there's a reason to show the grey dot:
        // - Flip is active (DSP position is mirrored from control position)
        // - Speed limiting is active
        // - OR tracking offset is non-zero
        // - OR LFO offset is non-zero
        bool hasFlip = flipX || flipY;
        bool maxSpeedActive = static_cast<int>(parameters.getInputParam(inputIndex, "inputMaxSpeedActive")) != 0;
        bool hasOffset = (std::abs(offsetX) > 0.001f || std::abs(offsetY) > 0.001f);
        bool hasLFO = (std::abs(lfoOffsetX) > 0.001f || std::abs(lfoOffsetY) > 0.001f);
        bool hasReasonForDifference = hasFlip || maxSpeedActive || hasOffset || hasLFO;

        // Only show grey dot if there's a reason AND actual difference
        float diffFromTarget = std::abs(greyDotX - posX) + std::abs(greyDotY - posY);
        if (hasReasonForDifference && diffFromTarget > 0.01f)
        {
            auto compositePos = stageToScreen({ greyDotX, greyDotY });

            // Draw thin grey line from input to composite position
            g.setColour(juce::Colours::grey);
            g.drawLine(screenPos.x, screenPos.y, compositePos.x, compositePos.y, 1.0f);

            // Draw small grey circle at composite position (half the marker size)
            float compositeRadius = markerRadius * 0.5f;
            g.setColour(juce::Colours::grey);
            g.fillEllipse(compositePos.x - compositeRadius, compositePos.y - compositeRadius,
                          compositeRadius * 2, compositeRadius * 2);

            // Draw number in the composite circle (white text for better visibility)
            {
                const float us = WfsLookAndFeel::uiScale;
                g.setColour(juce::Colours::white);
                g.setFont(juce::FontOptions().withHeight(juce::jmax(8.0f, 11.0f * us)).withStyle("Bold"));
                const int tw = static_cast<int>(juce::jmax(14.0f, 20.0f * us));
                const int th = static_cast<int>(juce::jmax(8.0f, 11.0f * us));
                g.drawText(juce::String(inputIndex + 1),
                           static_cast<int>(compositePos.x) - tw / 2,
                           static_cast<int>(compositePos.y) - th / 2,
                           tw, th, juce::Justification::centred);
            }
        }

        // Determine input state for color coding
        bool isTracked = isInputFullyTracked(inputIndex);
        int cluster = static_cast<int>(parameters.getInputParam(inputIndex, "inputCluster"));
        bool isReference = (cluster > 0) && (getClusterReferenceInput(cluster) == inputIndex);

        // Determine colors based on state
        juce::Colour outerColor;
        juce::Colour labelColor;

        if (isLocked)
        {
            outerColor = juce::Colours::lightgrey;
            labelColor = juce::Colours::red;
        }
        else if (isBeingDragged)
        {
            outerColor = juce::Colours::white;
            // Keep state-based label color when dragging
            if (isTracked && isReference)
                labelColor = juce::Colour(0xFFADFF2F);  // Yellow-green for tracked+reference
            else if (isTracked)
                labelColor = juce::Colour(0xFF00FF00);  // Green for tracked
            else if (isReference)
                labelColor = juce::Colours::yellow;     // Yellow for reference
            else
                labelColor = juce::Colours::white;
        }
        else
        {
            outerColor = WfsColorUtilities::getInputColor(inputIndex + 1);  // 1-based for color

            // Label color based on tracking/reference state
            if (isTracked && isReference)
                labelColor = juce::Colour(0xFFADFF2F);  // Yellow-green for tracked+reference
            else if (isTracked)
                labelColor = juce::Colour(0xFF00FF00);  // Lime green for tracked only
            else if (isReference)
                labelColor = juce::Colours::yellow;     // Yellow for reference only
            else
                labelColor = juce::Colours::white;      // White for normal
        }

        // Draw outer circle
        g.setColour(outerColor);
        g.fillEllipse(screenPos.x - markerRadius, screenPos.y - markerRadius,
                      markerRadius * 2, markerRadius * 2);

        // Draw inner black circle
        float innerRadius = markerRadius * innerRadiusRatio;
        g.setColour(juce::Colours::black);
        g.fillEllipse(screenPos.x - innerRadius, screenPos.y - innerRadius,
                      innerRadius * 2, innerRadius * 2);

        // Draw channel number
        {
            const float us = WfsLookAndFeel::uiScale;
            const int th = static_cast<int>(juce::jmax(8.0f, 12.0f * us));
            g.setColour(labelColor);
            g.setFont(juce::FontOptions().withHeight(juce::jmax(8.0f, 12.0f * us)).withStyle("Bold"));
            g.drawText(juce::String(inputIndex + 1),
                       static_cast<int>(screenPos.x) - static_cast<int>(markerRadius),
                       static_cast<int>(screenPos.y) - th / 2,
                       static_cast<int>(markerRadius * 2), th,
                       juce::Justification::centred);
        }

        // Draw height indicator triangle if Z != 0
        if (std::abs(targetZ) > 0.01f)
        {
            juce::Path triangle;
            float triSize = 5.0f;
            float triOffset = markerRadius + 4.0f;

            if (targetZ > 0)  // Above ground
            {
                triangle.addTriangle(screenPos.x, screenPos.y - triOffset - triSize,
                                     screenPos.x - triSize, screenPos.y - triOffset,
                                     screenPos.x + triSize, screenPos.y - triOffset);
            }
            else  // Below ground
            {
                triangle.addTriangle(screenPos.x, screenPos.y + triOffset + triSize,
                                     screenPos.x - triSize, screenPos.y + triOffset,
                                     screenPos.x + triSize, screenPos.y + triOffset);
            }

            g.setColour(ColorScheme::get().textPrimary);
            g.fillPath(triangle);
        }

        // Selection highlight
        if (isSelected)
        {
            g.setColour(juce::Colours::yellow);
            g.drawEllipse(screenPos.x - markerRadius - 3, screenPos.y - markerRadius - 3,
                          (markerRadius + 3) * 2, (markerRadius + 3) * 2, 2.0f);
        }

        // Level overlay - pulsing ring around input
        if (levelOverlayEnabled && getInputLevelDb)
        {
            float levelDb = getInputLevelDb(inputIndex);
            float normalized = normalizeLevelDb(levelDb);

            if (normalized > 0.01f)  // Only draw if there's signal
            {
                juce::Colour levelColor = levelToColor(levelDb);

                // Draw pulsing ring with radius that scales with level
                float ringRadius = markerRadius + 4.0f + normalized * 10.0f;
                g.setColour(levelColor.withAlpha(0.6f * normalized));
                g.drawEllipse(screenPos.x - ringRadius, screenPos.y - ringRadius,
                              ringRadius * 2, ringRadius * 2, 2.5f);

                // Draw second outer ring for louder signals
                if (normalized > 0.3f)
                {
                    float outerRingRadius = ringRadius + 4.0f;
                    g.setColour(levelColor.withAlpha(0.3f * normalized));
                    g.drawEllipse(screenPos.x - outerRingRadius, screenPos.y - outerRingRadius,
                                  outerRingRadius * 2, outerRingRadius * 2, 1.5f);
                }
            }
        }

        // Show coordinates when dragging
        if (isBeingDragged)
        {
            // Get coordinate mode and format accordingly
            int mode = static_cast<int>(parameters.getInputParam(inputIndex, "inputCoordinateMode"));
            auto coordMode = static_cast<WFSCoordinates::Mode>(mode);
            juce::String coordText = WFSCoordinates::formatCoordinate(coordMode, posX, posY, targetZ);

            // Color coding: yellow for Cartesian, light blue for Cylindrical, pink for Spherical
            juce::Colour coordColor;
            if (coordMode == WFSCoordinates::Mode::Cylindrical)
                coordColor = juce::Colour(0xFF87CEEB);  // Light blue
            else if (coordMode == WFSCoordinates::Mode::Spherical)
                coordColor = juce::Colour(0xFFFFB6C1);  // Light pink
            else
                coordColor = juce::Colours::yellow;     // Cartesian

            {
                const float us = WfsLookAndFeel::uiScale;
                g.setColour(coordColor);
                g.setFont(juce::jmax(7.0f, 10.0f * us));
                const int ctw = static_cast<int>(juce::jmax(80.0f, 120.0f * us));
                const int cth = static_cast<int>(juce::jmax(8.0f, 12.0f * us));
                const int gap = static_cast<int>(juce::jmax(3.0f, 5.0f * us));

                // Position text to the right or left depending on screen position
                if (screenPos.x < getWidth() / 2)
                    g.drawText(coordText, static_cast<int>(screenPos.x + markerRadius + gap),
                               static_cast<int>(screenPos.y) - cth / 2, ctw, cth, juce::Justification::left);
                else
                    g.drawText(coordText, static_cast<int>(screenPos.x - markerRadius - gap - ctw),
                               static_cast<int>(screenPos.y) - cth / 2, ctw, cth, juce::Justification::right);
            }
        }

        // Draw input name beneath marker
        juce::String inputName = parameters.getInputParam(inputIndex, "inputName").toString();
        if (inputName.isEmpty())
            inputName = "Input " + juce::String(inputIndex + 1);

        {
            const float us = WfsLookAndFeel::uiScale;
            g.setColour(ColorScheme::get().textPrimary.withAlpha(0.8f));
            g.setFont(juce::jmax(7.0f, 9.0f * us));
            const int ntw = static_cast<int>(juce::jmax(55.0f, 80.0f * us));
            const int nth = static_cast<int>(juce::jmax(8.0f, 12.0f * us));
            g.drawText(inputName,
                       static_cast<int>(screenPos.x) - ntw / 2,
                       static_cast<int>(screenPos.y) + static_cast<int>(markerRadius) + 2,
                       ntw, nth, juce::Justification::centred);
        }
    }

    void drawSecondaryTouchFeedback(juce::Graphics& g)
    {
        for (const auto& [sourceIdx, secTouch] : activeSecondaryTouches)
        {
            // Get current touch position from activeTouches
            auto touchIt = activeTouches.find(sourceIdx);
            if (touchIt == activeTouches.end())
                continue;

            juce::Point<float> currentTouchPos = touchIt->second.currentPos;

            // Get current marker screen position (marker may have moved during drag)
            juce::Point<float> currentMarkerScreen = getSecondaryTouchMarkerScreenPos(secTouch);

            // Calculate the initial reference vector (from initial marker to initial touch)
            juce::Point<float> initialVector = secTouch.initialTouchScreenPos - secTouch.initialMarkerScreenPos;

            // Translate reference vector to current marker position
            juce::Point<float> referenceEndpoint = currentMarkerScreen + initialVector;

            // Draw grey reference line (initial vector translated to current marker position)
            g.setColour(juce::Colours::grey);
            g.drawLine(currentMarkerScreen.x, currentMarkerScreen.y,
                       referenceEndpoint.x, referenceEndpoint.y, 2.0f);

            // Draw small grey circle at reference endpoint
            g.fillEllipse(referenceEndpoint.x - 4, referenceEndpoint.y - 4, 8, 8);

            // Draw white active line (from current marker to current touch)
            g.setColour(juce::Colours::white);
            g.drawLine(currentMarkerScreen.x, currentMarkerScreen.y,
                       currentTouchPos.x, currentTouchPos.y, 2.0f);

            // Draw small white circle at current touch position
            g.fillEllipse(currentTouchPos.x - 5, currentTouchPos.y - 5, 10, 10);

            // Show Z and rotation values for input secondary touches
            if (secTouch.targetType == SecondaryTouchInfo::TargetType::InputZ ||
                secTouch.targetType == SecondaryTouchInfo::TargetType::InputRotation)
            {
                float currentZ;
                if (secTouch.targetType == SecondaryTouchInfo::TargetType::InputZ)
                    currentZ = static_cast<float>(parameters.getInputParam(secTouch.targetIndex, "inputOffsetZ"));
                else
                    currentZ = static_cast<float>(parameters.getInputParam(secTouch.targetIndex, "inputPositionZ"));

                int currentRotation = static_cast<int>(parameters.getInputParam(secTouch.targetIndex, "inputRotation"));

                juce::String displayText = "Z=" + juce::String(currentZ, 2) + "m  R="
                                         + juce::String(currentRotation) + juce::String(juce::CharPointer_UTF8("\xC2\xB0"));

                {
                    const float us = WfsLookAndFeel::uiScale;
                    g.setColour(juce::Colours::yellow);
                    g.setFont(juce::jmax(7.0f, 10.0f * us));
                    const int stw = static_cast<int>(juce::jmax(80.0f, 120.0f * us));
                    const int sth = static_cast<int>(juce::jmax(8.0f, 12.0f * us));
                    const int sGap = static_cast<int>(juce::jmax(5.0f, 8.0f * us));

                    // Position text near the initial contact point (grey circle)
                    // to avoid overlapping with coordinate text shown near the marker
                    if (referenceEndpoint.x < getWidth() / 2)
                        g.drawText(displayText, static_cast<int>(referenceEndpoint.x + sGap),
                                   static_cast<int>(referenceEndpoint.y) - sth / 2, stw, sth, juce::Justification::left);
                    else
                        g.drawText(displayText, static_cast<int>(referenceEndpoint.x - sGap - stw),
                                   static_cast<int>(referenceEndpoint.y) - sth / 2, stw, sth, juce::Justification::right);
                }
            }
        }
    }

    //==========================================================================
    // Hit Testing
    //==========================================================================

    int getInputAtPosition(juce::Point<float> screenPos) const
    {
        int numInputs = parameters.getNumInputChannels();
        float pickupRadius = markerRadius * 2.5f;  // Generous for touch targeting

        int bestIndex = -1;
        float bestDistance = pickupRadius + 1.0f;

        for (int i = 0; i < numInputs; ++i)
        {
            // Skip hidden inputs
            auto visibleVar = parameters.getInputParam(i, "inputMapVisible");
            bool visible = visibleVar.isVoid() || static_cast<int>(visibleVar) != 0;
            if (!visible)
                continue;

            // Skip locked inputs (can't be dragged)
            auto lockedVar = parameters.getInputParam(i, "inputMapLocked");
            bool isLocked = !lockedVar.isVoid() && static_cast<int>(lockedVar) != 0;
            if (isLocked)
                continue;

            // Get TARGET position from ValueTree (main marker is displayed here)
            float posX = static_cast<float>(parameters.getInputParam(i, "inputPositionX"));
            float posY = static_cast<float>(parameters.getInputParam(i, "inputPositionY"));

            // Hit-test at raw position (no flip) - this is where the marker is drawn
            auto markerScreenPos = stageToScreen({ posX, posY });
            float distance = screenPos.getDistanceFrom(markerScreenPos);

            if (distance <= pickupRadius && distance < bestDistance)
            {
                bestDistance = distance;
                bestIndex = i;
            }
        }

        return bestIndex;
    }

    juce::Point<float> getInputPosition(int inputIndex) const
    {
        float posX = static_cast<float>(parameters.getInputParam(inputIndex, "inputPositionX"));
        float posY = static_cast<float>(parameters.getInputParam(inputIndex, "inputPositionY"));
        return { posX, posY };
    }

    // Check if a screen position hits a barycenter marker
    // Returns cluster number (1-10) if hit, -1 if none
    int getBarycenterAtPosition(juce::Point<float> screenPos) const
    {
        int numInputs = parameters.getNumInputChannels();
        float pickupRadius = 10.0f * 2.5f;  // Generous for touch targeting

        int bestCluster = -1;
        float bestDistance = pickupRadius + 1.0f;

        for (int cluster = 1; cluster <= 10; ++cluster)
        {
            // Check if this cluster is in barycenter mode
            int refInput = getClusterReferenceInput(cluster);
            if (refInput >= 0)
                continue;  // Not barycenter mode

            // Collect ALL cluster members (regardless of visibility)
            std::vector<int> clusterMembers;
            for (int i = 0; i < numInputs; ++i)
            {
                int inputCluster = static_cast<int>(parameters.getInputParam(i, "inputCluster"));
                if (inputCluster == cluster)
                    clusterMembers.push_back(i);
            }

            // Need at least 2 total members for barycenter hit test
            if (clusterMembers.size() < 2)
                continue;

            // Calculate barycenter
            float sumX = 0, sumY = 0;
            for (int idx : clusterMembers)
            {
                sumX += static_cast<float>(parameters.getInputParam(idx, "inputPositionX"));
                sumY += static_cast<float>(parameters.getInputParam(idx, "inputPositionY"));
            }
            float n = static_cast<float>(clusterMembers.size());
            auto barycenterScreen = stageToScreen({ sumX / n, sumY / n });

            float distance = screenPos.getDistanceFrom(barycenterScreen);
            if (distance <= pickupRadius && distance < bestDistance)
            {
                bestDistance = distance;
                bestCluster = cluster;
            }
        }

        return bestCluster;
    }

    // Check if a screen position hits a cluster marker for First Input/Tracked mode
    // when the reference input is hidden.
    // Returns {cluster, refInput} or {-1, -1} if none.
    std::pair<int, int> getHiddenClusterRefAtPosition(juce::Point<float> screenPos) const
    {
        int numInputs = parameters.getNumInputChannels();
        float pickupRadius = 10.0f * 2.5f;  // Generous for touch targeting

        int bestCluster = -1;
        int bestRefInput = -1;
        float bestDistance = pickupRadius + 1.0f;

        for (int cluster = 1; cluster <= 10; ++cluster)
        {
            int refInput = getClusterReferenceInput(cluster);
            if (refInput < 0)
                continue;  // Barycenter mode - handled by getBarycenterAtPosition()

            // Check if reference input is hidden
            auto visibleVar = parameters.getInputParam(refInput, "inputMapVisible");
            bool refVisible = visibleVar.isVoid() || static_cast<int>(visibleVar) != 0;
            if (refVisible)
                continue;  // Visible inputs are hit-tested by getInputAtPosition()

            // Count total cluster members to verify it's a valid cluster
            int memberCount = 0;
            for (int i = 0; i < numInputs; ++i)
            {
                int inputCluster = static_cast<int>(parameters.getInputParam(i, "inputCluster"));
                if (inputCluster == cluster)
                    memberCount++;
            }
            if (memberCount < 2)
                continue;

            // Get reference position
            float posX = static_cast<float>(parameters.getInputParam(refInput, "inputPositionX"));
            float posY = static_cast<float>(parameters.getInputParam(refInput, "inputPositionY"));
            auto markerScreen = stageToScreen({ posX, posY });

            float distance = screenPos.getDistanceFrom(markerScreen);
            if (distance <= pickupRadius && distance < bestDistance)
            {
                bestDistance = distance;
                bestCluster = cluster;
                bestRefInput = refInput;
            }
        }

        return { bestCluster, bestRefInput };
    }

    // Check if a screen position hits an output marker
    // Returns output index (0-based) if hit, -1 if none
    int getOutputAtPosition(juce::Point<float> screenPos) const
    {
        int numOutputs = parameters.getNumOutputChannels();
        float pickupRadius = markerRadius * 2.0f;  // Generous for touch targeting

        int bestIndex = -1;
        float bestDistance = pickupRadius + 1.0f;

        for (int i = 0; i < numOutputs; ++i)
        {
            // Check visibility
            int array = static_cast<int>(parameters.getOutputParam(i, "outputArray"));
            bool visible = true;
            if (array == 0)
            {
                auto val = parameters.getOutputParam(i, "outputMapVisible");
                visible = val.isVoid() || static_cast<int>(val) != 0;
            }
            else
            {
                auto val = parameters.getOutputParam(i, "outputArrayMapVisible");
                visible = val.isVoid() || static_cast<int>(val) != 0;
            }
            if (!visible)
                continue;

            float posX = static_cast<float>(parameters.getOutputParam(i, "outputPositionX"));
            float posY = static_cast<float>(parameters.getOutputParam(i, "outputPositionY"));
            auto markerScreenPos = stageToScreen({ posX, posY });

            float distance = screenPos.getDistanceFrom(markerScreenPos);
            if (distance <= pickupRadius && distance < bestDistance)
            {
                bestDistance = distance;
                bestIndex = i;
            }
        }

        return bestIndex;
    }

    // Check if a screen position hits a reverb marker
    // Returns reverb index (0-based) if hit, -1 if none
    int getReverbAtPosition(juce::Point<float> screenPos) const
    {
        // Check global reverb visibility toggle
        auto reverbsVisibleVar = parameters.getConfigParam("reverbsMapVisible");
        bool reverbsVisible = reverbsVisibleVar.isVoid() || static_cast<int>(reverbsVisibleVar) != 0;
        if (!reverbsVisible)
            return -1;

        int numReverbs = parameters.getNumReverbChannels();
        float pickupRadius = markerRadius * 2.0f;  // Generous for touch targeting

        int bestIndex = -1;
        float bestDistance = pickupRadius + 1.0f;

        for (int i = 0; i < numReverbs; ++i)
        {
            float posX = static_cast<float>(parameters.getReverbParam(i, "reverbPositionX"));
            float posY = static_cast<float>(parameters.getReverbParam(i, "reverbPositionY"));
            auto markerScreenPos = stageToScreen({ posX, posY });

            float distance = screenPos.getDistanceFrom(markerScreenPos);
            if (distance <= pickupRadius && distance < bestDistance)
            {
                bestDistance = distance;
                bestIndex = i;
            }
        }

        return bestIndex;
    }

    // Get barycenter position in stage coordinates for a cluster
    juce::Point<float> getClusterBarycenter(int clusterNum) const
    {
        int numInputs = parameters.getNumInputChannels();
        float sumX = 0, sumY = 0;
        int count = 0;

        for (int i = 0; i < numInputs; ++i)
        {
            int inputCluster = static_cast<int>(parameters.getInputParam(i, "inputCluster"));
            if (inputCluster == clusterNum)
            {
                sumX += static_cast<float>(parameters.getInputParam(i, "inputPositionX"));
                sumY += static_cast<float>(parameters.getInputParam(i, "inputPositionY"));
                count++;
            }
        }

        if (count > 0)
            return { sumX / count, sumY / count };
        return { 0.0f, 0.0f };
    }

    //==========================================================================
    // ValueTree::Listener
    //==========================================================================

    void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged,
                                  const juce::Identifier& property) override
    {
        juce::ignoreUnused(treeWhosePropertyHasChanged, property);
        // Don't auto-repaint on property changes - causes high CPU with 64+ channels
        // Map repaints are triggered explicitly by:
        // - mouseDrag/touchDrag (during interaction)
        // - MainComponent timer (when LFO active, at 50Hz)
        // - Pan/zoom gestures
        // - Channel add/remove (valueTreeChildAdded/Removed)
    }

    void valueTreeChildAdded(juce::ValueTree& parentTree, juce::ValueTree& child) override
    {
        juce::ignoreUnused(parentTree, child);
        repaint();
    }

    void valueTreeChildRemoved(juce::ValueTree& parentTree, juce::ValueTree& child, int index) override
    {
        juce::ignoreUnused(parentTree, child, index);
        selectedInput = -1;  // Clear selection if channel removed
        repaint();
    }

    void valueTreeChildOrderChanged(juce::ValueTree& parentTree, int oldIndex, int newIndex) override
    {
        juce::ignoreUnused(parentTree, oldIndex, newIndex);
    }

    void valueTreeParentChanged(juce::ValueTree& tree) override
    {
        juce::ignoreUnused(tree);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MapTab)
};
