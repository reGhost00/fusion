# FindLiburing.cmake

if (NOT Liburing_FOUND)
    find_path(LIBURING_INCLUDE_DIR
        NAMES liburing.h
        PATHS /usr/include /usr/local/include
    )

    find_library(LIBURING_LIBRARY
        NAMES uring
        PATHS /usr/lib /usr/local/lib
    )

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(uring DEFAULT_MSG LIBURING_INCLUDE_DIR LIBURING_LIBRARY)

    if (Liburing_FOUND)
        set(LIBURING_LIBRARIES ${LIBURING_LIBRARY})
        set(LIBURING_INCLUDE_DIRS ${LIBURING_INCLUDE_DIR})
    else()
        set(LIBURING_LIBRARIES)
        set(LIBURING_INCLUDE_DIRS)
    endif()

    mark_as_advanced(LIBURING_INCLUDE_DIR LIBURING_LIBRARY)
endif()