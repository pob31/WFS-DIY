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
                // ADM-OSC Cartesian: normalised -1..+1 per axis. The
                // plugin sends the combined /adm/obj/<id>/xyz path.
                { "admX", "X", "",  -1.0f, 1.0f, 0.0f, "", true },
                { "admY", "Y", "",  -1.0f, 1.0f, 0.0f, "", true },
                { "admZ", "Z", "",  -1.0f, 1.0f, 0.0f, "", true }
            }},
            true
        };
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new wfs::plugin::TrackProcessor (makeConfig());
}
