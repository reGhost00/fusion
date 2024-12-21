# file(GLOB core "core/*.c" "core/*.cc" "core/*.h")
# file(GLOB platform "platform/*.c" "platform/*.h")
file(GLOB warpper "warpper/*.c" "warpper/*.cc"  "warpper/*.h")
include(FindPkgConfig)
pkg_check_modules(DEPS REQUIRED sdl2 glfw3)

add_executable(${dir} ${warpper} main.c)
target_link_libraries(${dir} ${DEPS_LIBRARIES} fusion)