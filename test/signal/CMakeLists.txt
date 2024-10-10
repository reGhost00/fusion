add_executable(${subdir}_test main.c)
target_link_libraries(${subdir}_test PRIVATE ${PROJECT_NAME})