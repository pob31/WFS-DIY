//
// Streaming test for MetalWfsBackend (the productized class the app will use):
// drives it like the GpuAsyncPipeline pump would - moving matrices every block
// (50 Hz-style retargeting), latency compensation enabled - and validates the
// output against a CPU reference that models the SAME contract (prev->curr
// ramps over each block, -L pre-subtraction, ring history).
//
// Build:  clang++ -std=c++17 -O2 -fobjc-arc -framework Metal -framework Foundation \
//             backend_test.mm ../../Source/DSP/gpu/MetalWfsBackend.mm -I ../../Source/DSP/gpu -o backend_test
// Run:    ./backend_test
//

#include "MetalWfsBackend.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

int main()
{
    const int numIn = 8, numOut = 16, block = 128;
    const double sr = 48000.0;
    const double latencyMs = 10.6667; // emulate pipeline depth 4 @ 128
    const uint32_t maxDelaySamples = (uint32_t) sr;
    const uint32_t ringCap = maxDelaySamples + block;
    const int matrix = numIn * numOut;
    const float srScale = (float) (sr / 1000.0);

    MetalWfsBackend backend;
    if (! backend.prepare (numIn, numOut, block, sr, latencyMs))
    {
        fprintf (stderr, "prepare failed: %s\n", backend.getLastError().c_str());
        return 1;
    }

    // Live matrices the "app" owns (ms / linear), moving every block.
    std::vector<float> delaysMs (matrix), gains (matrix);
    std::mt19937 rng (7);
    std::uniform_real_distribution<float> dMs (12.0f, 200.0f); // > latencyMs: exact-compensation region
    std::uniform_real_distribution<float> dG (0.0f, 1.0f);
    for (int i = 0; i < matrix; ++i) { delaysMs[i] = dMs (rng); gains[i] = dG (rng); }
    backend.setMatrixPointers (delaysMs.data(), gains.data());

    // CPU mirror of the backend contract
    std::vector<float> cpuRing ((size_t) numIn * ringCap, 0.0f);
    uint32_t cpuPos = 0, cpuValid = 0;
    std::vector<float> dPrevS (matrix, 0.0f), gPrev (matrix, 0.0f);
    bool havePrev = false;

    std::vector<float> inBlock ((size_t) numIn * block), cpuOut ((size_t) numOut * block);
    std::vector<const float*> inPtrs (numIn);
    std::vector<float*> outPtrs (numOut);
    std::vector<float> gpuOut ((size_t) numOut * block);
    for (int ch = 0; ch < numIn; ++ch) inPtrs[ch] = inBlock.data() + (size_t) ch * block;
    for (int ch = 0; ch < numOut; ++ch) outPtrs[ch] = gpuOut.data() + (size_t) ch * block;

    float maxDiff = 0.0f;
    double sumMs = 0.0, maxMs = 0.0;
    const int iters = 300;

    for (int iter = 0; iter < iters; ++iter)
    {
        // test signal
        for (int ch = 0; ch < numIn; ++ch)
            for (int s = 0; s < block; ++s)
                inBlock[(size_t) ch * block + s] =
                    0.25f * std::sin (0.013f * (float) (iter * block + s) * (1.0f + 0.37f * ch));

        // retarget matrices every block (worst-case motion)
        for (int i = 0; i < matrix; ++i)
        {
            delaysMs[i] = std::clamp (delaysMs[i] + (dG (rng) - 0.5f) * 0.4f, 12.0f, 400.0f);
            gains[i] = std::clamp (gains[i] + (dG (rng) - 0.5f) * 0.02f, 0.0f, 1.0f);
        }

        // CPU reference (same contract: curr = (ms - L) * srScale clamped, ramp prev->curr)
        std::vector<float> dCurrS (matrix), gCurr (matrix);
        for (int i = 0; i < matrix; ++i)
        {
            dCurrS[i] = std::clamp ((delaysMs[i] - (float) latencyMs) * srScale, 0.0f, (float) maxDelaySamples);
            gCurr[i] = gains[i];
        }
        if (! havePrev) { dPrevS = dCurrS; gPrev = gCurr; havePrev = true; }

        auto fetch = [&](int in, int off) -> float {
            if (off >= 0) return inBlock[(size_t) in * block + off];
            if ((uint32_t) (-off) > cpuValid) return 0.0f;
            int idx = (int) cpuPos + off;
            if (idx < 0) idx += (int) ringCap;
            return cpuRing[(size_t) in * ringCap + idx];
        };
        const float invLen = 1.0f / (float) block;
        for (int out = 0; out < numOut; ++out)
            for (int s = 0; s < block; ++s)
            {
                const float t = (float) (s + 1) * invLen;
                float acc = 0.0f;
                for (int in = 0; in < numIn; ++in)
                {
                    const int mIdx = in * numOut + out;
                    const float gp = gPrev[mIdx], gc = gCurr[mIdx];
                    if (gp == 0.0f && gc == 0.0f) continue;
                    const float gain = gp + (gc - gp) * t;
                    float d = dPrevS[mIdx] + (dCurrS[mIdx] - dPrevS[mIdx]) * t;
                    d = std::max (d, 0.0f);
                    const uint32_t di = (uint32_t) d;
                    const float frac = d - (float) di;
                    const int off0 = s - (int) di;
                    acc += gain * (fetch (in, off0) * (1.0f - frac) + fetch (in, off0 - 1) * frac);
                }
                cpuOut[(size_t) out * block + s] = acc;
            }
        for (int in = 0; in < numIn; ++in)
            for (int s = 0; s < block; ++s)
                cpuRing[(size_t) in * ringCap + (cpuPos + s) % ringCap] = inBlock[(size_t) in * block + s];
        cpuPos = (cpuPos + block) % ringCap;
        cpuValid = std::min (maxDelaySamples, cpuValid + block);
        dPrevS = dCurrS;
        gPrev = gCurr;

        // backend
        if (! backend.processBlock (inPtrs.data(), outPtrs.data()))
        {
            fprintf (stderr, "processBlock failed: %s\n", backend.getLastError().c_str());
            return 1;
        }
        sumMs += backend.getLastLaunchMs();
        maxMs = std::max (maxMs, backend.getLastLaunchMs());

        for (size_t i = 0; i < gpuOut.size(); ++i)
            maxDiff = std::max (maxDiff, std::fabs (gpuOut[i] - cpuOut[i]));
    }

    printf ("MetalWfsBackend streaming test: %d blocks %dx%d @ %d, moving matrices, latencyComp %.2f ms\n",
            iters, numIn, numOut, block, latencyMs);
    printf ("  maxDiff vs CPU reference: %.3e  -> %s\n", maxDiff, maxDiff < 1e-3f ? "PASS" : "FAIL");
    printf ("  launch ms: mean %.3f  max %.3f\n", sumMs / iters, maxMs);
    return maxDiff < 1e-3f ? 0 : 2;
}
