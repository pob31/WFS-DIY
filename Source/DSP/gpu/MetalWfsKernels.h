#pragma once

// MSL kernel sources embedded as string literals (compiled at prepare time).
// Canonical source: Experiments/metal-wfs-spike/wfs_delay_sum.metal
// (validated against the CPU reference there - keep the two in sync).

static constexpr const char* kWfsDelaySumKernelSource = R"MSL(
//
// WFS delay-and-sum routing kernel - native Metal Shading Language port.
//
// Direct port of the wfs_input_buffer GPU Audio kernel's logic
// (Processors/wfs_input_buffer_processor/src/cuda/WfsInputProcessor.cuh),
// written against raw Metal instead of the GPU Audio SDK abstraction.
// Same architecture:
//   - threadgroup id <  numOutputs : gather block for that output channel
//   - threadgroup id >= numOutputs : ring-append block for input (id - numOutputs)
//   - thread index strides over samples
//   - persistent per-input ring buffers; positions host-tracked; reads only
//     ever touch previous launches' samples (current launch served from the
//     launch input directly) -> no atomics, no cross-group sync needed.
//   - per-sample prev->curr interpolation of delays and gains (zipper-free).
//

#include <metal_stdlib>
using namespace metal;

struct WfsParams
{
    uint numInputs;
    uint numOutputs;
    uint bufferLength;     // samples per channel this launch
    uint ringCapacity;     // per-input ring length (samples)
    uint ringWritePos;     // append position for this launch
    uint ringValidSamples; // valid history behind the write position
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

kernel void wfs_delay_sum(constant WfsParams&  params       [[buffer(0)]],
                          const device float*  input        [[buffer(1)]], // [numInputs][bufferLength]
                          device float*        output       [[buffer(2)]], // [numOutputs][bufferLength]
                          device float*        ring         [[buffer(3)]], // [numInputs][ringCapacity]
                          const device float*  delaysPrev   [[buffer(4)]], // [in][out] samples
                          const device float*  delaysCurr   [[buffer(5)]],
                          const device float*  gainsPrev    [[buffer(6)]], // [in][out] linear
                          const device float*  gainsCurr    [[buffer(7)]],
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

    if (groupId < numOut)
    {
        // ===== Gather block: one output channel, sum over all inputs =====
        const uint outIdx = groupId;
        device float* chOut = output + outIdx * len;
        const float invLen = 1.0f / float(len);

        for (uint s = threadId; s < len; s += threadsPerTg)
        {
            // Ramp reaches exactly `curr` on the last sample so the next
            // launch (whose prev == this curr) continues smoothly.
            const float t = float(s + 1) * invLen;
            float acc = 0.0f;

            for (uint inIdx = 0; inIdx < numIn; ++inIdx)
            {
                const uint m = inIdx * numOut + outIdx;

                const float gp = gainsPrev[m];
                const float gc = gainsCurr[m];
                if (gp == 0.0f && gc == 0.0f)
                    continue;

                const float gain = gp + (gc - gp) * t;

                float d = delaysPrev[m] + (delaysCurr[m] - delaysPrev[m]) * t;
                d = max(d, 0.0f);

                const uint  delayInt = uint(d);
                const float frac     = d - float(delayInt);
                const int   off0     = int(s) - int(delayInt);

                const float s0 = fetchSample(input, ring, inIdx, len, ringCap, pos, valid, off0);
                const float s1 = fetchSample(input, ring, inIdx, len, ringCap, pos, valid, off0 - 1);

                acc += gain * (s0 * (1.0f - frac) + s1 * frac);
            }

            chOut[s] = acc;
        }
    }
    else if (groupId < numOut + numIn)
    {
        // ===== Append block: copy this launch's input into the input's ring =====
        const uint inIdx = groupId - numOut;
        const device float* chIn = input + inIdx * len;
        const uint ringBase = inIdx * ringCap;

        for (uint s = threadId; s < len; s += threadsPerTg)
        {
            uint w = pos + s;
            if (w >= ringCap)
                w -= ringCap;
            ring[ringBase + w] = chIn[s];
        }
    }
}
)MSL";
