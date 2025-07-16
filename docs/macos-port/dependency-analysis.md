# Dependency Analysis for macOS Port

This document tracks all platform-specific dependencies that need to be addressed to port the SkinFlaps application to macOS on Apple Silicon.

## 1. Intel AVX Instructions

The physics engine heavily relies on Intel Advanced Vector Extensions (AVX/AVX2) for SIMD (Single Instruction, Multiple Data) operations. These are not available on ARM-based Apple Silicon processors and must be replaced with equivalent ARM NEON intrinsics.

### Files with AVX Dependencies:

- **`simd-numeric-kernels-new/Common/arch/x86_64/Mask.AVX2.h`**: Implements SIMD mask operations. Uses `_mm256_` intrinsics for bitwise AND, OR, XOR, etc., on 256-bit vectors.
- **`simd-numeric-kernels-new/Common/arch/x86_64/Discrete.AVX2.h`**: Implements SIMD integer arithmetic. Uses `_mm256_` intrinsics for integer addition, subtraction, multiplication, division, and comparisons.
- **`simd-numeric-kernels-new/Common/arch/x86_64/Number.AVX2.h`**: Implements SIMD floating-point (single and double precision) arithmetic. Uses `_mm256_` intrinsics for arithmetic operations, comparisons, sqrt, rsqrt, and more.

### Porting Strategy:

The `simd-numeric-kernels-new/References/` directory contains non-optimized, scalar reference implementations. For Phase 3, the build system will be configured to use these reference implementations on macOS to achieve initial functionality.

For Phase 4 (Optimization), new files will be created under a `simd-numeric-kernels-new/Common/arch/arm64/` (or similar) directory to provide ARM NEON-based implementations of the `Mask`, `Discrete`, and `Number` classes.

## 2. Intel Math Kernel Library (MKL)

The project uses the Intel MKL for high-performance math routines, particularly for linear algebra (BLAS, LAPACK). The dependency is managed at the build/linking level, not through direct `mkl_*` function calls in the application code.

### Files with MKL Dependencies:

- **`Build/msvc_2022/*.vcxproj`**: These Visual Studio project files link directly to MKL libraries (`mkl_rt.lib`, `mkl_core.lib`, etc.). These files will be replaced by the new CMake build system.
- **`PDTetPhysics/cmake/FindMKL.cmake`**: This CMake module contains logic to find the MKL library on the system.
- **`PDTetPhysics/CMakeLists.txt`**: This file links the `PDTetPhysics` library against MKL.

### Porting Strategy:

The dependency on MKL will be replaced with Apple's **Accelerate framework** on macOS. The Accelerate framework provides highly optimized implementations of BLAS and LAPACK specifically for Apple hardware.

The new root `CMakeLists.txt` will need to contain logic to:
1. Detect if the build target is macOS.
2. If it is, link against the Accelerate framework.
3. If it's Windows/Linux, maintain the existing logic to link against MKL.

## 3. Intel Threading Building Blocks (TBB)

The project uses TBB for multi-threaded, parallel processing to accelerate computationally expensive tasks like mesh cutting.

### Files with TBB Dependencies:

- **`SkinFlaps/src/vnBccTetCutter_tbb.h`** and **`vnBccTetCutter_tbb.cpp`**: This class uses TBB's concurrent data structures (`concurrent_hash_map`, `concurrent_vector`) and parallel algorithms (`parallel_for`) to speed up tetrahedral mesh generation.
- **`Build/msvc_2022/*.vcxproj`**: Visual Studio project files enable TBB and specify include/library paths.
- **`PDTetPhysics/cmake/FindTBB.cmake`**: The CMake build system for the physics library has a module to find and link TBB, often as a dependency for MKL's multi-threaded mode.

### Porting Strategy:

Intel's oneAPI TBB is cross-platform and available for macOS. The initial porting strategy will be to:
1.  Install TBB on the macOS development environment (e.g., via `brew install tbb`).
2.  Update the new root CMakeLists.txt to find and link against TBB on all platforms (Windows, Linux, and macOS).

This avoids a major refactoring effort upfront. As outlined in Phase 4.3 of the project plan, an evaluation of replacing TBB with Apple's Grand Central Dispatch (GCD) can be performed later as a potential optimization.

## 4. Platform-Specific API Usage

The codebase contains code that is specific to the Windows operating system and will not compile on macOS or Linux without modification.

### Files with Platform-Specific Code:

- **`SkinFlaps/src/FacialFlapsGui.h`**: This is the main file for GUI and user interaction, and it contains several Windows-specific sections.
    - **Directory Handling**: It uses `#ifdef WIN32` to switch between `_getcwd` (Windows) and `getcwd` (POSIX). This will need to be checked and standardized for all target platforms.
    - **Windows Registry Access**: The function `RegGetString` uses Windows-specific headers and functions (`RegGetValueW`, `HKEY`) to read default directory paths from the Windows Registry. This is the most critical platform-specific issue.

### Porting Strategy:

1.  **Standardize Preprocessor Macros**: Replace `#ifdef WIN32` with the more standard `_WIN32` for Windows, and add `#ifdef __APPLE__` for macOS-specific code.
2.  **Replace Registry Access**: The `RegGetString` function and its calls within `setDefaultDirectories` must be replaced. The new implementation should:
    - On macOS, read/write a configuration file (e.g., `config.json`) from the standard application support directory (`~/Library/Application Support/SkinFlaps/`).
    - The existing Windows implementation should also be refactored to use this configuration file instead of the registry to unify the approach across platforms. This will make future maintenance easier.
    - A simple key-value format will suffice (e.g., `{"model_directory": "/path/to/models"}`). 