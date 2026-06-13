#pragma once

/*
    MSL kernel source for the native GPU SDN reverb backend, embedded as a
    string literal (compiled at prepare() time by MetalSdnBackend, like the
    other native kernels). KEEP IN SYNC with CudaSdnKernels.h — the two strings
    implement the byte-identical algorithm; only the language scaffolding differs.

    sdn_process — one coupled Scattering Delay Network across all nodes, a
    sample-exact port of the synchronous SDNAlgorithm::processBlock
    (Source/DSP/ReverbSDNAlgorithm.h). The network is recurrent and couples
    nodes within a block (an inter-node delay can be shorter than the block), so
    the whole block runs sample-by-sample inside ONE launch, with all per-path /
    per-node state persisting across launches in device buffers.

    Mapping: ONE THREADGROUP, one THREAD per node (N <= 16). A node-thread owns
    its node's diffuser + tone/DC output state; the per-path delay lines and
    3-band decay-filter state live in device buffers indexed by pathIndex.

    Per sample (CPU op order preserved exactly):
      1. read incoming from every path i->n at the snapshot read head
         (ringWritePos + s - delay), with the crossfade dual-tap when a path's
         mix < 1; Householder X = (2/(N-1)) * sum(incoming).
      2. output = sum_i (X - incoming[i]) accumulated in path order (independent
         of, and before, the decay filter).
      3. diffuse the node input through 2 allpass stages (gated diffusion>1e-4,
         state frozen when off).
      4. for every outgoing path n->i: signal = (X - incoming[i]) + diffused/N,
         3-band decay filter, write at the shared write head (ringWritePos + s).
      5. tone one-pole LPF -> DC blocker -> * sdnOutputGain -> outputs[n][s].
      BARRIER (mem_device) so this sample's cross-node writes are visible before
      the next sample's reads. One barrier suffices: every delay >= 1, so a
      sample never reads a cell written this same sample.

    The single ring write head and modulo-MAX_DELAY_SAMPLES addressing match
    SDNAlgorithm exactly; the host advances ringWritePos by blockSize per launch.
    Parameters/geometry are applied stepwise on the pump thread (no in-kernel
    ramp); the crossfade mix is the block-start snapshot, advanced host-side.
*/

static const char* const kSdnProcessKernelSource = R"MSL(
#include <metal_stdlib>
using namespace metal;

constant uint NUM_DIFFUSERS = 2u;

struct SdnParams
{
    uint  numNodes;
    uint  numPaths;
    uint  blockSize;
    uint  maxDelaySamples;   // per-path delay-line stride (8192)
    uint  ringWritePos;      // block-start write head (shared by all paths)
    uint  maxDiffLen;        // per-stage diffuser-ring stride
    float diffusionCoeff;    // diffusion * 0.5
    float toneCoeff;         // one-pole 8 kHz LPF
    float lowCoeff;          // 3-band crossover (low)
    float highCoeff;         // 3-band crossover (high)
    float dcPole;            // 0.9995
    float sdnOutputGain;     // (1 + 18/N) * 0.25
    float inputDistribution; // 1/N
    float crossfadeRate;     // global; 1/(sr*0.01) while crossfading
};

inline uint sdnPathIndex (uint from, uint to, uint n)
{
    uint idx = from * (n - 1u);
    idx += (to > from) ? (to - 1u) : to;
    return idx;
}

kernel void sdn_process (
    constant SdnParams&   p                 [[buffer(0)]],
    const device float*   inputs            [[buffer(1)]],   // [numNodes][blockSize]
    device float*         outputs           [[buffer(2)]],   // [numNodes][blockSize]
    device float*         delayLines        [[buffer(3)]],   // [numPaths][maxDelaySamples]
    const device int*     delayLength       [[buffer(4)]],   // [numPaths]
    const device int*     targetDelayLength [[buffer(5)]],   // [numPaths]
    const device float*   crossfadeMix      [[buffer(6)]],   // [numPaths]
    const device float*   gainLow           [[buffer(7)]],   // [numPaths]
    const device float*   gainMid           [[buffer(8)]],   // [numPaths]
    const device float*   gainHigh          [[buffer(9)]],   // [numPaths]
    device float*         decayLowState     [[buffer(10)]],  // [numPaths]
    device float*         decayHighState    [[buffer(11)]],  // [numPaths]
    const device int*     diffuserDelays    [[buffer(12)]],  // [numNodes*2]
    device float*         diffRings         [[buffer(13)]],  // [numNodes*2*maxDiffLen]
    device int*           diffWritePos      [[buffer(14)]],  // [numNodes*2]
    device float*         toneState         [[buffer(15)]],  // [numNodes]
    device float*         dcState           [[buffer(16)]],  // [numNodes*2] (x1, y1)
    uint nodeId [[thread_position_in_threadgroup]])
{
    const uint N = p.numNodes;
    if (nodeId >= N)
        return;
    const uint n = nodeId;

    const uint   MAXD = p.maxDelaySamples;
    const int    iMAXD = (int) MAXD;
    const float  invN = p.inputDistribution;
    const float  twoOverNm1 = 2.0f / (float) (N - 1u);
    const float  c = p.diffusionCoeff;
    const bool   doDiffuse = (c > 0.0001f);

    // Per-node state -> registers.
    float toneSt = toneState[n];
    float dcX1   = dcState[n * 2u + 0u];
    float dcY1   = dcState[n * 2u + 1u];
    int   dfWp0  = diffWritePos[n * NUM_DIFFUSERS + 0u];
    int   dfWp1  = diffWritePos[n * NUM_DIFFUSERS + 1u];
    const int diffLen0 = diffuserDelays[n * NUM_DIFFUSERS + 0u];
    const int diffLen1 = diffuserDelays[n * NUM_DIFFUSERS + 1u];
    device float* dr0 = diffRings + (ulong) (n * NUM_DIFFUSERS + 0u) * p.maxDiffLen;
    device float* dr1 = diffRings + (ulong) (n * NUM_DIFFUSERS + 1u) * p.maxDiffLen;

    for (uint s = 0; s < p.blockSize; ++s)
    {
        const int base = (int) (p.ringWritePos + s);

        // 1. read incoming from every path i->n; Householder sum.
        float incoming[16];
        float sumIn = 0.0f;
        uint  k = 0u;
        for (uint i = 0u; i < N; ++i)
        {
            if (i == n) continue;
            const uint pidx = sdnPathIndex (i, n, N);
            const float mix = crossfadeMix[pidx];
            float val;
            if (mix >= 1.0f)
            {
                int rp = (base - delayLength[pidx]) % iMAXD;
                if (rp < 0) rp += iMAXD;
                val = delayLines[(ulong) pidx * MAXD + (uint) rp];
            }
            else
            {
                int orp = (base - delayLength[pidx]) % iMAXD;
                if (orp < 0) orp += iMAXD;
                int nrp = (base - targetDelayLength[pidx]) % iMAXD;
                if (nrp < 0) nrp += iMAXD;
                const float oldS = delayLines[(ulong) pidx * MAXD + (uint) orp];
                const float newS = delayLines[(ulong) pidx * MAXD + (uint) nrp];
                const float m = fmin (1.0f, mix + p.crossfadeRate * (float) s);
                val = oldS * (1.0f - m) + newS * m;
            }
            incoming[k] = val;
            sumIn += val;
            ++k;
        }
        const uint inCount = k;
        const float X = twoOverNm1 * sumIn;

        // 2. output = sum of scattered (X - incoming[i]), in path order.
        float output = 0.0f;
        for (uint i = 0u; i < inCount; ++i)
            output += (X - incoming[i]);

        // 3. diffuse the node input (2 serial allpass stages, gated).
        float diffused = inputs[n * p.blockSize + s];
        if (doDiffuse)
        {
            { float delayed = dr0[(uint) dfWp0]; float v = diffused - c * delayed;
              dr0[(uint) dfWp0] = v; if (++dfWp0 >= diffLen0) dfWp0 = 0; diffused = delayed + c * v; }
            { float delayed = dr1[(uint) dfWp1]; float v = diffused - c * delayed;
              dr1[(uint) dfWp1] = v; if (++dfWp1 >= diffLen1) dfWp1 = 0; diffused = delayed + c * v; }
        }

        // 4. write outgoing paths n->i with the 3-band decay filter.
        const uint writePos = (uint) (base % iMAXD);
        uint outIdx = 0u;
        for (uint i = 0u; i < N; ++i)
        {
            if (i == n) continue;
            const uint pidx = sdnPathIndex (n, i, N);
            const float signal = (X - incoming[outIdx]) + diffused * invN;

            float lowS  = decayLowState[pidx];
            float highS = decayHighState[pidx];
            lowS  += p.lowCoeff  * (signal - lowS);
            highS += p.highCoeff * (signal - highS);
            const float low  = lowS;
            const float high = signal - highS;
            const float mid  = highS - lowS;
            const float outSig = low * gainLow[pidx] + mid * gainMid[pidx] + high * gainHigh[pidx];
            decayLowState[pidx]  = lowS;
            decayHighState[pidx] = highS;

            delayLines[(ulong) pidx * MAXD + writePos] = outSig;
            ++outIdx;
        }

        // 5. tone LPF -> DC blocker -> output gain.
        toneSt += p.toneCoeff * (output - toneSt);
        const float toned = toneSt;
        const float dcOut = toned - dcX1 + p.dcPole * dcY1;
        dcX1 = toned;
        dcY1 = dcOut;
        outputs[n * p.blockSize + s] = dcOut * p.sdnOutputGain;

        threadgroup_barrier (mem_flags::mem_device);
    }

    // Persist per-node state.
    toneState[n] = toneSt;
    dcState[n * 2u + 0u] = dcX1;
    dcState[n * 2u + 1u] = dcY1;
    diffWritePos[n * NUM_DIFFUSERS + 0u] = dfWp0;
    diffWritePos[n * NUM_DIFFUSERS + 1u] = dfWp1;
}
)MSL";
