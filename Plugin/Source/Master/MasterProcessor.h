#pragma once

#include <map>
#include <mutex>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_osc/juce_osc.h>
#include "../Shared/BridgeLoader.h"
#include "../Shared/OscTransport.h"
#include "../Shared/OscQueryClient.h"
#include "../Shared/RateLimiter.h"
#include "../Shared/DiagnosticLog.h"
#include "../Shared/TargetProfile.h"

namespace wfs::plugin
{
    class MasterProcessor  : public juce::AudioProcessor,
                             private juce::OSCReceiver::Listener<juce::OSCReceiver::MessageLoopCallback>
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

        bool connectToApp (const juce::String& host, int udpPort, int httpPort, int admRxPort);
        void disconnectFromApp();
        bool isConnected() const;
        int  getRegisteredTrackCount() const;
        juce::String getConnectionStatus() const;
        bool isOpenLoop() const;  // true when the active profile's flow is SendOnly

        TargetProfileRegistry&       getProfileRegistry()       noexcept { return profileRegistry; }
        const TargetProfileRegistry& getProfileRegistry() const noexcept { return profileRegistry; }

        const DiagnosticLog& getDiagnosticLog() const noexcept { return diagLog; }
        static juce::String  getBuildStamp();

    private:
        static void bridgeOutboundCallback   (void* user, const char* oscPath, int channelId, double value);
        static void bridgeOutbound3fCallback (void* user, const char* oscPath, double v1, double v2, double v3);
        static void bridgeLifecycleCallback  (void* user, int inputId, const char* variantTag, int isRegister);

        void oscMessageReceived (const juce::OSCMessage& message) override;
        void dispatchAdmInbound (const juce::OSCMessage& msg);

        void onQueryOscPush (const juce::String& oscPath, float value);
        void onQueryStructureChanged (const juce::String& containerPath);
        void onTrackRegistered (int inputId, const juce::String& variantTag);
        void onTrackUnregistered (int inputId);
        void subscribeInput (int inputId, const juce::String& variantTag);
        void unsubscribeInput (int inputId);

        // --- Channel inventory ---
        // What the app's input namespace currently holds. A permanent channel
        // number is not an index: the list has gaps after deletions and its
        // display order is unrelated to its numbering, so a Track cannot infer
        // from its own inputId whether that channel exists, what it is called, or
        // whether the stereo image parameters apply to it. This is the plugin's
        // equivalent of the /remote/channelList the Android remote gets.
        struct ChannelInfo { juce::String name; bool stereo = false; };
        void refreshChannelInventory();
        void publishChannelInfo (int inputId);
        void publishChannelInfoToAllTracks();

        // refreshChannelInventory blocks on HTTP. PATH_CHANGED arrives on the
        // WebSocket's own thread, which is also the one delivering inbound OSC
        // value pushes, so fetching there would stall parameter feedback for as
        // long as the request takes -- up to the 3 s socket timeout when the app
        // has gone away, which is exactly when a notification is most likely.
        // The message thread is no better: that one freezes the DAW's UI. So the
        // refresh runs on a worker owned by this processor, which means it is
        // joined in the destructor and cannot outlive the members it touches.
        class InventoryWorker  : public juce::Thread
        {
        public:
            explicit InventoryWorker (MasterProcessor& o)
                : juce::Thread ("WFS-DIY inventory"), owner (o) {}
            void trigger()          { pending = true; notify(); }
            void run() override;
        private:
            MasterProcessor&  owner;
            std::atomic<bool> pending { false };
        };
        InventoryWorker inventoryWorker { *this };

        static juce::AudioProcessorValueTreeState::ParameterLayout buildLayout();
        static const std::vector<juce::String>& sharedNonPositionPaths();
        static const std::vector<juce::String>& positionPathsFor (const juce::String& variantTag);

        juce::AudioProcessorValueTreeState state;
        OscTransport    transport;
        OscQueryClient  query;
        RateLimiter     rateLimiter;
        juce::OSCReceiver admReceiver;
        bool              admReceiverOpen = false;
        WfsBridgeMasterHandle* bridgeHandle = nullptr;

        TargetProfileRegistry   profileRegistry;
        TargetProfileTranslator translator { profileRegistry };

        void dispatchOutEvent (const OutEvent& evt);

        std::mutex  lock;
        std::map<int, juce::String> subscribedInputs;
        std::map<int, ChannelInfo>  channelInventory;   // key = permanent channel number
        bool                        inventoryKnown = false;
        DiagnosticLog diagLog;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MasterProcessor)
    };
}
