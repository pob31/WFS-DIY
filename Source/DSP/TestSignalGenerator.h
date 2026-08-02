#pragma once

// The test-signal generator now lives in spatcore/io/TestSignalGenerator.h and
// is shared with XOA and Tight-WFS, which get this app's patch window rather
// than a channel picker of their own. Behaviour is unchanged, including the
// 500 ms protective ramp on the continuous signals; the one fix is that
// prepare() recomputes the tone's phase increment, so a Tone selected without
// a preceding setFrequency() is no longer silent and survives a sample-rate
// change.
//
// This header stays so the app's call sites keep compiling unqualified.

#include "../../spatcore/io/TestSignalGenerator.h"

using TestSignalGenerator = spatcore::io::TestSignalGenerator;
