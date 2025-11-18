# WFS DIY - Development Notes for Claude

## Project Overview
Wave Field Synthesis (WFS) audio application built with JUCE framework for real-time multi-channel audio processing.

## Current Implementation Status (As of 2025-01-18)

### Architecture
- **Multithreaded N-to-M audio engine**: One processing thread per input channel (scalable to 64+ channels)
- **Lock-free design**: Lock-free ring buffers for communication between audio thread and processing threads
- **Real-time safe**: No allocations or locks in audio callback

### Key Components

#### 1. MainComponent ([MainComponent.h](../Source/MainComponent.h), [MainComponent.cpp](../Source/MainComponent.cpp))
- Main UI component with AudioAppComponent and Timer
- **Routing matrices** (centralized):
  - `delayTimesMs`: Delay times in milliseconds for each input→output routing
  - `levels`: Gain levels (0.0-1.0) for each input→output routing
  - Indexed as: `[inputChannel * numOutputChannels + outputChannel]`
- **Exponential smoothing**: 5ms timer applies smoothing to prevent clicks (smoothingFactor = 0.22f ≈ 20ms settling)
- **Random generator** (temporary): Generates random delays (0-1000ms) and levels (0-1) every 1 second for testing
- **Audio device persistence**: Saves/restores ASIO device type and name between sessions
- **Graceful ASIO fallback**: Falls back to Windows Audio if ASIO device is locked or unavailable
- **CPU monitoring display**: Shows per-thread performance metrics in bottom-left corner

#### 2. InputProcessor ([InputProcessor.h](../Source/InputProcessor.h))
- Each instance processes ONE input channel on its own thread
- Outputs to ALL output channels with individual delays and levels
- **Delay implementation**:
  - 1-second circular delay buffer per input
  - Linear interpolation for fractional sample delays
  - Reads delay times from shared `delayTimesMs` array
- **Level implementation**:
  - Reads levels from shared `levels` array
  - Optimization: skips processing when level = 0.0
- **CPU monitoring**:
  - Wall-clock CPU usage percentage (matches Task Manager)
  - Average processing time per block in microseconds (for algorithm comparison)
  - Measures only `processBlock()` time vs total wall-clock time

#### 3. LockFreeRingBuffer ([LockFreeRingBuffer.h](../Source/LockFreeRingBuffer.h))
- Single producer/single consumer lock-free ring buffer
- Uses `std::atomic` for thread-safe communication
- Used for: input buffering and per-output buffering

### Current Processing Flow
1. **Audio callback** (`getNextAudioBlock`):
   - Distributes input samples to each InputProcessor via `pushInput()`
   - Clears output buffer
   - Pulls processed output from each InputProcessor via `pullOutput()`
   - Sums outputs (each input contributes to all outputs)

2. **InputProcessor threads**:
   - Wait for input data
   - Write input to circular delay buffer
   - For each output channel:
     - Read delay time and level from shared arrays
     - Calculate fractional delay position
     - Linear interpolation between adjacent samples
     - Apply level and write to output ring buffer

3. **Parameter updates** (5ms timer):
   - Apply exponential smoothing to delay and level matrices
   - Generate new random targets every 1 second (temporary)
   - Update CPU usage display every 50ms

### Settings Persistence
- **Saved settings** (PropertiesFile in `%APPDATA%/WFS-DIY/WFS-DIY.settings`):
  - Number of input channels (2-64)
  - Number of output channels (2-64)
  - Audio device type (e.g., "ASIO")
  - Audio device name (e.g., "ASIO Fireface USB")

### Known Issues & Solutions
- **ASIO device locking**: If app crashes, ASIO device may remain locked. Solution: Unplug/replug USB device, or app will fall back to Windows Audio on next start
- **UTF-8 encoding**: Avoid non-ASCII characters in debug strings (caused assertion with μ symbol)

## Building & Running

### Requirements
- JUCE 8.0.10
- Visual Studio 2022
- ASIO-capable audio interface (optional, falls back to Windows Audio)

### Build Configuration
- JUCE_JACK disabled (set to 0 in WFS-DIY.jucer)
- Debug configuration includes CPU monitoring

### Testing Current Implementation
1. Set desired input/output channel counts in UI (2-64)
2. Select audio device (ASIO recommended for low latency)
3. Enable "Processing ON/OFF" toggle
4. Observe CPU metrics in bottom-left:
   - Format: `Input N: X.X% | XX.X us/block`
   - First number: wall-clock CPU usage
   - Second number: processing time per block (for algorithm comparison)
5. Audio will have random delays (0-1000ms) and levels (0-1) changing every second

## Next Steps / TODO

### Immediate (Ready to Implement)
1. **Remove random generator**: Keep exponential smoothing infrastructure, remove random target generation
2. **Implement WFS geometry**:
   - Define speaker array positions (x, y coordinates)
   - Define virtual source position(s)
   - Calculate delays based on geometry
   - Calculate levels based on amplitude panning

### Future Enhancements
1. **Delay algorithm optimization**: Compare current linear interpolation with alternative approaches
2. **UI improvements**:
   - Geometry visualization
   - Real-time delay/level matrix display
   - Virtual source position control
3. **Save/load presets**: Store complete routing matrices
4. **Advanced WFS**:
   - Multiple simultaneous sources
   - Moving sources with smooth transitions
   - Frequency-dependent processing

## Code Organization Notes

### Adding New Routing Parameters
To add new per-routing parameters (like in/out delay times):
1. Add new `std::vector<float>` in MainComponent.h (similar to `delayTimesMs`)
2. Add target vector for smoothing (if needed)
3. Resize in constructor alongside existing matrices
4. Pass pointer to InputProcessors in `startAudioEngine()`
5. Add corresponding pointer member in InputProcessor
6. Read in `processBlock()` using `routingIndex = inputChannelIndex * numOutputChannels + outChannel`

### Performance Optimization Tips
- The `us/block` metric is key for algorithm comparison
- Current implementation (8 inputs × 8 outputs with linear interpolation): ~40-50 us/block per thread
- Watch for: modulo operations, floating-point conversions, unnecessary memory access

### Thread Safety Notes
- **Lock-free reads**: InputProcessors read from `delayTimesMs` and `levels` arrays without locks
- **Safe because**: Float reads/writes are atomic on x64, and smoothing happens on message thread (separate from audio/processing threads)
- **If adding complex structures**: Consider using `std::atomic` or message passing

## Git Workflow
- Main branch: `master`
- Commits should be atomic and well-described
- Documentation updates committed separately from code changes

## Contact / Collaboration
This project is being developed incrementally with human guidance. Major architectural decisions should be discussed before implementation.

---
*Last updated: 2025-01-18*
*JUCE Version: 8.0.10*
*Build: Visual Studio 2022, x64 Debug*
