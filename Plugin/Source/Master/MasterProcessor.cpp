#include "MasterProcessor.h"
#include "MasterEditor.h"

namespace wfs::plugin
{
    const std::vector<juce::String>& MasterProcessor::sharedNonPositionPaths()
    {
        static const std::vector<juce::String> paths = {
            "/wfs/input/attenuation",
            "/wfs/input/attenuationLaw",
            "/wfs/input/distanceAttenuation",
            "/wfs/input/distanceRatio",
            "/wfs/input/directivity",
            "/wfs/input/rotation",
            "/wfs/input/tilt",
            "/wfs/input/HFshelf",
            "/wfs/input/LFOactive"
        };
        return paths;
    }

    const std::vector<juce::String>& MasterProcessor::positionPathsFor (const juce::String& variantTag)
    {
        // All native variants exchange positions with the app in Cartesian
        // X/Y/Z — the app's OSC router only accepts those, and its OSCQuery
        // tree only publishes those. Tracks convert to/from their native
        // coordinate system locally before/after sending.
        static const std::vector<juce::String> none;
        static const std::vector<juce::String> nativeXYZ = {
            "/wfs/input/positionX",
            "/wfs/input/positionY",
            "/wfs/input/positionZ"
        };
        if (variantTag == "cartesian"
            || variantTag == "cylindrical"
            || variantTag == "spherical")
            return nativeXYZ;
        return none;
    }

    void MasterProcessor::bridgeOutboundCallback (void* user, const char* oscPath, int channelId, double value)
    {
        if (user == nullptr)
            return;
        auto* self = static_cast<MasterProcessor*> (user);
        if (! self->isConnected())
            return;
        const auto path    = juce::String::fromUTF8 (oscPath);
        const auto epsilon = 0.0001f;
        self->rateLimiter.post (path, channelId, static_cast<float> (value), epsilon);
    }

    void MasterProcessor::bridgeLifecycleCallback (void* user, int inputId,
                                                   const char* variantTag, int isRegister)
    {
        if (user == nullptr)
            return;
        auto* self = static_cast<MasterProcessor*> (user);
        const juce::String tag = variantTag != nullptr ? juce::String::fromUTF8 (variantTag) : juce::String();
        if (isRegister)
            self->onTrackRegistered (inputId, tag);
        else
            self->onTrackUnregistered (inputId);
    }

    juce::AudioProcessorValueTreeState::ParameterLayout MasterProcessor::buildLayout()
    {
        using namespace juce;
        AudioProcessorValueTreeState::ParameterLayout layout;
        layout.add (std::make_unique<AudioParameterBool> ("enabled", "Enabled", true));
        return layout;
    }

    MasterProcessor::MasterProcessor()
        : AudioProcessor (BusesProperties()
                              .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                              .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
          state (*this, nullptr, "MasterState", buildLayout())
    {
        rateLimiter.setSendFunction ([this] (const juce::String& path, int chan, float value)
        {
            transport.sendFloat (path, chan, value);
        });
        query.setOscCallback ([this] (const juce::String& path, float value)
        {
            onQueryOscPush (path, value);
        });
    }

    MasterProcessor::~MasterProcessor()
    {
        disconnectFromApp();
    }

    void MasterProcessor::prepareToPlay (double, int)
    {
        auto& loader = BridgeLoader::getInstance();
        if (loader.ensureLoaded() && bridgeHandle == nullptr)
            bridgeHandle = loader.masterRegister (this,
                                                  &bridgeOutboundCallback,
                                                  &bridgeLifecycleCallback);
    }

    void MasterProcessor::releaseResources()
    {
        auto& loader = BridgeLoader::getInstance();
        if (bridgeHandle != nullptr && loader.isLoaded())
            loader.masterUnregister (bridgeHandle);
        bridgeHandle = nullptr;
    }

    bool MasterProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
    {
        const auto& in  = layouts.getMainInputChannelSet();
        const auto& out = layouts.getMainOutputChannelSet();
        if (in != out) return false;
        if (in == juce::AudioChannelSet::disabled()) return false;
        return in == juce::AudioChannelSet::mono()
            || in == juce::AudioChannelSet::stereo();
    }

    void MasterProcessor::processBlock (juce::AudioBuffer<float>& /*buffer*/, juce::MidiBuffer&)
    {
        // Stereo audio passes through unchanged — Master has no DSP.
    }

    juce::AudioProcessorEditor* MasterProcessor::createEditor()
    {
        return new MasterEditor (*this);
    }

    void MasterProcessor::getStateInformation (juce::MemoryBlock& destData)
    {
        if (auto xml = state.copyState().createXml())
            copyXmlToBinary (*xml, destData);
    }

    void MasterProcessor::setStateInformation (const void* data, int sizeInBytes)
    {
        if (auto xml = getXmlFromBinary (data, sizeInBytes))
            state.replaceState (juce::ValueTree::fromXml (*xml));
    }

    bool MasterProcessor::connectToApp (const juce::String& host, int udpPort, int httpPort)
    {
        if (! transport.connect (host, udpPort))
            return false;
        if (! query.connect (host, httpPort))
        {
            transport.disconnect();
            return false;
        }

        // Re-subscribe for any Tracks that registered before we connected.
        // The lifecycle callback fired at bridge-register time recorded their
        // variant tags; push the subscriptions now that WebSocket is up.
        std::map<int, juce::String> knownTracks;
        {
            std::lock_guard<std::mutex> sl (lock);
            knownTracks = subscribedInputs;
            subscribedInputs.clear();
        }
        for (auto& [inputId, tag] : knownTracks)
            subscribeInput (inputId, tag);
        return true;
    }

    void MasterProcessor::disconnectFromApp()
    {
        query.disconnect();
        transport.disconnect();
    }

    bool MasterProcessor::isConnected() const
    {
        return transport.isConnected();
    }

    int MasterProcessor::getRegisteredTrackCount() const
    {
        auto& loader = BridgeLoader::getInstance();
        return loader.isLoaded() && loader.trackCount ? loader.trackCount() : 0;
    }

    juce::String MasterProcessor::getConnectionStatus() const
    {
        switch (query.getState())
        {
            case OscQueryClient::State::Idle:        return "Disconnected";
            case OscQueryClient::State::Connecting:  return juce::String::fromUTF8 ("Connecting\xe2\x80\xa6");
            case OscQueryClient::State::Handshaking: return juce::String::fromUTF8 ("Handshaking\xe2\x80\xa6");
            case OscQueryClient::State::Ready:       return "Connected";
            case OscQueryClient::State::Error:       return "Connection error";
        }
        return {};
    }

    void MasterProcessor::subscribeInput (int inputId, const juce::String& variantTag)
    {
        {
            std::lock_guard<std::mutex> sl (lock);
            auto [it, inserted] = subscribedInputs.emplace (inputId, variantTag);
            if (! inserted)
                it->second = variantTag;
        }

        // OscQueryClient::listen() records the path and sends the LISTEN
        // command lazily once the WebSocket is ready, so we can safely call
        // it regardless of current connection state. After subscribing, also
        // fetch the current value over HTTP so the plugin shows fresh state
        // immediately — the app's OSCQuery server doesn't push on LISTEN.
        const bool wsReady = (query.getState() == OscQueryClient::State::Ready);

        auto subscribe = [&] (const juce::String& base)
        {
            const auto tail = base.fromFirstOccurrenceOf ("/wfs/input", false, false);
            const auto fullPath = "/wfs/input/" + juce::String (inputId) + tail;
            query.listen (fullPath);
            if (wsReady)
                query.fetchCurrentValue (fullPath);
        };

        for (const auto& base : sharedNonPositionPaths())
            subscribe (base);
        for (const auto& base : positionPathsFor (variantTag))
            subscribe (base);
    }

    void MasterProcessor::unsubscribeInput (int inputId)
    {
        juce::String tag;
        {
            std::lock_guard<std::mutex> sl (lock);
            auto it = subscribedInputs.find (inputId);
            if (it == subscribedInputs.end())
                return;
            tag = it->second;
            subscribedInputs.erase (it);
        }

        auto unsubscribe = [&] (const juce::String& base)
        {
            const auto tail = base.fromFirstOccurrenceOf ("/wfs/input", false, false);
            const auto fullPath = "/wfs/input/" + juce::String (inputId) + tail;
            query.ignore (fullPath);
        };

        for (const auto& base : sharedNonPositionPaths())
            unsubscribe (base);
        for (const auto& base : positionPathsFor (tag))
            unsubscribe (base);
    }

    void MasterProcessor::onTrackRegistered (int inputId, const juce::String& variantTag)
    {
        subscribeInput (inputId, variantTag);
    }

    void MasterProcessor::onTrackUnregistered (int inputId)
    {
        unsubscribeInput (inputId);
    }

    void MasterProcessor::onQueryOscPush (const juce::String& oscPath, float value)
    {
        // Server pushes in OSCQuery canonical form: /wfs/input/<id>/<param>.
        // Strip the input id out so Track plugins receive the generic path that
        // matches their parameter table.
        if (! oscPath.startsWith ("/wfs/input/"))
            return;

        const auto tail = oscPath.fromFirstOccurrenceOf ("/wfs/input/", false, false);
        const int slashIdx = tail.indexOf ("/");
        if (slashIdx <= 0)
            return;

        const juce::String idStr = tail.substring (0, slashIdx);
        if (! idStr.containsOnly ("0123456789"))
            return;
        const int inputId = idStr.getIntValue();
        const juce::String genericPath = "/wfs/input/" + tail.substring (slashIdx + 1);

        auto& loader = BridgeLoader::getInstance();
        if (! loader.isLoaded() || loader.masterDispatch == nullptr || bridgeHandle == nullptr)
            return;

        loader.masterDispatch (bridgeHandle, inputId,
                               genericPath.toRawUTF8(),
                               static_cast<double> (value));
    }
}
