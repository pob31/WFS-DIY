# Building the WFS Input Buffer Processor

This GPU Audio processor implements delay-and-sum routing for Wave Field Synthesis.
It accepts routing messages (delay + gain per input-output pair) and processes audio
on the GPU in parallel across all routing pairs.

## Prerequisites

- **GPU Audio Platform** 2.3.0.219+ installed ([download](https://www.gpu.audio/sdk-binaries))
- **GPU Audio SDK** submodule initialized (`git submodule update --init --recursive ThirdParty/GPUAudioSDK`)
- **CUDA Toolkit** 12.x (NVIDIA) or ROCm/HIP 6.x (AMD)
- **CMake** 3.24.0+
- **Visual Studio 2022** with MSVC v143 toolchain (Windows)
- Environment variable `GPUAUDIO_PATH` pointing to GPU Audio Platform installation

## Build Steps

### Option A: Standalone Build

```bash
cd Processors/wfs_input_buffer_processor
mkdir build && cd build

# Configure (NVIDIA)
cmake .. -G "Visual Studio 17 2022" -A x64 -DWITH_CUDA=ON

# Build
cmake --build . --config RelWithDebInfo
```

### Option B: Integrate with SDK Build

Add this processor to the SDK's `CMakeLists.txt`:

```cmake
# In ThirdParty/GPUAudioSDK/CMakeLists.txt, add to PROCESSOR_LIST:
set(PROCESSOR_LIST
    "gain_processor"
    "../../Processors/wfs_input_buffer_processor"
)
```

Then build the SDK as normal.

## Deployment

After building, the processor DLL (`wfs_input_buffer_processor_nvidia.dll`) must be
discoverable by the GPU Audio engine. Set the `GPUAUDIO_PROCESSOR_PATH` environment
variable to include the directory containing the built DLL:

```powershell
# Windows (PowerShell)
$env:GPUAUDIO_PROCESSOR_PATH = "path\to\build\bin\RelWithDebInfo"

# Or add to system environment variables for persistence
```

## Enabling GPU Audio in WFS-DIY

1. Build this processor (see above)
2. Set `GPUAUDIO_PROCESSOR_PATH` environment variable
3. In the Projucer project:
   - Set `GPU_AUDIO_ENABLED=1` in preprocessor definitions
   - Change `compile="0"` to `compile="1"` for `GpuInputBufferAlgorithm.h/cpp` and `InputBufferAlgorithmGPU.h/cpp`
4. Rebuild WFS-DIY
5. Select "GPU Input Buffer" as the processing algorithm

## Architecture

```
Host (CPU):                          GPU:
MainComponent                        wfs_input_buffer processor
  └─ GpuInputBufferAlgorithm         ├─ WfsInputProcessor (host-side)
       ├─ GPU Audio Engine            │   ├─ SetData() ← RoutingMessage
       ├─ ProcessExecutor<eSync>      │   └─ PrepareChunk() → ProcessorParameter
       └─ RoutingMessage builder      └─ WfsInputProcessorDevice (CUDA kernel)
                                           └─ process(): delay + gain per pair
```

- **One GPU block per input×output routing pair** (N×M total)
- **atomicAdd** accumulates multiple inputs into each output channel
- **Synchronous execution** within the audio callback (no latency added)
- **SEH/try-catch** guards handle GPU driver faults gracefully

## Module ID

The processor registers as `wfs_input_buffer` and is loaded by `GpuInputBufferAlgorithm`
at runtime via the GPU Audio module provider.
