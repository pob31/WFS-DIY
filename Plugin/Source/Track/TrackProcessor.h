#pragma once

#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Shared/BridgeLoader.h"
#include "../Shared/VariantConfig.h"

namespace wfs::plugin
{
    enum class TrackWidget
    {
        LogSlider,        // horizontal slider, log skew, dB suffix
        LinearSlider,     // horizontal slider, linear, unit suffix
        RotaryDial,       // rotary knob
        BidirectionalBar, // horizontal slider with centre-zero visual
        Toggle            // ON/OFF button
    };

    struct NonPositionParamSpec
    {
        juce::String paramID;
        juce::String label;
        juce::String oscPath;
        bool isInteger;
        float minValue;
        float maxValue;
        float defaultValue;
        juce::String unit;
        TrackWidget widget;
        float skewMidpoint;   // 0 = linear; only applied for float params
    };

    const std::array<NonPositionParamSpec, 6>& getSharedTrackParams();

    class TrackProcessor  : public juce::AudioProcessor,
                            private juce::AudioProcessorValueTreeState::Listener
    {
    public:
        explicit TrackProcessor (VariantConfig cfg);
        ~TrackProcessor() override;

        const juce::String getName() const override            { return variant.productName; }
        bool   acceptsMidi() const override                    { return false; }
        bool   producesMidi() const override                   { return false; }
        bool   isMidiEffect() const override                   { return false; }
        double getTailLengthSeconds() const override           { return 0.0; }
        int    getNumPrograms() override                       { return 1; }
        int    getCurrentProgram() override                    { return 0; }
        void   setCurrentProgram (int) override                {}
        const juce::String getProgramName (int) override       { return {}; }
        void   changeProgramName (int, const juce::String&) override {}

        void prepareToPlay (double, int) override;
        void releaseResources() override;
        bool isBusesLayoutSupported (const BusesLayout&) const override;
        void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

        bool hasEditor() const override                        { return true; }
        juce::AudioProcessorEditor* createEditor() override;

        void getStateInformation (juce::MemoryBlock& destData) override;
        void setStateInformation (const void* data, int sizeInBytes) override;

        juce::AudioProcessorValueTreeState& getState() noexcept { return state; }
        const VariantConfig& getVariant() const noexcept        { return variant; }

        int  getInputId() const;

    private:
        void parameterChanged (const juce::String& paramID, float newValue) override;

        static void inboundCallback (void* user, const char* oscPath, int channelId, double value);
        juce::AudioProcessorValueTreeState::ParameterLayout buildLayout() const;

        VariantConfig variant;
        juce::AudioProcessorValueTreeState state;
        WfsBridgeTrackHandle* bridgeHandle = nullptr;
        std::atomic<bool> isApplyingRemoteChange { false };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackProcessor)
    };
}
