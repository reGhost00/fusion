#pragma once
#ifdef FU_USE_GLFW
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
typedef GLFWwindow RawWindow;
#elif defined(FU_USE_SDL)
#include <SDL2/SDL.h>
typedef SDL_Window RawWindow;
#endif
#include "core/array.h"
// #include "core/object.h"

// FU_DECLARE_TYPE(FUSurface, t_surface)
// #define FU_TYPE_SURFACE (t_surface_get_type())
typedef struct _FUContext FUContext;
typedef struct _FUSurface {
#ifdef FU_ENABLE_MESSAGE_CALLABLE
    void* dummy[165];
#else
    void* dummy[164];
#endif
} FUSurface;

typedef struct _FUSurfaceArgs {
    const char* title;
    const uint32_t width, height, version;
} FUSurfaceArgs;

void fu_surface_destroy(FUSurface* sf);
bool fu_surface_init(RawWindow* window, uint32_t width, uint32_t height, FUSurface* sf);
bool fu_surface_check_and_exchange(FUSurface* surface, const bool outOfDate);
bool fu_surface_update(FUSurface* surface, FUPtrArray* contexts);

// bool t_surface_init_command(FUSurface* sf, FUContext* ctx);
// bool t_surface_init_descriptor(FUSurface* sf, FUContext* ctx);
// void t_surface_resize(FUSurface* sf, FUPtrArray* ctxs);
// // bool t_surface_present(FUSurface* surface, FUContext* ctx);
// bool t_surface_valid_check_and_exchange(FUSurface* sf, const bool outOfDate);

bool fu_context_present(FUContext* ctx);
// bool fu_context_init_buffers__framebuffer(FUContext* ctx, FUSurface* sf);
// bool fu_context_init_buffers(FUContext* ctx, FUSurface* sf);
// bool fu_context_init_command(FUContext* ctx, FUSurface* sf);
// bool fu_context_init_descriptor(FUContext* ctx, FUSurface* sf);
// bool fu_context_init_synchronization_objects(FUContext* ctx, FUSurface* sf);
// void fu_context_init_finish(FUContext* ctx);
// void fu_context_free_framebuffers(FUContext* ctx);

// bool fu_context_record_command_buffer(FUSurface* sf, FUContext* ctx, uint32_t imageIndex);

// bool tvk_init_synchronization_objects(FUSurface* sf);