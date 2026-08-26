#pragma once

#include <JuceHeader.h>
#include "../../spatcore/dsp/StereoDecomposer.h"
#include "../../spatcore/rt/RtSnapshot.h"
#include "../../spatcore/wfs/RenderSourceMap.h"

//==============================================================================
/**
    App-side owner of the stereo-pair decomposition backends.

    One decomposer per stereo ordinal (the k-th stereo channel in input order,
    matching RenderSourceMap's derived-slot layout), plus the two RtSnapshot
    hand-offs per channel:

      - config DOWN (message thread → audio thread): width factor, active
        slice count, hop stagger. Published from the 50 Hz timer, acquired
        once per block in processChannel().
      - slice state UP (audio thread → message thread): per-slice azimuth /
        gain / confidence / active, for the 50 Hz slice-geometry refresh and
        the level-meter diagnostics. This runs RtSnapshot in the reverse of
        its documented direction, which is mechanically identical (the lock
        window is one POD copy) — the publisher is simply the audio thread.

    Phase 0 ships PassThroughStereoDecomposer (slice 0 = centre, active but
    silent; slice 1 = L, slice 2 = R; zero latency); Phase 1 swaps the backend
    behind the same interface and this class does not change.

    RT contract: processChannel() is noexcept, allocation- and log-free; all
    state is preallocated here and in prepare().
*/
class StereoChannelManager
{
public:
    static constexpr int kMaxSlices = spatcore::dsp::StereoDecomposer::kMaxSlices;
    static constexpr int kMaxStereoChannels = spatcore::wfs::RenderSourceMap::kMaxStereoChannels;

    /** POD wrapper so the slice-state array can ride an RtSnapshot. */
    struct SliceStates
    {
        spatcore::dsp::StereoSliceState slices[kMaxSlices];
    };

    StereoChannelManager()
    {
        for (auto& ch : channels)
        {
            ch.decomposer = std::make_unique<spatcore::dsp::PassThroughStereoDecomposer>();

            // The 50 Hz geometry refresh runs whether or not an audio device
            // ever opens, and a default-constructed SliceStates reads as an
            // inactive zero-width image — the Map would draw a stereo pair with
            // no spread on a machine with no interface. getSliceState() does not
            // depend on prepare(), so the backend's resting image is publishable
            // from here.
            SliceStates st {};
            ch.decomposer->getSliceState (st.slices);
            ch.stateSnapshot.publish (st);
        }
    }

    /** Non-RT (prepareToPlay). Prepares every ordinal — cheap for the
        pass-through backend, and it means a stereo channel configured while
        stopped needs no re-prepare on start. Republishes the slice states the
        constructor seeded, now from the prepared backend. */
    void prepare (double sampleRate, int maxBlockSize)
    {
        for (int k = 0; k < kMaxStereoChannels; ++k)
        {
            auto& ch = channels[k];

            auto cfg = ch.configSnapshot.acquire();
            cfg.staggerIndex = k;
            cfg.staggerCount = kMaxStereoChannels;

            ch.prepared = ch.decomposer->prepare (sampleRate, maxBlockSize, cfg);
            ch.decomposer->reset();

            SliceStates st {};
            ch.decomposer->getSliceState (st.slices);
            ch.stateSnapshot.publish (st);
        }
    }

    /** Message thread: hand a fresh config to ordinal k's audio-side
        decomposer. Values outside the prepared envelope are clamped by the
        backend, not honoured. */
    void publishConfig (int ordinal, const spatcore::dsp::StereoDecomposerConfig& cfg)
    {
        if (ordinal < 0 || ordinal >= kMaxStereoChannels)
            return;
        channels[static_cast<size_t> (ordinal)].configSnapshot.publish (cfg);
    }

    /** Audio thread. Decomposes one stereo channel's raw L/R into its
        kMaxSlices slice buffers (sliceOut[0] is the channel's primary slot).
        Inactive slots come back cleared (decomposer contract). If the ordinal
        was never prepared, all slots are cleared instead — never stale. */
    void processChannel (int ordinal, const float* left, const float* right,
                         float* const* sliceOut, int numSamples) noexcept
    {
        if (ordinal < 0 || ordinal >= kMaxStereoChannels || numSamples <= 0)
            return;

        auto& ch = channels[static_cast<size_t> (ordinal)];

        if (! ch.prepared)
        {
            for (int k = 0; k < kMaxSlices; ++k)
                juce::FloatVectorOperations::clear (sliceOut[k], numSamples);
            return;
        }

        ch.decomposer->setConfig (ch.configSnapshot.acquire());
        ch.decomposer->process (left, right, sliceOut, numSamples);

        SliceStates st {};
        ch.decomposer->getSliceState (st.slices);
        ch.stateSnapshot.publish (st);
    }

    /** Message thread: latest slice states published by the audio thread
        (or by prepare() before audio ever ran). */
    SliceStates getSliceStates (int ordinal) const
    {
        if (ordinal < 0 || ordinal >= kMaxStereoChannels)
            return {};
        return channels[static_cast<size_t> (ordinal)].stateSnapshot.acquire();
    }

    /** Intrinsic latency of ordinal k's backend (0 for pass-through). Feeds
        the render-latency reference hook. */
    float getLatencyMs (int ordinal) const
    {
        if (ordinal < 0 || ordinal >= kMaxStereoChannels)
            return 0.0f;
        return channels[static_cast<size_t> (ordinal)].decomposer->getLatencyMs();
    }

private:
    struct Channel
    {
        std::unique_ptr<spatcore::dsp::StereoDecomposer> decomposer;
        spatcore::rt::RtSnapshot<spatcore::dsp::StereoDecomposerConfig> configSnapshot;
        spatcore::rt::RtSnapshot<SliceStates> stateSnapshot;
        bool prepared = false;
    };

    std::array<Channel, kMaxStereoChannels> channels;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StereoChannelManager)
};
