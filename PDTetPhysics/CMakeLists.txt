cmake_minimum_required(VERSION 3.17)
project(PDTetPhysics LANGUAGES CXX)

# --- Find Dependencies ----------------------------------------------------
find_package(Threads REQUIRED)
find_package(OpenMP)

# --- Platform-specific configurations ---------------------------------------
if(APPLE)
    # macOS specific settings
    find_library(ACCELERATE_FRAMEWORK Accelerate)
    if(ACCELERATE_FRAMEWORK)
        message(STATUS "Found Accelerate Framework for macOS")
    else()
        message(FATAL_ERROR "Accelerate Framework not found on macOS")
    endif()
    
    # Remove the old SuiteSparse finding code - we use pkg-config now
    
    # macOS sparse solver selection
    set(MACOS_SPARSE_SOLVER "UMFPACK" CACHE STRING "Sparse solver to use on macOS (EIGEN or UMFPACK)")
    set_property(CACHE MACOS_SPARSE_SOLVER PROPERTY STRINGS "EIGEN" "UMFPACK")
    
    # We'll configure the sparse solver after the library is created
else()
    # Use MKL on other platforms
    find_package(MKL REQUIRED)
endif()

# --- Collect Source Files -------------------------------------------------
set(PDTET_SOURCES
    "src/MergedLevelSet.cpp"
    "src/PDTetSolver.cpp"
    "src/SchurSolver.cpp"
    "PDDeformer/src/Add_Force.cpp"
    "PDDeformer/src/GridDeformerTet.cpp"
    "PDDeformer/src/ReshapeDataStructure.cpp"
    "PDDeformer/src/SchurSolver.cpp"
    "PDDeformer/src/PardisoWrapper.cpp"
)

# --- Library Definition ----------------------------------------------------
add_library(PDTetPhysics ${PDTET_SOURCES})

# --- Include Directories --------------------------------------------------
target_include_directories(PDTetPhysics PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/PDDeformer/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../simd-numeric-kernels-new>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../PhysBAM_subset/Public_Library>
    $<INSTALL_INTERFACE:include>
)

# Add SuiteSparse includes on macOS
if(APPLE AND SUITESPARSE_INCLUDE_DIR)
    target_include_directories(PDTetPhysics PUBLIC ${SUITESPARSE_INCLUDE_DIR})
    target_compile_definitions(PDTetPhysics PUBLIC USE_SUITESPARSE)
endif()

# Fix for macOS C++ standard library paths
if(APPLE)
    target_include_directories(PDTetPhysics SYSTEM PUBLIC
        /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1
    )
endif()

# --- Platform-Specific Dependencies --------------------------------------
if(APPLE)
    # On macOS, use the Accelerate framework for BLAS/LAPACK
    find_library(ACCELERATE_LIBRARY Accelerate)
    if(ACCELERATE_LIBRARY)
        target_link_libraries(PDTetPhysics PUBLIC ${ACCELERATE_LIBRARY})
        message(STATUS "Found Accelerate Framework for macOS")
    else()
        message(FATAL_ERROR "Accelerate framework not found on macOS")
    endif()

    # Find and link TBB
    find_package(TBB REQUIRED)
    target_link_libraries(PDTetPhysics PUBLIC TBB::tbb)

    # macOS sparse solver selection
    set(MACOS_SPARSE_SOLVER "UMFPACK" CACHE STRING "Sparse solver to use on macOS (EIGEN or UMFPACK)")
    set_property(CACHE MACOS_SPARSE_SOLVER PROPERTY STRINGS "EIGEN" "UMFPACK")
    
    if(MACOS_SPARSE_SOLVER STREQUAL "UMFPACK")
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(SUITESPARSE REQUIRED umfpack)
        
        target_include_directories(PDTetPhysics PUBLIC ${SUITESPARSE_INCLUDE_DIRS})
        target_link_libraries(PDTetPhysics PUBLIC ${SUITESPARSE_LIBRARIES})
        target_compile_definitions(PDTetPhysics PUBLIC USE_SUITESPARSE)
        message(STATUS "Using UMFPACK sparse solver on macOS")
    else()
        # Use Eigen's built-in sparse solvers
        message(STATUS "Using Eigen sparse solver on macOS")
        target_compile_definitions(PDTetPhysics PUBLIC USE_EIGEN_SPARSE)
    endif()
else()
    # On Windows/Linux, find and link MKL and its threading layer
    set(MKL_THREADING "TBB") # Can be TBB or OpenMP
    find_package(MKL REQUIRED)
    include(${MKL_USE_FILE})
    target_link_libraries(PDTetPhysics PUBLIC ${MKL_LIBRARIES})
    target_include_directories(PDTetPhysics PUBLIC ${MKL_INCLUDE_DIRS})
    
    # If MKL uses OpenMP, we need to find it
    if(MKL_THREADING STREQUAL "OpenMP")
        find_package(OpenMP REQUIRED)
        target_link_libraries(PDTetPhysics PUBLIC OpenMP::OpenMP_CXX)
    endif()
endif()

# --- Link Libraries ------------------------------------------------------
target_link_libraries(PDTetPhysics PUBLIC
    PhysBAM_subset
    simd-numeric-kernels-new
    Threads::Threads
)

if(APPLE)
    target_link_libraries(PDTetPhysics PUBLIC ${ACCELERATE_FRAMEWORK})
    # SuiteSparse libraries are already linked via pkg-config above
    
    # Find and link TBB
    find_package(TBB REQUIRED)
    target_link_libraries(PDTetPhysics PUBLIC TBB::tbb)
    
    # Configure sparse solver for macOS
    if(MACOS_SPARSE_SOLVER STREQUAL "UMFPACK")
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(SUITESPARSE REQUIRED umfpack)
        
        target_include_directories(PDTetPhysics PUBLIC ${SUITESPARSE_INCLUDE_DIRS})
        target_link_directories(PDTetPhysics PUBLIC ${SUITESPARSE_LIBRARY_DIRS})
        target_link_libraries(PDTetPhysics PUBLIC ${SUITESPARSE_LIBRARIES})
        target_compile_definitions(PDTetPhysics PUBLIC USE_SUITESPARSE)
        message(STATUS "Using UMFPACK sparse solver on macOS")
    else()
        # Use Eigen's built-in sparse solvers
        message(STATUS "Using Eigen sparse solver on macOS")
        target_compile_definitions(PDTetPhysics PUBLIC USE_EIGEN_SPARSE)
    endif()
else()
    target_link_libraries(PDTetPhysics PUBLIC ${MKL_LIBRARIES})
endif()

if(OpenMP_CXX_FOUND)
    target_link_libraries(PDTetPhysics PUBLIC OpenMP::OpenMP_CXX)
    target_compile_definitions(PDTetPhysics PRIVATE USE_OPENMP)
else()
    message(WARNING "OpenMP not found. Building without OpenMP support.")
endif()
