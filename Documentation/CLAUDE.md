# WFS DIY - Development Notes for Claude

## Project Overview
Wave Field Synthesis (WFS) audio application built with JUCE framework for real-time multi-channel audio processing with comprehensive OSC network control.

## Current Implementation Status (As of 2025-12-29)

### Overall Progress: ~65% Complete

The application has established a solid foundation with infrastructure and core UI:
- Complete parameter management system
- Professional GUI framework with tabbed interface
- Bidirectional OSC communication
- Project-based save/load system
- Interactive multitouch Map view
- Complete Clusters management
- Network Log window
- **DSP Calculation Layer** (delay/level/HF matrices from geometry)
- **Input Visualisation** (real-time DSP matrix display)
- **Live Source Tamer** (per-speaker gain reduction for feedback prevention)
- **Floor Reflections** (simulated floor bounce with filtering and diffusion)
- **Audio Interface & Patching Window** (input/output patch matrices with test signal generation)

**Major features still to implement:**
- Reverb algorithm design (convolution/algorithmic)
- Snapshot system UI
- Tracking protocols (PSN, RTTrP, OSC for tracking)
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
- **ReverbTab** - Reverb processing with 3 condensed sub-tabs:
  - "Channel Parameters" (3-column layout: Reverb+Position, Reverb Feed, Reverb Return)
  - "Reverb EQ" (4-band parametric EQ with interactive display)
  - "Algorithm" (placeholder for reverb algorithm selection)
- **MapTab** - Spatial visualization

### Floating Windows
- **AudioInterfaceWindow** - Audio device settings and input/output patching with test signal generation
- **NetworkLogWindow** - Network traffic monitoring with filtering and export
- **OutputArrayHelperWindow** - "Wizard of OutZ" for speaker array positioning

### Core Systems Status

| System | Status | Description |
|--------|--------|-------------|
| Parameters | Complete | ValueTree-based hierarchical state management |
| GUI Tabs | 90% | All 7 tabs have UI, some features pending |
| OSC Network | 90% | OSC, Remote protocols complete; ADM-OSC, PSN, RTTrP pending |
| Save/Load | 80% | Project folder management (snapshots UI TODO) |
| Audio Engine | 80% | Dual algorithm support with DSP calculation layer + Live Source Tamer + Floor Reflections |
| Separate Windows | 90% | Log window, Patch window, Array Helper all complete |
| Map View | 90% | Interactive multitouch map complete |
| Data Processing | 90% | WFS delay/level/HF + reverb matrices implemented |
| DSP Algorithms | 75% | Delay/gain/HF/FR filters working, reverb TODO |

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
- **InputVisualisationComponent.h** - Real-time DSP matrix visualization

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
| Main Room Straight | OFF | -0.2 | 100 | 10 | -4 | - |
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
- `Source/Network/OSCManager.h/cpp` - Network coordination
- `Source/Network/OSCLogger.h/cpp` - Message logging
- `Source/gui/InputsTab.h` - Input channel controls with joystick
- `Source/gui/InputVisualisationComponent.h` - DSP matrix visualization sliders
- `Source/gui/ReverbTab.h` - Reverb settings with EQ
- `Source/gui/NetworkLogWindow.h/cpp` - Log window UI
- `Source/gui/OutputArrayHelperWindow.h/cpp` - Wizard of OutZ window
- `Source/Helpers/ArrayGeometryCalculator.h/cpp` - Speaker array geometry calculations

---

## Build Notes
- JUCE 8.x required
- Project file: WFS-DIY.jucer
- Builds: Visual Studio 2022, Xcode, Linux Makefile
- Build command: `MSBuild WFS-DIY.sln -p:Configuration=Debug -p:Platform=x64`

---

## TODO Summary by Priority

### High Priority
1. **Snapshot System UI**: InputsTab snapshot buttons need implementation

### Medium Priority
2. **Reverb Algorithm**: Design convolution/algorithmic reverb processing
3. **Remote handshake**: Initialize and transmit state of all inputs
4. **Protocol Implementation**: ADM-OSC, PSN, RTTrP

### Lower Priority
5. **Remote Protocol Enhancements**: Secondary touch functions
6. **GPU Audio Port**: After DSP algorithms are working
7. **Testing**: Comprehensive protocol and feature testing

---

*Last updated: 2026-01-02*
*JUCE Version: 8.0.12*
*Build: Visual Studio 2022 / Xcode, x64 Debug/Release*
