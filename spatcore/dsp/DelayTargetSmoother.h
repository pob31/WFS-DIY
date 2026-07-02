#pragma once

#include <cstdint>
#include <cmath>
#include <algorithm>

/**
 * DelayTargetSmoother
 *
 * Smooths a stream of delay-target observations (in samples) into a C1-continuous
 * output suitable for driving a per-sample delay-line read. Designed for the
 * WFS routing matrix where each (input, output) pair receives ~50 Hz updates
 * from the 50 Hz matrix-recalculation timer.
 *
 * Model
 *   - Observations V_k arrive at sample timestamps t_k.
 *   - Between observations, we assume a linear ramp from V_{k-1} to V_k
 *     over duration dt_k = t_k - t_{k-1}, starting at t_{k-1}.
 *   - At each junction t_k the slope may jump. A causal box filter of width W
 *     rounds off those slope jumps, giving C1 continuity.
 *
 * Closed-form smoothing inside the W-wide transition zone following junction t_j:
 *   Let mu = tau - t_j, lam = W - mu.
 *   y(tau) = V_j + (s_curr * mu^2 - s_prev * lam^2) / (2W)
 * Outside the transition zone (mu >= W), evaluation is just the delayed linear:
 *   y(tau) = rampValueAt(tau - W/2)
 * See the plan document for the derivation and C1 verification.
 *
 * "Current slope not yet known" at the latest junction: we extrapolate
 * currentSlope = prevSlope until the next observation. The resulting correction
 * artifact at the next observation is small when slopes vary smoothly.
 *
 * Teleport (large jumps): when |V_new - last| exceeds a threshold, we run a
 * mute-move-unmute envelope over W samples:
 *   - [t_T, t_T + W/2]: gain 1 -> 0, pre-snap state still produces the delay output.
 *   - At t_T + W/2: delay state snaps to V_new (committed by the next observe()).
 *   - [t_T + W/2, t_T + W]: gain 0 -> 1, delay output = V_new (snapped).
 *
 * State is per (input, output) routing cell. The owning processor holds an
 * array of these and calls:
 *   prepare(windowSamples) once on init / sample-rate change.
 *   observe(rawDelaySamples, blockStartSample) once at the top of each block.
 *   smoothedAt(sampleIndex) per sample inside the hot loop.
 */
class DelayTargetSmoother
{
public:
    struct Output
    {
        float delay;   // smoothed delay in samples
        float gain;    // 1.0 outside a teleport; V-shaped envelope during one
    };

    /** Set the smoothing window (in samples) and reset state.
     *  teleportThreshold defaults to 3 x W (e.g. ~30 ms at 10 ms W) and can
     *  be tuned by callers before calling prepare() if needed. */
    void prepare (int windowSamples)
    {
        windowSamples_   = std::max (2, windowSamples);
        halfWindow_      = windowSamples_ / 2;
        invWindow_       = 1.0f / static_cast<float> (windowSamples_);
        invTwoWindow_    = 0.5f / static_cast<float> (windowSamples_);
        teleportThreshold_ = 3.0f * static_cast<float> (windowSamples_);
        reset();
    }

    /** Forget all history. Next observe() becomes the new bootstrap point. */
    void reset()
    {
        initialized_         = false;
        teleportStart_       = -1;
        rampStartValue_      = 0.0f;
        rampEndValue_        = 0.0f;
        rampStartSample_     = 0;
        rampDurationSamples_ = windowSamples_ * 2;  // placeholder; refined at first observe
        prevSlope_           = 0.0f;
        currentSlope_        = 0.0f;
        teleportSnapValue_   = 0.0f;
    }

    /** Optionally override the teleport threshold (in samples of delay change
     *  per observation). Default is 3 x W. */
    void setTeleportThreshold (float samples) { teleportThreshold_ = samples; }

    /** Record one raw target observation. Called once per audio block. */
    void observe (float rawDelaySamples, std::int64_t sampleIndex)
    {
        if (! initialized_)
        {
            rampStartValue_      = rawDelaySamples;
            rampEndValue_        = rawDelaySamples;
            rampStartSample_     = sampleIndex;
            rampDurationSamples_ = windowSamples_ * 2;  // will be refined at next change
            prevSlope_           = 0.0f;
            currentSlope_        = 0.0f;
            teleportStart_       = -1;
            initialized_         = true;
            return;
        }

        // Finish any in-flight teleport envelope before processing new observation.
        if (teleportStart_ >= 0)
        {
            const std::int64_t elapsed = sampleIndex - teleportStart_;
            if (elapsed >= static_cast<std::int64_t> (windowSamples_))
            {
                // Envelope complete. Commit the snap: new segment at the envelope
                // midpoint, zero slopes on both sides, constant value.
                rampStartValue_      = teleportSnapValue_;
                rampEndValue_        = teleportSnapValue_;
                rampStartSample_     = teleportStart_ + halfWindow_;
                rampDurationSamples_ = windowSamples_ * 2;
                prevSlope_           = 0.0f;
                currentSlope_        = 0.0f;
                teleportStart_       = -1;
                // fall through to normal observation processing below
            }
            else
            {
                // Still in envelope. If a *second* large jump arrives, restart.
                if (std::fabs (rawDelaySamples - teleportSnapValue_) > teleportThreshold_)
                {
                    teleportStart_     = sampleIndex;
                    teleportSnapValue_ = rawDelaySamples;
                }
                // Don't disturb ramp/slope state during an active envelope.
                return;
            }
        }

        // No change since last observation -> nothing to do. This is the common
        // case: matrix updates ~50 Hz but observe() is called per audio block
        // (~200 Hz), so most calls see the same value as last time.
        if (rawDelaySamples == rampEndValue_)
            return;

        // Teleport threshold check.
        if (std::fabs (rawDelaySamples - rampEndValue_) > teleportThreshold_)
        {
            teleportStart_     = sampleIndex;
            teleportSnapValue_ = rawDelaySamples;
            return;
        }

        // Normal observation: shift state, start a new ramp from our current
        // position (wherever we are on the in-flight ramp) toward the new target.
        const float currentPosition = rampValueAt (sampleIndex);
        const std::int64_t elapsedSinceLast = sampleIndex - rampStartSample_;

        // Adaptive duration: the next ramp takes about as long as the just-ended one.
        // Clamp to a sane minimum so degenerate timing never divides by zero.
        const std::int64_t newDuration = std::max<std::int64_t> (elapsedSinceLast, windowSamples_);

        prevSlope_           = currentSlope_;
        rampStartValue_      = currentPosition;
        rampEndValue_        = rawDelaySamples;
        rampStartSample_     = sampleIndex;
        rampDurationSamples_ = newDuration;
        currentSlope_        = (rawDelaySamples - currentPosition) / static_cast<float> (newDuration);
    }

    /** Evaluate the smoothed delay at an arbitrary sample index. Cheap enough
     *  to call per sample. Does not modify state. */
    Output smoothedAt (std::int64_t sampleIndex) const
    {
        if (! initialized_)
            return { rampEndValue_, 1.0f };

        // Teleport envelope handling.
        if (teleportStart_ >= 0)
        {
            const std::int64_t elapsed = sampleIndex - teleportStart_;
            if (elapsed >= 0 && elapsed < static_cast<std::int64_t> (windowSamples_))
            {
                if (elapsed < static_cast<std::int64_t> (halfWindow_))
                {
                    // Fade-out phase: gain 1 -> 0, old delay state still evaluated
                    const float g = 1.0f - static_cast<float> (elapsed) / static_cast<float> (halfWindow_);
                    return { evaluateSmoothed (sampleIndex), g };
                }
                else
                {
                    // Fade-in phase: gain 0 -> 1, delay snapped to teleport target
                    const float g = static_cast<float> (elapsed - halfWindow_) / static_cast<float> (halfWindow_);
                    return { teleportSnapValue_, g };
                }
            }
            // elapsed >= windowSamples_: envelope finished; state commit is done
            // by observe(), but smoothedAt may be called between blocks with the
            // old teleportStart_ still set. Output the snapped value cleanly.
            return { teleportSnapValue_, 1.0f };
        }

        return { evaluateSmoothed (sampleIndex), 1.0f };
    }

    bool isInitialized() const noexcept { return initialized_; }

private:
    /** Value of the current linear ramp at time tau (clamped to endpoints). */
    float rampValueAt (std::int64_t tau) const
    {
        const std::int64_t elapsed = tau - rampStartSample_;
        if (elapsed <= 0)                              return rampStartValue_;
        if (elapsed >= rampDurationSamples_)           return rampEndValue_;
        const float t = static_cast<float> (elapsed) / static_cast<float> (rampDurationSamples_);
        return rampStartValue_ + (rampEndValue_ - rampStartValue_) * t;
    }

    /** Smoothed output at time tau, ignoring teleport envelope. */
    float evaluateSmoothed (std::int64_t tau) const
    {
        const std::int64_t mu64 = tau - rampStartSample_;
        if (mu64 < 0)
        {
            // Before the current junction (shouldn't happen in normal use since
            // observe() is called at the top of each block).
            return rampStartValue_;
        }

        if (mu64 >= static_cast<std::int64_t> (windowSamples_))
        {
            // Outside the transition zone: linear output delayed by W/2.
            return rampValueAt (tau - halfWindow_);
        }

        // Inside transition zone [rampStartSample_, rampStartSample_ + W]:
        // y = V_j + (s_curr * mu^2 - s_prev * lam^2) / (2W)
        const float mu  = static_cast<float> (mu64);
        const float lam = static_cast<float> (windowSamples_) - mu;
        return rampStartValue_
             + (currentSlope_ * mu * mu - prevSlope_ * lam * lam) * invTwoWindow_;
    }

    // Configuration
    int   windowSamples_     = 480;        // W — set by prepare()
    int   halfWindow_        = 240;        // W/2
    float invWindow_         = 1.0f / 480.0f;
    float invTwoWindow_      = 0.5f / 480.0f;
    float teleportThreshold_ = 3.0f * 480.0f;

    // State
    bool           initialized_         = false;
    float          rampStartValue_      = 0.0f;
    float          rampEndValue_        = 0.0f;
    std::int64_t   rampStartSample_     = 0;
    std::int64_t   rampDurationSamples_ = 960;
    float          prevSlope_           = 0.0f;
    float          currentSlope_        = 0.0f;

    // Teleport envelope state
    std::int64_t   teleportStart_       = -1;    // -1 when not in a teleport
    float          teleportSnapValue_   = 0.0f;
};
