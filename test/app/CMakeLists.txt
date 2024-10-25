find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)

add_executable(${subdir} main.c)
target_link_libraries(${subdir} PRIVATE ${PROJECT_NAME} glfw glad Vulkan::Vulkan)