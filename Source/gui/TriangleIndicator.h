#pragma once

#include <JuceHeader.h>

/**
    A triangle that lights for 250 ms when something fires and fades back on
    its own timer. The AutomOtion blocks use a pair: an up triangle when the
    audio trigger crossed its threshold, a down triangle when the level fell
    back under the rearm threshold.

    Moved out of InputsTab unchanged so the effects AutomOtion block can show
    the same two indicators without a second copy of the timer.
*/
class TriangleIndicator : public juce::Component,
                          private juce::Timer
{
public:
    enum Direction { Up, Down };

    TriangleIndicator (Direction dir, juce::Colour activeCol)
        : direction (dir), activeColour (activeCol) {}

    void setActive (bool shouldBeActive)
    {
        if (shouldBeActive)
        {
            active = true;
            lastActiveTime = juce::Time::getMillisecondCounter();
            if (! isTimerRunning()) startTimer (50);
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (1.0f);
        juce::Path tri;
        if (direction == Up)
        {
            tri.startNewSubPath (bounds.getCentreX(), bounds.getY());
            tri.lineTo (bounds.getRight(), bounds.getBottom());
            tri.lineTo (bounds.getX(), bounds.getBottom());
        }
        else
        {
            tri.startNewSubPath (bounds.getX(), bounds.getY());
            tri.lineTo (bounds.getRight(), bounds.getY());
            tri.lineTo (bounds.getCentreX(), bounds.getBottom());
        }
        tri.closeSubPath();
        g.setColour (active ? activeColour : juce::Colour (0xFF1A1A1A));
        g.fillPath (tri);
    }

private:
    void timerCallback() override
    {
        if (! active) { stopTimer(); return; }
        auto now = juce::Time::getMillisecondCounter();
        if (now - lastActiveTime >= 250)
        {
            active = false;
            stopTimer();
            repaint();
        }
    }

    Direction direction;
    juce::Colour activeColour;
    bool active = false;
    juce::uint32 lastActiveTime = 0;
};
