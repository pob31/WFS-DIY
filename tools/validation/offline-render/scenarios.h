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
#include <limits>
#include <string>
#include <vector>

#include "../../../spatcore/reverb/ReverbAlgorithm.h"   // AlgorithmParameters, NodePosition (POD)
#include "../../../spatcore/effects/EffectParams.h"     // EffectChannelParams, ChainOrder (POD)
#include "../../../spatcore/effects/LoopGuard.h"         // the engine scenario's NaN-policy self-test

namespace scenario
{

//==============================================================================
// Squirrel-hash noise — same idiom as spatcore/dsp/FrDiffusionModel.h
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
    Stereo,         // one stereo-pair channel (6 slice slots, width timeline) + mono channels

    // Null-test pair (--stereo-null): NOT baselined, not reachable from
    // --scenario. Both draw bit-identical L/R streams; the runner compares
    // their output hashes against each other instead of a baseline file.
    StereoNull,     // config A: width-0 stereo — 6 sources: silent centre on 0, L/R on 1/2, 3..5 claimed-and-silent
    StereoNullMono, // config B: two mono channels at the same position

    // --path effects: one scenario per module, plus the whole chain. APPENDED
    // on purpose — the enum value feeds inputSample()'s frequency map, so an
    // id inserted anywhere above would move every existing baseline hash.
    FxDist,         // distortion:  drive/shape/bias sweep + oversampling variant
    FxEq,           // 6-band EQ:   swept peak + band-shape switches
    FxDyn,          // dynamics:    comp + expander, detector and lookahead variants
    FxMod,          // chorus/flanger: voices and through-zero variants
    FxPhaser,       // phaser:      stage-count variant
    FxTrem,         // tremolo:     no variant — sweeps and the bypass toggle
    FxReverb,       // FDN reverb:  size changes spill over (the old network rings out)
    FxDelay,        // multitap:    tap count / pattern / manual / feedback tap
    FxCrush,        // bitcrusher:  decimation-filter variant + dither floor
    FxChain,        // all eleven slots: reorder x3, chain bypass, mute

    // The ENGINE, and the same append-only rule for the same reason: this
    // value feeds inputSample()'s frequency map, so an id put anywhere above
    // it would silently re-render every scenario after it.
    FxEngine,       // EffectsEngineCore end to end: 8 sources, 4 channels, a
                    // fixed feed matrix with an fx -> fx loop, the block
                    // ledger, a reorder, a bypass window, two mute windows,
                    // and a loop guard that trips and releases

    // The reverb's models (spatcore 0.4), appended under the same rule. Each
    // runs slot 8 like FxReverb; the last hops between them.
    FxReverbEr,     // FDN + early reflections: Room -> Hall -> Cathedral at Size 1.4
    FxReverbPlate,  // Dattorro plate: modulation and decay sweeps, two size spillovers
    FxReverbHall,   // modulated hall: Chamber reflections, a size spillover, reflections off
    FxReverbShimmer,// shimmer: an octave, then a fifth, then an octave down and up
    FxReverbModels, // every model in turn, then a burst of changes one tick apart
};

inline const char* name (Id id)
{
    switch (id)
    {
        case Id::Static:         return "static";
        case Id::Moving:         return "moving";
        case Id::FrToggle:       return "fr-toggle";
        case Id::Stereo:         return "stereo";
        case Id::StereoNull:     return "stereo-null";
        case Id::StereoNullMono: return "stereo-null-mono";

        case Id::FxDist:         return "dist";
        case Id::FxEq:           return "eq";
        case Id::FxDyn:          return "dyn";
        case Id::FxMod:          return "mod";
        case Id::FxPhaser:       return "phaser";
        case Id::FxTrem:         return "trem";
        case Id::FxReverb:       return "reverb";
        case Id::FxDelay:        return "delay";
        case Id::FxCrush:        return "crush";
        case Id::FxChain:        return "chain";
        case Id::FxEngine:       return "engine";
        case Id::FxReverbEr:     return "reverb-er";
        case Id::FxReverbPlate:  return "reverb-plate";
        case Id::FxReverbHall:   return "reverb-hall";
        case Id::FxReverbShimmer: return "reverb-shimmer";
        case Id::FxReverbModels: return "reverb-models";
    }
    return "?";
}

inline bool fromName (const std::string& s, Id& out)
{
    if (s == "static")    { out = Id::Static;   return true; }
    if (s == "moving")    { out = Id::Moving;   return true; }
    if (s == "fr-toggle") { out = Id::FrToggle; return true; }
    if (s == "stereo")    { out = Id::Stereo;   return true; }

    if (s == "dist")      { out = Id::FxDist;   return true; }
    if (s == "eq")        { out = Id::FxEq;     return true; }
    if (s == "dyn")       { out = Id::FxDyn;    return true; }
    if (s == "mod")       { out = Id::FxMod;    return true; }
    if (s == "phaser")    { out = Id::FxPhaser; return true; }
    if (s == "trem")      { out = Id::FxTrem;   return true; }
    if (s == "reverb")    { out = Id::FxReverb; return true; }
    if (s == "delay")     { out = Id::FxDelay;  return true; }
    if (s == "crush")     { out = Id::FxCrush;  return true; }
    if (s == "chain")     { out = Id::FxChain;  return true; }
    if (s == "engine")    { out = Id::FxEngine; return true; }
    if (s == "reverb-er")      { out = Id::FxReverbEr;      return true; }
    if (s == "reverb-plate")   { out = Id::FxReverbPlate;   return true; }
    if (s == "reverb-hall")    { out = Id::FxReverbHall;    return true; }
    if (s == "reverb-shimmer") { out = Id::FxReverbShimmer; return true; }
    if (s == "reverb-models")  { out = Id::FxReverbModels;  return true; }
    return false;
}

/** Scenario families. A scenario belongs to exactly one, and the runner pairs
    each path with its own family's list: --scenario all means the four WFS
    timelines on a render path and the eleven effects timelines on --path
    effects, never the cross product (which is what would have added 50 MISSING
    keys to the existing baseline check). */
inline bool isEffectsScenario (Id id)
{
    return static_cast<int> (id) >= static_cast<int> (Id::FxDist);
}

inline const std::vector<Id>& allScenarios()
{
    static const std::vector<Id> all { Id::Static, Id::Moving, Id::FrToggle, Id::Stereo };
    return all;
}

inline const std::vector<Id>& allEffectsScenarios()
{
    static const std::vector<Id> all {
        Id::FxDist, Id::FxEq, Id::FxDyn, Id::FxMod, Id::FxPhaser,
        Id::FxTrem, Id::FxReverb, Id::FxDelay, Id::FxCrush, Id::FxChain,
        Id::FxEngine,
        Id::FxReverbEr, Id::FxReverbPlate, Id::FxReverbHall, Id::FxReverbShimmer, Id::FxReverbModels };
    return all;
}

//==============================================================================
// Input signal: per-channel fixed-phase sine bank + an impulse at block 0
// + low-level hash-noise. Pure function of (scenario, channel, sample index),
// so the same combo always renders the identical input stream.
//==============================================================================
inline float inputSample (Id id, int channel, int64_t sampleIndex, double sampleRate)
{
    // Stereo scenario: source 0 is the stereo channel's CENTRE slot (active
    // but silent in the pass-through backend), sources 1/2 carry L/R, and
    // sources 3..5 are claimed-and-silent slice slots — exact zeros.
    if (id == Id::Stereo && (channel == 0 || (channel >= 3 && channel < 6)))
        return 0.0f;

    // Null pair: BOTH configs must feed bit-identical L/R, so they share one
    // scenario key and remap onto the same stream indices. Config A
    // (StereoNull): slot 0 = silent centre, slots 1/2 = L/R, 3..5 zeros.
    // Config B (StereoNullMono): slots 0/1 = L/R directly.
    if (id == Id::StereoNull || id == Id::StereoNullMono)
    {
        if (id == Id::StereoNull)
        {
            if (channel == 0 || channel >= 3)
                return 0.0f;
            channel -= 1;   // slots 1/2 -> streams 0/1
        }
        else if (channel >= 2)
        {
            return 0.0f;
        }
        id = Id::StereoNull;
    }

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

                case Id::Stereo:
                {
                    // Sources 0..5 = the six slice slots of ONE stereo-pair
                    // channel; sources >= 6 are ordinary mono channels.
                    // Slot 0 is the CENTRE (anchor row, silent audio in the
                    // pass-through backend), slots 1/2 the live L/R slices —
                    // they share the channel's anchor values (per-channel
                    // terms are shared by contract) and separate along a
                    // width timeline, a live inputStereoWidth edit. Slots
                    // 3..5 are claimed-and-silent: zero rows. FR is N/A for
                    // stereo channels: zero for every slice slot.
                    if (in >= 3 && in < 6)
                    {
                        m.delayMs[idx] = 0.0f;
                        m.levels[idx]  = 0.0f;
                        m.hfDb[idx]    = 0.0f;
                        m.frDelayMs[idx] = 0.0f;
                        m.frLevels[idx]  = 0.0f;
                        m.frHfDb[idx]    = 0.0f;
                        break;
                    }

                    if (in < 3)
                    {
                        // Anchor row = the Static formula evaluated at in = 0
                        const float anchorDelay = 2.0f + 0.5f * static_cast<float> ((out * 3) % 40);
                        const float anchorLevel = 0.25f + 0.05f * static_cast<float> ((2 * out) % 10);
                        const float anchorHf    = -1.0f * static_cast<float> (out % 6);

                        // Width 0..1 over 10 s; centre at azimuth 0, slices
                        // 1/2 at -1/+1. The per-output scale models the
                        // slice's distance to each speaker changing as it
                        // moves off the anchor.
                        const float width   = 0.5f + 0.5f * static_cast<float> (std::sin (twoPi * 0.1 * t));
                        const float azimuth = (in == 0) ? 0.0f : (in == 1 ? -1.0f : 1.0f);

                        m.delayMs[idx] = anchorDelay
                                       + azimuth * width * (0.4f + 0.05f * static_cast<float> (out % 5));
                        m.levels[idx]  = anchorLevel;
                        m.hfDb[idx]    = anchorHf;
                        m.frDelayMs[idx] = 0.0f;
                        m.frLevels[idx]  = 0.0f;
                        m.frHfDb[idx]    = 0.0f;
                        break;
                    }

                    // Mono channels: Static values, FR off
                    m.delayMs[idx] = staticDelay;
                    m.levels[idx]  = staticLevel;
                    m.hfDb[idx]    = staticHf;
                    m.frDelayMs[idx] = 0.0f;
                    m.frLevels[idx]  = 0.0f;
                    m.frHfDb[idx]    = 0.0f;
                    break;
                }

                case Id::StereoNull:
                case Id::StereoNullMono:
                {
                    // The null condition (handoff doc §8): every LIVE source
                    // renders through the IDENTICAL anchor row — a width-0
                    // stereo channel (centre + L + R, centre silent) and two
                    // mono channels at the same position must be
                    // indistinguishable. Config A's slots 3..5 are zero rows.
                    if (id == Id::StereoNull && in >= 3)
                    {
                        m.delayMs[idx] = 0.0f;
                        m.levels[idx]  = 0.0f;
                        m.hfDb[idx]    = 0.0f;
                    }
                    else
                    {
                        m.delayMs[idx] = 2.0f + 0.5f * static_cast<float> ((out * 3) % 40);
                        m.levels[idx]  = 0.25f + 0.05f * static_cast<float> ((2 * out) % 10);
                        m.hfDb[idx]    = -1.0f * static_cast<float> (out % 6);
                    }
                    m.frDelayMs[idx] = 0.0f;
                    m.frLevels[idx]  = 0.0f;
                    m.frHfDb[idx]    = 0.0f;
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

        case Id::Stereo:
        case Id::StereoNull:
        case Id::StereoNullMono:
            break;   // fixed defaults — the stereo content is in the input set
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

//==============================================================================
// EFFECTS PATH (--path effects): one scenario per module + one whole-chain
// scenario, each a scripted parameter timeline stepped at the SAME 50 Hz tick
// cadence as the WFS/reverb timelines above.
//
// Input: scenario::inputSample() — the same generator every other path uses.
// There is no second signal source in this harness, on purpose.
//
// A module scenario drives its module through a ModuleSlot rather than bare,
// because the three things worth gating live in the slot: the bypass
// crossfade, the reset-at-silence, and the commit of a variant change that
// cannot be interpolated. A render of a module at defaults is a render of a
// BYPASSED module (every module in EffectChannelParams starts bypassed), which
// hashes the input straight back and proves nothing.
//
// One shared temporal script, so all ten MODULE scenarios read the same way
// at the default shape (200 blocks x 512 @ 48 kHz = ticks 0..106). The engine
// scenario at the bottom of this file scripts its own, because what it drives
// is the engine rather than a module:
//
//   ticks  0..19    active, every continuous parameter sweeping
//   ticks 20..34    BYPASSED — the slot fades out and resets the module at
//                   silence, so its tail cannot reappear
//   ticks 35..      active again (the fade back in is in the hash too)
//   tick  50        variant switch A — needs silence: fade out, reset,
//                   commitPendingVariant(), fade back in
//   tick  75        variant switch B
//
// plus per-module discrete edits at ticks 40, 60 and 85 where the module has
// something else worth switching (a stage on, an LFO shape, a dither floor).
//==============================================================================
namespace fx
{
    inline constexpr int kBypassOn  = 20;   // tick the module is bypassed at
    inline constexpr int kBypassOff = 35;   // tick it comes back
    inline constexpr int kVariantA  = 50;   // first variant switch
    inline constexpr int kVariantB  = 75;   // second variant switch

    // The chain scenario's own slot-level windows. eq2 and dyn2 (instance 1 of
    // the EQ and of the dynamics) exist ONLY inside the chain — no module
    // scenario instantiates them — so without these they would render at
    // steady non-identity settings and never go through a ModuleSlot bypass
    // fade, a reset-at-silence or a commitPendingVariant(). A slot-level
    // regression specific to instance 1 would then pass the gate.
    // The windows are placed clear of the chain-bypass window (55..64) and the
    // mute window (90..95) so the slot fades land in audible output.
    inline constexpr int kChainEq2BypassOn   = 20;
    inline constexpr int kChainEq2BypassOff  = 35;   // same 15-tick fade room as a module scenario
    inline constexpr int kChainDyn2Variant   = 45;   // detector peak -> RMS: commits at silence
    inline constexpr int kChainDyn2BypassOn  = 72;
    inline constexpr int kChainDyn2BypassOff = 87;

    inline bool bypassed (int tick) noexcept
    {
        return tick >= kBypassOn && tick < kBypassOff;
    }

    /** 0..1, a pure function of the tick — the same sin-of-tick-time idiom the
        WFS and reverb timelines use, so there is one cadence in this file. */
    inline float sweep01 (int tick, double hz, double phase) noexcept
    {
        const double t = static_cast<double> (tick) / 50.0;
        return 0.5f + 0.5f * static_cast<float> (
                   std::sin (2.0 * 3.141592653589793 * hz * t + phase));
    }

    inline float sweep (int tick, double hz, double phase, float lo, float hi) noexcept
    {
        return lo + (hi - lo) * sweep01 (tick, hz, phase);
    }

    /** Chain orders, as the strings an operator would actually type — parsed
        through the shipped parser, so the harness gates that too. */
    inline const char* const kOrderDefault = "dist,eq1,eq2,dyn1,dyn2,mod,phaser,trem,reverb,delay,crush";
    inline const char* const kOrderB       = "reverb,delay,crush,trem,phaser,mod,dyn2,dyn1,eq2,eq1,dist";
    inline const char* const kOrderC       = "crush,dist,trem,eq1,dyn1,reverb,mod,eq2,dyn2,phaser,delay";
    inline const char* const kOrderD       = "eq1,dyn1,dist,mod,phaser,trem,delay,reverb,crush,eq2,dyn2";
}

/** Slot index in spatcore::effects::kSlots for a per-module scenario;
    -1 for the whole-chain scenario. */
inline int effectsSlotIndex (Id id) noexcept
{
    switch (id)
    {
        case Id::FxDist:   return 0;    // "dist"
        case Id::FxEq:     return 1;    // "eq1"   (instance 0)
        case Id::FxDyn:    return 3;    // "dyn1"  (instance 0)
        case Id::FxMod:    return 5;    // "mod"
        case Id::FxPhaser: return 6;    // "phaser"
        case Id::FxTrem:   return 7;    // "trem"
        case Id::FxReverb: return 8;    // "reverb"
        case Id::FxReverbEr:
        case Id::FxReverbPlate:
        case Id::FxReverbHall:
        case Id::FxReverbShimmer:
        case Id::FxReverbModels: return 8;
        case Id::FxDelay:  return 9;    // "delay"
        case Id::FxCrush:  return 10;   // "crush"

        // The engine scenario drives four whole CHAINS through
        // EffectsEngineCore and has no single slot of its own, so it lands
        // here with the chain scenario rather than naming a slot.
        case Id::FxEngine:
        default:           return -1;   // the chain or the engine, or not an effects scenario
    }
}

/** The parameter set for one 50 Hz tick. Pure function of (scenario, tick).
    Everything starts bypassed (EffectChannelParams' documented default), so a
    scenario only has to switch ON what it means to exercise. */
inline spatcore::effects::EffectChannelParams effectsParams (Id id, int tick)
{
    using namespace spatcore::effects;

    EffectChannelParams p;

    // The chain re-reads parameters only when revision MOVES, so every tick
    // gets its own. (A module scenario calls applyParams directly and never
    // consults it, but one rule is easier to trust than two.)
    p.revision = static_cast<std::uint32_t> (tick) + 1u;

    const bool off = fx::bypassed (tick);
    const bool vA  = tick >= fx::kVariantA;
    const bool vB  = tick >= fx::kVariantB;

    switch (id)
    {
        case Id::FxDist:
        {
            p.dist.bypass = off ? 1 : 0;
            p.dist.driveDb = fx::sweep (tick, 0.35, 0.0,  6.0f, 30.0f);
            p.dist.shape   = fx::sweep (tick, 0.17, 1.1,  0.0f,  1.0f);   // hard clip <-> tanh
            p.dist.bias    = fx::sweep (tick, 0.11, 2.3, -0.35f, 0.35f);  // even harmonics
            p.dist.mix     = fx::sweep (tick, 0.23, 0.7, 40.0f, 100.0f);
            p.dist.outputDb = -9.0f;
            p.dist.preLoShelfHz  =  140.0f; p.dist.preLoShelfDb  =  6.0f;
            p.dist.preHiShelfHz  = 6000.0f; p.dist.preHiShelfDb  = -4.0f;
            p.dist.postLoShelfHz =   90.0f; p.dist.postLoShelfDb = -3.0f;
            p.dist.postHiShelfHz = 9000.0f; p.dist.postHiShelfDb =  5.0f;

            // Variant: the oversampling factor. Rebuilding the oversampler
            // cannot be crossfaded, and it changes the reported latency.
            p.dist.oversample = vB ? 3 : (vA ? 2 : 1);   // off -> 2x -> 4x
            break;
        }

        case Id::FxEq:
        {
            p.eq[0].bypass = off ? 1 : 0;

            p.eq[0].shape[0] = 1;                                   // low cut
            p.eq[0].freqHz[0] = 60.0f;  p.eq[0].slope[0] = 0.9f;

            p.eq[0].shape[1] = 2;                                   // low shelf
            p.eq[0].freqHz[1] = 220.0f; p.eq[0].gainDb[1] = -6.0f; p.eq[0].slope[1] = 0.6f;

            p.eq[0].shape[2] = 3;                                   // swept peak
            p.eq[0].freqHz[2] = fx::sweep (tick, 0.29, 0.0, 400.0f, 3000.0f);
            p.eq[0].gainDb[2] = fx::sweep (tick, 0.19, 1.9, -12.0f,  12.0f);
            p.eq[0].q[2]      = fx::sweep (tick, 0.13, 0.4,   0.4f,   6.0f);

            // The EQ has no variantPending — a band that changes SHAPE is its
            // equivalent, and the bank has to redesign rather than interpolate.
            p.eq[0].shape[3] = vA ? 5 : 3;                          // peak -> high shelf
            p.eq[0].freqHz[3] = 3500.0f; p.eq[0].gainDb[3] = 5.0f; p.eq[0].q[3] = 1.2f;

            p.eq[0].shape[4] = 7;                                   // all pass
            p.eq[0].freqHz[4] = 1200.0f; p.eq[0].q[4] = 0.8f;

            p.eq[0].shape[5] = vB ? 3 : 6;                          // high cut -> peak
            p.eq[0].freqHz[5] = 9000.0f; p.eq[0].gainDb[5] = -8.0f;
            p.eq[0].q[5] = 1.5f; p.eq[0].slope[5] = 0.8f;
            break;
        }

        case Id::FxDyn:
        {
            p.dyn[0].bypass = off ? 1 : 0;
            p.dyn[0].compOn = 1;
            p.dyn[0].expOn  = (tick >= 60) ? 1 : 0;   // switch-on edge clears the stage

            p.dyn[0].compThresholdDb = fx::sweep (tick, 0.21, 0.0, -40.0f, -6.0f);
            p.dyn[0].compRatio       = fx::sweep (tick, 0.13, 1.4,   1.5f, 20.0f);
            p.dyn[0].compAttackMs    = fx::sweep (tick, 0.09, 2.2,   0.5f, 60.0f);
            p.dyn[0].compKneeDb      = 6.0f;
            p.dyn[0].compReleaseMs   = 120.0f;
            p.dyn[0].compDetectorDelayMs = 2.0f;      // transient pass, not lookahead
            p.dyn[0].compScLoCutHz = 80.0f;
            p.dyn[0].compScHiCutHz = 8000.0f;

            p.dyn[0].autoMakeup = (tick >= 40) ? 1 : 0;
            p.dyn[0].makeupDb = 2.0f;

            p.dyn[0].expThresholdDb = -38.0f;
            p.dyn[0].expRatio = 3.0f;
            p.dyn[0].expRangeDb = -30.0f;
            p.dyn[0].expAttackMs = 5.0f;
            p.dyn[0].expReleaseMs = 80.0f;
            p.dyn[0].expHoldMs = 15.0f;
            p.dyn[0].expScLoCutHz = 60.0f;
            p.dyn[0].expScHiCutHz = 9000.0f;

            // Two variants, both needing silence: the detector mode rebuilds
            // the followers, the lookahead re-times the audio delay line.
            p.dyn[0].detector    = vA ? 1 : 0;        // peak -> RMS
            p.dyn[0].lookaheadMs = vB ? 5.0f : 1.0f;
            break;
        }

        case Id::FxMod:
        {
            p.mod.bypass = off ? 1 : 0;
            p.mod.rateHz   = fx::sweep (tick, 0.11, 0.0,   0.2f,  3.0f);
            p.mod.depth    = fx::sweep (tick, 0.17, 1.3,  15.0f, 90.0f);
            p.mod.delayMs  = fx::sweep (tick, 0.07, 2.6,   4.0f, 25.0f);
            p.mod.feedback = fx::sweep (tick, 0.23, 0.9, -60.0f, 60.0f);
            p.mod.mix      = fx::sweep (tick, 0.29, 1.8,  25.0f, 85.0f);
            p.mod.phaseDeg = 120.0f;
            p.mod.loCutHz  = 120.0f;
            p.mod.shape    = (tick >= 60) ? 2 : 1;    // LFO waveform, no silence needed

            // Variants: the voice count and through-zero both change topology.
            p.mod.voices      = vA ? 3 : 2;           // kMaxVoices is 3
            p.mod.throughZero = vB ? 1 : 0;
            break;
        }

        case Id::FxPhaser:
        {
            p.phaser.bypass = off ? 1 : 0;
            p.phaser.centreHz  = fx::sweep (tick, 0.13, 0.0, 200.0f, 3000.0f);
            p.phaser.spreadOct = fx::sweep (tick, 0.07, 1.5,   0.2f,    2.5f);
            p.phaser.rateHz    = fx::sweep (tick, 0.19, 2.4,   0.1f,    2.0f);
            p.phaser.depthOct  = fx::sweep (tick, 0.11, 0.6,   0.5f,    3.5f);
            p.phaser.feedback  = fx::sweep (tick, 0.23, 1.1, -80.0f,   80.0f);
            p.phaser.mix       = fx::sweep (tick, 0.29, 3.0,  30.0f,   90.0f);
            p.phaser.shape     = (tick >= 60) ? 3 : 1;

            // Variant: the stage count (4, 6, 8 or 12 are the legal values).
            p.phaser.stages = vB ? 4 : (vA ? 12 : 6);
            break;
        }

        case Id::FxTrem:
        {
            // No variant parameter exists on this module; the bypass toggle and
            // the sweeps are its whole surface.
            p.trem.bypass = off ? 1 : 0;
            p.trem.rateHz  = fx::sweep (tick, 0.13, 0.0,  1.0f,  12.0f);
            p.trem.depthDb = fx::sweep (tick, 0.09, 1.7,  3.0f,  24.0f);
            p.trem.shape   = fx::sweep (tick, 0.05, 2.9,  0.0f,   1.0f);   // sine <-> triangle
            p.trem.mix     = fx::sweep (tick, 0.21, 0.8, 40.0f, 100.0f);
            break;
        }

        case Id::FxReverb:
        {
            p.reverb.bypass = off ? 1 : 0;
            p.reverb.predelayMs = fx::sweep (tick, 0.07, 0.0,    0.0f,    60.0f);
            p.reverb.rt60       = fx::sweep (tick, 0.11, 1.2,    0.6f,     4.0f);
            p.reverb.diffusion  = fx::sweep (tick, 0.17, 2.1,    0.0f,     1.0f);
            p.reverb.toneHz     = fx::sweep (tick, 0.13, 0.5, 2000.0f, 16000.0f);
            p.reverb.mix        = fx::sweep (tick, 0.23, 1.6,   20.0f,    80.0f);
            p.reverb.rt60LowMult   = 1.6f;
            p.reverb.rt60HighMult  = 0.35f;
            p.reverb.crossoverLow  = 180.0f;
            p.reverb.crossoverHigh = 3500.0f;

            // Size is another network: the module builds it on its idle twin
            // and the old tail spills over underneath (until spatcore 0.4 it
            // faded the slot out and rebuilt at silence).
            p.reverb.size = vB ? 0.75f : (vA ? 1.75f : 1.0f);
            break;
        }

        case Id::FxDelay:
        {
            p.delay.bypass = off ? 1 : 0;
            p.delay.timeMs      = fx::sweep (tick, 0.09, 0.0, 40.0f, 400.0f);
            p.delay.feedback    = fx::sweep (tick, 0.13, 1.4, 10.0f,  70.0f);
            p.delay.mix         = fx::sweep (tick, 0.19, 2.2, 20.0f,  80.0f);
            p.delay.diffusion   = fx::sweep (tick, 0.07, 0.9,  0.0f,   1.0f);
            p.delay.modDepthPct = fx::sweep (tick, 0.11, 1.1,  0.0f,  30.0f);
            p.delay.modRateHz = 0.6f;
            p.delay.inLoCutHz = 90.0f;
            p.delay.fbLoShelfHz = 250.0f;  p.delay.fbLoShelfDb =  4.0f;
            p.delay.fbHiShelfHz = 3500.0f; p.delay.fbHiShelfDb = -8.0f;
            p.delay.glideMs = 120.0f;

            // Manual tap times, used once tapMode switches away from the
            // pattern generator below.
            const float manual[8] = { 55.0f, 130.0f, 240.0f, 390.0f,
                                      570.0f, 780.0f, 1020.0f, 1290.0f };
            for (int k = 0; k < 8; ++k)
            {
                p.delay.tapTimeMs[k]  = manual[k];
                p.delay.tapLevelDb[k] = -2.0f * static_cast<float> (k);
            }

            // This module reports no variant — every discrete edit glides
            // instead (tap gains fade, the feedback tap crossfades), which is
            // exactly the behaviour worth having in a hash.
            p.delay.taps        = vA ? 5 : 3;
            p.delay.pattern     = (tick >= 60) ? 2 : 0;
            p.delay.tapMode     = vB ? 0 : 1;             // pattern -> manual
            p.delay.feedbackTap = (tick >= 85) ? 2 : 0;
            break;
        }

        case Id::FxCrush:
        {
            p.crush.bypass = off ? 1 : 0;
            p.crush.bits   = fx::sweep (tick, 0.11, 0.0,    3.0f,    14.0f);
            p.crush.rateHz = fx::sweep (tick, 0.17, 1.9, 2000.0f, 20000.0f);
            p.crush.mix    = fx::sweep (tick, 0.23, 0.6,   40.0f,   100.0f);

            // -96 dB is the documented "off"; crossing it turns the keyed
            // dither noise on, which is this module's only stochastic element.
            p.crush.ditherDb = (tick >= 40) ? -60.0f : -96.0f;

            // Variant: the decimation filter (hold/aliasing vs anti-aliased),
            // switched on at A and back off at B.
            p.crush.filter = (vA && ! vB) ? 1 : 0;
            break;
        }

        case Id::FxChain:
        {
            // Every slot live, at settings that stay well behaved when eleven
            // of them are stacked. The point of this scenario is the CHAIN —
            // order, bypass, mute — not a second pass at each module's own
            // extremes, so only three parameters sweep.
            p.dist.bypass = 0; p.dist.oversample = 2; p.dist.shape = 0.8f;
            p.dist.outputDb = -8.0f; p.dist.mix = 60.0f;
            p.dist.driveDb = fx::sweep (tick, 0.07, 1.3, 6.0f, 18.0f);

            p.eq[0].bypass = 0;
            p.eq[0].shape[2] = 3; p.eq[0].freqHz[2] = 900.0f; p.eq[0].gainDb[2] = 4.0f;

            // eq2: the second EQ instance, bypassed for one window so its slot
            // fades out, resets at silence and fades back in (see fx::kChainEq2*).
            p.eq[1].bypass = (tick >= fx::kChainEq2BypassOn
                              && tick < fx::kChainEq2BypassOff) ? 1 : 0;
            p.eq[1].shape[3] = 3; p.eq[1].freqHz[3] = 2600.0f; p.eq[1].gainDb[3] = -5.0f;

            p.dyn[0].bypass = 0; p.dyn[0].compOn = 1; p.dyn[0].expOn = 0;
            p.dyn[0].compThresholdDb = -18.0f; p.dyn[0].compRatio = 4.0f;
            p.dyn[0].compAttackMs = 8.0f; p.dyn[0].compReleaseMs = 120.0f;
            p.dyn[0].autoMakeup = 1;

            // dyn2: the second dynamics instance, the only slot in the harness
            // that gets BOTH a bypass window and a variant commit inside the
            // chain — the detector mode rebuilds the followers, so it cannot be
            // interpolated and has to be taken at silence.
            p.dyn[1].bypass = (tick >= fx::kChainDyn2BypassOn
                               && tick < fx::kChainDyn2BypassOff) ? 1 : 0;
            p.dyn[1].compOn = 0; p.dyn[1].expOn = 1;
            p.dyn[1].expThresholdDb = -45.0f; p.dyn[1].expRatio = 2.5f;
            p.dyn[1].expRangeDb = -24.0f;
            p.dyn[1].detector = (tick >= fx::kChainDyn2Variant) ? 1 : 0;   // peak -> RMS

            p.mod.bypass = 0; p.mod.rateHz = 0.7f; p.mod.depth = 40.0f;
            p.mod.delayMs = 12.0f; p.mod.feedback = 25.0f; p.mod.mix = 40.0f;

            p.phaser.bypass = 0; p.phaser.rateHz = 0.4f; p.phaser.stages = 8;
            p.phaser.centreHz = 700.0f; p.phaser.depthOct = 2.0f;
            p.phaser.feedback = 35.0f; p.phaser.mix = 45.0f;

            p.trem.bypass = 0; p.trem.depthDb = 6.0f; p.trem.mix = 70.0f;
            p.trem.rateHz = fx::sweep (tick, 0.11, 0.0, 2.0f, 8.0f);

            p.reverb.bypass = 0; p.reverb.predelayMs = 18.0f; p.reverb.rt60 = 2.2f;
            p.reverb.diffusion = 0.6f; p.reverb.toneHz = 9000.0f; p.reverb.size = 1.25f;
            p.reverb.mix = fx::sweep (tick, 0.09, 2.1, 10.0f, 50.0f);

            p.delay.bypass = 0; p.delay.taps = 3; p.delay.timeMs = 220.0f;
            p.delay.feedback = 35.0f; p.delay.mix = 30.0f; p.delay.fbHiShelfDb = -6.0f;

            p.crush.bypass = 0; p.crush.bits = 10.0f; p.crush.rateHz = 16000.0f;
            p.crush.mix = 35.0f;

            //---- the part no unit test can cover at render scale -------------
            // Three reorders, one of them (C) inside the chain-bypass window,
            // so BOTH swap paths are in the hash: the muted swap at a block
            // boundary, and the "silent anyway, just take it" branch.
            const char* order = fx::kOrderDefault;
            if (tick >= 70)      order = fx::kOrderD;
            else if (tick >= 58) order = fx::kOrderC;
            else if (tick >= 40) order = fx::kOrderB;
            (void) parseChainOrder (order, p.order);

            p.chainBypass = (tick >= 55 && tick < 65) ? 1 : 0;
            p.mute        = (tick >= 90 && tick < 96) ? 1 : 0;
            break;
        }

        case Id::FxReverbEr:
        {
            // The FDN with reflections in front: the profile and the size are
            // build-time, so each change starts a new world and the old room's
            // reflections play out of the ring behind the new one.
            p.reverb.bypass = off ? 1 : 0;
            p.reverb.rt60 = 1.8f;
            p.reverb.predelayMs = fx::sweep (tick, 0.05, 0.4, 0.0f, 20.0f);
            p.reverb.erLevelDb = fx::sweep (tick, 0.13, 1.0, -12.0f, 0.0f);
            p.reverb.mix = 60.0f;
            p.reverb.erProfile = vB ? 4 : (vA ? 3 : 1);            // Room -> Hall -> Cathedral
            p.reverb.size = vB ? 1.4f : 1.0f;
            break;
        }

        case Id::FxReverbPlate:
        {
            p.reverb.bypass = off ? 1 : 0;
            p.reverb.model = 1;
            p.reverb.rt60 = fx::sweep (tick, 0.09, 0.3, 1.0f, 3.5f);
            p.reverb.diffusion = fx::sweep (tick, 0.15, 1.7, 0.4f, 1.0f);
            p.reverb.modDepth = fx::sweep (tick, 0.21, 0.9, 0.0f, 100.0f);
            p.reverb.modRateHz = fx::sweep (tick, 0.07, 2.2, 0.2f, 3.0f);
            p.reverb.toneHz = fx::sweep (tick, 0.11, 0.6, 4000.0f, 16000.0f);
            p.reverb.mix = 70.0f;
            p.reverb.size = vB ? 0.7f : (vA ? 1.6f : 1.0f);
            break;
        }

        case Id::FxReverbHall:
        {
            p.reverb.bypass = off ? 1 : 0;
            p.reverb.model = 4;
            p.reverb.rt60 = 2.6f;
            p.reverb.rt60LowMult = 1.4f;
            p.reverb.rt60HighMult = 0.45f;
            p.reverb.diffusion = 0.7f;
            p.reverb.modDepth = fx::sweep (tick, 0.17, 0.2, 20.0f, 90.0f);
            p.reverb.modRateHz = 0.6f;
            p.reverb.mix = 65.0f;
            p.reverb.erProfile = vB ? 0 : 2;                        // Chamber, then off
            p.reverb.size = vA ? 1.5f : 1.0f;
            break;
        }

        case Id::FxReverbShimmer:
        {
            p.reverb.bypass = off ? 1 : 0;
            p.reverb.model = 5;
            p.reverb.rt60 = 4.0f;
            p.reverb.rt60HighMult = 0.5f;
            p.reverb.diffusion = 0.75f;
            p.reverb.size = 1.6f;
            p.reverb.shimmerAmount = fx::sweep (tick, 0.19, 0.5, 20.0f, 80.0f);
            p.reverb.shimmerPitch = vB ? 7 : (vA ? 1 : 0);         // +12 -> +7 -> -12 & +12
            p.reverb.mix = 70.0f;
            break;
        }

        case Id::FxReverbModels:
        {
            // Every class of tail in turn, each change a spillover - then from
            // tick 84 a new model every tick, faster than a crossfade and a
            // dying fade can clear, so the pool has to wait, steal the oldest
            // tail and land on the last request.
            p.reverb.bypass = 0;
            p.reverb.rt60 = 2.0f;
            p.reverb.mix = 60.0f;
            p.reverb.toneHz = 14000.0f;
            static const std::uint8_t burst[] = { 1, 4, 0, 5, 1, 0, 4 };
            if (tick >= 84 && tick < 91)
                p.reverb.model = burst[tick - 84];
            else if (tick >= 91)
                p.reverb.model = 4;
            else
                p.reverb.model = tick >= 66 ? 5 : (tick >= 48 ? 4 : (tick >= 30 ? 1 : 0));
            p.reverb.erProfile = (tick >= 48 && tick < 66) ? 3 : 0;
            break;
        }

        case Id::FxEngine:
            // Four channels with four different parameter sets: this one is
            // per CHANNEL, so it has its own function (engineChannelParams
            // below) and this single-channel entry point is never asked for
            // it. Named rather than defaulted so that adding a scenario
            // cannot make it fall through here unnoticed.
            break;

        case Id::Static:
        case Id::Moving:
        case Id::FrToggle:
        case Id::Stereo:
        case Id::StereoNull:
        case Id::StereoNullMono:
            break;   // not effects scenarios: every module stays bypassed
    }

    return p;
}
//==============================================================================
// THE ENGINE scenario (--scenario engine): effects/EffectsEngineCore driven
// end to end. It is the only scenario in this harness that renders the ENGINE
// rather than the DSP inside it, so it is built entirely out of what only a
// render can reach: the block ledger over two hundred callbacks, a chain
// reorder, a chain bypass window, two kinds of mute window, three scripted
// DRIVER STALLS, an emergency Clear, the operator's global guard switch, and
// the effect-to-effect path driven hard enough that the loop guard trips, is
// held off by its return veto, backs off on a second trip, and releases.
//
// EVERYTHING HERE IS ASSERTED, not merely hashed. A hash notices that a
// behaviour CHANGED; it cannot notice that one stopped happening at all, and
// the first run on a new machine records the hash from the assertions alone
// (--update). So the runner checks the per-channel trip vector, the exact
// ledger (batches, underruns, discards, overflows) and the exact resync and
// clear counts against what this file predicts, before any baseline is
// consulted.
//
// FOUR CHANNELS, one job each, because a scenario in which every channel does
// everything hashes the same way whatever broke:
//
//   fx0  loop leg A, the RETURN VETO, and TWO REORDERS (ticks 14 and 68 - the
//        second lands while its own guard is holding the loop down)
//   fx1  loop leg B, the BACKOFF LADDER (a second consecutive trip, held twice
//        as long), plus a CHAIN BYPASS window (ticks 22..30)
//   fx2  the long tail (reverb), a channel MUTE window (ticks 56..64) and the
//        per-channel emergency CLEAR at tick 58. Nothing feeds it from the
//        effect rows, so its guard renders the armed and idle path next to two
//        that are working.
//   fx3  fed one-way from fx0's return: an A -> B hop that never runs away, so
//        the ledger's two-block figure is in the hash beside the loop. It is
//        also the channel the published matrix DROPS at ticks 76..83, which is
//        the "routed count is not the live count" path: unrouted, its chain
//        still has to run and its return ring still has to feed the callback.
//
// THE LOOP, and why these numbers rather than tuned ones. fx0 and fx1 feed
// each other, and BOTH chains end at a hard clip (distortion, shape 0,
// oversampling off), so each return is bounded by construction at about 0.8:
// a runaway here builds to a known ceiling instead of to infinity, and a
// render that exploded would gate the NaN trap rather than the engine. At
// kLoopRunawayLevel the pre-gain effect-to-effect bus is therefore 3 to 6 -
// two to three times the guard's +6 dBFS ceiling (1.995 linear) with no
// reliance on a chain gain to three figures - and at the safe levels it is a
// third of the release threshold (the ceiling less 12 dB of hysteresis,
// 0.501) or less.
//
// THE VETO NEEDS A HOT RETURN OVER A CALM FEED, which is exactly the case
// LoopGuard documents: the guard never touches input-to-effect feeds, so a
// channel can sit far over the ceiling with its loop already cut. That is what
// the fx0 HOT windows are. Inside one, the input sends into fx0 are multiplied
// by kFx0HotInputFactor and its distortion is given +12 dB of post-clip
// makeup, so its return is pinned at 0.8 * 3.981 = 3.185: over the ceiling by
// 60 %, bounded by the clip, and completely independent of what the loop is
// doing. The first window (18..37) holds fx0's release off for twenty ticks
// while fx1 - whose own return is the bare 0.8 - releases on schedule beside
// it; the second (43..47) drives fx1's re-trip and vetoes fx0 a second time.
// The sends OUT of fx0 are 0.10 rather than 0.25 for the same reason: at 3.185
// a quarter would put fx1's bus over the release threshold, and fx1 would
// never let go.
//
// TICKS at the default shape (200 blocks x 512 @ 48 kHz = ticks 0..106; one
// block is 10.67 ms, one tick 20 ms):
//
//    4       the loop is cranked both ways. Both buses go over the ceiling at
//            once and both guards trip one trip time (60 ms) later
//   11       the loop is taken back to safe levels: both buses read calm
//   14       fx0 reorder: the shipped default order -> kOrderB, which puts the
//            clipper LAST and is what makes the hot window's 3.185 exact
//   18..37   fx0 HOT: the RETURN VETO holds its release off
//   22..30   fx1 chain bypass
//   28       fx1's dither floor crosses -96 dB, the engine's only keyed noise
//   43..47   fx0 HOT again, and fx0 -> fx1 cranked: fx1 re-trips - its second
//            CONSECUTIVE trip, so the backoff ladder doubles its hold - and
//            fx0's calm clock is vetoed back to zero a second time
//   56..64   fx2 channel mute - the CHAIN's mute, so its modules keep running
//            and the reverb tail moves on rather than freezing
//   58       requestClear(2): the per-channel emergency Clear, mid-mute, which
//            takes that tail away outright. The difference between the two
//            is the whole point of putting them one inside the other
//   68       fx0 reorder: kOrderB -> kOrderC, inside the guard's hold
//   76..83   the published matrix declares THREE effects while four are live:
//            fx3 is unrouted, and has to keep running anyway
//   86..91   setLoopGuardEnabled(false): fx1's doubled hold ends the way an
//            operator ends one, with the held feed snapped back to unity
//   95..100  ENGINE mute - EffectsEngineCore::setMuted, which silences every
//            channel's FEED and lets every chain keep running. A different
//            thing from the chain mute at 56, and only the engine has it
//  103       requestClear(-1): the emergency Clear on EVERY channel, which is
//            also the only thing that resets the shared feed history
//
// THE DRIVER STALLS are counted in CALLBACKS, not ticks, because that is what
// a stalled driver thread is: the audio callback keeps running - it keeps
// pulling returns, and keeps counting underruns while it gets none - and the
// engine's own thread misses wakes. Three of them, one per branch:
//
//   block 44       ONE missed wake. Two blocks are then resident on every
//                  source, so the next callback runs TWO batches, and the pull
//                  after it finds two blocks in a ring whose cushion is one:
//                  the pullReturn DISCARD rule has to choose what to trim.
//   blocks 116-117 TWO missed wakes. Three blocks resident is past
//                  maxSourceBacklogBlocks, so processBatch jumps FORWARD
//                  instead of working through the backlog: resync, and every
//                  chain reset because the input stream just skipped.
//   blocks 132-139 EIGHT missed wakes. Nine blocks written into an eight-block
//                  ring is a LAP, which the modular "available" cannot express
//                  and only the additive counter can see.
//
// Their cost is predicted exactly by expectedLedger() below and asserted, so
// "the engine fell behind" is a NUMBER this scenario knows rather than a
// tolerance it waives.
//==============================================================================
namespace engine
{
    /** Effects channels. Fixed, because the timeline below names them one by
        one; the INPUT count follows --in like every other scenario. */
    inline constexpr int kNumEffects = 4;

    /** The engine shape, HERE rather than at the call site, because the ledger
        arithmetic further down is derived from these four numbers: a driver
        that configured the engine differently would predict the wrong counts
        in silence. */
    inline constexpr int kSourceRingBlocks    = 8;
    inline constexpr int kReturnRingBlocks    = 8;
    inline constexpr int kReturnCushionBlocks = 1;
    inline constexpr int kBacklogBlocks       = 2;

    inline constexpr int kLoopArm       = 4;
    inline constexpr int kLoopSafe      = 11;
    inline constexpr int kReorderA      = 14;
    inline constexpr int kFx0HotOn      = 18;
    inline constexpr int kFx0HotOff     = 38;
    inline constexpr int kBypassOn      = 22;
    inline constexpr int kBypassOff     = 31;
    inline constexpr int kDitherOn      = 28;
    inline constexpr int kCrank2On      = 43;
    inline constexpr int kCrank2Off     = 48;
    inline constexpr int kChanMuteOn    = 56;
    inline constexpr int kChanMuteOff   = 65;
    inline constexpr int kClearChannel  = 58;
    inline constexpr int kReorderB      = 68;
    inline constexpr int kUnroutedOn    = 76;
    inline constexpr int kUnroutedOff   = 84;
    inline constexpr int kGuardOffOn    = 86;
    inline constexpr int kGuardOffOff   = 92;
    inline constexpr int kEngineMuteOn  = 95;
    inline constexpr int kEngineMuteOff = 101;
    inline constexpr int kClearAll      = 103;

    /** The channel the per-channel Clear at kClearChannel names. */
    inline constexpr int kClearChannelIndex = 2;

    /** How many effects the matrix DECLARES while fx3 is dropped. */
    inline constexpr int kUnroutedEffectCount = 3;

    /** Multiplied by a clipped return this clears the guard's ceiling two to
        three times over, so the trip does not hang on an exact chain gain. */
    inline constexpr float kLoopRunawayLevel = 6.0f;

    /** fx0 -> fx1 at rest. A third of the 1 -> 0 level, because fx0's return
        is 3.185 rather than 0.8 inside a hot window and a quarter of that
        would sit over fx1's release threshold. */
    inline constexpr float kSafeLevel01 = 0.10f;

    /** fx1 -> fx0 at rest, against a return the clip pins at 0.8. */
    inline constexpr float kSafeLevel10 = 0.25f;

    /** fx0 -> fx3: the one-way hop, never loud enough to trip anything - 0.955
        against a 1.995 ceiling even with fx0's return at its hot 3.185. */
    inline constexpr float kHopLevel = 0.3f;

    /** What the input sends into fx0 are multiplied by inside a hot window.
        Deliberately far past what the clipper needs: the clip bounds the
        return at 0.8 whatever arrives, so there is no cost to a margin here
        and every reason to want one. A hot window that failed to reach the
        clip would leave the veto's 3.185 at the mercy of how eight sines
        happened to line up inside one block. */
    inline constexpr float kFx0HotInputFactor = 200.0f;

    /** Post-clip makeup inside a hot window. +12 dB is the module's own clamp,
        and 0.8 * 3.981 = 3.185 is 60 % over the guard's +6 dBFS ceiling. */
    inline constexpr float kFx0HotOutputDb = 12.0f;

    /** True while fx0 is being driven into its clipper with the makeup on,
        which is what pins its return over the ceiling with its loop feed
        calm - the one state the return veto exists for. */
    inline bool fx0ReturnHot (int tick) noexcept
    {
        return (tick >= kFx0HotOn && tick < kFx0HotOff)
            || (tick >= kCrank2On && tick < kCrank2Off);
    }

    /** The ENGINE's mute (EffectsEngineCore::setMuted), not the chain's. */
    inline bool engineMuted (int tick) noexcept
    {
        return tick >= kEngineMuteOn && tick < kEngineMuteOff;
    }

    /** The operator's global loop-guard switch. Turning it OFF re-arms every
        guard and snaps a held feed straight back to unity - LoopGuard's one
        documented non-click-free transition, and the only thing in this
        timeline that ends fx1's doubled hold. */
    inline bool loopGuardEnabled (int tick) noexcept
    {
        return ! (tick >= kGuardOffOn && tick < kGuardOffOff);
    }

    /** How many effects channels the published matrix declares. Three inside
        the unrouted window; the engine must still run all four. */
    inline int publishedEffectCount (int tick) noexcept
    {
        return (tick >= kUnroutedOn && tick < kUnroutedOff) ? kUnroutedEffectCount
                                                            : kNumEffects;
    }

    /** No clear at all. Not -1, which is requestClear's "every channel". */
    inline constexpr int kNoClear = -2;

    /** The argument for requestClear() on the tick the render has just entered,
        or kNoClear. Both of its branches are scripted: the per-channel clear,
        which drops one chain's tails, and the all-channel clear, which is the
        only thing that also resets the shared feed history. */
    inline int clearRequestAt (int tick) noexcept
    {
        if (tick == kClearChannel) return kClearChannelIndex;
        if (tick == kClearAll)     return -1;
        return kNoClear;
    }

    /** True once the render is long enough to have reached the guard's first
        trip. Short shapes stop before tick 4 and legitimately never see one;
        only a run that got there may be held to it. */
    inline bool reachesLoopGuardTrip (int finalTick) noexcept
    {
        return finalTick >= kLoopArm + 6;
    }

    /** True once it is long enough for every hold to have been let go of.
        fx1's second hold is the doubled one, and the guard switch at
        kGuardOffOn is what ends it. */
    inline bool reachesLoopGuardRelease (int finalTick) noexcept
    {
        return finalTick >= kGuardOffOn + 2;
    }

    /** The per-channel trip counts this timeline predicts, in the note line's
        own format, or false when the run stops mid-event and the count is
        legitimately in flight.

        PER CHANNEL, and that is the whole point of predicting it rather than a
        total: a total of three reached any other way - all three on one leg,
        or one on the reverb channel nothing feeds - is a different engine, and
        a total is exactly what would not notice. */
    inline bool expectedTripVector (int finalTick, std::string& out)
    {
        if (finalTick < kLoopArm)
        {
            out = "0,0,0,0";
            return true;
        }

        if (finalTick >= kLoopArm + 6 && finalTick < kCrank2On)
        {
            out = "1,1,0,0";
            return true;
        }

        if (finalTick >= kCrank2On + 6)
        {
            out = "1,2,0,0";
            return true;
        }

        return false;
    }

    /** How many requestClear() calls have been made AND honoured by the time
        the run ends. One asked for on the very last tick may have no batch
        left to honour it, so the tick has to be strictly past. */
    inline std::uint32_t expectedClears (int finalTick) noexcept
    {
        std::uint32_t n = 0;

        if (finalTick > kClearChannel) ++n;
        if (finalTick > kClearAll)     ++n;

        return n;
    }

    //==========================================================================
    /** One scripted driver stall: the engine's thread misses `callbacks`
        consecutive wakes from `firstBlock` on. The audio callback keeps
        running throughout, which is what makes the arithmetic below
        predictable rather than a tolerance. */
    struct DriverStall
    {
        int firstBlock;
        int callbacks;
    };

    inline constexpr DriverStall kStalls[] = {
        {  44, 1 },     // -> two batches on one wake, then a pullReturn DISCARD
        { 116, 2 },     // -> three blocks resident: the BACKLOG resync
        { 132, 8 },     // -> nine blocks into an eight-block ring: the LAP
    };

    inline constexpr int kNumStalls = static_cast<int> (sizeof (kStalls) / sizeof (kStalls[0]));

    inline bool driverStalled (int block) noexcept
    {
        for (const auto& s : kStalls)
            if (block >= s.firstBlock && block < s.firstBlock + s.callbacks)
                return true;

        return false;
    }

    /** What the block ledger must read at the end of a run of `blocks`
        callbacks, derived from the stall table and the engine shape above
        rather than written down beside them, so the two cannot drift apart.

        THE ARITHMETIC, once, for all three stalls. A stall of L callbacks
        leaves L + 1 blocks resident on every source at the recovery callback
        (L missed writes, plus that callback's own), and the audio callback
        pulls a return on each of the L callbacks after the last one the ring
        still had a block for: L underruns.

          L + 1 <= backlog    the backlog test is not exceeded, so the recovery
                              wake runs BOTH batches and no batch is lost. The
                              return ring then holds two blocks against a
                              cushion of one, and the next pull DISCARDS the
                              older of them.
          backlog < L + 1     past the allowance but inside the ring:
            <= ring - 1       processBatch resyncs every source to one block
                              behind the head, counts a SOURCE SKIP, resets
                              every chain because the input stream jumped, and
                              the L missed batches are gone for good.
          L + 1 > ring - 1    the producer has lapped the consumer, which the
                              per-cursor "available" cannot express at all: the
                              additive counter catches it, counts a RING WRAP,
                              and resyncs the same way. */
    struct LedgerExpectation
    {
        bool          known               = true;   // false: a stall straddles the end
        int           batches             = 0;
        std::uint32_t underrunsPerChannel = 0;
        std::uint32_t discardsPerChannel  = 0;
        std::uint32_t sourceSkips         = 0;
        std::uint32_t ringWraps           = 0;
    };

    inline LedgerExpectation expectedLedger (int blocks) noexcept
    {
        LedgerExpectation e;
        e.batches = blocks;

        for (const auto& s : kStalls)
        {
            if (s.firstBlock >= blocks)
                continue;                           // never reached

            if (s.firstBlock + s.callbacks >= blocks)
            {
                e.known = false;                    // reached, but not recovered from
                continue;
            }

            e.underrunsPerChannel += static_cast<std::uint32_t> (s.callbacks);

            if (s.callbacks + 1 <= kBacklogBlocks)
            {
                e.discardsPerChannel += 1;
            }
            else
            {
                e.batches -= s.callbacks;

                if (s.callbacks + 1 > kSourceRingBlocks - 1)
                    e.ringWraps += 1;
                else
                    e.sourceSkips += 1;
            }
        }

        return e;
    }

    //==========================================================================
    /** The send from effect channel `from`'s RETURN into channel `to`. */
    inline float fxSendLevel (int from, int to, int tick) noexcept
    {
        const bool runaway = tick >= kLoopArm && tick < kLoopSafe;
        const bool crank2  = tick >= kCrank2On && tick < kCrank2Off;

        // The second crank is ONE WAY, and deliberately: fx0 is still being
        // held down at tick 43, so a two-way crank could not bootstrap the
        // loop at all. What drives fx1 over the ceiling instead is fx0's hot
        // return, 3.185 * 6 - which is also why the hot window and this one
        // are the same window.
        if (from == 0 && to == 1)
            return (runaway || crank2) ? kLoopRunawayLevel : kSafeLevel01;

        if (from == 1 && to == 0)
            return runaway ? kLoopRunawayLevel : kSafeLevel10;

        if (from == 0 && to == 3)
            return kHopLevel;

        return 0.0f;                     // and no channel ever feeds itself
    }

    /** Fractional on purpose: an effect row reaches a channel through the same
        delay line and interpolating tap as an input row, so it has to be
        rendered through one rather than at an integer number of samples. */
    inline float fxSendDelayMs (int from, int to) noexcept
    {
        if (from == 0 && to == 1) return 1.5f;
        if (from == 1 && to == 0) return 2.75f;
        if (from == 0 && to == 3) return 4.0f;
        return 0.0f;
    }

    /** Input -> effect send. One cell in seven is dead, so "this source does
        not reach this channel" is rendered too. */
    inline float inputSendLevel (int in, int fx, int tick) noexcept
    {
        if (((in * 3 + fx * 5) % 7) == 0)
            return 0.0f;

        const float base = (fx <= 1) ? 0.05f : 0.10f;   // see THE LOOP above
        const float hot  = (fx == 0 && fx0ReturnHot (tick)) ? kFx0HotInputFactor : 1.0f;

        return hot * base * (0.7f + 0.3f * fx::sweep01 (tick, 0.13, 0.31 * in + 0.77 * fx));
    }

    inline float inputSendDelayMs (int in, int fx, int tick) noexcept
    {
        const float fixed = 2.0f + 1.25f * static_cast<float> ((in * 3 + fx * 5) % 9);

        // One moving send per channel, so the tap's delay smoother has to glide
        // across a tick boundary instead of only ever holding still.
        return ((in % kNumEffects) == fx)
                   ? fixed + 2.0f * fx::sweep01 (tick, 0.07, 0.5 * fx)
                   : fixed;
    }

    inline float inputSendHfDb (int in, int fx) noexcept
    {
        return -1.0f * static_cast<float> ((in + 2 * fx) % 5);   // 0 .. -4 dB
    }

    //==========================================================================
    /** LoopGuard's NaN policy, gated directly rather than through a render.

        peakOf() IS the guard's NaN policy - a plain running max drops a NaN,
        because every comparison against one is false, and a channel that has
        gone non-finite then reads as the calmest in the engine. The render
        cannot gate it, and that is not an oversight: this scenario is bounded
        by a hard clip precisely so that a runaway does NOT go non-finite and
        gate the NaN trap instead of the engine, and a render that did produce
        one would be refused (exit 8) before its hash was ever recorded. So the
        policy is gated here instead, for the price of no render at all.

        Returns an EMPTY string when the policy holds, otherwise what broke. */
    inline std::string loopGuardSelfTestFailure()
    {
        using spatcore::effects::LoopGuard;

        float block[8] = { 0.1f, -0.2f, 0.3f, -0.1f, 0.05f, 0.0f, -0.4f, 0.2f };

        if (LoopGuard::peakOf (block, 8) != 0.4f)
            return "LoopGuard::peakOf does not return the absolute peak of a "
                   "finite block";

        // The NaN goes in the MIDDLE, and below a later sample, so a plain
        // `if (v > peak) peak = v;` steps straight over it and returns 0.4 -
        // which is exactly what this test is here to refuse.
        block[3] = std::numeric_limits<float>::quiet_NaN();

        if (! std::isnan (LoopGuard::peakOf (block, 8)))
            return "LoopGuard::peakOf drops a NaN, so a channel that has gone "
                   "non-finite reads as the calmest one in the engine";

        block[3] = std::numeric_limits<float>::infinity();

        if (! std::isinf (LoopGuard::peakOf (block, 8)))
            return "LoopGuard::peakOf drops a +inf";

        block[3] = -0.1f;

        // And the state machine's half of the same contract: a non-finite peak
        // counts as loud for the trip test and as not calm for both release
        // tests, so a channel producing NaN trips and STAYS tripped.
        LoopGuard guard;
        guard.prepare (48000.0);

        const float nan = std::numeric_limits<float>::quiet_NaN();

        for (int i = 0; i < 16; ++i)             // 16 x 512 = 170 ms, past the 60 ms trip
        {
            guard.observeBlock (nan, 0.0f, 512);
            guard.applyGain (block, 8);
        }

        if (! guard.isTripped())
            return "a LoopGuard fed a NaN feed peak never trips";

        for (int i = 0; i < 400; ++i)            // 400 x 512 = 4.3 s, past every release
        {
            guard.observeBlock (nan, 0.0f, 512);
            guard.applyGain (block, 8);
        }

        if (! guard.isTripped())
            return "a LoopGuard fed a NaN feed peak releases again - every "
                   "comparison against a NaN is false, so the false branch of "
                   "each test has to be the failure-safe one";

        return {};
    }
}

/** One effects channel's parameters for one 50 Hz tick of the engine scenario.
    A pure function of (channel, tick), like every other timeline in this file.

    The four channels carry DIFFERENT module sets deliberately. The ten module
    scenarios above already render each module's own extremes eight times over,
    so what four chains are worth spending here is four different JOBS inside
    one batch - two whose output is bounded by a clip because they are in a
    loop, one with a tail long enough to outlive a mute window, one fed from
    another channel's return. */
inline spatcore::effects::EffectChannelParams engineChannelParams (int fx, int tick)
{
    using namespace spatcore::effects;

    EffectChannelParams p;
    p.revision = static_cast<std::uint32_t> (tick) + 1u;   // the chain re-reads only on a move

    // Both loop legs END at a hard clip: shape 0 is the +-0.8 limiter, and
    // oversampling OFF keeps it an exact bound (an oversampled clip rings past
    // its own limit) while costing the channel no reported latency.
    const auto clipper = [] (DistortionParams& d, float driveDb, float outputDb)
    {
        d.bypass = 0;
        d.oversample = 1;          // off
        d.shape = 0.0f;            // hard clip at +-0.8, no tanh leg
        d.bias = 0.0f;
        d.driveDb = driveDb;
        d.outputDb = outputDb;
        d.mix = 100.0f;
    };

    switch (fx)
    {
        case 0:
        {
            // The makeup is POST-clip, so inside a hot window this return is
            // exactly 0.8 * 3.981 = 3.185 whatever arrives at the channel -
            // which is the whole basis of the return veto, and the reason the
            // reorder at tick 14 (kOrderB puts the clipper LAST) has to come
            // before the first hot window rather than after it.
            clipper (p.dist, 9.0f,
                     engine::fx0ReturnHot (tick) ? engine::kFx0HotOutputDb : 0.0f);

            // Cut-only, so nothing after the clip can put the return back over
            // the bound the loop arithmetic above is reasoned from.
            p.eq[0].bypass = 0;
            p.eq[0].shape[0] = 1; p.eq[0].freqHz[0] = 70.0f; p.eq[0].slope[0] = 0.8f;
            p.eq[0].shape[1] = 0;
            p.eq[0].shape[2] = 3;
            p.eq[0].freqHz[2] = fx::sweep (tick, 0.19, 0.0, 500.0f, 2400.0f);
            p.eq[0].gainDb[2] = -5.0f; p.eq[0].q[2] = 1.1f;
            p.eq[0].shape[3] = 0;
            p.eq[0].shape[4] = 0;
            p.eq[0].shape[5] = 6; p.eq[0].freqHz[5] = 11000.0f; p.eq[0].slope[5] = 0.7f;

            p.trem.bypass = 0;
            p.trem.rateHz  = fx::sweep (tick, 0.11, 1.3, 2.0f, 7.0f);
            p.trem.depthDb = 8.0f;
            p.trem.shape   = 0.3f;
            p.trem.mix     = 70.0f;

            // TWO REORDERS, through the shipped parser and the same order
            // strings effectsSelfTestFailure() already checks for the chain
            // scenario - so a typo in one of them is caught there, once.
            const char* order = fx::kOrderDefault;
            if (tick >= engine::kReorderB)      order = fx::kOrderC;
            else if (tick >= engine::kReorderA) order = fx::kOrderB;
            (void) parseChainOrder (order, p.order);
            break;
        }

        case 1:
        {
            clipper (p.dist, 9.0f, 0.0f);

            p.dyn[0].bypass = 0;
            p.dyn[0].compOn = 1;
            p.dyn[0].expOn = 0;
            p.dyn[0].detector = 0;
            p.dyn[0].autoMakeup = 0;
            p.dyn[0].makeupDb = 0.0f;
            p.dyn[0].lookaheadMs = 1.0f;
            p.dyn[0].compThresholdDb = -6.0f;
            p.dyn[0].compRatio = 3.0f;
            p.dyn[0].compKneeDb = 3.0f;
            p.dyn[0].compAttackMs = 5.0f;
            p.dyn[0].compReleaseMs = 90.0f;
            p.dyn[0].compScLoCutHz = 70.0f;
            p.dyn[0].compScHiCutHz = 9000.0f;

            p.crush.bypass = 0;
            p.crush.bits = 10.0f;
            p.crush.rateHz = 18000.0f;
            p.crush.mix = 60.0f;

            // -96 dB is the documented "off"; crossing it turns the per-channel
            // keyed dither on, which is the only keyed noise anywhere in this
            // scenario and therefore the only thing that would collapse if the
            // engine ever handed every chain the same noise key.
            p.crush.ditherDb = (tick >= engine::kDitherOn) ? -70.0f : -96.0f;

            p.chainBypass = (tick >= engine::kBypassOn && tick < engine::kBypassOff) ? 1 : 0;
            break;
        }

        case 2:
        {
            p.reverb.bypass = 0;
            p.reverb.predelayMs = 15.0f;
            p.reverb.rt60 = 1.8f;
            p.reverb.rt60LowMult = 1.4f;
            p.reverb.rt60HighMult = 0.4f;
            p.reverb.crossoverLow = 200.0f;
            p.reverb.crossoverHigh = 3800.0f;
            p.reverb.diffusion = 0.55f;
            p.reverb.size = 1.0f;
            p.reverb.toneHz = 9000.0f;
            p.reverb.mix = fx::sweep (tick, 0.09, 2.1, 30.0f, 70.0f);

            p.eq[0].bypass = 0;
            p.eq[0].shape[0] = 1; p.eq[0].freqHz[0] = 90.0f; p.eq[0].slope[0] = 0.7f;
            p.eq[0].shape[1] = 0;
            p.eq[0].shape[2] = 3;
            p.eq[0].freqHz[2] = fx::sweep (tick, 0.13, 1.7, 800.0f, 2600.0f);
            p.eq[0].gainDb[2] = 3.0f; p.eq[0].q[2] = 0.9f;
            p.eq[0].shape[3] = 0;
            p.eq[0].shape[4] = 0;
            p.eq[0].shape[5] = 6; p.eq[0].freqHz[5] = 13000.0f; p.eq[0].slope[5] = 0.7f;

            // The CHAIN's mute, which is not the engine's: the modules keep
            // running, so this tail has moved on by the time it comes back
            // rather than resuming where it stopped. The per-channel Clear at
            // tick 58 lands inside this window and takes the tail away
            // outright, which is the difference between the two.
            p.mute = (tick >= engine::kChanMuteOn && tick < engine::kChanMuteOff) ? 1 : 0;
            break;
        }

        default:
        {
            // Channel 3, and anything above it were the count ever raised.
            p.delay.bypass = 0;
            p.delay.taps = 4;
            p.delay.tapMode = 1;                    // pattern
            p.delay.pattern = 1;
            p.delay.timeMs = fx::sweep (tick, 0.07, 0.4, 140.0f, 260.0f);
            p.delay.feedback = 35.0f;
            p.delay.mix = 45.0f;
            p.delay.inLoCutHz = 90.0f;
            p.delay.fbHiShelfHz = 3500.0f; p.delay.fbHiShelfDb = -6.0f;
            p.delay.glideMs = 120.0f;
            p.delay.diffusion = 0.25f;

            p.phaser.bypass = 0;
            p.phaser.stages = 6;
            p.phaser.centreHz = 700.0f;
            p.phaser.spreadOct = 1.0f;
            p.phaser.rateHz = 0.35f;
            p.phaser.depthOct = 2.0f;
            p.phaser.feedback = 30.0f;
            p.phaser.mix = 40.0f;
            break;
        }
    }

    return p;
}


/** The chain scenario's order strings are data, and a typo in one of them
    would silently leave the default order running (parseChainOrder refuses a
    whole string rather than half-applying it) — which would quietly turn the
    reorder gate into no gate at all. Checked once before the render.

    Returns an EMPTY string when the data is sound, otherwise the reason —
    naming the offending string and what is actually wrong with it. The three
    failures are different bugs with the same symptom (an inert reorder gate),
    so a caller that prints only "does not parse" sends whoever reads it
    hunting for a typo that may not exist. */
inline std::string effectsSelfTestFailure()
{
    using namespace spatcore::effects;

    const char* const names[4]   = { "kOrderDefault", "kOrderB", "kOrderC", "kOrderD" };
    const char* const strings[4] = { fx::kOrderDefault, fx::kOrderB,
                                     fx::kOrderC, fx::kOrderD };
    ChainOrder o[4];

    for (int i = 0; i < 4; ++i)
    {
        o[i] = kDefaultOrder;

        if (! parseChainOrder (strings[i], o[i]))
            return std::string ("fx::") + names[i] + " does not parse — \""
                   + strings[i] + "\"";

        if (! isValidChainOrder (o[i]))
            return std::string ("fx::") + names[i]
                   + " parses but is not a valid permutation of the slots — \""
                   + strings[i] + "\"";
    }

    if (o[0] != kDefaultOrder)
        return "fx::kOrderDefault is not the shipped default order, so the "
               "chain scenario does not start where it claims to";

    for (int i = 1; i < 4; ++i)
    {
        if (o[i] == kDefaultOrder)
            return std::string ("fx::") + names[i]
                   + " is equal to the default order, so that reorder reorders nothing";

        for (int j = i + 1; j < 4; ++j)
            if (o[i] == o[j])
                return std::string ("fx::") + names[i] + " and fx::" + names[j]
                       + " are the same order, so one of the two reorders is a no-op";
    }

    return {};
}

} // namespace scenario
