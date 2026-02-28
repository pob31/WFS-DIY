#pragma once

/**
 * PatchWindowPages — Stream Deck+ page definitions for the Audio Interface & Patch window.
 *
 * Used as an override page factory when the Patch window has focus.
 * Top row: Audio Interface, Input Patch, Output Patch buttons.
 *
 * Sub-tabs:
 *   0: Audio Interface  (device info only — no controls)
 *   1: Input Patch      (Scroll/Patch modes, cell navigation dials)
 *   2: Output Patch     (Scroll/Patch/Test modes, test signal controls)
 */

#include "../StreamDeckPage.h"
#include "../../gui/PatchMatrixComponent.h"
#include "../../DSP/TestSignalGenerator.h"
#include "../../Localization/LocalizationManager.h"

namespace PatchWindowPages
{

//==============================================================================
// Callbacks struct — actions that must go through the GUI
//==============================================================================

struct PatchCallbacks
{
    // Sub-tab switching
    std::function<void (int)> switchOverrideSubTab;  // 0=AudioInterface, 1=Input, 2=Output

    // Tab-level mode switching (also switches PatchTabbedComponent)
    std::function<void (int)> switchPatchTab;       // 0=Input, 1=Output

    // Input patch mode
    std::function<void (PatchMatrixComponent::Mode)> setInputPatchMode;

    // Output patch mode
    std::function<void (PatchMatrixComponent::Mode)> setOutputPatchMode;

    // Cell navigation & scrolling
    std::function<void (int, int)> scrollInputByCell;    // (dx, dy)
    std::function<void (int, int)> scrollOutputByCell;
    std::function<void (int, int)> moveInputSelectedCell;   // (dx, dy)
    std::function<void (int, int)> moveOutputSelectedCell;
    std::function<void()>          activateInputSelectedCell;
    std::function<void()>          activateOutputSelectedCell;

    // Test signal controls (Output Patch only)
    std::function<void()>       toggleHold;
    std::function<void (int)>   setTestSignalType;    // 0=Off, 1=PinkNoise, 2=Tone, 3=Sweep, 4=DiracPulse
    std::function<void (float)> setTestLevel;         // dB (-92 to 0)
    std::function<void (float)> setTestFrequency;     // Hz (20 to 20000)
};

//==============================================================================
// State queries struct — read-only state
//==============================================================================

struct PatchStateQueries
{
    // Tab/mode state
    std::function<int()> getCurrentPatchTab;     // 0=Input, 1=Output
    std::function<int()> getInputPatchMode;      // PatchMatrixComponent::Mode as int
    std::function<int()> getOutputPatchMode;

    // Input patch matrix state
    std::function<int()> getInputNumHardwareChannels;
    std::function<int()> getInputNumWFSChannels;
    std::function<int()> getInputScrollCol;          // scrollOffsetX / cellWidth
    std::function<int()> getInputScrollRow;          // scrollOffsetY / cellHeight
    std::function<int()> getInputSelectedCol;        // selectedCell.x
    std::function<int()> getInputSelectedRow;        // selectedCell.y

    // Output patch matrix state
    std::function<int()> getOutputNumHardwareChannels;
    std::function<int()> getOutputNumWFSChannels;
    std::function<int()> getOutputScrollCol;
    std::function<int()> getOutputScrollRow;
    std::function<int()> getOutputSelectedCol;
    std::function<int()> getOutputSelectedRow;

    // Test signal state (Output Patch)
    std::function<bool()>  isHoldEnabled;
    std::function<int()>   getTestSignalType;    // 0-4 (SignalType enum)
    std::function<float()> getTestLevel;         // dB
    std::function<float()> getTestFrequency;     // Hz
};

//==============================================================================
// Colours
//==============================================================================

static const juce::Colour kActiveTabColour   { 0xFF4A90D9 };
static const juce::Colour kInactiveTabColour { 0xFF555555 };
static const juce::Colour kModeActiveColour  { 0xFF26A69A };
static const juce::Colour kModeInactiveColour{ 0xFF666666 };
static const juce::Colour kPatchToggleColour { 0xFFE67E22 };
static const juce::Colour kTestActionColour  { 0xFFE74C3C };
static const juce::Colour kHoldOnColour      { 0xFF2ECC71 };
static const juce::Colour kHoldOffColour     { 0xFF666666 };

//==============================================================================
// Page factory
//==============================================================================

inline StreamDeckPage createPage (int overrideSubTab,
                                   const PatchCallbacks& cb,
                                   const PatchStateQueries& q)
{
    StreamDeckPage page ("Patch Window");
    page.numSections = 1;

    //======================================================================
    // Top row: sub-tab selectors (always the same on all sub-tabs)
    //======================================================================

    // Button 0: Audio Interface
    {
        auto& btn = page.topRowButtons[0];
        btn.label  = LOC ("streamDeck.patch.tabs.audioInterface");
        btn.colour = (overrideSubTab == 0) ? kActiveTabColour : kInactiveTabColour;
        btn.type   = ButtonBinding::Action;
        btn.onPress = [cb]() { if (cb.switchOverrideSubTab) cb.switchOverrideSubTab (0); };
    }

    // Button 1: Input Patch
    {
        auto& btn = page.topRowButtons[1];
        btn.label  = LOC ("streamDeck.patch.tabs.inputPatch");
        btn.colour = (overrideSubTab == 1) ? kActiveTabColour : kInactiveTabColour;
        btn.type   = ButtonBinding::Action;
        btn.onPress = [cb]()
        {
            if (cb.switchPatchTab) cb.switchPatchTab (0);
            if (cb.switchOverrideSubTab) cb.switchOverrideSubTab (1);
        };
    }

    // Button 2: Output Patch
    {
        auto& btn = page.topRowButtons[2];
        btn.label  = LOC ("streamDeck.patch.tabs.outputPatch");
        btn.colour = (overrideSubTab == 2) ? kActiveTabColour : kInactiveTabColour;
        btn.type   = ButtonBinding::Action;
        btn.onPress = [cb]()
        {
            if (cb.switchPatchTab) cb.switchPatchTab (1);
            if (cb.switchOverrideSubTab) cb.switchOverrideSubTab (2);
        };
    }

    // Button 3: Hold toggle (only on Output Patch in Test mode)
    if (overrideSubTab == 2 && q.getOutputPatchMode
        && q.getOutputPatchMode() == static_cast<int> (PatchMatrixComponent::Mode::Testing))
    {
        auto& btn = page.topRowButtons[3];
        btn.colour       = kHoldOffColour;
        btn.activeColour = kHoldOnColour;
        btn.type         = ButtonBinding::Toggle;

        btn.getState = [q]() { return q.isHoldEnabled ? q.isHoldEnabled() : false; };

        btn.getDynamicLabel = [q]()
        {
            bool on = q.isHoldEnabled ? q.isHoldEnabled() : false;
            return on ? LOC ("streamDeck.patch.buttons.holdOn")
                      : LOC ("streamDeck.patch.buttons.holdOff");
        };

        btn.onPress = [cb]() { if (cb.toggleHold) cb.toggleHold(); };
        btn.requestsPageRebuild = true;
    }

    //======================================================================
    // Sub-tab 0: Audio Interface (info only — no controls)
    //======================================================================
    if (overrideSubTab == 0)
    {
        auto& sec = page.sections[0];
        sec.sectionName   = LOC ("streamDeck.patch.tabs.audioInterface");
        sec.sectionColour = kActiveTabColour;
        // All buttons and dials left empty
    }

    //======================================================================
    // Sub-tab 1: Input Patch
    //======================================================================
    else if (overrideSubTab == 1)
    {
        const int mode = q.getInputPatchMode ? q.getInputPatchMode() : 0;
        const bool isScrollMode = (mode == static_cast<int> (PatchMatrixComponent::Mode::Scrolling));
        const bool isPatchMode  = (mode == static_cast<int> (PatchMatrixComponent::Mode::Patching));

        auto& sec = page.sections[0];
        sec.sectionName   = LOC ("streamDeck.patch.tabs.inputPatch");
        sec.sectionColour = kActiveTabColour;

        // Bottom row button 0: Scroll mode
        {
            auto& btn = sec.buttons[0];
            btn.label        = LOC ("streamDeck.patch.buttons.scroll");
            btn.colour       = kModeInactiveColour;
            btn.activeColour = kModeActiveColour;
            btn.type         = ButtonBinding::Toggle;
            btn.requestsPageRebuild = true;
            btn.getState = [isScrollMode]() { return isScrollMode; };
            btn.onPress  = [cb]()
            {
                if (cb.setInputPatchMode) cb.setInputPatchMode (PatchMatrixComponent::Mode::Scrolling);
            };
        }

        // Bottom row button 1: Patch mode
        {
            auto& btn = sec.buttons[1];
            btn.label        = LOC ("streamDeck.patch.buttons.patch");
            btn.colour       = kModeInactiveColour;
            btn.activeColour = kModeActiveColour;
            btn.type         = ButtonBinding::Toggle;
            btn.requestsPageRebuild = true;
            btn.getState = [isPatchMode]() { return isPatchMode; };
            btn.onPress  = [cb]()
            {
                if (cb.setInputPatchMode) cb.setInputPatchMode (PatchMatrixComponent::Mode::Patching);
            };
        }

        // Bottom row button 3: Patch/Unpatch toggle (Patch mode only)
        if (isPatchMode)
        {
            auto& btn = sec.buttons[3];
            btn.label  = LOC ("streamDeck.patch.buttons.patchUnpatch");
            btn.colour = kPatchToggleColour;
            btn.type   = ButtonBinding::Action;
            btn.onPress = [cb]() { if (cb.activateInputSelectedCell) cb.activateInputSelectedCell(); };
        }

        // Dials
        if (isScrollMode)
        {
            // Dial 0: Scroll columns
            {
                auto& d = sec.dials[0];
                d.paramName = LOC ("streamDeck.patch.dials.columns");
                d.paramUnit = "";
                d.type      = DialBinding::Int;
                d.minValue  = 1;
                d.maxValue  = static_cast<float> (juce::jmax (1, q.getInputNumHardwareChannels ? q.getInputNumHardwareChannels() : 1));
                d.step      = 1;
                d.decimalPlaces = 0;

                d.getValue = [q]()
                {
                    return static_cast<float> ((q.getInputScrollCol ? q.getInputScrollCol() : 0) + 1);
                };

                d.setValue = [q, cb] (float newVal)
                {
                    int current = q.getInputScrollCol ? q.getInputScrollCol() : 0;
                    int target  = static_cast<int> (newVal) - 1;
                    int dx = target - current;
                    if (dx != 0 && cb.scrollInputByCell)
                        cb.scrollInputByCell (dx, 0);
                };
            }

            // Dial 1: Scroll rows
            {
                auto& d = sec.dials[1];
                d.paramName = LOC ("streamDeck.patch.dials.rows");
                d.paramUnit = "";
                d.type      = DialBinding::Int;
                d.minValue  = 1;
                d.maxValue  = static_cast<float> (juce::jmax (1, q.getInputNumWFSChannels ? q.getInputNumWFSChannels() : 1));
                d.step      = 1;
                d.decimalPlaces = 0;

                d.getValue = [q]()
                {
                    return static_cast<float> ((q.getInputScrollRow ? q.getInputScrollRow() : 0) + 1);
                };

                d.setValue = [q, cb] (float newVal)
                {
                    int current = q.getInputScrollRow ? q.getInputScrollRow() : 0;
                    int target  = static_cast<int> (newVal) - 1;
                    int dy = target - current;
                    if (dy != 0 && cb.scrollInputByCell)
                        cb.scrollInputByCell (0, dy);
                };
            }
        }
        else if (isPatchMode)
        {
            // Dial 0: Select column
            {
                auto& d = sec.dials[0];
                d.paramName = LOC ("streamDeck.patch.dials.columns");
                d.paramUnit = "";
                d.type      = DialBinding::Int;
                d.minValue  = 1;
                d.maxValue  = static_cast<float> (juce::jmax (1, q.getInputNumHardwareChannels ? q.getInputNumHardwareChannels() : 1));
                d.step      = 1;
                d.decimalPlaces = 0;

                d.getValue = [q]()
                {
                    return static_cast<float> ((q.getInputSelectedCol ? q.getInputSelectedCol() : 0) + 1);
                };

                d.setValue = [q, cb] (float newVal)
                {
                    int current = q.getInputSelectedCol ? q.getInputSelectedCol() : 0;
                    int target  = static_cast<int> (newVal) - 1;
                    int dx = target - current;
                    if (dx != 0 && cb.moveInputSelectedCell)
                        cb.moveInputSelectedCell (dx, 0);
                };
            }

            // Dial 1: Select row
            {
                auto& d = sec.dials[1];
                d.paramName = LOC ("streamDeck.patch.dials.rows");
                d.paramUnit = "";
                d.type      = DialBinding::Int;
                d.minValue  = 1;
                d.maxValue  = static_cast<float> (juce::jmax (1, q.getInputNumWFSChannels ? q.getInputNumWFSChannels() : 1));
                d.step      = 1;
                d.decimalPlaces = 0;

                d.getValue = [q]()
                {
                    return static_cast<float> ((q.getInputSelectedRow ? q.getInputSelectedRow() : 0) + 1);
                };

                d.setValue = [q, cb] (float newVal)
                {
                    int current = q.getInputSelectedRow ? q.getInputSelectedRow() : 0;
                    int target  = static_cast<int> (newVal) - 1;
                    int dy = target - current;
                    if (dy != 0 && cb.moveInputSelectedCell)
                        cb.moveInputSelectedCell (0, dy);
                };
            }
        }
    }

    //======================================================================
    // Sub-tab 2: Output Patch
    //======================================================================
    else if (overrideSubTab == 2)
    {
        const int mode = q.getOutputPatchMode ? q.getOutputPatchMode() : 0;
        const bool isScrollMode = (mode == static_cast<int> (PatchMatrixComponent::Mode::Scrolling));
        const bool isPatchMode  = (mode == static_cast<int> (PatchMatrixComponent::Mode::Patching));
        const bool isTestMode   = (mode == static_cast<int> (PatchMatrixComponent::Mode::Testing));

        auto& sec = page.sections[0];
        sec.sectionName   = LOC ("streamDeck.patch.tabs.outputPatch");
        sec.sectionColour = kActiveTabColour;

        // Bottom row button 0: Scroll mode
        {
            auto& btn = sec.buttons[0];
            btn.label        = LOC ("streamDeck.patch.buttons.scroll");
            btn.colour       = kModeInactiveColour;
            btn.activeColour = kModeActiveColour;
            btn.type         = ButtonBinding::Toggle;
            btn.requestsPageRebuild = true;
            btn.getState = [isScrollMode]() { return isScrollMode; };
            btn.onPress  = [cb]()
            {
                if (cb.setOutputPatchMode) cb.setOutputPatchMode (PatchMatrixComponent::Mode::Scrolling);
            };
        }

        // Bottom row button 1: Patch mode
        {
            auto& btn = sec.buttons[1];
            btn.label        = LOC ("streamDeck.patch.buttons.patch");
            btn.colour       = kModeInactiveColour;
            btn.activeColour = kModeActiveColour;
            btn.type         = ButtonBinding::Toggle;
            btn.requestsPageRebuild = true;
            btn.getState = [isPatchMode]() { return isPatchMode; };
            btn.onPress  = [cb]()
            {
                if (cb.setOutputPatchMode) cb.setOutputPatchMode (PatchMatrixComponent::Mode::Patching);
            };
        }

        // Bottom row button 2: Test mode
        {
            auto& btn = sec.buttons[2];
            btn.label        = LOC ("streamDeck.patch.buttons.test");
            btn.colour       = kModeInactiveColour;
            btn.activeColour = kTestActionColour;
            btn.type         = ButtonBinding::Toggle;
            btn.requestsPageRebuild = true;
            btn.getState = [isTestMode]() { return isTestMode; };
            btn.onPress  = [cb]()
            {
                if (cb.setOutputPatchMode) cb.setOutputPatchMode (PatchMatrixComponent::Mode::Testing);
            };
        }

        // Bottom row button 3: context-sensitive
        if (isPatchMode)
        {
            // Patch/Unpatch toggle at selected cell
            auto& btn = sec.buttons[3];
            btn.label  = LOC ("streamDeck.patch.buttons.patchUnpatch");
            btn.colour = kPatchToggleColour;
            btn.type   = ButtonBinding::Action;
            btn.onPress = [cb]() { if (cb.activateOutputSelectedCell) cb.activateOutputSelectedCell(); };
        }
        else if (isTestMode)
        {
            // Test at selected cell
            auto& btn = sec.buttons[3];
            btn.label  = LOC ("streamDeck.patch.buttons.testChannel");
            btn.colour = kTestActionColour;
            btn.type   = ButtonBinding::Action;
            btn.onPress = [cb]() { if (cb.activateOutputSelectedCell) cb.activateOutputSelectedCell(); };
        }

        // Dials 0-1: column/row (scroll or select depending on mode)
        if (isScrollMode)
        {
            // Dial 0: Scroll columns
            {
                auto& d = sec.dials[0];
                d.paramName = LOC ("streamDeck.patch.dials.columns");
                d.paramUnit = "";
                d.type      = DialBinding::Int;
                d.minValue  = 1;
                d.maxValue  = static_cast<float> (juce::jmax (1, q.getOutputNumHardwareChannels ? q.getOutputNumHardwareChannels() : 1));
                d.step      = 1;
                d.decimalPlaces = 0;

                d.getValue = [q]()
                {
                    return static_cast<float> ((q.getOutputScrollCol ? q.getOutputScrollCol() : 0) + 1);
                };

                d.setValue = [q, cb] (float newVal)
                {
                    int current = q.getOutputScrollCol ? q.getOutputScrollCol() : 0;
                    int target  = static_cast<int> (newVal) - 1;
                    int dx = target - current;
                    if (dx != 0 && cb.scrollOutputByCell)
                        cb.scrollOutputByCell (dx, 0);
                };
            }

            // Dial 1: Scroll rows
            {
                auto& d = sec.dials[1];
                d.paramName = LOC ("streamDeck.patch.dials.rows");
                d.paramUnit = "";
                d.type      = DialBinding::Int;
                d.minValue  = 1;
                d.maxValue  = static_cast<float> (juce::jmax (1, q.getOutputNumWFSChannels ? q.getOutputNumWFSChannels() : 1));
                d.step      = 1;
                d.decimalPlaces = 0;

                d.getValue = [q]()
                {
                    return static_cast<float> ((q.getOutputScrollRow ? q.getOutputScrollRow() : 0) + 1);
                };

                d.setValue = [q, cb] (float newVal)
                {
                    int current = q.getOutputScrollRow ? q.getOutputScrollRow() : 0;
                    int target  = static_cast<int> (newVal) - 1;
                    int dy = target - current;
                    if (dy != 0 && cb.scrollOutputByCell)
                        cb.scrollOutputByCell (0, dy);
                };
            }
        }
        else  // Patch or Test mode — cell selection
        {
            // Dial 0: Select column
            {
                auto& d = sec.dials[0];
                d.paramName = LOC ("streamDeck.patch.dials.columns");
                d.paramUnit = "";
                d.type      = DialBinding::Int;
                d.minValue  = 1;
                d.maxValue  = static_cast<float> (juce::jmax (1, q.getOutputNumHardwareChannels ? q.getOutputNumHardwareChannels() : 1));
                d.step      = 1;
                d.decimalPlaces = 0;

                d.getValue = [q]()
                {
                    return static_cast<float> ((q.getOutputSelectedCol ? q.getOutputSelectedCol() : 0) + 1);
                };

                d.setValue = [q, cb] (float newVal)
                {
                    int current = q.getOutputSelectedCol ? q.getOutputSelectedCol() : 0;
                    int target  = static_cast<int> (newVal) - 1;
                    int dx = target - current;
                    if (dx != 0 && cb.moveOutputSelectedCell)
                        cb.moveOutputSelectedCell (dx, 0);
                };
            }

            // Dial 1: Select row
            {
                auto& d = sec.dials[1];
                d.paramName = LOC ("streamDeck.patch.dials.rows");
                d.paramUnit = "";
                d.type      = DialBinding::Int;
                d.minValue  = 1;
                d.maxValue  = static_cast<float> (juce::jmax (1, q.getOutputNumWFSChannels ? q.getOutputNumWFSChannels() : 1));
                d.step      = 1;
                d.decimalPlaces = 0;

                d.getValue = [q]()
                {
                    return static_cast<float> ((q.getOutputSelectedRow ? q.getOutputSelectedRow() : 0) + 1);
                };

                d.setValue = [q, cb] (float newVal)
                {
                    int current = q.getOutputSelectedRow ? q.getOutputSelectedRow() : 0;
                    int target  = static_cast<int> (newVal) - 1;
                    int dy = target - current;
                    if (dy != 0 && cb.moveOutputSelectedCell)
                        cb.moveOutputSelectedCell (0, dy);
                };
            }
        }

        // Test mode dials 2-3: signal type and level
        if (isTestMode)
        {
            // Dial 2: Signal type (ComboBox)
            {
                auto& d = sec.dials[2];
                d.paramName = LOC ("streamDeck.patch.dials.signalType");
                d.type      = DialBinding::ComboBox;
                d.comboOptions = {
                    "Off",
                    "Pink Noise",
                    "Tone",
                    "Sweep",
                    "Pulse"
                };

                d.getValue = [q]()
                {
                    return static_cast<float> (q.getTestSignalType ? q.getTestSignalType() : 0);
                };

                d.setValue = [cb] (float v)
                {
                    if (cb.setTestSignalType)
                        cb.setTestSignalType (juce::roundToInt (v));
                };
            }

            // Dial 3: Level (-92 to 0 dB)
            {
                auto& d = sec.dials[3];
                d.paramName     = LOC ("streamDeck.patch.dials.level");
                d.paramUnit     = "dB";
                d.type          = DialBinding::Float;
                d.minValue      = -92.0f;
                d.maxValue      = 0.0f;
                d.step          = 1.0f;
                d.fineStep      = 0.1f;
                d.decimalPlaces = 1;

                d.getValue = [q]()
                {
                    return q.getTestLevel ? q.getTestLevel() : -40.0f;
                };

                d.setValue = [cb] (float v)
                {
                    if (cb.setTestLevel) cb.setTestLevel (v);
                };

                // altBinding for Frequency (only when signal type == Tone)
                int sigType = q.getTestSignalType ? q.getTestSignalType() : 0;
                if (sigType == static_cast<int> (TestSignalGenerator::SignalType::Tone))
                {
                    auto alt = std::make_unique<DialBinding>();
                    alt->paramName     = LOC ("streamDeck.patch.dials.frequency");
                    alt->paramUnit     = "Hz";
                    alt->type          = DialBinding::Float;
                    alt->minValue      = 20.0f;
                    alt->maxValue      = 20000.0f;
                    alt->step          = 0.02f;   // exponential normalized step
                    alt->fineStep      = 0.005f;
                    alt->isExponential = true;
                    alt->decimalPlaces = 0;

                    alt->getValue = [q]()
                    {
                        return q.getTestFrequency ? q.getTestFrequency() : 1000.0f;
                    };

                    alt->setValue = [cb] (float v)
                    {
                        if (cb.setTestFrequency) cb.setTestFrequency (v);
                    };

                    d.altBinding = std::move (alt);
                    d.fineStep   = 0.0f;  // no fine mode — press+turn uses altBinding
                }
            }
        }
    }

    return page;
}

} // namespace PatchWindowPages
