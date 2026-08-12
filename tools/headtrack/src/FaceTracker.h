#pragma once

#include <opencv2/core.hpp>
#include <opencv2/objdetect.hpp>
#include <string>
#include <cmath>

namespace wfs::headtrack
{

/**
    Face detection + head-pose estimation.

    YuNet (OpenCV Zoo, MIT) returns a face box plus five landmarks; the head
    angles are computed geometrically from the three robust ones (eye centers
    + nose tip) with the axes decoupled by construction — see
    anglesFromLandmarks for the reasoning (and why this is NOT a solvePnP).
    A dense-mesh landmark model would sharpen absolute accuracy; the plugin
    ABI exists so that swap needs no app change.

    Not thread-safe: one instance belongs to the inference thread.
*/
class FaceTracker
{
public:
    struct Pose
    {
        bool  valid = false;
        float yawRad = 0.0f, pitchRad = 0.0f, rollRad = 0.0f;
        float confidence = 0.0f;
    };

    /** Load the detector. Returns false and fills lastError on failure. */
    bool load (const std::string& modelPath, int width, int height)
    {
        try
        {
            detector = cv::FaceDetectorYN::create (modelPath, "", cv::Size (width, height),
                                                   kScoreThreshold, kNmsThreshold, kTopK);
        }
        catch (const cv::Exception& e)
        {
            lastError = std::string ("Could not load face detector model: ") + e.what();
            return false;
        }

        if (detector == nullptr)
        {
            lastError = "Could not create face detector from " + modelPath;
            return false;
        }

        return true;
    }

    /** Detect and estimate angles on one frame. Never throws. */
    Pose process (const cv::Mat& frame)
    {
        Pose pose;
        if (detector == nullptr || frame.empty())
            return pose;

        try
        {
            if (frame.cols != lastWidth || frame.rows != lastHeight)
            {
                detector->setInputSize (cv::Size (frame.cols, frame.rows));
                lastWidth = frame.cols;
                lastHeight = frame.rows;
            }

            cv::Mat faces;
            detector->detect (frame, faces);
            if (faces.rows < 1)
                return pose;

            // Rows are score-sorted; take the most confident face.
            const float* row = faces.ptr<float> (0);
            // Layout: x, y, w, h, then 5 landmark pairs, then score.
            const cv::Point2f rightEye { row[4], row[5] };   // subject's right, image left
            const cv::Point2f leftEye  { row[6], row[7] };
            const cv::Point2f nose     { row[8], row[9] };
            pose.confidence = row[14];

            // valid follows the estimate: a degenerate frame used to leave the
            // angles at their defaults and still report valid, snapping the
            // listener's head to "straight ahead" instead of saying no face.
            pose.valid = anglesFromLandmarks (rightEye, leftEye, nose, pose);
        }
        catch (const cv::Exception&)
        {
            // Detection hiccup: report "no face" for this frame.
        }

        return pose;
    }

    const std::string& getLastError() const { return lastError; }

private:
    static constexpr float kScoreThreshold = 0.7f;
    static constexpr float kNmsThreshold   = 0.3f;
    static constexpr int   kTopK           = 50;

    /**
        Head angles from the three robust landmarks (eyes + nose tip), with
        the axes DECOUPLED by construction:

          roll  — slope of the eye line, and nothing else. A nod cannot leak
                  into roll.
          yaw   — horizontal offset of the (protruding) nose tip from the
                  mid-eye point, measured in the de-rolled frame.
          pitch — vertical drop of the nose tip below the eye line, ditto.

        This replaced a 5-point solvePnP: the mouth corners are unreliable
        under facial hair, and with only five points that noise aliased across
        axes (a nod read mostly as roll on a bearded face). Eye centers and
        nose tip are YuNet's most stable outputs — glasses shift the eye
        centers roughly symmetrically, and beards don't touch them at all.

        Small-angle geometry of a nose protruding p in front of the eye
        baseline (interocular D): turning by ψ shifts the tip laterally by
        ~p·sin ψ, i.e. sin ψ ≈ (offset/D) / kNoseRatio with kNoseRatio = p/D.
        Absolute gain therefore depends on the wearer's nose (±25 % across
        faces) and the frontal values carry a constant bias — both are
        harmless: Set Zero removes the bias, and the renderer only needs
        relative motion with the right sign.

        The kSign* constants exist because mirroring differs between camera
        drivers — validated with the smoke host (shake / nod / tilt), and the
        only thing to flip if a camera reports mirrored.
    */
    bool anglesFromLandmarks (const cv::Point2f& rightEye, const cv::Point2f& leftEye,
                              const cv::Point2f& nose, Pose& pose) const
    {
        // Landmarks are model output: treat them as untrusted. Note the
        // positive-logic tests throughout — `x < limit` is useless against
        // NaN (every comparison with NaN is false), so a degenerate box would
        // sail past a `< ` guard and emit NaN angles, which downstream become
        // a non-orthonormal rotation and then permanent NaN in the renderer's
        // delay lines and filter state.
        for (const auto& p : { rightEye, leftEye, nose })
            if (! (std::isfinite (p.x) && std::isfinite (p.y)))
                return false;

        const cv::Point2f eyeVec = leftEye - rightEye;      // toward image right when frontal
        const float eyeDist = std::sqrt (eyeVec.x * eyeVec.x + eyeVec.y * eyeVec.y);
        if (! (eyeDist >= kMinEyeDistPx))
            return false;                                    // degenerate detection (or NaN)

        // Roll: eye-line slope. Image y grows downward; right-ear-down tips
        // the subject's right eye (image left) lower → eyeVec.y < 0 → the
        // app-positive roll needs the sign flip.
        const float rollAngle = std::atan2 (eyeVec.y, eyeVec.x);

        // De-roll the nose about the mid-eye point so yaw/pitch read from a
        // level eye line regardless of head tilt.
        const cv::Point2f midEye = (rightEye + leftEye) * 0.5f;
        const float cosR = std::cos (-rollAngle), sinR = std::sin (-rollAngle);
        const cv::Point2f d = nose - midEye;
        const cv::Point2f noseLevel { d.x * cosR - d.y * sinR,
                                      d.x * sinR + d.y * cosR };

        // Turning toward the subject's right moves the nose toward image
        // left (−x); looking up lifts the nose toward the eye line (−y).
        const float xOff = noseLevel.x / eyeDist;
        const float yOff = noseLevel.y / eyeDist;            // ≈ +kNoseDrop frontal (y down)

        const float yaw   = std::asin (clampUnit (-xOff / kNoseRatio));
        const float pitch = std::asin (clampUnit ((kNoseDrop - yOff) / kNoseRatio));
        const float roll  = -rollAngle;

        // The guards above make this unreachable in practice; keep it so the
        // ABI's "angles are finite" promise holds by construction rather than
        // by argument. Reporting no face beats reporting a fabricated one.
        if (! (std::isfinite (yaw) && std::isfinite (pitch) && std::isfinite (roll)))
            return false;

        pose.yawRad   = kSignYaw   * yaw;
        pose.pitchRad = kSignPitch * pitch;
        pose.rollRad  = kSignRoll  * roll;
        return true;
    }

    /** Clamp to asin's domain. NaN deliberately passes through rather than
        being mapped to a value: the caller's finite check then reports "no
        face", instead of this silently inventing an angle. */
    static float clampUnit (float v) noexcept
    {
        return v < -1.0f ? -1.0f : (v > 1.0f ? 1.0f : v);
    }

    // Below this interocular distance (pixels) the landmark geometry is too
    // small for the ratios to mean anything.
    static constexpr float kMinEyeDistPx = 1.0f;

    // Anthropometric ratios (relative to interocular distance): nose-tip
    // protrusion and its frontal drop below the eye line. Averages; per-face
    // deviation only scales gain / biases the zero, see anglesFromLandmarks.
    static constexpr float kNoseRatio = 0.45f;
    static constexpr float kNoseDrop  = 0.55f;

    static constexpr float kSignYaw   = 1.0f;
    static constexpr float kSignPitch = 1.0f;
    static constexpr float kSignRoll  = 1.0f;

    cv::Ptr<cv::FaceDetectorYN> detector;
    int lastWidth = 0, lastHeight = 0;
    std::string lastError;
};

} // namespace wfs::headtrack
