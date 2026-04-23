#include "TrackProcessor.h"
#include "TrackEditor.h"

namespace wfs::plugin
{
    juce::String TrackProcessor::getBuildStamp()
    {
        return juce::String (__DATE__) + " " + juce::String (__TIME__);
    }

    const std::array<NonPositionParamSpec, 9>& getSharedTrackParams()
    {
        static const std::array<NonPositionParamSpec, 9> params = {{
            { "attenuation",         "Attenuation",         "/wfs/input/attenuation",         false, -92.0f,   0.0f,   0.0f, "dB",
              TrackWidget::LogSlider,       -12.0f },
            { "attenuationLaw",      "Attenuation Law",     "/wfs/input/attenuationLaw",      true,    0.0f,   1.0f,   0.0f, "",
              TrackWidget::TwoStateCombo,     0.0f },
            { "distanceAttenuation", "Distance Atten.",     "/wfs/input/distanceAttenuation", false,  -6.0f,   0.0f,  -0.7f, "dB/m",
              TrackWidget::LinearSlider,      0.0f },
            { "distanceRatio",       "Distance Ratio",      "/wfs/input/distanceRatio",       false,   0.1f,  10.0f,   1.0f, "x",
              TrackWidget::LogSlider,         1.0f },
            { "directivity",         "Directivity",         "/wfs/input/directivity",         true,    2.0f, 360.0f, 360.0f, juce::CharPointer_UTF8 ("\xc2\xb0"),
              TrackWidget::LinearSlider,      0.0f },
            { "rotation",            "Rotation",            "/wfs/input/rotation",            true,  -179.0f, 180.0f,   0.0f, juce::CharPointer_UTF8 ("\xc2\xb0"),
              TrackWidget::RotaryDial,        0.0f },
            { "tilt",                "Tilt",                "/wfs/input/tilt",                true,   -90.0f,  90.0f,   0.0f, juce::CharPointer_UTF8 ("\xc2\xb0"),
              TrackWidget::BidirectionalBar,  0.0f },
            { "hfShelf",             "HF Shelf",            "/wfs/input/HFshelf",             false,  -24.0f,   0.0f,  -6.0f, "dB",
              TrackWidget::LogSlider,        -6.0f },
            { "lfoActive",           "LFO Active",          "/wfs/input/LFOactive",           true,     0.0f,   1.0f,   0.0f, "",
              TrackWidget::Toggle,            0.0f }
        }};
        return params;
    }

    juce::AudioProcessorValueTreeState::ParameterLayout TrackProcessor::buildLayout() const
    {
        using namespace juce;
        AudioProcessorValueTreeState::ParameterLayout layout;

        layout.add (std::make_unique<AudioParameterInt> (
            ParameterID ("inputId", 1), "Input ID", 1, 64, 1));

        for (const auto& spec : getSharedTrackParams())
        {
            if (spec.isInteger)
            {
                layout.add (std::make_unique<AudioParameterInt> (
                    ParameterID (spec.paramID, 1), spec.label,
                    static_cast<int> (spec.minValue),
                    static_cast<int> (spec.maxValue),
                    static_cast<int> (spec.defaultValue)));
            }
            else
            {
                NormalisableRange<float> range (spec.minValue, spec.maxValue);
                if (spec.skewMidpoint != 0.0f
                    && spec.skewMidpoint > spec.minValue
                    && spec.skewMidpoint < spec.maxValue)
                {
                    range.setSkewForCentre (spec.skewMidpoint);
                }
                layout.add (std::make_unique<AudioParameterFloat> (
                    ParameterID (spec.paramID, 1), spec.label, range, spec.defaultValue));
            }
        }

        for (const auto& pos : variant.positions)
        {
            const juce::String unit = pos.unit;

            auto attrs = AudioParameterFloatAttributes()
                .withStringFromValueFunction ([unit] (float v, int) -> juce::String
                {
                    auto s = juce::String (v, 2);
                    if (unit.isNotEmpty())
                        s += " " + unit;
                    return s;
                })
                .withValueFromStringFunction ([] (const juce::String& text) -> float
                {
                    return text.retainCharacters ("-0123456789.").getFloatValue();
                });

            // 0.01 unit interval — gives IncDec slider buttons a sensible step
            // (1 cm for metre positions, 0.01° for normalised ADM, etc.) and
            // matches the 2-decimal display precision of the text formatter.
            layout.add (std::make_unique<AudioParameterFloat> (
                ParameterID (pos.paramID, 1), pos.label,
                NormalisableRange<float> (pos.minValue, pos.maxValue, 0.01f),
                pos.defaultValue,
                attrs));
        }
        return layout;
    }

    TrackProcessor::TrackProcessor (VariantConfig cfg)
        : AudioProcessor (BusesProperties()
                              .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                              .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
          variant (std::move (cfg)),
          state (*this, nullptr, "TrackState", buildLayout())
    {
        state.addParameterListener ("inputId", this);

        for (const auto& spec : getSharedTrackParams())
            state.addParameterListener (spec.paramID, this);

        if (variant.positionsWired)
            for (const auto& pos : variant.positions)
                state.addParameterListener (pos.paramID, this);
    }

    TrackProcessor::~TrackProcessor()
    {
        state.removeParameterListener ("inputId", this);

        for (const auto& spec : getSharedTrackParams())
            state.removeParameterListener (spec.paramID, this);

        if (variant.positionsWired)
            for (const auto& pos : variant.positions)
                state.removeParameterListener (pos.paramID, this);
    }

    void TrackProcessor::prepareToPlay (double, int)
    {
        auto& loader = BridgeLoader::getInstance();
        if (loader.ensureLoaded() && bridgeHandle == nullptr)
        {
            bridgeHandle = loader.trackRegister (getInputId(),
                                                 variant.coordinateTag.toRawUTF8(),
                                                 this,
                                                 &inboundCallback);
            if (bridgeHandle != nullptr && loader.trackSetInbound3f != nullptr)
                loader.trackSetInbound3f (bridgeHandle, &inbound3fCallback);
        }
    }

    void TrackProcessor::releaseResources()
    {
        auto& loader = BridgeLoader::getInstance();
        if (bridgeHandle != nullptr && loader.isLoaded())
            loader.trackUnregister (bridgeHandle);
        bridgeHandle = nullptr;
    }

    bool TrackProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
    {
        const auto& in  = layouts.getMainInputChannelSet();
        const auto& out = layouts.getMainOutputChannelSet();
        if (in != out) return false;
        if (in == juce::AudioChannelSet::disabled()) return false;
        return in == juce::AudioChannelSet::mono()
            || in == juce::AudioChannelSet::stereo();
    }

    void TrackProcessor::processBlock (juce::AudioBuffer<float>& /*buffer*/, juce::MidiBuffer&)
    {
        // Audio passes through unchanged — plugin is a control surface, not a DSP.
    }

    juce::AudioProcessorEditor* TrackProcessor::createEditor()
    {
        return new TrackEditor (*this);
    }

    void TrackProcessor::getStateInformation (juce::MemoryBlock& destData)
    {
        if (auto xml = state.copyState().createXml())
            copyXmlToBinary (*xml, destData);
    }

    void TrackProcessor::setStateInformation (const void* data, int sizeInBytes)
    {
        if (auto xml = getXmlFromBinary (data, sizeInBytes))
            state.replaceState (juce::ValueTree::fromXml (*xml));
    }

    int TrackProcessor::getInputId() const
    {
        if (auto* p = state.getRawParameterValue ("inputId"))
            return static_cast<int> (std::round (p->load()));
        return 1;
    }

    int TrackProcessor::getAttenuationLaw() const
    {
        if (auto* p = state.getRawParameterValue ("attenuationLaw"))
            return static_cast<int> (p->load());
        return 0;
    }

    // ── Coordinate conversions ─────────────────────────────────────────
    // Azimuth convention (matches the main app's CLAUDE.md):
    //   θ = atan2(x, -y). 0° = audience (-Y), 90° = right (+X),
    //   ±180° = upstage (+Y), -90° = left (-X).
    //   So for cylindrical: x = r·sin(θ), y = -r·cos(θ).

    void TrackProcessor::displayToCartesian (float d0, float d1, float d2,
                                             float& x, float& y, float& z) const
    {
        const auto& tag = variant.coordinateTag;
        if (tag == "cylindrical")
        {
            const float thetaRad = juce::degreesToRadians (d1);
            x =  d0 * std::sin (thetaRad);
            y = -d0 * std::cos (thetaRad);
            z =  d2;
        }
        else if (tag == "spherical")
        {
            const float thetaRad = juce::degreesToRadians (d1);
            const float phiRad   = juce::degreesToRadians (d2);
            const float rCosPhi  = d0 * std::cos (phiRad);
            x =  rCosPhi * std::sin (thetaRad);
            y = -rCosPhi * std::cos (thetaRad);
            z =  d0 * std::sin (phiRad);
        }
        else // cartesian (or unknown — treat as identity)
        {
            x = d0; y = d1; z = d2;
        }
    }

    void TrackProcessor::cartesianToDisplay (float x, float y, float z,
                                             float& d0, float& d1, float& d2) const
    {
        const auto& tag = variant.coordinateTag;
        if (tag == "cylindrical")
        {
            d0 = std::sqrt (x * x + y * y);
            d1 = juce::radiansToDegrees (std::atan2 (x, -y));
            d2 = z;
        }
        else if (tag == "spherical")
        {
            d0 = std::sqrt (x * x + y * y + z * z);
            d1 = juce::radiansToDegrees (std::atan2 (x, -y));
            d2 = (d0 > 1.0e-4f)
                   ? juce::radiansToDegrees (std::asin (juce::jlimit (-1.0f, 1.0f, z / d0)))
                   : 0.0f;
        }
        else // cartesian
        {
            d0 = x; d1 = y; d2 = z;
        }
    }

    void TrackProcessor::recomputeCartesianFromDisplay()
    {
        if (! variant.positionsWired)
            return;
        float d[3] = { 0.0f, 0.0f, 0.0f };
        for (int i = 0; i < 3; ++i)
            if (auto* p = state.getRawParameterValue (variant.positions[(size_t) i].paramID))
                d[i] = p->load();
        float x = 0.0f, y = 0.0f, z = 0.0f;
        displayToCartesian (d[0], d[1], d[2], x, y, z);
        cachedX.store (x);
        cachedY.store (y);
        cachedZ.store (z);
    }

    void TrackProcessor::updateDisplayFromCartesian()
    {
        if (! variant.positionsWired)
            return;
        float d[3] = { 0.0f, 0.0f, 0.0f };
        cartesianToDisplay (cachedX.load(), cachedY.load(), cachedZ.load(),
                            d[0], d[1], d[2]);

        // Batch the in-flight guard around the whole set so async listener
        // callbacks can't slip through in the gap between axis updates.
        isApplyingRemoteChange.store (true);
        for (int i = 0; i < 3; ++i)
        {
            auto* param = state.getParameter (variant.positions[(size_t) i].paramID);
            if (param == nullptr)
                continue;
            param->setValueNotifyingHost (
                juce::jlimit (0.0f, 1.0f, param->convertTo0to1 (d[i])));
        }
        isApplyingRemoteChange.store (false);
    }

    void TrackProcessor::sendCartesianPositionsToApp()
    {
        if (bridgeHandle == nullptr)
            return;
        auto& loader = BridgeLoader::getInstance();
        if (! loader.isLoaded() || loader.trackSendOutbound == nullptr)
            return;
        const int id = getInputId();
        loader.trackSendOutbound (bridgeHandle, "/wfs/input/positionX", id,
                                  static_cast<double> (cachedX.load()));
        loader.trackSendOutbound (bridgeHandle, "/wfs/input/positionY", id,
                                  static_cast<double> (cachedY.load()));
        loader.trackSendOutbound (bridgeHandle, "/wfs/input/positionZ", id,
                                  static_cast<double> (cachedZ.load()));
    }

    juce::String TrackProcessor::admCombinedPath() const
    {
        const char* suffix = variant.coordinateTag == "adm-polar" ? "/aed" : "/xyz";
        return "/adm/obj/" + juce::String (getInputId()) + suffix;
    }

    void TrackProcessor::sendAdmPositionsToApp()
    {
        if (bridgeHandle == nullptr)
            return;
        auto& loader = BridgeLoader::getInstance();
        if (! loader.isLoaded() || loader.trackSendOutbound3f == nullptr)
            return;

        // Read all three ADM display params directly — no conversion, the
        // plugin's native ADM values go on the wire as-is.
        float v[3] = { 0.0f, 0.0f, 0.0f };
        for (int i = 0; i < 3; ++i)
            if (auto* p = state.getRawParameterValue (variant.positions[(size_t) i].paramID))
                v[i] = p->load();

        // Echo suppression: if the values about to be sent match the last
        // triple we received from the app (within an epsilon), this is the
        // plugin reacting to its own earlier outbound that the app bounced
        // back. Skip, otherwise we'd feedback-loop forever with both sides
        // drifting slightly on each round-trip.
        const float rx0 = lastRxAdmV1.load();
        const float rx1 = lastRxAdmV2.load();
        const float rx2 = lastRxAdmV3.load();
        const float eps = 1.0e-3f;
        if (std::isfinite (rx0) && std::isfinite (rx1) && std::isfinite (rx2)
            && std::abs (v[0] - rx0) < eps
            && std::abs (v[1] - rx1) < eps
            && std::abs (v[2] - rx2) < eps)
            return;

        loader.trackSendOutbound3f (bridgeHandle,
                                    admCombinedPath().toRawUTF8(),
                                    static_cast<double> (v[0]),
                                    static_cast<double> (v[1]),
                                    static_cast<double> (v[2]));
        diagLog.add ("Tx " + admCombinedPath()
                     + " (" + juce::String (v[0], 3)
                     + ", " + juce::String (v[1], 3)
                     + ", " + juce::String (v[2], 3) + ")");
    }

    void TrackProcessor::inbound3fCallback (void* user, const char* oscPath,
                                            int /*channelId*/,
                                            double v1, double v2, double v3)
    {
        if (user == nullptr)
            return;
        auto* self = static_cast<TrackProcessor*> (user);
        if (! self->isAdmVariant() || ! self->variant.positionsWired)
            return;

        const juce::String path = juce::String::fromUTF8 (oscPath);
        // Accept both exact /xyz|aed and any /adm/obj/<id>/(xyz|aed) — the
        // bridge only routes by inputId, so the suffix is what matters.
        const bool isCartCombined = path.endsWith ("/xyz") || path.endsWith ("/xy");
        const bool isPolarCombined = path.endsWith ("/aed");
        const bool expectCart = self->variant.coordinateTag == "adm-cartesian";
        const bool expectPolar = self->variant.coordinateTag == "adm-polar";

        if ((expectCart && ! isCartCombined) || (expectPolar && ! isPolarCombined))
            return;

        self->diagLog.add ("Rx " + path
                           + " (" + juce::String (v1, 3)
                           + ", " + juce::String (v2, 3)
                           + ", " + juce::String (v3, 3) + ")");

        // Apply to the params, then record the *post-quantization* stored
        // values so sendAdmPositionsToApp can recognise the inevitable echo
        // when parameterChanged fires asynchronously. The APVTS range has a
        // 0.01 interval, so storing the raw received value here would leave
        // lastRx out of sync with what the param reads back on echo.
        const double values[3] = { v1, v2, v3 };
        self->isApplyingRemoteChange.store (true);
        for (int i = 0; i < 3; ++i)
        {
            auto* param = self->state.getParameter (self->variant.positions[(size_t) i].paramID);
            if (param == nullptr)
                continue;
            param->setValueNotifyingHost (
                juce::jlimit (0.0f, 1.0f,
                              param->convertTo0to1 (static_cast<float> (values[i]))));
        }
        self->isApplyingRemoteChange.store (false);

        // Read back what actually got stored (after step quantization) so
        // the echo-suppression compare in sendAdmPositionsToApp matches.
        auto storedAxis = [self] (int i) -> float
        {
            if (auto* raw = self->state.getRawParameterValue (self->variant.positions[(size_t) i].paramID))
                return raw->load();
            return 0.0f;
        };
        self->lastRxAdmV1.store (storedAxis (0));
        self->lastRxAdmV2.store (storedAxis (1));
        self->lastRxAdmV3.store (storedAxis (2));
    }

    void TrackProcessor::parameterChanged (const juce::String& paramID, float newValue)
    {
        // Input ID change: re-register with the bridge under the new channel
        // so Master's subscription list and dispatch routing both follow.
        // This happens regardless of isApplyingRemoteChange — the ID is local
        // plugin state, never pushed from the app.
        if (paramID == "inputId")
        {
            auto& loader = BridgeLoader::getInstance();
            if (! loader.isLoaded())
                return;
            if (bridgeHandle != nullptr)
            {
                loader.trackUnregister (bridgeHandle);
                bridgeHandle = nullptr;
            }
            const int newId = static_cast<int> (std::round (newValue));
            bridgeHandle = loader.trackRegister (newId,
                                                 variant.coordinateTag.toRawUTF8(),
                                                 this,
                                                 &inboundCallback);
            if (bridgeHandle != nullptr && loader.trackSetInbound3f != nullptr)
                loader.trackSetInbound3f (bridgeHandle, &inbound3fCallback);
            return;
        }

        if (isApplyingRemoteChange.load())
            return;
        if (bridgeHandle == nullptr)
            return;

        auto& loader = BridgeLoader::getInstance();
        if (! loader.isLoaded() || loader.trackSendOutbound == nullptr)
            return;

        auto sendParam = [&] (const juce::String& oscPath, double value)
        {
            loader.trackSendOutbound (bridgeHandle, oscPath.toRawUTF8(),
                                      getInputId(), value);
        };

        // Distance attenuation and distance ratio are mutually exclusive;
        // only the one matching the current law is "live" at the app side.
        if (paramID == "distanceAttenuation" && getAttenuationLaw() != 0)
            return;
        if (paramID == "distanceRatio" && getAttenuationLaw() != 1)
            return;

        for (const auto& spec : getSharedTrackParams())
        {
            if (paramID == spec.paramID)
            {
                sendParam (spec.oscPath, static_cast<double> (newValue));

                // When the user flips the law, push the newly-active dial's
                // current value so the app stays in sync after the switch.
                if (paramID == "attenuationLaw")
                {
                    const bool nowLog = static_cast<int> (newValue) == 0;
                    const char* followId   = nowLog ? "distanceAttenuation"
                                                    : "distanceRatio";
                    const char* followPath = nowLog ? "/wfs/input/distanceAttenuation"
                                                    : "/wfs/input/distanceRatio";
                    if (auto* p = state.getParameter (followId))
                    {
                        const float real = p->convertFrom0to1 (p->getValue());
                        sendParam (followPath, static_cast<double> (real));
                    }
                }
                return;
            }
        }

        if (variant.positionsWired)
        {
            for (const auto& pos : variant.positions)
            {
                if (paramID == pos.paramID)
                {
                    if (isAdmVariant())
                    {
                        // Send /adm/obj/<id>/xyz or /aed with the three
                        // current ADM display values. No coord conversion —
                        // the app applies its per-input mapping preset.
                        sendAdmPositionsToApp();
                    }
                    else
                    {
                        // Native variants: recompute the cached Cartesian
                        // and push all three X/Y/Z. The app only accepts
                        // Cartesian at its OSC router; we convert at the
                        // boundary.
                        recomputeCartesianFromDisplay();
                        sendCartesianPositionsToApp();
                    }
                    return;
                }
            }
        }
    }

    void TrackProcessor::inboundCallback (void* user, const char* oscPath, int /*channelId*/, double value)
    {
        if (user == nullptr)
            return;
        auto* self = static_cast<TrackProcessor*> (user);
        const auto path = juce::String::fromUTF8 (oscPath);

        auto applyParam = [self] (const juce::String& paramID, double value)
        {
            if (auto* param = self->state.getParameter (paramID))
            {
                self->isApplyingRemoteChange.store (true);
                const auto norm = param->convertTo0to1 (static_cast<float> (value));
                param->setValueNotifyingHost (norm);
                self->isApplyingRemoteChange.store (false);
            }
        };

        for (const auto& spec : getSharedTrackParams())
        {
            if (path == spec.oscPath)
            {
                applyParam (spec.paramID, value);
                return;
            }
        }

        // Incoming positions always arrive as Cartesian (only paths the
        // app's OSC router + OSCQuery tree expose). Update the cache and
        // re-derive the variant's display params.
        if (self->variant.positionsWired)
        {
            auto updateAxis = [self] (std::atomic<float>& axis, double v)
            {
                axis.store (static_cast<float> (v));
                self->updateDisplayFromCartesian();
            };

            if (path == "/wfs/input/positionX") { updateAxis (self->cachedX, value); return; }
            if (path == "/wfs/input/positionY") { updateAxis (self->cachedY, value); return; }
            if (path == "/wfs/input/positionZ") { updateAxis (self->cachedZ, value); return; }
        }
    }
}
