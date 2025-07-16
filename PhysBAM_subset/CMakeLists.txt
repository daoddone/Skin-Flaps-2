# --- Collect Source Files --------------------------------------------------
file(GLOB_RECURSE PHYSBAM_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/Public_Library/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Common_Libraries/*.cpp"
)

# Add StackWalker only on Windows
if(WIN32)
    list(APPEND PHYSBAM_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/StackWalker/StackWalker.cpp")
endif()

# Remove files that depend on missing Grids_Dyadic headers
list(REMOVE_ITEM PHYSBAM_SOURCES
    # "${CMAKE_CURRENT_SOURCE_DIR}/Public_Library/PhysBAM_Geometry/Collisions_And_Grids/GRID_BASED_COLLISION_GEOMETRY.cpp"
    # "${CMAKE_CURRENT_SOURCE_DIR}/Public_Library/PhysBAM_Geometry/Collisions_And_Grids/OBJECTS_IN_CELL.cpp"
    # "${CMAKE_CURRENT_SOURCE_DIR}/Public_Library/PhysBAM_Geometry/Collisions/RIGID_COLLISION_GEOMETRY_3D.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Public_Library/PhysBAM_Tools/Grids_Dyadic_Boundaries/BOUNDARY_DYADIC.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Public_Library/PhysBAM_Tools/Read_Write/Grids_Dyadic/READ_WRITE_BINTREE_CELL.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Public_Library/PhysBAM_Tools/Read_Write/Grids_Dyadic/READ_WRITE_OCTREE_CELL.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Public_Library/PhysBAM_Tools/Read_Write/Grids_Dyadic/READ_WRITE_QUADTREE_CELL.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Public_Library/PhysBAM_Tools/Read_Write/Matlab/GNUPLOT_OUTPUT.cpp"
    # "${CMAKE_CURRENT_SOURCE_DIR}/Public_Library/PhysBAM_Geometry/Grids_Uniform_Collisions/GRID_BASED_COLLISION_GEOMETRY_UNIFORM.cpp"
    # "${CMAKE_CURRENT_SOURCE_DIR}/Public_Library/PhysBAM_Geometry/Grids_Uniform_Computations/LEVELSET_MAKER_UNIFORM.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Public_Library/PhysBAM_Geometry/Grids_Uniform_Computations/RIGID_GEOMETRY_RASTERIZATION_UNIFORM.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Public_Library/PhysBAM_Geometry/Grids_Uniform_Interpolation_Collidable/FACE_LOOKUP_COLLIDABLE_UNIFORM.cpp"
    # "${CMAKE_CURRENT_SOURCE_DIR}/Public_Library/PhysBAM_Geometry/Grids_Uniform_Interpolation_Collidable/LINEAR_INTERPOLATION_COLLIDABLE_CELL_UNIFORM.cpp"
    # "${CMAKE_CURRENT_SOURCE_DIR}/Public_Library/PhysBAM_Geometry/Grids_Uniform_Level_Sets/EXTRAPOLATION_UNIFORM.cpp"
    # "${CMAKE_CURRENT_SOURCE_DIR}/Public_Library/PhysBAM_Geometry/Grids_Uniform_Level_Sets/FAST_MARCHING_METHOD_UNIFORM.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Public_Library/PhysBAM_Geometry/Grids_Uniform_Level_Sets/LEVELSET_MULTIPLE.cpp"
    # "${CMAKE_CURRENT_SOURCE_DIR}/Public_Library/PhysBAM_Geometry/Grids_Uniform_Level_Sets/LEVELSET_UNIFORM.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Public_Library/PhysBAM_Tools/Read_Write/Utilities/ZIP.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Public_Library/PhysBAM_Tools/Utilities/PROCESS_UTILITIES.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Public_Library/PhysBAM_Geometry/Interpolation_Collidable/LINEAR_INTERPOLATION_UNIFORM_GEOMETRY.cpp"
    # "${CMAKE_CURRENT_SOURCE_DIR}/Public_Library/PhysBAM_Geometry/Solids_Geometry/RIGID_GEOMETRY_COLLECTION.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Public_Library/PhysBAM_Tools/Read_Write/Octave/OCTAVE_OUTPUT.cpp"
)

# Add stub implementations for missing symbols
list(APPEND PHYSBAM_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/physbam_stubs.cpp"
)

# --- Library Definition ----------------------------------------------------
add_library(PhysBAM_subset ${PHYSBAM_SOURCES})

# --- Include Directories -------------------------------------------------
target_include_directories(PhysBAM_subset PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Public_Library>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Common_Libraries>
    $<INSTALL_INTERFACE:include>
)

# Fix for macOS C++ standard library paths
if(APPLE)
    target_include_directories(PhysBAM_subset SYSTEM PUBLIC
        /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1
    )
endif()

# --- Compiler Definitions -------------------------------------------------
if(MSVC)
    target_compile_definitions(PhysBAM_subset PRIVATE -D_CRT_SECURE_NO_WARNINGS)
endif()

# Exclude optional features that we don't have
target_compile_definitions(PhysBAM_subset PUBLIC 
    COMPILE_WITHOUT_RLE_SUPPORT
    COMPILE_WITHOUT_DYADIC_SUPPORT
) 