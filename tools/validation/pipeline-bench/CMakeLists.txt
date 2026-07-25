# pipeline-bench — real-time latency/underrun harness for the GPU async
# pipeline. Unlike offline-render (which drives the vendor backends
# SYNCHRONOUSLY with pipelineLatencyMs = 0), this tool exercises the REAL
# GpuAsyncPipelineT: SPSC rings, pump thread, depth cushion, and underrun
# accounting — driven by a metronomic simulated audio callback at a
# configurable sr/block cadence (default: 96 kHz / 64 samples).
#
# Configure/build (Windows, VS-bundled cmake):
#   cmake -S tools/validation/pipeline-bench -B tools/validation/pipeline-bench/build \
#         -G "Visual Studio 18 2026"
#   cmake --build tools/validation/pipeline-bench/build --config Release
#
# Like offline-render and the app, no GPU runtime is linked: GpuBackendFactory
# dlopens the per-vendor plugin (wfs_cuda.dll / wfs_hip.dll) at runtime
# (--plugin-dir, or the auto-probed app build output dir).

cmake_minimum_required(VERSION 3.22)

project(pipeline-bench VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
set(JUCE_DIR  "${REPO_ROOT}/ThirdParty/JUCE")

add_subdirectory(${JUCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/juce EXCLUDE_FROM_ALL)

juce_add_console_app(pipeline-bench PRODUCT_NAME "pipeline-bench")

juce_generate_juce_header(pipeline-bench)

target_sources(pipeline-bench PRIVATE
    main.cpp)

target_include_directories(pipeline-bench PRIVATE
    ${REPO_ROOT}/Source)

# REVERB_DIAGNOSTICS=1: compile the LockFreeRingBuffer overflow/underflow
# counters in (they are #if'd out of shipping builds).
target_compile_definitions(pipeline-bench PRIVATE
    REVERB_DIAGNOSTICS=1
    WFS_GPU_NATIVE=1
    WFS_GPU_PLUGINS=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)

target_link_libraries(pipeline-bench PRIVATE
    juce::juce_core
    juce::juce_events
    juce::juce_audio_basics
    juce::juce_recommended_config_flags)

if(WIN32)
    # timeBeginPeriod for the 1 ms system timer floor (MMCSS elevation itself
    # is runtime-loaded from avrt.dll by spatcore/rt/RtThreadPriority.h).
    target_link_libraries(pipeline-bench PRIVATE winmm)
endif()
