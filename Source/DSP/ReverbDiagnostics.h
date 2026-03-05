#pragma once

// Master switch: set to 1 to enable, 0 to disable all diagnostics
#define REVERB_DIAGNOSTICS 0

#if REVERB_DIAGNOSTICS

#include <JuceHeader.h>
#include <atomic>
#include <cmath>

//==============================================================================
/**
    Shared diagnostic counters for ReverbEngine.
    All members are std::atomic so they can be written from the engine thread
    and read from the timer/message thread without locks.
*/
struct ReverbDiagnostics
{
    // --- A. Ring buffer overflow (input dropped) ---
    std::atomic<uint64_t> inputOverflowSamples  { 0 };
    std::atomic<uint64_t> inputOverflowEvents   { 0 };

    // --- B. Ring buffer underrun (output not ready) ---
    std::atomic<uint64_t> outputUnderrunSamples { 0 };
    std::atomic<uint64_t> outputUnderrunEvents  { 0 };

    // --- Buffer level monitoring (snapshot) ---
    std::atomic<int> inputBufferMinLevel  { 0 };
    std::atomic<int> inputBufferMaxLevel  { 0 };
    std::atomic<int> outputBufferMinLevel { 0 };
    std::atomic<int> outputBufferMaxLevel { 0 };

    // --- C. Convolution readiness ---
    std::atomic<int>  irCurrentSize     { 0 };
    std::atomic<int>  irExpectedSize    { 0 };
    std::atomic<bool> irFullyLoaded     { false };

    // --- D. Signal corruption ---
    std::atomic<uint64_t> nanInfCount     { 0 };
    std::atomic<float>    peakOutputLevel { 0.0f };
    std::atomic<uint64_t> clippingCount   { 0 };

    // --- E. Processing time ---
    std::atomic<float>    lastProcessBlockUs { 0.0f };
    std::atomic<float>    maxProcessBlockUs  { 0.0f };
    std::atomic<float>    avgProcessBlockUs  { 0.0f };
    std::atomic<float>    budgetUs           { 0.0f };
    std::atomic<uint64_t> overbudgetCount    { 0 };

    // --- F. Engine state ---
    std::atomic<uint64_t> blocksProcessed   { 0 };
    std::atomic<int>      fadeStateSnapshot  { 0 };
    std::atomic<int>      algorithmType     { -1 };

    void resetCounters()
    {
        inputOverflowSamples.store (0);
        inputOverflowEvents.store (0);
        outputUnderrunSamples.store (0);
        outputUnderrunEvents.store (0);
        nanInfCount.store (0);
        peakOutputLevel.store (0.0f);
        clippingCount.store (0);
        maxProcessBlockUs.store (0.0f);
        overbudgetCount.store (0);
        blocksProcessed.store (0);
    }
};

//==============================================================================
/**
    Timer-based reporter: polls ReverbDiagnostics every N ms
    and outputs a formatted DBG summary. Runs on the message thread.
*/
class ReverbDiagnosticReporter : public juce::Timer
{
public:
    explicit ReverbDiagnosticReporter (ReverbDiagnostics& d) : diag (d) {}

    void startReporting (int intervalMs = 1000) { startTimer (intervalMs); }
    void stopReporting() { stopTimer(); }

private:
    void timerCallback() override
    {
        auto overflows    = diag.inputOverflowSamples.load();
        auto overflowEvts = diag.inputOverflowEvents.load();
        auto underruns    = diag.outputUnderrunSamples.load();
        auto underrunEvts = diag.outputUnderrunEvents.load();

        auto inMin  = diag.inputBufferMinLevel.load();
        auto inMax  = diag.inputBufferMaxLevel.load();
        auto outMin = diag.outputBufferMinLevel.load();
        auto outMax = diag.outputBufferMaxLevel.load();

        auto irCur   = diag.irCurrentSize.load();
        auto irExp   = diag.irExpectedSize.load();
        auto irReady = diag.irFullyLoaded.load();

        auto nans = diag.nanInfCount.load();
        auto peak = diag.peakOutputLevel.load();
        auto clips = diag.clippingCount.load();

        auto lastUs     = diag.lastProcessBlockUs.load();
        auto maxUs      = diag.maxProcessBlockUs.load();
        auto avgUs      = diag.avgProcessBlockUs.load();
        auto budget     = diag.budgetUs.load();
        auto overbudget = diag.overbudgetCount.load();

        auto blocks = diag.blocksProcessed.load();
        auto fade   = diag.fadeStateSnapshot.load();
        auto algo   = diag.algorithmType.load();

        juce::String report;
        report << "=== REVERB DIAGNOSTICS ===" << juce::newLine;
        report << "Blocks: " << (int64_t) blocks
               << "  Algo: " << algo
               << "  Fade: " << fade << juce::newLine;

        report << "INPUT  overflow: " << (int64_t) overflows << " samples / "
               << (int64_t) overflowEvts << " events" << juce::newLine;

        report << "OUTPUT underrun: " << (int64_t) underruns << " samples / "
               << (int64_t) underrunEvts << " events" << juce::newLine;

        report << "BufLvl IN [" << inMin << ".." << inMax << "]"
               << "  OUT [" << outMin << ".." << outMax << "]" << juce::newLine;

        report << "IR size: " << irCur << "/" << irExp
               << (irReady ? " READY" : " NOT READY") << juce::newLine;

        report << "Signal: peak=" << peak
               << "  NaN/Inf=" << (int64_t) nans
               << "  clips(>10)=" << (int64_t) clips << juce::newLine;

        report << "Timing: last=" << juce::String (lastUs, 0) << "us"
               << "  avg=" << juce::String (avgUs, 0) << "us"
               << "  max=" << juce::String (maxUs, 0) << "us"
               << "  budget=" << juce::String (budget, 0) << "us"
               << "  overbudget=" << (int64_t) overbudget << juce::newLine;

        if (overflows > 0)
            report << ">>> LIKELY CAUSE: Ring buffer OVERFLOW (engine too slow)" << juce::newLine;
        if (underruns > 0)
            report << ">>> LIKELY CAUSE: Ring buffer UNDERRUN (output not ready)" << juce::newLine;
        if (! irReady && algo == 2)
            report << ">>> LIKELY CAUSE: IR NOT LOADED (processing with Dirac)" << juce::newLine;
        if (nans > 0)
            report << ">>> LIKELY CAUSE: NaN/Inf CORRUPTION in signal" << juce::newLine;
        if (peak > 10.0f)
            report << ">>> WARNING: Peak output " << peak << " exceeds +20dB" << juce::newLine;
        if (overbudget > 0)
            report << ">>> WARNING: " << (int64_t) overbudget
                   << " blocks exceeded time budget" << juce::newLine;

        DBG (report);
    }

    ReverbDiagnostics& diag;
};

#endif // REVERB_DIAGNOSTICS
