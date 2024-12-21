# FindXCB.cmake

find_package(PkgConfig REQUIRED)

pkg_check_modules(PC_XCB xcb)

find_library(XCB_LIBRARY NAMES xcb)

find_path(XCB_INCLUDE_DIR NAMES xcb/xcb.h PATHS ${PC_XCB_INCLUDE_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(XCB DEFAULT_MSG
    XCB_LIBRARY XCB_INCLUDE_DIR)

if(XCB_FOUND)
    set(XCB_LIBRARIES ${XCB_LIBRARY})
    set(XCB_INCLUDE_DIRS ${XCB_INCLUDE_DIR})
endif()

mark_as_advanced(XCB_INCLUDE_DIR XCB_LIBRARIES)