// test-gpu-plugin.cpp — headless smoke test for a GPU vendor plugin.
//
// Loads a wfs_<vendor> plugin (wfs_cuda.dll / wfs_hip.dll / libwfs_cuda.so /
// libwfs_hip.so), creates the WFS delay-and-sum backend through the shared
// IWfsBackend interface, and drives it directly (no audio device, no pipeline)
// to prove the full GPU compute path on the real hardware: plugin load ->
// backend create -> prepare (device init + runtime kernel compile, NVRTC/hipRTC)
// -> setMatrixPointers -> processBlock -> sane output.
//
// Two scenarios run on the real device:
//   A) 1x1, gain 1, delay 0      — output settles to the input (basic path).
//   B) 2 inputs -> 1 output,     — output = g0*in0 + g1*in1, which exercises the
//      distinct gains [1.0,0.5]    multi-input reduce sum (wfs_reduce streams
//                                   numInputs); a 1x1 passthrough would FAIL it.
// Both use constant inputs and check the LAST block's steady-state mean, so the
// per-sample prev->curr ramp has settled. (The delay-ring / fractional-delay
// path is covered by the Phase-1 bit-exact impulse validation, not here.)
//
// Build (Windows, from a VS dev prompt):
//   cl /nologo /EHsc /std:c++17 /DWFS_GPU_NATIVE=1 /I..\Source\DSP\gpu test-gpu-plugin.cpp /Fe:test-gpu-plugin.exe
// Build (Linux):
//   g++ -std=c++17 -DWFS_GPU_NATIVE=1 -I../Source/DSP/gpu test-gpu-plugin.cpp -ldl -o test-gpu-plugin
//
// Usage: test-gpu-plugin <plugin-path>
// Exit code 0 = backend created, prepared, and produced finite, non-silent,
// approximately-correct output for BOTH scenarios; non-zero = failure (reason printed).

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

// Drive a freshly-prepared backend with per-input CONSTANT signals for kBlocks
// blocks and report the LAST block's per-output mean and the global peak. The
// output buffers are zeroed before every processBlock, so a call that returns
// true but writes nothing leaves peak==0 (caught by the caller). Returns 0 on
// success, <0 on processBlock failure / non-finite output (err set).
static int driveConstant (IWfsBackend* b, int nIn, int nOut, int block,
                          const std::vector<float>& inConst,   // size nIn
                          const float* delays, const float* gains,
                          std::vector<float>& outMeanPerOut, float& peak,
                          float& lastLaunchMs, std::string& err)
{
    b->setMatrixPointers (delays, gains, nullptr, nullptr, nullptr, nullptr);

    std::vector<std::vector<float>> in (nIn,  std::vector<float> (block));
    std::vector<std::vector<float>> out (nOut, std::vector<float> (block, 0.0f));
    for (int i = 0; i < nIn; ++i) std::fill (in[i].begin(), in[i].end(), inConst[(size_t) i]);

    std::vector<const float*> inP  (nIn);
    std::vector<float*>       outP (nOut);
    for (int i = 0; i < nIn;  ++i) inP[i]  = in[i].data();

    outMeanPerOut.assign (nOut, 0.0f);
    peak = 0.0f;

    const int kBlocks = 16;
    for (int blk = 0; blk < kBlocks; ++blk)
    {
        for (int o = 0; o < nOut; ++o) { std::fill (out[o].begin(), out[o].end(), 0.0f); outP[o] = out[o].data(); }

        if (! b->processBlock (inP.data(), outP.data()))
        {
            err = "processBlock() returned false on block " + std::to_string (blk) + ": " + b->getLastError();
            return -1;
        }

        for (int o = 0; o < nOut; ++o)
        {
            double sum = 0.0;
            for (float v : out[o])
            {
                if (! std::isfinite (v)) { err = "non-finite output (" + std::to_string (v) + ") on block " + std::to_string (blk); return -2; }
                sum += v; peak = std::max (peak, std::fabs (v));
            }
            outMeanPerOut[(size_t) o] = (float) (sum / block);
        }
        lastLaunchMs = (float) b->getLastLaunchMs();
    }
    return 0;
}

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
    auto kill = [destroy] (IGpuBackend* p) { if (destroy) destroy (p); else delete p; };

    std::printf ("loaded: %s   vendor: %s\n", path, vendor ? vendor() : "(no wfs_plugin_vendor)");

    const int block = 64;
    const double sr = 48000.0;
    std::string err;

    // ---- Scenario A: 1x1, gain 1, delay 0 -> output settles to input ----
    {
        IWfsBackend* b = create (0);
        if (b == nullptr) { std::printf ("FAIL: create_wfs returned null (no device / init failed)\n"); return 5; }

        if (! b->prepare (1, 1, block, sr, /*latencyMs*/ 0.0, /*maxDelaySeconds*/ 1.0))
        { std::printf ("FAIL: [A] prepare() returned false: %s\n", b->getLastError().c_str()); kill (b); return 6; }
        std::printf ("device: %s\n", b->getDeviceName().c_str());

        const float delays[1] = { 0.0f };
        const float gains [1] = { 1.0f };
        std::vector<float> mean; float peak = 0.0f, launchMs = 0.0f;
        if (driveConstant (b, 1, 1, block, { 0.5f }, delays, gains, mean, peak, launchMs, err) != 0)
        { std::printf ("FAIL: [A] %s\n", err.c_str()); kill (b); return 7; }

        const float expect = 0.5f;
        const bool okA = (peak > 1e-4f) && std::fabs (mean[0] - expect) < 0.05f;
        std::printf ("[A] 1x1 g=1 d=0   out.mean=%.4f (expect ~%.2f)  peak=%.4f  launchMs=%.3f  -> %s\n",
                     mean[0], expect, peak, launchMs, okA ? "PASS" : "FAIL");
        kill (b);
        if (! okA) { std::printf ("FAIL: scenario A (basic path) wrong\n"); return 8; }
    }

    // ---- Scenario B: 2 inputs -> 1 output, distinct gains -> reduce sum ----
    // out = g0*in0 + g1*in1 (gains indexed [in*numOut+out]; reduce streams inputs).
    // A backend that ignored input 1 (a 1x1 passthrough) would give 0.5, not 0.6.
    {
        IWfsBackend* b = create (0);
        if (b == nullptr) { std::printf ("FAIL: [B] create_wfs returned null\n"); return 5; }

        if (! b->prepare (2, 1, block, sr, /*latencyMs*/ 0.0, /*maxDelaySeconds*/ 1.0))
        { std::printf ("FAIL: [B] prepare() returned false: %s\n", b->getLastError().c_str()); kill (b); return 6; }

        const float delays[2] = { 0.0f, 0.0f };
        const float gains [2] = { 1.0f, 0.5f };          // gains[0]=in0, gains[1]=in1
        const float in0 = 0.5f, in1 = 0.2f;
        std::vector<float> mean; float peak = 0.0f, launchMs = 0.0f;
        if (driveConstant (b, 2, 1, block, { in0, in1 }, delays, gains, mean, peak, launchMs, err) != 0)
        { std::printf ("FAIL: [B] %s\n", err.c_str()); kill (b); return 7; }

        const float expect = gains[0] * in0 + gains[1] * in1;   // 1.0*0.5 + 0.5*0.2 = 0.6
        const bool okB = (peak > 1e-4f) && std::fabs (mean[0] - expect) < 0.05f;
        std::printf ("[B] 2->1 reduce   out.mean=%.4f (expect %.2f = %.1f*%.1f + %.1f*%.1f)  peak=%.4f  launchMs=%.3f  -> %s\n",
                     mean[0], expect, gains[0], in0, gains[1], in1, peak, launchMs, okB ? "PASS" : "FAIL");
        kill (b);
        if (! okB) { std::printf ("FAIL: scenario B (multi-input reduce) wrong\n"); return 9; }
    }

    std::printf ("PASS: GPU compute produced finite, non-silent, ~correct output (basic path + multi-input reduce)\n");
    return 0;
}
