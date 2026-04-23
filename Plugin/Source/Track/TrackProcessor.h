#pragma once

#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Shared/BridgeLoader.h"
#include "../Shared/VariantConfig.h"
#include "../Shared/DiagnosticLog.h"

namespace wfs::plugin
{
    enum class TrackWidget
    {
        LogSlider,        // horizontal slider, log skew, dB suffix
        LinearSlider,     // horizontal slider, linear, unit suffix
        RotaryDial,       // rotary knob
        BidirectionalBar, // horizontal slider with centre-zero visual
        Toggle,           // ON/OFF button
        TwoStateCombo     // ComboBox with two mutually-exclusive choices
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

    const std::array<NonPositionParamSpec, 9>& getSharedTrackParams();

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
        int  getAttenuationLaw() const;

        const DiagnosticLog& getDiagnosticLog() const noexcept { return diagLog; }
        static juce::String  getBuildStamp();

    private:
        // All variants exchange positions with the app in Cartesian X/Y/Z
        // (that's the only coordinate system the app's OSC router accepts
        // and the only one exposed by OSCQuery). The plugin's DAW-facing
        // parameters stay in the variant's native system (R/Theta/Z for
        // Cyl, R/Theta/Phi for Sph); conversion happens at the app boundary.
        void displayToCartesian (float d0, float d1, float d2,
                                 float& x, float& y, float& z) const;
        void cartesianToDisplay (float x, float y, float z,
                                 float& d0, float& d1, float& d2) const;
        void recomputeCartesianFromDisplay();
        void updateDisplayFromCartesian();
        void sendCartesianPositionsToApp();

        std::atomic<float> cachedX { 0.0f };
        std::atomic<float> cachedY { 0.0f };
        std::atomic<float> cachedZ { 0.0f };

        // Last ADM position triple received from the app. Used to break
        // feedback loops: when the app echoes our outbound message back
        // (with small drift due to mapping rounding / constraints), the
        // plugin records those values, updates its display params, and
        // then parameterChanged fires a new outbound — which we must
        // suppress if it would just be an echo of what we just received.
        // NaN sentinel == "nothing received yet, never suppress".
        std::atomic<float> lastRxAdmV1 { std::numeric_limits<float>::quiet_NaN() };
        std::atomic<float> lastRxAdmV2 { std::numeric_limits<float>::quiet_NaN() };
        std::atomic<float> lastRxAdmV3 { std::numeric_limits<float>::quiet_NaN() };

        bool isAdmVariant() const noexcept
        {
            return variant.coordinateTag == "adm-cartesian"
                || variant.coordinateTag == "adm-polar";
        }

        juce::String admCombinedPath() const;  // "/adm/obj/<id>/xyz" or "/adm/obj/<id>/aed"
        void sendAdmPositionsToApp();
        static void inbound3fCallback (void* user, const char* oscPath,
                                       int channelId, double v1, double v2, double v3);
        void parameterChanged (const juce::String& paramID, float newValue) override;

        static void inboundCallback (void* user, const char* oscPath, int channelId, double value);
        juce::AudioProcessorValueTreeState::ParameterLayout buildLayout() const;

        VariantConfig variant;
        juce::AudioProcessorValueTreeState state;
        WfsBridgeTrackHandle* bridgeHandle = nullptr;
        std::atomic<bool> isApplyingRemoteChange { false };
        DiagnosticLog diagLog;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackProcessor)
    };
}
