# wfs_headtrack — webcam head-tracking plugin for WFS-DIY.
#
# Built OUT OF TREE, exactly like the GPU vendor plugins: the app dlopens the
# result through the C ABI in spatcore/binaural/plugin/HeadTrackPluginApi.h and
# links none of this. That is what keeps OpenCV out of the app build.
#
# Configure/build (from the repo root):
#   cmake -S tools/headtrack -B tools/headtrack/build -DOpenCV_DIR=<opencv>/build
#   cmake --build tools/headtrack/build --config Release
# or just run tools/headtrack/build-headtrack-plugin.ps1 / .sh, which also
# fetches a prebuilt OpenCV and stages the artefacts next to the app.
#
# Output: wfs_headtrack.dll / libwfs_headtrack.so / libwfs_headtrack.dylib,
# following spatcore/gpu/PlatformDynLib.h's pluginName() convention, plus the
# YuNet model file which the plugin loads from beside itself.

cmake_minimum_required(VERSION 3.22)
project(wfs_headtrack VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# Modules used: core (Mat), imgproc, videoio (capture), dnn (runs the YuNet
# ONNX model), objdetect (cv::FaceDetectorYN — anchors/NMS/landmarks), calib3d
# (cv::solvePnP). No COMPONENTS request: the official Windows prebuilt ships a
# single opencv_world target and would fail a per-component check, while
# distro/brew builds expose them individually. We verify by compiling instead.
find_package(OpenCV REQUIRED)
message(STATUS "wfs_headtrack: OpenCV ${OpenCV_VERSION} from ${OpenCV_DIR}")

add_library(wfs_headtrack SHARED src/HeadTrackPlugin.cpp)

target_include_directories(wfs_headtrack PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../spatcore/binaural/plugin")

target_link_libraries(wfs_headtrack PRIVATE ${OpenCV_LIBS})

# No "lib" prefix on Windows; PlatformDynLib::pluginName() adds it elsewhere.
set_target_properties(wfs_headtrack PROPERTIES
    OUTPUT_NAME "wfs_headtrack"
    CXX_VISIBILITY_PRESET hidden)

if(MSVC)
    # /O2 only outside Debug: it conflicts with Debug's default /RTC1.
    target_compile_options(wfs_headtrack PRIVATE $<$<NOT:$<CONFIG:Debug>>:/O2> /EHsc)
    target_compile_definitions(wfs_headtrack PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
else()
    target_compile_options(wfs_headtrack PRIVATE $<$<NOT:$<CONFIG:Debug>>:-O2>)
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(wfs_headtrack PRIVATE dl)
endif()

# The plugin loads the model from beside itself — keep them together.
add_custom_command(TARGET wfs_headtrack POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/models/face_detection_yunet_2023mar.onnx"
            "$<TARGET_FILE_DIR:wfs_headtrack>/face_detection_yunet_2023mar.onnx")

# Standalone smoke host: dlopens the plugin exactly as the app does, prints the
# pose stream. This is how the angle signs get validated (shake / nod / tilt).
add_executable(test-headtrack-plugin test-headtrack-plugin.cpp)
target_include_directories(test-headtrack-plugin PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../spatcore/binaural/plugin")
if(UNIX)
    target_link_libraries(test-headtrack-plugin PRIVATE dl)
endif()
add_dependencies(test-headtrack-plugin wfs_headtrack)
