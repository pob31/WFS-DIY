#pragma once

/*
    CUDA kernel source for the GPU IR-reverb backend (NVRTC-compiled at
    prepare() time by CudaIrBackend, like the WFS kernels — no .cu build
    step, no nvcc).

    KEEP IN SYNC with MetalIrKernels.h — the two strings implement the
    byte-identical algorithm; only the language scaffolding differs.
    Algorithm notes (FDL MAC, packed-spectrum format) live in the Metal twin.

    Dispatch: grid = numNodes blocks, block = bins threads (bins = engine
    block size P, <= 256). One thread owns one (node, bin): deterministic
    serial loop over segments, no atomics.
*/

static const char* const kIrFdlMacKernelSource = R"CUDA(

struct IrParams
{
    unsigned int numNodes;
    unsigned int bins;            // packed float2 bins per spectrum = block size P
    unsigned int segCapacity;     // ring slots per node = max IR segments
    unsigned int segmentsLoaded;  // IR segments transformed so far (<= segCapacity)
    unsigned int ringHead;        // slot holding the NEWEST input spectrum
};

extern "C" __global__ void ir_fdl_mac (const IrParams p,
                                       const float2* __restrict__ irSpectra,   // [seg][bins]
                                       const float2* __restrict__ inSpectra,   // [node][segCapacity][bins]
                                       float2* __restrict__ outSpectra)        // [node][bins]
{
    const unsigned int node = blockIdx.x;
    const unsigned int bin = threadIdx.x;
    if (node >= p.numNodes || bin >= p.bins)
        return;

    const float2* nodeRing = inSpectra
        + (unsigned long long) node * p.segCapacity * p.bins;

    float2 acc = make_float2 (0.0f, 0.0f);
    unsigned int slot = p.ringHead;
    for (unsigned int s = 0; s < p.segmentsLoaded; ++s)
    {
        const float2 a = nodeRing[(unsigned long long) slot * p.bins + bin];
        const float2 b = irSpectra[(unsigned long long) s * p.bins + bin];
        if (bin == 0u)
        {
            acc.x += a.x * b.x;                              // (DC*DC, Ny*Ny)
            acc.y += a.y * b.y;
        }
        else
        {
            acc.x += a.x * b.x - a.y * b.y;                  // complex MAC
            acc.y += a.x * b.y + a.y * b.x;
        }
        slot = (slot == 0u) ? p.segCapacity - 1u : slot - 1u;
    }

    outSpectra[(unsigned long long) node * p.bins + bin] = acc;
}
)CUDA";
