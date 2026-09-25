#pragma once

#include <JuceHeader.h>
#include "../../spatcore/ui/TypedValue.h"

/**
    TypedValue - the app's name for spatcore::ui::typed (spatcore/ui/TypedValue.h),
    the one reader for a number typed into a value field:

      TypedValue::number   ("-6.0 dB", "2.5 kHz", "1,2k")
      TypedValue::duration ("2m 30s", "2 min", "1h30", "1:30")
      TypedValue::ratio    ("4.0:1", "1:2.0")

    Every click-to-type label in the app reads through it and puts itself back
    when the text holds no number. It lives in spatcore so XOA and Go.dot read
    typed values the same way; the app keeps the short name so its call sites
    stay TypedValue::number (...).
*/
namespace TypedValue = spatcore::ui::typed;
