# Getting effect_name from the BG_PRODUCT_NAME
string(REGEX REPLACE "^(.*)_(.*)$" "\\1" effect_name "${PROJECT_NAME}")

# List of components included in the project.
set(components
    ${effect_name}_processor
)

# Find dependencies
find_package(os_utilities CONFIG)
find_package(processor_api CONFIG)
find_package(processor_utilities CONFIG)
find_package(GTest CONFIG)
if(APPLE)
    find_package(metal-cpp CONFIG)
else()
    find_package(hip_sdk CONFIG)
    find_package(CUDAToolkit ${CUDA_VERSION})
    set(CMAKE_CUDA_SEPARABLE_COMPILATION OFF)
    set(CMAKE_CUDA_RUNTIME_LIBRARY Static)
    set(CMAKE_CUDA_FLAGS "--relocatable-device-code=true -maxrregcount=${CUDA_MAXRREGCOUNT}")
endif()

# Component name.
set(component_id ${effect_name})
# Capitalize first letter: "wfs_input" -> "WfsInput"
string(SUBSTRING "${component_id}" 0 1 first_char)
string(TOUPPER "${first_char}" first_char_upper)
string(SUBSTRING "${component_id}" 1 -1 rest)
set(component_id_capitalized "${first_char_upper}${rest}")
string(TOUPPER "${component_id}" component_id_uppercase)
set(component_name ${component_id}_processor)

# Component type (library or executable).
set(component_type library)

if(APPLE)
    set(library_type MODULE)
else()
    set(library_type INTERFACE)
    set(nvidia_library_type MODULE)
    set(amd_library_type MODULE)
endif()

# target libraries
set(common_private_target_libraries
    os_utilities::os_utilities
    processor_api::processor_api
    processor_utilities::processor_utilities
)

set(device_common_private_target_libraries
    processor_utilities::processor_utilities
)

if(APPLE)
    set(private_target_libraries
        ${common_private_target_libraries}
    )
    set(device_metal_private_target_libraries
        ${device_common_private_target_libraries}
    )
else()
    set(nvidia_private_target_libraries
        ${common_private_target_libraries}
    )
    set(amd_private_target_libraries
        ${common_private_target_libraries}
    )
    set(device_nvidia_private_target_libraries
        ${device_common_private_target_libraries}
    )
    set(device_amd_private_target_libraries
        ${device_common_private_target_libraries}
    )
endif()

set(public_target_libraries
)

set(interface_target_libraries
)

# compile definitions
if(WIN32)
    set(win_common_private_compile_definitions
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        WIN32
        WIN64
    )
endif()

set(module_common_private_compile_definitions
    MODULE_EFFECT_NAME="${component_id_uppercase}"
    MODULE_ID="${component_id}"
    MODULE_MAJOR_VERSION=${${component_name}_MAJOR_VERSION}
    MODULE_MINOR_VERSION=${${component_name}_MINOR_VERSION}
    MODULE_PATCH_LEVEL=${${component_name}_PATCH_LEVEL}
)

set(common_private_compile_definitions
    ${win_common_private_compile_definitions}
    MODULE_IMPLEMENTATION
    ${module_common_private_compile_definitions}
)

if(APPLE)
    set(private_compile_definitions
        GPU_AUDIO_MAC
        ${common_private_compile_definitions}
    )
else()
    set(nvidia_private_compile_definitions
        ${common_private_compile_definitions}
    )
    set(amd_private_compile_definitions
        ${common_private_compile_definitions}
    )
endif()

if(NOT APPLE)
    set(common_device_private_compile_definitions
        $<$<CONFIG:Release>:N>GPU_PROFILING
    )
endif()

if(APPLE)
    set(device_metal_private_compile_definitions
        ${common_device_private_compile_definitions}
    )
else()
    set(device_nvidia_private_compile_definitions
        ${common_device_private_compile_definitions}
    )
    set(device_amd_private_compile_definitions
        ${common_device_private_compile_definitions}
    )
endif()

set(public_compile_definitions
)

set(interface_compile_definitions
)

# compile options
set(private_compile_options
)

set(public_compile_options
)

set(interface_compile_options
)

# link options
set(private_link_options
)

set(public_link_options
)

set(interface_link_options
)

# List of private include directories.
if(APPLE)
    set(private_include_directories
        ${common_include_directories}
        src/cuda
    )
    set(device_metal_private_include_directories
        src/cuda
    )
else()
    set(nvidia_private_include_directories
        ${common_include_directories}
        src/cuda
    )
    set(amd_private_include_directories
        ${common_include_directories}
        src/cuda
    )
    set(device_nvidia_private_include_directories
        src/cuda
    )
    set(device_amd_private_include_directories
        src/cuda
    )
endif()

# List of public include directories.
set(common_public_include_directories
    include
)

if(APPLE)
    set(public_include_directories
        ${common_public_include_directories}
    )
else()
    set(nvidia_public_include_directories
        ${common_public_include_directories}
    )
    set(amd_public_include_directories
        ${common_public_include_directories}
    )
endif()

# List of private header files.
set(common_private_headers
    include/wfs_input_buffer_processor/WfsInputSpecification.h
)

if(APPLE)
    set(private_headers
        ${common_private_headers}
    )
    set(device_metal_private_headers
        src/cuda/WfsInputProcessor.cuh
        src/cuda/Properties.h
    )
else()
    set(nvidia_private_headers
        ${common_private_headers}
    )
    set(amd_private_headers
        ${common_private_headers}
    )
    set(device_nvidia_private_headers
        src/cuda/WfsInputProcessor.cuh
        src/cuda/Properties.h
    )
    set(device_amd_private_headers
        src/cuda/WfsInputProcessor.cuh
        src/cuda/Properties.h
    )
endif()

# List of source files.
# NOTE: These will be created in a future step - placeholders for now
set(common_sources
    # src/WfsInputDeviceCodeProvider.cpp
    # src/WfsInputInputPort.cpp
    # src/WfsInputModule.cpp
    # src/WfsInputModuleInfoProvider.cpp
    # src/WfsInputModuleLibrary.cpp
    # src/WfsInputProcessor.cpp
)

if(APPLE)
    set(sources
        ${common_sources}
    )
    set(device_metal_sources
        # src/cuda/WfsInputProcessor.cu
    )
else()
    set(nvidia_sources
        ${common_sources}
    )
    set(amd_sources
        ${common_sources}
    )
    set(device_nvidia_sources
        # src/cuda/WfsInputProcessor.cu
    )
    set(device_amd_sources
        # src/cuda/WfsInputProcessor.cu
    )
endif()

# Tests - can be added later
set(common_test_private_compile_definitions
    ${win_common_private_compile_definitions}
)

set(common_test_headers
)

set(common_test_sources
)

if(APPLE)
    set(metal_test_private_compile_definitions
        ${common_test_private_compile_definitions}
    )
    set(metal_test_headers
        ${common_test_headers}
    )
    set(metal_test_sources
        ${common_test_sources}
    )
else()
    set(nvidia_test_private_compile_definitions
        ${common_test_private_compile_definitions}
    )
    set(amd_test_private_compile_definitions
        ${common_test_private_compile_definitions}
    )
    set(nvidia_test_headers
        ${common_test_headers}
    )
    set(amd_test_headers
        ${common_test_headers}
    )
    set(nvidia_test_sources
        ${common_test_sources}
    )
    set(amd_test_sources
        ${common_test_sources}
    )
endif()

set(common_test_private_target_libraries
    GTest::gtest_main
    os_utilities::os_utilities
    processor_api::processor_api
    processor_utilities::processor_utilities
)

if(APPLE)
    set(metal_test_private_target_libraries
        ${common_test_private_target_libraries}
    )
    set(metal_test_target_dependencies
        ${component_name}
    )
else()
    set(nvidia_test_private_target_libraries
        ${common_test_private_target_libraries}
    )
    set(amd_test_private_target_libraries
        ${common_test_private_target_libraries}
    )
    set(nvidia_test_target_dependencies
        ${component_name}_nvidia
    )
    set(amd_test_target_dependencies
        ${component_name}_amd
    )
endif()

