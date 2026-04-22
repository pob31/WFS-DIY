#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "TrackProcessor.h"
#include "../Shared/PluginLookAndFeel.h"
#include "../Shared/widgets/WfsStandardSlider.h"
#include "../Shared/widgets/WfsBidirectionalSlider.h"
#include "../Shared/widgets/WfsWidthExpansionSlider.h"
#include "../Shared/widgets/WfsRotationDial.h"
#include "../Shared/widgets/WfsParameterAttachments.h"

namespace wfs::plugin
{
    class TrackEditor  : public juce::AudioProcessorEditor
    {
    public:
        explicit TrackEditor (TrackProcessor&);
        ~TrackEditor() override;

        void paint (juce::Graphics&) override;
        void resized() override;

    private:
        void setupRowLabel (juce::Label&, const juce::String& text);
        void setupValueLabel (juce::Label&);

        juce::Rectangle<int> layoutSectionHeader (juce::Label& header, juce::Rectangle<int>& area);

        PluginLookAndFeel lookAndFeel;
        juce::Image       logoImage;
        TrackProcessor&   processor;

        // Header
        juce::Label titleLabel;
        juce::Label variantLabel;

        // Section headers
        juce::Label channelHeader  { {}, "Channel" };
        juce::Label positionHeader { {}, "Position" };
        juce::Label dirHeader      { {}, "Directivity" };
        juce::Label lfoHeader      { {}, "LFO" };

        // Channel: Input ID + Attenuation
        juce::Label  inputIdLabel;
        juce::Slider inputIdSlider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputIdAttachment;

        juce::Label         attenuationLabel;
        juce::Label         attenuationValueLabel;
        WfsStandardSlider   attenuationSlider { WfsSliderBase::Orientation::horizontal };
        std::unique_ptr<WfsSliderNormalisedAttachment> attenuationAttachment;

        // Position: X/Y/Z number boxes
        struct NumberBoxRow
        {
            juce::Label  label;
            juce::Slider slider;
            std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
        };
        std::array<NumberBoxRow, 3> positionRows;
        juce::Label positionsPlaceholder;

        // Directivity
        juce::Label               directivityLabel;
        juce::Label               directivityValueLabel;
        WfsWidthExpansionSlider   directivitySlider { WfsSliderBase::Orientation::horizontal };
        std::unique_ptr<WfsSliderNormalisedAttachment> directivityAttachment;

        juce::Label         rotationLabel;
        juce::Label         rotationValueLabel;
        WfsRotationDial     rotationDial;
        std::unique_ptr<WfsRotationDialAttachment> rotationAttachment;

        juce::Label             tiltLabel;
        juce::Label             tiltValueLabel;
        WfsBidirectionalSlider  tiltSlider { -90.0f, 90.0f, WfsSliderBase::Orientation::horizontal };
        std::unique_ptr<WfsSliderDirectAttachment> tiltAttachment;

        juce::Label         hfShelfLabel;
        juce::Label         hfShelfValueLabel;
        WfsStandardSlider   hfShelfSlider { WfsSliderBase::Orientation::horizontal };
        std::unique_ptr<WfsSliderNormalisedAttachment> hfShelfAttachment;

        // LFO
        juce::Label        lfoActiveLabel;
        juce::ToggleButton lfoActiveButton;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> lfoActiveAttachment;

        // Parameter listeners that only update value-display labels.
        juce::OwnedArray<juce::ParameterAttachment> valueLabelAttachments;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackEditor)
    };
}
