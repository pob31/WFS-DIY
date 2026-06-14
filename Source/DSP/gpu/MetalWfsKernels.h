#pragma once

// MSL kernel sources embedded as string literals (compiled at prepare time).
// Mirror of the CUDA kernels in CudaWfsKernels.h - keep the two in sync.
// (The CUDA string is the validated reference: Experiments/cuda-wfs-test
// checks it against a CPU model on real hardware.)
//
// Two kernels per launch (K1 then K2, ordered by the serial compute encoder):
//
//   K1 wfs_pairs - grid = pairGroups + 2*numInputs threadgroups:
//     - groups <  pairGroups : PAIR role. ONE THREAD per (in,out) pair,
//       serial over the block's samples (the per-pair HF shelf biquads are
//       IIRs - y[n] depends on y[n-1] - so samples cannot be parallelized
//       within a pair). Thread id is out-major (out = t / numIn,
//       in = t % numIn) so consecutive threads write consecutive scratch
//       addresses. Per sample: prev->curr ramp of direct delay+gain and FR
//       delay+gain, fractional-delay fetch (staging for off >= 0, persistent
//       ring for off < 0), DF1 shelf biquad per tap (coefficients computed
//       in-kernel once per launch from the raw dB matrices - stepwise like
//       the CPU's setGainDb), accumulate direct + FR into scratch.
//       Per-tap gate on (gainPrev != 0 || gainCurr != 0): a closed gate does
//       not fetch, evaluate, or advance filter state (CPU parity). When a
//       gate is open the shelf ALWAYS runs, even at 0 dB.
//     - groups >= pairGroups : APPEND roles, sample-parallel. First numIn
//       groups append the raw input to the direct ring; next numIn groups
//       append the host-pre-filtered FR input (frIn staging) to the FR ring.
//       No hazard vs the pair role: appends write ring [pos, pos+len) while
//       pair reads only touch history < pos (ringValidSamples guard).
//
//   K2 wfs_reduce - grid = numOutputs threadgroups, threads stride samples:
//     output[out][s] = sum over in of scratch, ascending in - a fixed,
//     deterministic summation order (no atomics).
//
// Scratch layout: scratch[(s * numOutputs + out) * numInputs + in].

static constexpr const char* kWfsDelaySumKernelSource = R"MSL(
#include <metal_stdlib>
using namespace metal;

struct WfsParams
{
    uint  numInputs;
    uint  numOutputs;
    uint  bufferLength;     // samples per channel this launch
    uint  ringCapacity;     // per-input ring length (samples)
    uint  ringWritePos;     // append position for this launch (both rings)
    uint  ringValidSamples; // valid history behind the write position
    uint  pairGroups;       // threadgroups reserved for the pair role
    float shelfCosW0;       // cos(2*pi*800/sr), precomputed at prepare
    float shelfSinW0;       // sin(2*pi*800/sr)
};

static inline float fetchSample(const device float* inBase,
                                const device float* ring,
                                uint inIdx,
                                uint bufferLength,
                                uint ringCapacity,
                                uint pos,
                                uint validSamples,
                                int off)
{
    if (off >= 0)
        return inBase[inIdx * bufferLength + uint(off)];

    if (uint(-off) > validSamples)
        return 0.0f;

    int idx = int(pos) + off;
    if (idx < 0)
        idx += int(ringCapacity);

    return ring[inIdx * ringCapacity + uint(idx)];
}

// High-shelf biquad coefficients at fixed 800 Hz / Q = 0.3, gain in dB.
// Verbatim port of WFSHighShelfFilter::recalculateCoefficients (Audio EQ
// Cookbook, Q used as the shelf slope parameter S).
static inline void shelfCoeffs(float gainDb, float cosw0, float sinw0,
                               thread float& b0, thread float& b1, thread float& b2,
                               thread float& a1, thread float& a2)
{
    const float A     = pow(10.0f, gainDb / 40.0f);
    const float rootA = sqrt(A);
    const float alpha = (sinw0 * 0.5f) * sqrt((A + 1.0f / A) * (1.0f / 0.3f - 1.0f) + 2.0f);

    const float a0inv = 1.0f / ((A + 1.0f) - (A - 1.0f) * cosw0 + 2.0f * rootA * alpha);

    b0 =  A * ((A + 1.0f) + (A - 1.0f) * cosw0 + 2.0f * rootA * alpha) * a0inv;
    b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosw0) * a0inv;
    b2 =  A * ((A + 1.0f) + (A - 1.0f) * cosw0 - 2.0f * rootA * alpha) * a0inv;
    a1 =  2.0f * ((A - 1.0f) - (A + 1.0f) * cosw0) * a0inv;
    a2 =  ((A + 1.0f) - (A - 1.0f) * cosw0 - 2.0f * rootA * alpha) * a0inv;
}

kernel void wfs_pairs(constant WfsParams&  params       [[buffer(0)]],
                      const device float*  input        [[buffer(1)]],  // [numInputs][bufferLength] raw
                      const device float*  frIn         [[buffer(2)]],  // [numInputs][bufferLength] host-FR-filtered
                      device float*        ring         [[buffer(3)]],  // [numInputs][ringCapacity] direct history
                      device float*        frRing       [[buffer(4)]],  // [numInputs][ringCapacity] FR history
                      const device float*  delaysPrev   [[buffer(5)]],  // [in][out] samples
                      const device float*  delaysCurr   [[buffer(6)]],
                      const device float*  gainsPrev    [[buffer(7)]],  // [in][out] linear
                      const device float*  gainsCurr    [[buffer(8)]],
                      const device float*  frDelaysPrev [[buffer(9)]],  // [in][out] samples (absolute)
                      const device float*  frDelaysCurr [[buffer(10)]],
                      const device float*  frGainsPrev  [[buffer(11)]], // [in][out] linear (absolute)
                      const device float*  frGainsCurr  [[buffer(12)]],
                      const device float*  hfAttenDb    [[buffer(13)]], // [in][out] dB, stepwise
                      const device float*  frHfAttenDb  [[buffer(14)]], // [in][out] dB, stepwise
                      device float*        shelfState   [[buffer(15)]], // [pairs][4] persistent
                      device float*        frShelfState [[buffer(16)]], // [pairs][4] persistent
                      device float*        scratch      [[buffer(17)]], // [(s*numOut+out)*numIn+in]
                      uint                 groupId      [[threadgroup_position_in_grid]],
                      uint                 threadId     [[thread_index_in_threadgroup]],
                      uint                 threadsPerTg [[threads_per_threadgroup]])
{
    const uint numIn   = params.numInputs;
    const uint numOut  = params.numOutputs;
    const uint len     = params.bufferLength;
    const uint ringCap = params.ringCapacity;
    const uint pos     = params.ringWritePos;
    const uint valid   = params.ringValidSamples;

    if (len == 0 || ringCap == 0)
        return;

    if (groupId >= params.pairGroups)
    {
        // ===== Append roles: copy this launch's staging into the rings =====
        const uint a = groupId - params.pairGroups;
        const device float* src;
        device float* dstRing;
        uint chIdx;
        if (a < numIn)
        {
            chIdx = a;
            src = input;
            dstRing = ring;
        }
        else
        {
            chIdx = a - numIn;
            if (chIdx >= numIn)
                return;
            src = frIn;
            dstRing = frRing;
        }

        const device float* chIn = src + chIdx * len;
        const uint ringBase = chIdx * ringCap;
        for (uint s = threadId; s < len; s += threadsPerTg)
        {
            uint w = pos + s;
            if (w >= ringCap)
                w -= ringCap;
            dstRing[ringBase + w] = chIn[s];
        }
        return;
    }

    // ===== Pair role: one thread per (in,out) pair, serial over samples =====
    const uint t = groupId * threadsPerTg + threadId;
    if (t >= numIn * numOut)
        return;

    const uint out = t / numIn;   // out-major: consecutive threads share out,
    const uint in  = t % numIn;   // stride in -> coalesced scratch stores
    const uint m   = in * numOut + out;

    const float dp  = delaysPrev[m];
    const float dc  = delaysCurr[m];
    const float gp  = gainsPrev[m];
    const float gc  = gainsCurr[m];
    const float fdp = frDelaysPrev[m];
    const float fdc = frDelaysCurr[m];
    const float fgp = frGainsPrev[m];
    const float fgc = frGainsCurr[m];

    const bool doDir = (gp != 0.0f || gc != 0.0f);
    const bool doFr  = (fgp != 0.0f || fgc != 0.0f);

    if (!doDir && !doFr)
    {
        // Silent pair: scratch must still be complete for the reduce.
        // No filter state touched (CPU parity: closed gates freeze state).
        for (uint s = 0; s < len; ++s)
            scratch[(s * numOut + out) * numIn + in] = 0.0f;
        return;
    }

    // Shelf coefficients: computed once per launch per open tap from the raw
    // dB matrices (stepwise per launch, like the CPU's per-block setGainDb).
    float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    float x1 = 0.0f, x2 = 0.0f, y1 = 0.0f, y2 = 0.0f;
    if (doDir)
    {
        shelfCoeffs(hfAttenDb[m], params.shelfCosW0, params.shelfSinW0, b0, b1, b2, a1, a2);
        x1 = shelfState[m * 4 + 0]; x2 = shelfState[m * 4 + 1];
        y1 = shelfState[m * 4 + 2]; y2 = shelfState[m * 4 + 3];
    }
    float fb0 = 0.0f, fb1 = 0.0f, fb2 = 0.0f, fa1 = 0.0f, fa2 = 0.0f;
    float fx1 = 0.0f, fx2 = 0.0f, fy1 = 0.0f, fy2 = 0.0f;
    if (doFr)
    {
        shelfCoeffs(frHfAttenDb[m], params.shelfCosW0, params.shelfSinW0, fb0, fb1, fb2, fa1, fa2);
        fx1 = frShelfState[m * 4 + 0]; fx2 = frShelfState[m * 4 + 1];
        fy1 = frShelfState[m * 4 + 2]; fy2 = frShelfState[m * 4 + 3];
    }

    const float invLen = 1.0f / float(len);

    for (uint s = 0; s < len; ++s)
    {
        // Ramp reaches exactly `curr` on the last sample so the next launch
        // (whose prev == this curr) continues smoothly.
        const float tt = float(s + 1) * invLen;
        float acc = 0.0f;

        if (doDir)
        {
            const float gain = gp + (gc - gp) * tt;

            float d = dp + (dc - dp) * tt;
            d = max(d, 0.0f);
            const uint  delayInt = uint(d);
            const float frac     = d - float(delayInt);
            const int   off0     = int(s) - int(delayInt);

            const float s0 = fetchSample(input, ring, in, len, ringCap, pos, valid, off0);
            const float s1 = fetchSample(input, ring, in, len, ringCap, pos, valid, off0 - 1);
            const float v  = s0 * (1.0f - frac) + s1 * frac;

            // DF1 shelf - always evaluated while the gate is open (even 0 dB)
            const float w = b0 * v + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = v; y2 = y1; y1 = w;

            acc += w * gain;
        }

        if (doFr)
        {
            const float gain = fgp + (fgc - fgp) * tt;

            float d = fdp + (fdc - fdp) * tt;
            d = max(d, 0.0f);
            const uint  delayInt = uint(d);
            const float frac     = d - float(delayInt);
            const int   off0     = int(s) - int(delayInt);

            const float s0 = fetchSample(frIn, frRing, in, len, ringCap, pos, valid, off0);
            const float s1 = fetchSample(frIn, frRing, in, len, ringCap, pos, valid, off0 - 1);
            const float v  = s0 * (1.0f - frac) + s1 * frac;

            const float w = fb0 * v + fb1 * fx1 + fb2 * fx2 - fa1 * fy1 - fa2 * fy2;
            fx2 = fx1; fx1 = v; fy2 = fy1; fy1 = w;

            acc += w * gain;
        }

        scratch[(s * numOut + out) * numIn + in] = acc;
    }

    if (doDir)
    {
        shelfState[m * 4 + 0] = x1; shelfState[m * 4 + 1] = x2;
        shelfState[m * 4 + 2] = y1; shelfState[m * 4 + 3] = y2;
    }
    if (doFr)
    {
        frShelfState[m * 4 + 0] = fx1; frShelfState[m * 4 + 1] = fx2;
        frShelfState[m * 4 + 2] = fy1; frShelfState[m * 4 + 3] = fy2;
    }
}

kernel void wfs_reduce(constant WfsParams&  params       [[buffer(0)]],
                       const device float*  scratch      [[buffer(1)]], // [(s*numOut+out)*numIn+in]
                       device float*        output       [[buffer(2)]], // [numOutputs][bufferLength]
                       uint                 groupId      [[threadgroup_position_in_grid]],
                       uint                 threadId     [[thread_index_in_threadgroup]],
                       uint                 threadsPerTg [[threads_per_threadgroup]])
{
    const uint numIn  = params.numInputs;
    const uint numOut = params.numOutputs;
    const uint len    = params.bufferLength;

    const uint out = groupId;
    if (out >= numOut || len == 0)
        return;

    for (uint s = threadId; s < len; s += threadsPerTg)
    {
        const uint base = (s * numOut + out) * numIn;
        float acc = 0.0f;
        for (uint in = 0; in < numIn; ++in)   // fixed ascending order: deterministic
            acc += scratch[base + in];
        output[out * len + s] = acc;
    }
}
)MSL";
