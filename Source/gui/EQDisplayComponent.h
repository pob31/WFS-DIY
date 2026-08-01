#pragma once

#include <JuceHeader.h>
#include "../../spatcore/ui/EQDisplayComponent.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/WFSParameterDefaults.h"
#include "ColorScheme.h"
#include "../Localization/LocalizationManager.h"

//==============================================================================
/**
 * Compat shim over the shared spatcore EQ display.
 *
 * The interactive EQ graph now lives in spatcore/ui/EQDisplayComponent.h and is
 * shared with XOA. That header deliberately exports nothing into the global
 * namespace, so this file owns the unqualified names the app has always used
 * and layers the app-specific config factories on top. Every call site in
 * OutputsTab.h / ReverbTab.h keeps compiling unchanged.
 */
using EQFilterType     = spatcore::ui::EQFilterType;
using EQDisplayPalette = spatcore::ui::EQDisplayPalette;

//==============================================================================
/**
 * App-side EQDisplayConfig: the shared config plus the three factories the tabs
 * call. It adds NO data members, so handing one to EQDisplayComponent (which
 * takes a const reference to the spatcore base and copies that base) slices
 * harmlessly and intentionally — there is nothing to lose.
 */
struct EQDisplayConfig : spatcore::ui::EQDisplayConfig
{
    /** ColorScheme -> palette-role mapping, shared by all three factories.
     *  Read at paint time (never snapshotted) so live theme switching works.
     *  Only panelBackground is pre-modified: the retired component applied
     *  .darker (0.3f) itself at paint time, and the shared one fills the graph
     *  area with whatever it is handed. Every other role is the raw scheme
     *  colour — the component applies its own alphas/darkening.
     */
    static EQDisplayPalette makePalette()
    {
        return { ColorScheme::get().background,                    // disabled scrim
                 ColorScheme::get().backgroundAlt.darker (0.3f),   // graph area fill
                 ColorScheme::get().chromeDivider,                 // freq + dB grid
                 ColorScheme::get().textPrimary,                   // curve + selection ring
                 ColorScheme::get().textSecondary,                 // axis labels + "EQ OFF"
                 ColorScheme::get().accentBlue };                  // fill under the curve
    }

    /** Localised "EQ OFF" caption, looked up at paint time so a language
     *  switch shows up on the next repaint.
     */
    static juce::String getDisabledText() { return LOC("eq.status.off"); }

    static EQDisplayConfig forOutputEQ()
    {
        EQDisplayConfig config;
        config.shapeId = WFSParameterIDs::eqShape;
        config.frequencyId = WFSParameterIDs::eqFrequency;
        config.gainId = WFSParameterIDs::eqGain;
        config.qId = WFSParameterIDs::eqQ;
        config.qMin = WFSParameterDefaults::eqQMin;
        config.qMax = WFSParameterDefaults::eqQMax;

        // BEHAVIOUR CHANGE (deliberate correctness fix): the shelves are now
        // drawn from the stored slope parameter the AUDIO path has always used,
        // instead of folding S into Q as the retired juce::dsp math did. Curves
        // visibly change at non-default slope values — they now match what is
        // actually heard.
        config.slopeId = WFSParameterIDs::eqSlope;
        config.slopeMin = WFSParameterDefaults::eqSlopeMin;
        config.slopeMax = WFSParameterDefaults::eqSlopeMax;

        config.hasBandPass = true;
        config.paletteProvider = [] { return makePalette(); };
        config.disabledTextProvider = [] { return getDisabledText(); };
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

        // See forOutputEQ(): the drawn shelves now honour the stored slope.
        config.slopeId = WFSParameterIDs::reverbPreEQslope;
        config.slopeMin = WFSParameterDefaults::reverbPreEQslopeMin;
        config.slopeMax = WFSParameterDefaults::reverbPreEQslopeMax;

        config.hasBandPass = false;
        config.paletteProvider = [] { return makePalette(); };
        config.disabledTextProvider = [] { return getDisabledText(); };
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

        // See forOutputEQ(): the drawn shelves now honour the stored slope.
        config.slopeId = WFSParameterIDs::reverbPostEQslope;
        config.slopeMin = WFSParameterDefaults::reverbPostEQslopeMin;
        config.slopeMax = WFSParameterDefaults::reverbPostEQslopeMax;

        config.hasBandPass = false;
        config.paletteProvider = [] { return makePalette(); };
        config.disabledTextProvider = [] { return getDisabledText(); };
        return config;
    }
};

//==============================================================================
using EQDisplayComponent = spatcore::ui::EQDisplayComponent;
