/*
    Standalone validation harness for the CUDA WFS kernels (wfs_pairs +
    wfs_reduce in Source/DSP/gpu/CudaWfsKernels.h).

    NVRTC-compiles the exact kernel string the app ships and runs targeted
    scenarios against a tiny CPU reference model:

      A. Pure delay+gain ramps (shelves at 0 dB = exact identity, no FR):
         multi-launch with cross-block history reads and prev->curr ramps,
         compared sample-exactly (1e-5) to the CPU model.
      B. Floor Reflection anatomy: impulse -> direct spike at D, FR spike at
         D+E with the FR gain (integral delays, 0 dB shelves).
      C. Shelf sanity at -12 dB: DC passes at ~unity (high shelf leaves lows),
         Nyquist-rate alternation attenuated by ~12 dB.
      D. Determinism: identical run twice -> bit-identical output.

    Build (from this directory; nvcc drives the host compile and links cudart):
      nvcc -o test_wfs_kernels.exe test_wfs_kernels.cpp -lnvrtc -lcuda
    Run:
      test_wfs_kernels.exe
*/

#include "../../spatcore/gpu/CudaWfsKernels.h"

#include <cuda.h>
#include <cuda_runtime.h>
#include <nvrtc.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#define CHECK_CU(call) do { CUresult e = (call); if (e != CUDA_SUCCESS) { \
    const char* s = nullptr; cuGetErrorString (e, &s); \
    std::fprintf (stderr, "CUDA driver error %s at %s:%d\n", s ? s : "?", __FILE__, __LINE__); std::exit (1); } } while (0)
#define CHECK_RT(call) do { cudaError_t e = (call); if (e != cudaSuccess) { \
    std::fprintf (stderr, "CUDA runtime error %s at %s:%d\n", cudaGetErrorString (e), __FILE__, __LINE__); std::exit (1); } } while (0)

struct WfsParamsGpu
{
    uint32_t numInputs;
    uint32_t numOutputs;
    uint32_t bufferLength;
    uint32_t ringCapacity;
    uint32_t ringWritePos;
    uint32_t ringValidSamples;
    uint32_t pairGroups;
    float    shelfCosW0;
    float    shelfSinW0;
};

// ---- Minimal GPU runner mirroring CudaWfsBackend's launch logic ----
struct Runner
{
    int numIn, numOut, len;
    uint32_t ringCap, pos = 0, valid = 0, maxDelay;
    uint32_t pairGroups;
    float cosW0, sinW0;
    CUfunction kPairs, kReduce;
    cudaStream_t stream;

    float *dIn, *dFrIn, *dOut, *dRing, *dFrRing, *dScratch, *dShelf, *dFrShelf;
    float *dDp, *dDc, *dGp, *dGc, *dFdp, *dFdc, *dFgp, *dFgc, *dHf, *dFrHf;

    void init (int nIn, int nOut, int blockLen, double sr, uint32_t maxDelaySamples,
               CUfunction pairs, CUfunction reduce)
    {
        numIn = nIn; numOut = nOut; len = blockLen;
        maxDelay = maxDelaySamples;
        ringCap = maxDelay + (uint32_t) len;
        pairGroups = (uint32_t) ((nIn * nOut + 255) / 256);
        const double w0 = 2.0 * 3.14159265358979 * 800.0 / sr;
        cosW0 = (float) std::cos (w0);
        sinW0 = (float) std::sin (w0);
        kPairs = pairs; kReduce = reduce;
        CHECK_RT (cudaStreamCreate (&stream));

        const size_t mat = (size_t) nIn * nOut;
        auto dev = [] (float** p, size_t n) { CHECK_RT (cudaMalloc ((void**) p, n * sizeof (float))); };
        dev (&dIn, (size_t) nIn * len);   dev (&dFrIn, (size_t) nIn * len);
        dev (&dOut, (size_t) nOut * len);
        dev (&dRing, (size_t) nIn * ringCap); dev (&dFrRing, (size_t) nIn * ringCap);
        dev (&dScratch, (size_t) nIn * nOut * len);
        dev (&dShelf, mat * 4); dev (&dFrShelf, mat * 4);
        dev (&dDp, mat); dev (&dDc, mat); dev (&dGp, mat); dev (&dGc, mat);
        dev (&dFdp, mat); dev (&dFdc, mat); dev (&dFgp, mat); dev (&dFgc, mat);
        dev (&dHf, mat); dev (&dFrHf, mat);
        resetState();
    }

    void resetState()
    {
        const size_t mat = (size_t) numIn * numOut;
        CHECK_RT (cudaMemset (dRing, 0, (size_t) numIn * ringCap * sizeof (float)));
        CHECK_RT (cudaMemset (dFrRing, 0, (size_t) numIn * ringCap * sizeof (float)));
        CHECK_RT (cudaMemset (dShelf, 0, mat * 4 * sizeof (float)));
        CHECK_RT (cudaMemset (dFrShelf, 0, mat * 4 * sizeof (float)));
        pos = 0; valid = 0;
    }

    // One launch. All matrices in SAMPLES / linear, already latency-folded.
    void launch (const std::vector<float>& in, const std::vector<float>& frIn,
                 const std::vector<float>& dp, const std::vector<float>& dc,
                 const std::vector<float>& gp, const std::vector<float>& gc,
                 const std::vector<float>& fdp, const std::vector<float>& fdc,
                 const std::vector<float>& fgp, const std::vector<float>& fgc,
                 const std::vector<float>& hf, const std::vector<float>& frHf,
                 std::vector<float>& out)
    {
        const size_t mat = (size_t) numIn * numOut;
        auto up = [this] (float* d, const float* h, size_t n) {
            CHECK_RT (cudaMemcpyAsync (d, h, n * sizeof (float), cudaMemcpyHostToDevice, stream));
        };
        up (dIn, in.data(), (size_t) numIn * len);
        up (dFrIn, frIn.data(), (size_t) numIn * len);
        up (dDp, dp.data(), mat); up (dDc, dc.data(), mat);
        up (dGp, gp.data(), mat); up (dGc, gc.data(), mat);
        up (dFdp, fdp.data(), mat); up (dFdc, fdc.data(), mat);
        up (dFgp, fgp.data(), mat); up (dFgc, fgc.data(), mat);
        up (dHf, hf.data(), mat); up (dFrHf, frHf.data(), mat);

        WfsParamsGpu p { (uint32_t) numIn, (uint32_t) numOut, (uint32_t) len,
                         ringCap, pos, valid, pairGroups, cosW0, sinW0 };
        void* a1[] = { &p, &dIn, &dFrIn, &dRing, &dFrRing,
                       &dDp, &dDc, &dGp, &dGc, &dFdp, &dFdc, &dFgp, &dFgc,
                       &dHf, &dFrHf, &dShelf, &dFrShelf, &dScratch };
        CHECK_CU (cuLaunchKernel (kPairs, pairGroups + 2u * (unsigned) numIn, 1, 1,
                                  256, 1, 1, 0, (CUstream) stream, a1, nullptr));
        void* a2[] = { &p, &dScratch, &dOut };
        CHECK_CU (cuLaunchKernel (kReduce, (unsigned) numOut, 1, 1,
                                  256, 1, 1, 0, (CUstream) stream, a2, nullptr));

        out.resize ((size_t) numOut * len);
        CHECK_RT (cudaMemcpyAsync (out.data(), dOut, out.size() * sizeof (float),
                                   cudaMemcpyDeviceToHost, stream));
        CHECK_RT (cudaStreamSynchronize (stream));

        pos = (pos + (uint32_t) len) % ringCap;
        valid = std::min (maxDelay, valid + (uint32_t) len);
    }
};

// ---- CPU reference for scenario A (shelves at 0 dB = identity, no FR) ----
struct CpuRef
{
    int numIn, numOut, len;
    uint32_t ringCap, pos = 0, valid = 0, maxDelay;
    std::vector<float> ring; // [in][ringCap]

    void init (int nIn, int nOut, int blockLen, uint32_t maxDelaySamples)
    {
        numIn = nIn; numOut = nOut; len = blockLen;
        maxDelay = maxDelaySamples;
        ringCap = maxDelay + (uint32_t) len;
        ring.assign ((size_t) nIn * ringCap, 0.0f);
        pos = 0; valid = 0;
    }

    float fetch (const std::vector<float>& in, int inIdx, int off) const
    {
        if (off >= 0) return in[(size_t) inIdx * len + (size_t) off];
        if ((uint32_t) (-off) > valid) return 0.0f;
        int idx = (int) pos + off;
        if (idx < 0) idx += (int) ringCap;
        return ring[(size_t) inIdx * ringCap + (size_t) idx];
    }

    void launch (const std::vector<float>& in,
                 const std::vector<float>& dp, const std::vector<float>& dc,
                 const std::vector<float>& gp, const std::vector<float>& gc,
                 std::vector<float>& out)
    {
        out.assign ((size_t) numOut * len, 0.0f);
        const float invLen = 1.0f / (float) len;
        for (int o = 0; o < numOut; ++o)
            for (int s = 0; s < len; ++s)
            {
                const float t = (float) (s + 1) * invLen;
                float acc = 0.0f;
                for (int i = 0; i < numIn; ++i)
                {
                    const size_t m = (size_t) i * numOut + o;
                    const float g = gp[m] + (gc[m] - gp[m]) * t;
                    if (gp[m] == 0.0f && gc[m] == 0.0f) continue;
                    float d = dp[m] + (dc[m] - dp[m]) * t;
                    if (d < 0.0f) d = 0.0f;
                    const uint32_t di = (uint32_t) d;
                    const float fr = d - (float) di;
                    const int off = s - (int) di;
                    const float s0 = fetch (in, i, off);
                    const float s1 = fetch (in, i, off - 1);
                    acc += g * (s0 * (1.0f - fr) + s1 * fr);
                }
                out[(size_t) o * len + s] = acc;
            }

        for (int i = 0; i < numIn; ++i)
            for (int s = 0; s < len; ++s)
                ring[(size_t) i * ringCap + ((pos + s) % ringCap)] = in[(size_t) i * len + s];
        pos = (pos + (uint32_t) len) % ringCap;
        valid = std::min (maxDelay, valid + (uint32_t) len);
    }
};

static int failures = 0;
static void expect (bool cond, const char* what)
{
    std::printf ("  [%s] %s\n", cond ? "PASS" : "FAIL", what);
    if (! cond) ++failures;
}

int main()
{
    // ---- NVRTC compile of the SHIPPED kernel string ----
    CHECK_RT (cudaSetDevice (0));
    cudaDeviceProp prop {};
    CHECK_RT (cudaGetDeviceProperties (&prop, 0));
    std::printf ("Device: %s (CC %d.%d)\n", prop.name, prop.major, prop.minor);

    CHECK_CU (cuInit (0));
    CUdevice dev; CHECK_CU (cuDeviceGet (&dev, 0));
    CUcontext ctx; CHECK_CU (cuDevicePrimaryCtxRetain (&ctx, dev));
    CHECK_CU (cuCtxSetCurrent (ctx));

    nvrtcProgram prog = nullptr;
    if (nvrtcCreateProgram (&prog, kWfsDelaySumKernelSource, "wfs.cu", 0, nullptr, nullptr) != NVRTC_SUCCESS)
    { std::fprintf (stderr, "nvrtcCreateProgram failed\n"); return 1; }
    const std::string arch = "--gpu-architecture=compute_" + std::to_string (prop.major * 10 + prop.minor);
    const char* opts[] = { arch.c_str() };
    if (nvrtcCompileProgram (prog, 1, opts) != NVRTC_SUCCESS)
    {
        size_t n = 0; nvrtcGetProgramLogSize (prog, &n);
        std::string log (n, '\0'); nvrtcGetProgramLog (prog, &log[0]);
        std::fprintf (stderr, "NVRTC compile failed:\n%s\n", log.c_str());
        return 1;
    }
    std::printf ("NVRTC compile of shipped kernel string: OK\n");
    size_t ptxN = 0; nvrtcGetPTXSize (prog, &ptxN);
    std::vector<char> ptx (ptxN); nvrtcGetPTX (prog, ptx.data());
    nvrtcDestroyProgram (&prog);

    CUmodule mod; CHECK_CU (cuModuleLoadDataEx (&mod, ptx.data(), 0, nullptr, nullptr));
    CUfunction kPairs, kReduce;
    CHECK_CU (cuModuleGetFunction (&kPairs, mod, "wfs_pairs"));
    CHECK_CU (cuModuleGetFunction (&kReduce, mod, "wfs_reduce"));

    const int numIn = 2, numOut = 3, len = 128;
    const double sr = 48000.0;
    const uint32_t maxDelay = 2048;
    const size_t mat = (size_t) numIn * numOut;

    Runner gpu; gpu.init (numIn, numOut, len, sr, maxDelay, kPairs, kReduce);

    // =====================================================================
    std::printf ("\nScenario A: ramped delay+gain vs CPU reference (0 dB shelves, no FR)\n");
    {
        CpuRef cpu; cpu.init (numIn, numOut, len, maxDelay);

        std::vector<float> zerosM (mat, 0.0f);
        std::vector<float> frIn ((size_t) numIn * len, 0.0f);

        // Per-launch matrices: delays move 200 -> 240 samples, gains 1.0 -> 0.7
        std::vector<float> dPrev (mat), dCurr (mat), gPrev (mat), gCurr (mat);
        std::srand (1234);
        float maxDiff = 0.0f;
        for (int launchIdx = 0; launchIdx < 6; ++launchIdx)
        {
            std::vector<float> in ((size_t) numIn * len);
            for (auto& v : in) v = (float) std::rand() / RAND_MAX - 0.5f;
            for (int i = 0; i < numIn; ++i)
                for (int s = 0; s < len; ++s)
                    frIn[(size_t) i * len + s] = 0.0f; // FR silent

            for (size_t m = 0; m < mat; ++m)
            {
                dPrev[m] = 200.0f + 8.0f * launchIdx + 3.0f * (float) m;
                dCurr[m] = dPrev[m] + 8.0f;          // ramping delay
                gPrev[m] = 1.0f - 0.05f * launchIdx;
                gCurr[m] = gPrev[m] - 0.05f;         // ramping gain
            }

            std::vector<float> outG, outC;
            gpu.launch (in, frIn, dPrev, dCurr, gPrev, gCurr,
                        zerosM, zerosM, zerosM, zerosM, zerosM, zerosM, outG);
            cpu.launch (in, dPrev, dCurr, gPrev, gCurr, outC);

            for (size_t k = 0; k < outG.size(); ++k)
                maxDiff = std::max (maxDiff, std::fabs (outG[k] - outC[k]));
        }
        std::printf ("  max |gpu - cpu| over 6 launches = %.3e\n", maxDiff);
        expect (maxDiff < 1e-5f, "matches CPU reference within 1e-5 (incl. cross-launch history)");
    }

    // =====================================================================
    std::printf ("\nScenario B: Floor Reflection anatomy (impulse, integral delays)\n");
    {
        gpu.resetState();
        std::vector<float> zerosM (mat, 0.0f);

        const float D = 32.0f, E = 24.0f, gDir = 0.8f, gFr = 0.5f;
        std::vector<float> dM (mat, D), gM (mat, gDir);
        std::vector<float> fdM (mat, D + E), fgM (mat, gFr);

        std::vector<float> in ((size_t) numIn * len, 0.0f);
        in[0] = 1.0f;            // impulse on input 0, sample 0
        std::vector<float> frIn = in;   // FR pre-filter chain inactive = passthrough

        std::vector<float> out;
        gpu.launch (in, frIn, dM, dM, gM, gM, fdM, fdM, fgM, fgM, zerosM, zerosM, out);

        // Output 0: direct spike at s=D with gDir, FR spike at s=D+E with gFr.
        const float* o0 = out.data();
        bool spikes = std::fabs (o0[(int) D] - gDir) < 1e-5f
                   && std::fabs (o0[(int) (D + E)] - gFr) < 1e-5f;
        expect (spikes, "direct spike at D and FR spike at D+E with the right gains");

        float stray = 0.0f;
        for (int s = 0; s < len; ++s)
            if (s != (int) D && s != (int) (D + E))
                stray = std::max (stray, std::fabs (o0[s]));
        expect (stray < 1e-6f, "no stray energy at other samples");
    }

    // =====================================================================
    std::printf ("\nScenario C: -12 dB shelf sanity (DC ~unity, Nyquist ~-12 dB)\n");
    {
        gpu.resetState();
        std::vector<float> zerosM (mat, 0.0f);
        std::vector<float> dM (mat, 0.0f), gM (mat, 1.0f), hfM (mat, -12.0f);
        std::vector<float> frIn ((size_t) numIn * len, 0.0f);

        // DC: settle over several launches, inspect the last block's tail.
        std::vector<float> in ((size_t) numIn * len, 1.0f), out;
        for (int l = 0; l < 8; ++l)
            gpu.launch (in, frIn, dM, dM, gM, gM, zerosM, zerosM, zerosM, zerosM, hfM, zerosM, out);
        float dcTail = out[(size_t) 0 * len + (len - 1)] / (float) numIn; // both inputs sum
        std::printf ("  DC tail per input = %.4f (expect ~1.0)\n", dcTail);
        expect (std::fabs (dcTail - 1.0f) < 0.02f, "DC passes at ~unity through -12 dB high shelf");

        // Nyquist: +-1 alternation.
        gpu.resetState();
        for (int i = 0; i < numIn; ++i)
            for (int s = 0; s < len; ++s)
                in[(size_t) i * len + s] = (s % 2 == 0) ? 1.0f : -1.0f;
        for (int l = 0; l < 8; ++l)
            gpu.launch (in, frIn, dM, dM, gM, gM, zerosM, zerosM, zerosM, zerosM, hfM, zerosM, out);
        float nyqAmp = std::fabs (out[(size_t) 0 * len + (len - 1)]) / (float) numIn;
        const float expected = std::pow (10.0f, -12.0f / 20.0f); // ~0.251
        std::printf ("  Nyquist tail amplitude per input = %.4f (expect ~%.4f)\n", nyqAmp, expected);
        expect (std::fabs (nyqAmp - expected) < 0.03f, "Nyquist attenuated by ~12 dB");
    }

    // =====================================================================
    std::printf ("\nScenario D: determinism (two identical runs)\n");
    {
        std::vector<float> zerosM (mat, 0.0f);
        std::vector<float> dM (mat, 100.0f), gM (mat, 0.9f), hfM (mat, -6.0f);
        std::vector<float> fdM (mat, 140.0f), fgM (mat, 0.4f), frHfM (mat, -9.0f);

        std::vector<float> outA, outB;
        for (int run = 0; run < 2; ++run)
        {
            gpu.resetState();
            std::srand (777);
            std::vector<float>& out = (run == 0) ? outA : outB;
            for (int l = 0; l < 5; ++l)
            {
                std::vector<float> in ((size_t) numIn * len);
                for (auto& v : in) v = (float) std::rand() / RAND_MAX - 0.5f;
                gpu.launch (in, in, dM, dM, gM, gM, fdM, fdM, fgM, fgM, hfM, frHfM, out);
            }
        }
        expect (std::memcmp (outA.data(), outB.data(), outA.size() * sizeof (float)) == 0,
                "bit-identical output across runs");
    }

    std::printf ("\n%s (%d failure%s)\n", failures == 0 ? "ALL PASS" : "FAILURES",
                 failures, failures == 1 ? "" : "s");
    cuModuleUnload (mod);
    cuDevicePrimaryCtxRelease (dev);
    return failures == 0 ? 0 : 1;
}
