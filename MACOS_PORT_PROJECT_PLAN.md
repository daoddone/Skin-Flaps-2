# SkinFlaps macOS/Apple Silicon Port Project Plan

## Project Overview

This document outlines the comprehensive plan to port the SkinFlaps surgical simulator from its current Intel/Windows/Linux-only implementation to a native macOS/Apple Silicon version. This is a community-driven effort to make this valuable medical simulation tool available to users of modern Mac computers.

**Project Start Date:** January 2025  
**Project Status:** Planning Phase  
**Current Contributors:** Community volunteers  

---

## Instructions for AI Agents

### How to Use This Document

When assisting with this project, AI agents should:

1. **Always read the "Work Log" section first** to understand what has been completed and what is currently in progress
2. **Update the Work Log** after making any changes or completing any tasks
3. **Follow the naming conventions** specified in each phase
4. **Check dependencies** before starting new work - ensure prerequisite phases are complete
5. **Document all changes** with clear commit messages referencing this plan
6. **Update status indicators** using the format: `[STATUS: Not Started | In Progress | Complete | Blocked]`

### Work Log Format

When updating the work log, use this format:
```
**[DATE]** - [YOUR_IDENTIFIER/CONTRIBUTOR]
- Task completed: [Brief description]
- Files modified: [List of files]
- Next steps: [What should be done next]
- Blockers: [Any issues encountered]
```

---

## Technical Challenges Summary

The main barriers to macOS compatibility are:

1. **Intel AVX Instructions**: The physics engine uses Intel-specific SIMD instructions
2. **Intel MKL (Math Kernel Library)**: Heavily optimized for Intel CPUs
3. **Intel TBB (Threading Building Blocks)**: Parallel processing optimized for Intel
4. **Build System**: Currently uses Visual Studio project files
5. **Platform-specific code**: Some Windows-specific file paths and registry access

---

## Phase 1: Analysis and Preparation
**[STATUS: Complete]**  
**Estimated Duration:** 1-2 weeks  
**Dependencies:** None  

### Objectives
- Fully understand the codebase architecture
- Identify all Intel-specific dependencies
- Create a dependency map
- Set up development environment

### Tasks

#### 1.1 Codebase Analysis
- [X] Map all uses of Intel AVX instructions in `simd-numeric-kernels-new/`
- [X] Document all MKL function calls
- [X] List all TBB usage patterns
- [X] Identify Windows-specific code sections
- [X] Create a comprehensive dependency graph

#### 1.2 Development Environment Setup
- [X] Set up a Mac development machine with Xcode and command line tools
- [X] Install CMake (version 3.20 or higher)
- [X] Install GLFW3 via Homebrew: `brew install glfw`
- [X] Set up a cross-platform C++ IDE (VS Code or CLion recommended)

#### 1.3 Create Project Infrastructure
- [X] Create a `macos-port` branch in git
- [X] Set up continuous integration for macOS builds (GitHub Actions)
- [X] Create `docs/macos-port/` directory for port-specific documentation

### Deliverables
- `docs/macos-port/dependency-analysis.md` - Complete analysis of Intel dependencies
- `docs/macos-port/codebase-map.md` - Architecture overview
- Development environment ready for work

---

## Phase 2: CMake Build System
**[STATUS: Complete]**  
**Estimated Duration:** 1 week  
**Dependencies:** Phase 1 complete  

### Objectives
- Replace Visual Studio build system with cross-platform CMake
- Ensure builds work on original platforms (Windows/Linux)
- Prepare for macOS-specific configurations

### Tasks

#### 2.1 Root CMakeLists.txt
- [X] Create top-level `CMakeLists.txt`
- [X] Define project structure and dependencies
- [X] Set up compiler flags for different platforms
- [X] Configure GLFW and OpenGL finding

#### 2.2 Subproject CMake Files
- [X] Create CMakeLists.txt for `gl3wGraphics/`
- [X] Create CMakeLists.txt for `SkinFlaps/`
- [X] Create CMakeLists.txt for `PhysBAM_subset/`
- [X] Create CMakeLists.txt for `PDTetPhysics/`
- [X] Create CMakeLists.txt for `simd-numeric-kernels-new/`

#### 2.3 Platform Detection
- [X] Add platform detection logic
- [X] Create separate build configurations for Intel vs Apple Silicon
- [X] Set up conditional compilation flags

### Deliverables
- Complete CMake build system
- Successful builds on Windows/Linux (maintaining compatibility)
- Build instructions in `docs/macos-port/building.md`

---

## Phase 3: Reference Implementation Port
**[STATUS: Not Started]**  
**Estimated Duration:** 2-3 weeks  
**Dependencies:** Phase 2 complete  

### Objectives
- Get SkinFlaps running on macOS using non-optimized reference implementations
- Replace platform-specific code
- Verify functionality (even if slow)

### Tasks

#### 3.1 Physics Engine Reference Implementation
- [ ] Modify build to use `simd-numeric-kernels-new/References/` implementations
- [ ] Create macOS-specific implementations where needed
- [ ] Remove all AVX-specific code paths for macOS builds

#### 3.2 Platform-Specific Code Replacement
- [ ] Replace Windows Registry code in `FacialFlapsGui.h` with macOS preferences
- [ ] Update file path handling for macOS conventions
- [ ] Fix any endianness issues (if any)

#### 3.3 Dependencies Resolution
- [ ] Find macOS alternatives to any Windows-specific libraries
- [ ] Update OpenGL context creation for macOS
- [ ] Ensure GLFW integration works properly

#### 3.4 Testing
- [ ] Create basic functionality test suite
- [ ] Verify all surgical tools work (even if slowly)
- [In Progress] Resolve hook movement crash after topology changes
- [In Progress] Recompute hook UV coordinates after surface edits to keep hooks attached
- [ ] Document performance baseline

### Deliverables
- Working (but slow) macOS build
- Test results in `docs/macos-port/reference-implementation-tests.md`
- Performance baseline metrics

---

## Phase 4: Apple Silicon Optimization
**[STATUS: Not Started]**  
**Estimated Duration:** 4-6 weeks  
**Dependencies:** Phase 3 complete  

### Objectives
- Implement high-performance Apple Silicon native code
- Achieve real-time performance comparable to Intel version
- Maintain cross-platform compatibility

### Tasks

#### 4.1 Accelerate Framework Integration
- [ ] Study Apple's Accelerate framework documentation
- [ ] Map Intel MKL functions to Accelerate equivalents
- [ ] Create wrapper functions for seamless integration

#### 4.2 NEON SIMD Implementation
- [ ] Learn ARM NEON intrinsics
- [ ] Port `Matrix_Times_Matrix` kernel to NEON
- [ ] Port `Matrix_Times_Transpose` kernel to NEON
- [ ] Port `Singular_Value_Decomposition` kernel to NEON
- [ ] Create comprehensive SIMD unit tests

#### 4.3 Threading Optimization
- [ ] Evaluate Grand Central Dispatch vs TBB on macOS
- [ ] Implement optimal threading strategy for Apple Silicon
- [ ] Profile and optimize thread scheduling

#### 4.4 Metal Compute Shaders (Stretch Goal)
- [ ] Investigate using Metal for GPU acceleration
- [ ] Prototype physics calculations on Apple GPU
- [ ] Compare performance with CPU implementation

### Deliverables
- Optimized macOS build with native performance
- Performance comparison report
- Updated documentation for macOS-specific optimizations

---

## Phase 5: Testing and Polish
**[STATUS: Not Started]**  
**Estimated Duration:** 2-3 weeks  
**Dependencies:** Phase 4 complete  

### Objectives
- Comprehensive testing on various Mac models
- UI/UX improvements for macOS
- Prepare for release

### Tasks

#### 5.1 Compatibility Testing
- [ ] Test on Intel Macs (if AVX-free version works)
- [ ] Test on M1, M2, M3 Mac variants
- [ ] Test on different macOS versions

#### 5.2 macOS Integration
- [ ] Implement native macOS menu bar
- [ ] Add macOS-specific keyboard shortcuts
- [ ] Create .app bundle for distribution
- [ ] Add code signing (if possible)

#### 5.3 Documentation
- [ ] Update README.md with macOS instructions
- [ ] Create macOS-specific user guide
- [ ] Document known limitations
- [ ] Create troubleshooting guide

### Deliverables
- Release-ready macOS build
- Complete documentation
- Test results from various Mac models

---

## Phase 6: Release and Maintenance
**[STATUS: Not Started]**  
**Estimated Duration:** Ongoing  
**Dependencies:** Phase 5 complete  

### Objectives
- Release to community
- Establish maintenance process
- Plan future improvements

### Tasks

#### 6.1 Release Preparation
- [ ] Create release notes
- [ ] Prepare distribution package
- [ ] Set up download infrastructure

#### 6.2 Community Engagement
- [ ] Announce on relevant forums
- [ ] Create feedback collection mechanism
- [ ] Establish bug reporting process

#### 6.3 Maintenance Planning
- [ ] Set up automated builds
- [ ] Create update distribution system
- [ ] Plan for future macOS updates

---

## Work Log

### 2025

**[2025-01-XX]** - [AI Assistant/Initial Planning]
- Task completed: Created comprehensive project plan
- Files modified: Created `MACOS_PORT_PROJECT_PLAN.md`
- Next steps: Find contributors and begin Phase 1 analysis
- Blockers: Need volunteers with macOS development experience

**[2025-07-16]** - [OpenAI Codex]
- Task completed: Implemented automatic re-addition of hook constraints when missing to prevent crashes during dragging. Updated project plan with new task.
- Files modified: `SkinFlaps/src/hooks.cpp`, `MACOS_PORT_PROJECT_PLAN.md`
- Next steps: Validate hook tool stability on macOS build.
- Blockers: None

**[2025-07-17]** - [OpenAI Codex]
- Task completed: Recomputed hook barycentric coordinates after topology changes to keep hooks draggable.
- Files modified: `SkinFlaps/src/hooks.cpp`, `MACOS_PORT_PROJECT_PLAN.md`
- Next steps: Test hook interaction following knife/undermine actions.
- Blockers: None

**[2025-07-18]** - [OpenAI Codex]
- Task completed: Increased hook drag displacement limit to improve visible movement.
- Files modified: `SkinFlaps/src/surgicalActions.cpp`, `MACOS_PORT_PROJECT_PLAN.md`
- Next steps: Verify that hook dragging now responds after undermining.
- Blockers: None

**[2025-07-19]** - [OpenAI Codex]
- Task completed: Added per-hook tet tracking and automatic constraint reattachment when moving across tets to prevent freezes.
- Files modified: `SkinFlaps/src/hooks.cpp`, `SkinFlaps/src/hooks.h`, `MACOS_PORT_PROJECT_PLAN.md`
- Next steps: Rebuild and validate hook drag across cut regions.
- Blockers: None

**[2025-07-20]** - [OpenAI Codex]
- Task completed: Simplified hook movement logic and only recreate constraints when missing. Updated cmake attempts.
- Files modified: `SkinFlaps/src/hooks.cpp`, `MACOS_PORT_PROJECT_PLAN.md`
- Next steps: Confirm hook dragging works after cuts and undermines.
- Blockers: Missing MKL package preventing full build

**[2025-07-21]** - [OpenAI Codex]
- Task completed: Detect when a hook crosses into a new tetrahedron and recreate the constraint automatically to maintain stable dragging.
- Files modified: `SkinFlaps/src/hooks.cpp`, `MACOS_PORT_PROJECT_PLAN.md`
- Next steps: Test hook dragging after undermining to ensure crash free operation.
- Blockers: None


**[2025-07-22]** - [OpenAI Codex]
- Task completed: Added fallback logic to relocate hooks when their original triangle is removed. Hooks now search nearby triangles or reuse the previous tet when possible.
- Files modified: `SkinFlaps/src/hooks.cpp`, `MACOS_PORT_PROJECT_PLAN.md`
- Next steps: Verify hook dragging works after deep cuts and undermines.
- Blockers: None

---

## Technical Notes

### Key Files to Modify

1. **Build System**
   - Create: `CMakeLists.txt` (root and subdirectories)
   - Remove dependency on: `*.vcxproj`, `*.sln`

2. **Platform-Specific Code**
   - `SkinFlaps/src/FacialFlapsGui.h` - Registry access code
   - `simd-numeric-kernels-new/Common/arch/` - SIMD architecture detection

3. **Physics Engine**
   - `simd-numeric-kernels-new/Kernels/` - All SIMD implementations
   - `PDTetPhysics/` - May need threading adjustments

### Useful Resources

- [Apple Accelerate Framework Documentation](https://developer.apple.com/documentation/accelerate)
- [ARM NEON Intrinsics Reference](https://developer.arm.com/architectures/instruction-sets/intrinsics/)
- [Porting from Intel Intrinsics to ARM NEON](https://developer.arm.com/documentation)
- [CMake Cross-Platform Guide](https://cmake.org/cmake/help/latest/guide/tutorial/index.html)

### Performance Targets

Based on the README, the original implementation handles:
- 620,000 tetrahedra at interactive frame rates (Intel version)
- 17,000 tetrahedra in low-resolution mode

The macOS port should aim for:
- Phase 3: Any frame rate that allows testing (even 1-5 fps)
- Phase 4: Minimum 30 fps for low-resolution mode
- Phase 4: Stretch goal of matching Intel performance

---

## Contributing

If you want to contribute to this porting effort:

1. Read this entire document
2. Check the Work Log for current status
3. Choose an uncompleted task from the current phase
4. Update the Work Log when starting work
5. Submit changes via pull request with clear documentation

For questions or coordination, please open an issue tagged with `macos-port`.

---

*This document is a living plan and will be updated as the project progresses.* 