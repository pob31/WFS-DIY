# WFS DIY - Development Notes for Claude

## Project Overview
Wave Field Synthesis (WFS) audio application built with JUCE framework for real-time multi-channel audio processing.

## Current Implementation Status (As of 2025-01-19)

### Architecture
- **Dual-algorithm multithreaded N-to-M audio engine**: Two different processing approaches available for comparison
  - **InputBufferProcessor**: One thread per input channel (read-time delays)
  - **OutputBufferProcessor**: One thread per output channel (write-time delays)
- **Lock-free design**: Lock-free ring buffers for communication between audio thread and processing threads
- **Real-time safe**: No allocations or locks in audio callback

### Key Components

#### 1. MainComponent ([MainComponent.h](../Source/MainComponent.h), [MainComponent.cpp](../Source/MainComponent.cpp))
- Main UI component with AudioAppComponent and Timer
- **Algorithm selection**: Dropdown to switch between InputBuffer and OutputBuffer algorithms
- **Routing matrices** (centralized, shared by both algorithms):
  - `delayTimesMs`: Delay times in milliseconds for each input→output routing
  - `levels`: Gain levels (0.0-1.0) for each input→output routing
  - Indexed as: `[inputChannel * numOutputChannels + outputChannel]`
- **Continuous ramping + exponential smoothing**:
  - 5ms timer with linear ramp over 1 second between random values
  - 20ms exponential smoothing applied on top (smoothingFactor = 0.22f)
  - Simulates moving WFS sources with smooth, click-free transitions
- **Audio device persistence**: Saves/restores ASIO device type and name between sessions
- **Graceful ASIO fallback**: Falls back to Windows Audio if ASIO device is locked or unavailable
- **CPU monitoring display**: Shows per-thread performance metrics in bottom-left corner

#### 2. InputBufferProcessor ([InputBufferProcessor.h](../Source/InputBufferProcessor.h))
- **Read-time delay algorithm** (original approach)
- Each instance processes ONE input channel on its own thread
- Outputs to ALL output channels with individual delays and levels
- **Processing flow**:
  1. Input arrives → write to circular delay buffer
  2. For each output channel:
     - Read delay time and level from shared arrays
     - Calculate read position: `currentPos - delay`
     - Linear interpolation for fractional sample delays
     - Apply level and output
- **Advantages**: Straightforward, one delay buffer per input
- **Performance**: Slightly more CPU spikes observed

#### 3. OutputBufferProcessor ([OutputBufferProcessor.h](../Source/OutputBufferProcessor.h))
- **Write-time delay algorithm** (alternative approach)
- Each instance processes ONE output channel on its own thread
- Receives from ALL input channels
- **Processing flow**:
  1. Input arrives → for each contributing input:
     - Read delay time and level from shared arrays
     - Calculate write position: `currentPos + delay`
     - Linear interpolation distributes contribution across two adjacent samples
     - **Read-modify-write**: Read current value, add contribution, write sum
  2. At output time → simply read from current position (no delay calculation)
- **Advantages**: Better cache locality, delays calculated once per input arrival
- **Performance**: Lower and steadier CPU usage observed (surprising result!)

#### 4. LockFreeRingBuffer ([LockFreeRingBuffer.h](../Source/LockFreeRingBuffer.h))
- Single producer/single consumer lock-free ring buffer
- Uses `std::atomic` for thread-safe communication
- Used for: input buffering and output buffering in both algorithms

### Current Processing Flow

**InputBuffer Algorithm:**
1. Audio callback distributes input samples to each InputBufferProcessor
2. Each processor writes to its delay buffer
3. Each processor generates outputs with delays calculated at read time
4. Audio callback sums outputs from all processors

**OutputBuffer Algorithm:**
1. Audio callback distributes each input to ALL OutputBufferProcessors
2. Each processor accumulates contributions from all inputs with delays calculated at write time
3. Audio callback pulls processed output from each processor (simple read, no delay calculation)

### Parameter Updates (5ms timer)
1. Calculate linear ramp progress (0→100% over 200 ticks = 1 second)
2. Update ramping targets by interpolating: `start + (final - start) * progress`
3. Apply exponential smoothing: `current += (target - current) * 0.22`
4. Every 1 second: generate new random final targets and restart ramp

### CPU Monitoring
Both algorithms include identical monitoring:
- **Wall-clock CPU usage percentage**: Matches Task Manager (processing time / elapsed time)
- **Processing time per block**: Average microseconds per 64-sample block (for algorithm comparison)
- Display shows appropriate label: "Thread Performance (InputBuffer)" or "Thread Performance (OutputBuffer)"

### Settings Persistence
- **Saved settings** (PropertiesFile in `%APPDATA%/WFS-DIY/WFS-DIY.settings`):
  - Number of input channels (2-64)
  - Number of output channels (2-64)
  - Audio device type (e.g., "ASIO")
  - Audio device name (e.g., "ASIO Fireface USB")

### Known Issues & Solutions
- **ASIO device locking**: If app crashes, ASIO device may remain locked. Solution: Unplug/replug USB device, or app will fall back to Windows Audio on next start
- **UTF-8 encoding**: Avoid non-ASCII characters in debug strings (caused assertion with μ symbol)
- **Algorithm switching**: Must disable processing before switching (enforced by UI)

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
3. Select algorithm from dropdown:
   - "InputBuffer (read-time delays)" - original algorithm
   - "OutputBuffer (write-time delays)" - alternative algorithm
4. Enable "Processing ON/OFF" toggle
5. Observe CPU metrics in bottom-left:
   - Format: `Input N: X.X% | XX.X us/block` or `Output N: X.X% | XX.X us/block`
   - First number: wall-clock CPU usage
   - Second number: processing time per block (for algorithm comparison)
6. Audio will have continuously ramping random delays (0-1000ms) and levels (0-1)

### Performance Observations (Preliminary)
- **Small configurations (4×4)**: Both algorithms show comparable performance
- **OutputBuffer**: Lower and steadier CPU usage, fewer spikes
- **InputBuffer**: Slightly more CPU spikes
- Further testing needed with larger asymmetric configurations (e.g., 8×64 for realistic WFS)

## Next Steps / TODO

### Immediate (Ready to Implement)
1. **Test with larger configurations**: 16×16, 32×32, 8×64 (realistic WFS scenarios)
2. **Remove random generator**: Keep exponential smoothing infrastructure, remove random ramping
3. **Implement WFS geometry**:
   - Define speaker array positions (x, y coordinates)
   - Define virtual source position(s)
   - Calculate delays based on geometry (speed of sound)
   - Calculate levels based on amplitude panning

### Algorithm Selection Strategy
- Keep both algorithms available for now
- Performance winner may depend on:
  - Input/output ratio (symmetric vs asymmetric)
  - Number of moving sources
  - Update frequency of delay changes
  - Cache effects at scale

### Future Enhancements
1. **UI improvements**:
   - Geometry visualization
   - Real-time delay/level matrix display
   - Virtual source position control
2. **Save/load presets**: Store complete routing matrices
3. **Advanced WFS**:
   - Multiple simultaneous sources
   - Moving sources with smooth transitions
   - Frequency-dependent processing

## Code Organization Notes

### Adding New Routing Parameters
To add new per-routing parameters:
1. Add new `std::vector<float>` in MainComponent.h (similar to `delayTimesMs`)
2. Add target/ramp vectors if continuous ramping needed
3. Resize in constructor alongside existing matrices
4. Pass pointer to both InputBufferProcessors and OutputBufferProcessors in `startAudioEngine()`
5. Add corresponding pointer members in both processor classes
6. Read in `processBlock()` using `routingIndex = inputChannelIndex * numOutputChannels + outChannel`

### Performance Optimization Tips
- The `us/block` metric is key for algorithm comparison
- Current implementation: ~40-50 us/block per thread (8×8 config)
- OutputBuffer showing lower CPU suggests better cache behavior
- Watch for: modulo operations, floating-point conversions, unnecessary memory access

### Thread Safety Notes
- **Lock-free reads**: Processors read from `delayTimesMs` and `levels` arrays without locks
- **Safe because**: Float reads/writes are atomic on x64, and ramping/smoothing happens on message thread (separate from audio/processing threads)
- **If adding complex structures**: Consider using `std::atomic` or message passing

### Algorithm Switching
- Disabled while processing is active (enforced by UI)
- Cleanly stops old processors and creates new ones
- Preserves processing state if it was enabled

## Git Workflow
- Main branch: `master`
- Commits should be atomic and well-described
- Documentation updates should reflect significant changes

## Contact / Collaboration
This project is being developed incrementally with human guidance. Major architectural decisions should be discussed before implementation.

---
*Last updated: 2025-01-19*
*JUCE Version: 8.0.10*
*Build: Visual Studio 2022, x64 Debug*
