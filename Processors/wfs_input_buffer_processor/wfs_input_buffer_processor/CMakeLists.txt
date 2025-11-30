# Include and apply custom component variables.
include(CMakeLists.var.cmake)

# This processor uses the GPU Audio SDK's build system.
# The BG_AddComponent() macro is provided by the SDK's braingines.cmake.
# For standalone builds, ensure the SDK's processor_cmake directory is in CMAKE_MODULE_PATH.
BG_AddComponent()

