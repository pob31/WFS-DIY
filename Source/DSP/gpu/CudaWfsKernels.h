#pragma once

// CUDA C kernel sources embedded as a string literal (compiled at prepare()
// time via NVRTC). Mirror of the Metal Shading Language kernels in
// MetalWfsKernels.h - keep the two in sync.
//
// Two kernels per launch (K1 then K2, ordered on one stream):
//
//   K1 wfs_pairs - grid = pairGroups + 2*numInputs blocks x 256 threads:
//     - blocks <  pairGroups : PAIR role. ONE THREAD per (in,out) pair,
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
//     - blocks >= pairGroups : APPEND roles, sample-parallel. First numIn
//       blocks append the raw input to the direct ring; next numIn blocks
//       append the host-pre-filtered FR input (frIn staging) to the FR ring.
//       No hazard vs the pair role: appends write ring [pos, pos+len) while
//       pair reads only touch history < pos (ringValidSamples guard).
//
//   K2 wfs_reduce - grid = numOutputs blocks x 256 threads striding samples:
//     output[out][s] = sum over in of scratch, ascending in - a fixed,
//     deterministic summation order (no atomics).
//
// Scratch layout: scratch[(s * numOutputs + out) * numInputs + in] -
// consecutive pair threads (consecutive in, same out) write consecutive
// addresses each sample iteration (coalesced); the reduce streams numInputs
// contiguous floats per (out, s).

static constexpr const char* kWfsDelaySumKernelSource = R"CUDA(
extern "C" {

struct WfsParams
{
    unsigned int numInputs;
    unsigned int numOutputs;
    unsigned int bufferLength;     // samples per channel this launch
    unsigned int ringCapacity;     // per-input ring length (samples)
    unsigned int ringWritePos;     // append position for this launch (both rings)
    unsigned int ringValidSamples; // valid history behind the write position
    unsigned int pairGroups;       // thread blocks reserved for the pair role
    float        shelfCosW0;       // cos(2*pi*800/sr), precomputed at prepare
    float        shelfSinW0;       // sin(2*pi*800/sr)
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

// High-shelf biquad coefficients at fixed 800 Hz / Q = 0.3, gain in dB.
// Verbatim port of WFSHighShelfFilter::recalculateCoefficients (Audio EQ
// Cookbook, Q used as the shelf slope parameter S).
__device__ __forceinline__ void shelfCoeffs(float gainDb, float cosw0, float sinw0,
                                            float* b0, float* b1, float* b2,
                                            float* a1, float* a2)
{
    const float A     = powf(10.0f, gainDb / 40.0f);
    const float rootA = sqrtf(A);
    const float alpha = (sinw0 * 0.5f) * sqrtf((A + 1.0f / A) * (1.0f / 0.3f - 1.0f) + 2.0f);

    const float a0inv = 1.0f / ((A + 1.0f) - (A - 1.0f) * cosw0 + 2.0f * rootA * alpha);

    *b0 =  A * ((A + 1.0f) + (A - 1.0f) * cosw0 + 2.0f * rootA * alpha) * a0inv;
    *b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosw0) * a0inv;
    *b2 =  A * ((A + 1.0f) + (A - 1.0f) * cosw0 - 2.0f * rootA * alpha) * a0inv;
    *a1 =  2.0f * ((A - 1.0f) - (A + 1.0f) * cosw0) * a0inv;
    *a2 =  ((A + 1.0f) - (A - 1.0f) * cosw0 - 2.0f * rootA * alpha) * a0inv;
}

__global__ void wfs_pairs(const WfsParams           params,
                          const float* __restrict__ input,        // [numInputs][bufferLength] raw
                          const float* __restrict__ frIn,         // [numInputs][bufferLength] host-FR-filtered
                          float* __restrict__       ring,         // [numInputs][ringCapacity] direct history
                          float* __restrict__       frRing,       // [numInputs][ringCapacity] FR history
                          const float* __restrict__ delaysPrev,   // [in][out] samples
                          const float* __restrict__ delaysCurr,
                          const float* __restrict__ gainsPrev,    // [in][out] linear
                          const float* __restrict__ gainsCurr,
                          const float* __restrict__ frDelaysPrev, // [in][out] samples (absolute)
                          const float* __restrict__ frDelaysCurr,
                          const float* __restrict__ frGainsPrev,  // [in][out] linear (absolute)
                          const float* __restrict__ frGainsCurr,
                          const float* __restrict__ hfAttenDb,    // [in][out] dB, stepwise
                          const float* __restrict__ frHfAttenDb,  // [in][out] dB, stepwise
                          float* __restrict__       shelfState,   // [pairs][4] x1,x2,y1,y2 persistent
                          float* __restrict__       frShelfState, // [pairs][4] persistent
                          float* __restrict__       scratch)      // [(s*numOut+out)*numIn+in]
{
    const unsigned int numIn   = params.numInputs;
    const unsigned int numOut  = params.numOutputs;
    const unsigned int len     = params.bufferLength;
    const unsigned int ringCap = params.ringCapacity;
    const unsigned int pos     = params.ringWritePos;
    const unsigned int valid   = params.ringValidSamples;

    if (len == 0 || ringCap == 0)
        return;

    if (blockIdx.x >= params.pairGroups)
    {
        // ===== Append roles: copy this launch's staging into the rings =====
        const unsigned int a = blockIdx.x - params.pairGroups;
        const float* __restrict__ src;
        float* __restrict__ dstRing;
        unsigned int chIdx;
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

        const float* chIn = src + chIdx * len;
        const unsigned int ringBase = chIdx * ringCap;
        for (unsigned int s = threadIdx.x; s < len; s += blockDim.x)
        {
            unsigned int w = pos + s;
            if (w >= ringCap)
                w -= ringCap;
            dstRing[ringBase + w] = chIn[s];
        }
        return;
    }

    // ===== Pair role: one thread per (in,out) pair, serial over samples =====
    const unsigned int t = blockIdx.x * blockDim.x + threadIdx.x;
    if (t >= numIn * numOut)
        return;

    const unsigned int out = t / numIn;   // out-major: consecutive threads share out,
    const unsigned int in  = t % numIn;   // stride in -> coalesced scratch stores
    const unsigned int m   = in * numOut + out;

    const float dp  = delaysPrev[m],   dc  = delaysCurr[m];
    const float gp  = gainsPrev[m],    gc  = gainsCurr[m];
    const float fdp = frDelaysPrev[m], fdc = frDelaysCurr[m];
    const float fgp = frGainsPrev[m],  fgc = frGainsCurr[m];

    const bool doDir = (gp != 0.0f || gc != 0.0f);
    const bool doFr  = (fgp != 0.0f || fgc != 0.0f);

    if (!doDir && !doFr)
    {
        // Silent pair: scratch must still be complete for the reduce.
        // No filter state touched (CPU parity: closed gates freeze state).
        for (unsigned int s = 0; s < len; ++s)
            scratch[(s * numOut + out) * numIn + in] = 0.0f;
        return;
    }

    // Shelf coefficients: computed once per launch per open tap from the raw
    // dB matrices (stepwise per launch, like the CPU's per-block setGainDb).
    float b0 = 0, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    float x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    if (doDir)
    {
        shelfCoeffs(hfAttenDb[m], params.shelfCosW0, params.shelfSinW0, &b0, &b1, &b2, &a1, &a2);
        x1 = shelfState[m * 4 + 0]; x2 = shelfState[m * 4 + 1];
        y1 = shelfState[m * 4 + 2]; y2 = shelfState[m * 4 + 3];
    }
    float fb0 = 0, fb1 = 0, fb2 = 0, fa1 = 0, fa2 = 0;
    float fx1 = 0, fx2 = 0, fy1 = 0, fy2 = 0;
    if (doFr)
    {
        shelfCoeffs(frHfAttenDb[m], params.shelfCosW0, params.shelfSinW0, &fb0, &fb1, &fb2, &fa1, &fa2);
        fx1 = frShelfState[m * 4 + 0]; fx2 = frShelfState[m * 4 + 1];
        fy1 = frShelfState[m * 4 + 2]; fy2 = frShelfState[m * 4 + 3];
    }

    const float invLen = 1.0f / (float) len;

    for (unsigned int s = 0; s < len; ++s)
    {
        // Ramp reaches exactly `curr` on the last sample so the next launch
        // (whose prev == this curr) continues smoothly.
        const float tt = (float)(s + 1) * invLen;
        float acc = 0.0f;

        if (doDir)
        {
            const float gain = gp + (gc - gp) * tt;

            float d = dp + (dc - dp) * tt;
            d = fmaxf(d, 0.0f);
            const unsigned int delayInt = (unsigned int) d;
            const float        frac     = d - (float) delayInt;
            const int          off0     = (int) s - (int) delayInt;

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
            d = fmaxf(d, 0.0f);
            const unsigned int delayInt = (unsigned int) d;
            const float        frac     = d - (float) delayInt;
            const int          off0     = (int) s - (int) delayInt;

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

__global__ void wfs_reduce(const WfsParams           params,
                           const float* __restrict__ scratch, // [(s*numOut+out)*numIn+in]
                           float* __restrict__       output)  // [numOutputs][bufferLength]
{
    const unsigned int numIn  = params.numInputs;
    const unsigned int numOut = params.numOutputs;
    const unsigned int len    = params.bufferLength;

    const unsigned int out = blockIdx.x;
    if (out >= numOut || len == 0)
        return;

    for (unsigned int s = threadIdx.x; s < len; s += blockDim.x)
    {
        const unsigned int base = (s * numOut + out) * numIn;
        float acc = 0.0f;
        for (unsigned int in = 0; in < numIn; ++in)   // fixed ascending order: deterministic
            acc += scratch[base + in];
        output[out * len + s] = acc;
    }
}

} // extern "C"
)CUDA";
