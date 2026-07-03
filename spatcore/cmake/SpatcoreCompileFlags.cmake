# SpatcoreCompileFlags.cmake — the WFS-DIY app's optimization / floating-point
# compile surface, transcribed from the app's Visual Studio project
# (Builds/VisualStudio2022/WFS-DIY_App.vcxproj) so CMake consumers of spatcore
# can compile the core exactly like the first (Projucer-built) consumer.
# Freezing the FP flags on the spatcore targets is a hard requirement of the
# extraction: docs/architecture/core-boundary-proposal-audio.md §5.1 R7
# (denormals / recursive-biquad output must not drift between builds).
#
# Transcription, Release|x64 <ClCompile> of WFS-DIY_App.vcxproj:
#   <Optimization>Full</Optimization>                   ->  /Ox
#   (no <FloatingPointModel> element)                   ->  MSVC default /fp:precise
#                                                           (made explicit below)
#   <LanguageStandard>stdcpp17</LanguageStandard>       ->  /std:c++17
#   <RuntimeLibrary>MultiThreadedDLL</RuntimeLibrary>   ->  /MD   (Debug config uses
#       MultiThreadedDebugDLL -> /MDd; both match CMake's MSVC defaults, so the
#       runtime is NOT overridden here)
#   <RuntimeTypeInfo>true</RuntimeTypeInfo>             ->  /GR   (MSVC default; not set)
#   <WarningLevel>Level4</WarningLevel>                 ->  /W4
#   <MultiProcessorCompilation>true</...>               ->  /MP
#   <AdditionalOptions>/bigobj ...</AdditionalOptions>  ->  /bigobj
#   <WholeProgramOptimization>true</...> (Release)      ->  /GL + /LTCG, expressed as
#       INTERPROCEDURAL_OPTIMIZATION_RELEASE so CMake also passes /LTCG to the
#       librarian / linker as appropriate
#   <PreprocessorDefinitions>_CRT_SECURE_NO_WARNINGS;...
#   <DebugInformationFormat>ProgramDatabase</...>       ->  /Zi (debug info only,
#       no codegen effect; left to CMake's per-config defaults)
# Debug|x64 <ClCompile>: <Optimization>Disabled</Optimization> -> /Od, which is
# CMake's Debug default; nothing to transcribe.
#
# GCC/Clang (the app's LinuxMakefile / Xcode exporters) use the Projucer
# release defaults (-O3, no -ffast-math), which match CMake's own Release
# defaults — no extra flags are pinned for those toolchains yet. Transcribe
# from the respective exporter if a non-MSVC consumer ever needs to be
# bit-exact against a Projucer build of the app.

# Usage: spatcore_apply_compile_flags(<target>)
# Applied to spatcore-audio / spatcore-control by spatcore/CMakeLists.txt.
# Consumers that want their own TUs compiled exactly like the app (matters for
# anything on the audio path, because spatcore's DSP is header-only and thus
# compiled under the CONSUMER's flags) can call it on their targets too.
function(spatcore_apply_compile_flags target)
    target_compile_features(${target} PUBLIC cxx_std_17)
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)

    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4                             # WarningLevel Level4
            /MP                             # MultiProcessorCompilation
            /bigobj                         # AdditionalOptions (also in the .jucer VS exporter)
            /fp:precise                     # app leaves MSVC's default; pinned explicitly
            $<$<CONFIG:Release>:/Ox>        # Optimization=Full (overrides CMake's /O2 default)
        )
        target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
        # WholeProgramOptimization=true on the app's Release config.
        set_target_properties(${target} PROPERTIES
            INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    endif()
endfunction()
