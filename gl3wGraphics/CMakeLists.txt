cmake_minimum_required(VERSION 3.20)

project(gl3wGraphics VERSION 0.1 LANGUAGES CXX)

# --- Library Definition ----------------------------------------------------
add_library(gl3wGraphics
    Bitmap.cpp
    gl3wGraphics.cpp
    GLmatrices.cpp
    lightsShaders.cpp
    lines.cpp
    materialTriangles.cpp
    sceneNode.cpp
    shapes.cpp
    surgGraphics.cpp
    staticTriangle.cpp
    textures.cpp
    trackball.cpp
)
set_source_files_properties(Bitmap.cpp PROPERTIES COMPILE_FLAGS "-x c++")

set_target_properties(gl3wGraphics PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# --- Include Directories -------------------------------------------------
target_include_directories(gl3wGraphics PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# --- Compiler Definitions -------------------------------------------------
if(WIN32)
    target_compile_definitions(gl3wGraphics PRIVATE NOMINMAX)
endif()

# --- Find Dependencies ---------------------------------------------------
find_package(glfw3 REQUIRED)

# --- Link Libraries ------------------------------------------------------
target_link_libraries(gl3wGraphics PUBLIC 
    glfw
    gl3w
)