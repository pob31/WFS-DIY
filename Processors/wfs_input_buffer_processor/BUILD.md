# Building the WFS Input Buffer Processor

This processor is built as an external processor (Option 3) outside the GPU Audio SDK submodule. There are two ways to build it:

## Option A: Standalone Build

Build the processor independently using its own CMakeLists.txt:

```bash
cd Processors/wfs_input_buffer_processor
mkdir build
cd build

# Configure
cmake .. -G "Visual Studio 17 2022" -A x64

# Build
cmake --build . --config RelWithDebInfo
```

The built DLL will be in `build/bin/RelWithDebInfo/` or `build/bin/Release/` depending on your platform.

## Option B: Integrate with SDK Build

You can also add this processor to the SDK's build system by modifying the SDK's main `CMakeLists.txt`:

1. Edit `ThirdParty/GPUAudioSDK/CMakeLists.txt`
2. Add to `PROCESSOR_LIST`:
   ```cmake
   set(PROCESSOR_LIST
       "gain_processor"
       "iir_processor"
       "fir_processor"
       "NeuralAmpModeler/nam_processor"
       "../../Processors/wfs_input_buffer_processor"  # Add this line
   )
   ```
3. Build the SDK as normal - your processor will be included

**Note**: This modifies the SDK submodule. Consider if you want to keep this change or rebuild standalone.

## Prerequisites

- GPU Audio SDK must be built first (provides dependencies)
- CMake 3.24.0 or later
- CUDA Toolkit (for NVIDIA GPUs) or ROCm (for AMD GPUs)
- Visual Studio 2022 (Windows) or appropriate build tools on other platforms

## Output Location

After building, set `GPUAUDIO_PROCESSOR_PATH` environment variable to point to the directory containing the built DLL:

```powershell
# Windows
$env:GPUAUDIO_PROCESSOR_PATH = "D:\Documents\WFS-DIY_JUCE\WFS-DIY\Processors\wfs_input_buffer_processor\build\bin\RelWithDebInfo"
```

The processor DLL should be named `wfs_input_buffer_processor_nvidia.dll` (or similar for your platform).

## Implementation Status

**Note**: This processor currently has a skeleton structure. The actual implementation files (source files in `src/`) still need to be created. This includes:

- `WfsInputModule.cpp/h`
- `WfsInputProcessor.cpp/h`
- `WfsInputModuleInfoProvider.cpp/h`
- `WfsInputDeviceCodeProvider.cpp/h`
- `WfsInputInputPort.cpp/h`
- `WfsInputModuleLibrary.cpp`
- CUDA device code in `src/cuda/WfsInputProcessor.cu/cuh`

See the GPU Audio SDK documentation and the `gain_processor` example for implementation patterns.

