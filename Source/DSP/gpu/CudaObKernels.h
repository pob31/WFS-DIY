#pragma once

// CUDA C kernel sources for the native GPU OutputBuffer (scatter / write-time)
// algorithm, embedded as a string literal (compiled at prepare() time via NVRTC).
// Mirror of the Metal Shading Language kernels in MetalObKernels.h - keep the
// two in sync. This file is the validated reference: Experiments/cuda-output-buffer-test
// checks it against a CPU scatter model on real hardware (mirroring the WFS twin).
//
// v2 architecture - PER-PAIR accumulators + reduce (one thread per (in,out)
// pair, the proven wfs_pairs/wfs_reduce occupancy shape). v1 used one thread
// per OUTPUT writing a shared per-output accumulator; at 32x27 that is only 27
// serial threads and the pump missed the buffer-64 deadline. Per-pair private
// accumulators restore numIn*numOut-way parallelism with NO atomics; the
// deterministic ascending reduce reproduces the CPU's shared-buffer semantics
// exactly by linearity. See MetalObKernels.h for the full architecture and the
// d >= 1 sample delay contract (a write-time scatter cannot represent d < 1:
// the head cell was just read+cleared, so a same-cell write only re-emerges
// when the head wraps ~1 s later; pipeline-compensated delays below the latency
// floor clamp to 1 sample).
//
// Two kernels per launch (K1 then K2, ordered on one stream):
//   K1 ob_pairs  - grid = ceil(pairs/256) blocks x 256 threads, one thread per
//     (in,out) pair, serial over samples: emit pairOut[s] = pairAcc[cell],
//     clear, per-pair 800 Hz shelf on raw input (direct) + host-pre-filtered
//     frIn (FR), scatter with fractional split to cell + d (direct d ramps
//     prev->curr; FR d is the host-staged per-sample delay). Emit+clear runs
//     for every pair (drains gate-close tails); filter+scatter gated on
//     (gainPrev != 0 || gainCurr != 0) per path.
//   K2 ob_reduce - grid = numOutputs blocks x 256 threads striding samples:
//     output[out][s] = sum over in of pairOut, ascending in (deterministic).
//
// pairOut layout: pairOut[(s * numOutputs + out) * numInputs + in].
// pairAcc layout: pairAcc[t * accLength + cell], t = thread-ordered pair index.

static constexpr const char* kObScatterKernelSource = R"CUDA(
extern "C" {

struct ObParams
{
    unsigned int numInputs;
    unsigned int numOutputs;
    unsigned int bufferLength;   // samples per channel this launch
    unsigned int accLength;      // per-pair accumulator length (samples, ~1 s)
    unsigned int writePos;       // accumulator head at the start of this launch
    float        shelfCosW0;     // cos(2*pi*800/sr), precomputed at prepare
    float        shelfSinW0;     // sin(2*pi*800/sr)
};

// High-shelf biquad coefficients at fixed 800 Hz / Q = 0.3, gain in dB.
// Verbatim port of WFSHighShelfFilter::recalculateCoefficients. Identical to WFS.
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

// ===== K1: per-pair filter + emit + scatter (one thread per (in,out) pair) =====
__global__ void ob_pairs(const ObParams            params,
                         const float* __restrict__ input,          // [numIn][len] raw
                         const float* __restrict__ frIn,           // [numIn][len] host-FR-filtered
                         const float* __restrict__ hfAttenDb,      // [in*numOut+out] dB, stepwise
                         const float* __restrict__ frHfAttenDb,    // [in*numOut+out] dB, stepwise
                         const float* __restrict__ delaysPrev,     // [in*numOut+out] samples (>= 1)
                         const float* __restrict__ delaysCurr,
                         const float* __restrict__ gainsPrev,      // [in*numOut+out] linear
                         const float* __restrict__ gainsCurr,
                         const float* __restrict__ frDelaySamples, // [pair*len+s] per-sample absolute FR delay (>= 1)
                         const float* __restrict__ frGainsPrev,
                         const float* __restrict__ frGainsCurr,
                         float* __restrict__       shelfState,     // [pairs][4] persistent
                         float* __restrict__       frShelfState,   // [pairs][4] persistent
                         float* __restrict__       pairAcc,        // [pairs][accLen] persistent (thread-ordered)
                         float* __restrict__       pairOut)        // [(s*numOut+out)*numIn+in]
{
    const unsigned int numIn  = params.numInputs;
    const unsigned int numOut = params.numOutputs;
    const unsigned int len    = params.bufferLength;
    const unsigned int accLen = params.accLength;
    const unsigned int pairs  = numIn * numOut;

    const unsigned int t = blockIdx.x * blockDim.x + threadIdx.x;
    if (t >= pairs || len == 0 || accLen == 0)
        return;

    const unsigned int out = t / numIn;   // out-major: consecutive threads share out,
    const unsigned int in  = t % numIn;   // stride in -> coalesced pairOut stores
    const unsigned int m   = in * numOut + out;

    float* acc = pairAcc + (size_t) t * accLen;

    const float gp  = gainsPrev[m],   gc  = gainsCurr[m];
    const float fgp = frGainsPrev[m], fgc = frGainsCurr[m];
    const bool doDir = (gp != 0.0f || gc != 0.0f);
    const bool doFr  = (fgp != 0.0f || fgc != 0.0f);

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

    const float dp = delaysPrev[m], dc = delaysCurr[m];
    const float invLen = 1.0f / (float) len;
    const float* chIn   = input + (size_t) in * len;
    const float* chFrIn = frIn  + (size_t) in * len;
    const float* frD    = frDelaySamples + (size_t) m * len;

    for (unsigned int s = 0; s < len; ++s)
    {
        unsigned int cell = params.writePos + s;
        if (cell >= accLen)
            cell -= accLen;

        // Emit + clear ALWAYS (drains the tail after a gate closes), in CPU
        // order: read before this sample's scatter.
        pairOut[(s * numOut + out) * numIn + in] = acc[cell];
        acc[cell] = 0.0f;

        if (!doDir && !doFr)
            continue;

        // Ramp reaches exactly `curr` on the last sample so the next launch
        // (whose prev == this curr) continues smoothly.
        const float tt = (float)(s + 1) * invLen;

        if (doDir)
        {
            const float v = chIn[s];
            const float w = b0 * v + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = v; y2 = y1; y1 = w;

            const float gain = gp + (gc - gp) * tt;
            float d = dp + (dc - dp) * tt;
            d = fmaxf(d, 1.0f);               // d >= 1: writes strictly future

            float ewp = (float) cell + d;     // d <= accLen-1 -> single wrap
            if (ewp >= (float) accLen)
                ewp -= (float) accLen;

            const unsigned int p1 = (unsigned int) ewp;
            unsigned int       p2 = p1 + 1;
            if (p2 >= accLen)
                p2 -= accLen;
            const float frac = ewp - (float) p1;

            const float c = w * gain;
            acc[p1] += c * (1.0f - frac);
            acc[p2] += c * frac;
        }

        if (doFr)
        {
            const float v = chFrIn[s];
            const float w = fb0 * v + fb1 * fx1 + fb2 * fx2 - fa1 * fy1 - fa2 * fy2;
            fx2 = fx1; fx1 = v; fy2 = fy1; fy1 = w;

            const float gain = fgp + (fgc - fgp) * tt;
            float d = frD[s];                 // per-sample, diffusion baked in
            d = fmaxf(d, 1.0f);

            float ewp = (float) cell + d;
            if (ewp >= (float) accLen)
                ewp -= (float) accLen;

            const unsigned int p1 = (unsigned int) ewp;
            unsigned int       p2 = p1 + 1;
            if (p2 >= accLen)
                p2 -= accLen;
            const float frac = ewp - (float) p1;

            const float c = w * gain;
            acc[p1] += c * (1.0f - frac);
            acc[p2] += c * frac;
        }
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

// ===== K2: deterministic per-output reduction (identical to wfs_reduce) =====
__global__ void ob_reduce(const ObParams            params,
                          const float* __restrict__ pairOut, // [(s*numOut+out)*numIn+in]
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
            acc += pairOut[base + in];
        output[out * len + s] = acc;
    }
}

} // extern "C"
)CUDA";
