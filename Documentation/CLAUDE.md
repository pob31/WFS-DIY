# WFS DIY - Development Notes for Claude

## Project Overview
Wave Field Synthesis (WFS) audio application built with JUCE framework for real-time multi-channel audio processing with comprehensive OSC network control.

## Codebase Statistics

| Metric | Value |
|--------|-------|
| **Total Source Files** | 99 files (76 headers, 23 cpp) |
| **Total Lines of Code** | ~55,400 lines |
| **Git Commits** | 185 commits |
| **Development Started** | October 2025 |

### Code Distribution by Module

| Module | Files | Lines | Description |
|--------|-------|-------|-------------|
| GUI | 44 | 31,100 | Tabs, windows, custom components |
| DSP | 29 | 12,500 | Audio processing, calculations, reverb engine |
| Network | 20 | 6,300 | OSC, protocols, logging |
| Parameters | 6 | 5,900 | ValueTree state, file I/O |
| Core | 7 | 3,000 | MainComponent, WfsParameters |
| Helpers | 3 | 800 | Geometry calculations |

---

## Current Implementation Status (As of 2026-02-09)

### Overall Progress: ~80% Complete

The application has established a solid foundation with infrastructure and core UI:
- Complete parameter management system
- Professional GUI framework with tabbed interface
- Bidirectional OSC communication
- Project-based save/load system with snapshot scope editing
- Interactive multitouch Map view
- Complete Clusters management
- Network Log window
- Color scheme system (3 themes: Default, OLED Black, Light)
- **DSP Calculation Layer** (delay/level/HF matrices from geometry)
- **Input Visualisation** (real-time DSP matrix display)
- **Live Source Tamer** (per-speaker gain reduction for feedback prevention)
- **Floor Reflections** (simulated floor bounce with filtering and diffusion)
- **Audio Interface & Patching Window** (input/output patch matrices with test signal generation)
- **Snapshot Scope Window** (parameter-level, per-channel granularity for snapshots)

- **Level Metering System** (floating window with input/output meters and thread performance)
- **Binaural Solo Monitoring** (virtual speaker rendering for headphone monitoring)

**Major features still to implement:**
- Snapshot system UI in InputsTab (scope window complete)
- ADM-OSC protocol
- GPU Audio framework port

---

## Architecture

### Core Components
- **MainComponent** - Main application window, audio engine, and tab container
- **WfsParameters** - Centralized parameter management with ValueTree state
- **WFSCalculationEngine** - DSP calculation engine (delay/level/HF matrices)
- **InputBufferAlgorithm / OutputBufferAlgorithm** - Audio processing algorithms

### GUI Structure
- **SystemConfigTab** - Processing toggle, channel count configuration
- **NetworkTab** - OSC target configuration, IP filtering, connection status
- **InputsTab** - Input channel parameters with sub-tabs
- **OutputsTab** - Output channel parameters with 3 condensed sub-tabs:
  - "Channel Parameters" (3-column layout: Position/Orientation, Angular Settings, Array Assignment)
  - "Output EQ" (6-band parametric EQ with interactive display)
  - "Options" (LS attenuation, FR enable, parallax, HF attenuation)
- **ClustersTab** - Input cluster management with position/rotation/scale/attenuation controls
- **ReverbTab** - Reverb processing with 4 sub-tabs:
  - "Channel Parameters" (3-column layout: Reverb+Position, Reverb Feed, Reverb Return)
  - "Pre-Processing" (4-band parametric pre-EQ per-channel + global pre-compressor with dials)
  - "Algorithm" (SDN/FDN/IR selector, decay params, wet level — global, with full DSP engine)
  - "Post-Processing" (4-band parametric post-EQ global + global post-expander with dials)
- **MapTab** - Spatial visualization

### Floating Windows
- **AudioInterfaceWindow** - Audio device settings and input/output patching with test signal generation
- **NetworkLogWindow** - Network traffic monitoring with filtering and export
- **OutputArrayHelperWindow** - "Wizard of OutZ" for speaker array positioning
- **SetAllInputsWindow** - Bulk parameter changes across all inputs (long-press access)
- **SnapshotScopeWindow** - Extended scope editing for input snapshots (parameter-level, per-channel)
- **LevelMeterWindow** - Real-time level metering with input/output meters, solo buttons, and thread performance

### Core Systems Status

| System | Status | Description |
|--------|--------|-------------|
| Parameters | Complete | ValueTree-based hierarchical state management |
| GUI Tabs | 90% | All 7 tabs have UI, some features pending |
| OSC Network | 95% | OSC, Remote, PSN, RTTrP protocols complete; ADM-OSC pending |
| Save/Load | 85% | Project folder management, snapshot scope editing complete |
| Audio Engine | 90% | Dual algorithm support with DSP calculation layer + Live Source Tamer + Floor Reflections + Reverb Engine |
| Separate Windows | 95% | Log, Patch, Array Helper, Snapshot Scope windows complete |
| Map View | 90% | Interactive multitouch map complete |
| Data Processing | 90% | WFS delay/level/HF + reverb matrices implemented |
| DSP Algorithms | 95% | Delay/gain/HF/FR filters + Reverb DSP (FDN/SDN/IR) + Pre/Post processing complete |
| Theming | Complete | 3 color schemes with live switching |

---

## DSP Calculation Layer (Source/DSP/)

### Overview
The DSP calculation layer transforms human control parameters into real-time DSP data for Wave Field Synthesis spatial audio processing.

### Core Files
- **WFSCalculationEngine.h/cpp** - Central calculation engine for all WFS DSP parameters
- **LFOProcessor.h** - Low Frequency Oscillator for position/rotation modulation
- **AutomOtionProcessor.h** - Programmed point-to-point position movement with audio triggering
- **WFSHighShelfFilter.h** - High-frequency shelf biquad filter for air absorption
- **WFSBiquadFilter.h** - Generic biquad filter for Floor Reflection low-cut and high-shelf
- **InputBufferProcessor.h** - Per-input threaded audio processor (with FR support)
- **OutputBufferProcessor.h** - Per-output threaded audio processor (with FR support)
- **InputBufferAlgorithm.h** - Manages collection of InputBufferProcessors
- **OutputBufferAlgorithm.h** - Manages collection of OutputBufferProcessors
- **LiveSourceLevelDetector.h** - Per-input audio level detection (peak envelope, short peak, RMS)
- **LiveSourceTamerEngine.h** - Control-rate engine for per-speaker LS gain calculation
- **LevelMeteringManager.h** - Audio level metering for inputs/outputs with thread performance
- **BinauralCalculationEngine.h** - Binaural solo delay/level/HF calculation
- **BinauralProcessor.h** - Binaural solo rendering with delay lines and HF filters
- **InputVisualisationComponent.h** - Real-time DSP matrix visualization
- **ReverbEngine.h** - Multi-channel reverb engine with pre/post processing and algorithm management
- **ReverbAlgorithm.h** - Abstract base class for reverb algorithms (processBlock, prepare, setParallelFor)
- **ReverbFDNAlgorithm.h** - Feedback Delay Network: 16 delay lines per node, Walsh-Hadamard mixing, 3-band decay
- **ReverbSDNAlgorithm.h** - Scattering Delay Network: N*(N-1) inter-node paths, Householder scattering, geometry-based delays
- **ReverbIRAlgorithm.h** - Impulse Response convolution using juce::dsp::Convolution per node
- **ReverbBiquadFilter.h** - Biquad filter for reverb EQ bands (low-shelf, peak, high-shelf)
- **ReverbPreProcessor.h** - Per-channel 4-band parametric EQ + global pre-compressor
- **ReverbPostProcessor.h** - Global 4-band parametric post-EQ + global post-expander (sidechain-keyed)
- **AudioParallelFor.h** - Fork-join thread pool for parallel per-node DSP processing

### Coordinate System
- **X**: Across stage (left-right, positive = right)
- **Y**: Along stage (positive = upstage/back, negative = downstage/front toward audience)
- **Z**: Height (positive = up)
- **Origin**: User-configurable offset from stage center (center-referenced system)

### Coordinate Display Modes
Positions can be displayed and input in three coordinate systems. Data is always stored as Cartesian (X, Y, Z) internally.

| Mode | Components | Description |
|------|------------|-------------|
| **Cartesian** | X, Y, Z (meters) | Default system, used for storage |
| **Cylindrical** | r, θ, Z | Radius, azimuth angle, height |
| **Spherical** | r, θ, φ | Radius, azimuth angle, elevation |

**Angle Conventions:**
- **Azimuth (θ)**: 0° = toward audience (-Y), 180°/-180° = upstage (+Y), 90° = stage right (+X)
- **Elevation (φ)**: 0° = horizontal plane, 90° = up (+Z), -90° = down (-Z)

**Conversion Formulas:**

Cartesian to Cylindrical:
```cpp
r = sqrt(x² + y²)
θ = atan2(-x, -y) * (180/π)  // 0° toward audience
Z = z
```

Cartesian to Spherical:
```cpp
r = sqrt(x² + y² + z²)
θ = atan2(-x, -y) * (180/π)  // 0° toward audience
φ = asin(z / r) * (180/π)    // 0° horizontal, 90° up
```

**Color Coding (MapTab):**
- **Yellow**: Cartesian mode (X, Y, Z)
- **Light Blue**: Cylindrical mode (r, θ, Z)
- **Light Pink**: Spherical mode (r, θ, φ)

Per-channel coordinate mode is set via the "Coord:" dropdown in each tab (Inputs, Outputs, Reverbs).

### Position Constraints
Input positions can be constrained to stay within stage bounds or distance limits.

**Axis Constraints (Cartesian Mode):**
- **Constraint X/Y/Z** - Per-axis toggles limiting position to stage bounds
- Stage bounds: `[-origin, stageSize - origin]` for each axis
- Applied in: joystick, text editors, map drag, keyboard nudge, OSC, cluster movement

**Distance Constraint (Cylindrical/Spherical Modes):**
When coordinate mode is Cylindrical or Spherical, a radial distance constraint replaces the X/Y axis constraints.

| Mode | Distance Calculation | Affected Axes |
|------|---------------------|---------------|
| **Cylindrical** | `sqrt(x² + y²)` | X, Y (Z unaffected) |
| **Spherical** | `sqrt(x² + y² + z²)` | X, Y, Z |

**Constraint Application:**
```cpp
// Scale position to maintain direction while constraining distance
float targetDist = jlimit(minDist, maxDist, currentDist);
float scale = targetDist / currentDist;
// Cylindrical: x *= scale; y *= scale;
// Spherical: x *= scale; y *= scale; z *= scale;
```

**UI Controls (InputsTab):**
- **Constraint R: ON/OFF** - Toggle button (replaces Constraint X in non-Cartesian modes)
- **Range slider** - Double-thumbed WfsRangeSlider for min/max distance
- **Min/Max editors** - Direct numeric entry (0-50m range)
- Controls are dimmed when constraint is disabled

**Parameters:**
| Parameter | Range | Description |
|-----------|-------|-------------|
| `inputConstraintDistance` | 0/1 | Enable distance constraint |
| `inputConstraintDistanceMin` | 0-50m | Minimum allowed distance from origin |
| `inputConstraintDistanceMax` | 0-50m | Maximum allowed distance from origin |

**OSC Addresses:**
- `/wfs/input/constraintDistance <ID> <value>` (0/1)
- `/wfs/input/constraintDistanceMin <ID> <value>` (meters)
- `/wfs/input/constraintDistanceMax <ID> <value>` (meters)

**Visibility Rules:**
| Coord Mode | X/Y Buttons | Z Button | Distance Controls |
|------------|-------------|----------|-------------------|
| Cartesian | Visible | Visible | Hidden |
| Cylindrical | Hidden | Visible | Visible |
| Spherical | Hidden | Hidden | Visible |

**Snap Behavior:**
- Position snaps immediately when constraint is enabled
- Position snaps when switching to Cylindrical/Spherical mode with constraint active
- Applied to all input paths: joystick, map drag, text editors, OSC, keyboard, cluster movement

### Stage Shapes
Three stage shapes available, selected in SystemConfigTab:

| Shape | Parameters | Description |
|-------|------------|-------------|
| **Box** | Width, Depth, Height (m) | Rectangular stage |
| **Cylinder** | Diameter, Height (m) | Vertical cylinder with circular footprint |
| **Dome** | Diameter, Elevation (1°-360°) | Partial sphere, 180°=hemisphere, 360°=full sphere |

**Origin Offset**: Stage center is at (0,0,0) in the center-referenced system. Origin offsets move the coordinate origin relative to stage center:
- **originWidth** (X offset): 0 = centered, negative = left
- **originDepth** (Y offset): 0 = centered, negative = front/downstage
- **originHeight** (Z offset): 0 = floor level, positive = above floor

**Origin Presets**:
- **Front**: Origin at front-center of stage (typical for frontal setups)
- **Center Ground**: Origin at stage center, floor level (typical for surround/cylinder)
- **Center**: Origin at volumetric center of stage

When changing stage shape, origin automatically resets to Center Ground (0,0,0).

### WFSCalculationEngine
Calculates per input->output pair:
1. **Delay (ms)** - Time alignment based on path length difference
2. **Level (linear 0-1)** - Distance-based attenuation + angular attenuation
3. **HF Shelf attenuation (dB)** - Air absorption based on distance

### Listener Position Calculation
Each speaker has a virtual "listener" position used as reference for delay calculations:
```cpp
float orientationRad = orientation * (PI / 180.0f);
listenerX = speakerX + Hparallax * sin(orientationRad);
listenerY = speakerY - Hparallax * cos(orientationRad);
listenerZ = speakerZ + Vparallax;
```

### Delay Calculation
```cpp
inputToListener = distance3D(inputPosition, listenerPosition)
speakerToListener = distance3D(speakerPosition, listenerPosition)
delayMs = max(0, (inputToListener - speakerToListener) / 343.0f * 1000.0f)
```

### Level Calculation
```cpp
attenuationDb = inputAttenuation + inputDistanceAttenuation * inputToSpeaker * (outputDistAttenPercent / 100.0f)
level = pow(10.0f, attenuationDb / 20.0f) * angularAttenuation
```

### HF Air Absorption
- **Filter**: High shelf at 800 Hz, Q = 0.3
- **Attenuation**: `outputHFattenuation * inputToSpeaker` (dB/m x distance)
- One biquad filter per input->output pair in processor threads

### Angular Attenuation
Based on speaker orientation, pitch, angleOn, and angleOff:
- **Rear axis**: Direction opposite to where speaker points (orientation + 180)
- **angleOn**: Cone behind speaker where inputs are fully reproduced (attenuation = 1.0)
- **angleOff**: Cone in front where inputs are muted (attenuation = 0.0)
- **Transition zone**: Linear interpolation between angleOn and angleOff

```cpp
// Calculate angle from speaker's rear axis to input
rearAxisX = sin(orientationRad) * cos(pitchRad)
rearAxisY = -cos(orientationRad) * cos(pitchRad)
rearAxisZ = sin(pitchRad)
angleFromRear = acos(dot(rearAxis, toInput))

// Zone-based attenuation
if (angle <= angleOn) return 1.0f;           // Full reproduction
if (angle >= angleOff) return 0.0f;          // Muted
return (angleOff - angle) / (angleOff - angleOn);  // Transition
```

### Input Muting
- Per-input `inputMutes` parameter: comma-separated list of muted outputs
- Example: `"1,5,12"` mutes this input for outputs 1, 5, and 12
- Muted routings skip calculation entirely (level = 0, no processing)

### Sidelines (Edge Muting)
Automatic position-based muting when sources approach stage edges. Per-channel feature that prevents sound from "spilling" outside the intended performance area.

**Stage Shape Behavior:**
| Shape | Affected Edges | Detection Method |
|-------|---------------|------------------|
| **Box** | Left, Right, Upstage (back) | Min distance to any edge |
| **Cylinder/Dome** | Circular edge | Radial distance from center |

**Note:** Downstage edge (front toward audience) is NOT affected - sound naturally projects toward audience.

**Fringe Zone Logic:**
```
Total Fringe = inputSidelinesFringe (0.1-10.0m)

Distance from edge:
  > fringe        → Full signal (attenuation = 1.0)
  fringe/2 to fringe → Linear fade (0.0 to 1.0)
  0 to fringe/2   → Full mute (attenuation = 0.0)
  < 0 (outside)   → Full mute (attenuation = 0.0)
```

**Parameters:**
| Parameter | Range | Description |
|-----------|-------|-------------|
| `inputSidelinesActive` | 0/1 | Enable sidelines for this input |
| `inputSidelinesFringe` | 0.1-10.0 m | Total fringe zone width |

**OSC Addresses:**
- `/wfs/input/sidelinesEnable <ID> <value>` (0/1)
- `/wfs/input/sidelinesFringe <ID> <value>` (meters)

**Integration:**
- Applied as final linear multiplier in level calculation (after Live Source Tamer)
- UI controls in InputsTab Mutes sub-tab
- Recalculated at 50Hz when position or sidelines parameters change

### Height Factor
- `inputHeightFactor` (0-100%) scales Z contribution in distance calculations
- Affects delay and level calculations, NOT angular calculations
- Useful for situations where vertical distance should have less effect

### Update Rate
- MainComponent calls `recalculateMatrix()` at 50Hz via timer
- Smoothing constant (0.1) for exponential interpolation to avoid Doppler artifacts
- Listener/speaker positions cached, recalculated only on parameter change

### DSP Parameters Used
| Parameter | Section | Purpose |
|-----------|---------|---------|
| `outputPositionX/Y/Z` | Output Position | Speaker location |
| `outputOrientation` | Output Position | Speaker facing direction |
| `outputPitch` | Output Position | Speaker vertical angle |
| `outputHparallax` | Output Options | Horizontal listener offset |
| `outputVparallax` | Output Options | Vertical listener offset |
| `outputDistanceAttenPercent` | Output Options | Distance attenuation scaling |
| `outputHFattenuation` | Output Options | HF loss per meter (dB/m) |
| `outputAngleOn` | Output Options | Rear cone angle (full) |
| `outputAngleOff` | Output Options | Front cone angle (muted) |
| `inputPositionX/Y/Z` | Input Position | Source location |
| `inputAttenuation` | Input Attenuation | Base attenuation (dB) |
| `inputDistanceAttenuation` | Input Attenuation | Distance attenuation factor |
| `inputHeightFactor` | Input Position | Z scaling (0-100%) |
| `inputMutes` | Input Options | Comma-separated muted outputs |
| `inputCommonAtten` | Input Attenuation | Common attenuation % (see below) |

### Common Attenuation
Prevents upstage sources from losing too much overall level by lifting all attenuations toward the minimum:
- **100%** = Keep full original attenuation (no lift applied)
- **0%** = Apply full lift (all outputs raised to match minimum attenuation)
- Formula: `adjustment = -minAttenuation * (1.0 - commonAttenFactor)`

### Flip X/Y/Z (Position Mirroring)
The flip feature mirrors input positions around the origin on selected axes, useful for symmetric setups.

**Implementation:**
- Applied BEFORE offset and LFO/AutomOtion in the position chain
- Position calculation order: Target → Speed Limit → Flip → Offset → LFO

**Parameters:**
| Parameter | Range | Description |
|-----------|-------|-------------|
| `inputFlipX` | 0/1 | Mirror X around origin |
| `inputFlipY` | 0/1 | Mirror Y around origin |
| `inputFlipZ` | 0/1 | Mirror Z around origin |

**Behavior:**
- Flip negates the position component (e.g., X=-2.5 becomes X=2.5)
- Keyboard nudge acts in opposite direction when flip is engaged
- Map grey dot shows flipped position with all transformations
- Affects WFSCalculationEngine, MapTab, InputsTab joystick/slider controls

### Input Speed Limiter
The InputSpeedLimiter provides smooth speed-limited movement for input positions with tanh-based acceleration/deceleration.

**Core File:** `Source/DSP/InputSpeedLimiter.h`

**Position Chain:**
```
Target Position (from OSC/UI/Tracking)
    ↓
InputSpeedLimiter.process(0.02f)  ← Speed limiting (50Hz)
    ↓
Flip Transformation
    ↓
Regular Offset (inputOffsetX/Y/Z)
    ↓
LFO + AutomOtion Offsets
    ↓
WFSCalculationEngine (distance calculations)
```

**Algorithm (Tanh Smoothing):**
```cpp
float normalizedDist = distance / (maxStep * 5.0f);  // 5.0 = tuning factor
float speedScale = std::tanh(normalizedDist) / normalizedDist;
float step = std::min(distance, maxStep * speedScale);
```

This provides:
- Natural acceleration from rest (soft start)
- Full speed when far from target
- Gradual deceleration when approaching target (soft stop)

**Parameters:**
| Parameter | Range | Description |
|-----------|-------|-------------|
| `inputMaxSpeedActive` | 0/1 | Enable speed limiting for this input |
| `inputMaxSpeed` | 0.01-20 m/s | Maximum movement speed |
| `inputPathModeActive` | 0/1 | Enable path mode (follow drawn path) |

**Path Mode:**
When enabled, the speed-limited marker follows the path drawn during drag operations instead of moving in a straight line to the target.

- **Waypoint capture:** During map drag, waypoints are captured at ~50Hz into a circular buffer (max 100 waypoints)
- **Immediate following:** The marker starts following waypoints immediately during drag, not after release
- **Constant speed:** Movement between waypoints uses constant speed (no deceleration) for smooth path following
- **Final deceleration:** Tanh smoothing is only applied when approaching the final target after all waypoints are consumed
- **Per-input toggle:** Path mode can be enabled/disabled independently for each input

**OSC Addresses:**
- `/wfs/input/maxSpeedActive <ID> <value>` (0/1)
- `/wfs/input/maxSpeed <ID> <value>` (m/s)
- `/wfs/input/pathModeActive <ID> <value>` (0/1)

**Integration:**
- Speed limiter runs at 50Hz in MainComponent timer callback
- Speed-limited positions passed to WFSCalculationEngine
- MapTab displays markers at speed-limited positions
- Hit-testing uses speed-limited positions for consistent interaction
- Waypoint callbacks wired from MapTab to InputSpeedLimiter in MainComponent

### LFO Processor (Position Modulation)
The LFOProcessor generates periodic position offsets for each input channel, creating automated movement effects.

**Architecture:**
- Runs at 50Hz (called from MainComponent timer callback)
- Per-input LFO state with independent parameters
- Outputs X/Y/Z position offsets added to base position + offset
- 500ms fade in/out when activating/deactivating

**Waveform Shapes** (all output -1 to +1, scaled by amplitude):
| Shape | Formula |
|-------|---------|
| Off | 0 |
| Sine | `-cos(2π × ramp)` |
| Square | `ramp < 0.5 ? -1 : 1` |
| Sawtooth | `2 × ramp - 1` |
| Triangle | `ramp < 0.5 ? 4×ramp - 1 : 3 - 4×ramp` |
| Keystone | Plateau at ends, ramp in middle (0.25 threshold) |
| Log | `2 × log10(20×ramp + 1) - 1` (normalized) |
| Exp | `pow(3, ramp×2) - 1` (normalized) |
| Random | Smooth ramp to new random target each period |

**Per-Axis Control:**
- Each axis (X/Y/Z) has independent: Shape, Rate multiplier, Amplitude, Phase offset
- Rate multiplier applies to main ramp: `axisRamp = mainRamp × rate + phase`
- Random shape picks new target independently when axis ramp wraps

**Parameters:**
| Parameter | Range | Description |
|-----------|-------|-------------|
| `inputLFOactive` | 0/1 | Enable LFO for this input |
| `inputLFOperiod` | 0.01-120s | Base period for one cycle |
| `inputLFOphase` | 0-360° | Global phase offset |
| `inputLFOshapeX/Y/Z` | 0-8 | Waveform shape per axis |
| `inputLFOrateX/Y/Z` | 0.01-100 | Rate multiplier per axis |
| `inputLFOamplitudeX/Y/Z` | 0-50m | Peak amplitude per axis |
| `inputLFOphaseX/Y/Z` | 0-360° | Phase offset per axis |

**UI Indicators (InputsTab LFO sub-tab):**
- Progress dial: Shows main ramp position (0→1) as rotating dot
- Output sliders: Bidirectional (-1 to +1) showing current normalized output per axis

### AutomOtion Processor (Programmed Position Movement)
The AutomOtionProcessor provides programmed point-to-point movement for input channel positions, enabling automated source movements.

**Core File:** `Source/DSP/AutomOtionProcessor.h`

**Movement Features:**
- **Coordinates**: Absolute (move to target) or Relative (offset from current)
- **Duration**: 0.1s to 3600s (1 hour)
- **Speed Profile**: 0% = constant speed, 100% = bell curve (gradual acceleration/deceleration)
- **Curve**: -100% to +100% bends path perpendicular to direction in XY plane (Z follows linear)
- **Stay/Return**: At end, stay at destination or return to origin

**Speed Profile Algorithm:**
```cpp
// Bell curve using cosine: (1 - cos(π * t)) / 2
float bellProgress = (1.0f - std::cos(PI * linearProgress)) / 2.0f;
float blend = speedProfile / 100.0f;
return linearProgress * (1.0f - blend) + bellProgress * blend;
```

**Curved Path Calculation:**
```cpp
// Perpendicular vector in XY plane (rotated 90° counter-clockwise)
perpX = -dy / pathLength2D;
perpY = dx / pathLength2D;

// Sine arc peaks at midpoint
arcFactor = sin(π * progress);
displacement = pathLength2D * 0.5f * |curve/100| * arcFactor * sign(curve);
```

**Audio Triggering:**
- **Trigger Mode**: Manual or Audio trigger toggle
- **Trigger Detection**: Short peak hold (5ms release) compared to threshold
- **Reset Detection**: RMS averaging (200ms window) compared to reset threshold
- **One-Shot Behavior**: Movement completes fully before any retriggering
- **Rearm Logic**: After movement completes + RMS drops below reset → rearm
- **Return Behavior**: Instant snap back (no animated return) when audio-triggered
- **Manual Override**: Start button works in audio mode, but waits for current movement

**Trigger State Machine:**
```
Stopped + Audio Mode:
  - If waitingForRearm && RMS < resetThreshold → triggerArmed = true
  - If triggerArmed && shortPeak > triggerThreshold → startMotion()

Playing → Completed:
  - If Return mode (audio trigger): instant snap back, waitingForRearm = true
  - If Return mode (manual): animated return phase
  - If Stay mode: keep final position, set waitingForRearm if audio mode
```

**Constraint**: AutomOtion only works when tracking is disabled for the input.

**Global Controls:**
- Stop All / Pause All / Resume All buttons in InputsTab AutomOtion sub-tab

**Parameters:**
| Parameter | Range | Description |
|-----------|-------|-------------|
| `inputOtomoX/Y/Z` | -50 to 50m | Destination coordinates |
| `inputOtomoAbsoluteRelative` | 0/1 | 0=Absolute, 1=Relative |
| `inputOtomoStayReturn` | 0/1 | 0=Stay, 1=Return |
| `inputOtomoDuration` | 0.1-3600s | Movement duration |
| `inputOtomoSpeedProfile` | 0-100% | Speed curve (0=linear, 100=bell) |
| `inputOtomoCurve` | -100 to +100 | Path bend (-=left, +=right) |
| `inputOtomoTrigger` | 0/1 | 0=Manual, 1=Audio |
| `inputOtomoThreshold` | -92 to 0 dB | Audio trigger threshold |
| `inputOtomoReset` | -92 to 0 dB | Rearm threshold (RMS) |

**Integration:**
- Processor runs at 50Hz in MainComponent timer callback
- Audio levels collected from LiveSourceLevelDetector (shortPeak 5ms, RMS 200ms)
- Offsets combined with LFO offsets in WFSCalculationEngine
- Map visualization updates automatically via offset system

### Gyrophone (HF Directivity Rotation)
The Gyrophone feature rotates the HF directivity pattern like a Leslie speaker rotating horn, creating a "brightness swirl" effect.

**Mechanism:**
- Adds rotation offset to input's HF directivity calculation
- Does NOT affect the UI rotation dial display
- Full rotation (2π radians) completes over one LFO period
- Direction: -1 = anti-clockwise, 0 = off, 1 = clockwise

**Implementation:**
```cpp
// In LFOProcessor
if (gyrophone != 0) {
    gyrophoneOffsetRad = gyrophone * ramp * 2π * fadeLevel;
}

// In WFSCalculationEngine (during HF directivity calculation)
rotationRad = inputRotation + gyrophoneOffset;  // Combined rotation for HF calc
```

**Parameter:**
| Parameter | Values | Description |
|-----------|--------|-------------|
| `inputLFOgyrophone` | -1, 0, 1 | Rotation direction (anti-CW, off, CW) |

### Smooth Transition Ramping
WFSCalculationEngine implements smooth transitions for parameter changes that would otherwise cause audible artifacts:

**Minimal Latency Mode Toggle** (`inputMinimalLatency`):
- 1 second linear ramp when switching between Acoustic Precedence and Minimal Latency modes
- Compensates for the instantaneous delay offset change
- Prevents audible delay jumps

**Common Attenuation Changes** (`inputCommonAtten`):
- Ramp time proportional to change magnitude: 1% change = 0.01s, 100% change = 1.0s
- Compensates for the instantaneous level change
- Prevents audible level jumps

Both ramps are updated at 50Hz in `updateDelayModeRamps()` and applied during matrix calculation.

### Live Source Tamer
The Live Source Tamer (LS) reduces speaker levels near live sources (microphones) to prevent feedback and improve clarity. It combines fixed attenuation based on distance/shape with dynamic compression based on audio input levels.

**Core Files:**
- **LiveSourceLevelDetector.h** - Per-input audio level detection (peak envelope + RMS)
- **LiveSourceTamerEngine.h** - Control-rate engine calculating per-speaker LS gains

**Three Attenuation Components:**
1. **Fixed Attenuation** - Distance-based, determined by radius and shape curve
2. **Peak Compressor** - Fast envelope follower (1 sample attack, 100ms release) for transients
3. **Slow Compressor** - RMS averaging (200ms window) for sustained levels

**Activation Conditions:**
- `inputLSactive` must be true (master enable per input)
- Output must be within `inputLSradius` of input position
- `outputLSattenEnable` must be non-zero (per-output bypass)

**Shape Curves** (attenuation profile from center to edge):
| Shape | Formula | Behavior |
|-------|---------|----------|
| Linear | `1 - t` | Constant rate of change |
| Log | `1 - log10(1 + 9*t)` | Gradual near center, steep at edge |
| Square | `1 - t²` | Gradual near center, steep at edge |
| Sine | `0.5 + 0.5*cos(t*π)` | S-curve, smooth at both ends |

Where `t` = normalized distance (0 at center, 1 at edge).

**Gain Calculation:**
```cpp
shapeFactor = calculateShapeFactor(normalizedDistance, shape);  // 1.0 at center, 0.0 at edge
combinedAtten = fixedAttenLinear * peakGR * slowGR;
targetGain = 1.0 - shapeFactor * (1.0 - combinedAtten);
```

**Smooth Enable/Disable Transition:**
- 500ms ramp when enabling or disabling LS
- `rampProgress` goes 0→1 when enabling, 1→0 when disabling
- Final gain: `lsGain = 1.0 + ramp * (targetGain - 1.0)`
- Prevents audible clicks/jumps when toggling

**Level Detection (LiveSourceLevelDetector):**
- **Peak path**: `abs → envelope(1 sample attack, 100ms release) → dB → gainCalc → smooth(2ms/2ms)`
- **Slow path**: `RMS(200ms window) → dB → gainCalc → smooth(2ms/20ms)`
- Soft knee compression with 20dB knee width
- Thread-safe via `std::atomic` for cross-thread communication

**Parameters:**
| Parameter | Range | Description |
|-----------|-------|-------------|
| `inputLSactive` | 0/1 | Enable LS for this input |
| `inputLSradius` | 0.1-50m | Effect radius from input |
| `inputLSshape` | 0-3 | Shape curve (linear/log/square/sine) |
| `inputLSattenuation` | -60-0 dB | Fixed attenuation at center |
| `inputLSpeakThreshold` | -60-0 dB | Peak compressor threshold |
| `inputLSpeakRatio` | 1-20 | Peak compressor ratio |
| `inputLSslowThreshold` | -60-0 dB | Slow compressor threshold |
| `inputLSslowRatio` | 1-20 | Slow compressor ratio |
| `outputLSattenEnable` | 0/1 | Per-output LS bypass |

**Integration:**
- Level detection runs on audio thread (per-sample processing)
- LS engine runs at 50Hz in MainComponent timer callback
- LS gains applied in WFSCalculationEngine during level calculation
- Visualization updates show combined attenuation in InputVisualisationComponent

### Floor Reflections
Floor Reflections (FR) simulate sound bouncing off the floor plane (z=0), creating a secondary reflected signal for each input/output pair.

**Core Files:**
- **WFSBiquadFilter.h** - Generic biquad filter for low-cut and high-shelf
- **WFSCalculationEngine.h/cpp** - FR matrix calculation (delay, level, HF)
- **InputBufferProcessor.h** - Per-input FR signal processing
- **OutputBufferProcessor.h** - Per-output FR signal processing

**Signal Flow:**
```
Input Audio
    |
    +---> Direct Path (existing)
    |         +--> delay --> HF air absorption filter --> level --> Sum to output
    |
    +---> FR Path (only if inputFRactive && z>0 && outputFRenable)
              +--> FR Low-Cut Filter (per-input, removes rumble)
              +--> FR High-Shelf Filter (per-input, simulates floor absorption)
              +--> frDelay + diffusionJitter --> FR HF filter --> frLevel --> Sum to output
```

**Reflected Position Calculation:**
```cpp
// Mirror source across floor plane (z=0)
Position reflected = { inputPos.x, inputPos.y, -inputPos.z };
```
When z ≤ 0, no reflection is generated (source is at or below floor).

**FR Delay Calculation:**
```cpp
float reflectedToListener = distance3D(reflectedPos, listenerPos);
float speakerToListener = distance3D(speakerPos, listenerPos);
float reflectedDelayMs = (reflectedToListener - speakerToListener) / speedOfSound * 1000.0f;
float frExtraDelayMs = reflectedDelayMs - directDelayMs;  // Extra delay beyond direct
```

**FR Level Calculation:**
```cpp
float reflectedToSpeaker = distance3D(reflectedPos, speakerPos);
float directDistance = distance3D(inputPos, speakerPos);
float distanceRatio = reflectedToSpeaker / directDistance;
float distanceAttenDb = -20.0f * log10(distanceRatio);  // Inverse square law
float totalFRAttenDb = inputFRattenuation + distanceAttenDb;
float frLevel = pow(10.0f, totalFRAttenDb / 20.0f) * angularAttenuation;
```

**FR HF Attenuation:**
FR signal passes through BOTH:
1. FR-specific filters (low-cut, high-shelf) - per input, shared across outputs
2. Existing air absorption filter with extra attenuation for longer path

**Time-Varying Diffusion (Floor Roughness Simulation):**
```cpp
// Per-output jitter state with smoothing (~50Hz update rate)
float maxJitterMs = diffusionPercent * 0.05f;  // 5ms max at 100%
float noiseTarget = random(-maxJitter, +maxJitter);
noiseState += (noiseTarget - noiseState) * 0.05f;  // Smooth transition
totalFRDelay = directDelay + frExtraDelay + noiseState;
```

**Activation Conditions:**
- `inputFRactive` = 1 (per-input enable)
- Source z > 0 (above floor)
- `outputFRenable` = 1 (per-output enable)
- Direct signal level > 0 (not muted)

**Parameters:**
| Parameter | Range | Description |
|-----------|-------|-------------|
| `inputFRactive` | 0/1 | Enable FR for this input |
| `inputFRattenuation` | -60 to 0 dB | Base FR attenuation |
| `inputFRlowCutActive` | 0/1 | Enable low-cut filter |
| `inputFRlowCutFreq` | 20-20000 Hz | Low-cut frequency |
| `inputFRhighShelfActive` | 0/1 | Enable high-shelf filter |
| `inputFRhighShelfFreq` | 20-20000 Hz | High-shelf frequency |
| `inputFRhighShelfGain` | -24 to 0 dB | High-shelf attenuation |
| `inputFRhighShelfSlope` | 0.1-0.9 | High-shelf transition slope |
| `inputFRdiffusion` | 0-100% | Floor roughness (jitter amount) |
| `outputFRenable` | 0/1 | Per-output FR bypass |

**Integration:**
- FR matrices (frDelayTimesMs, frLevels, frHFAttenuationDb) calculated at 50Hz
- FR filter parameters updated from ValueTree at 50Hz
- Both InputBufferAlgorithm and OutputBufferAlgorithm support FR
- FR and direct signals summed per output

### Level Metering System
The Level Metering System provides real-time audio level visualization for input and output channels, with thread performance monitoring.

**Core Files:**
- **LevelMeteringManager.h** - Central manager for audio level collection and retrieval
- **LevelMeterWindow.h** - Floating window UI with meter bars and controls

**Features:**
- **Input meters** - Peak + RMS levels with peak hold for each input channel
- **Output meters** - Peak + RMS levels with peak hold for each output channel
- **Thread performance bars** - CPU usage percentage per processing thread
- **Solo buttons** - "S" toggle under each input meter (linked to binaural solo)
- **Visual Solo dropdown** - Select single input for contribution visualization
- **Clear Solo button** - Clear all binaural solo states

**Level Measurement:**
```cpp
struct LevelInfo {
    float peakDb;    // Current peak level in dB
    float rmsDb;     // Current RMS level in dB
};
```

**Thread Performance:**
```cpp
struct ThreadPerformance {
    float cpuPercent;           // CPU usage as percentage
    float microsecondsPerBlock; // Processing time per audio block
};
```

**Visual Solo Linking (Single Mode):**
- In Single mode, clicking "S" button also sets Visual Solo dropdown
- In Multi mode, Visual Solo dropdown remains independent
- Output meter highlighting only occurs in Single mode when Visual Solo is active

**Processing Algorithm Modes:**
| Mode | Threads | Performance Display |
|------|---------|---------------------|
| InputBuffer | Per-input threads | Bars under input meters |
| OutputBuffer | Per-output threads | Bars under output meters |

**Meter Window Access:**
- Button in SystemConfigTab: "Level Meter" button
- Metering enabled when window is visible, disabled when closed (saves CPU)

**UI Components:**
- **LevelMeterBar** - Vertical bar showing peak (line) + RMS (filled) with peak hold
- **ThreadPerformanceBar** - Horizontal bar showing CPU % with color coding
- **LevelMeterWindowContent** - Main content with meters, labels, and controls
- **LevelMeterWindow** - DocumentWindow container with dark mode support

**Color Coding:**
| Level | Color |
|-------|-------|
| < -12 dB | Green |
| -12 to -6 dB | Yellow |
| > -6 dB | Red |

| CPU % | Color |
|-------|-------|
| < 50% | Green |
| 50-80% | Yellow |
| > 80% | Red |

### Binaural Solo Monitoring
Binaural Solo Monitoring renders soloed inputs through a virtual speaker pair for headphone monitoring, simulating the spatial position of sources.

**Studio Preview Mode:**
When no inputs are soloed, all inputs are rendered through the binaural spatialization, providing a full spatial mix preview on headphones. This enables pre-production work without a full WFS speaker array (home studio, hotel, train, airport).

**Core Files:**
- **BinauralCalculationEngine.h** - Calculates delay, level, and HF for virtual speaker pair
- **BinauralProcessor.h** - Audio processor with delay lines and HF filters

**Virtual Speaker Configuration:**
| Parameter | Value | Description |
|-----------|-------|-------------|
| Speaker spacing | 20cm | ±10cm from listener center |
| Speaker angles | 45° | Left/right from front-facing axis |
| On angle | 135° | Full coverage zone (behind) |
| Off angle | 30° | Mute zone (in front) |
| HF attenuation | -0.3 dB/m | Air absorption simulation |

**User Controls (SystemConfigTab):**
| Control | Range | Default | Description |
|---------|-------|---------|-------------|
| Solo Mode | Single/Multi | Single | Single = one input, Multi = multiple inputs |
| Output Channel | Off, 1-2, 3-4, ... 63-64 | Off | First channel of stereo output pair |
| Listener Distance | 0.5-10m | 2m | Distance from origin |
| Listener Angle | -180° to +180° | 0° | Horizontal rotation |
| Binaural Level | -40 to 0 dB | 0 dB | Overall level offset |
| Binaural Delay | 0-100ms | 0ms | Additional delay offset |

**Solo Mode Behavior:**
- **Single mode**: Only one input can be soloed at a time; clicking "S" clears others
- **Multi mode**: Multiple inputs can be soloed simultaneously
- **No solos**: Full spatial mix rendered to binaural output (studio preview mode)

**Binaural Calculation:**
```cpp
struct BinauralOutput {
    float delayMs;          // Propagation delay
    float level;            // Linear attenuation (0-1)
    float hfAttenuationDb;  // High-frequency loss
};
struct BinauralPair {
    BinauralOutput left;
    BinauralOutput right;
};
```

**Delay Calculation:**
```cpp
float distanceToSpeaker = distance3D(inputPos, virtualSpeakerPos);
float delayMs = (distanceToSpeaker / 343.0f) * 1000.0f + binauralDelay;
```

**Angular Attenuation (Keystone Pattern):**
```cpp
// Calculate angle from speaker's rear axis to input
float angleFromRear = acos(dot(rearAxis, toInput));
if (angle <= onAngle) return 1.0f;    // Full signal
if (angle >= offAngle) return 0.0f;   // Muted
return (offAngle - angle) / (offAngle - onAngle);  // Linear transition
```

**HF Attenuation:**
```cpp
float hfDb = distanceToSpeaker * -0.3f;  // -0.3 dB/m
```

**BinauralProcessor Signal Flow:**
```
Soloed Input Audio
    |
    +--> Delay Line (Left) --> HF Shelf Filter --> Level --> Sum to Left Output
    |
    +--> Delay Line (Right) --> HF Shelf Filter --> Level --> Sum to Right Output
```

**Parameters (ValueTree):**
| Parameter | Type | Description |
|-----------|------|-------------|
| `binauralSoloMode` | int | 0=Single, 1=Multi |
| `binauralOutputChannel` | int | First output channel (-1=disabled) |
| `binauralListenerDistance` | float | Listener distance in meters |
| `binauralListenerAngle` | int | Listener rotation in degrees |
| `binauralAttenuation` | float | Level offset in dB |
| `binauralDelay` | float | Delay offset in ms |
| `inputSoloStates` | string | Comma-separated 0/1 per input |

**API Methods (WFSValueTreeState):**
```cpp
bool isInputSoloed(int inputIndex);
void setInputSoloed(int inputIndex, bool soloed);
void clearAllSoloStates();
int getBinauralSoloMode();  // 0=Single, 1=Multi
```

**Integration:**
- BinauralCalculationEngine uses WFSCalculationEngine for input composite positions
- BinauralProcessor runs after main WFS processing in MainComponent audio callback
- Solo buttons in InputsTab, LevelMeterWindow linked to same ValueTree state
- Clear Solo button in SystemConfigTab, LevelMeterWindow

---

### Reverb Channel Calculations
WFSCalculationEngine handles two additional matrix paths for reverb:

**Input → Reverb Feed** (numInputs × numReverbs):
- Reverb feeds act like simplified outputs (spatial microphones)
- No parallax: `delayMs = inputToReverbFeed / speedOfSound * 1000.0f`
- Uses input's attenuation law (linear or inverse square)
- Receives common attenuation adjustment from outputs (but not included in minimum search)
- Muted when `inputMuteReverbSends = 1`

**Reverb Return → Output** (numReverbs × numOutputs):
- Reverb returns act like simplified inputs (ambient sources)
- Uses parallax: `delayMs = (returnToListener - speakerToListener) / speedOfSound * 1000.0f`
- Simple dB/m attenuation (no law switching)
- Has its own `reverbCommonAtten` parameter (same 0-100% interpretation)
- Per-output mutes via `reverbMutes` array

**Return Position**: `returnPos = feedPos + returnOffset`

### Reverb System (Complete — GUI, Parameters & DSP)
The reverb system is fully implemented across 4 sub-tabs with complete DSP processing, including 3 algorithm types, pre/post processing, and parallel per-node computation.

**Signal Flow:**
```
Input Audio → Feed Routing (WFSCalculationEngine)
  → Pre-EQ (per-channel, 4-band parametric)
  → Pre-Compressor (global, ReverbPreProcessor)
  → Algorithm (SDN/FDN/IR — parallel per-node via AudioParallelFor)
  → Post-EQ (global, 4-band parametric)
  → Post-Expander (global, sidechain-keyed, ReverbPostProcessor)
  → Return Routing → Output Mix
```

**ReverbEngine** owns the full chain: PreProcessor → Algorithm → PostProcessor. It runs on the audio thread, called from MainComponent's `getNextAudioBlock()`. Algorithm switching uses a fade-out → swap → fade-in crossfade (~50ms each) to avoid clicks.

**Algorithm Types:**
- **SDN (type 0)** — Scattering Delay Network: N*(N-1) inter-node delay paths with Householder scattering matrix. Geometry-based delays from node positions. Decay via per-path biquad filters (3-band RT60). Diffusion via allpass chains per node.
- **FDN (type 1)** — Feedback Delay Network: 16 delay lines per node with Walsh-Hadamard feedback mixing. Co-prime delay lengths scaled by multiplier. 3-band decay filtering (low/mid/high RT60).
- **IR (type 2)** — Impulse Response convolution via juce::dsp::Convolution per node. Supports file loading, trim, and per-node enable.

**Parallelization (AudioParallelFor):**
All 3 algorithms support parallel per-node processing via a fork-join thread pool:
- Worker count: `min(hardware_concurrency() - 2, numNodes - 1)`, max 7
- FDN/IR: trivially parallel (independent per-node processing)
- SDN: snapshot-based — `readBasePos` frozen at block start decouples readers from writers; minimum delay ~70 samples provides natural temporal separation. At most ~5ms extra latency on short paths.
- Calling thread participates in work (no idle main thread)

**Pre-Processing (ReverbPreProcessor):**
- 4-band parametric EQ per reverb channel (ReverbBiquadFilter: low-shelf, 2× peak, high-shelf)
- Global pre-compressor: RMS detection, variable knee, configurable attack/release/ratio/threshold

**Post-Processing (ReverbPostProcessor):**
- 4-band parametric EQ global (same filter topology as pre-EQ)
- Global post-expander: sidechain-keyed from pre-algorithm signal, variable knee, downward expansion

**Algorithm Sub-Tab (global parameters):**
- Algorithm selector: SDN (0), FDN (1), IR (2) — `reverbAlgoType`
- Decay section (SDN/FDN): RT60, RT60 Low/High multipliers, crossover frequencies, diffusion
- SDN-specific: inter-node delay scale
- FDN-specific: delay line size multiplier
- IR-specific: file load, trim, length, per-node toggle
- Wet level (always visible)

**Pre-Processing Sub-Tab (per-channel EQ + global compressor):**
- 4-band parametric EQ per reverb channel (reverbPreEQ* parameters)
- Pre-compressor (global): bypass, threshold, ratio, attack, release — WfsBasicDial controls

**Post-Processing Sub-Tab (global EQ + global expander):**
- 4-band parametric EQ global (reverbPostEQ* parameters)
- Post-expander (global): bypass, threshold, ratio, attack, release — WfsBasicDial controls

**ValueTree Structure:**
```
Reverbs
├── Reverb (id=1)          ← per-channel (Channel, Position, Feed, EQ, Return)
├── Reverb (id=2)          ← per-channel
├── ReverbAlgorithm        ← global algorithm params
├── ReverbPreComp          ← global pre-compressor params
├── ReverbPostEQ           ← global post-EQ params (with PostEQBand children)
└── ReverbPostExp          ← global post-expander params
```

**Important:** `getReverbState(channelIndex)` iterates by type (`Reverb`) to find the correct channel, since global sections are interleaved as siblings.

**OSC Paths:**
- Per-channel: `/wfs/reverb/<param> <ID> <value>` (e.g., `/wfs/reverb/preEQfreq 1 2 1000`)
- Global: `/wfs/config/reverb/<param> <value>` (e.g., `/wfs/config/reverb/rt60 1.5`)

### Input Visualisation Component
Real-time display of DSP matrix values in InputsTab "Visualisation" sub-tab:
- **Row 1 (Yellow)**: Delay times (0-350ms)
- **Row 2 (Pink)**: HF attenuation (-24 to 0 dB)
- **Row 3 (Blue)**: Level attenuation (-60 to 0 dB)

Features:
- One vertical slider per output channel + reverb feed
- Updates at 50Hz from WFSCalculationEngine
- Hover tooltips showing "Output X: value unit"
- Gap between output and reverb sections
- Adjusts to user-configured channel counts

---

## Network System (Source/Network/)

### OSC Communication
- **OSCManager** - Central coordinator for all OSC communication
- **OSCConnection** - Individual target connection (UDP/TCP transmission)
- **OSCReceiverWithSenderIP** - Custom UDP receiver exposing sender IP
- **OSCTCPReceiver** - TCP receiver for reliable connections
- **OSCMessageRouter** - Address pattern parsing and routing
- **OSCRateLimiter** - Message throttling with coalescing
- **OSCSerializer** - Header-only OSC message/bundle serialization for TCP transmission
- **OSCParser** - Header-only OSC message/bundle parsing for TCP reception

### TCP Implementation
Full bidirectional TCP support for OSC communication:

**TCP Reception (OSCTCPReceiver):**
- Multi-threaded server using `juce::StreamingSocket`
- Length-prefix framing (4-byte big-endian size before each OSC packet)
- Supports up to 16 simultaneous client connections
- Thread-safe client management with `CriticalSection`
- Async message posting to UI thread

**TCP Transmission (OSCConnection):**
- Uses `juce::StreamingSocket` for outbound connections
- Length-prefix framing matching receiver implementation
- 5 second connection timeout
- Thread-safe via `sendLock` mutex
- Automatic status update to `Disconnected` on send failure

**Framing Protocol:**
```
[4 bytes: big-endian length][N bytes: OSC message/bundle data]
```

### Logging System
- **OSCLogger** - Ring buffer storage for network messages (1000 entries max)
- **LogEntry** - Extended struct with IP, port, transport, protocol, rejected status
- Logging disabled by default to avoid background overhead

### Protocols
- **OSC** - Standard Open Sound Control
- **Remote** - Remote input control protocol
- **ADM-OSC** - Audio Definition Model (placeholder)
- **OSCQuery** - Parameter discovery server
- **PSN** - PosiStageNet tracking protocol (UDP port 56565)
- **RTTrP** - Real-Time Tracking Protocol (UDP port 24220)

### Network Log Window Features
- Independent floating window (for second monitor)
- Master logging switch (disabled by default)
- Filter modes: Transport (UDP/TCP), Protocol, Client IP, Rejected
- Dynamic toggles based on protocols/IPs seen in log
- Color-coded rows by filter mode
- CSV export (filtered or all data)
- Auto-scroll with manual override

### Audio Interface and Patching Window (Source/gui/AudioInterfaceWindow.h/cpp)
Comprehensive audio device configuration and channel patching interface.

**Core Files:**
- **AudioInterfaceWindow.h/cpp** - Main window container with device settings
- **AudioPatchTab.h/cpp** - Input/Output patch tab components with test signal controls
- **PatchMatrixComponent.h/cpp** - Scrollable patch matrix with visual patching
- **TestSignalGenerator.h/cpp** - Test signal generation (pink noise, tone, sweep, dirac pulse)

**Window Structure:**
```
AudioInterfaceWindow
├── DeviceInfoBar - Shows current device/sample rate/buffer size
├── DeviceSettingsPanel - Device type, device, sample rate, buffer size controls
└── TabbedComponent
    ├── Input Patch Tab
    │   ├── Mode buttons (Scrolling | Patching)
    │   ├── Unpatch All button
    │   └── PatchMatrixComponent (WFS inputs → hardware inputs)
    └── Output Patch Tab
        ├── Mode buttons (Scrolling | Patching | Testing)
        ├── Unpatch All button
        ├── PatchMatrixComponent (WFS outputs → hardware outputs)
        └── TestSignalControlPanel (signal type, frequency, level, hold)
```

**Patch Matrix Modes:**
| Mode | Behavior |
|------|----------|
| Scrolling | Mouse drag scrolls viewport, no patching allowed |
| Patching | Click cell to patch/unpatch, drag diagonally for sequential 1:1 patches |
| Testing | Click header/row/cell to play test signal on that hardware channel |

**Visual Feedback:**
- **Patched cells** - Filled with WFS channel color
- **Unpatched channels** - Orange text in row headers
- **Active test channel** - Green highlight on header, row, and patch cell
- **Hover highlighting** - White overlay on hovered elements

**Test Signal Types:**
| Type | Description |
|------|-------------|
| Off | No signal |
| Pink Noise | Continuous pink noise with 500ms fade-in |
| Tone | Sine wave at selected frequency (20-20kHz) with 500ms fade-in |
| Sweep | Logarithmic 20Hz-20kHz sweep over 1s, 3s gap, repeats |
| Dirac Pulse | Single sample impulse, repeats every 1s |

**Test Signal Parameters:**
- **Level**: Default -40 dB, range -92 to 0 dB
- **Frequency**: 20-20000 Hz (for Tone mode)
- **Hold**: When enabled, signal continues until another channel clicked

**Safety Features:**
- Test signals stop when: switching tabs, closing window, changing mode, starting WFS processing
- Mode resets to Scrolling when exiting tab or window
- No auto-configuration when clicking in test mode (user must manually set signal type)
- 500ms fade-in for Pink Noise and Tone prevents loud bursts

**Patch Data Storage (ValueTree):**
```
AudioPatch
├── InputPatch
│   ├── rows (WFS input count)
│   ├── cols (max hardware inputs = 64)
│   └── patchData ("1,0,0,0;0,1,0,0;..." semicolon-separated rows)
└── OutputPatch
    ├── rows (WFS output count)
    ├── cols (max hardware outputs = 64)
    └── patchData (same format)
```

**Key Features:**
- Device settings automatically saved and restored on startup
- All available hardware channels automatically enabled when device changes
- Patch matrices persist with project save/load
- Thread-safe test signal injection (after WFS processing in audio callback)

---

## GUI Components (Source/gui/)

### Custom Sliders (Source/gui/sliders/)
- **WfsSliderBase** - Base class for all custom sliders, `handlePointer()` protected for derived classes
- **WfsStandardSlider** - Standard 0-1 range slider
- **WfsBidirectionalSlider** - Center-zero slider (-1 to 1 range)
- **WfsAutoCenterSlider** - Returns to center on mouse release, supports 50Hz timer-based polling via `onPositionPolled` callback
- **WfsWidthExpansionSlider** - For angle parameters
- **WfsRangeSlider** - Double-thumbed slider for min/max range selection (crossable thumbs)

### Other Custom Components
- **WfsJoystickComponent** - 2D XY control with auto-center
- **WfsBasicDial** - Rotary dial control
- **WfsRotationDial** - 360 degree rotation control
- **ChannelSelectorButton/Overlay** - Grid-based channel selector
- **EQDisplayComponent** - Interactive parametric EQ visualization

### Bidirectional Slider Usage
When using WfsBidirectionalSlider, formulas must account for -1 to 1 range:
- **Display**: `value = v * maxValue` (e.g., `v * 100.0f` for +/-100ms)
- **Set slider**: `v = value / maxValue` (e.g., `ms / 100.0f`)

---

## Color Scheme System (Source/gui/ColorScheme.h)

### Overview
Centralized theming system with three color schemes: Default (dark gray), OLED Black, and Light. Theme selection persists to XML configuration.

### Core Files
- **ColorScheme.h** - Palette struct, Manager singleton, theme definitions
- **WfsLookAndFeel.h** - Custom LookAndFeel for JUCE widget theming
- **MainComponent.cpp** - Theme change listener and repaint coordination

### Available Themes

| Theme | Background | Text | Description |
|-------|------------|------|-------------|
| Default | 0xFF1E1E1E | White | Dark gray theme (original) |
| OLED Black | 0xFF000000 | 0xFFE8E8E8 | Pure black for OLED displays |
| Light | 0xFFF5F5F5 | 0xFF212121 | Light theme for daytime use |

### Palette Colors

| Semantic Name | Purpose |
|---------------|---------|
| `background` | Main component backgrounds |
| `backgroundAlt` | Alternate/canvas backgrounds |
| `surfaceCard` | Card/panel surfaces |
| `chromeBackground` | Status bar, tab bar background |
| `chromeSurface` | Footer areas |
| `chromeDivider` | Separator lines |
| `textPrimary` | Primary text |
| `textSecondary` | Dimmed/secondary text |
| `textDisabled` | Disabled state text |
| `buttonNormal/Hover/Pressed` | Button states |
| `accentBlue/Red/Green` | Functional accent colors (same across themes) |
| `sliderTrackBg/sliderThumb` | Slider components |
| `listBackground/listSelection` | ListBox colors |
| `tabBackground/tabSelected` | Tab bar colors |

### Usage Pattern

**Accessing colors:**
```cpp
#include "ColorScheme.h"

// In paint() methods
g.fillAll(ColorScheme::get().background);
g.setColour(ColorScheme::get().textPrimary);
```

**Listening for theme changes:**
```cpp
class MyComponent : public ColorScheme::Manager::Listener
{
    MyComponent() {
        ColorScheme::Manager::getInstance().addListener(this);
    }

    void colorSchemeChanged() override {
        repaint();
    }
};
```

**Setting theme programmatically:**
```cpp
ColorScheme::Manager::getInstance().setTheme(1);  // 0=Default, 1=OLED, 2=Light
```

### WfsLookAndFeel Integration
Custom LookAndFeel that:
- Inherits from `juce::LookAndFeel_V4`
- Listens for color scheme changes
- Updates all JUCE widget colors (TextEditor, ComboBox, Slider, Label, etc.)
- Provides font methods for future multilingual support

**Setup in MainComponent:**
```cpp
wfsLookAndFeel = std::make_unique<WfsLookAndFeel>();
juce::LookAndFeel::setDefaultLookAndFeel(wfsLookAndFeel.get());
```

### Theme Persistence
Saved in XML configuration:
```xml
<Config>
  <ColorScheme>1</ColorScheme>  <!-- 0=Default, 1=OLED, 2=Light -->
</Config>
```

### UI Location
Theme selector in SystemConfigTab, third column under "UI" section header.

---

## Internationalization (i18n) System (Source/Localization/)

### Overview
The application supports multiple languages through a JSON-based localization system. All user-facing strings are externalized to translation files, enabling runtime language switching.

### Core Files
- **LocalizationManager.h/cpp** - Singleton manager for loading and retrieving localized strings
- **Resources/lang/*.json** - Translation files (one per language)

### Available Languages (9 total)
| Locale | Language | Native Name | File |
|--------|----------|-------------|------|
| en | English | English | en.json |
| fr | French | Français | fr.json |
| de | German | Deutsch | de.json |
| es | Spanish | Español | es.json |
| it | Italian | Italiano | it.json |
| pt | Portuguese | Português | pt.json |
| ja | Japanese | 日本語 | ja.json |
| zh | Chinese | 中文 | zh.json |
| ko | Korean | 한국어 | ko.json |

### LOC() Macro
Convenience macro for retrieving localized strings:
```cpp
#include "../Localization/LocalizationManager.h"

// Basic usage
label.setText(LOC("systemConfig.labels.showName"), juce::dontSendNotification);

// With parameter substitution
showStatus(LocalizationManager::getInstance().get(
    "systemConfig.messages.languageChanged",
    {{"language", selectedLanguage}}));
```

### JSON File Structure
Translation files use nested JSON with dot-notation keys:
```json
{
  "meta": {
    "language": "English",
    "locale": "en",
    "version": "1.0.0"
  },
  "common": {
    "ok": "OK",
    "cancel": "Cancel",
    "apply": "Apply"
  },
  "systemConfig": {
    "labels": {
      "showName": "Name:",
      "inputChannels": "Input Channels:"
    },
    "messages": {
      "configSaved": "Configuration saved.",
      "error": "Error: {error}"
    }
  }
}
```

### Key Sections in Translation Files
| Section | Purpose |
|---------|---------|
| `meta` | Language metadata (name, locale, version) |
| `common` | Shared strings (OK, Cancel, Yes, No, etc.) |
| `units` | Unit abbreviations (m, dB, ms, Hz, %) |
| `tabs` | Tab names |
| `statusBar` | Status bar labels |
| `systemConfig` | System Config tab strings |
| `inputs` | Inputs tab strings |
| `outputs` | Outputs tab strings |
| `reverbs` | Reverb tab strings |
| `network` | Network tab strings |
| `clusters` | Clusters tab strings |
| `map` | Map tab strings |
| `audioPatch` | Audio Interface window strings |
| `networkLog` | Network Log window strings |
| `arrayHelper` | Wizard of OutZ strings |
| `setAllInputs` | Set All Inputs window strings |
| `snapshotScope` | Snapshot Scope window strings |
| `eq` | EQ component strings |
| `accessibility` | Screen reader strings |

### Parameter Substitution
Strings can contain `{placeholder}` markers for dynamic values:
```cpp
// JSON: "languageChanged": "Language changed to: {language}"
LocalizationManager::getInstance().get(
    "systemConfig.messages.languageChanged",
    {{"language", "Français"}});
// Result: "Language changed to: Français"
```

### Language Selection
- **UI Location**: SystemConfigTab, third column under "UI" section, below Color Scheme
- **Dropdown**: Dynamically populated by scanning `Resources/lang/` directory
- **Display Names**: Native language names from `languageNames` map in SystemConfigTab
- **Persistence**: Saved to XML configuration, loaded on startup

### Adding New Strings
1. Add key to `Resources/lang/en.json` (English reference)
2. Add translations to all other language files
3. Use `LOC("section.subsection.key")` in code
4. For parameterized strings, use `LocalizationManager::getInstance().get()` with substitution map

### Implementation Notes
- Language files are loaded from `Resources/lang/` directory
- `getAvailableLanguages()` scans directory for `*.json` files
- Missing keys fall back to the key itself (visible as untranslated)
- Language change requires application restart for full effect (some UI elements cached at startup)

---

## Snapshot and Scope System (Source/Parameters/WFSFileManager.h, Source/gui/SnapshotScopeWindow.h)

### Overview
The snapshot system allows saving and recalling input channel configurations with precise control over which parameters and channels are included. Supports two scope modes: legacy section-level filtering and extended parameter-level, per-channel granularity.

### Core Files
- **WFSFileManager.h/cpp** - File I/O and scope data structures (`SnapshotScope`, `ExtendedSnapshotScope`, `ScopeItem`)
- **SnapshotScopeWindow.h** - UI for editing extended scope (`ScopeGridComponent`, `ScopeChannelHeader`, `SnapshotScopeContent`)

### Snapshot Storage
Snapshots are stored as XML files in the project folder structure:
```
project_folder/
├── snapshots/
│   ├── inputs/         # Input snapshots (*.xml)
│   └── outputs/        # Output snapshots (*.xml)
```

### Legacy SnapshotScope (Section-Level)
Simple boolean flags for entire parameter sections:

```cpp
struct SnapshotScope {
    bool includePosition = true;
    bool includeAttenuation = true;
    bool includeDirectivity = true;
    bool includeLiveSource = true;
    bool includeHackoustics = true;
    bool includeLFO = true;
    bool includeAutomOtion = true;
    bool includeMutes = true;
    juce::Array<int> channelIndices;  // Empty = all channels
};
```

### Extended SnapshotScope (Parameter-Level, Per-Channel)
Fine-grained control over individual parameters for each channel:

**Apply Modes:**
| Mode | Behavior |
|------|----------|
| **OnSave** | Filter parameters when saving snapshot (excluded params not stored) |
| **OnRecall** | Filter parameters when loading snapshot (excluded params not applied) |

**Data Structure:**
```cpp
struct ExtendedSnapshotScope {
    ApplyMode applyMode = ApplyMode::OnRecall;
    std::map<juce::String, bool> itemChannelStates;  // Key: "itemId_channelIndex"
};
```

### Scope Items
Parameters are grouped into logical items for easier management. Each scope item contains related parameters:

| Section | Item ID | Display Name | Parameters |
|---------|---------|--------------|------------|
| **Channel** | `inputAttenuation` | Attenuation | `inputAttenuation` |
| **Channel** | `inputDelay` | Delay/Latency | `inputDelayLatency`, `inputMinimalLatency` |
| **Position** | `position` | Position (XYZ) | `inputPositionX/Y/Z`, `inputCoordinateMode` |
| **Position** | `offset` | Offset (XYZ) | `inputOffsetX/Y/Z` |
| **Position** | `constraints` | Constraints | All constraint params |
| **Position** | `flip` | Flip (XYZ) | `inputFlipX/Y/Z` |
| **Position** | `cluster` | Cluster | `inputCluster` |
| **Position** | `tracking` | Tracking | `inputTrackingActive/ID/Smooth` |
| **Position** | `speedLimit` | Speed Limit | `inputMaxSpeedActive`, `inputMaxSpeed` |
| **Position** | `pathMode` | Path Mode | `inputPathModeActive` |
| **Position** | `heightFactor` | Height Factor | `inputHeightFactor` |
| **Attenuation** | `attenuationLaw` | Attenuation Law | Law, distance atten, ratio |
| **Attenuation** | `commonAtten` | Common Atten | `inputCommonAtten` |
| **Directivity** | `directivity` | Directivity | Directivity, rotation, tilt |
| **Directivity** | `hfShelf` | HF Shelf | `inputHFshelf` |
| **LiveSourceTamer** | `lsEnable` | Enable | `inputLSactive` |
| **LiveSourceTamer** | `lsRadiusShape` | Radius/Shape | Radius, shape params |
| **LiveSourceTamer** | `lsFixedAtten` | Fixed Atten | `inputLSattenuation` |
| **LiveSourceTamer** | `lsPeakComp` | Peak Comp | Threshold, ratio |
| **LiveSourceTamer** | `lsSlowComp` | Slow Comp | Threshold, ratio |
| **Hackoustics** | `frEnable` | Enable | `inputFRactive` |
| **Hackoustics** | `frAttenuation` | Attenuation | `inputFRattenuation` |
| **Hackoustics** | `frLowCut` | Low Cut | Active, frequency |
| **Hackoustics** | `frHighShelf` | High Shelf | All high-shelf params |
| **Hackoustics** | `frDiffusion` | Diffusion | `inputFRdiffusion` |
| **Hackoustics** | `reverbSends` | Reverb Sends | `inputMuteReverbSends` |
| **LFO** | `lfoEnable` | Enable/Period | Active, period, phase, gyrophone |
| **LFO** | `lfoX/Y/Z` | LFO X/Y/Z | Shape, rate, amplitude, phase per axis |
| **LFO** | `jitter` | Jitter | `inputJitter` |
| **AutomOtion** | `otomoDestination` | Destination | X/Y/Z target, absolute/relative |
| **AutomOtion** | `otomoMovement` | Movement | Stay/return, duration, curve, speed |
| **AutomOtion** | `otomoAudioTrigger` | Audio Trigger | Trigger mode, thresholds |
| **Mutes** | `mutes` | Mutes | `inputMutes`, `inputMuteMacro` |
| **Mutes** | `sidelines` | Sidelines | Active, fringe |
| **Mutes** | `arrayAttens` | Array Attens | `inputArrayAtten1-10` |

### Sections
Items are organized into 9 sections:
1. **Channel** - Basic input properties
2. **Position** - Location, offset, constraints, tracking
3. **Attenuation** - Distance attenuation settings
4. **Directivity** - Beam pattern and HF control
5. **LiveSourceTamer** - Feedback prevention settings
6. **Hackoustics** - Floor reflections settings
7. **LFO** - Position modulation oscillator
8. **AutomOtion** - Programmed movement
9. **Mutes** - Output muting and sidelines

### Scope Window UI Components

**ScopeGridComponent:**
- Scrollable grid with rows (scope items) and columns (channels)
- Section headers expandable/collapsible with triangle icon
- Cells show inclusion state: green (included), grey (excluded), striped (partial)
- Click cell to toggle single item/channel
- Click section header to toggle entire section
- Click item label to toggle item for all channels

**ScopeChannelHeader:**
- Fixed header row with channel numbers
- "ALL" button in corner toggles everything
- Click channel number to toggle entire channel

**Visual Feedback:**
| State | Appearance |
|-------|------------|
| All Included | Solid green fill |
| All Excluded | Grey fill with border |
| Partial | Green/grey diagonal stripes |

### API Usage

**Creating a snapshot with extended scope:**
```cpp
WFSFileManager::ExtendedSnapshotScope scope;
scope.initializeDefaults(numInputChannels);  // All items included
scope.applyMode = ExtendedSnapshotScope::ApplyMode::OnRecall;

// Exclude specific items for specific channels
scope.setIncluded("position", 0, false);     // Exclude position for ch 0
scope.setItemForAllChannels("lfoEnable", false, numChannels);  // Exclude LFO enable for all

fileManager.saveInputSnapshotWithExtendedScope("MySnapshot", scope);
```

**Loading a snapshot:**
```cpp
auto scope = fileManager.getExtendedSnapshotScope("MySnapshot");
fileManager.loadInputSnapshotWithExtendedScope("MySnapshot", scope);
```

**Querying inclusion:**
```cpp
bool included = scope.isIncluded("position", channelIndex);
bool paramIncluded = scope.isParameterIncluded(inputPositionX, channelIndex);
auto state = scope.getChannelState(channelIndex);  // AllIncluded/AllExcluded/Partial
```

### XML Format

**Snapshot file structure:**
```xml
<InputSnapshot version="1.0" name="MySnapshot">
  <ExtendedScope applyMode="1">  <!-- 0=OnSave, 1=OnRecall -->
    <Item id="position_0" included="0"/>
    <Item id="lfoEnable_0" included="0"/>
    <Item id="lfoEnable_1" included="0"/>
    <!-- Only stores excluded items (default is included) -->
  </ExtendedScope>
  <Inputs>
    <Input id="1">
      <Position x="1.5" y="2.0" z="0.5" coordinateMode="0"/>
      <!-- Parameters filtered by scope if applyMode=OnSave -->
    </Input>
    <!-- More inputs... -->
  </Inputs>
</InputSnapshot>
```

### QLab Export

The snapshot scope window offers a **Write to QLab** mode as an exclusive alternative to save/recall. When selected, storing a snapshot exports its in-scope parameters as QLab network cues instead of writing an XML file.

**How it works:**
- The scope window has three mutually exclusive radio options: *When Saving*, *When Recalling*, and *Write to QLab*
- *Write to QLab* is only available when a QLab connection is active (configured in Network tab)
- The OK button triggers the selected mode — either XML save/recall or QLab export, never both

**QLab cue structure:**
- A **Group cue** named "Snapshot \<name\>" in playlist mode (mode 6)
- Inside it, one **Network cue** per in-scope parameter/channel
- Each network cue sends an OSC message back to WFS to recall that parameter value
- Network cues are named descriptively: "Input \<id\> \<param name\> \<value\>\<unit\>" (e.g., "Input 1 Volume -6.0 dB")
- Compression ratios display as "1:\<value\>" (e.g., "Input 2 LS Ratio 1:4.0")

**Key files:**
- `Source/Network/QLabCueBuilder.h` — Builds `QLabCueSequence` (group + network cue messages)
- `Source/gui/SnapshotScopeWindow.h` — Scope window UI with QLab radio option
- `Source/gui/InputsTab.h` — `storeNewSnapshot()` and `updateSnapshot()` handle exclusive save/QLab logic
- `Source/Network/OSCManager.h` — `sendToQLab()` sends the cue sequence with unique-ID-based move commands

**OSC flow (sendToQLab):**
1. Send group cue messages to QLab
2. Query `/cue/selected/uniqueID` on reply port 53001 to get the group's UUID
3. For each network cue: send creation messages, query unique ID, then `/move/<uuid>` into the group
4. Uses 30ms delays between steps for QLab processing

---

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

---

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

---

## Map Tab (Source/gui/MapTab.h)
Interactive 2D visualization of the WFS spatial layout:

### Display Elements
- **Grid** - 1m grid lines (dark gray)
- **Stage bounds** - White rectangle (Box) or circle (Cylinder/Dome) showing stage dimensions
- **Origin marker** - White crosshairs at coordinate origin (0,0)
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

### Double-Tap Actions
- **Double-tap on input marker** - Clears position offsets (X, Y, Z) for that input
- **Double-tap on cluster barycenter** - Clears position offsets for all inputs in that cluster

### Long-Press Navigation (700-1200ms)
Long-press on any marker (without moving) navigates to the corresponding tab and selects the channel:
- **Input marker** - Opens Inputs tab, selects that input channel
- **Cluster barycenter** - Opens Clusters tab, selects that cluster
- **Output marker** - Opens Outputs tab, selects that output channel
- **Reverb marker** - Opens Reverb tab, selects that reverb channel

Requirements: Hold 700-1200ms with less than 5px movement. Holding longer than 1.2s cancels the action.

### Coordinate System
- Stage coordinates in meters, origin at (originWidth, originDepth)
- Screen Y-axis inverted (Y increases downward on screen, upward on stage)
- `stageToScreen()` / `screenToStage()` conversion functions

### Visual Styling
- HSL-based colors for markers (matching Android WFS Control app)
- Inputs: hue = (id * 360 / 32) % 360, saturation 0.9, lightness 0.6
- Arrays/Clusters: hue = (id * 360 / 10) % 360, saturation 0.7, lightness 0.7
- Selected input: Yellow highlight ring
- Locked input: Gray outer circle, red channel number
- LS radius: White circle with 10% opacity when active
- Height indicator: Small triangle when Z != 0

### Speaker Icon Design
Outputs displayed as trapezoid (keystone) shapes representing speakers:
- **Outer trapezoid** - Dark gray stroke, wide base at back, narrow tip pointing toward audience
- **Inner membrane triangle** - Color-filled, base aligned with trapezoid back corners
- **Dimensions** - Height 24px, back width 21px, front width 11px
- **Membrane** - Extends 55% from back toward front
- **Channel number** - Bold 12pt black text at triangle centroid
- **Colors** - Array color (HSL-based) when assigned, light gray when single (array 0)
- **Orientation** - 0 degrees points toward audience (up on screen)

### Map Visibility & Lock Controls
Per-channel controls in respective tabs:
- **InputsTab** - Lock button + visibility toggle per input
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

---

## Wizard of OutZ (Output Array Helper)
A wizard-style dialog for quickly configuring speaker array positions with preset acoustic defaults.

### Access
- Button in OutputsTab header (right side): "Wizard of OutZ..."

### 7 Array Presets
| Preset | LS | HF dB/m | Dist% | Hp | Vp | EQ |
|--------|-----|---------|-------|-----|-----|-----|
| Near Field Straight | ON | -0.4 | 100 | 2 | 0.5 | LC 80Hz |
| Near Field Curved | ON | -0.4 | 100 | 2 | 0.5 | LC 80Hz |
| Main Flown Straight | OFF | -0.2 | 100 | 10 | -4 | - |
| Sub Bass | OFF | 0 | 50/100* | 0 | 0 | HC 300Hz |
| Surround | OFF | -0.3 | 100 | 3 | -2 | - |
| Delay Line | OFF | -0.15 | 100 | 3 | -2 | - |
| Circle | OFF | -0.3 | 100 | 0 | 0 | - |

*Sub Bass: 50% distance attenuation when N <= 2, otherwise 100%

### Geometry Methods
- **Center + Spacing** - Define center point and speaker spacing
- **Endpoints** - Define start and end points for the array
- **Curved arrays** - Use sag parameter (negative = toward audience)
- **Circle arrays** - Radius, start angle, facing inward/outward
- **Surround pairs** - Width and Y range for left/right mirrored speakers

### Key Features
- **Live preview** - Auto-updates as parameters change, shows stage shape (Box/Cylinder/Dome)
- **Target section** - Select array assignment and starting output
- **Auto-advance** - After Apply, advances to next array and output position
- **Orientation convention** - 0° = facing back of stage (+Y), 180° = facing audience (-Y)
- **Circle preset** - Defaults to center at origin (0,0)

### Geometry Calculations (Source/Helpers/ArrayGeometryCalculator.cpp)
- `calculateStraightFromCenter()` - Straight line from center point
- `calculateStraightFromEndpoints()` - Straight line between two points
- `calculateCurvedArray()` - Quadratic Bezier curve with sag, auto-fanning toward audience
- `calculateCircleArray()` - Circular arrangement with inward/outward facing
- `calculateSurroundPairs()` - Left/right mirrored pairs

---

## Set All Inputs Window (Source/gui/SetAllInputsWindow.h)
Bulk parameter control window for applying changes to ALL input channels simultaneously.

### Access
- **Long-press button** (2 seconds) in InputsTab header: "Set all Inputs..."
- Progress indicator fills during long-press, triggers on release
- Cancels if pointer moves away from button before release

### Window Design
- **Red warning strip** at top with black bold text: "Changes will apply to ALL inputs"
- Full ColorScheme theming support
- 450×850 pixel window size

### Available Controls

| Control | Type | Action |
|---------|------|--------|
| Coordinate mode | ComboBox | Set XYZ / r θ Z / r θ φ for all inputs |
| Curvature only | ON/OFF buttons | Enable/disable minimal latency mode |
| Flip XYZ > OFF | Action button | Reset all flip toggles to OFF |
| Constraint positions | ON/OFF buttons | Enable/disable X/Y/Z/Distance constraints |
| Height factor | Dial (0-100%) | Set height factor for all inputs |
| All Log | Action button | Set distance attenuation to Log mode |
| All 1/d | Action button | Set distance attenuation to 1/d mode |
| dB/m dial | Dial (-6 to 0 dB/m) | Set Log attenuation rate (visible when Log) |
| ratio dial | Dial (0.1-10x) | Set 1/d ratio (visible when 1/d) |
| common dial | Dial (0-100%) | Set common attenuation for all inputs |
| Reset directivity | Action button | Reset directivity, rotation, tilt, HF shelf to defaults |
| Mute macros | ComboBox | MUTE ALL, UNMUTE ALL, INVERT, ODD/EVEN, per-array |
| Turn OFF Live source atten | Action button | Disable Live Source Tamer on all inputs |
| Sidelines | ON/OFF buttons | Enable/disable sidelines edge muting |
| Fringe dial | Dial (0.1-10m) | Set sidelines fringe distance |
| Turn OFF jitter & LFO | Action button | Disable jitter and LFO on all inputs |
| Floor Reflections | ON/OFF buttons | Enable/disable floor reflections |
| CLOSE WINDOW | Button | Close the window |

### Button Feedback
- Action buttons briefly flash green (200ms) when clicked to confirm the action was applied

### Mute Macros
Applies mute changes to all inputs using comma-separated string format (compatible with InputsTab):
- **MUTE ALL** - Mute all outputs for all inputs
- **UNMUTE ALL** - Clear all mutes for all inputs
- **INVERT MUTES** - Toggle mute state of each output
- **MUTE ODD/EVEN** - Mute odd or even numbered outputs
- **MUTE/UNMUTE ARRAY 1-10** - Mute/unmute outputs assigned to specific array

### Implementation Notes
- Located in InputsTab.h as `SetAllInputsLongPressButton` class
- Window instance managed by InputsTab as `std::unique_ptr<SetAllInputsWindow>`
- Uses `applyToAllInputs()` helper to iterate all input channels
- Attenuation law dial visibility syncs between popup and InputsTab

---

## EQ Display Component (Source/gui/EQDisplayComponent.h)
Reusable interactive parametric EQ visualization for OutputsTab (6 bands) and ReverbTab (4 bands).

### Visual Elements
- **Grid** - Logarithmic frequency scale (20Hz-20kHz), linear dB scale (+/-24dB)
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

// For Reverb Pre-Processing EQ (4 bands)
EQDisplayConfig::forReverbPreEQ()

// For Reverb Post-Processing EQ (4 bands, global)
EQDisplayConfig::forReverbPostEQ()
```

### Parameter Mapping
| Aspect | Output EQ | Reverb Pre-EQ | Reverb Post-EQ |
|--------|-----------|---------------|----------------|
| Shape ID | `eqShape` | `reverbPreEQshape` | `reverbPostEQshape` |
| Frequency ID | `eqFrequency` | `reverbPreEQfreq` | `reverbPostEQfreq` |
| Gain ID | `eqGain` | `reverbPreEQgain` | `reverbPostEQgain` |
| Q ID | `eqQ` | `reverbPreEQq` | `reverbPostEQq` |
| Q Range | 0.1-10.0 | 0.1-20.0 | 0.1-20.0 |

### Band Colors (Rainbow 8-color palette)
1. Red (#E74C3C), 2. Orange (#E67E22), 3. Yellow (#FFEB3B), 4. Green (#2ECC71),
5. Blue (#3498DB), 6. Purple (#9B59B6), 7. Teal (#1ABC9C), 8. Pink (#E91E63)

### Band Marker Behavior
- **Active bands**: Full color marker at gain/frequency intersection
- **OFF bands**: Darkened marker (60% darker) positioned at frequency on 0dB line
- **Selected band**: White selection ring, larger marker size (28px vs 20px)
- **Band number**: Bold black text centered in marker

### Biquad Calculations
Uses Audio EQ Cookbook formulas matching filterCalc.js:
- **Cut filters**: `alpha = sin(w0) / (2 * Q)`
- **Shelf filters**: `alpha = (sin(w0)/2) * sqrt((A + 1/A) * (1/Q - 1) + 2)`
- **Peak/Notch**: `alpha = sin(w0) / (2 * Q)`

---

## Keyboard Shortcuts
- **F1-F10** - Assign input/output to cluster/array 1-10
- **F11** - Assign to Single (no cluster)
- **Up/Down arrows** - Navigate channels
- **Tab** - Switch between tabs

---

## Parameter Defaults (Source/Parameters/)
- **WFSParameterIDs.h** - Parameter identifier definitions
- **WFSParameterDefaults.h** - Default values, min/max ranges
- **WFSValueTreeState.cpp** - ValueTree structure and creation

### Reverb EQ Default Frequencies
Band 1: 200 Hz, Band 2: 800 Hz, Band 3: 2000 Hz, Band 4: 5000 Hz

---

## Key Files
- `Source/MainComponent.h/cpp` - Application entry point, keyboard handling, 50Hz DSP timer
- `Source/WfsParameters.h/cpp` - Parameter definitions
- `Source/Parameters/WFSValueTreeState.h/cpp` - State management
- `Source/DSP/WFSCalculationEngine.h/cpp` - WFS delay/level/HF + reverb matrix calculations
- `Source/DSP/LFOProcessor.h` - Position/rotation modulation oscillator
- `Source/DSP/AutomOtionProcessor.h` - Programmed position movement with audio triggering
- `Source/DSP/InputSpeedLimiter.h` - Speed-limited position interpolation with tanh smoothing
- `Source/DSP/WFSHighShelfFilter.h` - HF air absorption biquad filter
- `Source/DSP/WFSBiquadFilter.h` - Generic biquad filter (low-cut, high-shelf for FR)
- `Source/DSP/InputBufferProcessor.h` - Per-input threaded audio processor (with FR)
- `Source/DSP/OutputBufferProcessor.h` - Per-output threaded audio processor (with FR)
- `Source/DSP/LiveSourceLevelDetector.h` - Per-input audio level detection (peak, short peak, RMS)
- `Source/DSP/LiveSourceTamerEngine.h` - Live Source Tamer gain calculation
- `Source/DSP/LevelMeteringManager.h` - Audio level metering with thread performance
- `Source/DSP/BinauralCalculationEngine.h` - Binaural solo delay/level/HF calculation
- `Source/DSP/BinauralProcessor.h` - Binaural solo rendering with delay lines and HF filters
- `Source/Network/OSCManager.h/cpp` - Network coordination
- `Source/Network/OSCConnection.h/cpp` - Target connection with UDP/TCP transmission
- `Source/Network/OSCTCPReceiver.h/cpp` - TCP server for incoming connections
- `Source/Network/OSCSerializer.h` - OSC message/bundle serialization for TCP
- `Source/Network/OSCParser.h` - OSC message/bundle parsing for TCP
- `Source/Network/OSCLogger.h/cpp` - Message logging
- `Source/gui/InputsTab.h` - Input channel controls with joystick
- `Source/gui/SetAllInputsWindow.h` - Bulk parameter changes for all inputs
- `Source/gui/InputVisualisationComponent.h` - DSP matrix visualization sliders
- `Source/gui/ReverbTab.h` - Reverb settings with 4 sub-tabs (Channel Params, Pre-Processing, Algorithm, Post-Processing)
- `Source/gui/NetworkLogWindow.h/cpp` - Log window UI
- `Source/gui/OutputArrayHelperWindow.h/cpp` - Wizard of OutZ window
- `Source/gui/LevelMeterWindow.h` - Level metering floating window with solo buttons
- `Source/gui/ColorScheme.h` - Centralized color scheme system with 3 themes
- `Source/gui/WfsLookAndFeel.h` - Custom LookAndFeel for widget theming
- `Source/gui/sliders/WfsRangeSlider.h` - Double-thumbed range slider for distance constraints
- `Source/gui/SnapshotScopeWindow.h` - Extended scope editing UI for snapshots
- `Source/Helpers/ArrayGeometryCalculator.h/cpp` - Speaker array geometry calculations

---

## Build Notes

### Prerequisites & Cloning
```bash
git clone --recurse-submodules https://github.com/pob31/WFS-DIY.git
```
Or if already cloned:
```bash
git submodule update --init --recursive
```

**Dependencies (via submodules in `ThirdParty/`):**
- **JUCE 8.0.12** — `ThirdParty/JUCE` (pinned to tag 8.0.12)
- **ASIO SDK** — `ThirdParty/ASIOSDK` (from github.com/audiosdk/asio)
- **GPU Audio SDK** — `ThirdParty/GPUAudioSDK`

- Project file: WFS-DIY.jucer (open in Projucer to re-export build files)
- Builds: Visual Studio 2022, Xcode, Linux Makefile

### Build Commands (Windows)

**MSBuild (Visual Studio 2022):**
```bash
cd "d:\Dev\WFS_DIY_v1\Builds\VisualStudio2022"
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" WFS-DIY.sln -p:Configuration=Release -m
```

For Debug build:
```bash
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" WFS-DIY.sln -p:Configuration=Debug -m
```

**Note:** The `-m` flag enables parallel builds for faster compilation.

---

## TODO Summary by Priority

### High Priority
1. **Snapshot System UI**: InputsTab snapshot buttons need implementation (scope editing window complete)
2. **Reverb Polish**: Performance profiling, tuning algorithm parameters, edge-case testing

### Medium Priority
3. **Remote handshake**: Initialize and transmit state of all inputs
4. **Protocol Implementation**: ADM-OSC
5. **Remote Protocol Enhancements**: Secondary touch functions

### Lower Priority
6. **GPU Audio Port**: After DSP algorithms are fully tuned
7. **Testing**: Comprehensive protocol and feature testing

### Completed (Reverb DSP)
- ~~Phase 1-2: GUI, parameters, OSC, localization~~
- ~~Phase 3: ReverbEngine + FDN/SDN/IR algorithms~~
- ~~Phase 4: Pre/Post processing DSP (EQ, compressor, expander)~~
- ~~Parallelization: AudioParallelFor thread pool for all 3 algorithms~~
- ~~Algorithm switching: fade-out → swap → fade-in crossfade~~

---

*Last updated: 2026-02-09*
*Features added: Complete Reverb DSP (FDN/SDN/IR algorithms, Pre/Post processing, parallel per-node computation)*
*JUCE Version: 8.0.12*
*Build: Visual Studio 2022 / Xcode, x64 Debug/Release*
