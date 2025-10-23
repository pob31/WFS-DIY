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
