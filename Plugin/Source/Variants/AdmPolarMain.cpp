#include "../Track/TrackProcessor.h"

namespace
{
    wfs::plugin::VariantConfig makeConfig()
    {
        using namespace wfs::plugin;
        return VariantConfig {
            "WFS-DIY Track - ADM Polar",
            "trackAdmPol",
            "adm-polar",
            AddressingScheme::AdmOsc,
            {{
                // ADM-OSC polar: azimuth (-180..+180°), elevation (-90..+90°),
                // distance (0..1 normalised). Sent combined via /aed.
                { "admAzimuth",   "Azimuth",   "", -180.0f, 180.0f, 0.0f,
                  juce::CharPointer_UTF8 ("\xc2\xb0"), true },
                { "admElevation", "Elevation", "",  -90.0f,  90.0f, 0.0f,
                  juce::CharPointer_UTF8 ("\xc2\xb0"), true },
                { "admDistance",  "Distance",  "",    0.0f,   1.0f, 0.0f, "", true }
            }},
            true
        };
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new wfs::plugin::TrackProcessor (makeConfig());
}
