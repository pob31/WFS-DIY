# External Libraries and Resources

This document contains references to external libraries, protocols, and resources that are relevant to the WFS-DIY project.

## Positioning and Tracking Protocols

### PosiStageNet (PSN)
- **Repository**: [VYV/PSN-CPP](https://github.com/VYV/PSN-CPP)
- **Description**: An open protocol for on-stage, live 3D position data
- **License**: MIT
- **Use Case**: Real-time 3D positioning data for performers and objects on stage
- **Implementation**: C++ header-only library
- **Notes**: 
  - Cross-platform (Windows, Linux, macOS)
  - Used by VYV's Photon Media Server and MA Lighting's GrandMA2
  - Supports lighting, sound panning, and automation applications

### RTTrP (Real-Time Tracking Protocol)
- **Repository**: [RTTrP/RTTrP-v2.4.2.0](https://github.com/RTTrP/RTTrP-v2.4.2.0/tree/master/C++)
- **Description**: Real-time tracking protocol for motion capture and positioning systems
- **License**: MIT License
- **Use Case**: Alternative positioning protocol for motion capture systems
- **Implementation**: C++ implementation available
- **Notes**: 
  - Version 2.4.2.0 available
  - Alternative to PosiStageNet for different tracking systems
  - MIT license allows free integration into GPL v3 projects

## WFS Algorithm Reference

### Psychoacoustic Triangle Method
- **Description**: Practical WFS implementation based on human hearing perception
- **Approach**: Triangle calculations between sound object, speaker, and targeted listener
- **Benefits**: 
  - More efficient than theoretical wave field synthesis
  - Optimized for human psychoacoustics
  - Handles comb filtering naturally
  - Practical for live performance
- **Implementation**: 
  - Fixed parts: Speaker position + Targeted listener position
  - Dynamic part: Sound source movement
  - Updates triggered by source movement or position changes

### Level and Frequency Management
- **Components**: Delay matrix + Level attenuation + High frequency roll-off
- **Distance Attenuation Models**:
  - **Linear Model**: Adjustable 0 to -6dB/m attenuation
  - **Inverse Square Law**: Physically accurate distance-based attenuation
- **Attenuation Control Parameters**:
  - **Output Percentage**: Per-array or per-speaker attenuation percentage
  - **Subbass Protection**: Reduce distance effect for low-frequency speakers
  - **Closest Speaker Ratio**: Compensate for minimum distance attenuation
- **Use Cases**:
  - **Linear Model**: Predictable, adjustable level changes
  - **Inverse Square**: Physically accurate but may create hotspots
  - **Subbass Arrays**: Maintain level when sources move away
  - **Dynamic Range**: Preserve headroom and dynamics
- **Control Features**:
  - **Source Mutes**: Individual source muting
  - **Speaker Mutes**: Individual speaker muting
  - **Array Macros**: Bulk muting/unmuting for entire arrays
- **Frequency Response**: High frequency roll-off compensation

### High Frequency Damping System
- **Purpose**: Compensate for high frequency loss over distance and source directivity
- **Two Main Components**:
  - **Distance-based HF damping**: High shelf above 1kHz with smooth slope (0.2)
  - **Source directivity modeling**: Off-axis frequency response simulation
- **Distance-based Damping**:
  - **Output-based parameter**: Set per speaker, not per input
  - **Rationale**: Delay line speakers need different HF compensation than stage speakers
  - **Typical Settings**:
    - **Stage edge speakers**: ~-0.3dB/m
    - **Flown array**: ~-0.15dB/m  
    - **Delay line speakers**: None (0dB/m)
- **Source Directivity Modeling**:
  - **Concept**: Simulate off-axis frequency response of sources
  - **Implementation**: Adjustable angle with HF shelving "behind" source
  - **3D Orientation**: Fully orientable in 3D space
  - **Use Cases**:
    - **Special effects**: Dull direct sound + bright pseudoacoustic reverb
    - **Creative sound design**: Source pointed at reverb points upstage
    - **Realistic modeling**: Mimic voice/instrument directivity (use sparingly)
- **Benefits**:
  - **Realistic distance modeling**: Compensate for HF loss over distance
  - **Creative possibilities**: Special effects and sound design
  - **Flexible control**: Per-speaker and per-source adjustments
- **Considerations**:
  - **Intelligibility**: Excessive HF damping can reduce speech clarity
  - **Balance**: Use judiciously to maintain natural sound character

### Motion Control and Doppler Management
- **Purpose**: Control source movement speed and manage Doppler effects
- **Speed Limitations**:
  - **Maximum speed**: Prevents delay changes exceeding one sample duration
  - **Realistic speed**: Reflects actual movement capabilities (e.g., actor walking)
  - **Adjustable parameters**: Customizable speed limits per source
- **Doppler Effect Management**:
  - **Problem**: Fast movement creates strong Doppler effects
  - **Solution**: Speed limiting prevents abrupt Doppler changes
  - **Smooth transitions**: Gradual onset/end of movement
- **Relative Delay Processing**:
  - **Concept**: Apply only relative delay changes, not absolute delays
  - **Benefits**:
    - **Reduced Doppler effect**: Especially for virtual sources (soundtracks)
    - **Acoustic displacement**: Can overtake acoustic sound with low latency
    - **Position manipulation**: Move perceived source position
- **Creative Applications**:
  - **Screen voice placement**: Voice appears to come from projection screen
  - **Acoustic displacement**: Perceived position different from actual position
  - **Virtual source control**: Manage Doppler effects on soundtracks
- **Technical Implementation**:
  - **Speed ramping**: Smooth acceleration/deceleration
  - **Delay interpolation**: Gradual delay changes
  - **Relative processing**: Differential delay application
- **Use Cases**:
  - **Live actors**: Realistic movement speed limits
  - **Virtual sources**: Reduced Doppler effects
  - **Creative positioning**: Acoustic displacement effects
  - **Screen integration**: Voice-source separation
- **Fast Movement Handling**:
  - **Problem**: Sources moving faster than speed limit cause artifacts
  - **Solution**: Fade-out/jump/fade-in sequence
  - **Implementation**:
    - **Fade-out**: Gradual volume reduction over few samples
    - **Jump**: Instant position change
    - **Fade-in**: Gradual volume restoration over few samples
  - **Result**: Controlled discontinuity instead of audio artifacts
  - **Operator Control**: Sound designer handles discontinuity in cues
  - **Common Use**: Moving input source before triggering new soundtrack

### Live Source Damping System
- **Purpose**: Prevent sound reinforcement conflicts with loud acoustic sources
- **Problem**: Loud live sources (opera singers, drums, horns, guitar amps) don't need reinforcement from nearby speakers
- **Solution**: Radius-based attenuation around live sources
- **Attenuation Curve Types**:
  - **V-shaped**: Sharp attenuation at source, gradual recovery
  - **U-shaped**: Deep attenuation zone around source
  - **Custom curves**: Adjustable cross-section profiles
- **Damping Components**:
  - **Fixed Attenuation**: Constant level reduction at source position
  - **Peak Compression**: Dynamic response to peak signal levels
  - **Slow Integration**: Gradual response to sustained levels
- **Benefits**:
  - **Prevents feedback**: Reduces reinforcement near loud sources
  - **Maintains clarity**: Avoids double-reinforcement conflicts
  - **Preserves dynamics**: Smart compression based on source level
  - **Natural sound**: Acoustic source takes precedence when loud enough
- **Use Cases**:
  - **Opera singers**: Prevent nearby speaker interference
  - **Drum kits**: Avoid reinforcement conflicts
  - **Horn sections**: Maintain acoustic character
  - **Guitar amplifiers**: Preserve amp tone

### Dead Angle Filtering
- **Purpose**: Prevent speakers from reproducing sounds that are physically behind them
- **Definition**: Angular zone where speakers should not relay sound sources
- **Benefits**:
  - Eliminates impossible delays
  - Reduces computational load
  - Maintains psychoacoustic accuracy
  - Preserves precedence effect
- **Implementation**:
  - **On/Off angles**: Define active and dead zones
  - **Speaker orientation**: Adjustable axis direction
  - **Transition zones**: Smooth fade between active and dead zones
  - **No dead angle option**: For subbass speakers (fewer speakers, omnidirectional)
- **Adjustable Parameters**:
  - On angle (start of active zone)
  - Off angle (end of active zone)
  - Speaker axis orientation
  - Transition curve (smooth cutoff)
- **Special Cases**:
  - **Subbass speakers**: Often configured with no dead angle
  - **Moving sources**: Smooth transitions prevent audio artifacts
- **Method**: Calculate angular position of source relative to speaker axis
- **Result**: Only process "active" speakers that can contribute meaningfully

### Speaker Array Configurations
- **Primary Setup**: Frontal array (stage floor + suspended)
- **Secondary**: Surround speakers (when possible)
- **Target**: Most audience members hear 3+ speakers minimum
- **Coordinate Systems**: Cartesian, cylindrical, spherical
- **Array Management**: 
  - Speaker grouping with shared parameters
  - Macro controls for muting/activation
  - Helper tools for line/curve placement
  - Even spacing within arrays

### Latency Compensation System
- **Global Delay**: Console + WFS processor latency
- **Input-Specific**: Microphone processing time
- **Output-Specific**: Amplifier processing time
- **Haas Effect**: Additional delay for psychoacoustic enhancement
- **Calibration Method**: Click track → microphone → time difference measurement
- **Fine-Tuning**: Ear-based adjustment when theory doesn't match reality

### System Specifications
- **Maximum Inputs**: 64 channels (adjustable based on resources)
- **Maximum Outputs**: 128 channels (adjustable based on resources)
- **Reverb Channels**: Up to 16 channels for pseudoacoustics (adjustable)
- **Effects Channels**: Up to 16 channels for sound design (adjustable)
- **Latency**: Adjustable (target <3ms for acoustic sound reinforcement)
- **Sample Rate**: Adjustable (44.1kHz, 48kHz, 96kHz, 192kHz)
- **Scalability**: All parameters adjustable based on setup and processing power

### Reflections Processing System
- **Purpose**: Enhance physicality and realism of sound sources
- **Benefits**:
  - Blurs actual speaker reflections that reveal speaker placement
  - Maintains correct cues for moving sources
  - Prevents "speaker localization" when sources stop moving
  - Adds natural acoustic character to reproduced sound
- **Method**: Mirror source calculations for reflecting surfaces
- **Implementation**:
  - Calculate mirror position of source relative to reflecting surface
  - Apply triangle calculations (mirror source → speaker → listener)
  - Add decorrelated jitter to reflection signals
  - Mix reflections with direct signal
- **Processing Latency Considerations**:
  - **Floor reflections**: Small delays for stage-edge speakers (CPU processing)
  - **Distant surfaces**: Larger delays allow more processing time (GPU processing)
  - **Intelligent routing**: Route reflections based on delay requirements
- **Surfaces** (expandable based on processing power):
  - **Phase 1**: Floor reflections
  - **Future**: Side walls, upstage wall, ceiling, back wall
- **Processing Requirements**: Additional computational load for each surface

### Resource Management
- **Dynamic Scaling**: Adjust I/O count based on available processing power
- **Effects Allocation**: Distribute reverb/effects channels based on requirements
- **Reflections Scaling**: Add/remove surfaces based on processing capacity
- **Latency vs Quality**: Balance processing latency with audio quality
- **Real-time Adjustment**: Change parameters during operation without interruption

## Audio Processing Frameworks

### GPU-Audio SDK
- **Repository**: [gpuaudio/gpuaudio-sdk](https://github.com/gpuaudio/gpuaudio-sdk)
- **Description**: Open-source framework for GPU-accelerated real-time audio processing
- **License**: MIT License
- **Use Case**: High-performance audio processing with GPU acceleration for computationally intensive algorithms
- **Key Features**:
  - GPU-accelerated audio processing pipeline
  - Real-time processing with low latency
  - Cross-platform support (Windows, macOS, Linux)
  - Processor API for custom audio processors
  - Engine API for processing graph management
  - Built-in processors: FIR, IIR, Gain, Neural Amp Modeler
- **Technical Details**:
  - CMake-based build system
  - Header-only client library
  - Support for multiple GPU vendors
  - Directed Acyclic Graph (DAG) processing architecture
  - Efficient memory management and buffer reuse
- **Integration Benefits for WFS**:
  - Parallel processing for spatial audio calculations
  - Reduced CPU load for complex WFS algorithms
  - Real-time performance optimization
  - Scalable processing for multiple audio sources

## Audio Standards and Protocols

### ADM-OSC (Audio Definition Model over Open Sound Control)
- **Specification**: [ADM-OSC Living Standard](https://immersive-audio-live.github.io/ADM-OSC/)
- **Description**: Industry standard for Object-Based Audio positioning data in live production
- **Version**: Living Standard, October 2025
- **Use Case**: Immersive audio positioning and object-based audio workflows
- **Key Features**:
  - Object messages (gain, position)
  - Environment messages
  - Listener messages
  - Bi-directional communication
- **Industry Support**: 
  - Major companies involved: d&b audiotechnik, L-Acoustics, Meyer Sound, Yamaha, BBC, Dolby, etc.
  - Used in live music and broadcast domains
- **Technical Details**:
  - Uses OSC (Open Sound Control) protocol
  - Supports both polar and cartesian coordinates
  - Lightweight and human-readable
  - Cross-platform implementation

## Integration Considerations

### For WFS-DIY Project
1. **GPU-Audio SDK**: Primary choice for high-performance audio processing
   - MIT license allows free integration into GPL v3 projects
   - GPU acceleration addresses performance concerns in PRD
   - CMake build system compatible with JUCE projects
   - Real-time processing capabilities match WFS requirements

2. **PosiStageNet**: Primary choice for stage positioning data
   - MIT license allows free integration into GPL v3 projects
   - Header-only implementation fits JUCE workflow
   - Proven in professional lighting and media server applications

3. **ADM-OSC**: For audio object positioning
   - Industry standard for immersive audio
   - OSC protocol integrates well with JUCE's networking capabilities
   - Supports both polar and cartesian coordinate systems
   - Open standard - no license restrictions

4. **RTTrP**: Alternative positioning protocol
   - MIT license allows free integration into GPL v3 projects
   - Consider as backup or for specific motion capture systems
   - May require additional integration work

## Implementation Notes

- All protocols support real-time data transmission
- Consider network bandwidth and latency requirements
- Coordinate system conversions may be needed between protocols
- JUCE's networking modules can handle OSC communication
- Header-only libraries (like PSN-CPP) integrate easily with JUCE projects
- GPU-Audio SDK requires GPU drivers and appropriate hardware
- Consider fallback to CPU processing for systems without GPU support
- GPU memory management is handled automatically by the SDK

## License Compatibility Summary

| Resource | License | GPL v3 Compatible | Status |
|----------|---------|-------------------|---------|
| **WFS-DIY Project** | GPL v3.0 | N/A | ✅ Active |
| **GPU-Audio SDK** | MIT | ✅ Yes | ✅ Safe to use |
| **PosiStageNet** | MIT | ✅ Yes | ✅ Safe to use |
| **ADM-OSC** | Open Standard | ✅ Yes | ✅ Safe to use |
| **RTTrP** | MIT | ✅ Yes | ✅ Safe to use |

### License Compatibility Details

**MIT License → GPL v3 Integration**: ✅ **PERFECTLY COMPATIBLE**
- MIT-licensed libraries can be freely incorporated into GPL v3 projects
- MIT doesn't "infect" the GPL v3 license - your project remains GPL v3
- Users can use MIT parts under MIT terms, GPL parts under GPL terms
- No legal conflicts or restrictions

**Key Differences**:
- **MIT**: Permissive license - "do what you want, just give credit"
- **GPL v3**: Copyleft license - "keep it free and open"
- **Integration**: MIT libraries integrate seamlessly into GPL v3 projects

### ✅ **No License Conflicts!**
Your project setup is **perfectly legal and compatible**:
- MIT libraries can be freely incorporated
- Your project maintains GPL v3 copyleft protection
- Users get appropriate rights under both licenses
- No legal issues or conflicts

---
*Last Updated: January 2025*
*Project: WFS-DIY*
