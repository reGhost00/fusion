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

bool t_swapchain_init(VkInstance instance, TDevice* device, TSurface* surface, TSwapchain* swapchain)
{
    uint32_t formatCount, presentModeCount;

    vkGetPhysicalDeviceSurfaceFormatsKHR(device->physicalDevice.physicalDevice, surface->surface, &formatCount, NULL);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device->physicalDevice.physicalDevice, surface->surface, &presentModeCount, NULL);

    VkSurfaceFormatKHR* formats = (VkSurfaceFormatKHR*)fu_malloc(sizeof(VkSurfaceFormatKHR) * formatCount);
    VkPresentModeKHR* presentModes = (VkPresentModeKHR*)fu_malloc(sizeof(VkPresentModeKHR) * presentModeCount);

    vkGetPhysicalDeviceSurfaceFormatsKHR(device->physicalDevice.physicalDevice, surface->surface, &formatCount, formats);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device->physicalDevice.physicalDevice, surface->surface, &presentModeCount, presentModes);
    t_surface_update_extent(surface, device->physicalDevice.physicalDevice);

    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface->surface,
        .minImageCount = surface->capabilities.minImageCount + 1,
        .imageFormat = formats->format,
        .imageColorSpace = formats->colorSpace,
        .imageExtent = surface->extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = surface->capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .clipped = VK_TRUE
    };

    if (0 < surface->capabilities.maxImageCount && createInfo.minImageCount > surface->capabilities.maxImageCount)
        createInfo.minImageCount = surface->capabilities.maxImageCount;
    memcpy(&surface->format, formats, sizeof(VkSurfaceFormatKHR));
    for (uint32_t i = 0; i < formatCount; i++) {
        if (VK_COLOR_SPACE_SRGB_NONLINEAR_KHR != formats[i].colorSpace)
            continue;
        if (VK_FORMAT_R8G8B8A8_SRGB != formats[i].format)
            continue;
        memcpy(&surface->format, &formats[i], sizeof(VkSurfaceFormatKHR));
        createInfo.imageFormat = formats[i].format;
        createInfo.imageColorSpace = formats[i].colorSpace;
        break;
    }

    for (uint32_t i = 0; i < presentModeCount; i++) {
        if (VK_PRESENT_MODE_MAILBOX_KHR != presentModes[i])
            continue;
        createInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        surface->presentMode = createInfo.presentMode;
        break;
    }

    bool rev = false;
    if (FU_UNLIKELY(vkCreateSwapchainKHR(device->device, &createInfo, &defCustomAllocator, &swapchain->swapchain))) {
        fu_error("failed to create swapchain\n");
        goto out;
    }

    rev = t_swapchain_init_image_view(swapchain, device, surface);
out:
    fu_free(formats);
    fu_free(presentModes);
    return rev;
}

bool t_swapchain_update(TSwapchain* swapchain, TDevice* device, TSurface* surface)
{
    t_swapchain_destroy(swapchain, device);
    t_surface_update_extent(surface, device->physicalDevice.physicalDevice);

    const VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface->surface,
        .minImageCount = surface->capabilities.minImageCount + 1,
        .imageFormat = surface->format.format,
        .imageColorSpace = surface->format.colorSpace,
        .imageExtent = surface->extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = surface->capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = surface->presentMode,
        .clipped = VK_TRUE
    };
    if (FU_UNLIKELY(vkCreateSwapchainKHR(device->device, &createInfo, &defCustomAllocator, &swapchain->swapchain))) {
        fu_error("failed to create swapchain\n");
        return false;
    }

    return t_swapchain_init_image_view(swapchain, device, surface);
}

void t_swapchain_destroy(TSwapchain* swapchain, TDevice* device)
{
    for (uint32_t i = 0; i < swapchain->imageCount; i++)
        vkDestroyImageView(device->device, swapchain->images[i].view, &defCustomAllocator);
    vkDestroySwapchainKHR(device->device, swapchain->swapchain, &defCustomAllocator);
    fu_free(swapchain->images);
}
#endif