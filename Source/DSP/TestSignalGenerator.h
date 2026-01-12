#pragma once

#include <JuceHeader.h>
#include <atomic>

/**
 * TestSignalGenerator
 *
 * Generates test signals for audio output testing:
 * - Pink Noise: Continuous pink noise with 500ms fade-in
 * - Tone: Continuous sine wave at configurable frequency (20-20000 Hz)
 * - Sweep: Logarithmic sweep from 20Hz to 20kHz over 1 second with 1s gap
 * - Dirac Pulse: Single-sample click/burst for impulse testing
 *
 * Thread-safe design for use in audio callback. Test signals are injected
 * after WFS processing, directly to hardware output channels.
 */
class TestSignalGenerator
{
public:
    enum class SignalType
    {
        Off,
        PinkNoise,
        Tone,
        Sweep,
        DiracPulse
    };

    TestSignalGenerator();
    ~TestSignalGenerator() = default;

    /**
     * Prepare the generator for playback
     * Must be called before renderNextBlock
     */
    void prepare(double sampleRate, int maxBlockSize);

    /**
     * Set the type of test signal to generate
     */
    void setSignalType(SignalType type);

    /**
     * Set the frequency for Tone mode (20-20000 Hz)
     */
    void setFrequency(float hz);

    /**
     * Set the output level in dB (-92 to 0 dB)
     */
    void setLevel(float dB);

    /**
     * Set the target output channel (-1 to disable)
     */
    void setOutputChannel(int channel);

    /**
     * Enable/disable hold mode (maintains signal when releasing click)
     */
    void setHoldEnabled(bool hold);

    /**
     * Check if the generator is currently active
     */
    bool isActive() const;

    /**
     * Get current signal type
     */
    SignalType getSignalType() const { return currentType.load(); }

    /**
     * Get current level in dB
     */
    float getLevelDb() const;

    /**
     * Get current frequency for Tone mode (Hz)
     */
    float getFrequency() const { return frequency; }

    /**
     * Check if hold mode is enabled
     */
    bool isHoldEnabled() const { return holdEnabled.load(); }

    /**
     * Thread-safe reset (stops all signal generation)
     */
    void reset();

    /**
     * Render the next block of audio (called from audio thread)
     * Injects test signal directly to the target output channel
     */
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                         int startSample,
                         int numSamples);

private:
    // Pink noise generation using Gardner method (7 octave filters)
    float generatePinkNoise();

    // Logarithmic sweep generation
    float generateSweep();

    // Atomic state for thread-safe access from audio thread
    std::atomic<int> targetChannel{-1};
    std::atomic<float> fadePosition{0.0f};  // 0.0 to 1.0 for fade-in
    std::atomic<bool> holdEnabled{false};

    // Signal type (atomic for thread-safe access from UI and audio threads)
    std::atomic<SignalType> currentType{SignalType::Off};
    double sampleRate = 48000.0;
    std::atomic<float> levelLinear{0.01f};  // Default: -40 dB (10^(-40/20) = 0.01)
    float frequency = 1000.0f;

    // Pink noise state (Gardner method)
    float pinkNoiseState[7] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    juce::Random random;

    // Tone generator
    float phase = 0.0f;
    float phaseIncrement = 0.0f;

    // Sweep generator
    float sweepPosition = 0.0f;  // Position in sweep cycle (0.0 to 4.0)
    static constexpr float sweepDuration = 1.0f;  // 1 second sweep
    static constexpr float sweepGap = 3.0f;       // 3 seconds gap
    static constexpr float sweepStartHz = 20.0f;
    static constexpr float sweepEndHz = 20000.0f;

    // Dirac pulse (with repeat)
    bool pulsePlayed = false;
    float pulseGapPosition = 0.0f;  // Gap counter for repeat
    static constexpr float pulseGap = 1.0f;  // 1 second between pulses

    // Fade constants
    static constexpr float fadeDuration = 0.5f;  // 500ms fade-in

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestSignalGenerator)
};
