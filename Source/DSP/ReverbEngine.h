#pragma once

#include <JuceHeader.h>
#include "../LockFreeRingBuffer.h"
#include "ReverbAlgorithm.h"
#include "ReverbFDNAlgorithm.h"
#include "ReverbSDNAlgorithm.h"
#include "ReverbIRAlgorithm.h"
#include <atomic>
#include <memory>
#include <vector>

//==============================================================================
/**
    ReverbEngine

    Thread-based reverb processor for the WFS system.
    Processes per-node reverb audio through a pre-processing → algorithm → post-processing chain.

    Integration pattern (mirrors BinauralProcessor):
    - Audio callback pushes per-node feed audio via pushNodeInput()
    - Audio callback pulls per-node wet output via pullNodeOutput()
    - Timer callback pushes parameters at 50Hz via setter methods
    - Engine runs on its own high-priority thread

    Supports three algorithms: SDN (0), FDN (1), IR (2).
    Algorithm switching creates a new instance and replaces the active one.
*/
class ReverbEngine : public juce::Thread
{
public:
    ReverbEngine()
        : juce::Thread ("ReverbEngine")
    {
    }

    ~ReverbEngine() override
    {
        stopThread (1000);
    }

    //==========================================================================
    // Lifecycle
    //==========================================================================

    /**
        Prepare for playback. Allocate all ring buffers and working buffers.
        Must be called before startProcessing().
    */
    void prepareToPlay (double newSampleRate, int maxBlockSize, int numNodes)
    {
        sampleRate = newSampleRate;
        currentBlockSize = maxBlockSize;
        numReverbNodes = numNodes;

        // Use 256-sample internal blocks (reverb is not latency-critical)
        internalBlockSize = juce::jmin (256, maxBlockSize);

        // Create per-node ring buffers (4x block size for safety)
        int ringSize = maxBlockSize * 4;

        nodeInputBuffers.clear();
        nodeOutputBuffers.clear();

        for (int i = 0; i < numNodes; ++i)
        {
            nodeInputBuffers.push_back (std::make_unique<LockFreeRingBuffer>());
            nodeInputBuffers.back()->setSize (ringSize);

            nodeOutputBuffers.push_back (std::make_unique<LockFreeRingBuffer>());
            nodeOutputBuffers.back()->setSize (ringSize);
        }

        // Working buffers for internal processing
        nodeInputBlock.setSize (numNodes, internalBlockSize);
        nodeOutputBlock.setSize (numNodes, internalBlockSize);

        // Prepare the active algorithm if one exists
        if (algorithm)
            algorithm->prepare (sampleRate, internalBlockSize, numNodes);
    }

    /**
        Release all resources. Stops the thread first.
    */
    void releaseResources()
    {
        stopThread (1000);
        nodeInputBuffers.clear();
        nodeOutputBuffers.clear();
    }

    /**
        Start the processing thread.
    */
    void startProcessing()
    {
        if (! isThreadRunning())
            startThread (juce::Thread::Priority::high);
    }

    /**
        Stop the processing thread.
    */
    void stopProcessing()
    {
        stopThread (1000);
    }

    /**
        Reset all internal state to silence.
    */
    void reset()
    {
        for (auto& buf : nodeInputBuffers)
            buf->reset();
        for (auto& buf : nodeOutputBuffers)
            buf->reset();

        if (algorithm)
            algorithm->reset();
    }

    //==========================================================================
    // Audio Callback Interface (called from audio thread)
    //==========================================================================

    /**
        Push feed audio for a reverb node. Called from the audio callback.
        @param nodeIndex    Reverb node index (0-based)
        @param data         Input samples
        @param numSamples   Number of samples
    */
    void pushNodeInput (int nodeIndex, const float* data, int numSamples)
    {
        if (nodeIndex >= 0 && nodeIndex < (int) nodeInputBuffers.size())
            nodeInputBuffers[nodeIndex]->write (data, numSamples);
    }

    /**
        Pull wet output for a reverb node. Called from the audio callback.
        Zero-pads if not enough data is available (prevents glitches on underrun).
        @param nodeIndex    Reverb node index (0-based)
        @param dest         Destination buffer
        @param numSamples   Number of samples
    */
    void pullNodeOutput (int nodeIndex, float* dest, int numSamples)
    {
        if (nodeIndex >= 0 && nodeIndex < (int) nodeOutputBuffers.size())
        {
            int samplesRead = nodeOutputBuffers[nodeIndex]->read (dest, numSamples);

            // Zero-pad if not enough data (underrun)
            if (samplesRead < numSamples)
                juce::FloatVectorOperations::clear (dest + samplesRead, numSamples - samplesRead);
        }
        else
        {
            juce::FloatVectorOperations::clear (dest, numSamples);
        }
    }

    //==========================================================================
    // Parameter Setters (called from timer thread at 50Hz)
    //==========================================================================

    /** Set algorithm parameters (RT60, diffusion, size, wet level, etc.) */
    void setAlgorithmParameters (const AlgorithmParameters& params)
    {
        // Store locally for thread-safe access
        pendingParams.store (params);
        paramsChanged.store (true, std::memory_order_release);
    }

    /** Update node positions (for SDN geometry calculations) */
    void updateGeometry (const std::vector<NodePosition>& positions)
    {
        juce::SpinLock::ScopedLockType lock (geometryLock);
        pendingGeometry = positions;
        geometryChanged.store (true, std::memory_order_release);
    }

    /** Set the active algorithm instance. Ownership is transferred. */
    void setAlgorithm (std::unique_ptr<ReverbAlgorithm> newAlgorithm)
    {
        juce::SpinLock::ScopedLockType lock (algorithmLock);
        algorithm = std::move (newAlgorithm);

        if (algorithm && sampleRate > 0)
            algorithm->prepare (sampleRate, internalBlockSize, numReverbNodes);
    }

    /** Algorithm type constants matching WFSParameterIDs::reverbAlgoType */
    enum AlgorithmType { SDN = 0, FDN = 1, IR = 2 };

    /**
        Set algorithm type by ID. Creates a new algorithm instance if the type changed.
        @param type  0=SDN, 1=FDN, 2=IR
    */
    void setAlgorithmType (int type)
    {
        if (type == currentAlgorithmType)
            return;

        currentAlgorithmType = type;

        std::unique_ptr<ReverbAlgorithm> newAlgo;
        switch (type)
        {
            case SDN: newAlgo = std::make_unique<SDNAlgorithm>(); break;
            case FDN: newAlgo = std::make_unique<FDNAlgorithm>(); break;
            case IR:  newAlgo = std::make_unique<IRAlgorithm>();  break;
            default:  newAlgo = std::make_unique<FDNAlgorithm>(); break;
        }

        setAlgorithm (std::move (newAlgo));
    }

    /** Get the current algorithm type. */
    int getAlgorithmType() const { return currentAlgorithmType; }

    /** Load an IR file (only effective when IR algorithm is active). */
    void loadIRFile (const juce::File& file)
    {
        juce::SpinLock::ScopedLockType lock (algorithmLock);
        if (auto* ir = dynamic_cast<IRAlgorithm*> (algorithm.get()))
            ir->loadIRFile (file);
    }

    /** Set IR parameters (trim, length). Only effective for IR algorithm. */
    void setIRParameters (float trimMs, float lengthSec)
    {
        juce::SpinLock::ScopedLockType lock (algorithmLock);
        if (auto* ir = dynamic_cast<IRAlgorithm*> (algorithm.get()))
            ir->setIRParameters (trimMs, lengthSec);
    }

    //==========================================================================
    // State Queries
    //==========================================================================

    /** Check if the engine is actively processing. */
    bool isActive() const { return numReverbNodes > 0 && isThreadRunning(); }

    /** Get the current number of reverb nodes. */
    int getNumNodes() const { return numReverbNodes; }

    /** Update for changed node count. Stops/restarts the thread. */
    void setNumNodes (int numNodes)
    {
        if (numNodes != numReverbNodes && sampleRate > 0)
        {
            bool wasRunning = isThreadRunning();
            if (wasRunning)
                stopThread (1000);
            prepareToPlay (sampleRate, currentBlockSize, numNodes);
            if (wasRunning)
                startThread (juce::Thread::Priority::high);
        }
    }

private:
    //==========================================================================
    // Thread Main Loop
    //==========================================================================

    void run() override
    {
        while (! threadShouldExit())
        {
            if (numReverbNodes > 0)
            {
                // Check if we have enough input data to process an internal block
                bool hasData = true;
                for (int i = 0; i < numReverbNodes && hasData; ++i)
                {
                    if (nodeInputBuffers[i]->getAvailableData() < internalBlockSize)
                        hasData = false;
                }

                if (hasData)
                {
                    processBlock();
                }
                else
                {
                    wait (1);
                }
            }
            else
            {
                wait (10);
            }
        }
    }

    //==========================================================================
    // Internal Processing
    //==========================================================================

    void processBlock()
    {
        int numSamples = internalBlockSize;

        // Read input from ring buffers into working buffer
        for (int n = 0; n < numReverbNodes; ++n)
        {
            nodeInputBuffers[n]->read (nodeInputBlock.getWritePointer (n), numSamples);
        }

        // Clear output buffer
        nodeOutputBlock.clear();

        // Apply pending parameter changes
        if (paramsChanged.load (std::memory_order_acquire))
        {
            currentParams = pendingParams.load();
            paramsChanged.store (false, std::memory_order_release);

            juce::SpinLock::ScopedLockType lock (algorithmLock);
            if (algorithm)
                algorithm->setParameters (currentParams);
        }

        // Apply pending geometry changes
        if (geometryChanged.load (std::memory_order_acquire))
        {
            std::vector<NodePosition> geo;
            {
                juce::SpinLock::ScopedLockType lock (geometryLock);
                geo = pendingGeometry;
            }
            geometryChanged.store (false, std::memory_order_release);

            juce::SpinLock::ScopedLockType lock (algorithmLock);
            if (algorithm)
                algorithm->updateGeometry (geo);
        }

        // --- Pre-processing would go here (Phase 2) ---

        // --- Algorithm processing ---
        {
            juce::SpinLock::ScopedLockType lock (algorithmLock);
            if (algorithm)
            {
                algorithm->processBlock (nodeInputBlock, nodeOutputBlock, numSamples);
            }
            // If no algorithm, output stays silent (pass-through silence)
        }

        // --- Post-processing would go here (Phase 3) ---

        // Apply wet level
        float wetLevel = currentParams.wetLevel;
        if (wetLevel != 1.0f)
        {
            for (int n = 0; n < numReverbNodes; ++n)
                juce::FloatVectorOperations::multiply (nodeOutputBlock.getWritePointer (n),
                                                       wetLevel, numSamples);
        }

        // Write output to ring buffers
        for (int n = 0; n < numReverbNodes; ++n)
        {
            nodeOutputBuffers[n]->write (nodeOutputBlock.getReadPointer (n), numSamples);
        }
    }

    //==========================================================================
    // State
    //==========================================================================

    double sampleRate = 0.0;
    int currentBlockSize = 512;
    int internalBlockSize = 256;
    int numReverbNodes = 0;

    // Per-node ring buffers for audio thread <-> engine thread
    std::vector<std::unique_ptr<LockFreeRingBuffer>> nodeInputBuffers;
    std::vector<std::unique_ptr<LockFreeRingBuffer>> nodeOutputBuffers;

    // Working buffers (numNodes channels x internalBlockSize samples)
    juce::AudioBuffer<float> nodeInputBlock;
    juce::AudioBuffer<float> nodeOutputBlock;

    // Active algorithm (nullptr = silence pass-through)
    std::unique_ptr<ReverbAlgorithm> algorithm;
    juce::SpinLock algorithmLock;
    int currentAlgorithmType = -1;  // -1 = none set yet

    // Thread-safe parameter passing
    struct AtomicParams
    {
        AlgorithmParameters params;
        void store (const AlgorithmParameters& p) { params = p; }
        AlgorithmParameters load() const { return params; }
    };

    AtomicParams pendingParams;
    std::atomic<bool> paramsChanged { false };
    AlgorithmParameters currentParams;

    // Thread-safe geometry passing
    std::vector<NodePosition> pendingGeometry;
    juce::SpinLock geometryLock;
    std::atomic<bool> geometryChanged { false };
};
