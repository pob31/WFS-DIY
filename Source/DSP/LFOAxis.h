#pragma once

#include <JuceHeader.h>
#include "../../spatcore/dsp/LFOWaveforms.h"
#include <cmath>

/**
    One LFO axis - X, Y or Z of an input, effect return or cluster LFO, or a
    cluster's rotation or scale: its own ramp, its fade, its Random targets,
    and what keeps its output continuous.

    Nothing the operator does makes the output jump (re-audit F6). Switching
    an axis to Off dropped it to 0 in one tick: Off returns 0, and the
    one-second fade multiplied that. Off now keeps playing the shape it fades
    out. Every other step - one shape for another, an amplitude or a phase
    edit - goes through a bridge: the difference between what the previous
    settings give at this instant and what the new ones give is added to the
    output and released linearly over a second. The offsets land after the
    speed limiter, so nothing downstream smoothed them: a 3 m step was an
    8.7 ms delay step in one 20 ms tick.

    Message thread, 50 Hz. Fixed size, no allocation.
*/
struct LFOAxis
{
    static constexpr float fadeSeconds   = 1.0f;   // a shape coming in, or going Off
    static constexpr float bridgeSeconds = 1.0f;   // a step released

    float ramp = 0.0f;          // own ramp 0..1, advancing at the axis rate
    float fade = 0.0f;          // 0..1
    float normalized = 0.0f;    // -1..+1, the waveform alone (UI)

    /** Restart from silence: the LFO switched on after a full fade-out. */
    void restart (int shape) noexcept
    {
        ramp = 0.0f;
        if (shape != LFOWaveforms::Off && fade >= 1.0f)
            fade = 0.0f;
    }

    /** In over a second while a shape is chosen, out over a second at Off,
        from wherever the fade is: Off and back within the second carries on
        from there (it restarted from 0). */
    void updateFade (int shape, float deltaTime) noexcept
    {
        if (shape != LFOWaveforms::Off)
        {
            liveShape = shape;
            fade = juce::jmin (1.0f, fade + deltaTime / fadeSeconds);
        }
        else
        {
            fade = juce::jmax (0.0f, fade - deltaTime / fadeSeconds);
        }
    }

    void advance (float rampIncrement) noexcept
    {
        ramp += rampIncrement;
        if (ramp >= 1.0f)
            ramp = std::fmod (ramp, 1.0f);
    }

    /** This tick's output: the waveform at `phaseCycles`, times `amplitude`,
        `gain` (the LFO's own fade) and the axis fade, plus the bridge.
        Unipolar maps the waveform to 0..1 (a cluster's rotation and scale). */
    float evaluate (int shape, float amplitude, float phaseCycles, float gain,
                    float deltaTime, juce::Random& random, bool unipolar = false) noexcept
    {
        const int shapeNow = shape != LFOWaveforms::Off ? shape : liveShape;
        const float phased = wrap (ramp + phaseCycles);

        if (shapeNow == LFOWaveforms::Random && phased < prevPhasedRamp)
        {
            lastRandom = randomTarget;
            randomTarget = random.nextFloat() * 2.0f - 1.0f;
        }
        prevPhasedRamp = phased;

        normalized = LFOWaveforms::applyWaveform (shapeNow, phased, lastRandom, randomTarget);
        const float raw = map (normalized, unipolar) * amplitude * gain * fade;

        if (shapeNow != lastShape || amplitude != lastAmplitude || phaseCycles != lastPhase)
        {
            // What the previous settings give now, at this tick's ramp: the
            // waveform's own edges (a square, a saw's wrap) are not absorbed,
            // and a slider being dragged keeps moving.
            const float was = map (LFOWaveforms::applyWaveform (lastShape, wrap (ramp + lastPhase),
                                                                lastRandom, randomTarget), unipolar)
                              * lastAmplitude * gain * fade;
            bridge += was - raw;
            bridgeRate = std::abs (bridge) / bridgeSeconds;
        }
        else
        {
            const float step = bridgeRate * deltaTime;
            bridge = std::abs (bridge) <= step ? 0.0f : bridge - std::copysign (step, bridge);
        }

        remember (shapeNow, amplitude, phaseCycles);
        return raw + bridge;
    }

    /** Fully faded out: the output is 0, and what is set now becomes the
        reference, so an edit made while silent is not a step later. */
    void rest (int shape, float amplitude, float phaseCycles) noexcept
    {
        normalized = 0.0f;
        bridge = 0.0f;
        remember (shape != LFOWaveforms::Off ? shape : liveShape, amplitude, phaseCycles);
    }

    bool bridging() const noexcept { return bridge != 0.0f; }

private:
    static float wrap (float cycles) noexcept           { return cycles - std::floor (cycles); }
    static float map (float n, bool unipolar) noexcept  { return unipolar ? (n + 1.0f) * 0.5f : n; }

    void remember (int shapeNow, float amplitude, float phaseCycles) noexcept
    {
        lastShape = shapeNow;
        lastAmplitude = amplitude;
        lastPhase = phaseCycles;
    }

    int liveShape = LFOWaveforms::Off;    // the last shape that was not Off: what Off fades out
    int lastShape = LFOWaveforms::Off;    // the previous tick's shape, amplitude and phase
    float lastAmplitude = 0.0f;
    float lastPhase = 0.0f;
    float bridge = 0.0f;
    float bridgeRate = 0.0f;              // output units per second
    float prevPhasedRamp = 0.0f;          // Random's wrap detection
    float lastRandom = 0.0f;
    float randomTarget = 0.0f;
};
