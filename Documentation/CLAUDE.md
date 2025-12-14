# WFS DIY - Development Notes for Claude

## Project Overview
Wave Field Synthesis (WFS) audio application built with JUCE framework for real-time multi-channel audio processing with comprehensive OSC network control.

## Current Implementation Status (As of 2025-12-14)

### Overall Progress: ~35% Complete

The application has established a solid foundation with infrastructure and core UI:
- Complete parameter management system
- Professional GUI framework with tabbed interface
- Bidirectional OSC communication
- Project-based save/load system
- Interactive multitouch Map view
- Complete Clusters management
- Network Log window

**Major features still to implement:**
- Audio Patch routing window with input/output matrices
- Reverb algorithm design (convolution/algorithmic)
- EQ visualizations (Outputs: 6-band, Reverbs: 4-band)
- Snapshot system UI
- WFS geometry to audio engine data processing
- DSP algorithm development
- Tracking protocols (PSN, RTTrP, OSC for tracking)
- ADM-OSC protocol
- GPU Audio framework port

---

## Architecture Overview

### Core Systems

| System | Status | Description |
|--------|--------|-------------|
| Parameters | Complete | ValueTree-based hierarchical state management |
| GUI Tabs | 90% | All 7 tabs have UI, some features pending |
| OSC Network | 90% | OSC, Remote protocols complete; ADM-OSC, PSN, RTTrP pending |
| Save/Load | 80% | Project folder management (snapshots UI TODO) |
| Audio Engine | Basic | Dual algorithm support, needs geometry integration |
| Separate Windows | 50% | Log window complete, Patch window TODO |
| Map View | 90% | Interactive multitouch map complete |
| Data Processing | 0% | WFS calculations not yet implemented |
| DSP Algorithms | Basic | Simple delay/gain, proper WFS algorithms TODO |

---

## Tab Implementation Summary

### SystemConfigTab - COMPLETE
**Lines: ~1,359 | Status: 100%**

All UI elements implemented:
- Show Section: Name and location editors
- I/O Section: Channel count selectors, Audio Interface button, Algorithm selector, Processing toggle
- Stage Section: Dimensions, origin presets (custom icons), speed of sound/temperature
- Master Section: Level, latency, Haas effect
- Footer: Project management buttons (Store, Reload, Import, Export)

**TODO:**
- Audio Interface Window needs input/output patching matrices
- Test tone generation

---

### NetworkTab - MOSTLY COMPLETE
**Lines: ~2,400 | Status: 90%**

Implemented:
- Network interface selector with IP display
- UDP/TCP port configuration
- OSC Query server (port 5005)
- 6-target connection table with status indicators
- OSC and Remote protocols fully working
- IP filtering (Accept All / Registered Only)
- ADM-OSC transform section (UI complete)
- Tracking section UI

**TODO:**
- ADM-OSC protocol implementation (placeholder)
- PSN tracking protocol implementation (placeholder)
- RTTrP tracking protocol implementation (placeholder)
- OSC for tracking data reception
- Extensive protocol testing required

---

### OutputsTab - MOSTLY COMPLETE
**Lines: ~1,559 | Status: 85%**

Implemented:
- Channel selector with array assignment
- Output Properties: Attenuation, delay, parallax, options
- Position: X/Y/Z, orientation dial, angles, pitch, HF damping
- EQ: 6 bands with shape/freq/gain/Q controls

**TODO:**
- Output Array Wizard (generate arrays from geometry)
- EQ frequency response visualization (6-band graph)
- Array Position Helper (currently placeholder alert)

---

### InputsTab - MOSTLY COMPLETE
**Lines: ~4,086 | Status: 85%**

Implemented (9 sub-tabs):
1. Channel: Name, attenuation, delay, height factor
2. Position: XYZ position/offset, constraints, flip, cluster, tracking
3. Attenuation: Law, distance attenuation, ratio
4. Directivity: Angle, rotation dial, tilt, HF shelf
5. Live Source Taming: Radius, shape, thresholds
6. Hackoustics: Floor reflections with filters
7. Jitter: Random offset generators
8. LFO: Per-axis modulation parameters
9. AutomOtion: Automation targets and triggers

Additional:
- Transport controls (Play/Stop/Pause)
- Map lock/visibility controls
- Store/Reload footer

**TODO:**
- Snapshot system UI (Create, Load, Update, Edit Scope, Delete buttons exist but not implemented)
- Remote protocol secondary touch functions (Android app integration)

---

### ReverbTab - PARTIAL
**Lines: ~1,843 | Status: 50%**

Implemented:
- Reverb sub-tab: Time, pre-delay, type, algorithm selector, damping, width
- Position sub-tab: X/Y/Z coordinates
- Reverb Feed sub-tab: Per-input send sliders and mutes
- EQ sub-tab: 4 bands (200/800/2000/5000 Hz)
- Reverb Return sub-tab: Output assignment, mutes
- Map visibility toggle

**TODO:**
- Algorithm sub-tab (currently placeholder)
- Reverb algorithm implementation (Convolution/Algorithmic)
- EQ frequency response visualization (4-band graph)
- IR loading and management for convolution mode

---

### ClustersTab - COMPLETE
**Lines: ~868 | Status: 100%**

Fully implemented:
- 10 cluster selector buttons with dynamic count display
- Assigned inputs panel with tracked input highlighting
- Reference mode selector (First Input / Barycenter)
- Position joystick + Z slider (50Hz polling)
- Attenuation slider (delta-based)
- Rotation endless dial
- Scale joystick
- Plane selector (XY/XZ/YZ)
- Stage bounds enforcement per input constraints

**TODO (Minor):**
- Remote protocol enhancements (Android app + WFS processor coordination)

---

### MapTab - MOSTLY COMPLETE
**Lines: ~2,216 | Status: 90%**

Implemented:
- Grid, stage bounds, origin marker display
- Input circles with HSL coloring and channel numbers
- Output trapezoid speaker icons with membrane triangles
- Reverb diamond shapes
- Cluster boundary shapes
- Mouse navigation: Wheel zoom, right-drag pan, middle-click reset
- Touch navigation: Single/two/three finger gestures
- Secondary touch: Pinch for Z/scale, rotation for rotation/cluster rotation
- Selection and dragging with constraint enforcement
- Lock and visibility controls

**TODO:**
- Minor polish and edge case handling
- Mostly feature-complete

---

## Network Protocols Status

| Protocol | Status | Description |
|----------|--------|-------------|
| OSC | Complete | Standard bidirectional OSC communication |
| Remote | Complete | Android WFS Control app protocol |
| OSC Query | Complete | HTTP parameter discovery server |
| ADM-OSC | UI Only | Transform controls exist, protocol not implemented |
| PSN | Placeholder | PosiStageNet tracking protocol |
| RTTrP | Placeholder | Real-time Tracking Protocol |
| OSC Tracking | TODO | External tracking data via OSC |

---

## Data Processing & DSP

### Current State
The application has two basic audio algorithms:
- **InputBufferAlgorithm**: Read-time delays, per-input threads
- **OutputBufferAlgorithm**: Write-time delays, per-output threads

Both algorithms apply simple delay and gain but lack:
- WFS geometry calculations (delays from source/speaker positions)
- Proper amplitude panning based on physics
- Real-time updates as sources move

### TODO
1. **Data Processing Layer**
   - Calculate delays from source/speaker geometry
   - Calculate amplitude weights
   - Apply directivity patterns
   - Handle tracking updates

2. **DSP Algorithm Development**
   - Implement proper WFS delay/amplitude matrices
   - Add interpolation for smooth parameter changes
   - Implement reverb processing (convolution or algorithmic)

3. **GPU Audio Port**
   - Port algorithms to GPU Audio framework
   - Leverage parallel processing for multi-channel

---

## Floating Windows

### AudioInterfaceWindow - PARTIAL
Basic JUCE AudioDeviceSelectorComponent shown, but:
- **TODO**: Input patching matrix
- **TODO**: Output patching matrix
- **TODO**: Test tone generation and routing

### NetworkLogWindow - COMPLETE
Fully implemented:
- Independent floating window
- Master logging switch
- Filter modes: Transport, Protocol, Client IP, Rejected
- Dynamic toggles based on log content
- Color-coded rows
- CSV export
- Auto-scroll with manual override

---

## TODO Summary by Priority

### High Priority
1. **Data Processing**: WFS geometry calculations feeding audio engine
2. **DSP Algorithms**: Proper delay/amplitude matrix computation
3. **Snapshot System UI**: InputsTab snapshot buttons need implementation
4. **Audio Patch Window**: Input/output routing matrices

### Medium Priority
5. **Reverb Algorithm**: Design convolution/algorithmic reverb processing
6. **EQ Visualizations**: Frequency response graphs for Outputs (6-band) and Reverbs (4-band)
7. **Output Array Wizard**: Generate speaker arrays from geometry
8. **Protocol Implementation**: ADM-OSC, PSN, RTTrP

### Lower Priority
9. **Remote Protocol Enhancements**: Secondary touch functions
10. **GPU Audio Port**: After DSP algorithms are working
11. **Testing**: Comprehensive protocol and feature testing

---

## Recent Work (2025-12-03 to 2025-12-14)

### Network System (Dec 3-5)
- Complete OSC communication stack
- OSCManager, OSCConnection, OSCMessageRouter, OSCMessageBuilder
- OSCRateLimiter (50Hz), OSCLogger
- Custom UDP/TCP receivers with sender IP tracking
- IP filtering

### OSC Source Filtering (Dec 11)
- OSCReceiverWithSenderIP, OSCTCPReceiver, OSCParser
- UI toggle for filtering modes

### OSC Query & Connection Status (Dec 13)
- HTTP server for parameter discovery
- Visual connection status indicators

### Remote Protocol (Dec 13)
- Full bidirectional communication with Android app
- 48+ parameter mappings
- Channel dump on selection
- Array Adjust commands

### ClustersTab Implementation (Dec 13)
- Complete cluster management UI
- 50Hz polling with auto-centering controls
- Rotation/scale transformations
- Tracking integration with constraint enforcement

### MapTab Implementation (Dec 13-14)
- Interactive 2D visualization
- Mouse and touch navigation
- Secondary touch gestures for Z/rotation
- Cluster dragging and scaling
- Trapezoid speaker icons with membrane
- Visibility and lock controls

### Dial Formula Fixes (Dec 14)
- Fixed bidirectional slider formulas for -1 to 1 range
- Stage constraint enforcement in InputsTab and ClustersTab
- Continuous Z slider polling

---

## Code Statistics

| Component | Lines |
|-----------|-------|
| Parameters System | ~2,200 |
| Network/OSC System | ~5,300 |
| GUI Components | ~14,300 |
| MainComponent | ~850 |
| Audio Processing | ~1,500 |
| **Total** | **~24,150** |

---

## Building & Running

### Requirements
- JUCE 8.0.11
- Visual Studio 2022 (Windows) / Xcode (macOS)
- ASIO-capable audio interface (optional)

### Build Configuration
- JUCE_ASIO=1 (Windows only)
- JUCE_JACK=0
- macOS: Extra linker flags for SystemConfiguration framework

### Build Command
```
MSBuild WFS-DIY.sln -p:Configuration=Debug -p:Platform=x64
```

---

*Last updated: 2025-12-14*
*JUCE Version: 8.0.11*
*Build: Visual Studio 2022 / Xcode, x64 Debug/Release*
