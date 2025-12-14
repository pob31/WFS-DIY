# WFS-DIY Project Notes

## Project Overview
WFS-DIY is a Wave Field Synthesis audio processing application built with JUCE. It provides multi-channel audio processing with configurable delay and gain matrices for spatial audio rendering.

## Architecture

### Core Components
- **MainComponent** - Main application window, audio engine, and tab container
- **WfsParameters** - Centralized parameter management with ValueTree state
- **InputBufferAlgorithm / OutputBufferAlgorithm** - Audio processing algorithms

### GUI Structure
- **SystemConfigTab** - Processing toggle, channel count configuration
- **NetworkTab** - OSC target configuration, IP filtering, connection status
- **InputsTab / OutputsTab** - Channel parameter controls
- **ClustersTab** - Input cluster management with position/rotation/scale/attenuation controls
- **ReverbTab** - Reverb processing settings
- **MapTab** - Spatial visualization

### Floating Windows
- **AudioInterfaceWindow** - JUCE AudioDeviceSelectorComponent for device setup
- **NetworkLogWindow** - Network traffic monitoring with filtering and export
- **OutputArrayHelperWindow** - "Wizard of OutZ" for speaker array positioning

## Network System (Source/Network/)

### OSC Communication
- **OSCManager** - Central coordinator for all OSC communication
- **OSCConnection** - Individual target connection (UDP/TCP)
- **OSCReceiverWithSenderIP** - Custom UDP receiver exposing sender IP
- **OSCTCPReceiver** - TCP receiver for reliable connections
- **OSCMessageRouter** - Address pattern parsing and routing
- **OSCRateLimiter** - Message throttling with coalescing

### Logging System
- **OSCLogger** - Ring buffer storage for network messages (1000 entries max)
- **LogEntry** - Extended struct with IP, port, transport, protocol, rejected status
- Logging disabled by default to avoid background overhead

### Protocols
- **OSC** - Standard Open Sound Control
- **Remote** - Remote input control protocol
- **ADM-OSC** - Audio Definition Model (placeholder)
- **OSCQuery** - Parameter discovery server
- **PSN / RTTrP** - Tracking protocols (placeholder)

## Network Log Window Features
- Independent floating window (for second monitor)
- Master logging switch (disabled by default)
- Filter modes: Transport (UDP/TCP), Protocol, Client IP, Rejected
- Dynamic toggles based on protocols/IPs seen in log
- Color-coded rows by filter mode
- CSV export (filtered or all data)
- Auto-scroll with manual override

## GUI Components (Source/gui/)

### Custom Sliders (Source/gui/sliders/)
- **WfsSliderBase** - Base class for all custom sliders, `handlePointer()` protected for derived classes
- **WfsStandardSlider** - Standard 0-1 range slider
- **WfsBidirectionalSlider** - Center-zero slider (-1 to 1 range)
- **WfsAutoCenterSlider** - Returns to center on mouse release, supports 50Hz timer-based polling via `onPositionPolled` callback
- **WfsWidthExpansionSlider** - For angle parameters

### Other Custom Components
- **WfsJoystickComponent** - 2D XY control with auto-center
- **WfsBasicDial** - Rotary dial control
- **WfsRotationDial** - 360° rotation control
- **ChannelSelectorButton/Overlay** - Grid-based channel selector
- **EQDisplayComponent** - Interactive parametric EQ visualization

### Bidirectional Slider Usage
When using WfsBidirectionalSlider, formulas must account for -1 to 1 range:
- **Display**: `value = v * maxValue` (e.g., `v * 100.0f` for ±100ms)
- **Set slider**: `v = value / maxValue` (e.g., `ms / 100.0f`)

## Tracking System
Tracking is active only when ALL THREE conditions are true:
1. **Global toggle ON** - `trackingEnabled != 0`
2. **Protocol NOT disabled** - `trackingProtocol != 0` (1=OSC, 2=PSN, 3=RTTrP)
3. **Local input toggle ON** - Per-input `trackingActive` parameter

When tracking is active, joystick/remote control Offset X/Y/Z; otherwise Position X/Y/Z.

### Cluster Tracking Constraint
Only one input per cluster can have tracking enabled when global tracking is active.
- Enforced at ValueTree level in `WFSValueTreeState::enforceClusterTrackingConstraint()`
- Catches changes from all sources: UI, OSC, Remote protocol, file loading
- When a second input enables tracking in a cluster, the first is automatically disabled
- UI dialogs (InputsTab, NetworkTab) provide user-friendly warnings for interactive operations

## Clusters Tab (Source/gui/ClustersTab.h)
Manages groups of inputs with collective transformations:
- **10 cluster selector buttons** - Switch between clusters (greyed if empty)
- **Assigned inputs list** - Shows inputs in selected cluster (tracked input highlighted at top)
- **Reference mode** - First Input or Barycenter for transformation center
- **Position joystick + Z slider** - Move all inputs (or tracked input's offset)
- **Attenuation slider** - Relative dB change to all inputs in cluster
- **Rotation dial** - Rotate inputs around reference point in selected plane
- **Scale joystick** - Scale inputs relative to reference point
- **Plane selector** - XY, XZ, or YZ plane for rotation/scale operations

All controls use 50Hz timer-based updates with auto-centering behavior.

### Stage Bounds Constraint Enforcement
Both InputsTab and ClustersTab joystick/slider controls enforce stage bounds when constraint buttons are enabled:
- **Stage bounds**: Min = -origin, Max = stageSize - origin (per axis)
- **InputsTab**: Joystick checks `constraintXButton`/`constraintYButton`, Z slider checks `constraintZButton`
- **ClustersTab**: `applyPositionDelta()` checks each input's individual constraint parameters
- Constraints apply to total position (position + offset) when tracking is active

## Map Tab (Source/gui/MapTab.h)
Interactive 2D visualization of the WFS spatial layout:

### Display Elements
- **Grid** - 1m grid lines (dark gray)
- **Stage bounds** - White rectangle showing stage dimensions
- **Origin marker** - White circle with crosshairs at coordinate origin
- **Inputs** - Colored circles with channel numbers (interactive, draggable)
- **Outputs** - Trapezoid speaker icons with membrane, showing orientation, color-coded by array
- **Reverbs** - Purple diamond shapes
- **Clusters** - Boundary shapes around grouped inputs

### Mouse Navigation
- **Mouse wheel** - Zoom in/out (centered on cursor)
- **Right-click drag** - Pan view
- **Left+Right drag** - Zoom (vertical movement)
- **Middle-click** - Reset view to fit stage
- **Left-click** - Select input (yellow ring)
- **Left-drag** - Move selected input position

### Touch Navigation
- **Single touch on input** - Drag input position
- **Single touch on barycenter** - Drag entire cluster
- **Two-finger drag** - Pan view
- **Three-finger drag** - Zoom (vertical movement)

### Secondary Touch (Two-Finger Gestures on Inputs)
When dragging an input with one finger, a second finger on empty space creates a "secondary touch":
- **Target selection** - Affects closest dragged input/cluster not already engaged
- **Pinch/stretch** - Adjusts Z value (ratio-based, or additive if Z near 0)
- **Rotation** - Adjusts inputRotation parameter
- **Visual feedback** - Grey reference line (initial vector) + white active line (current)

| Input State | Pinch/Stretch Effect | Rotation Effect |
|-------------|---------------------|-----------------|
| Tracked (no cluster) | Offset Z | inputRotation |
| Normal (no cluster) | Position Z | inputRotation |
| Tracked + Cluster | Cluster scale (XY) | Cluster rotation |
| Cluster reference | Cluster scale (XY) | Cluster rotation |
| Barycenter | Cluster scale (XY) | Cluster rotation |

Z constraint checking: When `inputConstraintZ` is ON, Z is limited to [0, stageHeight].

### Input Drag Behavior
Based on input state:
- **Normal input** - Updates Position X/Y
- **Tracked input** - Updates Offset X/Y (position unchanged)
- **Cluster reference** - Moves entire cluster (maintains geometry)
- **Tracked + reference** - Updates offset AND moves cluster members

Stage constraints (inputConstraintX/Y/Z) only enforced when enabled.

### Coordinate System
- Stage coordinates in meters, origin at (originWidth, originDepth)
- Screen Y-axis inverted (Y increases downward on screen, upward on stage)
- `stageToScreen()` / `screenToStage()` conversion functions

### Visual Styling
- HSL-based colors for markers (matching Android WFS Control app)
- Inputs: hue = (id × 360 / 32) % 360, saturation 0.9, lightness 0.6
- Arrays/Clusters: hue = (id × 360 / 10) % 360, saturation 0.7, lightness 0.7
- Selected input: Yellow highlight ring
- Locked input: Gray outer circle, red channel number
- LS radius: White circle with 10% opacity when active
- Height indicator: Small triangle when Z ≠ 0

### Speaker Icon Design
Outputs displayed as trapezoid (keystone) shapes representing speakers:
- **Outer trapezoid** - Dark gray stroke, wide base at back, narrow tip pointing toward audience
- **Inner membrane triangle** - Color-filled, base aligned with trapezoid back corners
- **Dimensions** - Height 24px, back width 21px, front width 11px
- **Membrane** - Extends 55% from back toward front
- **Channel number** - Bold 12pt black text at triangle centroid
- **Colors** - Array color (HSL-based) when assigned, light gray when single (array 0)
- **Orientation** - 0° points toward audience (up on screen)

### Map Visibility & Lock Controls
Per-channel controls in respective tabs:
- **InputsTab** - Lock button (🔒/🔓) + visibility toggle per input
- **OutputsTab** - Visibility toggle (speaker or array visibility based on assignment)
- **ReverbTab** - Global visibility toggle for all reverbs

### Map Display Parameters
```
inputMapLocked      - Lock input position on map (0/1)
inputMapVisible     - Show input on map (0/1)
outputMapVisible    - Show individual speaker on map (0/1)
outputArrayMapVisible - Show array speakers on map (0/1)
reverbsMapVisible   - Show all reverbs on map (0/1, global)
```

## Keyboard Shortcuts
- **F1-F10** - Assign input/output to cluster/array 1-10
- **F11** - Assign to Single (no cluster)
- **Up/Down arrows** - Navigate channels
- **Tab** - Switch between tabs

## Parameter Defaults (Source/Parameters/)
- **WFSParameterIDs.h** - Parameter identifier definitions
- **WFSParameterDefaults.h** - Default values, min/max ranges
- **WFSValueTreeState.cpp** - ValueTree structure and creation

### Reverb EQ Default Frequencies
Band 1: 200 Hz, Band 2: 800 Hz, Band 3: 2000 Hz, Band 4: 5000 Hz

## Build Notes
- JUCE 8.x required
- Project file: WFS-DIY.jucer
- Builds: Visual Studio 2022, Xcode, Linux Makefile
- Build command: `MSBuild WFS-DIY.sln -p:Configuration=Debug -p:Platform=x64`

## Key Files
- `Source/MainComponent.h/cpp` - Application entry point, keyboard handling
- `Source/WfsParameters.h/cpp` - Parameter definitions
- `Source/Parameters/WFSValueTreeState.h/cpp` - State management
- `Source/Network/OSCManager.h/cpp` - Network coordination
- `Source/Network/OSCLogger.h/cpp` - Message logging
- `Source/gui/InputsTab.h` - Input channel controls with joystick
- `Source/gui/ReverbTab.h` - Reverb settings with EQ
- `Source/gui/NetworkLogWindow.h/cpp` - Log window UI
- `Source/gui/OutputArrayHelperWindow.h/cpp` - Wizard of OutZ window
- `Source/Helpers/ArrayGeometryCalculator.h/cpp` - Speaker array geometry calculations

## Wizard of OutZ (Output Array Helper)
A wizard-style dialog for quickly configuring speaker array positions with preset acoustic defaults.

### Access
- Button in OutputsTab header (right side): "Wizard of OutZ..."

### 7 Array Presets
| Preset | LS | HF dB/m | Dist% | Hp | Vp | EQ |
|--------|-----|---------|-------|-----|-----|-----|
| Near Field Straight | ON | -0.4 | 100 | 2 | 0.5 | LC 80Hz |
| Near Field Curved | ON | -0.4 | 100 | 2 | 0.5 | LC 80Hz |
| Main Room Straight | OFF | -0.2 | 100 | 10 | -4 | - |
| Sub Bass | OFF | 0 | 50/100* | 0 | 0 | HC 300Hz |
| Surround | OFF | -0.3 | 100 | 3 | -2 | - |
| Delay Line | OFF | -0.15 | 100 | 3 | -2 | - |
| Circle | OFF | -0.3 | 100 | 0 | 0 | - |

*Sub Bass: 50% distance attenuation when N ≤ 2, otherwise 100%

### Geometry Methods
- **Center + Spacing** - Define center point and speaker spacing
- **Endpoints** - Define start and end points for the array
- **Curved arrays** - Use sag parameter (negative = toward audience)
- **Circle arrays** - Radius, start angle, facing inward/outward
- **Surround pairs** - Width and Y range for left/right mirrored speakers

### Key Features
- **Live preview** - Auto-updates as parameters change
- **Target section** - Select array assignment and starting output
- **Auto-advance** - After Apply, advances to next array and output position
- **Orientation convention** - 0° = facing audience (toward -Y)

### Geometry Calculations (Source/Helpers/ArrayGeometryCalculator.cpp)
- `calculateStraightFromCenter()` - Straight line from center point
- `calculateStraightFromEndpoints()` - Straight line between two points
- `calculateCurvedArray()` - Quadratic Bezier curve with sag, auto-fanning toward audience
- `calculateCircleArray()` - Circular arrangement with inward/outward facing
- `calculateSurroundPairs()` - Left/right mirrored pairs

## EQ Display Component (Source/gui/EQDisplayComponent.h)
Reusable interactive parametric EQ visualization for OutputsTab (6 bands) and ReverbTab (4 bands).

### Visual Elements
- **Grid** - Logarithmic frequency scale (20Hz-20kHz), linear dB scale (±24dB)
- **Frequency lines** - At 20, 50, 100, 200, 500, 1k, 2k, 5k, 10k, 20k Hz
- **dB lines** - Every 6dB, 0dB emphasized
- **Response curve** - White outline with semi-transparent blue fill
- **Band markers** - Color-coded circles with band numbers

### Mouse Interaction
- **Click** - Select band marker
- **Drag horizontal** - Adjust frequency
- **Drag vertical** - Adjust gain (except Cut/BandPass filters)
- **Mouse wheel** - Adjust Q parameter

### Filter Types Supported
| Type | Shape ID (Output) | Shape ID (Reverb) | Notes |
|------|-------------------|-------------------|-------|
| Off | 0 | 0 | Band inactive |
| Low Cut | 1 | 1 | High-pass with Q resonance |
| Low Shelf | 2 | 2 | Q controls transition steepness |
| Peak/Notch | 3 | 3 | Q controls bandwidth |
| Band Pass | 4 | - | Output EQ only |
| High Shelf | 5 | 4 | Q controls transition steepness |
| High Cut | 6 | 5 | Low-pass with Q resonance |

### Configuration
```cpp
// For Output EQ (6 bands)
EQDisplayConfig::forOutputEQ()

// For Reverb EQ (4 bands)
EQDisplayConfig::forReverbEQ()
```

### Parameter Mapping
| Aspect | Output EQ | Reverb EQ |
|--------|-----------|-----------|
| Shape ID | `eqShape` | `reverbEQshape` |
| Frequency ID | `eqFrequency` | `reverbEQfreq` |
| Gain ID | `eqGain` | `reverbEQgain` |
| Q ID | `eqQ` | `reverbEQq` |
| Q Range | 0.1-10.0 | 0.1-20.0 |

### Band Colors (8-color palette)
1. Red, 2. Orange, 3. Yellow, 4. Green, 5. Teal, 6. Blue, 7. Purple, 8. Pink

### Biquad Calculations
Uses Audio EQ Cookbook formulas matching filterCalc.js:
- **Cut filters**: `alpha = sin(w0) / (2 * Q)`
- **Shelf filters**: `alpha = (sin(w0)/2) * sqrt((A + 1/A) * (1/Q - 1) + 2)`
- **Peak/Notch**: `alpha = sin(w0) / (2 * Q)`
