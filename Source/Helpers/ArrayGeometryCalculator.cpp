#include "ArrayGeometryCalculator.h"

namespace ArrayGeometry
{

//==============================================================================
// Helper Functions
//==============================================================================

float normalizeAngle(float degrees)
{
    while (degrees > 180.0f)
        degrees -= 360.0f;
    while (degrees < -180.0f)
        degrees += 360.0f;
    return degrees;
}

float calculateOrientationToward(float speakerX, float speakerY,
                                 float targetX, float targetY)
{
    float dx = targetX - speakerX;
    float dy = targetY - speakerY;

    // In our coordinate system:
    // - 0 degrees = facing audience (toward negative Y)
    // - 90 degrees = facing right (toward positive X)
    // - 180/-180 degrees = facing back of stage (toward positive Y)
    // Using atan2(-dx, -dy) so that 0° points toward -Y, clockwise positive
    float angleRad = std::atan2(-dx, -dy);  // 0° is toward -Y (audience), clockwise positive
    float angleDeg = juce::radiansToDegrees(angleRad);

    return normalizeAngle(angleDeg);
}

//==============================================================================
// Straight Line Arrays
//==============================================================================

std::vector<SpeakerPosition> calculateStraightFromCenter(
    int numSpeakers,
    float centerX,
    float centerY,
    float z,
    float spacing,
    float orientation)
{
    std::vector<SpeakerPosition> positions;

    if (numSpeakers <= 0)
        return positions;

    if (numSpeakers == 1)
    {
        positions.push_back({ centerX, centerY, z, orientation });
        return positions;
    }

    // Calculate total width and starting position
    float totalWidth = spacing * static_cast<float>(numSpeakers - 1);
    float startX = centerX - totalWidth / 2.0f;

    for (int i = 0; i < numSpeakers; ++i)
    {
        float x = startX + static_cast<float>(i) * spacing;
        positions.push_back({ x, centerY, z, orientation });
    }

    return positions;
}

std::vector<SpeakerPosition> calculateStraightFromEndpoints(
    int numSpeakers,
    float startX,
    float startY,
    float endX,
    float endY,
    float z,
    float orientation)
{
    std::vector<SpeakerPosition> positions;

    if (numSpeakers <= 0)
        return positions;

    if (numSpeakers == 1)
    {
        // Single speaker at midpoint
        float midX = (startX + endX) / 2.0f;
        float midY = (startY + endY) / 2.0f;
        positions.push_back({ midX, midY, z, orientation });
        return positions;
    }

    // If orientation is -1, calculate perpendicular to line
    float actualOrientation = orientation;
    if (orientation < -180.0f)  // Using -999 or similar as "auto" indicator
    {
        // Calculate perpendicular orientation (facing away from line direction)
        float dx = endX - startX;
        float dy = endY - startY;
        // Perpendicular direction (rotate 90 degrees)
        // For a line going left-to-right, we want speakers facing toward -Y (audience)
        actualOrientation = juce::radiansToDegrees(std::atan2(-dy, dx));
        actualOrientation = normalizeAngle(actualOrientation - 90.0f);
    }

    // Distribute speakers evenly between endpoints
    for (int i = 0; i < numSpeakers; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(numSpeakers - 1);
        float x = startX + t * (endX - startX);
        float y = startY + t * (endY - startY);
        positions.push_back({ x, y, z, actualOrientation });
    }

    return positions;
}

//==============================================================================
// Curved Arrays
//==============================================================================

std::vector<SpeakerPosition> calculateCurvedArray(
    int numSpeakers,
    float startX,
    float startY,
    float endX,
    float endY,
    float sag,
    float z)
{
    std::vector<SpeakerPosition> positions;

    if (numSpeakers <= 0)
        return positions;

    // Calculate midpoint
    float midX = (startX + endX) / 2.0f;
    float midY = (startY + endY) / 2.0f;

    // Calculate perpendicular direction
    float dx = endX - startX;
    float dy = endY - startY;
    float length = std::sqrt(dx * dx + dy * dy);

    if (length < 0.001f)
    {
        // Degenerate case: start and end are the same point
        for (int i = 0; i < numSpeakers; ++i)
            positions.push_back({ startX, startY, z, 0.0f });
        return positions;
    }

    // Normalized perpendicular vector (rotated 90 degrees counter-clockwise)
    // This makes positive sag bow toward negative Y (toward audience)
    float perpX = -dy / length;
    float perpY = dx / length;

    // Control point for quadratic Bezier
    float ctrlX = midX + perpX * sag;
    float ctrlY = midY + perpY * sag;

    // Calculate positions along quadratic Bezier curve
    for (int i = 0; i < numSpeakers; ++i)
    {
        float t = (numSpeakers > 1) ? static_cast<float>(i) / static_cast<float>(numSpeakers - 1) : 0.5f;

        // Quadratic Bezier: B(t) = (1-t)^2 * P0 + 2*(1-t)*t * P1 + t^2 * P2
        float oneMinusT = 1.0f - t;
        float x = oneMinusT * oneMinusT * startX + 2.0f * oneMinusT * t * ctrlX + t * t * endX;
        float y = oneMinusT * oneMinusT * startY + 2.0f * oneMinusT * t * ctrlY + t * t * endY;

        // For fanning: each speaker should point toward the audience
        // In our coordinate system, 0° = facing audience (toward -Y)
        // Calculate angle from speaker position toward audience (toward -Y direction)
        // The fan angle is based on the speaker's X position relative to center
        // Speakers on the left should angle slightly right, speakers on the right should angle slightly left

        // Calculate the normal to the curve at this point (perpendicular to tangent)
        // B'(t) = 2*(1-t)*(P1-P0) + 2*t*(P2-P1)
        float tangentX = 2.0f * oneMinusT * (ctrlX - startX) + 2.0f * t * (endX - ctrlX);
        float tangentY = 2.0f * oneMinusT * (ctrlY - startY) + 2.0f * t * (endY - ctrlY);

        // Normal vector (perpendicular to tangent, pointing toward audience)
        // Both cases should point toward the audience (-Y direction)
        // The normal is perpendicular to tangent, we choose the one pointing toward -Y
        float normalX = tangentY;
        float normalY = -tangentX;

        // Check if normal points toward audience (negative Y direction)
        // If not, flip it
        if (normalY > 0)
        {
            normalX = -normalX;
            normalY = -normalY;
        }

        // Calculate orientation:
        // 0° = facing audience (toward -Y), 180° = facing back of stage (toward +Y)
        // Using atan2(-x, -y) so that 0° points toward -Y, clockwise positive
        float orientation = juce::radiansToDegrees(std::atan2(-normalX, -normalY));
        orientation = normalizeAngle(orientation);

        positions.push_back({ x, y, z, orientation });
    }

    return positions;
}

//==============================================================================
// Circular Arrays
//==============================================================================

std::vector<SpeakerPosition> calculateCircleArray(
    int numSpeakers,
    float centerX,
    float centerY,
    float radius,
    float startAngle,
    float z,
    bool facingInward)
{
    std::vector<SpeakerPosition> positions;

    if (numSpeakers <= 0)
        return positions;

    float angleStep = 360.0f / static_cast<float>(numSpeakers);

    for (int i = 0; i < numSpeakers; ++i)
    {
        float angle = startAngle + static_cast<float>(i) * angleStep;
        float angleRad = juce::degreesToRadians(angle);

        // Position on circle
        // In our convention: 0° = top (negative Y direction from center)
        // So we use sin for X and -cos for Y
        float x = centerX + radius * std::sin(angleRad);
        float y = centerY - radius * std::cos(angleRad);

        // Orientation: facing inward means toward center, outward means away
        // In our system, 0° = facing audience (-Y), 180° = facing back (+Y)
        float orientation;
        if (facingInward)
        {
            // Face toward center
            orientation = calculateOrientationToward(x, y, centerX, centerY);
        }
        else
        {
            // Face away from center
            orientation = calculateOrientationToward(centerX, centerY, x, y);
        }

        positions.push_back({ x, y, z, orientation });
    }

    return positions;
}

//==============================================================================
// Surround Arrays
//==============================================================================

std::vector<SpeakerPosition> calculateSurroundPairs(
    int numPairs,
    float centerX,
    float width,
    float yStart,
    float yEnd,
    float z)
{
    std::vector<SpeakerPosition> positions;

    if (numPairs <= 0)
        return positions;

    for (int i = 0; i < numPairs; ++i)
    {
        float t = (numPairs > 1) ? static_cast<float>(i) / static_cast<float>(numPairs - 1) : 0.5f;
        float y = yStart + t * (yEnd - yStart);

        // Left speaker (facing right, toward center)
        // -90° = facing toward positive X (right)
        positions.push_back({ centerX - width, y, z, -90.0f });

        // Right speaker (facing left, toward center)
        // 90° = facing toward negative X (left)
        positions.push_back({ centerX + width, y, z, 90.0f });
    }

    return positions;
}

} // namespace ArrayGeometry
