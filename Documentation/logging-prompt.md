# Task: Integrate Session Logging System into WFS-DIY

## Overview

Add a logging system to the WFS-DIY application that captures session events and exceptions to a log file. This is for diagnostics — the app runs in live theatre venues, often without internet access, so the log file needs to be self-contained and easy to export for remote troubleshooting.

## Requirements

### Logger Setup
- Use JUCE's `FileLogger` class (or a custom wrapper around `juce::Logger`)
- Log file location: user's application data directory (use `juce::File::getSpecialLocation()`)
- One log file per session, named with date and time (e.g., `WFS-DIY_2026-02-11_20-30-00.log`)
- Implement log rotation: keep the last 20 session logs, delete older ones on startup
- Maximum log file size cap as a safety measure (e.g., 10MB)

### What to Log

**On session startup (log once):**
- Application version
- OS and platform info
- Audio device name, sample rate, buffer size
- Number of configured inputs, outputs, and reverb channels
- Speaker layout / configuration summary
- Network configuration (OSC ports, connected remotes)

**During the session (log on state changes, NOT on the audio thread):**
- Audio device changes (sample rate, buffer size, device switch)
- OSC connection events (remote connect/disconnect, handshake, heartbeat loss)
- Algorithm switches (reverb type changes: SDN, FDN, IR)
- Configuration file load/save events
- Cluster creation/modification
- Any parameter changes that affect the processing architecture (not routine fader moves)
- Buffer underruns or audio dropouts (if detectable — use JUCE's `AudioIODevice::getXRunCount()` if available, or implement a simple detector)

**On exceptions/errors:**
- Exception type and message
- Stack trace (use `juce::SystemStats::getStackBacktrace()`)
- Current state snapshot: active audio device, buffer size, sample rate, number of active inputs/outputs, active reverb algorithm, CPU load estimate
- What the app was doing at the time (if determinable from context)

### Critical Constraints

- **NEVER log from the audio callback / real-time thread.** Use a lock-free queue or `juce::AsyncUpdater` to pass messages from the audio thread to the logging thread. Audio performance is the top priority — logging must not cause dropouts or clicks.
- **Keep it lightweight.** This is a live performance tool. The logging must have zero audible impact.
- **Thread safety.** Multiple threads (message thread, OSC receivers, possibly timer threads) may want to log. Ensure thread-safe writes.

### Export Feature
- Add a utility method (and later a UI button, but not now) to copy the current and recent log files to a specified directory (e.g., USB drive path)
- The method should bundle: current session log + last 5 session logs + current configuration files

### Code Integration
- Examine the existing codebase to understand where session initialization, audio device setup, OSC handling, and exception catching currently happen
- Integrate logging calls at those existing points — do not restructure existing code
- Follow the existing code style and patterns
- Create the logger as a singleton or app-level service accessible from anywhere in the codebase without passing references through the whole architecture

### File Structure
- Create new files for the logging system (e.g., `WFSLogger.h` / `WFSLogger.cpp`)
- Keep it self-contained so it can be maintained independently

## Do NOT
- Do not refactor existing working code
- Do not add logging inside audio processing loops or real-time callbacks
- Do not add UI elements yet (that comes later)
- Do not add network/upload functionality
