# WFS-DIY Phase 1 Implementation

## Overview

This is Phase 1 of the WFS-DIY project, implementing a basic Wave Field Synthesis audio processor using JUCE. The implementation provides:

- **Core WFS Processing**: Psychoacoustic triangle-based calculations
- **Real-time Audio Processing**: 4 inputs to 8 outputs configuration
- **OSC Control**: Real-time parameter updates via OSC messages
- **Basic UI**: Speaker/source positioning controls and mute buttons
- **Parameter Persistence**: Save/load configurations
- **Performance Monitoring**: CPU usage and latency display
- **Error Handling**: Robust device management and parameter validation

## Architecture

### Core Components

1. **WFSProcessor** (`Source/DSP/WFSProcessor.h/cpp`)
   - Main audio processing engine
   - Implements psychoacoustic triangle calculations
   - Handles delay lines and gain calculations
   - Manages source and speaker positioning

2. **OSCController** (`Source/DSP/OSCController.h/cpp`)
   - Handles OSC message parsing and sending
   - Provides real-time parameter updates
   - Supports source/speaker positioning and control

3. **WFS_DIY** (`Source/WFS-DIY.h`)
   - Main application component
   - Integrates audio processing, OSC control, and UI
   - Manages audio device configuration

## Features Implemented

### ✅ Audio Processing
- 4 input channels to 8 output channels
- Real-time WFS processing with sub-3ms latency target
- Psychoacoustic triangle calculations
- Distance-based delay and gain calculations
- Smooth parameter interpolation

### ✅ OSC Control
- Port 8000 (configurable)
- Message format: `/wfs/source/{index}/{parameter}`
- Supported parameters: position, mute, gain
- Real-time parameter updates
- Performance metrics feedback

### ✅ User Interface
- Performance monitoring (CPU usage, latency)
- Global gain control
- Source positioning controls (X, Y, Z)
- Speaker positioning controls (X, Y, Z)
- Individual mute controls for sources and speakers
- Save/Load configuration buttons

### ✅ Parameter Management
- Save configurations to `.wfs` files
- Load configurations from files
- Automatic UI synchronization
- State persistence

### ✅ Error Handling
- Audio device initialization error handling
- OSC connection error handling
- Parameter validation
- Graceful degradation

## OSC Message Format

### Source Control
```
/wfs/source/{index}/position x y z
/wfs/source/{index}/mute {0|1}
/wfs/source/{index}/gain {0.0-2.0}
```

### Speaker Control
```
/wfs/speaker/{index}/position x y z
/wfs/speaker/{index}/mute {0|1}
/wfs/speaker/{index}/gain {0.0-2.0}
```

### Global Control
```
/wfs/global/gain {0.0-2.0}
/wfs/listener/position x y z
```

### Performance Feedback
```
/wfs/performance/metrics cpuUsage latencyMs
```

## Testing OSC Communication

### Built-in Testing
- **OSC Test Tool Button**: Click "OSC Test Tool" in the main application for OSC message examples
- **UI Controls**: All sliders and buttons send OSC messages in real-time
- **Status Display**: Monitor OSC connection status and message sending

### External Testing Tools
- **Python Script**: `Scripts/test_osc.py` - Cross-platform automated testing
- **OSC Clients**: Use any OSC client (TouchOSC, OSCulator, etc.) to send messages
- **Custom Tools**: Build your own OSC client using JUCE's OSCSender class

### Why Python for Testing?
While the main application is C++/JUCE, Python is used for testing because:
- **Rapid Prototyping**: Quick to modify test patterns without recompiling
- **Cross-platform**: Works on Windows, macOS, Linux without platform builds
- **Development Tool**: Not part of production - just a testing utility
- **Simple Scripting**: Easy to add new test cases and automation

For production C++ testing, use the built-in UI controls or create custom JUCE-based test tools.

## Default Configuration

### Sources (4)
- Source 0: Center stage (0, 0, 0)
- Source 1: Left stage (-2, 0, 0)
- Source 2: Right stage (2, 0, 0)
- Source 3: Back stage (0, 0, -2)

### Speakers (8)
- Speaker 0: Left front (-4, 0, 8)
- Speaker 1: Left center (-2, 0, 8)
- Speaker 2: Center (0, 0, 8)
- Speaker 3: Right center (2, 0, 8)
- Speaker 4: Right front (4, 0, 8)
- Speaker 5: Left suspended (-3, 2, 8)
- Speaker 6: Center suspended (0, 2, 8)
- Speaker 7: Right suspended (3, 2, 8)

### Listener Position
- Default: (0, 0, 10) - Center of audience area

## Building and Running

1. **Prerequisites**
   - JUCE 7.0+
   - Visual Studio 2022+ (Windows)
   - Xcode 12+ (macOS)
   - GCC 9+ (Linux)

2. **Build**
   ```bash
   # Open WFS-DIY.jucer in Projucer
   # Generate project files for your platform
   # Build the project
   ```

3. **Run**
   - Configure audio device for 4 inputs, 8 outputs
   - OSC will automatically connect on port 8000
   - Use UI controls or send OSC messages for real-time control

## Performance Targets

- **Latency**: < 3ms processing time
- **Sample Rate**: 96kHz (adjustable)
- **CPU Usage**: Optimized for real-time processing
- **Memory**: Efficient delay line management

## Next Steps (Phase 2)

- Dead angle filtering system
- Level attenuation models (linear + inverse square)
- High frequency damping
- Speaker array management
- Advanced parameter controls

## Troubleshooting

### Audio Issues
- Check audio device configuration
- Verify input/output channel mapping
- Monitor CPU usage and latency

### OSC Issues
- Check port 8000 availability
- Verify message format
- Enable OSC logging for debugging

### Performance Issues
- Reduce number of active sources/speakers
- Lower sample rate if needed
- Check system resources

## License

GPL v3.0 - See LICENSE file for details.
