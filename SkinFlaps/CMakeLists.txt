# --- Collect Source Files --------------------------------------------------
file(GLOB_RECURSE SKNFLAPS_SOURCES "src/*.cpp")

# --- Executable Definition -------------------------------------------------
add_executable(SkinFlaps ${SKNFLAPS_SOURCES})

# --- Include Directories -------------------------------------------------
target_include_directories(SkinFlaps PUBLIC 
    "src"
    "src/CDT/include"
    "../imgui_glfw_nfd_lib"
    "../imgui_glfw_nfd_lib/extLibs/gl3w"
    "../gl3wGraphics"
    "../PDTetPhysics/include"
    "../PDTetPhysics/PDDeformer/include"
)

# --- Compile Definitions -------------------------------------------------
target_compile_definitions(SkinFlaps PRIVATE 
    IMGUI_IMPL_OPENGL_LOADER_GL3W
)

# --- Link Libraries ------------------------------------------------------
target_link_libraries(SkinFlaps PUBLIC
    gl3wGraphics
    PhysBAM_subset
    simd-numeric-kernels-new
    PDTetPhysics
    imgui_glfw_nfd_lib
    gl3w
)

# --- Platform-Specific Setup ---------------------------------------------
if(APPLE)
    # Add resources to the app bundle
    # This will be necessary later for creating a distributable .app
endif() 