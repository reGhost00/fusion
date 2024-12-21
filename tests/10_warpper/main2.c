#include <stdio.h>
#include <stdlib.h>
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cglm/cglm.h>
#define SDL_MAIN_HANDLED
#include "core/_base.h"
#include "core/memory.h"
#include "libs/stb_image.h"
#include "libs/vk_mem_alloc.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <mimalloc.h>
#include <vulkan/vulkan.h>

#define DEF_WIN_WIDTH 800
#define DEF_WIN_HEIGHT 600

#define fu_error(format, ...) fprintf(stderr, "\033[91;49m" format "\033[0m", ##__VA_ARGS__)
#define fu_warning(format, ...) fprintf(stderr, "\033[93;49m" format "\033[0m", ##__VA_ARGS__)
#define fu_info(format, ...) fprintf(stderr, "\033[92;49m" format "\033[0m", ##__VA_ARGS__)
#define fu_print_func_name(...) (printf("%s "__VA_ARGS__ \
                                        "\n",            \
    __func__))
static void* _vk_malloc(void* usd, size_t size, size_t alignment, VkSystemAllocationScope scope) { return mi_aligned_alloc(alignment, size); }
static void* _vk_realloc(void* usd, void* p, size_t size, size_t alignment, VkSystemAllocationScope scope) { return mi_aligned_recalloc(p, 1, size, alignment); }
static void _vk_free(void* usd, void* p) { mi_free(p); }

static VkAllocationCallbacks defCustomAllocator = {
    .pfnAllocation = _vk_malloc,
    .pfnReallocation = _vk_realloc,
    .pfnFree = _vk_free
};
//
//  buffer allocator

static struct _def_allocator {
    VmaAllocator defAllocator;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
} _defAlloc;

void tvk_uniform_buffer_allocator_destroy()
{
    vmaDestroyAllocator(_defAlloc.defAllocator);
}

bool tvk_uniform_buffer_allocator_init(VkInstance instance, VkDevice device, VkPhysicalDevice physicalDevice)
{
    VmaAllocatorCreateInfo allocInfo = {
        .flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT,
        .physicalDevice = physicalDevice,
        .device = device,
        .pAllocationCallbacks = &defCustomAllocator,
        .instance = instance,
        .vulkanApiVersion = VK_API_VERSION_1_3
    };
    if (vmaCreateAllocator(&allocInfo, &_defAlloc.defAllocator))
        return false;
    _defAlloc.device = device;
    _defAlloc.physicalDevice = physicalDevice;
    return true;
}

typedef struct _TVertex {
    vec3 position;
    vec4 color;
    vec2 uv;
} TVertex;

typedef struct _TUniformBufferObject {
    alignas(16) mat4 model;
    alignas(16) mat4 view;
    alignas(16) mat4 proj;
} TUniformBufferObject;

static const TVertex vertices[] = {
    { { -0.5f, -0.5f }, { 1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f } },
    { { -0.5f, 0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { { 0.5f, 0.5f }, { 0.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
    { { 0.5f, -0.5f }, { 1.0f, 0.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },

    { { -0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f } },
    { { -0.5f, 0.5f, -0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { { 0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
    { { 0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } }
};

static const uint32_t indices[] = {
    0, 1, 2, 0, 2, 3,
    4, 5, 6, 4, 6, 7
};
#define DEF_IMG "E:\\Pictures\\Koala.jpg"
// #define DEF_IMG "E:\\Pictures\\bz_ladfdfdg_0011.jpg"

#define t_vertex_binding_free(binding) fu_free(binding)
VkVertexInputBindingDescription* t_vertex_get_binding()
{
    VkVertexInputBindingDescription* binding = fu_malloc0(sizeof(VkVertexInputBindingDescription));
    binding->stride = sizeof(TVertex);
    binding->inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return binding;
}

#define t_vertex_attributes_free(desc) fu_free(desc)
VkVertexInputAttributeDescription* t_vertex_get_attributes(uint32_t* count)
{
    VkVertexInputAttributeDescription* desc = fu_malloc0_n(3, sizeof(VkVertexInputAttributeDescription));
    desc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    desc[1].location = 1;
    desc[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    desc[1].offset = offsetof(TVertex, color);
    desc[2].location = 2;
    desc[2].format = VK_FORMAT_R32G32_SFLOAT;
    desc[2].offset = offsetof(TVertex, uv);
    *count = 3;
    return desc;
}

static struct _vk_fns_t {
    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
    PFN_vkTransitionImageLayoutEXT vkTransitionImageLayoutEXT;
    PFN_vkCopyMemoryToImageEXT vkCopyMemoryToImageEXT;
} vkfns;

void tvk_fns_init_instance(VkInstance instance)
{
    vkfns.vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    vkfns.vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    vkfns.vkTransitionImageLayoutEXT = (PFN_vkTransitionImageLayoutEXT)vkGetInstanceProcAddr(instance, "vkTransitionImageLayoutEXT");
    vkfns.vkCopyMemoryToImageEXT = (PFN_vkCopyMemoryToImageEXT)vkGetInstanceProcAddr(instance, "vkCopyMemoryToImageEXT");
}

VkResult vkCreateDebugUtilsMessengerEXT(VkInstance inst, const VkDebugUtilsMessengerCreateInfoEXT* info, const VkAllocationCallbacks* cb, VkDebugUtilsMessengerEXT* debugger)
{
    return vkfns.vkCreateDebugUtilsMessengerEXT(inst, info, cb, debugger);
}

void vkDestroyDebugUtilsMessengerEXT(VkInstance inst, VkDebugUtilsMessengerEXT debugger, const VkAllocationCallbacks* cb)
{
    vkfns.vkDestroyDebugUtilsMessengerEXT(inst, debugger, cb);
}

VkResult vkTransitionImageLayoutEXT(VkDevice device, uint32_t transitionCount, const VkHostImageLayoutTransitionInfoEXT* pTransitions)
{
    return vkfns.vkTransitionImageLayoutEXT(device, transitionCount, pTransitions);
}
VkResult vkCopyMemoryToImageEXT(VkDevice device, const VkCopyMemoryToImageInfoEXT* pInfo)
{
    return vkfns.vkCopyMemoryToImageEXT(device, pInfo);
}

typedef struct _TFence {
    VkFence* fences;
    VkDevice device;
    uint32_t count;
} TFence;

typedef struct _TCommandBuffer {
    VkCommandBuffer buffer;
    VkCommandPool pool;
    VkDevice device;
} TCommandBuffer;

typedef struct _TImage {
    VkImage image;
    VkImageView view;
} TImage;

typedef struct _TImageBuffer {
    VmaAllocation alloc;
    VmaAllocationInfo allocInfo;
    TImage buff;
    VkImageLayout layout;
    VkFormat format;
    VkExtent2D extent;
} TImageBuffer;

typedef struct _TUniformBuffer {
    VmaAllocation alloc;
    VmaAllocationInfo allocInfo;
    VkBuffer buffer;
} TUniformBuffer;

typedef struct _TSync {
    VkSemaphore* imageAvailable;
    VkSemaphore* renderFinished;
    VkFence* inFlight;
} TSync;

typedef struct _TBuffInitArgs {
    TFence* fence;
    TUniformBuffer* vertex;
    TUniformBuffer* index;
    TCommandBuffer* vertTranCmd;
    TCommandBuffer* idxTranCmd;
    TCommandBuffer* depthTranCmd;
} TBuffInitArgs;

typedef struct _TDrawing {
    TBuffInitArgs* args;
    TUniformBuffer vertexBuffer;
    TUniformBuffer indexBuffer;
    VkCommandPool commandPool;
    VkCommandBuffer* commandBuffers;
    VkFramebuffer* framebuffers;
} TDrawing;
#define T_FRAMEBUFFER(app) (app->drawing.framebuffers)
#define T_CMD_POOL(app) (app->drawing.commandPool)
#define T_CMD_BUFF(app) (app->drawing.commandBuffers)
#define T_VERTEX_BUFFER(app) (app->drawing.vertexBuffer)
#define T_INDEX_BUFFER(app) (app->drawing.indexBuffer)

typedef struct _TDescription {
    TUniformBuffer* uniformBuffer;
    TImageBuffer imageBuffer;
    TImageBuffer depthBuffer;
    VkDescriptorSet* sets;
    VkDescriptorSetLayout layout;
    VkDescriptorPool pool;
    VkSampler sampler;
    stbi_uc* pixels;
} TDescription;
#define T_DESC_POOL(app) (app->description.pool)
#define T_DESC_LAYOUT(app) (app->description.layout)
#define T_DESC_SETS(app) (app->description.sets)
#define T_DESC_BUFF(app) (app->description.uniformBuffer)
#define T_DESC_IMAGE(app) (app->description.imageBuffer)
#define T_DESC_DEPTH(app) (app->description.depthBuffer)
#define T_DESC_SAMPLER(app) (app->description.sampler)

typedef struct _TPipelineLibrary {
    VkPipelineCache cache;
    VkPipeline vertexInputInterface;
    VkPipeline preRasterizationShaders;
    VkPipeline fragmentOutputInterface;
    VkPipeline fragmentShaders;
} TPipelineLibrary;

typedef struct _TPipeline {
    TPipelineLibrary* library;
    VkPipeline pipeline;
    VkPipelineLayout layout;
    VkRenderPass renderPass;
} TPipeline;
#define T_RENDER_PASS(app) (app->pipeline.renderPass)
#define T_PIPELINE_LAYOUT(app) (app->pipeline.layout)
#define T_PIPELINE(app) (app->pipeline.pipeline)
#define T_PIPELINE_CACHE(app) (app->pipeline.library->cache)

typedef struct _TSwapchain {
    VkSwapchainKHR swapchain;
    TImage* images;
    uint32_t imageCount;
    bool outOfDate;
} TSwapchain;
#define T_SWAPCHAIN(app) (app->swapchain.swapchain)
#define T_SWAPCHAIN_IMAGES(app) (app->swapchain.images)

typedef struct _TQueueFamilyProperties {
    VkQueueFamilyProperties properties;
    uint32_t index;
    bool presentSupported;
} TQueueFamilyProperties;

typedef struct _TPhysicalDevice {
    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceProperties properties;
    VkExtensionProperties* extensions;
    TQueueFamilyProperties* queueFamilies;
    TQueueFamilyProperties* graphicsQueueFamilies;
    TQueueFamilyProperties* computeQueueFamilies;
    TQueueFamilyProperties* transferQueueFamilies;
    uint32_t extensionCount, queueFamilyCount, graphicsQueueFamilyCount, computeQueueFamilyCount, transferQueueFamilyCount;
} TPhysicalDevice;
#define T_PHYSICAL_DEVICE(app) (app->device.physicalDevice.physicalDevice)

typedef struct _TDevice {
    TPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue generalQueue;
    uint32_t queueFamilyIndex;
} TDevice;
#define T_DEVICE(app) (app->device.device)
#define T_QUEUE(app) (app->device.generalQueue)

typedef struct _TSurfaceDetail {
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR* format;
    VkPresentModeKHR* presentMode;
    uint32_t formatCount, presentModeCount;
} TSurfaceDetail;

typedef struct _TSurface {
    VkSurfaceKHR surface;
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR format;
    VkPresentModeKHR presentMode;
    VkExtent2D extent;
    SDL_Window* window;
} TSurface;
#define T_SURFACE(app) (app->surface.surface)
#define T_WINDOW(app) (app->surface.window)

typedef struct _TApp {
    TSync sync;
    TDrawing drawing;
    TPipeline pipeline;
    TDescription description;
    TSwapchain swapchain;
    TDevice device;
    TSurface surface;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugger;
} TApp;

//
//  common
// static bool tvk_check_stencil_format(VkFormat format)
// {
//     return VK_FORMAT_D32_SFLOAT_S8_UINT == format || VK_FORMAT_D24_UNORM_S8_UINT == format;
// }

TFence* tvk_fence_new(VkDevice device, uint32_t count, bool signaled)
{
    VkFenceCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
    };
    if (signaled)
        info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VkFence* fences = (VkFence*)fu_malloc(sizeof(VkFence) * count);
    for (uint32_t i = 0; i < count; i++) {
        if (FU_LIKELY(!vkCreateFence(device, &info, &defCustomAllocator, &fences[i])))
            continue;
        fu_error("Failed to create fence\n");
        fu_free(fences);
        return NULL;
    }
    TFence* rev = fu_malloc(sizeof(TFence));
    rev->fences = fu_steal_pointer(&fences);
    rev->device = device;
    rev->count = count;
    return rev;
}

void tvk_fence_free(TFence* fence)
{
    if (FU_UNLIKELY(!fence))
        return;
    for (uint32_t i = 0; i < fence->count; i++)
        vkDestroyFence(fence->device, fence->fences[i], &defCustomAllocator);
    fu_free(fence->fences);
    fu_free(fence);
}

static void tvk_command_buffer_free(TCommandBuffer* cmdf)
{
    if (!cmdf)
        return;
    vkFreeCommandBuffers(cmdf->device, cmdf->pool, 1, &cmdf->buffer);
    fu_free(cmdf);
}

TCommandBuffer* tvk_command_buffer_submit_free(TCommandBuffer* cmdf, VkQueue queue, VkFence fence)
{
    vkEndCommandBuffer(cmdf->buffer);

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmdf->buffer
    };

    if (fence) {
        vkQueueSubmit(queue, 1, &submitInfo, fence);
        return cmdf;
    }
    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);
    vkFreeCommandBuffers(cmdf->device, cmdf->pool, 1, &cmdf->buffer);
    fu_free(cmdf);
    return NULL;
}

static TCommandBuffer* tvk_command_buffer_new_once(VkDevice device, VkCommandPool pool)
{
    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    TCommandBuffer* cmdf = fu_malloc(sizeof(TCommandBuffer));
    if (vkAllocateCommandBuffers(device, &allocInfo, &cmdf->buffer)) {
        fu_error("Failed to allocate command buffers!");
        fu_free(cmdf);
        return NULL;
    }

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    if (vkBeginCommandBuffer(cmdf->buffer, &beginInfo)) {
        fu_error("Failed to begin recording command buffer!");
        vkFreeCommandBuffers(device, pool, 1, &cmdf->buffer);
        fu_free(cmdf);
        return NULL;
    }
    cmdf->device = device;
    cmdf->pool = pool;
    return cmdf;
}

static void tvk_uniform_buffer_destroy(TUniformBuffer* buff)
{
    vmaDestroyBuffer(_defAlloc.defAllocator, buff->buffer, buff->alloc);
}

static void tvk_uniform_buffer_free(TUniformBuffer* buff)
{
    vmaDestroyBuffer(_defAlloc.defAllocator, buff->buffer, buff->alloc);
    fu_free(buff);
}

static bool tvk_uniform_buffer_init(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags, TUniformBuffer* tar)
{
    VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };
    VmaAllocationCreateInfo allocInfo = {
        .usage = VMA_MEMORY_USAGE_AUTO,
        .flags = flags
    };

    if (vmaCreateBuffer(_defAlloc.defAllocator, &bufferInfo, &allocInfo, &tar->buffer, &tar->alloc, &tar->allocInfo)) {
        fu_error("Failed to create vertex buffer");
        return false;
    }
    return true;
}

static TUniformBuffer* tvk_uniform_buffer_new(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags)
{
    TUniformBuffer* buffer = fu_malloc(sizeof(TUniformBuffer));
    if (!tvk_uniform_buffer_init(size, usage, flags, buffer)) {
        fu_free(buffer);
        return NULL;
    }
    return buffer;
}

static TCommandBuffer* tvk_uniform_buffer_copy(VkCommandPool commandPool, VkQueue queue, TUniformBuffer* dst, TUniformBuffer* src, VkFence fence)
{
    TCommandBuffer* cmdf = tvk_command_buffer_new_once(_defAlloc.device, commandPool);
    if (!cmdf)
        return NULL;
    VkBufferCopy copyRegion = { .size = src->allocInfo.size };
    vkCmdCopyBuffer(cmdf->buffer, src->buffer, dst->buffer, 1, &copyRegion);
    return tvk_command_buffer_submit_free(cmdf, queue, fence);
}

static bool tvk_image_view_init(VkDevice device, VkFormat format, VkImage image, VkImageAspectFlags aspect, VkImageView* view)
{
    VkImageViewCreateInfo viewInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange = {
            .aspectMask = aspect, // VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1 }
    };
    if (vkCreateImageView(device, &viewInfo, &defCustomAllocator, view)) {
        fu_error("Failed to create image view\n");
        return false;
    }
    return true;
}

static void tvk_image_buffer_destroy(TImageBuffer* buff)
{
    vmaDestroyImage(_defAlloc.defAllocator, buff->buff.image, buff->alloc);
    vkDestroyImageView(_defAlloc.device, buff->buff.view, &defCustomAllocator);
}

static bool tvk_image_buffer_init(VkImageUsageFlags usage, VmaAllocationCreateFlags flags, VkFormat format, VkImageAspectFlags aspect, TImageBuffer* tar)
{
    VkImageCreateInfo imageInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format, // VK_FORMAT_R8G8B8A8_SRGB,
        .extent = {
            .width = tar->extent.width,
            .height = tar->extent.height,
            .depth = 1 },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    VmaAllocationCreateInfo allocInfo = {
        .flags = flags,
        .usage = VMA_MEMORY_USAGE_AUTO,
        .priority = 1.0f
    };

    if (vmaCreateImage(_defAlloc.defAllocator, &imageInfo, &allocInfo, &tar->buff.image, &tar->alloc, &tar->allocInfo)) {
        fu_error("Failed to create image buffer\n");
        return false;
    }
    if (!tvk_image_view_init(_defAlloc.device, imageInfo.format, tar->buff.image, aspect, &tar->buff.view)) {
        fu_error("Failed to create image view\n");
        tvk_image_buffer_destroy(tar);
        return false;
    }
    tar->layout = VK_IMAGE_LAYOUT_UNDEFINED;
    tar->format = format;
    return true;
}

static bool tvk_image_buffer_init_from_path(const char* path, TImageBuffer* tar)
{
    int channels;
    stbi_uc* pixels = stbi_load(path, (int*)&tar->extent.width, (int*)&tar->extent.height, &channels, STBI_rgb_alpha);
    if (!pixels) {
        fu_error("Failed to load texture image!\n");
        return false;
    }

    if (FU_UNLIKELY(!tvk_image_buffer_init(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_HOST_TRANSFER_BIT_EXT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, tar))) {
        stbi_image_free(pixels);
        return false;
    }

    VkHostImageLayoutTransitionInfoEXT transitionInfo = {
        .sType = VK_STRUCTURE_TYPE_HOST_IMAGE_LAYOUT_TRANSITION_INFO_EXT,
        .image = tar->buff.image,
        .oldLayout = tar->layout,
        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1 }
    };

    if (FU_UNLIKELY(vkTransitionImageLayoutEXT(_defAlloc.device, 1, &transitionInfo))) {
        fu_error("Failed to transition image layout!\n");
        goto out;
    }

    VkMemoryToImageCopyEXT mipCopy = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_TO_IMAGE_COPY_EXT,
        .pHostPointer = pixels,
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .layerCount = 1 },
        .imageExtent = { .width = tar->extent.width, .height = tar->extent.height, .depth = 1 },
    };
    VkCopyMemoryToImageInfoEXT copyInfo = {
        .sType = VK_STRUCTURE_TYPE_COPY_MEMORY_TO_IMAGE_INFO_EXT,
        .dstImage = tar->buff.image,
        .dstImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .regionCount = 1,
        .pRegions = &mipCopy
    };

    if (FU_UNLIKELY(vkCopyMemoryToImageEXT(_defAlloc.device, &copyInfo))) {
        fu_error("Failed to copy memory to image\n");
        goto out;
    }

    stbi_image_free(pixels);
    return true;
out:
    stbi_image_free(pixels);
    tvk_image_buffer_destroy(tar);
    return false;
}

static bool tvk_sampler_init(VkFilter filter, VkSamplerMipmapMode minimap, VkSamplerAddressMode addr, VkSampler* sampler)
{
    VkPhysicalDeviceProperties properties = {};
    vkGetPhysicalDeviceProperties(_defAlloc.physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = filter,
        .minFilter = filter,
        .mipmapMode = minimap,
        .addressModeU = addr,
        .addressModeV = addr,
        .addressModeW = addr,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK
    };

    if (FU_UNLIKELY(vkCreateSampler(_defAlloc.device, &samplerInfo, &defCustomAllocator, sampler))) {
        fu_error("Failed to create image sampler\n");
        return false;
    }
    return true;
}

//
//  Sync

static void t_app_destroy_sync(TApp* app)
{
    for (uint32_t i = 0; i < app->swapchain.imageCount; i++) {
        vkDestroySemaphore(T_DEVICE(app), app->sync.imageAvailable[i], &defCustomAllocator);
        vkDestroySemaphore(T_DEVICE(app), app->sync.renderFinished[i], &defCustomAllocator);
        vkDestroyFence(T_DEVICE(app), app->sync.inFlight[i], &defCustomAllocator);
    }
    fu_free(app->sync.imageAvailable);
    fu_free(app->sync.renderFinished);
    fu_free(app->sync.inFlight);
}

static bool t_app_init_sync(TApp* app)
{
    VkSemaphoreCreateInfo semaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };
    VkFenceCreateInfo fenceInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };
    bool rev = false;
    app->sync.imageAvailable = (VkSemaphore*)fu_malloc0_n(app->swapchain.imageCount, sizeof(VkSemaphore));
    app->sync.renderFinished = (VkSemaphore*)fu_malloc0_n(app->swapchain.imageCount, sizeof(VkSemaphore));
    app->sync.inFlight = (VkFence*)fu_malloc0_n(app->swapchain.imageCount, sizeof(VkFence));
    for (uint32_t i = 0; i < app->swapchain.imageCount; i++) {
        rev = !vkCreateSemaphore(T_DEVICE(app), &semaphoreInfo, &defCustomAllocator, &app->sync.imageAvailable[i]);
        rev = rev && !vkCreateSemaphore(T_DEVICE(app), &semaphoreInfo, &defCustomAllocator, &app->sync.renderFinished[i]);
        rev = rev && !vkCreateFence(T_DEVICE(app), &fenceInfo, &defCustomAllocator, &app->sync.inFlight[i]);
        if (!rev)
            return false;
    }

    return rev;
}

//
//  drawing stuff
static void t_app_destroy_buffers__init_args(TApp* app)
{
    if (!app->drawing.args)
        return;

    tvk_uniform_buffer_free(app->drawing.args->vertex);
    tvk_uniform_buffer_free(app->drawing.args->index);

    tvk_command_buffer_free(app->drawing.args->vertTranCmd);
    tvk_command_buffer_free(app->drawing.args->idxTranCmd);
    // tvk_command_buffer_free(app->drawing.args->depthTranCmd);

    tvk_fence_free(app->drawing.args->fence);
    fu_free(app->drawing.args);
    app->drawing.args = NULL;
}

static void t_app_init_buffers__init_args(TApp* app)
{
    app->drawing.args = fu_malloc0(sizeof(TBuffInitArgs));
    app->drawing.args->fence = tvk_fence_new(T_DEVICE(app), 2, false);
}

static void t_app_destroy_buffers(TApp* app)
{
    t_app_destroy_buffers__init_args(app);
    for (uint32_t i = 0; i < app->swapchain.imageCount; i++)
        vkDestroyFramebuffer(T_DEVICE(app), T_FRAMEBUFFER(app)[i], &defCustomAllocator);
    vkDestroyCommandPool(T_DEVICE(app), T_CMD_POOL(app), &defCustomAllocator);
    tvk_uniform_buffer_destroy(&T_VERTEX_BUFFER(app));
    tvk_uniform_buffer_destroy(&T_INDEX_BUFFER(app));
    fu_free(T_FRAMEBUFFER(app));
    fu_free(T_CMD_BUFF(app));
}

static bool t_app_init_buffers__framebuffers(TApp* app)
{
    VkImageView attachments[] = { NULL, T_DESC_DEPTH(app).buff.view };
    VkFramebufferCreateInfo framebufferInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = T_RENDER_PASS(app),
        .attachmentCount = FU_N_ELEMENTS(attachments),
        .pAttachments = attachments,
        .width = app->surface.extent.width,
        .height = app->surface.extent.height,
        .layers = 1
    };

    for (uint32_t i = 0; i < app->swapchain.imageCount; i++) {
        attachments[0] = T_SWAPCHAIN_IMAGES(app)[i].view;
        if (vkCreateFramebuffer(T_DEVICE(app), &framebufferInfo, &defCustomAllocator, &T_FRAMEBUFFER(app)[i])) {
            fu_error("Failed to create framebuffer\n");
            return false;
        }
    }

    return true;
}

static bool t_app_init_buffers__vertex(TApp* app)
{
    TUniformBuffer* staging = tvk_uniform_buffer_new(sizeof(vertices), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
    if (!staging)
        return false;
    memcpy(staging->allocInfo.pMappedData, vertices, sizeof(vertices));

    if (!tvk_uniform_buffer_init(sizeof(vertices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &T_VERTEX_BUFFER(app))) {
        fu_error("Failed to create vertex buffer\n");
        tvk_uniform_buffer_free(staging);
        return false;
    }
    app->drawing.args->vertTranCmd = tvk_uniform_buffer_copy(T_CMD_POOL(app), T_QUEUE(app), &T_VERTEX_BUFFER(app), staging, app->drawing.args->fence->fences[0]);
    if (!app->drawing.args->vertTranCmd) {
        tvk_uniform_buffer_free(staging);
        return false;
    }

    app->drawing.args->vertex = fu_steal_pointer(&staging);
    return true;
}

static bool t_app_init_buffers__index(TApp* app)
{
    TUniformBuffer* staging = tvk_uniform_buffer_new(sizeof(indices), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
    if (!staging)
        return false;
    memcpy(staging->allocInfo.pMappedData, indices, sizeof(indices));

    if (!tvk_uniform_buffer_init(sizeof(indices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &T_INDEX_BUFFER(app))) {
        fu_error("Failed to create index buffer\n");
        tvk_uniform_buffer_free(staging);
        return false;
    }
    app->drawing.args->idxTranCmd = tvk_uniform_buffer_copy(T_CMD_POOL(app), T_QUEUE(app), &T_INDEX_BUFFER(app), staging, app->drawing.args->fence->fences[1]);
    if (!app->drawing.args->idxTranCmd) {
        tvk_uniform_buffer_free(staging);
        return false;
    }

    app->drawing.args->index = fu_steal_pointer(&staging);
    return true;
}

static bool t_app_init_buffers(TApp* app)
{
    T_FRAMEBUFFER(app) = fu_malloc0_n(app->swapchain.imageCount, sizeof(VkFramebuffer));
    if (!t_app_init_buffers__framebuffers(app))
        return false;

    VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = app->device.queueFamilyIndex,
    };
    if (vkCreateCommandPool(T_DEVICE(app), &poolInfo, &defCustomAllocator, &T_CMD_POOL(app))) {
        fu_error("Failed to create command pool\n");
        return false;
    }
    t_app_init_buffers__init_args(app);

    if (!t_app_init_buffers__vertex(app))
        return false;
    if (!t_app_init_buffers__index(app))
        return false;
    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = T_CMD_POOL(app),
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = app->swapchain.imageCount
    };
    T_CMD_BUFF(app) = fu_malloc0_n(app->swapchain.imageCount, sizeof(VkCommandBuffer));
    if (vkAllocateCommandBuffers(T_DEVICE(app), &allocInfo, T_CMD_BUFF(app))) {
        fu_error("Failed to allocate command buffers\n");
        return false;
    }
    vkWaitForFences(T_DEVICE(app), 2, app->drawing.args->fence->fences, VK_TRUE, ~0U);
    t_app_destroy_buffers__init_args(app);
    return true;
}

static bool t_command_buffer_recording(TApp* app, VkCommandBuffer cmdf, uint32_t imageIndex)
{
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pInheritanceInfo = NULL
    };
    if (vkBeginCommandBuffer(cmdf, &beginInfo)) {
        fu_error("Failed to begin recording command buffer\n");
        return false;
    }
    static const VkClearValue clearColor[] = {
        { { { 0.0f, 0.0f, 0.0f, 1.0f } } },
        { .depthStencil = { 1.0f, 0 } },
    };
    VkRenderPassBeginInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = T_RENDER_PASS(app),
        .framebuffer = T_FRAMEBUFFER(app)[imageIndex],
        .renderArea = {
            .extent = app->surface.extent },
        .clearValueCount = FU_N_ELEMENTS(clearColor),
        .pClearValues = clearColor
    };
    vkCmdBeginRenderPass(cmdf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmdf, VK_PIPELINE_BIND_POINT_GRAPHICS, T_PIPELINE(app));

    VkViewport viewport = {
        .width = (float)app->surface.extent.width,
        .height = (float)app->surface.extent.height,
        .maxDepth = 1.0f
    };
    VkRect2D scissor = {
        .extent = app->surface.extent
    };
    VkBuffer vertexBuffer[] = { T_VERTEX_BUFFER(app).buffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdSetViewport(cmdf, 0, 1, &viewport);
    vkCmdSetScissor(cmdf, 0, 1, &scissor);
    vkCmdBindVertexBuffers(cmdf, 0, 1, vertexBuffer, offsets);
    vkCmdBindIndexBuffer(cmdf, T_INDEX_BUFFER(app).buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(cmdf, VK_PIPELINE_BIND_POINT_GRAPHICS, app->pipeline.layout, 0, 1, &T_DESC_SETS(app)[imageIndex], 0, NULL);
    vkCmdDrawIndexed(cmdf, FU_N_ELEMENTS(indices), 1, 0, 0, 0);

    vkCmdEndRenderPass(cmdf);
    if (vkEndCommandBuffer(cmdf)) {
        fu_error("Failed to record command buffer\n");
        return false;
    }
    return true;
}

//
//  pipeline
typedef struct _TPipelineCache {
    size_t size;
    char data[];
} TPipelineCache;
static void t_app_pipeline_library_destroy(TApp* app)
{
    FILE* fp = fopen("pipeline_cache.bin", "wb");

    if (fp) {
        size_t cacheSize;
        vkGetPipelineCacheData(T_DEVICE(app), T_PIPELINE_CACHE(app), &cacheSize, NULL);
        size_t blockSize = sizeof(TPipelineCache) + cacheSize;
        TPipelineCache* block = fu_malloc(blockSize);
        vkGetPipelineCacheData(T_DEVICE(app), T_PIPELINE_CACHE(app), &cacheSize, block->data);
        block->size = cacheSize;
        fwrite(block, blockSize, 1, fp);
        fu_free(block);
        fclose(fp);
    }
    vkDestroyPipelineCache(T_DEVICE(app), T_PIPELINE_CACHE(app), &defCustomAllocator);
    vkDestroyPipeline(T_DEVICE(app), app->pipeline.library->fragmentOutputInterface, &defCustomAllocator);
    vkDestroyPipeline(T_DEVICE(app), app->pipeline.library->fragmentShaders, &defCustomAllocator);
    vkDestroyPipeline(T_DEVICE(app), app->pipeline.library->preRasterizationShaders, &defCustomAllocator);
    vkDestroyPipeline(T_DEVICE(app), app->pipeline.library->vertexInputInterface, &defCustomAllocator);
    fu_free(app->pipeline.library);
}

static void t_app_pipeline_library_init(TApp* app)
{
    VkPipelineCacheCreateInfo pipelineCacheInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO
    };
    TPipelineCache* block = NULL;
    TPipelineLibrary* lib = fu_malloc0(sizeof(TPipelineLibrary));
    FILE* fp = fopen("pipeline_cache.bin", "r"); // TODO: use vkCreatePipelineCache", const char *)
    if (fp) {
        TPipelineCache cache;
        fread(&cache, sizeof(TPipelineCache), 1, fp);
        block = fu_malloc0(sizeof(TPipelineCache) + cache.size);
        fread(block->data, cache.size, 1, fp);
        pipelineCacheInfo.initialDataSize = cache.size;
        pipelineCacheInfo.pInitialData = block->data;
        fclose(fp);
    }
    if (vkCreatePipelineCache(T_DEVICE(app), &pipelineCacheInfo, &defCustomAllocator, &lib->cache)) {
        fu_error("Failed to create pipeline cache\n");
    }
    app->pipeline.library = fu_steal_pointer(&lib);
    fu_free(block);
}

static bool t_app_pipeline_library_init_fragment_stage(TApp* app)
{
    VkGraphicsPipelineLibraryCreateInfoEXT libraryInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_LIBRARY_CREATE_INFO_EXT,
        .flags = VK_GRAPHICS_PIPELINE_LIBRARY_FRAGMENT_OUTPUT_INTERFACE_BIT_EXT
    };

    // multisample state
    VkPipelineMultisampleStateCreateInfo multisampleState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
    };

    // color blend state
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };
    VkPipelineColorBlendStateCreateInfo colorBlendState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment
    };

    VkGraphicsPipelineCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &libraryInfo,
        .flags = VK_PIPELINE_CREATE_LIBRARY_BIT_KHR | VK_PIPELINE_CREATE_RETAIN_LINK_TIME_OPTIMIZATION_INFO_BIT_EXT,
        .pMultisampleState = &multisampleState,
        .pColorBlendState = &colorBlendState,
        .layout = T_PIPELINE_LAYOUT(app),
        .renderPass = T_RENDER_PASS(app)
    };

    if (FU_UNLIKELY(vkCreateGraphicsPipelines(T_DEVICE(app), T_PIPELINE_CACHE(app), 1, &createInfo, &defCustomAllocator, &app->pipeline.library->fragmentOutputInterface))) {
        fu_error("Failed to create graphics pipeline @fragmentOutputStage\n");
        return false;
    }
    return true;
}

static bool t_app_pipeline_library_init_shader_stage(TApp* app)
{
    const uint32_t shaderVertCode[] = {
        0x07230203, 0x00010600, 0x0008000b, 0x00000035, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
        0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
        0x000c000f, 0x00000000, 0x00000004, 0x6e69616d, 0x00000000, 0x0000000d, 0x00000013, 0x00000021,
        0x0000002b, 0x0000002d, 0x00000031, 0x00000033, 0x00030003, 0x00000002, 0x000001cc, 0x00040005,
        0x00000004, 0x6e69616d, 0x00000000, 0x00060005, 0x0000000b, 0x505f6c67, 0x65567265, 0x78657472,
        0x00000000, 0x00060006, 0x0000000b, 0x00000000, 0x505f6c67, 0x7469736f, 0x006e6f69, 0x00070006,
        0x0000000b, 0x00000001, 0x505f6c67, 0x746e696f, 0x657a6953, 0x00000000, 0x00070006, 0x0000000b,
        0x00000002, 0x435f6c67, 0x4470696c, 0x61747369, 0x0065636e, 0x00070006, 0x0000000b, 0x00000003,
        0x435f6c67, 0x446c6c75, 0x61747369, 0x0065636e, 0x00030005, 0x0000000d, 0x00000000, 0x00070005,
        0x00000011, 0x66696e55, 0x426d726f, 0x65666675, 0x6a624f72, 0x00746365, 0x00050006, 0x00000011,
        0x00000000, 0x65646f6d, 0x0000006c, 0x00050006, 0x00000011, 0x00000001, 0x77656976, 0x00000000,
        0x00050006, 0x00000011, 0x00000002, 0x6a6f7270, 0x00000000, 0x00030005, 0x00000013, 0x006f6275,
        0x00050005, 0x00000021, 0x6f506e69, 0x69746973, 0x00006e6f, 0x00050005, 0x0000002b, 0x67617266,
        0x6f6c6f43, 0x00000072, 0x00040005, 0x0000002d, 0x6f436e69, 0x00726f6c, 0x00040005, 0x00000031,
        0x67617266, 0x00005655, 0x00040005, 0x00000033, 0x56556e69, 0x00000000, 0x00050048, 0x0000000b,
        0x00000000, 0x0000000b, 0x00000000, 0x00050048, 0x0000000b, 0x00000001, 0x0000000b, 0x00000001,
        0x00050048, 0x0000000b, 0x00000002, 0x0000000b, 0x00000003, 0x00050048, 0x0000000b, 0x00000003,
        0x0000000b, 0x00000004, 0x00030047, 0x0000000b, 0x00000002, 0x00040048, 0x00000011, 0x00000000,
        0x00000005, 0x00050048, 0x00000011, 0x00000000, 0x00000023, 0x00000000, 0x00050048, 0x00000011,
        0x00000000, 0x00000007, 0x00000010, 0x00040048, 0x00000011, 0x00000001, 0x00000005, 0x00050048,
        0x00000011, 0x00000001, 0x00000023, 0x00000040, 0x00050048, 0x00000011, 0x00000001, 0x00000007,
        0x00000010, 0x00040048, 0x00000011, 0x00000002, 0x00000005, 0x00050048, 0x00000011, 0x00000002,
        0x00000023, 0x00000080, 0x00050048, 0x00000011, 0x00000002, 0x00000007, 0x00000010, 0x00030047,
        0x00000011, 0x00000002, 0x00040047, 0x00000013, 0x00000022, 0x00000000, 0x00040047, 0x00000013,
        0x00000021, 0x00000000, 0x00040047, 0x00000021, 0x0000001e, 0x00000000, 0x00040047, 0x0000002b,
        0x0000001e, 0x00000000, 0x00040047, 0x0000002d, 0x0000001e, 0x00000001, 0x00040047, 0x00000031,
        0x0000001e, 0x00000001, 0x00040047, 0x00000033, 0x0000001e, 0x00000002, 0x00020013, 0x00000002,
        0x00030021, 0x00000003, 0x00000002, 0x00030016, 0x00000006, 0x00000020, 0x00040017, 0x00000007,
        0x00000006, 0x00000004, 0x00040015, 0x00000008, 0x00000020, 0x00000000, 0x0004002b, 0x00000008,
        0x00000009, 0x00000001, 0x0004001c, 0x0000000a, 0x00000006, 0x00000009, 0x0006001e, 0x0000000b,
        0x00000007, 0x00000006, 0x0000000a, 0x0000000a, 0x00040020, 0x0000000c, 0x00000003, 0x0000000b,
        0x0004003b, 0x0000000c, 0x0000000d, 0x00000003, 0x00040015, 0x0000000e, 0x00000020, 0x00000001,
        0x0004002b, 0x0000000e, 0x0000000f, 0x00000000, 0x00040018, 0x00000010, 0x00000007, 0x00000004,
        0x0005001e, 0x00000011, 0x00000010, 0x00000010, 0x00000010, 0x00040020, 0x00000012, 0x00000002,
        0x00000011, 0x0004003b, 0x00000012, 0x00000013, 0x00000002, 0x0004002b, 0x0000000e, 0x00000014,
        0x00000002, 0x00040020, 0x00000015, 0x00000002, 0x00000010, 0x0004002b, 0x0000000e, 0x00000018,
        0x00000001, 0x00040017, 0x0000001f, 0x00000006, 0x00000003, 0x00040020, 0x00000020, 0x00000001,
        0x0000001f, 0x0004003b, 0x00000020, 0x00000021, 0x00000001, 0x0004002b, 0x00000006, 0x00000023,
        0x3f800000, 0x00040020, 0x00000029, 0x00000003, 0x00000007, 0x0004003b, 0x00000029, 0x0000002b,
        0x00000003, 0x00040020, 0x0000002c, 0x00000001, 0x00000007, 0x0004003b, 0x0000002c, 0x0000002d,
        0x00000001, 0x00040017, 0x0000002f, 0x00000006, 0x00000002, 0x00040020, 0x00000030, 0x00000003,
        0x0000002f, 0x0004003b, 0x00000030, 0x00000031, 0x00000003, 0x00040020, 0x00000032, 0x00000001,
        0x0000002f, 0x0004003b, 0x00000032, 0x00000033, 0x00000001, 0x00050036, 0x00000002, 0x00000004,
        0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x00050041, 0x00000015, 0x00000016, 0x00000013,
        0x00000014, 0x0004003d, 0x00000010, 0x00000017, 0x00000016, 0x00050041, 0x00000015, 0x00000019,
        0x00000013, 0x00000018, 0x0004003d, 0x00000010, 0x0000001a, 0x00000019, 0x00050092, 0x00000010,
        0x0000001b, 0x00000017, 0x0000001a, 0x00050041, 0x00000015, 0x0000001c, 0x00000013, 0x0000000f,
        0x0004003d, 0x00000010, 0x0000001d, 0x0000001c, 0x00050092, 0x00000010, 0x0000001e, 0x0000001b,
        0x0000001d, 0x0004003d, 0x0000001f, 0x00000022, 0x00000021, 0x00050051, 0x00000006, 0x00000024,
        0x00000022, 0x00000000, 0x00050051, 0x00000006, 0x00000025, 0x00000022, 0x00000001, 0x00050051,
        0x00000006, 0x00000026, 0x00000022, 0x00000002, 0x00070050, 0x00000007, 0x00000027, 0x00000024,
        0x00000025, 0x00000026, 0x00000023, 0x00050091, 0x00000007, 0x00000028, 0x0000001e, 0x00000027,
        0x00050041, 0x00000029, 0x0000002a, 0x0000000d, 0x0000000f, 0x0003003e, 0x0000002a, 0x00000028,
        0x0004003d, 0x00000007, 0x0000002e, 0x0000002d, 0x0003003e, 0x0000002b, 0x0000002e, 0x0004003d,
        0x0000002f, 0x00000034, 0x00000033, 0x0003003e, 0x00000031, 0x00000034, 0x000100fd, 0x00010038
    };
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &T_DESC_LAYOUT(app)
    };
    if (FU_UNLIKELY(vkCreatePipelineLayout(T_DEVICE(app), &pipelineLayoutInfo, &defCustomAllocator, &T_PIPELINE_LAYOUT(app)))) {
        fu_error("Failed to create pipeline layout @vertexShaderStage\n");
        return false;
    }

    VkGraphicsPipelineLibraryCreateInfoEXT libraryInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_LIBRARY_CREATE_INFO_EXT,
        .flags = VK_GRAPHICS_PIPELINE_LIBRARY_PRE_RASTERIZATION_SHADERS_BIT_EXT
    };

    // stage
    VkShaderModuleCreateInfo shaderInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = sizeof(shaderVertCode),
        .pCode = shaderVertCode
    };
    VkPipelineShaderStageCreateInfo shaderStageState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = &shaderInfo,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .pName = "main"
    };

    // viewport
    VkPipelineViewportStateCreateInfo viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };

    // rasterizer state
    VkPipelineRasterizationStateCreateInfo rasterizerState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .lineWidth = 1.0f
    };

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = FU_N_ELEMENTS(dynamicStates),
        .pDynamicStates = dynamicStates
    };

    VkGraphicsPipelineCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &libraryInfo,
        .flags = VK_PIPELINE_CREATE_LIBRARY_BIT_KHR | VK_PIPELINE_CREATE_RETAIN_LINK_TIME_OPTIMIZATION_INFO_BIT_EXT,
        .stageCount = 1,
        .pStages = &shaderStageState,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizerState,
        .pDynamicState = &dynamicState,
        .layout = T_PIPELINE_LAYOUT(app),
        .renderPass = T_RENDER_PASS(app)
    };

    if (FU_UNLIKELY(vkCreateGraphicsPipelines(T_DEVICE(app), T_PIPELINE_CACHE(app), 1, &createInfo, &defCustomAllocator, &app->pipeline.library->preRasterizationShaders))) {
        fu_error("Failed to create graphics pipeline @vertexShaderStage\n");
        return false;
    }
    return true;
}

static bool t_app_pipeline_library_init_input_stage(TApp* app)
{
    VkGraphicsPipelineLibraryCreateInfoEXT libraryInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_LIBRARY_CREATE_INFO_EXT,
        .flags = VK_GRAPHICS_PIPELINE_LIBRARY_VERTEX_INPUT_INTERFACE_BIT_EXT
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = t_vertex_get_binding()
    };
    vertexInputInfo.pVertexAttributeDescriptions = t_vertex_get_attributes(&vertexInputInfo.vertexAttributeDescriptionCount);

    VkGraphicsPipelineCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .flags = VK_PIPELINE_CREATE_LIBRARY_BIT_KHR | VK_PIPELINE_CREATE_RETAIN_LINK_TIME_OPTIMIZATION_INFO_BIT_EXT,
        .pNext = &libraryInfo,
        .pInputAssemblyState = &inputAssembly,
        .pVertexInputState = &vertexInputInfo
    };

    if (FU_UNLIKELY(vkCreateGraphicsPipelines(T_DEVICE(app), T_PIPELINE_CACHE(app), 1, &createInfo, &defCustomAllocator, &app->pipeline.library->vertexInputInterface))) {
        fu_error("Failed to create graphics pipeline @vertexInputStage\n");
        fu_free((void*)vertexInputInfo.pVertexBindingDescriptions);
        fu_free((void*)vertexInputInfo.pVertexAttributeDescriptions);
        return false;
    }
    fu_free((void*)vertexInputInfo.pVertexBindingDescriptions);
    fu_free((void*)vertexInputInfo.pVertexAttributeDescriptions);
    return true;
}

static void t_app_destroy_pipeline(TApp* app)
{
    vkDestroyPipeline(T_DEVICE(app), T_PIPELINE(app), &defCustomAllocator);
    vkDestroyPipelineLayout(T_DEVICE(app), T_PIPELINE_LAYOUT(app), &defCustomAllocator);
    vkDestroyRenderPass(T_DEVICE(app), T_RENDER_PASS(app), &defCustomAllocator);
    t_app_pipeline_library_destroy(app);
}

static bool t_app_init_pipeline__render_pass(TApp* app)
{
    VkAttachmentDescription colorAttachment = {
        .format = app->surface.format.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference colorAttachmentRef = {
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentDescription depthAttachment = {
        .format = T_DESC_DEPTH(app).format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference depthAttachmentRef = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkAttachmentDescription attachments[] = { colorAttachment, depthAttachment };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .pDepthStencilAttachment = &depthAttachmentRef
    };

    VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
    };

    VkRenderPassCreateInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = FU_N_ELEMENTS(attachments),
        .pAttachments = attachments,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    if (vkCreateRenderPass(T_DEVICE(app), &renderPassInfo, &defCustomAllocator, &T_RENDER_PASS(app))) {
        fu_error("Failed to create render pass\n");
        return false;
    }
    return true;
}

static bool t_app_init_pipeline__graphics_pipeline(TApp* app)
{
    const uint32_t shaderFragCode[] = {
        0x07230203, 0x00010600, 0x0008000b, 0x00000018, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
        0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
        0x0009000f, 0x00000004, 0x00000004, 0x6e69616d, 0x00000000, 0x00000009, 0x0000000d, 0x00000011,
        0x00000015, 0x00030010, 0x00000004, 0x00000007, 0x00030003, 0x00000002, 0x000001cc, 0x00040005,
        0x00000004, 0x6e69616d, 0x00000000, 0x00050005, 0x00000009, 0x4374756f, 0x726f6c6f, 0x00000000,
        0x00050005, 0x0000000d, 0x53786574, 0x6c706d61, 0x00007265, 0x00040005, 0x00000011, 0x67617266,
        0x00005655, 0x00050005, 0x00000015, 0x67617266, 0x6f6c6f43, 0x00000072, 0x00040047, 0x00000009,
        0x0000001e, 0x00000000, 0x00040047, 0x0000000d, 0x00000022, 0x00000000, 0x00040047, 0x0000000d,
        0x00000021, 0x00000001, 0x00040047, 0x00000011, 0x0000001e, 0x00000001, 0x00040047, 0x00000015,
        0x0000001e, 0x00000000, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00030016,
        0x00000006, 0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000004, 0x00040020, 0x00000008,
        0x00000003, 0x00000007, 0x0004003b, 0x00000008, 0x00000009, 0x00000003, 0x00090019, 0x0000000a,
        0x00000006, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x0003001b,
        0x0000000b, 0x0000000a, 0x00040020, 0x0000000c, 0x00000000, 0x0000000b, 0x0004003b, 0x0000000c,
        0x0000000d, 0x00000000, 0x00040017, 0x0000000f, 0x00000006, 0x00000002, 0x00040020, 0x00000010,
        0x00000001, 0x0000000f, 0x0004003b, 0x00000010, 0x00000011, 0x00000001, 0x00040020, 0x00000014,
        0x00000001, 0x00000007, 0x0004003b, 0x00000014, 0x00000015, 0x00000001, 0x00050036, 0x00000002,
        0x00000004, 0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x0004003d, 0x0000000b, 0x0000000e,
        0x0000000d, 0x0004003d, 0x0000000f, 0x00000012, 0x00000011, 0x00050057, 0x00000007, 0x00000013,
        0x0000000e, 0x00000012, 0x0004003d, 0x00000007, 0x00000016, 0x00000015, 0x00050085, 0x00000007,
        0x00000017, 0x00000013, 0x00000016, 0x0003003e, 0x00000009, 0x00000017, 0x000100fd, 0x00010038
    };

    VkGraphicsPipelineLibraryCreateInfoEXT libraryInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_LIBRARY_CREATE_INFO_EXT,
        .flags = VK_GRAPHICS_PIPELINE_LIBRARY_FRAGMENT_SHADER_BIT_EXT
    };

    VkShaderModuleCreateInfo shaderInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = sizeof(shaderFragCode),
        .pCode = shaderFragCode
    };

    VkPipelineShaderStageCreateInfo shaderStageState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = &shaderInfo,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pName = "main"
    };

    // depth stencil state
    VkPipelineDepthStencilStateCreateInfo depthStencilState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
    };

    VkGraphicsPipelineCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &libraryInfo,
        .flags = VK_PIPELINE_CREATE_LIBRARY_BIT_KHR | VK_PIPELINE_CREATE_RETAIN_LINK_TIME_OPTIMIZATION_INFO_BIT_EXT,
        .stageCount = 1,
        .pStages = &shaderStageState,
        .pDepthStencilState = &depthStencilState,
        .layout = T_PIPELINE_LAYOUT(app),
        .renderPass = T_RENDER_PASS(app)
    };

    if (FU_UNLIKELY(vkCreateGraphicsPipelines(T_DEVICE(app), T_PIPELINE_CACHE(app), 1, &createInfo, &defCustomAllocator, &app->pipeline.library->fragmentShaders))) {
        fu_error("Failed to create graphics pipeline @fragmentShaderStage\n");
        return false;
    }

    VkPipeline libraries[] = {
        app->pipeline.library->vertexInputInterface,
        app->pipeline.library->preRasterizationShaders,
        app->pipeline.library->fragmentOutputInterface,
        app->pipeline.library->fragmentShaders
    };

    VkPipelineLibraryCreateInfoKHR linkingInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR,
        .libraryCount = FU_N_ELEMENTS(libraries),
        .pLibraries = libraries
    };

    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &linkingInfo,
        .pDepthStencilState = &depthStencilState,
        .layout = T_PIPELINE_LAYOUT(app),
        .renderPass = T_RENDER_PASS(app)
    };
    if (FU_UNLIKELY(vkCreateGraphicsPipelines(T_DEVICE(app), T_PIPELINE_CACHE(app), 1, &pipelineInfo, &defCustomAllocator, &T_PIPELINE(app)))) {
        fu_error("Failed to create graphics pipeline @pipeline\n");
        return false;
    }
    return true;
}

static bool t_app_init_pipeline(TApp* app)
{
    t_app_pipeline_library_init(app);
    bool rev = t_app_init_pipeline__render_pass(app);
    rev = rev && t_app_pipeline_library_init_input_stage(app);
    rev = rev && t_app_pipeline_library_init_shader_stage(app);
    rev = rev && t_app_pipeline_library_init_fragment_stage(app);
    rev = rev && t_app_init_pipeline__graphics_pipeline(app);
    return rev;
}
//
//  description

static void t_app_destroy_description(TApp* app)
{
    vkDestroyDescriptorPool(T_DEVICE(app), T_DESC_POOL(app), &defCustomAllocator);
    vkDestroyDescriptorSetLayout(T_DEVICE(app), T_DESC_LAYOUT(app), &defCustomAllocator);
    vkDestroySampler(T_DEVICE(app), T_DESC_SAMPLER(app), &defCustomAllocator);
    for (uint32_t i = 0; i < app->swapchain.imageCount; i++) {
        tvk_uniform_buffer_destroy(&T_DESC_BUFF(app)[i]);
    }
    tvk_image_buffer_destroy(&T_DESC_IMAGE(app));
    tvk_image_buffer_destroy(&T_DESC_DEPTH(app));
    fu_free(T_DESC_SETS(app));
    fu_free(T_DESC_BUFF(app));
}

static bool t_app_init_description__uniform(TApp* app)
{
    T_DESC_BUFF(app) = fu_malloc(sizeof(TUniformBuffer) * app->swapchain.imageCount);
    for (uint32_t i = 0; i < app->swapchain.imageCount; i++) {
        if (!tvk_uniform_buffer_init(sizeof(TUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT, &T_DESC_BUFF(app)[i])) {
            fu_error("Failed to init desc buffer: %u\n", i);
            return false;
        }
    }
    return true;
}

static bool t_app_init_description__depth(TApp* app)
{
    T_DESC_DEPTH(app).extent.width = app->surface.extent.width;
    T_DESC_DEPTH(app).extent.height = app->surface.extent.height;
    return tvk_image_buffer_init(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_IMAGE_ASPECT_DEPTH_BIT, &T_DESC_DEPTH(app));
}

static bool t_app_init_description__texture(TApp* app)
{
    if (t_app_init_description__depth(app)) {
        if (tvk_image_buffer_init_from_path(DEF_IMG, &T_DESC_IMAGE(app)))
            return tvk_sampler_init(VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, &T_DESC_SAMPLER(app));
    }
    return false;
}

static bool t_app_init_description__layout(TApp* app)
{
    VkDescriptorSetLayoutBinding uboLayoutBinding = {
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    };

    VkDescriptorSetLayoutBinding samplerLayoutBinding = {
        .binding = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
    };

    VkDescriptorSetLayoutBinding bindings[] = { uboLayoutBinding, samplerLayoutBinding };
    VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = FU_N_ELEMENTS(bindings),
        .pBindings = bindings,
    };

    if (vkCreateDescriptorSetLayout(T_DEVICE(app), &layoutInfo, &defCustomAllocator, &T_DESC_LAYOUT(app))) {
        fu_error("Failed to create descriptor set layout\n");
        return false;
    }
    return true;
}

static bool t_app_init_description__pool(TApp* app)
{
    VkDescriptorPoolSize sizes[] = {
        { .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = app->swapchain.imageCount },
        { .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = app->swapchain.imageCount }
    };
    VkDescriptorPoolCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = FU_N_ELEMENTS(sizes),
        .pPoolSizes = sizes,
        .maxSets = app->swapchain.imageCount
    };
    if (vkCreateDescriptorPool(T_DEVICE(app), &info, &defCustomAllocator, &T_DESC_POOL(app))) {
        fu_error("Failed to create descriptor pool\n");
        return false;
    }
    return true;
}

static bool t_app_init_description__sets(TApp* app)
{
    VkDescriptorSetLayout* layouts = fu_malloc(sizeof(VkDescriptorSetLayout) * app->swapchain.imageCount);
    VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = T_DESC_POOL(app),
        .descriptorSetCount = app->swapchain.imageCount,
        .pSetLayouts = layouts
    };
    T_DESC_SETS(app) = fu_malloc(sizeof(VkDescriptorSet) * app->swapchain.imageCount);
    for (uint32_t i = 0; i < app->swapchain.imageCount; i++)
        layouts[i] = T_DESC_LAYOUT(app);
    if (vkAllocateDescriptorSets(T_DEVICE(app), &allocInfo, T_DESC_SETS(app))) {
        fu_error("Failed to allocate descriptor sets\n");
        fu_free(layouts);
        return false;
    }

    VkDescriptorBufferInfo bufferInfo = {
        .range = sizeof(TUniformBufferObject)
    };
    VkDescriptorImageInfo imageInfo = {
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .imageView = T_DESC_IMAGE(app).buff.view,
        .sampler = T_DESC_SAMPLER(app)
    };
    VkWriteDescriptorSet write[] = {
        { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &bufferInfo },
        { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstBinding = 1,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &imageInfo }
    };
    for (uint32_t i = 0; i < app->swapchain.imageCount; i++) {
        bufferInfo.buffer = T_DESC_BUFF(app)[i].buffer;
        write[0].dstSet = T_DESC_SETS(app)[i];
        write[1].dstSet = T_DESC_SETS(app)[i];
        vkUpdateDescriptorSets(T_DEVICE(app), FU_N_ELEMENTS(write), write, 0, NULL);
    }
    fu_free(layouts);
    return true;
}

static bool t_app_init_description(TApp* app)
{
    bool rev = t_app_init_description__uniform(app);
    rev = rev && t_app_init_description__texture(app);
    rev = rev && t_app_init_description__layout(app);
    rev = rev && t_app_init_description__pool(app);
    rev = rev && t_app_init_description__sets(app);
    return rev;
}

static void t_app_update_uniform_buffer(TApp* app, uint32_t imageIndex)
{
    TUniformBufferObject ubo = {
        .model = GLM_MAT4_IDENTITY_INIT,
    };

    glm_rotate(ubo.model, SDL_GetTicks() / 100.0f * glm_rad(5.0f), (vec3) { 0.0f, 0.0f, 1.0f });
    glm_lookat((vec3) { 2.0f, 2.0f, 2.0f }, GLM_VEC3_ZERO, (vec3) { 0.0f, 0.0f, 1.0f }, ubo.view);
    glm_perspective(glm_rad(30.0f), app->surface.extent.width / (float)app->surface.extent.height, 0.1f, 10.0f, ubo.proj);
    ubo.proj[1][1] *= -1;
    memcpy(T_DESC_BUFF(app)[imageIndex].allocInfo.pMappedData, &ubo, sizeof(TUniformBufferObject));
}

//
//  swapchain
static void t_app_swapchain_cleanup(TApp* app)
{
    for (uint32_t i = 0; i < app->swapchain.imageCount; i++) {
        vkDestroyImageView(T_DEVICE(app), T_SWAPCHAIN_IMAGES(app)[i].view, &defCustomAllocator);
        vkDestroyFramebuffer(T_DEVICE(app), T_FRAMEBUFFER(app)[i], &defCustomAllocator);
    }
    vkDestroySwapchainKHR(T_DEVICE(app), T_SWAPCHAIN(app), &defCustomAllocator);
    tvk_image_buffer_destroy(&T_DESC_DEPTH(app));
    fu_free(T_SWAPCHAIN_IMAGES(app));
}

static void t_app_destroy_swapchain(TApp* app)
{
    for (uint32_t i = 0; i < app->swapchain.imageCount; i++)
        vkDestroyImageView(T_DEVICE(app), T_SWAPCHAIN_IMAGES(app)[i].view, &defCustomAllocator);
    vkDestroySwapchainKHR(T_DEVICE(app), T_SWAPCHAIN(app), &defCustomAllocator);
    fu_free(T_SWAPCHAIN_IMAGES(app));
}

static bool t_surface_update_detail(TApp* app);
static bool t_app_init_swapchain(TApp* app)
{
    t_surface_update_detail(app);
    if (~0U > app->surface.capabilities.currentExtent.width && ~0U > app->surface.capabilities.currentExtent.height)
        memcpy(&app->surface.extent, &app->surface.capabilities.currentExtent, sizeof(VkExtent2D));
    else {
        int width, height;
        SDL_Vulkan_GetDrawableSize(T_WINDOW(app), &width, &height);
        app->surface.extent.width = glm_clamp(width, app->surface.capabilities.minImageExtent.width, app->surface.capabilities.maxImageExtent.width);
        app->surface.extent.height = glm_clamp(height, app->surface.capabilities.minImageExtent.height, app->surface.capabilities.maxImageExtent.height);
    }
    uint32_t imageCount = app->surface.capabilities.minImageCount + 1;
    if (0 < app->surface.capabilities.maxImageCount && imageCount > app->surface.capabilities.maxImageCount)
        imageCount = app->surface.capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = T_SURFACE(app),
        .minImageCount = imageCount,
        .imageFormat = app->surface.format.format,
        .imageColorSpace = app->surface.format.colorSpace,
        .imageExtent = app->surface.extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = app->surface.capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = app->surface.presentMode,
        .clipped = VK_TRUE
    };
    if (vkCreateSwapchainKHR(T_DEVICE(app), &createInfo, &defCustomAllocator, &T_SWAPCHAIN(app))) {
        fu_error("failed to create swapchain\n");
        return false;
    }

    vkGetSwapchainImagesKHR(T_DEVICE(app), T_SWAPCHAIN(app), &imageCount, NULL);
    VkImage* images = (VkImage*)fu_malloc0_n(imageCount, sizeof(VkImage));
    T_SWAPCHAIN_IMAGES(app) = (TImage*)fu_malloc0_n(imageCount, sizeof(TImage));
    vkGetSwapchainImagesKHR(T_DEVICE(app), T_SWAPCHAIN(app), &imageCount, images);
    for (uint32_t i = 0; i < imageCount; i++) {
        app->swapchain.images[i].image = fu_steal_pointer(&images[i]);
        if (!tvk_image_view_init(T_DEVICE(app), app->surface.format.format, app->swapchain.images[i].image, VK_IMAGE_ASPECT_COLOR_BIT, &T_SWAPCHAIN_IMAGES(app)[i].view)) {
            fu_error("failed to create image view\n");
            fu_free(images);
            return false;
        }
    }
    app->swapchain.imageCount = imageCount;
    fu_free(images);
    return true;
}

static void t_app_swapchain_update(TApp* app)
{
    SDL_Event ev;
    while (SDL_GetWindowFlags(T_WINDOW(app)) & SDL_WINDOW_MINIMIZED) {
        SDL_WaitEvent(&ev);
    }

    vkDeviceWaitIdle(T_DEVICE(app));

    t_app_swapchain_cleanup(app);

    t_app_init_swapchain(app);
    t_app_init_description__depth(app);
    t_app_init_buffers__framebuffers(app);
}
//
//  device

static void t_physical_device_free_queue_family(TPhysicalDevice* dev)
{
    if (!dev)
        return;
    fu_free(dev->queueFamilies);
    fu_free(dev->graphicsQueueFamilies);
    fu_free(dev->computeQueueFamilies);
    fu_free(dev->transferQueueFamilies);
}

static void t_physical_device_destroy(TPhysicalDevice* device)
{
    if (!device)
        return;
    t_physical_device_free_queue_family(device);
    fu_free(device->extensions);
}

static void t_physical_device_free(TPhysicalDevice* devices, uint32_t count)
{
    if (!(devices && count))
        return;
    for (uint32_t i = 0; i < count; i++) {
        t_physical_device_free_queue_family(&devices[i]);
    }
    fu_free(devices);
}

static void t_app_destroy_device(TApp* app)
{
    tvk_uniform_buffer_allocator_destroy();
    vkDestroyDevice(T_DEVICE(app), &defCustomAllocator);
    t_physical_device_destroy(&app->device.physicalDevice);
}

/** 枚举队列族 */
static bool t_physical_device_enumerate_queue_family(TPhysicalDevice* dev, VkSurfaceKHR surface)
{
    vkGetPhysicalDeviceQueueFamilyProperties(dev->physicalDevice, &dev->queueFamilyCount, NULL);
    if (!dev->queueFamilyCount) {
        fu_error("No queue family found\n");
        return false;
    }
    dev->queueFamilies = (TQueueFamilyProperties*)fu_malloc0_n(dev->queueFamilyCount, sizeof(TQueueFamilyProperties));
    dev->graphicsQueueFamilies = (TQueueFamilyProperties*)fu_malloc0_n(dev->queueFamilyCount, sizeof(TQueueFamilyProperties));
    dev->computeQueueFamilies = (TQueueFamilyProperties*)fu_malloc0_n(dev->queueFamilyCount, sizeof(TQueueFamilyProperties));
    dev->transferQueueFamilies = (TQueueFamilyProperties*)fu_malloc0_n(dev->queueFamilyCount, sizeof(TQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(dev->physicalDevice, &dev->queueFamilyCount, &dev->queueFamilies->properties);
    VkBool32 presentSupported;
    for (uint32_t i = 0; i < dev->queueFamilyCount; i++) {
        if (dev->queueFamilies[i].properties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            dev->graphicsQueueFamilies[dev->graphicsQueueFamilyCount++] = dev->queueFamilies[i];
        if (dev->queueFamilies[i].properties.queueFlags & VK_QUEUE_COMPUTE_BIT)
            dev->computeQueueFamilies[dev->computeQueueFamilyCount++] = dev->queueFamilies[i];
        if (dev->queueFamilies[i].properties.queueFlags & VK_QUEUE_TRANSFER_BIT)
            dev->transferQueueFamilies[dev->transferQueueFamilyCount++] = dev->queueFamilies[i];
        vkGetPhysicalDeviceSurfaceSupportKHR(dev->physicalDevice, i, surface, &presentSupported);
        dev->queueFamilies[i].presentSupported = presentSupported;
        dev->queueFamilies[i].index = i;
    }

    return true;
}

/** 枚举物理设备以及其队列族 */
static TPhysicalDevice* t_physical_device_enumerate(VkInstance instance, VkSurfaceKHR surface, uint32_t* count)
{
    vkEnumeratePhysicalDevices(instance, count, NULL);
    if (!*count) {
        fu_error("No GPU with Vulkan support found\n");
        return NULL;
    }
    VkPhysicalDevice* devices = (VkPhysicalDevice*)fu_malloc0_n(*count, sizeof(VkPhysicalDevice));
    TPhysicalDevice* rev = (TPhysicalDevice*)fu_malloc0_n(*count, sizeof(TPhysicalDevice));
    vkEnumeratePhysicalDevices(instance, count, devices);
    for (uint32_t i = 0; i < *count; i++) {
        rev[i].physicalDevice = fu_steal_pointer(&devices[i]);
        vkGetPhysicalDeviceFeatures(rev[i].physicalDevice, &rev[i].features);
        vkGetPhysicalDeviceProperties(rev[i].physicalDevice, &rev[i].properties);
        if (t_physical_device_enumerate_queue_family(&rev[i], surface))
            continue;
        fu_error("failed to enumerate queue family\n");
        fu_free(devices);
        fu_free(rev);
        return NULL;
    }
    fu_free(devices);
    return rev;
}

/** 物理设备打分 */
TPhysicalDevice* t_physical_device_rate(TPhysicalDevice* devs, uint32_t count)
{
    if (!(devs && count))
        return NULL;
    VkBool32* fp;
    uint32_t currScore, bestScore = 0, bestDevice = 0, i, j;
    const uint32_t featureCount = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);
    for (i = 0; i < count; i++) {
        if (!(devs[i].graphicsQueueFamilyCount && devs[i].queueFamilies[0].presentSupported))
            continue;
        fp = (VkBool32*)&devs[i].features;
        if (VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU == devs[i].properties.deviceType)
            currScore = 1000;
        else
            currScore = 0;
        for (j = 0; j < featureCount; j++) {
            if (fp[j])
                currScore += 1;
        }
        currScore *= 100;
        currScore += devs[i].properties.limits.maxBoundDescriptorSets;
        currScore += devs[i].properties.limits.maxComputeSharedMemorySize;
        currScore += devs[i].properties.limits.maxImageArrayLayers;
        currScore += devs[i].properties.limits.maxImageDimension2D;
        currScore += devs[i].properties.limits.maxImageDimensionCube;

        if (currScore > bestScore) {
            bestScore = currScore;
            bestDevice = i;
        }
    }
    return &devs[bestDevice];
}

static int t_physical_device_select_queue(TPhysicalDevice* dev)
{
    TQueueFamilyProperties* queueFamilyProperties;
    for (uint32_t i = 0; i < dev->queueFamilyCount; i++) {
        queueFamilyProperties = &dev->queueFamilies[i];
        if (!queueFamilyProperties->presentSupported)
            continue;
        if (!(queueFamilyProperties->properties.queueFlags & VK_QUEUE_GRAPHICS_BIT))
            continue;
        if (!(queueFamilyProperties->properties.queueFlags & VK_QUEUE_COMPUTE_BIT))
            continue;
        return i;
    }
    return -1;
}

/** 枚举并检查物理设备支持的扩展 */
static int t_physical_device_check_support_extension(TPhysicalDevice* dev, const char* const* const names, uint32_t count)
{
    vkEnumerateDeviceExtensionProperties(dev->physicalDevice, NULL, &dev->extensionCount, NULL);
    if (!dev->extensionCount) {
        fu_error("failed to enumerate device extension properties\n");
        return 0;
    }
    dev->extensions = (VkExtensionProperties*)fu_malloc0_n(dev->extensionCount, sizeof(VkExtensionProperties));
    vkEnumerateDeviceExtensionProperties(dev->physicalDevice, NULL, &dev->extensionCount, dev->extensions);
    for (uint32_t i = 0, j; i < count; i++) {
        for (j = 0; j < dev->extensionCount; j++) {
            if (!strcmp(names[i], dev->extensions[j].extensionName))
                break;
        }
        if (j >= dev->extensionCount)
            return i;
    }
    return -1;
}

/** 物理设备复制构造 */
static void t_physical_device_init_with(TPhysicalDevice* target, TPhysicalDevice* source)
{
    memcpy(target, source, sizeof(TPhysicalDevice));
    target->queueFamilies = (TQueueFamilyProperties*)fu_memdup(source->queueFamilies, sizeof(TQueueFamilyProperties) * source->queueFamilyCount);
    target->graphicsQueueFamilies = (TQueueFamilyProperties*)fu_memdup(source->graphicsQueueFamilies, sizeof(TQueueFamilyProperties) * source->graphicsQueueFamilyCount);
    target->computeQueueFamilies = (TQueueFamilyProperties*)fu_memdup(source->computeQueueFamilies, sizeof(TQueueFamilyProperties) * source->computeQueueFamilyCount);
    target->transferQueueFamilies = (TQueueFamilyProperties*)fu_memdup(source->transferQueueFamilies, sizeof(TQueueFamilyProperties) * source->transferQueueFamilyCount);
}

static bool t_surface_update_detail(TApp* app)
{
    TSurfaceDetail* detail = (TSurfaceDetail*)fu_malloc0(sizeof(TSurfaceDetail));
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(T_PHYSICAL_DEVICE(app), T_SURFACE(app), &detail->capabilities);
    vkGetPhysicalDeviceSurfaceFormatsKHR(T_PHYSICAL_DEVICE(app), T_SURFACE(app), &detail->formatCount, NULL);
    vkGetPhysicalDeviceSurfacePresentModesKHR(T_PHYSICAL_DEVICE(app), T_SURFACE(app), &detail->presentModeCount, NULL);
    if (!(detail->formatCount && detail->presentModeCount)) {
        fu_error("Failed to query swapchain details!\n");
        fu_free(detail);
        return false;
    }
    detail->format = (VkSurfaceFormatKHR*)fu_malloc0_n(detail->formatCount, sizeof(VkSurfaceFormatKHR));
    detail->presentMode = (VkPresentModeKHR*)fu_malloc0_n(detail->presentModeCount, sizeof(VkPresentModeKHR));
    vkGetPhysicalDeviceSurfaceFormatsKHR(T_PHYSICAL_DEVICE(app), T_SURFACE(app), &detail->formatCount, detail->format);
    vkGetPhysicalDeviceSurfacePresentModesKHR(T_PHYSICAL_DEVICE(app), T_SURFACE(app), &detail->presentModeCount, detail->presentMode);

    memcpy(&app->surface.capabilities, &detail->capabilities, sizeof(VkSurfaceCapabilitiesKHR));
    memcpy(&app->surface.format, detail->format, sizeof(VkSurfaceFormatKHR));
    app->surface.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (uint32_t i = 0; i < detail->formatCount; i++) {
        if (VK_COLOR_SPACE_SRGB_NONLINEAR_KHR == detail->format[i].colorSpace && VK_FORMAT_R8G8B8A8_SRGB == detail->format[i].format) {
            memcpy(&app->surface.format, &detail->format[i], sizeof(VkSurfaceFormatKHR));
            break;
        }
    }
    for (uint32_t i = 0; i < detail->presentModeCount; i++) {
        if (VK_PRESENT_MODE_MAILBOX_KHR == detail->presentMode[i]) {
            app->surface.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }
    fu_free(detail->format);
    fu_free(detail->presentMode);
    fu_free(detail);
    return true;
}

static bool t_app_init_device(TApp* app)
{
    const char* const defDeviceExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
        VK_EXT_GRAPHICS_PIPELINE_LIBRARY_EXTENSION_NAME,
        VK_EXT_HOST_IMAGE_COPY_EXTENSION_NAME,
        VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME,
    };
    const char* const defLayers[] = {
        "VK_LAYER_KHRONOS_validation"
    };
    uint32_t deviceCount = 0;
    TPhysicalDevice* devices = t_physical_device_enumerate(app->instance, T_SURFACE(app), &deviceCount);
    if (!(devices && deviceCount))
        return false;
    bool rev = false;
    TPhysicalDevice* selectedDevice = t_physical_device_rate(devices, deviceCount);
    int queueIndex = t_physical_device_select_queue(selectedDevice);
    if (0 > queueIndex) {
        fu_error("Failed to find suitable queue family\n");
        goto out;
    }
    int i = t_physical_device_check_support_extension(selectedDevice, defDeviceExtensions, FU_N_ELEMENTS(defDeviceExtensions));
    if (0 <= i) {
        fu_error("Device does not support extension: %s\n", defDeviceExtensions[i]);
        goto out;
    }
    const float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queueIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    };

    VkPhysicalDeviceMemoryPriorityFeaturesEXT memoryPriorityFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PRIORITY_FEATURES_EXT,
        .memoryPriority = VK_TRUE
    };

    VkPhysicalDeviceHostImageCopyFeaturesEXT hostImageCopyFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_IMAGE_COPY_FEATURES_EXT,
        .pNext = &memoryPriorityFeatures,
        .hostImageCopy = VK_TRUE
    };

    VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT graphicsPipelineLibraryFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GRAPHICS_PIPELINE_LIBRARY_FEATURES_EXT,
        .pNext = &hostImageCopyFeatures,
        .graphicsPipelineLibrary = VK_TRUE
    };
    VkPhysicalDeviceFeatures2 deviceFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &graphicsPipelineLibraryFeatures,
        .features = {
            .robustBufferAccess = VK_TRUE,
            .samplerAnisotropy = VK_TRUE }
    };
    VkDeviceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &deviceFeatures,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .enabledLayerCount = FU_N_ELEMENTS(defLayers),
        .ppEnabledLayerNames = defLayers,
        .enabledExtensionCount = FU_N_ELEMENTS(defDeviceExtensions),
        .ppEnabledExtensionNames = defDeviceExtensions
    };
    if (vkCreateDevice(selectedDevice->physicalDevice, &createInfo, &defCustomAllocator, &T_DEVICE(app))) {
        fu_error("Failed to create logical device\n");
        goto out;
    }

    vkGetDeviceQueue(T_DEVICE(app), queueIndex, 0, &T_QUEUE(app));
    t_physical_device_init_with(&app->device.physicalDevice, selectedDevice);
    tvk_uniform_buffer_allocator_init(app->instance, T_DEVICE(app), T_PHYSICAL_DEVICE(app));

    app->device.queueFamilyIndex = queueIndex;
    rev = true;
out:
    t_physical_device_free(devices, deviceCount);
    return rev;
}

//
//  surface
static void t_app_destroy_surface(TApp* app)
{
    vkDestroySurfaceKHR(app->instance, app->surface.surface, NULL);
    SDL_DestroyWindow(T_WINDOW(app));
    SDL_Quit();
}

static bool t_app_init_surface(TApp* app)
{
    if (SDL_Init(SDL_INIT_VIDEO)) {
        fu_error("Failed to init SDL: %s\n", SDL_GetError());
        return false;
    }
    SDL_Vulkan_LoadLibrary(NULL);
    T_WINDOW(app) = SDL_CreateWindow("Vulkan Test Triangle", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, DEF_WIN_WIDTH, DEF_WIN_HEIGHT, SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!T_WINDOW(app)) {
        fu_error("failed to create window\n");
        return false;
    }

    if (!SDL_Vulkan_CreateSurface(T_WINDOW(app), app->instance, &T_SURFACE(app))) {
        fu_error("Failed to create window surface: %s\n", SDL_GetError());
        return false;
    }

    SDL_SetWindowData(T_WINDOW(app), "usd", app);
    return true;
}

//
//  instance
static VKAPI_ATTR VkBool32 VKAPI_CALL _tvk_debug_cb(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* cbd, void* usd)
{
    static char buff[120];
    memset(buff, 0, sizeof(buff));
    if (VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT & type)
        sprintf(buff, "%s[General]", buff);
    if (VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT & type)
        sprintf(buff, "%s[Validation]", buff);
    if (VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT & type)
        sprintf(buff, "%s[Performance]", buff);
    if (VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT & type)
        sprintf(buff, "%s[Device Address Binding]", buff);
    if (VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT == severity) {
        fu_error("%s%s\n", buff, cbd->pMessage);
        return VK_FALSE;
    }
    if (VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT == severity) {
        fu_warning("%s%s\n", buff, cbd->pMessage);
        return VK_FALSE;
    }
    if (VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT == severity) {
        fu_info("%s%s\n", buff, cbd->pMessage);
        return VK_FALSE;
    }
    fprintf(stdout, "%s%s\n", buff, cbd->pMessage);
    return VK_FALSE;
}

static void t_app_destroy_instance(TApp* app)
{
    vkDestroyDebugUtilsMessengerEXT(app->instance, app->debugger, &defCustomAllocator);
    vkDestroyInstance(app->instance, &defCustomAllocator);
}

static bool t_app_init_instance(TApp* app)
{
    const char* const defInstanceExtensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        VK_EXT_LAYER_SETTINGS_EXTENSION_NAME,
#ifdef VK_USE_PLATFORM_WIN32_KHR
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif defined(VK_USE_PLATFORM_XCB_KHR)
        VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
        VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
#endif
    };
    const char* const defLayers[] = {
        "VK_LAYER_KHRONOS_validation"
    };
    const VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Vulkan Test Triangle",
        .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
        .pEngineName = "Vulkan Test Engine",
        .engineVersion = VK_MAKE_VERSION(0, 0, 1),
        .apiVersion = VK_API_VERSION_1_3
    };
    const VkBool32 setting_validate_best_practices = VK_TRUE;
    const VkLayerSettingEXT layerSettings[] = {
        { "VK_LAYER_KHRONOS_validation", "validate_best_practices", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &setting_validate_best_practices }
    };

    const VkLayerSettingsCreateInfoEXT layerInfo = {
        .sType = VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT,
        .settingCount = 1,
        .pSettings = layerSettings
    };

    const VkDebugUtilsMessengerCreateInfoEXT debuggerInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = &layerInfo,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = _tvk_debug_cb
    };

    const VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = &debuggerInfo,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = FU_N_ELEMENTS(defLayers),
        .ppEnabledLayerNames = defLayers,
        .enabledExtensionCount = FU_N_ELEMENTS(defInstanceExtensions),
        .ppEnabledExtensionNames = defInstanceExtensions
    };

    if (vkCreateInstance(&createInfo, &defCustomAllocator, &app->instance)) {
        fu_error("failed to create vulkan instance");
        return false;
    }

    tvk_fns_init_instance(app->instance);
    if (vkCreateDebugUtilsMessengerEXT(app->instance, &debuggerInfo, &defCustomAllocator, &app->debugger)) {
        fu_warning("failed to create vulkan debug messenger");
    }
    return true;
}

static void t_app_free(TApp* app)
{
    t_app_destroy_sync(app);
    t_app_destroy_buffers(app);
    t_app_destroy_pipeline(app);
    t_app_destroy_description(app);
    t_app_destroy_swapchain(app);
    t_app_destroy_device(app);
    t_app_destroy_surface(app);
    t_app_destroy_instance(app);
    fu_free(app);
};

static TApp* t_app_new()
{
    TApp* app = (TApp*)fu_malloc0(sizeof(TApp));
    if (!t_app_init_instance(app))
        goto out;
    if (!t_app_init_surface(app))
        goto out;
    if (!t_app_init_device(app))
        goto out;
    if (!t_app_init_swapchain(app))
        goto out;
    if (!t_app_init_description(app))
        goto out;
    if (!t_app_init_pipeline(app))
        goto out;
    if (!t_app_init_buffers(app))
        goto out;
    if (!t_app_init_sync(app))
        goto out;
    return app;
out:
    t_app_free(app);
    return NULL;
};

static void t_app_draw(TApp* app)
{
    static uint32_t imageIndex, frameIndex = 0;
    vkWaitForFences(T_DEVICE(app), 1, &app->sync.inFlight[frameIndex], VK_TRUE, ~0U);
    VkResult rev = vkAcquireNextImageKHR(T_DEVICE(app), T_SWAPCHAIN(app), ~0U, app->sync.imageAvailable[frameIndex], VK_NULL_HANDLE, &imageIndex);
    if (rev) {
        if (VK_ERROR_OUT_OF_DATE_KHR == rev) {
            app->swapchain.outOfDate = true;
            return;
        }
        if (VK_SUBOPTIMAL_KHR != rev) {
            fu_error("Failed to acquire swapchain image\n");
            return;
        }
        fu_warning("swapchain suboptiomal\n");
    }

    VkCommandBuffer cmdf = T_CMD_BUFF(app)[frameIndex];

    vkResetFences(T_DEVICE(app), 1, &app->sync.inFlight[frameIndex]);
    vkResetCommandBuffer(cmdf, 0);
    t_app_update_uniform_buffer(app, imageIndex);
    if (!t_command_buffer_recording(app, cmdf, imageIndex)) {
        fu_error("Failed to record command buffer\n");
        return;
    }

    VkSemaphore waitSemaphores[] = { app->sync.imageAvailable[frameIndex] };
    VkSemaphore signalSemaphores[] = { app->sync.renderFinished[frameIndex] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmdf,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signalSemaphores
    };
    if (vkQueueSubmit(T_QUEUE(app), 1, &submitInfo, app->sync.inFlight[frameIndex])) {
        fu_error("Failed to submit command buffer\n");
        return;
    }
    VkSwapchainKHR swapchains[] = { T_SWAPCHAIN(app) };
    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemaphores,
        .swapchainCount = 1,
        .pSwapchains = swapchains,
        .pImageIndices = &imageIndex
    };
    rev = vkQueuePresentKHR(T_QUEUE(app), &presentInfo);
    if (rev) {
        if (VK_ERROR_OUT_OF_DATE_KHR == rev || VK_SUBOPTIMAL_KHR == rev)
            app->swapchain.outOfDate = true;
        else {
            fu_error("Failed to present command buffer\n");
        }
    }
    frameIndex = (frameIndex + 1) % app->swapchain.imageCount;
}

static bool t_app_check_and_exchange(TApp* app, const bool next)
{
    const bool prev = app->swapchain.outOfDate;
    if (prev != next)
        app->swapchain.outOfDate = next;
    return prev;
}

static void t_app_run(TApp* app)
{
    SDL_Event ev;
    bool running = true;
    do {
        while (SDL_PollEvent(&ev)) {
            if (SDL_QUIT == ev.type) {
                running = false;
                break;
            }
            if (SDL_WINDOWEVENT == ev.type) {
                if (SDL_WINDOWEVENT_CLOSE == ev.window.event) {
                    running = false;
                    break;
                }
                if (SDL_WINDOWEVENT_RESIZED == ev.window.event || SDL_WINDOWEVENT_RESTORED == ev.window.event) {
                    app->swapchain.outOfDate = true;
                }
            }
        }
        t_app_draw(app);
        if (t_app_check_and_exchange(app, false))
            t_app_swapchain_update(app);
    } while (running);

    vkDeviceWaitIdle(T_DEVICE(app));
}

int main(void)
{
    TApp* app = t_app_new();
    if (!app)
        return 1;
    t_app_run(app);
    t_app_free(app);
    printf("bye");
    return 0;
}