#pragma once

/*
    IrHostFft — small JUCE-free real FFT for the GPU IR-reverb backends.

    Purpose-built for the hybrid convolution scheme (host FFT, GPU
    frequency-domain MAC): forward() transforms 2P real samples into P packed
    complex bins, inverse() transforms P packed bins back into 2P reals.

    Packing (the GPU kernels and both backends rely on it):
        bin 0   = (DC, Nyquist)            — both real
        bin k   = X[k], k = 1 .. P-1       — complex (re, im)
    so a spectrum is exactly P float2 values for an FFT of length N = 2P.
    The per-bin product of two packed spectra is the element-wise product
    (dc*dc, ny*ny) at bin 0 and the complex product elsewhere; inverse() of
    such a product yields the exact N-point circular convolution (the 1/N
    falls out of the half-size IFFT scaling), which is what the overlap-add
    partitioning needs.

    Implementation: N-point real FFT via the standard M = N/2 complex FFT
    pack/untangle identity (exact, not an approximation), iterative radix-2
    with precomputed bit-reversal and twiddle tables. Roundtrip
    inverse(forward(x)) == x to float precision. Sizes: N a power of two,
    8 <= N <= 8192 — far beyond the engine's 2*internalBlockSize <= 512.

    Thread contract: forward()/inverse() use internal scratch — one instance
    per calling thread (the backends call it from the pump thread only).
    Verified against a naive DFT in Experiments/metal-ir-test.
*/

#include <cmath>
#include <cstring>
#include <vector>

namespace spatcore::gpu {

class IrHostFft
{
public:
    /** realLength = N = 2P, power of two in [8, 8192]. */
    bool prepare (int realLength)
    {
        n = 0;
        if (realLength < 8 || realLength > 8192
            || (realLength & (realLength - 1)) != 0)
            return false;

        n = realLength;
        m = n / 2;

        const double twoPi = 6.283185307179586476925;

        // Bit-reversal permutation for the M-point complex FFT.
        bitrev.assign ((size_t) m, 0);
        int bits = 0;
        while ((1 << bits) < m)
            ++bits;
        for (int i = 0; i < m; ++i)
        {
            int r = 0;
            for (int b = 0; b < bits; ++b)
                if (i & (1 << b))
                    r |= 1 << (bits - 1 - b);
            bitrev[(size_t) i] = r;
        }

        // Complex-FFT twiddles: e^{-2*pi*i*j/M}, j < M/2.
        twRe.assign ((size_t) (m / 2), 0.0f);
        twIm.assign ((size_t) (m / 2), 0.0f);
        for (int j = 0; j < m / 2; ++j)
        {
            const double a = -twoPi * j / m;
            twRe[(size_t) j] = (float) std::cos (a);
            twIm[(size_t) j] = (float) std::sin (a);
        }

        // Split (untangle) twiddles: e^{-2*pi*i*k/N}, k < M.
        spRe.assign ((size_t) m, 0.0f);
        spIm.assign ((size_t) m, 0.0f);
        for (int k = 0; k < m; ++k)
        {
            const double a = -twoPi * k / n;
            spRe[(size_t) k] = (float) std::cos (a);
            spIm[(size_t) k] = (float) std::sin (a);
        }

        work.assign ((size_t) n, 0.0f);
        return true;
    }

    bool isPrepared() const noexcept { return n > 0; }
    int getRealLength() const noexcept { return n; }
    int getNumBins() const noexcept { return m; }   // packed float2 count = P

    /** in: N reals. out: N floats = P packed (re,im) pairs. May alias.
        Member-scratch overload (single-thread callers). */
    void forward (const float* in, float* out) { forward (in, out, work.data()); }

    /** in: N floats = P packed pairs. out: N reals. May alias.
        Member-scratch overload (single-thread callers). */
    void inverse (const float* in, float* out) { inverse (in, out, work.data()); }

    /** External-scratch forward (M3): the twiddle/bit-reversal tables are
        read-only after prepare(), so one IrHostFft instance can be driven from
        several worker threads at once, each passing its OWN `work` buffer
        (>= N floats). `in` may equal `work` (transform in place); `out` must
        NOT alias `work`. Bit-identical to the member-scratch overload. */
    void forward (const float* in, float* out, float* work) const
    {
        // z[j] = (x[2j], x[2j+1]) — the interleaved input IS the packed
        // complex sequence, straight into the work buffer.
        if (work != in)
            std::memcpy (work, in, (size_t) n * sizeof (float));
        fftComplex (work, false);

        const float z0re = work[0], z0im = work[1];
        out[0] = z0re + z0im;   // DC
        out[1] = z0re - z0im;   // Nyquist

        for (int k = 1; k < m; ++k)
        {
            const float are = work[(size_t) (2 * k)];
            const float aim = work[(size_t) (2 * k + 1)];
            const float bre = work[(size_t) (2 * (m - k))];
            const float bim = work[(size_t) (2 * (m - k) + 1)];

            // Xe = (Z[k] + conj(Z[M-k])) / 2 ; Xo = (Z[k] - conj(Z[M-k])) / 2i
            const float xeRe = 0.5f * (are + bre);
            const float xeIm = 0.5f * (aim - bim);
            const float xoRe = 0.5f * (aim + bim);
            const float xoIm = 0.5f * (bre - are);

            // X[k] = Xe + W^k * Xo, W^k = e^{-2*pi*i*k/N}
            const float wre = spRe[(size_t) k], wim = spIm[(size_t) k];
            out[(size_t) (2 * k)]     = xeRe + wre * xoRe - wim * xoIm;
            out[(size_t) (2 * k + 1)] = xeIm + wre * xoIm + wim * xoRe;
        }
    }

    /** External-scratch inverse (M3): as forward() above. `in` must NOT alias
        `work`; `out` MAY equal `work` (the final scale writes out[j] from
        work[j] index-for-index). Bit-identical to the member-scratch overload. */
    void inverse (const float* in, float* out, float* work) const
    {
        // Z[0] from the real DC/Nyquist pair.
        const float dc = in[0], ny = in[1];
        work[0] = 0.5f * (dc + ny);
        work[1] = 0.5f * (dc - ny);

        for (int k = 1; k < m; ++k)
        {
            const float are = in[(size_t) (2 * k)];
            const float aim = in[(size_t) (2 * k + 1)];
            const float bre = in[(size_t) (2 * (m - k))];
            const float bim = in[(size_t) (2 * (m - k) + 1)];

            // Xe = (X[k] + conj(X[M-k])) / 2 ; Xo = (X[k] - conj(X[M-k])) / 2 * conj(W^k)
            const float xeRe = 0.5f * (are + bre);
            const float xeIm = 0.5f * (aim - bim);
            const float dRe  = 0.5f * (are - bre);
            const float dIm  = 0.5f * (aim + bim);
            const float wre = spRe[(size_t) k], wim = -spIm[(size_t) k]; // conj(W^k)
            const float xoRe = dRe * wre - dIm * wim;
            const float xoIm = dRe * wim + dIm * wre;

            // Z[k] = Xe + i * Xo
            work[(size_t) (2 * k)]     = xeRe - xoIm;
            work[(size_t) (2 * k + 1)] = xeIm + xoRe;
        }

        fftComplex (work, true);

        const float scale = 1.0f / (float) m;
        for (int j = 0; j < n; ++j)
            out[j] = work[(size_t) j] * scale;
    }

private:
    // In-place iterative radix-2 complex FFT of M points, interleaved (re,im).
    void fftComplex (float* d, bool inv) const
    {
        for (int i = 0; i < m; ++i)
        {
            const int r = bitrev[(size_t) i];
            if (r > i)
            {
                std::swap (d[2 * i],     d[2 * r]);
                std::swap (d[2 * i + 1], d[2 * r + 1]);
            }
        }

        for (int len = 2; len <= m; len <<= 1)
        {
            const int half = len >> 1;
            const int step = m / len;   // twiddle index stride
            for (int base = 0; base < m; base += len)
            {
                for (int j = 0; j < half; ++j)
                {
                    const float wre = twRe[(size_t) (j * step)];
                    const float wim = inv ? -twIm[(size_t) (j * step)]
                                          :  twIm[(size_t) (j * step)];
                    float* a = d + 2 * (base + j);
                    float* b = d + 2 * (base + j + half);
                    const float tre = b[0] * wre - b[1] * wim;
                    const float tim = b[0] * wim + b[1] * wre;
                    b[0] = a[0] - tre;
                    b[1] = a[1] - tim;
                    a[0] += tre;
                    a[1] += tim;
                }
            }
        }
    }

    int n = 0, m = 0;
    std::vector<int> bitrev;
    std::vector<float> twRe, twIm;   // complex-FFT twiddles (forward sign)
    std::vector<float> spRe, spIm;   // split twiddles e^{-2 pi i k / N}
    std::vector<float> work;
};

} // namespace spatcore::gpu

// Extraction-compat alias — app code migrates to qualified names later.
using spatcore::gpu::IrHostFft;
