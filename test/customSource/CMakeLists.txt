file(GLOB SRCS "*.c" "*.h")

add_executable(${subdir} ${SRCS})
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(${subdir} PRIVATE ${PROJECT_NAME} pthread curl)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(${subdir} PRIVATE ${PROJECT_NAME} winhttp)
endif()