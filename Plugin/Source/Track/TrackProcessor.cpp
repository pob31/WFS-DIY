#include "TrackProcessor.h"
#include "TrackEditor.h"

namespace wfs::plugin
{
    const std::array<NonPositionParamSpec, 6>& getSharedTrackParams()
    {
        static const std::array<NonPositionParamSpec, 6> params = {{
            { "attenuation", "Attenuation", "/wfs/input/attenuation", false, -92.0f,   0.0f,  0.0f, "dB",
              TrackWidget::LogSlider,       -12.0f },
            { "directivity", "Directivity", "/wfs/input/directivity", true,    2.0f, 360.0f, 360.0f, juce::CharPointer_UTF8 ("\xc2\xb0"),
              TrackWidget::LinearSlider,      0.0f },
            { "rotation",    "Rotation",    "/wfs/input/rotation",    true, -179.0f, 180.0f,   0.0f, juce::CharPointer_UTF8 ("\xc2\xb0"),
              TrackWidget::RotaryDial,        0.0f },
            { "tilt",        "Tilt",        "/wfs/input/tilt",        true,  -90.0f,  90.0f,   0.0f, juce::CharPointer_UTF8 ("\xc2\xb0"),
              TrackWidget::BidirectionalBar,  0.0f },
            { "hfShelf",     "HF Shelf",    "/wfs/input/HFshelf",     false, -24.0f,   0.0f,  -6.0f, "dB",
              TrackWidget::LogSlider,        -6.0f },
            { "lfoActive",   "LFO Active",  "/wfs/input/LFOactive",   true,    0.0f,   1.0f,   0.0f, "",
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

            layout.add (std::make_unique<AudioParameterFloat> (
                ParameterID (pos.paramID, 1), pos.label,
                NormalisableRange<float> (pos.minValue, pos.maxValue),
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
        for (const auto& spec : getSharedTrackParams())
            state.addParameterListener (spec.paramID, this);

        if (variant.positionsWired)
            for (const auto& pos : variant.positions)
                state.addParameterListener (pos.paramID, this);
    }

    TrackProcessor::~TrackProcessor()
    {
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
            return static_cast<int> (p->load());
        return 1;
    }

    void TrackProcessor::parameterChanged (const juce::String& paramID, float newValue)
    {
        if (isApplyingRemoteChange.load())
            return;
        if (bridgeHandle == nullptr)
            return;

        auto& loader = BridgeLoader::getInstance();
        if (! loader.isLoaded() || loader.trackSendOutbound == nullptr)
            return;

        for (const auto& spec : getSharedTrackParams())
        {
            if (paramID == spec.paramID)
            {
                loader.trackSendOutbound (bridgeHandle, spec.oscPath.toRawUTF8(),
                                          getInputId(), static_cast<double> (newValue));
                return;
            }
        }

        if (variant.positionsWired)
        {
            for (const auto& pos : variant.positions)
            {
                if (paramID == pos.paramID)
                {
                    loader.trackSendOutbound (bridgeHandle, pos.oscPath.toRawUTF8(),
                                              getInputId(), static_cast<double> (newValue));
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

        if (self->variant.positionsWired)
        {
            for (const auto& pos : self->variant.positions)
            {
                if (path == pos.oscPath)
                {
                    applyParam (pos.paramID, value);
                    return;
                }
            }
        }
    }
}
