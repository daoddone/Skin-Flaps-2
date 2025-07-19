## **SkinFlaps - MacOS Port (in progress)**
### MacOS port of SkinFlaps - https://github.com/uwgraphics/SkinFlaps.git
### A soft tissue surgical simulator using projective dynamics


----------


### **Purpose**

----------
This open source project provides a soft tissue surgical simulation program which allows a surgeon to experiment with his/her own surgical designs to solve a soft tissue surgery problem.  The technical details regarding the inner workings of the code as well as the motivations for its use may be found in this *[paper][1]*.

----------

## Port Overview
An in-progress port of the Skin Flaps surgical simulation software to macOS by replacing three Intel-specific dependencies considered essential for the program's operation.

## The Three Main Components We Replaced

### 1. **Intel AVX Instructions → Scalar Reference Implementations**
- **Original**: Intel AVX SIMD instructions for vectorized math operations
- **Replaced with**: Non-optimized scalar reference implementations
- **Status**: ✅ Functional but not optimized for ARM NEON

### 2. **Intel MKL → Apple Accelerate + UMFPACK**
- **Original**: Intel Math Kernel Library for linear algebra operations
- **Replaced with**: 
  - Apple Accelerate framework (BLAS/LAPACK operations)
  - UMFPACK for sparse matrix solving
- **Status**: ✅ Fully functional with native performance

### 3. **Intel TBB → TBB (macOS version)**
- **Original**: Intel Threading Building Blocks for parallel processing
- **Replaced with**: TBB macOS version installed via Homebrew
- **Status**: ✅ Fully functional with same TBB API

## Current State

### ✅ **Early Stages of Functionalilty**
- Surgical simulation works on Apple Silicon without overload
- Interactive manipulation of 3D tissue models
- Physics solver handles ~13,000 nodes and ~3,000 constraints effectively
- Multiresolution system (recommended by original Skin Flaps) reduces complexity from 500K+ to ~17K tetrahedra

### 📊 **Performance**
- Currently using **scalar implementations** instead of SIMD optimizations
- Performance is enough for interactive use
- System handles typical models (3K-17K tetrahedra) smoothly
- Functionality is limited and in-progress; however, demonstrating capability

### 🖥️ **Platform Support**
- Tested and working on Apple Silicon (M2 Chip)
- No CUDA/GPU acceleration (CPU-only mode)
- Compatible with macOS build tools and environment

## Next Steps

### 1. **Optional Performance Optimizations**
- Convert scalar implementations to ARM NEON SIMD instructions
- Could provide significant speedup for math operations
- Not critical for current functionality but would improve performance, especially if larger models were utilized

### 2. **GPU Acceleration**
- Investigate Metal Compute Shaders as CUDA alternative
- Would benefit larger models and more complex simulations
- Low priority given current performance is capable

### 3. **Documentation & Polish**
- Complete documentation for macOS-specific build instructions
- Create installation guide for end users

## Key Achievement
We successfully proved that the software **does not** require Intel-specific hardware/libraries. The surgical simulator (in early stages of porting) runs on Apple Silicon using platform-native alternatives. 


-------------
-------------


Commit Notes:
7/18/2025:
macOS port: Update build configuration and source files for macOS compatibility
- Updated CMake configuration to support macOS build
- Modified PDTetPhysics to work without CUDA support on macOS
- Updated MKL wrapper to use Apple Accelerate framework
- Fixed include paths and dependencies for macOS
- Added hooks implementation
- Updated .gitignore to properly exclude build artifacts