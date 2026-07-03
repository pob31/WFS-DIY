#pragma once

#include <JuceHeader.h>
#include <cmath>
#include <algorithm>
#include <vector>
#include <array>
#include "../../../../spatcore/control/mcp/MCPToolRegistry.h"
#include "../../../../spatcore/control/mcp/MCPChangeRecords.h"
#include "../../../Parameters/WFSValueTreeState.h"
#include "../../../Parameters/WFSParameterIDs.h"

namespace WFSNetwork::Tools::ReverbAutoLayout
{

namespace detail
{
    // --- Constants ---------------------------------------------------------

    inline constexpr double kStandoffMean         = 5.0;   // m, halo
    inline constexpr double kStandoffJitterSDN    = 1.0;   // m
    inline constexpr double kStandoffJitterReg    = 0.2;   // m
    inline constexpr double kSurroundOffset       = 4.0;   // m, outside speaker ring
    inline constexpr double kDomeOffset           = 4.0;   // m, outside shell
    inline constexpr double kOrientationJitterDeg = 5.0;
    inline constexpr double kPitchJitterDeg       = 3.0;
    inline constexpr double kZMin                 = 2.0;
    inline constexpr double kZMax                 = 3.0;
    inline constexpr double kAdjacentStandoffGuard = 0.4;
    inline constexpr double kVenueWarnRadius      = 50.0;
    inline constexpr double kPi                   = 3.14159265358979323846;

    // --- Data types --------------------------------------------------------

    struct OutputPoint
    {
        double x = 0.0, y = 0.0, z = 0.0;
        bool   atDefault = false;   // all three are 0
    };

    enum class Algorithm { SDN = 0, FDN = 1, IR = 2 };

    struct SceneSnapshot
    {
        // Stage
        int    stageShape       = 0;     // 0=box, 1=cyl, 2=dome
        double stageWidth       = 20.0;
        double stageDepth       = 10.0;
        double stageHeight      = 5.0;
        double stageDiameter    = 20.0;
        double domeElevationDeg = 180.0;
        double originWidth      = 0.0;
        double originDepth      = 0.0;
        double originHeight     = 0.0;

        // Outputs
        std::vector<OutputPoint> outputs;
        bool   allOutputsAtDefault = true;

        // Listener (always origin in v1)
        double listenerX = 0.0, listenerY = 0.0, listenerZ = 0.0;

        // Reverbs
        int       numReverbs = 0;
        Algorithm algorithm  = Algorithm::SDN;
        bool      mixedAlgorithms = false;     // always false in v1
    };

    struct Descriptors
    {
        double centroidX = 0.0, centroidY = 0.0, centroidZ = 0.0;
        double spreadXY  = 0.0;                // std deviation of distance from centroid
        double extentX   = 0.0;
        double extentY   = 0.0;
        double extentZ   = 0.0;
        double listenerToCentroidXY = 0.0;
        std::array<int, 4> quadrantCountsListener { 0, 0, 0, 0 };
                                               // [FL, FR, BL, BR] relative to listener
                                               // F = -Y (audience side), B = +Y (upstage)
        std::array<int, 4> quadrantCountsStage { 0, 0, 0, 0 };
                                               // same but relative to stage center
        int    quadrantsCovered = 0;
        double meanRadiusXY     = 0.0;
        double radialSpreadXY   = 0.0;
        double meanRadius3D     = 0.0;
        double radialSpread3D   = 0.0;
        double heightSpread     = 0.0;
        double zMin = 0.0, zMax = 0.0;
        double audienceSideCount = 0;          // outputs at y_stage < stage_center_y
    };

    struct LayoutNode
    {
        double x = 0.0, y = 0.0, z = 0.0;
        double orientationDeg = 0.0;
        double pitchDeg       = 0.0;
        bool   writePitch     = false;
    };

    struct Layout
    {
        std::vector<LayoutNode> nodes;
        juce::String standoffDescription;
        juce::String variantId;   // internal variant name (e.g. stage_halo_behind_frontal_array)
    };

    // --- Small utilities ---------------------------------------------------

    inline double clampd (double v, double lo, double hi)
    {
        return v < lo ? lo : (v > hi ? hi : v);
    }

    inline double wrapDeg (double d)
    {
        while (d >  180.0) d -= 360.0;
        while (d <= -180.0) d += 360.0;
        return d;
    }

    inline double degrees (double rad) { return rad * (180.0 / kPi); }
    inline double radians (double deg) { return deg * (kPi / 180.0); }

    /** Project's angle convention: 0deg = audience (-Y), +90 = stage right (+X). */
    inline double orientationToTarget (double xNode, double yNode,
                                         double xTarget = 0.0, double yTarget = 0.0)
    {
        const double dx = xNode - xTarget;
        const double dy = yNode - yTarget;
        return wrapDeg (degrees (std::atan2 (dx, -dy)));
    }

    inline double pitchToTarget (double xNode, double yNode, double zNode,
                                   double xTarget, double yTarget, double zTarget)
    {
        const double dx = xNode - xTarget;
        const double dy = yNode - yTarget;
        const double dz = zNode - zTarget;
        const double r  = std::sqrt (dx * dx + dy * dy);
        return wrapDeg (degrees (std::atan2 (dz, r)));
    }

    inline double randUniform (juce::Random& rng, double lo, double hi)
    {
        return lo + (hi - lo) * rng.nextDouble();
    }

    /** Vogel / golden-angle spiral on the upper hemisphere.
        Returns unit vector (x, y, z) with z >= 0. */
    inline void vogelHemispherePoint (int i, int n, double startOffset,
                                        double& outX, double& outY, double& outZ)
    {
        // Distribute n points on hemisphere: z = (i+0.5)/n, theta = i * goldenAngle + start.
        const double goldenAngle = kPi * (3.0 - std::sqrt (5.0));
        const double z   = (static_cast<double> (i) + 0.5) / static_cast<double> (n);
        const double r   = std::sqrt (1.0 - z * z);
        const double th  = static_cast<double> (i) * goldenAngle + startOffset;
        outX = r * std::cos (th);
        outY = r * std::sin (th);
        outZ = z;
    }

    /** Sample N standoff offsets from Uniform(-mag, +mag), then ensure no
        adjacent pair is within `kAdjacentStandoffGuard` of each other. The
        path is `cyclic` for ring/dome, linear (open ends) for frontal halo. */
    inline void applyNonClusteringStandoff (std::vector<double>& offsets,
                                              juce::Random& rng,
                                              double magnitude,
                                              bool   cyclic)
    {
        const int n = static_cast<int> (offsets.size());
        for (int i = 0; i < n; ++i)
            offsets[(size_t) i] = randUniform (rng, -magnitude, +magnitude);

        if (n < 2 || magnitude <= 0.0)
            return;

        // Up to 20 passes to break clusters. Each pass walks neighbours.
        for (int pass = 0; pass < 20; ++pass)
        {
            bool changed = false;
            for (int i = 0; i < n; ++i)
            {
                const int j = (i + 1) % n;
                if (! cyclic && j == 0)
                    continue;
                if (std::abs (offsets[(size_t) i] - offsets[(size_t) j]) < kAdjacentStandoffGuard)
                {
                    offsets[(size_t) j] = randUniform (rng, -magnitude, +magnitude);
                    changed = true;
                }
            }
            if (! changed)
                break;
        }
    }

    // --- Scene reader ------------------------------------------------------

    inline SceneSnapshot readScene (WFSValueTreeState& state)
    {
        SceneSnapshot s;

        s.stageShape       = static_cast<int>    (state.getParameter (WFSParameterIDs::stageShape,    -1));
        s.stageWidth       = static_cast<double> (state.getParameter (WFSParameterIDs::stageWidth,    -1));
        s.stageDepth       = static_cast<double> (state.getParameter (WFSParameterIDs::stageDepth,    -1));
        s.stageHeight      = static_cast<double> (state.getParameter (WFSParameterIDs::stageHeight,   -1));
        s.stageDiameter    = static_cast<double> (state.getParameter (WFSParameterIDs::stageDiameter, -1));
        s.domeElevationDeg = static_cast<double> (state.getParameter (WFSParameterIDs::domeElevation, -1));
        s.originWidth      = static_cast<double> (state.getParameter (WFSParameterIDs::originWidth,   -1));
        s.originDepth      = static_cast<double> (state.getParameter (WFSParameterIDs::originDepth,   -1));
        s.originHeight     = static_cast<double> (state.getParameter (WFSParameterIDs::originHeight,  -1));

        const int numOutputs = state.getNumOutputChannels();
        s.outputs.reserve ((size_t) numOutputs);
        s.allOutputsAtDefault = (numOutputs > 0);
        for (int i = 0; i < numOutputs; ++i)
        {
            OutputPoint p;
            p.x = static_cast<double> (state.getParameter (WFSParameterIDs::outputPositionX, i));
            p.y = static_cast<double> (state.getParameter (WFSParameterIDs::outputPositionY, i));
            p.z = static_cast<double> (state.getParameter (WFSParameterIDs::outputPositionZ, i));
            p.atDefault = (std::abs (p.x) < 1e-6 && std::abs (p.y) < 1e-6 && std::abs (p.z) < 1e-6);
            if (! p.atDefault)
                s.allOutputsAtDefault = false;
            s.outputs.push_back (p);
        }

        s.numReverbs = state.getNumReverbChannels();
        const int algoInt = static_cast<int> (state.getParameter (WFSParameterIDs::reverbAlgoType, -1));
        s.algorithm = (algoInt == 1) ? Algorithm::FDN
                    : (algoInt == 2) ? Algorithm::IR
                                     : Algorithm::SDN;
        s.mixedAlgorithms = false;   // global algo only in v1

        return s;
    }

    // --- Descriptors -------------------------------------------------------

    inline Descriptors computeDescriptors (const SceneSnapshot& scene)
    {
        Descriptors d;
        const int n = static_cast<int> (scene.outputs.size());
        if (n == 0)
            return d;

        // Centroid + extents.
        double minX = scene.outputs[0].x, maxX = minX;
        double minY = scene.outputs[0].y, maxY = minY;
        double minZ = scene.outputs[0].z, maxZ = minZ;
        double sumX = 0.0, sumY = 0.0, sumZ = 0.0;
        for (const auto& p : scene.outputs)
        {
            sumX += p.x; sumY += p.y; sumZ += p.z;
            if (p.x < minX) minX = p.x; if (p.x > maxX) maxX = p.x;
            if (p.y < minY) minY = p.y; if (p.y > maxY) maxY = p.y;
            if (p.z < minZ) minZ = p.z; if (p.z > maxZ) maxZ = p.z;
        }
        d.centroidX = sumX / n;
        d.centroidY = sumY / n;
        d.centroidZ = sumZ / n;
        d.extentX = maxX - minX;
        d.extentY = maxY - minY;
        d.extentZ = maxZ - minZ;
        d.zMin = minZ;
        d.zMax = maxZ;

        // Spread / mean radius (XY around centroid).
        double sumDistXY = 0.0, sumDistXYsq = 0.0;
        double sumDist3D = 0.0, sumDist3Dsq = 0.0;
        for (const auto& p : scene.outputs)
        {
            const double dx = p.x - d.centroidX;
            const double dy = p.y - d.centroidY;
            const double dz = p.z - d.centroidZ;
            const double rXY = std::sqrt (dx * dx + dy * dy);
            const double r3D = std::sqrt (dx * dx + dy * dy + dz * dz);
            sumDistXY += rXY; sumDistXYsq += rXY * rXY;
            sumDist3D += r3D; sumDist3Dsq += r3D * r3D;
        }
        const double meanXY = sumDistXY / n;
        const double meanR3 = sumDist3D / n;
        d.meanRadiusXY    = meanXY;
        d.meanRadius3D    = meanR3;
        d.radialSpreadXY  = std::sqrt (std::max (0.0, sumDistXYsq / n - meanXY * meanXY));
        d.radialSpread3D  = std::sqrt (std::max (0.0, sumDist3Dsq / n - meanR3 * meanR3));
        d.spreadXY        = d.radialSpreadXY;
        const double meanZ = sumZ / n;
        double sumZdev2 = 0.0;
        for (const auto& p : scene.outputs)
            sumZdev2 += (p.z - meanZ) * (p.z - meanZ);
        d.heightSpread = std::sqrt (sumZdev2 / n);

        // Distance from listener (always origin in v1) to centroid.
        const double dxL = d.centroidX - scene.listenerX;
        const double dyL = d.centroidY - scene.listenerY;
        d.listenerToCentroidXY = std::sqrt (dxL * dxL + dyL * dyL);

        // Quadrant counts relative to listener.
        // F = -Y (audience), B = +Y (upstage), L = -X, R = +X.
        for (const auto& p : scene.outputs)
        {
            const double rx = p.x - scene.listenerX;
            const double ry = p.y - scene.listenerY;
            const bool isFront = (ry <= 0.0);
            const bool isRight = (rx >= 0.0);
            int idx = (isFront ? 0 : 2) + (isRight ? 1 : 0);   // FL=0, FR=1, BL=2, BR=3
            d.quadrantCountsListener[(size_t) idx]++;
        }
        d.quadrantsCovered = 0;
        for (int q : d.quadrantCountsListener)
            if (q > 0) ++d.quadrantsCovered;

        // Stage center in world frame: origin offsets are subtracted from
        // stage coords; world-coords of stage center = (originWidth, originDepth)?
        // No - the convention is origin = user-configurable offset moving
        // (0,0,0) relative to stage center. Stage center in world coords =
        // (-originWidth, -originDepth) for box / cyl / dome.
        const double stageCx = -scene.originWidth;
        const double stageCy = -scene.originDepth;

        d.audienceSideCount = 0;
        for (const auto& p : scene.outputs)
        {
            const double rx = p.x - stageCx;
            const double ry = p.y - stageCy;
            // Audience is on -Y side of stage center.
            if (ry < 0.0)
                d.audienceSideCount += 1.0;

            const bool isFront = (ry <= 0.0);
            const bool isRight = (rx >= 0.0);
            int idx = (isFront ? 0 : 2) + (isRight ? 1 : 0);
            d.quadrantCountsStage[(size_t) idx]++;
        }

        return d;
    }

    // --- Topology classification ------------------------------------------

    /** Saturating linear ramp: 0 below `lo`, 1 above `hi`. */
    inline double ramp (double v, double lo, double hi)
    {
        if (v <= lo) return 0.0;
        if (v >= hi) return 1.0;
        return (v - lo) / (hi - lo);
    }

    /** Inverted ramp: 1 below `lo`, 0 above `hi`. */
    inline double rampInv (double v, double lo, double hi)
    {
        if (v <= lo) return 1.0;
        if (v >= hi) return 0.0;
        return 1.0 - (v - lo) / (hi - lo);
    }

    inline double confidenceFrontalArray (const Descriptors& d, const SceneSnapshot& scene)
    {
        const int n = static_cast<int> (scene.outputs.size());
        if (n < 2)
            return 0.0;

        // (a) Outputs concentrated in 1-2 adjacent listener quadrants.
        // Score: fraction of outputs in the two "front" quadrants (FL+FR).
        const int frontCount = d.quadrantCountsListener[0] + d.quadrantCountsListener[1];
        const double frontFrac = static_cast<double> (frontCount) / n;
        const double quadScore = ramp (frontFrac, 0.6, 0.95);

        // (b) Narrow on one axis vs the other.
        const double small = std::min (d.extentX, d.extentY);
        const double large = std::max (d.extentX, d.extentY);
        const double narrowRatio = (large > 1e-6) ? (small / large) : 1.0;
        const double narrowScore = rampInv (narrowRatio, 0.15, 0.5);

        // (c) Radial clustering OK (not too wild).
        const double radialClust = (d.meanRadiusXY > 1e-6)
                                    ? (d.radialSpreadXY / d.meanRadiusXY) : 0.0;
        const double clusterScore = rampInv (radialClust, 0.3, 0.6);

        return std::min (quadScore, std::min (narrowScore, clusterScore));
    }

    inline double confidencePerimeterArray (const Descriptors& d, const SceneSnapshot& scene)
    {
        const int n = static_cast<int> (scene.outputs.size());
        if (n < 4)
            return 0.0;

        // (a) Span >= 3 stage quadrants.
        int covered = 0;
        for (int q : d.quadrantCountsStage)
            if (q > 0) ++covered;
        const double quadScore = (covered >= 3) ? 1.0 : ((covered == 2) ? 0.3 : 0.0);

        // (b) Audience side (downstage of stage center) sparse: <=1 output.
        const double audSideFrac = d.audienceSideCount / n;
        const double openAudScore = rampInv (audSideFrac, 0.15, 0.5);

        // (c) Low height spread (planar).
        const double planarScore = rampInv (d.heightSpread, 1.0, 2.5);

        return std::min (quadScore, std::min (openAudScore, planarScore));
    }

    inline double confidenceSurroundRing (const Descriptors& d, const SceneSnapshot& scene)
    {
        const int n = static_cast<int> (scene.outputs.size());
        if (n < 4)
            return 0.0;

        // (a) All four listener quadrants occupied.
        const double fullCoverage = (d.quadrantsCovered == 4) ? 1.0
                                  : (d.quadrantsCovered == 3 ? 0.4 : 0.0);

        // (b) Tight radial clustering.
        const double radialClust = (d.meanRadiusXY > 1e-6)
                                    ? (d.radialSpreadXY / d.meanRadiusXY) : 1.0;
        const double clusterScore = rampInv (radialClust, 0.3, 0.5);

        // (c) Planar (low height spread).
        const double planarScore = rampInv (d.heightSpread, 1.0, 2.0);

        // (d) Centroid close to listener.
        const double centerScore = (d.meanRadiusXY > 1e-6)
                                    ? rampInv (d.listenerToCentroidXY / d.meanRadiusXY, 0.2, 0.5)
                                    : 1.0;

        return std::min (fullCoverage, std::min (clusterScore, std::min (planarScore, centerScore)));
    }

    inline double confidenceOutwardCluster (const Descriptors& d, const SceneSnapshot& scene)
    {
        const int n = static_cast<int> (scene.outputs.size());
        if (n < 2)
            return 0.0;

        const double tightSpread = rampInv (d.spreadXY, 1.5, 4.0);
        const double nearListener = rampInv (d.listenerToCentroidXY, 1.0, 3.0);
        return std::min (tightSpread, nearListener);
    }

    inline double confidenceDomeShell (const Descriptors& d, const SceneSnapshot& scene)
    {
        const int n = static_cast<int> (scene.outputs.size());
        if (n < 6)
            return 0.0;

        // (a) Significant height spread.
        const double heightScore = ramp (d.heightSpread, 0.8, 1.5);

        // (b) Outputs fit a sphere (small 3D radial spread).
        const double radialFit = (d.meanRadius3D > 1e-6)
                                  ? (d.radialSpread3D / d.meanRadius3D) : 1.0;
        const double fitScore = rampInv (radialFit, 0.2, 0.4);

        // (c) Coverage of upper hemisphere.
        const double zRange = d.zMax - d.zMin;
        const double coverScore = ramp (zRange / std::max (d.meanRadius3D, 1.0), 0.4, 0.8);

        return std::min (heightScore, std::min (fitScore, coverScore));
    }

    struct TopologyResult
    {
        juce::String              topologyId;        // operator-facing enum
        juce::String              internalVariant;   // frontal vs perimeter
        double                    confidence = 0.0;
        bool                      ambiguous  = false;
        juce::Array<juce::var>    candidates;        // populated when ambiguous
    };

    inline juce::String operatorEnumOf (const juce::String& internal)
    {
        if (internal == "stage_halo_behind_frontal_array")    return "stage_halo";
        if (internal == "stage_halo_behind_perimeter_array")  return "stage_halo";
        if (internal == "surround_ring")                       return "surround_ring";
        if (internal == "outer_ring_beyond_audience")          return "outer_ring_beyond_audience";
        if (internal == "dome_shell_behind")                   return "dome_shell_behind";
        return internal;
    }

    inline juce::String meaningOf (const juce::String& topologyId)
    {
        if (topologyId == "stage_halo")
            return "Reverbs sit behind the speaker array on three or four "
                   "sides of the stage; audience face open downstage if the "
                   "speakers leave it open.";
        if (topologyId == "surround_ring")
            return "Surround active-acoustic system; reverbs distributed 360 "
                   "degrees just outside the speaker ring.";
        if (topologyId == "outer_ring_beyond_audience")
            return "Central outward-facing cluster of speakers; reverbs form "
                   "a large ring outside the audience, modeling distant venue "
                   "boundaries.";
        if (topologyId == "dome_shell_behind")
            return "Hemispherical speaker shell; reverbs distributed on a "
                   "larger shell outside, audience underneath.";
        return juce::String();
    }

    inline juce::var buildCandidate (const juce::String& topologyId)
    {
        auto obj = std::make_unique<juce::DynamicObject>();
        obj->setProperty ("topology", topologyId);
        obj->setProperty ("meaning",  meaningOf (topologyId));
        return juce::var (obj.release());
    }

    inline TopologyResult classify (const Descriptors& d, const SceneSnapshot& scene)
    {
        struct ScoreEntry { juce::String internalId; double score; };
        std::vector<ScoreEntry> scores;
        scores.push_back ({ "stage_halo_behind_frontal_array",  confidenceFrontalArray   (d, scene) });
        scores.push_back ({ "stage_halo_behind_perimeter_array", confidencePerimeterArray (d, scene) });
        scores.push_back ({ "surround_ring",                     confidenceSurroundRing   (d, scene) });
        scores.push_back ({ "outer_ring_beyond_audience",        confidenceOutwardCluster (d, scene) });
        scores.push_back ({ "dome_shell_behind",                 confidenceDomeShell      (d, scene) });

        std::sort (scores.begin(), scores.end(),
                   [] (const ScoreEntry& a, const ScoreEntry& b) { return a.score > b.score; });

        TopologyResult result;
        const double topScore  = scores[0].score;
        const double nextScore = scores.size() > 1 ? scores[1].score : 0.0;

        const double kClear      = 0.7;
        const double kWeak       = 0.4;
        const double kAmbigDelta = 0.15;

        // Collect operator-enum candidates, deduping (frontal+perimeter both
        // collapse to stage_halo for the operator-facing enum).
        auto pushCandidate = [&result] (const juce::String& internalId)
        {
            const auto opEnum = operatorEnumOf (internalId);
            for (const auto& existing : result.candidates)
                if (auto* obj = existing.getDynamicObject())
                    if (obj->getProperty ("topology").toString() == opEnum)
                        return;
            result.candidates.add (buildCandidate (opEnum));
        };

        if (topScore >= kClear)
        {
            if (nextScore >= kClear && (topScore - nextScore) < kAmbigDelta)
            {
                // Two strong candidates - but if both collapse to the same
                // operator-facing enum (frontal+perimeter), it's not ambiguous
                // from the operator's point of view; the geometry picks the
                // variant internally.
                const auto opA = operatorEnumOf (scores[0].internalId);
                const auto opB = operatorEnumOf (scores[1].internalId);
                if (opA == opB)
                {
                    result.topologyId      = opA;
                    result.internalVariant = scores[0].internalId;
                    result.confidence      = topScore;
                    return result;
                }
                result.ambiguous = true;
                pushCandidate (scores[0].internalId);
                pushCandidate (scores[1].internalId);
                return result;
            }
            result.topologyId      = operatorEnumOf (scores[0].internalId);
            result.internalVariant = scores[0].internalId;
            result.confidence      = topScore;
            return result;
        }

        // No clear winner; collect everyone with score >= weak threshold.
        result.ambiguous = true;
        for (const auto& s : scores)
            if (s.score >= kWeak)
                pushCandidate (s.internalId);
        // If nothing scores at all, return all five as candidates (very rare).
        if (result.candidates.isEmpty())
            for (const auto& s : scores)
                pushCandidate (s.internalId);
        return result;
    }

    /** When operator passes topology=stage_halo, sub-classify between
        frontal and perimeter to pick the geometry variant. */
    inline juce::String pickStageHaloVariant (const Descriptors& d, const SceneSnapshot& scene)
    {
        const double fr = confidenceFrontalArray  (d, scene);
        const double pe = confidencePerimeterArray (d, scene);
        if (std::abs (fr - pe) < 1e-3)
            return "stage_halo_behind_perimeter_array";   // default to more general
        return (fr > pe) ? "stage_halo_behind_frontal_array"
                         : "stage_halo_behind_perimeter_array";
    }

    // --- Geometry primitives ----------------------------------------------

    /** Compute halo path for stage_halo. For frontal: three sides of stage
        (left, upstage, right) offset behind speakers. For perimeter: four
        sides (audience side included only if speakers cover that side).
        Audience side is -Y of stage center; speaker locus inferred from
        descriptors. */
    inline Layout geomStageHalo (const Descriptors& d, const SceneSnapshot& scene,
                                   int n, bool isSDN, bool isPerimeterVariant,
                                   juce::Random& rng)
    {
        Layout layout;
        layout.variantId = isPerimeterVariant ? "stage_halo_behind_perimeter_array"
                                              : "stage_halo_behind_frontal_array";

        // Build a "halo rectangle" around the speaker bounding box, offset
        // outward by kStandoffMean. For frontal variant, drop the audience
        // side; for perimeter variant, keep all four.
        const double pad = kStandoffMean;
        // Speaker bounding box, padded by standoff.
        double minX = scene.outputs[0].x, maxX = minX;
        double minY = scene.outputs[0].y, maxY = minY;
        for (const auto& p : scene.outputs)
        {
            if (p.x < minX) minX = p.x; if (p.x > maxX) maxX = p.x;
            if (p.y < minY) minY = p.y; if (p.y > maxY) maxY = p.y;
        }
        minX -= pad; maxX += pad;
        minY -= pad; maxY += pad;

        // Build path segments: left (along minX), upstage (along maxY), right (along maxX),
        // and for perimeter, audience-side (along minY).
        struct Segment { double x0, y0, x1, y1; double len; };
        std::vector<Segment> segs;
        auto addSeg = [&segs] (double x0, double y0, double x1, double y1)
        {
            const double dx = x1 - x0, dy = y1 - y0;
            const double L = std::sqrt (dx * dx + dy * dy);
            segs.push_back ({ x0, y0, x1, y1, L });
        };

        if (isPerimeterVariant)
        {
            // Full loop, going CCW: left->upstage->right->audience->close.
            addSeg (minX, minY, minX, maxY);    // left
            addSeg (minX, maxY, maxX, maxY);    // upstage
            addSeg (maxX, maxY, maxX, minY);    // right
            addSeg (maxX, minY, minX, minY);    // audience side
        }
        else
        {
            // Three sides only: left -> upstage -> right.
            addSeg (minX, minY, minX, maxY);    // left
            addSeg (minX, maxY, maxX, maxY);    // upstage
            addSeg (maxX, maxY, maxX, minY);    // right
        }

        double totalLen = 0.0;
        for (const auto& s : segs)
            totalLen += s.len;

        if (n < 1 || totalLen < 1e-6)
            return layout;

        // Even arc-length spacing.
        const double stepLen = totalLen / n;
        std::vector<double> baseS;
        baseS.reserve ((size_t) n);
        for (int i = 0; i < n; ++i)
            baseS.push_back (stepLen * (i + 0.5));

        // Per-node radial standoff offsets.
        std::vector<double> standoffOffsets ((size_t) n, 0.0);
        const double mag = isSDN ? kStandoffJitterSDN : kStandoffJitterReg;
        applyNonClusteringStandoff (standoffOffsets, rng, mag, /* cyclic */ isPerimeterVariant);

        // Position each node along the path; outward normal = perpendicular
        // to the segment in the outward direction (relative to the centroid
        // of the segs which is the speaker centroid +/- pad).
        const double cx = 0.5 * (minX + maxX);
        const double cy = 0.5 * (minY + maxY);

        layout.nodes.reserve ((size_t) n);
        for (int i = 0; i < n; ++i)
        {
            // Walk to the right segment for baseS[i].
            double remaining = baseS[(size_t) i];
            Segment activeSeg = segs[0];
            for (const auto& s : segs)
            {
                if (remaining <= s.len)
                {
                    activeSeg = s;
                    break;
                }
                remaining -= s.len;
            }

            const double dirX = (activeSeg.x1 - activeSeg.x0) / activeSeg.len;
            const double dirY = (activeSeg.y1 - activeSeg.y0) / activeSeg.len;
            // Outward normal: rotate dir by -90deg (so it points away from cx,cy).
            // Test by checking dot with vector from centroid to midpoint.
            double nxA = -dirY, nyA =  dirX;
            const double midX = 0.5 * (activeSeg.x0 + activeSeg.x1);
            const double midY = 0.5 * (activeSeg.y0 + activeSeg.y1);
            const double dotA = (midX - cx) * nxA + (midY - cy) * nyA;
            if (dotA < 0.0) { nxA = -nxA; nyA = -nyA; }

            LayoutNode node;
            node.x = activeSeg.x0 + dirX * remaining + nxA * standoffOffsets[(size_t) i];
            node.y = activeSeg.y0 + dirY * remaining + nyA * standoffOffsets[(size_t) i];
            node.z = randUniform (rng, kZMin, kZMax);

            node.orientationDeg = orientationToTarget (node.x, node.y, 0.0, 0.0);
            if (isSDN)
                node.orientationDeg = wrapDeg (node.orientationDeg
                    + randUniform (rng, -kOrientationJitterDeg, +kOrientationJitterDeg));
            node.pitchDeg   = 0.0;
            node.writePitch = false;
            layout.nodes.push_back (node);
        }

        layout.standoffDescription = isSDN
            ? juce::String ("4-6m (irregular for SDN)")
            : juce::String ("~5m (uniform)");
        return layout;
    }

    inline Layout geomSurroundRing (const Descriptors& d, const SceneSnapshot& scene,
                                      int n, double audienceRadius, bool isSDN,
                                      juce::Random& rng)
    {
        Layout layout;
        layout.variantId = "surround_ring";

        // Ring center = speaker centroid (typically near listener).
        const double cx = d.centroidX;
        const double cy = d.centroidY;
        // Speaker ring radius from outputs.
        double sumR = 0.0;
        for (const auto& p : scene.outputs)
        {
            const double dx = p.x - cx, dy = p.y - cy;
            sumR += std::sqrt (dx * dx + dy * dy);
        }
        const double speakerRingR = scene.outputs.empty()
            ? 8.0
            : sumR / scene.outputs.size();
        const double reverbR = speakerRingR + kSurroundOffset;

        std::vector<double> offsets ((size_t) n, 0.0);
        const double mag = isSDN ? kStandoffJitterSDN : kStandoffJitterReg;
        applyNonClusteringStandoff (offsets, rng, mag, /* cyclic */ true);

        const double theta0 = randUniform (rng, 0.0, 2.0 * kPi);
        layout.nodes.reserve ((size_t) n);
        for (int i = 0; i < n; ++i)
        {
            const double theta = theta0 + 2.0 * kPi * i / std::max (1, n);
            const double r = reverbR + offsets[(size_t) i];
            LayoutNode node;
            node.x = cx + r * std::cos (theta);
            node.y = cy + r * std::sin (theta);
            node.z = randUniform (rng, kZMin, kZMax);

            node.orientationDeg = orientationToTarget (node.x, node.y, 0.0, 0.0);
            if (isSDN)
                node.orientationDeg = wrapDeg (node.orientationDeg
                    + randUniform (rng, -kOrientationJitterDeg, +kOrientationJitterDeg));
            node.pitchDeg   = 0.0;
            node.writePitch = false;
            layout.nodes.push_back (node);
        }

        layout.standoffDescription = isSDN
            ? juce::String ("3-5m outside speaker ring (irregular for SDN)")
            : juce::String ("~4m outside speaker ring (uniform)");
        juce::ignoreUnused (audienceRadius);   // not used directly here
        return layout;
    }

    inline Layout geomOuterRing (const Descriptors& d, const SceneSnapshot& scene,
                                   int n, double audienceRadius, bool isSDN,
                                   juce::Random& rng)
    {
        Layout layout;
        layout.variantId = "outer_ring_beyond_audience";

        // Ring center at listener (origin); reverbs at audience_radius + 4.
        const double cx = scene.listenerX;
        const double cy = scene.listenerY;
        const double reverbR = audienceRadius + kSurroundOffset;

        std::vector<double> offsets ((size_t) n, 0.0);
        const double mag = isSDN ? kStandoffJitterSDN : kStandoffJitterReg;
        applyNonClusteringStandoff (offsets, rng, mag, /* cyclic */ true);

        const double theta0 = randUniform (rng, 0.0, 2.0 * kPi);
        layout.nodes.reserve ((size_t) n);
        for (int i = 0; i < n; ++i)
        {
            const double theta = theta0 + 2.0 * kPi * i / std::max (1, n);
            const double r = reverbR + offsets[(size_t) i];
            LayoutNode node;
            node.x = cx + r * std::cos (theta);
            node.y = cy + r * std::sin (theta);
            node.z = randUniform (rng, kZMin, kZMax);

            node.orientationDeg = orientationToTarget (node.x, node.y, 0.0, 0.0);
            if (isSDN)
                node.orientationDeg = wrapDeg (node.orientationDeg
                    + randUniform (rng, -kOrientationJitterDeg, +kOrientationJitterDeg));
            node.pitchDeg   = 0.0;
            node.writePitch = false;
            layout.nodes.push_back (node);
        }

        layout.standoffDescription = isSDN
            ? juce::String ("audience_radius + 4m (irregular for SDN)")
            : juce::String ("audience_radius + 4m (uniform)");
        juce::ignoreUnused (d);
        return layout;
    }

    inline Layout geomDomeShell (const Descriptors& d, const SceneSnapshot& scene,
                                   int n, bool isSDN, juce::Random& rng)
    {
        Layout layout;
        layout.variantId = "dome_shell_behind";

        // Speaker shell center + radius (3D fit around centroid).
        const double cx = d.centroidX;
        const double cy = d.centroidY;
        const double cz = d.centroidZ;
        const double shellR = std::max (1.0, d.meanRadius3D);
        const double reverbR = shellR + kDomeOffset;

        std::vector<double> offsets ((size_t) n, 0.0);
        const double mag = isSDN ? kStandoffJitterSDN : kStandoffJitterReg;
        applyNonClusteringStandoff (offsets, rng, mag, /* cyclic */ true);

        const double startOffset = randUniform (rng, 0.0, 2.0 * kPi);
        layout.nodes.reserve ((size_t) n);
        for (int i = 0; i < n; ++i)
        {
            double ux, uy, uz;
            vogelHemispherePoint (i, n, startOffset, ux, uy, uz);
            const double r = reverbR + offsets[(size_t) i];
            LayoutNode node;
            node.x = cx + r * ux;
            node.y = cy + r * uy;
            node.z = cz + r * uz;
            if (node.z < kZMin) node.z = kZMin;

            node.orientationDeg = orientationToTarget (node.x, node.y, 0.0, 0.0);
            node.pitchDeg       = pitchToTarget (node.x, node.y, node.z, 0.0, 0.0, 0.0);
            if (isSDN)
            {
                node.orientationDeg = wrapDeg (node.orientationDeg
                    + randUniform (rng, -kOrientationJitterDeg, +kOrientationJitterDeg));
                node.pitchDeg = wrapDeg (node.pitchDeg
                    + randUniform (rng, -kPitchJitterDeg, +kPitchJitterDeg));
            }
            node.writePitch = true;
            layout.nodes.push_back (node);
        }

        layout.standoffDescription = isSDN
            ? juce::String ("speaker_shell + 4m (irregular for SDN)")
            : juce::String ("speaker_shell + 4m (uniform)");
        return layout;
    }

    // --- Apply path: write to state + populate ChangeRecord ---------------

    inline void applyLayout (WFSValueTreeState& state, const Layout& layout,
                               ChangeRecord* record)
    {
        const int n = static_cast<int> (layout.nodes.size());

        // Pass 1: capture before-values.
        struct Write { juce::Identifier paramId; int channelIndex; juce::var value; juce::var before; };
        std::vector<Write> writes;
        writes.reserve ((size_t) n * 5);

        for (int i = 0; i < n; ++i)
        {
            const auto& node = layout.nodes[(size_t) i];
            writes.push_back ({ WFSParameterIDs::reverbPositionX, i, juce::var (node.x),    {} });
            writes.push_back ({ WFSParameterIDs::reverbPositionY, i, juce::var (node.y),    {} });
            writes.push_back ({ WFSParameterIDs::reverbPositionZ, i, juce::var (node.z),    {} });
            writes.push_back ({ WFSParameterIDs::reverbOrientation, i,
                                juce::var (juce::roundToInt (node.orientationDeg)), {} });
            if (node.writePitch)
                writes.push_back ({ WFSParameterIDs::reverbPitch, i,
                                    juce::var (juce::roundToInt (node.pitchDeg)), {} });
        }

        jassert (writes.size() <= 100);   // SetParameterBatch cap; layouts never exceed.

        for (auto& w : writes)
            w.before = state.getParameter (w.paramId, w.channelIndex);

        // Pass 2: apply writes.
        for (const auto& w : writes)
            state.setParameter (w.paramId, w.value, w.channelIndex);

        // Pass 3: capture after-values.
        std::vector<juce::var> afterValues;
        afterValues.reserve (writes.size());
        for (const auto& w : writes)
            afterValues.push_back (state.getParameter (w.paramId, w.channelIndex));

        // Pass 4: populate ChangeRecord.
        if (record != nullptr)
        {
            record->subWrites.reserve (writes.size());
            auto flatBefore = std::make_unique<juce::DynamicObject>();
            auto flatAfter  = std::make_unique<juce::DynamicObject>();
            juce::StringArray seenParams;
            std::vector<std::pair<int, juce::String>> seenGroups;

            for (size_t i = 0; i < writes.size(); ++i)
            {
                const auto& w = writes[i];
                const auto paramName = w.paramId.toString();

                ChangeSubWrite sw;
                sw.channelIndex = w.channelIndex;
                sw.bandIndex    = -1;
                auto sb = std::make_unique<juce::DynamicObject>();
                sb->setProperty (w.paramId, w.before);
                sw.beforeState = juce::var (sb.release());
                auto sa = std::make_unique<juce::DynamicObject>();
                sa->setProperty (w.paramId, afterValues[i]);
                sw.afterState = juce::var (sa.release());
                record->subWrites.push_back (std::move (sw));

                flatBefore->setProperty (w.paramId, w.before);
                flatAfter ->setProperty (w.paramId, afterValues[i]);

                seenParams.addIfNotAlreadyThere (paramName);

                std::pair<int, juce::String> key { w.channelIndex + 1, juce::String ("Position") };
                if (std::find (seenGroups.begin(), seenGroups.end(), key) == seenGroups.end())
                    seenGroups.push_back (key);
            }

            record->beforeState = juce::var (flatBefore.release());
            record->afterState  = juce::var (flatAfter.release());
            record->affectedParameters = std::move (seenParams);
            for (const auto& g : seenGroups)
                record->affectedGroups.push_back ({ g.first, g.second });
        }
    }

    // --- Response builders -------------------------------------------------

    inline juce::String algorithmLabel (Algorithm a)
    {
        switch (a)
        {
            case Algorithm::SDN: return "SDN";
            case Algorithm::FDN: return "FDN";
            case Algorithm::IR:  return "IR";
        }
        return "SDN";
    }

    inline juce::var buildClarificationPayload (const TopologyResult& tr,
                                                  const Descriptors& d)
    {
        auto obj = std::make_unique<juce::DynamicObject>();
        obj->setProperty ("status", "clarification_needed");
        obj->setProperty ("observed",
            juce::String ("Outputs distribution did not match a single ")
            + juce::String ("topology with high confidence. Candidates follow."));
        obj->setProperty ("options", juce::var (tr.candidates));
        obj->setProperty ("prompt",
            "Re-call with topology=<id> matching your system intent.");
        // A few descriptors for the AI to reason about, optional but helpful.
        obj->setProperty ("mean_radius_xy", d.meanRadiusXY);
        obj->setProperty ("height_spread",  d.heightSpread);
        obj->setProperty ("quadrants_covered_listener", d.quadrantsCovered);
        return juce::var (obj.release());
    }

    inline juce::String buildRationale (const juce::String& internalVariant,
                                          Algorithm algorithm, int nodeCount,
                                          juce::int64 seed)
    {
        juce::String text;
        if (internalVariant == "stage_halo_behind_frontal_array")
            text << "Speakers form a frontal array; reverbs placed in a "
                    "U-halo behind on three sides with audience face open.";
        else if (internalVariant == "stage_halo_behind_perimeter_array")
            text << "Speakers form a perimeter around the stage; reverbs "
                    "placed in a full halo behind on all four sides.";
        else if (internalVariant == "surround_ring")
            text << "Speakers form a surround ring; reverbs distributed 360 "
                    "degrees just outside the speaker ring.";
        else if (internalVariant == "outer_ring_beyond_audience")
            text << "Central outward-facing cluster; reverbs placed as a "
                    "large outer ring beyond the audience.";
        else if (internalVariant == "dome_shell_behind")
            text << "Hemispherical speaker shell; reverbs distributed on a "
                    "larger Vogel-spiral shell outside.";
        else
            text << "Reverbs placed behind the speaker locus.";

        if (algorithm == Algorithm::SDN)
            text << " SDN algorithm uses chaotic per-node standoff (+/-1m) "
                    "and +/-5 deg orientation jitter to avoid metallic "
                    "ringing from symmetric placements.";
        else
            text << " " << algorithmLabel (algorithm) << " algorithm uses "
                    "uniform standoff with minimal jitter.";

        text << " Node count " << nodeCount << ", seed " << juce::String (seed) << ".";

        if (nodeCount > 0 && (nodeCount % 2) == 0 && algorithm == Algorithm::SDN)
            text << " Note: even reverb count with SDN can produce "
                    "left-right pairing artefacts; odd counts are preferred.";
        return text;
    }

    inline juce::var buildAppliedPayload (const TopologyResult& tr,
                                            const Layout& layout,
                                            const SceneSnapshot& scene,
                                            juce::int64 seedUsed)
    {
        auto obj = std::make_unique<juce::DynamicObject>();
        obj->setProperty ("status",                  "applied");
        obj->setProperty ("topology",                tr.topologyId);
        obj->setProperty ("topology_variant",        layout.variantId);
        obj->setProperty ("reverb_algorithm",        algorithmLabel (scene.algorithm));
        obj->setProperty ("node_count",              static_cast<int> (layout.nodes.size()));
        obj->setProperty ("standoff_behind_speakers", layout.standoffDescription);
        obj->setProperty ("seed",                    seedUsed);
        obj->setProperty ("rationale",
            buildRationale (layout.variantId, scene.algorithm,
                              static_cast<int> (layout.nodes.size()), seedUsed));
        return juce::var (obj.release());
    }

    // --- Schema ------------------------------------------------------------

    inline juce::var buildSchema()
    {
        auto props = std::make_unique<juce::DynamicObject>();

        auto topology = std::make_unique<juce::DynamicObject>();
        topology->setProperty ("type", "string");
        {
            juce::Array<juce::var> e;
            e.add ("stage_halo");
            e.add ("surround_ring");
            e.add ("outer_ring_beyond_audience");
            e.add ("dome_shell_behind");
            topology->setProperty ("enum", juce::var (e));
        }
        topology->setProperty ("description",
            "Explicit topology. Omit on first call - the tool detects the "
            "topology from output positions and returns status "
            "`clarification_needed` if more than one fits comparably. The "
            "operator re-calls with this set to disambiguate. Note: "
            "stage_halo collapses two internal variants (frontal array vs "
            "perimeter array); the tool sub-classifies and surfaces which "
            "variant fired in `topology_variant`.");
        props->setProperty ("topology", juce::var (topology.release()));

        auto audienceR = std::make_unique<juce::DynamicObject>();
        audienceR->setProperty ("type",    "number");
        audienceR->setProperty ("minimum", 1.0);
        audienceR->setProperty ("maximum", 200.0);
        audienceR->setProperty ("description",
            "Meters from listener to outer edge of audience zone. Required "
            "when topology resolves to surround_ring or "
            "outer_ring_beyond_audience.");
        props->setProperty ("audience_radius", juce::var (audienceR.release()));

        auto seed = std::make_unique<juce::DynamicObject>();
        seed->setProperty ("type",    "integer");
        seed->setProperty ("minimum", 0);
        seed->setProperty ("maximum", 2147483647);
        seed->setProperty ("description",
            "Random seed. Same seed yields the same layout; the seed used "
            "is returned in the response so the operator can reproduce a "
            "layout they liked.");
        props->setProperty ("seed", juce::var (seed.release()));

        auto confirm = std::make_unique<juce::DynamicObject>();
        confirm->setProperty ("type", "string");
        confirm->setProperty ("description",
            "Tier-2 confirmation token. Re-call with confirm set to this "
            "token within 30s. Tier-2 auto-confirm (Network tab) skips the "
            "handshake.");
        props->setProperty ("confirm", juce::var (confirm.release()));

        auto schema = std::make_unique<juce::DynamicObject>();
        schema->setProperty ("type", "object");
        schema->setProperty ("properties", juce::var (props.release()));
        schema->setProperty ("additionalProperties", false);
        return juce::var (schema.release());
    }

} // namespace detail

// --- Public handler ---------------------------------------------------------

inline ToolResult autoLayout (WFSValueTreeState& state, const juce::var& args,
                                ChangeRecord* record)
{
    using namespace detail;

    juce::String userTopology;
    bool         hasAudienceR = false;
    double       audienceRadius = 0.0;
    bool         hasSeed      = false;
    juce::int64  seedArg      = 0;

    if (args.isObject())
    {
        if (auto* obj = args.getDynamicObject())
        {
            if (obj->hasProperty ("topology"))
                userTopology = obj->getProperty ("topology").toString();
            if (obj->hasProperty ("audience_radius"))
            {
                hasAudienceR = true;
                audienceRadius = static_cast<double> (obj->getProperty ("audience_radius"));
            }
            if (obj->hasProperty ("seed"))
            {
                hasSeed = true;
                seedArg = static_cast<juce::int64> (obj->getProperty ("seed"));
            }
        }
    }

    auto scene = readScene (state);

    if (scene.numReverbs <= 0)
        return ToolResult::error ("no_reverbs",
            "No reverb channels exist. Create some first with reverb_create.");

    if (scene.mixedAlgorithms)
        return ToolResult::error ("mixed_algorithms",
            "Reverb channels report different algorithms. The active-acoustic "
            "model assumes one algorithm per setup. Unify reverbAlgoType first.");

    if (scene.outputs.empty() || scene.allOutputsAtDefault)
        return ToolResult::error ("speakers_not_placed",
            "Outputs are at default position (0,0,0). Place speakers first; "
            "reverbs go behind them.");

    auto descriptors = computeDescriptors (scene);

    juce::String resolvedTopology;
    juce::String internalVariant;
    if (userTopology.isNotEmpty())
    {
        resolvedTopology = userTopology;
        if (userTopology == "stage_halo")
            internalVariant = pickStageHaloVariant (descriptors, scene);
        else
            internalVariant = userTopology;
    }
    else
    {
        auto tr = classify (descriptors, scene);
        if (tr.ambiguous)
        {
            // clarification_needed is a success envelope with a status field,
            // not an error envelope - state did not change but the call was
            // valid in shape.
            return ToolResult::ok (buildClarificationPayload (tr, descriptors));
        }
        resolvedTopology = tr.topologyId;
        internalVariant  = tr.internalVariant;
    }

    if ((resolvedTopology == "surround_ring"
         || resolvedTopology == "outer_ring_beyond_audience")
        && ! hasAudienceR)
    {
        return ToolResult::error ("missing_parameter",
            "audience_radius required for " + resolvedTopology
            + ". Meters from listener to outer edge of audience zone.");
    }

    // Seed handling.
    juce::int64 seedUsed = hasSeed ? seedArg
                                    : (juce::int64) juce::Random::getSystemRandom().nextInt (1 << 30);
    juce::Random rng (seedUsed);

    const bool isSDN = (scene.algorithm == Algorithm::SDN);

    Layout layout;
    if (internalVariant == "stage_halo_behind_frontal_array")
        layout = geomStageHalo (descriptors, scene, scene.numReverbs, isSDN,
                                  /* perimeter */ false, rng);
    else if (internalVariant == "stage_halo_behind_perimeter_array")
        layout = geomStageHalo (descriptors, scene, scene.numReverbs, isSDN,
                                  /* perimeter */ true, rng);
    else if (internalVariant == "surround_ring")
        layout = geomSurroundRing (descriptors, scene, scene.numReverbs,
                                     audienceRadius, isSDN, rng);
    else if (internalVariant == "outer_ring_beyond_audience")
        layout = geomOuterRing (descriptors, scene, scene.numReverbs,
                                  audienceRadius, isSDN, rng);
    else if (internalVariant == "dome_shell_behind")
        layout = geomDomeShell (descriptors, scene, scene.numReverbs, isSDN, rng);
    else
        return ToolResult::error ("invalid_args",
            "Unknown topology: " + resolvedTopology);

    if (layout.nodes.empty())
        return ToolResult::error ("no_reverbs",
            "Layout produced 0 nodes - this should not happen if scene "
            "validation passed. Please report.");

    applyLayout (state, layout, record);

    if (record != nullptr)
    {
        record->operatorDescription = juce::String ("Auto-layout reverbs: ")
            + layout.variantId + ", " + juce::String ((int) layout.nodes.size())
            + " nodes, seed " + juce::String (seedUsed)
            + (isSDN ? juce::String (", SDN chaotic standoff")
                     : juce::String (", uniform standoff"));
    }

    return ToolResult::ok (buildAppliedPayload (
        TopologyResult { resolvedTopology, internalVariant, 1.0, false, {} },
        layout, scene, seedUsed));
}

inline ToolDescriptor describe (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name = "reverb_auto_layout";
    d.description =
        "Place all existing reverb channels around the speaker locus, with "
        "geometry and randomization tuned for the configured reverb "
        "algorithm. Reads outputs, listener (origin in v1), stage geometry, "
        "and the global reverbAlgoType, classifies the speaker topology, "
        "picks the appropriate placement geometry, and applies positions, "
        "orientations, and (dome only) pitch to every reverb in one atomic "
        "batch - one undoable entry. Returns either status `applied` (with "
        "topology, node_count, seed, rationale) or status "
        "`clarification_needed` (with candidate topologies, no state "
        "changed). Refuses with `no_reverbs`, `speakers_not_placed`, "
        "`mixed_algorithms`, or `missing_parameter` (when audience_radius "
        "is required and absent)."
      + juce::String (kTier2DescriptionSuffix);
    d.inputSchema   = detail::buildSchema();
    d.modifiesState = true;
    d.tier          = 2;
    d.handler = [&state] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        return autoLayout (state, args, record);
    };
    return d;
}

} // namespace WFSNetwork::Tools::ReverbAutoLayout
