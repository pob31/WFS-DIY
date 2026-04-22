#pragma once

#include <map>
#include <mutex>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Shared/BridgeLoader.h"
#include "../Shared/OscTransport.h"
#include "../Shared/OscQueryClient.h"
#include "../Shared/RateLimiter.h"

namespace wfs::plugin
{
    class MasterProcessor  : public juce::AudioProcessor
    {
    public:
        MasterProcessor();
        ~MasterProcessor() override;

        const juce::String getName() const override            { return "WFS-DIY Master"; }
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

        bool connectToApp (const juce::String& host, int udpPort, int httpPort);
        void disconnectFromApp();
        bool isConnected() const;
        int  getRegisteredTrackCount() const;
        juce::String getConnectionStatus() const;

    private:
        static void bridgeOutboundCallback (void* user, const char* oscPath, int channelId, double value);
        static void bridgeLifecycleCallback (void* user, int inputId, const char* variantTag, int isRegister);

        void onQueryOscPush (const juce::String& oscPath, float value);
        void onTrackRegistered (int inputId, const juce::String& variantTag);
        void onTrackUnregistered (int inputId);
        void subscribeInput (int inputId, const juce::String& variantTag);
        void unsubscribeInput (int inputId);

        static juce::AudioProcessorValueTreeState::ParameterLayout buildLayout();
        static const std::vector<juce::String>& sharedNonPositionPaths();
        static const std::vector<juce::String>& positionPathsFor (const juce::String& variantTag);

        juce::AudioProcessorValueTreeState state;
        OscTransport    transport;
        OscQueryClient  query;
        RateLimiter     rateLimiter;
        WfsBridgeMasterHandle* bridgeHandle = nullptr;

        std::mutex  lock;
        std::map<int, juce::String> subscribedInputs;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MasterProcessor)
    };
}
