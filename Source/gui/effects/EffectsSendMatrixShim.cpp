// The one file in Source/gui/effects that names both the shared send matrix
// and the application's schema. The widget is schema-free by design; this is
// where the effects family's two row keyings - input PERMANENT number for the
// first block of rows, dense effect index for the second - are folded into
// the plain (row, column) the widget speaks.

#include "EffectsSendsPanel.h"
#include "../ColorScheme.h"
#include "../InputColour.h"
#include "../WfsLookAndFeel.h"
#include "../../Accessibility/TTSManager.h"
#include "../../Localization/LocalizationManager.h"
#include "../../Parameters/WFSParameterIDs.h"
#include "../../Parameters/WFSParameterDefaults.h"

using spatcore::ui::sends::SendMatrixConfig;
using spatcore::ui::sends::SendMatrixPalette;
using spatcore::ui::sends::SendRowKind;

namespace
{
    // Effect returns on every map and grid: teal, as the binaural controls.
    const juce::Colour kEffectRowColour (0xFF26A69A);
}

SendMatrixConfig makeEffectsSendMatrixConfig (EffectsTabContext& ctx,
                                              const std::function<bool (int column)>& columnInCycle)
{
    SendMatrixConfig config;
    auto& params = ctx.parameters;

    //--------------------------------------------------------------------------
    // Shape. Rows: every live input in slot order, then every live effect in
    // dense order. Columns: every live effect.
    config.numRows    = [&params] { return params.getNumInputChannels() + params.getNumEffectChannels(); };
    config.numColumns = [&params] { return params.getNumEffectChannels(); };
    config.rowKind    = [&params] (int row)
    {
        return row < params.getNumInputChannels() ? SendRowKind::Input : SendRowKind::Effect;
    };

    //--------------------------------------------------------------------------
    // Labels. An input is named by its PERMANENT number, which is what every
    // wire protocol and the send row itself are keyed by; an effect by its
    // dense index, because effect ids are dense.
    config.rowLabel = [&params] (int row)
    {
        auto& vts = params.getValueTreeState();
        const int numInputs = params.getNumInputChannels();

        if (row < numInputs)
        {
            const int number = vts.getInputChannelNumber (row);
            const auto name = vts.getInputParameter (row, WFSParameterIDs::inputName).toString();
            return LOC ("effects.sends.inputPrefix") + " " + juce::String (number)
                 + (name.isNotEmpty() ? " " + name : juce::String());
        }

        const int fx = row - numInputs;
        const auto name = vts.getEffectParameter (fx, WFSParameterIDs::effectName).toString();
        return LOC ("effects.sends.effectPrefix") + " " + juce::String (fx + 1)
             + (name.isNotEmpty() ? " " + name : juce::String());
    };

    config.columnLabel = [&params] (int column)
    {
        const auto name = params.getValueTreeState().getEffectParameter (column, WFSParameterIDs::effectName).toString();
        return juce::String (column + 1) + (name.isNotEmpty() ? "\n" + name : juce::String());
    };

    config.rowColour = [&params] (int row)
    {
        const int numInputs = params.getNumInputChannels();
        if (row < numInputs)
            return WfsInputColour::resolveForSlot (params.getValueTreeState(), row);

        return kEffectRowColour;
    };

    //--------------------------------------------------------------------------
    // Grouping: effect rows by link group, inputs never. A bunch that shares a
    // link group reads as one collapsible block.
    config.rowGroup = [&params] (int row)
    {
        const int numInputs = params.getNumInputChannels();
        if (row < numInputs)
            return 0;

        return params.getValueTreeState().getEffectLinkGroup (row - numInputs);
    };

    config.groupLabel = [&params] (int group)
    {
        juce::StringArray names;
        names.addTokens (params.getConfigParam ("effectsGlobalLinkNames").toString(), ",", "");
        const auto name = names[group - 1].trim();
        return name.isNotEmpty() ? name : LOC ("effects.link.group") + " " + juce::String (group);
    };

    config.selectedColumn = [&ctx] { return ctx.hasChannels() ? ctx.slot() : -1; };

    //--------------------------------------------------------------------------
    // Cells, through the typed accessors and nothing else: they canonicalise
    // the packed rows, refuse the diagonal, and key the two halves correctly.
    config.cellLevelDb = [&params] (int row, int column)
    {
        auto& vts = params.getValueTreeState();
        const int numInputs = params.getNumInputChannels();
        if (row < numInputs)
            return vts.getEffectSendLevelFromInput (column, vts.getInputChannelNumber (row));

        return vts.getEffectFxSendLevelFromEffect (column, row - numInputs);
    };

    config.cellOn = [&params] (int row, int column)
    {
        auto& vts = params.getValueTreeState();
        const int numInputs = params.getNumInputChannels();
        if (row < numInputs)
            return vts.getEffectSendOnFromInput (column, vts.getInputChannelNumber (row));

        return vts.getEffectFxSendOnFromEffect (column, row - numInputs);
    };

    // THE DIAGONAL. An effect cannot feed itself: a loop around a delay line,
    // not a routing choice. The accessor refuses the write; the widget never
    // offers it.
    config.isCellForbidden = [&params] (int row, int column)
    {
        const int numInputs = params.getNumInputChannels();
        return row >= numInputs && (row - numInputs) == column;
    };

    config.columnInCycle = columnInCycle;

    // The ENTRY point of a bunch is emergent (decision of 2026-09-22): a
    // column fed by at least one input. Derived from the rows, stored nowhere.
    config.columnIsEntry = [&params] (int column)
    {
        auto& vts = params.getValueTreeState();
        const int numInputs = params.getNumInputChannels();
        for (int slot = 0; slot < numInputs; ++slot)
            if (vts.getEffectSendOnFromInput (column, vts.getInputChannelNumber (slot)))
                return true;
        return false;
    };

    config.levelMinDb     = WFSParameterDefaults::effectSendLevelMin;
    config.levelMaxDb     = WFSParameterDefaults::effectSendLevelMax;
    config.levelDefaultDb = WFSParameterDefaults::effectSendLevelDefault;

    //--------------------------------------------------------------------------
    // Presentation. Read at paint/layout time so live theme, language and
    // scale changes show up without rebuilding the matrix.
    config.paletteProvider = []
    {
        const auto& scheme = ColorScheme::get();
        SendMatrixPalette p;
        p.background    = scheme.background;
        p.backgroundAlt = scheme.backgroundAlt;
        p.surfaceCard   = scheme.surfaceCard;
        p.divider       = scheme.chromeDivider;
        p.textPrimary   = scheme.textPrimary;
        p.textSecondary = scheme.textSecondary;
        p.textDisabled  = scheme.textDisabled;
        p.selection     = juce::Colour (0xFFFFC107);
        p.warning       = juce::Colour (0xFFE53935);
        p.entry         = juce::Colour (0xFF4CAF50);
        return p;
    };

    // The widget's keys are "sends.*"; the app files them under "effects.".
    config.translate = [] (const char* key) { return LOC (juce::String ("effects.") + key); };
    config.uiScaleProvider = [] { return WfsLookAndFeel::uiScale; };

    //--------------------------------------------------------------------------
    // Accessibility.
    config.announce = [] (const juce::String& text)
    {
        TTSManager::getInstance().announceImmediate (
            text, juce::AccessibilityHandler::AnnouncementPriority::medium);
    };
    config.announceDebounced = [] (const juce::String& text)
    {
        TTSManager::getInstance().announceDebounced (text);
    };
    config.cancelDebouncedAnnouncement = []
    {
        TTSManager::getInstance().cancelDebouncedAnnouncement();
    };

    return config;
}
