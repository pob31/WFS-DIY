#pragma once

/*
    CUDA kernel source for the native GPU SDN reverb backend (NVRTC-compiled at
    prepare() time by CudaSdnBackend, like the other native kernels — no .cu
    build step, no nvcc).

    KEEP IN SYNC with MetalSdnKernels.h — the two strings implement the
    byte-identical algorithm; only the language scaffolding differs. The full
    algorithm/threading notes live in the Metal twin.

    Mapping: grid = 1 block, block = numNodes threads (one per node).
    threadIdx.x = node. One coupled Scattering Delay Network across all nodes,
    stepped sample-by-sample with one __syncthreads() per sample (which orders
    global memory within the block, the analogue of Metal's mem_device barrier).
*/

static const char* const kSdnProcessKernelSource = R"CUDA(

#define NUM_DIFFUSERS 2u

struct SdnParams
{
    unsigned int numNodes;
    unsigned int numPaths;
    unsigned int blockSize;
    unsigned int maxDelaySamples;
    unsigned int ringWritePos;
    unsigned int maxDiffLen;
    float diffusionCoeff;
    float toneCoeff;
    float lowCoeff;
    float highCoeff;
    float dcPole;
    float sdnOutputGain;
    float inputDistribution;
    float crossfadeRate;
};

__device__ inline unsigned int sdnPathIndex (unsigned int from, unsigned int to, unsigned int n)
{
    unsigned int idx = from * (n - 1u);
    idx += (to > from) ? (to - 1u) : to;
    return idx;
}

extern "C" __global__ void sdn_process (
    const SdnParams           p,
    const float* __restrict__ inputs,             // [numNodes][blockSize]
    float* __restrict__       outputs,            // [numNodes][blockSize]
    float* __restrict__       delayLines,         // [numPaths][maxDelaySamples]
    const int* __restrict__   delayLength,        // [numPaths]
    const int* __restrict__   targetDelayLength,  // [numPaths]
    const float* __restrict__ crossfadeMix,       // [numPaths]
    const float* __restrict__ gainLow,            // [numPaths]
    const float* __restrict__ gainMid,            // [numPaths]
    const float* __restrict__ gainHigh,           // [numPaths]
    float* __restrict__       decayLowState,      // [numPaths]
    float* __restrict__       decayHighState,     // [numPaths]
    const int* __restrict__   diffuserDelays,     // [numNodes*2]
    float* __restrict__       diffRings,          // [numNodes*2*maxDiffLen]
    int* __restrict__         diffWritePos,       // [numNodes*2]
    float* __restrict__       toneState,          // [numNodes]
    float* __restrict__       dcState)            // [numNodes*2]
{
    const unsigned int N = p.numNodes;
    const unsigned int nodeId = threadIdx.x;
    if (nodeId >= N)
        return;
    const unsigned int n = nodeId;

    const unsigned int MAXD = p.maxDelaySamples;
    const int   iMAXD = (int) MAXD;
    const float invN = p.inputDistribution;
    const float twoOverNm1 = 2.0f / (float) (N - 1u);
    const float c = p.diffusionCoeff;
    const bool  doDiffuse = (c > 0.0001f);

    float toneSt = toneState[n];
    float dcX1   = dcState[n * 2u + 0u];
    float dcY1   = dcState[n * 2u + 1u];
    int   dfWp0  = diffWritePos[n * NUM_DIFFUSERS + 0u];
    int   dfWp1  = diffWritePos[n * NUM_DIFFUSERS + 1u];
    const int diffLen0 = diffuserDelays[n * NUM_DIFFUSERS + 0u];
    const int diffLen1 = diffuserDelays[n * NUM_DIFFUSERS + 1u];
    float* dr0 = diffRings + (unsigned long long) (n * NUM_DIFFUSERS + 0u) * p.maxDiffLen;
    float* dr1 = diffRings + (unsigned long long) (n * NUM_DIFFUSERS + 1u) * p.maxDiffLen;

    for (unsigned int s = 0; s < p.blockSize; ++s)
    {
        const int base = (int) (p.ringWritePos + s);

        float incoming[32];  // per-node scratch; size >= MAX_NODES (reverb maxReverbChannels)
        float sumIn = 0.0f;
        unsigned int k = 0u;
        for (unsigned int i = 0u; i < N; ++i)
        {
            if (i == n) continue;
            const unsigned int pidx = sdnPathIndex (i, n, N);
            const float mix = crossfadeMix[pidx];
            float val;
            if (mix >= 1.0f)
            {
                int rp = (base - delayLength[pidx]) % iMAXD;
                if (rp < 0) rp += iMAXD;
                val = delayLines[(unsigned long long) pidx * MAXD + (unsigned int) rp];
            }
            else
            {
                int orp = (base - delayLength[pidx]) % iMAXD;
                if (orp < 0) orp += iMAXD;
                int nrp = (base - targetDelayLength[pidx]) % iMAXD;
                if (nrp < 0) nrp += iMAXD;
                const float oldS = delayLines[(unsigned long long) pidx * MAXD + (unsigned int) orp];
                const float newS = delayLines[(unsigned long long) pidx * MAXD + (unsigned int) nrp];
                const float m = fminf (1.0f, mix + p.crossfadeRate * (float) s);
                val = oldS * (1.0f - m) + newS * m;
            }
            incoming[k] = val;
            sumIn += val;
            ++k;
        }
        const unsigned int inCount = k;
        const float X = twoOverNm1 * sumIn;

        float output = 0.0f;
        for (unsigned int i = 0u; i < inCount; ++i)
            output += (X - incoming[i]);

        float diffused = inputs[n * p.blockSize + s];
        if (doDiffuse)
        {
            { float delayed = dr0[(unsigned int) dfWp0]; float v = diffused - c * delayed;
              dr0[(unsigned int) dfWp0] = v; if (++dfWp0 >= diffLen0) dfWp0 = 0; diffused = delayed + c * v; }
            { float delayed = dr1[(unsigned int) dfWp1]; float v = diffused - c * delayed;
              dr1[(unsigned int) dfWp1] = v; if (++dfWp1 >= diffLen1) dfWp1 = 0; diffused = delayed + c * v; }
        }

        const unsigned int writePos = (unsigned int) (base % iMAXD);
        unsigned int outIdx = 0u;
        for (unsigned int i = 0u; i < N; ++i)
        {
            if (i == n) continue;
            const unsigned int pidx = sdnPathIndex (n, i, N);
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

            delayLines[(unsigned long long) pidx * MAXD + writePos] = outSig;
            ++outIdx;
        }

        toneSt += p.toneCoeff * (output - toneSt);
        const float toned = toneSt;
        const float dcOut = toned - dcX1 + p.dcPole * dcY1;
        dcX1 = toned;
        dcY1 = dcOut;
        outputs[n * p.blockSize + s] = dcOut * p.sdnOutputGain;

        __syncthreads();
    }

    toneState[n] = toneSt;
    dcState[n * 2u + 0u] = dcX1;
    dcState[n * 2u + 1u] = dcY1;
    diffWritePos[n * NUM_DIFFUSERS + 0u] = dfWp0;
    diffWritePos[n * NUM_DIFFUSERS + 1u] = dfWp1;
}
)CUDA";
