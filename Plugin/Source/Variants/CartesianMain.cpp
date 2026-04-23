#include "../Track/TrackProcessor.h"

namespace
{
    wfs::plugin::VariantConfig makeConfig()
    {
        using namespace wfs::plugin;
        return VariantConfig {
            "WFS-DIY Track - Cartesian",
            "trackCart",
            "cartesian",
            AddressingScheme::NativeWFS,
            {{
                { "positionX", "Position X", "/wfs/input/positionX", -50.0f, 50.0f, 0.0f, "m", true },
                { "positionY", "Position Y", "/wfs/input/positionY", -50.0f, 50.0f, 0.0f, "m", true },
                { "positionZ", "Position Z", "/wfs/input/positionZ", -50.0f, 50.0f, 0.0f, "m", true }
            }},
            true
        };
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new wfs::plugin::TrackProcessor (makeConfig());
}
