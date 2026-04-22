#pragma once

#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include "WfsSliderBase.h"
#include "WfsRotationDial.h"

namespace wfs::plugin
{
    // Bridge from a juce::RangedAudioParameter to a WfsSliderBase that operates
    // on the parameter's real (denormalised) value range directly. Use this
    // when the slider's (min, max) matches the parameter's range exactly
    // (e.g. WfsBidirectionalSlider(-90, 90) for Tilt).
    class WfsSliderDirectAttachment
    {
    public:
        WfsSliderDirectAttachment (juce::RangedAudioParameter& parameter,
                                   WfsSliderBase& slider)
            : slider (slider)
        {
            inner = std::make_unique<juce::ParameterAttachment> (
                parameter,
                [this] (float newValue)
                {
                    updatingFromParam.store (true);
                    this->slider.setValue (newValue);
                    updatingFromParam.store (false);
                });

            slider.onGestureStart = [this] { inner->beginGesture(); };
            slider.onValueChanged = [this] (float v)
            {
                if (updatingFromParam.load())
                    return;
                inner->setValueAsPartOfGesture (v);
            };
            slider.onGestureEnd = [this] { inner->endGesture(); };

            inner->sendInitialUpdate();
        }

        ~WfsSliderDirectAttachment()
        {
            slider.onGestureStart = nullptr;
            slider.onValueChanged = nullptr;
            slider.onGestureEnd   = nullptr;
        }

    private:
        WfsSliderBase& slider;
        std::atomic<bool> updatingFromParam { false };
        std::unique_ptr<juce::ParameterAttachment> inner;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WfsSliderDirectAttachment)
    };

    // Bridge from a juce::RangedAudioParameter to a WfsSliderBase whose own
    // range is normalised 0..1. Uses the parameter's NormalisableRange so
    // any skew configured on the parameter is reflected in the slider feel
    // (e.g. log-skewed attenuation, HF shelf).
    class WfsSliderNormalisedAttachment
    {
    public:
        WfsSliderNormalisedAttachment (juce::RangedAudioParameter& parameter,
                                       WfsSliderBase& slider)
            : parameter (parameter), slider (slider)
        {
            inner = std::make_unique<juce::ParameterAttachment> (
                parameter,
                [this] (float newValue)
                {
                    updatingFromParam.store (true);
                    this->slider.setValue (this->parameter.convertTo0to1 (newValue));
                    updatingFromParam.store (false);
                });

            slider.onGestureStart = [this] { inner->beginGesture(); };
            slider.onValueChanged = [this] (float normalisedPos)
            {
                if (updatingFromParam.load())
                    return;
                inner->setValueAsPartOfGesture (
                    this->parameter.convertFrom0to1 (juce::jlimit (0.0f, 1.0f, normalisedPos)));
            };
            slider.onGestureEnd = [this] { inner->endGesture(); };

            inner->sendInitialUpdate();
        }

        ~WfsSliderNormalisedAttachment()
        {
            slider.onGestureStart = nullptr;
            slider.onValueChanged = nullptr;
            slider.onGestureEnd   = nullptr;
        }

    private:
        juce::RangedAudioParameter& parameter;
        WfsSliderBase& slider;
        std::atomic<bool> updatingFromParam { false };
        std::unique_ptr<juce::ParameterAttachment> inner;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WfsSliderNormalisedAttachment)
    };

    // Bridge from a juce::RangedAudioParameter (in degrees) to a WfsRotationDial.
    // The dial's internal range is already degrees, so no conversion required.
    class WfsRotationDialAttachment
    {
    public:
        WfsRotationDialAttachment (juce::RangedAudioParameter& parameter,
                                   WfsRotationDial& dial)
            : dial (dial)
        {
            inner = std::make_unique<juce::ParameterAttachment> (
                parameter,
                [this] (float degrees)
                {
                    updatingFromParam.store (true);
                    this->dial.setAngle (degrees);
                    updatingFromParam.store (false);
                });

            dial.onGestureStart = [this] { inner->beginGesture(); };
            dial.onAngleChanged = [this] (float degrees)
            {
                if (updatingFromParam.load())
                    return;
                inner->setValueAsPartOfGesture (degrees);
            };
            dial.onGestureEnd = [this] { inner->endGesture(); };

            inner->sendInitialUpdate();
        }

        ~WfsRotationDialAttachment()
        {
            dial.onGestureStart = nullptr;
            dial.onAngleChanged = nullptr;
            dial.onGestureEnd   = nullptr;
        }

    private:
        WfsRotationDial& dial;
        std::atomic<bool> updatingFromParam { false };
        std::unique_ptr<juce::ParameterAttachment> inner;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WfsRotationDialAttachment)
    };
}
