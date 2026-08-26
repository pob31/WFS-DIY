#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "TrackProcessor.h"
#include "../Shared/PluginLookAndFeel.h"
#include "../Shared/widgets/WfsStandardSlider.h"
#include "../Shared/widgets/WfsBidirectionalSlider.h"
#include "../Shared/widgets/WfsWidthExpansionSlider.h"
#include "../Shared/widgets/WfsRotationDial.h"
#include "../Shared/widgets/WfsParameterAttachments.h"
#include "../Shared/StatusLogView.h"

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
        juce::Label stereoHeader   { {}, "Stereo Image" };
        juce::Label positionHeader { {}, "Position" };
        juce::Label dirHeader      { {}, "Directivity" };
        juce::Label lfoHeader      { {}, "LFO" };

        // Channel: Input ID + Attenuation
        juce::Label  inputIdLabel;
        juce::Slider inputIdSlider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputIdAttachment;

        // What the app says the Input ID above currently refers to. A permanent
        // channel number is not a position, so the number alone does not identify
        // a channel to an operator once the list has been reordered.
        juce::Label channelInfoLabel;

        juce::Label         attenuationLabel;
        juce::Label         attenuationValueLabel;
        WfsStandardSlider   attenuationSlider { WfsSliderBase::Orientation::horizontal };
        std::unique_ptr<WfsSliderNormalisedAttachment> attenuationAttachment;

        juce::Label         attenuationLawLabel;
        juce::ComboBox      attenuationLawCombo;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attenuationLawAttachment;

        juce::Label         distanceAttenuationLabel;
        juce::Label         distanceAttenuationValueLabel;
        WfsStandardSlider   distanceAttenuationSlider { WfsSliderBase::Orientation::horizontal };
        std::unique_ptr<WfsSliderNormalisedAttachment> distanceAttenuationAttachment;

        juce::Label         distanceRatioLabel;
        juce::Label         distanceRatioValueLabel;
        WfsStandardSlider   distanceRatioSlider { WfsSliderBase::Orientation::horizontal };
        std::unique_ptr<WfsSliderNormalisedAttachment> distanceRatioAttachment;

        std::unique_ptr<juce::ParameterAttachment> lawVisibilityAttachment;

        juce::Label         commonAttenLabel;
        juce::Label         commonAttenValueLabel;
        WfsStandardSlider   commonAttenSlider { WfsSliderBase::Orientation::horizontal };
        std::unique_ptr<WfsSliderNormalisedAttachment> commonAttenAttachment;

        // Stereo image — only meaningful on a stereo input channel.
        juce::Label         stereoWidthLabel;
        juce::Label         stereoWidthValueLabel;
        WfsStandardSlider   stereoWidthSlider { WfsSliderBase::Orientation::horizontal };
        std::unique_ptr<WfsSliderNormalisedAttachment> stereoWidthAttachment;

        juce::Label         stereoAxisLabel;
        juce::Label         stereoAxisValueLabel;
        WfsRotationDial     stereoAxisDial;
        std::unique_ptr<WfsRotationDialAttachment> stereoAxisAttachment;

        // Applies what the processor last heard from the app: the identity line,
        // and whether the stereo section is shown at all.
        void refreshChannelInfo();
        bool stereoSectionVisible = false;

        // 774 as it always was, plus one 24 px identity line under Input ID.
        static constexpr int kBaseHeight = 774 + 28;
        // Section header (24 + 6), width row (30 + 4), axis dial row (72 + 4),
        // trailing gap (6) — the same metrics resized() lays the section out with.
        static constexpr int kStereoSectionHeight = 146;

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

        juce::Label                     buildLabel;
        std::unique_ptr<StatusLogView>  statusLog;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackEditor)
    };
}
