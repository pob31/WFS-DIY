cmake_minimum_required(VERSION 3.24.0 FATAL_ERROR)

# This CMakeLists.txt allows building the processor standalone.
# It requires the GPU Audio SDK to be available and configured.

# Get the GPU Audio SDK path (relative to this file)
# Assuming SDK is at ../../ThirdParty/GPUAudioSDK from this processor directory
get_filename_component(GPU_AUDIO_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../ThirdParty/GPUAudioSDK" ABSOLUTE)

if(NOT EXISTS "${GPU_AUDIO_SDK_DIR}/processor_cmake")
    message(FATAL_ERROR "GPU Audio SDK not found at ${GPU_AUDIO_SDK_DIR}. Please ensure the SDK submodule is initialized.")
endif()

# Include SDK's processor_cmake preamble
list(APPEND CMAKE_MODULE_PATH "${GPU_AUDIO_SDK_DIR}/processor_cmake")
include(${GPU_AUDIO_SDK_DIR}/processor_cmake/preamble.cmake)

# Set FETCH_LOCATION for dependencies
set(FETCH_LOCATION "${GPU_AUDIO_SDK_DIR}/gpuaudio")

# Project setup
project(wfs_input_buffer_processor
        VERSION 1.0.0
        LANGUAGES C CXX
        DESCRIPTION "WFS Input Buffer Processor for GPU Audio")

# Parse VERSION file
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" VERSION_FILE)
string(REGEX MATCH "^.*([0-9]+)\\.([0-9]+)\\.([0-9]+).*$" UNUSED_MATCH ${VERSION_FILE})
set(wfs_input_buffer_processor_MAJOR_VERSION ${CMAKE_MATCH_1})
set(wfs_input_buffer_processor_MINOR_VERSION ${CMAKE_MATCH_2})
set(wfs_input_buffer_processor_PATCH_LEVEL ${CMAKE_MATCH_3})

# Parse PROCESSOR file
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/PROCESSOR" PROCESSOR_FILE)
string(REGEX MATCH "^.*PROCESSOR_IDENTIFIER=[\"]([A-Za-z]+[A-Za-z0-9]*)[\"].*$" UNUSED_MATCH ${PROCESSOR_FILE})
set(PARSED_PROCESSOR_ID ${CMAKE_MATCH_1})

# Include settings and fetch dependencies
include(${GPU_AUDIO_SDK_DIR}/processor_cmake/settings.cmake)
include(${GPU_AUDIO_SDK_DIR}/processor_cmake/fetch.cmake)

# Configure CUDA if enabled
if(NOT APPLE AND WITH_CUDA)
    enable_language(CUDA)
    if(DEFINED CUDA_VERSION)
        find_package(CUDAToolkit ${CUDA_VERSION} REQUIRED EXACT MODULE BYPASS_PROVIDER)
    else()
        find_package(CUDAToolkit REQUIRED MODULE BYPASS_PROVIDER)
        set(CUDA_VERSION "${CUDAToolkit_VERSION_MAJOR}.${CUDAToolkit_VERSION_MINOR}")
    endif()
    
    if(CUDA_VERSION VERSION_LESS 12.0)
        set(CMAKE_CUDA_STANDARD "17")
    else()
        set(CMAKE_CUDA_STANDARD "20")
    endif()
    
    set(CMAKE_CUDA_STANDARD_REQUIRED "ON")
    set(CUDA_MAXRREGCOUNT "128")
endif()

# Add cmake-common modules
list(APPEND CMAKE_MODULE_PATH ${FETCH_LOCATION}/cmake-common)

# Load cmake-common macros (includes BG_AddComponent)
include("${FETCH_LOCATION}/cmake-common/braingines.cmake")

# Set common include directories (this will be used by CMakeLists.var.cmake)
set(common_include_directories
    ${FETCH_LOCATION}/include
)

# Build the processor component
add_subdirectory(wfs_input_buffer_processor)

