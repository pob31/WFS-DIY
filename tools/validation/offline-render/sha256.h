#pragma once

//==============================================================================
// Self-contained SHA-256 (FIPS 180-4). Kept local to the offline-render harness
// so the hash of the raw float32 PCM byte dump matches `sha256sum` /
// `Get-FileHash -Algorithm SHA256` of the same bytes, with no dependency on
// juce_cryptography (which the app project does not build).
//==============================================================================

#include <cstdint>
#include <cstring>
#include <cstddef>
#include <string>

namespace orh
{

class Sha256
{
public:
    Sha256() { reset(); }

    void reset()
    {
        state[0] = 0x6a09e667u; state[1] = 0xbb67ae85u;
        state[2] = 0x3c6ef372u; state[3] = 0xa54ff53au;
        state[4] = 0x510e527fu; state[5] = 0x9b05688cu;
        state[6] = 0x1f83d9abu; state[7] = 0x5be0cd19u;
        totalBits = 0;
        bufferLen = 0;
    }

    void update (const void* data, size_t len)
    {
        const auto* p = static_cast<const uint8_t*> (data);
        totalBits += static_cast<uint64_t> (len) * 8u;

        while (len > 0)
        {
            const size_t take = (len < (64 - bufferLen)) ? len : (64 - bufferLen);
            std::memcpy (buffer + bufferLen, p, take);
            bufferLen += take;
            p += take;
            len -= take;

            if (bufferLen == 64)
            {
                transform (buffer);
                bufferLen = 0;
            }
        }
    }

    /** Finalises and returns the digest as a lowercase hex string. */
    std::string finalHex()
    {
        // Padding: 0x80, zeros, 64-bit big-endian bit length.
        uint8_t pad[72] = {};
        pad[0] = 0x80;
        const size_t padLen = (bufferLen < 56) ? (56 - bufferLen) : (120 - bufferLen);

        uint8_t lenBytes[8];
        for (int i = 0; i < 8; ++i)
            lenBytes[i] = static_cast<uint8_t> (totalBits >> (56 - 8 * i));

        // update() would keep counting bits, so feed the tail manually.
        appendRaw (pad, padLen);
        appendRaw (lenBytes, 8);

        static const char* hex = "0123456789abcdef";
        std::string out;
        out.reserve (64);
        for (int i = 0; i < 8; ++i)
            for (int b = 0; b < 4; ++b)
            {
                const uint8_t byte = static_cast<uint8_t> (state[i] >> (24 - 8 * b));
                out.push_back (hex[byte >> 4]);
                out.push_back (hex[byte & 0x0f]);
            }
        return out;
    }

    /** One-shot convenience. */
    static std::string hashHex (const void* data, size_t len)
    {
        Sha256 h;
        h.update (data, len);
        return h.finalHex();
    }

    /** Known-answer self-test ("abc"). Returns true when the implementation
        is producing standard SHA-256 digests. */
    static bool selfTest()
    {
        return hashHex ("abc", 3)
            == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
    }

private:
    void appendRaw (const uint8_t* p, size_t len)
    {
        while (len > 0)
        {
            const size_t take = (len < (64 - bufferLen)) ? len : (64 - bufferLen);
            std::memcpy (buffer + bufferLen, p, take);
            bufferLen += take;
            p += take;
            len -= take;

            if (bufferLen == 64)
            {
                transform (buffer);
                bufferLen = 0;
            }
        }
    }

    static uint32_t rotr (uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

    void transform (const uint8_t* chunk)
    {
        static const uint32_t K[64] = {
            0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
            0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
            0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
            0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
            0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
            0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
            0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
            0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u };

        uint32_t w[64];
        for (int i = 0; i < 16; ++i)
            w[i] = (static_cast<uint32_t> (chunk[4 * i]) << 24)
                 | (static_cast<uint32_t> (chunk[4 * i + 1]) << 16)
                 | (static_cast<uint32_t> (chunk[4 * i + 2]) << 8)
                 |  static_cast<uint32_t> (chunk[4 * i + 3]);

        for (int i = 16; i < 64; ++i)
        {
            const uint32_t s0 = rotr (w[i - 15], 7) ^ rotr (w[i - 15], 18) ^ (w[i - 15] >> 3);
            const uint32_t s1 = rotr (w[i - 2], 17) ^ rotr (w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }

        uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
        uint32_t e = state[4], f = state[5], g = state[6], h = state[7];

        for (int i = 0; i < 64; ++i)
        {
            const uint32_t S1 = rotr (e, 6) ^ rotr (e, 11) ^ rotr (e, 25);
            const uint32_t ch = (e & f) ^ (~e & g);
            const uint32_t temp1 = h + S1 + ch + K[i] + w[i];
            const uint32_t S0 = rotr (a, 2) ^ rotr (a, 13) ^ rotr (a, 22);
            const uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            const uint32_t temp2 = S0 + maj;

            h = g; g = f; f = e; e = d + temp1;
            d = c; c = b; b = a; a = temp1 + temp2;
        }

        state[0] += a; state[1] += b; state[2] += c; state[3] += d;
        state[4] += e; state[5] += f; state[6] += g; state[7] += h;
    }

    uint32_t state[8] = {};
    uint64_t totalBits = 0;
    uint8_t  buffer[64] = {};
    size_t   bufferLen = 0;
};

} // namespace orh
