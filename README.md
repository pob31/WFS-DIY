# WFS-DIY

A Wave Field Synthesis WFS DIY project built with JUCE framework.

## Project Overview

This project is based on the Cycling74's Max8 Prototype found at https://wfs-diy.net

## Features

- Cross-platform audio device management
- Real-time audio processing capabilities
- JUCE framework integration
- Multi-platform build support (Windows, macOS, Linux)

## Building the Project

### Prerequisites

**Windows:**
1. Install [Git for Windows](https://git-scm.com/download/win)
2. Install [Visual Studio 2022 Community](https://visualstudio.microsoft.com/) (free) — during install, select the **"Desktop development with C++"** workload

**macOS:**
1. Install [Xcode](https://apps.apple.com/app/xcode/id497799835) from the App Store (free)
2. Git is included with Xcode. If you need it before opening Xcode, run: `xcode-select --install`

**Linux:**
- GCC or Clang with development tools

### Step-by-step build

**1. Clone the repository**

Open a terminal and run:

```bash
git clone --recurse-submodules https://github.com/pob31/WFS-DIY.git
```

> **Important:** The `--recurse-submodules` flag is required. Without it, dependencies (JUCE, ASIO SDK) will be missing and the build will fail. If you already cloned without it, run:
> ```bash
> cd WFS-DIY
> git submodule update --init --recursive
> ```

**2. Open the project and build**

**Windows:**
1. Open `Builds/VisualStudio2022/WFS-DIY.sln` in Visual Studio
2. In the toolbar, set the platform to **x64** (not x86)
3. Select **Debug** or **Release** configuration
4. Build > Build Solution (or press Ctrl+Shift+B)
5. Debug > Start Debugging (or press F5) to run

**macOS:**
1. Open `Builds/MacOSX/WFS-DIY.xcodeproj` in Xcode
2. Select the **WFS-DIY** target and **My Mac** as destination
3. Product > Build (or press Cmd+B)
4. Product > Run (or press Cmd+R) to run
5. macOS will ask for microphone permission on first run — click Allow (required for audio input)

**Linux:**
- Use the Makefile in `Builds/LinuxMakefile/`

### GPU Audio (experimental)

WFS-DIY includes experimental support for GPU-accelerated audio processing using the GPU-Audio SDK. The SDK is integrated as a Git submodule at `ThirdParty/GPUAudioSDK`.

**For detailed setup instructions, see [GPU-Audio SDK Setup Guide](Documentation/GPU_AUDIO_SDK_SETUP.md)**

Quick start:
1. Initialize the submodule: `git submodule update --init --recursive`
2. Build the SDK following the setup guide
3. Set environment variables: `GPUAUDIO_PATH` and `GPUAUDIO_PROCESSOR_PATH`
4. In the app, choose `GPU InputBuffer (GPU Audio)` and enable processing

**Note**: The GPU Audio feature requires the [GPU Audio Platform](https://www.gpu.audio/sdk-binaries) to be installed separately.

## Running the Application

### Windows
- **Required**: [Microsoft Visual C++ Redistributable 2022 (x64)](https://aka.ms/vs/17/release/vc_redist.x64.exe) — may already be installed on your system
- **Recommended**: ASIO drivers for your audio interface (WASAPI is available as fallback)

### macOS
- No additional runtime dependencies — all required frameworks ship with macOS 10.13+
- On first launch, macOS may ask you to confirm opening an unnotarized app (right-click > Open)
- Grant microphone permission when prompted (required for audio input)

## Development

This project uses Git for version control and is designed for cross-platform development. Make sure to:

- Test changes on multiple platforms when possible
- Keep build files synchronized across platforms
- Follow JUCE coding conventions

## Contributing

When contributing to this project:

1. Make sure your changes compile on your target platform
2. Test functionality before committing
3. Update documentation as needed
4. Follow the existing code style

## License

This project is licensed under the GNU General Public License v3.0 (GPL-3.0).

Copyright (c) 2026 Pierre-Olivier Boulant

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

### GPL v3 Key Principles

- **Freedom to use**: You can run the software for any purpose
- **Freedom to study**: You can examine and modify the source code
- **Freedom to distribute**: You can share copies of the software
- **Freedom to distribute modifications**: You can share your modified versions

**Important**: Any derivative works must also be licensed under GPL v3, ensuring the software remains free and open source.
