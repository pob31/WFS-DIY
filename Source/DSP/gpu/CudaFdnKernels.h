#pragma once

/*
    CUDA kernel source for the native GPU FDN reverb backend (NVRTC-compiled
    at prepare() time by CudaFdnBackend, like the other native kernels — no
    .cu build step, no nvcc).

    KEEP IN SYNC with MetalFdnKernels.h — the two strings implement the
    byte-identical algorithm; only the language scaffolding differs. The full
    algorithm/threading notes live in the Metal twin.

    Mapping: grid = numNodes blocks, block = 16 threads (one per delay line).
    blockIdx.x = node, threadIdx.x = line. Thread 0 owns the per-node diffuser
    and output filters. Two __syncthreads() per sample.
*/

static const char* const kFdnProcessKernelSource = R"CUDA(

#define NUM_LINES 16u
#define NUM_DIFFUSERS 4u

struct FdnParams
{
    unsigned int  numNodes;
    unsigned int  blockSize;
    unsigned int  maxDelayLen;
    unsigned int  maxDiffLen;
    unsigned int  maxFbApLen;
    float toneCoeff;
    float lowCoeff;
    float highCoeff;
    float diffusionCoeff;
    float feedbackAPCoeff;
    float dcPole;
    float outputGain;
};

extern "C" __global__ void fdn_process (
    const FdnParams p,
    const float* __restrict__ inputs,         // [numNodes][blockSize]
    float* __restrict__       outputs,        // [numNodes][blockSize]
    const int* __restrict__   delayLengths,   // [numNodes*16]
    const int* __restrict__   diffuserDelays, // [numNodes*4]
    const int* __restrict__   fbApDelays,     // [numNodes*16]
    const float* __restrict__ nodeTapSigns,   // [numNodes*16]
    const float* __restrict__ inputGains,     // [16]
    const float* __restrict__ gainLow,        // [numNodes*16]
    const float* __restrict__ gainMid,        // [numNodes*16]
    const float* __restrict__ gainHigh,       // [numNodes*16]
    float* __restrict__       delayRings,     // [numNodes*16*maxDelayLen]
    int* __restrict__         delayWritePos,  // [numNodes*16]
    float* __restrict__       diffRings,      // [numNodes*4*maxDiffLen]
    int* __restrict__         diffWritePos,   // [numNodes*4]
    float* __restrict__       fbApRings,      // [numNodes*16*maxFbApLen]
    int* __restrict__         fbApWritePos,   // [numNodes*16]
    float* __restrict__       decayLowState,  // [numNodes*16]
    float* __restrict__       decayHighState, // [numNodes*16]
    float* __restrict__       toneState,      // [numNodes]
    float* __restrict__       dcState)        // [numNodes*2]
{
    const unsigned int nodeId = blockIdx.x;
    const unsigned int lineId = threadIdx.x;
    if (nodeId >= p.numNodes || lineId >= NUM_LINES)
        return;

    __shared__ float taps[16];
    __shared__ float diffusedShared;

    const unsigned int nl = nodeId * NUM_LINES + lineId;

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

    float* dRing  = delayRings + (unsigned long long) nl * p.maxDelayLen;
    float* fbRing = fbApRings  + (unsigned long long) nl * p.maxFbApLen;

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

    for (unsigned int s = 0; s < p.blockSize; ++s)
    {
        taps[lineId] = dRing[(unsigned int) dwp];

        if (lineId == 0u)
        {
            float diffused = inputs[nodeId * p.blockSize + s];
            if (p.diffusionCoeff > 0.0001f)
            {
                const float c = p.diffusionCoeff;
                {
                    float* r = diffRings + (unsigned long long) (nodeId * NUM_DIFFUSERS + 0u) * p.maxDiffLen;
                    const int len = diffuserDelays[nodeId * NUM_DIFFUSERS + 0u];
                    float delayed = r[(unsigned int) dfWp0];
                    float v = diffused - c * delayed;
                    r[(unsigned int) dfWp0] = v;
                    if (++dfWp0 >= len) dfWp0 = 0;
                    diffused = delayed + c * v;
                }
                {
                    float* r = diffRings + (unsigned long long) (nodeId * NUM_DIFFUSERS + 1u) * p.maxDiffLen;
                    const int len = diffuserDelays[nodeId * NUM_DIFFUSERS + 1u];
                    float delayed = r[(unsigned int) dfWp1];
                    float v = diffused - c * delayed;
                    r[(unsigned int) dfWp1] = v;
                    if (++dfWp1 >= len) dfWp1 = 0;
                    diffused = delayed + c * v;
                }
                {
                    float* r = diffRings + (unsigned long long) (nodeId * NUM_DIFFUSERS + 2u) * p.maxDiffLen;
                    const int len = diffuserDelays[nodeId * NUM_DIFFUSERS + 2u];
                    float delayed = r[(unsigned int) dfWp2];
                    float v = diffused - c * delayed;
                    r[(unsigned int) dfWp2] = v;
                    if (++dfWp2 >= len) dfWp2 = 0;
                    diffused = delayed + c * v;
                }
                {
                    float* r = diffRings + (unsigned long long) (nodeId * NUM_DIFFUSERS + 3u) * p.maxDiffLen;
                    const int len = diffuserDelays[nodeId * NUM_DIFFUSERS + 3u];
                    float delayed = r[(unsigned int) dfWp3];
                    float v = diffused - c * delayed;
                    r[(unsigned int) dfWp3] = v;
                    if (++dfWp3 >= len) dfWp3 = 0;
                    diffused = delayed + c * v;
                }
            }
            diffusedShared = diffused;
        }

        __syncthreads();

        float hb[16];
        for (unsigned int k = 0; k < NUM_LINES; ++k)
            hb[k] = taps[k];
        for (unsigned int len = 1u; len < NUM_LINES; len <<= 1)
            for (unsigned int i = 0u; i < NUM_LINES; i += len << 1)
                for (unsigned int j = i; j < i + len; ++j)
                {
                    float a = hb[j];
                    float b = hb[j + len];
                    hb[j]       = a + b;
                    hb[j + len] = a - b;
                }
        for (unsigned int k = 0; k < NUM_LINES; ++k)
            hb[k] *= 0.25f;

        if (lineId == 0u)
        {
            float output = 0.0f;
            for (unsigned int k = 0; k < NUM_LINES; ++k)
                output += hb[k] * nodeTapSigns[nodeId * NUM_LINES + k];

            toneSt += p.toneCoeff * (output - toneSt);
            output = toneSt;

            float dcOut = output - dcX1 + p.dcPole * dcY1;
            dcX1 = output;
            dcY1 = dcOut;

            outputs[nodeId * p.blockSize + s] = dcOut * p.outputGain;
        }

        {
            float h = hb[lineId];
            float delayed = fbRing[(unsigned int) fbwp];
            float v = h - apc * delayed;
            fbRing[(unsigned int) fbwp] = v;
            if (++fbwp >= fbLen) fbwp = 0;
            float scattered = delayed + apc * v;

            lowS  += p.lowCoeff  * (scattered - lowS);
            highS += p.highCoeff * (scattered - highS);
            float low  = lowS;
            float high = scattered - highS;
            float mid  = highS - lowS;
            float decayed = low * gL + mid * gM + high * gH;

            float writeVal = decayed + diffusedShared * ig;
            dRing[(unsigned int) dwp] = writeVal;
            if (++dwp >= dLen) dwp = 0;
        }

        __syncthreads();
    }

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
)CUDA";
