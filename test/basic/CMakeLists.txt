add_executable(${subdir} main.c)
target_link_libraries(${subdir} PRIVATE ${PROJECT_NAME})