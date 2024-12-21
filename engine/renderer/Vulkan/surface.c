#ifdef FU_RENDERER_TYPE_VK
#include "core/logger.h"

#include "misc.inner.h"

#ifdef FU_ENABLE_MESSAGE_CALLABLE
#define T_INSTANCE(sf) ((sf)->instance.instance)
#else
#define T_INSTANCE(sf) ((sf)->instance)
#endif

#define T_DEVICE(sf) ((sf)->device.device)
#define T_SURFACE(sf) ((sf)->surface.surface)
#define T_SWAPCHAIN(sf) ((sf)->swapchain.swapchain)

void fu_surface_destroy(FUSurface* surface)
{
    fu_surface_t* sf = (fu_surface_t*)surface;
    t_pipeline_library_destroy(&sf->library, T_DEVICE(sf));
    t_swapchain_destroy(&sf->swapchain, &sf->device);
    t_device_destroy(&sf->device);
#ifdef FU_USE_GLFW
    vkDestroySurfaceKHR(T_INSTANCE(sf), T_SURFACE(sf), &defCustomAllocator);
#elif defined(FU_USE_SDL)
    vkDestroySurfaceKHR(T_INSTANCE(sf), T_SURFACE(sf), NULL);
#endif
#ifdef FU_ENABLE_MESSAGE_CALLABLE
    t_instance_destroy(&sf->instance);
#else
    t_instance_destroy(sf->instance);
#endif
}

bool fu_surface_init(RawWindow* window, uint32_t width, uint32_t height, FUSurface* surface)
{
    fu_surface_t* sf = (fu_surface_t*)surface;
    if (FU_UNLIKELY(!t_instance_init(DEF_APP_NAME, DEF_APP_VER, &sf->instance))) {
        fu_error("Failed to create vulkan instance\n");
        return false;
    }
#ifdef FU_USE_GLFW
    if (FU_UNLIKELY(glfwCreateWindowSurface(T_INSTANCE(sf), window, &defCustomAllocator, &T_SURFACE(sf)))) {
        fu_error("Failed to create window surface\n");
        return false;
    }
#elif defined(FU_USE_SDL)
    if (FU_UNLIKELY(!SDL_Vulkan_CreateSurface(window, T_INSTANCE(sf), &T_SURFACE(sf)))) {
        fu_error("Failed to create window surface: %s\n", SDL_GetError());
        return false;
    }
#endif
    if (FU_UNLIKELY(!t_device_init(T_INSTANCE(sf), T_SURFACE(sf), &sf->device))) {
        fu_error("Failed to create device\n");
        return false;
    }

    if (FU_UNLIKELY(!t_swapchain_init(T_INSTANCE(sf), &sf->device, &sf->surface, &sf->swapchain))) {
        fu_error("Failed to create swapchain\n");
        return false;
    }
    sf->surface.window = window;
    return true;
}

bool t_surface_valid_check_and_exchange(FUSurface* sf, const bool outOfDate) { return false; }
#endif