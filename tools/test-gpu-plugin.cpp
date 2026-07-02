// test-gpu-plugin.cpp — headless smoke test for a GPU vendor plugin.
//
// Loads a wfs_<vendor> plugin (wfs_cuda.dll / wfs_hip.dll / libwfs_cuda.so /
// libwfs_hip.so) and drives EVERY GPU backend it exports through the shared
// IXBackend interfaces (no audio device, no async pipeline) to prove the full
// GPU compute path on the real hardware: plugin load -> backend create ->
// prepare (device init + runtime kernel compile, NVRTC/hipRTC) -> per-backend
// setup -> processBlock -> sane output.
//
// Seven scenarios cover all five GPU kernel families on the real device:
//   WFS (IWfsBackend, gather delay-and-sum):
//     A) 1x1, gain 1, delay 0      — output settles to the input (basic path).
//     B) 2 inputs -> 1 output,     — output = g0*in0 + g1*in1, exercising the
//        distinct gains [1.0,0.5]    multi-input reduce sum (a 1x1 passthrough
//                                     would FAIL it).
//   Reverbs (IIrBackend / IFdnBackend / ISdnBackend, stateful):
//     C) IR convolution — stage a decaying mono IR, fire ONE impulse on block 0,
//        expect a multi-block convolution response (finite, non-silent tail).
//     D) FDN reverb     — set RT60 params, fire one impulse, expect a decaying
//        feedback tail AFTER the input has gone silent.
//     E) SDN reverb     — set node geometry + params, fire one impulse, expect a
//        geometry-coupled tail AFTER the input has gone silent.
//   OutputBuffer (IObBackend, the write-time SCATTER dual of WFS — same matrix
//   delay-and-sum surface, so it reuses the WFS constant-drive check):
//     F) 1x1, gain 1               — output settles to the input (basic scatter;
//                                     OB's delay clamps to >= 1 sample, a startup
//                                     delay invisible in the settled last block).
//     G) 1 input -> 2 outputs,     — out0=g0*in, out1=g1*in: one input DISTRIBUTED
//        distinct gains [1.0,0.5]    to many outputs (a single-output backend
//                                     would leave out1 silent).
//   The reverbs have no closed-form output, so the assertion is: output is
//   finite and NON-SILENT in a "tail" window (blocks after the impulse, once the
//   input is silent) — energy there can only come from the algorithm's internal
//   state (delay-line feedback / node coupling / multi-tap convolution), which a
//   dry passthrough could not produce.
//
// The WFS scenarios use constant inputs and check the LAST block's steady-state
// mean, so the per-sample prev->curr ramp has settled. (The delay-ring /
// fractional-delay path is covered by the Phase-1 bit-exact impulse validation.)
//
// Build (Windows, from a VS dev prompt):
//   cl /nologo /EHsc /std:c++17 /DWFS_GPU_NATIVE=1 /I..\Source\DSP\gpu test-gpu-plugin.cpp /Fe:test-gpu-plugin.exe
// Build (Linux):
//   g++ -std=c++17 -DWFS_GPU_NATIVE=1 -I../spatcore/gpu test-gpu-plugin.cpp -ldl -o test-gpu-plugin
//
// Usage: test-gpu-plugin <plugin-path> [device-index]
//   device-index (optional, default 0) selects which GPU of the vendor to bind
//   (e.g. 1 for the second of two identical cards) — passed to create(idx).
// Exit code 0 = every backend created, prepared, and produced finite, non-silent,
// approximately-correct output; non-zero = failure (reason + exit code printed).

#include "GpuBackendInterface.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
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
using CreateObFn  = IObBackend*  (*) (int);
using CreateIrFn  = IIrBackend*  (*) (int);
using CreateFdnFn = IFdnBackend* (*) (int);
using CreateSdnFn = ISdnBackend* (*) (int);
using VendorFn    = const char*  (*) ();
using DestroyFn   = void         (*) (IGpuBackend*);

// Drive a freshly-prepared WFS or OB (matrix delay-and-sum / scatter) backend
// with per-input CONSTANT signals for kBlocks blocks and report the LAST block's
// per-output mean and the global peak. The output buffers are zeroed before
// every processBlock, so a call that returns true but writes nothing leaves
// peak==0 (caught by the caller). IWfsBackend and IObBackend expose the identical
// setMatrixPointers/processBlock surface, so this is templated over both.
// Returns 0 on success, <0 on processBlock failure / non-finite output (err set).
template <class Backend>
static int driveConstant (Backend* b, int nIn, int nOut, int block,
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

// Result of driving a stateful (reverb) backend with a single impulse.
struct ReverbDrive
{
    bool  ok         = true;    // all outputs finite AND no processBlock failure
    float globalPeak = 0.0f;    // max |out| over every block
    float tailPeak   = 0.0f;    // max |out| over blocks >= tailStart (input silent there)
    float lastLaunchMs = 0.0f;
    std::string err;
};

// Pump `numBlocks` blocks of `numNodes` x `block` through any IGpuBackend
// (IIr/IFdn/ISdn share the pump-facing processBlock surface). A single impulse
// of `impulseAmp` is placed on node 0, sample 0 of block 0 ONLY; every other
// sample is silence. Tracks finite-ness, the global peak, and the "tail" peak
// over blocks [tailStart, numBlocks) — energy there is downstream of the
// algorithm's own state, not the current input.
static ReverbDrive driveImpulse (IGpuBackend* b, int numNodes, int block,
                                 int numBlocks, float impulseAmp, int tailStart)
{
    ReverbDrive r;
    std::vector<std::vector<float>> in  (numNodes, std::vector<float> (block, 0.0f));
    std::vector<std::vector<float>> out (numNodes, std::vector<float> (block, 0.0f));
    std::vector<const float*> inP  (numNodes);
    std::vector<float*>       outP (numNodes);

    for (int blk = 0; blk < numBlocks; ++blk)
    {
        for (int n = 0; n < numNodes; ++n)
        {
            std::fill (in[n].begin(),  in[n].end(),  0.0f);
            std::fill (out[n].begin(), out[n].end(), 0.0f);
            inP[n]  = in[n].data();
            outP[n] = out[n].data();
        }
        if (blk == 0) in[0][0] = impulseAmp;      // one impulse, node 0, sample 0

        if (! b->processBlock (inP.data(), outP.data()))
        {
            r.ok = false;
            r.err = "processBlock() returned false on block " + std::to_string (blk) + ": " + b->getLastError();
            return r;
        }

        float blkPeak = 0.0f;
        for (int n = 0; n < numNodes; ++n)
            for (float v : out[n])
            {
                if (! std::isfinite (v))
                {
                    r.ok = false;
                    r.err = "non-finite output (" + std::to_string (v) + ") on block " + std::to_string (blk);
                    return r;
                }
                blkPeak = std::max (blkPeak, std::fabs (v));
            }

        r.globalPeak = std::max (r.globalPeak, blkPeak);
        if (blk >= tailStart) r.tailPeak = std::max (r.tailPeak, blkPeak);
        r.lastLaunchMs = (float) b->getLastLaunchMs();
    }
    return r;
}

int main (int argc, char** argv)
{
    if (argc < 2) { std::printf ("usage: %s <plugin-path> [device-index]\n", argv[0]); return 2; }
    const char* path = argv[1];
    const int deviceIndex = (argc >= 3) ? std::atoi (argv[2]) : 0;

    Lib lib = openLib (path);
    if (lib == nullptr) { std::printf ("FAIL: could not load '%s'\n", path); return 3; }

    auto vendor    = (VendorFn)    sym (lib, "wfs_plugin_vendor");
    auto create    = (CreateWfsFn) sym (lib, "wfs_plugin_create_wfs");
    auto createOb  = (CreateObFn)  sym (lib, "wfs_plugin_create_ob");
    auto createIr  = (CreateIrFn)  sym (lib, "wfs_plugin_create_ir");
    auto createFdn = (CreateFdnFn) sym (lib, "wfs_plugin_create_fdn");
    auto createSdn = (CreateSdnFn) sym (lib, "wfs_plugin_create_sdn");
    auto destroy   = (DestroyFn)   sym (lib, "wfs_plugin_destroy");
    if (create == nullptr) { std::printf ("FAIL: wfs_plugin_create_wfs not exported\n"); return 4; }
    auto kill = [destroy] (IGpuBackend* p) { if (destroy) destroy (p); else delete p; };

    std::printf ("loaded: %s   vendor: %s   deviceIndex: %d\n",
                 path, vendor ? vendor() : "(no wfs_plugin_vendor)", deviceIndex);

    const int block = 64;
    const double sr = 48000.0;
    std::string err;

    // ---- Scenario A: 1x1, gain 1, delay 0 -> output settles to input ----
    {
        IWfsBackend* b = create (deviceIndex);
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
        std::printf ("[A] WFS 1x1 g=1 d=0   out.mean=%.4f (expect ~%.2f)  peak=%.4f  launchMs=%.3f  -> %s\n",
                     mean[0], expect, peak, launchMs, okA ? "PASS" : "FAIL");
        kill (b);
        if (! okA) { std::printf ("FAIL: scenario A (basic path) wrong\n"); return 8; }
    }

    // ---- Scenario B: 2 inputs -> 1 output, distinct gains -> reduce sum ----
    // out = g0*in0 + g1*in1 (gains indexed [in*numOut+out]; reduce streams inputs).
    // A backend that ignored input 1 (a 1x1 passthrough) would give 0.5, not 0.6.
    {
        IWfsBackend* b = create (deviceIndex);
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
        std::printf ("[B] WFS 2->1 reduce   out.mean=%.4f (expect %.2f = %.1f*%.1f + %.1f*%.1f)  peak=%.4f  launchMs=%.3f  -> %s\n",
                     mean[0], expect, gains[0], in0, gains[1], in1, peak, launchMs, okB ? "PASS" : "FAIL");
        kill (b);
        if (! okB) { std::printf ("FAIL: scenario B (multi-input reduce) wrong\n"); return 9; }
    }

    // ---- Scenario C: IR convolution reverb (IIrBackend) ----
    // Every node convolves the SAME staged mono IR. Fire one impulse on node 0
    // block 0; a multi-tap IR (4096 samples @ block 256 = ~16 response blocks)
    // must produce a finite, non-silent response spanning blocks past block 0.
    {
        if (createIr == nullptr) { std::printf ("FAIL: wfs_plugin_create_ir not exported\n"); return 10; }
        IIrBackend* b = createIr (deviceIndex);
        if (b == nullptr) { std::printf ("FAIL: [C] create_ir returned null\n"); return 11; }

        const int nodes = 4, irBlock = 256, numBlocks = 24;    // block must be pow2 in [4,1024]
        if (! b->prepare (nodes, irBlock, sr, /*maxIrSamples*/ (int) sr))   // 1 s device cap
        { std::printf ("FAIL: [C] IR prepare() returned false: %s\n", b->getLastError().c_str()); kill (b); return 12; }

        // Simple decaying (alternating-sign) mono IR; segments (<=64) load fully
        // on the first processBlock, so the impulse on block 0 is convolved live.
        const int irLen = 4096;
        std::vector<float> ir ((size_t) irLen);
        for (int i = 0; i < irLen; ++i)
            ir[(size_t) i] = std::exp (-3.0f * (float) i / (float) irLen) * ((i & 1) ? -1.0f : 1.0f);
        ir[0] = 1.0f;
        b->stageIr (ir.data(), irLen);

        ReverbDrive r = driveImpulse (b, nodes, irBlock, numBlocks, 1.0f, /*tailStart*/ 1);
        const bool okC = r.ok && r.globalPeak > 1e-4f && r.tailPeak > 1e-5f;
        std::printf ("[C] IR conv    nodes=%d  peak=%.4f  tailPeak=%.4f  segs=%d/%d  launchMs=%.3f  -> %s\n",
                     nodes, r.globalPeak, r.tailPeak, b->getSegmentsLoaded(), b->getSegmentsTotal(),
                     r.lastLaunchMs, okC ? "PASS" : "FAIL");
        kill (b);
        if (! okC) { std::printf ("FAIL: scenario C (IR reverb) %s\n", r.ok ? "silent / no tail" : r.err.c_str()); return 13; }
    }

    // ---- Scenario D: FDN reverb (IFdnBackend) ----
    // One 16-line FDN per node. Fire one impulse; feedback rings warm up over the
    // first blocks, so we require energy in a TAIL window (blocks >= 4) where the
    // input is already silent — that energy is pure recirculating feedback.
    {
        if (createFdn == nullptr) { std::printf ("FAIL: wfs_plugin_create_fdn not exported\n"); return 14; }
        IFdnBackend* b = createFdn (deviceIndex);
        if (b == nullptr) { std::printf ("FAIL: [D] create_fdn returned null\n"); return 15; }

        const int nodes = 8, fdnBlock = 512, numBlocks = 24;
        if (! b->prepare (nodes, fdnBlock, sr, /*fdnSize*/ 1.0f))
        { std::printf ("FAIL: [D] FDN prepare() returned false: %s\n", b->getLastError().c_str()); kill (b); return 16; }

        b->setParameters (/*rt60*/ 1.5f, /*lowMult*/ 1.3f, /*highMult*/ 0.5f,
                          /*xoverLow*/ 200.0f, /*xoverHigh*/ 4000.0f, /*diffusion*/ 0.5f);
        b->requestReset();

        ReverbDrive r = driveImpulse (b, nodes, fdnBlock, numBlocks, 1.0f, /*tailStart*/ 4);
        const bool okD = r.ok && r.globalPeak > 1e-3f && r.tailPeak > 1e-4f;
        std::printf ("[D] FDN reverb nodes=%d  peak=%.4f  tailPeak=%.4f (feedback tail)  launchMs=%.3f  -> %s\n",
                     nodes, r.globalPeak, r.tailPeak, r.lastLaunchMs, okD ? "PASS" : "FAIL");
        kill (b);
        if (! okD) { std::printf ("FAIL: scenario D (FDN reverb) %s\n", r.ok ? "silent / no tail" : r.err.c_str()); return 17; }
    }

    // ---- Scenario E: SDN reverb (ISdnBackend) ----
    // 8 nodes at the corners of a ~4x3x2.5 m box so inter-node delays are
    // non-trivial (not clamped to 1 sample). Fire one impulse; require a tail
    // (blocks >= 2, after the geometry crossfade settles) from node coupling.
    {
        if (createSdn == nullptr) { std::printf ("FAIL: wfs_plugin_create_sdn not exported\n"); return 18; }
        ISdnBackend* b = createSdn (deviceIndex);
        if (b == nullptr) { std::printf ("FAIL: [E] create_sdn returned null\n"); return 19; }

        const int nodes = 8, sdnBlock = 256, numBlocks = 24;
        if (! b->prepare (nodes, sdnBlock, sr))
        { std::printf ("FAIL: [E] SDN prepare() returned false: %s\n", b->getLastError().c_str()); kill (b); return 20; }

        float xyz[3 * 8];                                  // count == numNodes, xyz triplets (metres)
        for (int n = 0; n < nodes; ++n)
        {
            xyz[3 * n + 0] = (n & 1) ? 4.0f : 0.0f;
            xyz[3 * n + 1] = (n & 2) ? 3.0f : 0.0f;
            xyz[3 * n + 2] = (n & 4) ? 2.5f : 0.0f;
        }
        b->setGeometry (xyz, nodes);
        b->setParameters (/*rt60*/ 1.5f, /*lowMult*/ 1.3f, /*highMult*/ 0.5f,
                          /*xoverLow*/ 200.0f, /*xoverHigh*/ 4000.0f, /*diffusion*/ 0.5f, /*sdnScale*/ 1.0f);
        b->requestReset();

        ReverbDrive r = driveImpulse (b, nodes, sdnBlock, numBlocks, 1.0f, /*tailStart*/ 2);
        const bool okE = r.ok && r.globalPeak > 1e-4f && r.tailPeak > 1e-5f;
        std::printf ("[E] SDN reverb nodes=%d  peak=%.4f  tailPeak=%.4f (coupled tail)  launchMs=%.3f  -> %s\n",
                     nodes, r.globalPeak, r.tailPeak, r.lastLaunchMs, okE ? "PASS" : "FAIL");
        kill (b);
        if (! okE) { std::printf ("FAIL: scenario E (SDN reverb) %s\n", r.ok ? "silent / no tail" : r.err.c_str()); return 21; }
    }

    // ---- Scenario F: OutputBuffer (scatter) 1x1, gain 1 -> settles to input ----
    // OB is the write-time scatter dual of WFS and shares its matrix surface, so
    // the constant-drive check is reused verbatim. OB's delay contract clamps to
    // d >= 1 sample (a same-cell write can't re-emerge instantly), giving a
    // 1-sample startup delay that is invisible in the settled LAST block's mean.
    {
        if (createOb == nullptr) { std::printf ("FAIL: wfs_plugin_create_ob not exported\n"); return 22; }
        IObBackend* b = createOb (deviceIndex);
        if (b == nullptr) { std::printf ("FAIL: [F] create_ob returned null\n"); return 23; }

        if (! b->prepare (1, 1, block, sr, /*latencyMs*/ 0.0, /*maxDelaySeconds*/ 1.0))
        { std::printf ("FAIL: [F] OB prepare() returned false: %s\n", b->getLastError().c_str()); kill (b); return 24; }

        const float delays[1] = { 0.0f };
        const float gains [1] = { 1.0f };
        std::vector<float> mean; float peak = 0.0f, launchMs = 0.0f;
        if (driveConstant (b, 1, 1, block, { 0.5f }, delays, gains, mean, peak, launchMs, err) != 0)
        { std::printf ("FAIL: [F] %s\n", err.c_str()); kill (b); return 25; }

        const float expect = 0.5f;
        const bool okF = (peak > 1e-4f) && std::fabs (mean[0] - expect) < 0.05f;
        std::printf ("[F] OB 1x1 g=1 d=0    out.mean=%.4f (expect ~%.2f)  peak=%.4f  launchMs=%.3f  -> %s\n",
                     mean[0], expect, peak, launchMs, okF ? "PASS" : "FAIL");
        kill (b);
        if (! okF) { std::printf ("FAIL: scenario F (OB basic scatter) wrong\n"); return 26; }
    }

    // ---- Scenario G: OB 1 input -> 2 outputs, distinct gains -> scatter ----
    // The dual of WFS's reduce: one input DISTRIBUTED to many outputs. gains are
    // indexed [in*numOut+out], so out0=g0*in, out1=g1*in. A single-output or
    // gather-collapsed backend would leave out1 silent.
    {
        IObBackend* b = createOb (deviceIndex);
        if (b == nullptr) { std::printf ("FAIL: [G] create_ob returned null\n"); return 23; }

        if (! b->prepare (1, 2, block, sr, /*latencyMs*/ 0.0, /*maxDelaySeconds*/ 1.0))
        { std::printf ("FAIL: [G] OB prepare() returned false: %s\n", b->getLastError().c_str()); kill (b); return 24; }

        const float delays[2] = { 0.0f, 0.0f };          // [in*numOut+out]: out0, out1
        const float gains [2] = { 1.0f, 0.5f };
        const float in0 = 0.5f;
        std::vector<float> mean; float peak = 0.0f, launchMs = 0.0f;
        if (driveConstant (b, 1, 2, block, { in0 }, delays, gains, mean, peak, launchMs, err) != 0)
        { std::printf ("FAIL: [G] %s\n", err.c_str()); kill (b); return 25; }

        const float e0 = gains[0] * in0, e1 = gains[1] * in0;   // 1.0*0.5 = 0.50, 0.5*0.5 = 0.25
        const bool okG = (peak > 1e-4f)
                      && std::fabs (mean[0] - e0) < 0.05f
                      && std::fabs (mean[1] - e1) < 0.05f;
        std::printf ("[G] OB 1->2 scatter   out0=%.4f (expect %.2f)  out1=%.4f (expect %.2f)  peak=%.4f  launchMs=%.3f  -> %s\n",
                     mean[0], e0, mean[1], e1, peak, launchMs, okG ? "PASS" : "FAIL");
        kill (b);
        if (! okG) { std::printf ("FAIL: scenario G (OB multi-output scatter) wrong\n"); return 27; }
    }

    std::printf ("PASS: all GPU backends (WFS gather+reduce, OB scatter, IR conv, FDN, SDN) produced finite, non-silent, ~correct output\n");
    return 0;
}
