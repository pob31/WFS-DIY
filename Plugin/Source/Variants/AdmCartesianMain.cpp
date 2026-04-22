#include "../Track/TrackProcessor.h"

namespace
{
    wfs::plugin::VariantConfig makeConfig()
    {
        using namespace wfs::plugin;
        return VariantConfig {
            "WFS-DIY Track (ADM Cartesian)",
            "trackAdmCart",
            "adm-cartesian",
            AddressingScheme::AdmOsc,
            {{
                { "admX", "ADM X", "/adm/obj/1/x", -1.0f, 1.0f, 0.0f, "", false },
                { "admY", "ADM Y", "/adm/obj/1/y", -1.0f, 1.0f, 0.0f, "", false },
                { "admZ", "ADM Z", "/adm/obj/1/z", -1.0f, 1.0f, 0.0f, "", false }
            }},
            false
        };
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new wfs::plugin::TrackProcessor (makeConfig());
}
