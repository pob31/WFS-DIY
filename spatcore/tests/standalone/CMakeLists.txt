# Standalone build of the spatcore unit tests from inside the WFS-DIY repo.
#
# This is the entry point CI/dev configures TODAY: it supplies the JUCE
# checkout and the vendored juce_simpleweb module from the host repo's
# ThirdParty/, then adds spatcore with SPATCORE_STANDALONE_TESTS=ON.
# After the Phase 6 repo split, the spatcore repository's own top-level
# CMakeLists (with its own JUCE pins) replaces this shim.
#
# Configure/build/run (Windows, VS-bundled cmake, from the repo root):
#   "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" ^
#         -S spatcore/tests/standalone -B spatcore/tests/standalone/build ^
#         -G "Visual Studio 18 2026"
#   cmake --build spatcore/tests/standalone/build --config Release
#   spatcore\tests\standalone\build\spatcore\tests\spatcore-tests_artefacts\Release\spatcore-tests.exe
# (exit code 0 = all tests passed)

cmake_minimum_required(VERSION 3.22)

project(spatcore-standalone-tests VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")

# Consumer side of spatcore's consumer-provides-JUCE contract (same pattern
# as Plugin/CMakeLists.txt): JUCE first, then the vendored simpleweb module.
add_subdirectory("${REPO_ROOT}/ThirdParty/JUCE" "${CMAKE_CURRENT_BINARY_DIR}/juce" EXCLUDE_FROM_ALL)
juce_add_module("${REPO_ROOT}/ThirdParty/juce_simpleweb")

# juce_simpleweb's .cpps include a Projucer-style "JuceHeader.h"; give every
# target that compiles the module sources this minimal stub (Plugin/
# CMakeLists.txt does the same via Source/JuceHeaderStub).
target_include_directories(juce_simpleweb INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/JuceHeaderStub")

# The module header declares `windowsLibs: libssl,libcrypto` / `OSXLibs: ...`
# unconditionally, but spatcore-control builds it with
# SIMPLEWEB_SECURE_SUPPORTED=0 (plain HTTP/WS - no TLS code paths, no
# OpenSSL). Strip the never-referenced import libs from the module's link
# interface so consumers don't need an OpenSSL SDK (Plugin/CMakeLists.txt
# does the same for macOS).
get_target_property(_sw_libs juce_simpleweb INTERFACE_LINK_LIBRARIES)
if(_sw_libs)
    list(REMOVE_ITEM _sw_libs libssl libcrypto z)
    set_target_properties(juce_simpleweb PROPERTIES INTERFACE_LINK_LIBRARIES "${_sw_libs}")
endif()

# Consumer side of spatcore-controllers' contracts: the ROLI BLOCKS module
# for the Lightpad driver, and the vendored hidapi HEADERS for the HID
# drivers (headers only - hid_* impls live in the final app, so the test
# build never links a HID backend).
juce_add_module("${REPO_ROOT}/ThirdParty/roli_blocks_basics")
set(SPATCORE_HIDAPI_INCLUDE_DIR "${REPO_ROOT}/ThirdParty/hidapi")

set(SPATCORE_STANDALONE_TESTS ON)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../.." "${CMAKE_CURRENT_BINARY_DIR}/spatcore")
