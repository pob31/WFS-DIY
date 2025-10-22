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

This project is using GPL v3 licences.
Copyright (c) 2025, Pierre-Olivier Boulant
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * The name of the author may not be used to endorse or promote
      products derived from this software without specific prior 
      written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
