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
# ONNX model), objdetect (cv::FaceDetectorYN — anchors/NMS/landmarks).
#
# Two ways to get OpenCV:
#  - default: find_package against a prebuilt/system OpenCV (Windows official
#    prebuilt = single opencv_world target, hence no COMPONENTS request —
#    a per-component check would fail there while distro/brew builds pass).
#  - WFS_HEADTRACK_BUNDLED_OPENCV=ON: FetchContent the OpenCV sources and build
#    a minimal STATIC OpenCV into the plugin. This is the release-packaging
#    path for Linux/macOS: the result is one self-contained shared library with
#    no OpenCV runtime to bundle, fix up (macOS install names) or codesign
#    separately. Camera backends only (V4L2 / AVFoundation / MSMF) — ffmpeg,
#    GTK/Qt and IPP are disabled: this plugin reads a webcam, not video files.
option(WFS_HEADTRACK_BUNDLED_OPENCV
       "Build a minimal static OpenCV from source into the plugin (release packaging)" OFF)

if(WFS_HEADTRACK_BUNDLED_OPENCV)
    include(FetchContent)
    set(BUILD_SHARED_LIBS OFF)
    # BUILD_LIST resolves module dependencies itself (core/imgproc/flann/... are
    # pulled in as needed by the listed leaves).
    set(BUILD_LIST "videoio,dnn,objdetect" CACHE STRING "" FORCE)
    set(BUILD_opencv_apps OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_PERF_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_JAVA OFF CACHE BOOL "" FORCE)
    set(BUILD_opencv_python3 OFF CACHE BOOL "" FORCE)
    # OpenCV static defaults to the static CRT (/MT) on MSVC, which conflicts
    # with this plugin's /MD (LNK2038 RuntimeLibrary mismatch). Keep everything
    # on the dynamic CRT — the repo-wide convention.
    set(BUILD_WITH_STATIC_CRT OFF CACHE BOOL "" FORCE)
    set(WITH_FFMPEG OFF CACHE BOOL "" FORCE)
    set(WITH_GSTREAMER OFF CACHE BOOL "" FORCE)
    set(WITH_GTK OFF CACHE BOOL "" FORCE)
    set(WITH_QT OFF CACHE BOOL "" FORCE)
    set(WITH_IPP OFF CACHE BOOL "" FORCE)
    set(WITH_OPENEXR OFF CACHE BOOL "" FORCE)
    set(WITH_TIFF OFF CACHE BOOL "" FORCE)
    set(WITH_WEBP OFF CACHE BOOL "" FORCE)
    set(WITH_OPENJPEG OFF CACHE BOOL "" FORCE)
    set(WITH_JASPER OFF CACHE BOOL "" FORCE)
    set(OPENCV_ENABLE_NONFREE OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(opencv
        URL https://github.com/opencv/opencv/archive/refs/tags/4.10.0.tar.gz
        URL_HASH SHA256=b2171af5be6b26f7a06b1229948bbb2bdaa74fcf5cd097e0af6378fce50a6eb9)
    FetchContent_MakeAvailable(opencv)

    set(OpenCV_LIBS opencv_core opencv_imgproc opencv_videoio opencv_dnn opencv_objdetect)
    set(OpenCV_VERSION "4.10.0-static")

    # In-tree module targets do not export their include dirs to outside
    # consumers; collect them plus the generated config headers explicitly.
    set(WFS_HEADTRACK_OPENCV_INCLUDES "${OPENCV_CONFIG_FILE_INCLUDE_DIR}")
    foreach(m core imgproc videoio dnn objdetect)
        list(APPEND WFS_HEADTRACK_OPENCV_INCLUDES "${OPENCV_MODULE_opencv_${m}_LOCATION}/include")
    endforeach()
else()
    find_package(OpenCV REQUIRED)
endif()
message(STATUS "wfs_headtrack: OpenCV ${OpenCV_VERSION}")

add_library(wfs_headtrack SHARED src/HeadTrackPlugin.cpp)

target_include_directories(wfs_headtrack PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../spatcore/binaural/plugin")
if(WFS_HEADTRACK_BUNDLED_OPENCV)
    target_include_directories(wfs_headtrack PRIVATE ${WFS_HEADTRACK_OPENCV_INCLUDES})
endif()

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
