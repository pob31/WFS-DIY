#pragma once
#if WFS_GPU_NATIVE

/*
    IrConvHostState — backend-shared host side of the hybrid GPU IR
    convolution (the IR-reverb analogue of WfsFrHostState).

    Owns everything the Metal and CUDA backends would otherwise duplicate:
    the IR staging handoff (engine thread -> pump thread), the progressive
    IR-segment FFT loader, the per-launch input FFTs, and the per-node
    overlap-add tails. The backends keep only the device plumbing (buffers,
    kernel dispatch, copies).

    Threading contract: stageIr() and requestReset() may be called from the
    engine/UI threads; everything else runs on the single pump thread.
    The pump consumes staged IRs with a try-lock so it never blocks behind
    a staging copy.

    Spectrum layout: every spectrum is fftLen = 2 * blockSize floats =
    blockSize packed (re,im) bins in the IrHostFft packing (bin 0 = DC +
    Nyquist). The destination base pointers passed in by the backend are
    CPU-visible (Metal shared storage / CUDA pinned staging).
*/

#include "IrHostFft.h"
#include "GpuHostWorkPool.h"

#include <algorithm>
#include <atomic>
#include <cstring>
#include <mutex>
#include <vector>

namespace spatcore::gpu {

class IrConvHostState
{
public:
    // Progressive IR-loader batch cap (per launch). Sizes the per-segment FFT
    // scratch, so the M3 pool-driven loadMoreSegments never needs more than
    // this many concurrent scratch rows. The backends pass this as maxSegments.
    static constexpr int kMaxLoadSegmentsPerLaunch = 64;

    /** False if blockSize is unusable (not a power of two in [4, 1024]). */
    bool prepare (int numNodesIn, int blockSizeIn, int maxIrSamplesIn)
    {
        numNodes = std::max (1, numNodesIn);
        blockSize = blockSizeIn;
        fftLen = 2 * blockSize;
        if (blockSize < 4 || blockSize > 1024 || ! fft.prepare (fftLen))
            return false;

        maxIrSamples = std::max (blockSize, maxIrSamplesIn);
        segCapacity = (maxIrSamples + blockSize - 1) / blockSize;

        tails.assign ((size_t) numNodes * (size_t) blockSize, 0.0f);
        timeScratch.assign ((size_t) fftLen, 0.0f);
        // M3 per-item FFT scratch (external-scratch overloads read the tables
        // read-only, so worker threads each drive their own row).
        nodeScratch.assign ((size_t) numNodes * (size_t) fftLen, 0.0f);
        segScratch.assign ((size_t) kMaxLoadSegmentsPerLaunch * (size_t) fftLen, 0.0f);
        ringHead = segCapacity - 1;   // first advance lands on slot 0

        irTimeDomain.clear();
        segmentsLoaded.store (0, std::memory_order_relaxed);
        segTotal.store (0, std::memory_order_relaxed);

        return true;
    }

    int getNumNodes() const noexcept     { return numNodes; }
    int getBlockSize() const noexcept    { return blockSize; }
    int getFftLen() const noexcept       { return fftLen; }
    int getSegCapacity() const noexcept  { return segCapacity; }
    int getRingHead() const noexcept     { return ringHead; }

    /** Diagnostics (any thread). */
    int getSegmentsLoaded() const noexcept { return segmentsLoaded.load (std::memory_order_relaxed); }
    int getSegmentsTotal() const noexcept  { return segTotal.load (std::memory_order_relaxed); }

    //==========================================================================
    // Engine/UI-thread API
    //==========================================================================

    /** Hands a final mono IR (already trimmed/resampled/normalised) to the
        pump. Extra samples beyond the device allocation are dropped. */
    void stageIr (const float* monoIr, int numSamples)
    {
        const int n = std::max (0, std::min (numSamples, maxIrSamples));
        std::lock_guard<std::mutex> lock (stageMutex);
        staged.assign (monoIr, monoIr + n);
        stagedDirty.store (true, std::memory_order_release);
    }

    /** Asks the pump to clear input history + tails at the next launch
        (the loaded IR spectra survive). */
    void requestReset() noexcept { resetRequested.store (true, std::memory_order_release); }

    //==========================================================================
    // Pump-thread API (single caller thread)
    //==========================================================================

    /** True when the backend must also zero its device input ring. */
    bool consumeResetRequest() noexcept
    {
        if (! resetRequested.exchange (false, std::memory_order_acq_rel))
            return false;
        std::fill (tails.begin(), tails.end(), 0.0f);
        ringHead = segCapacity - 1;
        return true;
    }

    /** Picks up a freshly staged IR if one is pending (try-lock: a staging
        copy in progress just defers to the next launch). Restarts the
        progressive segment loader. */
    void consumeStagedIr()
    {
        if (! stagedDirty.load (std::memory_order_acquire))
            return;
        std::unique_lock<std::mutex> lock (stageMutex, std::try_to_lock);
        if (! lock.owns_lock())
            return;
        stagedDirty.store (false, std::memory_order_release);
        irTimeDomain = std::move (staged);
        staged.clear();
        segmentsLoaded.store (0, std::memory_order_relaxed);
        segTotal.store ((int) ((irTimeDomain.size() + (size_t) blockSize - 1)
                               / (size_t) blockSize),
                        std::memory_order_relaxed);
    }

    /** FFTs up to maxSegments pending IR segments into
        irSpectraBase[seg * fftLen] (zero-padding the final partial segment).
        Budgeted per launch so a 10 s IR loads over a few tens of launches
        without ever blowing the pump deadline. Returns segments written. */
    int loadMoreSegments (float* irSpectraBase, int maxSegments)
    {
        const int total = segTotal.load (std::memory_order_relaxed);
        int loaded = segmentsLoaded.load (std::memory_order_relaxed);
        int written = 0;

        while (loaded < total && written < maxSegments)
        {
            const size_t start = (size_t) loaded * (size_t) blockSize;
            const size_t avail = irTimeDomain.size() - start;
            const size_t copyN = std::min (avail, (size_t) blockSize);

            std::memset (timeScratch.data(), 0, (size_t) fftLen * sizeof (float));
            std::memcpy (timeScratch.data(), irTimeDomain.data() + start, copyN * sizeof (float));
            fft.forward (timeScratch.data(), irSpectraBase + (size_t) loaded * (size_t) fftLen);

            ++loaded;
            ++written;
        }

        if (written > 0)
            segmentsLoaded.store (loaded, std::memory_order_relaxed);
        return written;
    }

    /** Advances the input ring; call once per launch BEFORE transformInput. */
    void advanceRing() noexcept { ringHead = (ringHead + 1) % segCapacity; }

    /** Zero-pads one node's input block and FFTs it into destSpectrum
        (fftLen floats). input may be null (silence). */
    void transformInput (const float* input, float* destSpectrum)
    {
        if (input != nullptr)
            std::memcpy (timeScratch.data(), input, (size_t) blockSize * sizeof (float));
        else
            std::memset (timeScratch.data(), 0, (size_t) blockSize * sizeof (float));
        std::memset (timeScratch.data() + blockSize, 0, (size_t) blockSize * sizeof (float));
        fft.forward (timeScratch.data(), destSpectrum);
    }

    /** Inverse-FFTs one node's accumulated spectrum and overlap-adds the
        node tail; writes blockSize samples to out. */
    void produceOutput (int node, const float* accSpectrum, float* out)
    {
        fft.inverse (accSpectrum, timeScratch.data());
        float* tail = tails.data() + (size_t) node * (size_t) blockSize;
        for (int i = 0; i < blockSize; ++i)
            out[i] = timeScratch[(size_t) i] + tail[i];
        std::memcpy (tail, timeScratch.data() + blockSize, (size_t) blockSize * sizeof (float));
    }

    //==========================================================================
    // M3 pool-driven variants: each item (node / segment) transforms into its
    // own spectrum row using its own scratch row, so ANY worker count is
    // bit-identical to the sequential loop (FFT tables read-only, per-node tail
    // + spectra rows disjoint — section-4 IR rows). The IR pump wraps its per-
    // node/segment loops in these; the FFT math is IrHostFft-shared, so CUDA /
    // HIP / Metal stay identical.
    //==========================================================================

    /** Zero-pads + FFTs every node's input block into hInSpectraBase[node*fftLen]
        across the pool (node-safe: per-node scratch row). inputs[node] may be
        null (silence). Bit-identical to a transformInput() loop. */
    void transformInputs (const float* const* inputs, float* hInSpectraBase, GpuHostWorkPool& pool)
    {
        pool.parallelFor (numNodes, [this, inputs, hInSpectraBase] (int node)
        {
            float* scratch = nodeScratch.data() + (size_t) node * (size_t) fftLen;
            const float* input = (inputs != nullptr) ? inputs[node] : nullptr;
            if (input != nullptr)
                std::memcpy (scratch, input, (size_t) blockSize * sizeof (float));
            else
                std::memset (scratch, 0, (size_t) blockSize * sizeof (float));
            std::memset (scratch + blockSize, 0, (size_t) blockSize * sizeof (float));
            // in == work == scratch (transform in place), out = the spectrum row.
            fft.forward (scratch, hInSpectraBase + (size_t) node * (size_t) fftLen, scratch);
        });
    }

    /** Inverse-FFTs + overlap-adds every node across the pool (node-safe:
        per-node scratch row + per-node tail). outputs[node] may be null (skip).
        Bit-identical to a produceOutput() loop. */
    void produceOutputs (float* const* outputs, const float* hOutSpectraBase, GpuHostWorkPool& pool)
    {
        pool.parallelFor (numNodes, [this, outputs, hOutSpectraBase] (int node)
        {
            float* out = (outputs != nullptr) ? outputs[node] : nullptr;
            if (out == nullptr)
                return;
            float* scratch = nodeScratch.data() + (size_t) node * (size_t) fftLen;
            const float* acc = hOutSpectraBase + (size_t) node * (size_t) fftLen;
            // in = acc (external, != work), out == work == scratch.
            fft.inverse (acc, scratch, scratch);
            const float* tail = tails.data() + (size_t) node * (size_t) blockSize;
            for (int i = 0; i < blockSize; ++i)
                out[i] = scratch[i] + tail[i];
            std::memcpy (tails.data() + (size_t) node * (size_t) blockSize,
                         scratch + blockSize, (size_t) blockSize * sizeof (float));
        });
    }

    /** Pool-driven progressive loader: FFTs up to maxSegments pending IR segments
        into irSpectraBase[seg*fftLen] (per-segment scratch row), advancing the
        loaded counter after the join. maxSegments is clamped to
        kMaxLoadSegmentsPerLaunch. Returns segments written. Bit-identical to the
        sequential loadMoreSegments (segments independent). */
    int loadMoreSegments (float* irSpectraBase, int maxSegments, GpuHostWorkPool& pool)
    {
        const int total  = segTotal.load (std::memory_order_relaxed);
        const int before = segmentsLoaded.load (std::memory_order_relaxed);

        int count = std::min (total - before, maxSegments);
        if (count > kMaxLoadSegmentsPerLaunch)
            count = kMaxLoadSegmentsPerLaunch;
        if (count <= 0)
            return 0;

        pool.parallelFor (count, [this, irSpectraBase, before] (int j)
        {
            const int seg = before + j;
            const size_t start = (size_t) seg * (size_t) blockSize;
            const size_t avail = irTimeDomain.size() - start;
            const size_t copyN = std::min (avail, (size_t) blockSize);

            float* scratch = segScratch.data() + (size_t) j * (size_t) fftLen;
            std::memset (scratch, 0, (size_t) fftLen * sizeof (float));
            std::memcpy (scratch, irTimeDomain.data() + start, copyN * sizeof (float));
            // in == work == scratch, out = the IR spectrum row.
            fft.forward (scratch, irSpectraBase + (size_t) seg * (size_t) fftLen, scratch);
        });

        segmentsLoaded.store (before + count, std::memory_order_relaxed);
        return count;
    }

private:
    IrHostFft fft;

    int numNodes = 0;
    int blockSize = 0;
    int fftLen = 0;
    int maxIrSamples = 0;
    int segCapacity = 0;
    int ringHead = 0;

    std::vector<float> tails;        // [numNodes][blockSize]
    std::vector<float> timeScratch;  // [fftLen] (single-thread member-scratch path)
    std::vector<float> nodeScratch;  // [numNodes][fftLen] M3 per-node FFT scratch
    std::vector<float> segScratch;   // [kMaxLoadSegmentsPerLaunch][fftLen] M3 per-segment

    // Engine thread -> pump thread IR handoff
    std::mutex stageMutex;
    std::vector<float> staged;
    std::atomic<bool> stagedDirty { false };
    std::atomic<bool> resetRequested { false };

    // Progressive loader state (pump thread; atomics only for diagnostics)
    std::vector<float> irTimeDomain;
    std::atomic<int> segmentsLoaded { 0 };
    std::atomic<int> segTotal { 0 };
};

} // namespace spatcore::gpu

// Extraction-compat alias — app code migrates to qualified names later.
using spatcore::gpu::IrConvHostState;

#endif // WFS_GPU_NATIVE
