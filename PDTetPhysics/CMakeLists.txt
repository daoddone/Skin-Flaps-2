cmake_minimum_required(VERSION 3.17)
project(PDTetPhysics LANGUAGES CXX)

# --- Find Dependencies ----------------------------------------------------
find_package(Threads REQUIRED)
find_package(OpenMP)

# Platform-specific configuration
if(APPLE)
    # Use Accelerate framework on macOS
    find_library(ACCELERATE_FRAMEWORK Accelerate)
    if(NOT ACCELERATE_FRAMEWORK)
        message(FATAL_ERROR "Accelerate framework not found")
    endif()
    message(STATUS "Found Accelerate Framework for macOS")
    
    # Find SuiteSparse for sparse matrix solving on macOS
    find_path(SUITESPARSE_INCLUDE_DIR umfpack.h
        PATHS /opt/homebrew/include/suitesparse /usr/local/include/suitesparse
        REQUIRED
    )
    
    find_library(UMFPACK_LIB umfpack
        PATHS /opt/homebrew/lib /usr/local/lib
        REQUIRED
    )
    
    find_library(CHOLMOD_LIB cholmod
        PATHS /opt/homebrew/lib /usr/local/lib
        REQUIRED
    )
    
    find_library(AMD_LIB amd
        PATHS /opt/homebrew/lib /usr/local/lib
        REQUIRED
    )
    
    find_library(COLAMD_LIB colamd
        PATHS /opt/homebrew/lib /usr/local/lib
        REQUIRED
    )
    
    find_library(SUITESPARSE_CONFIG_LIB suitesparseconfig
        PATHS /opt/homebrew/lib /usr/local/lib
        REQUIRED
    )
    
    message(STATUS "Found SuiteSparse libraries for macOS")
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

    # Option to choose sparse solver on macOS
    set(MACOS_SPARSE_SOLVER "EIGEN" CACHE STRING "Sparse solver to use on macOS (EIGEN or UMFPACK)")
    set_property(CACHE MACOS_SPARSE_SOLVER PROPERTY STRINGS "EIGEN" "UMFPACK")
    
    if(MACOS_SPARSE_SOLVER STREQUAL "EIGEN")
        message(STATUS "Using Eigen's built-in sparse solvers on macOS")
        add_definitions(-DUSE_EIGEN_SPARSE)
    else()
        message(STATUS "Using UMFPACK sparse solver on macOS")
        # Find UMFPACK (part of SuiteSparse)
        find_path(UMFPACK_INCLUDE_DIR umfpack.h
            PATHS /usr/local/include /opt/homebrew/include /opt/local/include
            PATH_SUFFIXES suitesparse)
        
        find_library(UMFPACK_LIBRARY umfpack
            PATHS /usr/local/lib /opt/homebrew/lib /opt/local/lib)
        
        find_library(AMD_LIBRARY amd
            PATHS /usr/local/lib /opt/homebrew/lib /opt/local/lib)
        
        find_library(CHOLMOD_LIBRARY cholmod
            PATHS /usr/local/lib /opt/homebrew/lib /opt/local/lib)
        
        find_library(SUITESPARSECONFIG_LIBRARY suitesparseconfig
            PATHS /usr/local/lib /opt/homebrew/lib /opt/local/lib)
        
        if(UMFPACK_INCLUDE_DIR AND UMFPACK_LIBRARY AND AMD_LIBRARY)
            message(STATUS "Found UMFPACK: ${UMFPACK_LIBRARY}")
            set(UMFPACK_LIBRARIES ${UMFPACK_LIBRARY} ${AMD_LIBRARY} ${CHOLMOD_LIBRARY} ${SUITESPARSECONFIG_LIBRARY})
            include_directories(${UMFPACK_INCLUDE_DIR})
            add_definitions(-DUSE_SUITESPARSE)
        else()
            message(FATAL_ERROR "UMFPACK not found. Install SuiteSparse or switch to EIGEN solver.")
        endif()
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
    # Link SuiteSparse libraries on macOS
    if(UMFPACK_LIB AND CHOLMOD_LIB AND AMD_LIB AND COLAMD_LIB AND SUITESPARSE_CONFIG_LIB)
        target_link_libraries(PDTetPhysics PUBLIC 
            ${UMFPACK_LIB} 
            ${CHOLMOD_LIB} 
            ${AMD_LIB}
            ${COLAMD_LIB}
            ${SUITESPARSE_CONFIG_LIB}
        )
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
