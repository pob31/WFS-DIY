#pragma once

#include <JuceHeader.h>
#include "../../spatcore/ui/patch/PatchMatrixComponent.h"
#include "../WfsParameters.h"
#include "../DSP/TestSignalGenerator.h"

/**
 * Compat shim over the shared spatcore patch matrix.
 *
 * The matrix now lives in spatcore/ui/patch/ so XOA and Tight-WFS can adopt the
 * whole audio-interface window rather than each growing a channel picker of
 * their own. That header deliberately exports nothing into the global
 * namespace, so this file owns the unqualified name the app has always used and
 * supplies the app-specific config.
 *
 * Unlike the shared EQ, this is a derived class rather than a `using` alias:
 * the shared component takes a PatchMatrixConfig, while every existing call
 * site constructs it with (parameters, isInputPatch, testSignalGen). Deriving
 * keeps those call sites, the `Mode` enum and the Stream Deck accessors
 * unchanged.
 */
class PatchMatrixComponent : public spatcore::ui::patch::PatchMatrixComponent
{
public:
    PatchMatrixComponent (WFSValueTreeState& valueTreeState,
                          bool isInputPatch,
                          TestSignalGenerator* testSignalGen = nullptr)
        : spatcore::ui::patch::PatchMatrixComponent (makeConfig (valueTreeState, isInputPatch),
                                                     isInputPatch,
                                                     testSignalGen)
    {
    }

    /** Builds the shared component's config from this app's parameter schema,
        colour scheme, localisation and accessibility singletons. Providers are
        deliberately not snapshotted — they are invoked at paint/layout time so
        a theme, language or UI-scale change lands on the next repaint. */
    static spatcore::ui::patch::PatchMatrixConfig makeConfig (WFSValueTreeState& parameters,
                                                              bool isInputPatch);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PatchMatrixComponent)
};
