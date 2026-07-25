#pragma once

#include <JuceHeader.h>
#include <cmath>
#include <cstdint>
#include <vector>

/*
    ReverbNodePlacement — the default geometry for reverb nodes.

    Reverb node positions are NOT cosmetic: the feed's angular attenuation can
    mute a feed outright, and the return position is where the reverb is
    perceived in the room. They also set the SDN inter-node delays, so nodes
    that start stacked produce delays of a few samples, which is what forces the
    GPU/CPU SDN sweeps into their degenerate single-sample window.

    The previous default laid every node on a straight line along X at 1 m
    spacing with Z = 0 — nearly collinear, on the floor, and perfectly
    symmetrical. This replaces it with a semi-ellipse (box stages) or a full
    ring (cylinder/dome) at 1.5x the stage dimensions, at a 2 m working height,
    with a deterministic jitter to break the symmetry and a height-based
    de-crowding pass that guarantees a usable minimum spacing.

    Height is the de-crowding axis on purpose: it is the perceptually cheapest
    direction to move a reverb node, so a user who stacks nodes gets a layout
    that stays effectively transparent to them while the inter-node distance —
    which is 3-D — recovers the margin the parallel SDN sweep wants.

    Coordinates: X and Y are the floor plan (what the Map tab edits), Z is
    height. Distances are metres.
*/
namespace ReverbNodePlacement
{
    /** Standoff from the stage: the arc sits at 1.5x the stage's own extent. */
    inline constexpr float kStandoffFactor = 1.5f;

    /** Upstage depth standoff, applied to the +Y half of the arc only.

        The arc is an egg rather than an ellipse on purpose: upstage has room
        and benefits from the extra spread, while the downstage ends want to
        stay just past the front edge where they wrap the corners. Scaling the
        whole ellipse would have dragged those ends further into the audience
        along with it. */
    inline constexpr float kStandoffFactorUpstage = 2.25f;

    /** Default working height for a reverb node. */
    inline constexpr float kDefaultHeight = 2.0f;

    /** Symmetry-breaking jitter, as a fraction of the stage dimension on each
        axis. Deliberately small: enough to stop the layout being a mirror of
        itself, not enough to look arbitrary. */
    inline constexpr float kJitterFraction = 0.05f;

    /** Minimum 3-D separation between any two nodes.

        Derived from the SDN constraint rather than from taste: one internal
        block at 48 kHz / 256 samples is 256/48000*343 = 1.83 m of propagation,
        and an inter-node delay of at least one block is what lets the node
        sweep run a full-width parallel window instead of collapsing to the
        per-sample lockstep. Rounded up to 2 m for margin at other rates. */
    inline constexpr float kMinNodeSpacing = 2.0f;

    /** Fallback stage extent when a dimension is missing or degenerate. */
    inline constexpr float kFallbackExtent = 10.0f;

    /** How far past a half-turn the box-stage arc wraps, each end, in radians.

        A plain 180 deg semi-ellipse terminates level with the stage's mid-depth
        line, which leaves the downstage corners uncovered — the reverb sits
        entirely upstage of the audience. Carrying each end ~45 deg further
        brings the last nodes down alongside the downstage edges (at 45 deg the
        end sits 0.53 stage-depths below centre, just past the front edge). */
    inline constexpr float kArcExtension = 0.7853982f;   // 45 deg

    /** Height band the de-crowding may use.

        A hard bound matters: on a small stage with many nodes the horizontal
        arc simply cannot give every pair kMinNodeSpacing, and an unbounded
        relaxation would answer that by flinging nodes to absurd heights. Inside
        the band the pass does what it can and stops. Partial separation is
        still worth having — the SDN chunk bound is recomputed per block from
        the real geometry, so it takes whatever margin exists, and the lockstep
        remains correct when there is none. */
    inline constexpr float kMinHeight = 0.5f;
    inline constexpr float kMaxHeight = 8.0f;

    struct Stage
    {
        int   shape = 0;          // 0 = box, 1 = cylinder, 2 = dome
        float width = 0.0f;       // box: X extent
        float depth = 0.0f;       // box: Y extent
        float height = 0.0f;      // used only to scale the Z jitter
        float diameter = 0.0f;    // cylinder/dome

        // Stored positions are ORIGIN-relative, and the origin is not the stage
        // centre — originDepth defaults to -5 ("downstage centre"), putting the
        // origin on the stage's front edge. The stage centre is therefore at
        // (-originW, -originD), the same convention as getDefaultInputPosition
        // and scaleAllInputPositions (minX = -width/2 - originW).
        float originW = 0.0f;
        float originD = 0.0f;
    };

    struct Node
    {
        float x = 0.0f, y = 0.0f, z = kDefaultHeight;
    };

    inline void deCrowdInZ (std::vector<Node>& nodes, int maxPasses = 8);

    /** Deterministic jitter in [-1, 1).

        Deliberately NOT std::rand or a time-seeded generator. Reverb positions
        feed the DSP, so a layout that differs run to run would make the same
        session render differently and would put nondeterminism into a
        golden-checked path. A hash of (index, axis) is just as asymmetric as
        true randomness for this purpose and is reproducible. */
    inline float jitterFor (int index, int axis) noexcept
    {
        uint32_t h = (uint32_t) index * 0x9E3779B9u + (uint32_t) axis * 0x85EBCA6Bu + 0x7F4A7C15u;
        h ^= h >> 16; h *= 0x7FEB352Du;
        h ^= h >> 15; h *= 0x846CA68Bu;
        h ^= h >> 16;
        return (float) (h & 0xFFFFFFu) / 8388608.0f - 1.0f;
    }

    /** Nodes on a semi-ellipse (box) or ring (cylinder/dome) at kStandoffFactor
        times the stage extent, at kDefaultHeight, jittered, then de-crowded in
        Z so no two nodes are closer than kMinNodeSpacing. */
    inline std::vector<Node> layout (const Stage& stage, int count)
    {
        std::vector<Node> nodes;
        if (count <= 0)
            return nodes;

        const bool isRound = (stage.shape == 1 || stage.shape == 2);

        float extentX, extentY;
        if (isRound)
        {
            const float r = stage.diameter > 0.01f ? stage.diameter * 0.5f : kFallbackExtent * 0.5f;
            extentX = extentY = r * kStandoffFactor;
        }
        else
        {
            const float w = stage.width > 0.01f ? stage.width : kFallbackExtent;
            const float d = stage.depth > 0.01f ? stage.depth : kFallbackExtent;
            extentX = w * 0.5f * kStandoffFactor;
            extentY = d * 0.5f * kStandoffFactor;
        }

        // Jitter amplitudes track the stage, so the spread stays proportional
        // on a 4 m room and a 40 m arena alike.
        const float jx = (isRound ? stage.diameter : stage.width);
        const float jy = (isRound ? stage.diameter : stage.depth);
        const float ampX = kJitterFraction * (jx > 0.01f ? jx : kFallbackExtent);
        const float ampY = kJitterFraction * (jy > 0.01f ? jy : kFallbackExtent);
        const float ampZ = kJitterFraction * (stage.height > 0.01f ? stage.height : kDefaultHeight);

        const float centreX = -stage.originW;
        const float centreY = -stage.originD;

        nodes.resize ((size_t) count);
        for (int i = 0; i < count; ++i)
        {
            // Half-step inset keeps a semi-ellipse off its own endpoints, where
            // nodes would sit level with the stage edge.
            const float t = ((float) i + 0.5f) / (float) count;
            const float arcStart = -kArcExtension;
            const float arcSpan  = juce::MathConstants<float>::pi + 2.0f * kArcExtension;
            const float angle = isRound ? (t * juce::MathConstants<float>::twoPi)
                                        : (arcStart + t * arcSpan);

            // Centred on the STAGE, not on the origin. Centring on (0,0) put the
            // arc's focus on the stage's front edge, so it cut up through the
            // stage instead of enclosing it.
            const float sy = std::sin (angle);
            const float extentYHalf = (sy >= 0.0f && ! isRound)
                                        ? extentY * (kStandoffFactorUpstage / kStandoffFactor)
                                        : extentY;

            auto& n = nodes[(size_t) i];
            n.x = centreX + extentX     * std::cos (angle) + ampX * jitterFor (i, 0);
            n.y = centreY + extentYHalf * sy               + ampY * jitterFor (i, 1);
            n.z = kDefaultHeight       + ampZ * jitterFor (i, 2);
        }

        deCrowdInZ (nodes);
        return nodes;
    }

    /** Pushes offending pairs apart along Z until every pair clears
        kMinNodeSpacing in 3-D, leaving X and Y untouched.

        Separating in the plane would move nodes the user reasons about on the
        Map tab; separating in height is close to inaudible for a reverb node
        and still buys the full 3-D distance. Bounded iteration count: this
        runs on a UI/setup path, and a pathological stage should degrade to
        "as separated as a few passes get" rather than spin. */
    inline void deCrowdInZ (std::vector<Node>& nodes, int maxPasses)   // default on the declaration above
    {
        const int n = (int) nodes.size();
        if (n < 2)
            return;

        for (int pass = 0; pass < maxPasses; ++pass)
        {
            bool moved = false;

            for (int i = 0; i < n; ++i)
            {
                for (int j = i + 1; j < n; ++j)
                {
                    const float dx = nodes[(size_t) i].x - nodes[(size_t) j].x;
                    const float dy = nodes[(size_t) i].y - nodes[(size_t) j].y;
                    const float dz = nodes[(size_t) i].z - nodes[(size_t) j].z;
                    const float d2 = dx * dx + dy * dy + dz * dz;
                    if (d2 >= kMinNodeSpacing * kMinNodeSpacing)
                        continue;

                    // Height needed to make up the shortfall at this horizontal
                    // separation. If the pair is horizontally further apart than
                    // the minimum this is never reached, so the plane wins
                    // whenever it already suffices.
                    const float horiz2 = dx * dx + dy * dy;
                    const float needed = std::sqrt (
                        juce::jmax (0.0f, kMinNodeSpacing * kMinNodeSpacing - horiz2));

                    // Split the correction symmetrically so neither node is
                    // privileged, and keep the existing ordering in Z.
                    const float half = 0.5f * (needed - std::abs (dz)) + 0.001f;
                    const float dir = (dz >= 0.0f) ? 1.0f : -1.0f;
                    nodes[(size_t) i].z = juce::jlimit (kMinHeight, kMaxHeight,
                                                        nodes[(size_t) i].z + dir * half);
                    nodes[(size_t) j].z = juce::jlimit (kMinHeight, kMaxHeight,
                                                        nodes[(size_t) j].z - dir * half);
                    moved = true;
                }
            }

            if (! moved)
                return;
        }
    }
}
