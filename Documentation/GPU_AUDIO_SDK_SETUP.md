# GPU-Audio SDK Setup Guide

This document explains how to set up and build the GPU-Audio SDK that is integrated into WFS-DIY as a Git submodule.

## Overview

The GPU-Audio SDK is integrated into this project as a Git submodule located at `ThirdParty/GPUAudioSDK`. This allows us to:
- Track a specific version of the SDK
- Keep the SDK code separate from our project code
- Easily update to new SDK versions when needed

## Initial Setup

### 1. Clone the Repository with Submodules

If you're cloning the WFS-DIY repository for the first time, use:

```bash
git clone --recursive https://github.com/pob31/WFS-DIY.git
```

This will automatically clone the GPU-Audio SDK submodule.

### 2. Initialize Existing Repository

If you already have the repository cloned but don't have the submodule initialized:

```bash
git submodule update --init --recursive
```

The `--recursive` flag ensures that nested submodules within the SDK are also initialized.

### 3. Update Submodules

To update the GPU-Audio SDK to the latest commit from the remote:

```bash
git submodule update --remote --recursive
```

To update to a specific version, navigate to the submodule directory and checkout a specific tag or commit:

```bash
cd ThirdParty/GPUAudioSDK
git checkout <tag-or-commit>
cd ../..
git add ThirdParty/GPUAudioSDK
git commit -m "Update GPU-Audio SDK to <version>"
```

## Building the SDK

### Prerequisites

Before building the GPU-Audio SDK, ensure you have:

1. **GPU Audio Platform** installed:
   - Download from: https://www.gpu.audio/sdk-binaries
   - Follow the installation instructions for your platform

2. **CMake** (version 3.24.0 or later)

3. **Platform-specific build tools**:
   - **Windows**: Visual Studio 2022 or later with C++ build tools
   - **macOS**: Xcode with Command Line Tools
   - **Linux**: GCC or Clang with development tools

4. **CUDA/ROCm** (depending on your GPU):
   - NVIDIA GPUs: CUDA Toolkit
   - AMD GPUs: ROCm

### Build Steps

1. **Navigate to the SDK directory**:
   ```bash
   cd ThirdParty/GPUAudioSDK
   ```

2. **Create a build directory**:
   ```bash
   mkdir build
   cd build
   ```

3. **Configure with CMake**:
   
   **Windows (Visual Studio)**:
   ```bash
   cmake .. -G "Visual Studio 17 2022" -A x64
   ```
   
   **macOS**:
   ```bash
   cmake .. -G "Xcode"
   ```
   
   **Linux**:
   ```bash
   cmake .. -G "Unix Makefiles"
   ```

4. **Build the SDK**:
   
   **Windows**:
   ```bash
   cmake --build . --config RelWithDebInfo
   ```
   
   **macOS/Linux**:
   ```bash
   cmake --build . --config Release
   ```

   Or open the generated solution/project files in your IDE and build from there.

5. **Expected Output Location**:
   
   After building, you should find the libraries and binaries in:
   - Libraries: `ThirdParty/GPUAudioSDK/build/lib/RelWithDebInfo/` (Windows) or `build/lib/Release/` (macOS/Linux)
   - Binaries: `ThirdParty/GPUAudioSDK/build/bin/RelWithDebInfo/` (Windows) or `build/bin/Release/` (macOS/Linux)

## Environment Variables

After building, you need to set up environment variables so the GPU Audio runtime can find the binaries:

### Windows

Set the following environment variables:

```powershell
$env:GPUAUDIO_PATH = "C:\Path\To\GPUAudio\Platform\Install"  # Path to GPU Audio Platform installation
$env:GPUAUDIO_PROCESSOR_PATH = "D:\Documents\WFS-DIY_JUCE\WFS-DIY\ThirdParty\GPUAudioSDK\build\bin\RelWithDebInfo"  # Path to built processors
```

Or set them permanently via System Properties → Environment Variables.

### macOS/Linux

Add to your `~/.bashrc`, `~/.zshrc`, or equivalent:

```bash
export GPUAUDIO_PATH="/path/to/gpuaudio/platform/install"
export GPUAUDIO_PROCESSOR_PATH="$(pwd)/ThirdParty/GPUAudioSDK/build/bin/Release"
```

## Project Configuration

The WFS-DIY project is configured to use the SDK via relative paths in the `.jucer` file:

- **Header paths**: 
  - `ThirdParty/GPUAudioSDK/gpuaudio/include`
  - `ThirdParty/GPUAudioSDK/gain_processor/gain_processor/include`
  - `ThirdParty/GPUAudioSDK/build/_deps/json-src/single_include` (after CMake configure)

- **Library paths**:
  - `ThirdParty/GPUAudioSDK/build/lib/RelWithDebInfo` (Windows)
  - `ThirdParty/GPUAudioSDK/build/GainLib/gainlib/RelWithDebInfo`
  - `ThirdParty/GPUAudioSDK/build/simple_processor_launcher/ProcLaunchLib/RelWithDebInfo`

Note: The `gpuaudio` directory is typically fetched or created during the CMake configuration phase of the SDK build.

## Troubleshooting

### Submodule Shows as Empty

If the submodule directory appears empty:

```bash
git submodule update --init --recursive
```

### Build Fails to Find Headers

1. Ensure you've run CMake configuration first (this fetches dependencies)
2. Verify the paths in `WFS-DIY.jucer` are correct relative to the project root
3. Check that nested submodules are initialized: `git submodule status --recursive`

### GPU Audio Runtime Not Found

1. Verify `GPUAUDIO_PATH` points to the GPU Audio Platform installation
2. Verify `GPUAUDIO_PROCESSOR_PATH` points to the built processor binaries directory
3. Ensure the GPU Audio Platform is properly installed and your GPU drivers are up to date

### CMake Configuration Errors

1. Ensure you have CMake 3.24.0 or later: `cmake --version`
2. Check that all required dependencies are installed (CUDA/ROCm, etc.)
3. Review the SDK's installation guide: `ThirdParty/GPUAudioSDK/installation/main.md`

## Updating the SDK

To update to a newer version of the GPU-Audio SDK:

1. **Check available versions**:
   ```bash
   cd ThirdParty/GPUAudioSDK
   git fetch --tags
   git tag
   ```

2. **Checkout the desired version**:
   ```bash
   git checkout <tag-name>
   ```

3. **Update nested submodules**:
   ```bash
   git submodule update --init --recursive
   ```

4. **Rebuild the SDK**:
   ```bash
   cd build
   cmake --build . --config RelWithDebInfo
   ```

5. **Commit the update**:
   ```bash
   cd ../..
   git add ThirdParty/GPUAudioSDK
   git commit -m "Update GPU-Audio SDK to <version>"
   ```

## Additional Resources

- GPU Audio SDK Repository: https://github.com/gpuaudio/gpuaudio-sdk
- GPU Audio Platform: https://www.gpu.audio
- SDK Installation Guide: `ThirdParty/GPUAudioSDK/installation/main.md`

