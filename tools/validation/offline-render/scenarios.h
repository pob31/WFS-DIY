#pragma once

//==============================================================================
// Scripted deterministic scenario timelines for the offline render harness
// (docs/architecture/offline-render-harness.md).
//
// Every value produced here is a pure function of (scenario, tick index) or
// (scenario, channel, sample index) — no RNG objects, no wall-clock time.
// Matrix timelines are stepped at the app's 50 Hz tick cadence: the runner
// re-writes the six matrix arrays between blocks whenever
// tick = floor(blockStartSample * 50 / sampleRate) changes, exactly as the
// app's 50 Hz timer thread does (the algorithms re-smooth internally).
//==============================================================================

#include <cstdint>
#include <cmath>
#include <string>
#include <vector>

#include "../../../spatcore/reverb/ReverbAlgorithm.h"   // AlgorithmParameters, NodePosition (POD)

namespace scenario
{

//==============================================================================
// Squirrel-hash noise — same idiom as Source/DSP/FrDiffusionModel.h
// (FrDiffusion::hashNoiseBipolar / makeKey), duplicated locally so the input
// generator has no dependency on app RNG or app headers beyond the reverb PODs.
//==============================================================================
inline float hashNoiseBipolar (uint32_t n, uint32_t key) noexcept
{
    n *= 0xB5297A4Du;
    n += key;
    n ^= n >> 8;
    n += 0x68E31DA4u;
    n ^= n << 8;
    n *= 0x1B56C4E9u;
    n ^= n >> 8;
    return static_cast<float> (static_cast<int32_t> (n)) * (1.0f / 2147483648.0f);
}

inline uint32_t makeKey (uint32_t a, uint32_t b) noexcept
{
    return a * 0x9E3779B9u + (b + 1u) * 0x85EBCA6Bu + 1u;
}

//==============================================================================
enum class Id
{
    Static = 0,     // fixed matrices, FR off
    Moving,         // source sweep: delay/level ramps stepped at 50 Hz ticks
    FrToggle,       // floor reflections on, diffusion nonzero, toggled mid-run
};

inline const char* name (Id id)
{
    switch (id)
    {
        case Id::Static:   return "static";
        case Id::Moving:   return "moving";
        case Id::FrToggle: return "fr-toggle";
    }
    return "?";
}

inline bool fromName (const std::string& s, Id& out)
{
    if (s == "static")    { out = Id::Static;   return true; }
    if (s == "moving")    { out = Id::Moving;   return true; }
    if (s == "fr-toggle") { out = Id::FrToggle; return true; }
    return false;
}

inline const std::vector<Id>& allScenarios()
{
    static const std::vector<Id> all { Id::Static, Id::Moving, Id::FrToggle };
    return all;
}

//==============================================================================
// Input signal: per-channel fixed-phase sine bank + an impulse at block 0
// + low-level hash-noise. Pure function of (scenario, channel, sample index),
// so the same combo always renders the identical input stream.
//==============================================================================
inline float inputSample (Id id, int channel, int64_t sampleIndex, double sampleRate)
{
    const int sid = static_cast<int> (id);
    const double freq = 110.0 + 97.0 * static_cast<double> (channel % 8)
                      + 13.0 * static_cast<double> (sid);

    float s = 0.25f * static_cast<float> (
        std::sin (2.0 * 3.141592653589793 * freq
                  * static_cast<double> (sampleIndex) / sampleRate));

    if (sampleIndex == 0)
        s += 0.9f;   // impulse at block 0

    s += 0.001f * hashNoiseBipolar (static_cast<uint32_t> (sampleIndex),
                                    makeKey (static_cast<uint32_t> (sid) * 31u + 7u,
                                             static_cast<uint32_t> (channel)));
    return s;
}

//==============================================================================
// WFS matrix timelines. Layout is the app's input-major [in * numOutputs + out]
// for all six families (see InputBufferProcessor.h:597-604).
//==============================================================================
struct WfsMatrices
{
    std::vector<float> delayMs, levels, hfDb, frDelayMs, frLevels, frHfDb;

    void allocate (int numIn, int numOut)
    {
        const size_t n = static_cast<size_t> (numIn) * static_cast<size_t> (numOut);
        delayMs.assign (n, 0.0f);   levels.assign (n, 0.0f);   hfDb.assign (n, 0.0f);
        frDelayMs.assign (n, 0.0f); frLevels.assign (n, 0.0f); frHfDb.assign (n, 0.0f);
    }
};

/** Per-scenario FR processor settings (constant for the whole run; the
    fr-toggle scenario toggles the FR tap via the frLevels matrix, which is
    how MainComponent engages/disengages FR). */
struct FrSettings
{
    float diffusionPercent = 0.0f;
    bool  lowCutActive = false;
    float lowCutFreq = 100.0f;
    bool  highShelfActive = false;
    float highShelfFreq = 3000.0f;
    float highShelfGain = -2.0f;
    float highShelfSlope = 0.4f;
};

inline FrSettings frSettings (Id id)
{
    FrSettings s;
    if (id == Id::FrToggle)
    {
        s.diffusionPercent = 35.0f;   // nonzero: exercises the hash-keyed grain
        s.lowCutActive = true;    s.lowCutFreq = 120.0f;
        s.highShelfActive = true; s.highShelfFreq = 3200.0f;
        s.highShelfGain = -2.5f;  s.highShelfSlope = 0.4f;
    }
    return s;
}

/** Write the six matrices for the given 50 Hz tick. Pure function of
    (scenario, tick, in, out). */
inline void applyWfsTick (Id id, int tick, int numIn, int numOut, WfsMatrices& m)
{
    const double t = static_cast<double> (tick) / 50.0;   // seconds at tick cadence
    const double twoPi = 2.0 * 3.141592653589793;

    for (int in = 0; in < numIn; ++in)
    {
        for (int out = 0; out < numOut; ++out)
        {
            const size_t idx = static_cast<size_t> (in) * static_cast<size_t> (numOut)
                             + static_cast<size_t> (out);

            // Fixed per-pair values (also the direct path of fr-toggle)
            const float staticDelay = 2.0f + 0.5f * static_cast<float> ((in * 7 + out * 3) % 40);
            const float staticLevel = 0.25f + 0.05f * static_cast<float> ((in + 2 * out) % 10);
            const float staticHf    = -1.0f * static_cast<float> ((in + out) % 6);

            switch (id)
            {
                case Id::Static:
                {
                    m.delayMs[idx] = staticDelay;
                    m.levels[idx]  = staticLevel;
                    m.hfDb[idx]    = staticHf;
                    m.frDelayMs[idx] = 0.0f;
                    m.frLevels[idx]  = 0.0f;
                    m.frHfDb[idx]    = 0.0f;
                    break;
                }

                case Id::Moving:
                {
                    const double phase = 0.37 * in + 0.11 * out;
                    m.delayMs[idx] = 5.0f + 5.0f * static_cast<float> (
                                         1.0 + std::sin (twoPi * 0.5 * t + phase));       // 5..15 ms sweep
                    m.levels[idx]  = 0.20f + 0.15f * static_cast<float> (
                                         1.0 + std::sin (twoPi * 0.3 * t + 1.7 * phase)); // 0.2..0.5
                    m.hfDb[idx]    = -3.0f * static_cast<float> (
                                         0.5 + 0.5 * std::sin (twoPi * 0.2 * t + phase)); // 0..-3 dB
                    m.frDelayMs[idx] = 0.0f;
                    m.frLevels[idx]  = 0.0f;
                    m.frHfDb[idx]    = 0.0f;
                    break;
                }

                case Id::FrToggle:
                {
                    m.delayMs[idx] = staticDelay;
                    m.levels[idx]  = staticLevel;
                    m.hfDb[idx]    = staticHf;

                    // FR toggles every 50 ticks (1 s): on -> off -> on ...
                    const bool frOn = ((tick / 50) % 2) == 0;
                    m.frDelayMs[idx] = 3.0f + 0.5f * static_cast<float> ((in + out) % 10); // extra ms
                    m.frLevels[idx]  = frOn ? 0.2f : 0.0f;
                    m.frHfDb[idx]    = -3.0f;
                    break;
                }
            }
        }
    }
}

//==============================================================================
// Reverb parameter timeline (SDN/FDN; the IR algorithm ignores setParameters,
// its scenarios differ through the scenario-keyed input signal).
//==============================================================================
inline AlgorithmParameters reverbParams (Id id, int tick)
{
    const double t = static_cast<double> (tick) / 50.0;
    const double twoPi = 2.0 * 3.141592653589793;

    AlgorithmParameters p;   // defaults: rt60 1.5, xover 200/4000, diffusion 0.5 ...
    p.wetLevel = 1.0f;

    switch (id)
    {
        case Id::Static:
            break;   // fixed defaults for the whole run

        case Id::Moving:
            p.rt60      = 1.2f + 0.4f * static_cast<float> (1.0 + std::sin (twoPi * 0.25 * t));
            p.diffusion = 0.3f + 0.2f * static_cast<float> (0.5 + 0.5 * std::sin (twoPi * 0.4 * t));
            break;

        case Id::FrToggle:
            // parameter step change mid-run (design doc: "reverb param change mid-run")
            p.diffusion = (((tick / 50) % 2) == 0) ? 0.7f : 0.2f;
            p.rt60      = (((tick / 50) % 2) == 0) ? 1.8f : 1.2f;
            break;
    }
    return p;
}

/** SDN node geometry: corners of a ~4 x 3 x 2.5 m box (same idea as
    tools/test-gpu-plugin.cpp scenario E), with a small deterministic offset
    for node counts above 8 so no two nodes coincide. */
inline std::vector<NodePosition> nodeBox (int numNodes)
{
    std::vector<NodePosition> pos (static_cast<size_t> (numNodes));
    for (int n = 0; n < numNodes; ++n)
    {
        const float lift = 0.11f * static_cast<float> (n / 8);
        pos[static_cast<size_t> (n)] = NodePosition {
            ((n & 1) != 0 ? 4.0f : 0.0f) + lift,
            ((n & 2) != 0 ? 3.0f : 0.0f) + lift,
            ((n & 4) != 0 ? 2.5f : 0.0f) + lift };
    }
    return pos;
}

/** Deterministic decaying hash-noise IR (like test-gpu-plugin.cpp scenario C,
    but noise-bodied so the convolution tail is broadband). */
inline std::vector<float> deterministicIr (double sampleRate)
{
    const int len = static_cast<int> (sampleRate * 0.5);   // 0.5 s
    std::vector<float> ir (static_cast<size_t> (len));
    for (int i = 0; i < len; ++i)
        ir[static_cast<size_t> (i)] =
            std::exp (-4.0f * static_cast<float> (i) / static_cast<float> (len))
            * hashNoiseBipolar (static_cast<uint32_t> (i), 0xC0FFEE01u);
    ir[0] = 1.0f;
    return ir;
}

} // namespace scenario
