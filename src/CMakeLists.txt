# Creates C resources file from files in given directory
function(create_resources dir output)
    # Create empty output file
    file(WRITE ${output} "")
    # Collect input files
    file(GLOB bins ${dir}/*)
    # Iterate through input files
    foreach(bin ${bins})
        # Get short filename
        string(REGEX MATCH "([^/]+)$" filename ${bin})
        # Replace filename spaces & extension separator for C compatibility
        string(REGEX REPLACE "\\.| |-" "_" filename ${filename})
        # Read hex data from file
        file(READ ${bin} filedata HEX)
        # Convert hex data for C compatibility
        string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," filedata ${filedata})
        # Append data to output file
        file(APPEND ${output} "const unsigned char ${filename}[] = {${filedata}};\nconst unsigned ${filename}_size = sizeof(${filename});\n")
    endforeach()
endfunction()


# create_resources("${CMAKE_CURRENT_SOURCE_DIR}\\asset" "${CMAKE_CURRENT_SOURCE_DIR}\\asset\\res.h")


file(GLOB_RECURSE SRCS "*.c" "*.h")
add_library(${PROJECT_NAME} STATIC ${SRCS})
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_compile_definitions(FU_OS_LINUX)
    if (FU_USE_SDL)
        target_link_libraries(${PROJECT_NAME} PRIVATE SDL2 glad mimalloc-static pthread uring)
    else()
        target_link_libraries(${PROJECT_NAME} PRIVATE glfw glad mimalloc-static pthread uring)
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_compile_definitions(FU_OS_WINDOW)
    if (FU_USE_SDL)
        target_link_libraries(${PROJECT_NAME} PRIVATE SDL2 glad mimalloc-static)
    else()
        target_link_libraries(${PROJECT_NAME} PRIVATE glfw glad mimalloc-static)
    endif()
else()
    message("Unsupported platform: ${CMAKE_SYSTEM_NAME}")
endif()
