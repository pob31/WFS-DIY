cmake_minimum_required(VERSION 3.22)

project(WFS-DIY-Plugins VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "macOS deployment target")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal binary archs")
endif()

set(WFS_PLUGIN_COMPANY        "Pix et Bel")
set(WFS_PLUGIN_BUNDLE_PREFIX  "net.wfsdiy")
set(WFS_PLUGIN_MANUFACTURER   "PxBl")
set(WFS_PLUGIN_VERSION_STRING "0.0.1")

set(JUCE_DIR           "${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/JUCE")
set(JUCE_SIMPLEWEB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/juce_simpleweb")

add_subdirectory(${JUCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/juce EXCLUDE_FROM_ALL)
juce_add_module(${JUCE_SIMPLEWEB_DIR})

# juce_simpleweb's module header declares `OSXLibs: libssl,libcrypto,z`
# unconditionally. We build with SIMPLEWEB_SECURE_SUPPORTED=0 (see below), so
# the TLS code paths are compiled out and we don't want a runtime dep on
# libssl/libcrypto. Strip them from the target's interface link libs.
if(APPLE AND TARGET juce_simpleweb)
    get_target_property(_sw_libs juce_simpleweb INTERFACE_LINK_LIBRARIES)
    if(_sw_libs)
        list(REMOVE_ITEM _sw_libs libssl libcrypto)
        set_target_properties(juce_simpleweb PROPERTIES INTERFACE_LINK_LIBRARIES "${_sw_libs}")
    endif()
endif()

if(APPLE)
    set(WFS_PLUGIN_FORMATS VST3 AU Standalone)
elseif(WIN32)
    set(WFS_PLUGIN_FORMATS VST3 Standalone)
else()
    set(WFS_PLUGIN_FORMATS VST3 Standalone)
endif()

# ---------------------------------------------------------------------------
# Binary assets (logo etc.) — produced as a static lib with a BinaryData.h
# ---------------------------------------------------------------------------
juce_add_binary_data(WFSPluginBinaryData
    SOURCES
        ../Resources/WFS-DIY_logo.png)

# ---------------------------------------------------------------------------
# Shared plugin code (linked into every plugin target)
# ---------------------------------------------------------------------------
add_library(WFSPluginShared STATIC
    Source/Shared/RateLimiter.cpp
    Source/Shared/OscTransport.cpp
    Source/Shared/OscQueryClient.cpp
    Source/Shared/BridgeLoader.cpp
    Source/Shared/PluginLookAndFeel.cpp
)
target_include_directories(WFSPluginShared PUBLIC
    Source/Shared
    Source/JuceHeaderStub)
target_compile_features(WFSPluginShared PUBLIC cxx_std_17)
target_link_libraries(WFSPluginShared PUBLIC
    juce::juce_core
    juce::juce_cryptography
    juce::juce_events
    juce::juce_data_structures
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_osc
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce_simpleweb
    WFSPluginBinaryData)
target_compile_definitions(WFSPluginShared PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    # Disable juce_simpleweb's TLS/SSL paths — the WFS-DIY app speaks plain
    # HTTP/WebSocket (http://, ws://) so we don't need OpenSSL. Leaving this
    # on pulls libcrypto-1_1-x64.dll as a runtime dependency, which most
    # hosts (Reaper, Live Professor, Max) can't resolve at plugin load.
    SIMPLEWEB_SECURE_SUPPORTED=0)

# ---------------------------------------------------------------------------
# Bridge shared library (process-wide singleton across plugin modules)
# ---------------------------------------------------------------------------
add_library(WFSPluginBridge SHARED
    Source/Bridge/Bridge.cpp
)
target_include_directories(WFSPluginBridge PUBLIC Source/Shared)
set_target_properties(WFSPluginBridge PROPERTIES
    OUTPUT_NAME "WFS-DIY-PluginBridge"
    POSITION_INDEPENDENT_CODE ON)
if(APPLE)
    set_target_properties(WFSPluginBridge PROPERTIES
        INSTALL_NAME_DIR "@loader_path")
endif()

# ---------------------------------------------------------------------------
# Helper: declare one plugin target
# ---------------------------------------------------------------------------
function(wfs_add_plugin TARGET_NAME PLUGIN_NAME PLUGIN_CODE IS_SYNTH MAIN_SOURCE)
    juce_add_plugin(${TARGET_NAME}
        COMPANY_NAME                "${WFS_PLUGIN_COMPANY}"
        BUNDLE_ID                   "${WFS_PLUGIN_BUNDLE_PREFIX}.${TARGET_NAME}"
        PLUGIN_MANUFACTURER_CODE    "${WFS_PLUGIN_MANUFACTURER}"
        PLUGIN_CODE                 "${PLUGIN_CODE}"
        FORMATS                     ${WFS_PLUGIN_FORMATS}
        PRODUCT_NAME                "${PLUGIN_NAME}"
        IS_SYNTH                    ${IS_SYNTH}
        NEEDS_MIDI_INPUT            FALSE
        NEEDS_MIDI_OUTPUT           FALSE
        VERSION                     "${WFS_PLUGIN_VERSION_STRING}"
        COPY_PLUGIN_AFTER_BUILD     FALSE
        VST3_CATEGORIES             "Fx" "Spatial"
        AU_MAIN_TYPE                "kAudioUnitType_Effect"
        PLUGIN_DESCRIPTION          "WFS-DIY DAW-side controller for the WFS-DIY spatial audio app"
        DESCRIPTION                 "${PLUGIN_NAME}")

    target_sources(${TARGET_NAME} PRIVATE ${MAIN_SOURCE})
    target_compile_features(${TARGET_NAME} PRIVATE cxx_std_17)
    target_link_libraries(${TARGET_NAME} PRIVATE
        WFSPluginShared
        juce::juce_audio_utils
        juce::juce_gui_basics
        juce::juce_gui_extra)
    target_compile_definitions(${TARGET_NAME} PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)
endfunction()

# ---------------------------------------------------------------------------
# Master plugin
# ---------------------------------------------------------------------------
wfs_add_plugin(WFSPluginMaster
    "WFS-DIY Master"
    "WfMa"
    FALSE
    Source/Master/MasterMain.cpp)
target_sources(WFSPluginMaster PRIVATE
    Source/Master/MasterProcessor.cpp
    Source/Master/MasterEditor.cpp)
target_include_directories(WFSPluginMaster PRIVATE Source/Master)

# ---------------------------------------------------------------------------
# Track plugin variants (share Track/ code, differ only in variant main file)
# ---------------------------------------------------------------------------
function(wfs_add_track_variant TARGET_NAME PLUGIN_NAME PLUGIN_CODE VARIANT_MAIN)
    wfs_add_plugin(${TARGET_NAME} ${PLUGIN_NAME} ${PLUGIN_CODE} FALSE ${VARIANT_MAIN})
    target_sources(${TARGET_NAME} PRIVATE
        Source/Track/TrackProcessor.cpp
        Source/Track/TrackEditor.cpp)
    target_include_directories(${TARGET_NAME} PRIVATE
        Source/Track
        Source/Variants)
endfunction()

wfs_add_track_variant(WFSPluginTrackCart
    "WFS-DIY Track - Cartesian"    "WfTC"
    Source/Variants/CartesianMain.cpp)

wfs_add_track_variant(WFSPluginTrackCyl
    "WFS-DIY Track - Cylindrical"  "WfTY"
    Source/Variants/CylindricalMain.cpp)

wfs_add_track_variant(WFSPluginTrackSph
    "WFS-DIY Track - Spherical"    "WfTS"
    Source/Variants/SphericalMain.cpp)

wfs_add_track_variant(WFSPluginTrackAdmC
    "WFS-DIY Track - ADM Cartesian" "WfAC"
    Source/Variants/AdmCartesianMain.cpp)

wfs_add_track_variant(WFSPluginTrackAdmP
    "WFS-DIY Track - ADM Polar"    "WfAP"
    Source/Variants/AdmPolarMain.cpp)

# ---------------------------------------------------------------------------
# Convenience: build-all target
# ---------------------------------------------------------------------------
add_custom_target(WFSPluginsAll
    DEPENDS
        WFSPluginBridge
        WFSPluginMaster_All
        WFSPluginTrackCart_All
        WFSPluginTrackCyl_All
        WFSPluginTrackSph_All
        WFSPluginTrackAdmC_All
        WFSPluginTrackAdmP_All)
