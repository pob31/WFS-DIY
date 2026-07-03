# spatcore — shared real-time spatial-audio core extracted from WFS-DIY.
# Design: docs/architecture/core-boundary-proposal-audio.md (§3 dependency
# rules) + core-boundary-proposal-control.md.
#
# Targets
#   spatcore-audio    STATIC  rt/ dsp/ wfs/ reverb/ gpu/   (header-only from
#                             the consumer's point of view on Windows/Linux;
#                             the gpu backend .cpps self-guard to empty TUs —
#                             see SpatcoreAudioCompileCheck.cpp; Metal .mm
#                             carries real code on Apple)
#   spatcore-control  STATIC  control/{osc,state,mcp}
#
# Consumer-provides-JUCE contract: this file does NOT add_subdirectory(JUCE).
# The consumer must make the juce::* module targets available BEFORE adding
# spatcore (add_subdirectory of a JUCE checkout, or find_package(JUCE)), and
# must juce_add_module() the vendored juce_simpleweb module. See
# tests/standalone/CMakeLists.txt for the reference wiring used inside the
# WFS-DIY repo (ThirdParty/JUCE + ThirdParty/juce_simpleweb).
#
# The WFS-DIY app itself does NOT use these targets: it stays Projucer-built
# and compiles the spatcore sources directly (same compiler, same flags —
# zero bit-exact risk). These targets serve spatcore's own tests and the
# future CMake-native consumers (XOA, Tight-WFS).

cmake_minimum_required(VERSION 3.22)

project(spatcore VERSION 0.1.0 LANGUAGES C CXX)

# Metal backends (.mm) only exist on Apple.
if(APPLE)
    enable_language(OBJCXX)
endif()

option(SPATCORE_STANDALONE_TESTS
    "Build spatcore's own unit-test console app (tests/)" OFF)

# ---------------------------------------------------------------------------
# Consumer-provides-JUCE contract
# ---------------------------------------------------------------------------
if(NOT TARGET juce::juce_core)
    message(FATAL_ERROR
        "spatcore: no juce::juce_core target. The consumer provides JUCE: "
        "add_subdirectory(<JUCE checkout>) (or find_package(JUCE)) BEFORE "
        "add_subdirectory(spatcore). Inside the WFS-DIY repo, build via "
        "spatcore/tests/standalone/CMakeLists.txt, which wires up "
        "ThirdParty/JUCE.")
endif()

if(NOT TARGET juce_simpleweb)
    message(FATAL_ERROR
        "spatcore: no juce_simpleweb target (needed by spatcore-control's "
        "MCPTransport). The consumer must register the vendored module with "
        "juce_add_module(<path>/juce_simpleweb) after adding JUCE. Inside "
        "the WFS-DIY repo the module lives at ThirdParty/juce_simpleweb; see "
        "spatcore/tests/standalone/CMakeLists.txt or Plugin/CMakeLists.txt.")
endif()

include("${CMAKE_CURRENT_LIST_DIR}/cmake/SpatcoreCompileFlags.cmake")

# Public include root: the directory CONTAINING spatcore/, so consumer code
# includes headers exactly like the app does: "spatcore/rt/RtSnapshot.h".
# Until the Phase 6 repo split this is the WFS-DIY repo root; the dependency
# lint (tools/validation/spatcore_dep_lint.py) enforces that no spatcore file
# reaches back into the app tree through it. After the split it is the
# submodule mount parent.
get_filename_component(SPATCORE_INCLUDE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

# ---------------------------------------------------------------------------
# spatcore-audio
# ---------------------------------------------------------------------------
# The Cuda*/Hip* .cpps are in every consumer's TU list exactly like in the
# app's vcxproj: they self-guard on WFS_GPU_NATIVE / WFS_GPU_PLUGINS /
# WFS_GPU_HIP and compile to empty TUs here (vendor code is compiled only
# into the per-vendor dlopen plugins by tools/gpu/build-gpu-plugins.*).
# gpu/plugin/GpuVendorPlugin.cpp is deliberately NOT listed — it is the
# plugin entry-point TU, compile="0" in the app project too.
add_library(spatcore-audio STATIC
    SpatcoreAudioCompileCheck.cpp
    gpu/CudaWfsBackend.cpp
    gpu/CudaObBackend.cpp
    gpu/CudaIrBackend.cpp
    gpu/CudaFdnBackend.cpp
    gpu/CudaSdnBackend.cpp
    gpu/HipWfsBackend.cpp
    gpu/HipObBackend.cpp
    gpu/HipIrBackend.cpp
    gpu/HipFdnBackend.cpp
    gpu/HipSdnBackend.cpp
)

if(APPLE)
    target_sources(spatcore-audio PRIVATE
        gpu/MetalWfsBackend.mm
        gpu/MetalObBackend.mm
        gpu/MetalIrBackend.mm
        gpu/MetalFdnBackend.mm
        gpu/MetalSdnBackend.mm
    )
endif()

target_include_directories(spatcore-audio PUBLIC "${SPATCORE_INCLUDE_ROOT}")

# GPU build-flag surface, mirroring the app's exporters (WFS-DIY.jucer):
#   Windows/Linux: WFS_GPU_NATIVE=1 WFS_GPU_PLUGINS=1 (backends dlopen'd from
#                  per-vendor plugin DLLs; no GPU runtime linked here)
#   macOS:         WFS_GPU_NATIVE=1 (Metal backend compiled in-process)
# PUBLIC on purpose: consumer TUs that include spatcore headers must see the
# same preprocessor state as the library (core-boundary doc §5.1 R6).
if(APPLE)
    target_compile_definitions(spatcore-audio PUBLIC WFS_GPU_NATIVE=1)
else()
    target_compile_definitions(spatcore-audio PUBLIC WFS_GPU_NATIVE=1 WFS_GPU_PLUGINS=1)
endif()

# JUCE module deps as measured from spatcore includes (2026-07):
#   juce_audio_basics  rt/dsp/wfs/reverb buffers (AudioBuffer, jmin/jmax, ...)
#   juce_dsp           reverb IR convolution FFT (ReverbIRAlgorithm[GPU].h)
#   juce_audio_formats reverb/ReverbEngine.h IR file loading
#                      (AudioFormatManager/Reader; the header relies on the
#                      includer providing the module header first - hygiene
#                      pass pending, see SpatcoreAudioCompileCheck.cpp)
#   juce_events        rt/ReverbDiagnostics.h
#   juce_core          rt/RtSnapshot.h (SpinLock) + transitive base
# No juce_osc on the audio side.
target_link_libraries(spatcore-audio PUBLIC
    juce::juce_audio_basics
    juce::juce_dsp
    juce::juce_audio_formats
    juce::juce_events
    juce::juce_core)

# Consumers may be shared objects (plugin formats); PIC is free on Win/mac
# and required on Linux (see Plugin/CMakeLists.txt for the ld error text).
set_target_properties(spatcore-audio PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(MSVC)
    # The self-guarded gpu TUs are empty on Windows by design; silence the
    # librarian's LNK4221 "no public symbols" warning for them.
    set_target_properties(spatcore-audio PROPERTIES STATIC_LIBRARY_OPTIONS "/IGNORE:4221")
endif()

spatcore_apply_compile_flags(spatcore-audio)

# ---------------------------------------------------------------------------
# spatcore-control
# ---------------------------------------------------------------------------
add_library(spatcore-control STATIC
    control/osc/OSCIngestQueue.cpp
    control/osc/OSCRateLimiter.cpp
    control/osc/OSCReceiverWithSenderIP.cpp
    control/osc/OSCTCPReceiver.cpp
    control/osc/TrackingIngestQueue.cpp
    control/state/TreeParameterStore.cpp
    control/state/XmlPersistence.cpp
    control/mcp/MCPChangeRecords.cpp
    control/mcp/MCPDispatcher.cpp
    control/mcp/MCPPromptRegistry.cpp
    control/mcp/MCPResourceRegistry.cpp
    control/mcp/MCPTierEnforcement.cpp
    control/mcp/MCPToolRegistry.cpp
    control/mcp/MCPTransport.cpp
)

target_include_directories(spatcore-control PUBLIC "${SPATCORE_INCLUDE_ROOT}")

# Disable juce_simpleweb's TLS/SSL paths, mirroring the app + DAW plugins:
# the MCP transport speaks plain HTTP/WebSocket, and leaving TLS on adds an
# OpenSSL runtime dependency (see Plugin/CMakeLists.txt for the full story).
# PUBLIC because the juce_simpleweb module sources are compiled into whichever
# target links them — every TU must agree on this flag.
target_compile_definitions(spatcore-control PUBLIC SIMPLEWEB_SECURE_SUPPORTED=0)

# JUCE module deps as measured from spatcore includes (2026-07):
#   juce_core             all of control/
#   juce_osc              osc/OSC{RateLimiter,ReceiverWithSenderIP,TCPReceiver}.h
#                         (+ OSCParser/OSCSerializer use juce::OSC* types)
#   juce_events           mcp/MCP{Dispatcher.cpp,TierEnforcement.h}, osc/OSCIngestQueue.h
#   juce_data_structures  state/{TreeParameterStore,XmlPersistence}.h, osc/TrackingIngestQueue.h
#   juce_simpleweb        mcp/MCPTransport.h (HTTP/WebSocket MCP transport)
#   juce_cryptography     included unconditionally by juce_simpleweb.h
#                         (WSCrypto websocket handshake), which declares no
#                         module dependencies of its own
target_link_libraries(spatcore-control PUBLIC
    juce::juce_core
    juce::juce_osc
    juce::juce_events
    juce::juce_data_structures
    juce::juce_cryptography
    juce_simpleweb)

set_target_properties(spatcore-control PROPERTIES POSITION_INDEPENDENT_CODE ON)

spatcore_apply_compile_flags(spatcore-control)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(SPATCORE_STANDALONE_TESTS)
    add_subdirectory(tests)
endif()
