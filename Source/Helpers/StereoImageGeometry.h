#pragma once

#include <JuceHeader.h>
#include <cmath>

/*
    StereoImageGeometry — how far and in which direction a stereo pair's legs
    sit from the position the user placed.

    A stereo channel's stored position (inputPositionX/Y/Z) is the CENTRE of the
    pair. The left and right legs are offsets, recomputed on every refresh and
    never persisted. Two callers need that mapping — the 50 Hz geometry refresh
    that feeds the calculation engine, and the Map tab that draws the legs — and
    they must agree exactly, because a Map tab drawing an image the renderer is
    not producing is worse than no drawing at all. That is why the mapping lives
    here and not in either caller.

    Width is an absolute distance in metres and NO speaker position is read.
    The reference used to be a fraction of the array's X extent, which held only
    for a frontal bar: on a circular array of radius R the X extent is 2R, so
    100% spread ±R wherever the source stood, and on a straight array running
    along Y the X extent is zero, so the width dial did nothing at any setting
    with no error to show for it. Metres mean the same thing on every rig.

    The image is strictly planar: spread happens in XY, never in Z.

    Coordinates are stage metres as the Map tab uses them (+X stage right,
    +Y upstage). Angles are degrees, positive counter-clockwise viewed from
    above — the same convention as inputRotation.
*/
namespace WFSStereoImage
{
    /** Unit vector the pair spreads along. Defaults to +X so a caller's
        freshly constructed follower already describes a house-left/right pair. */
    struct Axis
    {
        float x = 1.0f, y = 0.0f;
    };

    /** Per-channel state followAxis() carries between ticks. The caller owns one
        of these per stereo channel and default-initialises it; a default-built
        follower is unprimed, so its first evaluation snaps rather than glides. */
    struct AxisFollower
    {
        Axis  axis {};                        // current axis, always unit length
        float lastX = 0.0f, lastY = 0.0f;     // anchor the state was last evaluated at
        bool  primed = false;                 // false => next evaluation snaps
    };

    /** Anchor distance below which there is no bearing to derive an axis from.
        Not a freeze band: at 1 cm the bearing is still meaningful, it is only
        AT the origin that it does not exist, and the rate limiter below is what
        keeps the approach sane. */
    inline constexpr float kAxisMinRadius = 0.01f;   // metres

    /** How fast the automatic axis may turn: 360 °/s, which is 7.2° per 50 Hz
        tick. The tangential bearing turns at v/r rad/s, so at a stage-realistic
        1 m/s this only engages inside about 16 cm of the origin — everywhere
        else followAxis() assigns the live axis bit-exactly and the limiter
        costs nothing. */
    inline constexpr float kAxisMaxRateDegPerSec = 360.0f;

    /** Per-tick anchor movement above which the source is treated as having
        teleported (project load, snapshot recall, a flung drag at minimum zoom)
        and the axis snaps instead of gliding. 1 m at 50 Hz is 50 m/s, far above
        anything a moving source does. */
    inline constexpr float kAxisTeleportMetres = 1.0f;

    /** The automatic spread axis for an anchor at (anchorX, anchorY):
        perpendicular to the origin→anchor bearing, oriented so azimuth +1 lands
        audience-right for an upstage anchor.

        Handedness falls out of the geometry instead of from a sign rule: a
        channel mirrored by inputFlipX/Y mirrors its anchor, the mirrored
        bearing carries the perpendicular with it, and the image mirrors along
        with no explicit azimuth negation anywhere.

        RATE LIMIT — the reason this function takes a reference and a dt. The
        bearing sweeps 180° over a few centimetres near the origin, so a source
        crossing the middle of an in-the-round rig would whip its legs around
        and swap left for right. This used to be handled by holding the last
        axis computed outside a 1 m disc, which traded the whip for something
        worse: a source that drove THROUGH the disc entered at one bearing and
        left at the opposite one, so the axis stopped following on the way in
        and snapped through up to 180° at the exit boundary. The old comment
        claimed the handover was continuous by construction; that only held for
        a radial approach and retreat, and the default stage origin sits at
        downstage centre (originDepth = -stageDepth/2), which put the disc — and
        so the snap — right at the front edge of the stage.

        Capping the angular rate removes the discontinuity instead of relocating
        it. Outside the pathological radius the target is reached in one step and
        assigned exactly, so the axis is bit-identical to a build with no limiter
        at all; near the origin it sweeps at kAxisMaxRateDegPerSec instead of
        jumping. Do not reintroduce a freeze band on top of this: a held axis is
        a discontinuity waiting for an exit. */
    inline Axis followAxis (float anchorX, float anchorY,
                            AxisFollower& state, float dtSeconds) noexcept
    {
        const float r = std::hypot (anchorX, anchorY);

        if (r < kAxisMinRadius)
            return state.axis;      // no bearing exists — hold, do not re-baseline

        const Axis target { anchorY / r, -anchorX / r };

        const float moved = std::hypot (anchorX - state.lastX, anchorY - state.lastY);

        state.lastX = anchorX;
        state.lastY = anchorY;

        // Snap: nothing to glide from, or the source did not travel here.
        if (! state.primed || moved > kAxisTeleportMetres)
        {
            state.axis   = target;
            state.primed = true;
            return state.axis;
        }

        // Shortest signed rotation from the current axis to the target. atan2 of
        // (cross, dot) is already wrapped into ±pi, so there is no unwrap
        // bookkeeping and no ±180° boundary to cross.
        const float dot   = state.axis.x * target.x + state.axis.y * target.y;
        const float cross = state.axis.x * target.y - state.axis.y * target.x;
        const float delta = std::atan2 (cross, dot);

        const float maxStep = juce::degreesToRadians (kAxisMaxRateDegPerSec)
                              * juce::jlimit (0.0f, 0.1f, dtSeconds);

        if (std::abs (delta) <= maxStep)
        {
            // The common case, and it must assign rather than rotate: routing an
            // already-reached target through a cos/sin pair would leave the axis
            // a few ULPs off the value a build without the limiter computes, and
            // the width-0 null and the width-invariance test both depend on it
            // being the same float.
            state.axis   = target;
            state.primed = true;
            return state.axis;
        }

        const float step = std::copysign (maxStep, delta);
        const float c    = std::cos (step);
        const float s    = std::sin (step);

        Axis turned { state.axis.x * c - state.axis.y * s,
                      state.axis.x * s + state.axis.y * c };

        // Renormalise: this branch runs tick after tick while a source crosses
        // the origin, and chained rotations drift off unit length.
        const float len = std::hypot (turned.x, turned.y);
        if (len > 0.0f)
            turned = { turned.x / len, turned.y / len };

        state.axis   = turned;
        state.primed = true;
        return state.axis;
    }

    /** The axis for a channel whose orientation is locked (inputStereoAxisLock):
        the fixed world axis, house-left/right, with no tangential term at all,
        so inputStereoAxisOffset reads as an absolute bearing rather than a
        rotation applied to something that moves.

        The follower is pinned to the same value rather than left to go stale,
        so unlocking glides back onto the live tangential axis under the rate
        limit instead of snapping to wherever the source has since travelled. */
    inline Axis lockedAxis (float anchorX, float anchorY, AxisFollower& state) noexcept
    {
        state.axis   = Axis {};     // {1, 0}
        state.lastX  = anchorX;
        state.lastY  = anchorY;
        state.primed = true;
        return state.axis;
    }

    /** Rotates an axis by offsetDegrees, positive counter-clockwise viewed from
        above. 0 leaves the automatic axis alone; ±180 is an explicit L/R swap.

        The zero case returns its argument bit-exactly rather than going through
        the rotation. Relying on cos/sin to hand back exactly 1 and 0, and on
        the products not turning a zero's sign over, would leave the automatic
        path a few ULPs away from a build without an axis offset at all — enough
        to cost the width-0 null its exactness for a feature that was not even
        engaged. */
    inline Axis rotate (Axis axis, float offsetDegrees) noexcept
    {
        if (offsetDegrees == 0.0f)
            return axis;

        const float a = juce::degreesToRadians (offsetDegrees);
        const float c = std::cos (a);
        const float s = std::sin (a);

        return { axis.x * c - axis.y * s,
                 axis.x * s + axis.y * c };
    }

    /** Metre offset from the pair's centre for one decomposed slice.

        widthMetres is the FULL left-to-right distance, so each leg travels half
        of it: azimuth ±1 at 4 m puts the legs 2 m either side of the centre,
        4 m apart from each other.

        Slice 0 carries azimuth 0 by contract — the decomposer anchors the
        centre slice there — so the anchor row's offset is exactly zero and the
        position the user placed never moves out from under them.

        Width 0 yields ±0.0f. The sign is not a defect to tidy away: -0.0f
        compares equal to 0.0f, so the engine's change detection does not see a
        moving source and the collapsed pair stays bit-identical to a point
        source. An std::abs here would buy nothing and risks being "improved"
        into a snap-to-zero that does cost that. */
    inline void sliceOffset (float azimuth, float widthMetres, Axis axis,
                             float& outX, float& outY) noexcept
    {
        const float d = azimuth * (widthMetres * 0.5f);

        outX = d * axis.x;
        outY = d * axis.y;
    }
}
