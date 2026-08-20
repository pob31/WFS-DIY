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
        freshly constructed latch already describes a house-left/right pair. */
    struct Axis
    {
        float x = 1.0f, y = 0.0f;
    };

    /** Anchor distance from the origin inside which the automatic axis is held
        instead of recomputed. */
    inline constexpr float kAxisFreezeRadius = 1.0f;   // metres

    /** The automatic spread axis for an anchor at (anchorX, anchorY):
        perpendicular to the origin→anchor bearing, oriented so azimuth +1 lands
        audience-right for an upstage anchor.

        Handedness falls out of the geometry instead of from a sign rule: a
        channel mirrored by inputFlipX/Y mirrors its anchor, the mirrored
        bearing carries the perpendicular with it, and the image mirrors along
        with no explicit azimuth negation anywhere.

        LATCH — the reason this function takes a reference. Within
        kAxisFreezeRadius of the origin the last axis computed outside it is
        returned and the latch is left alone. The bearing is perfectly well
        defined at 10 cm out; the problem is that it sweeps 180° over a few
        centimetres, so a source crossing the middle of an in-the-round rig —
        routine there, not a corner case — would whip its legs around and swap
        left for right. Holding the axis the source arrived with is the fix.

        No hysteresis band is wanted and none should be added later: at exactly
        r == kAxisFreezeRadius the held value IS what the live branch computes,
        so the handover is continuous in both directions by construction, and a
        second threshold would only introduce a discontinuity to guard against.

        The caller owns the latch and default-initialises it to {1,0}, so a pair
        that has never been outside the freeze radius still spreads
        house-left/right and stays re-orientable through rotate(). */
    inline Axis autoAxis (float anchorX, float anchorY, Axis& latch) noexcept
    {
        const float r = std::hypot (anchorX, anchorY);

        if (r < kAxisFreezeRadius)
            return latch;

        latch = { anchorY / r, -anchorX / r };
        return latch;
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
