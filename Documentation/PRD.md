# Product Requirements Document (PRD)
## WFS-DIY: Wave Field Synthesis Implementation

### Document Information
- **Version**: 1.0
- **Date**: 2025-01-22
- **Author**: Pierre-Olivier Boulant
- **Project**: WFS-DIY
- **Based on**: Cycling74 Max8 Prototype (https://wfs-diy.net)

---

## 1. Executive Summary

### 1.1 Project Overview
The WFS-DIY project is a JUCE-based implementation of Wave Field Synthesis (WFS) technology, porting the functionality from the Cycling74 Max8 prototype to a standalone cross-platform application.

### 1.2 Objectives
- Create a real-time WFS audio processing application
- Provide intuitive user interface for WFS parameter control
- Support multiple audio input/output configurations
- Enable spatial audio rendering for various speaker array configurations

### 1.3 Success Criteria
- Real-time audio processing with low latency
- Accurate WFS algorithm implementation
- Cross-platform compatibility (Windows, macOS, Linux)
- User-friendly interface for audio professionals

---

## 2. Technical Requirements

### 2.1 Core Functionality
- **Real-time Audio Processing**: Low-latency audio I/O and processing
- **Psychoacoustic WFS Algorithm**: Triangle-based calculations optimized for human hearing
- **Speaker Array Management**: Support for various speaker configurations (frontal, suspended, surround)
- **Audio Source Positioning**: Virtual source placement and movement with dead angle filtering
- **Parameter Control**: Real-time adjustment of WFS parameters with visual on-site tuning
- **Latency Compensation**: Global and per-channel delay compensation for signal path
- **Array Management**: Speaker grouping, macro controls, and shared parameters
- **Scalable I/O**: Up to 64 inputs and 128 outputs (adjustable based on resources)
- **Effects Processing**: Up to 16 reverb channels and 16 effects channels (adjustable)
- **Reflections Processing**: Floor reflections with mirror source calculations (expandable to multiple surfaces)
- **Pseudoacoustic Enhancement**: Decorrelated jitter for natural sound character

### 2.2 Performance Requirements
- **Latency**: < 3ms processing time for acoustic sound reinforcement (adjustable)
- **Sample Rate**: Primary target 96kHz (adjustable: 44.1kHz, 48kHz, 96kHz, 192kHz)
- **CPU Usage**: Optimized for available computing power
- **Memory**: Efficient memory management for real-time processing
- **Real-time**: Must maintain consistent low latency for live applications
- **Scalability**: Adjustable based on setup and available processing power

### 2.3 Platform Requirements
- **Windows**: Windows 10/11, Visual Studio 2022+
- **macOS**: macOS 10.15+, Xcode 12+
- **Linux**: Ubuntu 20.04+, GCC 9+

---

## 3. User Interface Requirements

### 3.1 Main Interface Components
- **Speaker Array Visualization**: Visual representation of speaker layout
- **Source Positioning**: Interactive source placement and movement
- **Parameter Controls**: Real-time adjustment sliders and knobs
- **Audio Device Selection**: Input/output device configuration
- **Preset Management**: Save/load WFS configurations

### 3.2 User Experience Goals
- **Intuitive**: Easy to understand for audio professionals
- **Responsive**: Real-time feedback for all parameter changes
- **Professional**: Clean, modern interface suitable for studio use

---

## 4. Technical Architecture

### 4.1 Core Components
- **Audio Engine**: Real-time audio processing pipeline with multi-backend support
- **Psychoacoustic WFS Processor**: Triangle-based calculations with dead angle filtering
- **Speaker Array Manager**: Flexible speaker positioning and array management
- **Latency Compensation System**: Global and per-channel delay management
- **UI Controller**: User interface with visual on-site tuning capabilities
- **Audio Device Manager**: Audio I/O handling for multichannel setups
- **Configuration Manager**: Preset and settings management

### 4.2 Data Flow
1. Audio input → Audio Engine
2. Audio Engine → Psychoacoustic WFS Processor
3. Dead angle filtering → Active speaker selection
4. Triangle calculations → Delay compensation
5. WFS Processor → Speaker Array Output
6. UI Controls → Parameter Updates (visual on-site tuning)
7. Parameter Updates → WFS Processor
8. Array Management → Speaker grouping and macro controls

---

## 5. Implementation Phases

### Phase 1: Basic WFS Implementation
**Goal**: Establish core audio processing with basic distance calculations

**Core Features**:
- Basic JUCE application structure with audio device management
- Few inputs and outputs (4-8 channels to start)
- Basic distance calculations (source → speaker)
- CPU audio engine (scalar implementation)
- Basic OSC control for position updates
- Real-time parameter updates (smooth changes)

**Additional Features**:
- Parameter persistence (save/load configurations)
- Basic monitoring (CPU usage, latency, audio levels)
- Simple UI (speaker/source position editors, mute controls)
- Error handling (device management, parameter validation)

**Success Criteria**:
- Audio processing works without artifacts
- Real-time parameter updates are smooth
- OSC control responds correctly
- Performance meets sub-3ms target
- Configuration can be saved/loaded

### Phase 2: Enhanced WFS Features
- Dead angle filtering system
- Level attenuation models (linear + inverse square)
- High frequency damping
- Speaker array management
- Advanced parameter controls

### Phase 3: Psychoacoustic Enhancements
- Reflections processing (floor reflections)
- Live source damping system
- Motion control and Doppler management
- Advanced UI and visualization

### Phase 4: Multi-Backend Architecture
- CPU optimization (AVX2/AVX512 backends)
- GPU-Audio SDK integration
- Intelligent processing routing
- Performance optimization

### Phase 5: Advanced Features
- Effects channels with virtual positioning
- Pseudoacoustic reverb system
- Clustering inputs
- Plugin support and advanced integrations

---

## 6. Risk Assessment

### 6.1 Technical Risks
- **Ultra-low Latency**: Achieving <3ms processing at 96kHz is extremely challenging
- **Algorithm Complexity**: WFS calculations may be computationally intensive
- **Real-time Performance**: Maintaining consistent ultra-low latency for live applications
- **Hardware Dependency**: Performance heavily dependent on CPU/GPU capabilities
- **Cross-platform Compatibility**: Ensuring consistent behavior across platforms

### 6.2 Mitigation Strategies
- **Hybrid Processing Architecture**: CPU for ultra-low latency live processing, GPU for high-latency effects
- **Multiple Backend Architecture**: Implement CPU scalar, AVX2/AVX512, and GPU-Audio SDK backends
- **Intelligent Workload Distribution**: Route processing based on latency requirements
- **Runtime Backend Selection**: Allow automatic detection and manual selection of optimal backend
- **Performance Profiling**: Continuous monitoring of processing load and latency
- **Hardware Optimization**: Leverage available CPU/GPU capabilities for maximum performance
- **Fallback Mechanisms**: Ensure reliable operation even if optimized backends fail
- **Early Prototyping**: Test core algorithms with different backends before full implementation

---

## 7. Hybrid Processing Architecture Strategy

### 7.1 Multi-Backend Processing Approach

The WFS-DIY project will implement a **hybrid processing architecture** that intelligently routes audio processing based on latency requirements and computational complexity.

#### 7.1.1 CPU Processing Path (<3ms latency)
**Target**: Ultra-low latency processing for critical live audio
- **Live audio sources** with stage reference
- **Critical WFS calculations** for real-time positioning
- **Essential spatial processing** that performers depend on
- **Low-latency monitoring** and feedback
- **Real-time parameter adjustments**

**Backend Options**:
- CPU Scalar (fallback)
- CPU AVX2 (modern processors)
- CPU AVX512 (high-end processors)

#### 7.1.2 GPU Processing Path (higher latency acceptable)
**Target**: High-performance processing for non-critical audio
- **Convolution reverbs** and complex effects
- **Background audio tracks** without live reference
- **Non-critical spatial processing**
- **Heavy computational tasks** that can tolerate buffer delays
- **Complex audio analysis** and visualization

**Backend Options**:
- GPU-Audio SDK (primary)
- CPU fallback (if GPU unavailable)

### 7.2 Intelligent Routing Strategy

```
Audio Input → Latency Classification → Processing Router
├── Live Sources → CPU Backend (<3ms)
├── Background Audio → GPU Backend (higher latency OK)
└── Effects → GPU Backend (convolution, etc.)
```

#### 7.2.1 Routing Criteria
- **Latency Requirements**: Critical vs. non-critical processing
- **Computational Complexity**: Simple vs. complex algorithms
- **Real-time Dependencies**: Live performance vs. background processing
- **Resource Availability**: CPU/GPU load balancing

#### 7.2.2 Benefits
- **CPU Power Conservation**: Reserve CPU cycles for critical low-latency tasks
- **GPU Utilization**: Offload heavy processing to GPU when latency allows
- **Scalability**: Add more GPU-intensive effects without impacting live performance
- **Flexibility**: Route different audio sources through different processing paths
- **Performance Optimization**: Each processor does what it does best

### 7.3 Implementation Considerations

#### 7.3.1 Runtime Backend Selection
- **Automatic Detection**: System chooses optimal backend based on hardware capabilities
- **Manual Override**: User can force specific backend selection
- **Fallback Mechanisms**: Graceful degradation if preferred backend fails
- **Performance Monitoring**: Continuous assessment of backend performance

#### 7.3.2 Cross-Platform Compatibility
- **Hardware Detection**: Automatic detection of CPU/GPU capabilities
- **Backend Availability**: Dynamic assessment of available processing options
- **Consistent Behavior**: Uniform performance across different platforms

---

## 8. Future Enhancements

### 7.1 Potential Features
- **Multi-source Support**: Multiple simultaneous audio sources
- **Advanced Visualization**: 3D visualization of wave fields
- **Plugin Support**: VST/AU plugin versions
- **Network Audio**: Remote audio processing capabilities

---

## 8. References

- [Cycling74 Max8 WFS Prototype](https://wfs-diy.net)
- [JUCE Framework Documentation](https://docs.juce.com)
- [Wave Field Synthesis Theory](https://en.wikipedia.org/wiki/Wave_field_synthesis)
- [External Libraries Documentation](./References/External_Libraries.md)

---

## ⚠️ DEVELOPMENT STATUS

**IMPORTANT**: This PRD is currently in **DRAFT STATUS** and **NOT READY FOR DEVELOPMENT**.

### Current Status:
- Requirements are still being defined and refined
- Technical architecture decisions are pending
- Implementation approach needs to be determined
- External library integration strategy under review

### Before Starting Development:
1. ✅ Review and finalize all requirements
2. ✅ Define specific WFS algorithms to implement
3. ✅ Determine performance optimization strategy
4. ✅ Finalize technical architecture
5. ✅ Complete external library evaluation

**DO NOT** begin implementation until all requirements are finalized and approved.

---

*This document will be updated as the project evolves and requirements become more defined.*

