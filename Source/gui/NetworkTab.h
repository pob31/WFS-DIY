#pragma once

#include <JuceHeader.h>
#include <map>
#include "../WfsParameters.h"
#include "../Accessibility/TTSManager.h"
#include "StatusBar.h"
#include "../Network/OSCManager.h"
#include "ColorScheme.h"
#include "../Localization/LocalizationManager.h"
#include "buttons/LongPressButton.h"
#include "ColumnFocusTraverser.h"
#include "../AppSettings.h"
#include "../Network/ADMOSCMapping.h"
#include "HelpCard.h"

#if JUCE_WINDOWS
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <iphlpapi.h>
    #pragma comment(lib, "iphlpapi.lib")
    #pragma comment(lib, "ws2_32.lib")
#elif JUCE_MAC
    #include <ifaddrs.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
    #include <net/if.h>
    #include <SystemConfiguration/SystemConfiguration.h>
    #include <CoreFoundation/CoreFoundation.h>
#endif

/**
 * Network Tab Component
 * Configuration for network settings:
 * - Network Interface Selector
 * - Current IP address
 * - UDP/TCP Ports
 * - Network Connections Table (up to 6 targets/servers)
 * - ADM-OSC settings (Offset, Scale, Flip)
 * - Tracking settings (Enable, Protocol, Port, Offset, Scale, Flip)
 */

/** Panel showing piecewise-linear ADM-OSC Cartesian mapping curves for all 3 axes,
    plus an XY rectangle overview.  Supports drag-editing of control points. */
class AdmMappingPanel : public juce::Component, private juce::Timer
{
public:
    AdmMappingPanel()
    {
        addChildComponent (valueEditor);
        valueEditor.setMultiLine (false);
        valueEditor.setReturnKeyStartsNewLine (false);
        valueEditor.setScrollbarsShown (false);
        valueEditor.setFont (juce::Font (juce::FontOptions (12.0f)));
        valueEditor.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff1a1a1a));
        valueEditor.setColour (juce::TextEditor::textColourId, juce::Colour (0xffe0e0e0));
        valueEditor.setColour (juce::TextEditor::outlineColourId, juce::Colour (0xff4fc3f7));
        valueEditor.setColour (juce::TextEditor::focusedOutlineColourId, juce::Colour (0xff4fc3f7));
        valueEditor.setJustification (juce::Justification::centred);

        valueEditor.onReturnKey = [this] { commitEdit(); };
        valueEditor.onEscapeKey = [this] { cancelEdit(); };
        valueEditor.onFocusLost = [this] { juce::MessageManager::callAsync ([this] { commitEdit(); }); };
    }

    ~AdmMappingPanel() override { stopTimer(); }

    /** Set the full 3-axis config and repaint. */
    void setConfig (const ADMOSCMapping::CartesianMappingConfig& newCfg)
    {
        cfg = newCfg;
        repaint();
    }

    /** Callback fired when the user drags a control point.
        axis: 0=X, 1=Y, 2=Z.  pointIndex: 0-4.
        Remaining args are the updated parameter values for that axis. */
    std::function<void (int axis, int pointIndex,
                        float newBreakpoint, float newInnerWidth, float newOuterWidth,
                        float newCenterOffset, bool isPositiveDirection)> onParameterDragged;

    /** Callback fired when axis swap is changed via popup menu. */
    std::function<void (int axis, int newSwapValue)> onSwapChanged;

    /** Callback fired when flip is toggled by clicking the flip indicator. */
    std::function<void (int axis, bool newFlipState)> onFlipChanged;

    /** Callback fired when reset long-press completes. */
    std::function<void()> onResetLongPress;

    //==========================================================================
    void resized() override
    {
        const int gap = 4;
        int totalW = getWidth();
        int graphW = (totalW - gap * 2) / 3;
        int halfH  = (getHeight() - gap) / 2;

        graphAreas[0] = { 0,                    0,          graphW, halfH };   // X graph
        rectArea      = { graphW + gap,         0,          graphW, halfH };   // XY rect
        graphAreas[2] = { graphW * 2 + gap * 2, 0,          graphW, halfH };   // Z graph
        graphAreas[1] = { graphW + gap,         halfH + gap, graphW, halfH };  // Y graph
    }

    //==========================================================================
    void paint (juce::Graphics& g) override
    {
        static const char* axisLabels[3] = { "X", "Y", "Z" };
        for (int a = 0; a < 3; ++a)
            paintAxisGraph (g, a, axisLabels[a], graphAreas[a]);

        paintXYRect (g, rectArea);
    }

    //==========================================================================
    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        auto pos = e.position;

        // Check breakpoint labels
        for (int a = 0; a < 3; ++a)
        {
            if (bpLabelRects[a].contains (pos))
            {
                editAxis = a;
                editPoint = -1;
                editIsBreakpoint = true;
                showValueEditor (bpLabelRects[a], juce::String (cfg.axes[a].breakpoint, 2));
                return;
            }
        }

        // Check dot value labels
        for (int a = 0; a < 3; ++a)
        {
            for (int p = 0; p < 5; ++p)
            {
                if (dotLabelRects[a][p].contains (pos))
                {
                    editAxis = a;
                    editPoint = p;
                    editIsBreakpoint = false;
                    float v, m;
                    getPointVM (a, p, v, m);
                    showValueEditor (dotLabelRects[a][p], juce::String (m, 2));
                    return;
                }
            }
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (isEditingValue)
        {
            commitEdit();
            return;
        }

        dragAxis  = -1;
        dragPoint = -1;

        auto pos = e.position;

        // Check reset hit area (long press)
        if (resetHitArea.contains (pos))
        {
            resetPressActive = true;
            resetPressStartMs = juce::Time::currentTimeMillis();
            startTimer (50);
            repaint();
            return;
        }

        // Check swap/flip hit areas first
        for (int a = 0; a < 3; ++a)
        {
            if (swapHitAreas[a].contains (pos))
            {
                showSwapPopupMenu (a);
                return;
            }
            if (flipHitAreas[a].contains (pos))
            {
                cfg.axes[a].signFlip = ! cfg.axes[a].signFlip;
                repaint();
                if (onFlipChanged)
                    onFlipChanged (a, cfg.axes[a].signFlip);
                return;
            }
        }

        // Hit-test drag dots
        constexpr float hitRadius = 10.0f;
        for (int a = 0; a < 3; ++a)
        {
            auto area = graphAreas[a].toFloat();
            if (! area.contains (pos))
                continue;

            auto ga = getGraphSubArea (area);
            float minM, maxM;
            getAxisYRange (a, minM, maxM);

            for (int p = 0; p < 5; ++p)
            {
                float v, m;
                getPointVM (a, p, v, m);
                float px = vToX (v, ga);
                float py = mToY (m, minM, maxM, ga);
                if (pos.getDistanceFrom ({ px, py }) <= hitRadius)
                {
                    dragAxis  = a;
                    dragPoint = p;
                    dragStartV = v;
                    dragStartM = m;
                    setMouseCursor (juce::MouseCursor::DraggingHandCursor);
                    return;
                }
            }
        }
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragAxis < 0 || dragPoint < 0)
            return;

        auto area = graphAreas[dragAxis].toFloat();
        auto ga   = getGraphSubArea (area);
        float minM, maxM;
        getAxisYRange (dragAxis, minM, maxM);

        float rawV = xToV (e.position.x, ga);
        float rawM = yToM (e.position.y, minM, maxM, ga);

        // Alt+drag: 10x precision (reduce effective movement relative to drag start)
        float newV = rawV;
        float newM = rawM;
        if (e.mods.isAltDown())
        {
            newV = dragStartV + (rawV - dragStartV) * 0.1f;
            newM = dragStartM + (rawM - dragStartM) * 0.1f;
        }

        auto& ax = cfg.axes[dragAxis];

        constexpr float wMin = WFSParameterDefaults::admCartWidthMin;   // 0.1
        constexpr float wMax = WFSParameterDefaults::admCartWidthMax;   // 50.0
        bool shiftHeld = e.mods.isShiftDown();

        bool posDir = true;

        switch (dragPoint)
        {
            case 0: // v=-1: vertical drag -> negOuterWidth
            {
                float newOuter = (ax.centerOffset - newM) - ax.negInnerWidth;
                ax.negOuterWidth = juce::jlimit (wMin, wMax, newOuter);
                if (shiftHeld) ax.posOuterWidth = ax.negOuterWidth;
                posDir = false;
                break;
            }
            case 1: // v=-bp: horiz->breakpoint, vert->negInnerWidth
            {
                float bpVal = std::abs (juce::jlimit (-0.99f, -0.01f, newV));
                ax.breakpoint = juce::jlimit (0.01f, 0.99f, bpVal);
                float newInner = ax.centerOffset - newM;
                ax.negInnerWidth = juce::jlimit (wMin, wMax, newInner);
                if (shiftHeld) ax.posInnerWidth = ax.negInnerWidth;
                posDir = false;
                break;
            }
            case 2: // v=0: vertical drag -> centerOffset
            {
                ax.centerOffset = juce::jlimit (-50.0f, 50.0f, newM);
                break;
            }
            case 3: // v=+bp: horiz->breakpoint, vert->posInnerWidth
            {
                ax.breakpoint = juce::jlimit (0.01f, 0.99f, newV);
                float newInner = newM - ax.centerOffset;
                ax.posInnerWidth = juce::jlimit (wMin, wMax, newInner);
                if (shiftHeld) ax.negInnerWidth = ax.posInnerWidth;
                posDir = true;
                break;
            }
            case 4: // v=+1: vertical drag -> posOuterWidth
            {
                float newOuter = newM - ax.centerOffset - ax.posInnerWidth;
                ax.posOuterWidth = juce::jlimit (wMin, wMax, newOuter);
                if (shiftHeld) ax.negOuterWidth = ax.posOuterWidth;
                posDir = true;
                break;
            }
        }

        repaint();

        if (onParameterDragged)
        {
            float innerW = posDir ? ax.posInnerWidth : ax.negInnerWidth;
            float outerW = posDir ? ax.posOuterWidth : ax.negOuterWidth;
            onParameterDragged (dragAxis, dragPoint, ax.breakpoint,
                                innerW, outerW, ax.centerOffset, posDir);
        }

        // Shift held: also fire callback for the mirror side
        if (shiftHeld && onParameterDragged && dragPoint != 2)
        {
            bool mirrorPos = ! posDir;
            float mirrorInner = mirrorPos ? ax.posInnerWidth : ax.negInnerWidth;
            float mirrorOuter = mirrorPos ? ax.posOuterWidth : ax.negOuterWidth;
            int mirrorPoint = (dragPoint == 0) ? 4 : (dragPoint == 1) ? 3
                            : (dragPoint == 3) ? 1 : 0;
            onParameterDragged (dragAxis, mirrorPoint, ax.breakpoint,
                                mirrorInner, mirrorOuter, ax.centerOffset, mirrorPos);
        }
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (resetPressActive)
        {
            resetPressActive = false;
            stopTimer();
            repaint();
        }
        dragAxis  = -1;
        dragPoint = -1;
        setMouseCursor (juce::MouseCursor::NormalCursor);
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        auto pos = e.position;
        if (resetHitArea.contains (pos))
        {
            setMouseCursor (juce::MouseCursor::PointingHandCursor);
            return;
        }
        for (int a = 0; a < 3; ++a)
        {
            if (swapHitAreas[a].contains (pos) || flipHitAreas[a].contains (pos))
            {
                setMouseCursor (juce::MouseCursor::PointingHandCursor);
                return;
            }
        }
        // Check if hovering a drag dot
        constexpr float hitRadius = 10.0f;
        for (int a = 0; a < 3; ++a)
        {
            auto area = graphAreas[a].toFloat();
            if (! area.contains (pos)) continue;
            auto ga = getGraphSubArea (area);
            float minM, maxM;
            getAxisYRange (a, minM, maxM);
            for (int p = 0; p < 5; ++p)
            {
                float v, m;
                getPointVM (a, p, v, m);
                if (pos.getDistanceFrom ({ vToX (v, ga), mToY (m, minM, maxM, ga) }) <= hitRadius)
                {
                    setMouseCursor (juce::MouseCursor::DraggingHandCursor);
                    return;
                }
            }
        }
        setMouseCursor (juce::MouseCursor::NormalCursor);
    }

    //==========================================================================
    void timerCallback() override
    {
        if (! resetPressActive) { stopTimer(); return; }
        auto elapsed = juce::Time::currentTimeMillis() - resetPressStartMs;
        if (elapsed >= resetPressDurationMs)
        {
            resetPressActive = false;
            stopTimer();
            repaint();
            if (onResetLongPress) onResetLongPress();
        }
        else
        {
            repaint();  // redraw progress
        }
    }

private:
    ADMOSCMapping::CartesianMappingConfig cfg;
    juce::Rectangle<int> graphAreas[3];   // [0]=X, [1]=Y, [2]=Z
    juce::Rectangle<int> rectArea;        // XY overview
    int dragAxis  = -1;
    int dragPoint = -1;
    float dragStartV = 0.0f;   // initial normalized value at drag start (for Alt precision)
    float dragStartM = 0.0f;   // initial meters value at drag start (for Alt precision)

    // Reset long-press state
    juce::Rectangle<float> resetHitArea;
    bool resetPressActive = false;
    int64_t resetPressStartMs = 0;
    static constexpr int resetPressDurationMs = 1000;

    // Hit-test areas for clickable swap/flip text (computed in paint, used in mouseDown)
    juce::Rectangle<float> swapHitAreas[3];
    juce::Rectangle<float> flipHitAreas[3];

    // Inline value editing
    juce::TextEditor valueEditor;
    bool isEditingValue = false;
    int editAxis = -1;
    int editPoint = -1;
    bool editIsBreakpoint = false;
    juce::Rectangle<float> dotLabelRects[3][5];  // [axis][point], stored during paint
    juce::Rectangle<float> bpLabelRects[3];      // breakpoint labels, stored during paint

    //==========================================================================
    // Helpers: coordinate transforms
    //==========================================================================

    static juce::Rectangle<float> getGraphSubArea (juce::Rectangle<float> panel)
    {
        const float padL = 32.0f, padR = 6.0f, padT = 36.0f, padB = 16.0f;
        return { panel.getX() + padL, panel.getY() + padT,
                 panel.getWidth() - padL - padR,
                 panel.getHeight() - padT - padB };
    }

    static float vToX (float v, const juce::Rectangle<float>& ga)
    {
        return ga.getX() + (v + 1.0f) * 0.5f * ga.getWidth();
    }
    static float mToY (float m, float minM, float maxM, const juce::Rectangle<float>& ga)
    {
        return ga.getBottom() - ((m - minM) / (maxM - minM)) * ga.getHeight();
    }
    static float xToV (float px, const juce::Rectangle<float>& ga)
    {
        return ((px - ga.getX()) / ga.getWidth()) * 2.0f - 1.0f;
    }
    static float yToM (float py, float minM, float maxM, const juce::Rectangle<float>& ga)
    {
        return minM + (1.0f - (py - ga.getY()) / ga.getHeight()) * (maxM - minM);
    }

    //==========================================================================
    // Point and range helpers
    //==========================================================================

    void getPointVM (int axis, int pt, float& v, float& m) const
    {
        const auto& ax = cfg.axes[axis];
        float c = ax.centerOffset;
        switch (pt)
        {
            case 0: v = -1.0f;          m = c - (ax.negInnerWidth + ax.negOuterWidth); break;
            case 1: v = -ax.breakpoint;  m = c - ax.negInnerWidth;                      break;
            case 2: v =  0.0f;           m = c;                                          break;
            case 3: v =  ax.breakpoint;  m = c + ax.posInnerWidth;                      break;
            case 4: v =  1.0f;           m = c + (ax.posInnerWidth + ax.posOuterWidth); break;
            default: v = 0.0f; m = 0.0f; break;
        }
    }

    void getAxisYRange (int axis, float& minM, float& maxM) const
    {
        float vals[5];
        for (int p = 0; p < 5; ++p)
        {
            float v;
            getPointVM (axis, p, v, vals[p]);
        }
        minM = vals[0]; maxM = vals[0];
        for (int i = 1; i < 5; ++i)
        {
            minM = juce::jmin (minM, vals[i]);
            maxM = juce::jmax (maxM, vals[i]);
        }
        if (maxM - minM < 0.1f) { minM -= 1.0f; maxM += 1.0f; }
        float range = maxM - minM;
        minM -= range * 0.08f;
        maxM += range * 0.08f;
    }

    //==========================================================================
    // Swap popup
    //==========================================================================

    void showSwapPopupMenu (int axis)
    {
        juce::PopupMenu menu;
        menu.addItem (1, "ADM X", true, cfg.axes[axis].axisSwap == 0);
        menu.addItem (2, "ADM Y", true, cfg.axes[axis].axisSwap == 1);
        menu.addItem (3, "ADM Z", true, cfg.axes[axis].axisSwap == 2);

        menu.showMenuAsync (juce::PopupMenu::Options(),
            [this, axis] (int result)
            {
                if (result > 0)
                {
                    cfg.axes[axis].axisSwap = result - 1;
                    repaint();
                    if (onSwapChanged)
                        onSwapChanged (axis, result - 1);
                }
            });
    }

    //==========================================================================
    // Inline editing helpers
    //==========================================================================

    void showValueEditor (const juce::Rectangle<float>& labelRect, const juce::String& currentText)
    {
        isEditingValue = true;
        auto editorBounds = labelRect.expanded (4.0f, 2.0f).toNearestInt();
        editorBounds.setWidth (juce::jmax (editorBounds.getWidth(), 60));
        editorBounds.setHeight (juce::jmax (editorBounds.getHeight(), 22));
        valueEditor.setBounds (editorBounds);
        valueEditor.setText (currentText, juce::dontSendNotification);
        valueEditor.setVisible (true);
        valueEditor.selectAll();
        valueEditor.grabKeyboardFocus();
    }

    void commitEdit()
    {
        if (! isEditingValue) return;
        isEditingValue = false;
        valueEditor.setVisible (false);

        float val = valueEditor.getText().getFloatValue();

        if (editIsBreakpoint)
        {
            val = juce::jlimit (0.01f, 0.99f, val);
            cfg.axes[editAxis].breakpoint = val;
            repaint();
            if (onParameterDragged)
            {
                auto& ax = cfg.axes[editAxis];
                onParameterDragged (editAxis, 3, ax.breakpoint,
                                    ax.posInnerWidth, ax.posOuterWidth,
                                    ax.centerOffset, true);
                onParameterDragged (editAxis, 1, ax.breakpoint,
                                    ax.negInnerWidth, ax.negOuterWidth,
                                    ax.centerOffset, false);
            }
            return;
        }

        auto& ax = cfg.axes[editAxis];
        constexpr float wMin = WFSParameterDefaults::admCartWidthMin;
        constexpr float wMax = WFSParameterDefaults::admCartWidthMax;
        bool posDir = true;

        switch (editPoint)
        {
            case 0:
            {
                float newOuter = (ax.centerOffset - val) - ax.negInnerWidth;
                ax.negOuterWidth = juce::jlimit (wMin, wMax, newOuter);
                posDir = false;
                break;
            }
            case 1:
            {
                float newInner = ax.centerOffset - val;
                ax.negInnerWidth = juce::jlimit (wMin, wMax, newInner);
                posDir = false;
                break;
            }
            case 2:
                ax.centerOffset = juce::jlimit (-50.0f, 50.0f, val);
                break;
            case 3:
            {
                float newInner = val - ax.centerOffset;
                ax.posInnerWidth = juce::jlimit (wMin, wMax, newInner);
                posDir = true;
                break;
            }
            case 4:
            {
                float newOuter = val - ax.centerOffset - ax.posInnerWidth;
                ax.posOuterWidth = juce::jlimit (wMin, wMax, newOuter);
                posDir = true;
                break;
            }
        }

        repaint();

        if (onParameterDragged)
        {
            float innerW = posDir ? ax.posInnerWidth : ax.negInnerWidth;
            float outerW = posDir ? ax.posOuterWidth : ax.negOuterWidth;
            onParameterDragged (editAxis, editPoint, ax.breakpoint,
                                innerW, outerW, ax.centerOffset, posDir);
        }
    }

    void cancelEdit()
    {
        isEditingValue = false;
        valueEditor.setVisible (false);
    }

    //==========================================================================
    // Painting
    //==========================================================================

    void paintAxisGraph (juce::Graphics& g, int axis, const char* title,
                         const juce::Rectangle<int>& bounds)
    {
        auto bf = bounds.toFloat();
        auto ga = getGraphSubArea (bf);
        float minM, maxM;
        getAxisYRange (axis, minM, maxM);

        const auto& ax = cfg.axes[axis];

        // Background
        g.setColour (juce::Colour (0xff2a2a2a));
        g.fillRoundedRectangle (bf, 4.0f);

        // --- Header: title + swap + flip ---
        {
            float hx = bf.getX() + 4.0f;
            float hy = bf.getY() + 2.0f;
            g.setFont (14.0f);

            // Swap text (clickable)
            static const char* swapNames[3] = { "ADM X", "ADM Y", "ADM Z" };
            juce::String swapText;
            if (ax.axisSwap == axis)
                swapText = juce::String (title) + ":  " + swapNames[ax.axisSwap];
            else
                swapText = juce::String (title) + " -> " + swapNames[ax.axisSwap];

            g.setColour (juce::Colour (0xffcccccc));
            float swapW = swapText.length() * 8.0f + 14.0f;  // approximate at 14pt
            swapHitAreas[axis] = { hx, hy, swapW, 18.0f };
            g.drawText (swapText, (int) hx, (int) hy, (int) swapW, 18,
                        juce::Justification::centredLeft);

            // Small dropdown triangle
            float triX = hx + swapW - 2.0f;
            float triY = hy + 6.0f;
            juce::Path tri;
            tri.addTriangle (triX, triY, triX + 6.0f, triY, triX + 3.0f, triY + 4.0f);
            g.setColour (juce::Colour (0xff999999));
            g.fillPath (tri);

            // Flip indicator (clickable)
            float flipX = hx + swapW + 10.0f;
            juce::String flipText = ax.signFlip ? "Flip: ON" : "Flip: OFF";
            float flipW = flipText.length() * 8.0f + 4.0f;
            flipHitAreas[axis] = { flipX, hy, flipW, 18.0f };

            if (ax.signFlip)
                g.setColour (juce::Colour (0xff4fc3f7));  // accent when active
            else
                g.setColour (juce::Colour (0xff888888));
            g.drawText (flipText, (int) flipX, (int) hy, (int) flipW, 18,
                        juce::Justification::centredLeft);

            // Breakpoint value label (right side of header)
            juce::String bpText = "bp: " + juce::String (ax.breakpoint, 2);
            g.setColour (juce::Colour (0xff999999));
            bpLabelRects[axis] = { bf.getRight() - 64.0f, hy, 60.0f, 18.0f };
            g.drawText (bpText, (int) bf.getRight() - 64, (int) hy, 60, 18,
                        juce::Justification::centredRight);
        }

        // Grid: zero lines
        g.setColour (juce::Colour (0xff555555));
        float zeroX = vToX (0.0f, ga);
        float zeroY = mToY (0.0f, minM, maxM, ga);
        g.drawVerticalLine ((int) zeroX, ga.getY(), ga.getBottom());
        if (zeroY >= ga.getY() && zeroY <= ga.getBottom())
            g.drawHorizontalLine ((int) zeroY, ga.getX(), ga.getRight());

        // Breakpoint dashed lines
        g.setColour (juce::Colour (0xff666666));
        float bpXpos = vToX (ax.breakpoint, ga);
        float bpXneg = vToX (-ax.breakpoint, ga);
        const float dashLengths[] = { 4.0f, 4.0f };
        g.drawDashedLine (juce::Line<float> (bpXpos, ga.getY(), bpXpos, ga.getBottom()),
                          dashLengths, 2, 1.0f);
        g.drawDashedLine (juce::Line<float> (bpXneg, ga.getY(), bpXneg, ga.getBottom()),
                          dashLengths, 2, 1.0f);

        // Inner zone shading
        g.setColour (juce::Colour (0x15ffffff));
        g.fillRect (bpXneg, ga.getY(), bpXpos - bpXneg, ga.getHeight());

        // 5 control points
        float ptV[5], ptM[5];
        for (int p = 0; p < 5; ++p)
            getPointVM (axis, p, ptV[p], ptM[p]);

        // Curve
        juce::Path curve;
        curve.startNewSubPath (vToX (ptV[0], ga), mToY (ptM[0], minM, maxM, ga));
        for (int p = 1; p < 5; ++p)
            curve.lineTo (vToX (ptV[p], ga), mToY (ptM[p], minM, maxM, ga));
        g.setColour (juce::Colour (0xff4fc3f7));
        g.strokePath (curve, juce::PathStrokeType (2.0f));

        // Dots + value labels
        g.setFont (12.0f);
        for (int p = 0; p < 5; ++p)
        {
            float dx = vToX (ptV[p], ga);
            float dy = mToY (ptM[p], minM, maxM, ga);

            // Dot
            bool isActive = (dragAxis == axis && dragPoint == p);
            g.setColour (isActive ? juce::Colour (0xff4fc3f7) : juce::Colour (0xffffffff));
            float dotR = isActive ? 5.0f : 3.5f;
            g.fillEllipse (dx - dotR, dy - dotR, dotR * 2.0f, dotR * 2.0f);

            // Value label near each dot
            juce::String valText = juce::String (ptM[p], 2) + "m";
            g.setColour (juce::Colour (0xffbbbbbb));
            int labelW = 48;
            juce::Rectangle<float> lr;
            if (p <= 1)
            {
                lr = { dx - labelW - 4.0f, dy - 7.0f, (float) labelW, 14.0f };
                g.drawText (valText, lr.toNearestInt(), juce::Justification::centredRight);
            }
            else if (p == 2)
            {
                lr = { dx - labelW / 2.0f, dy - 18.0f, (float) labelW, 14.0f };
                g.drawText (valText, lr.toNearestInt(), juce::Justification::centred);
            }
            else
            {
                lr = { dx + 4.0f, dy - 7.0f, (float) labelW, 14.0f };
                g.drawText (valText, lr.toNearestInt(), juce::Justification::centredLeft);
            }
            dotLabelRects[axis][p] = lr;
        }

        // X-axis labels
        g.setFont (11.0f);
        g.setColour (juce::Colour (0xffaaaaaa));
        float labelY = ga.getBottom() + 2.0f;
        g.drawText ("-1",  (int) vToX (-1.0f, ga) - 10, (int) labelY, 20, 14, juce::Justification::centred);
        g.drawText ("0",   (int) zeroX - 10,            (int) labelY, 20, 14, juce::Justification::centred);
        g.drawText ("+1",  (int) vToX (1.0f, ga) - 10,  (int) labelY, 20, 14, juce::Justification::centred);
    }

    //--------------------------------------------------------------------------

    void paintXYRect (juce::Graphics& g, const juce::Rectangle<int>& bounds)
    {
        auto bf = bounds.toFloat();

        // Background
        g.setColour (juce::Colour (0xff2a2a2a));
        g.fillRoundedRectangle (bf, 4.0f);

        // Compute extents for X (axis 0) and Y (axis 1)
        const auto& axX = cfg.axes[0];
        const auto& axY = cfg.axes[1];

        float xNegFull = axX.negInnerWidth + axX.negOuterWidth;
        float xPosFull = axX.posInnerWidth + axX.posOuterWidth;
        float yNegFull = axY.negInnerWidth + axY.negOuterWidth;
        float yPosFull = axY.posInnerWidth + axY.posOuterWidth;

        float xSpan = xNegFull + xPosFull;
        float ySpan = yNegFull + yPosFull;
        if (xSpan < 0.01f) xSpan = 1.0f;
        if (ySpan < 0.01f) ySpan = 1.0f;

        auto drawArea = bf.reduced (24.0f, 20.0f);

        float scaleX = drawArea.getWidth()  / xSpan;
        float scaleY = drawArea.getHeight() / ySpan;
        float scale  = juce::jmin (scaleX, scaleY);

        float drawW = xSpan * scale;
        float drawH = ySpan * scale;
        float ox = drawArea.getCentreX() - drawW * 0.5f;
        float oy = drawArea.getCentreY() - drawH * 0.5f;

        auto mToPixX = [&](float fromNeg) { return ox + fromNeg * scale; };
        auto mToPixY = [&](float fromNeg) { return oy + drawH - fromNeg * scale; };

        // Outer rectangle
        float outerL = mToPixX (0.0f);
        float outerR = mToPixX (xSpan);
        float outerT = mToPixY (ySpan);
        float outerB = mToPixY (0.0f);
        g.setColour (juce::Colour (0x10ffffff));
        g.fillRect (outerL, outerT, outerR - outerL, outerB - outerT);
        g.setColour (juce::Colour (0xff888888));
        g.drawRect (outerL, outerT, outerR - outerL, outerB - outerT, 1.0f);

        // Inner rectangle (breakpoint boundary)
        float innerL = mToPixX (xNegFull - axX.negInnerWidth);
        float innerR = mToPixX (xNegFull + axX.posInnerWidth);
        float innerT = mToPixY (yNegFull + axY.posInnerWidth);
        float innerB = mToPixY (yNegFull - axY.negInnerWidth);
        g.setColour (juce::Colour (0x20ffffff));
        g.fillRect (innerL, innerT, innerR - innerL, innerB - innerT);
        g.setColour (juce::Colour (0xffaaaaaa));
        g.drawRect (innerL, innerT, innerR - innerL, innerB - innerT, 1.0f);

        // Center offset crosshair
        float cx = mToPixX (xNegFull);
        float cy = mToPixY (yNegFull);
        g.setColour (juce::Colour (0xff666666));
        const float chDash[] = { 3.0f, 3.0f };
        g.drawDashedLine (juce::Line<float> (cx, outerT, cx, outerB), chDash, 2, 1.0f);
        g.drawDashedLine (juce::Line<float> (outerL, cy, outerR, cy), chDash, 2, 1.0f);

        // Side labels
        g.setFont (13.0f);
        g.setColour (juce::Colour (0xffaaaaaa));
        g.drawText ("+X", (int) outerR + 2, (int) cy - 8, 22, 16, juce::Justification::centredLeft);
        g.drawText ("-X", (int) outerL - 24, (int) cy - 8, 22, 16, juce::Justification::centredRight);
        g.drawText ("+Y", (int) cx - 12, (int) outerT - 17, 24, 16, juce::Justification::centred);
        g.drawText ("-Y", (int) cx - 12, (int) outerB + 2,  24, 16, juce::Justification::centred);

        // Title
        g.setFont (14.0f);
        g.setColour (juce::Colour (0xffcccccc));
        g.drawText ("XY", (int) bf.getX() + 4, (int) bf.getY() + 2, 24, 16,
                    juce::Justification::centredLeft);

        // Reset text (long press) — top-right of XY rect
        {
            float resetW = 48.0f, resetH = 16.0f;
            float resetX = bf.getRight() - resetW - 4.0f;
            float resetY = bf.getY() + 2.0f;
            resetHitArea = { resetX, resetY, resetW, resetH };

            if (resetPressActive)
            {
                auto elapsed = juce::Time::currentTimeMillis() - resetPressStartMs;
                float progress = juce::jlimit (0.0f, 1.0f, (float) elapsed / (float) resetPressDurationMs);
                bool reached = progress >= 1.0f;

                // Progress bar background
                g.setColour (reached ? juce::Colour (0x4066bb6a) : juce::Colour (0x404fc3f7));
                g.fillRoundedRectangle (resetX, resetY, resetW * progress, resetH, 3.0f);

                g.setColour (reached ? juce::Colour (0xff66bb6a) : juce::Colour (0xff4fc3f7));
            }
            else
            {
                g.setColour (juce::Colour (0xff888888));
            }

            g.setFont (11.0f);
            g.drawText ("Reset", (int) resetX, (int) resetY, (int) resetW, (int) resetH,
                        juce::Justification::centred);
        }
    }
};

/** Panel showing piecewise-linear ADM-OSC Polar distance mapping curve (half-axis, 3 points)
    plus a top-down circle overview with distance rings and azimuth offset line. */
class AdmPolarPanel : public juce::Component, private juce::Timer
{
public:
    AdmPolarPanel()
    {
        addChildComponent (valueEditor);
        valueEditor.setMultiLine (false);
        valueEditor.setReturnKeyStartsNewLine (false);
        valueEditor.setScrollbarsShown (false);
        valueEditor.setFont (juce::Font (juce::FontOptions (12.0f)));
        valueEditor.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff1a1a1a));
        valueEditor.setColour (juce::TextEditor::textColourId, juce::Colour (0xffe0e0e0));
        valueEditor.setColour (juce::TextEditor::outlineColourId, juce::Colour (0xff4fc3f7));
        valueEditor.setColour (juce::TextEditor::focusedOutlineColourId, juce::Colour (0xff4fc3f7));
        valueEditor.setJustification (juce::Justification::centred);

        valueEditor.onReturnKey = [this] { commitEdit(); };
        valueEditor.onEscapeKey = [this] { cancelEdit(); };
        valueEditor.onFocusLost = [this] { juce::MessageManager::callAsync ([this] { commitEdit(); }); };
    }

    ~AdmPolarPanel() override { stopTimer(); }

    void setConfig (const ADMOSCMapping::PolarMappingConfig& newCfg)
    {
        cfg = newCfg;
        repaint();
    }

    /** Callback: distance point dragged. pointIndex: 0=center, 1=breakpoint, 2=max */
    std::function<void (int pointIndex, float newBreakpoint, float newInner,
                        float newOuter, float newCenter)> onDistDragged;

    /** Callback: azimuth offset changed (from circle drag) */
    std::function<void (float newAzOffset)> onAzOffsetChanged;

    /** Callback: flip toggles */
    std::function<void (bool azFlip)> onAzFlipChanged;
    std::function<void (bool elFlip)> onElFlipChanged;

    /** Callback: reset long-press completed */
    std::function<void()> onResetLongPress;

    //==========================================================================
    void resized() override
    {
        int halfW = getWidth() / 2;
        distGraphArea = { 0, 0, halfW - 2, getHeight() };
        circleArea    = { halfW + 2, 0, getWidth() - halfW - 2, getHeight() };
    }

    //==========================================================================
    void paint (juce::Graphics& g) override
    {
        paintDistGraph (g, distGraphArea);
        paintCircleOverview (g, circleArea);
    }

    //==========================================================================
    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        auto pos = e.position;

        // Breakpoint label
        if (bpLabelRect.contains (pos))
        {
            editTargetType = EditTarget::Breakpoint;
            editPoint = -1;
            showValueEditor (bpLabelRect, juce::String (cfg.distBreakpoint, 2));
            return;
        }

        // Azimuth offset label (header)
        if (azOffsetLabelRect.contains (pos))
        {
            editTargetType = EditTarget::AzOffset;
            editPoint = -1;
            showValueEditor (azOffsetLabelRect, juce::String (cfg.azimuthOffset, 1));
            return;
        }

        // Azimuth offset label (circle endpoint)
        if (azCircleLabelRect.contains (pos))
        {
            editTargetType = EditTarget::AzOffset;
            editPoint = -1;
            showValueEditor (azCircleLabelRect, juce::String (cfg.azimuthOffset, 1));
            return;
        }

        // Distance dot labels
        for (int p = 0; p < 3; ++p)
        {
            if (distDotLabelRects[p].contains (pos))
            {
                editTargetType = EditTarget::DistDot;
                editPoint = p;
                float v, m;
                getDistPointVM (p, v, m);
                showValueEditor (distDotLabelRects[p], juce::String (m, 2));
                return;
            }
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (isEditingValue)
        {
            commitEdit();
            return;
        }

        dragTarget = DragNone;
        auto pos = e.position;

        // Check reset hit area (long press)
        if (resetHitArea.contains (pos))
        {
            resetPressActive = true;
            resetPressStartMs = juce::Time::currentTimeMillis();
            startTimer (50);
            repaint();
            return;
        }

        // Check flip hit areas
        if (azFlipHitArea.contains (pos))
        {
            cfg.azimuthFlip = ! cfg.azimuthFlip;
            repaint();
            if (onAzFlipChanged) onAzFlipChanged (cfg.azimuthFlip);
            return;
        }
        if (elFlipHitArea.contains (pos))
        {
            cfg.elevationFlip = ! cfg.elevationFlip;
            repaint();
            if (onElFlipChanged) onElFlipChanged (cfg.elevationFlip);
            return;
        }

        // Check distance graph dots
        constexpr float hitRadius = 10.0f;
        auto ga = getDistGraphSubArea (distGraphArea.toFloat());
        float minM, maxM;
        getDistYRange (minM, maxM);

        for (int p = 0; p < 3; ++p)
        {
            float v, m;
            getDistPointVM (p, v, m);
            float px = dToX (v, ga);
            float py = dMToY (m, minM, maxM, ga);
            if (pos.getDistanceFrom ({ px, py }) <= hitRadius)
            {
                dragTarget = (DragTarget) (DragDistP0 + p);
                dragStartV = v;
                dragStartM = m;
                setMouseCursor (juce::MouseCursor::DraggingHandCursor);
                return;
            }
        }

        // Check azimuth offset line drag (in circle area)
        if (circleArea.toFloat().contains (pos))
        {
            auto ca = getCircleSubArea (circleArea.toFloat());
            float cx = ca.getCentreX();
            float cy = ca.getCentreY();
            float maxR = juce::jmin (ca.getWidth(), ca.getHeight()) * 0.5f - 8.0f;

            // Check if near the azimuth offset line endpoint
            float azRad = juce::degreesToRadians (cfg.azimuthOffset);
            float lineEndX = cx + maxR * std::sin (azRad);
            float lineEndY = cy - maxR * std::cos (azRad);
            if (pos.getDistanceFrom ({ lineEndX, lineEndY }) <= hitRadius * 1.5f)
            {
                dragTarget = DragAzOffset;
                setMouseCursor (juce::MouseCursor::DraggingHandCursor);
                return;
            }

        }
    }

    //==========================================================================
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragTarget == DragNone) return;

        if (dragTarget >= DragDistP0 && dragTarget <= DragDistP2)
        {
            auto ga = getDistGraphSubArea (distGraphArea.toFloat());
            float minM, maxM;
            getDistYRange (minM, maxM);

            float rawV = xToD (e.position.x, ga);
            float rawM = dYToM (e.position.y, minM, maxM, ga);

            float newV = rawV;
            float newM = rawM;
            if (e.mods.isAltDown())
            {
                newV = dragStartV + (rawV - dragStartV) * 0.1f;
                newM = dragStartM + (rawM - dragStartM) * 0.1f;
            }

            constexpr float wMin = WFSParameterDefaults::admCartWidthMin;  // 0.1
            constexpr float wMax = WFSParameterDefaults::admCartWidthMax;  // 50.0

            int pt = dragTarget - DragDistP0;
            switch (pt)
            {
                case 0: // center offset (vertical drag only)
                    cfg.distCenter = juce::jlimit (-50.0f, 50.0f, newM);
                    break;
                case 1: // breakpoint dot: horiz -> bp, vert -> inner
                {
                    cfg.distBreakpoint = juce::jlimit (0.01f, 0.99f, newV);
                    float newInner = newM - cfg.distCenter;
                    cfg.distInner = juce::jlimit (wMin, wMax, newInner);
                    if (e.mods.isShiftDown())
                        cfg.distOuter = cfg.distInner;
                    break;
                }
                case 2: // max dot (vertical drag only)
                {
                    float newOuter = newM - cfg.distCenter - cfg.distInner;
                    cfg.distOuter = juce::jlimit (wMin, wMax, newOuter);
                    if (e.mods.isShiftDown())
                        cfg.distInner = cfg.distOuter;
                    break;
                }
            }

            repaint();
            if (onDistDragged)
                onDistDragged (pt, cfg.distBreakpoint, cfg.distInner,
                               cfg.distOuter, cfg.distCenter);
        }
        else if (dragTarget == DragAzOffset)
        {
            auto ca = getCircleSubArea (circleArea.toFloat());
            float cx = ca.getCentreX();
            float cy = ca.getCentreY();
            float dx = e.position.x - cx;
            float dy = -(e.position.y - cy);  // flip Y for math
            float angle = juce::radiansToDegrees (std::atan2 (dx, dy));
            cfg.azimuthOffset = juce::jlimit (-180.0f, 180.0f, angle);
            repaint();
            if (onAzOffsetChanged)
                onAzOffsetChanged (cfg.azimuthOffset);
        }
    }

    //==========================================================================
    void mouseUp (const juce::MouseEvent&) override
    {
        if (resetPressActive)
        {
            resetPressActive = false;
            stopTimer();
            repaint();
        }
        dragTarget = DragNone;
        setMouseCursor (juce::MouseCursor::NormalCursor);
    }

    //==========================================================================
    void mouseMove (const juce::MouseEvent& e) override
    {
        auto pos = e.position;
        if (resetHitArea.contains (pos) || azFlipHitArea.contains (pos) || elFlipHitArea.contains (pos))
        {
            setMouseCursor (juce::MouseCursor::PointingHandCursor);
            return;
        }

        constexpr float hitRadius = 10.0f;
        // Check distance graph dots
        if (distGraphArea.toFloat().contains (pos))
        {
            auto ga = getDistGraphSubArea (distGraphArea.toFloat());
            float minM, maxM;
            getDistYRange (minM, maxM);
            for (int p = 0; p < 3; ++p)
            {
                float v, m;
                getDistPointVM (p, v, m);
                float px = dToX (v, ga);
                float py = dMToY (m, minM, maxM, ga);
                if (pos.getDistanceFrom ({ px, py }) <= hitRadius)
                {
                    setMouseCursor (juce::MouseCursor::DraggingHandCursor);
                    return;
                }
            }
        }
        // Check azimuth line endpoint
        if (circleArea.toFloat().contains (pos))
        {
            auto ca = getCircleSubArea (circleArea.toFloat());
            float cx = ca.getCentreX();
            float cy = ca.getCentreY();
            float maxR = juce::jmin (ca.getWidth(), ca.getHeight()) * 0.5f - 8.0f;
            float azRad = juce::degreesToRadians (cfg.azimuthOffset);
            float lineEndX = cx + maxR * std::sin (azRad);
            float lineEndY = cy - maxR * std::cos (azRad);
            if (pos.getDistanceFrom ({ lineEndX, lineEndY }) <= hitRadius * 1.5f)
            {
                setMouseCursor (juce::MouseCursor::DraggingHandCursor);
                return;
            }
        }
        setMouseCursor (juce::MouseCursor::NormalCursor);
    }

    //==========================================================================
    void timerCallback() override
    {
        if (! resetPressActive) { stopTimer(); return; }
        auto elapsed = juce::Time::currentTimeMillis() - resetPressStartMs;
        if (elapsed >= resetPressDurationMs)
        {
            resetPressActive = false;
            stopTimer();
            repaint();
            if (onResetLongPress) onResetLongPress();
        }
        else
        {
            repaint();
        }
    }

private:
    ADMOSCMapping::PolarMappingConfig cfg;
    juce::Rectangle<int> distGraphArea;
    juce::Rectangle<int> circleArea;

    enum DragTarget { DragNone = -1, DragDistP0 = 0, DragDistP1 = 1, DragDistP2 = 2, DragAzOffset = 10 };
    DragTarget dragTarget = DragNone;
    float dragStartV = 0.0f;
    float dragStartM = 0.0f;

    // Reset long-press state
    juce::Rectangle<float> resetHitArea;
    bool resetPressActive = false;
    int64_t resetPressStartMs = 0;
    static constexpr int resetPressDurationMs = 1000;

    juce::Rectangle<float> azFlipHitArea;
    juce::Rectangle<float> elFlipHitArea;

    // Inline value editing
    juce::TextEditor valueEditor;
    bool isEditingValue = false;
    int editPoint = -1;
    enum class EditTarget { DistDot, Breakpoint, AzOffset };
    EditTarget editTargetType = EditTarget::DistDot;
    juce::Rectangle<float> distDotLabelRects[3];
    juce::Rectangle<float> bpLabelRect;
    juce::Rectangle<float> azOffsetLabelRect;
    juce::Rectangle<float> azCircleLabelRect;

    //==========================================================================
    // Inline editing helpers
    //==========================================================================

    void showValueEditor (const juce::Rectangle<float>& labelRect, const juce::String& currentText)
    {
        isEditingValue = true;
        auto editorBounds = labelRect.expanded (4.0f, 2.0f).toNearestInt();
        editorBounds.setWidth (juce::jmax (editorBounds.getWidth(), 60));
        editorBounds.setHeight (juce::jmax (editorBounds.getHeight(), 22));
        valueEditor.setBounds (editorBounds);
        valueEditor.setText (currentText, juce::dontSendNotification);
        valueEditor.setVisible (true);
        valueEditor.selectAll();
        valueEditor.grabKeyboardFocus();
    }

    void commitEdit()
    {
        if (! isEditingValue) return;
        isEditingValue = false;
        valueEditor.setVisible (false);

        float val = valueEditor.getText().getFloatValue();

        constexpr float wMin = WFSParameterDefaults::admCartWidthMin;
        constexpr float wMax = WFSParameterDefaults::admCartWidthMax;

        switch (editTargetType)
        {
            case EditTarget::Breakpoint:
                cfg.distBreakpoint = juce::jlimit (0.01f, 0.99f, val);
                repaint();
                if (onDistDragged)
                    onDistDragged (1, cfg.distBreakpoint, cfg.distInner,
                                   cfg.distOuter, cfg.distCenter);
                break;

            case EditTarget::AzOffset:
                cfg.azimuthOffset = juce::jlimit (-180.0f, 180.0f, val);
                repaint();
                if (onAzOffsetChanged)
                    onAzOffsetChanged (cfg.azimuthOffset);
                break;

            case EditTarget::DistDot:
                switch (editPoint)
                {
                    case 0:
                        cfg.distCenter = juce::jlimit (-50.0f, 50.0f, val);
                        break;
                    case 1:
                    {
                        float newInner = val - cfg.distCenter;
                        cfg.distInner = juce::jlimit (wMin, wMax, newInner);
                        break;
                    }
                    case 2:
                    {
                        float newOuter = val - cfg.distCenter - cfg.distInner;
                        cfg.distOuter = juce::jlimit (wMin, wMax, newOuter);
                        break;
                    }
                }
                repaint();
                if (onDistDragged)
                    onDistDragged (editPoint, cfg.distBreakpoint, cfg.distInner,
                                   cfg.distOuter, cfg.distCenter);
                break;
        }
    }

    void cancelEdit()
    {
        isEditingValue = false;
        valueEditor.setVisible (false);
    }

    //==========================================================================
    // Distance graph helpers
    //==========================================================================

    static juce::Rectangle<float> getDistGraphSubArea (juce::Rectangle<float> panel)
    {
        const float padL = 36.0f, padR = 6.0f, padT = 40.0f, padB = 16.0f;
        return { panel.getX() + padL, panel.getY() + padT,
                 panel.getWidth() - padL - padR, panel.getHeight() - padT - padB };
    }

    static juce::Rectangle<float> getCircleSubArea (juce::Rectangle<float> panel)
    {
        return panel.reduced (8.0f, 28.0f);
    }

    // Normalized distance [0, 1] to pixel X
    static float dToX (float d, const juce::Rectangle<float>& ga)
    {
        return ga.getX() + d * ga.getWidth();
    }
    static float dMToY (float m, float minM, float maxM, const juce::Rectangle<float>& ga)
    {
        return ga.getBottom() - ((m - minM) / (maxM - minM)) * ga.getHeight();
    }
    static float xToD (float px, const juce::Rectangle<float>& ga)
    {
        return (px - ga.getX()) / ga.getWidth();
    }
    static float dYToM (float py, float minM, float maxM, const juce::Rectangle<float>& ga)
    {
        return minM + (1.0f - (py - ga.getY()) / ga.getHeight()) * (maxM - minM);
    }

    void getDistPointVM (int pt, float& v, float& m) const
    {
        switch (pt)
        {
            case 0: v = 0.0f;               m = cfg.distCenter; break;
            case 1: v = cfg.distBreakpoint;  m = cfg.distCenter + cfg.distInner; break;
            case 2: v = 1.0f;               m = cfg.distCenter + cfg.distInner + cfg.distOuter; break;
            default: v = 0.0f; m = 0.0f; break;
        }
    }

    void getDistYRange (float& minM, float& maxM) const
    {
        float vals[3];
        for (int p = 0; p < 3; ++p)
        {
            float v;
            getDistPointVM (p, v, vals[p]);
        }
        minM = vals[0]; maxM = vals[0];
        for (int i = 1; i < 3; ++i)
        {
            minM = juce::jmin (minM, vals[i]);
            maxM = juce::jmax (maxM, vals[i]);
        }
        if (maxM - minM < 0.1f) { minM -= 1.0f; maxM += 1.0f; }
        float range = maxM - minM;
        minM -= range * 0.1f;
        maxM += range * 0.1f;
    }

    //==========================================================================
    // Painting
    //==========================================================================

    void paintDistGraph (juce::Graphics& g, const juce::Rectangle<int>& bounds)
    {
        auto bf = bounds.toFloat();
        auto ga = getDistGraphSubArea (bf);
        float minM, maxM;
        getDistYRange (minM, maxM);

        // Background
        g.setColour (juce::Colour (0xff2a2a2a));
        g.fillRoundedRectangle (bf, 4.0f);

        // Header: flip controls
        {
            float hx = bf.getX() + 4.0f;
            float hy = bf.getY() + 2.0f;
            g.setFont (14.0f);

            // Title
            g.setColour (juce::Colour (0xffcccccc));
            g.drawText ("Distance", (int) hx, (int) hy, 70, 18, juce::Justification::centredLeft);

            // Az flip
            float flipX = hx + 76.0f;
            juce::String azText = cfg.azimuthFlip ? "Az Flip: ON" : "Az Flip: OFF";
            float azW = azText.length() * 8.0f + 4.0f;
            azFlipHitArea = { flipX, hy, azW, 18.0f };
            g.setColour (cfg.azimuthFlip ? juce::Colour (0xff4fc3f7) : juce::Colour (0xff888888));
            g.drawText (azText, (int) flipX, (int) hy, (int) azW, 18, juce::Justification::centredLeft);

            // El flip
            float elX = flipX + azW + 12.0f;
            juce::String elText = cfg.elevationFlip ? "El Flip: ON" : "El Flip: OFF";
            float elW = elText.length() * 8.0f + 4.0f;
            elFlipHitArea = { elX, hy, elW, 18.0f };
            g.setColour (cfg.elevationFlip ? juce::Colour (0xff4fc3f7) : juce::Colour (0xff888888));
            g.drawText (elText, (int) elX, (int) hy, (int) elW, 18, juce::Justification::centredLeft);

            // Breakpoint value (right side)
            juce::String bpText = "bp: " + juce::String (cfg.distBreakpoint, 2);
            g.setColour (juce::Colour (0xff999999));
            bpLabelRect = { bf.getRight() - 64.0f, hy, 60.0f, 18.0f };
            g.drawText (bpText, (int) bf.getRight() - 64, (int) hy, 60, 18,
                        juce::Justification::centredRight);

            // Az offset value (second line)
            g.setFont (12.0f);
            juce::String azOffText = "Az Offset: " + juce::String (cfg.azimuthOffset, 1) + juce::String::fromUTF8 ("\xc2\xb0");
            g.setColour (juce::Colour (0xff999999));
            azOffsetLabelRect = { hx, hy + 18.0f, 140.0f, 16.0f };
            g.drawText (azOffText, (int) hx, (int) hy + 18, 140, 16, juce::Justification::centredLeft);

            // Reset text (long press) — right side of second line
            float resetW = 48.0f, resetH = 16.0f;
            float resetX = bf.getRight() - resetW - 4.0f;
            float resetY = hy + 18.0f;
            resetHitArea = { resetX, resetY, resetW, resetH };

            if (resetPressActive)
            {
                auto elapsed = juce::Time::currentTimeMillis() - resetPressStartMs;
                float progress = juce::jlimit (0.0f, 1.0f, (float) elapsed / (float) resetPressDurationMs);
                bool reached = progress >= 1.0f;
                g.setColour (reached ? juce::Colour (0x4066bb6a) : juce::Colour (0x404fc3f7));
                g.fillRoundedRectangle (resetX, resetY, resetW * progress, resetH, 3.0f);
                g.setColour (reached ? juce::Colour (0xff66bb6a) : juce::Colour (0xff4fc3f7));
            }
            else
            {
                g.setColour (juce::Colour (0xff888888));
            }
            g.setFont (11.0f);
            g.drawText ("Reset", (int) resetX, (int) resetY, (int) resetW, (int) resetH,
                        juce::Justification::centred);
        }

        // Grid: zero line
        g.setColour (juce::Colour (0xff555555));
        float zeroY = dMToY (0.0f, minM, maxM, ga);
        if (zeroY >= ga.getY() && zeroY <= ga.getBottom())
            g.drawHorizontalLine ((int) zeroY, ga.getX(), ga.getRight());

        // Breakpoint dashed line
        float bpX = dToX (cfg.distBreakpoint, ga);
        g.setColour (juce::Colour (0xff666666));
        const float dashLengths[] = { 4.0f, 4.0f };
        g.drawDashedLine (juce::Line<float> (bpX, ga.getY(), bpX, ga.getBottom()),
                          dashLengths, 2, 1.0f);

        // Inner zone shading
        g.setColour (juce::Colour (0x15ffffff));
        g.fillRect (ga.getX(), ga.getY(), bpX - ga.getX(), ga.getHeight());

        // 3 control points
        float ptV[3], ptM[3];
        for (int p = 0; p < 3; ++p)
            getDistPointVM (p, ptV[p], ptM[p]);

        // Curve
        juce::Path curve;
        curve.startNewSubPath (dToX (ptV[0], ga), dMToY (ptM[0], minM, maxM, ga));
        for (int p = 1; p < 3; ++p)
            curve.lineTo (dToX (ptV[p], ga), dMToY (ptM[p], minM, maxM, ga));
        g.setColour (juce::Colour (0xff4fc3f7));
        g.strokePath (curve, juce::PathStrokeType (2.0f));

        // Dots + value labels
        g.setFont (12.0f);
        for (int p = 0; p < 3; ++p)
        {
            float dx = dToX (ptV[p], ga);
            float dy = dMToY (ptM[p], minM, maxM, ga);

            bool isActive = (dragTarget == (DragTarget) (DragDistP0 + p));
            g.setColour (isActive ? juce::Colour (0xff4fc3f7) : juce::Colour (0xffffffff));
            float dotR = isActive ? 5.0f : 3.5f;
            g.fillEllipse (dx - dotR, dy - dotR, dotR * 2.0f, dotR * 2.0f);

            juce::String valText = juce::String (ptM[p], 2) + "m";
            g.setColour (juce::Colour (0xffbbbbbb));
            int labelW = 52;
            juce::Rectangle<float> lr;
            if (p == 0)
            {
                lr = { dx - labelW / 2.0f, dy - 18.0f, (float) labelW, 14.0f };
                g.drawText (valText, lr.toNearestInt(), juce::Justification::centred);
            }
            else
            {
                lr = { dx + 4.0f, dy - 7.0f, (float) labelW, 14.0f };
                g.drawText (valText, lr.toNearestInt(), juce::Justification::centredLeft);
            }
            distDotLabelRects[p] = lr;
        }

        // X-axis labels
        g.setFont (11.0f);
        g.setColour (juce::Colour (0xffaaaaaa));
        float labelY = ga.getBottom() + 2.0f;
        g.drawText ("0", (int) dToX (0.0f, ga) - 8, (int) labelY, 16, 14, juce::Justification::centred);
        g.drawText ("1", (int) dToX (1.0f, ga) - 8, (int) labelY, 16, 14, juce::Justification::centred);
    }

    //--------------------------------------------------------------------------

    void paintCircleOverview (juce::Graphics& g, const juce::Rectangle<int>& bounds)
    {
        auto bf = bounds.toFloat();
        auto ca = getCircleSubArea (bf);

        // Background
        g.setColour (juce::Colour (0xff2a2a2a));
        g.fillRoundedRectangle (bf, 4.0f);

        // Title
        g.setFont (14.0f);
        g.setColour (juce::Colour (0xffcccccc));
        g.drawText ("Top View", (int) bf.getX() + 4, (int) bf.getY() + 2, 76, 18,
                    juce::Justification::centredLeft);

        float cx = ca.getCentreX();
        float cy = ca.getCentreY();
        float maxR = juce::jmin (ca.getWidth(), ca.getHeight()) * 0.5f - 8.0f;

        float totalDist = cfg.distCenter + cfg.distInner + cfg.distOuter;
        if (totalDist < 0.01f) totalDist = 1.0f;
        float pxPerM = maxR / totalDist;

        // Outer circle (max distance)
        float outerR = totalDist * pxPerM;
        g.setColour (juce::Colour (0xff888888));
        g.drawEllipse (cx - outerR, cy - outerR, outerR * 2.0f, outerR * 2.0f, 1.0f);

        // Outer distance label
        g.setFont (12.0f);
        g.setColour (juce::Colour (0xffaaaaaa));
        juce::String outerLabel = juce::String (totalDist, 1) + "m";
        g.drawText (outerLabel, (int) (cx + outerR * 0.707f + 2), (int) (cy - outerR * 0.707f - 7),
                    40, 14, juce::Justification::centredLeft);

        // Inner circle (breakpoint boundary = distCenter + distInner)
        float innerDist = cfg.distCenter + cfg.distInner;
        float innerR = innerDist * pxPerM;
        if (innerR > 1.0f)
        {
            g.setColour (juce::Colour (0xffaaaaaa));
            const float dashLens[] = { 4.0f, 4.0f };
            // Draw dashed circle using path
            juce::Path innerCircle;
            innerCircle.addEllipse (cx - innerR, cy - innerR, innerR * 2.0f, innerR * 2.0f);
            g.strokePath (innerCircle, juce::PathStrokeType (1.0f));

            // Inner distance label
            juce::String innerLabel = juce::String (innerDist, 1) + "m";
            g.drawText (innerLabel, (int) (cx + innerR * 0.707f + 2), (int) (cy - innerR * 0.707f - 7),
                        40, 14, juce::Justification::centredLeft);
        }

        // Center circle (distCenter offset)
        if (cfg.distCenter > 0.01f)
        {
            float centerR = cfg.distCenter * pxPerM;
            g.setColour (juce::Colour (0xff666666));
            g.drawEllipse (cx - centerR, cy - centerR, centerR * 2.0f, centerR * 2.0f, 1.0f);

            juce::String centerLabel = juce::String (cfg.distCenter, 1) + "m";
            g.setColour (juce::Colour (0xff999999));
            g.drawText (centerLabel, (int) (cx + centerR + 2), (int) cy - 7,
                        40, 14, juce::Justification::centredLeft);
        }

        // Center dot
        g.setColour (juce::Colour (0xffffffff));
        g.fillEllipse (cx - 3.0f, cy - 3.0f, 6.0f, 6.0f);

        // Crosshair
        g.setColour (juce::Colour (0xff444444));
        g.drawVerticalLine ((int) cx, ca.getY(), ca.getBottom());
        g.drawHorizontalLine ((int) cy, ca.getX(), ca.getRight());

        // Cardinal labels
        g.setFont (11.0f);
        g.setColour (juce::Colour (0xff888888));
        g.drawText ("Front", (int) cx - 20, (int) ca.getBottom() - 14, 40, 14, juce::Justification::centred);
        g.drawText ("Back",  (int) cx - 20, (int) ca.getY(), 40, 14, juce::Justification::centred);
        g.drawText ("L", (int) ca.getX(), (int) cy - 7, 14, 14, juce::Justification::centred);
        g.drawText ("R", (int) ca.getRight() - 14, (int) cy - 7, 14, 14, juce::Justification::centred);

        // Azimuth offset line
        float azRad = juce::degreesToRadians (cfg.azimuthOffset);
        float lineEndX = cx + maxR * std::sin (azRad);
        float lineEndY = cy - maxR * std::cos (azRad);

        g.setColour (juce::Colour (0xff4fc3f7));
        g.drawLine (cx, cy, lineEndX, lineEndY, 2.0f);

        // Azimuth endpoint dot (draggable)
        g.fillEllipse (lineEndX - 4.0f, lineEndY - 4.0f, 8.0f, 8.0f);

        // Azimuth offset label near the dot
        g.setFont (12.0f);
        juce::String azLabel = juce::String (cfg.azimuthOffset, 1) + juce::String::fromUTF8 ("\xc2\xb0");
        g.setColour (juce::Colour (0xffbbbbbb));
        azCircleLabelRect = { lineEndX + 6.0f, lineEndY - 7.0f, 44.0f, 14.0f };
        g.drawText (azLabel, (int) lineEndX + 6, (int) lineEndY - 7, 44, 14,
                    juce::Justification::centredLeft);

        // Inner zone fill (semi-transparent)
        if (innerR > 1.0f)
        {
            g.setColour (juce::Colour (0x10ffffff));
            g.fillEllipse (cx - innerR, cy - innerR, innerR * 2.0f, innerR * 2.0f);
        }
    }
};

class NetworkTab : public juce::Component,
                   private juce::ValueTree::Listener,
                   private juce::TextEditor::Listener,
                   public ColorScheme::Manager::Listener
{
public:
    using NetworkLogWindowCallback = std::function<void()>;
    NetworkTab(WfsParameters& params, StatusBar* statusBarPtr = nullptr)
        : parameters(params), statusBar(statusBarPtr)
    {
        ColorScheme::Manager::getInstance().addListener(this);
        setFocusContainerType(FocusContainerType::keyboardFocusContainer);

        // ==================== NETWORK SECTION ====================
        addAndMakeVisible(networkInterfaceLabel);
        networkInterfaceLabel.setText(LOC("network.labels.interface"), juce::dontSendNotification);
        addAndMakeVisible(networkInterfaceSelector);
        networkInterfaceSelector.onChange = [this]() {
            onNetworkInterfaceChanged();
            // TTS: Announce selection change
            TTSManager::getInstance().announceValueChange(LOC("network.labels.interface"), networkInterfaceSelector.getText());
        };

        addAndMakeVisible(currentIPLabel);
        currentIPLabel.setText(LOC("network.labels.currentIPv4"), juce::dontSendNotification);
        addAndMakeVisible(currentIPEditor);
        currentIPEditor.setReadOnly(true);

        addAndMakeVisible(udpPortLabel);
        udpPortLabel.setText(LOC("network.labels.udpPort"), juce::dontSendNotification);
        addAndMakeVisible(udpPortEditor);

        addAndMakeVisible(tcpPortLabel);
        tcpPortLabel.setText(LOC("network.labels.tcpPort"), juce::dontSendNotification);
        addAndMakeVisible(tcpPortEditor);

        // ==================== OSC QUERY ====================
        addAndMakeVisible(oscQueryLabel);
        oscQueryLabel.setText(LOC("network.labels.oscQuery"), juce::dontSendNotification);
        addAndMakeVisible(oscQueryPortEditor);
        oscQueryPortEditor.setText("5005");
        oscQueryPortEditor.setInputRestrictions(5, "0123456789");
        oscQueryPortEditor.addListener(this);

        addAndMakeVisible(oscQueryEnableButton);
        oscQueryEnableButton.setButtonText(LOC("network.toggles.disabled"));
        oscQueryEnableButton.setClickingTogglesState(true);
        oscQueryEnableButton.onClick = [this]() {
            bool enabled = oscQueryEnableButton.getToggleState();
            oscQueryEnableButton.setButtonText(enabled ? LOC("network.toggles.enabled") : LOC("network.toggles.disabled"));
            saveOscQueryToValueTree();
            updateOSCQueryServer();
        };

        // ==================== NETWORK CONNECTIONS TABLE ====================
        setupNetworkConnectionsTable();

        // ==================== ADM-OSC SECTION ====================
        setupAdmOscSection();

        // ADM-OSC Help Card
        addAndMakeVisible(admOscHelpButton);
        addChildComponent(admOscHelpCard);
        admOscHelpCard.setContent(LOC("help.admOsc.title"), LOC("help.admOsc.body"));
        admOscHelpButton.setCard(&admOscHelpCard);

        // ==================== TRACKING SECTION ====================
        setupTrackingSection();

        // ==================== FOOTER BUTTONS ====================
        addAndMakeVisible(storeButton);
        storeButton.setButtonText(LOC("network.buttons.storeConfig"));
        storeButton.setBaseColour(juce::Colour(0xFF8C3333));  // Reddish
        storeButton.onLongPress = [this]() { storeNetworkConfiguration(); };

        addAndMakeVisible(reloadButton);
        reloadButton.setButtonText(LOC("network.buttons.reloadConfig"));
        reloadButton.setBaseColour(juce::Colour(0xFF338C33));  // Greenish
        reloadButton.onLongPress = [this]() { reloadNetworkConfiguration(); };

        addAndMakeVisible(reloadBackupButton);
        reloadBackupButton.setButtonText(LOC("network.buttons.reloadBackup"));
        reloadBackupButton.setBaseColour(juce::Colour(0xFF266626));  // Darker green
        reloadBackupButton.onLongPress = [this]() { reloadNetworkConfigBackup(); };

        addAndMakeVisible(importButton);
        importButton.setButtonText(LOC("network.buttons.import"));
        importButton.setBaseColour(juce::Colour(0xFF338C33));  // Greenish
        importButton.onLongPress = [this]() { importNetworkConfiguration(); };

        addAndMakeVisible(exportButton);
        exportButton.setButtonText(LOC("network.buttons.export"));
        exportButton.setBaseColour(juce::Colour(0xFF8C3333));  // Reddish
        exportButton.onLongPress = [this]() { exportNetworkConfiguration(); };

        // Setup numeric input filtering
        setupNumericEditors();

        // Add text editor listeners
        udpPortEditor.addListener(this);
        tcpPortEditor.addListener(this);

        // Populate network interfaces
        populateNetworkInterfaces();

        // Load initial values from parameters
        loadParametersFromValueTree();

        // Listen to parameter changes
        configTree = parameters.getConfigTree();
        configTree.addListener(this);

        // Update current IP address
        updateCurrentIP();

        // Initialize appearance (ADM-OSC greyed out, Tracking greyed out if disabled)
        updateAdmOscAppearance();
        updateTrackingAppearance();

        // Setup mouse listeners for status bar help text
        setupMouseListeners();
    }

    ~NetworkTab() override
    {
        ColorScheme::Manager::getInstance().removeListener(this);
        configTree.removeListener(this);
    }

    /** ColorScheme::Manager::Listener callback - refresh colors when theme changes */
    void colorSchemeChanged() override
    {
        // Update TextEditor colors - JUCE TextEditors cache colors internally
        const auto& colors = ColorScheme::get();
        auto updateTextEditor = [&colors](juce::TextEditor& editor) {
            editor.setColour(juce::TextEditor::textColourId, colors.textPrimary);
            editor.setColour(juce::TextEditor::backgroundColourId, colors.surfaceCard);
            editor.setColour(juce::TextEditor::outlineColourId, colors.buttonBorder);
            editor.applyFontToAllText(editor.getFont(), true);
        };

        updateTextEditor(currentIPEditor);
        updateTextEditor(udpPortEditor);
        updateTextEditor(tcpPortEditor);
        updateTextEditor(oscQueryPortEditor);
        updateTextEditor(trackingPortEditor);
        updateTextEditor(trackingOffsetXEditor);
        updateTextEditor(trackingOffsetYEditor);
        updateTextEditor(trackingOffsetZEditor);
        updateTextEditor(trackingScaleXEditor);
        updateTextEditor(trackingScaleYEditor);
        updateTextEditor(trackingScaleZEditor);
        updateTextEditor(trackingOscPathEditor);

        // Update network connection rows
        for (int i = 0; i < maxTargets; ++i)
        {
            updateTextEditor(targetRows[i].nameEditor);
            updateTextEditor(targetRows[i].ipEditor);
            updateTextEditor(targetRows[i].txPortEditor);
        }

        repaint();
    }

    void setStatusBar(StatusBar* bar)
    {
        statusBar = bar;
    }

    void setNetworkLogWindowCallback(NetworkLogWindowCallback callback)
    {
        onNetworkLogWindowRequested = std::move(callback);
    }

    /** Refresh UI from ValueTree - call after config reload */
    void refreshFromValueTree() { loadParametersFromValueTree(); }

    /** Programmatically toggle OSC source filter (for Stream Deck sync). */
    void toggleOscFilter()
    {
        bool newState = ! oscSourceFilterButton.getToggleState();
        oscSourceFilterButton.setToggleState (newState, juce::dontSendNotification);
        if (oscSourceFilterButton.onClick)
            oscSourceFilterButton.onClick();
    }

    /** Programmatically toggle tracking enabled (for Stream Deck sync). */
    void toggleTracking()
    {
        bool newState = ! trackingEnabledButton.getToggleState();
        trackingEnabledButton.setToggleState (newState, juce::dontSendNotification);
        if (trackingEnabledButton.onClick)
            trackingEnabledButton.onClick();
    }

    void setOSCManager(WFSNetwork::OSCManager* manager)
    {
        oscManager = manager;

        // Register callback for connection status changes
        if (oscManager != nullptr)
        {
            oscManager->onConnectionStatusChanged = [this](int targetIndex, WFSNetwork::ConnectionStatus status)
            {
                // Must update UI on message thread
                juce::MessageManager::callAsync([this, targetIndex, status]()
                {
                    updateTargetConnectionStatus(targetIndex, status);
                });
            };
        }

        updateOSCManagerConfig();
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(ColorScheme::get().background);

        // Footer background (matching Input/Output tabs)
        const int footerH = scaled(30) + 2 * scaled(10);
        g.setColour(ColorScheme::get().chromeSurface);
        g.fillRect(0, getHeight() - footerH, getWidth(), footerH);

        // Footer divider line
        g.setColour(ColorScheme::get().chromeDivider);
        g.drawLine(0.0f, (float)(getHeight() - footerH), (float)getWidth(), (float)(getHeight() - footerH), 1.0f);

        // Main column divider
        g.setColour(ColorScheme::get().chromeDivider);
        float topY = (float)scaled(10);
        float botY = (float)(getHeight() - footerH);
        g.drawVerticalLine(mainColumnDividerX, topY, botY);

        // Tracking column dividers
        g.drawLine((float)trackingDivX1, (float)trackingSectionY, (float)trackingDivX1, (float)trackingSectionBottom, 1.0f);
        g.drawLine((float)trackingDivX2, (float)trackingSectionY, (float)trackingDivX2, (float)trackingSectionBottom, 1.0f);

        // Draw section headers
        const int headerH = scaled(20);
        const int headerOffset = scaled(25);
        g.setColour(ColorScheme::get().textPrimary);
        g.setFont(juce::FontOptions().withHeight(juce::jmax(10.0f, 14.0f * layoutScale)).withStyle("Bold"));
        g.drawText(LOC("network.sections.network"), scaled(20), scaled(10), scaled(200), headerH, juce::Justification::left);
        g.drawText(LOC("network.sections.connections"), scaled(20), networkConnectionsSectionY - headerOffset, scaled(200), headerH, juce::Justification::left);
        // Tracking and ADM-OSC headers are in the right column (Tracking first)
        g.drawText(LOC("network.sections.tracking"), rightColumnX, trackingSectionY - headerOffset, scaled(200), headerH, juce::Justification::left);
        g.drawText(LOC("network.sections.admOsc"), rightColumnX, admOscSectionY - headerOffset, scaled(200), headerH, juce::Justification::left);

    }

    void resized() override
    {
        layoutScale = static_cast<float>(getHeight()) / 932.0f;
        const int rowHeight = scaled(30);
        const int spacing = scaled(5);
        const int sectionSpacing = scaled(40);
        const int margin = scaled(20);
        const int columnGap = scaled(30);
        const int footerHeight = scaled(30) + 2 * scaled(10);

        // Calculate available width and split into two columns
        const int totalWidth = getWidth() - margin * 2;
        const int leftColumnWidth = (totalWidth - columnGap) / 2;
        const int rightColumnWidth = totalWidth - leftColumnWidth - columnGap;

        const int leftX = margin;
        rightColumnX = leftX + leftColumnWidth + columnGap;
        mainColumnDividerX = leftX + leftColumnWidth + columnGap / 2;

        // ==================== LEFT COLUMN ====================
        int leftY = scaled(35);

        // --- Network Section ---
        // Calculate proportional widths for left column
        const int leftLabelWidth = juce::jmin(scaled(120), leftColumnWidth / 5);

        networkInterfaceLabel.setBounds(leftX, leftY, leftLabelWidth, rowHeight);
        networkInterfaceSelector.setBounds(leftX + leftLabelWidth, leftY, (leftColumnWidth - leftLabelWidth) * 2 / 5, rowHeight);
        leftY += rowHeight + spacing;

        currentIPLabel.setBounds(leftX, leftY, leftLabelWidth, rowHeight);
        currentIPEditor.setBounds(leftX + leftLabelWidth, leftY, scaled(150), rowHeight);
        leftY += rowHeight + spacing;

        // UDP, TCP, OSC Query on same row - distribute across column width
        const int portGroupWidth = (leftColumnWidth - scaled(40)) / 3;
        const int portLabelW = scaled(70);
        const int portEditorW = scaled(60);
        udpPortLabel.setBounds(leftX, leftY, portLabelW, rowHeight);
        udpPortEditor.setBounds(leftX + portLabelW, leftY, portEditorW, rowHeight);

        tcpPortLabel.setBounds(leftX + portGroupWidth, leftY, portLabelW, rowHeight);
        tcpPortEditor.setBounds(leftX + portGroupWidth + portLabelW, leftY, portEditorW, rowHeight);

        oscQueryLabel.setBounds(leftX + portGroupWidth * 2, leftY, portLabelW, rowHeight);
        oscQueryPortEditor.setBounds(leftX + portGroupWidth * 2 + portLabelW, leftY, scaled(50), rowHeight);
        oscQueryEnableButton.setBounds(leftX + portGroupWidth * 2 + portLabelW + scaled(55), leftY, portLabelW, rowHeight);
        leftY += rowHeight + sectionSpacing;

        // --- Network Connections Table ---
        networkConnectionsSectionY = leftY;

        const int tableSpacing = scaled(5);
        const int numTableCols = 8;
        const int totalTableSpacing = (numTableCols - 1) * tableSpacing;
        const int tableAvailableWidth = leftColumnWidth - totalTableSpacing;

        // Distribute width proportionally across columns
        // Weights: Name=2.5, Protocol=2, Mode=1.2, IP=2.5, Port=1.2, Rx=0.8, Tx=0.8, Remove=0.8
        const float totalWeight = 2.5f + 1.2f + 2.5f + 1.2f + 0.8f + 0.8f + 2.0f + 0.8f;

        const int nameColWidth = (int)(tableAvailableWidth * 2.5f / totalWeight);
        cachedNameColWidth = nameColWidth;
        const int modeColWidth = (int)(tableAvailableWidth * 1.2f / totalWeight);
        const int ipColWidth = (int)(tableAvailableWidth * 2.5f / totalWeight);
        const int portColWidth = (int)(tableAvailableWidth * 1.2f / totalWeight);
        const int rxTxColWidth = (int)(tableAvailableWidth * 0.8f / totalWeight);
        const int protocolColWidth = (int)(tableAvailableWidth * 2.0f / totalWeight);
        const int removeColWidth = (int)(tableAvailableWidth * 0.8f / totalWeight);

        // Header row
        int colX = leftX;
        headerNameLabel.setBounds(colX, leftY, nameColWidth, rowHeight);
        colX += nameColWidth + tableSpacing;
        headerProtocolLabel.setBounds(colX, leftY, protocolColWidth, rowHeight);
        colX += protocolColWidth + tableSpacing;
        headerDataModeLabel.setBounds(colX, leftY, modeColWidth, rowHeight);
        colX += modeColWidth + tableSpacing;
        headerIpLabel.setBounds(colX, leftY, ipColWidth, rowHeight);
        colX += ipColWidth + tableSpacing;
        headerTxPortLabel.setBounds(colX, leftY, portColWidth, rowHeight);
        colX += portColWidth + tableSpacing;
        headerRxEnableLabel.setBounds(colX, leftY, rxTxColWidth, rowHeight);
        colX += rxTxColWidth + tableSpacing;
        headerTxEnableLabel.setBounds(colX, leftY, rxTxColWidth, rowHeight);
        colX += rxTxColWidth + tableSpacing;
        addTargetButton.setBounds(colX, leftY, removeColWidth, rowHeight);
        leftY += rowHeight + spacing;

        // Target rows
        for (int i = 0; i < maxTargets; ++i)
        {
            auto& row = targetRows[i];
            colX = leftX;

            row.nameEditor.setBounds(colX, leftY, nameColWidth, rowHeight);
            colX += nameColWidth + tableSpacing;
            row.protocolSelector.setBounds(colX, leftY, protocolColWidth, rowHeight);
            colX += protocolColWidth + tableSpacing;
            row.dataModeSelector.setBounds(colX, leftY, modeColWidth, rowHeight);
            colX += modeColWidth + tableSpacing;
            row.ipEditor.setBounds(colX, leftY, ipColWidth, rowHeight);
            colX += ipColWidth + tableSpacing;
            row.txPortEditor.setBounds(colX, leftY, portColWidth, rowHeight);
            colX += portColWidth + tableSpacing;
            row.rxEnableButton.setBounds(colX, leftY, rxTxColWidth, rowHeight);
            row.txEnableButton.setBounds(colX + rxTxColWidth + tableSpacing, leftY, rxTxColWidth, rowHeight);
            // QLab patch fields in right portion of name column (positioned here, visibility via updateQLabAppearance)
            {
                auto nb = row.nameEditor.getBounds();
                int patchLabelW = scaled(32);
                int patchEditorW = scaled(28);
                int patchGap = scaled(3);
                int patchX = nb.getRight() - patchLabelW - patchGap - patchEditorW;
                row.qlabPatchLabel.setBounds(patchX, nb.getY(), patchLabelW, nb.getHeight());
                row.qlabPatchEditor.setBounds(patchX + patchLabelW + patchGap, nb.getY(), patchEditorW, nb.getHeight());
            }
            colX += rxTxColWidth + tableSpacing + rxTxColWidth + tableSpacing;
            row.removeButton.setBounds(colX, leftY, removeColWidth, rowHeight);

            leftY += rowHeight + spacing;
        }

        // Buttons beneath table - aligned with table, footer-matching gap
        leftY += scaled(35);  // Padding after table
        const int tableButtonGap = spacing;  // match footer button spacing
        const int tableButtonWidth = (leftColumnWidth - tableButtonGap * 2) / 3;
        oscSourceFilterButton.setBounds(leftX, leftY, tableButtonWidth, rowHeight);
        openLogWindowButton.setBounds(leftX + tableButtonWidth + tableButtonGap, leftY, tableButtonWidth, rowHeight);
        findMyRemoteButton.setBounds(leftX + tableButtonWidth * 2 + tableButtonGap * 2, leftY, tableButtonWidth, rowHeight);
        leftY += rowHeight + spacing;

        // ADM-OSC help card — bottom of left column, below connection buttons
        {
            int cardW = leftColumnWidth;
            int cardH = admOscHelpCard.getIdealHeight(cardW);
            int cardY = getHeight() - footerHeight - scaled(10) - cardH;
            admOscHelpCard.setBounds(leftX, cardY, cardW, cardH);
        }

        // ==================== RIGHT COLUMN: TRACKING & ADM-OSC ====================
        int rightY = scaled(35);

        // Calculate widths for right column (3 sub-columns for X, Y, Z)
        const int rightSubColSpacing = scaled(15);
        const int rightSubColWidth = (rightColumnWidth - rightSubColSpacing * 2) / 3;
        const int rightLabelWidth = scaled(75);
        const int rightUnitWidth = scaled(20);
        const int rightEditorWidth = rightSubColWidth - rightLabelWidth - rightUnitWidth;
        const int rightRowSpacing = scaled(10);

        int rcol1 = rightColumnX;
        int rcol2 = rcol1 + rightSubColWidth + rightSubColSpacing;
        int rcol3 = rcol2 + rightSubColWidth + rightSubColSpacing;
        trackingDivX1 = rcol2 - rightSubColSpacing / 2;
        trackingDivX2 = rcol3 - rightSubColSpacing / 2;

        // --- Tracking Section (now first in right column) ---
        trackingSectionY = rightY;

        // Row 1: Enable, Protocol, Port — aligned with offset/scale editor boxes
        const int btnInset = 6;  // matches WfsLookAndFeel drawButtonBackground/drawComboBox inset
        trackingEnabledButton.setBounds(rcol1, rightY, rightSubColWidth, rowHeight);
        trackingProtocolLabel.setBounds(rcol2, rightY, rightLabelWidth, rowHeight);
        trackingProtocolSelector.setBounds(rcol2 + rightLabelWidth - btnInset, rightY, rightEditorWidth + btnInset * 2, rowHeight);
        trackingPortLabel.setBounds(rcol3, rightY, rightLabelWidth, rowHeight);
        trackingPortEditor.setBounds(rcol3 + rightLabelWidth, rightY, rightEditorWidth, rowHeight);
        rightY += rowHeight + rightRowSpacing;

        // Row 2: Offset X, Y, Z
        trackingOffsetXLabel.setBounds(rcol1, rightY, rightLabelWidth, rowHeight);
        trackingOffsetXEditor.setBounds(rcol1 + rightLabelWidth, rightY, rightEditorWidth, rowHeight);
        trackingOffsetXUnitLabel.setBounds(rcol1 + rightLabelWidth + rightEditorWidth, rightY, rightUnitWidth, rowHeight);

        trackingOffsetYLabel.setBounds(rcol2, rightY, rightLabelWidth, rowHeight);
        trackingOffsetYEditor.setBounds(rcol2 + rightLabelWidth, rightY, rightEditorWidth, rowHeight);
        trackingOffsetYUnitLabel.setBounds(rcol2 + rightLabelWidth + rightEditorWidth, rightY, rightUnitWidth, rowHeight);

        trackingOffsetZLabel.setBounds(rcol3, rightY, rightLabelWidth, rowHeight);
        trackingOffsetZEditor.setBounds(rcol3 + rightLabelWidth, rightY, rightEditorWidth, rowHeight);
        trackingOffsetZUnitLabel.setBounds(rcol3 + rightLabelWidth + rightEditorWidth, rightY, rightUnitWidth, rowHeight);
        rightY += rowHeight + rightRowSpacing;

        // Row 3: Scale X, Y, Z
        trackingScaleXLabel.setBounds(rcol1, rightY, rightLabelWidth, rowHeight);
        trackingScaleXEditor.setBounds(rcol1 + rightLabelWidth, rightY, rightEditorWidth, rowHeight);
        trackingScaleXUnitLabel.setBounds(rcol1 + rightLabelWidth + rightEditorWidth, rightY, rightUnitWidth, rowHeight);

        trackingScaleYLabel.setBounds(rcol2, rightY, rightLabelWidth, rowHeight);
        trackingScaleYEditor.setBounds(rcol2 + rightLabelWidth, rightY, rightEditorWidth, rowHeight);
        trackingScaleYUnitLabel.setBounds(rcol2 + rightLabelWidth + rightEditorWidth, rightY, rightUnitWidth, rowHeight);

        trackingScaleZLabel.setBounds(rcol3, rightY, rightLabelWidth, rowHeight);
        trackingScaleZEditor.setBounds(rcol3 + rightLabelWidth, rightY, rightEditorWidth, rowHeight);
        trackingScaleZUnitLabel.setBounds(rcol3 + rightLabelWidth + rightEditorWidth, rightY, rightUnitWidth, rowHeight);
        rightY += rowHeight + rightRowSpacing;

        // Row 4: Flip X, Y, Z — aligned with scale editor boxes (compensate for button inset)
        trackingFlipXButton.setBounds(rcol1 + rightLabelWidth - btnInset, rightY, rightEditorWidth + btnInset * 2, rowHeight);
        trackingFlipYButton.setBounds(rcol2 + rightLabelWidth - btnInset, rightY, rightEditorWidth + btnInset * 2, rowHeight);
        trackingFlipZButton.setBounds(rcol3 + rightLabelWidth - btnInset, rightY, rightEditorWidth + btnInset * 2, rowHeight);
        trackingSectionBottom = rightY + rowHeight;
        rightY += rowHeight + rightRowSpacing;

        // Row 5: OSC Path / PSN Interface / MQTT Host+Topic
        const int oscPathLabelWidth = scaled(75);
        trackingOscPathLabel.setBounds(rcol1, rightY, oscPathLabelWidth, rowHeight);
        trackingOscPathEditor.setBounds(rcol1 + oscPathLabelWidth, rightY, rightColumnWidth - oscPathLabelWidth, rowHeight);
        const int psnInterfaceLabelWidth = scaled(95);
        trackingPsnInterfaceLabel.setBounds(rcol1, rightY, psnInterfaceLabelWidth, rowHeight);
        trackingPsnInterfaceSelector.setBounds(rcol1 + psnInterfaceLabelWidth, rightY, rightColumnWidth - psnInterfaceLabelWidth, rowHeight);

        // MQTT row 1: Host
        const int mqttLabelW = scaled(55);
        trackingMqttHostLabel.setBounds(rcol1, rightY, mqttLabelW, rowHeight);
        trackingMqttHostEditor.setBounds(rcol1 + mqttLabelW, rightY, rightColumnWidth - mqttLabelW, rowHeight);
        rightY += rowHeight + rightRowSpacing;

        // MQTT row 2: Topic + Tag IDs button
        const int tagBtnW = scaled(65);
        trackingMqttTopicLabel.setBounds(rcol1, rightY, mqttLabelW, rowHeight);
        trackingMqttTopicEditor.setBounds(rcol1 + mqttLabelW, rightY, rightColumnWidth - mqttLabelW - tagBtnW - scaled(4), rowHeight);
        trackingMqttTagIdsButton.setBounds(rcol1 + rightColumnWidth - tagBtnW, rightY, tagBtnW, rowHeight);
        rightY += rowHeight + rightRowSpacing;

        // MQTT row 3: JSON field names (4 compact fields)
        {
            const int jLabelW = scaled(22);
            const int jFieldW = scaled(52);
            const int jSpacing = scaled(4);
            int jx = rcol1;
            auto layoutJsonField = [&](juce::Label& lbl, juce::ComboBox& ed) {
                lbl.setBounds(jx, rightY, jLabelW, rowHeight);
                ed.setBounds(jx + jLabelW, rightY, jFieldW, rowHeight);
                jx += jLabelW + jFieldW + jSpacing;
            };
            layoutJsonField(trackingMqttJsonXLabel, trackingMqttJsonXCombo);
            layoutJsonField(trackingMqttJsonYLabel, trackingMqttJsonYCombo);
            layoutJsonField(trackingMqttJsonZLabel, trackingMqttJsonZCombo);
            layoutJsonField(trackingMqttJsonQLabel, trackingMqttJsonQCombo);
        }
        rightY += rowHeight + sectionSpacing;

        // --- ADM-OSC Section (now second in right column) ---
        admOscSectionY = rightY;

        // ADM-OSC help button — right end of ADM-OSC header line
        {
            const int btnSize = scaled(20);
            const int hdrOffset = scaled(25);
            admOscHelpButton.setBounds(rightColumnX + rightColumnWidth - btnSize,
                                        admOscSectionY - hdrOffset, btnSize, btnSize);
        }

        // Row 1: Mapping selector
        const int mappingLabelW = scaled(75);
        const int mappingSelectorW = scaled(120);
        admMappingSelectorLabel.setBounds(rcol1, rightY, mappingLabelW, rowHeight);
        admMappingSelector.setBounds(rcol1 + mappingLabelW, rightY, mappingSelectorW, rowHeight);
        rightY += rowHeight + rightRowSpacing;

        // --- Cartesian or Polar mapping layout (only layout the visible one) ---
        if (currentAdmMapping < 4)
        {
            // Mapping panel (3 axis graphs + XY rectangle, all controls integrated)
            admMappingGraph.setBounds(rcol1, rightY, rightColumnWidth, scaled(400));
            rightY += scaled(400) + rightRowSpacing;
        }
        else
        {
            // Polar mapping panel (distance graph + circle overview, all controls integrated)
            admPolarGraph.setBounds(rcol1, rightY, rightColumnWidth, scaled(400));
            rightY += scaled(400) + rightRowSpacing;
        }

        // ==================== FOOTER BUTTONS ====================
        const int footerPadding = scaled(10);
        auto footerArea = getLocalBounds().removeFromBottom(footerHeight).reduced(footerPadding, footerPadding);
        const int buttonWidth = (footerArea.getWidth() - spacing * 4) / 5;

        storeButton.setBounds(footerArea.removeFromLeft(buttonWidth));
        footerArea.removeFromLeft(spacing);
        reloadButton.setBounds(footerArea.removeFromLeft(buttonWidth));
        footerArea.removeFromLeft(spacing);
        reloadBackupButton.setBounds(footerArea.removeFromLeft(buttonWidth));
        footerArea.removeFromLeft(spacing);
        importButton.setBounds(footerArea.removeFromLeft(buttonWidth));
        footerArea.removeFromLeft(spacing);
        exportButton.setBounds(footerArea.removeFromLeft(buttonWidth));

        WfsLookAndFeel::scaleTextEditorFonts(*this, layoutScale);
    }

    //==========================================================================
    // Custom keyboard focus traversal — 2 column circuits
    //==========================================================================

    std::unique_ptr<juce::ComponentTraverser> createKeyboardFocusTraverser() override
    {
        std::vector<juce::Component*> leftCol = {
            &currentIPEditor, &udpPortEditor, &tcpPortEditor, &oscQueryPortEditor
        };
        // Add connection table rows (row-major: name → qlabPatch → ip → txPort per row)
        for (int i = 0; i < maxTargets; ++i)
        {
            leftCol.push_back(&targetRows[i].nameEditor);
            leftCol.push_back(&targetRows[i].qlabPatchEditor);
            leftCol.push_back(&targetRows[i].ipEditor);
            leftCol.push_back(&targetRows[i].txPortEditor);
        }

        std::vector<juce::Component*> rightCol = {
            &trackingPortEditor,
            &trackingOffsetXEditor, &trackingOffsetYEditor, &trackingOffsetZEditor,
            &trackingScaleXEditor, &trackingScaleYEditor, &trackingScaleZEditor,
            &trackingOscPathEditor
        };

        return std::make_unique<ColumnCircuitTraverser>(std::vector<std::vector<juce::Component*>>{
            std::move(leftCol), std::move(rightCol)
        });
    }

private:
    WfsParameters& parameters;
    juce::ValueTree configTree;  // Store for safe listener removal in destructor
    StatusBar* statusBar = nullptr;
    WFSNetwork::OSCManager* oscManager = nullptr;
    NetworkLogWindowCallback onNetworkLogWindowRequested;

    // ==================== NETWORK CONNECTIONS TABLE ====================
    static constexpr int maxTargets = 6;

    // Structure for each target row
    struct NetworkTargetRow
    {
        juce::TextEditor nameEditor;
        juce::ComboBox dataModeSelector;       // UDP / TCP
        juce::TextEditor ipEditor;
        juce::TextEditor txPortEditor;
        juce::TextButton rxEnableButton;
        juce::TextButton txEnableButton;
        juce::ComboBox protocolSelector;       // DISABLED / OSC / REMOTE / ADM-OSC / QLab
        LongPressButton removeButton { 800 };
        juce::Label qlabPatchLabel;            // "Patch" label for QLab targets
        juce::TextEditor qlabPatchEditor;      // QLab patch number editor

        bool isActive = false;  // Whether this row has data
    };

    // Header labels for the table
    juce::Label headerNameLabel;
    juce::Label headerDataModeLabel;
    juce::Label headerIpLabel;
    juce::Label headerTxPortLabel;
    juce::Label headerRxEnableLabel;
    juce::Label headerTxEnableLabel;
    juce::Label headerProtocolLabel;
    juce::TextButton addTargetButton;

    // 6 target rows
    NetworkTargetRow targetRows[maxTargets];
    int activeTargetCount = 0;

    // Buttons beneath the table
    juce::TextButton openLogWindowButton;
    juce::TextButton findMyRemoteButton;
    juce::TextButton oscSourceFilterButton;

    // Find My Remote password (session-only, not saved)
    juce::String findDevicePassword;

    // Section Y position
    int networkConnectionsSectionY = 0;
    int cachedNameColWidth = 0;  // Stored from resized() for updateQLabAppearance()

    // Network Interface Section
    juce::Label networkInterfaceLabel;
    juce::ComboBox networkInterfaceSelector;
    juce::StringArray interfaceNames;
    juce::StringArray interfaceIPs;

    // Network Section
    juce::Label currentIPLabel;
    juce::TextEditor currentIPEditor;
    juce::Label udpPortLabel;
    juce::TextEditor udpPortEditor;
    juce::Label tcpPortLabel;
    juce::TextEditor tcpPortEditor;

    // OSC Query
    juce::Label oscQueryLabel;
    juce::TextEditor oscQueryPortEditor;
    juce::TextButton oscQueryEnableButton;

    // Footer buttons
    LongPressButton storeButton;
    LongPressButton reloadButton;
    LongPressButton reloadBackupButton;
    LongPressButton importButton;
    LongPressButton exportButton;

    // Section Y positions for painting
    int admOscSectionY = 0;
    int trackingSectionY = 0;
    int trackingSectionBottom = 0;
    int trackingDivX1 = 0;  // divider between sub-col 1 and 2
    int trackingDivX2 = 0;  // divider between sub-col 2 and 3
    int rightColumnX = 0;  // X position for right column (ADM-OSC & Tracking)
    int mainColumnDividerX = 0;
    float layoutScale = 1.0f;

    /** Scale a reference pixel value by layoutScale with a 65% minimum floor */
    int scaled(int ref) const { return juce::jmax(static_cast<int>(ref * 0.65f), static_cast<int>(ref * layoutScale)); }

    // ==================== ADM-OSC MAPPING SECTION ====================
    juce::ComboBox admMappingSelector;
    juce::Label admMappingSelectorLabel;
    int currentAdmMapping = 0;  // Currently editing: 0-3 = Cart, 4-7 = Polar

    HelpCardButton admOscHelpButton;
    HelpCard admOscHelpCard;

    // Cartesian mapping visualization panel (all controls integrated)
    AdmMappingPanel admMappingGraph;

    // Polar mapping visualization panel (all controls integrated)
    AdmPolarPanel admPolarGraph;

    // Tracking Section
    juce::TextButton trackingEnabledButton;
    juce::Label trackingProtocolLabel;
    juce::ComboBox trackingProtocolSelector;
    juce::Label trackingPortLabel;
    juce::TextEditor trackingPortEditor;

    juce::Label trackingOffsetXLabel;
    juce::TextEditor trackingOffsetXEditor;
    juce::Label trackingOffsetXUnitLabel;
    juce::Label trackingOffsetYLabel;
    juce::TextEditor trackingOffsetYEditor;
    juce::Label trackingOffsetYUnitLabel;
    juce::Label trackingOffsetZLabel;
    juce::TextEditor trackingOffsetZEditor;
    juce::Label trackingOffsetZUnitLabel;

    juce::Label trackingScaleXLabel;
    juce::TextEditor trackingScaleXEditor;
    juce::Label trackingScaleXUnitLabel;
    juce::Label trackingScaleYLabel;
    juce::TextEditor trackingScaleYEditor;
    juce::Label trackingScaleYUnitLabel;
    juce::Label trackingScaleZLabel;
    juce::TextEditor trackingScaleZEditor;
    juce::Label trackingScaleZUnitLabel;

    juce::TextButton trackingFlipXButton;
    juce::TextButton trackingFlipYButton;
    juce::TextButton trackingFlipZButton;

    // OSC Path for tracking (shown when protocol is OSC)
    juce::Label trackingOscPathLabel;
    juce::TextEditor trackingOscPathEditor;

    // PSN Interface selector (shown when protocol is PSN)
    juce::Label trackingPsnInterfaceLabel;
    juce::ComboBox trackingPsnInterfaceSelector;
    juce::StringArray psnInterfaceNames;
    juce::StringArray psnInterfaceIPs;

    // MQTT controls (shown when protocol is MQTT)
    juce::Label trackingMqttHostLabel;
    juce::TextEditor trackingMqttHostEditor;
    juce::Label trackingMqttTopicLabel;
    juce::TextEditor trackingMqttTopicEditor;
    juce::TextButton trackingMqttTagIdsButton;
    juce::Label trackingMqttJsonXLabel;
    juce::ComboBox trackingMqttJsonXCombo;
    juce::Label trackingMqttJsonYLabel;
    juce::ComboBox trackingMqttJsonYCombo;
    juce::Label trackingMqttJsonZLabel;
    juce::ComboBox trackingMqttJsonZCombo;
    juce::Label trackingMqttJsonQLabel;
    juce::ComboBox trackingMqttJsonQCombo;

    void setupAdmOscSection()
    {
        // --- Mapping selector ---
        addAndMakeVisible(admMappingSelectorLabel);
        admMappingSelectorLabel.setText(LOC("network.labels.admMapping"), juce::dontSendNotification);
        addAndMakeVisible(admMappingSelector);
        admMappingSelector.addItem("Cartesian 1", 1);
        admMappingSelector.addItem("Cartesian 2", 2);
        admMappingSelector.addItem("Cartesian 3", 3);
        admMappingSelector.addItem("Cartesian 4", 4);
        admMappingSelector.addItem("Polar 1", 5);
        admMappingSelector.addItem("Polar 2", 6);
        admMappingSelector.addItem("Polar 3", 7);
        admMappingSelector.addItem("Polar 4", 8);
        admMappingSelector.setSelectedId(1, juce::dontSendNotification);
        admMappingSelector.onChange = [this]() {
            currentAdmMapping = admMappingSelector.getSelectedId() - 1;
            loadAdmMappingToUI();
            updateAdmMappingVisibility();
        };

        // --- Mapping panel (all Cartesian controls integrated) ---
        addAndMakeVisible(admMappingGraph);
        admMappingGraph.onParameterDragged = [this](int axis, int pointIndex,
            float newBp, float newInner, float newOuter, float newCenter, bool /*isPos*/)
        {
            auto um = getConfigUndoManager();
            auto mappingNode = findAdmMappingNode(currentAdmMapping);
            if (!mappingNode.isValid()) return;
            auto axisNode = findAxisNode(mappingNode, axis);
            if (!axisNode.isValid()) return;

            switch (pointIndex)
            {
                case 0: axisNode.setProperty(WFSParameterIDs::admCartNegOuterWidth, newOuter, um); break;
                case 1:
                    axisNode.setProperty(WFSParameterIDs::admCartBreakpoint, newBp, um);
                    axisNode.setProperty(WFSParameterIDs::admCartNegInnerWidth, newInner, um);
                    break;
                case 2: axisNode.setProperty(WFSParameterIDs::admCartCenterOffset, newCenter, um); break;
                case 3:
                    axisNode.setProperty(WFSParameterIDs::admCartBreakpoint, newBp, um);
                    axisNode.setProperty(WFSParameterIDs::admCartPosInnerWidth, newInner, um);
                    break;
                case 4: axisNode.setProperty(WFSParameterIDs::admCartPosOuterWidth, newOuter, um); break;
            }
        };

        admMappingGraph.onSwapChanged = [this](int axis, int newSwapValue)
        {
            auto um = getConfigUndoManager();
            auto mappingNode = findAdmMappingNode(currentAdmMapping);
            if (!mappingNode.isValid()) return;
            auto axisNode = findAxisNode(mappingNode, axis);
            if (!axisNode.isValid()) return;
            axisNode.setProperty(WFSParameterIDs::admCartAxisSwap, newSwapValue, um);
        };

        admMappingGraph.onFlipChanged = [this](int axis, bool newFlipState)
        {
            auto um = getConfigUndoManager();
            auto mappingNode = findAdmMappingNode(currentAdmMapping);
            if (!mappingNode.isValid()) return;
            auto axisNode = findAxisNode(mappingNode, axis);
            if (!axisNode.isValid()) return;
            axisNode.setProperty(WFSParameterIDs::admCartSignFlip, newFlipState ? 1 : 0, um);
        };

        admMappingGraph.onResetLongPress = [this]()
        {
            auto um = getConfigUndoManager();
            um->beginNewTransaction("Reset Cartesian Mapping");
            auto mappingNode = findAdmMappingNode(currentAdmMapping);
            if (!mappingNode.isValid()) return;
            for (int a = 0; a < 3; ++a)
            {
                auto axisNode = findAxisNode(mappingNode, a);
                if (!axisNode.isValid()) continue;
                axisNode.setProperty(WFSParameterIDs::admCartAxisSwap, a, um);
                axisNode.setProperty(WFSParameterIDs::admCartSignFlip, 0, um);
                axisNode.setProperty(WFSParameterIDs::admCartCenterOffset, 0.0f, um);
                axisNode.setProperty(WFSParameterIDs::admCartBreakpoint, WFSParameterDefaults::admCartBreakpointDefault, um);
                axisNode.setProperty(WFSParameterIDs::admCartPosInnerWidth, WFSParameterDefaults::admCartWidthDefault, um);
                axisNode.setProperty(WFSParameterIDs::admCartPosOuterWidth, WFSParameterDefaults::admCartWidthDefault, um);
                axisNode.setProperty(WFSParameterIDs::admCartNegInnerWidth, WFSParameterDefaults::admCartWidthDefault, um);
                axisNode.setProperty(WFSParameterIDs::admCartNegOuterWidth, WFSParameterDefaults::admCartWidthDefault, um);
            }
            loadAdmMappingToUI();
        };

        // --- Polar mapping panel ---
        addAndMakeVisible(admPolarGraph);
        admPolarGraph.onDistDragged = [this](int pointIndex, float newBp, float newInner,
                                              float newOuter, float newCenter)
        {
            auto um = getConfigUndoManager();
            auto mappingNode = findAdmMappingNode(currentAdmMapping);
            if (!mappingNode.isValid()) return;
            switch (pointIndex)
            {
                case 0:
                    mappingNode.setProperty(WFSParameterIDs::admPolarDistCenter, newCenter, um);
                    break;
                case 1:
                    mappingNode.setProperty(WFSParameterIDs::admPolarDistBreakpoint, newBp, um);
                    mappingNode.setProperty(WFSParameterIDs::admPolarDistInner, newInner, um);
                    break;
                case 2:
                    mappingNode.setProperty(WFSParameterIDs::admPolarDistOuter, newOuter, um);
                    break;
            }
        };
        admPolarGraph.onAzOffsetChanged = [this](float newAzOffset)
        {
            auto um = getConfigUndoManager();
            auto mappingNode = findAdmMappingNode(currentAdmMapping);
            if (!mappingNode.isValid()) return;
            mappingNode.setProperty(WFSParameterIDs::admPolarAzimuthOffset, newAzOffset, um);
        };
        admPolarGraph.onAzFlipChanged = [this](bool azFlip)
        {
            auto um = getConfigUndoManager();
            auto mappingNode = findAdmMappingNode(currentAdmMapping);
            if (!mappingNode.isValid()) return;
            mappingNode.setProperty(WFSParameterIDs::admPolarAzimuthFlip, azFlip ? 1 : 0, um);
        };
        admPolarGraph.onElFlipChanged = [this](bool elFlip)
        {
            auto um = getConfigUndoManager();
            auto mappingNode = findAdmMappingNode(currentAdmMapping);
            if (!mappingNode.isValid()) return;
            mappingNode.setProperty(WFSParameterIDs::admPolarElevationFlip, elFlip ? 1 : 0, um);
        };
        admPolarGraph.onResetLongPress = [this]()
        {
            auto um = getConfigUndoManager();
            um->beginNewTransaction("Reset Polar Mapping");
            auto mappingNode = findAdmMappingNode(currentAdmMapping);
            if (!mappingNode.isValid()) return;
            mappingNode.setProperty(WFSParameterIDs::admPolarAzimuthOffset, WFSParameterDefaults::admPolarAzimuthOffsetDefault, um);
            mappingNode.setProperty(WFSParameterIDs::admPolarAzimuthFlip, WFSParameterDefaults::admPolarAzimuthFlipDefault, um);
            mappingNode.setProperty(WFSParameterIDs::admPolarElevationFlip, WFSParameterDefaults::admPolarElevationFlipDefault, um);
            mappingNode.setProperty(WFSParameterIDs::admPolarDistBreakpoint, WFSParameterDefaults::admPolarDistBreakpointDefault, um);
            mappingNode.setProperty(WFSParameterIDs::admPolarDistInner, WFSParameterDefaults::admPolarDistInnerDefault, um);
            mappingNode.setProperty(WFSParameterIDs::admPolarDistOuter, WFSParameterDefaults::admPolarDistOuterDefault, um);
            mappingNode.setProperty(WFSParameterIDs::admPolarDistCenter, WFSParameterDefaults::admPolarDistCenterDefault, um);
            loadAdmMappingToUI();
        };

        // Initial visibility
        updateAdmMappingVisibility();
    }

    void setupTrackingSection()
    {
        // Enable button
        addAndMakeVisible(trackingEnabledButton);
        trackingEnabledButton.setButtonText(LOC("network.toggles.trackingOff"));
        trackingEnabledButton.setClickingTogglesState(true);
        trackingEnabledButton.onClick = [this]() {
            bool enabling = trackingEnabledButton.getToggleState();
            if (enabling)
            {
                // Check for cluster conflicts before enabling
                checkGlobalTrackingConstraintAsync();
            }
            else
            {
                trackingEnabledButton.setButtonText(LOC("network.toggles.trackingOff"));
                parameters.setConfigParam("trackingEnabled", 0);
                updateTrackingAppearance();
            }
        };

        // Protocol selector
        addAndMakeVisible(trackingProtocolLabel);
        trackingProtocolLabel.setText(LOC("network.labels.protocol"), juce::dontSendNotification);
        addAndMakeVisible(trackingProtocolSelector);
        trackingProtocolSelector.addItem(LOC("network.protocols.disabled"), 1);
        trackingProtocolSelector.addItem(LOC("network.protocols.osc"), 2);
        trackingProtocolSelector.addItem(LOC("network.protocols.psn"), 3);
        trackingProtocolSelector.addItem(LOC("network.protocols.rttrp"), 4);
        trackingProtocolSelector.addItem(LOC("network.protocols.mqtt"), 5);
        trackingProtocolSelector.setSelectedId(1, juce::dontSendNotification);
        trackingProtocolSelector.onChange = [this]() {
            int newProtocol = trackingProtocolSelector.getSelectedId() - 1;
            int globalEnabled = static_cast<int>(parameters.getConfigParam("trackingEnabled"));

            // Auto-fill default port based on protocol selection
            // OSC (ID 2) = 5000, PSN (ID 3) = 56565, RTTrP (ID 4) = 24220
            int protocolId = trackingProtocolSelector.getSelectedId();
            if (protocolId == 2)  // OSC
            {
                trackingPortEditor.setText("5000", juce::dontSendNotification);
                parameters.setConfigParam("trackingPort", 5000);
            }
            else if (protocolId == 3)  // PSN
            {
                trackingPortEditor.setText("56565", juce::dontSendNotification);
                parameters.setConfigParam("trackingPort", 56565);
            }
            else if (protocolId == 4)  // RTTrP
            {
                trackingPortEditor.setText("24220", juce::dontSendNotification);
                parameters.setConfigParam("trackingPort", 24220);
            }
            else if (protocolId == 5)  // MQTT
            {
                trackingPortEditor.setText("1883", juce::dontSendNotification);
                parameters.setConfigParam("trackingPort", 1883);
            }

            // If enabling protocol while global tracking is on, check for conflicts
            if (newProtocol != 0 && globalEnabled != 0)
            {
                checkGlobalTrackingConstraintAsync(true);  // true = called from protocol change
            }
            else
            {
                parameters.setConfigParam("trackingProtocol", newProtocol);
            }
            // Update appearance (show/hide OSC path based on protocol)
            updateTrackingAppearance();
            // TTS: Announce selection change
            TTSManager::getInstance().announceValueChange(LOC("network.labels.protocol"), trackingProtocolSelector.getText());
        };

        // Port
        addAndMakeVisible(trackingPortLabel);
        trackingPortLabel.setText(LOC("network.labels.rxPort"), juce::dontSendNotification);
        addAndMakeVisible(trackingPortEditor);

        // Offset X
        addAndMakeVisible(trackingOffsetXLabel);
        trackingOffsetXLabel.setText(LOC("network.labels.offsetX"), juce::dontSendNotification);
        addAndMakeVisible(trackingOffsetXEditor);
        addAndMakeVisible(trackingOffsetXUnitLabel);
        trackingOffsetXUnitLabel.setText(LOC("units.meters"), juce::dontSendNotification);
        trackingOffsetXUnitLabel.setColour(juce::Label::textColourId, ColorScheme::get().textSecondary);

        // Offset Y
        addAndMakeVisible(trackingOffsetYLabel);
        trackingOffsetYLabel.setText(LOC("network.labels.offsetY"), juce::dontSendNotification);
        addAndMakeVisible(trackingOffsetYEditor);
        addAndMakeVisible(trackingOffsetYUnitLabel);
        trackingOffsetYUnitLabel.setText(LOC("units.meters"), juce::dontSendNotification);
        trackingOffsetYUnitLabel.setColour(juce::Label::textColourId, ColorScheme::get().textSecondary);

        // Offset Z
        addAndMakeVisible(trackingOffsetZLabel);
        trackingOffsetZLabel.setText(LOC("network.labels.offsetZ"), juce::dontSendNotification);
        addAndMakeVisible(trackingOffsetZEditor);
        addAndMakeVisible(trackingOffsetZUnitLabel);
        trackingOffsetZUnitLabel.setText(LOC("units.meters"), juce::dontSendNotification);
        trackingOffsetZUnitLabel.setColour(juce::Label::textColourId, ColorScheme::get().textSecondary);

        // Scale X
        addAndMakeVisible(trackingScaleXLabel);
        trackingScaleXLabel.setText(LOC("network.labels.scaleX"), juce::dontSendNotification);
        addAndMakeVisible(trackingScaleXEditor);
        addAndMakeVisible(trackingScaleXUnitLabel);
        trackingScaleXUnitLabel.setText("x", juce::dontSendNotification);
        trackingScaleXUnitLabel.setColour(juce::Label::textColourId, ColorScheme::get().textSecondary);

        // Scale Y
        addAndMakeVisible(trackingScaleYLabel);
        trackingScaleYLabel.setText(LOC("network.labels.scaleY"), juce::dontSendNotification);
        addAndMakeVisible(trackingScaleYEditor);
        addAndMakeVisible(trackingScaleYUnitLabel);
        trackingScaleYUnitLabel.setText("x", juce::dontSendNotification);
        trackingScaleYUnitLabel.setColour(juce::Label::textColourId, ColorScheme::get().textSecondary);

        // Scale Z
        addAndMakeVisible(trackingScaleZLabel);
        trackingScaleZLabel.setText(LOC("network.labels.scaleZ"), juce::dontSendNotification);
        addAndMakeVisible(trackingScaleZEditor);
        addAndMakeVisible(trackingScaleZUnitLabel);
        trackingScaleZUnitLabel.setText("x", juce::dontSendNotification);
        trackingScaleZUnitLabel.setColour(juce::Label::textColourId, ColorScheme::get().textSecondary);

        // Flip buttons
        addAndMakeVisible(trackingFlipXButton);
        trackingFlipXButton.setButtonText(LOC("network.toggles.flipXOff"));
        trackingFlipXButton.setClickingTogglesState(true);
        trackingFlipXButton.onClick = [this]() {
            trackingFlipXButton.setButtonText(trackingFlipXButton.getToggleState() ? LOC("network.toggles.flipXOn") : LOC("network.toggles.flipXOff"));
            parameters.setConfigParam("trackingFlipX", trackingFlipXButton.getToggleState() ? 1 : 0);
            updateTrackingTransformations();
        };

        addAndMakeVisible(trackingFlipYButton);
        trackingFlipYButton.setButtonText(LOC("network.toggles.flipYOff"));
        trackingFlipYButton.setClickingTogglesState(true);
        trackingFlipYButton.onClick = [this]() {
            trackingFlipYButton.setButtonText(trackingFlipYButton.getToggleState() ? LOC("network.toggles.flipYOn") : LOC("network.toggles.flipYOff"));
            parameters.setConfigParam("trackingFlipY", trackingFlipYButton.getToggleState() ? 1 : 0);
            updateTrackingTransformations();
        };

        addAndMakeVisible(trackingFlipZButton);
        trackingFlipZButton.setButtonText(LOC("network.toggles.flipZOff"));
        trackingFlipZButton.setClickingTogglesState(true);
        trackingFlipZButton.onClick = [this]() {
            trackingFlipZButton.setButtonText(trackingFlipZButton.getToggleState() ? LOC("network.toggles.flipZOn") : LOC("network.toggles.flipZOff"));
            parameters.setConfigParam("trackingFlipZ", trackingFlipZButton.getToggleState() ? 1 : 0);
            updateTrackingTransformations();
        };

        // OSC Path (shown when protocol is OSC)
        addAndMakeVisible(trackingOscPathLabel);
        trackingOscPathLabel.setText(LOC("network.labels.oscPath"), juce::dontSendNotification);
        addAndMakeVisible(trackingOscPathEditor);
        trackingOscPathEditor.setText("/wfs/tracking <ID> <x> <y> <z>", juce::dontSendNotification);
        trackingOscPathEditor.setVisible(false);  // Hidden by default
        trackingOscPathLabel.setVisible(false);

        // PSN Interface selector (shown when protocol is PSN)
        addAndMakeVisible(trackingPsnInterfaceLabel);
        trackingPsnInterfaceLabel.setText(LOC("network.labels.psnInterface"), juce::dontSendNotification);
        addAndMakeVisible(trackingPsnInterfaceSelector);
        trackingPsnInterfaceSelector.setVisible(false);  // Hidden by default
        trackingPsnInterfaceLabel.setVisible(false);
        trackingPsnInterfaceSelector.onChange = [this]() {
            // Save the selected PSN interface
            int selectedId = trackingPsnInterfaceSelector.getSelectedId();
            if (selectedId > 0 && selectedId <= psnInterfaceNames.size())
            {
                parameters.setConfigParam("trackingPsnInterface", psnInterfaceNames[selectedId - 1]);
            }
            // Update PSN receiver with new interface
            updateTrackingReceiver();
        };

        // MQTT controls (shown when protocol is MQTT)
        addAndMakeVisible(trackingMqttHostLabel);
        trackingMqttHostLabel.setText(LOC("network.labels.mqttHost"), juce::dontSendNotification);
        addAndMakeVisible(trackingMqttHostEditor);
        trackingMqttHostEditor.setText("192.168.1.1", juce::dontSendNotification);
        trackingMqttHostLabel.setVisible(false);
        trackingMqttHostEditor.setVisible(false);

        addAndMakeVisible(trackingMqttTopicLabel);
        trackingMqttTopicLabel.setText(LOC("network.labels.mqttTopic"), juce::dontSendNotification);
        addAndMakeVisible(trackingMqttTopicEditor);
        trackingMqttTopicEditor.setText("dwm/node/+/uplink/location", juce::dontSendNotification);
        trackingMqttTopicLabel.setVisible(false);
        trackingMqttTopicEditor.setVisible(false);

        addAndMakeVisible(trackingMqttTagIdsButton);
        trackingMqttTagIdsButton.setButtonText(LOC("network.labels.mqttTagIds"));
        trackingMqttTagIdsButton.setVisible(false);
        trackingMqttTagIdsButton.onClick = [this]() { showMqttTagIdPanel(); };

        // MQTT JSON field labels + comboboxes (editable, auto-populated from discovered keys)
        auto setupJsonCombo = [this](juce::Label& label, juce::ComboBox& combo,
                                      const juce::String& labelText, const juce::String& defaultVal) {
            addAndMakeVisible(label);
            label.setText(labelText, juce::dontSendNotification);
            label.setColour(juce::Label::textColourId, ColorScheme::get().textSecondary);
            addAndMakeVisible(combo);
            combo.setEditableText(true);
            combo.setText(defaultVal, juce::dontSendNotification);
            label.setVisible(false);
            combo.setVisible(false);
            combo.onChange = [this]() { saveMqttJsonFieldNames(); };
        };

        setupJsonCombo(trackingMqttJsonXLabel, trackingMqttJsonXCombo, LOC("network.labels.mqttJsonX"), "x");
        setupJsonCombo(trackingMqttJsonYLabel, trackingMqttJsonYCombo, LOC("network.labels.mqttJsonY"), "y");
        setupJsonCombo(trackingMqttJsonZLabel, trackingMqttJsonZCombo, LOC("network.labels.mqttJsonZ"), "z");
        setupJsonCombo(trackingMqttJsonQLabel, trackingMqttJsonQCombo, LOC("network.labels.mqttJsonQ"), "quality");

        // Add text editor listeners
        trackingPortEditor.addListener(this);
        trackingOffsetXEditor.addListener(this);
        trackingOffsetYEditor.addListener(this);
        trackingOffsetZEditor.addListener(this);
        trackingScaleXEditor.addListener(this);
        trackingScaleYEditor.addListener(this);
        trackingScaleZEditor.addListener(this);
        trackingOscPathEditor.addListener(this);
        trackingMqttHostEditor.addListener(this);
        trackingMqttTopicEditor.addListener(this);
    }

    // Helper to check if source component is or is a child of target (for ComboBox hover detection)
    bool isOrIsChildOf(juce::Component* source, juce::Component* target)
    {
        while (source != nullptr)
        {
            if (source == target) return true;
            source = source->getParentComponent();
        }
        return false;
    }

    void setupMouseListeners()
    {
        // ==================== NETWORK SECTION ====================
        networkInterfaceLabel.addMouseListener(this, false);
        networkInterfaceSelector.addMouseListener(this, true);  // true for ComboBox children
        currentIPLabel.addMouseListener(this, false);
        currentIPEditor.addMouseListener(this, false);
        udpPortLabel.addMouseListener(this, false);
        udpPortEditor.addMouseListener(this, false);
        tcpPortLabel.addMouseListener(this, false);
        tcpPortEditor.addMouseListener(this, false);
        oscQueryLabel.addMouseListener(this, false);
        oscQueryPortEditor.addMouseListener(this, false);
        oscQueryEnableButton.addMouseListener(this, false);

        // ==================== NETWORK CONNECTIONS TABLE ====================
        // Header labels already have mouse listeners from setupNetworkConnectionsTable
        addTargetButton.addMouseListener(this, false);
        openLogWindowButton.addMouseListener(this, false);
        findMyRemoteButton.addMouseListener(this, false);
        oscSourceFilterButton.addMouseListener(this, false);

        // Target row components
        for (int i = 0; i < maxTargets; ++i)
        {
            auto& row = targetRows[i];
            row.nameEditor.addMouseListener(this, false);
            row.dataModeSelector.addMouseListener(this, true);  // true for ComboBox children
            row.ipEditor.addMouseListener(this, false);
            row.txPortEditor.addMouseListener(this, false);
            row.rxEnableButton.addMouseListener(this, false);
            row.txEnableButton.addMouseListener(this, false);
            row.protocolSelector.addMouseListener(this, true);  // true for ComboBox children
            row.removeButton.addMouseListener(this, false);
        }

        // ==================== ADM-OSC SECTION ====================
        admMappingSelectorLabel.addMouseListener(this, false);
        admMappingSelector.addMouseListener(this, true);
        admMappingGraph.addMouseListener(this, false);
        admPolarGraph.addMouseListener(this, false);

        // ==================== TRACKING SECTION ====================
        trackingEnabledButton.addMouseListener(this, false);
        trackingProtocolLabel.addMouseListener(this, false);
        trackingProtocolSelector.addMouseListener(this, true);  // true for ComboBox children
        trackingPortLabel.addMouseListener(this, false);
        trackingPortEditor.addMouseListener(this, false);
        trackingOffsetXLabel.addMouseListener(this, false);
        trackingOffsetXEditor.addMouseListener(this, false);
        trackingOffsetYLabel.addMouseListener(this, false);
        trackingOffsetYEditor.addMouseListener(this, false);
        trackingOffsetZLabel.addMouseListener(this, false);
        trackingOffsetZEditor.addMouseListener(this, false);
        trackingScaleXLabel.addMouseListener(this, false);
        trackingScaleXEditor.addMouseListener(this, false);
        trackingScaleYLabel.addMouseListener(this, false);
        trackingScaleYEditor.addMouseListener(this, false);
        trackingScaleZLabel.addMouseListener(this, false);
        trackingScaleZEditor.addMouseListener(this, false);
        trackingFlipXButton.addMouseListener(this, false);
        trackingFlipYButton.addMouseListener(this, false);
        trackingFlipZButton.addMouseListener(this, false);
        trackingOscPathLabel.addMouseListener(this, false);
        trackingOscPathEditor.addMouseListener(this, false);
        trackingPsnInterfaceLabel.addMouseListener(this, false);
        trackingPsnInterfaceSelector.addMouseListener(this, true);  // true for ComboBox children

        // ==================== FOOTER BUTTONS ====================
        storeButton.addMouseListener(this, false);
        reloadButton.addMouseListener(this, false);
        reloadBackupButton.addMouseListener(this, false);
        importButton.addMouseListener(this, false);
        exportButton.addMouseListener(this, false);
    }

    void updateOSCManagerConfig()
    {
        if (oscManager == nullptr)
            return;

        // Build allowed IPs list from targets with rxEnabled=true
        juce::StringArray allowedIPs;
        for (int i = 0; i < maxTargets; ++i)
        {
            if (targetRows[i].isActive && targetRows[i].rxEnableButton.getToggleState())
            {
                juce::String ip = targetRows[i].ipEditor.getText().trim();
                if (ip.isNotEmpty() && !allowedIPs.contains(ip))
                    allowedIPs.add(ip);
            }
        }

        // Apply global config including IP filtering
        WFSNetwork::GlobalConfig globalConfig;
        globalConfig.udpReceivePort = udpPortEditor.getText().getIntValue();
        globalConfig.tcpReceivePort = tcpPortEditor.getText().getIntValue();
        globalConfig.ipFilteringEnabled = oscSourceFilterButton.getToggleState();
        globalConfig.allowedIPs = allowedIPs;
        oscManager->applyGlobalConfig(globalConfig);

        // Apply target configs
        for (int i = 0; i < maxTargets; ++i)
        {
            WFSNetwork::TargetConfig targetConfig;
            targetConfig.name = targetRows[i].nameEditor.getText();
            targetConfig.ipAddress = targetRows[i].ipEditor.getText();
            targetConfig.port = targetRows[i].txPortEditor.getText().getIntValue();
            targetConfig.rxEnabled = targetRows[i].rxEnableButton.getToggleState();
            targetConfig.txEnabled = targetRows[i].txEnableButton.getToggleState();

            // Map protocol selector to enum
            int protocolId = targetRows[i].protocolSelector.getSelectedId();
            switch (protocolId)
            {
                case 1: targetConfig.protocol = WFSNetwork::Protocol::Disabled; break;
                case 2: targetConfig.protocol = WFSNetwork::Protocol::OSC; break;
                case 3: targetConfig.protocol = WFSNetwork::Protocol::Remote; break;
                case 4: targetConfig.protocol = WFSNetwork::Protocol::ADMOSC; break;
                case 8: targetConfig.protocol = WFSNetwork::Protocol::QLab; break;
                default: targetConfig.protocol = WFSNetwork::Protocol::Disabled; break;
            }

            // Map data mode to enum
            int modeId = targetRows[i].dataModeSelector.getSelectedId();
            targetConfig.mode = (modeId == 2) ? WFSNetwork::ConnectionMode::TCP
                                               : WFSNetwork::ConnectionMode::UDP;

            // QLab patch number
            targetConfig.qlabPatchNumber = targetRows[i].qlabPatchEditor.getText().getIntValue();

            oscManager->applyTargetConfig(i, targetConfig);

            // Connect if protocol is enabled and tx is enabled
            bool shouldConnect = (targetConfig.protocol != WFSNetwork::Protocol::Disabled)
                && targetConfig.txEnabled;
            if (shouldConnect)
            {
                oscManager->connectTarget(i);
            }
        }

        // Start listening if any target has Rx enabled
        bool anyRxEnabled = false;
        for (int i = 0; i < maxTargets; ++i)
        {
            if (targetRows[i].rxEnableButton.getToggleState())
            {
                anyRxEnabled = true;
                break;
            }
        }

        if (anyRxEnabled && !oscManager->isListening())
        {
            oscManager->startListening();
        }
    }

    void setupNetworkConnectionsTable()
    {
        // ==================== HEADER ROW ====================
        auto setupHeaderLabel = [this](juce::Label& label, const juce::String& text)
        {
            addAndMakeVisible(label);
            label.setText(text, juce::dontSendNotification);
            label.setFont(juce::FontOptions().withHeight(12.0f).withStyle("Bold"));
            label.setJustificationType(juce::Justification::centred);

            // Add mouse listener for hover help
            label.addMouseListener(this, false);
        };

        setupHeaderLabel(headerNameLabel, LOC("network.table.name"));
        setupHeaderLabel(headerProtocolLabel, LOC("network.table.protocol"));
        setupHeaderLabel(headerDataModeLabel, LOC("network.table.mode"));
        setupHeaderLabel(headerIpLabel, LOC("network.table.ipv4Address"));
        setupHeaderLabel(headerTxPortLabel, LOC("network.table.txPort"));
        setupHeaderLabel(headerRxEnableLabel, LOC("network.table.rx"));
        setupHeaderLabel(headerTxEnableLabel, LOC("network.table.tx"));

        // Add button in header
        addAndMakeVisible(addTargetButton);
        addTargetButton.setButtonText(LOC("network.buttons.add"));
        addTargetButton.onClick = [this]() { addNewTarget(); };

        // ==================== TARGET ROWS ====================
        for (int i = 0; i < maxTargets; ++i)
        {
            auto& row = targetRows[i];

            // Name editor
            addAndMakeVisible(row.nameEditor);
            row.nameEditor.setText(LOC("network.table.defaultTarget").replace("{num}", juce::String(i + 1)), false);
            row.nameEditor.setJustification(juce::Justification::centred);

            // Data Mode selector (UDP/TCP)
            addAndMakeVisible(row.dataModeSelector);
            row.dataModeSelector.addItem(LOC("network.protocols.udp"), 1);
            row.dataModeSelector.addItem(LOC("network.protocols.tcp"), 2);
            row.dataModeSelector.setSelectedId(1, juce::dontSendNotification);
            row.dataModeSelector.onChange = [this, i]() {
                saveTargetToValueTree(i);
                // TTS: Announce selection change
                TTSManager::getInstance().announceValueChange(LOC("network.table.defaultTarget").replace("{num}", juce::String(i + 1)) + " " + LOC("network.table.mode"), targetRows[i].dataModeSelector.getText());
            };

            // IP editor
            addAndMakeVisible(row.ipEditor);
            row.ipEditor.setText("127.0.0.1", false);
            row.ipEditor.setJustification(juce::Justification::centred);

            // Tx Port editor
            addAndMakeVisible(row.txPortEditor);
            row.txPortEditor.setText("9000", false);
            row.txPortEditor.setInputRestrictions(5, "0123456789");
            row.txPortEditor.setJustification(juce::Justification::centred);

            // Rx Enable button
            addAndMakeVisible(row.rxEnableButton);
            row.rxEnableButton.setButtonText(LOC("network.toggles.off"));
            row.rxEnableButton.setClickingTogglesState(true);
            row.rxEnableButton.onClick = [this, i]() {
                auto& btn = targetRows[i].rxEnableButton;
                btn.setButtonText(btn.getToggleState() ? LOC("network.toggles.on") : LOC("network.toggles.off"));
                saveTargetToValueTree(i);
            };

            // Tx Enable button
            addAndMakeVisible(row.txEnableButton);
            row.txEnableButton.setButtonText(LOC("network.toggles.off"));
            row.txEnableButton.setClickingTogglesState(true);
            row.txEnableButton.onClick = [this, i]() {
                auto& btn = targetRows[i].txEnableButton;
                btn.setButtonText(btn.getToggleState() ? LOC("network.toggles.on") : LOC("network.toggles.off"));
                saveTargetToValueTree(i);
            };

            // Protocol selector
            addAndMakeVisible(row.protocolSelector);
            row.protocolSelector.addItem(LOC("network.protocols.disabled"), 1);
            row.protocolSelector.addItem(LOC("network.protocols.osc"), 2);
            row.protocolSelector.addItem(LOC("network.protocols.remote"), 3);
            row.protocolSelector.addItem(LOC("network.protocols.admOsc"), 4);
            row.protocolSelector.addItem(LOC("network.protocols.qlab"), 8);  // QLab = Protocol enum 7, ComboBox ID = 7+1
            row.protocolSelector.setSelectedId(1, juce::dontSendNotification);
            row.protocolSelector.onChange = [this, i]() {
                // Check if trying to select REMOTE when one already exists
                if (targetRows[i].protocolSelector.getSelectedId() == 3)  // REMOTE
                {
                    if (hasExistingRemoteConnection(i))
                    {
                        // Revert to DISABLED and show message
                        targetRows[i].protocolSelector.setSelectedId(1, juce::dontSendNotification);
                        if (statusBar != nullptr)
                            statusBar->setHelpText(LOC("network.messages.onlyOneRemote"));
                        return;
                    }
                }
                // Defaults for ADM-OSC targets
                if (targetRows[i].protocolSelector.getSelectedId() == 4)  // ADM-OSC
                {
                    if (targetRows[i].txPortEditor.getText() == "9000")
                        targetRows[i].txPortEditor.setText ("4001", false);
                }
                // Defaults for QLab targets
                if (targetRows[i].protocolSelector.getSelectedId() == 8)  // QLab
                {
                    if (targetRows[i].txPortEditor.getText() == "9000")
                        targetRows[i].txPortEditor.setText(juce::String(WFSNetwork::DEFAULT_QLAB_PORT), false);
                    // Auto-enable Tx and default name
                    targetRows[i].txEnableButton.setToggleState(true, juce::dontSendNotification);
                    if (targetRows[i].nameEditor.getText() == LOC("network.table.defaultTarget").replace("{num}", juce::String(i + 1)))
                        targetRows[i].nameEditor.setText("QLab", false);
                }
                // Update ADM-OSC appearance when protocol changes
                updateAdmOscAppearance();
                updateQLabAppearance();
                saveTargetToValueTree(i);
                // TTS: Announce selection change
                TTSManager::getInstance().announceValueChange(LOC("network.table.defaultTarget").replace("{num}", juce::String(i + 1)) + " " + LOC("network.table.protocol"), targetRows[i].protocolSelector.getText());
            };

            // QLab patch number editor
            addAndMakeVisible(row.qlabPatchLabel);
            row.qlabPatchLabel.setText("Patch", juce::dontSendNotification);
            row.qlabPatchLabel.setFont(juce::Font(juce::FontOptions(11.0f)));
            row.qlabPatchLabel.setJustificationType(juce::Justification::centred);
            row.qlabPatchLabel.setVisible(false);

            addAndMakeVisible(row.qlabPatchEditor);
            row.qlabPatchEditor.setText("1", false);
            row.qlabPatchEditor.setInputRestrictions(2, "0123456789");
            row.qlabPatchEditor.setJustification(juce::Justification::centred);
            row.qlabPatchEditor.onTextChange = [this, i]() { saveTargetToValueTree(i); };
            row.qlabPatchEditor.setVisible(false);

            // Remove button (long press to avoid accidental deletion)
            addAndMakeVisible(row.removeButton);
            row.removeButton.setButtonText("X");
            row.removeButton.onLongPress = [this, i]() { removeTarget(i); };

            // Add text change listeners
            row.nameEditor.onTextChange = [this, i]() { saveTargetToValueTree(i); };
            row.ipEditor.onTextChange = [this, i]() { saveTargetToValueTree(i); };
            row.txPortEditor.onTextChange = [this, i]() { saveTargetToValueTree(i); };

            // Start with rows disabled - user must add them
            row.isActive = false;
        }

        // Initialize active count to 0 - no active targets by default
        activeTargetCount = 0;
        updateAddButtonState();
        updateTargetRowVisibility();

        // ==================== BUTTONS BENEATH TABLE ====================
        addAndMakeVisible(openLogWindowButton);
        openLogWindowButton.setButtonText(LOC("network.buttons.openLogWindow"));
        openLogWindowButton.onClick = [this]() { openNetworkLogWindow(); };

        addAndMakeVisible(findMyRemoteButton);
        findMyRemoteButton.setButtonText(LOC("network.buttons.findMyRemote"));
        findMyRemoteButton.onClick = [this]() { showFindMyRemoteDialog(); };

        // OSC Source Filter toggle
        addAndMakeVisible(oscSourceFilterButton);
        oscSourceFilterButton.setButtonText(LOC("network.toggles.oscFilterAcceptAll"));
        oscSourceFilterButton.setClickingTogglesState(true);
        oscSourceFilterButton.onClick = [this]() {
            oscSourceFilterButton.setButtonText(
                oscSourceFilterButton.getToggleState()
                    ? LOC("network.toggles.oscFilterRegisteredOnly")
                    : LOC("network.toggles.oscFilterAcceptAll")
            );
            saveOscSourceFilterToValueTree();
            updateOSCManagerConfig();
        };
    }

    void addNewTarget()
    {
        if (activeTargetCount >= maxTargets)
        {
            if (statusBar != nullptr)
                statusBar->setHelpText(LOC("network.messages.maxTargetsReached"));
            return;
        }

        // Find first inactive row and activate it
        for (int i = 0; i < maxTargets; ++i)
        {
            if (!targetRows[i].isActive)
            {
                targetRows[i].isActive = true;
                activeTargetCount++;
                updateTargetRowVisibility();
                updateAddButtonState();
                saveTargetToValueTree(i);  // Save new target to ValueTree
                break;
            }
        }
    }

    void confirmRemoveTarget(int index)
    {
        juce::String targetName = targetRows[index].nameEditor.getText();
        if (targetName.isEmpty())
            targetName = LOC("network.table.defaultTarget").replace("{num}", juce::String(index + 1));

        juce::AlertWindow::showOkCancelBox(
            juce::AlertWindow::QuestionIcon,
            LOC("network.dialogs.removeTargetTitle"),
            LOC("network.dialogs.removeTargetMessage").replace("{name}", targetName),
            LOC("common.ok"),
            LOC("common.cancel"),
            nullptr,
            juce::ModalCallbackFunction::create([this, index](int result) {
                if (result == 1)  // OK clicked
                {
                    removeTarget(index);
                }
            })
        );
    }

    void removeTarget(int index)
    {
        if (index < 0 || index >= maxTargets)
            return;

        // Remove from ValueTree first
        removeTargetFromValueTree(index);

        // Reset row to defaults
        auto& row = targetRows[index];
        row.nameEditor.setText(LOC("network.table.defaultTarget").replace("{num}", juce::String(index + 1)), false);
        row.dataModeSelector.setSelectedId(1, juce::dontSendNotification);
        row.ipEditor.setText("127.0.0.1", false);
        row.txPortEditor.setText("9000", false);
        row.rxEnableButton.setToggleState(false, juce::dontSendNotification);
        row.rxEnableButton.setButtonText(LOC("network.toggles.off"));
        row.txEnableButton.setToggleState(false, juce::dontSendNotification);
        row.txEnableButton.setButtonText(LOC("network.toggles.off"));
        row.protocolSelector.setSelectedId(1, juce::dontSendNotification);
        row.isActive = false;

        activeTargetCount--;
        updateTargetRowVisibility();
        updateAddButtonState();
        updateAdmOscAppearance();
    }

    void updateTargetRowVisibility()
    {
        for (int i = 0; i < maxTargets; ++i)
        {
            auto& row = targetRows[i];
            float alpha = row.isActive ? 1.0f : 0.2f;  // Very faint when disabled

            row.nameEditor.setAlpha(alpha);
            row.nameEditor.setEnabled(row.isActive);
            row.dataModeSelector.setAlpha(alpha);
            row.dataModeSelector.setEnabled(row.isActive);
            row.ipEditor.setAlpha(alpha);
            row.ipEditor.setEnabled(row.isActive);
            row.txPortEditor.setAlpha(alpha);
            row.txPortEditor.setEnabled(row.isActive);
            row.rxEnableButton.setAlpha(alpha);
            row.rxEnableButton.setEnabled(row.isActive);
            row.txEnableButton.setAlpha(alpha);
            row.txEnableButton.setEnabled(row.isActive);
            row.protocolSelector.setAlpha(alpha);
            row.protocolSelector.setEnabled(row.isActive);
            row.qlabPatchLabel.setAlpha(alpha);
            row.qlabPatchEditor.setAlpha(alpha);
            row.qlabPatchEditor.setEnabled(row.isActive);
            row.removeButton.setAlpha(alpha);
            row.removeButton.setEnabled(row.isActive);

            // Reset connection status color when visibility changes
            if (row.isActive && oscManager != nullptr)
            {
                updateTargetConnectionStatus(i, oscManager->getTargetStatus(i));
            }
            else
            {
                // Inactive rows get default background from theme
                row.nameEditor.setColour(juce::TextEditor::backgroundColourId, ColorScheme::get().surfaceCard);
            }
        }
    }

    void updateTargetConnectionStatus(int targetIndex, WFSNetwork::ConnectionStatus status)
    {
        if (targetIndex < 0 || targetIndex >= maxTargets)
            return;

        auto& row = targetRows[targetIndex];
        if (!row.isActive)
            return;

        // Set background and text color based on connection status
        juce::Colour bgColor;
        juce::Colour textColor;

        switch (status)
        {
            case WFSNetwork::ConnectionStatus::Connected:
                bgColor = juce::Colour(0xFF1A4D1A);  // Dark green tint
                textColor = juce::Colours::white;
                break;
            case WFSNetwork::ConnectionStatus::Connecting:
                bgColor = juce::Colour(0xFF4D4D1A);  // Dark yellow tint
                textColor = juce::Colours::white;
                break;
            case WFSNetwork::ConnectionStatus::Error:
                bgColor = juce::Colour(0xFF4D1A1A);  // Dark red tint
                textColor = juce::Colours::white;
                break;
            case WFSNetwork::ConnectionStatus::Disconnected:
            default:
                bgColor = ColorScheme::get().surfaceCard;  // Default from theme
                textColor = ColorScheme::get().textPrimary;
                break;
        }

        row.nameEditor.setColour(juce::TextEditor::backgroundColourId, bgColor);
        row.nameEditor.setColour(juce::TextEditor::textColourId, textColor);
        row.nameEditor.applyFontToAllText(row.nameEditor.getFont(), true);
        row.nameEditor.repaint();
    }

    void updateAddButtonState()
    {
        bool canAdd = activeTargetCount < maxTargets;
        addTargetButton.setEnabled(canAdd);
        addTargetButton.setAlpha(canAdd ? 1.0f : 0.4f);
    }

    bool hasExistingRemoteConnection(int excludeIndex = -1)
    {
        for (int i = 0; i < maxTargets; ++i)
        {
            if (i == excludeIndex) continue;
            if (targetRows[i].isActive && targetRows[i].protocolSelector.getSelectedId() == 3)  // REMOTE
                return true;
        }
        return false;
    }

    void updateAdmOscAppearance()
    {
        bool hasAdmOsc = false;
        for (int i = 0; i < maxTargets; ++i)
        {
            if (targetRows[i].isActive &&
                targetRows[i].protocolSelector.getSelectedId() == 4)  // ADM-OSC
            {
                hasAdmOsc = true;
                break;
            }
        }

        float alpha = hasAdmOsc ? 1.0f : 0.4f;
        bool enabled = hasAdmOsc;

        admMappingSelectorLabel.setAlpha(alpha);
        admMappingSelector.setAlpha(alpha);
        admMappingSelector.setEnabled(enabled);

        admMappingGraph.setAlpha(alpha);
        admMappingGraph.setEnabled(enabled);
        admPolarGraph.setAlpha(alpha);
        admPolarGraph.setEnabled(enabled);
    }

    /** Show/hide Cartesian vs Polar editors based on currentAdmMapping */
    void updateAdmMappingVisibility()
    {
        bool isCart = currentAdmMapping < 4;
        bool isPolar = currentAdmMapping >= 4;

        // Cartesian panel
        admMappingGraph.setVisible(isCart);

        // Polar panel
        admPolarGraph.setVisible(isPolar);

        // Recalculate layout so only visible controls consume space
        resized();
    }

    /** Find a Cart or Polar mapping ValueTree node by index */
    juce::UndoManager* getConfigUndoManager()
    {
        return parameters.getValueTreeState().getUndoManagerForDomain(UndoDomain::Config);
    }

    juce::ValueTree findAdmMappingNode(int mappingIndex)
    {
        auto admosc = parameters.getValueTreeState().getADMOSCState();
        if (!admosc.isValid()) return {};

        bool isCart = mappingIndex < 4;
        int subIndex = isCart ? mappingIndex : (mappingIndex - 4);
        auto targetType = isCart ? WFSParameterIDs::ADMCartMapping : WFSParameterIDs::ADMPolarMapping;

        for (int c = 0; c < admosc.getNumChildren(); ++c)
        {
            auto child = admosc.getChild(c);
            if (child.getType() == targetType && (int)child.getProperty(WFSParameterIDs::id) == subIndex)
                return child;
        }
        return {};
    }

    /** Load current mapping config from ValueTree into editors */
    void loadAdmMappingToUI()
    {
        auto mappingNode = findAdmMappingNode(currentAdmMapping);
        if (!mappingNode.isValid()) return;

        if (currentAdmMapping < 4)
        {
            // Cartesian mapping — panel handles all display
            auto cfg = ADMOSCMapping::loadCartesianConfig(mappingNode);
            admMappingGraph.setConfig(cfg);
        }
        else
        {
            // Polar mapping — panel handles all display
            auto cfg = ADMOSCMapping::loadPolarConfig(mappingNode);
            admPolarGraph.setConfig(cfg);
        }
    }

    // Note: saveAdmMappingFromUI() removed — both Cartesian and Polar now save via panel callbacks

    /** Helper to find an axis ValueTree node within a mapping node */
    juce::ValueTree findAxisNode (const juce::ValueTree& mappingNode, int axis) const
    {
        for (int c = 0; c < mappingNode.getNumChildren(); ++c)
        {
            auto child = mappingNode.getChild(c);
            if (child.getType() == WFSParameterIDs::ADMCartAxis &&
                (int)child.getProperty(WFSParameterIDs::admCartAxisId) == axis)
                return child;
        }
        return {};
    }

    void updateQLabAppearance()
    {
        for (int i = 0; i < maxTargets; ++i)
        {
            auto& row = targetRows[i];
            bool isQLab = (row.protocolSelector.getSelectedId() == 8);  // QLab

            // Rx/Tx always visible
            row.rxEnableButton.setVisible(row.isActive);
            row.txEnableButton.setVisible(row.isActive);

            // QLab patch fields in name area
            row.qlabPatchLabel.setVisible(isQLab && row.isActive);
            row.qlabPatchEditor.setVisible(isQLab && row.isActive);

            // Shrink/restore name editor to make room for patch
            if (cachedNameColWidth > 0)
            {
                auto nb = row.nameEditor.getBounds();
                if (isQLab && row.isActive)
                    row.nameEditor.setBounds(nb.getX(), nb.getY(), cachedNameColWidth / 2, nb.getHeight());
                else
                    row.nameEditor.setBounds(nb.getX(), nb.getY(), cachedNameColWidth, nb.getHeight());
            }
        }
    }

    void updateTrackingAppearance()
    {
        bool enabled = trackingEnabledButton.getToggleState();
        float alpha = enabled ? 1.0f : 0.4f;

        trackingProtocolLabel.setAlpha(alpha);
        trackingProtocolSelector.setAlpha(alpha);
        trackingPortLabel.setAlpha(alpha);
        trackingPortEditor.setAlpha(alpha);

        trackingOffsetXLabel.setAlpha(alpha);
        trackingOffsetXEditor.setAlpha(alpha);
        trackingOffsetXUnitLabel.setAlpha(alpha);
        trackingOffsetYLabel.setAlpha(alpha);
        trackingOffsetYEditor.setAlpha(alpha);
        trackingOffsetYUnitLabel.setAlpha(alpha);
        trackingOffsetZLabel.setAlpha(alpha);
        trackingOffsetZEditor.setAlpha(alpha);
        trackingOffsetZUnitLabel.setAlpha(alpha);

        trackingScaleXLabel.setAlpha(alpha);
        trackingScaleXEditor.setAlpha(alpha);
        trackingScaleXUnitLabel.setAlpha(alpha);
        trackingScaleYLabel.setAlpha(alpha);
        trackingScaleYEditor.setAlpha(alpha);
        trackingScaleYUnitLabel.setAlpha(alpha);
        trackingScaleZLabel.setAlpha(alpha);
        trackingScaleZEditor.setAlpha(alpha);
        trackingScaleZUnitLabel.setAlpha(alpha);

        trackingFlipXButton.setAlpha(alpha);
        trackingFlipYButton.setAlpha(alpha);
        trackingFlipZButton.setAlpha(alpha);

        // Protocol-specific controls visibility
        int protocolId = trackingProtocolSelector.getSelectedId();
        bool isOscProtocol = (protocolId == 2);
        bool isPsnProtocol = (protocolId == 3);
        bool isMqttProtocol = (protocolId == 5);

        trackingOscPathLabel.setVisible(isOscProtocol);
        trackingOscPathEditor.setVisible(isOscProtocol);
        trackingOscPathLabel.setAlpha(alpha);
        trackingOscPathEditor.setAlpha(alpha);

        trackingPsnInterfaceLabel.setVisible(isPsnProtocol);
        trackingPsnInterfaceSelector.setVisible(isPsnProtocol);
        trackingPsnInterfaceLabel.setAlpha(alpha);
        trackingPsnInterfaceSelector.setAlpha(alpha);

        trackingMqttHostLabel.setVisible(isMqttProtocol);
        trackingMqttHostEditor.setVisible(isMqttProtocol);
        trackingMqttTopicLabel.setVisible(isMqttProtocol);
        trackingMqttTopicEditor.setVisible(isMqttProtocol);
        trackingMqttTagIdsButton.setVisible(isMqttProtocol);
        trackingMqttJsonXLabel.setVisible(isMqttProtocol);
        trackingMqttJsonXCombo.setVisible(isMqttProtocol);
        trackingMqttJsonYLabel.setVisible(isMqttProtocol);
        trackingMqttJsonYCombo.setVisible(isMqttProtocol);
        trackingMqttJsonZLabel.setVisible(isMqttProtocol);
        trackingMqttJsonZCombo.setVisible(isMqttProtocol);
        trackingMqttJsonQLabel.setVisible(isMqttProtocol);
        trackingMqttJsonQCombo.setVisible(isMqttProtocol);
        trackingMqttHostLabel.setAlpha(alpha);
        trackingMqttHostEditor.setAlpha(alpha);
        trackingMqttTopicLabel.setAlpha(alpha);
        trackingMqttTopicEditor.setAlpha(alpha);
        trackingMqttTagIdsButton.setAlpha(alpha);
        trackingMqttJsonXLabel.setAlpha(alpha);
        trackingMqttJsonXCombo.setAlpha(alpha);
        trackingMqttJsonYLabel.setAlpha(alpha);
        trackingMqttJsonYCombo.setAlpha(alpha);
        trackingMqttJsonZLabel.setAlpha(alpha);
        trackingMqttJsonZCombo.setAlpha(alpha);
        trackingMqttJsonQLabel.setAlpha(alpha);
        trackingMqttJsonQCombo.setAlpha(alpha);

        // Update tracking receiver state
        updateTrackingReceiver();
    }

    /**
     * Start/stop the tracking receiver based on current settings.
     * Handles both OSC and PSN protocols.
     * Called when tracking enabled, protocol, port, path, or interface changes.
     * @param forceRestart If true, always restart the receiver (e.g., when port changes)
     */
    void updateTrackingReceiver(bool forceRestart = false)
    {
        if (oscManager == nullptr)
            return;

        bool trackingEnabled = trackingEnabledButton.getToggleState();
        int protocolId = trackingProtocolSelector.getSelectedId();
        bool isOscProtocol = (protocolId == 2);   // OSC = ID 2
        bool isPsnProtocol = (protocolId == 3);   // PSN = ID 3
        bool isRttrpProtocol = (protocolId == 4); // RTTrP = ID 4
        bool isMqttProtocol = (protocolId == 5);  // MQTT = ID 5

        // Get transformation values (shared by all protocols)
        float offsetX = trackingOffsetXEditor.getText().getFloatValue();
        float offsetY = trackingOffsetYEditor.getText().getFloatValue();
        float offsetZ = trackingOffsetZEditor.getText().getFloatValue();
        float scaleX = trackingScaleXEditor.getText().getFloatValue();
        float scaleY = trackingScaleYEditor.getText().getFloatValue();
        float scaleZ = trackingScaleZEditor.getText().getFloatValue();
        bool flipX = trackingFlipXButton.getToggleState();
        bool flipY = trackingFlipYButton.getToggleState();
        bool flipZ = trackingFlipZButton.getToggleState();

        // Ensure scale values are valid
        if (scaleX <= 0.0f) scaleX = 1.0f;
        if (scaleY <= 0.0f) scaleY = 1.0f;
        if (scaleZ <= 0.0f) scaleZ = 1.0f;

        // Stop all receivers first if switching protocols or disabling
        if (!trackingEnabled || (!isOscProtocol && oscManager->isTrackingReceiverRunning()))
        {
            oscManager->stopTrackingReceiver();
        }
        if (!trackingEnabled || (!isPsnProtocol && oscManager->isPSNReceiverRunning()))
        {
            oscManager->stopPSNReceiver();
        }
        if (!trackingEnabled || (!isRttrpProtocol && oscManager->isRTTrPReceiverRunning()))
        {
            oscManager->stopRTTrPReceiver();
        }
        if (!trackingEnabled || (!isMqttProtocol && oscManager->isMQTTReceiverRunning()))
        {
            oscManager->stopMQTTReceiver();
        }

        if (trackingEnabled && isOscProtocol)
        {
            // Get port and validate
            int port = trackingPortEditor.getText().getIntValue();
            if (port <= 0 || port > 65535)
            {
                oscManager->stopTrackingReceiver();
                return;
            }

            // Get path pattern and validate
            juce::String pathPattern = trackingOscPathEditor.getText().trim();
            if (!isValidOscPath(pathPattern))
            {
                oscManager->stopTrackingReceiver();
                return;
            }

            // Start or update the tracking receiver
            bool needsRestart = forceRestart || !oscManager->isTrackingReceiverRunning();

            if (needsRestart)
            {
                // Start (or restart) the receiver
                if (oscManager->startTrackingReceiver(port, pathPattern))
                {
                    // Set initial transformations
                    oscManager->updateTrackingTransformations(offsetX, offsetY, offsetZ,
                                                              scaleX, scaleY, scaleZ,
                                                              flipX, flipY, flipZ);
                }
                else
                {
                    DBG("NetworkTab: Failed to start tracking OSC receiver on port " << port);
                }
            }
            else
            {
                // Update transformations and path pattern in place
                oscManager->updateTrackingTransformations(offsetX, offsetY, offsetZ,
                                                          scaleX, scaleY, scaleZ,
                                                          flipX, flipY, flipZ);
                oscManager->updateTrackingPathPattern(pathPattern);
            }
        }
        else if (trackingEnabled && isPsnProtocol)
        {
            // Get port and validate
            int port = trackingPortEditor.getText().getIntValue();
            if (port <= 0 || port > 65535)
            {
                oscManager->stopPSNReceiver();
                return;
            }

            // Get selected PSN interface
            juce::String psnInterface;
            int selectedId = trackingPsnInterfaceSelector.getSelectedId();
            if (selectedId > 0 && selectedId <= psnInterfaceNames.size())
            {
                // Get the IP address for the selected interface
                psnInterface = psnInterfaceIPs[selectedId - 1];
            }

            // Start or update the PSN receiver
            bool needsRestart = forceRestart || !oscManager->isPSNReceiverRunning();

            if (needsRestart)
            {
                // Start (or restart) the PSN receiver
                if (oscManager->startPSNReceiver(port, psnInterface))
                {
                    // Set initial transformations
                    oscManager->updatePSNTransformations(offsetX, offsetY, offsetZ,
                                                         scaleX, scaleY, scaleZ,
                                                         flipX, flipY, flipZ);
                }
                else
                {
                    DBG("NetworkTab: Failed to start PSN receiver on port " << port);
                }
            }
            else
            {
                // Update transformations in place
                oscManager->updatePSNTransformations(offsetX, offsetY, offsetZ,
                                                     scaleX, scaleY, scaleZ,
                                                     flipX, flipY, flipZ);
            }
        }
        else if (trackingEnabled && isRttrpProtocol)
        {
            // Get port and validate
            int port = trackingPortEditor.getText().getIntValue();
            if (port <= 0 || port > 65535)
            {
                oscManager->stopRTTrPReceiver();
                return;
            }

            // Start or update the RTTrP receiver
            bool needsRestart = forceRestart || !oscManager->isRTTrPReceiverRunning();

            if (needsRestart)
            {
                // Start (or restart) the RTTrP receiver
                if (oscManager->startRTTrPReceiver(port))
                {
                    // Set initial transformations
                    oscManager->updateRTTrPTransformations(offsetX, offsetY, offsetZ,
                                                           scaleX, scaleY, scaleZ,
                                                           flipX, flipY, flipZ);
                }
                else
                {
                    DBG("NetworkTab: Failed to start RTTrP receiver on port " << port);
                }
            }
            else
            {
                // Update transformations in place
                oscManager->updateRTTrPTransformations(offsetX, offsetY, offsetZ,
                                                       scaleX, scaleY, scaleZ,
                                                       flipX, flipY, flipZ);
            }
        }
        else if (trackingEnabled && isMqttProtocol)
        {
            // Get host, port, and topic
            juce::String host = trackingMqttHostEditor.getText().trim();
            int port = trackingPortEditor.getText().getIntValue();
            juce::String topic = trackingMqttTopicEditor.getText().trim();

            if (host.isEmpty() || port <= 0 || port > 65535 || topic.isEmpty())
            {
                oscManager->stopMQTTReceiver();
                return;
            }

            bool needsRestart = forceRestart || !oscManager->isMQTTReceiverRunning();

            if (needsRestart)
            {
                // Save config
                parameters.setConfigParam("trackingMqttHost", host);
                parameters.setConfigParam("trackingMqttTopic", topic);
                parameters.setConfigParam("trackingMqttJsonX", trackingMqttJsonXCombo.getText().trim());
                parameters.setConfigParam("trackingMqttJsonY", trackingMqttJsonYCombo.getText().trim());
                parameters.setConfigParam("trackingMqttJsonZ", trackingMqttJsonZCombo.getText().trim());
                parameters.setConfigParam("trackingMqttJsonQ", trackingMqttJsonQCombo.getText().trim());

                if (oscManager->startMQTTReceiver(host, port, topic))
                {
                    oscManager->updateMQTTTransformations(offsetX, offsetY, offsetZ,
                                                           scaleX, scaleY, scaleZ,
                                                           flipX, flipY, flipZ);

                    // Wire up JSON key auto-discovery callback
                    if (oscManager->getMQTTReceiver() != nullptr)
                    {
                        oscManager->getMQTTReceiver()->onJsonKeysDiscovered =
                            [this](const juce::StringArray& keys) { populateMqttJsonComboboxes(keys); };
                    }
                }
                else
                {
                    DBG("NetworkTab: Failed to start MQTT receiver for " << host << ":" << port);
                }
            }
            else
            {
                oscManager->updateMQTTTransformations(offsetX, offsetY, offsetZ,
                                                       scaleX, scaleY, scaleZ,
                                                       flipX, flipY, flipZ);
            }
        }
    }

    /**
     * Update just the tracking transformations without restarting receiver.
     * Called when offset/scale/flip values change while receiver is running.
     * Updates OSC, PSN, RTTrP, and MQTT receivers if they are running.
     */
    void updateTrackingTransformations()
    {
        if (oscManager == nullptr)
            return;

        // Check if any receiver is running
        bool oscRunning = oscManager->isTrackingReceiverRunning();
        bool psnRunning = oscManager->isPSNReceiverRunning();
        bool rttrpRunning = oscManager->isRTTrPReceiverRunning();
        bool mqttRunning = oscManager->isMQTTReceiverRunning();
        if (!oscRunning && !psnRunning && !rttrpRunning && !mqttRunning)
            return;

        float offsetX = trackingOffsetXEditor.getText().getFloatValue();
        float offsetY = trackingOffsetYEditor.getText().getFloatValue();
        float offsetZ = trackingOffsetZEditor.getText().getFloatValue();
        float scaleX = trackingScaleXEditor.getText().getFloatValue();
        float scaleY = trackingScaleYEditor.getText().getFloatValue();
        float scaleZ = trackingScaleZEditor.getText().getFloatValue();
        bool flipX = trackingFlipXButton.getToggleState();
        bool flipY = trackingFlipYButton.getToggleState();
        bool flipZ = trackingFlipZButton.getToggleState();

        // Ensure scale values are valid
        if (scaleX <= 0.0f) scaleX = 1.0f;
        if (scaleY <= 0.0f) scaleY = 1.0f;
        if (scaleZ <= 0.0f) scaleZ = 1.0f;

        if (oscRunning)
        {
            oscManager->updateTrackingTransformations(offsetX, offsetY, offsetZ,
                                                      scaleX, scaleY, scaleZ,
                                                      flipX, flipY, flipZ);
        }
        if (psnRunning)
        {
            oscManager->updatePSNTransformations(offsetX, offsetY, offsetZ,
                                                 scaleX, scaleY, scaleZ,
                                                 flipX, flipY, flipZ);
        }
        if (rttrpRunning)
        {
            oscManager->updateRTTrPTransformations(offsetX, offsetY, offsetZ,
                                                   scaleX, scaleY, scaleZ,
                                                   flipX, flipY, flipZ);
        }
        if (mqttRunning)
        {
            oscManager->updateMQTTTransformations(offsetX, offsetY, offsetZ,
                                                   scaleX, scaleY, scaleZ,
                                                   flipX, flipY, flipZ);
        }
    }

    /**
     * Show the MQTT Tag ID mapping panel as a dialog.
     * 32 slots, sequential mapping (slot N = input N).
     */
    /** Save current JSON field name selections to config and update receiver. */
    void saveMqttJsonFieldNames()
    {
        juce::String kx = trackingMqttJsonXCombo.getText().trim();
        juce::String ky = trackingMqttJsonYCombo.getText().trim();
        juce::String kz = trackingMqttJsonZCombo.getText().trim();
        juce::String kq = trackingMqttJsonQCombo.getText().trim();

        parameters.setConfigParam ("trackingMqttJsonX", kx);
        parameters.setConfigParam ("trackingMqttJsonY", ky);
        parameters.setConfigParam ("trackingMqttJsonZ", kz);
        parameters.setConfigParam ("trackingMqttJsonQ", kq);

        if (oscManager != nullptr && oscManager->getMQTTReceiver() != nullptr)
            oscManager->getMQTTReceiver()->setJsonFieldNames (kx, ky, kz, kq);
    }

    /** Populate JSON field comboboxes with discovered keys from MQTT receiver. */
    void populateMqttJsonComboboxes (const juce::StringArray& keys)
    {
        auto populateCombo = [&keys] (juce::ComboBox& combo) {
            juce::String current = combo.getText();
            combo.clear (juce::dontSendNotification);
            for (int i = 0; i < keys.size(); ++i)
                combo.addItem (keys[i], i + 1);
            combo.setText (current, juce::dontSendNotification);
        };

        populateCombo (trackingMqttJsonXCombo);
        populateCombo (trackingMqttJsonYCombo);
        populateCombo (trackingMqttJsonZCombo);
        populateCombo (trackingMqttJsonQCombo);
    }

    void showMqttTagIdPanel()
    {
        // Build content panel with OK button
        // deleteAllChildren() ensures no leaked objects on dialog close
        struct TagIdPanel : public juce::Component
        {
            juce::OwnedArray<juce::Label> labels;
            juce::OwnedArray<juce::TextEditor> editors;
            juce::TextButton okButton { "OK" };

            TagIdPanel (int numSlots)
            {
                const int editorH = 21;
                const int panelH = numSlots * editorH + 45;
                setSize (300, panelH);

                for (int i = 0; i < numSlots; ++i)
                {
                    auto* label = labels.add (new juce::Label());
                    label->setText (juce::String (i + 1) + ":", juce::dontSendNotification);
                    label->setBounds (10, 5 + i * editorH, 30, 20);
                    addAndMakeVisible (label);

                    auto* editor = editors.add (new juce::TextEditor());
                    editor->setInputRestrictions (8, "0123456789abcdefABCDEF");
                    editor->setBounds (42, 5 + i * editorH, 240, 20);
                    addAndMakeVisible (editor);
                }

                okButton.setBounds (110, panelH - 35, 80, 28);
                addAndMakeVisible (okButton);
            }
        };

        auto* panel = new TagIdPanel (32);

        // Load current values
        for (int i = 0; i < 32; ++i)
        {
            if (oscManager != nullptr && oscManager->getMQTTReceiver() != nullptr)
                panel->editors[i]->setText (oscManager->getMQTTReceiver()->getTagId (i), juce::dontSendNotification);
        }

        // Keep a raw pointer to read editors from callback (panel is owned by dialog)
        auto* panelPtr = panel;

        juce::DialogWindow::LaunchOptions options;
        options.content.setOwned (panel);
        options.dialogTitle = "MQTT Tag ID Mapping";
        options.dialogBackgroundColour = getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId);
        options.escapeKeyTriggersCloseButton = true;
        options.useNativeTitleBar = false;
        options.resizable = false;

        auto* dialog = options.launchAsync();

        if (dialog != nullptr)
        {
            // OK button closes the dialog
            panelPtr->okButton.onClick = [dialog]() { dialog->exitModalState (1); };

            // Save tag IDs when dialog closes (callback fires before dialog is destroyed)
            juce::ModalComponentManager::getInstance()->attachCallback (
                dialog,
                juce::ModalCallbackFunction::create ([this, panelPtr] (int)
                {
                    if (oscManager == nullptr)
                        return;

                    auto* receiver = oscManager->getMQTTReceiver();
                    juce::StringArray ids;

                    for (int i = 0; i < 32; ++i)
                    {
                        juce::String tagId = panelPtr->editors[i]->getText().trim().toUpperCase();
                        if (receiver != nullptr)
                            receiver->setTagId (i, tagId);
                        ids.add (tagId);
                    }

                    parameters.setConfigParam ("trackingMqttTagIds", ids.joinIntoString (","));
                }));
        }
    }

    void setupNumericEditors()
    {
        // Port editors - integers only
        udpPortEditor.setInputRestrictions(5, "0123456789");
        tcpPortEditor.setInputRestrictions(5, "0123456789");
        trackingPortEditor.setInputRestrictions(5, "0123456789");

        // ADM-OSC Polar editors
        // Tracking editors - floats (offset: -50 to 50, scale: 0.01 to 100)
        trackingOffsetXEditor.setInputRestrictions(8, "-0123456789.");
        trackingOffsetYEditor.setInputRestrictions(8, "-0123456789.");
        trackingOffsetZEditor.setInputRestrictions(8, "-0123456789.");
        trackingScaleXEditor.setInputRestrictions(8, "0123456789.");
        trackingScaleYEditor.setInputRestrictions(8, "0123456789.");
        trackingScaleZEditor.setInputRestrictions(8, "0123456789.");

        trackingPortEditor.setText("5000", false);
        trackingOffsetXEditor.setText("0.0", false);
        trackingOffsetYEditor.setText("0.0", false);
        trackingOffsetZEditor.setText("0.0", false);
        trackingScaleXEditor.setText("1.0", false);
        trackingScaleYEditor.setText("1.0", false);
        trackingScaleZEditor.setText("1.0", false);
    }

    void loadParametersFromValueTree()
    {
        int udpPort = (int)parameters.getConfigParam(WFSParameterIDs::networkRxUDPport.toString());
        int tcpPort = (int)parameters.getConfigParam(WFSParameterIDs::networkRxTCPport.toString());
        if (udpPort <= 0) udpPort = 8000;  // Default
        if (tcpPort <= 0) tcpPort = 8001;  // Default
        udpPortEditor.setText(juce::String(udpPort), false);
        tcpPortEditor.setText(juce::String(tcpPort), false);

        // Load saved network interface
        juce::String savedInterface = parameters.getConfigParam("NetworkInterface").toString();
        if (savedInterface.isNotEmpty())
        {
            int index = interfaceNames.indexOf(savedInterface);
            if (index >= 0)
                networkInterfaceSelector.setSelectedId(index + 1, juce::dontSendNotification);
        }

        // Load network targets
        loadTargetsFromValueTree();

        // Load OSC Source Filter setting
        bool filterEnabled = (int)parameters.getConfigParam("networkOscSourceFilter") != 0;
        oscSourceFilterButton.setToggleState(filterEnabled, juce::dontSendNotification);
        oscSourceFilterButton.setButtonText(filterEnabled ? LOC("network.toggles.oscFilterRegisteredOnly") : LOC("network.toggles.oscFilterAcceptAll"));

        // Load ADM-OSC mapping parameters
        loadAdmMappingToUI();
        updateAdmMappingVisibility();

        // Load Tracking parameters
        bool trackingEnabled = (int)parameters.getConfigParam("trackingEnabled") != 0;
        trackingEnabledButton.setToggleState(trackingEnabled, juce::dontSendNotification);
        trackingEnabledButton.setButtonText(trackingEnabled ? LOC("network.toggles.trackingOn") : LOC("network.toggles.trackingOff"));

        trackingProtocolSelector.setSelectedId((int)parameters.getConfigParam("trackingProtocol") + 1, juce::dontSendNotification);
        trackingPortEditor.setText(juce::String((int)parameters.getConfigParam("trackingPort")), false);

        trackingOffsetXEditor.setText(juce::String((float)parameters.getConfigParam("trackingOffsetX")), false);
        trackingOffsetYEditor.setText(juce::String((float)parameters.getConfigParam("trackingOffsetY")), false);
        trackingOffsetZEditor.setText(juce::String((float)parameters.getConfigParam("trackingOffsetZ")), false);
        trackingScaleXEditor.setText(juce::String((float)parameters.getConfigParam("trackingScaleX")), false);
        trackingScaleYEditor.setText(juce::String((float)parameters.getConfigParam("trackingScaleY")), false);
        trackingScaleZEditor.setText(juce::String((float)parameters.getConfigParam("trackingScaleZ")), false);

        bool trackFlipX = (int)parameters.getConfigParam("trackingFlipX") != 0;
        bool trackFlipY = (int)parameters.getConfigParam("trackingFlipY") != 0;
        bool trackFlipZ = (int)parameters.getConfigParam("trackingFlipZ") != 0;
        trackingFlipXButton.setToggleState(trackFlipX, juce::dontSendNotification);
        trackingFlipXButton.setButtonText(trackFlipX ? LOC("network.toggles.flipXOn") : LOC("network.toggles.flipXOff"));
        trackingFlipYButton.setToggleState(trackFlipY, juce::dontSendNotification);
        trackingFlipYButton.setButtonText(trackFlipY ? LOC("network.toggles.flipYOn") : LOC("network.toggles.flipYOff"));
        trackingFlipZButton.setToggleState(trackFlipZ, juce::dontSendNotification);
        trackingFlipZButton.setButtonText(trackFlipZ ? LOC("network.toggles.flipZOn") : LOC("network.toggles.flipZOff"));

        // Load Tracking OSC Path
        juce::String oscPath = parameters.getConfigParam("trackingOscPath").toString();
        if (oscPath.isEmpty())
            oscPath = "/wfs/tracking <ID> <x> <y> <z>";  // Default
        trackingOscPathEditor.setText(oscPath, false);

        // Load MQTT settings
        juce::String mqttHost = parameters.getConfigParam("trackingMqttHost").toString();
        if (mqttHost.isEmpty()) mqttHost = "192.168.1.1";
        trackingMqttHostEditor.setText(mqttHost, false);

        juce::String mqttTopic = parameters.getConfigParam("trackingMqttTopic").toString();
        if (mqttTopic.isEmpty()) mqttTopic = "dwm/node/+/uplink/location";
        trackingMqttTopicEditor.setText(mqttTopic, false);

        juce::String mqttJx = parameters.getConfigParam("trackingMqttJsonX").toString();
        juce::String mqttJy = parameters.getConfigParam("trackingMqttJsonY").toString();
        juce::String mqttJz = parameters.getConfigParam("trackingMqttJsonZ").toString();
        juce::String mqttJq = parameters.getConfigParam("trackingMqttJsonQ").toString();
        trackingMqttJsonXCombo.setText(mqttJx.isNotEmpty() ? mqttJx : "x", juce::dontSendNotification);
        trackingMqttJsonYCombo.setText(mqttJy.isNotEmpty() ? mqttJy : "y", juce::dontSendNotification);
        trackingMqttJsonZCombo.setText(mqttJz.isNotEmpty() ? mqttJz : "z", juce::dontSendNotification);
        trackingMqttJsonQCombo.setText(mqttJq.isNotEmpty() ? mqttJq : "quality", juce::dontSendNotification);

        updateTrackingAppearance();

        // Load OSC Query parameters
        int oscQueryPort = (int)parameters.getConfigParam(WFSParameterIDs::networkOscQueryPort.toString());
        if (oscQueryPort <= 0) oscQueryPort = 5005;  // Default port
        oscQueryPortEditor.setText(juce::String(oscQueryPort), false);

        bool oscQueryEnabled = (int)parameters.getConfigParam(WFSParameterIDs::networkOscQueryEnabled.toString()) != 0;
        oscQueryEnableButton.setToggleState(oscQueryEnabled, juce::dontSendNotification);
        oscQueryEnableButton.setButtonText(oscQueryEnabled ? LOC("network.toggles.enabled") : LOC("network.toggles.disabled"));

        // Start OSC Query server if enabled
        updateOSCQueryServer();
    }

    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override
    {
        if (tree == parameters.getConfigTree())
        {
            if (property == WFSParameterIDs::networkRxUDPport)
                udpPortEditor.setText(juce::String((int)parameters.getConfigParam(WFSParameterIDs::networkRxUDPport.toString())), false);
            else if (property == WFSParameterIDs::networkRxTCPport)
                tcpPortEditor.setText(juce::String((int)parameters.getConfigParam(WFSParameterIDs::networkRxTCPport.toString())), false);
        }
    }

    void valueTreeChildAdded(juce::ValueTree&, juce::ValueTree&) override {}
    void valueTreeChildRemoved(juce::ValueTree&, juce::ValueTree&, int) override {}
    void valueTreeChildOrderChanged(juce::ValueTree&, int, int) override {}
    void valueTreeParentChanged(juce::ValueTree&) override {}

    void textEditorTextChanged(juce::TextEditor& editor) override
    {
        updateParameterFromEditor(&editor);
    }

    void textEditorReturnKeyPressed(juce::TextEditor&) override {}
    void textEditorEscapeKeyPressed(juce::TextEditor&) override {}
    void textEditorFocusLost(juce::TextEditor&) override {}

    void mouseEnter(const juce::MouseEvent& e) override
    {
        if (statusBar == nullptr) return;

        auto* source = e.eventComponent;
        juce::String helpText;

        // ==================== NETWORK SECTION ====================
        if (source == &networkInterfaceLabel || isOrIsChildOf(source, &networkInterfaceSelector))
            helpText = LOC("network.help.networkInterface");
        else if (source == &currentIPLabel || source == &currentIPEditor)
            helpText = LOC("network.help.currentIP");
        else if (source == &udpPortLabel || source == &udpPortEditor)
            helpText = LOC("network.help.udpPort");
        else if (source == &tcpPortLabel || source == &tcpPortEditor)
            helpText = LOC("network.help.tcpPort");
        else if (source == &oscQueryLabel || source == &oscQueryPortEditor)
            helpText = LOC("network.help.oscQueryPort");
        else if (source == &oscQueryEnableButton)
            helpText = LOC("network.help.oscQueryEnable");

        // ==================== NETWORK CONNECTIONS TABLE ====================
        else if (source == &headerNameLabel)
            helpText = LOC("network.help.targetName");
        else if (source == &headerDataModeLabel)
            helpText = LOC("network.help.dataMode");
        else if (source == &headerIpLabel)
            helpText = LOC("network.help.targetIP");
        else if (source == &headerTxPortLabel)
            helpText = LOC("network.help.txPort");
        else if (source == &headerRxEnableLabel)
            helpText = LOC("network.help.rxEnable");
        else if (source == &headerTxEnableLabel)
            helpText = LOC("network.help.txEnable");
        else if (source == &headerProtocolLabel)
            helpText = LOC("network.help.protocol");
        else if (source == &addTargetButton)
            helpText = LOC("network.help.addTarget");
        else if (source == &openLogWindowButton)
            helpText = LOC("network.help.openLogWindow");
        else if (source == &findMyRemoteButton)
            helpText = LOC("network.help.findMyRemote");
        else if (source == &oscSourceFilterButton)
            helpText = LOC("network.help.oscSourceFilter");

        // ==================== ADM-OSC SECTION ====================
        else if (source == &admMappingSelectorLabel || isOrIsChildOf(source, &admMappingSelector))
            helpText = LOC("network.help.admMapping");
        else if (source == &admMappingGraph || source == &admPolarGraph)
            helpText = LOC("network.help.admMappingPanel");
        // ==================== TRACKING SECTION ====================
        if (helpText.isEmpty() && source == &trackingEnabledButton)
            helpText = LOC("network.help.trackingEnabled");
        else if (source == &trackingProtocolLabel || isOrIsChildOf(source, &trackingProtocolSelector))
            helpText = LOC("network.help.trackingProtocol");
        else if (source == &trackingPortLabel || source == &trackingPortEditor)
            helpText = LOC("network.help.trackingPort");
        else if (source == &trackingOffsetXLabel || source == &trackingOffsetXEditor)
            helpText = LOC("network.help.trackingOffsetX");
        else if (source == &trackingOffsetYLabel || source == &trackingOffsetYEditor)
            helpText = LOC("network.help.trackingOffsetY");
        else if (source == &trackingOffsetZLabel || source == &trackingOffsetZEditor)
            helpText = LOC("network.help.trackingOffsetZ");
        else if (source == &trackingScaleXLabel || source == &trackingScaleXEditor)
            helpText = LOC("network.help.trackingScaleX");
        else if (source == &trackingScaleYLabel || source == &trackingScaleYEditor)
            helpText = LOC("network.help.trackingScaleY");
        else if (source == &trackingScaleZLabel || source == &trackingScaleZEditor)
            helpText = LOC("network.help.trackingScaleZ");
        else if (source == &trackingFlipXButton)
            helpText = LOC("network.help.trackingFlipX");
        else if (source == &trackingFlipYButton)
            helpText = LOC("network.help.trackingFlipY");
        else if (source == &trackingFlipZButton)
            helpText = LOC("network.help.trackingFlipZ");
        else if (source == &trackingOscPathLabel || source == &trackingOscPathEditor)
            helpText = LOC("network.help.trackingOscPath");
        else if (source == &trackingPsnInterfaceLabel || isOrIsChildOf(source, &trackingPsnInterfaceSelector))
            helpText = LOC("network.help.psnInterface");

        // ==================== FOOTER BUTTONS ====================
        else if (source == &storeButton)
            helpText = LOC("network.help.storeConfig");
        else if (source == &reloadButton)
            helpText = LOC("network.help.reloadConfig");
        else if (source == &reloadBackupButton)
            helpText = LOC("network.help.reloadBackup");
        else if (source == &importButton)
            helpText = LOC("network.help.importConfig");
        else if (source == &exportButton)
            helpText = LOC("network.help.exportConfig");

        // ==================== TARGET ROW COMPONENTS ====================
        else
        {
            for (int i = 0; i < maxTargets; ++i)
            {
                auto& row = targetRows[i];
                if (source == &row.nameEditor)
                    helpText = LOC("network.help.targetName");
                else if (isOrIsChildOf(source, &row.dataModeSelector))
                    helpText = LOC("network.help.dataMode");
                else if (source == &row.ipEditor)
                    helpText = LOC("network.help.targetIP");
                else if (source == &row.txPortEditor)
                    helpText = LOC("network.help.txPort");
                else if (source == &row.rxEnableButton)
                    helpText = LOC("network.help.rxEnable");
                else if (source == &row.txEnableButton)
                    helpText = LOC("network.help.txEnable");
                else if (isOrIsChildOf(source, &row.protocolSelector))
                    helpText = LOC("network.help.protocol");
                else if (source == &row.removeButton)
                    helpText = LOC("network.help.removeTarget");

                if (helpText.isNotEmpty())
                    break;
            }
        }

        // Update status bar and TTS
        if (helpText.isNotEmpty())
        {
            statusBar->setHelpText(helpText);
            // TTS: Announce parameter name and current value for accessibility
            juce::String paramName = TTSManager::extractParameterName(helpText);
            juce::String currentValue = TTSManager::getComponentValue(source);
            TTSManager::getInstance().onComponentEnter(paramName, currentValue, helpText);
        }
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        if (statusBar != nullptr)
            statusBar->clearText();

        // TTS: Cancel any pending announcements
        TTSManager::getInstance().onComponentExit();
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        // Refresh network interfaces when clicking on the dropdown
        // This allows detecting newly connected adapters
        if (e.eventComponent == &networkInterfaceSelector)
        {
            refreshNetworkInterfacesBeforePopup();
        }
        // Also refresh PSN interface list when clicking its dropdown
        else if (isOrIsChildOf(e.eventComponent, &trackingPsnInterfaceSelector))
        {
            refreshPsnInterfacesBeforePopup();
        }
    }

    void updateParameterFromEditor(juce::TextEditor* editor)
    {
        juce::String text = editor->getText();

        if (editor == &udpPortEditor)
        {
            int value = text.getIntValue();
            if (value >= 0 && value <= 65535)
                parameters.setConfigParam(WFSParameterIDs::networkRxUDPport.toString(), value);
        }
        else if (editor == &tcpPortEditor)
        {
            int value = text.getIntValue();
            if (value >= 0 && value <= 65535)
                parameters.setConfigParam(WFSParameterIDs::networkRxTCPport.toString(), value);
        }
        // Tracking Port
        else if (editor == &trackingPortEditor)
        {
            int value = text.getIntValue();
            if (value >= 0 && value <= 65535)
            {
                parameters.setConfigParam("trackingPort", value);
                updateTrackingReceiver(true);  // Force restart receiver with new port
            }
        }
        // Tracking Offset parameters
        else if (editor == &trackingOffsetXEditor)
        {
            float value = juce::jlimit(-50.0f, 50.0f, text.getFloatValue());
            parameters.setConfigParam("trackingOffsetX", value);
            updateTrackingTransformations();
        }
        else if (editor == &trackingOffsetYEditor)
        {
            float value = juce::jlimit(-50.0f, 50.0f, text.getFloatValue());
            parameters.setConfigParam("trackingOffsetY", value);
            updateTrackingTransformations();
        }
        else if (editor == &trackingOffsetZEditor)
        {
            float value = juce::jlimit(-50.0f, 50.0f, text.getFloatValue());
            parameters.setConfigParam("trackingOffsetZ", value);
            updateTrackingTransformations();
        }
        // Tracking Scale parameters
        else if (editor == &trackingScaleXEditor)
        {
            float value = juce::jlimit(0.01f, 100.0f, text.getFloatValue());
            parameters.setConfigParam("trackingScaleX", value);
            updateTrackingTransformations();
        }
        else if (editor == &trackingScaleYEditor)
        {
            float value = juce::jlimit(0.01f, 100.0f, text.getFloatValue());
            parameters.setConfigParam("trackingScaleY", value);
            updateTrackingTransformations();
        }
        else if (editor == &trackingScaleZEditor)
        {
            float value = juce::jlimit(0.01f, 100.0f, text.getFloatValue());
            parameters.setConfigParam("trackingScaleZ", value);
            updateTrackingTransformations();
        }
        // OSC Query Port
        else if (editor == &oscQueryPortEditor)
        {
            int value = text.getIntValue();
            if (value >= 0 && value <= 65535)
            {
                saveOscQueryToValueTree();
            }
        }
        // Tracking OSC Path
        else if (editor == &trackingOscPathEditor)
        {
            juce::String path = text.trim();
            if (isValidOscPath(path))
            {
                parameters.setConfigParam("trackingOscPath", path);
                editor->setColour(juce::TextEditor::outlineColourId, ColorScheme::get().buttonBorder);
                updateTrackingReceiver();  // Update receiver with new path
            }
            else
            {
                // Invalid path - show red outline
                editor->setColour(juce::TextEditor::outlineColourId, juce::Colours::red);
            }
        }
        // MQTT editors
        else if (editor == &trackingMqttHostEditor || editor == &trackingMqttTopicEditor)
        {
            if (editor == &trackingMqttHostEditor)
                parameters.setConfigParam("trackingMqttHost", text.trim());
            else
                parameters.setConfigParam("trackingMqttTopic", text.trim());
            updateTrackingReceiver(true);  // Force restart with new settings
        }
    }

    /** Validate OSC path format - must start with / and contain valid characters */
    bool isValidOscPath(const juce::String& path)
    {
        if (path.isEmpty() || !path.startsWith("/"))
            return false;

        // OSC path can contain alphanumeric, /, _, -, <, >, and space (for placeholders)
        for (int i = 0; i < path.length(); ++i)
        {
            juce::juce_wchar c = path[i];
            if (!juce::CharacterFunctions::isLetterOrDigit(c) &&
                c != '/' && c != '_' && c != '-' && c != '<' && c != '>' && c != ' ')
                return false;
        }
        return true;
    }

    void onNetworkInterfaceChanged()
    {
        int selectedId = networkInterfaceSelector.getSelectedId();
        if (selectedId > 0)
        {
            int index = selectedId - 1;
            if (index < interfaceNames.size())
            {
                // Save selected interface name to parameters
                parameters.setConfigParam("NetworkInterface", interfaceNames[index]);

                // Update current IP display
                if (index < interfaceIPs.size())
                    currentIPEditor.setText(interfaceIPs[index], false);
            }
        }
    }

    void refreshNetworkInterfacesBeforePopup()
    {
        // Remember current selection
        juce::String previousSelection;
        int selectedId = networkInterfaceSelector.getSelectedId();
        if (selectedId > 0)
        {
            int index = selectedId - 1;
            if (index < interfaceNames.size())
                previousSelection = interfaceNames[index];
        }

        // Refresh the interface list
        populateNetworkInterfaces();

        // Try to re-select the previously selected interface
        if (previousSelection.isNotEmpty())
        {
            int newIndex = interfaceNames.indexOf(previousSelection);
            if (newIndex >= 0)
                networkInterfaceSelector.setSelectedId(newIndex + 1, juce::dontSendNotification);
        }
    }

    void populateNetworkInterfaces()
    {
        networkInterfaceSelector.clear();
        interfaceNames.clear();
        interfaceIPs.clear();

#if JUCE_WINDOWS
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0)
        {
            PIP_ADAPTER_ADDRESSES pAddresses = nullptr;
            ULONG outBufLen = 15000;
            ULONG family = AF_INET;  // IPv4

            for (int attempts = 0; attempts < 3; attempts++)
            {
                pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
                if (pAddresses == nullptr)
                    break;

                DWORD result = GetAdaptersAddresses(family, GAA_FLAG_INCLUDE_PREFIX, nullptr, pAddresses, &outBufLen);

                if (result == ERROR_BUFFER_OVERFLOW)
                {
                    free(pAddresses);
                    pAddresses = nullptr;
                    continue;
                }

                if (result == NO_ERROR)
                {
                    PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses;
                    int interfaceIndex = 1;

                    while (pCurrAddresses)
                    {
                        // Skip loopback interfaces
                        if (pCurrAddresses->OperStatus == IfOperStatusUp &&
                            pCurrAddresses->IfType != IF_TYPE_SOFTWARE_LOOPBACK)
                        {
                            PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurrAddresses->FirstUnicastAddress;
                            while (pUnicast)
                            {
                                if (pUnicast->Address.lpSockaddr->sa_family == AF_INET)
                                {
                                    sockaddr_in* sa_in = (sockaddr_in*)pUnicast->Address.lpSockaddr;
                                    char ipBuffer[INET_ADDRSTRLEN];
                                    inet_ntop(AF_INET, &(sa_in->sin_addr), ipBuffer, INET_ADDRSTRLEN);

                                    // Convert adapter name from wide string
                                    juce::String adapterName = juce::String(pCurrAddresses->FriendlyName);
                                    juce::String ipAddress = juce::String(ipBuffer);

                                    // Add to combo box with format: "Adapter Name (IP)"
                                    juce::String displayName = adapterName + " (" + ipAddress + ")";
                                    networkInterfaceSelector.addItem(displayName, interfaceIndex++);

                                    // Store for later use
                                    interfaceNames.add(adapterName);
                                    interfaceIPs.add(ipAddress);

                                    break;  // Only take first IPv4 address per adapter
                                }
                                pUnicast = pUnicast->Next;
                            }
                        }
                        pCurrAddresses = pCurrAddresses->Next;
                    }
                }
                break;
            }

            if (pAddresses)
                free(pAddresses);
            WSACleanup();
        }

        // Always add localhost as a fallback option
        {
            int nextIndex = networkInterfaceSelector.getNumItems() + 1;
            networkInterfaceSelector.addItem(LOC("network.labels.localhost"), nextIndex);
            interfaceNames.add("Localhost");
            interfaceIPs.add("127.0.0.1");
        }
#elif JUCE_MAC
        // First, get friendly names from SystemConfiguration framework
        juce::HashMap<juce::String, juce::String> bsdToFriendlyName;
        CFArrayRef interfaces = SCNetworkInterfaceCopyAll();
        if (interfaces != nullptr)
        {
            CFIndex count = CFArrayGetCount(interfaces);
            for (CFIndex i = 0; i < count; ++i)
            {
                SCNetworkInterfaceRef interface = (SCNetworkInterfaceRef)CFArrayGetValueAtIndex(interfaces, i);
                if (interface != nullptr)
                {
                    // Get BSD name (e.g., "en0", "en1")
                    CFStringRef bsdName = SCNetworkInterfaceGetBSDName(interface);
                    // Get display/friendly name (e.g., "Ethernet", "Wi-Fi")
                    CFStringRef displayName = SCNetworkInterfaceGetLocalizedDisplayName(interface);
                    
                    if (bsdName != nullptr)
                    {
                        // Convert CFStringRef to juce::String
                        char bsdNameBuffer[256];
                        if (CFStringGetCString(bsdName, bsdNameBuffer, sizeof(bsdNameBuffer), kCFStringEncodingUTF8))
                        {
                            juce::String bsdNameStr = juce::String(bsdNameBuffer);
                            juce::String friendlyNameStr;
                            
                            if (displayName != nullptr)
                            {
                                char displayNameBuffer[256];
                                if (CFStringGetCString(displayName, displayNameBuffer, sizeof(displayNameBuffer), kCFStringEncodingUTF8))
                                {
                                    friendlyNameStr = juce::String(displayNameBuffer);
                                }
                                else
                                {
                                    // Fallback to BSD name if conversion fails
                                    friendlyNameStr = bsdNameStr;
                                }
                            }
                            else
                            {
                                // Fallback to BSD name if no display name
                                friendlyNameStr = bsdNameStr;
                            }
                            
                            bsdToFriendlyName.set(bsdNameStr, friendlyNameStr);
                        }
                    }
                }
            }
            CFRelease(interfaces);
        }
        
        // Now get actual interfaces with IP addresses using getifaddrs
        struct ifaddrs* ifaddrsList = nullptr;
        if (getifaddrs(&ifaddrsList) == 0)
        {
            // Map to track interfaces we've already added (to avoid duplicates)
            juce::HashMap<juce::String, juce::String> interfaceMap;
            
            int interfaceIndex = 1;
            for (struct ifaddrs* ifa = ifaddrsList; ifa != nullptr; ifa = ifa->ifa_next)
            {
                if (ifa->ifa_addr == nullptr)
                    continue;
                
                // Only process IPv4 addresses
                if (ifa->ifa_addr->sa_family != AF_INET)
                    continue;
                
                // Skip loopback interfaces
                if ((ifa->ifa_flags & IFF_LOOPBACK) != 0)
                    continue;
                
                // Only process interfaces that are up and running
                if ((ifa->ifa_flags & IFF_UP) == 0 || (ifa->ifa_flags & IFF_RUNNING) == 0)
                    continue;
                
                // Get interface BSD name
                juce::String bsdName = juce::String(ifa->ifa_name);
                
                // Get friendly name if available, otherwise use BSD name
                juce::String interfaceName = bsdToFriendlyName.contains(bsdName) 
                    ? bsdToFriendlyName[bsdName] 
                    : bsdName;
                
                // Get IPv4 address
                struct sockaddr_in* sa_in = (struct sockaddr_in*)ifa->ifa_addr;
                char ipBuffer[INET_ADDRSTRLEN];
                if (inet_ntop(AF_INET, &(sa_in->sin_addr), ipBuffer, INET_ADDRSTRLEN) != nullptr)
                {
                    juce::String ipAddress = juce::String(ipBuffer);
                    
                    // Check if we've already added this interface (some interfaces can have multiple addresses)
                    // We'll use the first IPv4 address we find for each interface name
                    if (!interfaceMap.contains(bsdName))
                    {
                        // Add to combo box with format: "Friendly Name (IP)"
                        juce::String displayName = interfaceName + " (" + ipAddress + ")";
                        networkInterfaceSelector.addItem(displayName, interfaceIndex++);
                        
                        // Store BSD name for later use (for matching when saving/loading)
                        interfaceNames.add(bsdName);
                        interfaceIPs.add(ipAddress);
                        
                        interfaceMap.set(bsdName, ipAddress);
                    }
                }
            }
            
            freeifaddrs(ifaddrsList);
        }
        
        // Always add localhost as a fallback option
        {
            int nextIndex = networkInterfaceSelector.getNumItems() + 1;
            networkInterfaceSelector.addItem(LOC("network.labels.localhost"), nextIndex);
            interfaceNames.add("Localhost");
            interfaceIPs.add("127.0.0.1");
        }
#else
        // Unsupported platform - just add localhost
        networkInterfaceSelector.addItem(LOC("network.labels.localhost"), 1);
        interfaceNames.add("Localhost");
        interfaceIPs.add("127.0.0.1");
#endif

        // Select first item by default if nothing saved
        if (networkInterfaceSelector.getSelectedId() == 0 && networkInterfaceSelector.getNumItems() > 0)
        {
            networkInterfaceSelector.setSelectedId(1, juce::sendNotification);
        }
    }

    /**
     * Refresh the PSN interface selector before popup.
     * Similar to refreshNetworkInterfacesBeforePopup but for the PSN multicast interface.
     */
    void refreshPsnInterfacesBeforePopup()
    {
        // Remember current selection
        juce::String previousSelection;
        int selectedId = trackingPsnInterfaceSelector.getSelectedId();
        if (selectedId > 0)
        {
            int index = selectedId - 1;
            if (index < psnInterfaceNames.size())
                previousSelection = psnInterfaceNames[index];
        }

        // Clear existing items
        trackingPsnInterfaceSelector.clear();
        psnInterfaceNames.clear();
        psnInterfaceIPs.clear();

        int interfaceIndex = 1;

#if JUCE_WINDOWS
        // Windows: Use GetAdaptersAddresses
        PIP_ADAPTER_ADDRESSES pAddresses = nullptr;
        ULONG outBufLen = 15000;
        ULONG family = AF_INET;

        for (int attempts = 0; attempts < 3; attempts++)
        {
            pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
            if (pAddresses == nullptr)
                break;

            ULONG adapterFlags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
            DWORD result = GetAdaptersAddresses(family, adapterFlags, nullptr, pAddresses, &outBufLen);

            if (result == ERROR_BUFFER_OVERFLOW)
            {
                free(pAddresses);
                pAddresses = nullptr;
                continue;
            }

            if (result == NO_ERROR)
            {
                for (PIP_ADAPTER_ADDRESSES pCurr = pAddresses; pCurr != nullptr; pCurr = pCurr->Next)
                {
                    if (pCurr->OperStatus != IfOperStatusUp)
                        continue;

                    if (pCurr->IfType == IF_TYPE_SOFTWARE_LOOPBACK)
                        continue;

                    for (PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurr->FirstUnicastAddress;
                         pUnicast != nullptr; pUnicast = pUnicast->Next)
                    {
                        if (pUnicast->Address.lpSockaddr->sa_family == AF_INET)
                        {
                            sockaddr_in* addr = (sockaddr_in*)pUnicast->Address.lpSockaddr;
                            char ipStr[INET_ADDRSTRLEN];
                            inet_ntop(AF_INET, &addr->sin_addr, ipStr, INET_ADDRSTRLEN);
                            juce::String ipAddress(ipStr);

                            // Get friendly name
                            juce::String friendlyName(pCurr->FriendlyName);
                            juce::String adapterName(pCurr->FriendlyName);

                            juce::String displayName = friendlyName + " (" + ipAddress + ")";
                            trackingPsnInterfaceSelector.addItem(displayName, interfaceIndex++);

                            psnInterfaceNames.add(adapterName);
                            psnInterfaceIPs.add(ipAddress);

                            break;  // Only take first IPv4 address per adapter
                        }
                    }
                }
            }
            break;
        }

        if (pAddresses != nullptr)
            free(pAddresses);

        // Add localhost option
        int nextIndex = interfaceIndex;
        if (nextIndex == 1 || trackingPsnInterfaceSelector.getNumItems() == 0)
        {
            trackingPsnInterfaceSelector.addItem(LOC("network.labels.localhost"), nextIndex);
            psnInterfaceNames.add("Localhost");
            psnInterfaceIPs.add("127.0.0.1");
        }

#elif JUCE_MAC
        // macOS: Use getifaddrs
        struct ifaddrs* ifAddrList = nullptr;
        if (getifaddrs(&ifAddrList) == 0)
        {
            for (struct ifaddrs* ifa = ifAddrList; ifa != nullptr; ifa = ifa->ifa_next)
            {
                if (ifa->ifa_addr == nullptr)
                    continue;

                if (ifa->ifa_addr->sa_family != AF_INET)
                    continue;

                if ((ifa->ifa_flags & IFF_UP) == 0 || (ifa->ifa_flags & IFF_RUNNING) == 0)
                    continue;

                juce::String bsdName(ifa->ifa_name);
                if (bsdName.startsWith("lo"))
                    continue;

                char ipStr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr, ipStr, INET_ADDRSTRLEN);
                juce::String ipAddress(ipStr);

                juce::String displayName = bsdName + " (" + ipAddress + ")";
                trackingPsnInterfaceSelector.addItem(displayName, interfaceIndex++);

                psnInterfaceNames.add(bsdName);
                psnInterfaceIPs.add(ipAddress);
            }
            freeifaddrs(ifAddrList);
        }

        // Add localhost if no other interfaces found
        int nextIndex = interfaceIndex;
        if (nextIndex == 1 || trackingPsnInterfaceSelector.getNumItems() == 0)
        {
            trackingPsnInterfaceSelector.addItem(LOC("network.labels.localhost"), nextIndex);
            psnInterfaceNames.add("Localhost");
            psnInterfaceIPs.add("127.0.0.1");
        }
#else
        // Unsupported platform - just add localhost
        trackingPsnInterfaceSelector.addItem(LOC("network.labels.localhost"), 1);
        psnInterfaceNames.add("Localhost");
        psnInterfaceIPs.add("127.0.0.1");
#endif

        // Restore previous selection or select first item
        if (previousSelection.isNotEmpty())
        {
            for (int i = 0; i < psnInterfaceNames.size(); ++i)
            {
                if (psnInterfaceNames[i] == previousSelection)
                {
                    trackingPsnInterfaceSelector.setSelectedId(i + 1, juce::dontSendNotification);
                    return;
                }
            }
        }

        // Select first item by default if nothing saved
        if (trackingPsnInterfaceSelector.getSelectedId() == 0 && trackingPsnInterfaceSelector.getNumItems() > 0)
        {
            trackingPsnInterfaceSelector.setSelectedId(1, juce::sendNotification);
        }
    }

    void updateCurrentIP()
    {
        // If a network interface is selected, show its IP
        int selectedId = networkInterfaceSelector.getSelectedId();
        if (selectedId > 0)
        {
            int index = selectedId - 1;
            if (index < interfaceIPs.size())
            {
                currentIPEditor.setText(interfaceIPs[index], false);
                return;
            }
        }

        // Otherwise fall back to detecting any active interface
#if JUCE_WINDOWS
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0)
        {
            PIP_ADAPTER_ADDRESSES pAddresses = nullptr;
            ULONG outBufLen = 15000;
            ULONG family = AF_INET;  // IPv4

            for (int attempts = 0; attempts < 3; attempts++)
            {
                pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
                if (pAddresses == nullptr)
                    break;

                DWORD result = GetAdaptersAddresses(family, GAA_FLAG_INCLUDE_PREFIX, nullptr, pAddresses, &outBufLen);

                if (result == ERROR_BUFFER_OVERFLOW)
                {
                    free(pAddresses);
                    pAddresses = nullptr;
                    continue;
                }

                if (result == NO_ERROR)
                {
                    PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses;
                    while (pCurrAddresses)
                    {
                        if (pCurrAddresses->OperStatus == IfOperStatusUp &&
                            pCurrAddresses->IfType != IF_TYPE_SOFTWARE_LOOPBACK)
                        {
                            PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurrAddresses->FirstUnicastAddress;
                            while (pUnicast)
                            {
                                if (pUnicast->Address.lpSockaddr->sa_family == AF_INET)
                                {
                                    sockaddr_in* sa_in = (sockaddr_in*)pUnicast->Address.lpSockaddr;
                                    char ipBuffer[INET_ADDRSTRLEN];
                                    inet_ntop(AF_INET, &(sa_in->sin_addr), ipBuffer, INET_ADDRSTRLEN);
                                    currentIPEditor.setText(juce::String(ipBuffer), false);
                                    free(pAddresses);
                                    WSACleanup();
                                    return;
                                }
                                pUnicast = pUnicast->Next;
                            }
                        }
                        pCurrAddresses = pCurrAddresses->Next;
                    }
                }
                break;
            }

            if (pAddresses)
                free(pAddresses);
            WSACleanup();
        }
#elif JUCE_MAC
        struct ifaddrs* ifaddrsList = nullptr;
        if (getifaddrs(&ifaddrsList) == 0)
        {
            for (struct ifaddrs* ifa = ifaddrsList; ifa != nullptr; ifa = ifa->ifa_next)
            {
                if (ifa->ifa_addr == nullptr)
                    continue;
                
                // Only process IPv4 addresses
                if (ifa->ifa_addr->sa_family != AF_INET)
                    continue;
                
                // Skip loopback interfaces
                if ((ifa->ifa_flags & IFF_LOOPBACK) != 0)
                    continue;
                
                // Only process interfaces that are up and running
                if ((ifa->ifa_flags & IFF_UP) != 0 && (ifa->ifa_flags & IFF_RUNNING) != 0)
                {
                    struct sockaddr_in* sa_in = (struct sockaddr_in*)ifa->ifa_addr;
                    char ipBuffer[INET_ADDRSTRLEN];
                    if (inet_ntop(AF_INET, &(sa_in->sin_addr), ipBuffer, INET_ADDRSTRLEN) != nullptr)
                    {
                        currentIPEditor.setText(juce::String(ipBuffer), false);
                        freeifaddrs(ifaddrsList);
                        return;
                    }
                }
            }
            freeifaddrs(ifaddrsList);
        }
#endif
        currentIPEditor.setText(LOC("network.labels.notAvailable"), false);
    }

    // ==================== NETWORK LOG WINDOW ====================
    void openNetworkLogWindow()
    {
        if (onNetworkLogWindowRequested)
            onNetworkLogWindowRequested();
    }

    // ==================== FIND MY REMOTE ====================
    void showFindMyRemoteDialog()
    {
        auto* alertWindow = new juce::AlertWindow(
            LOC("network.dialogs.findMyRemoteTitle"),
            LOC("network.dialogs.findMyRemoteMessage"),
            juce::AlertWindow::QuestionIcon
        );

        alertWindow->addTextEditor("password", findDevicePassword, LOC("network.dialogs.findMyRemotePassword"), true);
        alertWindow->addButton(LOC("common.ok"), 1, juce::KeyPress(juce::KeyPress::returnKey));
        alertWindow->addButton(LOC("common.cancel"), 0, juce::KeyPress(juce::KeyPress::escapeKey));

        alertWindow->enterModalState(true, juce::ModalCallbackFunction::create(
            [this, alertWindow](int result)
            {
                if (result == 1)  // OK clicked
                {
                    findDevicePassword = alertWindow->getTextEditorContents("password");
                    if (findDevicePassword.isNotEmpty())
                    {
                        sendFindDeviceOsc();
                    }
                    else
                    {
                        if (statusBar != nullptr)
                            statusBar->setHelpText(LOC("network.messages.passwordEmpty"));
                    }
                }
                delete alertWindow;
            }
        ), true);
    }

    void sendFindDeviceOsc()
    {
        if (oscManager != nullptr)
        {
            oscManager->sendFindDevice(findDevicePassword);

            if (statusBar != nullptr)
                statusBar->setHelpText(LOC("network.messages.findDeviceSent"));
        }
        else
        {
            if (statusBar != nullptr)
                statusBar->setHelpText(LOC("network.messages.oscManagerError"));
        }
    }

    // Helper to show status bar messages
    void showStatusMessage(const juce::String& message, int durationMs = 3000)
    {
        if (statusBar != nullptr)
            statusBar->showTemporaryMessage(message, durationMs);
    }

    // Store/Reload methods (saves network config separately from system config)
    void storeNetworkConfiguration()
    {
        auto& fileManager = parameters.getFileManager();

        if (!fileManager.hasValidProjectFolder())
        {
            showStatusMessage(LOC("network.messages.selectFolderFirst"));
            return;
        }

        // Backup is created automatically by file manager before overwrite
        if (fileManager.saveNetworkConfig())
            showStatusMessage(LOC("network.messages.configSaved"));
        else
            showStatusMessage(LOC("network.messages.error").replace("{error}", fileManager.getLastError()));
    }

    void reloadNetworkConfiguration()
    {
        auto& fileManager = parameters.getFileManager();

        if (!fileManager.hasValidProjectFolder())
        {
            showStatusMessage(LOC("network.messages.selectFolderFirst"));
            return;
        }

        auto configFile = fileManager.getNetworkConfigFile();
        if (!configFile.existsAsFile())
        {
            showStatusMessage(LOC("network.messages.configNotFound"));
            return;
        }

        if (fileManager.loadNetworkConfig())
        {
            loadParametersFromValueTree();
            updateOSCManagerConfig();
            showStatusMessage(LOC("network.messages.configReloaded"));
        }
        else
            showStatusMessage(LOC("network.messages.error").replace("{error}", fileManager.getLastError()));
    }

    void reloadNetworkConfigBackup()
    {
        auto& fileManager = parameters.getFileManager();

        if (!fileManager.hasValidProjectFolder())
        {
            showStatusMessage(LOC("network.messages.selectFolderFirst"));
            return;
        }

        auto backups = fileManager.getBackups("network");
        if (backups.isEmpty())
        {
            showStatusMessage(LOC("network.messages.noBackupFound"));
            return;
        }

        if (fileManager.loadNetworkConfigBackup(0))
        {
            loadParametersFromValueTree();
            updateOSCManagerConfig();
            showStatusMessage(LOC("network.messages.configLoadedFromBackup"));
        }
        else
            showStatusMessage(LOC("network.messages.error").replace("{error}", fileManager.getLastError()));
    }

    void importNetworkConfiguration()
    {
        auto chooser = std::make_shared<juce::FileChooser>(LOC("network.dialogs.importConfig"),
            AppSettings::getLastFolder("lastXmlFolder"),
            "*.xml");
        auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

        chooser->launchAsync(chooserFlags, [this, chooser](const juce::FileChooser& fc)
        {
            auto result = fc.getResult();
            if (result.existsAsFile())
            {
                AppSettings::setLastFolder("lastXmlFolder", result.getParentDirectory());
                auto& fileManager = parameters.getFileManager();
                if (fileManager.importNetworkConfig(result))
                {
                    loadParametersFromValueTree();
                    updateOSCManagerConfig();
                    showStatusMessage(LOC("network.messages.configImported"));
                }
                else
                    showStatusMessage(LOC("network.messages.error").replace("{error}", fileManager.getLastError()));
            }
        });
    }

    void exportNetworkConfiguration()
    {
        auto chooser = std::make_shared<juce::FileChooser>(LOC("network.dialogs.exportConfig"),
            AppSettings::getLastFolder("lastXmlFolder"),
            "*.xml");
        auto chooserFlags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles;

        chooser->launchAsync(chooserFlags, [this, chooser](const juce::FileChooser& fc)
        {
            auto result = fc.getResult();
            if (result != juce::File())
            {
                if (!result.hasFileExtension(".xml"))
                    result = result.withFileExtension(".xml");

                AppSettings::setLastFolder("lastXmlFolder", result.getParentDirectory());
                auto& fileManager = parameters.getFileManager();
                if (fileManager.exportNetworkConfig(result))
                    showStatusMessage(LOC("network.messages.configExported"));
                else
                    showStatusMessage(LOC("network.messages.error").replace("{error}", fileManager.getLastError()));
            }
        });
    }

    // ==================== OSC SOURCE FILTER VALUETREE METHODS ====================

    void saveOscSourceFilterToValueTree()
    {
        parameters.setConfigParam("networkOscSourceFilter", oscSourceFilterButton.getToggleState() ? 1 : 0);
    }

    // ==================== TARGET VALUETREE METHODS ====================

    void saveTargetToValueTree(int index)
    {
        if (index < 0 || index >= maxTargets)
            return;

        auto& row = targetRows[index];
        if (!row.isActive)
            return;  // Don't save inactive targets

        // Get the Network section from Config
        auto config = parameters.getConfigTree();
        if (!config.isValid())
            return;

        auto network = config.getChildWithName(WFSParameterIDs::Network);
        if (!network.isValid())
            return;

        // Find or create target child
        juce::ValueTree target;
        for (int i = 0; i < network.getNumChildren(); ++i)
        {
            auto child = network.getChild(i);
            if (child.getType() == WFSParameterIDs::NetworkTarget)
            {
                int targetId = child.getProperty(WFSParameterIDs::id, -1);
                if (targetId == index)
                {
                    target = child;
                    break;
                }
            }
        }

        // Create new target if not found
        if (!target.isValid())
        {
            target = juce::ValueTree(WFSParameterIDs::NetworkTarget);
            target.setProperty(WFSParameterIDs::id, index, nullptr);
            network.appendChild(target, nullptr);
        }

        // Save all properties
        target.setProperty(WFSParameterIDs::networkTSname, row.nameEditor.getText(), nullptr);
        target.setProperty(WFSParameterIDs::networkTSdataMode, row.dataModeSelector.getSelectedId() - 1, nullptr);
        target.setProperty(WFSParameterIDs::networkTSip, row.ipEditor.getText(), nullptr);
        target.setProperty(WFSParameterIDs::networkTSport, row.txPortEditor.getText().getIntValue(), nullptr);
        target.setProperty(WFSParameterIDs::networkTSrxEnable, row.rxEnableButton.getToggleState() ? 1 : 0, nullptr);
        target.setProperty(WFSParameterIDs::networkTStxEnable, row.txEnableButton.getToggleState() ? 1 : 0, nullptr);
        target.setProperty(WFSParameterIDs::networkTSProtocol, row.protocolSelector.getSelectedId() - 1, nullptr);
        target.setProperty(WFSParameterIDs::networkTSqlabPatch, row.qlabPatchEditor.getText().getIntValue(), nullptr);

        // Update OSCManager with new configuration
        updateOSCManagerConfig();
    }

    void removeTargetFromValueTree(int index)
    {
        if (index < 0 || index >= maxTargets)
            return;

        auto config = parameters.getConfigTree();
        if (!config.isValid())
            return;

        auto network = config.getChildWithName(WFSParameterIDs::Network);
        if (!network.isValid())
            return;

        // Find and remove target with matching id
        for (int i = network.getNumChildren() - 1; i >= 0; --i)
        {
            auto child = network.getChild(i);
            if (child.getType() == WFSParameterIDs::NetworkTarget)
            {
                int targetId = child.getProperty(WFSParameterIDs::id, -1);
                if (targetId == index)
                {
                    network.removeChild(i, nullptr);
                    break;
                }
            }
        }
    }

    // ==================== OSC QUERY ====================

    void saveOscQueryToValueTree()
    {
        int port = oscQueryPortEditor.getText().getIntValue();
        if (port < 0 || port > 65535)
            port = 5005;

        parameters.setConfigParam(WFSParameterIDs::networkOscQueryPort.toString(), port);
        parameters.setConfigParam(WFSParameterIDs::networkOscQueryEnabled.toString(),
                                  oscQueryEnableButton.getToggleState() ? 1 : 0);
    }

    void updateOSCQueryServer()
    {
        if (oscManager == nullptr)
            return;

        bool enabled = oscQueryEnableButton.getToggleState();
        int httpPort = oscQueryPortEditor.getText().getIntValue();
        if (httpPort <= 0 || httpPort > 65535)
            httpPort = 5005;

        if (enabled)
        {
            // Get UDP port for OSC from current config
            int oscPort = udpPortEditor.getText().getIntValue();
            if (oscPort <= 0)
                oscPort = 9001;

            if (!oscManager->isOSCQueryRunning())
            {
                oscManager->startOSCQuery(oscPort, httpPort);
            }
        }
        else
        {
            if (oscManager->isOSCQueryRunning())
            {
                oscManager->stopOSCQuery();
            }
        }
    }

    void loadTargetsFromValueTree()
    {
        // Reset all rows first
        for (int i = 0; i < maxTargets; ++i)
        {
            auto& row = targetRows[i];
            row.nameEditor.setText(LOC("network.table.defaultTarget").replace("{num}", juce::String(i + 1)), false);
            row.dataModeSelector.setSelectedId(1, juce::dontSendNotification);
            row.ipEditor.setText("127.0.0.1", false);
            row.txPortEditor.setText("9000", false);
            row.rxEnableButton.setToggleState(false, juce::dontSendNotification);
            row.rxEnableButton.setButtonText(LOC("network.toggles.off"));
            row.txEnableButton.setToggleState(false, juce::dontSendNotification);
            row.txEnableButton.setButtonText(LOC("network.toggles.off"));
            row.protocolSelector.setSelectedId(1, juce::dontSendNotification);
            row.isActive = false;
        }
        activeTargetCount = 0;

        // Get the Network section from Config
        auto config = parameters.getConfigTree();
        if (!config.isValid())
        {
            updateTargetRowVisibility();
            updateAddButtonState();
            return;
        }

        auto network = config.getChildWithName(WFSParameterIDs::Network);
        if (!network.isValid())
        {
            updateTargetRowVisibility();
            updateAddButtonState();
            return;
        }

        // Load all targets
        for (int i = 0; i < network.getNumChildren(); ++i)
        {
            auto child = network.getChild(i);
            if (child.getType() == WFSParameterIDs::NetworkTarget)
            {
                int targetId = child.getProperty(WFSParameterIDs::id, 0);
                if (targetId >= 0 && targetId < maxTargets)
                {
                    auto& row = targetRows[targetId];
                    row.isActive = true;
                    activeTargetCount++;

                    row.nameEditor.setText(child.getProperty(WFSParameterIDs::networkTSname, "Target " + juce::String(targetId + 1)).toString(), false);
                    row.dataModeSelector.setSelectedId((int)child.getProperty(WFSParameterIDs::networkTSdataMode, 0) + 1, juce::dontSendNotification);
                    row.ipEditor.setText(child.getProperty(WFSParameterIDs::networkTSip, "127.0.0.1").toString(), false);
                    row.txPortEditor.setText(juce::String((int)child.getProperty(WFSParameterIDs::networkTSport, 9000)), false);

                    bool rxEnabled = (int)child.getProperty(WFSParameterIDs::networkTSrxEnable, 0) != 0;
                    row.rxEnableButton.setToggleState(rxEnabled, juce::dontSendNotification);
                    row.rxEnableButton.setButtonText(rxEnabled ? "ON" : "OFF");

                    bool txEnabled = (int)child.getProperty(WFSParameterIDs::networkTStxEnable, 0) != 0;
                    row.txEnableButton.setToggleState(txEnabled, juce::dontSendNotification);
                    row.txEnableButton.setButtonText(txEnabled ? "ON" : "OFF");

                    row.protocolSelector.setSelectedId((int)child.getProperty(WFSParameterIDs::networkTSProtocol, 0) + 1, juce::dontSendNotification);

                    int qlabPatch = (int)child.getProperty(WFSParameterIDs::networkTSqlabPatch, 1);
                    row.qlabPatchEditor.setText(juce::String(juce::jlimit(1, 16, qlabPatch)), false);
                }
            }
        }

        updateTargetRowVisibility();
        updateAddButtonState();
        updateAdmOscAppearance();
        updateQLabAppearance();
    }

    /**
     * Check for cluster conflicts when enabling global tracking.
     * If any cluster has more than one input with local tracking enabled,
     * show a warning and keep only the first one, disabling the others.
     * @param fromProtocolChange true if called from protocol selector change
     */
    void checkGlobalTrackingConstraintAsync(bool fromProtocolChange = false)
    {
        int protocolEnabled = fromProtocolChange ?
            (trackingProtocolSelector.getSelectedId() - 1) :
            static_cast<int>(parameters.getConfigParam("trackingProtocol"));

        // If protocol is disabled, no conflict possible
        if (protocolEnabled == 0 && !fromProtocolChange)
        {
            trackingEnabledButton.setButtonText(LOC("network.toggles.trackingOn"));
            parameters.setConfigParam("trackingEnabled", 1);
            updateTrackingAppearance();
            return;
        }

        // Find clusters with multiple locally-tracked inputs
        int numInputs = parameters.getNumInputChannels();
        std::map<int, std::vector<int>> clusterTrackedInputs;  // cluster -> list of inputs with local tracking

        for (int i = 0; i < numInputs; ++i)
        {
            int cluster = static_cast<int>(parameters.getInputParam(i, "inputCluster"));
            if (cluster > 0)  // Not "Single"
            {
                int localTracking = static_cast<int>(parameters.getInputParam(i, "inputTrackingActive"));
                if (localTracking != 0)
                {
                    clusterTrackedInputs[cluster].push_back(i);
                }
            }
        }

        // Find conflicts (clusters with more than one tracked input)
        std::vector<std::pair<int, std::vector<int>>> conflicts;
        for (const auto& [cluster, inputs] : clusterTrackedInputs)
        {
            if (inputs.size() > 1)
            {
                conflicts.push_back({cluster, inputs});
            }
        }

        if (conflicts.empty())
        {
            // No conflicts, proceed with enabling
            if (fromProtocolChange)
            {
                parameters.setConfigParam("trackingProtocol", trackingProtocolSelector.getSelectedId() - 1);
            }
            else
            {
                trackingEnabledButton.setButtonText(LOC("network.toggles.trackingOn"));
                parameters.setConfigParam("trackingEnabled", 1);
                updateTrackingAppearance();
            }
            return;
        }

        // Build conflict message
        juce::String conflictMsg = "The following clusters have multiple inputs with tracking enabled:\n\n";
        for (const auto& [cluster, inputs] : conflicts)
        {
            conflictMsg += "Cluster " + juce::String(cluster) + ": Inputs ";
            for (size_t j = 0; j < inputs.size(); ++j)
            {
                if (j > 0) conflictMsg += ", ";
                conflictMsg += juce::String(inputs[j] + 1);
            }
            conflictMsg += "\n";
        }
        conflictMsg += LOC("network.dialogs.trackingConflictsMessage");

        juce::AlertWindow::showOkCancelBox(
            juce::AlertWindow::WarningIcon,
            LOC("network.dialogs.trackingConflictsTitle"),
            conflictMsg,
            LOC("network.dialogs.trackingConflictsContinue"),
            LOC("common.cancel"),
            nullptr,
            juce::ModalCallbackFunction::create([this, conflicts, fromProtocolChange](int result) {
                if (result == 1)  // Continue
                {
                    // Disable tracking on all but the first input in each conflicting cluster
                    for (const auto& [cluster, inputs] : conflicts)
                    {
                        for (size_t j = 1; j < inputs.size(); ++j)
                        {
                            parameters.setInputParam(inputs[j], "inputTrackingActive", 0);
                        }
                    }

                    // Now enable global tracking or protocol
                    if (fromProtocolChange)
                    {
                        parameters.setConfigParam("trackingProtocol", trackingProtocolSelector.getSelectedId() - 1);
                    }
                    else
                    {
                        trackingEnabledButton.setButtonText(LOC("network.toggles.trackingOn"));
                        parameters.setConfigParam("trackingEnabled", 1);
                        updateTrackingAppearance();
                    }
                }
                else  // Cancel
                {
                    // Revert the toggle/selector
                    if (fromProtocolChange)
                    {
                        // Revert to DISABLED
                        trackingProtocolSelector.setSelectedId(1, juce::dontSendNotification);
                    }
                    else
                    {
                        trackingEnabledButton.setToggleState(false, juce::dontSendNotification);
                        trackingEnabledButton.setButtonText(LOC("network.toggles.trackingOff"));
                    }
                }
            })
        );
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NetworkTab)
};
