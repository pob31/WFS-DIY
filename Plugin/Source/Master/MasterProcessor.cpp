#include "MasterProcessor.h"
#include "MasterEditor.h"

namespace wfs::plugin
{
    juce::String MasterProcessor::getBuildStamp()
    {
        return juce::String (__DATE__) + " " + juce::String (__TIME__);
    }
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

    void MasterProcessor::dispatchOutEvent (const OutEvent& evt)
    {
        if (evt.isThreeFloat)
        {
            // 3f messages bypass the rate limiter (combined ADM triples and
            // custom 3f sends are inherently low-frequency).
            transport.sendFloats3 (evt.path, evt.v1, evt.v2, evt.v3);
        }
        else
        {
            const auto epsilon = 0.0001f;
            rateLimiter.post (evt.path, evt.channelId, evt.v1, epsilon);
        }
    }

    void MasterProcessor::bridgeOutboundCallback (void* user, const char* oscPath, int channelId, double value)
    {
        if (user == nullptr)
            return;
        auto* self = static_cast<MasterProcessor*> (user);
        if (! self->isConnected())
            return;
        const auto path = juce::String::fromUTF8 (oscPath);

        std::vector<OutEvent> events;
        events.reserve (4);
        self->translator.translate1f (path, channelId, static_cast<float> (value), events);
        for (const auto& e : events)
            self->dispatchOutEvent (e);
    }

    void MasterProcessor::bridgeOutbound3fCallback (void* user, const char* oscPath,
                                                    double v1, double v2, double v3)
    {
        if (user == nullptr)
            return;
        auto* self = static_cast<MasterProcessor*> (user);
        if (! self->isConnected())
            return;

        std::vector<OutEvent> events;
        events.reserve (2);
        self->translator.translate3f (juce::String::fromUTF8 (oscPath),
                                      static_cast<float> (v1),
                                      static_cast<float> (v2),
                                      static_cast<float> (v3),
                                      events);
        for (const auto& e : events)
            self->dispatchOutEvent (e);
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
        {
            bridgeHandle = loader.masterRegister (this,
                                                  &bridgeOutboundCallback,
                                                  &bridgeLifecycleCallback);
            if (bridgeHandle != nullptr && loader.masterSetOutbound3f != nullptr)
                loader.masterSetOutbound3f (bridgeHandle, &bridgeOutbound3fCallback);
        }
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
        auto root = state.copyState();
        // Strip any prior profile child so we don't accumulate duplicates.
        root.removeChild (root.getChildWithName ("TargetProfileState"), nullptr);
        root.appendChild (profileRegistry.toState(), nullptr);
        if (auto xml = root.createXml())
            copyXmlToBinary (*xml, destData);
    }

    void MasterProcessor::setStateInformation (const void* data, int sizeInBytes)
    {
        if (auto xml = getXmlFromBinary (data, sizeInBytes))
        {
            auto restored = juce::ValueTree::fromXml (*xml);
            const auto profileNode = restored.getChildWithName ("TargetProfileState");
            if (profileNode.isValid())
            {
                profileRegistry.fromState (profileNode);
                restored.removeChild (profileNode, nullptr);
            }
            state.replaceState (restored);
        }
    }

    bool MasterProcessor::connectToApp (const juce::String& host, int udpPort, int httpPort, int admRxPort)
    {
        if (! transport.connect (host, udpPort))
            return false;

        const auto& profile = profileRegistry.active();
        const bool wantOscQuery = (profile.flow == ProfileFlow::Bidirectional);
        const bool wantAdmRx    = (profile.flow == ProfileFlow::Bidirectional)
                                || (profile.flow == ProfileFlow::SendOnly && profile.admEchoEnabled);

        if (wantOscQuery)
        {
            if (! query.connect (host, httpPort))
            {
                transport.disconnect();
                return false;
            }
        }
        else
        {
            diagLog.add ("Open-loop profile (" + profile.displayName
                         + ") - skipping OSCQuery handshake");
        }

        // ADM-OSC isn't exposed over OSCQuery, so we need a dedicated UDP
        // receiver for inbound /adm/obj/N/... messages from the app. For
        // SendOnly profiles this is opt-in via TargetProfile::admEchoEnabled.
        if (wantAdmRx && admRxPort > 0)
        {
            if (admReceiverOpen)
            {
                admReceiver.removeListener (this);
                admReceiver.disconnect();
                admReceiverOpen = false;
            }
            if (admReceiver.connect (admRxPort))
            {
                admReceiver.addListener (this);
                admReceiverOpen = true;
                diagLog.add ("ADM-OSC listening on port " + juce::String (admRxPort));
            }
            else
            {
                diagLog.add ("ADM-OSC bind FAILED on port " + juce::String (admRxPort));
            }
        }

        // Re-subscribe for any Tracks that registered before we connected.
        // The lifecycle callback fired at bridge-register time recorded their
        // variant tags; push the subscriptions now that WebSocket is up.
        if (wantOscQuery)
        {
            std::map<int, juce::String> knownTracks;
            {
                std::lock_guard<std::mutex> sl (lock);
                knownTracks = subscribedInputs;
                subscribedInputs.clear();
            }
            for (auto& [inputId, tag] : knownTracks)
                subscribeInput (inputId, tag);
        }
        return true;
    }

    void MasterProcessor::disconnectFromApp()
    {
        if (admReceiverOpen)
        {
            admReceiver.removeListener (this);
            admReceiver.disconnect();
            admReceiverOpen = false;
        }
        query.disconnect();
        transport.disconnect();
    }

    bool MasterProcessor::isConnected() const
    {
        return transport.isConnected();
    }

    bool MasterProcessor::isOpenLoop() const
    {
        return profileRegistry.active().flow == ProfileFlow::SendOnly;
    }

    int MasterProcessor::getRegisteredTrackCount() const
    {
        auto& loader = BridgeLoader::getInstance();
        return loader.isLoaded() && loader.trackCount ? loader.trackCount() : 0;
    }

    juce::String MasterProcessor::getConnectionStatus() const
    {
        if (profileRegistry.active().flow == ProfileFlow::SendOnly)
            return transport.isConnected() ? "Open loop (one-way)" : "Disconnected";

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
        translator.setVariantTag (inputId, variantTag);
        subscribeInput (inputId, variantTag);
    }

    void MasterProcessor::onTrackUnregistered (int inputId)
    {
        translator.clearVariantTag (inputId);
        unsubscribeInput (inputId);
    }

    void MasterProcessor::oscMessageReceived (const juce::OSCMessage& message)
    {
        dispatchAdmInbound (message);
    }

    void MasterProcessor::dispatchAdmInbound (const juce::OSCMessage& msg)
    {
        const juce::String addr = msg.getAddressPattern().toString();
        if (! addr.startsWith ("/adm/obj/"))
            return;

        // /adm/obj/<id>/<param>
        const auto tail = addr.fromFirstOccurrenceOf ("/adm/obj/", false, false);
        const int slashIdx = tail.indexOf ("/");
        if (slashIdx <= 0)
            return;
        const juce::String idStr = tail.substring (0, slashIdx);
        if (! idStr.containsOnly ("0123456789"))
            return;
        const int inputId = idStr.getIntValue();
        const juce::String param = tail.substring (slashIdx + 1);

        auto argFloat = [] (const juce::OSCArgument& a) -> float
        {
            if (a.isFloat32())  return a.getFloat32();
            if (a.isInt32())    return static_cast<float> (a.getInt32());
            return 0.0f;
        };

        auto& loader = BridgeLoader::getInstance();
        if (! loader.isLoaded() || bridgeHandle == nullptr)
            return;

        // Combined triples first.
        if ((param == "xyz" || param == "aed") && msg.size() >= 3 && loader.masterDispatch3f != nullptr)
        {
            diagLog.add ("Rx /adm/obj/" + juce::String (inputId) + "/" + param
                         + " (" + juce::String (argFloat (msg[0]), 3)
                         + ", " + juce::String (argFloat (msg[1]), 3)
                         + ", " + juce::String (argFloat (msg[2]), 3) + ")");
            loader.masterDispatch3f (bridgeHandle, inputId, addr.toRawUTF8(),
                                     argFloat (msg[0]),
                                     argFloat (msg[1]),
                                     argFloat (msg[2]));
            return;
        }

        if (param == "xy" && msg.size() >= 2 && loader.masterDispatch3f != nullptr)
        {
            loader.masterDispatch3f (bridgeHandle, inputId, addr.toRawUTF8(),
                                     argFloat (msg[0]),
                                     argFloat (msg[1]),
                                     0.0f);
            return;
        }

        // Per-axis individual messages fall through to the 1f dispatcher so
        // tracks can update single axes piecemeal.
        if (msg.size() >= 1 && loader.masterDispatch != nullptr)
        {
            loader.masterDispatch (bridgeHandle, inputId, addr.toRawUTF8(),
                                   static_cast<double> (argFloat (msg[0])));
        }
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
