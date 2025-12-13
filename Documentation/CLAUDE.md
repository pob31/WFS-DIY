# WFS DIY - Development Notes for Claude

## Project Overview
Wave Field Synthesis (WFS) audio application built with JUCE framework for real-time multi-channel audio processing with comprehensive OSC network control.

## Current Implementation Status (As of 2025-12-13)

### Overall Progress: ~25% Complete

The application has established a solid foundation with infrastructure and core UI:
- Complete parameter management system
- Professional GUI framework with tabbed interface
- Bidirectional OSC communication
- Project-based save/load system

**Major features still to implement:**
- Interactive multitouch Map view
- Audio Patch routing window
- Network Log window
- Reverb configuration
- Cluster management
- Snapshot system UI
- WFS geometry ↔ audio engine connection

---

## Architecture Overview

### Core Systems

| System | Status | Description |
|--------|--------|-------------|
| Parameters | Complete | ValueTree-based hierarchical state management |
| GUI Tabs | 40% | 4 of 7 tabs have UI, 3 are placeholders |
| OSC Network | Complete | Bidirectional OSC with multiple protocols |
| Save/Load | 80% | Project folder management (snapshots UI TODO) |
| Audio Engine | Basic | Dual algorithm support, needs geometry integration |
| Separate Windows | 0% | Patch window, Log window not started |
| Map View | 0% | Interactive multitouch map not started |

---

## Parameter System

### WFSParameterIDs.h
Complete parameter identifier definitions organized by section:
- **ValueTree Types**: WFSProcessor, Config, Inputs, Outputs, etc.
- **Show Section**: showName, showLocation
- **I/O Section**: inputChannels, outputChannels, reverbChannels, algorithmDSP, runDSP
- **Stage Section**: stageWidth/Depth/Height, origin coordinates, speedOfSound, temperature
- **Master Section**: masterLevel, systemLatency, haasEffect
- **Network Section**: networkInterface, ports (UDP 8000, TCP 8001), up to 6 targets, OSC Query (port 5005)
- **ADM-OSC Section**: offset, scale, flip for X/Y/Z axes
- **Tracking Section**: enabled, protocol, port, transforms
- **Input Parameters** (87 per channel): position, attenuation, directivity, live source tamer, hackoustics, LFO, AutomOtion, mutes
- **Output Parameters** (16 per channel): position, orientation, options, 6-band EQ
- **Audio Patch**: driverMode, audioInterface, test tone settings

### WFSParameterDefaults.h
Default values and ranges:
- maxInputChannels = 64
- maxOutputChannels = 64
- maxReverbChannels = 6
- maxNetworkTargets = 6
- Temperature to speed-of-sound formula included

### WFSValueTreeState.h/cpp (~800 lines)
- Hierarchical ValueTree state management
- Type-safe parameter access (getFloatParameter, getIntParameter, getStringParameter)
- Dynamic channel management (input/output/reverb)
- Network target management
- Undo/Redo via UndoManager
- Listener registration for UI binding
- State validation and reset

### WFSFileManager.h/cpp (~1000 lines)
- Project folder structure (backups/, input_snapshots/, output_snapshots/)
- Complete config save/load
- Section-specific save/load (system, network, inputs, outputs)
- Snapshot management with scope filtering
- Automatic backup rotation
- XML serialization with human-readable formatting

### WfsParameters.h
Backward-compatibility wrapper providing old-style API access to new parameter system.

---

## GUI Implementation

### Tab Structure (MainComponent)

```
TabbedComponent
├── System Configuration (SystemConfigTab) ✓ Complete
├── Network (NetworkTab) ✓ Complete
├── Outputs (OutputsTab) ✓ Complete
├── Inputs (InputsTab) ✓ 90% Complete
├── Reverb (ReverbTab) □ Placeholder
├── Clusters (ClustersTab) □ Placeholder
└── Map (MapTab) □ Placeholder
```

### SystemConfigTab.h (~1300 lines) - COMPLETE
- **Show Section**: Name and location text editors
- **I/O Section**:
  - Input/Output/Reverb channel count selectors
  - Audio Interface button (opens AudioInterfaceWindow)
  - Algorithm selector (InputBuffer/OutputBuffer)
  - Processing toggle
- **Stage Section**:
  - Dimensions (width, depth, height)
  - Origin position with 3 preset buttons (custom-drawn icons)
  - Speed of sound, temperature
- **Master Section**: Level, latency, Haas effect
- **Footer**: Select Folder, Store, Reload, Reload Backup, Import, Export

### NetworkTab.h (~2400 lines) - COMPLETE
- **Network Section**:
  - Interface selector with platform-specific friendly names
  - Current IP display
  - UDP Port (default 8000), TCP Port (default 8001)
  - OSC Query port (default 5005) with enable toggle
- **Network Connections Table** (6 targets max):
  - Name (with connection status color indicator)
  - Mode (UDP/TCP)
  - IP Address
  - Tx Port
  - Rx Enable, Tx Enable
  - Protocol (DISABLED/OSC/REMOTE/ADM-OSC)
  - Remove button
- **OSC Source Filter**: Toggle for IP-based message filtering
- **ADM-OSC Section**: Offset/Scale/Flip for X/Y/Z (grayed when no ADM-OSC target)
- **Tracking Section**: Enable, protocol, port, offset/scale/flip
- **Footer**: Open Log Window, Find My Remote, Store, Reload, Import, Export

**Connection Status Indicators** (name field background):
- Gray: Disconnected
- Yellow: Connecting
- Green: Connected
- Red: Error

### InputsTab.h (~3575 lines) - 90% COMPLETE
- **Channel Selector**: Dynamic count based on configuration
- **Transport Controls**: Play, Stop, Pause (custom-drawn icons)
- **Sub-Tabs**:
  - Channel (name, attenuation, delay, height factor)
  - Position (X/Y/Z, offset, constraints, flip, cluster, tracking)
  - Attenuation (law, distance attenuation, ratio)
  - Directivity (directivity, rotation, tilt, HF shelf)
  - Live Source Taming (radius, shape, attenuation, thresholds)
  - Hackoustics (floor reflections with filters)
  - Jitter
  - LFO (period, phase, shape/rate/amplitude/phase per axis, gyrophone)
  - AutomOtion (targets, mode, trigger, reset)
  - Mutes (per-output mutes, macro buttons)
- **Snapshots Section**: TODO - buttons exist but not implemented
- **Footer**: Store, Reload

### OutputsTab.h (~1400 lines) - COMPLETE
- **Channel Selector**: Dynamic count
- **Channel Header**: Name, array selector, apply-to-array
- **Sub-Tabs**:
  - Output Properties (attenuation, delay, options, parallax)
  - Position (X/Y/Z, orientation, angles, pitch, HF damping)
  - EQ (enabled toggle, 6 bands with freq/gain/Q/shape/slope)
- **Footer**: Store, Reload

### Placeholder Tabs
- **ReverbTab.h**: "Reverb Configuration" label only
- **ClustersTab.h**: "Clusters Configuration" label only
- **MapTab.h**: "Map View" label only

### Supporting Components
- **StatusBar.h**: Help text display at bottom of window
- **AudioInterfaceWindow.h**: Separate window for audio device selection
- **ConfigTabPreviewWindow.h**: Preview window for testing UI components

---

## Network/OSC System (~4600 lines total)

### OSCProtocolTypes.h
- **Enums**: Protocol, ConnectionMode, ConnectionStatus, Axis, DeltaDirection
- **Structures**: TargetConfig, GlobalConfig, LogEntry
- **Constants**: MAX_TARGETS=6, MAX_RATE_HZ=50, DEFAULT_UDP_PORT=8000, DEFAULT_TCP_PORT=8001

### OSCManager.h/cpp (~1100 lines)
Central coordinator for all OSC communication:
- Manages up to 6 network targets
- UDP and TCP receivers with sender IP tracking
- Rate limiting (50Hz max via OSCRateLimiter)
- Message routing to/from ValueTree
- Protocol support: OSC, REMOTE (Android app), ADM-OSC
- IP filtering (only accept from configured targets)
- Loop prevention (tracks incoming protocol to avoid echo)
- OSC Query server integration
- Connection status callbacks for UI updates
- **REMOTE protocol handlers**:
  - handleRemoteParameterSet() - absolute value setting
  - handleRemoteParameterDelta() - incremental changes with inc/dec
  - sendRemoteChannelDump() - sends 48+ parameters on channel select
- **Array Adjust handler**:
  - handleArrayAdjustMessage() - applies delta to all outputs in array

### OSCConnection.h/cpp
Per-target connection manager:
- UDP sender (fully implemented)
- TCP sender (basic implementation)
- Connection status tracking
- Message statistics

### OSCMessageRouter.h/cpp (~600 lines)
- Parse incoming OSC messages
- Address pattern matching (/wfs/input/, /wfs/output/, /wfs/reverb/)
- Parameter ID extraction from address maps
- Value extraction (float, int, string)
- **REMOTE protocol parsing** (/remoteInput/*):
  - ParsedRemoteInput struct with types: ChannelSelect, PositionDelta, ParameterSet, ParameterDelta
  - getRemoteAddressMap() with 48+ parameter mappings
  - Supports absolute values and inc/dec mode
- **Array Adjust parsing** (/arrayAdjust/*):
  - ParsedArrayAdjustMessage struct
  - Bulk adjustment for delayLatency, attenuation, Hparallax, Vparallax

### OSCMessageBuilder.h/cpp (~500 lines)
- Build outgoing OSC messages
- Input/output parameter messages
- REMOTE protocol messages
- Channel dump for initial sync

### OSCReceiverWithSenderIP.h/cpp
Custom UDP receiver exposing sender IP for filtering.

### OSCTCPReceiver.h/cpp
TCP server with multi-client support and sender IP tracking.

### OSCParser.h
Custom OSC message/bundle parser (JUCE's OSCInputStream is private).

### OSCLogger.h/cpp (~250 lines)
- Ring buffer message logging
- Thread-safe access
- Filtering by direction/target/protocol/address
- Entry count for UI updates

### OSCRateLimiter.h/cpp (~200 lines)
- 50Hz rate limiting per target
- Message coalescing (latest value wins)
- Broadcast support
- Statistics tracking

### OSCQueryServer.h/cpp (~300 lines)
HTTP server for OSC Query protocol:
- Responds to GET requests with JSON parameter tree
- Exposes /wfs/input/{n}/ and /wfs/output/{n}/ namespaces
- Parameters: positionX/Y/Z, attenuation, muteMacro (inputs), attenuation (outputs)
- Runs in separate thread
- Configurable port (default 5005)

---

## Audio Processing

### Algorithms
Two processing approaches available:

**InputBufferAlgorithm (read-time delays)**
- One thread per input channel
- Outputs to all outputs with individual delays/levels
- Straightforward implementation

**OutputBufferAlgorithm (write-time delays)**
- One thread per output channel
- Receives from all inputs
- Better cache locality, lower CPU observed

### Supporting Components
- **LockFreeRingBuffer.h**: Single producer/consumer lock-free buffer
- **InputBufferProcessor.h**: Per-input thread processor
- **OutputBufferProcessor.h**: Per-output thread processor
- **GpuInputBufferAlgorithm.h**: GPU version (commented out - SDK not configured)

---

## Save/Load System

### Project Folder Structure
```
[ProjectFolder]/
├── config.xml           # Complete configuration
├── system_config.xml    # System section only
├── network_config.xml   # Network section only
├── input_config.xml     # Input channels
├── output_config.xml    # Output channels
├── backups/
│   └── config_YYYYMMDD_HHMMSS.xml
├── input_snapshots/
│   └── [snapshot_name].xml
└── output_snapshots/
    └── [snapshot_name].xml
```

### Save/Load Operations
- **Store**: Save complete config to project folder
- **Reload**: Load config from project folder
- **Reload Backup**: Load from backup subfolder
- **Import/Export**: File dialog for arbitrary locations
- **Snapshots**: Per-channel/subsection scope filtering (TODO in UI)

---

## TODO / Remaining Work

### Major Features (High Priority)
1. **Audio Patch Window**: Separate window for input/output routing matrix
   - Driver mode selection
   - Input/Output patch matrices with scroll/patch/test modes
   - Test tone generation (sine/pink noise)
   - See [WFS-UI_audioPatch.csv](WFS-UI_audioPatch.csv) for specification

2. **Network Log Window**: Separate window for OSC message monitoring
   - Display OSCLogger entries
   - Filtering by direction/target/protocol
   - Clear/pause functionality

3. **MapTab - Interactive Map**: 2D/3D visualization and control
   - Display speaker array positions
   - Display/control virtual source positions
   - Multitouch support for source positioning
   - Zoom/pan controls
   - This is a significant feature

4. **ReverbTab**: Implement reverb channel configuration
   - Up to 6 reverb channels
   - Send levels from inputs
   - Reverb parameters

5. **ClustersTab**: Implement cluster management
   - Group inputs into clusters
   - Cluster-level controls

### Integration Work (Medium Priority)
6. **InputsTab Snapshots**: Implement snapshot creation/loading/editing UI
   - Snapshot creation dialog with scope selection
   - Snapshot list management
   - Load/update/delete operations

7. **WFS Geometry Integration**: Connect UI parameters to audio engine
   - Calculate delays from source/speaker positions
   - Calculate levels from amplitude panning
   - Real-time updates as sources move

8. **UI Polish**: Layout refinements across all tabs
   - Consistent spacing and alignment
   - Responsive layouts

### Lower Priority
9. **TCP proper implementation**: Full TCP support with StreamingSocket
10. **GPU Audio**: Re-enable when SDK is configured
11. **Find My Remote**: Device discovery feature
12. **Undo/Redo UI**: Expose undo/redo functionality in UI

---

## Building & Running

### Requirements
- JUCE 8.0.10
- Visual Studio 2022 (Windows) / Xcode (macOS)
- ASIO-capable audio interface (optional)

### Build Configuration
- JUCE_ASIO=1 (Windows only)
- JUCE_JACK=0
- macOS: Extra linker flags for SystemConfiguration framework

### Platforms
- Windows (VS2022) - Primary development
- macOS (Xcode) - Tested
- Linux (Makefile) - Available but not actively tested

---

## Recent Work (2025-12-03 to 2025-12-13)

### OSC Network System (2025-12-03 to 2025-12-05)
- Implemented complete OSC communication stack
- Added OSCManager as central coordinator
- Created OSCConnection for per-target management
- Built OSCMessageRouter for incoming message parsing
- Built OSCMessageBuilder for outgoing message creation
- Added OSCRateLimiter (50Hz, message coalescing)
- Added OSCLogger with ring buffer
- Implemented custom UDP/TCP receivers with sender IP tracking
- Added IP filtering for security

### OSC Source Filtering (2025-12-11)
- Implemented IP-based filtering of incoming OSC messages
- Created OSCReceiverWithSenderIP (custom UDP receiver)
- Created OSCTCPReceiver (TCP server with multi-client support)
- Created OSCParser (custom parser since JUCE's is private)
- UI toggle in NetworkTab ("OSC Filter: Accept All" / "Registered Only")

### OSC Query Server (2025-12-13)
- Implemented simple HTTP server for OSC Query protocol
- Exposes WFS parameters as JSON namespace
- UI controls in NetworkTab (port editor, enable toggle)
- Default port 5005

### Connection Status Indicators (2025-12-13)
- Added visual status to network connections table
- Name field background changes color based on status
- Proper disconnect on Tx off or Protocol disabled

### Network Tab Polish (2025-12-13)
- Fixed UDP/TCP port parameter names (networkRxUDPport/networkRxTCPport)
- Default ports: UDP 8000, TCP 8001
- Improved OSC Query control layout

### Complete Remote OSC Protocol (2025-12-13)
Implemented full bidirectional communication with Android remote app:

**Receiving (/remoteInput/*):**
- `/remoteInput/inputNumber <id>` - Select channel and receive parameter dump
- `/remoteInput/<param> <id> <value>` - Absolute parameter setting
- `/remoteInput/<param> <id> <inc/dec> <delta>` - Incremental changes
- 48+ parameter mappings including position, attenuation, directivity, LFO, etc.

**Sending (/remoteInput/*):**
- Fixed address prefix from /remoteOutput/ to /remoteInput/
- sendRemoteChannelDump() sends all parameters on channel select

### Array Adjust Commands (2025-12-13)
One-way commands from remote to adjust output arrays (Rx only):
- `/arrayAdjust/delayLatency <array#> <delta>` - Adjust delay for all outputs in array
- `/arrayAdjust/attenuation <array#> <delta>` - Adjust attenuation
- `/arrayAdjust/Hparallax <array#> <delta>` - Adjust horizontal parallax
- `/arrayAdjust/Vparallax <array#> <delta>` - Adjust vertical parallax

---

## Code Statistics

| Component | Lines |
|-----------|-------|
| Parameters System | ~2,200 |
| Network/OSC System | ~5,300 |
| GUI Components | ~8,700 |
| MainComponent | ~850 |
| Audio Processing | ~1,500 |
| **Total** | **~18,550** |

---

## Architecture Highlights

1. **Hierarchical ValueTree**: All parameters in tree structure matching CSV spec
2. **Type-Safe Access**: Float, int, string parameter methods with channel indexing
3. **Bidirectional OSC**: Send/receive with OSC, REMOTE, ADM-OSC protocols
4. **Rate Limiting**: 50Hz message coalescing prevents network flooding
5. **IP Filtering**: Security feature for trusted sources only
6. **OSC Query**: HTTP-based parameter discovery for external tools
7. **Project Organization**: Complete configs saved to project folders
8. **Thread Safety**: CriticalSection locks, lock-free buffers
9. **Undo/Redo**: Full UndoManager integration
10. **Platform-Specific**: Network interface enumeration for Windows/macOS

---

*Last updated: 2025-12-13*
*JUCE Version: 8.0.10*
*Build: Visual Studio 2022 / Xcode, x64 Debug/Release*
