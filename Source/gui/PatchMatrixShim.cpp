#include "PatchMatrixComponent.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/WFSParameterDefaults.h"
#include "ColorScheme.h"
#include "ColorUtilities.h"
#include "WfsLookAndFeel.h"
#include "../Accessibility/TTSManager.h"
#include "../Localization/LocalizationManager.h"

//==============================================================================
// The app half of the shared patch matrix: everything spatcore deliberately
// does not know — this app's ValueTree schema, its colour scheme, its
// localisation table and its screen reader.
//==============================================================================

spatcore::ui::patch::PatchMatrixConfig
PatchMatrixComponent::makeConfig (WFSValueTreeState& parameters, bool isInputPatch)
{
    using namespace spatcore::ui::patch;

    PatchMatrixConfig config;

    //--------------------------------------------------------------------------
    // Trees. The shared component is handed the trees it works on rather than
    // walking the schema to find them.
    auto state = parameters.getState();
    auto audioPatchTree = state.getChildWithName (WFSParameterIDs::AudioPatch);

    config.patchTree = isInputPatch
                         ? audioPatchTree.getChildWithName (WFSParameterIDs::InputPatch)
                         : audioPatchTree.getChildWithName (WFSParameterIDs::OutputPatch);

    config.channelsTree = isInputPatch
                            ? state.getChildWithName (WFSParameterIDs::Inputs)
                            : state.getChildWithName (WFSParameterIDs::Outputs);

    if (! isInputPatch)
        config.binauralTree = state.getChildWithName (WFSParameterIDs::Config)
                                   .getChildWithName (WFSParameterIDs::Binaural);

    //--------------------------------------------------------------------------
    // Schema. Only five property names; everything else the matrix used to walk
    // is now a callback below.
    config.ids.patchData = WFSParameterIDs::patchData;
    config.ids.rows = WFSParameterIDs::rows;
    config.ids.cols = WFSParameterIDs::cols;
    config.ids.activeHardwareChannels = isInputPatch ? WFSParameterIDs::activeHardwareInputs
                                                     : WFSParameterIDs::activeHardwareOutputs;
    config.ids.binauralOutputChannel = WFSParameterIDs::binauralOutputChannel;

    // Column-count fallback when the patch tree carries no `cols` yet. This is
    // the WFS output count, not the 512 hardware ceiling — matching what the
    // matrix used before the move.
    config.maxHardwareChannels = WFSParameterDefaults::maxOutputChannels;

    //--------------------------------------------------------------------------
    // Host queries.
    config.numChannelsProvider = [&parameters, isInputPatch]
    {
        return isInputPatch ? parameters.getNumInputChannels()
                            : parameters.getNumOutputChannels();
    };

    config.recomputeColumns = [&parameters] { parameters.recomputePatchCols(); };

    // A stereo-pair input row holds two hardware columns (lower = L). The
    // type is per-channel (inputChannelType on the row's <Input>), read live
    // so a structural/type change reshapes the matrix on the next repaint.
    // Output rows stay strict 1:1 (provider omitted -> capacity 1).
    if (isInputPatch)
    {
        config.rowCapacityProvider = [&parameters] (int channel) -> int
        {
            return parameters.isInputChannelStereo (channel) ? 2 : 1;
        };
    }

    config.channelNameProvider = [&parameters, isInputPatch] (int channel) -> juce::String
    {
        auto listTree = parameters.getState().getChildWithName (isInputPatch ? WFSParameterIDs::Inputs
                                                                             : WFSParameterIDs::Outputs);
        if (channel < 0 || channel >= listTree.getNumChildren())
            return {};

        auto channelTree = listTree.getChild (channel).getChildWithName (WFSParameterIDs::Channel);

        if (! channelTree.isValid())
            return {};

        return channelTree.getProperty (isInputPatch ? WFSParameterIDs::inputName
                                                     : WFSParameterIDs::outputName).toString();
    };

    config.rowColourProvider = [&parameters, isInputPatch] (int channel) -> juce::Colour
    {
        if (isInputPatch)
            return WfsColorUtilities::getInputColor (channel + 1);

        // Outputs are coloured by the array they belong to. Note the lookup is
        // by NAME ("Output7"), not by index — the two differ once outputs have
        // been reordered.
        auto outputTree = parameters.getState()
                            .getChildWithName (WFSParameterIDs::Outputs)
                            .getChildWithName (WFSParameterIDs::Output + juce::String (channel + 1));

        if (outputTree.isValid())
        {
            auto channelTree = outputTree.getChildWithName (WFSParameterIDs::Channel);
            const int arrayNum = channelTree.getProperty (WFSParameterIDs::outputArray);
            return WfsColorUtilities::getArrayColor (arrayNum);
        }

        return juce::Colours::grey;
    };

    config.contrastingTextProvider = [] (juce::Colour background)
    {
        return WfsColorUtilities::getContrastingTextColor (background);
    };

    //--------------------------------------------------------------------------
    // Presentation. Read at paint/layout time so live theme, language and
    // scale changes show up without rebuilding the matrix.
    config.paletteProvider = []
    {
        const auto& scheme = ColorScheme::get();
        return PatchMatrixPalette { scheme.background,
                                    scheme.backgroundAlt,
                                    scheme.surfaceCard,
                                    scheme.chromeDivider,
                                    scheme.textPrimary,
                                    scheme.textSecondary,
                                    scheme.textDisabled };
    };

    config.translate = [] (const char* key) { return LOC (key); };
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
