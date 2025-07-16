cmake_minimum_required(VERSION 3.20)
project(gl3w LANGUAGES C)

add_library(gl3w GL/gl3w.c)

target_include_directories(gl3w PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
) 