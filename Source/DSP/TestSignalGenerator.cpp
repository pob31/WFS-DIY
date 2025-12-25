#include "TestSignalGenerator.h"

TestSignalGenerator::TestSignalGenerator()
{
}

void TestSignalGenerator::prepare(double newSampleRate, int maxBlockSize)
{
    sampleRate = newSampleRate;

    // Initialize pink noise state
    for (int i = 0; i < 7; ++i)
        pinkNoiseState[i] = 0.0f;

    // Initialize random number generator
    random = juce::Random(juce::Time::currentTimeMillis());

    // Reset all state
    phase = 0.0f;
    sweepPosition = 0.0f;
    pulsePlayed = false;
    pulseGapPosition = 0.0f;
    fadePosition.store(0.0f);
}

void TestSignalGenerator::setSignalType(SignalType type)
{
    SignalType oldType = currentType.load();
    if (oldType != type)
    {
        currentType.store(type);

        // Reset state when changing signal type
        phase = 0.0f;
        sweepPosition = 0.0f;
        pulsePlayed = false;
        pulseGapPosition = 0.0f;

        // Reset fade for pink noise and tone (500ms fade-in)
        if (type == SignalType::PinkNoise || type == SignalType::Tone)
            fadePosition.store(0.0f);
        else
            fadePosition.store(1.0f);  // No fade for sweep/pulse
    }
}

void TestSignalGenerator::setFrequency(float hz)
{
    frequency = juce::jlimit(20.0f, 20000.0f, hz);
    phaseIncrement = frequency / static_cast<float>(sampleRate);
}

void TestSignalGenerator::setLevel(float dB)
{
    levelLinear.store(juce::Decibels::decibelsToGain(dB));
}

float TestSignalGenerator::getLevelDb() const
{
    return juce::Decibels::gainToDecibels(levelLinear.load());
}

void TestSignalGenerator::setOutputChannel(int channel)
{
    DBG("TestSignalGenerator::setOutputChannel(" + juce::String(channel) +
        ") - currentType=" + juce::String((int)currentType.load()) +
        ", level=" + juce::String(levelLinear.load(), 3));

    int oldChannel = targetChannel.load();
    targetChannel.store(channel);

    // Reset fade when starting playback (for Pink Noise and Tone)
    // This ensures 500ms fade-in every time test signal starts
    SignalType type = currentType.load();
    if (channel >= 0 && oldChannel < 0)
    {
        if (type == SignalType::PinkNoise || type == SignalType::Tone)
        {
            fadePosition.store(0.0f);
        }
    }

    // Reset pulse flag when channel changes
    if (type == SignalType::DiracPulse)
        pulsePlayed = false;
}

void TestSignalGenerator::setHoldEnabled(bool hold)
{
    holdEnabled.store(hold);
}

bool TestSignalGenerator::isActive() const
{
    return targetChannel.load() >= 0 && currentType.load() != SignalType::Off;
}

void TestSignalGenerator::reset()
{
    targetChannel.store(-1);
    currentType.store(SignalType::Off);
    fadePosition.store(0.0f);
    holdEnabled.store(false);
}

void TestSignalGenerator::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                          int startSample,
                                          int numSamples)
{
    int channel = targetChannel.load();
    SignalType signalType = currentType.load();
    float level = levelLinear.load();

    // Early exit if inactive or channel out of range
    if (channel < 0 || channel >= outputBuffer.getNumChannels() || signalType == SignalType::Off)
    {
        // Debug: Log why we're not generating
        static int debugCounter = 0;
        if (++debugCounter % 1000 == 0)  // Log every 1000 calls to avoid spam
        {
            DBG("TestSignal: Not active - channel=" + juce::String(channel) +
                ", bufferChannels=" + juce::String(outputBuffer.getNumChannels()) +
                ", signalType=" + juce::String((int)signalType) +
                ", level=" + juce::String(level, 3));
        }
        return;
    }

    // Debug: Log that we're generating (occasionally)
    static int activeCounter = 0;
    if (++activeCounter % 5000 == 0)  // Log every 5000 calls
    {
        DBG("TestSignal: ACTIVE - channel=" + juce::String(channel) +
            ", signalType=" + juce::String((int)signalType) +
            ", level=" + juce::String(level, 3));
    }

    auto* channelData = outputBuffer.getWritePointer(channel, startSample);
    float currentFade = fadePosition.load();
    const float fadeStep = 1.0f / (fadeDuration * static_cast<float>(sampleRate));

    for (int i = 0; i < numSamples; ++i)
    {
        float sample = 0.0f;

        // Generate signal based on type
        switch (signalType)
        {
            case SignalType::PinkNoise:
                sample = generatePinkNoise();
                break;

            case SignalType::Tone:
                sample = std::sin(phase * juce::MathConstants<float>::twoPi);
                phase += phaseIncrement;
                if (phase >= 1.0f)
                    phase -= 1.0f;
                break;

            case SignalType::Sweep:
                sample = generateSweep();
                break;

            case SignalType::DiracPulse:
                if (!pulsePlayed)
                {
                    sample = 1.0f;
                    pulsePlayed = true;
                    pulseGapPosition = 0.0f;
                }
                else
                {
                    sample = 0.0f;
                    // Increment gap counter and reset after pulseGap seconds
                    pulseGapPosition += 1.0f / static_cast<float>(sampleRate);
                    if (pulseGapPosition >= pulseGap)
                    {
                        pulsePlayed = false;  // Ready for next pulse
                    }
                }
                break;

            default:
                sample = 0.0f;
                break;
        }

        // Apply fade-in and level
        sample *= level * currentFade;

        // Replace output (not mix) - for testing purposes
        channelData[i] = sample;

        // Update fade
        if (currentFade < 1.0f)
        {
            currentFade = juce::jmin(1.0f, currentFade + fadeStep);
        }
    }

    fadePosition.store(currentFade);
}

float TestSignalGenerator::generatePinkNoise()
{
    // Gardner method for pink noise (1/f spectrum)
    // Uses 7 octave filters for approximation

    float white = random.nextFloat() * 2.0f - 1.0f;

    pinkNoiseState[0] = 0.99886f * pinkNoiseState[0] + white * 0.0555179f;
    pinkNoiseState[1] = 0.99332f * pinkNoiseState[1] + white * 0.0750759f;
    pinkNoiseState[2] = 0.96900f * pinkNoiseState[2] + white * 0.1538520f;
    pinkNoiseState[3] = 0.86650f * pinkNoiseState[3] + white * 0.3104856f;
    pinkNoiseState[4] = 0.55000f * pinkNoiseState[4] + white * 0.5329522f;
    pinkNoiseState[5] = -0.7616f * pinkNoiseState[5] - white * 0.0168980f;

    float pink = pinkNoiseState[0] + pinkNoiseState[1] + pinkNoiseState[2] +
                 pinkNoiseState[3] + pinkNoiseState[4] + pinkNoiseState[5] +
                 pinkNoiseState[6] + white * 0.5362f;

    pinkNoiseState[6] = white * 0.115926f;

    // Normalize (approximate)
    return pink * 0.11f;
}

float TestSignalGenerator::generateSweep()
{
    // Logarithmic sweep from 20Hz to 20kHz over 1 second
    // Followed by 1 second of silence

    float cyclePosition = sweepPosition / (sweepDuration + sweepGap);

    if (sweepPosition < sweepDuration)
    {
        // Sweep phase (0 to 1 second)
        float t = sweepPosition / sweepDuration;  // 0.0 to 1.0

        // Logarithmic frequency interpolation
        float logStart = std::log(sweepStartHz);
        float logEnd = std::log(sweepEndHz);
        float currentFreq = std::exp(logStart + t * (logEnd - logStart));

        // Calculate instantaneous phase increment
        float instantPhaseInc = currentFreq / static_cast<float>(sampleRate);

        // Generate sine wave
        float sample = std::sin(phase * juce::MathConstants<float>::twoPi);

        // Update phase
        phase += instantPhaseInc;
        if (phase >= 1.0f)
            phase -= 1.0f;

        // Update sweep position
        sweepPosition += 1.0f / static_cast<float>(sampleRate);

        return sample;
    }
    else
    {
        // Gap phase (1 to 2 seconds)
        sweepPosition += 1.0f / static_cast<float>(sampleRate);

        // Reset at end of cycle
        if (sweepPosition >= (sweepDuration + sweepGap))
        {
            sweepPosition = 0.0f;
            phase = 0.0f;
        }

        return 0.0f;  // Silence during gap
    }
}
