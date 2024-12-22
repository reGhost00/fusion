#ifdef FU_RENDERER_TYPE_VK
#include <string.h>

#include <cglm/cglm.h>

#include "core/_base.h"
#include "core/logger.h"
#include "core/memory.h"

#include "wrapper.h"

#ifdef FU_USE_GLFWxx
void t_surface_destroy(TSurface* surface, VkInstance instance)
{
    vkDestroySurfaceKHR(instance, surface->surface, &defCustomAllocator);
    glfwDestroyWindow(surface->window);
    glfwTerminate();
}

bool t_surface_init(uint32_t width, uint32_t height, const char* title, VkInstance instance, TSurface* surface)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    surface->window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (FU_UNLIKELY(!surface->window)) {
        fu_error("failed to create window\n");
        return false;
    }
    if (FU_UNLIKELY(glfwCreateWindowSurface(instance, surface->window, &defCustomAllocator, &surface->surface))) {
        fu_error("Failed to create window surface\n");
        return false;
    }
    return true;
}

static void t_surface_update_extent(TSurface* surface, TDevice* device)
{
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->physicalDevice.physicalDevice, surface->surface, &surface->capabilities);
    if (~0U > surface->capabilities.currentExtent.width && ~0U > surface->capabilities.currentExtent.height)
        memcpy(&surface->extent, &surface->capabilities.currentExtent, sizeof(VkExtent2D));
    else {
        int width, height;
        glfwGetFramebufferSize(surface->window, &width, &height);
        surface->extent.width = glm_clamp(width, surface->capabilities.minImageExtent.width, surface->capabilities.maxImageExtent.width);
        surface->extent.height = glm_clamp(height, surface->capabilities.minImageExtent.height, surface->capabilities.maxImageExtent.height);
    }
}
#elif defined(FU_USE_SDLxx)
void t_surface_destroy(TSurface* surface, VkInstance instance)
{
    vkDestroySurfaceKHR(instance, surface->surface, NULL);
    SDL_DestroyWindow(surface->window);
    SDL_Quit();
}

bool t_surface_init(uint32_t width, uint32_t height, const char* title, VkInstance instance, TSurface* surface)
{
    if (FU_UNLIKELY(SDL_Init(SDL_INIT_VIDEO))) {
        fu_error("Failed to init SDL: %s\n", SDL_GetError());
        return false;
    }
    SDL_Vulkan_LoadLibrary(NULL);
    surface->window = SDL_CreateWindow("Vulkan Test Triangle", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (FU_UNLIKELY(!surface->window)) {
        fu_error("failed to create window\n");
        return false;
    }

    if (FU_UNLIKELY(!SDL_Vulkan_CreateSurface(surface->window, instance, &surface->surface))) {
        fu_error("Failed to create window surface: %s\n", SDL_GetError());
        return false;
    }
    return true;
}

#endif
static bool t_swapchain_init_image_view(TSwapchain* swapchain, TDevice* device, TSurface* surface)
{
    vkGetSwapchainImagesKHR(device->device, swapchain->swapchain, &swapchain->imageCount, NULL);
    VkImage* images = (VkImage*)fu_malloc(sizeof(VkImage) * swapchain->imageCount);
    swapchain->images = (TImage*)fu_malloc(sizeof(TImage) * swapchain->imageCount);
    vkGetSwapchainImagesKHR(device->device, swapchain->swapchain, &swapchain->imageCount, images);

    for (uint32_t i = 0; i < swapchain->imageCount; i++) {
        swapchain->images[i].image = fu_steal_pointer(&images[i]);
        if (FU_LIKELY(vk_image_view_init(device->device, surface->format.format, swapchain->images[i].image, VK_IMAGE_ASPECT_COLOR_BIT, 1, &swapchain->images[i].view)))
            continue;
        fu_error("failed to create image view\n");
        fu_free(images);
        return false;
    }
    fu_free(images);
    return true;
}

static void t_surface_update_extent(TSurface* surface, VkPhysicalDevice device)
{
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface->surface, &surface->capabilities);
    if (~0U > surface->capabilities.currentExtent.width && ~0U > surface->capabilities.currentExtent.height)
        memcpy(&surface->extent, &surface->capabilities.currentExtent, sizeof(VkExtent2D));
    else {
        int width, height;
#ifdef FU_USE_GLFW
        glfwGetFramebufferSize(surface->window, &width, &height);
#elif defined(FU_USE_SDL)
        SDL_Vulkan_GetDrawableSize(surface->window, &width, &height);
#endif
        surface->extent.width = glm_clamp(width, surface->capabilities.minImageExtent.width, surface->capabilities.maxImageExtent.width);
        surface->extent.height = glm_clamp(height, surface->capabilities.minImageExtent.height, surface->capabilities.maxImageExtent.height);
    }
}

bool t_swapchain_init_full(VkInstance instance, TDevice* device, TSurface* surface, TSwapchain* swapchain, VkSurfaceFormatKHR* format, VkPresentModeKHR presentMode)
{
    t_surface_update_extent(surface, device->physicalDevice.physicalDevice); // for resize
    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface->surface,
        .minImageCount = swapchain->imageCount,
        .imageFormat = format->format,
        .imageColorSpace = format->colorSpace,
        .imageExtent = surface->extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = surface->capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE
    };

    if (FU_UNLIKELY(vkCreateSwapchainKHR(device->device, &createInfo, &defCustomAllocator, &swapchain->swapchain))) {
        fu_error("failed to create swapchain\n");
        return false;
    }

    return t_swapchain_init_image_view(swapchain, device, surface);
}

bool t_swapchain_init(VkInstance instance, TDevice* device, TSurface* surface, TSwapchain* swapchain)
{
    TSurfaceFormat formats;
    t_surface_format_init(device->physicalDevice.physicalDevice, surface->surface, &formats);
    t_surface_update_extent(surface, device->physicalDevice.physicalDevice);
    surface->format = formats.formats[0];
    surface->presentMode = VK_PRESENT_MODE_FIFO_KHR;

    swapchain->imageCount = surface->capabilities.minImageCount + 1;
    if (0 < surface->capabilities.maxImageCount && swapchain->imageCount > surface->capabilities.maxImageCount)
        swapchain->imageCount = surface->capabilities.maxImageCount;
    for (uint32_t i = 0; i < formats.formatCount; i++) {
        if (VK_COLOR_SPACE_SRGB_NONLINEAR_KHR != formats.formats[i].colorSpace)
            continue;
        if (VK_FORMAT_R8G8B8A8_SRGB != formats.formats[i].format)
            continue;
        surface->format = formats.formats[i];
        break;
    }

    for (uint32_t i = 0; i < formats.presentModeCount; i++) {
        if (VK_PRESENT_MODE_MAILBOX_KHR != formats.presentModes[i])
            continue;
        surface->presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        break;
    }

    bool rev = t_swapchain_init_full(instance, device, surface, swapchain, &surface->format, surface->presentMode);
    t_surface_format_destroy(&formats);
    return rev;
}

void t_swapchain_destroy(TSwapchain* swapchain, TDevice* device)
{
    for (uint32_t i = 0; i < swapchain->imageCount; i++)
        vkDestroyImageView(device->device, swapchain->images[i].view, &defCustomAllocator);
    vkDestroySwapchainKHR(device->device, swapchain->swapchain, &defCustomAllocator);
    fu_free(swapchain->images);
}
#endif