#pragma once

#include <cmath>
#include <cstdint>
#include <algorithm>

/*
    FrDiffusionModel — the shared Floor-Reflection diffusion ("grain") model.

    Models floor roughness scattering in a simplified way: the FR tap's delay
    wanders as one-pole low-passed white noise, applied AFTER the per-tap delay
    smoothing so the anti-zipper filter can't average the grain away. The
    Diffusion dial (0..1) spans three perceptual zones:

        low  -> "clinical": excursion ~0 (near-specular, polished floor)
        mid  -> "dirty but discrete": fractions of a ms at a few Hz
        high -> "wild": excursion up to kMaxMsAt100 at a slower rate

    excursion = kMaxMsAt100 x d^kCurvePow      (power curve: stays tiny through
                                                the low zone, grows toward the top)
    fc        = kFcMaxHz / (1 + kFcDrop x d)   (rate drops as excursion grows,
                                                bounding delay slew = pitch wobble,
                                                so the top of the dial gets wild
                                                without harsh flange distortion)

    Every (input x output) routing uses an INDEPENDENT noise stream (hash-keyed,
    not seed offsets of one sequence): each source scatters off its own patch of
    floor toward each speaker, so no pattern can march across the array or move
    in lockstep between sources.

    Used identically by the CPU InputBuffer processor (per-sample update), the
    CPU OutputBuffer processor (per-sample update), and the GPU backends'
    host-side staging (per-launch update; the kernels' prev->curr per-sample
    interpolation reconstructs the same low-rate waveform). One header so the
    three algorithms can never drift apart in character.
*/
namespace spatcore::dsp {

namespace FrDiffusion
{
    // ==== Zone-map tunables (audition-tuned 2026-06-12; see header comment) ====
    constexpr float kMaxMsAt100 = 2.0f;   // max excursion at 100% Diffusion
    constexpr float kCurvePow   = 1.8f;   // dial curve: excursion ~ d^pow
    constexpr float kFcMaxHz    = 6.0f;   // grain frequency at low Diffusion
    constexpr float kFcDrop     = 2.0f;   // -> 2 Hz at 100% Diffusion
    constexpr float kCrest      = 5.0f;   // crest headroom before the excursion clamp
    constexpr float kEngageEps  = 1.0e-4f; // -80 dB: below this an FR tap counts as silent

    /** Bipolar white noise in [-1, 1) for step index n on the stream identified
        by key. Squirrel-style integer hash: consecutive n gives white-noise
        quality; distinct keys give fully independent streams (unlike xorshift,
        where every seed is just a point on one shared orbit). */
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

    /** Stream key for an (input, output) routing pair. */
    inline uint32_t makeKey (int inputIndex, int outputIndex) noexcept
    {
        return static_cast<uint32_t> (inputIndex) * 0x9E3779B9u
             + static_cast<uint32_t> (outputIndex + 1) * 0x85EBCA6Bu + 1u;
    }

    struct Coeffs
    {
        float ampSamples = 0.0f;  // excursion bound (audio samples); 0 = grain off
        float alpha      = 0.0f;  // one-pole coefficient per update step
        float scale      = 0.0f;  // LP-gain-normalised amplitude scale
    };

    /** Derive the per-update coefficients from the Diffusion fraction d (0..1).

        @param sampleRate            audio sample rate (Hz) — sets the excursion in samples
        @param updateIntervalSamples noise update step in audio samples:
                                     1 for per-sample CPU loops,
                                     blockSize for per-launch GPU staging.
        The amplitude normalisation (sqrt(alpha/(2-alpha)) = the one-pole's
        white-noise RMS gain at that update rate) keeps the excursion set by d
        alone, identical across update rates — so CPU and GPU grain match. */
    inline Coeffs computeCoeffs (float d, float sampleRate, float updateIntervalSamples) noexcept
    {
        Coeffs c;
        d = std::min (1.0f, std::max (0.0f, d));
        const float excursionMs = kMaxMsAt100 * std::pow (d, kCurvePow);
        c.ampSamples = (excursionMs / 1000.0f) * sampleRate;

        if (c.ampSamples <= 0.0f || sampleRate <= 0.0f || updateIntervalSamples <= 0.0f)
            return Coeffs {};

        const float fc = kFcMaxHz / (1.0f + kFcDrop * d);
        c.alpha = 1.0f - std::exp (-6.283185307f * fc * updateIntervalSamples / sampleRate);

        const float rmsGain = std::sqrt (c.alpha / (2.0f - c.alpha));
        c.scale = (rmsGain > 1.0e-6f) ? (c.ampSamples / (kCrest * rmsGain)) : 0.0f;
        return c;
    }

    /** One noise-update step: advances the one-pole state and returns the
        clamped jitter in audio samples. */
    inline float step (float& state, uint32_t n, uint32_t key, const Coeffs& c) noexcept
    {
        state += c.alpha * (hashNoiseBipolar (n, key) - state);
        const float j = state * c.scale;
        return std::min (c.ampSamples, std::max (-c.ampSamples, j));
    }
} // namespace FrDiffusion

} // namespace spatcore::dsp

// Extraction-compat alias — app code migrates to qualified names later.
namespace FrDiffusion = spatcore::dsp::FrDiffusion;
