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

This project is based on JUCE framework examples and follows the same licensing terms.
