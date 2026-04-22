#include "../Track/TrackProcessor.h"

namespace
{
    wfs::plugin::VariantConfig makeConfig()
    {
        using namespace wfs::plugin;
        return VariantConfig {
            "WFS-DIY Track (ADM Polar)",
            "trackAdmPol",
            "adm-polar",
            AddressingScheme::AdmOsc,
            {{
                { "admDistance",  "ADM Distance",  "/adm/obj/1/distance",  0.0f, 1.0f, 0.0f, "", false },
                { "admAzimuth",   "ADM Azimuth",   "/adm/obj/1/azimuth",  -1.0f, 1.0f, 0.0f, "", false },
                { "admElevation", "ADM Elevation", "/adm/obj/1/elevation", -1.0f, 1.0f, 0.0f, "", false }
            }},
            false
        };
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new wfs::plugin::TrackProcessor (makeConfig());
}
