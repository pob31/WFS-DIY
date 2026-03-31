#pragma once

#include <JuceHeader.h>
#include "ColorScheme.h"
#include "WfsLookAndFeel.h"
#include "../Localization/LocalizationManager.h"

//==============================================================================
/**
 * Defines a single step in the Getting Started wizard.
 */
struct WizardStep
{
    int tabIndex;                                           // Tab to switch to (-1 = stay on current)
    juce::String titleKey;                                  // Localization key for step title
    juce::String descriptionKey;                            // Localization key for step description
    std::function<juce::Rectangle<int>()> getSpotlightBounds; // Returns spotlight rect in MainComponent coords
    std::function<bool()> isComplete;                       // Optional completion check
    std::function<void()> onEnter;                          // Optional action when entering this step
    std::function<juce::Component*()> getExternalWindowContent; // Returns external window content (nullptr = main window)
    int skipToStepIndex = -1;                               // If >= 0, shows Skip button jumping to this step
    bool freeInteraction = false;                           // If true, overlay passes all clicks through and card fades out after interaction
    std::function<void()> onExit;                           // Optional action when leaving this step (e.g. close external window)
};

//==============================================================================
/**
 * Full-window overlay that dims everything except a spotlight cutout.
 * Mouse clicks pass through the cutout to the real controls beneath.
 */
class WizardSpotlightOverlay : public juce::Component
{
public:
    WizardSpotlightOverlay()
    {
        setOpaque(false);
        setInterceptsMouseClicks(true, false);
        setPaintingIsUnclipped(true);
    }

    void setSpotlightBounds(juce::Rectangle<int> bounds)
    {
        spotlightRect = bounds.expanded(6); // 6px padding around the zone of interest
        repaint();
    }

    void clearSpotlight()
    {
        spotlightRect = {};
        repaint();
    }

    void setPassthrough(bool passthrough)
    {
        passthroughMode = passthrough;
        setInterceptsMouseClicks(!passthrough, false);
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        if (passthroughMode)
            return; // fully transparent — don't paint anything

        auto area = getLocalBounds();

        if (spotlightRect.isEmpty())
        {
            // No spotlight — dim everything
            g.setColour(juce::Colours::black.withAlpha(0.5f));
            g.fillRect(area);
            return;
        }

        // Paint 4 rectangles around the cutout to create the spotlight effect
        g.setColour(juce::Colours::black.withAlpha(0.5f));

        // Top
        g.fillRect(area.getX(), area.getY(),
                    area.getWidth(), spotlightRect.getY() - area.getY());
        // Bottom
        g.fillRect(area.getX(), spotlightRect.getBottom(),
                    area.getWidth(), area.getBottom() - spotlightRect.getBottom());
        // Left
        g.fillRect(area.getX(), spotlightRect.getY(),
                    spotlightRect.getX() - area.getX(), spotlightRect.getHeight());
        // Right
        g.fillRect(spotlightRect.getRight(), spotlightRect.getY(),
                    area.getRight() - spotlightRect.getRight(), spotlightRect.getHeight());

        // Draw a subtle highlight border around the cutout
        g.setColour(ColorScheme::get().accentBlue.withAlpha(0.8f));
        g.drawRoundedRectangle(spotlightRect.toFloat(), 4.0f, 2.0f);
    }

    bool hitTest(int x, int y) override
    {
        // Passthrough mode: let all clicks through
        if (passthroughMode)
            return false;
        // Let clicks through inside the spotlight cutout
        if (!spotlightRect.isEmpty() && spotlightRect.contains(x, y))
            return false;
        // Block clicks on the dimmed area
        return true;
    }

    void mouseDown(const juce::MouseEvent&) override
    {
        // Clicks on the dimmed area are consumed (do nothing)
    }

private:
    juce::Rectangle<int> spotlightRect;
    bool passthroughMode = false;
};

//==============================================================================
/**
 * Floating instruction card that appears near the spotlight cutout.
 * Shows step indicator, title, description, and navigation buttons.
 */
class WizardInstructionCard : public juce::Component,
                               private ColorScheme::Manager::Listener,
                               private juce::Timer
{
public:
    WizardInstructionCard()
    {
        setOpaque(false);
        setAlwaysOnTop(true);

        // Step indicator
        addAndMakeVisible(stepIndicatorLabel);
        stepIndicatorLabel.setJustificationType(juce::Justification::centredLeft);

        // Title
        addAndMakeVisible(titleLabel);
        titleLabel.setJustificationType(juce::Justification::centredLeft);

        // Description
        addAndMakeVisible(descriptionLabel);
        descriptionLabel.setJustificationType(juce::Justification::topLeft);

        // Completion indicator
        addAndMakeVisible(completionLabel);
        completionLabel.setJustificationType(juce::Justification::centredLeft);
        completionLabel.setVisible(false);

        // Buttons
        addAndMakeVisible(backButton);
        backButton.setButtonText(LOC("wizard.buttons.back"));
        backButton.onClick = [this]() { if (onBack) onBack(); };

        addAndMakeVisible(nextButton);
        nextButton.setButtonText(LOC("wizard.buttons.next"));
        nextButton.onClick = [this]() { if (onNext) onNext(); };

        addAndMakeVisible(skipButton);
        skipButton.setButtonText(LOC("wizard.buttons.skip"));
        skipButton.onClick = [this]() { if (onSkip) onSkip(); };
        skipButton.setVisible(false);

        addAndMakeVisible(closeButton);
        closeButton.setButtonText(LOC("wizard.buttons.close"));
        closeButton.onClick = [this]() { if (onClose) onClose(); };

        updateColors();
        ColorScheme::Manager::getInstance().addListener(this);
    }

    ~WizardInstructionCard() override
    {
        ColorScheme::Manager::getInstance().removeListener(this);
    }

    void setStep(int currentStep, int totalSteps,
                 const juce::String& title, const juce::String& description,
                 bool isFirstStep, bool isLastStep, bool stepComplete, bool showSkip)
    {
        auto& loc = LocalizationManager::getInstance();
        std::map<juce::String, juce::String> params;
        params["current"] = juce::String(currentStep + 1);
        params["total"] = juce::String(totalSteps);
        stepIndicatorLabel.setText(loc.get("wizard.stepIndicator", params),
                                   juce::dontSendNotification);

        titleLabel.setText(title, juce::dontSendNotification);
        descriptionLabel.setText(description, juce::dontSendNotification);

        backButton.setVisible(!isFirstStep);
        nextButton.setButtonText(isLastStep ? LOC("wizard.buttons.done") : LOC("wizard.buttons.next"));
        skipButton.setVisible(showSkip);

        completionLabel.setVisible(stepComplete);
        completionLabel.setText("OK", juce::dontSendNotification);

        updateSize();
    }

    void positionRelativeTo(juce::Rectangle<int> spotlight, juce::Rectangle<int> parentBounds)
    {
        const int cardWidth = juce::jmax(400, (int)(parentBounds.getWidth() * 0.35f));
        updateSize(cardWidth);

        int cardHeight = getHeight();
        int x, y;

        // Try to position below the spotlight
        int spaceBelow = parentBounds.getBottom() - spotlight.getBottom();
        int spaceAbove = spotlight.getY() - parentBounds.getY();
        int spaceRight = parentBounds.getRight() - spotlight.getRight();
        int spaceLeft = spotlight.getX() - parentBounds.getX();

        if (spaceBelow >= cardHeight + 16)
        {
            x = spotlight.getX();
            y = spotlight.getBottom() + 12;
        }
        else if (spaceAbove >= cardHeight + 16)
        {
            x = spotlight.getX();
            y = spotlight.getY() - cardHeight - 12;
        }
        else if (spaceRight >= cardWidth + 16)
        {
            x = spotlight.getRight() + 12;
            y = spotlight.getY();
        }
        else if (spaceLeft >= cardWidth + 16)
        {
            x = spotlight.getX() - cardWidth - 12;
            y = spotlight.getY();
        }
        else
        {
            // Fallback: bottom-right corner of parent
            x = parentBounds.getRight() - cardWidth - 20;
            y = parentBounds.getBottom() - cardHeight - 20;
        }

        // Clamp to parent bounds
        x = juce::jlimit(parentBounds.getX() + 8, parentBounds.getRight() - cardWidth - 8, x);
        y = juce::jlimit(parentBounds.getY() + 8, parentBounds.getBottom() - cardHeight - 8, y);

        setBounds(x, y, cardWidth, cardHeight);
    }

    /** Position the card inside an external window's content area (bottom-right, in the preview zone). */
    void positionInExternalWindow(juce::Component* windowContent)
    {
        if (windowContent == nullptr) return;
        const int cardWidth = juce::jmax(360, (int)(windowContent->getWidth() * 0.42f));
        updateSize(cardWidth);
        int cardHeight = getHeight();
        int x = windowContent->getWidth() - cardWidth - 12;
        int y = windowContent->getHeight() - cardHeight - 50; // above the Apply/Close buttons
        setBounds(x, y, cardWidth, cardHeight);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Shadow
        juce::DropShadow shadow(juce::Colours::black.withAlpha(0.4f), 12, {0, 4});
        shadow.drawForRectangle(g, getLocalBounds());

        // Background
        g.setColour(ColorScheme::get().surfaceCard);
        g.fillRoundedRectangle(bounds, 8.0f);

        // Border
        g.setColour(ColorScheme::get().buttonBorder);
        g.drawRoundedRectangle(bounds, 8.0f, 1.0f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(14);
        float scale = WfsLookAndFeel::uiScale;

        int labelH = (int)(34 * scale);
        int titleH = (int)(43 * scale);
        int buttonH = (int)(40 * scale);

        stepIndicatorLabel.setBounds(area.removeFromTop(labelH));
        area.removeFromTop(4);

        // Title + completion on same row
        auto titleRow = area.removeFromTop(titleH);
        if (completionLabel.isVisible())
        {
            int checkW = (int)(50 * scale);
            completionLabel.setBounds(titleRow.removeFromRight(checkW));
        }
        titleLabel.setBounds(titleRow);
        area.removeFromTop(10);

        // Buttons at bottom
        auto buttonArea = area.removeFromBottom(buttonH);
        int buttonW = (int)(100 * scale);
        int buttonSpacing = 10;

        closeButton.setBounds(buttonArea.removeFromRight(buttonW));
        if (skipButton.isVisible())
        {
            buttonArea.removeFromRight(buttonSpacing);
            skipButton.setBounds(buttonArea.removeFromRight(buttonW));
        }
        buttonArea.removeFromRight(buttonSpacing);
        nextButton.setBounds(buttonArea.removeFromRight(buttonW));
        if (backButton.isVisible())
        {
            buttonArea.removeFromRight(buttonSpacing);
            backButton.setBounds(buttonArea.removeFromRight(buttonW));
        }

        area.removeFromBottom(10);

        // Description takes remaining space
        descriptionLabel.setBounds(area);
    }

    /** Start a fade-out: after delayMs, the card alpha drops over 2 seconds, then fires onClose. */
    void startFadeOut(int delayMs)
    {
        fadeAlpha = 1.0f;
        fadeDelayMs = delayMs;
        fadeElapsed = 0;
        startTimer(50); // 20 fps
    }

    void cancelFadeOut()
    {
        stopTimer();
        fadeAlpha = 1.0f;
        setAlpha(1.0f);
    }

    // Callbacks
    std::function<void()> onNext;
    std::function<void()> onBack;
    std::function<void()> onSkip;
    std::function<void()> onClose;

private:
    juce::Label stepIndicatorLabel;
    juce::Label titleLabel;
    juce::Label descriptionLabel;
    juce::Label completionLabel;
    juce::TextButton backButton;
    juce::TextButton nextButton;
    juce::TextButton skipButton;
    juce::TextButton closeButton;

    // Fade-out state
    float fadeAlpha = 1.0f;
    int fadeDelayMs = 0;
    int fadeElapsed = 0;

    void timerCallback() override
    {
        fadeElapsed += 50;
        if (fadeElapsed < fadeDelayMs)
            return; // still in delay phase

        int fadeTime = fadeElapsed - fadeDelayMs;
        const int fadeDuration = 2000; // 2 seconds fade
        fadeAlpha = juce::jmax(0.0f, 1.0f - (float)fadeTime / (float)fadeDuration);
        setAlpha(fadeAlpha);

        if (fadeAlpha <= 0.0f)
        {
            stopTimer();
            if (onClose)
                onClose();
        }
    }

    void colorSchemeChanged() override
    {
        updateColors();
        repaint();
    }

    void updateColors()
    {
        auto& palette = ColorScheme::get();

        stepIndicatorLabel.setColour(juce::Label::textColourId, palette.textSecondary);
        titleLabel.setColour(juce::Label::textColourId, palette.textPrimary);
        descriptionLabel.setColour(juce::Label::textColourId, palette.textSecondary);
        completionLabel.setColour(juce::Label::textColourId, palette.accentGreen);

        float scale = WfsLookAndFeel::uiScale;
        stepIndicatorLabel.setFont(juce::FontOptions().withHeight(juce::jmax(17.0f, 22.0f * scale)));
        titleLabel.setFont(juce::FontOptions().withHeight(juce::jmax(22.0f, 29.0f * scale)).withStyle("Bold"));
        descriptionLabel.setFont(juce::FontOptions().withHeight(juce::jmax(19.0f, 24.0f * scale)));
        completionLabel.setFont(juce::FontOptions().withHeight(juce::jmax(19.0f, 24.0f * scale)).withStyle("Bold"));
    }

    void updateSize(int width = 400)
    {
        float scale = WfsLookAndFeel::uiScale;
        int padding = 28; // 14px on each side

        int labelH = (int)(34 * scale);
        int titleH = (int)(43 * scale);
        int buttonH = (int)(40 * scale);
        int descTextWidth = width - padding;

        // Estimate description height from text content
        auto descText = descriptionLabel.getText();
        auto descFont = descriptionLabel.getFont();
        float lineH = descFont.getHeight() * 1.4f;
        // Approximate number of lines using GlyphArrangement
        juce::GlyphArrangement glyphs;
        glyphs.addJustifiedText(descFont, descText, 0.0f, 0.0f, (float)descTextWidth, juce::Justification::left);
        int numLines = glyphs.getNumGlyphs() > 0
            ? (int)((glyphs.getBoundingBox(glyphs.getNumGlyphs() - 1, 1, true).getBottom()) / lineH) + 1
            : 1;
        int descHeight = juce::jlimit((int)(50 * scale), (int)(200 * scale), (int)(numLines * lineH + lineH));

        int totalHeight = padding + labelH + 4 + titleH + 10 + descHeight + 10 + buttonH + padding / 2;
        setSize(width, totalHeight);
    }
};

//==============================================================================
/**
 * Getting Started Wizard — orchestrates the spotlight tutorial flow.
 * Not a Component itself; manages WizardSpotlightOverlay and WizardInstructionCard.
 */
class GettingStartedWizard : private juce::KeyListener
{
public:
    GettingStartedWizard(juce::Component& parentComponent,
                          std::function<void(int)> switchTabCallback)
        : parent(parentComponent),
          switchTab(std::move(switchTabCallback))
    {
        overlay = std::make_unique<WizardSpotlightOverlay>();
        card = std::make_unique<WizardInstructionCard>();

        card->onNext = [this]() { nextStep(); };
        card->onBack = [this]() { previousStep(); };
        card->onClose = [this]() { close(); };
        card->onSkip = [this]() {
            if (currentStepIndex >= 0 && currentStepIndex < (int)steps.size())
            {
                int target = steps[(size_t)currentStepIndex].skipToStepIndex;
                if (target >= 0)
                    skipToStep(target);
            }
        };

        parent.addKeyListener(this);
    }

    ~GettingStartedWizard()
    {
        parent.removeKeyListener(this);
        returnCardToParent();
        if (overlay)
            parent.removeChildComponent(overlay.get());
        if (card)
            parent.removeChildComponent(card.get());
    }

    void addStep(WizardStep step)
    {
        steps.push_back(std::move(step));
    }

    void start()
    {
        startFromStep(0);
    }

    void startFromStep(int stepIndex)
    {
        if (steps.empty()) return;

        currentStepIndex = juce::jlimit(0, (int)steps.size() - 1, stepIndex);

        parent.addAndMakeVisible(overlay.get());
        parent.addAndMakeVisible(card.get());
        overlay->toFront(false);
        card->toFront(true);

        updateLayout();
        showCurrentStep();
    }

    void close()
    {
        returnCardToParent();

        auto* overlayPtr = overlay.release();
        auto* cardPtr = card.release();

        // Use MessageManager::callAsync for safe cleanup (same pattern as ChannelSelectorOverlay)
        juce::MessageManager::callAsync([this, overlayPtr, cardPtr]()
        {
            parent.removeChildComponent(overlayPtr);
            parent.removeChildComponent(cardPtr);
            delete overlayPtr;
            delete cardPtr;

            if (onWizardClosed)
                onWizardClosed();
        });
    }

    void nextStep()
    {
        exitCurrentStep();

        if (currentStepIndex < (int)steps.size() - 1)
        {
            currentStepIndex++;
            showCurrentStep();
        }
        else
        {
            // Last step — close the wizard
            close();
        }
    }

    void previousStep()
    {
        exitCurrentStep();

        if (currentStepIndex > 0)
        {
            currentStepIndex--;
            showCurrentStep();
        }
    }

    void skipToStep(int index)
    {
        exitCurrentStep();

        if (index >= 0 && index < (int)steps.size())
        {
            currentStepIndex = index;
            showCurrentStep();
        }
    }

    void updateLayout()
    {
        if (overlay)
            overlay->setBounds(parent.getLocalBounds());

        if (card && !steps.empty() && currentStepIndex >= 0 && currentStepIndex < (int)steps.size())
        {
            auto& step = steps[(size_t)currentStepIndex];

            // If card is on an external window, reposition within that window
            if (step.getExternalWindowContent)
            {
                auto* extContent = step.getExternalWindowContent();
                if (extContent != nullptr && card->getParentComponent() == extContent)
                {
                    card->positionInExternalWindow(extContent);
                    overlay->clearSpotlight();
                    return;
                }
            }

            auto spotlightBounds = step.getSpotlightBounds ? step.getSpotlightBounds() : juce::Rectangle<int>();

            if (!spotlightBounds.isEmpty())
                overlay->setSpotlightBounds(spotlightBounds);
            else
                overlay->clearSpotlight();

            card->positionRelativeTo(spotlightBounds.isEmpty() ? parent.getLocalBounds().reduced(100) : spotlightBounds,
                                     parent.getLocalBounds());
        }
    }

    bool isActive() const { return overlay != nullptr && overlay->isVisible(); }
    int getCurrentStepIndex() const { return currentStepIndex; }

    std::function<void()> onWizardClosed;

private:
    juce::Component& parent;
    std::function<void(int)> switchTab;
    std::unique_ptr<WizardSpotlightOverlay> overlay;
    std::unique_ptr<WizardInstructionCard> card;
    std::vector<WizardStep> steps;
    int currentStepIndex = 0;
    bool cardOnExternalWindow = false;

    bool keyPressed(const juce::KeyPress& key, juce::Component*) override
    {
        if (key == juce::KeyPress::escapeKey && isActive())
        {
            // Don't close if a TextEditor has focus (user is editing a value)
            if (auto* focused = juce::Component::getCurrentlyFocusedComponent())
                if (dynamic_cast<juce::TextEditor*>(focused) != nullptr)
                    return false;

            close();
            return true;
        }
        return false;
    }

    /** Fire the onExit callback for the current step (e.g. close an external window). */
    void exitCurrentStep()
    {
        if (currentStepIndex >= 0 && currentStepIndex < (int)steps.size())
        {
            auto& step = steps[(size_t)currentStepIndex];
            if (step.onExit)
                step.onExit();
        }
    }

    /** Move the card back to the main parent if it's currently on an external window. */
    void returnCardToParent()
    {
        if (cardOnExternalWindow && card)
        {
            if (auto* currentParent = card->getParentComponent())
                currentParent->removeChildComponent(card.get());
            parent.addAndMakeVisible(card.get());
            cardOnExternalWindow = false;
        }
    }

    void showCurrentStep()
    {
        if (currentStepIndex < 0 || currentStepIndex >= (int)steps.size())
            return;

        auto& step = steps[(size_t)currentStepIndex];

        // Switch tab if needed
        if (step.tabIndex >= 0 && switchTab)
            switchTab(step.tabIndex);

        // Run onEnter callback
        if (step.onEnter)
            step.onEnter();

        // Update spotlight — use callAsync so the tab switch/layout has time to complete
        juce::MessageManager::callAsync([this]()
        {
            if (!overlay || !card || currentStepIndex < 0 || currentStepIndex >= (int)steps.size())
                return;

            auto& s = steps[(size_t)currentStepIndex];

            // Check if this step targets an external window
            juce::Component* extContent = nullptr;
            if (s.getExternalWindowContent)
                extContent = s.getExternalWindowContent();

            if (extContent != nullptr)
            {
                // Reparent card to external window content
                returnCardToParent(); // safety: return first if already somewhere else
                parent.removeChildComponent(card.get());
                extContent->addAndMakeVisible(card.get());
                cardOnExternalWindow = true;

                // Dim the main window fully
                overlay->clearSpotlight();

                bool complete = s.isComplete ? s.isComplete() : false;
                card->setStep(currentStepIndex, (int)steps.size(),
                              LOC(s.titleKey), LOC(s.descriptionKey),
                              currentStepIndex == 0,
                              currentStepIndex == (int)steps.size() - 1,
                              complete, s.skipToStepIndex >= 0);

                card->positionInExternalWindow(extContent);
                card->toFront(true);
            }
            else
            {
                // Return card to main parent if it was on an external window
                returnCardToParent();

                auto spotlightBounds = s.getSpotlightBounds ? s.getSpotlightBounds() : juce::Rectangle<int>();

                // Free interaction mode: overlay passes all clicks through, card fades out
                if (s.freeInteraction)
                {
                    overlay->setPassthrough(true);
                    overlay->clearSpotlight();
                }
                else
                {
                    overlay->setPassthrough(false);
                    if (!spotlightBounds.isEmpty())
                        overlay->setSpotlightBounds(spotlightBounds);
                    else
                        overlay->clearSpotlight();
                }

                bool complete = s.isComplete ? s.isComplete() : false;
                card->cancelFadeOut();
                card->setStep(currentStepIndex, (int)steps.size(),
                              LOC(s.titleKey), LOC(s.descriptionKey),
                              currentStepIndex == 0,
                              currentStepIndex == (int)steps.size() - 1,
                              complete, s.skipToStepIndex >= 0);

                card->positionRelativeTo(spotlightBounds.isEmpty() ? parent.getLocalBounds().reduced(100) : spotlightBounds,
                                         parent.getLocalBounds());

                overlay->toFront(false);
                card->toFront(true);

                // Start fade-out for free interaction steps
                if (s.freeInteraction)
                    card->startFadeOut(10000); // fade after 10 seconds
            }
        });
    }
};
