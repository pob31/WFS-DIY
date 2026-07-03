/*
    SpatcoreTests.cpp - minimal, dependency-free unit tests for spatcore
    (no gtest; plain asserts + exit code). Built as the `spatcore-tests`
    console app when SPATCORE_STANDALONE_TESTS=ON; run it -> exit 0 = pass.

    Coverage (Phase 6 prep - a seed, not a suite):
      1. rt/LockFreeRingBuffer   write/read roundtrip, wraparound, capacity clamp
      2. dsp/DelayTargetSmoother determinism: same observation sequence on two
                                 fresh instances -> bit-identical output streams
                                 (includes a teleport jump)
      3. rt/RtSnapshot           publish/acquire roundtrip; instantiating it is
                                 the compile-proof of the POD static_assert
                                 (a non-trivially-copyable T must NOT compile -
                                 can't be expressed in a passing build)
      4. control/osc parser+serializer   OSCSerializer::serializeMessage ->
                                 OSCParser::parseMessage roundtrip + byte-stable
                                 re-serialization
*/

// OSCParser.h / OSCSerializer.h use juce::OSC* types but (verbatim-moved,
// hygiene pass pending) only include juce_core themselves; every includer in
// the app provides juce_osc first, and so do we.
#include <juce_osc/juce_osc.h>

#include "spatcore/rt/LockFreeRingBuffer.h"
#include "spatcore/rt/RtSnapshot.h"
#include "spatcore/dsp/DelayTargetSmoother.h"
#include "spatcore/control/osc/OSCSerializer.h"
#include "spatcore/control/osc/OSCParser.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

static int failures = 0;

#define CHECK(expr)                                                          \
    do                                                                       \
    {                                                                        \
        if (!(expr))                                                         \
        {                                                                    \
            std::fprintf (stderr, "FAIL %s:%d: %s\n",                        \
                          __FILE__, __LINE__, #expr);                        \
            ++failures;                                                      \
        }                                                                    \
    } while (false)

static bool bitEqualFloat (float a, float b) noexcept
{
    return std::memcmp (&a, &b, sizeof (float)) == 0;
}

//==============================================================================
static void testLockFreeRingBuffer()
{
    spatcore::rt::LockFreeRingBuffer rb;
    rb.setSize (8);   // one slot stays empty -> usable capacity 7

    float in[16], out[16];
    for (int i = 0; i < 16; ++i)
        in[i] = static_cast<float> (i + 1);

    CHECK (rb.getAvailableData() == 0);

    // Simple roundtrip
    CHECK (rb.write (in, 5) == 5);
    CHECK (rb.getAvailableData() == 5);
    CHECK (rb.read (out, 5) == 5);
    CHECK (std::memcmp (out, in, 5 * sizeof (float)) == 0);
    CHECK (rb.getAvailableData() == 0);

    // Wraparound: write pointer sits at 5 of 8; 6 samples span the seam
    CHECK (rb.write (in + 5, 6) == 6);
    CHECK (rb.getAvailableData() == 6);
    CHECK (rb.read (out, 6) == 6);
    CHECK (std::memcmp (out, in + 5, 6 * sizeof (float)) == 0);

    // Overfill clamps to capacity (7), data intact
    CHECK (rb.write (in, 16) == 7);
    CHECK (rb.read (out, 16) == 7);
    CHECK (std::memcmp (out, in, 7 * sizeof (float)) == 0);

    // reset() empties
    rb.write (in, 3);
    rb.reset();
    CHECK (rb.getAvailableData() == 0);
}

//==============================================================================
static std::vector<float> runSmootherSequence()
{
    spatcore::dsp::DelayTargetSmoother s;
    s.prepare (64);   // W = 64 samples -> teleport threshold 192

    // ~50 Hz-style observations at block boundaries; 500 - 120 = 380 > 192
    // exercises the teleport (mute-move-unmute) path.
    static const float targets[] = { 100.0f, 110.0f, 108.0f, 120.0f,
                                     500.0f, 505.0f, 490.0f, 495.0f,
                                     495.0f, 480.0f };
    const int blockSize = 48;

    std::vector<float> stream;
    stream.reserve (2 * blockSize * (sizeof (targets) / sizeof (targets[0])));

    std::int64_t t = 0;
    for (float target : targets)
    {
        s.observe (target, t);
        for (int i = 0; i < blockSize; ++i)
        {
            const auto o = s.smoothedAt (t + i);
            stream.push_back (o.delay);
            stream.push_back (o.gain);
        }
        t += blockSize;
    }
    return stream;
}

static void testDelayTargetSmootherDeterminism()
{
    const auto a = runSmootherSequence();
    const auto b = runSmootherSequence();

    CHECK (! a.empty());
    CHECK (a.size() == b.size());
    CHECK (std::memcmp (a.data(), b.data(), a.size() * sizeof (float)) == 0);

    // Basic sanity on the stream: finite delays, gains within [0, 1]
    for (size_t i = 0; i + 1 < a.size(); i += 2)
    {
        CHECK (std::isfinite (a[i]));
        CHECK (a[i + 1] >= 0.0f && a[i + 1] <= 1.0f);
    }
}

//==============================================================================
static void testRtSnapshot()
{
    struct Pod
    {
        float gain;
        int index;
        double position;
    };

    // Instantiation is the compile-proof of RtSnapshot's internal
    // static_assert(std::is_trivially_copyable_v<T>).
    spatcore::rt::RtSnapshot<Pod> snap;

    // Default-constructed snapshot is value-initialized (T value {})
    const Pod def = snap.acquire();
    CHECK (bitEqualFloat (def.gain, 0.0f));
    CHECK (def.index == 0);
    CHECK (def.position == 0.0);

    snap.publish ({ 1.5f, 42, -2.25 });
    const Pod got = snap.acquire();
    CHECK (bitEqualFloat (got.gain, 1.5f));
    CHECK (got.index == 42);
    CHECK (got.position == -2.25);

    // Second publish overwrites
    snap.publish ({ 0.25f, -7, 12.5 });
    const Pod got2 = snap.acquire();
    CHECK (bitEqualFloat (got2.gain, 0.25f));
    CHECK (got2.index == -7);
    CHECK (got2.position == 12.5);
}

//==============================================================================
static void testOscRoundtrip()
{
    juce::OSCMessage msg (juce::OSCAddressPattern ("/spatcore/test"));
    msg.addInt32 (42);
    msg.addFloat32 (3.5f);
    msg.addString ("hello");

    const juce::MemoryBlock bytes = WFSNetwork::OSCSerializer::serializeMessage (msg);
    CHECK (bytes.getSize() > 0);
    CHECK (bytes.getSize() % 4 == 0);   // OSC packets are 4-byte aligned

    int pos = 0;
    const juce::OSCMessage parsed = WFSNetwork::OSCParser::parseMessage (
        static_cast<const char*> (bytes.getData()),
        static_cast<int> (bytes.getSize()), pos);

    CHECK (pos == static_cast<int> (bytes.getSize()));
    CHECK (parsed.getAddressPattern().toString() == "/spatcore/test");
    CHECK (parsed.size() == 3);
    CHECK (parsed[0].isInt32() && parsed[0].getInt32() == 42);
    CHECK (parsed[1].isFloat32() && bitEqualFloat (parsed[1].getFloat32(), 3.5f));
    CHECK (parsed[2].isString() && parsed[2].getString() == "hello");

    // Encode(decode(x)) is byte-identical
    const juce::MemoryBlock bytes2 = WFSNetwork::OSCSerializer::serializeMessage (parsed);
    CHECK (bytes2 == bytes);
}

//==============================================================================
int main()
{
    try
    {
        testLockFreeRingBuffer();
        testDelayTargetSmootherDeterminism();
        testRtSnapshot();
        testOscRoundtrip();
    }
    catch (const std::exception& e)
    {
        std::fprintf (stderr, "FAIL: unexpected exception: %s\n", e.what());
        ++failures;
    }

    if (failures == 0)
    {
        std::printf ("spatcore-tests: all tests passed\n");
        return 0;
    }

    std::fprintf (stderr, "spatcore-tests: %d check(s) FAILED\n", failures);
    return 1;
}
