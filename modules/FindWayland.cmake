# FindWayland.cmake

find_package(PkgConfig REQUIRED)

pkg_check_modules(PC_WAYLAND wayland-client wayland-server)

find_library(WAYLAND_CLIENT_LIBRARY NAMES wayland-client)
find_library(WAYLAND_SERVER_LIBRARY NAMES wayland-server)

find_path(WAYLAND_INCLUDE_DIR NAMES wayland-client.h PATHS ${PC_WAYLAND_INCLUDE_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Wayland DEFAULT_MSG
    WAYLAND_CLIENT_LIBRARY WAYLAND_SERVER_LIBRARY WAYLAND_INCLUDE_DIR)

if(WAYLAND_FOUND)
    set(WAYLAND_LIBRARIES ${WAYLAND_CLIENT_LIBRARY} ${WAYLAND_SERVER_LIBRARY})
    set(WAYLAND_INCLUDE_DIRS ${WAYLAND_INCLUDE_DIR})
endif()

mark_as_advanced(WAYLAND_INCLUDE_DIR WAYLAND_LIBRARIES)