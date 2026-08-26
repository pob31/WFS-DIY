#pragma once

#include <JuceHeader.h>
#include <cmath>

/*
    BinauralListenerGeometry — where the binaural listener sits and which way
    they face.

    The listener is placed in POLAR terms: binauralListenerDistance is the
    radius, binauralListenerAngle ("Orbit") is the bearing, and the head is
    oriented to face the stage origin from wherever that lands. Orbit is
    therefore a seat-placement control, not a head rotation — turning it walks
    the listener around a circle rather than turning them on the spot, and the
    origin stays dead ahead at every setting. Head rotation is the separate
    yaw/pitch/roll offset, applied on top of this baseline.

    binauralListenerX is a LATERAL offset, so it displaces along the listener's
    own right-ear axis rather than along world X. The two coincide only at
    Orbit 0, which is why this used to be written as a bare "+ lateralX" and
    went unnoticed: at Orbit 90 that moved the listener fore/aft instead of
    sideways, and at 180 it moved them to their left.

    Two callers need this mapping — the 50 Hz snapshot that feeds the render
    worker (BinauralCalculationEngine::refreshRtSnapshot) and the Map tab that
    draws the listener glyph — and they must agree exactly, for the same reason
    StereoImageGeometry gives: a Map drawing a listener the renderer is not
    rendering is worse than drawing no listener at all. That is why the mapping
    lives here and not in either caller.

    Coordinates are stage metres as the Map tab uses them (+X stage right,
    +Y upstage). Angles are degrees, positive counter-clockwise viewed from
    above — the same convention as inputRotation and outputOrientation.
*/
namespace WFSBinauralListener
{
    /** Seat position in stage metres.

        Orbit 0 seats the listener at (0, -distance): downstage of the origin,
        on the audience side, which is the BOTTOM of the Map's plan view. */
    inline juce::Point<float> seatPosition (float distanceM, float orbitDeg, float lateralXM) noexcept
    {
        const float a = juce::degreesToRadians (orbitDeg);
        const float sa = std::sin (a), ca = std::cos (a);

        // (sa, -ca) is the radial direction; (ca, sa) is the listener's
        // right-ear axis at that seat (see spatcore/binaural/HeadFrame.h's
        // R_baseline, whose first column is exactly (cos a, sin a)).
        return { distanceM * sa + lateralXM * ca,
                 -distanceM * ca + lateralXM * sa };
    }

    /** Which way the head points, in the app's orientation convention
        (angle t faces direction (sin t, -cos t); t = 0 faces the audience).

        With yaw 0 the listener faces the origin, from any seat. Yaw is
        seat-relative and positive means turning RIGHT, which is clockwise
        from above — hence the subtraction against a counter-clockwise-positive
        angle convention. */
    inline float facingDegrees (float orbitDeg, float yawDeg) noexcept
    {
        return orbitDeg + 180.0f - yawDeg;
    }

    /** Unit facing vector in stage metres, for drawing. */
    inline juce::Point<float> facingVector (float orbitDeg, float yawDeg) noexcept
    {
        const float t = juce::degreesToRadians (facingDegrees (orbitDeg, yawDeg));
        return { std::sin (t), -std::cos (t) };
    }
}
