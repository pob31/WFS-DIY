#pragma once

#include <JuceHeader.h>
#include <vector>
#include <cmath>

/**
 * Represents a calculated speaker position with orientation
 */
struct SpeakerPosition
{
    float x;           // X position in meters
    float y;           // Y position in meters
    float z;           // Z position in meters (height)
    float orientation; // Orientation in degrees (0 = facing audience/-Y, 180 = facing back/+Y)
};

/**
 * Array geometry calculation functions for speaker positioning
 */
namespace ArrayGeometry
{
    //==========================================================================
    // Straight Line Arrays
    //==========================================================================

    /**
     * Calculate positions for a straight line from center point with spacing.
     * Speakers are distributed evenly along X axis centered at (centerX, centerY).
     *
     * @param numSpeakers Number of speakers to place
     * @param centerX Center X position in meters
     * @param centerY Center Y position in meters
     * @param z Height in meters
     * @param spacing Distance between speakers in meters
     * @param orientation Orientation for all speakers in degrees
     * @return Vector of calculated speaker positions
     */
    std::vector<SpeakerPosition> calculateStraightFromCenter(
        int numSpeakers,
        float centerX,
        float centerY,
        float z,
        float spacing,
        float orientation);

    /**
     * Calculate positions for a straight line between two endpoints.
     * Speakers are distributed evenly from start to end point.
     *
     * @param numSpeakers Number of speakers to place
     * @param startX Start X position in meters
     * @param startY Start Y position in meters
     * @param endX End X position in meters
     * @param endY End Y position in meters
     * @param z Height in meters
     * @param orientation Orientation for all speakers in degrees (or -1 for auto-perpendicular)
     * @return Vector of calculated speaker positions
     */
    std::vector<SpeakerPosition> calculateStraightFromEndpoints(
        int numSpeakers,
        float startX,
        float startY,
        float endX,
        float endY,
        float z,
        float orientation);

    //==========================================================================
    // Curved Arrays
    //==========================================================================

    /**
     * Calculate positions for a curved array (arc) with specified sag.
     * Uses quadratic Bezier curve with sag determining the control point offset.
     * Orientations are calculated perpendicular to the curve (facing outward).
     *
     * @param numSpeakers Number of speakers to place
     * @param startX Start X position in meters
     * @param startY Start Y position in meters
     * @param endX End X position in meters
     * @param endY End Y position in meters
     * @param sag Perpendicular distance from midpoint to chord (positive = toward audience)
     * @param z Height in meters
     * @return Vector of calculated speaker positions
     */
    std::vector<SpeakerPosition> calculateCurvedArray(
        int numSpeakers,
        float startX,
        float startY,
        float endX,
        float endY,
        float sag,
        float z);

    //==========================================================================
    // Circular Arrays
    //==========================================================================

    /**
     * Calculate positions for a circular array.
     * Speakers are distributed evenly around the circle.
     *
     * @param numSpeakers Number of speakers to place (must be > 0)
     * @param centerX Center X position in meters
     * @param centerY Center Y position in meters
     * @param radius Radius of the circle in meters
     * @param startAngle Starting angle in degrees (0 = top, clockwise)
     * @param z Height in meters
     * @param facingInward If true, speakers face toward center; if false, face outward
     * @return Vector of calculated speaker positions
     */
    std::vector<SpeakerPosition> calculateCircleArray(
        int numSpeakers,
        float centerX,
        float centerY,
        float radius,
        float startAngle,
        float z,
        bool facingInward);

    //==========================================================================
    // Surround Arrays
    //==========================================================================

    /**
     * Calculate positions for surround pairs (left/right mirrored speakers).
     * Creates matching pairs on left and right sides facing inward.
     *
     * @param numPairs Number of speaker pairs to place
     * @param centerX Center X position (speakers will be at centerX +/- width)
     * @param width Distance from center to each side in meters
     * @param yStart Y position of first pair in meters
     * @param yEnd Y position of last pair in meters
     * @param z Height in meters
     * @return Vector of calculated speaker positions (left, right, left, right, ...)
     */
    std::vector<SpeakerPosition> calculateSurroundPairs(
        int numPairs,
        float centerX,
        float width,
        float yStart,
        float yEnd,
        float z);

    //==========================================================================
    // Helper Functions
    //==========================================================================

    /**
     * Calculate orientation from speaker position toward a target point.
     * Returns angle in degrees where 0 = facing audience (toward negative Y),
     * 90 = facing right (toward positive X), 180 = facing back of stage (toward positive Y).
     *
     * @param speakerX Speaker X position
     * @param speakerY Speaker Y position
     * @param targetX Target X position
     * @param targetY Target Y position
     * @return Orientation in degrees (-180 to 180)
     */
    float calculateOrientationToward(float speakerX, float speakerY,
                                     float targetX, float targetY);

    /**
     * Normalize angle to -180 to 180 degrees range.
     *
     * @param degrees Input angle in degrees
     * @return Normalized angle in degrees
     */
    float normalizeAngle(float degrees);

} // namespace ArrayGeometry
