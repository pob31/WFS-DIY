#pragma once

/*
    Metal kernel source for the GPU IR-reverb backend (MSL, compiled at
    prepare() time by MetalIrBackend like the WFS kernels).

    KEEP IN SYNC with CudaIrKernels.h — the two strings implement the
    byte-identical algorithm; only the language scaffolding differs.

    ir_fdl_mac — frequency-domain delay-line multiply-accumulate, the only
    GPU stage of the hybrid convolution (host does the FFTs, see IrHostFft.h):

        out[node][bin] = sum over s < segmentsLoaded of
                         in[node][(ringHead - s) mod segCapacity][bin] * ir[s][bin]

    Spectra are packed (IrHostFft format): bin 0 carries (DC, Nyquist) — its
    "product" is element-wise — all other bins are ordinary complex numbers.

    Dispatch: numNodes threadgroups x bins threads (bins = engine block size,
    <= 256). One thread owns one (node, bin): a deterministic serial loop over
    segments, no atomics, bit-exact run-to-run.
*/

static const char* const kIrFdlMacKernelSource = R"MSL(
#include <metal_stdlib>
using namespace metal;

struct IrParams
{
    uint numNodes;
    uint bins;            // packed float2 bins per spectrum = block size P
    uint segCapacity;     // ring slots per node = max IR segments
    uint segmentsLoaded;  // IR segments transformed so far (<= segCapacity)
    uint ringHead;        // slot holding the NEWEST input spectrum
};

kernel void ir_fdl_mac (constant IrParams&    p          [[buffer(0)]],
                        const device float2*  irSpectra  [[buffer(1)]],  // [seg][bins]
                        const device float2*  inSpectra  [[buffer(2)]],  // [node][segCapacity][bins]
                        device float2*        outSpectra [[buffer(3)]],  // [node][bins]
                        uint node [[threadgroup_position_in_grid]],
                        uint bin  [[thread_position_in_threadgroup]])
{
    if (node >= p.numNodes || bin >= p.bins)
        return;

    const device float2* nodeRing = inSpectra + (ulong) node * p.segCapacity * p.bins;

    float2 acc = float2 (0.0f, 0.0f);
    uint slot = p.ringHead;
    for (uint s = 0; s < p.segmentsLoaded; ++s)
    {
        const float2 a = nodeRing[(ulong) slot * p.bins + bin];
        const float2 b = irSpectra[(ulong) s * p.bins + bin];
        if (bin == 0u)
            acc += float2 (a.x * b.x, a.y * b.y);            // (DC*DC, Ny*Ny)
        else
            acc += float2 (a.x * b.x - a.y * b.y,
                           a.x * b.y + a.y * b.x);           // complex MAC
        slot = (slot == 0u) ? p.segCapacity - 1u : slot - 1u;
    }

    outSpectra[(ulong) node * p.bins + bin] = acc;
}
)MSL";
