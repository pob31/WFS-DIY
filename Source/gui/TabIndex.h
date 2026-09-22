#pragma once

/**
    The order of the main tabs, in one place.

    The index is not decoration: it is the key the undo-domain table, the
    Stream Deck page registry, the Space Mouse's per-tab behaviour and the
    Map's navigate-to-item callback are all written against. It used to be a
    literal in about thirty places across MainComponent, ControllerManager and
    the seven page headers, and inserting a tab meant finding every one of
    them.

    EFFECTS SITS BETWEEN REVERB AND INPUTS (decision Q14 of the effects plan),
    which is what pushed Inputs, Clusters and Map up by one. Appending it at
    the end would have avoided the renumbering and put a channel family after
    the Map, away from the other three.

    Nothing persists a main-tab index - not the settings file, not the tablet
    protocol, not MCP - so there is no migration and a session written by an
    older build opens on the same tab it always did.
*/
namespace TabIndex
{
    static constexpr int SystemConfig = 0;
    static constexpr int Network      = 1;
    static constexpr int Outputs      = 2;
    static constexpr int Reverb       = 3;
    static constexpr int Effects      = 4;
    static constexpr int Inputs       = 5;
    static constexpr int Clusters     = 6;
    static constexpr int Map          = 7;

    /** One past the last tab. The undo-domain table is sized from this. */
    static constexpr int Count        = 8;
}
