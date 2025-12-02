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

- JUCE framework installed
- Platform-specific development tools:
  - **Windows**: Visual Studio 2022 or later
  - **macOS**: Xcode with latest macOS SDK
  - **Linux**: GCC or Clang with development tools

### Building

1. Open the project in Projucer:
   ```
   Open WFS-DIY.jucer in Projucer
   ```

2. Configure your modules and export targets as needed

3. Build using your preferred IDE or command line tools

### Platform-Specific Builds

- **Windows**: Open `Builds/VisualStudio2022/WFS-DIY.sln` in Visual Studio
- **macOS**: Open `Builds/MacOSX/WFS-DIY.xcodeproj` in Xcode
- **Linux**: Use the generated Makefile in `Builds/LinuxMakefile/`

### GPU Audio (experimental)

WFS-DIY includes experimental support for GPU-accelerated audio processing using the GPU-Audio SDK. The SDK is integrated as a Git submodule at `ThirdParty/GPUAudioSDK`.

**For detailed setup instructions, see [GPU-Audio SDK Setup Guide](Documentation/GPU_AUDIO_SDK_SETUP.md)**

Quick start:
1. Initialize the submodule: `git submodule update --init --recursive`
2. Build the SDK following the setup guide
3. Set environment variables: `GPUAUDIO_PATH` and `GPUAUDIO_PROCESSOR_PATH`
4. In the app, choose `GPU InputBuffer (GPU Audio)` and enable processing

**Note**: The GPU Audio feature requires the [GPU Audio Platform](https://www.gpu.audio/sdk-binaries) to be installed separately.

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

Copyright (c) 2025 Pierre-Olivier Boulant

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
