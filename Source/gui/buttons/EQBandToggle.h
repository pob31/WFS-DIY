#pragma once

#include <JuceHeader.h>
#include "../../../spatcore/ui/EQBandToggle.h"
#include "../ColorScheme.h"

/**
 * EQBandToggle
 *
 * Compat shim over the shared spatcore widget: a tiny colored indicator button
 * for toggling individual EQ bands on/off. When ON, displays a filled rounded
 * rectangle in the band's color. When OFF, displays a dark grey rounded
 * rectangle.
 *
 * spatcore may never name an app symbol, so the two theme colours the original
 * read straight from ColorScheme are injected here through the colourProvider
 * seam. The provider is called at paint time and never cached, which is what
 * keeps live theme switching working.
 */
class EQBandToggle : public spatcore::ui::EQBandToggle
{
public:
    EQBandToggle()
    {
        colourProvider = []
        {
            return spatcore::ui::EQBandToggleColours { ColorScheme::get().sliderTrackBg,
                                                       ColorScheme::get().buttonBorder };
        };
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQBandToggle)
};
