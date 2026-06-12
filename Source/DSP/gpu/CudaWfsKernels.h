#pragma once

// CUDA C kernel source embedded as a string literal (compiled at prepare()
// time via NVRTC). Direct translation of the Metal Shading Language kernel in
// MetalWfsKernels.h - keep the two in sync.
//
// Metal threadgroup model maps 1:1 onto CUDA blocks:
//   threadgroup id (groupId)        -> blockIdx.x
//   thread index in tg (threadId)   -> threadIdx.x
//   threads per threadgroup         -> blockDim.x
//   grid = (numOutputs + numInputs) blocks, 256 threads each
//
//   - blockIdx.x <  numOutputs : gather block for that output channel
//   - blockIdx.x >= numOutputs : ring-append block for input (id - numOutputs)
//   - threads stride over samples
//   - persistent per-input ring buffers; positions host-tracked; reads only
//     ever touch previous launches' samples (current launch served from the
//     launch input directly) -> no atomics, no cross-block sync needed.
//   - per-sample prev->curr interpolation of delays and gains (zipper-free).

static constexpr const char* kWfsDelaySumKernelSource = R"CUDA(
extern "C" {

struct WfsParams
{
    unsigned int numInputs;
    unsigned int numOutputs;
    unsigned int bufferLength;     // samples per channel this launch
    unsigned int ringCapacity;     // per-input ring length (samples)
    unsigned int ringWritePos;     // append position for this launch
    unsigned int ringValidSamples; // valid history behind the write position
};

__device__ __forceinline__ float fetchSample(const float* __restrict__ inBase,
                                              const float* __restrict__ ring,
                                              unsigned int inIdx,
                                              unsigned int bufferLength,
                                              unsigned int ringCapacity,
                                              unsigned int pos,
                                              unsigned int validSamples,
                                              int off)
{
    if (off >= 0)
        return inBase[inIdx * bufferLength + (unsigned int) off];

    if ((unsigned int)(-off) > validSamples)
        return 0.0f;

    int idx = (int) pos + off;
    if (idx < 0)
        idx += (int) ringCapacity;

    return ring[inIdx * ringCapacity + (unsigned int) idx];
}

__global__ void wfs_delay_sum(const WfsParams      params,
                              const float* __restrict__ input,      // [numInputs][bufferLength]
                              float* __restrict__       output,     // [numOutputs][bufferLength]
                              float* __restrict__       ring,       // [numInputs][ringCapacity]
                              const float* __restrict__ delaysPrev, // [in][out] samples
                              const float* __restrict__ delaysCurr,
                              const float* __restrict__ gainsPrev,  // [in][out] linear
                              const float* __restrict__ gainsCurr)
{
    const unsigned int numIn   = params.numInputs;
    const unsigned int numOut  = params.numOutputs;
    const unsigned int len     = params.bufferLength;
    const unsigned int ringCap = params.ringCapacity;
    const unsigned int pos     = params.ringWritePos;
    const unsigned int valid   = params.ringValidSamples;

    const unsigned int groupId      = blockIdx.x;
    const unsigned int threadId     = threadIdx.x;
    const unsigned int threadsPerTg = blockDim.x;

    if (len == 0 || ringCap == 0)
        return;

    if (groupId < numOut)
    {
        // ===== Gather block: one output channel, sum over all inputs =====
        const unsigned int outIdx = groupId;
        float* chOut = output + outIdx * len;
        const float invLen = 1.0f / (float) len;

        for (unsigned int s = threadId; s < len; s += threadsPerTg)
        {
            // Ramp reaches exactly `curr` on the last sample so the next
            // launch (whose prev == this curr) continues smoothly.
            const float t = (float)(s + 1) * invLen;
            float acc = 0.0f;

            for (unsigned int inIdx = 0; inIdx < numIn; ++inIdx)
            {
                const unsigned int m = inIdx * numOut + outIdx;

                const float gp = gainsPrev[m];
                const float gc = gainsCurr[m];
                if (gp == 0.0f && gc == 0.0f)
                    continue;

                const float gain = gp + (gc - gp) * t;

                float d = delaysPrev[m] + (delaysCurr[m] - delaysPrev[m]) * t;
                d = fmaxf(d, 0.0f);

                const unsigned int delayInt = (unsigned int) d;
                const float        frac     = d - (float) delayInt;
                const int          off0     = (int) s - (int) delayInt;

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
        const unsigned int inIdx = groupId - numOut;
        const float* chIn = input + inIdx * len;
        const unsigned int ringBase = inIdx * ringCap;

        for (unsigned int s = threadId; s < len; s += threadsPerTg)
        {
            unsigned int w = pos + s;
            if (w >= ringCap)
                w -= ringCap;
            ring[ringBase + w] = chIn[s];
        }
    }
}

} // extern "C"
)CUDA";
