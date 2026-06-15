// test-gpu-plugin.cpp — headless smoke test for a GPU vendor plugin.
//
// Loads a wfs_<vendor> plugin (wfs_cuda.dll / wfs_hip.dll / libwfs_cuda.so /
// libwfs_hip.so), creates the WFS delay-and-sum backend through the shared
// IWfsBackend interface, and drives it directly (no audio device, no pipeline)
// to prove the full GPU compute path on the real hardware: plugin load ->
// backend create -> prepare (device init + runtime kernel compile, NVRTC/hipRTC)
// -> setMatrixPointers -> processBlock -> sane output.
//
// Build (Windows, from a VS dev prompt):
//   cl /nologo /EHsc /std:c++17 /DWFS_GPU_NATIVE=1 /I..\Source\DSP\gpu test-gpu-plugin.cpp /Fe:test-gpu-plugin.exe
// Build (Linux):
//   g++ -std=c++17 -DWFS_GPU_NATIVE=1 -I../Source/DSP/gpu test-gpu-plugin.cpp -ldl -o test-gpu-plugin
//
// Usage: test-gpu-plugin <plugin-path>
// Exit code 0 = backend created, prepared, and produced finite, non-silent,
// approximately-correct output; non-zero = failure (reason printed).

#include "GpuBackendInterface.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#if defined(_WIN32)
 #define WIN32_LEAN_AND_MEAN
 #define NOMINMAX
 #include <windows.h>
 using Lib = HMODULE;
 static Lib openLib (const char* p)            { return LoadLibraryA (p); }
 static void* sym (Lib h, const char* s)       { return (void*) GetProcAddress (h, s); }
#else
 #include <dlfcn.h>
 using Lib = void*;
 static Lib openLib (const char* p)            { return dlopen (p, RTLD_NOW | RTLD_LOCAL); }
 static void* sym (Lib h, const char* s)       { return dlsym (h, s); }
#endif

using CreateWfsFn = IWfsBackend* (*) (int);
using VendorFn    = const char*  (*) ();
using DestroyFn   = void         (*) (IGpuBackend*);

int main (int argc, char** argv)
{
    if (argc < 2) { std::printf ("usage: %s <plugin-path>\n", argv[0]); return 2; }
    const char* path = argv[1];

    Lib lib = openLib (path);
    if (lib == nullptr) { std::printf ("FAIL: could not load '%s'\n", path); return 3; }

    auto vendor  = (VendorFn)    sym (lib, "wfs_plugin_vendor");
    auto create  = (CreateWfsFn) sym (lib, "wfs_plugin_create_wfs");
    auto destroy = (DestroyFn)   sym (lib, "wfs_plugin_destroy");
    if (create == nullptr) { std::printf ("FAIL: wfs_plugin_create_wfs not exported\n"); return 4; }

    std::printf ("loaded: %s   vendor: %s\n", path, vendor ? vendor() : "(no wfs_plugin_vendor)");

    IWfsBackend* b = create (0);
    if (b == nullptr) { std::printf ("FAIL: create_wfs returned null (no device / init failed)\n"); return 5; }

    // 1 input -> 1 output, gain 1.0, delay 0, no HF/FR. latency 0 so output is
    // in-block. maxDelay 1 s. prepare() does device init + runtime kernel compile.
    const int nIn = 1, nOut = 1, block = 64;
    const double sr = 48000.0;
    if (! b->prepare (nIn, nOut, block, sr, /*latencyMs*/ 0.0, /*maxDelaySeconds*/ 1.0))
    {
        std::printf ("FAIL: prepare() returned false: %s\n", b->getLastError().c_str());
        destroy ? destroy (b) : (void) (delete b);
        return 6;
    }
    std::printf ("device: %s\n", b->getDeviceName().c_str());

    static float delays[nIn * nOut] = { 0.0f };
    static float gains [nIn * nOut] = { 1.0f };
    b->setMatrixPointers (delays, gains, nullptr, nullptr, nullptr, nullptr);

    // Constant input; after the per-sample ramp settles, a 1.0-gain / 0-delay
    // 1x1 path should yield output ~= input. Push several blocks, watch it settle.
    const float kIn = 0.5f;
    std::vector<float> in (block, kIn), out (block, 0.0f);
    const float* inPtrs[1]  = { in.data() };
    float*       outPtrs[1] = { out.data() };

    float firstEnergy = 0.0f, lastMean = 0.0f, lastPeak = 0.0f, lastLaunchMs = 0.0f;
    const int kBlocks = 16;
    for (int blk = 0; blk < kBlocks; ++blk)
    {
        std::fill (out.begin(), out.end(), 0.0f);
        if (! b->processBlock (inPtrs, outPtrs))
        {
            std::printf ("FAIL: processBlock() returned false on block %d: %s\n", blk, b->getLastError().c_str());
            destroy ? destroy (b) : (void) (delete b);
            return 7;
        }
        double energy = 0.0, sum = 0.0; float peak = 0.0f;
        for (float v : out)
        {
            if (! std::isfinite (v)) { std::printf ("FAIL: non-finite output (%g) on block %d\n", v, blk); destroy ? destroy (b) : (void) (delete b); return 8; }
            energy += (double) v * v; sum += v; peak = std::max (peak, std::fabs (v));
        }
        if (blk == 0) firstEnergy = (float) energy;
        lastMean = (float) (sum / block); lastPeak = peak;
        lastLaunchMs = (float) b->getLastLaunchMs();
    }

    std::printf ("blocks=%d  firstBlockEnergy=%.4f  lastBlockMean=%.4f (expect ~%.2f)  lastBlockPeak=%.4f  lastLaunchMs=%.3f\n",
                 kBlocks, firstEnergy, lastMean, kIn, lastPeak, lastLaunchMs);

    bool ok = (lastPeak > 1e-4f) && std::fabs (lastMean - kIn) < 0.05f;
    std::printf ("%s: GPU compute %s\n", ok ? "PASS" : "FAIL",
                 ok ? "produced finite, non-silent, ~correct output" : "output silent or wrong");

    destroy ? destroy (b) : (void) (delete b);
    return ok ? 0 : 9;
}
