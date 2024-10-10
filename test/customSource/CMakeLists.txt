file(GLOB SRCS "*.c" "*.h")

add_executable(${subdir} ${SRCS})
target_link_libraries(${subdir} PRIVATE ${PROJECT_NAME} winhttp)