add_compile_definitions(FU_SIZEOF_VOID_P=${CMAKE_SIZEOF_VOID_P})


file(GLOB CORE "core/*.c" "core/*.cc" "core/*.h")
file(GLOB 3RD "libs/*.c" "libs/*.cc" "libs/*.h")
file(GLOB PLATFORM "platform/*.c" "platform/*.cc" "platform/*.h")
file(GLOB RENDERER "renderer/*.c" "renderer/*.cc" "renderer/*.h")

if (FU_RENDERER_TYPE_GL)
    file(GLOB_RECURSE RENDERER_GL "renderer/OpenGL/*.c" "renderer/OpenGL/*.cc" "renderer/OpenGL/*.h")
    list(APPEND RENDERER ${RENDERER_GL})
elseif(FU_RENDERER_TYPE_VK)
    file(GLOB_RECURSE RENDERER_VK "renderer/Vulkan/*.c" "renderer/Vulkan/*.cc" "renderer/Vulkan/*.h")
    list(APPEND RENDERER ${RENDERER_VK})
endif()

add_library(fusion STATIC ${CORE} ${3RD} ${PLATFORM} ${RENDERER})
target_link_libraries(fusion PUBLIC ${LIBS})

if(FU_HAS_C23_FLAG)
    target_compile_features(fusion PUBLIC c_std_23)
else()
    target_compile_features(fusion PUBLIC c_std_11)
endif()
