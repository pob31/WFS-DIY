#include "TrackEditor.h"
#include "BinaryData.h"

namespace wfs::plugin
{
    namespace
    {
        void styleSectionHeader (juce::Label& h)
        {
            h.setFont (juce::FontOptions (16.0f).withStyle ("Bold"));
            h.setColour (juce::Label::textColourId, juce::Colour (DarkPalette::textPrimary));
            h.setJustificationType (juce::Justification::centredLeft);
        }

        juce::String formatDecibels (float dB)
        {
            return juce::String (dB, 1) + " dB";
        }

        juce::String formatDegreesInt (float d)
        {
            return juce::String (static_cast<int> (std::round (d))) + juce::String (juce::CharPointer_UTF8 ("\xc2\xb0"));
        }
    }

    void TrackEditor::setupRowLabel (juce::Label& label, const juce::String& text)
    {
        label.setText (text, juce::dontSendNotification);
        label.setFont (juce::FontOptions (14.0f));
        label.setColour (juce::Label::textColourId, juce::Colour (DarkPalette::textSecondary));
        label.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (label);
    }

    void TrackEditor::setupValueLabel (juce::Label& label)
    {
        label.setFont (juce::FontOptions (14.0f));
        label.setColour (juce::Label::textColourId, juce::Colour (DarkPalette::textPrimary));
        label.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (label);
    }

    TrackEditor::TrackEditor (TrackProcessor& p)
        : juce::AudioProcessorEditor (&p), processor (p)
    {
        setLookAndFeel (&lookAndFeel);
        logoImage = juce::ImageCache::getFromMemory (BinaryData::WFSDIY_logo_png,
                                                     BinaryData::WFSDIY_logo_pngSize);
        setSize (500, 640);

        // Title
        titleLabel.setText ("WFS-DIY Track", juce::dontSendNotification);
        titleLabel.setFont (juce::FontOptions (18.0f).withStyle ("Bold"));
        titleLabel.setColour (juce::Label::textColourId, juce::Colour (DarkPalette::textPrimary));
        addAndMakeVisible (titleLabel);

        variantLabel.setText (p.getVariant().coordinateTag.toUpperCase(), juce::dontSendNotification);
        variantLabel.setFont (juce::FontOptions (12.0f).withStyle ("Bold"));
        variantLabel.setColour (juce::Label::textColourId, juce::Colour (DarkPalette::accentBlueBright));
        variantLabel.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (variantLabel);

        // Section headers
        for (auto* h : { &channelHeader, &positionHeader, &dirHeader, &lfoHeader })
        {
            styleSectionHeader (*h);
            addAndMakeVisible (*h);
        }

        // ── Channel ─────────────────────────────────────────────────────
        setupRowLabel (inputIdLabel, "Input ID");
        inputIdSlider.setSliderStyle (juce::Slider::IncDecButtons);
        inputIdSlider.setIncDecButtonsMode (juce::Slider::incDecButtonsDraggable_Vertical);
        inputIdSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 70, 26);
        inputIdSlider.setNumDecimalPlacesToDisplay (0);
        addAndMakeVisible (inputIdSlider);
        inputIdAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            p.getState(), "inputId", inputIdSlider);

        setupRowLabel (attenuationLabel, "Attenuation");
        setupValueLabel (attenuationValueLabel);
        attenuationSlider.setTrackColours (juce::Colour (DarkPalette::sliderTrackBg),
                                           juce::Colour (DarkPalette::accentBlueBright));
        addAndMakeVisible (attenuationSlider);
        if (auto* param = p.getState().getParameter ("attenuation"))
            attenuationAttachment = std::make_unique<WfsSliderNormalisedAttachment> (*param, attenuationSlider);

        // Attenuation Law — ComboBox (Log / 1/d) driving the two distance dials.
        setupRowLabel (attenuationLawLabel, "Attenuation Law");
        attenuationLawCombo.addItem ("Log",  1);
        attenuationLawCombo.addItem ("1/d",  2);
        addAndMakeVisible (attenuationLawCombo);
        attenuationLawAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            p.getState(), "attenuationLaw", attenuationLawCombo);

        setupRowLabel   (distanceAttenuationLabel, "Distance Atten.");
        setupValueLabel (distanceAttenuationValueLabel);
        distanceAttenuationSlider.setTrackColours (juce::Colour (DarkPalette::sliderTrackBg),
                                                   juce::Colour (DarkPalette::accentBlueBright));
        addAndMakeVisible (distanceAttenuationSlider);
        if (auto* param = p.getState().getParameter ("distanceAttenuation"))
            distanceAttenuationAttachment = std::make_unique<WfsSliderNormalisedAttachment> (
                *param, distanceAttenuationSlider);

        setupRowLabel   (distanceRatioLabel, "Distance Ratio");
        setupValueLabel (distanceRatioValueLabel);
        distanceRatioSlider.setTrackColours (juce::Colour (DarkPalette::sliderTrackBg),
                                             juce::Colour (DarkPalette::accentBlueBright));
        addAndMakeVisible (distanceRatioSlider);
        if (auto* param = p.getState().getParameter ("distanceRatio"))
            distanceRatioAttachment = std::make_unique<WfsSliderNormalisedAttachment> (
                *param, distanceRatioSlider);

        // Listen on the law parameter to toggle visibility of the two dials.
        if (auto* law = p.getState().getParameter ("attenuationLaw"))
        {
            lawVisibilityAttachment = std::make_unique<juce::ParameterAttachment> (
                *law,
                [this] (float value)
                {
                    const bool isLog = value < 0.5f;
                    distanceAttenuationLabel.setVisible       (isLog);
                    distanceAttenuationSlider.setVisible      (isLog);
                    distanceAttenuationValueLabel.setVisible  (isLog);
                    distanceRatioLabel.setVisible             (! isLog);
                    distanceRatioSlider.setVisible            (! isLog);
                    distanceRatioValueLabel.setVisible        (! isLog);
                    resized();
                });
            lawVisibilityAttachment->sendInitialUpdate();
        }

        // ── Position ───────────────────────────────────────────────────
        if (p.getVariant().positionsWired)
        {
            for (size_t i = 0; i < 3; ++i)
            {
                auto& row = positionRows[i];
                auto& pos = p.getVariant().positions[i];
                setupRowLabel (row.label, pos.label);
                row.slider.setSliderStyle (juce::Slider::IncDecButtons);
                row.slider.setIncDecButtonsMode (juce::Slider::incDecButtonsDraggable_Vertical);
                row.slider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 90, 26);
                addAndMakeVisible (row.slider);
                // Text formatting lives on the parameter (see TrackProcessor::buildLayout),
                // because SliderParameterAttachment overrides slider-level text callbacks.
                row.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
                    p.getState(), pos.paramID, row.slider);
            }
        }
        else
        {
            positionsPlaceholder.setText ("Position parameters wired up in Phase 1",
                                          juce::dontSendNotification);
            positionsPlaceholder.setJustificationType (juce::Justification::centred);
            positionsPlaceholder.setColour (juce::Label::textColourId,
                                            juce::Colour (DarkPalette::textSecondary));
            addAndMakeVisible (positionsPlaceholder);
        }

        // ── Directivity ────────────────────────────────────────────────
        setupRowLabel    (directivityLabel, "Directivity");
        setupValueLabel  (directivityValueLabel);
        addAndMakeVisible (directivitySlider);
        if (auto* param = p.getState().getParameter ("directivity"))
            directivityAttachment = std::make_unique<WfsSliderNormalisedAttachment> (*param, directivitySlider);

        setupRowLabel   (rotationLabel, "Rotation");
        setupValueLabel (rotationValueLabel);
        addAndMakeVisible (rotationDial);
        if (auto* param = p.getState().getParameter ("rotation"))
            rotationAttachment = std::make_unique<WfsRotationDialAttachment> (*param, rotationDial);

        setupRowLabel   (tiltLabel, "Tilt");
        setupValueLabel (tiltValueLabel);
        addAndMakeVisible (tiltSlider);
        if (auto* param = p.getState().getParameter ("tilt"))
            tiltAttachment = std::make_unique<WfsSliderDirectAttachment> (*param, tiltSlider);

        setupRowLabel    (hfShelfLabel, "HF Shelf");
        setupValueLabel  (hfShelfValueLabel);
        hfShelfSlider.setTrackColours (juce::Colour (DarkPalette::sliderTrackBg),
                                       juce::Colour (DarkPalette::accentBlueBright));
        addAndMakeVisible (hfShelfSlider);
        if (auto* param = p.getState().getParameter ("hfShelf"))
            hfShelfAttachment = std::make_unique<WfsSliderNormalisedAttachment> (*param, hfShelfSlider);

        // ── LFO ─────────────────────────────────────────────────────────
        setupRowLabel (lfoActiveLabel, "Active");
        lfoActiveButton.setButtonText ("");
        addAndMakeVisible (lfoActiveButton);
        lfoActiveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
            p.getState(), "lfoActive", lfoActiveButton);

        // Live value readouts — one ParameterAttachment per value label.
        auto wireValueLabel = [&] (const juce::String& paramId,
                                   juce::Label& valueLabel,
                                   std::function<juce::String (float)> formatter)
        {
            if (auto* prm = p.getState().getParameter (paramId))
            {
                auto* holder = valueLabelAttachments.add (
                    new juce::ParameterAttachment (
                        *prm,
                        [&valueLabel, formatter = std::move (formatter)] (float realValue)
                        {
                            valueLabel.setText (formatter (realValue), juce::dontSendNotification);
                        }));
                holder->sendInitialUpdate();
            }
        };

        wireValueLabel ("attenuation",         attenuationValueLabel,         formatDecibels);
        wireValueLabel ("directivity",         directivityValueLabel,         formatDegreesInt);
        wireValueLabel ("rotation",            rotationValueLabel,            formatDegreesInt);
        wireValueLabel ("tilt",                tiltValueLabel,                formatDegreesInt);
        wireValueLabel ("hfShelf",             hfShelfValueLabel,             formatDecibels);
        wireValueLabel ("distanceAttenuation", distanceAttenuationValueLabel,
                        [] (float v) { return juce::String (v, 2) + " dB/m"; });
        wireValueLabel ("distanceRatio",       distanceRatioValueLabel,
                        [] (float v) { return juce::String (v, 2) + " x"; });
    }

    TrackEditor::~TrackEditor()
    {
        setLookAndFeel (nullptr);
    }

    void TrackEditor::paint (juce::Graphics& g)
    {
        g.fillAll (juce::Colour (DarkPalette::background));

        // Separator under title
        g.setColour (juce::Colour (DarkPalette::chromeDivider));
        g.drawHorizontalLine (44, 14.0f, static_cast<float> (getWidth() - 14));

        // Separator under each visible section header
        auto drawHeaderDivider = [&] (const juce::Label& header)
        {
            if (! header.isVisible())
                return;
            const auto b = header.getBounds();
            g.drawHorizontalLine (b.getBottom() + 1,
                                  static_cast<float> (b.getX()),
                                  static_cast<float> (getWidth() - 14));
        };
        g.setColour (juce::Colour (DarkPalette::chromeDivider).withAlpha (0.5f));
        drawHeaderDivider (channelHeader);
        drawHeaderDivider (positionHeader);
        drawHeaderDivider (dirHeader);
        drawHeaderDivider (lfoHeader);

        // Logo bottom-right
        if (logoImage.isValid())
        {
            auto logoArea = getLocalBounds()
                                .removeFromBottom (52)
                                .removeFromRight (130)
                                .reduced (10, 6);
            g.setOpacity (0.85f);
            g.drawImageWithin (logoImage,
                               logoArea.getX(), logoArea.getY(),
                               logoArea.getWidth(), logoArea.getHeight(),
                               juce::RectanglePlacement::xRight | juce::RectanglePlacement::yBottom
                                 | juce::RectanglePlacement::onlyReduceInSize);
            g.setOpacity (1.0f);
        }
    }

    juce::Rectangle<int> TrackEditor::layoutSectionHeader (juce::Label& header, juce::Rectangle<int>& area)
    {
        auto headerArea = area.removeFromTop (24);
        header.setBounds (headerArea);
        area.removeFromTop (6);
        return headerArea;
    }

    void TrackEditor::resized()
    {
        auto area = getLocalBounds().reduced (14);
        area.removeFromBottom (52); // logo strip

        // Title row
        auto titleArea = area.removeFromTop (32);
        variantLabel.setBounds (titleArea.removeFromRight (140));
        titleLabel.setBounds   (titleArea);
        area.removeFromTop (8);

        const int rowHeight   = 30;
        const int rowSpacing  = 4;
        const int labelWidth  = 110;
        const int valueWidth  = 90;

        auto layoutLabelSliderValue = [&] (juce::Label& label,
                                           juce::Component& control,
                                           juce::Label& value)
        {
            auto r = area.removeFromTop (rowHeight);
            label.setBounds   (r.removeFromLeft  (labelWidth));
            value.setBounds   (r.removeFromRight (valueWidth));
            control.setBounds (r.reduced (2, 4));
            area.removeFromTop (rowSpacing);
        };

        auto layoutLabelControl = [&] (juce::Label& label, juce::Component& control, int heightOverride = 0)
        {
            auto r = area.removeFromTop (heightOverride > 0 ? heightOverride : rowHeight);
            label.setBounds   (r.removeFromLeft (labelWidth));
            control.setBounds (r.reduced (2, 2));
            area.removeFromTop (rowSpacing);
        };

        // ── Channel ───────────────────────────────
        layoutSectionHeader (channelHeader, area);
        layoutLabelControl        (inputIdLabel, inputIdSlider);
        layoutLabelSliderValue    (attenuationLabel, attenuationSlider, attenuationValueLabel);
        layoutLabelControl        (attenuationLawLabel, attenuationLawCombo);
        if (distanceAttenuationSlider.isVisible())
            layoutLabelSliderValue (distanceAttenuationLabel,
                                    distanceAttenuationSlider,
                                    distanceAttenuationValueLabel);
        else
            layoutLabelSliderValue (distanceRatioLabel,
                                    distanceRatioSlider,
                                    distanceRatioValueLabel);
        area.removeFromTop (6);

        // ── Position ──────────────────────────────
        layoutSectionHeader (positionHeader, area);
        if (processor.getVariant().positionsWired)
        {
            for (auto& row : positionRows)
                layoutLabelControl (row.label, row.slider);
        }
        else
        {
            positionsPlaceholder.setBounds (area.removeFromTop (32));
        }
        area.removeFromTop (6);

        // ── Directivity ───────────────────────────
        layoutSectionHeader (dirHeader, area);
        layoutLabelSliderValue (directivityLabel, directivitySlider, directivityValueLabel);
        {
            // Rotation dial needs a taller row
            auto r = area.removeFromTop (72);
            rotationLabel.setBounds      (r.removeFromLeft  (labelWidth));
            rotationValueLabel.setBounds (r.removeFromRight (valueWidth));
            rotationDial.setBounds       (r.withSizeKeepingCentre (juce::jmin (r.getWidth(), 64), 64));
            area.removeFromTop (rowSpacing);
        }
        layoutLabelSliderValue (tiltLabel,    tiltSlider,    tiltValueLabel);
        layoutLabelSliderValue (hfShelfLabel, hfShelfSlider, hfShelfValueLabel);
        area.removeFromTop (6);

        // ── LFO ───────────────────────────────────
        layoutSectionHeader (lfoHeader, area);
        layoutLabelControl (lfoActiveLabel, lfoActiveButton);
    }
}
