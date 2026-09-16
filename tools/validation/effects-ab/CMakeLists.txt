# effects-ab — the Max/MSP A/B renderer for the effects modules.
# Spec: section 10 of Documentation/effects-channels-plan.md.
#
# NOT a CI gate. Section 10 says so, and the prototypes are acknowledged
# starting points open to improvement rather than bit-for-bit targets: this
# tool tells the user whether a port SOUNDS right, which no unit test can.
#
# Modelled on tools/validation/offline-render/CMakeLists.txt (same vendored
# JUCE, same console-app shape, same headless contract) but far smaller: the
# effects modules are header-only, need no GPU plugin and no app headers, so
# this links four JUCE modules and compiles one .cpp.
#
# Configure/build (Windows). There is no `cmake` on PATH on the dev box - use
# the one Visual Studio ships, or run from a Developer PowerShell where it is
# already on PATH:
#
#   CMAKE="C:/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/\
#   CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"
#   "$CMAKE" -S tools/validation/effects-ab -B tools/validation/effects-ab/build \
#            -G "Visual Studio 18 2026"
#   "$CMAKE" --build tools/validation/effects-ab/build --config Release
#
# The executable lands in build/ab-render_artefacts/Release/ab-render.exe.

cmake_minimum_required(VERSION 3.22)

project(effects-ab VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
set(JUCE_DIR  "${REPO_ROOT}/ThirdParty/JUCE")

# ---------------------------------------------------------------- spatcore
# This tool cannot be built against every spatcore commit: ab_render.cpp includes
# spatcore/effects/EffectChain.h, and the effects/ tree only arrives on spatcore's
# main line at a4e4e57 (PR #12, feature/effects-phase2).
#
# The adversarial review caught the superproject recording an OLDER pointer
# (ed60a80) while the working tree happened to have a4e4e57 checked out - so the
# harness built here and would have failed at the first #include on a fresh
# clone. Whoever commits tools/validation/effects-ab MUST commit the spatcore
# submodule bump in the same commit. Until then, fail here with the reason
# rather than 400 lines into a compiler error.
set(SPATCORE_DIR       "${REPO_ROOT}/spatcore")
set(SPATCORE_MIN_COMMIT "a4e4e57")

foreach(needed
        "effects/EffectChain.h"
        "effects/EffectModule.h"
        "effects/EffectParams.h"
        "effects/modules/MultitapDelayModule.h"
        "effects/modules/TremoloModule.h"
        "effects/modules/ModulationModule.h"
        "effects/modules/DistortionModule.h"
        "effects/modules/DynamicsModule.h"
        "effects/modules/BitcrusherModule.h")
    if (NOT EXISTS "${SPATCORE_DIR}/${needed}")
        message(FATAL_ERROR
            "effects-ab needs spatcore/${needed}, which is not there.\n"
            "The effects modules land on spatcore at ${SPATCORE_MIN_COMMIT} or later.\n"
            "If the submodule is behind that, from the superproject root:\n"
            "    git submodule update --init spatcore\n"
            "    git -C spatcore log --oneline -1        # must be ${SPATCORE_MIN_COMMIT} or a descendant\n"
            "If it still predates ${SPATCORE_MIN_COMMIT}, the superproject's recorded\n"
            "pointer is older than this tool needs: the commit that added\n"
            "tools/validation/effects-ab should have bumped it and did not.\n"
            "See the Requirements section of tools/validation/effects-ab/README.md.")
    endif()
endforeach()

add_subdirectory(${JUCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/juce EXCLUDE_FROM_ALL)

juce_add_console_app(ab-render PRODUCT_NAME "ab-render")

# spatcore/dsp/MultiChannelEQBank.h and the FDN reverb include <JuceHeader.h>
# indirectly through the juce module headers; generate one from the linked
# module set, exactly as offline-render does.
juce_generate_juce_header(ab-render)

target_sources(ab-render PRIVATE
    ab_render.cpp)

target_compile_definitions(ab-render PRIVATE
    REVERB_DIAGNOSTICS=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)

target_link_libraries(ab-render PRIVATE
    juce::juce_core
    juce::juce_events
    juce::juce_audio_basics
    juce::juce_audio_formats
    juce::juce_dsp
    juce::juce_recommended_config_flags)
