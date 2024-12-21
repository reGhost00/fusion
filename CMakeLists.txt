cmake_minimum_required(VERSION 3.30.0)
project(fusionEngine VERSION 0.0.3)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/modules/" ${CMAKE_MODULE_PATH})

set(LIB_PATH "${PROJECT_SOURCE_DIR}/libs")
# 设置构建目录
set(LIB_BUILD_PATH "${PROJECT_SOURCE_DIR}/build/libs")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared library")
#
# mimalloc
set(MI_SECURE ON CACHE BOOL "Use full security mitigations (like guard pages, allocation randomization, double-free mitigation, and free-list corruption detection)")
set(MI_OVERRIDE OFF CACHE BOOL "Override the standard malloc interface (e.g. define entry points for malloc() etc)")
set(MI_XMALLOC ON CACHE BOOL "Enable abort() call on memory allocation failure by default")
set(MI_BUILD_SHARED OFF CACHE BOOL "Build shared library")
set(MI_BUILD_TESTS OFF CACHE BOOL "Build tests")
set(MI_DISABLE_INSTALL ON CACHE BOOL "no install")
add_subdirectory("${LIB_PATH}/mimalloc" "${LIB_BUILD_PATH}/mimalloc")
include_directories("${LIB_PATH}/mimalloc/include")

#
# cglm
set(CGLM_SHARED OFF CACHE BOOL "Shared build")
set(CGLM_STATIC ON CACHE BOOL "Static build")
add_subdirectory("${LIB_PATH}/cglm" "${LIB_BUILD_PATH}/cglm")
include_directories("${LIB_PATH}/cglm/include")

#
# KTX
set(KTX_FEATURE_TESTS OFF CACHE BOOL "Build KTX tests")
add_subdirectory("${LIB_PATH}/KTX-Software" "${LIB_BUILD_PATH}/KTX-Software")
include_directories("${LIB_PATH}/KTX-Software/include")

option(FU_USE_GLFW "Use GLFW for windowing. this is default" OFF)
option(FU_USE_SDL "Use SDL for windowing" ON)

option(FU_RENDERER_TYPE_GL "Use OpenGL renderer" OFF)
option(FU_RENDERER_TYPE_VK "Use Vulkan renderer" ON)
option(FU_ENABLE_SPIRV_SUPPORT "Use SPIRV in OpenGL. this will require OpenGL 4.6" OFF)
option(FU_ENABLE_DESC_BUFF "Use descriptor buffer in Vulkan" OFF)

option(FU_DEBUG_DISABLE_TRACK_MEMORY_ALLOCS "Track memory allocations" OFF)
option(FU_DEBUG_DISABLE_ERROR_GUARD "Disable error guard in DEBUG mode" OFF)
option(FU_DEBUG_DISABLE_FLOW_CONTROL "Disable flow control in DEBUG mode" OFF)
option(FU_DEBUG_DISABLE_MESSAGE_CALLABLE "Disable OpenGL/Vulkan message callable in DEBUG mode" OFF)

option(FU_RELEASE_ENABLE_ERROR_GUARD "Enable error guard in RELEASE mode" OFF)
option(FU_RELEASE_ENABLE_FLOW_CONTROL "Enable flow control in RELEASE mode" OFF)
option(FU_RELEASE_ENABLE_MESSAGE_CALLABLE "Enable OpenGL/Vulkan message callable in RELEASE mode" OFF)

# 检查 C++23/C23 支持
include(CheckCCompilerFlag)
check_c_compiler_flag(-std=c11 FU_HAS_C11_FLAG)

if(NOT CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    message(WARNING "Unknown compiler. Use GCC or CLang in preference")
endif()

if(FU_HAS_C11_FLAG)
    add_compile_definitions(FU_COMPILER_C11)
    check_c_compiler_flag(-std=c23 FU_HAS_C23_FLAG)
    if (FU_HAS_C23_FLAG)
        add_compile_definitions(FU_COMPILER_C23)
    else()
        message(WARNING "Your compiler does not support C23.")
    endif()
else()
    message(FATAL_ERROR "Your compiler does not support C11.")
endif()

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    if (FU_DEBUG_DISABLE_ERROR_GUARD)
        add_compile_definitions(FU_DISABLE_ERROR_GUARD)
    endif()
    if (FU_DEBUG_DISABLE_FLOW_CONTROL)
        add_compile_definitions(FU_DISABLE_CONTROL_FLOW)
    endif()
    if (FU_DEBUG_DISABLE_MESSAGE_CALLABLE)
        add_compile_definitions(FU_DISABLE_MESSAGE_CALLABLE)
    endif()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
    add_compile_definitions(FU_ENABLE_TRACK_MEMORY FU_ENABLE_DEBUG_MESSAGE FU_ENABLE_MESSAGE_CALLABLE)
elseif (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    if (FU_RELEASE_ENABLE_ERROR_GUARD)
        add_compile_definitions(FU_ENABLE_ERROR_GUARD)
    endif()
    if (FU_RELEASE_ENABLE_FLOW_CONTROL)
        add_compile_definitions(FU_ENABLE_CONTROL_FLOW)
    endif()
    if (FU_RELEASE_ENABLE_MESSAGE_CALLABLE)
        add_compile_definitions(FU_ENABLE_MESSAGE_CALLABLE)
    endif()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -DNDEBUG")
    add_compile_definitions(FU_DISABLE_ERROR_GUARD FU_DISABLE_CONTROL_FLOW FU_DISABLE_DEBUG_MESSAGE FU_DISABLE_MESSAGE_CALLABLE)
endif()


set(LIBS cglm mimalloc-static ktx)
find_package(PkgConfig REQUIRED)

#
# platform
if(WIN32)
    add_compile_definitions(FU_OS_WINDOW VK_USE_PLATFORM_WIN32_KHR)
elseif(UNIX AND NOT APPLE)
    # 检查是否安装了Wayland
    pkg_check_modules(WAYLAND wayland-client wayland-server)
    pkg_check_modules(URING REQUIRED liburing)
    if(WAYLAND_FOUND)
        add_compile_definitions(FU_OS_LINUX FU_OS_LINUX_WAYLAND VK_USE_PLATFORM_WAYLAND_KHR)
        include_directories(${URING_INCLUDE_DIRS} ${WAYLAND_INCLUDE_DIRS})
        set(LIBS ${LIBS} ${WAYLAND_LIBRARIES} ${URING_LIBRARIES} pthread m)
    else()
        # 检查是否安装了X11
        # sudo apt-get install libxcb1-dev libxcb-icccm4-dev libxcb-keysyms1-dev libxcb-render-util0-dev libxcb-xfixes0-dev
        # sudo dnf install libxcb-devel libxcb-icccm-devel libxcb-keysyms-devel libxcb-render-util-devel libxcb-xfixes-devel
        pkg_check_modules(XCB REQUIRED xcb)
        add_compile_definitions(FU_OS_LINUX FU_OS_LINUX_XCB VK_USE_PLATFORM_XCB_KHR)
        include_directories(${XCB_INCLUDE_DIRS} ${URING_INCLUDE_DIRS})
        set(LIBS ${LIBS} ${XCB_LIBRARIES} ${URING_LIBRARIES} pthread m)
    endif()
else()
    message(FATAL_ERROR "Unsupported platform.")
endif()

#
# Windowing
if (FU_USE_SDL)
    pkg_check_modules(SDL REQUIRED sdl2)
    add_compile_definitions(FU_USE_SDL)
    include_directories(${SDL_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${SDL_LIBRARIES})
else()
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "GLFW lib only")
    set(GLFW_INSTALL OFF CACHE BOOL "GLFW lib only")
    set(GLAD_GL "")
    add_subdirectory("${LIB_PATH}/glfw" "${LIB_BUILD_PATH}/glfw")
    include_directories("${LIB_PATH}/glfw/include")
    add_compile_definitions(FU_USE_GLFW)
    set(LIBS ${LIBS} glfw)
endif()

#
# Renderer
if (FU_RENDERER_TYPE_GL)
    add_compile_definitions(FU_RENDERER_TYPE_GL)
elseif(FU_RENDERER_TYPE_VK)
    find_package(Vulkan REQUIRED)
    if (NOT VULKAN_FOUND)
        message(FATAL_ERROR "Vulkan not found")
    endif()
    include_directories(${VULKAN_INCLUDE_DIRS})
    add_compile_definitions(FU_RENDERER_TYPE_VK)
    set(LIBS ${LIBS} Vulkan::Vulkan)
    
endif()

include_directories("engine")
add_subdirectory("engine")
add_subdirectory("tests")