#pragma once

#include <JuceHeader.h>
#include <map>
#include <limits>
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/WFSParameterDefaults.h"
#include "ColorScheme.h"
#include "../Localization/LocalizationManager.h"

//==============================================================================
/**
 * Unified filter type enum for EQ display.
 * Maps from different shape encodings used in Output EQ vs Reverb EQ.
 */
enum class EQFilterType
{
    Off = 0,
    LowCut,      // High-pass with resonance
    LowShelf,
    PeakNotch,
    BandPass,
    AllPass,     // Phase shift only, flat magnitude response
    HighShelf,
    HighCut      // Low-pass with resonance
};

//==============================================================================
/**
 * Configuration for EQDisplayComponent to handle different parameter IDs
 * between Output EQ and Reverb EQ.
 */
struct EQDisplayConfig
{
    juce::Identifier shapeId;
    juce::Identifier frequencyId;
    juce::Identifier gainId;
    juce::Identifier qId;

    float qMin = 0.1f;
    float qMax = 10.0f;

    bool hasBandPass = false;

    static EQDisplayConfig forOutputEQ()
    {
        EQDisplayConfig config;
        config.shapeId = WFSParameterIDs::eqShape;
        config.frequencyId = WFSParameterIDs::eqFrequency;
        config.gainId = WFSParameterIDs::eqGain;
        config.qId = WFSParameterIDs::eqQ;
        config.qMin = WFSParameterDefaults::eqQMin;
        config.qMax = WFSParameterDefaults::eqQMax;
        config.hasBandPass = true;
        return config;
    }

    static EQDisplayConfig forReverbPreEQ()
    {
        EQDisplayConfig config;
        config.shapeId = WFSParameterIDs::reverbPreEQshape;
        config.frequencyId = WFSParameterIDs::reverbPreEQfreq;
        config.gainId = WFSParameterIDs::reverbPreEQgain;
        config.qId = WFSParameterIDs::reverbPreEQq;
        config.qMin = WFSParameterDefaults::reverbPreEQqMin;
        config.qMax = WFSParameterDefaults::reverbPreEQqMax;
        config.hasBandPass = false;
        return config;
    }

    static EQDisplayConfig forReverbPostEQ()
    {
        EQDisplayConfig config;
        config.shapeId = WFSParameterIDs::reverbPostEQshape;
        config.frequencyId = WFSParameterIDs::reverbPostEQfreq;
        config.gainId = WFSParameterIDs::reverbPostEQgain;
        config.qId = WFSParameterIDs::reverbPostEQq;
        config.qMin = WFSParameterDefaults::reverbPostEQqMin;
        config.qMax = WFSParameterDefaults::reverbPostEQqMax;
        config.hasBandPass = false;
        return config;
    }
};

//==============================================================================
/**
 * Interactive parametric EQ visualization component.
 * Displays frequency response curve with draggable band markers.
 * Supports variable number of bands and different parameter configurations.
 */
class EQDisplayComponent : public juce::Component,
                           private juce::ValueTree::Listener
{
public:
    //==========================================================================
    EQDisplayComponent (juce::ValueTree eqParentTree,
                        int numBandsIn,
                        const EQDisplayConfig& configIn)
        : eqTree (eqParentTree),
          numBands (numBandsIn),
          config (configIn)
    {
        eqTree.addListener (this);

        // Initialize coefficient cache
        bandCoefficients.resize (static_cast<size_t> (numBands));
        updateAllCoefficients();

        // Ensure we receive all mouse events
        setInterceptsMouseClicks (true, false);
        setWantsKeyboardFocus (true);
    }

    ~EQDisplayComponent() override
    {
        eqTree.removeListener (this);
    }

    //==========================================================================
    void setdBRange (float newMinDb, float newMaxDb)
    {
        mindB = newMinDb;
        maxdB = newMaxDb;
        repaint();
    }

    void setSampleRate (double newSampleRate)
    {
        if (sampleRate != newSampleRate)
        {
            sampleRate = newSampleRate;
            updateAllCoefficients();
            repaint();
        }
    }

    void setEQEnabled (bool enabled)
    {
        if (eqEnabled != enabled)
        {
            eqEnabled = enabled;
            repaint();
        }
    }

    bool isEQEnabled() const { return eqEnabled; }

    //==========================================================================
    void paint (juce::Graphics& g) override
    {
        drawGrid (g);
        drawResponseCurve (g);
        drawBandMarkers (g);

        // Draw grey overlay when EQ is disabled
        if (! eqEnabled)
        {
            g.setColour (ColorScheme::get().background.withAlpha (0.7f));
            g.fillRect (getLocalBounds());

            // Draw "EQ OFF" text
            g.setColour (ColorScheme::get().textSecondary);
            g.setFont (juce::FontOptions (24.0f));
            g.drawText (LOC("eq.status.off"), getLocalBounds(), juce::Justification::centred);
        }
    }

    void resized() override
    {
        // Recalculate paths on resize
        repaint();
    }

    //==========================================================================
    // Mouse interaction (with multitouch pinch support)
    //==========================================================================
    void mouseDown (const juce::MouseEvent& e) override
    {
        int touchIndex = e.source.getIndex();

        // Track this touch point
        TouchInfo touch;
        touch.position = e.position;
        touch.startPosition = e.position;
        activeTouches[touchIndex] = touch;

        // Check if this is the second touch (pinch gesture start)
        if (activeTouches.size() == 2)
        {
            isPinching = true;
            pinchStartDistance = getTouchDistance();
            pinchStartQ = 0.0f;

            // Find the band closest to the midpoint between the two fingers
            auto midpoint = getTouchMidpoint();
            int centeredBand = findBandNearestToPoint (midpoint);

            // If a band is near the center of the pinch, select it
            if (centeredBand >= 0)
            {
                selectedBand = centeredBand;
                isDragging = false;  // Cancel any single-finger drag

                // Begin undo transaction for pinch gesture
                if (undoManagerPtr)
                    undoManagerPtr->beginNewTransaction ("EQ Pinch Q");
            }

            // Get Q of selected band for pinch
            if (selectedBand >= 0)
            {
                auto bandTree = eqTree.getChild (selectedBand);
                if (bandTree.isValid())
                    pinchStartQ = bandTree.getProperty (config.qId);
            }

            repaint();  // Update selection highlight
            return;
        }

        // Single touch - normal band selection/drag
        int clickedBand = findBandAtPosition (e.position);

        if (clickedBand >= 0)
        {
            selectedBand = clickedBand;
            isDragging = true;
            dragMode = DragMode::Both;
            dragStartPos = e.position;
            setMouseCursor (juce::MouseCursor::DraggingHandCursor);

            // Begin undo transaction for this EQ drag gesture
            if (undoManagerPtr)
                undoManagerPtr->beginNewTransaction ("EQ Band " + juce::String (selectedBand + 1));

            // Begin drag auto-repeat to ensure we receive continuous drag events
            beginDragAutoRepeat (50);
            grabKeyboardFocus();
        }
        else
        {
            // Check if clicked on a crosshair line of the selected band
            auto crosshairMode = findCrosshairAtPosition (e.position);
            if (crosshairMode != DragMode::None)
            {
                isDragging = true;
                dragMode = crosshairMode;
                dragStartPos = e.position;

                // Store original values for relative drag
                auto bandTree = eqTree.getChild (selectedBand);
                dragStartFreq = static_cast<float> (static_cast<int> (bandTree.getProperty (config.frequencyId)));
                dragStartGain = bandTree.getProperty (config.gainId);

                if (crosshairMode == DragMode::GainOnly)
                    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
                else
                    setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);

                if (undoManagerPtr)
                    undoManagerPtr->beginNewTransaction ("EQ Band " + juce::String (selectedBand + 1));

                beginDragAutoRepeat (50);
            }
            else
            {
                // Clicked on empty space - deselect
                selectedBand = -1;
                isDragging = false;
                dragMode = DragMode::None;
            }
        }

        repaint();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        int touchIndex = e.source.getIndex();

        // Update touch position
        auto it = activeTouches.find (touchIndex);
        if (it != activeTouches.end())
            it->second.position = e.position;

        // Handle pinch gesture (two fingers)
        if (isPinching && activeTouches.size() >= 2 && selectedBand >= 0)
        {
            float currentDistance = getTouchDistance();
            if (pinchStartDistance > 0.0f && pinchStartQ > 0.0f)
            {
                // Calculate scale factor from distance change
                float scaleFactor = currentDistance / pinchStartDistance;

                // Apply to Q - pinch in = increase Q (narrower), pinch out = decrease Q (wider)
                float newQ = pinchStartQ / scaleFactor;
                newQ = juce::jlimit (config.qMin, config.qMax, newQ);

                setBandParameter (selectedBand, config.qId, newQ);
                updateBandCoefficients (selectedBand);
                repaint();
            }
            return;
        }

        // Normal single-finger drag
        if (! isDragging || selectedBand < 0)
            return;

        auto bandTree = eqTree.getChild (selectedBand);
        if (! bandTree.isValid())
            return;

        int shape = bandTree.getProperty (config.shapeId);
        EQFilterType filterType = shapeToFilterType (shape);

        // Update frequency (unless gain-only mode)
        if (dragMode != DragMode::GainOnly)
        {
            float newFreq;
            if (dragMode == DragMode::FrequencyOnly)
            {
                // Relative: apply X delta to original frequency
                float startX = frequencyToX (dragStartFreq);
                newFreq = xToFrequency (startX + (e.position.x - dragStartPos.x));
            }
            else
            {
                newFreq = xToFrequency (e.position.x);
            }
            newFreq = juce::jlimit (20.0f, 20000.0f, newFreq);
            setBandParameter (selectedBand, config.frequencyId, static_cast<int> (newFreq));
        }

        // Update gain (unless freq-only mode, or cut/bandpass/allpass)
        if (dragMode != DragMode::FrequencyOnly &&
            filterType != EQFilterType::LowCut &&
            filterType != EQFilterType::HighCut &&
            filterType != EQFilterType::BandPass &&
            filterType != EQFilterType::AllPass)
        {
            float newGain;
            if (dragMode == DragMode::GainOnly)
            {
                // Relative: apply Y delta to original gain
                float startY = dBToY (dragStartGain);
                newGain = yTodB (startY + (e.position.y - dragStartPos.y));
            }
            else
            {
                newGain = yTodB (e.position.y);
            }
            newGain = juce::jlimit (mindB, maxdB, newGain);
            setBandParameter (selectedBand, config.gainId, newGain);
        }

        updateBandCoefficients (selectedBand);
        repaint();
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        int touchIndex = e.source.getIndex();

        // Remove this touch
        activeTouches.erase (touchIndex);

        // End pinch if we're below 2 touches
        if (activeTouches.size() < 2)
            isPinching = false;

        // End drag if no touches remain
        if (activeTouches.empty())
        {
            isDragging = false;
            dragMode = DragMode::None;
            setMouseCursor (juce::MouseCursor::NormalCursor);
        }
        // Keep selection for wheel/keyboard adjustment
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        // Only update cursor when not dragging
        if (isDragging)
            return;

        // Show pointing hand when hovering over a band marker
        int hoveredBand = findBandAtPosition (e.position);
        if (hoveredBand >= 0)
        {
            setMouseCursor (juce::MouseCursor::PointingHandCursor);
        }
        else
        {
            // Check crosshair lines of selected band
            auto crosshairMode = findCrosshairAtPosition (e.position);
            if (crosshairMode == DragMode::GainOnly)
                setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
            else if (crosshairMode == DragMode::FrequencyOnly)
                setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
            else
                setMouseCursor (juce::MouseCursor::NormalCursor);
        }
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        if (! isDragging)
            setMouseCursor (juce::MouseCursor::NormalCursor);
    }

    void mouseWheelMove (const juce::MouseEvent&,
                         const juce::MouseWheelDetails& wheel) override
    {
        if (selectedBand < 0)
            return;

        auto bandTree = eqTree.getChild (selectedBand);
        if (! bandTree.isValid())
            return;

        // Begin undo transaction for wheel gesture
        if (undoManagerPtr)
            undoManagerPtr->beginNewTransaction ("EQ Wheel Q");

        // Adjust Q for all filter types
        float currentQ = bandTree.getProperty (config.qId);
        float delta = wheel.deltaY * 0.5f;

        // Multiplicative adjustment for Q
        float newQ = currentQ * (1.0f + delta);
        newQ = juce::jlimit (config.qMin, config.qMax, newQ);
        setBandParameter (selectedBand, config.qId, newQ);

        updateBandCoefficients (selectedBand);
        repaint();
    }

    void mouseMagnify (const juce::MouseEvent& e, float scaleFactor) override
    {
        // Fallback for platforms that support native magnify gesture
        int targetBand = selectedBand;

        if (targetBand < 0)
            targetBand = findBandAtPosition (e.position);

        if (targetBand < 0)
            return;

        auto bandTree = eqTree.getChild (targetBand);
        if (! bandTree.isValid())
            return;

        // Begin undo transaction for magnify gesture
        if (undoManagerPtr)
            undoManagerPtr->beginNewTransaction ("EQ Magnify Q");

        float currentQ = bandTree.getProperty (config.qId);
        float newQ = currentQ * scaleFactor;
        newQ = juce::jlimit (config.qMin, config.qMax, newQ);
        setBandParameter (targetBand, config.qId, newQ);

        if (selectedBand != targetBand)
            selectedBand = targetBand;

        updateBandCoefficients (targetBand);
        repaint();
    }

    //==========================================================================
    // Keyboard interaction
    //==========================================================================
    bool keyPressed (const juce::KeyPress& key) override
    {
        if (selectedBand < 0)
            return false;

        auto bandTree = eqTree.getChild (selectedBand);
        if (! bandTree.isValid())
            return false;

        int shape = bandTree.getProperty (config.shapeId);
        EQFilterType filterType = shapeToFilterType (shape);
        bool noGainControl = (filterType == EQFilterType::LowCut ||
                              filterType == EQFilterType::HighCut ||
                              filterType == EQFilterType::BandPass ||
                              filterType == EQFilterType::AllPass);

        int keyCode = key.getKeyCode();

        if (keyCode == juce::KeyPress::leftKey || keyCode == juce::KeyPress::rightKey)
        {
            // Frequency: logarithmic increment = freq / 20
            int currentFreq = bandTree.getProperty (config.frequencyId);
            int increment = juce::jmax (1, currentFreq / 20);
            int newFreq = (keyCode == juce::KeyPress::rightKey)
                              ? currentFreq + increment
                              : currentFreq - increment;
            newFreq = juce::jlimit (20, 20000, newFreq);

            if (undoManagerPtr)
                undoManagerPtr->beginNewTransaction ("EQ Arrow Freq");

            setBandParameter (selectedBand, config.frequencyId, newFreq);
            updateBandCoefficients (selectedBand);
            repaint();
            return true;
        }

        if ((keyCode == juce::KeyPress::upKey || keyCode == juce::KeyPress::downKey)
            && ! noGainControl)
        {
            // Gain: ±0.1 dB
            float currentGain = bandTree.getProperty (config.gainId);
            float newGain = (keyCode == juce::KeyPress::upKey)
                                ? currentGain + 0.1f
                                : currentGain - 0.1f;
            newGain = juce::jlimit (mindB, maxdB, newGain);

            if (undoManagerPtr)
                undoManagerPtr->beginNewTransaction ("EQ Arrow Gain");

            setBandParameter (selectedBand, config.gainId, newGain);
            updateBandCoefficients (selectedBand);
            repaint();
            return true;
        }

        return false;
    }

    //==========================================================================
    // Get currently selected band (-1 if none)
    int getSelectedBand() const { return selectedBand; }

    // Programmatically select a band
    void setSelectedBand (int band)
    {
        selectedBand = (band >= 0 && band < numBands) ? band : -1;
        repaint();
    }

    // Callback for parameter changes (for array propagation)
    // Parameters: bandIndex, paramId, newValue
    std::function<void (int, const juce::Identifier&, const juce::var&)> onParameterChanged;

    /** Set the UndoManager used for EQ touch interactions */
    void setUndoManager (juce::UndoManager* um) { undoManagerPtr = um; }

    // Get band colour - static so it can be used by other components
    // Rainbow progression: Red -> Orange -> Yellow -> Green -> Blue -> Purple
    static juce::Colour getBandColour (int band)
    {
        static const juce::Colour colours[] = {
            juce::Colour (0xFFE74C3C),  // Band 1: Red
            juce::Colour (0xFFE67E22),  // Band 2: Orange
            juce::Colour (0xFFFFEB3B),  // Band 3: Yellow (pure)
            juce::Colour (0xFF2ECC71),  // Band 4: Green
            juce::Colour (0xFF3498DB),  // Band 5: Blue
            juce::Colour (0xFF9B59B6),  // Band 6: Purple
            juce::Colour (0xFF1ABC9C),  // Band 7: Teal
            juce::Colour (0xFFE91E63),  // Band 8: Pink
        };
        return colours[band % 8];
    }

private:
    //==========================================================================
    // ValueTree::Listener
    //==========================================================================
    void valueTreePropertyChanged (juce::ValueTree& tree,
                                   const juce::Identifier& property) override
    {
        juce::ignoreUnused (property);

        // Check if this is one of our band trees
        for (int i = 0; i < numBands; ++i)
        {
            if (tree == eqTree.getChild (i))
            {
                updateBandCoefficients (i);
                repaint();
                return;
            }
        }

        // If it's the parent tree, update all
        if (tree == eqTree)
        {
            updateAllCoefficients();
            repaint();
        }
    }

    void valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&) override {}
    void valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree&, int) override {}
    void valueTreeChildOrderChanged (juce::ValueTree&, int, int) override {}
    void valueTreeParentChanged (juce::ValueTree&) override {}

    //==========================================================================
    // Drawing
    //==========================================================================
    void drawGrid (juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat();

        // Background - use dark theme color for contrast
        g.setColour (ColorScheme::get().backgroundAlt.darker (0.3f));
        g.fillRect (bounds);

        // Frequency grid lines (logarithmic)
        const float freqLines[] = {
            20, 30, 40, 50, 60, 70, 80, 90,
            100, 200, 300, 400, 500, 600, 700, 800, 900,
            1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000,
            10000, 20000
        };

        auto gridColor = ColorScheme::get().chromeDivider;
        for (float freq : freqLines)
        {
            float x = frequencyToX (freq);

            // Major lines at decade points
            bool isMajor = (freq == 100 || freq == 1000 || freq == 10000);
            g.setColour (isMajor ? gridColor.withAlpha (0.6f)
                                 : gridColor.withAlpha (0.3f));

            g.drawVerticalLine (static_cast<int> (x), bounds.getY(), bounds.getBottom());
        }

        // Frequency labels
        g.setColour (ColorScheme::get().textSecondary);
        g.setFont (10.0f);

        const float labelFreqs[] = { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
        const char* labelTexts[] = { "20", "50", "100", "200", "500", "1k", "2k", "5k", "10k", "20k" };

        for (int i = 0; i < 10; ++i)
        {
            float x = frequencyToX (labelFreqs[i]);
            g.drawText (labelTexts[i],
                        static_cast<int> (x) - 15,
                        static_cast<int> (bounds.getBottom()) - 15,
                        30, 12,
                        juce::Justification::centred);
        }

        // dB grid lines
        for (float dB = mindB; dB <= maxdB; dB += 6.0f)
        {
            float y = dBToY (dB);

            // 0 dB line emphasized
            if (std::abs (dB) < 0.1f)
            {
                g.setColour (gridColor.withAlpha (0.8f));
                g.drawHorizontalLine (static_cast<int> (y), bounds.getX(), bounds.getRight());
            }
            else
            {
                g.setColour (gridColor.withAlpha (0.4f));
                g.drawHorizontalLine (static_cast<int> (y), bounds.getX(), bounds.getRight());
            }

            // dB labels
            g.setColour (ColorScheme::get().textSecondary);
            juce::String label = (dB > 0 ? "+" : "") + juce::String (static_cast<int> (dB));
            g.drawText (label, 2, static_cast<int> (y) - 6, 25, 12,
                        juce::Justification::left);
        }
    }

    void drawResponseCurve (juce::Graphics& g)
    {
        juce::Path responseCurve;

        const int numPoints = getWidth();
        const float zeroY = dBToY (0.0f);

        for (int x = 0; x < numPoints; ++x)
        {
            float freq = xToFrequency (static_cast<float> (x));
            float totalGaindB = calculateTotalResponse (freq);
            float y = dBToY (totalGaindB);

            if (x == 0)
                responseCurve.startNewSubPath (static_cast<float> (x), y);
            else
                responseCurve.lineTo (static_cast<float> (x), y);
        }

        // Filled area under/over 0dB line
        juce::Path filledCurve = responseCurve;
        filledCurve.lineTo (static_cast<float> (getWidth()), zeroY);
        filledCurve.lineTo (0.0f, zeroY);
        filledCurve.closeSubPath();

        g.setColour (ColorScheme::get().accentBlue.withAlpha (0.2f));
        g.fillPath (filledCurve);

        // Curve outline
        g.setColour (ColorScheme::get().textPrimary);
        g.strokePath (responseCurve, juce::PathStrokeType (2.0f));
    }

    void drawBandMarkers (juce::Graphics& g)
    {
        for (int band = 0; band < numBands; ++band)
        {
            auto bandTree = eqTree.getChild (band);
            if (! bandTree.isValid())
                continue;

            int shape = bandTree.getProperty (config.shapeId);
            bool isOff = (shape == 0);

            // Position marker at frequency and gain (even when off)
            float freq = bandTree.getProperty (config.frequencyId);
            float gain = bandTree.getProperty (config.gainId);
            float x = frequencyToX (freq);
            float y = isOff ? dBToY (gain) : getBandMarkerPosition (band).y;

            // Band color (darkened if OFF, like inactive slider track)
            juce::Colour bandColour = getBandColour (band);
            if (isOff)
                bandColour = bandColour.darker (0.6f);

            bool isSelected = (selectedBand == band);
            float markerSize = isSelected ? 28.0f : 20.0f;

            // Draw marker circle
            g.setColour (bandColour);
            g.fillEllipse (x - markerSize / 2, y - markerSize / 2, markerSize, markerSize);

            // Selection ring and crosshair lines
            if (isSelected)
            {
                g.setColour (ColorScheme::get().textPrimary);
                g.drawEllipse (x - markerSize / 2 - 3, y - markerSize / 2 - 3,
                               markerSize + 6, markerSize + 6, 2.0f);

                // Crosshair lines for selected band (including off bands)
                {
                    EQFilterType filterType = shapeToFilterType (shape);
                    bool noGainControl = (filterType == EQFilterType::LowCut ||
                                          filterType == EQFilterType::HighCut ||
                                          filterType == EQFilterType::BandPass ||
                                          filterType == EQFilterType::AllPass);

                    g.setColour (bandColour.withAlpha (0.35f));

                    // Vertical crosshair (frequency adjustment) — always drawn
                    g.drawLine (x, 0.0f, x, static_cast<float> (getHeight()), 1.0f);

                    // Horizontal crosshair (gain adjustment) — skip for active cut/bandpass/allpass
                    // (off bands always show both since we don't know the remembered type)
                    if (isOff || ! noGainControl)
                        g.drawLine (0.0f, y, static_cast<float> (getWidth()), y, 1.0f);
                }
            }

            // Band number
            g.setColour (juce::Colours::black);
            g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
            g.drawText (juce::String (band + 1),
                        static_cast<int> (x - markerSize / 2), static_cast<int> (y - markerSize / 2),
                        static_cast<int> (markerSize), static_cast<int> (markerSize),
                        juce::Justification::centred);
        }
    }

    //==========================================================================
    // Coordinate conversion
    //==========================================================================
    float frequencyToX (float freq) const
    {
        const float minFreq = 20.0f;
        const float maxFreq = 20000.0f;

        float normalized = std::log10 (freq / minFreq) / std::log10 (maxFreq / minFreq);
        return static_cast<float> (getWidth()) * normalized;
    }

    float xToFrequency (float x) const
    {
        const float minFreq = 20.0f;
        const float maxFreq = 20000.0f;

        float normalized = x / static_cast<float> (getWidth());
        return minFreq * std::pow (maxFreq / minFreq, normalized);
    }

    float dBToY (float dB) const
    {
        float normalized = (dB - mindB) / (maxdB - mindB);
        return static_cast<float> (getHeight()) * (1.0f - normalized);
    }

    float yTodB (float y) const
    {
        float normalized = 1.0f - (y / static_cast<float> (getHeight()));
        return mindB + normalized * (maxdB - mindB);
    }

    //==========================================================================
    // Filter type conversion
    //==========================================================================
    EQFilterType shapeToFilterType (int shape) const
    {
        if (config.hasBandPass)
        {
            // Output EQ: 0=Off, 1=LowCut, 2=LowShelf, 3=Peak, 4=BandPass, 5=HighShelf, 6=HighCut, 7=AllPass
            switch (shape)
            {
                case 0: return EQFilterType::Off;
                case 1: return EQFilterType::LowCut;
                case 2: return EQFilterType::LowShelf;
                case 3: return EQFilterType::PeakNotch;
                case 4: return EQFilterType::BandPass;
                case 5: return EQFilterType::HighShelf;
                case 6: return EQFilterType::HighCut;
                case 7: return EQFilterType::AllPass;
                default: return EQFilterType::Off;
            }
        }
        else
        {
            // Reverb EQ: 0=Off, 1=LowCut, 2=LowShelf, 3=Peak, 4=HighShelf, 5=HighCut, 6=BandPass
            switch (shape)
            {
                case 0: return EQFilterType::Off;
                case 1: return EQFilterType::LowCut;
                case 2: return EQFilterType::LowShelf;
                case 3: return EQFilterType::PeakNotch;
                case 4: return EQFilterType::HighShelf;
                case 5: return EQFilterType::HighCut;
                case 6: return EQFilterType::BandPass;
                default: return EQFilterType::Off;
            }
        }
    }

    //==========================================================================
    // Filter response calculation
    //==========================================================================
    void updateBandCoefficients (int bandIndex)
    {
        if (bandIndex < 0 || bandIndex >= numBands)
            return;

        auto bandTree = eqTree.getChild (bandIndex);
        if (! bandTree.isValid())
        {
            bandCoefficients[static_cast<size_t> (bandIndex)] = nullptr;
            return;
        }

        int shape = bandTree.getProperty (config.shapeId);
        float freq = bandTree.getProperty (config.frequencyId);
        float gain = bandTree.getProperty (config.gainId);
        float Q = bandTree.getProperty (config.qId);

        EQFilterType filterType = shapeToFilterType (shape);

        if (filterType == EQFilterType::Off)
        {
            bandCoefficients[static_cast<size_t> (bandIndex)] = nullptr;
            return;
        }

        // Clamp values
        freq = juce::jlimit (20.0f, 20000.0f, freq);
        Q = juce::jlimit (config.qMin, config.qMax, Q);

        juce::dsp::IIR::Coefficients<float>::Ptr coeffs;

        switch (filterType)
        {
            case EQFilterType::LowCut:
                coeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, freq, Q);
                break;

            case EQFilterType::HighCut:
                coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, freq, Q);
                break;

            case EQFilterType::LowShelf:
                coeffs = makeLowShelfCoefficients (freq, gain, Q);
                break;

            case EQFilterType::HighShelf:
                coeffs = makeHighShelfCoefficients (freq, gain, Q);
                break;

            case EQFilterType::PeakNotch:
                coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
                    sampleRate, freq, Q, juce::Decibels::decibelsToGain (gain));
                break;

            case EQFilterType::BandPass:
                coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass (sampleRate, freq, Q);
                break;

            case EQFilterType::AllPass:
                coeffs = juce::dsp::IIR::Coefficients<float>::makeAllPass (sampleRate, freq, Q);
                break;

            default:
                coeffs = nullptr;
                break;
        }

        bandCoefficients[static_cast<size_t> (bandIndex)] = coeffs;
    }

    void updateAllCoefficients()
    {
        for (int i = 0; i < numBands; ++i)
            updateBandCoefficients (i);
    }

    // Custom shelf filter calculation matching filterCalc.js formulas
    // Uses Q as the S (slope) parameter: alpha = (sin(w0)/2) * sqrt((A + 1/A) * (1/S - 1) + 2)
    juce::dsp::IIR::Coefficients<float>::Ptr makeLowShelfCoefficients (float freq, float gaindB, float S)
    {
        double A = std::pow (10.0, gaindB / 40.0);
        double w0 = 2.0 * juce::MathConstants<double>::pi * freq / sampleRate;
        double cosw0 = std::cos (w0);
        double sinw0 = std::sin (w0);
        double alpha = (sinw0 / 2.0) * std::sqrt ((A + 1.0 / A) * (1.0 / S - 1.0) + 2.0);
        double sqrtA = std::sqrt (A);
        double twoSqrtAalpha = 2.0 * sqrtA * alpha;

        double b0 = A * ((A + 1.0) - (A - 1.0) * cosw0 + twoSqrtAalpha);
        double b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cosw0);
        double b2 = A * ((A + 1.0) - (A - 1.0) * cosw0 - twoSqrtAalpha);
        double a0 = (A + 1.0) + (A - 1.0) * cosw0 + twoSqrtAalpha;
        double a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cosw0);
        double a2 = (A + 1.0) + (A - 1.0) * cosw0 - twoSqrtAalpha;

        // Use array constructor - JUCE expects {b0, b1, b2, a0, a1, a2}
        std::array<float, 6> coeffArray = {
            static_cast<float> (b0 / a0),
            static_cast<float> (b1 / a0),
            static_cast<float> (b2 / a0),
            1.0f,  // a0 normalized
            static_cast<float> (a1 / a0),
            static_cast<float> (a2 / a0)
        };

        return juce::dsp::IIR::Coefficients<float>::Ptr (
            new juce::dsp::IIR::Coefficients<float> (coeffArray));
    }

    juce::dsp::IIR::Coefficients<float>::Ptr makeHighShelfCoefficients (float freq, float gaindB, float S)
    {
        double A = std::pow (10.0, gaindB / 40.0);
        double w0 = 2.0 * juce::MathConstants<double>::pi * freq / sampleRate;
        double cosw0 = std::cos (w0);
        double sinw0 = std::sin (w0);
        double alpha = (sinw0 / 2.0) * std::sqrt ((A + 1.0 / A) * (1.0 / S - 1.0) + 2.0);
        double sqrtA = std::sqrt (A);
        double twoSqrtAalpha = 2.0 * sqrtA * alpha;

        double b0 = A * ((A + 1.0) + (A - 1.0) * cosw0 + twoSqrtAalpha);
        double b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cosw0);
        double b2 = A * ((A + 1.0) + (A - 1.0) * cosw0 - twoSqrtAalpha);
        double a0 = (A + 1.0) - (A - 1.0) * cosw0 + twoSqrtAalpha;
        double a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cosw0);
        double a2 = (A + 1.0) - (A - 1.0) * cosw0 - twoSqrtAalpha;

        // Use array constructor - JUCE expects {b0, b1, b2, a0, a1, a2}
        std::array<float, 6> coeffArray = {
            static_cast<float> (b0 / a0),
            static_cast<float> (b1 / a0),
            static_cast<float> (b2 / a0),
            1.0f,  // a0 normalized
            static_cast<float> (a1 / a0),
            static_cast<float> (a2 / a0)
        };

        return juce::dsp::IIR::Coefficients<float>::Ptr (
            new juce::dsp::IIR::Coefficients<float> (coeffArray));
    }

    float calculateBandResponse (int bandIndex, float frequency)
    {
        if (bandIndex < 0 || bandIndex >= numBands)
            return 0.0f;

        auto coeffs = bandCoefficients[static_cast<size_t> (bandIndex)];
        if (coeffs == nullptr)
            return 0.0f;

        double mag = coeffs->getMagnitudeForFrequency (frequency, sampleRate);
        return static_cast<float> (juce::Decibels::gainToDecibels (mag));
    }

    float calculateTotalResponse (float frequency)
    {
        float totalGaindB = 0.0f;

        for (int band = 0; band < numBands; ++band)
        {
            totalGaindB += calculateBandResponse (band, frequency);
        }

        return juce::jlimit (mindB - 6.0f, maxdB + 6.0f, totalGaindB);
    }

    //==========================================================================
    // Band marker positioning and hit testing
    //==========================================================================
    juce::Point<float> getBandMarkerPosition (int bandIndex) const
    {
        auto bandTree = eqTree.getChild (bandIndex);
        if (! bandTree.isValid())
            return { 0.0f, 0.0f };

        int shape = bandTree.getProperty (config.shapeId);
        float freq = bandTree.getProperty (config.frequencyId);
        float gain = bandTree.getProperty (config.gainId);

        float x = frequencyToX (freq);
        float y;

        EQFilterType filterType = shapeToFilterType (shape);

        // For cuts, bandpass, and allpass, marker sits at 0dB line
        if (filterType == EQFilterType::LowCut ||
            filterType == EQFilterType::HighCut ||
            filterType == EQFilterType::BandPass ||
            filterType == EQFilterType::AllPass)
        {
            y = dBToY (0.0f);
        }
        else
        {
            y = dBToY (gain);
        }

        return { x, y };
    }

    int findBandAtPosition (juce::Point<float> pos)
    {
        const float hitRadius = 15.0f;
        const float hitRadiusSq = hitRadius * hitRadius;

        for (int band = 0; band < numBands; ++band)
        {
            auto bandTree = eqTree.getChild (band);
            if (! bandTree.isValid())
                continue;

            // Get marker position (works for both on and off bands)
            int shape = bandTree.getProperty (config.shapeId);
            float freq = bandTree.getProperty (config.frequencyId);
            float x = frequencyToX (freq);
            float y;

            if (shape == 0)
            {
                float gain = bandTree.getProperty (config.gainId);
                y = dBToY (gain);
            }
            else
            {
                y = getBandMarkerPosition (band).y;
            }

            float dx = pos.x - x;
            float dy = pos.y - y;

            if (dx * dx + dy * dy < hitRadiusSq)
                return band;
        }

        return -1;
    }

    enum class DragMode { None, Both, FrequencyOnly, GainOnly };

    DragMode findCrosshairAtPosition (juce::Point<float> pos) const
    {
        if (selectedBand < 0)
            return DragMode::None;

        auto bandTree = eqTree.getChild (selectedBand);
        if (! bandTree.isValid())
            return DragMode::None;

        int shape = bandTree.getProperty (config.shapeId);
        bool isOff = (shape == 0);

        // Get marker position (same logic as drawBandMarkers)
        float freq = bandTree.getProperty (config.frequencyId);
        float markerX = frequencyToX (freq);
        float markerY;

        if (isOff)
        {
            float gain = bandTree.getProperty (config.gainId);
            markerY = dBToY (gain);
        }
        else
        {
            markerY = getBandMarkerPosition (selectedBand).y;
        }

        const float hitTolerance = 8.0f;

        EQFilterType filterType = shapeToFilterType (shape);
        bool noGainControl = (filterType == EQFilterType::LowCut ||
                              filterType == EQFilterType::HighCut ||
                              filterType == EQFilterType::BandPass ||
                              filterType == EQFilterType::AllPass);

        // Check vertical crosshair (frequency adjustment) — always available
        if (std::abs (pos.x - markerX) < hitTolerance)
            return DragMode::FrequencyOnly;

        // Check horizontal crosshair (gain adjustment)
        // Available for off bands (always) and active bands with gain control
        if ((isOff || ! noGainControl) && std::abs (pos.y - markerY) < hitTolerance)
            return DragMode::GainOnly;

        return DragMode::None;
    }

    //==========================================================================
    // Multitouch helpers
    //==========================================================================
    float getTouchDistance() const
    {
        if (activeTouches.size() < 2)
            return 0.0f;

        auto it = activeTouches.begin();
        auto pos1 = it->second.position;
        ++it;
        auto pos2 = it->second.position;

        return pos1.getDistanceFrom (pos2);
    }

    juce::Point<float> getTouchMidpoint() const
    {
        if (activeTouches.size() < 2)
            return { 0.0f, 0.0f };

        auto it = activeTouches.begin();
        auto pos1 = it->second.position;
        ++it;
        auto pos2 = it->second.position;

        return { (pos1.x + pos2.x) * 0.5f, (pos1.y + pos2.y) * 0.5f };
    }

    // Find the band nearest to a point (for pinch gesture targeting)
    // Returns -1 if no band is reasonably close
    int findBandNearestToPoint (juce::Point<float> pos) const
    {
        int nearestBand = -1;
        float nearestDistSq = (std::numeric_limits<float>::max)();
        const float maxSearchRadius = 150.0f;  // Max distance to consider
        const float maxSearchRadiusSq = maxSearchRadius * maxSearchRadius;

        for (int band = 0; band < numBands; ++band)
        {
            auto bandTree = eqTree.getChild (band);
            if (! bandTree.isValid())
                continue;

            int shape = bandTree.getProperty (config.shapeId);
            float freq = bandTree.getProperty (config.frequencyId);
            float x = frequencyToX (freq);
            float y;

            if (shape == 0)
            {
                float gain = bandTree.getProperty (config.gainId);
                y = dBToY (gain);
            }
            else
            {
                y = getBandMarkerPosition (band).y;
            }

            float dx = pos.x - x;
            float dy = pos.y - y;
            float distSq = dx * dx + dy * dy;

            if (distSq < nearestDistSq && distSq < maxSearchRadiusSq)
            {
                nearestDistSq = distSq;
                nearestBand = band;
            }
        }

        return nearestBand;
    }

    //==========================================================================
    // Parameter setting with callback support
    //==========================================================================
    void setBandParameter (int bandIndex, const juce::Identifier& paramId, const juce::var& value)
    {
        auto bandTree = eqTree.getChild (bandIndex);
        if (! bandTree.isValid())
            return;

        // Set with undo support (nullptr if no manager assigned)
        bandTree.setProperty (paramId, value, undoManagerPtr);

        // Notify parent for array propagation
        if (onParameterChanged)
            onParameterChanged (bandIndex, paramId, value);
    }

    //==========================================================================
    // Member variables
    //==========================================================================
    juce::ValueTree eqTree;
    int numBands;
    EQDisplayConfig config;

    float mindB = -24.0f;
    float maxdB = 24.0f;
    double sampleRate = 48000.0;
    bool eqEnabled = true;

    int selectedBand = -1;
    bool isDragging = false;
    DragMode dragMode = DragMode::None;
    juce::Point<float> dragStartPos;
    float dragStartFreq = 0.0f;  // Original freq at crosshair drag start
    float dragStartGain = 0.0f;  // Original gain at crosshair drag start

    std::vector<juce::dsp::IIR::Coefficients<float>::Ptr> bandCoefficients;

    // Multitouch tracking
    struct TouchInfo
    {
        juce::Point<float> position;
        juce::Point<float> startPosition;
    };
    std::map<int, TouchInfo> activeTouches;
    bool isPinching = false;
    float pinchStartDistance = 0.0f;
    float pinchStartQ = 0.0f;

    // Undo support
    juce::UndoManager* undoManagerPtr = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQDisplayComponent)
};
