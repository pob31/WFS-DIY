#pragma once

/*
    MSL kernel source for the native GPU FDN reverb backend, embedded as a
    string literal (compiled at prepare() time by MetalFdnBackend, like the
    other native kernels). KEEP IN SYNC with CudaFdnKernels.h — the two
    strings implement the byte-identical algorithm; only the language
    scaffolding differs.

    fdn_process — one 16-line Feedback Delay Network per node, a sample-exact
    port of FDNAlgorithm::processNodeSample (Source/DSP/ReverbFDNAlgorithm.h).
    The feedback recurrence is serial per node, so the whole block is processed
    sample-by-sample inside one launch with all per-node/per-line state
    persisting across launches in device buffers.

    Mapping: one THREADGROUP per node, 16 THREADS (one per delay line). Each
    thread owns its line's delay ring + writePos, feedback-allpass ring +
    writePos, and 3-band decay-filter state. Thread 0 additionally owns the
    per-node 4-stage input diffuser, the output tap sum, and the tone/DC
    output filters.

    Per sample (CPU op order preserved exactly):
      1. each thread: taps[line] = delayRing[line][wp]   (read == write pos:
         the ring length equals the delay, so reading at wp before writing
         yields the sample from `delayLen` ago — same as the CPU).
         thread 0 also computes `diffused` (4-stage allpass cascade, gated on
         diffusionCoeff > 0.0001 exactly like the CPU; state frozen when off).
         BARRIER (taps + diffused visible).
      2. every thread computes the full 16-pt Walsh-Hadamard from shared taps
         into a private hb[16] (redundant but barrier-free; ×0.25 like the CPU).
      3. thread 0: output = sum hb[k]*nodeTapSigns[k]; one-pole 8 kHz tone LPF;
         DC blocker; ×4 (+12 dB); write outputs[node][s].
      4. each thread: feedback allpass(0.55) on hb[line] -> 3-band decay filter
         -> + diffused*inputGains[line] -> write delayRing[line][wp]; advance wp.
         BARRIER (taps/diffused reused next sample).
    State persisted after the block.

    Parameters are applied stepwise (the CPU FDN does not ramp), so there is no
    prev->curr machinery — coefficients are constant across the block.
*/

static const char* const kFdnProcessKernelSource = R"MSL(
#include <metal_stdlib>
using namespace metal;

constant uint NUM_LINES = 16u;
constant uint NUM_DIFFUSERS = 4u;

struct FdnParams
{
    uint  numNodes;
    uint  blockSize;
    uint  maxDelayLen;     // per-line delay-ring stride
    uint  maxDiffLen;      // per-stage diffuser-ring stride
    uint  maxFbApLen;      // per-line feedback-allpass-ring stride
    float toneCoeff;       // one-pole 8 kHz LPF coeff
    float lowCoeff;        // 3-band crossover (low)
    float highCoeff;       // 3-band crossover (high)
    float diffusionCoeff;  // diffusion * 0.85
    float feedbackAPCoeff; // 0.55
    float dcPole;          // 0.9995
    float outputGain;      // 4.0 (+12 dB)
};

kernel void fdn_process (
    constant FdnParams&   p              [[buffer(0)]],
    const device float*   inputs         [[buffer(1)]],   // [numNodes][blockSize]
    device float*         outputs        [[buffer(2)]],   // [numNodes][blockSize]
    const device int*     delayLengths   [[buffer(3)]],   // [numNodes*16]
    const device int*     diffuserDelays [[buffer(4)]],   // [numNodes*4]
    const device int*     fbApDelays     [[buffer(5)]],   // [numNodes*16]
    const device float*   nodeTapSigns   [[buffer(6)]],   // [numNodes*16]
    const device float*   inputGains     [[buffer(7)]],   // [16] (global)
    const device float*   gainLow        [[buffer(8)]],   // [numNodes*16]
    const device float*   gainMid        [[buffer(9)]],   // [numNodes*16]
    const device float*   gainHigh       [[buffer(10)]],  // [numNodes*16]
    device float*         delayRings     [[buffer(11)]],  // [numNodes*16*maxDelayLen]
    device int*           delayWritePos  [[buffer(12)]],  // [numNodes*16]
    device float*         diffRings      [[buffer(13)]],  // [numNodes*4*maxDiffLen]
    device int*           diffWritePos   [[buffer(14)]],  // [numNodes*4]
    device float*         fbApRings      [[buffer(15)]],  // [numNodes*16*maxFbApLen]
    device int*           fbApWritePos   [[buffer(16)]],  // [numNodes*16]
    device float*         decayLowState  [[buffer(17)]],  // [numNodes*16]
    device float*         decayHighState [[buffer(18)]],  // [numNodes*16]
    device float*         toneState      [[buffer(19)]],  // [numNodes]
    device float*         dcState        [[buffer(20)]],  // [numNodes*2] (x1, y1)
    uint nodeId [[threadgroup_position_in_grid]],
    uint lineId [[thread_position_in_threadgroup]])
{
    if (nodeId >= p.numNodes || lineId >= NUM_LINES)
        return;

    threadgroup float taps[16];
    threadgroup float diffusedShared;

    const uint nl = nodeId * NUM_LINES + lineId;

    // This line's persistent state -> registers.
    const int   dLen  = delayLengths[nl];
    const int   fbLen = fbApDelays[nl];
    const float gL = gainLow[nl];
    const float gM = gainMid[nl];
    const float gH = gainHigh[nl];
    const float ig = inputGains[lineId];
    int   dwp   = delayWritePos[nl];
    int   fbwp  = fbApWritePos[nl];
    float lowS  = decayLowState[nl];
    float highS = decayHighState[nl];

    device float* dRing  = delayRings + (ulong) nl * p.maxDelayLen;
    device float* fbRing = fbApRings  + (ulong) nl * p.maxFbApLen;

    // Thread 0: per-node diffuser + output-filter state -> registers.
    float toneSt = 0.0f, dcX1 = 0.0f, dcY1 = 0.0f;
    int   dfWp0 = 0, dfWp1 = 0, dfWp2 = 0, dfWp3 = 0;
    if (lineId == 0u)
    {
        toneSt = toneState[nodeId];
        dcX1 = dcState[nodeId * 2u + 0u];
        dcY1 = dcState[nodeId * 2u + 1u];
        dfWp0 = diffWritePos[nodeId * NUM_DIFFUSERS + 0u];
        dfWp1 = diffWritePos[nodeId * NUM_DIFFUSERS + 1u];
        dfWp2 = diffWritePos[nodeId * NUM_DIFFUSERS + 2u];
        dfWp3 = diffWritePos[nodeId * NUM_DIFFUSERS + 3u];
    }

    const float apc = p.feedbackAPCoeff;

    for (uint s = 0; s < p.blockSize; ++s)
    {
        // 1a. read this line's delay tap (read pos == write pos).
        taps[lineId] = dRing[(uint) dwp];

        // 1b. thread 0: 4-stage input allpass diffusion (gated like the CPU).
        if (lineId == 0u)
        {
            float diffused = inputs[nodeId * p.blockSize + s];
            if (p.diffusionCoeff > 0.0001f)
            {
                const float c = p.diffusionCoeff;
                // stage 0
                {
                    device float* r = diffRings + (ulong) (nodeId * NUM_DIFFUSERS + 0u) * p.maxDiffLen;
                    const int len = diffuserDelays[nodeId * NUM_DIFFUSERS + 0u];
                    float delayed = r[(uint) dfWp0];
                    float v = diffused - c * delayed;
                    r[(uint) dfWp0] = v;
                    if (++dfWp0 >= len) dfWp0 = 0;
                    diffused = delayed + c * v;
                }
                // stage 1
                {
                    device float* r = diffRings + (ulong) (nodeId * NUM_DIFFUSERS + 1u) * p.maxDiffLen;
                    const int len = diffuserDelays[nodeId * NUM_DIFFUSERS + 1u];
                    float delayed = r[(uint) dfWp1];
                    float v = diffused - c * delayed;
                    r[(uint) dfWp1] = v;
                    if (++dfWp1 >= len) dfWp1 = 0;
                    diffused = delayed + c * v;
                }
                // stage 2
                {
                    device float* r = diffRings + (ulong) (nodeId * NUM_DIFFUSERS + 2u) * p.maxDiffLen;
                    const int len = diffuserDelays[nodeId * NUM_DIFFUSERS + 2u];
                    float delayed = r[(uint) dfWp2];
                    float v = diffused - c * delayed;
                    r[(uint) dfWp2] = v;
                    if (++dfWp2 >= len) dfWp2 = 0;
                    diffused = delayed + c * v;
                }
                // stage 3
                {
                    device float* r = diffRings + (ulong) (nodeId * NUM_DIFFUSERS + 3u) * p.maxDiffLen;
                    const int len = diffuserDelays[nodeId * NUM_DIFFUSERS + 3u];
                    float delayed = r[(uint) dfWp3];
                    float v = diffused - c * delayed;
                    r[(uint) dfWp3] = v;
                    if (++dfWp3 >= len) dfWp3 = 0;
                    diffused = delayed + c * v;
                }
            }
            diffusedShared = diffused;
        }

        threadgroup_barrier (mem_flags::mem_threadgroup);

        // 2. full 16-pt Walsh-Hadamard from shared taps (redundant per thread).
        float hb[16];
        for (uint k = 0; k < NUM_LINES; ++k)
            hb[k] = taps[k];
        for (uint len = 1u; len < NUM_LINES; len <<= 1)
            for (uint i = 0u; i < NUM_LINES; i += len << 1)
                for (uint j = i; j < i + len; ++j)
                {
                    float a = hb[j];
                    float b = hb[j + len];
                    hb[j]       = a + b;
                    hb[j + len] = a - b;
                }
        for (uint k = 0; k < NUM_LINES; ++k)
            hb[k] *= 0.25f;

        // 3. thread 0: output tap + tone LPF + DC blocker + output gain.
        if (lineId == 0u)
        {
            float output = 0.0f;
            for (uint k = 0; k < NUM_LINES; ++k)
                output += hb[k] * nodeTapSigns[nodeId * NUM_LINES + k];

            toneSt += p.toneCoeff * (output - toneSt);
            output = toneSt;

            float dcOut = output - dcX1 + p.dcPole * dcY1;
            dcX1 = output;
            dcY1 = dcOut;

            outputs[nodeId * p.blockSize + s] = dcOut * p.outputGain;
        }

        // 4. feedback: per-line allpass(0.55) -> 3-band decay -> + diffused*gain,
        //    write back, advance.
        {
            float h = hb[lineId];
            float delayed = fbRing[(uint) fbwp];
            float v = h - apc * delayed;
            fbRing[(uint) fbwp] = v;
            if (++fbwp >= fbLen) fbwp = 0;
            float scattered = delayed + apc * v;

            lowS  += p.lowCoeff  * (scattered - lowS);
            highS += p.highCoeff * (scattered - highS);
            float low  = lowS;
            float high = scattered - highS;
            float mid  = highS - lowS;
            float decayed = low * gL + mid * gM + high * gH;

            float writeVal = decayed + diffusedShared * ig;
            dRing[(uint) dwp] = writeVal;
            if (++dwp >= dLen) dwp = 0;
        }

        threadgroup_barrier (mem_flags::mem_threadgroup);
    }

    // Persist per-line state.
    delayWritePos[nl]  = dwp;
    fbApWritePos[nl]   = fbwp;
    decayLowState[nl]  = lowS;
    decayHighState[nl] = highS;

    if (lineId == 0u)
    {
        toneState[nodeId] = toneSt;
        dcState[nodeId * 2u + 0u] = dcX1;
        dcState[nodeId * 2u + 1u] = dcY1;
        diffWritePos[nodeId * NUM_DIFFUSERS + 0u] = dfWp0;
        diffWritePos[nodeId * NUM_DIFFUSERS + 1u] = dfWp1;
        diffWritePos[nodeId * NUM_DIFFUSERS + 2u] = dfWp2;
        diffWritePos[nodeId * NUM_DIFFUSERS + 3u] = dfWp3;
    }
}
)MSL";
