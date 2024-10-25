find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)

add_executable(${subdir}_test main_0.c)
target_link_libraries(${subdir}_test PRIVATE glfw glad mimalloc-static Vulkan::Vulkan)