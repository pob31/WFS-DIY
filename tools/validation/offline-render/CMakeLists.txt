# offline-render — bit-exact offline render gate for the spatcore extraction.
# Design: docs/architecture/offline-render-harness.md
#
# Consumes JUCE the same way Plugin/CMakeLists.txt does (add_subdirectory of
# the vendored ThirdParty/JUCE) and compiles the app's DSP headers in place.
#
# Configure/build (Windows, VS-bundled cmake):
#   cmake -S tools/validation/offline-render -B tools/validation/offline-render/build \
#         -G "Visual Studio 18 2026"
#   cmake --build tools/validation/offline-render/build --config Release
#
# Milestone 1 is CPU + reverb only; do NOT define WFS_GPU_NATIVE here (the GPU
# paths are milestone 2 and are #if-guarded in main.cpp).

cmake_minimum_required(VERSION 3.22)

project(offline-render VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
set(JUCE_DIR  "${REPO_ROOT}/ThirdParty/JUCE")

add_subdirectory(${JUCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/juce EXCLUDE_FROM_ALL)

juce_add_console_app(offline-render PRODUCT_NAME "offline-render")

# The app's DSP headers include <JuceHeader.h>; generate one from the linked
# module set (same trick as the Projucer-generated header they normally see).
juce_generate_juce_header(offline-render)

target_sources(offline-render PRIVATE
    main.cpp)

target_include_directories(offline-render PRIVATE
    ${REPO_ROOT}/Source)

target_compile_definitions(offline-render PRIVATE
    REVERB_DIAGNOSTICS=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)

target_link_libraries(offline-render PRIVATE
    juce::juce_core
    juce::juce_events
    juce::juce_audio_basics
    juce::juce_audio_formats
    juce::juce_dsp
    juce::juce_recommended_config_flags)
