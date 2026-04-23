#include "../Track/TrackProcessor.h"

namespace
{
    wfs::plugin::VariantConfig makeConfig()
    {
        using namespace wfs::plugin;
        return VariantConfig {
            "WFS-DIY Track (Cylindrical)",
            "trackCyl",
            "cylindrical",
            AddressingScheme::NativeWFS,
            {{
                { "positionR",     "Position R",     "/wfs/input/positionR",      0.0f,   50.0f,   0.0f, "m",   true },
                { "positionTheta", "Position Theta", "/wfs/input/positionTheta", -180.0f, 180.0f,  0.0f, juce::CharPointer_UTF8 ("\xc2\xb0"), true },
                { "positionZ",     "Position Z",     "/wfs/input/positionZ",     -50.0f,  50.0f,   0.0f, "m",   true }
            }},
            true
        };
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new wfs::plugin::TrackProcessor (makeConfig());
}
