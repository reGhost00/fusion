cmake_minimum_required(VERSION 3.28.0)
project(fusion VERSION 0.0.1)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/modules/" ${CMAKE_MODULE_PATH})
# find_package(OpenGL REQUIRED)
# include(FindPkgConfig)
# pkg_check_modules(DEPS REQUIRED glfw3)
# include_directories(${DEPS_INCLUDE_DIRS})
#
# GLAD
set(GLAD_GL_HEADERS "libs/glad/include/glad/gl.h" "libs/glad/include/KHR/khrplatform.h")
set(GLAD_GL_SOURCES "libs/glad/src/gl.c")
# set(GLAD_H "libs/glad/include")
add_library(glad STATIC ${GLAD_GL_HEADERS} ${GLAD_GL_SOURCES})
include_directories("libs/glad/include")

#
# mimalloc
set(MI_SECURE ON CACHE BOOL "Use full security mitigations (like guard pages, allocation randomization, double-free mitigation, and free-list corruption detection)")
set(MI_OVERRIDE OFF CACHE BOOL "Override the standard malloc interface (e.g. define entry points for malloc() etc)")
set(MI_XMALLOC ON CACHE BOOL "Enable abort() call on memory allocation failure by default")
set(MI_BUILD_SHARED OFF CACHE BOOL "Build shared library")
set(MI_BUILD_TESTS OFF CACHE BOOL "Build tests")
set(MI_DISABLE_INSTALL ON CACHE BOOL "no install")
add_subdirectory("libs/mimalloc")
include_directories("libs/mimalloc/include")

option(FU_USE_GLFW "Use GLFW for windowing" OFF)
option(FU_USE_SDL "Use SDL for windowing" OFF)

if (FU_USE_SDL)
    find_package(SDL2 REQUIRED)
    include_directories(${SDL2_INCLUDE_DIRS})
    add_compile_definitions(FU_USE_SDL)
else()
#
# GLFW
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "GLFW lib only")
    set(GLFW_INSTALL OFF CACHE BOOL "GLFW lib only")
    set(GLAD_GL "")
    add_subdirectory("libs/glfw")
    include_directories("libs/glfw/include")
    add_compile_definitions(FU_USE_GLFW)
endif()
# file(GLOB_RECURSE SRCS "src/*.c" "src/*.h")
# add_library(${PROJECT_NAME} STATIC ${SRCS})

# set(CMAKE_EXPORT_COMPILE_COMMANDS true)
set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)

option(FU_TRACK_MEMORY_ALLOCS "Track memory allocations" ON)

option(FU_RENDERER_TYPE_GL "Use OpenGL renderer" OFF)
option(FU_RENDERER_TYPE_VK "Use Vulkan renderer" OFF)
# set_property(CACHE FU_DISABLE_TRACK_MEMORY PROPERTY BOOL ON OFF)

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    add_compile_options(-O0 -Wall -g)
else()
    add_compile_definitions(FU_NO_DEBUG)
    add_compile_definitions(FU_NO_TRACK_MEMORY)
    add_compile_options(-O2 -Wall)
endif()

if (NOT FU_TRACK_MEMORY_ALLOCS)
    add_compile_definitions(FU_NO_TRACK_MEMORY)
endif()

if (FU_RENDERER_TYPE_VK)
    add_compile_definitions(FU_RENDERER_TYPE_VK)
else()
    add_compile_definitions(FU_RENDERER_TYPE_GL)
endif()



include_directories("src")
add_subdirectory("src")
add_subdirectory("test")