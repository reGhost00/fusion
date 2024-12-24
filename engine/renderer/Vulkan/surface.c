#include <stdio.h>
#ifdef FU_RENDERER_TYPE_VK
#include "core/array.h"
#include "core/logger.h"
#include "core/memory.h"

#include "../misc.h"
#include "misc.inner.h"

#ifdef FU_ENABLE_MESSAGE_CALLABLE
#define T_INSTANCE(sf) ((sf)->instance.instance)
#else
#define T_INSTANCE(sf) ((sf)->instance)
#endif

#define T_DEVICE(sf) ((sf)->device.device)
#define T_SURFACE(sf) ((sf)->surface.surface)
#define T_SWAPCHAIN(sf) ((sf)->swapchain.swapchain)

void t_surface_format_init(VkPhysicalDevice device, VkSurfaceKHR surface, TSurfaceFormat* formats)
{

    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formats->formatCount, NULL);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &formats->presentModeCount, NULL);

    formats->formats = (VkSurfaceFormatKHR*)fu_malloc(sizeof(VkSurfaceFormatKHR) * formats->formatCount);
    formats->presentModes = (VkPresentModeKHR*)fu_malloc(sizeof(VkPresentModeKHR) * formats->presentModeCount);

    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formats->formatCount, formats->formats);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &formats->presentModeCount, formats->presentModes);
}

void t_surface_format_destroy(TSurfaceFormat* formats)
{
    fu_free(formats->formats);
    fu_free(formats->presentModes);
}

void fu_surface_destroy(FUSurface* surface)
{
    fu_surface_t* sf = (fu_surface_t*)surface;
    vkDeviceWaitIdle(T_DEVICE(sf));
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

bool fu_surface_check_and_exchange(FUSurface* surface, const bool outOfDate)
{
    fu_surface_t* sf = (fu_surface_t*)surface;
    bool rev = sf->swapchain.outOfDate;
    if (outOfDate != rev)
        sf->swapchain.outOfDate = outOfDate;
    return rev;
}

static void fu_surface_cleanup(fu_surface_t* sf, FUPtrArray* contexts)
{
    FUContext* ctx;
    vkDeviceWaitIdle(T_DEVICE(sf));
    for (uint32_t i = 0; i < contexts->len; i++) {
        ctx = fu_ptr_array_index(contexts, i);
        for (uint32_t j = 0; j < sf->swapchain.imageCount; j++)
            vkDestroyFramebuffer(T_DEVICE(sf), ctx->frame.buffers[j], &defCustomAllocator);
    }
    for (uint32_t i = 0; i < sf->swapchain.imageCount; i++)
        vkDestroyImageView(T_DEVICE(sf), sf->swapchain.images[i].view, &defCustomAllocator);
    vkDestroySwapchainKHR(T_DEVICE(sf), T_SWAPCHAIN(sf), &defCustomAllocator);
    fu_free(sf->swapchain.images);
}

bool fu_surface_update(FUSurface* surface, FUPtrArray* contexts)
{
    fu_surface_t* sf = (fu_surface_t*)surface;
    FUContext* ctx;
    fu_print_func_name();
    fu_surface_cleanup(sf, contexts);
    if (FU_LIKELY(t_swapchain_init_full(T_INSTANCE(sf), &sf->device, &sf->surface, &sf->swapchain, &sf->surface.format, sf->surface.presentMode))) {
        for (uint32_t i = 0; i < contexts->len; i++) {
            if (FU_LIKELY(fu_context_init_framebuffers(fu_ptr_array_index(contexts, i), sf)))
                continue;
            fu_error("Failed to update framebuffers @context[%u]\n", i);
            return false;
        }
        return true;
    }
    fu_error("Failed to update swapchain\n");
    return false;
}
#endif