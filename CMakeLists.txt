cmake_minimum_required(VERSION 3.20)
project(SkinFlaps LANGUAGES CXX C)

# --- Basic Setup -----------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Platform Detection ----------------------------------------------------
if(WIN32)
    set(PLATFORM_WINDOWS 1)
    add_definitions(-DWIN32)
elseif(APPLE)
    set(PLATFORM_MACOS 1)
    # Fix for macOS systems where C++ stdlib headers aren't in default search path
    include_directories(SYSTEM /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1)
endif()

# --- Find Dependencies -----------------------------------------------------
find_package(glfw3 REQUIRED)

# --- Add Subdirectories ----------------------------------------------------
add_subdirectory(gl3wGraphics)
add_subdirectory(PhysBAM_subset)
add_subdirectory(simd-numeric-kernels-new)
add_subdirectory(PDTetPhysics)
add_subdirectory(imgui_glfw_nfd_lib)
add_subdirectory(SkinFlaps)

# --- Final Executable ------------------------------------------------------
# This will be defined in the SkinFlaps/CMakeLists.txt
# and linked against the libraries from the subdirectories.