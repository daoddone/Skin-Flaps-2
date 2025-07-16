cmake_minimum_required(VERSION 3.20)

project(imgui_glfw_nfd_lib LANGUAGES CXX C OBJCXX)

# --- Source Files --------------------------------------------------------
set(IMGUI_SOURCES
    "imgui.cpp"
    "imgui_demo.cpp"
    "imgui_draw.cpp"
    "imgui_tables.cpp"
    "imgui_widgets.cpp"
    "imgui_impl_glfw.cpp"
    "imgui_impl_opengl3.cpp"
    "ImGuiFileDialog.cpp"
)

# --- Platform-specific sources for Native File Dialog --------------------
# NFD library is not included in the project, ImGuiFileDialog is used instead.
# The code below is commented out for future reference if NFD is added.
# if(APPLE)
#     list(APPEND IMGUI_SOURCES "extLibs/nfd/nfd_cocoa.m")
# elseif(UNIX AND NOT APPLE)
#     list(APPEND IMGUI_SOURCES "extLibs/nfd/nfd_gtk.c")
#     find_package(PkgConfig REQUIRED)
#     pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
#     include_directories(${GTK3_INCLUDE_DIRS})
#     link_libraries(${GTK3_LIBRARIES})
# elseif(WIN32)
#     list(APPEND IMGUI_SOURCES "extLibs/nfd/nfd_win.cpp")
# endif()

# --- Add Subdirectories --------------------------------------------------
add_subdirectory(extLibs/gl3w)

# --- Library Definition ----------------------------------------------------
add_library(imgui_glfw_nfd_lib ${IMGUI_SOURCES})

# --- Include Directories -------------------------------------------------
target_include_directories(imgui_glfw_nfd_lib PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

# --- Find Dependencies ---------------------------------------------------
find_package(glfw3 REQUIRED)

# --- Link Libraries ------------------------------------------------------
target_link_libraries(imgui_glfw_nfd_lib PUBLIC 
    glfw
    gl3w
)

if(APPLE)
    find_library(APPKIT_LIBRARY AppKit)
    target_link_libraries(imgui_glfw_nfd_lib PUBLIC ${APPKIT_LIBRARY})
endif()