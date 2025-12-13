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
- **ClustersTab** - Speaker cluster management
- **ReverbTab** - Reverb processing settings
- **MapTab** - Spatial visualization

### Floating Windows
- **AudioInterfaceWindow** - JUCE AudioDeviceSelectorComponent for device setup
- **NetworkLogWindow** - Network traffic monitoring with filtering and export

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

## Build Notes
- JUCE 8.x required
- Project file: WFS-DIY.jucer
- Builds: Visual Studio 2022, Xcode, Linux Makefile

## Key Files
- `Source/MainComponent.h/cpp` - Application entry point
- `Source/WfsParameters.h/cpp` - Parameter definitions
- `Source/Network/OSCManager.h/cpp` - Network coordination
- `Source/Network/OSCLogger.h/cpp` - Message logging
- `Source/gui/NetworkLogWindow.h/cpp` - Log window UI
