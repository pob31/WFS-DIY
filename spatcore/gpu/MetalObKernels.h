#pragma once

// MSL kernel sources for the native GPU OutputBuffer (scatter / write-time)
// algorithm, embedded as a string literal (compiled at prepare() time).
// Mirror of the CUDA kernels in CudaObKernels.h - keep the two in sync.
// (The CUDA string is the validated reference: Experiments/cuda-output-buffer-test
// checks it against a CPU scatter model on real hardware, mirroring the WFS twin.)
//
// This is the SCATTER twin of the WFS gather kernels (MetalWfsKernels.h). The
// WFS path reads each input's history ring at a fractional NEGATIVE offset and
// reduces per output (read-time delay = the CPU InputBuffer formulation). This
// path instead SCATTERS each input's filtered sample FORWARD into a persistent
// delay accumulator at the fractional write position writePos + delay
// (write-time delay = the CPU OutputBufferProcessor formulation,
// Source/DSP/OutputBufferProcessor.h:472-578).
//
// v2 architecture - PER-PAIR accumulators + reduce (one thread per (in,out)
// pair, the proven wfs_pairs/wfs_reduce occupancy shape). v1 used one thread
// per OUTPUT writing a shared per-output accumulator; at 32x27 that is only 27
// serial threads and the pump missed the buffer-64 deadline (~2.7 ms vs 1.33 ms
// = ~350 underruns/s in the field). Per-pair private accumulators restore
// numIn*numOut-way parallelism with NO atomics, and the deterministic ascending
// reduce reproduces the CPU's shared-buffer semantics exactly by linearity
// (the same contributions land in the same output samples; only float summation
// order differs).
//
// Two kernels per launch (K1 then K2, ordered by the serial compute encoder):
//
//   K1 ob_pairs - grid = numInputs*numOutputs threads (one per (in,out) pair),
//     serial over the block's samples (the per-pair HF shelf biquads are IIRs -
//     y[n] depends on y[n-1] - so samples cannot be parallelised within a pair;
//     the emit/clear/scatter on the pair's own accumulator is serial for the
//     same read-then-write reason as the CPU loop). Thread id is out-major
//     (out = t / numIn, in = t % numIn) so consecutive threads write
//     consecutive pairOut addresses (coalesced, same as wfs_pairs). Per sample,
//     in CPU order:
//       1. emit  pairOut[s] = pairAcc[cell]   (cell = writePos + s; this pair's
//          delayed contribution stream - direct + FR share ONE accumulator,
//          they are summed at read in the CPU too)
//       2. clear pairAcc[cell]
//       3. filter: per-pair 800 Hz / Q 0.3 air shelf on the raw input (direct)
//          and on the host-pre-filtered frIn (FR path; LowCut + HighShelf are
//          applied per input on the host - an LTI shelf before vs. after the
//          delay is equivalent, so the CPU's per-(in,out) FR chain reduces to
//          per-input host filtering + the per-pair air shelf here). Shelf
//          coefficients computed in-kernel once per launch from the raw dB
//          matrices (stepwise like the CPU's per-block setGainDb).
//       4. scatter: split each filtered contribution across cell+d and cell+d+1
//          with the fractional weights (1-frac)/frac. Direct d ramps prev->curr
//          across the block; FR d is the host-staged PER-SAMPLE delay (diffusion
//          grain sub-stepped at 64 samples, CPU parity).
//     DELAY CONTRACT: d >= 1 sample, enforced host-side and re-clamped here.
//     A write-time scatter cannot represent d < 1: the cell at the head was
//     just read+cleared, so a same-cell write only re-emerges when the head
//     wraps (~1 s later). Pipeline-compensated delays below the latency floor
//     therefore clamp to 1 sample (the scatter analogue of the gather's
//     "delays >= L are exact, below L clamp to the floor" contract).
//     Gating: emit+clear runs for EVERY pair every launch (a gate that just
//     closed leaves a tail in the accumulator that must still drain); only the
//     filter+scatter work is gated on (gainPrev != 0 || gainCurr != 0) per
//     path, and a closed gate does not evaluate or advance filter state
//     (CPU parity).
//
//   K2 ob_reduce - grid = numOutputs threadgroups, threads stride samples:
//     output[out][s] = sum over in of pairOut, ascending in - a fixed,
//     deterministic summation order (no atomics). Identical to wfs_reduce.
//
// pairOut layout: pairOut[(s * numOutputs + out) * numInputs + in].
// pairAcc layout: pairAcc[t * accLength + cell], t = thread-ordered pair index.
// Device memory: pairs * accLength * 4 B persistent (e.g. 32x27 @ 1 s/48 kHz
// ~ 166 MB; 64x64 ~ 787 MB - acceptable on unified memory, watch small VRAM).
//
// Parity note: like the WFS GPU port, the direct delay/gain trajectory is a
// per-block prev->curr linear ramp, NOT the CPU OutputBuffer's
// DelayTargetSmoother box filter + teleport mute-move-unmute envelope (same
// documented divergence as NativeGpuWfsAlgorithm). The scatter architecture,
// fractional write-split and block-stepped diffusion grain are reproduced
// exactly.

static constexpr const char* kObScatterKernelSource = R"MSL(
#include <metal_stdlib>
using namespace metal;

struct ObParams
{
    uint  numInputs;
    uint  numOutputs;
    uint  bufferLength;     // samples per channel this launch
    uint  accLength;        // per-pair accumulator length (samples, ~1 s)
    uint  writePos;         // accumulator head position at the start of this launch
    float shelfCosW0;       // cos(2*pi*800/sr), precomputed at prepare
    float shelfSinW0;       // sin(2*pi*800/sr)
};

// High-shelf biquad coefficients at fixed 800 Hz / Q = 0.3, gain in dB.
// Verbatim port of WFSHighShelfFilter::recalculateCoefficients (Audio EQ
// Cookbook, Q used as the shelf slope parameter S). Identical to the WFS kernel.
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

// ===== K1: per-pair filter + emit + scatter (one thread per (in,out) pair) =====
kernel void ob_pairs(constant ObParams&  params         [[buffer(0)]],
                     const device float*  input          [[buffer(1)]],  // [numIn][len] raw
                     const device float*  frIn           [[buffer(2)]],  // [numIn][len] host-FR-filtered
                     const device float*  hfAttenDb      [[buffer(3)]],  // [in*numOut+out] dB, stepwise
                     const device float*  frHfAttenDb    [[buffer(4)]],  // [in*numOut+out] dB, stepwise
                     const device float*  delaysPrev     [[buffer(5)]],  // [in*numOut+out] samples (>= 1)
                     const device float*  delaysCurr     [[buffer(6)]],
                     const device float*  gainsPrev      [[buffer(7)]],  // [in*numOut+out] linear
                     const device float*  gainsCurr      [[buffer(8)]],
                     const device float*  frDelaySamples [[buffer(9)]],  // [pair*len+s] per-sample absolute FR delay (>= 1, jitter sub-stepped on host)
                     const device float*  frGainsPrev    [[buffer(10)]],
                     const device float*  frGainsCurr    [[buffer(11)]],
                     device float*        shelfState     [[buffer(12)]], // [pairs][4] persistent
                     device float*        frShelfState   [[buffer(13)]], // [pairs][4] persistent
                     device float*        pairAcc        [[buffer(14)]], // [pairs][accLen] persistent (thread-ordered)
                     device float*        pairOut        [[buffer(15)]], // [(s*numOut+out)*numIn+in]
                     uint                 t              [[thread_position_in_grid]])
{
    const uint numIn  = params.numInputs;
    const uint numOut = params.numOutputs;
    const uint len    = params.bufferLength;
    const uint accLen = params.accLength;
    const uint pairs  = numIn * numOut;

    if (t >= pairs || len == 0 || accLen == 0)
        return;

    const uint out = t / numIn;   // out-major: consecutive threads share out,
    const uint in  = t % numIn;   // stride in -> coalesced pairOut stores
    const uint m   = in * numOut + out;

    device float* acc = pairAcc + (size_t) t * accLen;

    const float gp  = gainsPrev[m],   gc  = gainsCurr[m];
    const float fgp = frGainsPrev[m], fgc = frGainsCurr[m];
    const bool doDir = (gp != 0.0f || gc != 0.0f);
    const bool doFr  = (fgp != 0.0f || fgc != 0.0f);

    // Shelf coefficients: once per launch per open gate, from the raw dB
    // matrices (stepwise like the CPU's per-block setGainDb). Closed gates do
    // not evaluate or advance filter state (CPU parity).
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

    const float dp = delaysPrev[m], dc = delaysCurr[m];
    const float invLen = 1.0f / float(len);
    const device float* chIn   = input + (size_t) in * len;
    const device float* chFrIn = frIn  + (size_t) in * len;
    const device float* frD    = frDelaySamples + (size_t) m * len;

    for (uint s = 0; s < len; ++s)
    {
        uint cell = params.writePos + s;
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
        const float tt = float(s + 1) * invLen;

        if (doDir)
        {
            const float v = chIn[s];
            const float w = b0 * v + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = v; y2 = y1; y1 = w;

            const float gain = gp + (gc - gp) * tt;
            float d = dp + (dc - dp) * tt;
            d = max(d, 1.0f);                 // d >= 1: writes strictly future

            float ewp = float(cell) + d;      // d <= accLen-1 -> single wrap
            if (ewp >= float(accLen))
                ewp -= float(accLen);

            const uint  p1   = uint(ewp);
            uint        p2   = p1 + 1;
            if (p2 >= accLen)
                p2 -= accLen;
            const float frac = ewp - float(p1);

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
            d = max(d, 1.0f);

            float ewp = float(cell) + d;
            if (ewp >= float(accLen))
                ewp -= float(accLen);

            const uint  p1   = uint(ewp);
            uint        p2   = p1 + 1;
            if (p2 >= accLen)
                p2 -= accLen;
            const float frac = ewp - float(p1);

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
kernel void ob_reduce(constant ObParams&  params       [[buffer(0)]],
                      const device float*  pairOut      [[buffer(1)]], // [(s*numOut+out)*numIn+in]
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
            acc += pairOut[base + in];
        output[out * len + s] = acc;
    }
}
)MSL";
