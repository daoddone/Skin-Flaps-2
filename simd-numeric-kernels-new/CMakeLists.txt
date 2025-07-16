cmake_minimum_required(VERSION 3.20)
set (CMAKE_CXX_STANDARD 11)

project(simd_numeric-kernels)
find_package(Eigen3 CONFIG)

# add_subdirectory(Tests) # Temporarily disabled

# --- Collect Source Files --------------------------------------------------
# For the initial macOS port, we use the non-optimized reference implementations.
# The optimized SIMD kernels will be added later with platform-specific flags.

file(GLOB_RECURSE SIMD_REF_SOURCES
    "References/Matrix_Times_Matrix/Matrix_Times_Matrix_Reference.cpp"
    "References/Matrix_Times_Transpose/Matrix_Times_Transpose_Reference.cpp"
    "References/Singular_Value_Decomposition/Singular_Value_Decomposition_Reference.cpp"
)

file(GLOB_RECURSE SIMD_KERNEL_SOURCES
    "Kernels/Matrix_Times_Matrix/Matrix_Times_Matrix.cpp"
    "Kernels/Matrix_Times_Transpose/Matrix_Times_Transpose.cpp"
    "Kernels/Singular_Value_Decomposition/Singular_Value_Decomposition.cpp"
)

# --- Library Definition ----------------------------------------------------
add_library(simd-numeric-kernels-new
    ${SIMD_REF_SOURCES}
    ${SIMD_KERNEL_SOURCES}
)

# --- Include Directories -------------------------------------------------
target_include_directories(simd-numeric-kernels-new PUBLIC
    "Common"
    "Kernels"
    "References"
)

# --- Find Dependencies ---------------------------------------------------
find_package(Eigen3 REQUIRED)

# --- Link Libraries ------------------------------------------------------
target_link_libraries(simd-numeric-kernels-new PUBLIC Eigen3::Eigen)

# --- Compiler Definitions for macOS Port -----------------------------------
if(APPLE)
    target_compile_definitions(simd-numeric-kernels-new PUBLIC USE_REFERENCE_IMPLEMENTATION)
endif()
